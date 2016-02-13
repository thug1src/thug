/*	Header file functionality...
	.Hed files that describe the contents of .Wad files
	Written by Ken, stolen by Matt*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <core/macros.h>
#include <core/defines.h>

#include <gel/scripting/script.h> 

#include <sys/file/filesys.h>
#include <sys/file/xbox/hed.h>

namespace File
{

// Searches the hed file for the filename. If not found, returns NULL.
SHed *FindFileInHed(const char *pFilename, SHed *pHed)
{
	Dbg_MsgAssert(pFilename,("NULL pFilename"));
	Dbg_MsgAssert(pHed,("NULL pHed"));
	
	#define FILENAME_BUF_SIZE 200
	char pBuf[FILENAME_BUF_SIZE];
	Dbg_MsgAssert(strlen(pFilename)<FILENAME_BUF_SIZE,("File name too long."));
	// Make sure that pBuf starts with a backslash, since the names listed in
	// the hed file all do.
	if (pFilename[0]!='\\' && pFilename[0]!='/')
	{
		pBuf[0]='\\';
		pBuf[1]=0;
	}
	else
	{
		pBuf[0]=0;
	}
	// Append the passed filename.
	strcat(pBuf,pFilename);
	// Scan through and convert all forward slashes to backslashes since that's
	// how they're listed in the hed.
	char *pCh=pBuf;
	while (*pCh)
	{
		if (*pCh=='/')
		{
			*pCh='\\';
		}
		++pCh;
	}				
	
	// Search the hed until the file is found, or not.
	SHed *pHd=pHed;
	while (pHd->Offset!=0xffffffff)
	{						  
		if (stricmp(pBuf,pHd->pFilename)==0)
		{
			return pHd;
		}
		
		// Get the total space occupied by the file name, which is
		// the length +1 (to include the terminator) then rounded up
		// to a multiple of 4.
		int Len=strlen(pHd->pFilename)+1;
		Len=(Len+3)&~3;
		pHd=(SHed*)(pHd->pFilename+Len);
	}
	return NULL;	
}

// Searches the hed file for the filename with the same checksum. If not found, returns NULL.
SHed *FindFileInHedUsingChecksum( uint32 checksum, SHed *pHed, bool stripPath )
{
	Dbg_MsgAssert(pHed,("NULL pHed"));
	
	// Search the hed until the file is found, or not.
	SHed *pHd=pHed;
	const char *pFilename;
	while (pHd->Offset!=0xffffffff)
	{
		pFilename = pHd->pFilename;
		if ( stripPath )
		{
			int i;
			for ( i = strlen( pFilename ) - 2; i >= 0; i-- )
			{
				if ( pFilename[ i ] == '\\' )
				{
					pFilename = &pFilename[ i + 1 ];
					i = -1;
				}
			}
		}
		if ( Script::GenerateCRC( pFilename ) == checksum )
		{
			return ( pHd );
		}
		// Get the total space occupied by the file name, which is
		// the length +1 (to include the terminator) then rounded up
		// to a multiple of 4.
		int Len=strlen(pHd->pFilename)+1;
		Len=(Len+3)&~3;
		pHd=(SHed*)(pHd->pFilename+Len);
	}
	return NULL;	
}



SHed *LoadHed( const char *filename )
{
	char tempFileName[255];
	sprintf( tempFileName, "%s.HED", filename );

	void *fp = File::Open( tempFileName, "rb" );
	if ( !fp )
	{
		Dbg_MsgAssert( 0,( "Couldn't find hed file %s", tempFileName ));
		return NULL;
	}
	int fileSize = File::GetFileSize( fp );
	File::Seek( fp, 0, SEEK_SET );
	
	SHed *pHed = (SHed*)Mem::Malloc(( fileSize + 2047 ) & ~2047 );
	Dbg_MsgAssert( pHed,( "Failed to allocate memory for hed %s", tempFileName ));
	
	File::Read( pHed, 1, fileSize, fp );
	File::Close( fp );
	return pHed;
}

} // namespace File
