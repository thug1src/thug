//****************************************************************************
//* MODULE:         File
//* FILENAME:       FileLibrary.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  01/21/2003
//****************************************************************************

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sys/file/filelibrary.h>

#include <sys/file/asyncfilesys.h>
#include <sys/file/filesys.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>

#include <sys/mem/memman.h>

#ifdef __PLAT_NGC__
#include <gel/music/music.h>
#include <sys/ngc/p_aram.h>
#include <sys/ngc/p_dma.h>
#endif		// __PLAT_NGC__

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace File
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

struct SLibHeader
{
	uint32	versionNumber;
	int		numFiles;
};

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void filesync_async_callback(File::CAsyncFileHandle*, File::EAsyncFunctionType function, int result, unsigned int arg0, unsigned int arg1)
{
	// Dbg_Message("Got callback from %x", arg0);
	if (function == File::FUNC_READ)
	{
		CFileLibrary* p_data = (CFileLibrary*)arg0;
		bool assertOnFail = (bool)arg1;

		p_data->PostLoad( assertOnFail, result );
	}
}

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				   
CFileLibrary::CFileLibrary()
{
#ifndef __PLAT_NGC__
	mp_baseBuffer = NULL;
	mp_fileBuffer = NULL;
#endif		// __PLAT_NGC__
	mp_fileHandle = NULL;
	m_dataLoaded = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CFileLibrary::~CFileLibrary()
{
#ifdef __PLAT_NGC__
	NsARAM::free( m_aramOffset );
#else
	if ( mp_baseBuffer )
	{
		Mem::Free( mp_baseBuffer );
		mp_baseBuffer = NULL;
	}
#endif		// __PLAT_NGC__
	
	if ( mp_fileHandle )
	{
		Dbg_MsgAssert( m_dataLoaded, ( "Can't delete CFileLibrary while it is still being loaded" ) );
		File::CAsyncFileLoader::sClose( mp_fileHandle );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				   
bool CFileLibrary::LoadFinished() const
{
	return m_dataLoaded;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CFileLibrary::PostLoad( bool assertOnFail, int file_size )
{
	// Handle end of async, if that was used
	if ( mp_fileHandle )
	{
		File::CAsyncFileLoader::sClose( mp_fileHandle );
		mp_fileHandle = NULL;
	}
	
	Dbg_MsgAssert( (file_size & 0x3) == 0, ( "Size of file is not multiple of 4 (%d bytes)", file_size ) );

#ifdef __PLAT_NGC__
#define XFER_SIZE ( sizeof( SLibHeader ) + ( vMAX_LIB_FILES * sizeof( SFileInfo ) ) )
	char buffer[ XFER_SIZE ];

	NsDMA::toMRAM( buffer, m_aramOffset, XFER_SIZE );

	uint8 *pFileData = (uint8*)buffer; 
#else
	uint8 *pFileData = (uint8*)mp_fileBuffer;
#endif		// __PLAT_NGC__

	SLibHeader* pHeader = (SLibHeader*)pFileData;
	
	pFileData += sizeof( SLibHeader );
#ifndef __PLAT_NGC__
	if ( pFileData > ( mp_fileBuffer + file_size ) )
	{
		// out of bounds
		goto load_fail;
	}
#endif		// __PLAT_NGC__

	m_numFiles = pHeader->numFiles;

#ifndef __PLAT_NGC__
	Dbg_MsgAssert( m_numFiles > 0, ( "No files found in lib mp_fileBuffer = %p", mp_fileBuffer ) );
    Dbg_MsgAssert( m_numFiles < vMAX_LIB_FILES, ( "Too many subfiles found in lib (%d files, max=%d)", m_numFiles, vMAX_LIB_FILES ) );
#endif		// __PLAT_NGC__

	// read in table of contents information
	memcpy( &m_fileInfo, pFileData, (sizeof(SFileInfo) * m_numFiles) );
	pFileData += (sizeof(SFileInfo) * m_numFiles);
#ifndef __PLAT_NGC__
	if ( pFileData > ( mp_fileBuffer + file_size ) )
	{
		// out of bounds
		goto load_fail;
	}
#endif		// __PLAT_NGC__
	
	for ( int i = 0; i < m_numFiles; i++ )
	{
#ifdef __PLAT_NGC__
		m_fileOffsets[i] = (uint32)(m_aramOffset + m_fileInfo[i].fileOffset);
		mp_filePointers[i] = NULL;
#else
		mp_filePointers[i] = (uint32*)(mp_fileBuffer + m_fileInfo[i].fileOffset);
#endif		// __PLAT_NGC__
		Dbg_Message( "File %d @ %08x %s [%d bytes]", i, m_fileInfo[i].fileNameChecksum, Script::FindChecksumName(m_fileInfo[i].fileExtensionChecksum), m_fileInfo[i].fileSize );
	}
	
#ifndef __PLAT_NGC__
	// to reduce the amount of temp memory needed, we first load the LIB file
	// on the bottom up heap, and then copy sub-file individually onto the top
	// down heap (as we reallocate shrink the original LIB file on the bottom up heap)
	for ( int i = m_numFiles - 1; i >= 0; i-- )
	{
		int file_size = m_fileInfo[i].fileSize;
		
		uint32* pTempFilePointer = mp_filePointers[i];
		
		// GJ:  When I first wrote this class, I wanted to make it generic, 
		// but it gradually became more cutscene-specific to fix
		// memory issues.  For example, the following uses the
		// "cutscene heap" to reduce temp memory overhead.  Perhaps
		// later on, I can just store the appropriate top/bottomheap pointers
		// during initialization
		Dbg_MsgAssert( Mem::Manager::sHandle().CutsceneTopDownHeap(), ( "No cutscene heap?" ) ); 
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().CutsceneTopDownHeap());
		mp_filePointers[i] = (uint32*)Mem::Malloc( file_size );
		memcpy( mp_filePointers[i], pTempFilePointer, file_size );
		Mem::Manager::sHandle().PopContext();
		
#ifdef	__NOPT_ASSERT__
		uint8* pOldBuffer = mp_baseBuffer;
#endif
		
		Dbg_MsgAssert( Mem::Manager::sHandle().CutsceneBottomUpHeap(), ( "No cutscene heap?" ) ); 
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().CutsceneBottomUpHeap());
		mp_baseBuffer = (uint8*)Mem::ReallocateShrink( Mem::GetAllocSize(mp_baseBuffer) - file_size, mp_baseBuffer );
		Dbg_MsgAssert( mp_baseBuffer == pOldBuffer, ( "Pointer was not supposed to change except for shrinking" ) );
		Mem::Manager::sHandle().PopContext();
	}

	// by this point, all that should be left in the original block
	// is the header, which we don't need any more
	Mem::Free( mp_baseBuffer );
	mp_baseBuffer = NULL;
	mp_fileBuffer = NULL;
#endif		// __PLAT_NGC__

	// if the data is ready to be accessed
	m_dataLoaded = true;

	return true;

#ifndef __PLAT_NGC__
load_fail:
	Dbg_MsgAssert( 0, ( "Parsing of library failed" ) );
	return false;
#endif		// __PLAT_NGC__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CFileLibrary::Load( const char* p_fileName, bool assertOnFail, bool async_load )
{
#ifndef __PLAT_NGC__
	// See Garrett, Garrett, or Mick for explanation.
	
	Dbg_MsgAssert( !async_load, ( "Cutscenes aren't supposed to be async loaded anymore" ) );
	int file_size;
	
	Dbg_MsgAssert( Mem::Manager::sHandle().CutsceneBottomUpHeap(), ( "No cutscene heap?" ) ); 
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().CutsceneBottomUpHeap());
	
	mp_baseBuffer = (uint8*)LoadAlloc( p_fileName, &file_size );
	mp_fileBuffer = mp_baseBuffer;
	
	Mem::Manager::sHandle().PopContext();
	
	return PostLoad(assertOnFail, file_size);
#endif

#ifdef __PLAT_NGC__
	// Make sure music is off here as we really need the memory.
//	Pcm::StopMusic();

	async_load = false;
#else
	// Turn off async for platforms that don't support it
	if (!File::CAsyncFileLoader::sAsyncSupported())
	{
		async_load = false;
	}
	
	if ( async_load )
	{
		// Pre-load
		Dbg_Assert( !mp_fileHandle );

		mp_fileHandle = File::CAsyncFileLoader::sOpen( p_fileName, !async_load );

		// make sure the file is valid
		if ( !mp_fileHandle )
		{
			Dbg_MsgAssert( assertOnFail, ( "Load of %s failed - file not found?", p_fileName ) );
			return false;
		}

		int file_size = mp_fileHandle->GetFileSize();

		if ( file_size == 0 )
		{
			Dbg_MsgAssert( 0, ("Library file %s size is 0 (possibly not found?)", p_fileName) );
			File::CAsyncFileLoader::sClose( mp_fileHandle );
			mp_fileHandle = NULL;
			return false;
		}

		Dbg_Assert( !mp_fileBuffer );
		Dbg_Assert( !mp_baseBuffer );
		
		// to reduce the amount of temp memory needed, we first load the LIB file
		// on the bottom up heap, and then copy sub-file individually onto the top
		// down heap (as we reallocate shrink the original LIB file on the bottom up heap)
		Dbg_MsgAssert( Mem::Manager::sHandle().CutsceneBottomUpHeap(), ( "No cutscene heap?" ) ); 
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().CutsceneBottomUpHeap());
		mp_baseBuffer = (uint8*) Mem::Malloc( file_size + 64 );
		mp_fileBuffer = (uint8*)(((uint32)mp_baseBuffer+63)&~63);
		mp_fileBuffer = mp_baseBuffer;		// Garrett: De-aligning pointer since async filesystem should take care of it.		
		Mem::Manager::sHandle().PopContext();

		// Set the callback
		mp_fileHandle->SetCallback( filesync_async_callback, (unsigned int)this, (unsigned int)assertOnFail );

		// read the file in
		mp_fileHandle->Read( mp_fileBuffer, 1, file_size );

		return true;
	}
	else
#endif		// __PLAT_NGC__
	{				   
		// open the file as a stream
		void* pStream = File::Open( p_fileName, "rb" );
	
		// make sure the file is valid
		if ( !pStream )
		{
			Dbg_MsgAssert( assertOnFail, ("Load of %s failed - file not found?", p_fileName) );
			return false;
		}

		int file_size = File::GetFileSize( pStream );
		Dbg_MsgAssert( file_size, ( "Library file size is 0" ) );
		
#ifndef __PLAT_NGC__
		Dbg_Assert( !mp_fileBuffer );
		Dbg_Assert( !mp_baseBuffer );
#endif		// __PLAT_NGC__
		
#ifdef __PLAT_NGC__
		// Allocate ARAM
		m_aramOffset = NsARAM::alloc( file_size, NsARAM::TOPDOWN );

#define BUFFER_SIZE ( 16 * 1024 )

		// read the file in blocks & transfer to ARAM
		char buffer[BUFFER_SIZE];
		int size = file_size;
		uint32 offset = m_aramOffset;

		while ( size > BUFFER_SIZE )
		{
			File::Read( buffer, 1, BUFFER_SIZE, pStream );
			NsDMA::toARAM( offset, buffer, BUFFER_SIZE );

			size -= BUFFER_SIZE;
			offset += BUFFER_SIZE;
		}

		// Read & transfer last bit.
		if ( size )
		{
			File::Read( buffer, 1, size, pStream ); 
			NsDMA::toARAM( offset, buffer, size );
		}
//		mp_baseBuffer = (uint8*)Mem::Malloc( file_size );
//		mp_fileBuffer = mp_baseBuffer;
#else
		// to reduce the amount of temp memory needed, we first load the LIB file
		// on the bottom up heap, and then copy sub-file individually onto the top
		// down heap (as we reallocate shrink the original LIB file on the bottom up heap)
		Dbg_MsgAssert( Mem::Manager::sHandle().CutsceneBottomUpHeap(), ( "No cutscene heap?" ) ); 
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().CutsceneBottomUpHeap());
		mp_baseBuffer = (uint8*)Mem::Malloc( file_size );
		mp_fileBuffer = mp_baseBuffer;
		Mem::Manager::sHandle().PopContext();

		// read the file in
		if ( !File::Read( mp_fileBuffer, 1, file_size, pStream ) )
		{
			Mem::Free( mp_fileBuffer );
			mp_baseBuffer = NULL;
			mp_fileBuffer = NULL;
			File::Close( pStream );

			Dbg_MsgAssert( assertOnFail, ( "Load of %s failed - bad format?", p_fileName ) );
			return false;
		}
#endif		// __PLAT_NGC__

		File::Close(pStream);

		return PostLoad(assertOnFail, file_size);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				   
int CFileLibrary::GetNumFiles() const 
{
	return m_numFiles;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				   
uint32* CFileLibrary::GetFileData( int index )
{
	Dbg_MsgAssert( index >= 0 && index < m_numFiles, ( "Out of range file index %d", index ) );

#ifdef __PLAT_NGC__
	if ( !mp_filePointers[index] )
	{
		// Allocate memory & copy over.
//		Dbg_MsgAssert( Mem::Manager::sHandle().CutsceneTopDownHeap(), ( "No cutscene heap?" ) ); 
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().CutsceneTopDownHeap());

		int mem_available;
		bool need_to_pop = false;
		int size = m_fileInfo[index].fileSize;

		Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().AudioHeap() );
		mem_available = Mem::Manager::sHandle().Available();
		if ( size < ( mem_available - ( 15 * 1024 ) ) )
		{
			need_to_pop = true;
		}
		else
		{
			Mem::Manager::sHandle().PopContext();
			Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().FrontEndHeap() );
			mem_available = Mem::Manager::sHandle().Available();
			if ( size < ( mem_available - ( 40 * 1024 ) ) )
			{
				need_to_pop = true;
			}
			else
			{
				Mem::Manager::sHandle().PopContext();
				Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ThemeHeap() );
				mem_available = Mem::Manager::sHandle().Available();
				if ( size < ( mem_available - ( 5 * 1024 ) ) )
				{
					need_to_pop = true;
				}
				else
				{
					Mem::Manager::sHandle().PopContext();
					Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ScriptHeap() );
					mem_available = Mem::Manager::sHandle().Available();
					if ( size < ( mem_available - ( 40 * 1024 ) ) )
					{
						need_to_pop = true;
					}
					else
					{
						Mem::Manager::sHandle().PopContext();
					}
				}
			}
		}

		mp_filePointers[index] = (uint32*)Mem::Malloc( m_fileInfo[index].fileSize );
		NsDMA::toMRAM( mp_filePointers[index], m_fileOffsets[index], m_fileInfo[index].fileSize );

		if ( need_to_pop )
		{
			Mem::Manager::sHandle().PopContext();
		}

		Mem::Manager::sHandle().PopContext();
	}
#endif		// __PLAT_NGC__

	return mp_filePointers[index];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				   
const SFileInfo* CFileLibrary::GetFileInfo( int index ) const
{
	Dbg_MsgAssert( index >= 0 && index < m_numFiles, ( "Out of range file index %d", index ) );

	return &m_fileInfo[index];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				   
const SFileInfo* CFileLibrary::GetFileInfo( uint32 name, uint32 extension, bool assertOnFail ) const
{
	for ( int index = 0; index < m_numFiles; index++ )
	{
		if ( m_fileInfo[index].fileNameChecksum == name && m_fileInfo[index].fileExtensionChecksum == extension )
		{
			return &m_fileInfo[index];
		}
	}

	if ( assertOnFail )
	{
		Dbg_MsgAssert( 0, ( "Couldn't find file %08x %08x\n", name, extension ) );
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				   
uint32* CFileLibrary::GetFileData( uint32 name, uint32 extension, bool assertOnFail )
{
	for ( int index = 0; index < m_numFiles; index++ )
	{
		if ( m_fileInfo[index].fileNameChecksum == name && m_fileInfo[index].fileExtensionChecksum == extension )
		{
			return GetFileData( index );
		}
	}

	if ( assertOnFail )
	{
		Dbg_MsgAssert( 0, ( "Couldn't find file %08x %08x\n", name, extension ) );
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				   
bool CFileLibrary::ClearFile( uint32 name, uint32 extension )
{
	bool success = false;

	for ( int index = 0; index < m_numFiles; index++ )
	{
		if ( m_fileInfo[index].fileNameChecksum == name && m_fileInfo[index].fileExtensionChecksum == extension )
		{
			Mem::Free( mp_filePointers[index] );
			mp_filePointers[index] = NULL;
			success = true;
		}
	}

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				   
bool CFileLibrary::FileExists( uint32 name, uint32 extension )
{
	for ( int index = 0; index < m_numFiles; index++ )
	{
		if ( m_fileInfo[index].fileNameChecksum == name && m_fileInfo[index].fileExtensionChecksum == extension )
		{
			return true;
		}
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				   
} // namespace File
