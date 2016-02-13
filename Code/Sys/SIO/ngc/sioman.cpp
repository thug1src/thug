/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		SYS Library												**
**																			**
**	Module:			SYS (SYS_) 												**
**																			**
**	File name:		sioman.cpp												**
**																			**
**	Created:		05/26/2000	-	spg										**
**																			**
**	Description:	Serial IO Manager										**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <sys/sioman.h>
#include <sys/siodev.h>
//#include <sys/sio/keyboard.h>

#include <gel/module.h>
//#include <gel/soundfx/soundfx.h>
//#include <gel/soundfx/xbox/p_sfx.h>
#include <gel/music/music.h>

#include <dolphin.h>
#include "sys/ngc/p_hwpad.h"

	PADStatus padData[PAD_MAX_CONTROLLERS]; // game pad state

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace SIO
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/


/*****************************************************************************
**								Private Types								**
*****************************************************************************/


/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

DefineSingletonClass( Manager, "Serial IO Manager" );

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/



/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

void Manager::process_devices( const Tsk::Task< Manager::DeviceList >& task )
{
//	Dbg_AssertType ( task, Tsk::Task< INP_MANAGER::DEVICE_LIST > );

	Device*					device;
	Lst::Search< Device >	    sh;
	Manager::DeviceList&	device_list = task.GetData();

	device = sh.FirstItem ( device_list );

	// Deal with hot-switching controllers.
	u32 reset_mask = 0;
	if( padData[0].err == PAD_ERR_NO_CONTROLLER )
	{
		reset_mask |= PAD_CHAN0_BIT;
	}
	if( padData[1].err == PAD_ERR_NO_CONTROLLER )
	{
		reset_mask |= PAD_CHAN1_BIT;
	}
	if( reset_mask )
	{
		PADReset( reset_mask );
	}

	while ( device )
	{
		Dbg_AssertType ( device, Device );

		device->process();
		device = sh.NextItem();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Device* Manager::create_device( int index, int port, int slot )
{
    Device *device;
        
    device = new Device( index, port, slot );

    return device;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Manager::Manager ( void )
{
    int i, j, index;

//	XDEVICE_PREALLOC_TYPE xdpt[] = {{ XDEVICE_TYPE_GAMEPAD,			4 },
//									{ XDEVICE_TYPE_MEMORY_UNIT,		8 }};
////									{ XDEVICE_TYPE_DEBUG_KEYBOARD,	1 }};
//
//	// Initialize the peripherals.
//	XInitDevices( sizeof( xdpt ) / sizeof( XDEVICE_PREALLOC_TYPE ), xdpt );

	// Create the keyboard queue.
//	XInputDebugInitKeyboardQueue( &xdkp );

//#	if KEYBOARD_ON
//	// Initialize the keyboard.
//	KeyboardInit();
//#	endif

    m_process_devices_task = new Tsk::Task< DeviceList > ( Manager::process_devices, m_devices );
    
	// Pause briefly here to give the system time to enumerate the attached devices.
//	Sleep( 500 );

    index = 0;
    for( i = 0; i < vMAX_PORT; i++ )
    {
//		for( j = 0; j < scePadGetSlotMax ( i ); j++ )
		for( j = 0; j < 1; j++ )
		{
		    Device* p_device;

			if(( p_device = create_device ( index, i, j )))
			{
				m_devices.AddToTail( p_device->m_node );
				p_device->Acquire();
				index++;
			}
		}
    }
    
#	if !NO_MUSIC_PLEASE
	// Moved this since the pcm stuff for ngc needs the sfx stuff initialised first.
//	Pcm::Init();
#	endif

    Dbg_Message( "Initialized IOP controller lib\n" );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Manager::~Manager( void )
{
    Device*					device;
    Device*					next;
    Lst::Search< Device >	sh;
    
    device = sh.FirstItem ( m_devices );
    
    while ( device )
    {
        Dbg_AssertType ( device, Device );
    
        next = sh.NextItem();
    
        delete device;
        device = next;
    }

    delete m_process_devices_task;

	Dbg_Message( "Shut down IOP Controller Lib\n" );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Device* Manager::GetDevice( int port, int slot )
{
    Device*					device;
    Lst::Search< Device >	sh;
    
    device = sh.FirstItem ( m_devices );
    
    for( device = sh.FirstItem ( m_devices ); device;
            device = sh.NextItem ())
    {
        if( ( device->GetPort() == port ) &&
            ( device->GetSlot() == slot ))
        {
            return device;
        }
    }
    
    return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Device* Manager::GetDeviceByIndex( int index )
{
    Device*					device;
    Lst::Search< Device >	sh;
    
    device = sh.FirstItem ( m_devices );
    
    for( device = sh.FirstItem ( m_devices ); device;
            device = sh.NextItem ())
    {
        if( device->GetIndex() == index )
        {
            return device;
        }
    }
    
    return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::Pause()
{
    Device*					device;
    Lst::Search< Device >	sh;
    
    for( device = sh.FirstItem( m_devices ); device; device = sh.NextItem())
    {
		device->Pause();
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::UnPause( void )
{
    Device*					device;
    Lst::Search< Device >	sh;
    
    for( device = sh.FirstItem( m_devices ); device; device = sh.NextItem())
    {
		device->UnPause();
    }
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace SIO



