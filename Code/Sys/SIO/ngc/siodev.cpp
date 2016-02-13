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
**	File name:		siodev.cpp												**
**																			**
**	Created:		05/26/2000	-	spg										**
**																			**
**	Description:	Generic input device									**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <stdlib.h>
#include <string.h>
         
#include <core/defines.h>
#include <sys/sioman.h>
#include <sys/mem/memman.h>
//#include <libpad.h>

#include <dolphin.h>
#include "sys/ngc/p_hwpad.h"

// PJR - Doh!
extern PADStatus padData[PAD_MAX_CONTROLLERS]; // game pad state
/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								DBG Defines									**
*****************************************************************************/

namespace SIO
{
  

// Maps Xbox thumbstick values in the range [-32767, 32767] to PS2 thumstick values in the range [0, 255].
#define XboxThumbToPS2Thumb( v )	(( v + 32767 ) / 256 )


/*****************************************************************************
**								Private Types								**
*****************************************************************************/


/*****************************************************************************
**								 Private Data								**
*****************************************************************************/


/*****************************************************************************
**								 Public Data								**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
    
void Device::process( void )
{
    switch( m_state )
    {

    case vIDLE:
        break;

    case vBUSY:
        wait();
        break;

    case vACQUIRING:
        acquisition_pending();
        break;

    case vACQUIRED:
        read_data();
        break;

    default:
        break;

    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Device::wait( void )
{
    Dbg_MsgAssert(( m_next_state >= 0 ) && ( m_next_state <= vNUM_STATES ),( "No next state set for wait state" ));

	if ( get_status() != vNOTREADY )
	{
		m_state = m_next_state;
		m_next_state = vIDLE;
	}
}


//int gButton_Option = 0;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void Device::read_data( void )
{
	PADStatus * p;

	// Need to check here for the controller becoming detached.
	if( padData[m_data.m_port].err == PAD_ERR_NO_CONTROLLER )
	{
		m_plugged_in	= false;
		m_data.m_valid	= false;
		Unacquire();
		Acquire();
		return;
	}

	m_data.m_valid	= true;
	m_plugged_in	= true;

	p = &padData[m_data.m_port];

	// Convert this data back into PS2-style 'raw' data for a DUALSHOCK2.
	m_data.m_control_data[0] = 0;						// 'Valid' info flag.
	m_data.m_control_data[1] = ( 0x07 << 4 ) | 16;		// DUALSHOCK2 id + data length.

	m_data.m_control_data[2] = 0xFF;					// Turn off all buttons by default.
	m_data.m_control_data[3] = 0xFF;					// Turn off all buttons by default.

	m_data.m_control_data[2] ^= ( ( p->button & PAD_BUTTON_START ) && !( p->button & PAD_TRIGGER_Z ) ) ? ( 1 << 3 ) : 0;	// Gamecube 'Start' = PS2 'Start' (only if Z is not also pressed).

	#if 0 // Dan: none of the Z-shifting crap
	// Gamecube 'Z' = PS2 'Select'.
	// Directly mapping to select is disabled.

	// Gamecube 'Z' plus 'L' = PS2 'Right Stick Button'.
	// Gamecube 'Z' plus 'R' = PS2 'shift' (this allows the camera mode shifting).
	if( p->button & PAD_TRIGGER_Z )
	{
		// with Z pressed, triggers are L2/R2
		
		// GameCube 'Z' plus 'L' = PS2 'Right Stick Button'. 
		if( p->button & PAD_TRIGGER_L )
		{
			m_data.m_control_data[2] ^= ( 1 << 1 );
		} else

		// GameCube 'Z' plus 'R' = PS2 'Right Stick Button'. 
		if( p->button & PAD_TRIGGER_R )
		{
			m_data.m_control_data[2] ^= ( 1 << 2 );
		} else

//		// Z button + START is PS2 'Select'.
//		if( p->button & PAD_BUTTON_START )
//		{
//			m_data.m_control_data[2] ^= ( 1 << 0 );
//		}
		// Z button without L & R is PS2 L1+R1.
		{
			m_data.m_control_data[3] ^= ( 1 << 2 ) | ( 1 << 3 );
			m_data.m_control_data[2] ^= ( 1 << 0 );
		}
	}
	#endif

	// Temp hack to get fly-around working.
	if( p->button & PAD_TRIGGER_Z )
	{
		m_data.m_control_data[2] ^= ( 1 << 0 );
	}

	m_data.m_control_data[3] ^= ( p->button & PAD_BUTTON_Y ) ? ( 1 << 4 ) : 0;		// Gamecube 'Y' = PS2 'Triangle'.
	m_data.m_control_data[3] ^= ( p->button & PAD_BUTTON_X ) ? ( 1 << 5 ) : 0;		// Gamecibe 'X' = PS2 'Circle'.
	m_data.m_control_data[3] ^= ( p->button & PAD_BUTTON_A ) ? ( 1 << 6 ) : 0;		// Gamecube 'A' = PS2 'X'.
	m_data.m_control_data[3] ^= ( p->button & PAD_BUTTON_B ) ? ( 1 << 7 ) : 0;		// Gamecube 'B' = PS2 'Square'.

	// Dan: none of this analog crap
	// m_data.m_control_data[3] ^= ( p->triggerLeft  > 128 ) ? ( 1 << 2 ) : 0;		// XBox 'Left Trigger pressed down > halfway' = PS2 'L1'.
	// m_data.m_control_data[3] ^= ( p->triggerRight > 128 ) ? ( 1 << 3 ) : 0;		// XBox 'Right Trigger pressed down > halfway' = PS2 'R1'.
	// m_data.m_control_data[3] ^= ( p->triggerLeft  > 16 ) ? ( 1 << 0 ) : 0;		// XBox 'Left Trigger' = PS2 'L2'.
	// m_data.m_control_data[3] ^= ( p->triggerRight > 16 ) ? ( 1 << 1 ) : 0;		// XBox 'Right Trigger' = PS2 'R2'.
	
	m_data.m_control_data[3] ^= ( p->triggerLeft  > 128 ) ? ( 1 << 2 ) : 0;			// Gamecube 'Left Trigger = PS2 'L1'.
	m_data.m_control_data[3] ^= ( p->triggerRight > 128 ) ? ( 1 << 3 ) : 0;			// Gamecube 'Right Trigger = PS2 'R1'.
	
	// Dan: Store the state of Z in m_control_data[20] above Xbox's black and white buttons.
	m_data.m_control_data[20]=0;
	if (p->button & PAD_TRIGGER_Z)
	{
		m_data.m_control_data[20] |= (1<<2);
	}	

	// Make analog buttons full depression if pressed.
	// Ngc analog buttons return analog value in range [0, 255].
	m_data.m_control_data[12] = ( p->button & PAD_BUTTON_Y ) ? 255 : 0;				// Gamecube 'Y' = PS2 Analog 'Triangle'.
	m_data.m_control_data[13] = ( p->button & PAD_BUTTON_X ) ? 255 : 0;				// Gamecube 'X' = PS2 Analog 'Circle'.
	m_data.m_control_data[14] = p->analogA;											// Gamecube 'A' = PS2 Analog 'X'.
	m_data.m_control_data[15] = p->analogB;											// Gamecube 'B' = PS2 'Square'.
	
	m_data.m_control_data[16] = ( p->triggerLeft  > 128 ) ? 255 : 0;				// Gamecube 'Left Trigger' = PS2 Analog 'L1'.
	m_data.m_control_data[17] = ( p->triggerRight > 128 ) ? 255 : 0;				// Gamecube 'Right Trigger' = PS2 Analog 'R1'.
	m_data.m_control_data[18] = 0;													// Gamecube no 'L2'
	m_data.m_control_data[19] = 0;													// Gamecube no 'R2'

	// Zero out the d-pad pressure values.
	m_data.m_control_data[8]	= 0x00;
	m_data.m_control_data[9]	= 0x00;
	m_data.m_control_data[10]	= 0x00;
	m_data.m_control_data[11]	= 0x00;

	// Handle 8 position d-pad.
	m_data.m_control_data[2] ^= ( p->button & PAD_BUTTON_UP ) ? ( 1 << 4 ) : 0;		// Gamecube 'DPad Up' = PS2 'DPad Up'.
	m_data.m_control_data[10] = ( p->button & PAD_BUTTON_UP ) ? 0xFF : 0;			// Gamecube 'DPad Up' = PS2 'DPad Up'.
	m_data.m_control_data[2] ^= ( p->button & PAD_BUTTON_RIGHT ) ? ( 1 << 5 ) : 0;	// Gamecube 'DPad Right' = PS2 'DPad Right'.
	m_data.m_control_data[8] = ( p->button & PAD_BUTTON_RIGHT ) ? 0xFF : 0;			// Gamecube 'DPad Right' = PS2 'DPad Right'.
	m_data.m_control_data[2] ^= ( p->button & PAD_BUTTON_DOWN ) ? ( 1 << 6 ) : 0;	// Gamecube 'DPad Down' = PS2 'DPad Down'.
	m_data.m_control_data[11] = ( p->button & PAD_BUTTON_DOWN ) ? 0xFF : 0;			// Gamecube 'DPad Down' = PS2 'DPad Down'.
	m_data.m_control_data[2] ^= ( p->button & PAD_BUTTON_LEFT ) ? ( 1 << 7 ) : 0;	// Gamecube 'DPad Left' = PS2 'DPad Left'.
	m_data.m_control_data[9] = ( p->button & PAD_BUTTON_LEFT ) ? 0xFF : 0;			// Gamecube 'DPad Left' = PS2 'DPad Left'.

	// Gamecube thumbsticks return analog value in range [-128, 127].
	int stx = (int)(((float)p->stickX) * 2.0f );
	int sty = (int)(((float)p->stickY) * 2.0f );
	if ( stx < -128 ) stx = -128;
	if ( stx > 127 ) stx = 127;
	if ( sty < -128 ) sty = -128;
	if ( sty > 127 ) sty = 127;

	int subx = (int)(((float)p->substickX) * 2.0f );
	int suby = (int)(((float)p->substickY) * 2.0f );
	if ( subx < -128 ) subx = -128;
	if ( subx > 127 ) subx = 127;
	if ( suby < -128 ) suby = -128;
	if ( suby > 127 ) suby = 127;

	m_data.m_control_data[4] = (unsigned char)(subx+128);		// Analog stick right (X direction).
	m_data.m_control_data[5] = 255 - (unsigned char)(suby+128);	// Analog stick right (Y direction).
	m_data.m_control_data[6] = (unsigned char)(stx+128);		// Analog stick left (X direction).
	m_data.m_control_data[7] = 255 - (unsigned char)(sty+128);	// Analog stick left (Y direction).
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Device::IsPluggedIn( void )
{
	return m_plugged_in;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Device::acquisition_pending( void )
{
    int status;

    status = get_status();
                        
    if(( status == vCONNECTED ) || ( status == vREADY ))
    {
        // sucessful.  Now query the controller for capabilities
        query_capabilities();
    }
        
	// failed to acquire controller
	// stay in this state and continue to try to acquire it or prompt the user if it is mandatory
	// that they have a controller in   
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Device::query_capabilities( void )
{   
	// Currently assumes standard Gamecube controller.
    int id = vANALOG_CTRL;
    
    m_data.m_type = id;

	switch( id )
	{   
		case vANALOG_CTRL:
		{
			m_data.m_caps.SetMask( mANALOG_BUTTONS );

			// GameCube actually only has 1, but we need to track 2 since PS2 and Xbox have 2,
			// and incoming requests could be for either.
			m_data.m_num_actuators = 2;
			m_data.m_caps.SetMask( mACTUATORS );
			m_data.m_actuator_max[0] = 1;
			m_data.m_actuator_max[1] = 1;

			m_state = vBUSY;
			m_next_state = vACQUIRED;
			return;
		}

		default:
		{
			Dbg_Message( "Detected Controller of unknown type in %d:%d", m_data.m_port, m_data.m_slot );
			break;
		}
	}
    m_state = vACQUIRED;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Device::get_status( void )
{
	if( m_data.m_port < 4 )
	{
		if( padData[m_data.m_port].err == PAD_ERR_NONE )
		{
			return vREADY;
		}
		else
		{
			// Could do more checking here of the return value.
			return vDISCONNECTED;
		}
	}
	return vDISCONNECTED;
}



/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

Device::Device ( int index, int port, int slot )
{
	m_node = new Lst::Node< Device > ( this );
	Dbg_AssertType ( m_node, Lst::Node< Device > );

    Dbg_Assert( port < vMAX_PORT );
    Dbg_Assert( slot < vMAX_SLOT );

    m_state = vIDLE;
    m_next_state = vIDLE;
    m_index = index;

    // Initialize device
    m_data.m_port = port;
    m_data.m_slot = slot;
    m_data.m_caps.ClearAll();
    m_data.m_num_actuators = 0;
    m_data.m_valid = false;

    memset( m_data.m_actuator_direct, 0, ACTUATOR_BUFFER_LENGTH );
    memset( m_data.m_actuator_align, 0xFF, ACTUATOR_BUFFER_LENGTH );
    m_data.m_actuator_align[0] = 0;
    m_data.m_actuator_align[1] = 1;

    memset( m_data.m_control_data, 0, CTRL_BUFFER_LENGTH );
//	m_data.m_prealbuffer = Mem::Malloc( scePadDmaBufferMax * sizeof( uint128) + 63 );
	m_data.m_dma_buff =  (unsigned char*)((((uint32)m_data.m_prealbuffer)+63)&~63);
	
//    memset( m_data.m_dma_buff, 0, sizeof( uint128 ) * scePadDmaBufferMax );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
Device::~Device ( void )
{
	Dbg_AssertType ( m_node, Lst::Node< Device > );
//	Mem::Free( m_data.m_prealbuffer );

//	XInputClose( m_data.m_handle );

	delete m_node;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Device::Acquire( void )
{
    Dbg_Message( "Acquiring controller port %d slot %d\n", m_data.m_port, m_data.m_slot );

    if( m_state == vIDLE )
    {
		if( m_data.m_port < 4 )
		{
			// Acquire device handle.
//			m_data.m_handle = XInputOpen( XDEVICE_TYPE_GAMEPAD, m_data.m_port, XDEVICE_NO_SLOT, NULL );
//
//			if( m_data.m_handle )
			{
				// Store capabilites of the device
//				XINPUT_CAPABILITIES caps;
//				XInputGetCapabilities( m_data.m_handle, &caps );

//				memset( m_data.m_control_data, 0, CTRL_BUFFER_LENGTH );
				m_data.m_valid = false;
				m_state = vACQUIRING;
				return;
			}
		}
	}
	Dbg_Warning( "failed to open controller port %d slot %d\n", m_data.m_port, m_data.m_slot );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void Device::Unacquire ( void )
{
    Dbg_Message( "Unacquiring controller port %d slot %d\n", m_data.m_port, m_data.m_slot );

    if( m_state == vACQUIRED )
    {
//		if( scePadPortClose( m_data.m_port, m_data.m_slot ) != 1 )
//		{
//			Dbg_Warning( "failed to close controller port %d slot %d\n", m_data.m_port, m_data.m_slot );
//		}
    }

    m_state = vIDLE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void Device::ActivateActuator( int act_num, int percent )
{
	// Do nothing if the actuators are disabled.
	if( m_data.m_actuators_disabled )
	{
		return;
	}	
	
    float act_strength;
    
    // First, make sure we're in a ready state and our controller has actuators
    if(( m_state == vACQUIRED ) && ( m_data.m_caps.TestMask( mACTUATORS )))
    {
		// Rumble regardless. The problem here is that incoming rumble requests can be for motors
		// other than 0 - PS2 and Xbox have 2 motors. So, if the request is not for motor0, if it 
		if(( act_num >= 0 ) && ( act_num < m_data.m_num_actuators ))
        {
//			act_strength = ((float) percent * m_data.m_actuator_max[act_num] ) / 100.0f;
			act_strength = percent;

            // for lack of a rounding function, perform this check here
//			if( m_data.m_actuator_max[act_num] == 1 )
//			{
//				if( act_strength > 0.0f )
//				{
//					m_data.m_actuator_direct[act_num] = 1;
//				}
//				else
//				{
//					m_data.m_actuator_direct[act_num] = 0;
//				}
//			}
//			else
//			{
//				m_data.m_actuator_direct[act_num] = (unsigned char) act_strength;
//			}
			m_data.m_actuator_direct[act_num] = ( act_strength > 0.0f ) ? 1 : 0;
            
			// If either tracked motor is on, rumble the single motor, otherwise turn it off.
			PADControlMotor( m_data.m_port, (( m_data.m_actuator_direct[0] > 0 ) || ( m_data.m_actuator_direct[1] > 0 )) ? PAD_MOTOR_RUMBLE : PAD_MOTOR_STOP );
        }
    }
}

/******************************************************************/
/* These disable or enable pad vibration.
/******************************************************************/
void Device::DisableActuators()
{
	// If disabled already do nothing.
	if (m_data.m_actuators_disabled)
	{
		return;
	}
		
	// Run through all the actuators and make sure they're off.
    if(( m_state == vACQUIRED ) && ( m_data.m_caps.TestMask( mACTUATORS )))
    {
		for (int i=0; i<m_data.m_num_actuators; ++i)
		{
			// Switch it off.
			m_data.m_actuator_direct[i] = 0;
		}	

        //scePadSetActDirect( m_data.m_port, m_data.m_slot, m_data.m_actuator_direct );
		PADControlMotor( m_data.m_port, PAD_MOTOR_STOP );
	}	
	
	// Set the flag.
	m_data.m_actuators_disabled=true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void Device::EnableActuators( void )
{
	m_data.m_actuators_disabled=false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void Device::ResetActuators( void )
{
	if (m_data.m_actuators_disabled)
	{
		// If disabled, then should be off anyway
		// but toggle the states to make sure
		EnableActuators();
		DisableActuators();
	}
	else
	{
		// If enabled, then we disable them, which switched them off
		// and then enable them again (in the off position).
		DisableActuators();
		EnableActuators();
	}
	
}



/******************************************************************/
/*                                                                */
/* Ken: This gets called for each pad device when the game gets   */
/* paused. It remembers whether the pad was vibrating & switches  */
/* off vibration.
/*                                                                */
/******************************************************************/

void Device::Pause()
{
	// If paused already do nothing.
	if( !m_data.m_paused_ref.InUse())
	{		
	    // First, make sure we're in a ready state and our controller has actuators
	    if(( m_state == vACQUIRED ) && ( m_data.m_caps.TestMask( mACTUATORS )))
		{
			for (int i=0; i<m_data.m_num_actuators; ++i)
			{
				// Save the old actuator vibration strength.
				m_data.m_actuator_old_direct[i]=m_data.m_actuator_direct[i];

				// Then switch it off.
				m_data.m_actuator_direct[i] = 0;
			}	
			PADControlMotor( m_data.m_port, PAD_MOTOR_STOP );
		}	
	}
	else
	{
		// Just to be sure, ensure the motor is stopped.		
		PADControlMotor( m_data.m_port, PAD_MOTOR_STOP );
	}
	
	m_data.m_paused_ref.Acquire();
}



/******************************************************************/
/*                                                                */
/* Restores the vibration status.                                 */
/*                                                                */
/******************************************************************/

void Device::UnPause()
{
	// If not paused, do nothing.
	if( m_data.m_paused_ref.InUse())
	{		
		m_data.m_paused_ref.Release();
		if( !m_data.m_paused_ref.InUse())
		{
		    // First, make sure we're in a ready state and our controller has actuators
		    if(( m_state == vACQUIRED ) && ( m_data.m_caps.TestMask( mACTUATORS )))
		    {
				for( int i = 0; i < m_data.m_num_actuators; ++i )
				{
					// Restore the saved vibration strength.
					m_data.m_actuator_direct[i]=m_data.m_actuator_old_direct[i];
				}

				// If either tracked motor is on, rumble the single motor, otherwise turn it off.
				PADControlMotor( m_data.m_port, (( m_data.m_actuator_direct[0] > 0 ) || ( m_data.m_actuator_direct[1] > 0 )) ? PAD_MOTOR_RUMBLE : PAD_MOTOR_STOP );

//				scePadSetActDirect( m_data.m_port, m_data.m_slot, m_data.m_actuator_direct );
			}	
		}
	}
}



// Switches off vibration, and also zeros the saved
// vibration levels too so that unpausing will not 
// switch them on again for goodness sake.         
void Device::StopAllVibrationIncludingSaved()
{
	// First, make sure we're in a ready state and our controller has actuators
	if(( m_state == vACQUIRED ) && ( m_data.m_caps.TestMask( mACTUATORS )))
	{
		for (int i=0; i<m_data.m_num_actuators; ++i)
		{
			m_data.m_actuator_direct[i]=0;
			m_data.m_actuator_old_direct[i]=0;
		}	
		PADControlMotor( m_data.m_port, (( m_data.m_actuator_direct[0] > 0 ) || ( m_data.m_actuator_direct[1] > 0 )) ? PAD_MOTOR_RUMBLE : PAD_MOTOR_STOP );

//		scePadSetActDirect( m_data.m_port, m_data.m_slot, m_data.m_actuator_direct );
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Device::ActivatePressureSensitiveMode( void )
{
    if( m_data.m_caps.TestMask( mANALOG_BUTTONS ))
    {
//		if( scePadEnterPressMode( m_data.m_port, m_data.m_slot ) == 1 )
		{
			m_state = vBUSY;
			m_next_state = vACQUIRED;
		}
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Device::DeactivatePressureSensitiveMode( void )
{
    if( m_data.m_caps.TestMask( mANALOG_BUTTONS ))
    {
//		if( scePadExitPressMode( m_data.m_port, m_data.m_slot ) == 1 )
		{
			m_state = vBUSY;
			m_next_state = vACQUIRED;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
} // namespace SIO

