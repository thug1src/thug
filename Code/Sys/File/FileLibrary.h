//****************************************************************************
//* MODULE:         File
//* FILENAME:       FileLibrary.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  01/21/2003
//****************************************************************************

#ifndef __SYS_FILE_FILELIBRARY_H
#define __SYS_FILE_FILELIBRARY_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/support.h>

namespace File
{

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

	class CAsyncFileHandle;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

struct SFileInfo
{
	uint32	fileOffset;
	int		fileSize;
	uint32	fileNameChecksum;
	uint32	fileExtensionChecksum;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class CFileLibrary : public Spt::Class
{
protected:
	enum
	{
		vMAX_LIB_FILES = 128
	};

public:
	CFileLibrary();
	virtual 			~CFileLibrary();

public:
	bool 				Load( const char* p_fileName, bool assertOnFail, bool async_load );
	bool				PostLoad( bool assertOnFail, int file_size );
	bool				LoadFinished() const;

public:
	int					GetNumFiles() const;
	uint32*				GetFileData( int index );
	uint32*				GetFileData( uint32 name, uint32 extension, bool assertOnFail = true );
	const SFileInfo*	GetFileInfo( int index ) const;
	const SFileInfo*	GetFileInfo( uint32 name, uint32 extension, bool assertOnFail = true ) const;
	bool				ClearFile( uint32 name, uint32 extension );
	bool				FileExists( uint32 name, uint32 extension );

protected:
	CAsyncFileHandle*	mp_fileHandle;
#ifdef __PLAT_NGC__
	uint32				m_aramOffset;
#else
	uint8* 				mp_baseBuffer;
	uint8* 				mp_fileBuffer;
#endif		// __PLAT_NGC__
	int					m_numFiles;
	int					m_originalNumFiles;
	SFileInfo			m_fileInfo[vMAX_LIB_FILES];
#ifdef __PLAT_NGC__
	uint32				m_fileOffsets[vMAX_LIB_FILES];
#endif		// __PLAT_NGC__
	uint32*				mp_filePointers[vMAX_LIB_FILES];
	bool				m_dataLoaded;
};

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

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace File

#endif	// __SYS_FILE_FILELIBRARY_H


