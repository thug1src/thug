/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GEL (Game Engine Library)								**
**																			**
**	Module:			Input (INP) 		 									**
**																			**
**	File name:		inpman.cpp												**
**																			**
**	Created:		05/26/2000	-	spg										**
**																			**
**	Description:	Input manager code										**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/singleton.h>

#ifdef __NOPT_DEBUG__
#include <sys/timer.h>
#endif

#include <sys/sioman.h>
#include <sys/siodev.h>
 
#include <gel/inpman.h>

#include <gel/scripting/script.h> 
#include <gel/scripting/checksum.h> 

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/



namespace Inp
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

DefineSingletonClass( Manager, "Input Manager" );

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

Manager::Manager ( void )
{
	
	int i;
	
	SIO::Manager * sio_manager = SIO::Manager::Instance();

	m_process_handlers_task = new Tsk::Task< Manager >( process_handlers, *this, Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_PROCESS_HANDLERS );
	for( i = 0; i < SIO::vMAX_DEVICES; i++ )
	{
		m_server[i].m_device = sio_manager->GetDeviceByIndex( i );
	}

	m_override_pad_with_stick = true;
	Dbg_AssertType( m_process_handlers_task, Tsk::Task< Manager > );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Manager::~Manager( void )
{
	
	
	Dbg_AssertType( m_process_handlers_task, Tsk::Task< Manager > );
	delete m_process_handlers_task;   
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::process_handlers( const Tsk::Task< Manager >& task )
{
	

	int i;
	Manager&	manager = task.GetData();      

	for( i = 0; i < SIO::vMAX_DEVICES; i++ )
	{
		// See if we hit the end of our record/playback buffers
		if( ( manager.m_server[i].m_data_in ) &&
			( manager.m_server[i].m_data_in >= manager.m_server[i].m_data_in_end ))
		{
			manager.m_server[i].m_data_in = NULL;
		}
		if( ( manager.m_server[i].m_data_out ) &&
			( manager.m_server[i].m_data_out >= manager.m_server[i].m_data_out_end ))
		{
			manager.m_server[i].m_data_out = NULL;
		}
		
        manager.m_server[i].service_handlers();
	}   
}

// Note: The order of these checksums must match the corresponding order of the PAD_ defines
// for the buttons in inpman.h
static uint32 ButtonChecksums[PAD_NUMBUTTONS]=
{
	0x0,		// Nowt
	0xbc6b118f, // Up
	0xe3006fc4, // Down
	0x85981897, // Left
	0x4b358aeb, // Right
	0xb7231a95, // UpLeft
	0xa50950c5, // UpRight
	0xd8847efa, // DownLeft
	0x786b8b68, // DownRight
	0x2b489a86, // Circle
	0x321c9756, // Square
	0x7323e97c, // X
	0x20689278, // Triangle
	0x26b0c991, // L1
	0xbfb9982b, // L2
	0xc8bea8bd, // L3
	0xf2f1f64e, // R1
	0x6bf8a7f4, // R2
	0x1cff9762, // R3
	0x767a45d7, // Black
	0xbd30325b, // White
	0x9d2d8850, // Z
};	

uint32 GetButtonChecksum( int whichButton )
{
	
	Dbg_MsgAssert( ( whichButton > 0 ) && ( whichButton < PAD_NUMBUTTONS ), ( "button %d out of range", whichButton ) );
	return ( ButtonChecksums[ whichButton ] );
}

int GetButtonIndex(uint32 Checksum)
{
	
	for (int i=1; i<PAD_NUMBUTTONS; ++i)
	{
		if (ButtonChecksums[i]==Checksum)
		{
			return i;
		}
	}
	Dbg_MsgAssert(0,("Bad button name '%s' sent to GetButtonIndex",Script::FindChecksumName(Checksum)));
	return 0;
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

void	Manager::RecordInput( int index, RecordedData *data_buffer, int byte_length )
{
	
	
	Dbg_MsgAssert( index < SIO::vMAX_DEVICES,( "Invalid controller index" ));
	Dbg_Assert( data_buffer );
	
	m_server[index].m_data_out = data_buffer;
	m_server[index].m_data_out_end = (sint8 *) data_buffer + byte_length;   
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::PlaybackInput( int index, RecordedData *recorded_input, int byte_length )
{
	
	
	Dbg_MsgAssert( index < SIO::vMAX_DEVICES,( "Invalid controller index" ));
	Dbg_Assert( recorded_input );   
	
	m_server[index].m_data_in = recorded_input;
	m_server[index].m_data_in_end = (sint8 *) recorded_input + byte_length;  
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::PushInputLogicTasks( void )
{
	
	
	int i;

	for( i = 0; i < SIO::vMAX_DEVICES; i++ )
	{
        m_server[i].m_handler_stack.Push();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::PopInputLogicTasks( void )
{
	

	int i;

	for( i = 0; i < SIO::vMAX_DEVICES; i++ )
	{
        m_server[i].m_handler_stack.Pop();
	}
}

bool	Manager::ActuatorEnabled( int i )
{
	
	Dbg_MsgAssert(i>=0 && i<SIO::vMAX_DEVICES,("Bad actuator index sent to DisableActuator"));
	if (m_server[i].m_device)
	{
		return m_server[i].m_device->Enabled();
	}	
	else
	{
		return false;
	}	
}

/******************************************************************/
/*                                                                */
/* K: Disables the actuators on all devices.                      */
/* Used by the VibrationOff script command.						  */
/*                                                                */
/******************************************************************/

void	Manager::DisableActuators( void )
{
	

	int i;

	for( i = 0; i < SIO::vMAX_DEVICES; i++ )
	{
		if (m_server[i].m_device)
		{
			m_server[i].m_device->DisableActuators();
		}	
	}
}

// Disables just one actuator. Called by the VibrationOff script command.
void	Manager::DisableActuator( int i )
{
	

	Dbg_MsgAssert(i>=0 && i<SIO::vMAX_DEVICES,("Bad actuator index sent to DisableActuator"));
	if (m_server[i].m_device)
	{
		m_server[i].m_device->DisableActuators();
	}	
}

/******************************************************************/
/*                                                                */
/* K: Enables the actuators on all devices.                       */
/* Used by the VibrationOn script command.						  */
/*                                                                */
/******************************************************************/

void	Manager::EnableActuators( void )
{
	

	int i;

	for( i = 0; i < SIO::vMAX_DEVICES; i++ )
	{
		if (m_server[i].m_device)
		{
			m_server[i].m_device->EnableActuators();
		}	
	}
}

// Enables just one actuator. Called by the VibrationOn script command.
void	Manager::EnableActuator( int i )
{
	
	Dbg_MsgAssert(i>=0 && i<SIO::vMAX_DEVICES,("Bad actuator index %i sent to EnableActuator", i));
	if (m_server[i].m_device)
	{
		m_server[i].m_device->EnableActuators();
	}	
}


/******************************************************************/
/*                                                                */
/* Mick: stops the actuators on all devices.                      */
/*                                                                */
/******************************************************************/

void	Manager::ResetActuators( void )
{
	

	int i;

	for( i = 0; i < SIO::vMAX_DEVICES; i++ )
	{
		if (m_server[i].m_device)
		{
			m_server[i].m_device->ResetActuators();
		}	
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetAnalogStickOverride( bool should_override_pad )
{
	m_override_pad_with_stick = should_override_pad;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ShouldAnalogStickOverride( void )
{
	return m_override_pad_with_stick;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Inp
