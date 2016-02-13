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
**	Created by:		09/25/00	-	dc										**
**																			**
**	Description:	XBox File System										**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <stdio.h>
#include <string.h>

#include <core/defines.h>
#include <core/support.h>
#include <sys/file/filesys.h>
#include <sys/file/PRE.h>

#include <sys/ngc/p_file.h>

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

#define CDROMx   // Use this define to get CDROM behavior

#define RWGDFSGLOBAL(var)	(RWPLUGINOFFSET(gdfsGlobals, RwEngineInstance, gdfsModuleInfo.globalsOffset)->var)

#define	PREPEND_START_POS	8

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

//#define READBUFFERSIZE (10240*5)

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
//	uint8			readBuffer[READBUFFERSIZE];
	uint32		bufferPos;
	bool			bufferValid;
	bool			locked;			// whether the skyfile is currently in use
	
	// Used when CD
	uint32			WadOffset;
	const char		*pFilename;
	
	// Used when non-CD
	#define MAX_PFILESYS_NAME_SIZE 100
	char filename[ MAX_PFILESYS_NAME_SIZE ];

	NsFile ngcFile;
};

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

//static	RwFileFunctions	s_old_fs;
//static	RwInt32			s_open_files = 0;

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
/* prefopen														  */
/* Same as fopen() except it will prepend the data root directory */
/* For Xbox, path is always relative to location of .xbe image	  */
/* and default path from that location is d:\					  */
/* So incoming files of the form:								  */
/* c:\skate3\data\foo...	become	d:\data\foo...				  */
/* foo...					become	d:\data\foo...				  */
/*                                                                */
/******************************************************************/

//static void* prefopen( const char *filename, const char *mode )
//{
//	// Used for prepending the root data directory on filesystem calls.
//	char		nameConversionBuffer[256] = "d:\\data\\";
//	int			index = PREPEND_START_POS;
//	const char*	p_skip;
//
//	if(( filename[0] == 'c' ) && ( filename[1] == ':' ))
//	{
//		// Filename is of the form c:\skate3\data\foo...
//		p_skip = filename + 15;
//	}
//	else
//	{
//		// Filename is of the form foo...
//		p_skip = filename;
//	}
//
//	while( nameConversionBuffer[index] = *p_skip )
//	{
//		// Switch forward slash directory separators to the supported backslash.
//		if( nameConversionBuffer[index] == '/' )
//		{
//			nameConversionBuffer[index] = '\\';
//		}
//		++index;
//		++p_skip;
//	}
//	nameConversionBuffer[index] = 0;
//
//	// Final hack - if this is a .tex file, switch to a .txx file.
//	if((( nameConversionBuffer[index - 1] ) == 'x' ) &&
//	   (( nameConversionBuffer[index - 2] ) == 'e' ) &&
//	   (( nameConversionBuffer[index - 3] ) == 't' ))
//	{
//		nameConversionBuffer[index - 2] = 'x';
//	}
//
////	return fopen( nameConversionBuffer, mode );
//
//	HANDLE h_file = CreateFile( nameConversionBuffer, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
//	return ( h_file == INVALID_HANDLE_VALUE ) ? NULL : h_file;
//}


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
	int max_open_files=MAXOPENFILES;
	
	int i;
	// this tries to find an unlocked skyfile
	for ( i = 0; i < max_open_files; i++ )
	{
		if ( !g_skyFile[i].locked )
		{
			g_skyFile[i].locked = true;
			g_skyFile[i].ngcFile.m_FileOpen			= 0; 
			g_skyFile[i].ngcFile.m_sizeCached		= 0; 
			g_skyFile[i].ngcFile.m_numCacheBytes	= 0; 
			g_skyFile[i].ngcFile.m_seekOffset		= 0;
			g_skyFile[i].ngcFile.m_preHandle		= NULL;
			g_skyFile[i].ngcFile.m_unique_tag		= 0x1234ABCD;
			return &g_skyFile[i];
		}
	}

	Dbg_Message( "Here are the files currently open:" );
	for ( i = 0; i < max_open_files; i++ )
	{
		Dbg_Message( "%s", g_skyFile[ i ].filename );
	}	

	// if we get here, that means that all the sky files were locked
	Dbg_MsgAssert( 0, ( "Trying to open too many files simultaneously (max=%d)", max_open_files ) );
	return NULL;
}

void unlock_skyfile( skyFile* pSkyFile )
{
	// cd builds can only have one file open at any given time
	int max_open_files=MAXOPENFILES;
	

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
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

long GetFileSize( void* pFP )
{
	Dbg_MsgAssert(pFP,("NULL pFP sent to GetFileSize"));

    if (PreMgr::sPreEnabled())
    {
        int retval = PreMgr::pre_get_file_size((PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	skyFile* fp = (skyFile*)pFP;
	return fp->ngcFile.size();
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

	skyFile* fp = (skyFile*)pFP;
	return fp->ngcFile.tell();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void InstallFileSystem( void )
{
/*
	RwFileFunctions*	fs = RwOsGetFileInterface();
	Dbg_MsgAssert( fs,( "File System not found" ));
	
	s_old_fs = *fs;    
    
	fs->rwfexist	= fexist;			
	fs->rwfopen		= (rwFnFopen)prefopen;
	fs->rwfclose	= (rwFnFclose)fclose;
	fs->rwfread		= (rwFnFread)fread;
	fs->rwfwrite	= (rwFnFwrite)fwrite;
	fs->rwfgets		= (rwFnFgets)fgets;
	fs->rwfputs		= (rwFnFputs)fputs;
	fs->rwfeof		= (rwFnFeof)feof;
	fs->rwfseek		= (rwFnFseek)fseek;
	fs->rwfflush	= (rwFnFflush)fflush;
	fs->rwftell		= (rwFnFtell)ftell;
*/
}

void InitQuickFileSystem( void )
{
}

uint32	CanFileBeLoadedQuickly( const char* filename )
{
	return 0;
}

bool LoadFileQuicklyPlease( const char* filename, uint8 *addr )
{
	return false;
}

void StopStreaming( void )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void UninstallFileSystem( void )
{
/*
	RwFileFunctions*	fs = RwOsGetFileInterface();

	Dbg_MsgAssert( fs,( "File System not found" ));

	*fs = s_old_fs;		// restore old
*/

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

	NsFile f;
	return f.exist( filename );
}

////////////////////////////////////////////////////////////////////
// Open                                                          
void * Open( const char *filename, const char *access )
{
    if (PreMgr::sPreEnabled())
    {
        void * retval = PreMgr::pre_fopen(filename, access);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	skyFile *fp = lock_skyfile();
	if ( fp )
	{
		strcpy( fp->filename, filename );
	
		if ( fp->ngcFile.open( filename ) )
		{
			fp->SOF = fp->ngcFile.size();
	
			return (void *)fp;
		}
		else
		{
			unlock_skyfile( fp );
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}

////////////////////////////////////////////////////////////////////
// Close                                                          
int Close( void *pFP )
{
    if (PreMgr::sPreEnabled())
    {
        int retval = PreMgr::pre_fclose((PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	skyFile* fp = (skyFile*)pFP;
	fp->ngcFile.close();

	unlock_skyfile( fp );
	return 0;
}

////////////////////////////////////////////////////////////////////
// Read                                                          
size_t Read( void *addr, size_t size, size_t count, void *pFP )
{
    if (PreMgr::sPreEnabled())
    {
        size_t retval = PreMgr::pre_fread(addr, size, count, (PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	skyFile* fp = (skyFile*)pFP;
	return fp->ngcFile.read( addr, size * count ); 
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
//    if (PreMgr::sPreEnabled())
//    {
//        size_t retval = PreMgr::pre_fwrite(addr, size, count, (PreFile::FileHandle *) pFP);
//        if (PreMgr::sPreExecuteSuccess())
//            return retval;
//    }
//
////    return skyFwrite(addr, size, count, pFP);
	return 0;
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

	skyFile* fp = (skyFile*)pFP;
	return fp->ngcFile.gets( buffer, maxlen ); 
}

////////////////////////////////////////////////////////////////////
// PutS                                                          
int PutS( const char *buffer, void *pFP )
{
//    if (PreMgr::sPreEnabled())
//    {
//        int retval = PreMgr::pre_fputs(buffer, (PreFile::FileHandle *) pFP);
//        if (PreMgr::sPreExecuteSuccess())
//            return retval;
//    }
//
////    return skyFputs(buffer, pFP);
	return 0;
}

////////////////////////////////////////////////////////////////////
// Eof                                                          
int Eof( void *pFP )
{
    if (PreMgr::sPreEnabled())
    {
        int retval = PreMgr::pre_feof((PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	skyFile* fp = (skyFile*)pFP;
	return fp->ngcFile.eof();
}

////////////////////////////////////////////////////////////////////
// Seek                                                          
int Seek( void *pFP, long offset, int origin )
{
	if( PreMgr::sPreEnabled())
    {
        int retval = PreMgr::pre_fseek((PreFile::FileHandle *) pFP, offset, origin);
		if( PreMgr::sPreExecuteSuccess())
			return retval;
	}
	skyFile* fp = (skyFile*)pFP;
	switch ( origin ) {
		case SEEK_SET:
			fp->ngcFile.seek( NsFileSeek_Start, offset );
			break;
		case SEEK_END:
			fp->ngcFile.seek( NsFileSeek_End, offset );
			break;
		case SEEK_CUR:
		default:
			fp->ngcFile.seek( NsFileSeek_Current, offset );
			break;
	}
	return fp->ngcFile.tell();
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
//    if (PreMgr::sPreEnabled())
//    {
//        int retval = PreMgr::pre_fflush((PreFile::FileHandle *) pFP);
//        if (PreMgr::sPreExecuteSuccess())
//            return retval;
//    }
//
////    return skyFflush(pFP);
	return 0;
}






/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace File



