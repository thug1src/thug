/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		SYS														**
**																			**
**	Module:			MemCard  (Mc)											**
**																			**
**	File name:		sys/mcman.h												**
**																			**
**	Created: 		03/06/2001	-	spg										**
**																			**
*****************************************************************************/

#ifndef __SYS_MCMAN_H
#define __SYS_MCMAN_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/singleton.h>
#ifdef __PLAT_NGC__
#include <dolphin.h>
#endif

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Mc
{

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class Card;

class DateTime
{
public:
	unsigned char 	m_Seconds;
	unsigned char	m_Minutes;
	unsigned char 	m_Hour;
	unsigned char 	m_Day;
	unsigned char 	m_Month;
	unsigned short	m_Year;
};

class File : public Lst::Node< File >
{
public:
	enum
	{
		mMODE_READ		= 0x0001,
		mMODE_WRITE		= 0x0002,
		mMODE_CREATE	= 0x0004,
	};

	enum FilePointerBase
	{
		BASE_START,
		BASE_CURRENT,
		BASE_END,
	};

	enum
	{
		mATTRIB_READABLE		= 0x0001,
		mATTRIB_WRITEABLE		= 0x0002,
		mATTRIB_EXECUTABLE		= 0x0004,
		mATTRIB_DUP_PROHIBITED	= 0x0008,
		mATTRIB_DIRECTORY		= 0x0010,
		mATTRIB_CLOSED			= 0x0020,
		mATTRIB_PDA_APP			= 0x0040,
		mATTRIB_PS1_FILE		= 0x0080,
	};
	enum
	{
		vMAX_FILENAME_LEN = 31,
		vMAX_DISPLAY_FILENAME_LEN	= 63
	};

					File( int fd, Card* card );
					~File();
					
		
	char			m_Filename[vMAX_FILENAME_LEN+1];
	DateTime		m_Created;	
	DateTime		m_Modified;
	unsigned int	m_Size;
	unsigned short	m_Attribs;

	bool			Close( void );
	int				Write( void* data, int len );
	int				Read( void* data, int len );
	int				Seek( int offset, FilePointerBase base );
	bool			Flush( void );

#	ifdef __PLAT_XBOX__
	char			m_DisplayFilename[vMAX_DISPLAY_FILENAME_LEN+1];
#	endif

#	ifdef __PLAT_NGC__
	CARDFileInfo	m_file_info;
	int				m_file_number;		// Set when opened.
	
	bool			m_need_to_flush;
	int				m_write_offset;
	uint8		   *mp_write_buffer;
	int				m_read_offset;
	uint8		   *mp_read_buffer;
#	endif

private:
	int				m_fd;
	Card*			m_card;
};

class  Card  : public Spt::Class
{
public:
	friend class Manager;

	enum
	{
		vERROR_FORMATTED,
		vERROR_UNFORMATTED,
		vINSUFFICIENT_SPACE,
		vINVALID_PATH,
		vNO_CARD,
		vACCESS_ERROR,
		vSUBDIR_NOT_EMPTY,
		vTOO_MANY_OPEN_FILES,
		vUNKNOWN,
	};

	enum
	{
		vDEV_NONE,
		vDEV_PS1_CARD,
		vDEV_PS2_CARD,
		vDEV_POCKET_STATION,
		vDEV_XBOX_CARD,
		vDEV_XBOX_HARD_DRIVE,
		vDEV_GC_CARD,
	};

	enum
	{
		vMAX_FILES_PER_DIRECTORY = 32,
	};

	bool			MakeDirectory( const char* dir_name );
	bool			DeleteDirectory( const char* dir_name );
	const char	   *ConvertDirectory( const char* dir_name );
	
	bool			ChangeDirectory( const char* dir_name );
	
	File*			Open( const char* filename, int mode, int size = 0 );	// Size used by NGC for file creation.
	
	bool			Delete( const char* filename );
	bool			Rename( const char* old_name, const char* new_name );

	bool			Format( void );
	bool			Unformat( void );
	bool			IsFormatted( void );
	
	int				GetSlot( void )			{ return m_slot; }
	int				GetDeviceType( void );
	int				GetNumFreeClusters( void );
	int				GetNumFreeEntries( const char* path );

	void			SetError( int error ) { m_last_error = error; }
	int				GetLastError( void ) { return m_last_error; }

	bool			GetFileList( const char* mask, Lst::Head< File > &file_list );

#	ifdef __PLAT_NGC__
	int				m_mem_size;				// Overall memory size of unit.
	int				m_sector_size;			// Sector size of unit.
	char*			mp_work_area;			// Used also as a flag: == NULL means not yet mounted.
	bool			m_broken;
	int 			CountFilesLeft();			// Required to check if there are enough files left to allow a game save.
	bool			IsForeign( void );
	bool			IsBadDevice( void );
#	endif

#	ifdef __PLAT_XBOX__
	void			SetAsHardDrive();
#	endif // #ifdef __PLAT_XBOX__

private:
	int				m_port;
	int				m_slot;
	int				m_last_error;

#	ifdef __PLAT_XBOX__
	char			m_mounted_drive_letter;	// Used also as a flag: == 0 means not yet mounted.
	enum
	{
		vDIRECTORY_NAME_BUF_SIZE=64
	};	
#	endif // #ifdef __PLAT_XBOX__
};

class  Manager  : public Spt::Class
{

public :
	enum
	{
#		ifdef __PLAT_XBOX__
		vMAX_PORT = 4,			// Four controllers...
		vMAX_SLOT = 2,			// ...each with 2 slots for memory cards.
#		elif defined __PLAT_NGC__
		vMAX_PORT = 1,			// Just plug them into the console itself, so 1 port...
		vMAX_SLOT = 1,			// ...with 2 slots for memory cards.
#		else
		vMAX_PORT = 2,
		vMAX_SLOT = 4,
#		endif // #ifdef __PLAT_XBOX__
	};

	int				GetMaxSlots( int port );
	Card*			GetCard( int port, int slot );
	Card*			GetCardEx( int port, int slot )	{ return &( m_card[port][slot] ); }
#	ifdef __PLAT_NGC__
	void			SetFatalError( bool error )		{ m_hasFatalError = error; }
	bool			GotFatalError( void )			{ return m_hasFatalError; }
	void			SetWrongDevice( bool error )	{ m_wrongDevice = error; }
	bool			GotWrongDevice( void )			{ return m_wrongDevice; }
#	endif // #ifdef __PLAT_NGC__

private :
					~Manager ( void );
					Manager ( void );

					Card			m_card[vMAX_PORT][vMAX_SLOT];
#	ifdef __PLAT_NGC__
	bool			m_hasFatalError;
	bool			m_wrongDevice;
#	endif // #ifdef __PLAT_NGC__

#	ifdef __PLAT_XBOX__
	Card m_hard_drive;
#	endif // #ifdef __PLAT_XBOX__
	
	DeclareSingletonClass( Manager );
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

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mc

#endif	// __GEL_MCMAN_H

