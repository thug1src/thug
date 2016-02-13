/*****************************************************************************
**																			**
**			              Neversoft Entertainment	                        **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Sys Library												**
**																			**
**	Module:			Memory Manager (Mem)									**
**																			**
**	Created:		03/20/00	-	mjb										**
**																			**
**	File name:		core/sys/mem/alloc.h									**
**																			**
*****************************************************************************/

#ifndef	__SYS_FILE_FILESYS_H
#define	__SYS_FILE_FILESYS_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace File
{



/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

void InstallFileSystem( void );
void UninstallFileSystem( void );
long GetFileSize(void *pFP);
long GetFilePosition(void *pFP);
float GetPercentageRead(void *pFP);
void InitQuickFileSystem( void );
void ResetQuickFileSystem( void );

uint32	CanFileBeLoadedQuickly( const char *filename );
bool LoadFileQuicklyPlease( const char *filename, uint8 *addr );

void   *LoadAlloc(const char *p_fileName, int *p_filesize=NULL, void *p_dest = NULL, int maxSize=0);


void StopStreaming( void );

/////////////////////////////////////////////////////////////////////
// The following are our versions of the ANSI file IO functions.
//
bool   Exist( const char *filename );
void * Open( const char *filename, const char *access );
int    Close( void *pFP );
size_t Read( void *addr, size_t size, size_t count, void *pFP );
size_t ReadInt( void *addr, void *pFP );
size_t Write( const void *addr, size_t size, size_t count, void *pFP );
char * GetS( char *buffer, int maxlen, void *pFP );
int    PutS( const char *buffer, void *pFP );
int    Eof( void *pFP );
int    Seek( void *pFP, long offset, int origin );
int	   Tell( void* pFP );
int    Flush( void *pFP );


/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace File

#endif  // __SYS_FILE_FILESYS_H
