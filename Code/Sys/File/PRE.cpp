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
**	Module:									 								**
**																			**
**	File name:																**
**																			**
**	Created by:		rjm														**
**																			**
**	Description:						 									**
**																			**
*****************************************************************************/

// start autoduck documentation
// @DOC pre
// @module pre | None
// @subindex Scripting Database
// @index script | pre

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifdef	__PLAT_NGPS__
#include <eekernel.h>
#endif		//	__PLAT_NGPS__
#include <core/defines.h>
//#include <core/HashTable.h>
#include <core/StringHashTable.h>
#include <sys/File/PRE.h>
#include <sys/file/filesys.h>
#include <sys/file/AsyncFilesys.h>
#include <sys/config/config.h>

// cd shared by the music streaming stuff...  ASSERT if file access attempted
// while music is streaming:
#include <gel/music/music.h>

// script stuff
#include <gel/scripting/struct.h> 
#include <gel/scripting/symboltable.h>

#ifdef __PLAT_NGC__
#include "sys/ngc/p_aram.h"
#include "sys/ngc/p_dma.h"
#include "sys/ngc/p_display.h"
#ifdef __NOPT_FINAL__
#define __PRE_ARAM__
#endif		// __NOPT_FINAL__
#endif

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

#define DEBUG_PRE 0

#define	PRE_NAME_OFFSET 16		// was 12, but then I added an extra checksum field

namespace File
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define CURRENT_PRE_VERSION	0xabcd0003			// as of 3/14/2001
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
#ifdef	__PLAT_NGPS__
// ring buffer is just over 4K 4096+17), 
// so fits nicely in the PS2's 16K scratchpad 
unsigned char	text_buf[RINGBUFFERSIZE + MATCHLIMIT - 1];	
// Note:  if we try to use the scratchpad, like this
// then the code actually runs slower
// if we want to optimize this, then it should
// be hand crafted in assembly, using 128bit registers
//	const unsigned char * text_buf = (unsigned char*) 0x70000000;
//	#define text_buf ((unsigned char*) 0x70000000)
#else
unsigned char
		text_buf[RINGBUFFERSIZE + MATCHLIMIT - 1];	/* ring buffer of size N,
			with extra F-1 bytes to facilitate string comparison */
#endif
#endif


#ifdef __PRE_ARAM__
#define	ReadInto(x)		if (!Len) break; Len--; x = get_byte( p_buffer ) 
#define	ReadInto2(x)	Len--; x = get_byte( p_buffer ) 	  // version that knows Len is Ok
#else
#define	ReadInto(x)		if (!Len) break; Len--; x = *pIn++ 
#define	ReadInto2(x)	Len--; x = *pIn++ 	  // version that knows Len is Ok
#endif		// __PRE_ARAM__


// Decode an LZSS encoded stream
// Runs at approx 12MB/s on PS2	 without scratchpad (which slows it down in C)
// a 32x CD would run at 4.8MB/sec, although we seem to get a lot less than this
// with our current file system, more like 600K per seconds.....
// Need to write a fast streaming file system....

#ifdef __PRE_ARAM__
#define INPUT_BUFFER_SIZE (16 * 1024)
static uint32 p_aram_in;
static int aram_len = 0;
static int buffer_bytes = 0;
static int buffer_offset = 0;
static unsigned char get_byte( unsigned char * p_buffer )
{
	if ( buffer_bytes == 0 )
	{
		int toread;
		if ( aram_len >= INPUT_BUFFER_SIZE )
		{
			toread = INPUT_BUFFER_SIZE;
		}
		else
		{
			toread = aram_len;
		}

		NsDMA::toMRAM( p_buffer, p_aram_in, toread );
		p_aram_in += INPUT_BUFFER_SIZE;
		aram_len -= INPUT_BUFFER_SIZE;
		buffer_bytes = toread; 
		buffer_offset = 0;
	}
	unsigned char bb = p_buffer[buffer_offset];
	buffer_bytes--;
	buffer_offset++;
	return bb;
}
#endif		// __PRE_ARAM__

void DecodeLZSS(unsigned char *pIn, unsigned char *pOut, int Len)	/* Just the reverse of Encode(). */
{
	int  i, j, k, r, c;
//	uint64	LongWord;
//	int bytes = 0;
//	unsigned char *pScratch;
	unsigned int  flags;

	// Ensure we have decent values for the decode length
	Dbg_Assert(( Len >= 0 ) && ( Len < 0x2000000 ));

	if(( Len < 0 ) || ( Len >= 0x2000000 ))
	{
		while( 1 );
	}
#ifdef __PRE_ARAM__
	unsigned char p_buffer[INPUT_BUFFER_SIZE];
	p_aram_in = (uint32)pIn;
	aram_len = Len;
#endif		// __PRE_ARAM__


//	int basetime =  (int) Tmr::ElapsedTime(0);
//	int len = Len;

//	int	OutBytes = 4;
//	int	OutWord = 0;

	#if USE_BUFFER
	for (i = 0; i < RINGBUFFERSIZE - MATCHLIMIT; i++)
		 text_buf[i] = ' ';
	r = RINGBUFFERSIZE - MATCHLIMIT;
	#else
	r = RINGBUFFERSIZE - MATCHLIMIT;
	#endif
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

void EndOfDecodeLZSS( void )
{
} 


void PreFile::s_delete_file(_File *pFile, void *pData)
{
	Dbg_Assert(pFile);
	delete pFile;
}




PreFile::PreFile(uint8 *p_file_buffer, bool useBottomUpHeap)
{
	m_use_bottom_up_heap=useBottomUpHeap;
	
	mp_table = new Lst::StringHashTable<_File>(4);	

	Dbg_AssertPtr(p_file_buffer);

	mp_buffer = p_file_buffer;
	#ifdef __NOPT_ASSERT__
#ifdef __PRE_ARAM__
	uint version;
	NsDMA::toMRAM( &version, (uint32)mp_buffer + 4, 4 );
#else
	uint version = 	*((int *) (mp_buffer + 4));
#endif		// __PRE_ARAM__
	Dbg_MsgAssert(version == CURRENT_PRE_VERSION,( "PRE file version (%x) not current (%x)",version,CURRENT_PRE_VERSION));
	#endif
#ifdef __PRE_ARAM__
	NsDMA::toMRAM( &m_numEntries, (uint32)mp_buffer + 8, 4 );
#else
	m_numEntries = *((int *)(mp_buffer + 8));
#endif		// __PRE_ARAM__

	uint8 *pEntry = mp_buffer + 12;
	for (int i = 0; i < m_numEntries; i++)
	{
#ifdef __PRE_ARAM__
		int data_size;
		int compressed_data_size;
		short text_size;
		NsDMA::toMRAM( &data_size, (uint32)pEntry, 4 );
		NsDMA::toMRAM( &compressed_data_size, (uint32)pEntry + 4, 4 );
		NsDMA::toMRAM( &text_size, (uint32)pEntry + 8, 2 );
#else
		int data_size 				= *((int *) pEntry);
		int compressed_data_size 	= *((int *) (pEntry + 4));
		int text_size	 			= *((short *) (pEntry + 8));
#endif		// __PRE_ARAM__
		int actual_data_size = (compressed_data_size != 0) ? compressed_data_size : data_size;
			
#ifdef __PRE_ARAM__
		char pName[1024];
		NsDMA::toMRAM( pName, (uint32)pEntry + PRE_NAME_OFFSET, text_size );
#else
		char *pName = (char *) pEntry + PRE_NAME_OFFSET;
#endif		// __PRE_ARAM__
		uint8 *pCompressedData = pEntry + PRE_NAME_OFFSET + text_size;
		
		_File *pFile;
		if (m_use_bottom_up_heap)
		{
			pFile = new (Mem::Manager::sHandle().BottomUpHeap()) _File;
		}
		else
		{
			pFile = new (Mem::Manager::sHandle().TopDownHeap()) _File;
		}	

		if (!mp_table->GetItem(pName)) 
		{
			// file is not in table, safe to add
			mp_table->PutItem(pName, pFile);
			
			pFile->compressedDataSize = compressed_data_size;
			pFile->pCompressedData = pCompressedData; 
			pFile->pData = NULL;
			pFile->m_position = 0;
			pFile->m_filesize = data_size;
		}
		else
			// Somehow, file is already in table, just kill it
			// Later, I'll want to add an assert
			delete pFile;
		
#		if DEBUG_PRE
		printf("   %s, size %d\n", pName, data_size);
#		endif
		
		pEntry += PRE_NAME_OFFSET + text_size + ((actual_data_size + 3) & (~3));
	}

#	if DEBUG_PRE
	printf("Done loading PRE\n");
#	endif
	
	mp_activeFile = NULL;
	m_numOpenAsyncFiles = 0;
}



PreFile::~PreFile()
{
#ifdef __PRE_ARAM__
	NsARAM::free( (uint32)mp_buffer );
#else
	delete mp_buffer;
#endif		// __PRE_ARAM__
	mp_table->HandleCallback(s_delete_file, NULL);
	mp_table->FlushAllItems();

	delete mp_table;

	Dbg_MsgAssert(m_numOpenAsyncFiles == 0, ("Can't unload Pre because there are still %d async files open", m_numOpenAsyncFiles));
}


bool PreFile::FileExists(const char *pName)
{
	
	_File *pFile = mp_table->GetItem(pName, false);
	return (pFile != NULL);
}

// returns handle pointer
PreFile::FileHandle *PreFile::GetContainedFile(const char *pName)
{
	
	_File *pFile = mp_table->GetItem(pName, false);
	if (!pFile) 
		return NULL;
	
	PreFile::FileHandle *pHandle = pFile;
	//// kinda roundabout, but sets mp_activeFile
	GetContainedFileByHandle(pHandle);
	
#ifdef __PLAT_NGC__
	NsDisplay::doReset();
#endif		// __PLAT_NGC__

	// do we need to fetch file data?
	if (!mp_activeFile->pData)
	{
		if (mp_activeFile->compressedDataSize)
		{
			Mem::PushMemProfile((char*)pName);
			if (m_use_bottom_up_heap)
			{
				mp_activeFile->pData = new (Mem::Manager::sHandle().BottomUpHeap()) uint8[mp_activeFile->m_filesize];		
			}
			else
			{
				mp_activeFile->pData = new (Mem::Manager::sHandle().TopDownHeap()) uint8[mp_activeFile->m_filesize];		
			}	
			Mem::PopMemProfile();
			// need to uncompress data
			DecodeLZSS(mp_activeFile->pCompressedData, mp_activeFile->pData, mp_activeFile->compressedDataSize);	
		}
#ifdef __PRE_ARAM__
		else
		{
			// Just DMA to main RAM.
			Mem::PushMemProfile((char*)pName);
			if (m_use_bottom_up_heap)
			{
				mp_activeFile->pData = new (Mem::Manager::sHandle().BottomUpHeap()) uint8[mp_activeFile->m_filesize];
			}
			else
			{
				mp_activeFile->pData = new (Mem::Manager::sHandle().TopDownHeap()) uint8[mp_activeFile->m_filesize];
			}	
			Mem::PopMemProfile();
			NsDMA::toMRAM( mp_activeFile->pData, (uint32)mp_activeFile->pCompressedData, mp_activeFile->m_filesize );
		}
#endif		// __PRE_ARAM__
	}

	

	return pHandle;
}


// allocate memory and load file directly from a pre file, if it is there
// This avoids the problem of having to have the decompressed file in memory twice
// when we are loading directly, like with Pip::Load()
// using this enables us to actually load network game, where there is 1MB less heap during loading
//
// returns a pointer to the file in memory
// or NULL if the file is not in this pre file.
// optional parameter p_dest, if set to anything other then NULL, then load file to this destination
void *PreFile::LoadContainedFile(const char *pName,int *p_size, void *p_dest)
{

//	printf ("LoadContainedFile(%s\n",pName);
	
	_File *pFile = mp_table->GetItem(pName, false);
	if (!pFile) 
	{
		return NULL;
	}	
	
	*p_size = pFile->m_filesize;

	// If destination was passed as NULL, then allocate memory	
	if (!p_dest)
	{
		p_dest = Mem::Malloc(pFile->m_filesize);		
	}
	
	// do we need to deompress file data?
	if (!pFile->pData)
	{
		if (pFile->compressedDataSize)
		{
			// need to uncompress data
			//DecodeLZSS(mp_activeFile->pCompressedData, mp_activeFile->pData, mp_activeFile->compressedDataSize);	
			DecodeLZSS(pFile->pCompressedData, (uint8*)p_dest, pFile->compressedDataSize);	
		}
		else
		{
#ifdef __PRE_ARAM__
			// Just DMA to main RAM.
			NsDMA::toMRAM( p_dest, (uint32)pFile->pCompressedData, pFile->m_filesize );
#else
			memcpy(p_dest,(void*)pFile->pCompressedData,pFile->m_filesize);
#endif		// __PRE_ARAM__
		}
	}
	else
	{
//		printf ("Copying %d bytes from %p to %p\n",pFile->sky.SOF,p_dest,(void*)pFile->pData);
		memcpy(p_dest,(void*)pFile->pData,pFile->m_filesize);
	}
	return  p_dest;
}
	


uint8 *PreFile::GetContainedFileByHandle(PreFile::FileHandle *pHandle)
{
	mp_table->IterateStart();
	_File *pFile = mp_table->IterateNext();
	while(pFile)
	{
		uint8 *pCompressedData = pFile->pCompressedData;
		if (pCompressedData && pFile == pHandle)
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



void PreFile::Reset()
{
	
	Dbg_AssertPtr(mp_activeFile);

	mp_activeFile->m_position = 0;
}



uint32 PreFile::Read(void *addr, uint32 count)
{
	
	Dbg_AssertPtr(mp_activeFile);

	int seek_offs = mp_activeFile->m_position;
	unsigned int limit = mp_activeFile->m_filesize - seek_offs;
	int copy_number = (count <= limit) ? count : limit; 
#ifdef __PRE_ARAM__
	memcpy(addr, mp_activeFile->pData + mp_activeFile->m_position, copy_number);
#else
	if (mp_activeFile->compressedDataSize)
	{
		Dbg_MsgAssert(mp_activeFile->pData,( "file not uncompressed"));
		memcpy(addr, mp_activeFile->pData + mp_activeFile->m_position, copy_number);
	}
	else
	{
		memcpy(addr, mp_activeFile->pCompressedData + mp_activeFile->m_position, copy_number);
	}
#endif		// __PRE_ARAM__

	mp_activeFile->m_position += copy_number;

#if DEBUG_PRE
		printf("PRE: read %d bytes from file, handle 0x%x\n", copy_number, (int) mp_activeFile->pData);
#endif
	return copy_number;
}



int PreFile::Eof()
{
	
	Dbg_AssertPtr(mp_activeFile);

	if (mp_activeFile->m_position >= mp_activeFile->m_filesize)
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



void PreFile::Open(bool async)
{
	if (async)
	{
		m_numOpenAsyncFiles++;
	}
}

void PreFile::Close(bool async)
{
	
	//Dbg_MsgAssert(mp_activeFile->pData,( "file not uncompressed"));

	if (mp_activeFile->pData)
		delete mp_activeFile->pData;
	mp_activeFile->pData = NULL;

	if (async)
	{
		m_numOpenAsyncFiles--;
		Dbg_MsgAssert(m_numOpenAsyncFiles >= 0, ("PreFile: m_numOpenAsyncFiles is negative after Close()"));
	}
}



int PreFile::Seek(long offset, int origin)
{
	int32 old_pos = mp_activeFile->m_position;

	// SEEK_CUR, SEEK_END, SEEK_SET
	switch(origin)
	{
		case SEEK_CUR:
			mp_activeFile->m_position += offset;
			break;
		case SEEK_END:
			mp_activeFile->m_position = mp_activeFile->m_filesize - offset;
			break;
		case SEEK_SET:
			mp_activeFile->m_position = offset;
			break;
		default:
			return -1;
	}

	if (mp_activeFile->m_position < 0 || mp_activeFile->m_position > mp_activeFile->m_filesize)
	{
		mp_activeFile->m_position = old_pos;
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

	m_num_pending_pre_files = 0;
}



PreMgr::~PreMgr()
{
	delete mp_table;
}



// Returns handle
// Not frequently called
PreFile::FileHandle *PreMgr::getContainedFile(const char *pName)
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

	PreFile::FileHandle *pHandle = NULL;

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
			scePrintf("+++ %s is in PRE\n", cleaned_name);
#			endif
			return pHandle;
		}
		pPre = mp_table->IterateNext();
	}

#	ifdef __PLAT_NGPS__
	scePrintf("--- %s not found in PRE\n", cleaned_name);
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
uint8 *PreMgr::getContainedFileByHandle(PreFile::FileHandle *pHandle)
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
void PreMgr::loadPre(const char *pFilename, bool async, bool dont_assert, bool useBottomUpHeap)
{
#ifdef DVDETH
	m_blockPreLoading = true;
#else
	m_blockPreLoading = (bool) Script::GetInt("block_pre_loading", false);
#endif		// DVDETH 

	if (m_blockPreLoading)
		return;

	// Turn off async for platforms that don't support it
	if (!File::CAsyncFileLoader::sAsyncSupported())
	{
		async = false;
	}

#ifdef __PRE_ARAM__
	Dbg_MsgAssert(!async, ("Async loading not implemented for ARAM transfer"));
#endif

	if( !async && Pcm::UsingCD() )
	{
		Dbg_MsgAssert( 0,( "File access forbidden while PCM audio is in progress." ));
		return;
	}

	// Moved this to below the Pcm::UsingCD() call as that is used (bad!!) to turn off
	// music and streams.
#	ifdef __PLAT_NGPS__
//	scePrintf("Loading PRE file %s...\n", pFilename);
#	endif

	char fullname[256];
	sprintf(fullname, "pre\\%s", pFilename);

#	ifdef __PLAT_XBOX__
	// Replace the .pre extension (if one exists) with .prx for Xbox PRE file.
	if( strrchr( pFilename, '.' ) && ( strlen( fullname ) > 4 ))
	{
		fullname[strlen( fullname ) - 1] = 'x';
	}
#	endif

#	if !defined( __PLAT_NGC__ ) || ( defined( __PLAT_NGC__ ) && !defined( __NOPT_FINAL__ ) )
	Tmr::Time basetime = Tmr::ElapsedTime(0);
#endif

	int file_size;
	uint8 *pFile = NULL;

	// Try loading asynchronously
	if (async)
	{
		Dbg_MsgAssert(m_num_pending_pre_files < MAX_NUM_ASYNC_LOADS, ("Too many async LoadPre's pending"));

		CAsyncFileHandle *p_fileHandle = CAsyncFileLoader::sOpen( fullname, !async );
		if (p_fileHandle)
		{
			Dbg_MsgAssert(strlen(pFilename) < MAX_COMPACT_FILE_NAME, ("Pre file name %s is greater than %d bytes", pFilename, MAX_COMPACT_FILE_NAME - 1));

			// Add to pending list
			strcpy(m_pending_pre_files[m_num_pending_pre_files].m_file_name, pFilename);
			m_pending_pre_files[m_num_pending_pre_files++].mp_file_handle = p_fileHandle;

			file_size = p_fileHandle->GetFileSize();
			Dbg_MsgAssert(file_size, ("Pre file size is 0"));

			Mem::PushMemProfile((char*)fullname);
			if (useBottomUpHeap)
			{
				pFile = new (Mem::Manager::sHandle().BottomUpHeap()) uint8[file_size];
			}
			else
			{
				pFile = new (Mem::Manager::sHandle().TopDownHeap()) uint8[file_size];
			}	
			Mem::PopMemProfile();

			// Set the callback
			p_fileHandle->SetCallback(async_callback, (unsigned int) this, (unsigned int) pFile);

			// read the file in
			p_fileHandle->Read( pFile, 1, file_size );
			return;
		}
	}

	// If we got here, we didn't do an async load
	file_size = CanFileBeLoadedQuickly( fullname );
	if ( file_size )
	{
		Mem::PushMemProfile((char*)fullname);
		if (useBottomUpHeap)
		{
			pFile = new (Mem::Manager::sHandle().BottomUpHeap()) uint8[file_size];
		}
		else
		{
			pFile = new (Mem::Manager::sHandle().TopDownHeap()) uint8[file_size];
		}	
		Mem::PopMemProfile();
		bool fileLoaded= LoadFileQuicklyPlease( fullname, pFile );
		if ( !fileLoaded )
		{
			printf( "pre file %s failed to load quickly.\n", fullname );
			Dbg_MsgAssert( 0,( "Fire Matt - pre file didn't load quickly." ));
		}
	}
	else
	{
			void *fp = File::Open(fullname, "rb");
			if (!fp)
			{
			// always run the code below if CD build
				if (dont_assert || Config::CD()) 
				{
					printf("couldn't open %s\n", fullname);
					return;
				}
				Dbg_MsgAssert(0,( "couldn't open %s\n", fullname));
			}


#ifdef __PRE_ARAM__
			File::Read(&file_size, 4, 1, fp);
			Dbg_MsgAssert(file_size > 0,( "%s has incorrect file size\n", fullname));
			if (Config::CD())
			{
				if (file_size <= 0) printf("%s has incorrect file size\n", fullname);
			}	

			// Stream the file into ARAM.
			#define PRE_BUFFER_SIZE (16 * 1024)
			char p_buffer[PRE_BUFFER_SIZE];

			uint32 p_aram;
			if (useBottomUpHeap)
			{
				p_aram = NsARAM::alloc( file_size, NsARAM::BOTTOMUP );
			}
			else
			{
				p_aram = NsARAM::alloc( file_size, NsARAM::TOPDOWN );
			}	
			if ( p_aram )
			{
				int adjust = 4;
				uint32 toread = file_size;
				uint32 current_aram_offset = p_aram;
				while ( toread )
				{
					uint32 thistime = PRE_BUFFER_SIZE;
					if ( toread < PRE_BUFFER_SIZE ) thistime = toread;

					File::Read(&p_buffer[adjust], 1, thistime - adjust, fp);
					DCFlushRange ( p_buffer, thistime );
					NsDMA::toARAM( current_aram_offset, p_buffer, thistime );
					toread -= thistime;
					current_aram_offset += thistime;
					adjust = 0;
				}
				File::Close(fp);
				pFile = (uint8 *)p_aram;
			}
#else
			file_size = File::GetFileSize(fp);
		// Now allocates the .PRE file from the top of the heap, to avoid fragmentation.
			Mem::PushMemProfile((char*)fullname);
			if (useBottomUpHeap)
			{
				pFile = new (Mem::Manager::sHandle().BottomUpHeap()) uint8[file_size];
			}
			else
			{
				pFile = new (Mem::Manager::sHandle().TopDownHeap()) uint8[file_size];
			}	
			Mem::PopMemProfile();
		//uint8 *pFile = new uint8[file_size];
			File::Read(pFile, 1, file_size, fp);

			int read_file_size = *((int *) pFile);
			Dbg_MsgAssert(file_size == read_file_size,( "%s has incorrect file size: %d vs. expected %d\n", fullname, file_size, read_file_size));
			if (Config::CD())
			{
				if (file_size != read_file_size) printf("%s has incorrect file size\n", fullname);
			}

			File::Close(fp);
#endif		// __PRE_ARAM__
		}

#	if !defined( __PLAT_NGC__ ) || ( defined( __PLAT_NGC__ ) && !defined( __NOPT_FINAL__ ) )
	printf("load time for file %s size %d is %d ms\n", pFilename, file_size, (int) Tmr::ElapsedTime(basetime));
#endif

	// the PRE file object winds up at the top of the heap, too. This is fine because
	// it will be unloaded at the same time as the big file buffer
	if (useBottomUpHeap)
	{
		if (!mp_table->PutItem(pFilename, new (Mem::Manager::sHandle().BottomUpHeap()) PreFile(pFile,useBottomUpHeap)))
			Dbg_MsgAssert(0,( "PRE %s loaded twice", pFilename));
	}
	else
	{
		if (!mp_table->PutItem(pFilename, new (Mem::Manager::sHandle().TopDownHeap()) PreFile(pFile)))
			Dbg_MsgAssert(0,( "PRE %s loaded twice", pFilename));
	}		
}

// Finishes the loading sequence
void   	PreMgr::postLoadPre(CAsyncFileHandle *p_file_handle, uint8 *pData, int size)
{
	// Find entry in pending list
	for (int i = 0; i < m_num_pending_pre_files; i++)
	{
		if (m_pending_pre_files[i].mp_file_handle == p_file_handle)
		{
			// the PRE file object winds up at the top of the heap, too. This is fine because
			// it will be unloaded at the same time as the big file buffer
			if (!mp_table->PutItem(m_pending_pre_files[i].m_file_name, new (Mem::Manager::sHandle().TopDownHeap()) PreFile(pData)))
			{
				Dbg_MsgAssert(0,( "PRE %s loaded twice", m_pending_pre_files[i].m_file_name));
			}

			// Copy last one to this position
			m_pending_pre_files[i] = m_pending_pre_files[--m_num_pending_pre_files];
			return;
		}
	}

	Dbg_MsgAssert(0, ("PreMgr::postLoadPre(): Can't find entry in pending Pre file list"));
}

// Handles all the async callbacks.  Makes sure we only do something on the appropriate callback
void	PreMgr::async_callback(CAsyncFileHandle *p_file_handle, EAsyncFunctionType function,
							   int result, unsigned int arg0, unsigned int arg1)
{
	if (function == File::FUNC_READ)
	{
		PreMgr *p_mgr = (PreMgr *) arg0;

		p_mgr->postLoadPre(p_file_handle, (uint8 *) arg1, result);
	}
}

// Returns point in string where it will fit in compact space
char *	PreMgr::getCompactFileName(char *pName)
{
	int length = strlen(pName);

	if (length < MAX_COMPACT_FILE_NAME)
	{
		return pName;
	}
	else
	{
		int offset = length - (MAX_COMPACT_FILE_NAME - 1);

		return pName + offset;
		//return (char *) ((int) pName + offset);
	}
}

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/


DefineSingletonClass(PreMgr, "PRE Manager");



bool PreMgr::InPre(const char *pFilename)
{
	

	return mp_table->GetItem( pFilename );
}



void PreMgr::LoadPre(const char *pFilename, bool async, bool dont_assert, bool useBottomUpHeap)
{
	// GJ:  This function is a wrapper around loadPRE(), to
	// make sure that all allocations go on the top-down heap
	
	// K: Unless they want to use the bottom up heap :)
	// Needed so that the anims pre can be put on the bottom up, so that the decompressed anims
	// can be put on the top-down, then the pre removed without leaving a hole.
	if (useBottomUpHeap)
	{
		//printf("Loading %s to bottom up heap\n",pFilename);
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
	}
	else
	{
		//printf("Loading %s to top down heap\n",pFilename);
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	}	
	loadPre(pFilename, async, dont_assert, useBottomUpHeap);
	Mem::Manager::sHandle().PopContext();	
}


// This function exisits for historical reasons
void PreMgr::LoadPrePermanently(const char *pFilename, bool async, bool dont_assert)
{

	// Load the pre file...
	// This will go on the top-down heap by default
	LoadPre(pFilename, async, dont_assert);

}



void PreMgr::UnloadPre(const char *pFilename, bool dont_assert)
{
#	ifdef __PLAT_NGPS__
//	scePrintf("Unloading PRE file %s\n", pFilename);
#	endif
	//printf("Unloading PRE file %s\n", pFilename);
	
	if (m_blockPreLoading)
		return;
	
	PreFile *pThePre = mp_table->GetItem(pFilename);
	if (!pThePre)
	{
		if (dont_assert) return;
#	ifndef __PLAT_NGC__
		Dbg_MsgAssert(0,( "PRE file %s not in PRE manager", pFilename));
#	endif
	}

	mp_table->FlushItem(pFilename);
	delete pThePre;
}

bool PreMgr::IsLoadPreFinished(const char *pFilename)
{
	// If it's in the pending list, it isn't done loading
	for (int i = 0; i < m_num_pending_pre_files; i++)
	{
		if (strcmp(m_pending_pre_files[i].m_file_name, pFilename) == 0)
		{
			return false;
		}
	}

	Dbg_MsgAssert(InPre(pFilename), ("IsLoadPreFinished(): Can't find Pre file"));

	return true;
}

bool PreMgr::AllLoadPreFinished()
{
	return m_num_pending_pre_files == 0;
}

void PreMgr::WaitLoadPre(const char *pFilename)
{
	while (!IsLoadPreFinished(pFilename))
	{
		// We got to call this to allow callbacks to come through
		CAsyncFileLoader::sWaitForIOEvent(false);
	}
}

void PreMgr::WaitAllLoadPre()
{
	while (!AllLoadPreFinished())
	{
		// We got to call this to allow callbacks to come through
		CAsyncFileLoader::sWaitForIOEvent(false);
	}
}

bool PreMgr::sPreEnabled()
{
	return sp_mgr != NULL;
}

bool PreMgr::sPreExecuteSuccess()
{
	return s_lastExecuteSuccess; 
}

bool PreMgr::pre_fexist(const char *name)
{
	
	Dbg_MsgAssert(name,( "requesting file NULL"));	
	
	if (sp_mgr->fileExists(name)) 
	{
#		if DEBUG_PRE
		printf("PRE: file %s exists\n", name);
#		endif
		s_lastExecuteSuccess = true;
		return true;
	}
//	if ( Pcm::UsingCD( ) )
//	{
//		Dbg_MsgAssert( 0,( "File access forbidden while PCM audio is in progress." ));
//		return false;
//	}

	return s_lastExecuteSuccess = false;
}



PreFile::FileHandle *PreMgr::pre_fopen(const char *name, const char *access, bool async)
{
	Dbg_MsgAssert(name,( "trying to open file NULL"));	

	PreFile::FileHandle *pHandle = sp_mgr->getContainedFile(name);
	if (pHandle)
	{
		sp_mgr->mp_activePre->Open(async);

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
			sp_mgr->mp_activePre->Close(async);
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

//	// if we get here, we are using the regular file system
//	if ( Pcm::UsingCD( ) )
//	{
//		Dbg_MsgAssert( 0,( "File access forbidden while PCM audio is in progress." ));
//		return NULL;
//	}

	s_lastExecuteSuccess = false;
	return NULL;

	//return pHandle;
}



int PreMgr::pre_fclose(PreFile::FileHandle *fptr, bool async)
{
		
	Dbg_MsgAssert(fptr,( "calling fclose with invalid file ptr"));	
	
	uint8 *pData = sp_mgr->getContainedFileByHandle(fptr);
	if (pData)
	{
#if DEBUG_PRE
		printf("PRE: closed file, handle 0x%x\n", (int) fptr);
#endif
		sp_mgr->mp_activePre->Close(async);
		s_lastExecuteSuccess = true;
		return 0;
	}
	
	s_lastExecuteSuccess = false;
	return 0;
}



size_t PreMgr::pre_fread(void *addr, size_t size, size_t count, PreFile::FileHandle *fptr)
{
		
	Dbg_MsgAssert(fptr,( "calling fread with invalid file ptr"));		

	uint8 *pData = sp_mgr->getContainedFileByHandle(fptr);
	if (pData)
	{
		// read from a simulated file stream in PRE file
		Dbg_AssertPtr(sp_mgr->mp_activePre);
		s_lastExecuteSuccess = true;
		return sp_mgr->mp_activePre->Read(addr, size * count);
	}
//	if ( Pcm::UsingCD( ) )
//	{
//		Dbg_MsgAssert( 0,( "File access forbidden while PCM audio is in progress." ));
//		return 0;
//	}
	
	s_lastExecuteSuccess = false;
	return 0;
}



size_t  PreMgr::pre_fwrite(const void *addr, size_t size, size_t count, PreFile::FileHandle *fptr)
{
		
	#ifdef __NOPT_ASSERT__
	uint8 *pData = 
	#endif
	sp_mgr->getContainedFileByHandle(fptr);
	Dbg_MsgAssert(!pData,( "can't write to a PRE file"));
	
//	if ( Pcm::UsingCD( ) )
//	{
//		Dbg_MsgAssert( 0,( "File access forbidden while PCM audio is in progress." ));
//		return 0;
//	}

	s_lastExecuteSuccess = false;
	return 0;
}



char *PreMgr::pre_fgets(char *buffer, int maxLen, PreFile::FileHandle *fptr)
{
		

	#ifdef __NOPT_ASSERT__
	uint8 *pData = 
	#endif
	sp_mgr->getContainedFileByHandle(fptr);
	Dbg_MsgAssert(!pData,( "can't do string ops on a PRE file"));
	
	s_lastExecuteSuccess = false;
	return NULL;
}



int PreMgr::pre_fputs(const char *buffer, PreFile::FileHandle *fptr)
{
		

	#ifdef __NOPT_ASSERT__
	uint8 *pData = 
	#endif
	sp_mgr->getContainedFileByHandle(fptr);
	Dbg_MsgAssert(!pData,( "can't do string ops on a PRE file"));
	
	s_lastExecuteSuccess = false;
	return 0;
}



int PreMgr::pre_feof(PreFile::FileHandle *fptr)
{
		
	Dbg_MsgAssert(fptr,( "calling feof with invalid file ptr"));		

	uint8 *pData = 	sp_mgr->getContainedFileByHandle(fptr);
	if (pData)
	{
		Dbg_AssertPtr(sp_mgr->mp_activePre);
		s_lastExecuteSuccess = true;
		return sp_mgr->mp_activePre->Eof();
	}
	
	s_lastExecuteSuccess = false;
	return 0;
}



int PreMgr::pre_fseek(PreFile::FileHandle *fptr, long offset, int origin)
{
		

	uint8 *pData = 	sp_mgr->getContainedFileByHandle(fptr);
	if (pData)
	{
		s_lastExecuteSuccess = true;
		return sp_mgr->mp_activePre->Seek(offset, origin);
	}

	Dbg_MsgAssert(!pData,( "seek not supported for PRE file"));
	s_lastExecuteSuccess = false;
	return 0;
}



int PreMgr::pre_fflush(PreFile::FileHandle *fptr)
{
		

	#ifdef __NOPT_ASSERT__
	uint8 *pData = 
	#endif
	sp_mgr->getContainedFileByHandle(fptr);
	Dbg_MsgAssert(!pData,( "flush not supported for PRE file"));
	
	s_lastExecuteSuccess = false;
	return 0;
}



int PreMgr::pre_ftell(PreFile::FileHandle *fptr)
{
		

	#ifdef __NOPT_ASSERT__
	uint8 *pData = 
	#endif
	sp_mgr->getContainedFileByHandle(fptr);
	Dbg_MsgAssert(!pData,( "tell supported for PRE file"));
	
	s_lastExecuteSuccess = false;
	return 0;
}


int	PreMgr::pre_get_file_size(PreFile::FileHandle *fptr)
{
	uint8 *pData = 	sp_mgr->getContainedFileByHandle(fptr);
	if (pData)
	{
		s_lastExecuteSuccess = true;
		return sp_mgr->mp_activePre->GetFileSize();
	}

	Dbg_MsgAssert(!pData,( "get_file_size not supported for PRE file"));
	s_lastExecuteSuccess = false;
	return 0;
}

int PreMgr::pre_get_file_position(PreFile::FileHandle *fptr)
{
	uint8 *pData = 	sp_mgr->getContainedFileByHandle(fptr);
	if (pData)
	{
		s_lastExecuteSuccess = true;
		return sp_mgr->mp_activePre->GetFilePosition();
	}

	Dbg_MsgAssert(!pData,( "get_file_position not supported for PRE file"));
	s_lastExecuteSuccess = false;
	return 0;
}


// @script | InPreFile | 
// @uparm "string" | filename
bool ScriptInPreFile(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char *pFilename;
	pParams->GetText(NONAME, &pFilename, true);

	PreMgr* pre_mgr = PreMgr::Instance();
	return pre_mgr->InPre(pFilename);
}



// @script | LoadPreFile | 
// @uparm "string" | filename
// @flag async | Load Asynchronously
// @flag dont_assert | 
// @flag use_bottom_up_heap | 
bool ScriptLoadPreFile(Script::CStruct *pParams, Script::CScript *pScript)
{
	const char *pFilename;
	pParams->GetText(NONAME, &pFilename, true);

	PreMgr* pre_mgr = PreMgr::Instance();
	pre_mgr->LoadPre(pFilename, 
					 pParams->ContainsFlag(CRCD(0x90e07c79,"async")),
					pParams->ContainsFlag(CRCD(0x3d92465e,"dont_assert")),
					pParams->ContainsFlag(CRCD(0xa44c770f,"use_bottom_up_heap")));
	return true;
}



// @script | UnloadPreFile | 
// @flag BoardsPre | 
// @flag dont_assert | 
// @uparm "string" | filename
bool ScriptUnloadPreFile(Script::CStruct *pParams, Script::CScript *pScript)
{
	

	PreMgr* pre_mgr = PreMgr::Instance();
	
	if (pParams->ContainsFlag("BoardsPre"))
	{
		
//		Mdl::Skate * pSkate = Mdl::Skate::Instance();
//		pSkate->UnloadBoardPreIfPresent(pParams->ContainsFlag("dont_assert"));
		printf ("STUBBED:  Unload BoardsPre, in ScriptUnloadPreFile\n");
		return true;
	}
	
	const char *pFilename;
	pParams->GetText(NONAME, &pFilename, true);

	pre_mgr->UnloadPre(pFilename, pParams->ContainsFlag("dont_assert"));
	return true;
}

// @script | IsLoadPreFinished | Returns true if Pre file has finished loading
// @uparm "string" | filename
bool ScriptIsLoadPreFinished(Script::CStruct *pParams, Script::CScript *pScript)
{
	PreMgr* pre_mgr = PreMgr::Instance();

	const char *pFilename;
	pParams->GetText(NONAME, &pFilename, true);

	return pre_mgr->IsLoadPreFinished(pFilename);
}

// @script | AllLoadPreFinished | Returns true if all LoadPre commands have completed
bool ScriptAllLoadPreFinished(Script::CStruct *pParams, Script::CScript *pScript)
{
	PreMgr* pre_mgr = PreMgr::Instance();

	return pre_mgr->AllLoadPreFinished();
}

// @script | WaitLoadPre | Waits for Pre file to finished loading
// @uparm "string" | filename
bool ScriptWaitLoadPre(Script::CStruct *pParams, Script::CScript *pScript)
{
	PreMgr* pre_mgr = PreMgr::Instance();

	const char *pFilename;
	pParams->GetText(NONAME, &pFilename, true);

	pre_mgr->WaitLoadPre(pFilename);
	return true;
}

// @script | WaitAllLoadPre | Waits for all Pre files to finished loading
bool ScriptWaitAllLoadPre(Script::CStruct *pParams, Script::CScript *pScript)
{
	PreMgr* pre_mgr = PreMgr::Instance();

	pre_mgr->WaitAllLoadPre();
	return true;
}

// if a file is in a pre, then:
// allocate memory for the file
// if file is uncompressed
//   copy it down
// else
//   decompress

void * PreMgr::LoadFile(const char *pName, int *p_size, void *p_dest)
{
// NOTE: THIS IS JUST CUT AND PASTE FROM Pre::fileExists
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
		
		void *p_data = pPre->LoadContainedFile(cleaned_name,p_size, p_dest);
		if (p_data)
		{
			return p_data;
		}
		pPre = mp_table->IterateNext();
	}
	return NULL;

}



} // namespace File




