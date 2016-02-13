/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		skate3													**
**																			**
**	Module:			Xbox specific PRE module 								**
**																			**
**	File name:		p_pre.cpp												**
**																			**
**	Created by:		dc														**
**																			**
**	Description:						 									**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/HashTable.h>
#include <core/StringHashTable.h>
#include <sys/File/PRE.h>
#include <sys/file/filesys.h>
#include <sys/config/config.h>
#include <gel/music/music.h>

// script stuff
#include <gel/scripting/struct.h> 
#include <gel/scripting/symboltable.h>


/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

#define DEBUG_PRE 0

namespace File
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define CURRENT_PRE_VERSION	0xabcd0002			// as of 3/14/2001
//#define CURRENT_PRE_VERSION	0xabcd0001		// until 3/14/2001

#define RINGBUFFERSIZE		 4096	/* N size of ring buffer */	
#define MATCHLIMIT		   18	/* F upper limit for match_length */
#define THRESHOLD	2   /* encode string into position and length */

#define WriteOut(x) 	{*pOut++ = x;}

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

PreMgr *PreMgr::sp_mgr = NULL;
bool    PreMgr::s_lastExecuteSuccess = false; 

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/



#define USE_BUFFER	1		 // we don't need no stinking buffer!!!!
 
#if USE_BUFFER
unsigned char	text_buf[RINGBUFFERSIZE + MATCHLIMIT - 1];	
#endif


#define	ReadInto(x)		if (!Len) break; Len--; x = *pIn++ 
#define	ReadInto2(x)	Len--; x = *pIn++ 	  // version that knows Len is Ok


// Decode an LZSS encoded stream
// Runs at approx 12MB/s on PS2	 without scratchpad (which slows it down in C)
// a 32x CD would run at 4.8MB/sec, although we seem to get a lot less than this
// with our current file system, more like 600K per seconds.....
// Need to write a fast streaming file system....

void DecodeLZSS( unsigned char *pIn, unsigned char *pOut, int Len )	/* Just the reverse of Encode(). */
{
	int  i, j, k, r, c;
//	uint64	LongWord;
//	int bytes = 0;
//	unsigned char *pScratch;
	unsigned int  flags;



//	int basetime =  (int) Tmr::ElapsedTime(0);
//	int len = Len;

//	int	OutBytes = 4;
//	int	OutWord = 0;

#	if USE_BUFFER
	for (i = 0; i < RINGBUFFERSIZE - MATCHLIMIT; i++)
		 text_buf[i] = ' ';
	r = RINGBUFFERSIZE - MATCHLIMIT;
#	else
	r = RINGBUFFERSIZE - MATCHLIMIT;
#	endif
	flags = 0;
	for ( ; ; )
	{
		if (((flags >>= 1) & 256) == 0)
		{
			ReadInto(c);
			flags = c | 0xff00;			/* uses higher byte cleverly */
		}										/* to count eight */
		if (flags & 1)
		{
			ReadInto(c);
			//			putc(c, outfile);
			WriteOut(c);
			#if USE_BUFFER
			text_buf[r++] = c;
			r &= (RINGBUFFERSIZE - 1);
			#else
			r++;
//			r &= (RINGBUFFERSIZE - 1);	  // don't need to wrap r until it is used
			#endif
		}
		else
		{
			ReadInto(i);			
			ReadInto2(j);			// note, don't need to check len on this one.... 
			
			i |= ((j & 0xf0) << 4);						// i is 12 bit offset
			
			#if !USE_BUFFER
			j = (j & 0x0f) + THRESHOLD+1;				// j is 4 bit length (above the threshold)
			unsigned char *pStream;
			r &= (RINGBUFFERSIZE - 1);					// wrap r around before it is used
			pStream = pOut - r;					  		// get base of block
			if (i>=r)										// if offset > r, then
				pStream -= RINGBUFFERSIZE;				// it's the previous block
			pStream += i;									// add in the offset to the base
			r+=j;												// add size to r
			while (j--)										// copy j bytes
				WriteOut(*pStream++);
			#else
			
			j = (j & 0x0f) + THRESHOLD;				// j is 4 bit length (above the threshold)
			for (k = 0; k <= j; k++)					// just copy the bytes
			{
				c =  text_buf[(i+k) & (RINGBUFFERSIZE - 1)]; 
				WriteOut(c);
				text_buf[r++] = c;
				r &= (RINGBUFFERSIZE - 1);
			}
			#endif
		}
	}
//	int Time = (int) Tmr::ElapsedTime(basetime);
//	if (Time > 5)
//	{
//		printf("decomp time is %d ms, for %d bytes,  %d bytes/second\n", Time,len, len * 1000 /Time );
//	}

}




void PreFile::s_delete_file(_File *pFile, void *pData)
{
	Dbg_Assert(pFile);
	delete pFile;
}




PreFile::PreFile(uint8 *p_file_buffer)
{
	mp_table = new Lst::StringHashTable<_File>(4);	

	Dbg_AssertPtr(p_file_buffer);

	mp_buffer = p_file_buffer;
	uint version = *((int *) (mp_buffer + 4));
	Dbg_MsgAssert(version == CURRENT_PRE_VERSION,( "PRE file version not current"));
	m_numEntries = *((int *)(mp_buffer + 8));

	uint8 *pEntry = mp_buffer + 12;
	for (int i = 0; i < m_numEntries; i++)
	{
		int data_size 				= *((int *) pEntry);
		int compressed_data_size 	= *((int *) (pEntry + 4));
		int text_size 				= *((int *) (pEntry + 8));
		int actual_data_size = (compressed_data_size != 0) ? compressed_data_size : data_size;
			
		char *pName = (char *) pEntry + 12;
		uint8 *pCompressedData = pEntry + 12 + text_size;
		
		_File *pFile = new (Mem::Manager::sHandle().TopDownHeap()) _File;

		if (!mp_table->GetItem(pName)) 
		{
			// file is not in table, safe to add
			mp_table->PutItem(pName, pFile);

		pFile->compressedDataSize = compressed_data_size;
		pFile->pCompressedData = pCompressedData; 
		pFile->pData = NULL;
		pFile->sky.POS = 0;
		pFile->sky.SOF = data_size;
		}
		else
			// Somehow, file is already in table, just kill it
			// Later, I'll want to add an assert
			delete pFile;
		
#		if DEBUG_PRE
		printf("   %s, size %d\n", pName, data_size);
#		endif
		
		pEntry += 12 + text_size + ((actual_data_size + 3) & (~3));
	}

#	if DEBUG_PRE
	printf("Done loading PRE\n");
#	endif
	
	mp_activeFile = NULL;
}



PreFile::~PreFile()
{
	delete mp_buffer;
	mp_table->HandleCallback(s_delete_file, NULL);
	mp_table->FlushAllItems();

	delete mp_table;
}



bool PreFile::FileExists(const char *pName)
{
	
	_File *pFile = mp_table->GetItem(pName, false);
	return (pFile != NULL);
}



// returns handle pointer
void *PreFile::GetContainedFile(const char *pName)
{

	_File *pFile = mp_table->GetItem(pName, false);
	if (!pFile) 
		return NULL;
	
	dumbSkyFile *pHandle = &pFile->sky;
	// kinda roundabout, but sets mp_activeFile
	GetContainedFileByHandle(pHandle);
	
	// do we need to fetch file data?
	if (!mp_activeFile->pData)
	{
		if (mp_activeFile->compressedDataSize)
		{
		mp_activeFile->pData = new (Mem::Manager::sHandle().TopDownHeap()) uint8[mp_activeFile->sky.SOF];		
			// need to uncompress data
			DecodeLZSS(mp_activeFile->pCompressedData, mp_activeFile->pData, mp_activeFile->compressedDataSize);	
		}
	}

	return pHandle;
}
	


uint8 *PreFile::GetContainedFileByHandle(void *pHandle)
{
	mp_table->IterateStart();
	_File *pFile = mp_table->IterateNext();
	while(pFile)
	{
		uint8 *pCompressedData = pFile->pCompressedData;
		if (pCompressedData && &pFile->sky == pHandle)
		{
			mp_activeFile = pFile;
			
			if (mp_activeFile->compressedDataSize)
			return mp_activeFile->pData;
			else
				return mp_activeFile->pCompressedData;
		}
		pFile = mp_table->IterateNext();
	}

	return NULL;
}



// allocate memory and load file directly from a pre file, if it is there
// This avoids the problem of having to have the decompressed file in memory twice
// when we are loading directly, like with Pip::Load()
// using this enables us to actually load network game, where there is 1MB less heap during loading
//
// returns a pointer to the file in memory
// or NULL if the file is not in this pre file.
void *PreFile::LoadContainedFile( const char *pName, int *p_size, void *p_dest )
{
	// The dest parameter is used only on Ps2.
	Dbg_Assert( p_dest == NULL );
	
	_File *pFile = mp_table->GetItem( pName, false );
	if( !pFile ) 
	{
		return NULL;
	}	
	
	*p_size = pFile->sky.SOF;
	
	p_dest = Mem::Malloc( pFile->sky.SOF );
	
	// Do we need to decompress file data?
	if( !pFile->pData )
	{
		if( pFile->compressedDataSize )
		{
			// Need to uncompress data.
			DecodeLZSS( pFile->pCompressedData, (uint8*)p_dest, pFile->compressedDataSize );	
		}
		else
		{
			memcpy( p_dest, (void*)pFile->pCompressedData, pFile->sky.SOF );
		}
	}
	else
	{
		memcpy( p_dest, (void*)pFile->pData, pFile->sky.SOF );
	}
	return p_dest;
}



void PreFile::Reset()
{
	
	Dbg_AssertPtr(mp_activeFile);

	mp_activeFile->sky.POS = 0;
}



uint32 PreFile::Read( void *addr, uint32 count )
{
	
	Dbg_AssertPtr( mp_activeFile );

	int seek_offs = mp_activeFile->sky.POS;
	unsigned int limit = mp_activeFile->sky.SOF - seek_offs;
	int copy_number = (count <= limit) ? count : limit; 
	if (mp_activeFile->compressedDataSize)
	{
		Dbg_MsgAssert(mp_activeFile->pData,( "file not uncompressed"));
	memcpy( addr, mp_activeFile->pData + mp_activeFile->sky.POS, copy_number );
	}
	else
	{
		memcpy(addr, mp_activeFile->pCompressedData + mp_activeFile->sky.POS, copy_number);
	}
	
	mp_activeFile->sky.POS += copy_number;

#	if DEBUG_PRE
	printf("PRE: read %d bytes from file, handle 0x%x\n", copy_number, (int) mp_activeFile->pData);
#	endif
	return copy_number;
}



int PreFile::Eof()
{
	
	Dbg_AssertPtr(mp_activeFile);

	if (mp_activeFile->sky.POS >= mp_activeFile->sky.SOF)
	{
#if DEBUG_PRE
		printf("PRE: at end of file\n");
#endif
		return 1;
	}

#if DEBUG_PRE
	printf("PRE: not at end of file\n");
#endif
	return 0;
}



void PreFile::Close()
{

	//Dbg_MsgAssert(mp_activeFile->pData,( "file not uncompressed"));

	if (mp_activeFile->pData)
	delete mp_activeFile->pData;
	mp_activeFile->pData = NULL;
}



int PreFile::Seek(long offset, int origin)
{
	int32 old_pos = mp_activeFile->sky.POS;

	// SEEK_CUR, SEEK_END, SEEK_SET
	switch(origin)
	{
		case SEEK_CUR:
			mp_activeFile->sky.POS += offset;
			break;
		case SEEK_END:
			mp_activeFile->sky.POS = mp_activeFile->sky.SOF - offset;
			break;
		case SEEK_SET:
			mp_activeFile->sky.POS = offset;
			break;
		default:
			return -1;
	}

	if (mp_activeFile->sky.POS < 0 || mp_activeFile->sky.POS > mp_activeFile->sky.SOF)
	{
		mp_activeFile->sky.POS = old_pos;
		return -1;
	}

	return 0;
}



PreMgr::PreMgr() 
{
	mp_table = new Lst::StringHashTable<PreFile>(4);

	sp_mgr = this;


	mp_activeHandle = NULL;
	mp_activeData = NULL;

	mp_activeNonPreHandle = NULL;
}



PreMgr::~PreMgr()
{
	delete mp_table;
}



// Returns handle
// Not frequently called
void *PreMgr::getContainedFile(const char *pName)
{
	
	Dbg_AssertPtr(pName);

	// replace all '/' with '\'
	char cleaned_name[128];
	const char *pCharIn = pName;
	char *pCharOut = cleaned_name;
	while (1)
	{
		*pCharOut = *pCharIn;
		if (*pCharIn == '\0') break;
		if (*pCharOut == '/') *pCharOut = '\\';
		pCharIn++;
		pCharOut++;		
	}

	void *pHandle = NULL;

	mp_table->IterateStart();
	PreFile *pPre = mp_table->IterateNext();
	while(pPre)
	{
			pHandle = pPre->GetContainedFile(cleaned_name);
		if (pHandle) 
		{
			mp_activePre = pPre;
			mp_activeHandle = pHandle;
			mp_activeData = pPre->GetContainedFileByHandle(pHandle);
#			ifdef __PLAT_NGPS__
//			scePrintf("+++ %s is in PRE\n", cleaned_name);
#			endif
			return pHandle;
		}
		pPre = mp_table->IterateNext();
	}

#	ifdef __PLAT_NGPS__
//	scePrintf("--- %s not found in PRE\n", cleaned_name);
#	endif
	return NULL;
}



// returns true if the file exists in any of the pre files
bool	PreMgr::fileExists(const char *pName)
{
	Dbg_AssertPtr(pName);
	// replace all '/' with '\'
	char cleaned_name[128];
	const char *pCharIn = pName;
	char *pCharOut = cleaned_name;
	while (1)
	{
		*pCharOut = *pCharIn;
		if (*pCharIn == '\0') break;
		if (*pCharOut == '/') *pCharOut = '\\';
		pCharIn++;
		pCharOut++;		
	}

	mp_table->IterateStart();
	PreFile *pPre = mp_table->IterateNext();
	while(pPre)
	{
		if (pPre->FileExists(cleaned_name))
		{
			return true;
		}
		pPre = mp_table->IterateNext();
	}
	return false;
}



// returns pointer to data
uint8 *PreMgr::getContainedFileByHandle(void *pHandle)
{
	
	Dbg_AssertPtr(pHandle);

	// if we know that the file in question is not in the PRE system,
	// then it's a regular file, don't waste time looking for it
	if (mp_activeNonPreHandle == pHandle)
		return NULL;
	
	if (mp_activeHandle == pHandle)
		// mp_activePre will be unchanged
		return mp_activeData;
	
	uint8 *pData = NULL;
	mp_table->IterateStart();
	PreFile *pPre = mp_table->IterateNext();
	while(pPre)
	{
			pData = pPre->GetContainedFileByHandle(pHandle);
		if (pData)
		{
			mp_activePre = pPre;
			mp_activeHandle = pHandle;
			mp_activeData = pData;			
			return pData;
		}
		pPre = mp_table->IterateNext();
	}

	// obviously this file is not in the PRE system, mark as such
	mp_activeNonPreHandle = pHandle;
	return NULL;
}

// there's a wrapper around this now, so that we can do
// some memory-context switching
void PreMgr::loadPre(const char *pFilename, bool dont_assert)
{
	m_blockPreLoading = (bool) Script::GetInt("block_pre_loading", false);

	if (m_blockPreLoading)
		return;

	if( Pcm::UsingCD() )
	{
		Dbg_MsgAssert( 0,( "File access forbidden while PCM audio is in progress." ));
		return;
	}

	// Moved this to below the Pcm::UsingCD() call as that is used (bad!!) to turn off
	// music and streams.
#	ifdef __PLAT_NGC__
	OSReport ( "Warning: Attempting to load pre file: %s (ignored)\n", pFilename );
	return;
#	endif // __PLAT_NGC__

#	ifdef __PLAT_NGPS__
//	scePrintf("Loading PRE file %s...\n", pFilename);
#	endif

	char fullname[256];
	sprintf(fullname, "pre\\%s", pFilename);

	// Replace the .pre extension with .prx for Xbox PRE file.
	fullname[strlen( fullname ) - 1] = 'x';

	// Create a lower-case version.
	char lower[128];
	for( unsigned int lp = 0; lp < strlen( fullname ) + 1; lp++ ) lower[lp] = tolower( fullname[lp] );

	Tmr::Time basetime = Tmr::ElapsedTime(0);

	int file_size;
	uint8 *pFile = NULL;

	file_size = CanFileBeLoadedQuickly( fullname );
	if ( file_size )
	{
		pFile = new (Mem::Manager::sHandle().TopDownHeap()) uint8[file_size];
		bool fileLoaded= LoadFileQuicklyPlease( fullname, pFile );
		if ( !fileLoaded )
		{
			printf( "pre file %s failed to load quickly.\n", fullname );
			Dbg_MsgAssert( 0,( "Fire Matt - pre file didn't load quickly." ));
		}
	}
	else
	{
#ifdef __PLAT_NGC__
		NsFile file;
		if ( file.exist( fullname ) )
		{
			file.open( fullname );
#else
			void *fp = File::Open(fullname, "rb");
			if (!fp)
			{
#endif		// __PLAT_NGC__
			// always run the code below if CD build
				if (dont_assert || Config::CD()) 
				{
					printf("couldn't open %s\n", fullname);
					return;
				}
				Dbg_MsgAssert(0,( "couldn't open %s\n", fullname));
			}


#ifdef __PLAT_NGC__
			file.read( &file_size, 4 );
			file_size = nLtEn32(file_size);
#else
			File::Read(&file_size, 4, 1, fp);
			Dbg_MsgAssert(file_size > 0,( "%s has incorrect file size\n", fullname));
#endif		// __PLAT_NGC__
			if (Config::CD())
			{
				if (file_size <= 0) printf("%s has incorrect file size\n", fullname);
			}	


		// Now allocates the .PRE file from the top of the heap, to avoid fragmentation.
			pFile = new (Mem::Manager::sHandle().TopDownHeap()) uint8[file_size];
		//uint8 *pFile = new uint8[file_size];
#ifdef __PLAT_NGC__
			file.read( pFile + 4, file_size );
			file.close();
#else
			File::Read(pFile + 4, 1, file_size, fp);
			File::Close(fp);
#endif		// __PLAT_NGC__
		}

		printf("load time for file size %d is %d ms\n", file_size, (int) Tmr::ElapsedTime(basetime));

	// the PRE file object winds up at the top of the heap, too. This is fine because
	// it will be unloaded at the same time as the big file buffer
		if (!mp_table->PutItem(pFilename, new (Mem::Manager::sHandle().TopDownHeap()) PreFile(pFile)))
			Dbg_MsgAssert(0,( "PRE %s loaded twice", pFilename));
}



/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/


DefineSingletonClass(PreMgr, "PRE Manager");



bool PreMgr::InPre(const char *pFilename)
{


	return mp_table->GetItem( pFilename );
}



void PreMgr::LoadPre( const char *pFilename, bool dont_assert )
{
	// GJ:  This function is a wrapper around loadPRE(), to
	// make sure that all allocations go on the top-down heap
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	loadPre(pFilename, dont_assert);
	Mem::Manager::sHandle().PopContext();	
}



void PreMgr::LoadPrePermanently(const char *pFilename, bool dont_assert)
{
	// GJ:  This function is a wrapper around LoadPRE(),
	// used for loading PRE files which will reside in memory
	// permanently.  Essentially, we need to make sure that the
	// data is above Renderware's resource arena.  If you didn't
	// use this function, you'd get fragmentation when the resource
	// arena resized itself.

	// Mick: Removed reference to renderware, the old "arena" concept is gone

	
	// Load the pre file...
	// This will go on the top-down heap by default
	LoadPre(pFilename, dont_assert);

}



void PreMgr::UnloadPre(const char *pFilename, bool dont_assert)
{

//	scePrintf("Unloading PRE file %s\n", pFilename);
	
	if (m_blockPreLoading)
		return;
	
	PreFile *pThePre = mp_table->GetItem(pFilename);
	if (!pThePre)
	{
		if (dont_assert) return;
		Dbg_MsgAssert(0,( "PRE file %s not in PRE manager", pFilename));
	}

	mp_table->FlushItem(pFilename);
	delete pThePre;
}


bool PreMgr::sPreEnabled()
{
	return sp_mgr != NULL;
}

bool PreMgr::sPreExecuteSuccess()
{
	return s_lastExecuteSuccess; 
}

#	ifndef __PLAT_NGC__
bool PreMgr::pre_fexist(const char *name)
{
	Dbg_MsgAssert(name,( "requesting file NULL"));	
	
	if( sp_mgr->fileExists( name ))
	{
#		if DEBUG_PRE
		printf("PRE: file %s exists\n", name);
#		endif
		s_lastExecuteSuccess = true;
		return true;
	}
	if ( Pcm::UsingCD( ) )
	{
		Dbg_MsgAssert( 0,( "File access forbidden while PCM audio is in progress." ));
		return false;
	}

	return s_lastExecuteSuccess = false;
}



void *PreMgr::pre_fopen(const char *name, const char *access)
{
	Dbg_MsgAssert(name,( "trying to open file NULL" ));	

	char nameConversionBuffer[256];

	// Just as we do in the Xbox version of prefopen(), if this is a .tex file, switch to a .txx file.
	int length = strlen( name );
	strncpy( nameConversionBuffer, name, length + 1 );
	if((( nameConversionBuffer[length - 1] ) == 'x' ) &&
	   (( nameConversionBuffer[length - 2] ) == 'e' ) &&
	   (( nameConversionBuffer[length - 3] ) == 't' ))
	{
		nameConversionBuffer[length - 2] = 'x';
	}

	void *pHandle = sp_mgr->getContainedFile( nameConversionBuffer );
	if( pHandle )
	{
		// if we are going to write the file, we want to use the regular file system
		const char *pChar = access;
		bool am_writing = false;
		while(*pChar)
		{
			if (*pChar != 'r' && *pChar != 'b')
				am_writing = true;
			pChar++;
		}
		
		if (am_writing)
		{
#			ifdef __PLAT_NGPS__
//			scePrintf("    writing file %s\n", name);
#			endif

			// am writing, so we don't need file open in PRE system
			sp_mgr->mp_activePre->Close();
		}
		else
		{
			// we're reading the file from the PRE system
#			if DEBUG_PRE
			printf("PRE: opened file %s, handle is 0x%x\n", name, (int) pHandle);
#			endif
			sp_mgr->mp_activePre->Reset();
			s_lastExecuteSuccess = true;
			return pHandle;
		}
	}

	// if we get here, we are using the regular file system
	if( Pcm::UsingCD())
	{
		Dbg_MsgAssert( 0, ( "File access forbidden while PCM audio is in progress." ));
		return NULL;
	}

	s_lastExecuteSuccess = false;
	return NULL;

	//return pHandle;
}



int PreMgr::pre_fclose(void *fptr)
{
		
	Dbg_MsgAssert(fptr,( "calling fclose with invalid file ptr"));	
	
	uint8 *pData = sp_mgr->getContainedFileByHandle(fptr);
	if (pData)
	{
#if DEBUG_PRE
		printf("PRE: closed file, handle 0x%x\n", (int) fptr);
#endif
		sp_mgr->mp_activePre->Close();
		s_lastExecuteSuccess = true;
		return 0;
	}
	
	s_lastExecuteSuccess = false;
	return 0;
}



size_t PreMgr::pre_fread(void *addr, size_t size, size_t count, void *fptr)
{
		
	Dbg_MsgAssert(fptr,( "calling fread with invalid file ptr"));		

	uint8 *pData = sp_mgr->getContainedFileByHandle( fptr );
	if (pData)
	{
		// read from a simulated file stream in PRE file
		Dbg_AssertPtr(sp_mgr->mp_activePre);

		// There is a subtle bug in the PS2 version, in that the pre version of fread()
		// returns the number of bytes read, rather than the number of items. This
		// version does return the correct value.
		size_t bytes_read = sp_mgr->mp_activePre->Read( addr, size * count );
//		return bytes_read / size;
		return bytes_read;
	}
	if ( Pcm::UsingCD( ) )
	{
		Dbg_MsgAssert( 0,( "File access forbidden while PCM audio is in progress." ));
		return 0;
	}
	
	s_lastExecuteSuccess = false;
	return 0;
}



size_t  PreMgr::pre_fwrite(const void *addr, size_t size, size_t count, void *fptr)
{


	uint8 *pData = sp_mgr->getContainedFileByHandle(fptr);
	Dbg_MsgAssert(!pData,( "can't write to a PRE file"));
	
	if ( Pcm::UsingCD( ) )
	{
		Dbg_MsgAssert( 0,( "File access forbidden while PCM audio is in progress." ));
		return 0;
	}

	s_lastExecuteSuccess = false;
	return 0;
}



char *PreMgr::pre_fgets(char *buffer, int maxLen, void *fptr)
{
		

	uint8 *pData = sp_mgr->getContainedFileByHandle(fptr);
	Dbg_MsgAssert(!pData,( "can't do string ops on a PRE file"));
	
	s_lastExecuteSuccess = false;
	return NULL;
}



int PreMgr::pre_fputs(const char *buffer, void *fptr)
{
		

	uint8 *pData = sp_mgr->getContainedFileByHandle(fptr);
	Dbg_MsgAssert(!pData,( "can't do string ops on a PRE file"));
	
	s_lastExecuteSuccess = false;
	return 0;
}



int PreMgr::pre_feof(void *fptr)
{
		
	Dbg_MsgAssert(fptr,( "calling feof with invalid file ptr"));		

	uint8 *pData = sp_mgr->getContainedFileByHandle(fptr);
	if (pData)
	{
		Dbg_AssertPtr(sp_mgr->mp_activePre);
		s_lastExecuteSuccess = true;
		return sp_mgr->mp_activePre->Eof();
	}
	
	s_lastExecuteSuccess = false;
	return 0;
}



int PreMgr::pre_fseek(void *fptr, long offset, int origin)
{
		

	uint8 *pData = sp_mgr->getContainedFileByHandle(fptr);
	if( pData )
	{
		s_lastExecuteSuccess = true;
		return sp_mgr->mp_activePre->Seek( offset, origin );
	}

	Dbg_MsgAssert( !pData, ( "seek not supported for PRE file" ));
	s_lastExecuteSuccess = false;
	return 0;
}



int PreMgr::pre_fflush(void *fptr)
{


	uint8 *pData = sp_mgr->getContainedFileByHandle(fptr);
	Dbg_MsgAssert(!pData,( "flush not supported for PRE file"));
	
	s_lastExecuteSuccess = false;
	return 0;
}



int PreMgr::pre_ftell(void *fptr)
{


	uint8 *pData = sp_mgr->getContainedFileByHandle(fptr);
	if( pData )
	{
		return sp_mgr->mp_activePre->TellActive();
	}

	Dbg_MsgAssert( !pData, ( "tell supported for PRE file" ));
	
	s_lastExecuteSuccess = false;
	return 0;
}
#	endif		// __PLAT_NGC__



// @script | InPreFile | 
// @uparm "string" | filename
bool ScriptInPreFile(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char *pFilename;
	pParams->GetText(NONAME, &pFilename, true);

	Spt::SingletonPtr<PreMgr> pre_mgr;
	return pre_mgr->InPre(pFilename);
}



// @script | LoadPreFile | 
// @uparm "string" | filename
// @flag dont_assert | 
bool ScriptLoadPreFile(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char *pFilename;
	pParams->GetText(NONAME, &pFilename, true);

	Spt::SingletonPtr<PreMgr> pre_mgr;
	pre_mgr->LoadPre(pFilename, pParams->ContainsFlag("dont_assert"));
	return true;
}



// @script | UnloadPreFile | 
// @flag BoardsPre | 
// @flag dont_assert | 
// @uparm "string" | filename
bool ScriptUnloadPreFile(Script::CStruct *pParams, Script::CScript *pScript)
{


	Spt::SingletonPtr<PreMgr> pre_mgr;
	
	if (pParams->ContainsFlag("BoardsPre"))
	{
		
//		Spt::SingletonPtr< Mdl::Skate >	pSkate;
//		pSkate->UnloadBoardPreIfPresent(pParams->ContainsFlag("dont_assert"));
		printf ("STUBBED:  Unload BoardsPre, in ScriptUnloadPreFile\n");
		return true;
	}
	
	const char *pFilename;
	pParams->GetText(NONAME, &pFilename, true);

	pre_mgr->UnloadPre(pFilename, pParams->ContainsFlag("dont_assert"));
	return true;
}



void *PreMgr::LoadFile( const char *pName, int *p_size, void *p_dest )
{
	// The dest parameter is used only on Ps2.
	Dbg_Assert( p_dest == NULL );

	// NOTE: THIS IS JUST CUT AND PASTE FROM Pre::fileExists
	Dbg_AssertPtr( pName );

	// Replace all '/' with '\'
	char cleaned_name[128];
	const char *pCharIn = pName;
	char *pCharOut = cleaned_name;
	while (1)
	{
		*pCharOut = *pCharIn;
		if (*pCharIn == '\0') break;
		if (*pCharOut == '/') *pCharOut = '\\';
		pCharIn++;
		pCharOut++;		
	}

	mp_table->IterateStart();
	PreFile *pPre = mp_table->IterateNext();
	while(pPre)
	{
		
		void *p_data = pPre->LoadContainedFile(cleaned_name,p_size);
		if (p_data)
		{
			return p_data;
		}
		pPre = mp_table->IterateNext();
	}
	return NULL;

}



} // namespace File




