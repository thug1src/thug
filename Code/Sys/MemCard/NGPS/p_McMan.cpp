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

#include <core/defines.h>
#include <core/singleton.h>

#include <sys/mcman.h>

#include <libmc.h>
#include <sifdev.h>
#include <libscf.h>

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

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

DefineSingletonClass( Manager, "MemCard Manager" );

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
	

	int result;
	int i, j;

	result = sceMcInit();
	Dbg_MsgAssert( result == 0,( "Failed to initialize memory card system\n" ));

	for( i = 0; i < vMAX_PORT; i++ )
	{
		for( j = 0; j < vMAX_SLOT; j++ )
		{
			m_card[i][j].m_port = i;
			m_card[i][j].m_slot = j;
		}
	}
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

int		Manager::GetMaxSlots( int port )
{
	int result;

   	result = sceMcGetSlotMax( port );
	if( result >= 0 )
	{
		return result;
	}
	else
	{
		return 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Card*	Manager::GetCard( int port, int slot )
{
	
	int type;

	Dbg_Assert( port < vMAX_PORT );
	Dbg_Assert( slot < vMAX_SLOT );
	
	Card* card = &m_card[port][slot];

	type = card->GetDeviceType();
	if(	( type == Card::vDEV_PS1_CARD ) ||
		( type == Card::vDEV_PS2_CARD ))
	{
		return card;
	}
    
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		wait_for_async_call_to_finish( Card* card )
{
	int ret_val;

	

	sceMcSync( 0, NULL, &ret_val );
	if( ret_val < 0 )
	{   
		switch( ret_val )
		{
			case -1:
				Dbg_Printf( "Switched to formated card\n" );
				card->SetError( Card::vERROR_FORMATTED );
				break;
			case -2:
				Dbg_Printf( "Error: Unformatted Card\n" );
				card->SetError( Card::vERROR_UNFORMATTED );
				break;
			case -3:
				Dbg_Printf( "Error: Insufficient Space\n" );
				card->SetError( Card::vINSUFFICIENT_SPACE );
				break;
			case -4:
				Dbg_Printf( "Error: Invalid Path\n" );
				card->SetError( Card::vINVALID_PATH );
				break;
			case -5:
				Dbg_Printf( "Error: Access Error\n" );
				card->SetError( Card::vACCESS_ERROR );
				break;
			case -6:
				Dbg_Printf( "Error: Subdir not empty\n" );
				card->SetError( Card::vSUBDIR_NOT_EMPTY );
				break;
			case -7:
				Dbg_Printf( "Error: Too many open files\n" );
				card->SetError( Card::vTOO_MANY_OPEN_FILES );
				break;
			case -10:
				Dbg_Printf( "Error: No card\n" );
				card->SetError( Card::vNO_CARD );
				break;
			default:
				card->SetError( Card::vUNKNOWN );
				break;
		}
	}

	return ret_val;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Card::MakeDirectory( const char* dir_name )
{
	int result;
	 
	

	result = sceMcMkdir( m_port, m_slot, dir_name );
	if( result != 0 )
	{
		SetError( vUNKNOWN );
		return false;
	}
    
	int ErrorCode=wait_for_async_call_to_finish( this );
	if (ErrorCode==-4)
	{
		// If, the directory exists already, don't make it an error
		ErrorCode=0;
	}	
	
	return( ErrorCode == 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Card::DeleteDirectory( const char* dir_name )
{
	// On the PS2, the Delete() function used for files can also
	// be used to delete a directory.
	return Delete(dir_name);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const char *Card::ConvertDirectory( const char* dir_name )
{
	return NULL;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool 	Card::ChangeDirectory( const char* dir_name )
{
	int result;

	result = sceMcChdir( m_port, m_slot, dir_name, NULL );
	if( result != 0 )
	{
		SetError( vUNKNOWN );
		return false;
	}
    
	return( wait_for_async_call_to_finish( this ) == 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Card::Format( void )
{
	int result;

	result = sceMcFormat( m_port, m_slot );
	if( result != 0 )
	{
		SetError( vUNKNOWN );
		return false;
	}
    
	return( wait_for_async_call_to_finish( this ) == 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Card::Unformat( void )
{
	int result;

	result = sceMcUnformat( m_port, m_slot );
	if( result != 0 )
	{
		SetError( vUNKNOWN );
		return false;
	}
    
	return( wait_for_async_call_to_finish( this ) == 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Card::IsFormatted( void )
{
	int formatted;
	int result;

    result = sceMcGetInfo( m_port, m_slot, NULL, NULL, &formatted );
	if( result != 0 )
	{
		SetError( vUNKNOWN );
		return false;
	}
    
	result = wait_for_async_call_to_finish( this );
	if( result != 0 )
	{
		SetError( vUNKNOWN );
		return false;
	}

	return( formatted == 1 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Card::GetDeviceType( void )
{
	int type;
	int result;

    result = sceMcGetInfo( m_port, m_slot, &type, NULL, NULL );
	if( result != 0 )
	{
		SetError( vUNKNOWN );
		return vDEV_NONE;
	}
    
	result = wait_for_async_call_to_finish( this );
	if(	result != 0 )
	{
		if(	( GetLastError() != vERROR_UNFORMATTED ) &&
			( GetLastError() != vERROR_FORMATTED ))
		{
			SetError( vUNKNOWN );
			return vDEV_NONE;
		}
	}

	switch( type )
	{
		case 1:
			return vDEV_PS1_CARD;
		case 2:
			return vDEV_PS2_CARD;
		case 3:
			return vDEV_POCKET_STATION;
		default:
			return vDEV_NONE;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Card::GetNumFreeClusters( void )
{
	int free_clusters;
	int result;

    result = sceMcGetInfo( m_port, m_slot, NULL, &free_clusters, NULL );
	if( result != 0 )
	{
		SetError( vUNKNOWN );
		return 0;
	}
    
	result = wait_for_async_call_to_finish( this );
	if( result != 0 )
	{
		SetError( vUNKNOWN );
		return 0;
	}

	return free_clusters;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Card::GetNumFreeEntries( const char* path )
{
	int result;

    result = sceMcGetEntSpace( m_port, m_slot, path );
	if( result != 0 )
	{
		SetError( vUNKNOWN );
		return 0;
	}
    
	return( wait_for_async_call_to_finish( this ));	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Card::Delete( const char* filename )
{
	int result;

	result = sceMcDelete( m_port, m_slot, filename );
	if( result != 0 )
	{
		SetError( vUNKNOWN );
		return false;
	}
    
	int ErrorCode=wait_for_async_call_to_finish( this );
	if (ErrorCode==-4)
	{
		// The the error was that the file already did not exist, don't report this as an error.
		ErrorCode=0;
	}
		
	return( ErrorCode==0 );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Card::Rename( const char* old_name, const char* new_name )
{
	int result;

	result = sceMcRename( m_port, m_slot, old_name, new_name );
	if( result != 0 )
	{
		SetError( vUNKNOWN );
		return false;
	}
    
	return( wait_for_async_call_to_finish( this ) == 0 );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

File*	Card::Open( const char* filename, int mode, int size )
{
	int result, fd;
	int open_mode;
	File* file;

	

	//Dbg_MsgAssert( strlen( filename ) < 31,( "Filename exceeds max length\n" ));

	file = NULL;
	open_mode = 0;
	if( mode & File::mMODE_READ )
	{
		open_mode |= SCE_RDONLY;
	}
	if( mode & File::mMODE_WRITE )
	{
		open_mode |= SCE_WRONLY;
	}
	if( mode & File::mMODE_CREATE )
	{
		open_mode |= SCE_CREAT;
	}
	result = sceMcOpen( m_port, m_slot, filename, open_mode );
	if( result != 0 )
	{
		Dbg_Printf( "Unknown Error\n" );
		SetError( vUNKNOWN );
		return NULL;
	}
    
	fd = wait_for_async_call_to_finish( this );
	if( fd >= 0 )
	{
		file = new File( fd, this );
	}
	return file;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Ken: This used to be a local variable in Card::GetFileList, but sceMcGetDir requires that
// this buffer be 64 byte aligned, and occasionally it would not be, causing the contents of
// file_table to be undefined after the call to sceMcGetDir.
// Unfortunately, Sony did not make sceMcGetDir return an error code in this case, the blighters.
//
static sceMcTblGetDir file_table[Card::vMAX_FILES_PER_DIRECTORY];   	

bool	Card::GetFileList( const char* mask, Lst::Head< File > &file_list )
{
	
	int result, i;
		
	Dbg_MsgAssert( mask != NULL,( "Mask must be a valid search string. You may use wildcards\n" ));

	Dbg_MsgAssert( (((uint32)file_table)&63)==0, ("file_table not 64 byte aligned"));
	
	
	int CallNumber=0;
	while (true)
	{
		result = sceMcGetDir( m_port, m_slot, mask, CallNumber, vMAX_FILES_PER_DIRECTORY, file_table );
		if( result != 0 )
		{
			SetError( vUNKNOWN );
			return 0;
		}
		
		int NumFilesReturned = wait_for_async_call_to_finish( this );
		for( i = 0; i < NumFilesReturned; i++ )
		{
			File *new_file;

			new_file = new File( 0, this );

			strcpy( new_file->m_Filename, (char *) file_table[i].EntryName );
			new_file->m_Size = file_table[i].FileSizeByte;
			
			new_file->m_Attribs = 0;
			if( file_table[i].AttrFile & sceMcFileAttrReadable )
			{
				new_file->m_Attribs |= File::mATTRIB_READABLE;
			}
			if( file_table[i].AttrFile & sceMcFileAttrWriteable )
			{
				new_file->m_Attribs |= File::mATTRIB_WRITEABLE;
			}
			if( file_table[i].AttrFile & sceMcFileAttrExecutable )
			{
				new_file->m_Attribs |= File::mATTRIB_EXECUTABLE;
			}
			if( file_table[i].AttrFile & sceMcFileAttrDupProhibit )
			{
				new_file->m_Attribs |= File::mATTRIB_DUP_PROHIBITED;
			}
			if( file_table[i].AttrFile & sceMcFileAttrSubdir )
			{
				new_file->m_Attribs |= File::mATTRIB_DIRECTORY;
			}
			if( file_table[i].AttrFile & sceMcFileAttrClosed )
			{
				new_file->m_Attribs |= File::mATTRIB_CLOSED;
			}
			if( file_table[i].AttrFile & sceMcFileAttrPDAExec )
			{
				new_file->m_Attribs |= File::mATTRIB_PDA_APP;
			}
			if( file_table[i].AttrFile & sceMcFileAttrPS1 )
			{
				new_file->m_Attribs |= File::mATTRIB_PS1_FILE;
			}
			
			sceCdCLOCK myClock;
			myClock.day=itob(file_table[i]._Modify.Day);
			myClock.hour=itob(file_table[i]._Modify.Hour);
			myClock.minute=itob(file_table[i]._Modify.Min);
			myClock.month=itob(file_table[i]._Modify.Month);
			myClock.second=itob(file_table[i]._Modify.Sec);
			myClock.year=itob(file_table[i]._Modify.Year%100);
			sceScfGetLocalTimefromRTC(&myClock);
			new_file->m_Modified.m_Seconds 	= btoi(myClock.second);
			new_file->m_Modified.m_Minutes 	= btoi(myClock.minute);
			new_file->m_Modified.m_Hour 	= btoi(myClock.hour);
			new_file->m_Modified.m_Day 		= btoi(myClock.day);
			new_file->m_Modified.m_Month 	= btoi(myClock.month);
			new_file->m_Modified.m_Year 	= btoi(myClock.year)+2000;
			
			myClock.day=itob(file_table[i]._Create.Day);
			myClock.hour=itob(file_table[i]._Create.Hour);
			myClock.minute=itob(file_table[i]._Create.Min);
			myClock.month=itob(file_table[i]._Create.Month);
			myClock.second=itob(file_table[i]._Create.Sec);
			myClock.year=itob(file_table[i]._Create.Year%100);
			sceScfGetLocalTimefromRTC(&myClock);
			new_file->m_Created.m_Seconds 	= btoi(myClock.second);
			new_file->m_Created.m_Minutes 	= btoi(myClock.minute);
			new_file->m_Created.m_Hour 		= btoi(myClock.hour);  
			new_file->m_Created.m_Day 		= btoi(myClock.day);   
			new_file->m_Created.m_Month 	= btoi(myClock.month); 
			new_file->m_Created.m_Year 		= btoi(myClock.year)+2000;  

			file_list.AddToTail( new_file );
		}
		
		if (NumFilesReturned<vMAX_FILES_PER_DIRECTORY)
		{
			break;
		}
		++CallNumber;
	}

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

bool	File::Close( void )
{
	int result;

	result = sceMcClose( m_fd );
	if( result != 0 )
	{
		m_card->SetError( Card::vUNKNOWN );
		return 0;
	}
    
	return( wait_for_async_call_to_finish( m_card ) == 0 );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		File::Seek( int offset, FilePointerBase base )
{
	int result;
	int mode;

	

	switch( base )
	{
		case BASE_START:
			mode = 0;
			break;
		case BASE_CURRENT:
			mode = 1;
			break;
		case BASE_END:
			mode = 2;
			break;
		default:
			mode = 1;
			Dbg_MsgAssert( 0,( "Invalid FilePointerBase\n" ));
			break;
	}

	result = sceMcSeek( m_fd, offset, mode );
	if( result != 0 )
	{
		m_card->SetError( Card::vUNKNOWN );
		return 0;
	}
    
	return( wait_for_async_call_to_finish( m_card ));	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool 	File::Flush( void )
{
	int result;

	result = sceMcFlush( m_fd );
	if( result != 0 )
	{
		m_card->SetError( Card::vUNKNOWN );
		return false;
	}
    
	return( wait_for_async_call_to_finish( m_card ) == 0 );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		File::Write( void* data, int len )
{
	int result;

	result = sceMcWrite( m_fd, data, len );
	if( result != 0 )
	{
		m_card->SetError( Card::vUNKNOWN );
		return 0;
	}
    
	return( wait_for_async_call_to_finish( m_card ));	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		File::Read( void* buff, int len )
{
	int result;

	

	result = sceMcRead( m_fd, buff, len );
	if( result != 0 )
	{
		Dbg_Printf( "Error: Could not read data\n" );
		m_card->SetError( Card::vUNKNOWN );
		return 0;
	}
    
	return( wait_for_async_call_to_finish( m_card ));	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mc




