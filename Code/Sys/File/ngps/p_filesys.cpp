/*****************************************************************************
**																			**
**			              Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		System Library											**
**																			**
**	Module:			File IO (File) 											**
**																			**
**	File name:		p_filesys.cpp											**
**																			**
**	Created by:		03/20/00	-	mjb										**
**																			**
**	Description:	PS2 File System											**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sifdev.h>
#include <sifrpc.h>
#include <libcdvd.h>
#include <libdma.h>

#include <core/defines.h>
#include <sys/file/filesys.h>
#include <sys/file/AsyncFilesys.h>
#include <sys/file/PRE.h>
#include <sys/file/ngps/hed.h>
#include <sys/file/ngps/p_AsyncFilesys.h>
#include <sys/config/config.h>

#include <gel/music/music.h>
#include <gel/scripting/symboltable.h>

#include <dnas/dnas2.h>

#ifdef	PAL
#define	__DVD_ONLY__ 0				// 0 = DVD or CD,  1 = Just DVD (for copy protection)
#else
#define	__DVD_ONLY__ 0				// 0 = DVD or CD,  1 = Just DVD (for copy protection)
#endif

#define ASYNC_QUICK_FILESYS 1
#define ASYNC_HOST_FILESYS	0
#define DISABLE_QUICK_FILESYSTEM	0

/*****************************************************************************
**							  DBG Information								**
*****************************************************************************/

namespace File
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/


/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#ifndef SECTOR_SIZE
#define SECTOR_SIZE 2048
#endif // SECTOR_SIZE



// MEMOPT: If we run out of IOP memory again, it is possible that we could use the pcm buffer
// (The one that is TOTAL_IOP_BUFFER_SIZE_NEEDED) for the file i/o, since probably music or 
// streams will not need to be playing when a file is loading.
// That would free up 2048*48=98304 bytes!!
#define FILESYS_NUM_SECTORS_IN_BUFFER			48
#define FILESYS_STREAM_BUFFER_NUM_PARTITIONS	3
#define FILESYS_IOP_BUFFER_SIZE					( SECTOR_SIZE * FILESYS_NUM_SECTORS_IN_BUFFER )
#define FILESYS_NUM_SECTORS_PER_READ			16
#define	FILESYS_CD_READ_SIZE					( SECTOR_SIZE * FILESYS_NUM_SECTORS_PER_READ )
#define IOP_TO_EE_BUFFER_ALLIGNMENT				64

// Static globals:
static void				*gFilesysIOPStreamBuffer = NULL;
//static char				*gIOPToEEBuffer;
	   unsigned int		gWadLSN = 0;          // Made global for p_AsyncFilesystem
static bool				gQuickFileSystemInitialzed = false;
static bool				gStreaming = false;

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

#define READBUFFERSIZE (10240*5)

// GJ:  I added this to track how many skyFiles are active
// at any given time.  I am assuming that there's only one,
// in which case I can just create one global instance
// (previously, we were allocating it off the current heap,
// which was causing me grief during the CAS loading process)
//int g_fileOpenCount = 0;

struct skyFile
{
	
	// the following must match the dumbSkyFile struct
	// in pre.cpp, or else pre file i/o won't work properly
	int				gdfs;
	int32			POS;
	int32			SOF;

	// the rest is skyFile-specific
	uint8			readBuffer[READBUFFERSIZE];
	uint32		bufferPos;
	bool			bufferValid;
	bool			locked;			// whether the skyfile is currently in use
	
	// Used when CD
	uint32			WadOffset;
	const char		*pFilename;
	
	// Used when non-CD
	#define MAX_PFILESYS_NAME_SIZE 255
	char filename[ MAX_PFILESYS_NAME_SIZE ];
#if ASYNC_HOST_FILESYS
	CAsyncFileHandle *p_async_file;
#endif
};

// At first, I assumed that only one skyfile could be open at
// a time, but that's not true when testing streams from the
// PC build.
// pc builds need to have two (one for normal files, and one for streams)
// GJ:  for THPS4, we increased this to 3, so that we can play multiple streams
const int MAXOPENFILES = 3;

// Ken note: It used to be that MAXOPENFILES was defined to be 1 when __NOPT_CDROM__OLD
// was set. Now that __NOPT_CDROM__OLD is not used anymore and CD() is used at runtime
// instead, I made MAXOPENFILES always 2. Then, where MAXOPENFILES used to be used in the code
// it now uses a var set to 1 or 2 depending on CD()
static skyFile g_skyFile[MAXOPENFILES];

skyFile* lock_skyfile( void )
{
	// cd builds can only have one file open at any given time
	int max_open_files=Config::CD() ? 1:MAXOPENFILES;
	
	int i;
	// this tries to find an unlocked skyfile
	for ( i = 0; i < max_open_files; i++ )
	{
		if ( !g_skyFile[i].locked )
		{
			g_skyFile[i].locked = true;
			return &g_skyFile[i];
		}
	}

	if (!Config::CD())
	{
		Dbg_Message( "Here are the files currently open:" );
		for ( i = 0; i < max_open_files; i++ )
		{
			Dbg_Message( "%s", g_skyFile[ i ].filename );
		}
	}	

	// if we get here, that means that all the sky files were locked
	Dbg_MsgAssert( 0, ( "Trying to open too many files simultaneously (max=%d)", max_open_files ) );
	return NULL;
}

void unlock_skyfile( skyFile* pSkyFile )
{
	// cd builds can only have one file open at any given time
	int max_open_files=Config::CD() ? 1:MAXOPENFILES;
	

	// this tries to find the sky file that the caller is referencing
	for ( int i = 0; i < max_open_files; i++ )
	{
		if ( pSkyFile == &g_skyFile[i] )
		{
			Dbg_MsgAssert( g_skyFile[i].locked, ( "Trying to unlock a sky file too many times" ) );
			g_skyFile[i].locked = false;
			return;
		}
	}

	// if we get here, that means that the pointer didn't match one of the valid skyfiles
	Dbg_MsgAssert( 0, ( "Unrecognized sky file" ) );
}

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

static	int32			s_open_files = 0;

// The header file (SKATE4.hed) used for looking up where files are within
// SKATE4.wad
SHed *gpHed=NULL;		// Made global for p_AsyncFilesystem
static int WadId=-1;

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/* skyTransMode                                                   */
/*                                                                */
/* Attempt to convert a mode string to an open mode				  */ 
/*                                                                */
/* On entry  : access mode										  */
/* On exit   : integer mode										  */
/*                                                                */
/******************************************************************/

static int skyTransMode(const char *access)
{
	

	int mode;
	char *r;
	char *w;
	char *a;
	char *plus;
	char *n;
	char *d;

	/* I add a couple of new characters for now:
	   n non-blocking mode
	   d no write back d cache */

	r = strrchr(access, (int)'r');
	w = strrchr(access, (int)'w');
	a = strrchr(access, (int)'a');
	plus = strrchr(access, (int)'+');
	n = strrchr(access, (int)'n');
	d = strrchr(access, (int)'d');

	if (plus)
		mode = SCE_RDWR;
	else if (r)
		mode = SCE_RDONLY;
	else if (w)
		mode = SCE_WRONLY;
	else if (a)
		mode = SCE_WRONLY;
	else
		return(0);

	/* later we will test for SCE_CREAT & !SCE_TRUNC as a seek to end of file */
	if (w)
		mode |= SCE_CREAT | SCE_TRUNC;

	if (a)
		mode |= SCE_CREAT;

	if (n)
		mode |= SCE_NOWAIT;

	if (d)
		Dbg_MsgAssert( 0,( "Hmm... gotta find out what SCE_NOWBDC was in previous library." ));
//		mode |= SCE_NOWBDC;

	return(mode);
}

void StartStreaming( uint32 lsn )
{
	Dbg_MsgAssert( gFilesysIOPStreamBuffer,( "IOP stream buffer not initialized" ));

	sceCdSync(0);			// Make sure there aren't any non-blocking CD reads happening
	Pcm::StopStreams( );	// Garrett: I found streamed sounds can skip when using the quick load
//	if ( Pcm::UsingCD( ) )
//	{
//		Dbg_MsgAssert( 0,( "File access forbidden while PCM audio is in progress." ));
//	}
	sceCdRMode mode;
	mode.trycount = 255;
	mode.spindlctrl = SCECdSpinNom;
    mode.datapattern = SCECdSecS2048;
	mode.pad = 0;
	while ( !( gStreaming = sceCdStStart( lsn, &mode ) ) )
	{
		printf( "aah ha! would have caused the crash!!!\n" );
		sceCdStop();
		sceCdSync(0);
	}
}

void StopStreaming( void )
{
	if (!Config::CD())
	{
		return;
	}
		
	if ( gStreaming )
	{
		while ( !sceCdStStop( ) )
		{
			printf( "sceCdStStop failed\n" );
		}
	}
	gStreaming = false;
}


uint32 CanFileBeLoadedQuickly( const char *filename )
{
#if DISABLE_QUICK_FILESYSTEM
	return 0;
#endif

	if (!Config::CD())
	{
		return 0;
	}
		
	if ( !gQuickFileSystemInitialzed )
	{
		return ( 0 );
	}
    if ( !gpHed )
	{
		return ( 0 );
	}
	SHedFile *tempHed = FindFileInHed( filename, gpHed );
	if ( tempHed )
		return ( tempHed->GetFileSize() );
	return ( 0 );
}

#if ASYNC_QUICK_FILESYS == 0
static void HandleCDStreamingError( uint32 lsn )
{
	
	int err;
	err = sceCdGetError( );
	printf( "CD Error %d\n", err );
	// let's just stop everything, and try again fresh and new:
	sceCdBreak( );
	sceCdSync( 0 );
	StartStreaming( lsn );
/*	switch ( err )
	{
		case ( SCECdErNO ):
			break;
		case ( SCECdErTRMOPN ):
			break;
		case ( ):
			break;
		case ( ):
			break;
		case ( ):
			break;
		default:
			break;
	}*/
}
#endif // ASYNC_QUICK_FILESYS == 0

bool LoadFileQuicklyPlease( const char *filename, uint8 *addr )
{
	if (!Config::CD())
	{
		return false;
	}

	if ( !gQuickFileSystemInitialzed )
	{
		return ( false );
	}
		
#if ASYNC_QUICK_FILESYS

	CAsyncFileHandle *p_handle = CAsyncFileLoader::sOpen(filename, false);
	if (p_handle)
	{
		p_handle->Load(addr);
		p_handle->WaitForIO();
		CAsyncFileLoader::sClose(p_handle);
	}
	else
	{
		Dbg_MsgAssert(0, ("Didn't get handle pointer from async filesys for file %s", filename));
		return false;
	}

#else
	Dbg_MsgAssert( gFilesysIOPStreamBuffer,( "IOP stream buffer not initialized" ));

	if ( !gpHed )
	{
		return ( false );
	}

	SHedFile *tempHed = FindFileInHed( filename, gpHed );
	if ( !tempHed )
	{
		Dbg_MsgAssert( 0,( "File cannot be loaded quickly." ));
		return ( false );
	}
	uint32 bytesRequired = tempHed->GetFileSize();
//	uint32 offsetWithinBuffer = tempHed->Offset & ( SECTOR_SIZE - 1 );
	if ( tempHed->Offset & ( SECTOR_SIZE - 1 ) )
	{
		int i;
		for ( i = 0; i != -1; i++ )
		{
			printf( "fix wad so all files start on 2048 boundary.\n" );
		}
	}

	if (CAsyncFileLoader::sAsyncInUse())
	{
		CAsyncFileLoader::sWaitForIOEvent(true);
		Dbg_Message("************************ Can't do a normal read when async filesystem is in use");
		//Dbg_MsgAssert(0, ("Can't do a normal read when async filesystem is in use"));
	}

	uint32 lsn = gWadLSN;
	lsn += tempHed->Offset / SECTOR_SIZE;

	uint8 *pEEBuffer = new uint8[ FILESYS_CD_READ_SIZE + IOP_TO_EE_BUFFER_ALLIGNMENT ];

	uint8 *pData = pEEBuffer;
	pData = ( uint8 * )( ( ( int )pEEBuffer + ( IOP_TO_EE_BUFFER_ALLIGNMENT - 1 ) ) & ~( IOP_TO_EE_BUFFER_ALLIGNMENT - 1 ) );

	uint32 numSectorsToRead;
	uint32 err;
	uint32 numBytesToCopy;
	uint32 i;
	uint32 *pAI;
	uint32 *pDI;
	if ( gStreaming )
	{
		if ( !sceCdStSeek( lsn ) )
		{
			StartStreaming( lsn );
		}
	}
	else
	{
		StartStreaming( lsn );
	}
	while ( bytesRequired )
	{
		if ( /*offsetWithinBuffer +*/ bytesRequired < FILESYS_CD_READ_SIZE )
		{
			numBytesToCopy = bytesRequired;
			numSectorsToRead = ( ( /*offsetWithinBuffer +*/ bytesRequired ) / SECTOR_SIZE );
			if ( bytesRequired & ( SECTOR_SIZE - 1 ) )
			 numSectorsToRead++;
		}
		else
		{
			numBytesToCopy = FILESYS_CD_READ_SIZE;// - offsetWithinBuffer;
			numSectorsToRead = FILESYS_NUM_SECTORS_PER_READ;
		}
		
		// read in a chunk at a time ( 50 sectors? 100 sectors? )
		uint32 sectorsRead;
		sectorsRead = sceCdStRead( numSectorsToRead, ( uint32 * )pData, STMBLK, &err );
		
		int numErrors = 0;
		while ( sectorsRead != numSectorsToRead )
		{
			printf("QuickLoad: Could only read %d of the %d sectors for file %s\n", sectorsRead, numSectorsToRead, filename);
			sceCdStSeek( lsn );
			sectorsRead = sceCdStRead( numSectorsToRead, ( uint32 * )pData, STMBLK, &err );
			if ( numErrors++ >= 12 )
			{
				// figure out what the problem is using sceCDGetError( ) and either try to
				// recover or put up an error screen or something?
				HandleCDStreamingError( lsn );
				numErrors = 0;
			}
		}

		pAI = ( uint32 * )( addr );
		pDI = ( uint32 * )( pData );
		// copy the data from the buffer to the given address:
		for ( i = 0; i < ( numBytesToCopy >> 2 ); i++ )
		{
			*pAI++ = *pDI++;
		}
		for ( i = 0; i < ( numBytesToCopy & 3 ); i++ )
		{
			//addr[ 1 + ( numBytesToCopy & ~3 ) + i ] = pData[ 1 + ( numBytesToCopy & ~3 ) + i ];
			addr[  ( numBytesToCopy & ~3 ) + i ] = pData[ ( numBytesToCopy & ~3 ) + i ];
		}
		
		addr += numBytesToCopy;
		bytesRequired -= numBytesToCopy;
		
		lsn += numSectorsToRead;
//		offsetWithinBuffer = 0;
	}
//	sceCdStPause( );

// Mick: 8/28/02 - I added the next line
// the aim is to stop streaming now, so we don't have to do the extra call to
// sceCdStSeek  for the next call to this functions
// The speeds up the loading of skaters by 20%
	StopStreaming();

	delete pEEBuffer;
#endif // ASYNC_QUICK_FILESYS

	return ( true );
}

static void* cdFopen(const char *fname, const char *access)
{
	
	
	SHedFile *pHd=FindFileInHed(fname, gpHed);
	if (!pHd)
	{
		return NULL;
	}

	skyFile *fp = lock_skyfile();

	if (!fp)
	{
		return (NULL);
	}

	int mode = skyTransMode(access);
	if (!mode)
	{
		unlock_skyfile( fp );
		return (NULL);
	}

	fp->pFilename=fname;
	fp->WadOffset=pHd->Offset;					
	fp->SOF=pHd->FileSize;
	fp->POS=0;
	fp->gdfs=0;
#if ASYNC_HOST_FILESYS
	fp->p_async_file = NULL;
#endif
	/* Initialise the buffer to show nothing buffered */
	fp->bufferPos = READBUFFERSIZE;
	
	/* SCE_CREAT & !SCE_TRUNC mean seek to end of file */
	//if (!((mode & SCE_CREAT) && !(mode & SCE_TRUNC)))
	//{
	//	fp->POS = fp->SOF;
	//}
	
	s_open_files++;

	return ((void *)fp);
}

static int cdFclose( void* fptr )
{
	
	
	skyFile* fp = (skyFile*)fptr;

	Dbg_AssertPtr( fptr );


	if ( fp && s_open_files )
	{
		unlock_skyfile( fp );
        
		s_open_files--;

		return 0;
	}

	return -1;
}

static bool cdFexist( const char* name )
{
	
	if (FindFileInHed(name, gpHed))
	{
		return true;
	}	
	else
	{
		return false;
	}	
}

static size_t cdFread(void *addr, size_t size, size_t count, void *fptr)
{
	
	skyFile     *fp = (skyFile *)fptr;
	size_t      numBytesToRead = size * count;
	int         bytesRead, bytesRead2;

	bytesRead = 0;

	StopStreaming( );

//	char * after = (char*)addr + numBytesToRead; 							   

															  

	/* Trim number of bytes for the size of the file */
	if ((fp->POS + (int32)numBytesToRead) > fp->SOF)
	{
		numBytesToRead = fp->SOF - fp->POS;
	}

	
	/* First try and use the buffer */
	if ((fp->bufferPos < READBUFFERSIZE) &&
		(bytesRead < (int32)numBytesToRead))
	{
		/* Pull from the buffer */
		if (numBytesToRead < (READBUFFERSIZE-fp->bufferPos))
		{
			/* Can satisfy entirely from buffer */
			bytesRead = numBytesToRead;
		}
		else
		{
			/* Pull as much as possible from the buffer */
			bytesRead = READBUFFERSIZE-fp->bufferPos;
		}

		/* Copy it */
		memcpy(addr, &fp->readBuffer[fp->bufferPos], bytesRead);

		/* Update target address and source address */
		addr = (void *)((uint8 *)addr + bytesRead);
		fp->bufferPos += bytesRead;
		fp->POS += bytesRead;
	}

	/* If next bit is bigger than a buffer, read it directly and ignore the
	 * buffer.
	 */
	if ((numBytesToRead-bytesRead) > 0)
	{
		if ((numBytesToRead-bytesRead) >= READBUFFERSIZE)
		{
			bytesRead2 = (numBytesToRead-bytesRead);
			//bytesRead2 = sceRead(fp->gdfs, addr, bytesRead2);
			Dbg_MsgAssert(WadId>=0,("Bad WadId"));	
			//Dbg_Message("Seeking to LSN %d", fp->WadOffset+fp->POS);
			sceLseek(WadId, fp->WadOffset+fp->POS, SCE_SEEK_SET);
			bytesRead2=sceRead(WadId,addr,bytesRead2);
			
			if (bytesRead2 < 0)
			{
				bytesRead2 = 0;
			}
		}
		else
		{
			/* Go via the buffer */
			//sceRead(fp->gdfs, fp->readBuffer, READBUFFERSIZE);
			Dbg_MsgAssert(WadId>=0,("Bad WadId"));	
			//Dbg_Message("Seeking to LSN %d", fp->WadOffset+fp->POS);
			sceLseek(WadId, fp->WadOffset+fp->POS, SCE_SEEK_SET);
			sceRead(WadId,fp->readBuffer, READBUFFERSIZE);

			
			bytesRead2 = (numBytesToRead-bytesRead);
			memcpy(addr, fp->readBuffer, bytesRead2);
			fp->bufferPos = bytesRead2;
		}
		fp->POS += bytesRead2;
		bytesRead += bytesRead2;
	}

	return (bytesRead/size);
}

static size_t cdFwrite( const void *addr, size_t size, size_t count, void *fptr )
{
	
	skyFile  *fp = (skyFile *)fptr;
	Dbg_AssertPtr( fptr );
	
	fp->POS+=size*count;			 
	fp->SOF=fp->POS;
	return count;
}

static int cdFseek(void *fptr, long offset, int origin)
{
	

	skyFile      *fp = (skyFile *)fptr;
	int32      oldFPos, bufStart;
	bool       noBuffer = FALSE;

	Dbg_AssertPtr( fptr );

	oldFPos = fp->POS;
	bufStart = oldFPos - fp->bufferPos;
	if (fp->bufferPos == READBUFFERSIZE) noBuffer = TRUE;
	fp->bufferPos = READBUFFERSIZE;

	switch (origin)
	{
		case SEEK_CUR:
		{            
			/* Does the seek stay in the buffer */
			if ((oldFPos + offset >= bufStart) &&
				(oldFPos + offset < bufStart+READBUFFERSIZE))
			{
				fp->bufferPos = (oldFPos + offset) - bufStart;
				fp->POS += offset;
			}
			else
			{
				fp->POS+=offset;
				if (fp->POS<0 || fp->POS>fp->SOF)
				{
					fp->POS=-1;
					Dbg_MsgAssert(0,("Bad offset sent to cdFseek (SEEK_CUR) offset=%ld pos=%d size=%d",offset,fp->POS-offset,fp->SOF));
				}	
			}
			break;
		}
		case SEEK_END:
		{
			fp->POS=fp->SOF-offset;
			if (fp->POS<0 || fp->POS>fp->SOF)
			{
				fp->POS=-1;
				Dbg_MsgAssert(0,("Bad offset sent to cdFseek (SEEK_END)"));			
			}	
			break;
		}
		case SEEK_SET:
		{
			fp->POS=offset;
			if (fp->POS<0 || fp->POS>fp->SOF)
			{
				fp->POS=-1;
				Dbg_MsgAssert(0,("Bad offset sent to cdFseek (SEEK_SET)"));			
			}	
			break;
		}
		default:
		{
			return (-1);
		}
	}

	if (noBuffer)
		fp->bufferPos = READBUFFERSIZE;

	if (fp->POS == -1)
	{
		/* This may not be valid */
		fp->POS = oldFPos;
		fp->bufferPos = READBUFFERSIZE;
		return (-1);
	}

	return (0);
}

static char * cdFgets(char *buffer, int maxLen, void *fptr)
{
	
	Dbg_MsgAssert(0,("fgets not done yet"));
	return NULL;
}

static int cdFputs( const char *buffer, void *fptr)
{
	
	Dbg_MsgAssert(0,("Cannot fputs to CD"));
	return 0;
}

static int cdFeof( void* fptr )
{
	

	skyFile  *fp = (skyFile*)fptr;

	Dbg_AssertPtr( fptr );

	return ( fp->POS >= fp->SOF) ;
}

static int cdFflush( void* )
{
	
	
	return 0;
}



/******************************************************************/
/* skyFopen                                                       */
/*                                                                */
/* On entry   : Filename, access mode                             */
/*                                                                */
/*                                                                */
/******************************************************************/

static void* skyFopen(const char *fname, const char *access)
{
	

	skyFile*	fp;
	int         mode;

	StopStreaming( );

	/* Allocate structure for holding info */
	fp = lock_skyfile();

	if (!fp)
	{
		return (NULL);
	}

	mode = skyTransMode(access);
	if (!mode)
	{
		return (NULL);
	}

#if ASYNC_HOST_FILESYS
	fp->p_async_file = CAsyncFileLoader::sOpen(fname, true);

	if (!fp->p_async_file)
	{
		unlock_skyfile( fp );
		return (NULL);
	}

	// Get file size (will come up with better way)
	fp->SOF = fp->p_async_file->GetFileSize();
	fp->POS = 0;
	//fp->p_async_file->Seek(0, SEEK_END);
	//fp->SOF = fp->p_async_file->WaitForIO();
	//fp->p_async_file->Seek(0, SEEK_SET);
	//fp->POS = fp->p_async_file->WaitForIO();
#else
	char      name[256];
	char*		nameptr;

	/* First manipulate the filename into a Sky friendly name */
	if (strchr(fname, ':'))
	{
		strncpy(name, fname, 255);
	}
	else
	{
#ifdef SCE_11
		strcpy(name, "sim:");
		strncpy(&name[4], fname, 251);
#else
		strcpy(name, "host:");
		strncpy(&name[5], fname, 250);
#endif       
	}
	/* force null termination */
	name[255] = 0;

	nameptr = name;
	while(*nameptr)
	{
		if (*nameptr == '\\') *nameptr = '/';
		nameptr++;
	}

	fp->gdfs = sceOpen(name, mode);

	if (fp->gdfs < 0)
	{
		unlock_skyfile( fp );
		return (NULL);
	}

	/* We seek to the end of the file to get size */
	fp->SOF = fp->POS = sceLseek(fp->gdfs, 0, SCE_SEEK_END);
	if (fp->SOF< 0)
	{
		sceClose(fp->gdfs);
		unlock_skyfile( fp );
		return (NULL);
	}
	/* SCE_CREAT & !SCE_TRUNC mean seek to end of file */
	if (!((mode & SCE_CREAT) && !(mode & SCE_TRUNC)))
	{
		fp->POS = sceLseek(fp->gdfs, 0, SCE_SEEK_SET);

		if (fp->POS < 0)
		{
			sceClose(fp->gdfs);
			unlock_skyfile( fp );
			
			return (NULL);
		}
	}
#endif

	/* Initialise the buffer to show nothing buffered */
	fp->bufferPos = READBUFFERSIZE;
	strncpy( fp->filename, fname, MAX_PFILESYS_NAME_SIZE );
	s_open_files++;

	return ((void *)fp);
}


/******************************************************************/
/* skyFclose                                                      */
/*                                                                */
/******************************************************************/

static int skyFclose( void* fptr )
{
	

	skyFile* fp = (skyFile*)fptr;

	Dbg_AssertPtr( fptr );

	if ( fp && s_open_files )
	{
#if ASYNC_HOST_FILESYS
		CAsyncFileLoader::sClose(fp->p_async_file);
		fp->p_async_file = NULL;
#else
		sceClose( fp->gdfs );
#endif
		unlock_skyfile( fp );
        
		s_open_files--;

		return 0;
	}

	return -1;
}

/******************************************************************/
/* skyFexist                                                      */
/*                                                                */
/******************************************************************/

static bool skyFexist( const char* name )
{
	

	void* res = File::Open( name, "r" );

	if ( res )
	{
		File::Close( res );
		return TRUE;
	}

	return FALSE;
}

/******************************************************************/
/* skyFread                                                       */
/*                                                                */
/* On entry : Address to read to, block size, block count, file   */
/* On exit  : Number of bytes read                                */
/*                                                                */
/******************************************************************/

static size_t skyFread(void *addr, size_t size, size_t count, void *fptr)
{
	
	
	skyFile     *fp = (skyFile *)fptr;
	size_t      numBytesToRead = size * count;
	int         bytesRead = 0;

	StopStreaming( );

	/* Trim number of bytes for the size of the file */
	if ((fp->POS + (int32)numBytesToRead) > fp->SOF)
	{
		numBytesToRead = fp->SOF - fp->POS;
	}

#if ASYNC_HOST_FILESYS
	fp->p_async_file->Read(addr, size, count);
	bytesRead = fp->p_async_file->WaitForIO();
	fp->POS += bytesRead;
#else
	int bytesRead2;

	/* First try and use the buffer */
	if ((fp->bufferPos < READBUFFERSIZE) &&
		(bytesRead < (int32)numBytesToRead))
	{
		/* Pull from the buffer */
		if (numBytesToRead < (READBUFFERSIZE-fp->bufferPos))
		{
			/* Can satisfy entirely from buffer */
			bytesRead = numBytesToRead;
		}
		else
		{
			/* Pull as much as possible from the buffer */
			bytesRead = READBUFFERSIZE-fp->bufferPos;
		}

		/* Copy it */
		memcpy(addr, &fp->readBuffer[fp->bufferPos], bytesRead);

		/* Update target address and source address */
		addr = (void *)((uint8 *)addr + bytesRead);
		fp->bufferPos += bytesRead;
		fp->POS += bytesRead;
	}

	/* If next bit is bigger than a buffer, read it directly and ignore the
	 * buffer.
	 */
	if ((numBytesToRead-bytesRead) > 0)
	{
		if ((numBytesToRead-bytesRead) >= READBUFFERSIZE)
		{
			bytesRead2 = (numBytesToRead-bytesRead);
			bytesRead2 = sceRead(fp->gdfs, addr, bytesRead2);
			if (bytesRead2 < 0)
			{
				bytesRead2 = 0;
			}
		}
		else
		{
			/* Go via the buffer */
			sceRead(fp->gdfs, fp->readBuffer, READBUFFERSIZE);
			bytesRead2 = (numBytesToRead-bytesRead);
			memcpy(addr, fp->readBuffer, bytesRead2);
			fp->bufferPos = bytesRead2;
		}
		fp->POS += bytesRead2;
		bytesRead += bytesRead2;
	}
#endif

	return (bytesRead/size);
}

/******************************************************************/
/* skyFwrite                                                      */
/*                                                                */
/* On entry : Address to write from, block size, block count, file*/
/* On exit  : Number of bytes written                             */
/*                                                                */
/******************************************************************/

static size_t skyFwrite( const void *addr, size_t size, size_t count, void *fptr )
{
	

	int         bytesWritten;
	skyFile*	fp = (skyFile*)fptr;
	int32     numBytesToWrite = size * count;

	Dbg_AssertPtr( addr );
	Dbg_AssertPtr( fptr );

	bytesWritten = sceWrite( fp->gdfs, (void*)addr, numBytesToWrite );

	if (bytesWritten != -1)
	{
		fp->POS += bytesWritten;
		if (fp->POS > fp->SOF)
			fp->SOF = fp->POS;
		return (size>0?bytesWritten/size:0);
	}
	return (0);
}

/******************************************************************/
/* skyFseek                                                       */
/*                                                                */
/* On entry   : file to seek in, offset, how to seek	  		  */
/* On exit    : success/failure                                   */
/*                                                                */
/******************************************************************/

static int skyFseek(void *fptr, long offset, int origin)
{
	

	skyFile      *fp = (skyFile *)fptr;

	Dbg_AssertPtr( fptr );

	StopStreaming( );

#if ASYNC_HOST_FILESYS
	fp->p_async_file->Seek(offset, origin);
	fp->POS = fp->p_async_file->WaitForIO();
#else
	int32      oldFPos, bufStart;
	bool       noBuffer = FALSE;

	oldFPos = fp->POS;
	bufStart = oldFPos - fp->bufferPos;
	if (fp->bufferPos == READBUFFERSIZE) noBuffer = TRUE;
	fp->bufferPos = READBUFFERSIZE;

	switch (origin)
	{
		case SEEK_CUR:
		{            
			/* Does the seek stay in the buffer */
			if ((oldFPos + offset >= bufStart) &&
				(oldFPos + offset < bufStart+READBUFFERSIZE))
			{
				fp->bufferPos = (oldFPos + offset) - bufStart;
				fp->POS += offset;
			}
			else
			{
				fp->POS = sceLseek(fp->gdfs, oldFPos + offset, SCE_SEEK_SET);
			}
			break;
		}
		case SEEK_END:
		{
			fp->POS = sceLseek(fp->gdfs, offset, SCE_SEEK_END);
			break;
		}
		case SEEK_SET:
		{
			fp->POS = sceLseek(fp->gdfs, offset, SCE_SEEK_SET);
			break;
		}
		default:
		{
			return (-1);
		}
	}

	if (noBuffer)
		fp->bufferPos = READBUFFERSIZE;

	if (fp->POS == -1)
	{
		/* This may not be valid */
		fp->POS = oldFPos;
		fp->bufferPos = READBUFFERSIZE;
		return (-1);
	}
#endif
	return (0);
}

/******************************************************************/
/* skyFgets                                                       */
/*                                                                */
/* On entry   : Buffer to read into, max chars to read, file	  */
/* On exit    : Non negative value on success                     */
/*                                                                */
/******************************************************************/

static char * skyFgets(char *buffer, int maxLen, void *fptr)
{
	

	skyFile            *fp = (skyFile *) fptr;
	int32             i;
	int32             numBytesRead;

	Dbg_AssertPtr( buffer );
	Dbg_AssertPtr( fptr );

	i = 0;

	numBytesRead = skyFread(buffer, 1, maxLen - 1, fp);

	if (numBytesRead == 0)
	{
		return (NULL);
	}

	while (i < numBytesRead)
	{
		if (buffer[i] == '\n')
		{
			i++;

			buffer[i] = '\0';

			/* the file pointer needs */
			/* to be reset as skyFread */
			/* will have overshot the */
			/* first new line         */

			i -= numBytesRead;
			skyFseek(fp, i, SEEK_CUR);

			return (buffer);
		}
		else if ( buffer[i] == 0x0D )
		{
			if ((i < (numBytesRead - 1)) && (buffer[i + 1] == '\n'))
			{
				memcpy(&buffer[i], &buffer[i + 1], (numBytesRead - i - 1));
				numBytesRead--;
			}
			else
				i++;
		}
		else
			i++;
	}

	/*
	 * Don't return NULL because we could have read maxLen bytes
	 * without finding a \n
	 */
	return (buffer);
}

/******************************************************************/
/* skyFputs                                                       */
/*                                                                */
/* On entry   : Buffer to write from, file to write to            */
/* On exit    : Non negative value on success                     */
/*                                                                */
/******************************************************************/

static int skyFputs( const char *buffer, void *fptr)
{
	

	skyFile   *fp = (skyFile *)fptr;
	int i, j;

	Dbg_AssertPtr( buffer );
	Dbg_AssertPtr( fptr );

	i = strlen(buffer);
	j = sceWrite(fp->gdfs, (void*)buffer, i);

	if (j != -1)
	{
		fp->POS += j;
		if (fp->POS > fp->SOF)
			fp->SOF = fp->POS;
	}
	if ((j == -1) || (i != j))
	{
		return (EOF);
	}
	return (j);
}

/******************************************************************/
/* skyFeof                                                        */
/*                                                                */
/* On entry   : File to test for eof                              */
/* On exit    : Non zero if end of file reached                   */
/*                                                                */
/******************************************************************/

static int skyFeof( void* fptr )
{
	

	skyFile  *fp = (skyFile*)fptr;

	Dbg_AssertPtr( fptr );

	return ( fp->POS >= fp->SOF) ;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static int skyFflush( void* )
{
	

	return 0;
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

long GetFileSize(void *pFP)
{
	
	Dbg_MsgAssert(pFP,("NULL pFP sent to GetFileSize"));

#if ASYNC_HOST_FILESYS
	skyFile      *fp = (skyFile *)pFP;
	if (!Config::CD() && fp->p_async_file)
	{
		return fp->p_async_file->GetFileSize();
	}
#endif

    if (PreMgr::sPreEnabled())
    {
        int retval = PreMgr::pre_get_file_size((PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	return ((skyFile*)pFP)->SOF;
}

long GetFilePosition(void *pFP)
{
	
	Dbg_MsgAssert(pFP,("NULL pFP sent to GetFilePosition"));

    if (PreMgr::sPreEnabled())
    {
        int retval = PreMgr::pre_get_file_position((PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	return ((skyFile*)pFP)->POS;
}

float GetPercentageRead(void *pFP)
{
	Dbg_MsgAssert(pFP,("NULL pFP sent to GetPercentageRead"));


    if (PreMgr::sPreEnabled())
    {
        int position = PreMgr::pre_get_file_position((PreFile::FileHandle *) pFP);
        int size = PreMgr::pre_get_file_size((PreFile::FileHandle *) pFP);

        if (PreMgr::sPreExecuteSuccess())
			return 100.0 * position / size;
    }

	return 100.0*((skyFile*)pFP)->POS/((skyFile*)pFP)->SOF;
}
	
void InstallFileSystem( void )
{
	// cd builds can only have one file open at any given time
	int max_open_files=Config::CD() ? 1:MAXOPENFILES;


	// initialize the locks on the sky files
	for ( int i = 0; i < max_open_files; i++ )
	{
		g_skyFile[i].locked = false;
	}
	
    
	if (Config::CD())
	{
		int	disk_type = SCECdDVD;
	
		// Initialise the CD
		printf("Initialising CD ...\n");
	//	int err1 = 
		sceCdInit(SCECdINIT);
		
		//int err2 = sceCdMmode(SCECdDVD);
	//	int err2 = 
		sceCdMmode(disk_type);
	
	
		// find the disk type, this might be different from what we intialized to 
		printf(" sceCdGetDiskType   ");
		int detected_disk_type= sceCdGetDiskType();
		switch(detected_disk_type)
		{
			case SCECdIllgalMedia:
			printf("Disk Type= IllgalMedia\n"); break;
			case SCECdPS2DVD:
			printf("Disk Type= PlayStation2 DVD\n"); break;
			case SCECdPS2CD:
			printf("Disk Type= PlayStation2 CD\n"); break;
			case SCECdPS2CDDA:
			printf("Disk Type= PlayStation2 CD with CDDA\n"); break;
			case SCECdPSCD:
			printf("Disk Type= PlayStation CD\n"); break;
			case SCECdPSCDDA:
			printf("Disk Type= PlayStation CD with CDDA\n"); break;
			case SCECdDVDV:
			printf("Disk Type= DVD video\n"); break;
			case SCECdCDDA:
			printf("Disk Type= CD-DA\n"); break;
			case SCECdDETCT:
			printf("Working\n"); break;
			case SCECdNODISC: 
			printf("Disk Type= No Disc\n"); break;
			default:
			printf("Disk Type= OTHER DISK\n"); break;
		}
		
		// If disk type has changed to a CD, then need to re-initialize it							   
		if (detected_disk_type == SCECdPS2CD)
		{
			#if __DVD_ONLY__
			printf( "*** ERROR - CD Detected, needs DVD.\n" );
			#else
			printf( "reinitializing for Ps2CD\n" );
			disk_type = SCECdCD;
			sceCdMmode(disk_type);
			printf( "done reinitializing\n" );
			#endif		
		}
		
	
	//	printf("'tis done. errs = %d, %d\n",err1,err2);
		
	
	
						   
						   
		// This next bit is essential for when making a bootable disc, ie one that will
		// boot on the actual PS rather than just the dev system.
	//#ifdef __NOPT_BOOTABLE__
		// K: Commented out __NOPT_BOOTABLE__, since it is set when __NOPT_CDROM__OLD is set.
	
		/* Reboot IOP, replace default modules  */
		char  path[128];
//		sprintf(path,"cdrom0:\\%sIOP\\IOPRP260.IMG;1",Config::GetDirectory()); 
//		sprintf(path,"host0:\\SKATE5\\DATA\\IOPMODULES\\DNAS280.IMG;1");
		
		
		sprintf(path,"cdrom0:\\%sIOP\\DNAS280.IMG;1",Config::GetDirectory()); 
		while ( !sceSifRebootIop((const char*) path) ); /* (Important) Unlimited retries */
		while( !sceSifSyncIop() );
		
		
		/* Reinitialize */
		sceSifInitRpc(0);
		sceCdInit(SCECdINIT);
		
	//    sceCdMmode(SCECdDVD);   /* Media: CD-ROM */
		sceCdMmode(disk_type);   /* Media: CD-ROM */
		
		sceFsReset();
	//#endif // __NOPT_BOOTABLE__

		printf("Opening hed file ...\n");
		// Open the hed file and load it into memory.
		gpHed = LoadHed( "SKATE5");
		
	/*	
		printf ("Size = %d, adjusted = %d\n",HedSize,(HedSize+2047)&~204);
	
		for (int i = HedSize; i< (HedSize+2047)&~204; i++)
		{
			if (p[i] != 0x55)
			{
				printf ("Overrun ? %d = %2x\n",i,p[i]);
				break;
			}
		}
	*/
		printf("Opening wad file ...\n");												 	
		// Open the wad and keep it open for future reference.	
		Dbg_MsgAssert(WadId==-1,("WadId not -1 ?"));					   
		while (WadId<0)
		{
			char  path[128];
			sprintf(path,"cdrom0:\\%sSKATE5.WAD;1",Config::GetDirectory()); 
			WadId=sceOpen(path,SCE_RDONLY);
			if (WadId<0)
			{
				printf("Retrying opening SKATE5.wad ...\n");
			}
		}		
		printf( "opened successfully\n" );
	}   
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

void UninstallFileSystem( void )
{
	if (Config::CD())
	{
		// Free the hed file buffer.
		Dbg_MsgAssert(gpHed,("NULL gpHed ?"));
		Mem::Free(gpHed);
		gpHed=NULL;
		
		Dbg_MsgAssert(WadId>=0,("Bad WadId"));
		sceClose(WadId);
		WadId=-1;
	
		// free IOP memory:
		if ( gFilesysIOPStreamBuffer )
		{
			sceSifFreeIopHeap( gFilesysIOPStreamBuffer ) ;
			gFilesysIOPStreamBuffer = 0;
		}
	}
}

void InitQuickFileSystem( void )
{

	// get the LSN to allow us to figure out where every other file is...
	sceCdlFILE wadInfo;
	char  path[128];
	sprintf(path,"\\%sSKATE5.WAD;1",Config::GetDirectory()); 
	while ( !sceCdSearchFile( &wadInfo, path ) )
	{
		printf("Retrying finding SKATE5.wad ...\n");
	}
	//printf( "after search file\n" );
	gWadLSN = wadInfo.lsn;

#if DISABLE_QUICK_FILESYSTEM
	return;
#endif

#if ASYNC_QUICK_FILESYS == 0
	// allocate our streaming buffer (iop side) for files:
	//printf( "before alloc heap\n" );	

	gFilesysIOPStreamBuffer = sceSifAllocIopHeap( FILESYS_IOP_BUFFER_SIZE );
	
	//printf( "after alloc heap: buffer %x of size %d\n", gFilesysIOPStreamBuffer, FILESYS_IOP_BUFFER_SIZE );	

	Dbg_MsgAssert( gFilesysIOPStreamBuffer,( "IOP stream buffer allocation failed." ));

	//printf( "before stream init\n" );	
	// initialize streaming capabilities and shit...
	while ( !sceCdStInit( FILESYS_NUM_SECTORS_IN_BUFFER, FILESYS_STREAM_BUFFER_NUM_PARTITIONS, (unsigned int)gFilesysIOPStreamBuffer ) )
	{
		printf( "trying to init CD stream\n" );
	}
	//printf( "after stream init\n" );

	StartStreaming( gWadLSN );
#endif

	gQuickFileSystemInitialzed = true;

	//printf( "after StartStreaming()\n" );
} // end of InitQuickFileSystem( )	


// We need to execute this after we have used the Cd Stream calls in other routines
void ResetQuickFileSystem( void )
{
#if ASYNC_QUICK_FILESYS == 0
	Dbg_MsgAssert( gFilesysIOPStreamBuffer,( "IOP stream buffer not initialized" ));

	// initialize streaming capabilities and shit...
	while ( !sceCdStInit( FILESYS_NUM_SECTORS_IN_BUFFER, FILESYS_STREAM_BUFFER_NUM_PARTITIONS, (unsigned int)gFilesysIOPStreamBuffer ) )
	{
		printf( "trying to init CD stream\n" );
	}
#endif
}

////////////////////////////////////////////////////////////////////
// Our versions of the ANSI file IO functions.  They call
// the PreMgr first to see if the file is in a PRE file.
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// Exist                                                          
bool Exist( const char *filename )
{
    if (PreMgr::sPreEnabled())
    {
        bool retval = PreMgr::pre_fexist(filename);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	if (Config::CD())
	{
		return cdFexist(filename);
	}
	else
	{
		return skyFexist(filename);
	}	
}

////////////////////////////////////////////////////////////////////
// Open                                                          
void * Open( const char *filename, const char *access )
{
	bool use_pre = true;
#if ASYNC_HOST_FILESYS
	use_pre = Config::CD();
#endif

	// Don't use pre files if writing to disk (eg when writing parks)
	if (access[0]=='w')
	{
		use_pre=false;
	}
		
	if (Script::GetInt(CRCD(0xe99935c2,"show_filenames"),false))
	{
		printf (".... Open %s\n",filename);
	}


    if (use_pre && PreMgr::sPreEnabled())
    {
        void * retval = PreMgr::pre_fopen(filename, access);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	if (Config::CD())
	{
		return cdFopen(filename, access);
	}
	else
	{
		return skyFopen(filename, access);
	}	
}

////////////////////////////////////////////////////////////////////
// Close                                                          
int Close( void *pFP )
{
	bool use_pre = true;
#if ASYNC_HOST_FILESYS
	use_pre = Config::CD();
#endif

    if (use_pre && PreMgr::sPreEnabled())
    {
        int retval = PreMgr::pre_fclose((PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	if (Config::CD())
	{
		return cdFclose(pFP);
	}
	else
	{
		return skyFclose(pFP);
	}	
}

////////////////////////////////////////////////////////////////////
// Read                                                          
size_t Read( void *addr, size_t size, size_t count, void *pFP )
{
	bool use_pre = true;
#if ASYNC_HOST_FILESYS
	use_pre = Config::CD();
#endif

    if (use_pre && PreMgr::sPreEnabled())
    {
        size_t retval = PreMgr::pre_fread(addr, size, count, (PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	if (CAsyncFileLoader::sAsyncInUse())
	{
		CAsyncFileLoader::sWaitForIOEvent(true);
		Dbg_Message("************************ Can't do a normal read when async filesystem is in use");
		//Dbg_MsgAssert(0, ("Can't do a normal read when async filesystem is in use"));
	}

	if (Config::CD())
	{
		return cdFread(addr, size, count, pFP);
	}
	else
	{
		return skyFread(addr, size, count, pFP);
	}	
}

///////////////////////////////////////////////////////////////////////
// Read an Integer in PS2 (littleendian) format
// we just read it directly into memory...
size_t ReadInt( void *addr, void *pFP )
{
	return Read(addr,4,1,pFP);	
}

////////////////////////////////////////////////////////////////////
// Write                                                          
size_t Write( const void *addr, size_t size, size_t count, void *pFP )
{
    if (PreMgr::sPreEnabled())
    {
        size_t retval = PreMgr::pre_fwrite(addr, size, count, (PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	if (Config::CD())
	{
		return cdFwrite(addr, size, count, pFP);
	}
	else
	{
		return skyFwrite(addr, size, count, pFP);
	}	
}

////////////////////////////////////////////////////////////////////
// GetS                                                          
char * GetS( char *buffer, int maxlen, void *pFP )
{
    if (PreMgr::sPreEnabled())
    {
        char * retval = PreMgr::pre_fgets(buffer, maxlen, (PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	if (Config::CD())
	{
		return cdFgets(buffer, maxlen, pFP);
	}
	else
	{
		return skyFgets(buffer, maxlen, pFP);
	}	
}

////////////////////////////////////////////////////////////////////
// PutS                                                          
int PutS( const char *buffer, void *pFP )
{
    if (PreMgr::sPreEnabled())
    {
        int retval = PreMgr::pre_fputs(buffer, (PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	if (Config::CD())
	{
		return cdFputs(buffer, pFP);
	}	
	else
	{
		return skyFputs(buffer, pFP);
	}	
}

////////////////////////////////////////////////////////////////////
// Eof                                                          
int Eof( void *pFP )
{
	bool use_pre = true;
#if ASYNC_HOST_FILESYS
	use_pre = Config::CD();
#endif

    if (use_pre && PreMgr::sPreEnabled())
    {
        int retval = PreMgr::pre_feof((PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	if (Config::CD())
	{
		return cdFeof(pFP);
	}
	else
	{
		return skyFeof(pFP);
	}	
}

////////////////////////////////////////////////////////////////////
// Seek                                                          
int Seek( void *pFP, long offset, int origin )
{
	bool use_pre = true;
#if ASYNC_HOST_FILESYS
	use_pre = Config::CD();
#endif

    if (use_pre && PreMgr::sPreEnabled())
    {
        int retval = PreMgr::pre_fseek((PreFile::FileHandle *) pFP, offset, origin);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	if (Config::CD())
	{
		return cdFseek(pFP, offset, origin);
	}
	else
	{
		return skyFseek(pFP, offset, origin);
	}	
}

int32	Tell( void* pFP )
{
	skyFile* fp = (skyFile *) pFP;
	return fp->POS;
}

////////////////////////////////////////////////////////////////////
// Flush                                                          
int Flush( void *pFP )
{
	bool use_pre = true;
#if ASYNC_HOST_FILESYS
	use_pre = Config::CD();
#endif

    if (use_pre && PreMgr::sPreEnabled())
    {
        int retval = PreMgr::pre_fflush((PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	if (Config::CD())
	{
		return cdFflush(pFP);
	}
	else
	{
		return skyFflush(pFP);
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace File


