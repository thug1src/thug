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
#include <sys/ngc/p_display.h>
#include <gel/mainloop.h>
#include <gfx/ngc/nx/nx_init.h>

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

#define MAX_FILENAME_LENGTH	CARD_FILENAME_MAX

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

DefineSingletonClass( Manager, "MemCard Manager" );

static char cardFilenameBuffer[MAX_FILENAME_LENGTH];

static bool initCalled = false;

// Implicit assumption here is that vMAX_PORT == 1, which is always true for Gamecube.
static char cardWorkArea[Manager::vMAX_SLOT][CARD_WORKAREA_SIZE] __attribute__((aligned( 32 )));

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

static void detachCallback( s32 chan, s32 result )
{
	OSReport( "card %d detached: result = %d\n", chan, result );

	Spt::SingletonPtr< Mc::Manager > mc_man;

	// Don't want to call GetCard() here, since that will go off and try to verify the card...
	Card* p_card = mc_man->GetCardEx( 0, chan );
	if( p_card && p_card->mp_work_area )
	{
//		delete[] p_card->mp_work_area;
		p_card->mp_work_area	= NULL;
	}
}



Manager::Manager( void )
{
	if( !initCalled )
	{
		initCalled = true;
		CARDInit();
	}

	for( int i = 0; i < vMAX_PORT; i++ )
	{
		for( int j = 0; j < vMAX_SLOT; j++ )
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
	Dbg_Assert( port < vMAX_PORT );
	Dbg_Assert( slot < vMAX_SLOT );
	
	// Disable the reset button. Scripts will also do this by calling the script command
	// DisableReset at appropriate points.
	// The scripts may disable reset a bit earlier, eg when a 'checking card' message is on
	// screen. They do that to prevent lots of TT bugs because if the reset button is enabled
	// during the message it will seem like you can reset during card access, even though the
	// card is not really being accessed then.
	// It is done here too as a guarantee.
	//printf("GetCard is disabling reset ...\n");
	//NxNgc::EngineGlobals.disableReset = true;
	
	Card*	card	= &m_card[port][slot];
	int		type	= card->GetDeviceType();

	if( type == Card::vDEV_GC_CARD )
	{
		return card;
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Card::MakeDirectory( const char* dir_name )
{
//	Dbg_Assert( m_mounted_drive_letter != 0 );
//
//	// Seems incoming filenames are of the form /foo etc.
//
//	cardFilenameBuffer[0] = m_mounted_drive_letter;
//	cardFilenameBuffer[1] = ':';
//
//	int index = 2;
//	while( cardFilenameBuffer[index] = *dir_name )
//	{
//		// Switch forward slash directory separators to the supported backslash.
//		if( cardFilenameBuffer[index] == '/' )
//		{
//			cardFilenameBuffer[index] = '\\';
//		}
//		++index;
//		++dir_name;
//	}
//
//	if( CreateDirectory( cardFilenameBuffer, NULL ))
//	{
//		return true;
//	}

	// Directory structure is not supported on Gamecube memory cards.
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool Card::DeleteDirectory( const char* dir_name )
{
	return false;
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

bool Card::ChangeDirectory( const char* dir_name )
{
	// Directory structure is not supported on Gamecube memory cards.
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool Card::Format( void )
{
	if( mp_work_area )
	{
		s32 format_result = CARDFormat( m_slot );
		
		if( format_result != CARD_RESULT_READY )
		{
			// If the format fails, then we need to do a CARDMount again, otherwise	
			// CARDCheck will always return CARD_RESULT_NOCARD, even if a good card is inserted. (TT4975)
			// Force a CARDMount by deleting the mp_work_area buffer and calling GetDeviceType.
			if( mp_work_area )
			{
	//			delete [] mp_work_area;
				mp_work_area = NULL;
			}
			GetDeviceType();
			
			m_broken=true;
		}
		
		if( format_result == CARD_RESULT_READY )
		{
			return true;
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool Card::Unformat( void )
{
	// Not supported from within a GameCube title.
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool Card::IsFormatted( void )
{
	if( mp_work_area )
	{
		s32 check_result = CARDCheck( m_slot );
		if (check_result==CARD_RESULT_IOERROR)
		{
			m_broken=true;
		}	
		if( check_result == CARD_RESULT_READY )
		{
			return true;
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool Card::IsForeign( void )
{
	if( mp_work_area )
	{
		u16 encode;
		CARDGetEncoding(m_slot,&encode);
		if (encode==CARD_ENCODE_SJIS)
		{
			return true;
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool Card::IsBadDevice( void )
{
	if( mp_work_area )
	{
		s32 check_result = CARDCheck( m_slot );
		if( check_result == CARD_RESULT_WRONGDEVICE )
		{
			return true;
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	Card::GetDeviceType( void )
{
	// See whether this card is present.
	s32	mem_size;
	s32	sector_size;
	s32	probe_result;
	s32 loop_iterations	= 0;

	// Cleat fatal error tracking variable.
	Spt::SingletonPtr< Mc::Manager > mc_man;
	mc_man->SetFatalError( false );
	mc_man->SetWrongDevice( false );

	do
	{
		// This would appear to be a problem with the 0.93 dev hardware.
		probe_result = CARDProbeEx( m_slot, &mem_size, &sector_size );

		// CARDProbeEx() will return BUSY if the system is still checking the card, so safe to quit on a NOCARD.
		if( probe_result == CARD_RESULT_NOCARD )
		{
			break;
		}

		if( probe_result == CARD_RESULT_WRONGDEVICE )
		{
			mc_man->SetWrongDevice( true );
			break;
		}	

//		// Check for reset being pushed.
//		if( NsDisplay::shouldReset())
//		{
//			// Prepare the game side of things for reset.
//			Spt::SingletonPtr< Mlp::Manager > mlp_manager;
//			//mlp_manager->PrepareReset();
//
//			// Prepare the system side of things for reset.
//			NsDisplay::doReset();
//		}
	}
	while(( probe_result != CARD_RESULT_READY ) && ( ++loop_iterations < 1000000 ));

	switch( probe_result )
	{
		case CARD_RESULT_BUSY:
		case CARD_RESULT_NOCARD:
		case CARD_RESULT_READY:
		case CARD_RESULT_FATAL_ERROR:
		{
			// If this card is not mounted, mount it now.
			if( mp_work_area == NULL )
			{
				m_broken=false;
				
//				mp_work_area = new char[CARD_WORKAREA_SIZE];
				mp_work_area = &( cardWorkArea[m_slot][0] );
				Dbg_MsgAssert( (((uint32)mp_work_area)&0x1f)==0, ("mp_work_area not 32 byte aligned"));
				
				s32 mount_result = CARDMount( m_slot, mp_work_area, detachCallback );
				
				if (mount_result == CARD_RESULT_BROKEN)
				{
					s32 check_result = CARDCheck( m_slot );
					//if (check_result == CARD_RESULT_BROKEN || check_result==CARD_RESULT_IOERROR)
					if (check_result==CARD_RESULT_IOERROR)
					{
						m_broken=true;
					}	
				}	
				
				if(( mount_result == CARD_RESULT_READY ) || ( mount_result == CARD_RESULT_BROKEN ) || ( mount_result == CARD_RESULT_ENCODING ))
				{
					// Mounted. Required to call CARDCheck() at this stage.
					s32 check_result = CARDCheck( m_slot );
					
					if(( check_result == CARD_RESULT_READY ) || ( check_result == CARD_RESULT_BROKEN ) || ( mount_result == CARD_RESULT_ENCODING ))
					{
						m_mem_size		= mem_size;
						m_sector_size	= sector_size;

						return vDEV_GC_CARD;
					}
					else
					{
//						delete [] mp_work_area;
						mp_work_area = NULL;
					}
				}
				else
				{
					if(( mount_result == CARD_RESULT_IOERROR ) || ( mount_result == CARD_RESULT_FATAL_ERROR ))
					{
						// Set Manager tracking variable.
						mc_man->SetFatalError( true );
					}
					else if (mount_result==CARD_RESULT_WRONGDEVICE)
					{
						mc_man->SetWrongDevice( true );
					}	
//					delete [] mp_work_area;
					mp_work_area = NULL;
				}
			}			
			else
			{
				return vDEV_GC_CARD;
			}
			break;
		}

		case CARD_RESULT_WRONGDEVICE:
		default:
		{
			// No valid card, or can't be processed at this time.
			break;
		}
	}	

	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	Card::GetNumFreeClusters( void )
{
	s32 bytes_free;
	s32 files_free;
	s32 free_result = CARDFreeBlocks( m_slot, &bytes_free, &files_free );
	if( free_result == CARD_RESULT_READY )
	{
		return bytes_free / 8192;
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
	// Seems incoming filenames are of the form /foo/bar etc. Since there is no
	// directory structure on the Gamecube memory card, just convert to a single filename
	// by removing '\' and '/'.
	int index = 0;
	while(( cardFilenameBuffer[index] = *filename ))
	{
		Dbg_Assert( index < MAX_FILENAME_LENGTH );

		// Switch forward slash directory separators to the supported backslash.
		if(( cardFilenameBuffer[index] == '/' ) || ( cardFilenameBuffer[index] == '\\' ) || ( cardFilenameBuffer[index] == '.' ))
		{
		}
		else
		{
			++index;
		}
		++filename;
	}

	s32 delete_result = CARDDelete( m_slot, cardFilenameBuffer );
	if( delete_result == CARD_RESULT_IOERROR )
	{
		m_broken=true;
	}

	if( delete_result == CARD_RESULT_READY )
	{
		return true;
	}

	return false;
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
	Dbg_Assert( mp_work_area != 0 );

	// Create the file here, it may be deleted if the open fails.
	File* p_file = new File( 0, this );

	// Seems incoming filenames are of the form /foo/bar etc. Since there is no
	// directory structure on the Gamecube memory card, just convert to a single filename
	// by removing '\' and '/'.
	int index = 0;
	while(( cardFilenameBuffer[index] = *filename ))
	{
		Dbg_MsgAssert( index < MAX_FILENAME_LENGTH, ( "Filename: %s too long\n", filename ));

		// Switch forward slash directory separators to the supported backslash.
		if(( cardFilenameBuffer[index] == '/' ) || ( cardFilenameBuffer[index] == '\\' ) || ( cardFilenameBuffer[index] == '.' ))
		{
		}
		else
		{
			++index;
		}
		++filename;
	}

	// If we might be creating the file, ensure the size is aligned to a multiple of the sector size.
	if( mode & File::mMODE_CREATE )
	{
		Dbg_Assert( m_sector_size > 0 );
		size = ( size + ( m_sector_size - 1 )) & ~( m_sector_size - 1 );
	}

	s32 open_result;
	switch( mode )
	{
		case File::mMODE_READ:
		{
			open_result = CARDOpen( m_slot, cardFilenameBuffer, &p_file->m_file_info );
			break;
		}

		case File::mMODE_WRITE:
		{
			open_result = CARDOpen( m_slot, cardFilenameBuffer, &p_file->m_file_info );
			break;
		}

		case ( File::mMODE_WRITE | File::mMODE_CREATE ):
		{
			open_result = CARDOpen( m_slot, cardFilenameBuffer, &p_file->m_file_info );
			if( open_result == CARD_RESULT_NOFILE )
			{
				open_result = CARDCreate( m_slot, cardFilenameBuffer, size, &p_file->m_file_info );
			}
			break;
		}

		case File::mMODE_CREATE:
		{
			open_result = CARDCreate( m_slot, cardFilenameBuffer, size, &p_file->m_file_info );
			break;
		}

		case ( File::mMODE_READ | File::mMODE_WRITE ):
		{
			open_result = CARDOpen( m_slot, cardFilenameBuffer, &p_file->m_file_info );
			break;
		}

		default:
		{
			Dbg_Assert( 0 );
			open_result = CARD_RESULT_FATAL_ERROR;
			break;
		}
	}

	if (open_result == CARD_RESULT_INSSPACE )
	{
		SetError(vINSUFFICIENT_SPACE);
	}
		
	if( open_result == CARD_RESULT_READY )
	{
		// Set the file number.
		p_file->m_file_number = CARDGetFileNo( &p_file->m_file_info );
		return p_file;
	}

	delete p_file;
	return NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static void s_convert_time(uint32 seconds, DateTime *p_dateTime)
{
	// seconds is the number of seconds since 01/01/2000 midnight
	
	OSCalendarTime ct;

	OSTicksToCalendarTime(OSSecondsToTicks((uint64)seconds),&ct);
	p_dateTime->m_Seconds = ct.sec;
	p_dateTime->m_Minutes = ct.min;
	p_dateTime->m_Hour = ct.hour;
	p_dateTime->m_Day = ct.mday;
	// The month value in OSCalendarTime starts at 0. May as well make it consistent
	// with the other platforms by adding 1. It doesn't really matter whether 1 is added
	// or not though cos the date is only used to determine the most recent save.
	p_dateTime->m_Month = ct.mon+1;
	p_dateTime->m_Year = ct.year;
}

bool Card::GetFileList( const char* mask, Lst::Head< File > &file_list )
{
	for( int i = 0; i < 127; ++i )
	{
		CARDStat stat;
		s32	result = CARDGetStatus( 0, i, &stat );
		if( result == CARD_RESULT_READY )
		{
			File* new_file;

			new_file = new File( 0, this );
			strcpy( new_file->m_Filename, (char*)stat.fileName );
			new_file->m_Size = stat.length;
			
			new_file->m_Attribs = 0;
			new_file->m_Attribs |= File::mATTRIB_READABLE;
			new_file->m_Attribs |= File::mATTRIB_WRITEABLE;

			s_convert_time(stat.time,&new_file->m_Modified);
			
//			if( file_table[i].AttrFile & sceMcFileAttrExecutable )
//			{
//				new_file->m_Attribs |= File::mATTRIB_EXECUTABLE;
//			}
//			if( file_table[i].AttrFile & sceMcFileAttrDupProhibit )
//			{
//				new_file->m_Attribs |= File::mATTRIB_DUP_PROHIBITED;
//			}
//			if( file_table[i].AttrFile & sceMcFileAttrSubdir )
//			{
//				new_file->m_Attribs |= File::mATTRIB_DIRECTORY;
//			}
//			if( file_table[i].AttrFile & sceMcFileAttrClosed )
//			{
//				new_file->m_Attribs |= File::mATTRIB_CLOSED;
//			}
//			if( file_table[i].AttrFile & sceMcFileAttrPDAExec )
//			{
//				new_file->m_Attribs |= File::mATTRIB_PDA_APP;
//			}
//			if( file_table[i].AttrFile & sceMcFileAttrPS1 )
//			{
//				new_file->m_Attribs |= File::mATTRIB_PS1_FILE;
//			}
//			new_file->m_Created.m_Seconds = file_table[i]._Create.Sec;
//			new_file->m_Created.m_Minutes = file_table[i]._Create.Min;
//			new_file->m_Created.m_Hour = file_table[i]._Create.Hour;
//			new_file->m_Created.m_Day = file_table[i]._Create.Day;
//			new_file->m_Created.m_Month = file_table[i]._Create.Month;
//			new_file->m_Created.m_Year = file_table[i]._Create.Year;

//			new_file->m_Modified.m_Seconds = file_table[i]._Modify.Sec;
//			new_file->m_Modified.m_Minutes = file_table[i]._Modify.Min;
//			new_file->m_Modified.m_Hour = file_table[i]._Modify.Hour;
//			new_file->m_Modified.m_Day = file_table[i]._Modify.Day;
//			new_file->m_Modified.m_Month = file_table[i]._Modify.Month;
//			new_file->m_Modified.m_Year = file_table[i]._Modify.Year;

			file_list.AddToTail( new_file );
		}
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Card::CountFilesLeft()
{
	int NumFiles=0;
	for( int i = 0; i < 127; ++i )
	{
		CARDStat stat;
		s32	result = CARDGetStatus( 0, i, &stat );
		if( result == CARD_RESULT_READY )
		{
			++NumFiles;
		}
	}
	return 127-NumFiles;
}

File::File( int fd, Card* card ) : Lst::Node< File > ( this ), m_fd( fd ), m_card( card )
{
	m_need_to_flush=false;
	m_write_offset=0;
	mp_write_buffer=NULL;
	
	m_read_offset=0;
	mp_read_buffer=NULL;
}
		
File::~File()
{
	Dbg_MsgAssert(!m_need_to_flush,("Closed mem card file when it needed flushing"));
	
	if (mp_write_buffer)
	{
		Mem::Free(mp_write_buffer);
		mp_write_buffer=NULL;
	}	
	if (mp_read_buffer)
	{
		Mem::Free(mp_read_buffer);
		mp_read_buffer=NULL;
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int File::Seek( int offset, FilePointerBase base )
{
	m_read_offset=offset;
	// Force a reload of the read buffer by deleting it.
	if (mp_read_buffer)
	{
		Mem::Free(mp_read_buffer);
		mp_read_buffer=NULL;
	}
		
	int result = 0;

	switch( base )
	{
		case BASE_START:
			break;
		case BASE_CURRENT:
			Dbg_MsgAssert( 0, ( "Not supported" ));
			break;
		case BASE_END:
		{
			CARDStat	status;
			s32			status_result = CARDGetStatus( m_card->GetSlot(), m_file_number, &status );
			if( status_result == CARD_RESULT_READY )
			{
				result = status.length - offset;
			}
			break;
		}
		default:
			Dbg_MsgAssert( 0,( "Invalid FilePointerBase\n" ));
			return 0;
			break;
	}

	return result;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool File::Flush( void )
{
	// The FlushFileBuffers() is pretty strict about what types of files wmay be flushed,
	// whereas the PS2 equivalent doesn't really care. Just return a positive response always,
	// no critical stuff predicated on this return anway.
	
	if (!m_need_to_flush)
	{
		return true;
	}
	
	int num_bytes_to_write=m_write_offset%m_card->m_sector_size;
	Dbg_MsgAssert(num_bytes_to_write,("Got zero num_bytes_to_write in File::Flush"));
	
	Dbg_MsgAssert(mp_write_buffer,("NULL mp_write_buffer"));
	memset(mp_write_buffer+num_bytes_to_write,0,m_card->m_sector_size-num_bytes_to_write);
	
	int file_offset=m_write_offset-num_bytes_to_write;
	
	Dbg_MsgAssert(file_offset%m_card->m_sector_size == 0,("Bad file_offset"));
	s32 write_result = CARDWrite( &m_file_info, mp_write_buffer, m_card->m_sector_size, file_offset );
	if ( write_result != CARD_RESULT_READY )
	{
		return false;
	}
	
	m_need_to_flush=false;
	m_write_offset=0;
	if (mp_write_buffer)
	{
		Mem::Free(mp_write_buffer);
		mp_write_buffer=NULL;
	}	
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	File::Write( void* data, int len )
{
	Dbg_MsgAssert(data,("NULL data"));
	Dbg_Assert( len > 0 );
	Dbg_Assert( m_card->m_sector_size > 0 );

	if (!mp_write_buffer)
	{
		// Grab a sector-sized chunk of memory, ensuring it is 32-byte aligned.
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
		Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
		mp_write_buffer = (uint8*)Mem::Malloc(m_card->m_sector_size);
		Mem::Manager::sHandle().BottomUpHeap()->PopAlign();
		Mem::Manager::sHandle().PopContext();
	}
	Dbg_MsgAssert(mp_write_buffer,("NULL mp_write_buffer"));
	
	// Ensure the buffer address is 32 byte aligned.
	Dbg_MsgAssert((((unsigned int)mp_write_buffer ) & 31 ) == 0, ( "file write buffer must be 32 byte aligned" ));

	m_need_to_flush=false;

	int	bytes_remaining_to_write = len;
	unsigned char*	p_source_buffer	= (unsigned char*)data;

	int write_buffer_sector_offset=m_write_offset%m_card->m_sector_size;
	int write_buffer_left=m_card->m_sector_size - write_buffer_sector_offset;
		
	if (bytes_remaining_to_write < write_buffer_left)
	{
		memcpy( mp_write_buffer+write_buffer_sector_offset, p_source_buffer, bytes_remaining_to_write);
		m_write_offset+=bytes_remaining_to_write;
		m_need_to_flush=true;
		return len;
	}

	int file_offset=m_write_offset-write_buffer_sector_offset;

	if (write_buffer_left)
	{
		Dbg_MsgAssert(write_buffer_sector_offset+write_buffer_left==m_card->m_sector_size,("Oops"));
		memcpy( mp_write_buffer+write_buffer_sector_offset, p_source_buffer, write_buffer_left);
		m_write_offset+=write_buffer_left;
		bytes_remaining_to_write-=write_buffer_left;
		p_source_buffer+=write_buffer_left;
		
		Dbg_MsgAssert(file_offset%m_card->m_sector_size == 0,("Bad file_offset"));
		s32 write_result = CARDWrite( &m_file_info, mp_write_buffer, m_card->m_sector_size, file_offset );
		if ( write_result != CARD_RESULT_READY )
		{
			return 0;
		}
		file_offset+=m_card->m_sector_size;
		m_need_to_flush=false;
	}

	while (bytes_remaining_to_write)
	{
		if (bytes_remaining_to_write >= m_card->m_sector_size)
		{
			Dbg_MsgAssert(m_write_offset%m_card->m_sector_size == 0,("Oops"));
			
			memcpy( mp_write_buffer, p_source_buffer, m_card->m_sector_size);
			m_write_offset+=m_card->m_sector_size;
			bytes_remaining_to_write-=m_card->m_sector_size;
			p_source_buffer+=m_card->m_sector_size;
			
			Dbg_MsgAssert(file_offset%m_card->m_sector_size == 0,("Bad file_offset"));
			s32 write_result = CARDWrite( &m_file_info, mp_write_buffer, m_card->m_sector_size, file_offset );
			if ( write_result != CARD_RESULT_READY )
			{
				return 0;
			}
			file_offset+=m_card->m_sector_size;
			m_need_to_flush=false;
		}
		else
		{
			memcpy( mp_write_buffer, p_source_buffer, bytes_remaining_to_write);
			memset( mp_write_buffer+bytes_remaining_to_write, 0, m_card->m_sector_size-bytes_remaining_to_write);
			
			m_write_offset+=bytes_remaining_to_write;
			bytes_remaining_to_write=0;
			m_need_to_flush=true;
		}
	}
	return len;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int	File::Read( void* buff, int len )
{
	Dbg_Assert( len > 0 );
	Dbg_Assert( m_card->m_sector_size > 0 );

	int read_buffer_sector_offset=m_read_offset%m_card->m_sector_size;
	int read_buffer_left=m_card->m_sector_size - read_buffer_sector_offset;

	if (!mp_read_buffer)
	{
		// Similar process to write, in that we read 1 sector at a time. So grab a sector-sized chunk of memory,
		// ensuring it is 32-byte aligned.
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
		Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
		mp_read_buffer = (uint8*)Mem::Malloc(m_card->m_sector_size);
		Mem::Manager::sHandle().BottomUpHeap()->PopAlign();
		Mem::Manager::sHandle().PopContext();
		
		Dbg_MsgAssert(mp_read_buffer,("NULL mp_read_buffer"));
		
		// Ensure the buffer address is 32 byte aligned.
		Dbg_MsgAssert((((unsigned int)mp_read_buffer ) & 31 ) == 0, ( "file read buffer must be 32 byte aligned" ));
		
		// Also ensure the buffer is a multiple of CARD_READ_SIZE (should be, since this is smaller than the smallest sector).
		Dbg_MsgAssert(( m_card->m_sector_size & ( CARD_READ_SIZE - 1 )) == 0, ( "read buffer must be multiple of CARD_READ_SIZE" ));
		

		int file_offset=m_read_offset-read_buffer_sector_offset;
		Dbg_MsgAssert(file_offset%m_card->m_sector_size == 0,("Bad file_offset"));
		s32 read_result = CARDRead( &m_file_info, mp_read_buffer, m_card->m_sector_size, file_offset);
		if (read_result != CARD_RESULT_READY)
		{
			return 0;
		}	
	}
	else
	{
		if (read_buffer_sector_offset==0)
		{
			Dbg_MsgAssert(m_read_offset%m_card->m_sector_size == 0,("Bad m_read_offset"));
			s32 read_result = CARDRead( &m_file_info, mp_read_buffer, m_card->m_sector_size, m_read_offset);
			if (read_result != CARD_RESULT_READY)
			{
				return 0;
			}	
		}	
	}


	int	bytes_remaining_to_read	= len;
	unsigned char*	p_dest_buffer	= (unsigned char*)buff;

	while (true)
	{
		if (bytes_remaining_to_read < read_buffer_left)
		{
			if (bytes_remaining_to_read)
			{
				// The required data is all in the buffer already so copy it out and return.
				memcpy(p_dest_buffer,mp_read_buffer+read_buffer_sector_offset,bytes_remaining_to_read);
				m_read_offset+=bytes_remaining_to_read;
			}	
			return len;
		}
			
		// Copy out what is left in the buffer
		memcpy(p_dest_buffer,mp_read_buffer+read_buffer_sector_offset,read_buffer_left);
		p_dest_buffer+=read_buffer_left;
		m_read_offset+=read_buffer_left;
		bytes_remaining_to_read-=read_buffer_left;
		
		if (bytes_remaining_to_read==0)
		{
			return len;
		}
		
		
		Dbg_MsgAssert(m_read_offset%m_card->m_sector_size == 0,("Bad m_read_offset"));
		s32 read_result = CARDRead( &m_file_info, mp_read_buffer, m_card->m_sector_size, m_read_offset);
		if (read_result != CARD_RESULT_READY)
		{
			return 0;
		}	
			
		read_buffer_left=m_card->m_sector_size;
		read_buffer_sector_offset=0;
	}
			
	return 0;	
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool File::Close( void )
{
	s32 close_result = CARDClose( &m_file_info );
	if( close_result == CARD_RESULT_READY )
	{
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mc





