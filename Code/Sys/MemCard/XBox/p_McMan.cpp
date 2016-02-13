/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		SYS														**
**																			**
**	Module:			Mc			 											**
**																			**
**	File name:		memcard.cpp												**
**																			**
**	Created by:		03/06/01	-	spg										**
**																			**
**	Description:	Memcard - platform-specific implementations				**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <xtl.h>
#include <core/defines.h>
#include <core/singleton.h>

#include <sys/mcman.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Mc
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define MAX_FILENAME_LENGTH	128

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

DefineSingletonClass( Manager, "MemCard Manager" );

static char cardFilenameBuffer[MAX_FILENAME_LENGTH];

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

Manager::Manager( void )
{
	int i, j;

	for( i = 0; i < vMAX_PORT; i++ )
	{
		for( j = 0; j < vMAX_SLOT; j++ )
		{
			m_card[i][j].m_port = i;
			m_card[i][j].m_slot = j;
		}
	}
	
	m_hard_drive.SetAsHardDrive();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Manager::~Manager( void )
{
}

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

int	Manager::GetMaxSlots( int port )
{
	return vMAX_SLOT;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Card* Manager::GetCard( int port, int slot )
{
	// Ignore port and slot, since on the XBox we're only using the hard drive.
	return &m_hard_drive;
}

// Skate3 code, disabled for now.
#	if 0
Card* Manager::GetHardDrive( )
{
	return &m_hard_drive;
}

const char* Manager::GetLastCardName( int port, int slot )
{
	Dbg_Assert( port < vMAX_PORT );
	Dbg_Assert( slot < vMAX_SLOT );
	
	Card *p_card = &m_card[port][slot];
	return p_card->GetLastPersonalizedName();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const char *Card::GetPersonalizedName()
{
	// Only get the name if the card is mounted and is not the hard drive.
	if (m_mounted_drive_letter && m_mounted_drive_letter!='u')
	{
		sprintf(mp_personalized_name,"");
		
		WCHAR p_personalized_name[MAX_MUNAME+10];
		DWORD rv=XMUNameFromDriveLetter(m_mounted_drive_letter,p_personalized_name,MAX_MUNAME);
		if (rv==ERROR_SUCCESS)
		{
			// Instead of doing a wsprintfA( mp_personalized_name, "%ls", p_personalized_name );
			// which will terminate as soon as it hits a bad char, convert each WCHAR one at a 
			// time so that all good characters come across.
			char *p_dest=mp_personalized_name;
			int count=0;
			WCHAR p_temp[2]; // A buffer for holding one WCHAR at a time for sending to wsprintfA
			p_temp[1]=0;
			const WCHAR *p_scan=p_personalized_name;
			while (*p_scan) // WCHAR strings are terminated by a 0 just like normal strings, except its a 2byte 0.
			{
				p_temp[0]=*p_scan++;
				
				char p_one_char[10];
				wsprintfA( p_one_char, "%ls", p_temp);
				// p_one_char now contains a one char string.
				
				if (count<MAX_MUNAME)
				{
					if (*p_one_char)
					{
						*p_dest=*p_one_char;
					}
					else
					{
						// Bad char, so write a ~ so that it appears as a xbox null char.
						*p_dest='~';
					}	
					++p_dest;
					++count;
				}
			}	
			*p_dest=0;
		
		
			int len=strlen(mp_personalized_name);
			
			for (int i=0; i<len; ++i)
			{
				// Force any special characters (arrows or button icons) to be displayed
				// as the xbox NULL character by changing them to an invalid character.
				switch (mp_personalized_name[i])
				{
				case '¡': case '¢': case '£': case '¤':
				case '¥': case '¦': case '§': case '¨':
				case '©': case 'ª': case '«': case '¬':
				case -1: // This is the weird lower-case-y-umlaut character.
						 // Note: The upper case y-umlaut character appears to be viewed as a
						 // terminator character by XMUNameFromDriveLetter ( = bug?)
					mp_personalized_name[i]='~';
					break;
				default:
					break;
				}	
			}
		}
		else
		{
			#ifdef __NOPT_ASSERT__
			char p_temp[100];
			sprintf(p_temp,"XMUNameFromDriveLetter error code = %d\n",rv);
			OutputDebugString(p_temp);
			#endif
		}	
	}
	
	return mp_personalized_name;
}

const char *Card::GetLastPersonalizedName()
{
	return mp_personalized_name;
}

bool Card::THPS3SavesExist()
{
	if ( m_mounted_drive_letter == 0 )
	{
		return false;
	}	

	cardFilenameBuffer[0] = m_mounted_drive_letter;
	cardFilenameBuffer[1] = ':';
	cardFilenameBuffer[2] = '\\';
	cardFilenameBuffer[3] = 0;
	
	XGAME_FIND_DATA data;
	HANDLE rv=XFindFirstSaveGame(cardFilenameBuffer,&data);
	if (rv==INVALID_HANDLE_VALUE)
	{
		return false;
	}	
	else
	{
		XFindClose(rv);
		return true;
	}	
}
	
bool Card::CasParkOrReplaysExist()
{
	if ( m_mounted_drive_letter == 0 )
	{
		return false;
	}	

	char p_optpros_name[100];
	sprintf( p_optpros_name,"/Options and Pros" );
	ConvertDirectory( p_optpros_name, p_optpros_name );
	strcat(p_optpros_name,"\\");
	
	cardFilenameBuffer[0] = m_mounted_drive_letter;
	cardFilenameBuffer[1] = ':';
	cardFilenameBuffer[2] = '\\';
	cardFilenameBuffer[3] = 0;
	
	XGAME_FIND_DATA data;
	HANDLE rv=XFindFirstSaveGame(cardFilenameBuffer,&data);
	if (rv==INVALID_HANDLE_VALUE)
	{
		return false;
	}	
	
	bool cas_park_or_replays_exist=false;
	while (true)
	{
		if (stricmp(data.szSaveGameDirectory+3,p_optpros_name)==0)
		{
			// It's the optpros file
		}	
		else
		{
			// It either a cas, park or replay.
			cas_park_or_replays_exist=true;
			break;
		}	
		
		if (XFindNextSaveGame(rv,&data))
		{
		}
		else
		{
			break;
		}
	}
			
	XFindClose(rv);
	return cas_park_or_replays_exist;
}

// Returns true if the total number of cas, park or replay files is the max allowed.
bool Card::MaxFilesReached()
{
	if ( m_mounted_drive_letter == 0 )
	{
		return false;
	}	

	char p_optpros_name[100];
	sprintf( p_optpros_name,"/Options and Pros" );
	ConvertDirectory( p_optpros_name, p_optpros_name );
	strcat(p_optpros_name,"\\");
	
	cardFilenameBuffer[0] = m_mounted_drive_letter;
	cardFilenameBuffer[1] = ':';
	cardFilenameBuffer[2] = '\\';
	cardFilenameBuffer[3] = 0;
	
	XGAME_FIND_DATA data;
	HANDLE rv=XFindFirstSaveGame(cardFilenameBuffer,&data);
	if (rv==INVALID_HANDLE_VALUE)
	{
		return false;
	}	
	
	int num_files=0;
	while (true)
	{
		if (stricmp(data.szSaveGameDirectory+3,p_optpros_name)==0)
		{
			// It's the optpros file
		}	
		else
		{
			// It either a cas, park or replay.
			++num_files;
		}	
		
		if (!XFindNextSaveGame(rv,&data))
		{
			break;
		}
	}
			
	XFindClose(rv);
	
	if (num_files>=75)
	{
		return true;
	}	
	return false;
}
#	endif
	

// Note: dir_name must no longer start with a backslash, it should just be "Career2-Career" etc.
bool Card::MakeDirectory( const char* dir_name )
{
	char	output_dir[vDIRECTORY_NAME_BUF_SIZE];
	WCHAR	input_name[vDIRECTORY_NAME_BUF_SIZE];

	wsprintfW( input_name, L"%hs", dir_name );
	DWORD rv = XCreateSaveGame(	"u:\\",				// Root of device on which to create the save game.
								input_name,			// Name of save game (effectively directory name).
								OPEN_ALWAYS,		// Open disposition.
								0,					// Creation flags.
								output_dir,			// String to take resultant directory name buffer.
								vDIRECTORY_NAME_BUF_SIZE );	// Size of directory name buffer.

	if( rv == ERROR_SUCCESS )
	{
		return true;
	}

	if (rv==ERROR_DISK_FULL)
	{
		m_last_error=vINSUFFICIENT_SPACE;
	}
		
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const char *Card::ConvertDirectory( const char* dir_name )
{
	static char	output_dir[vDIRECTORY_NAME_BUF_SIZE];
	WCHAR	input_name[vDIRECTORY_NAME_BUF_SIZE];

	wsprintfW( input_name, L"%hs", dir_name );
	DWORD rv = XCreateSaveGame(	"u:\\",				// Root of device on which to create the save game.
								input_name,			// Name of save game (effectively directory name).
								OPEN_EXISTING,		// Open disposition.
								0,					// Creation flags.
								output_dir,			// String to take resultant directory name buffer.
								vDIRECTORY_NAME_BUF_SIZE );	// Size of directory name buffer.

	if( rv == ERROR_SUCCESS )
	{
		// Remove the initial "u:\" and the final "\"
		strcpy(output_dir,output_dir+3);
		output_dir[strlen(output_dir)-1]=0;
		
		return output_dir;
	}

	return NULL;
}	

// Skate3 code, disabled for now.
#if 0

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool Card::ConvertDirectory( const char* dir_name, char* output_name )
{
	// K: This used to assert.
	if (m_mounted_drive_letter==0)
	{
		return false;
	}	

	// Seems incoming filenames are of the form /foo etc.
	cardFilenameBuffer[0] = m_mounted_drive_letter;
	cardFilenameBuffer[1] = ':';
	cardFilenameBuffer[2] = '\\';
	cardFilenameBuffer[3] = 0;

	++dir_name;
	int index = 4;
	while( cardFilenameBuffer[index] = *dir_name )
	{
		// Switch forward slash directory separators to the supported backslash.
		if( cardFilenameBuffer[index] == '/' )
		{
			cardFilenameBuffer[index] = '\\';
		}
		++index;
		++dir_name;
	}

	char	output_dir[64];
	WCHAR	input_name[64];

	wsprintfW( input_name, L"%hs", &cardFilenameBuffer[4] );
	DWORD rv = XCreateSaveGame(	cardFilenameBuffer,	// Root of device on which to create the save game.
								input_name,			// Name of save game (effectively directory name).
								OPEN_EXISTING,		// Open disposition.
								0,					// Creation flags.
								output_dir,			// String to take resultant directory name buffer.
								64 );				// Size of directory name buffer.

	if( rv == ERROR_SUCCESS )
	{
		// Copy over output directory, stripping leading drive, colon and backslash, and removing trailing backslash.
		strcpy( output_name, &output_dir[3] );
		output_name[strlen( output_name ) - 1] = 0;
		return true;
	}

	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool Card::DeleteDirectory( const char* dir_name )
{
	Dbg_Assert( m_mounted_drive_letter != 0 );

	// Seems incoming filenames are of the form /foo etc.
	cardFilenameBuffer[0] = m_mounted_drive_letter;
	cardFilenameBuffer[1] = ':';
	cardFilenameBuffer[2] = '\\';
	cardFilenameBuffer[3] = 0;

	++dir_name;
	int index = 4;
	while( cardFilenameBuffer[index] = *dir_name )
	{
		// Switch forward slash directory separators to the supported backslash.
		if( cardFilenameBuffer[index] == '/' )
		{
			cardFilenameBuffer[index] = '\\';
		}
		++index;
		++dir_name;
	}

	WCHAR	input_name[64];
	wsprintfW( input_name, L"%hs", &cardFilenameBuffer[4] );
	DWORD rv = XDeleteSaveGame(	cardFilenameBuffer,			// Root of device on which to create the save game.
								input_name );

	if( rv == ERROR_SUCCESS )
	{
		return true;
	}

	return false;
}

#endif


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Given the name of a file, this will delete it's directory, which
// will result in the deletion of the directory, the file, and the icon.
// The name must not be preceded with a backslash, ie should be "Career12-Career" for example.
bool Card::DeleteDirectory( const char* dir_name )
{
	WCHAR	input_name[64];
	wsprintfW( input_name, L"%hs", dir_name );
	DWORD rv = XDeleteSaveGame(	"u:\\", input_name );

	if( rv == ERROR_SUCCESS )
	{
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool Card::ChangeDirectory( const char* dir_name )
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool Card::Format( void )
{
	// Not supported from within an Xbox title.
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool Card::Unformat( void )
{
	// Not supported from within an Xbox title.
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool Card::IsFormatted( void )
{
	// Must be formatted to have got this far on an Xbox title.
	return true;
}


void Card::SetAsHardDrive()
{
	m_mounted_drive_letter='u';
}

// Skate3 code, disabled for now.
#if 0
bool Card::IsHardDrive()
{
	return m_mounted_drive_letter=='u';
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Card::UnMount( void )
{
	// Can't unmount the hard drive.
	if( m_mounted_drive_letter=='u' )
	{
		return;
	}	
	
    m_mount_error=false;
	m_mount_failed_due_to_card_full=false;
	m_mount_failed_due_to_card_unformatted=false;
	
	if( m_mounted_drive_letter )
	{
		DWORD rv = XUnmountMU(m_port, ( m_slot == 0 ) ? XDEVICE_TOP_SLOT : XDEVICE_BOTTOM_SLOT);
		if (rv != ERROR_SUCCESS)
		{
			m_mount_error=true;
			if (rv==ERROR_DISK_FULL)
			{
				m_mount_failed_due_to_card_full=true;
			}	
			if (rv==ERROR_UNRECOGNIZED_VOLUME)	
			{
				m_mount_failed_due_to_card_unformatted=true;
			}
		}    
		m_mounted_drive_letter=0;
	}
}
	
#endif


int	Card::GetDeviceType( void )
{
	return vDEV_XBOX_HARD_DRIVE;
}


// Skate3 code, disabled for now.
#if 0

bool Card::GetMountError()
{
    return m_mount_error;
}

bool Card::MountFailedDueToCardFull()
{
	return m_mount_failed_due_to_card_full;
}	

bool Card::MountFailedDueToCardUnformatted()
{
	return m_mount_failed_due_to_card_unformatted;
}	

#endif


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int	Card::GetNumFreeClusters( void )
{
	if( m_mounted_drive_letter )
	{
		char p_drive[20];
		strcpy(p_drive,"z:\\");
		
		ULARGE_INTEGER	uliFreeAvail;
		ULARGE_INTEGER	uliTotal;

		p_drive[0] = m_mounted_drive_letter;
		BOOL br		= GetDiskFreeSpaceEx( p_drive, &uliFreeAvail, &uliTotal, NULL );
		if( br )
		{
			// Each increment of HighPart represents 2^32 bytes, which is (2^32)/16384=262144 blocks.
			return uliFreeAvail.HighPart*262144 + uliFreeAvail.LowPart/16384;
		}
		else
		{
			return 0;
		}
	}
	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int	Card::GetNumFreeEntries( const char* path )
{
	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool Card::Delete( const char* filename )
{
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool Card::Rename( const char* old_name, const char* new_name )
{
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
File* Card::Open( const char* filename, int mode, int size )
{
	Dbg_Assert( m_mounted_drive_letter != 0 );

	m_last_error=0;
	
	File*	p_file		= NULL;
	HANDLE	handle;

	// Seems incoming filenames are of the form /foo/bar etc.
	cardFilenameBuffer[0] = m_mounted_drive_letter;
	cardFilenameBuffer[1] = ':';

	int index = 2;
	while( cardFilenameBuffer[index] = *filename )
	{
		// Switch forward slash directory separators to the supported backslash.
		if( cardFilenameBuffer[index] == '/' )
		{
			cardFilenameBuffer[index] = '\\';
		}
		++index;
		++filename;
	}

	DWORD dwDesiredAccess;
	DWORD dwCreationDisposition;

	switch( mode )
	{
		case File::mMODE_READ:
		{
			dwDesiredAccess			= GENERIC_READ;
			dwCreationDisposition	= OPEN_EXISTING;
			break;
		}

		case File::mMODE_WRITE:
		{
			dwDesiredAccess			= GENERIC_WRITE;
			dwCreationDisposition	= OPEN_EXISTING;
			break;
		}

		case ( File::mMODE_WRITE | File::mMODE_CREATE ):
		{
			dwDesiredAccess			= GENERIC_WRITE;
			dwCreationDisposition	= OPEN_ALWAYS;
			break;
		}

		case File::mMODE_CREATE:
		{
			dwDesiredAccess			= GENERIC_WRITE;
			dwCreationDisposition	= CREATE_NEW;
			break;
		}

		case ( File::mMODE_READ | File::mMODE_WRITE ):
		{
			dwDesiredAccess	= GENERIC_READ | GENERIC_WRITE;
			dwCreationDisposition	= OPEN_EXISTING;
			break;
		}

		default:
		{
			Dbg_Assert( 0 );
			return NULL;
		}
	}

	handle = CreateFile(	cardFilenameBuffer,							// file name
							dwDesiredAccess,							// access mode
							0,											// share mode
							NULL,										// security attributes
							dwCreationDisposition,						// how to create
							FILE_ATTRIBUTE_NORMAL,						// file attributes and flags
							NULL );				                        // handle to template file

	if( handle != INVALID_HANDLE_VALUE )
	{
		p_file = new File( (int)handle, this );
		
		WIN32_FILE_ATTRIBUTE_DATA file_attribute_data;
		if (GetFileAttributesEx(cardFilenameBuffer,GetFileExInfoStandard,&file_attribute_data))
		{
//			Skate3 code, disabled for now.
#			if 0
			p_file->m_file_time=file_attribute_data.ftLastWriteTime;
#			endif
		}	
		
		return p_file;
	}
	else
	{
		if (GetLastError()==ERROR_DISK_FULL)
		{
			m_last_error=vINSUFFICIENT_SPACE;
		}
	}	

	return NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool Card::GetFileList( const char* mask, Lst::Head< File > &file_list )
{
	HANDLE			handle;
	XGAME_FIND_DATA	find_data;

	cardFilenameBuffer[0] = m_mounted_drive_letter;
	cardFilenameBuffer[1] = ':';
	cardFilenameBuffer[2] = '\\';

	cardFilenameBuffer[3] = 0;
	if(( handle = XFindFirstSaveGame( cardFilenameBuffer, &find_data )) == INVALID_HANDLE_VALUE )
	{
		return true;
	}

	do
	{
		File* new_file = new File( 0, this );

		// Skip copying the drive stuff, just copy the directory, and strip the trailing '\'.
		strcpy( new_file->m_Filename, (char*)&find_data.szSaveGameDirectory[3] );
		new_file->m_Filename[strlen( new_file->m_Filename ) - 1] = 0;

		wsprintfA( new_file->m_DisplayFilename, "%ls", find_data.szSaveGameName );
		
		FILETIME local_file_time;
		FileTimeToLocalFileTime(&find_data.wfd.ftLastWriteTime,&local_file_time);
		SYSTEMTIME system_file_time;
		FileTimeToSystemTime(&local_file_time,&system_file_time);
		
		new_file->m_Modified.m_Year=system_file_time.wYear;
		new_file->m_Modified.m_Month=system_file_time.wMonth;
		new_file->m_Modified.m_Day=system_file_time.wDay;
		new_file->m_Modified.m_Hour=system_file_time.wHour;
		new_file->m_Modified.m_Minutes=system_file_time.wMinute;
		new_file->m_Modified.m_Seconds=system_file_time.wSecond;
		
		new_file->m_Size	= find_data.wfd.nFileSizeLow;
		new_file->m_Attribs	= 0;
		file_list.AddToTail( new_file );
	}
	while( XFindNextSaveGame( handle, &find_data ));
	XFindClose( handle );
	return true;
}


File::File( int fd, Card* card ) : Lst::Node< File > ( this ), m_fd( fd ), m_card( card )
{
}
		
File::~File()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int File::Seek( int offset, FilePointerBase base )
{
	Dbg_Assert( m_fd != 0 );

	DWORD dwMoveMethod;

	switch( base )
	{
		case BASE_START:
			dwMoveMethod = FILE_BEGIN;
			break;
		case BASE_CURRENT:
			dwMoveMethod = FILE_CURRENT;
			break;
		case BASE_END:
			dwMoveMethod = FILE_END;
			break;
		default:
			dwMoveMethod = FILE_END;
			Dbg_MsgAssert( 0,( "Invalid FilePointerBase\n" ));
			break;
	}


	DWORD result = SetFilePointer(	(HANDLE)m_fd,		// handle to file
									offset,				// bytes to move pointer
									NULL,				// high-order bytes to move pointer
									dwMoveMethod );		// starting point

	return result;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool File::Flush( void )
{
	Dbg_Assert( m_fd != 0 );

	FlushFileBuffers((HANDLE)m_fd );

	// The FlushFileBuffers() is pretty strict about what types of files wmay be flushed,
	// whereas the PS2 equivalent doesn't really care. Just return a positive response always,
	// no critical stuff predicated on this return anway.
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	File::Write( void* data, int len )
{
	Dbg_Assert( m_fd != 0 );

//	Skate3 code, disabled for now.
#	if 0
	m_not_enough_space_to_write_file=false;
#	endif

	DWORD bytes_written;
	BOOL rv = WriteFile(	(HANDLE)m_fd,		// handle to file
							data,				// data buffer
							len,				// number of bytes to write
							&bytes_written,		// number of bytes written
							NULL );				// overlapped buffer
							
//	Skate3 code, disabled for now.
#	if 0
	if (rv==ERROR_NOT_ENOUGH_MEMORY)
	{
		m_not_enough_space_to_write_file=true;
	}
	if (GetLastError()==ERROR_DISK_FULL)
	{
		m_not_enough_space_to_write_file=true;
	}
#	endif
		
	if( rv )
	{
		return (int)bytes_written;
	}

	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	File::Read( void* buff, int len )
{
	Dbg_Assert( m_fd != 0 );

	DWORD bytes_read;
	BOOL rv = ReadFile(	(HANDLE)m_fd,			// handle to file
						buff,					// data buffer
						len,					// number of bytes to read
						&bytes_read,			// number of bytes read
						NULL );					// overlapped buffer
	if( rv )
	{
		return (int)bytes_read;
	}

	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool File::Close( void )
{
	Dbg_Assert( m_fd != 0 );

	return CloseHandle((HANDLE)m_fd );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mc




