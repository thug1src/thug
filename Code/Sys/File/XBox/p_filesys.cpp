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

#include <xtl.h>
#include <stdio.h>
#include <string.h>

#include <core/defines.h>
#include <core/support.h>
#include <sys/file/filesys.h>
#include <sys/file/AsyncFilesys.h>
#include <sys/file/PRE.h>
#include <sys/file/xbox/p_streamer.h>
#include <sys/config/config.h>

#include <gfx/xbox/nx/nx_init.h>

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

#define READBUFFERSIZE				10240
#define	PREPEND_START_POS			8

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

BOOL OkayToUseUtilityDrive = FALSE;

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
static void* prefopen( const char *filename, const char *mode )
{
	// Used for prepending the root data directory on filesystem calls.
	char		nameConversionBuffer[256] = "d:\\data\\";
	int			index = PREPEND_START_POS;
	const char*	p_skip;

	if(( filename[0] == 'c' ) && ( filename[1] == ':' ))
	{
		// Filename is of the form c:\skate4\data\foo...
		p_skip = filename + 15;
	}
	else
	{
		// Filename is of the form foo...
		p_skip = filename;
	}

	while( nameConversionBuffer[index] = *p_skip )
	{
		// Switch forward slash directory separators to the supported backslash.
		if( nameConversionBuffer[index] == '/' )
		{
			nameConversionBuffer[index] = '\\';
		}
		++index;
		++p_skip;
	}
	nameConversionBuffer[index] = 0;

	// If this is a .tex file, switch to a .txx file.
	if((( nameConversionBuffer[index - 1] ) == 'x' ) &&
	   (( nameConversionBuffer[index - 2] ) == 'e' ) &&
	   (( nameConversionBuffer[index - 3] ) == 't' ))
	{
		nameConversionBuffer[index - 2] = 'x';
	}

	// If this is a .pre file, switch to a .prx file.
	if(((( nameConversionBuffer[index - 1] ) == 'e' ) || (( nameConversionBuffer[index - 1] ) == 'x' )) &&
	    (( nameConversionBuffer[index - 2] ) == 'r' ) &&
	    (( nameConversionBuffer[index - 3] ) == 'p' ))
	{
#		ifdef __PAL_BUILD__
		// Switch to a .prf, .prg or .prx file, depending on language.
		switch( Config::GetLanguage())
		{
			case Config::LANGUAGE_FRENCH:
			{
				nameConversionBuffer[index - 1] = 'f';
				break;
			}
			case Config::LANGUAGE_GERMAN:
			{
				nameConversionBuffer[index - 1] = 'g';
				break;
			}
			default:
			{
				nameConversionBuffer[index - 1] = 'x';
				break;
			}
		}
#		else
		nameConversionBuffer[index - 1] = 'x';
#		endif // __PLAT_BUILD__
	}

	// First we try reading the file from the utility partition (z:\) on the HD, rather than the DVD.
	HANDLE h_file = INVALID_HANDLE_VALUE;
	if( OkayToUseUtilityDrive )
	{
		nameConversionBuffer[0] = 'Z';
		h_file = CreateFile( nameConversionBuffer, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	}

	if( h_file == INVALID_HANDLE_VALUE )
	{
		// Not on the utility partition, so load it from the DVD.
		nameConversionBuffer[0] = 'D';
		h_file = CreateFile( nameConversionBuffer, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

		// Deal with various error returns.
		if( h_file == INVALID_HANDLE_VALUE )
		{
			DWORD error = GetLastError();

			// Need to exclude this error from the test, since screenshot and other code sometimes check to see if a file exists, and it
			// is valid to just return the error code if it doesn't.
			if( error != ERROR_FILE_NOT_FOUND )
			{
				// Catch-all error indicating a fatal problem. Can't continue at this point.
				// The ideal solution would be a catch/throw exception mechanism, but we don't include exception handling at the moment.
				// For now just call this NxXbox function, which is slightly messy since it means we have to include a gfx\ file.
				printf( "FatalFileError: %x %s\n", error, nameConversionBuffer );
				NxXbox::FatalFileError((uint32)INVALID_HANDLE_VALUE );
			}
			return NULL;
		}
		else
		{
			// All is well.
			return h_file;
		}
	}
	else
	{
		return h_file;
	}
}


/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

static CThreadedLevelLoader *pLoader;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void InstallFileSystem( void )
{
#	if 0
	// This is where we start the thread that will deal with copying commonly used data from the DVD to the utility
	// region (z:\) on the HD.
	pLoader = new CThreadedLevelLoader();

	SLevelDesc level_descs[] = {{ "pre\\alc.prx" },
								{ "pre\\alccol.prx" },
								{ "pre\\alcscn.prx" },
								{ "pre\\anims.prx" },
								{ "pre\\bits.prx" },
								{ "pre\\cnv.prx" },
								{ "pre\\cnvcol.prx" },
								{ "pre\\cnvscn.prx" },
								{ "pre\\fonts.prx" },
								{ "pre\\jnk.prx" },
								{ "pre\\jnkcol.prx" },
								{ "pre\\jnkscn.prx" },
								{ "pre\\kon.prx" },
								{ "pre\\koncol.prx" },
								{ "pre\\konscn.prx" },
								{ "pre\\lon.prx" },
								{ "pre\\loncol.prx" },
								{ "pre\\lonscn.prx" },
								{ "pre\\qb.prx" },
								{ "pre\\sch.prx" },
								{ "pre\\schcol.prx" },
								{ "pre\\schscn.prx" },
								{ "pre\\sf2.prx" },
								{ "pre\\sf2col.prx" },
								{ "pre\\sf2scn.prx" },
								{ "pre\\skaterparts.prx" },
								{ "pre\\skater_sounds.prx" },
								{ "pre\\skateshop.prx" },
								{ "pre\\skateshopcol.prx" },
								{ "pre\\skateshopscn.prx" },
								{ "pre\\skeletons.prx" },
								{ "pre\\zoo.prx" },
								{ "pre\\zoocol.prx" },
								{ "pre\\zooscn.prx" }};

	static BYTE data_buffer[32 * 1024];
	pLoader->Initialize( level_descs, 34, data_buffer, 32 * 1024, &OkayToUseUtilityDrive );
	pLoader->AsyncStreamLevel( 0 );
#	endif

	// Initialise the async file system.
	File::CAsyncFileLoader::sInit();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
long GetFileSize( void* pFP )
{
	Dbg_MsgAssert( pFP, ( "NULL pFP sent to GetFileSize" ));

	if( PreMgr::sPreEnabled())
    {
		int retval = PreMgr::pre_get_file_size( (PreFile::FileHandle *) pFP );
		if( PreMgr::sPreExecuteSuccess())
			return retval;
	}
	
	LARGE_INTEGER	li;
	GetFileSizeEx((HANDLE)pFP, &li );
	return (long)li.LowPart;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
long GetFilePosition( void *pFP )
{
	Dbg_MsgAssert( pFP, ( "NULL pFP sent to GetFilePosition" ));

	if( PreMgr::sPreEnabled())
	{
		int retval = PreMgr::pre_get_file_position((PreFile::FileHandle*)pFP );
		if( PreMgr::sPreExecuteSuccess())
			return retval;
	}

	long pos = SetFilePointer(	(HANDLE)pFP,	// Handle to file
								0,				// Bytes to move pointer
								0,				// High bytes to move pointer
								FILE_CURRENT );	// Starting point
	return pos;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void InitQuickFileSystem( void )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint32	CanFileBeLoadedQuickly( const char* filename )
{
	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool LoadFileQuicklyPlease( const char* filename, uint8 *addr )
{
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void StopStreaming( void )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void UninstallFileSystem( void )
{
}



////////////////////////////////////////////////////////////////////
// Our versions of the ANSI file IO functions. They call
// the PreMgr first to see if the file is in a PRE file.
////////////////////////////////////////////////////////////////////

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool Exist( const char *filename )
{
    if (PreMgr::sPreEnabled())
    {
        bool retval = PreMgr::pre_fexist(filename);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	void *p_result = prefopen( filename, "rb" );
	if( p_result != NULL )
	{
		Close( p_result );
	}

	return( p_result != NULL );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void * Open( const char *filename, const char *access )
{
    if (PreMgr::sPreEnabled())
    {
        void * retval = PreMgr::pre_fopen(filename, access);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	return prefopen( filename, access );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int Close( void *pFP )
{
    if (PreMgr::sPreEnabled())
    {
        int retval = PreMgr::pre_fclose((PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }

	CloseHandle((HANDLE)pFP );

	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
size_t Read( void *addr, size_t size, size_t count, void *pFP )
{
    if (PreMgr::sPreEnabled())
    {
        size_t retval = PreMgr::pre_fread( addr, size, count, (PreFile::FileHandle*)pFP );
		if( PreMgr::sPreExecuteSuccess())
            return retval;
    }

	DWORD bytes_read;
	if( ReadFile((HANDLE)pFP, addr, size * count, &bytes_read, NULL ))
	{
		// All is well.
		return bytes_read;
	}
	else
	{
		// Read error.
		DWORD last_error = GetLastError();

		if( last_error == ERROR_HANDLE_EOF )
		{
			// Continue in this case.
			return bytes_read;
		}

		NxXbox::FatalFileError( last_error );
		return bytes_read;
	}
}



/******************************************************************/
/*                                                                */
/* Read an Integer in little endian format. Just read it directly */
/* into memory...												  */
/*                                                                */
/******************************************************************/
size_t ReadInt( void *addr, void *pFP )
{
	return Read( addr, 4, 1, pFP );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
size_t Write( const void *addr, size_t size, size_t count, void *pFP )
{
    if (PreMgr::sPreEnabled())
    {
        size_t retval = PreMgr::pre_fwrite(addr, size, count, (PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }
	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
char * GetS( char *buffer, int maxlen, void *pFP )
{
    if (PreMgr::sPreEnabled())
    {
        char * retval = PreMgr::pre_fgets(buffer, maxlen, (PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }
	return NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int PutS( const char *buffer, void *pFP )
{
    if (PreMgr::sPreEnabled())
    {
        int retval = PreMgr::pre_fputs(buffer, (PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }
	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int Eof( void *pFP )
{
    if (PreMgr::sPreEnabled())
    {
        int retval = PreMgr::pre_feof((PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }
	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int Seek( void *pFP, long offset, int origin )
{
	if( PreMgr::sPreEnabled())
    {
        int retval = PreMgr::pre_fseek((PreFile::FileHandle *) pFP, offset, origin);
		if( PreMgr::sPreExecuteSuccess())
			return retval;
	}
	return SetFilePointer((HANDLE)pFP, offset, NULL, ( origin == SEEK_CUR ) ? FILE_CURRENT : (( origin == SEEK_SET ) ? FILE_BEGIN : FILE_END ));
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int Flush( void *pFP )
{
    if (PreMgr::sPreEnabled())
    {
        int retval = PreMgr::pre_fflush((PreFile::FileHandle *) pFP);
        if (PreMgr::sPreExecuteSuccess())
            return retval;
    }
	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace File


