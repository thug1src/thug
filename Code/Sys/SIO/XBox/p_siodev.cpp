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

#include <xtl.h>
#include <stdlib.h>
#include <string.h>
         
#include <core/defines.h>
#include <sys/sioman.h>
#include <sys/mem/memman.h>
#include <sk/parkeditor2/parked.h>
#include <gel/scripting/script.h>
#include <gfx/2d/screenelemman.h>

//#include <libpad.h>

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

// K: Index of the last pad to have a button pressed. Used when a level is chosen in the skateshop.
// The pad used to control the player is whatever pad was used to choose the level.
int gLastPadPressed=0;

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

static void set_xbox_actuators( HANDLE handle, unsigned short left_motor, unsigned short right_motor )
{
	// Want this to be static since otherwise it would potentially go out of scope and be written to.
	static XINPUT_FEEDBACK input_feedback[32];
	static int next_index			= 0;
	
	// The Ps2 left motor is the high frequency motor, the right motor is a simple on/off low frequency motor.
	// On the Xbox it is the reverse (although the left motor has more control than simple on/off).
	
	input_feedback[next_index].Header.dwStatus			= 0;
	input_feedback[next_index].Header.hEvent			= NULL;
	input_feedback[next_index].Rumble.wLeftMotorSpeed	= right_motor;
	input_feedback[next_index].Rumble.wRightMotorSpeed	= left_motor;

	DWORD status = XInputSetState( handle, &input_feedback[next_index] );
	Dbg_Assert(( status == ERROR_IO_PENDING ) || ( status == ERROR_SUCCESS ) || ( status == ERROR_DEVICE_NOT_CONNECTED ));

	// Cycle array member.
	if( ++next_index >= 32 )
	{
		next_index = 0;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
    
void Device::process( void )
{
	m_plugged_in = false;

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

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Device::read_data ( void )
{
	XINPUT_STATE	xis;
	HRESULT			hr;

	m_plugged_in = false;

	if( m_data.m_handle == NULL )
	{
		return;
	}

	hr = XInputGetState( m_data.m_handle, &xis );
	if( hr != ERROR_SUCCESS )
	{
		XInputClose( m_data.m_handle );
		m_data.m_handle	= NULL;
		m_data.m_valid	= false;
		Unacquire();
		Acquire();
		return;
	}

	m_data.m_valid	= true;
	m_plugged_in	= true;

	// Convert this data back into PS2-style 'raw' data for a DUALSHOCK2.
	m_data.m_control_data[0] = 0;						// 'Valid' info flag.
	m_data.m_control_data[1] = ( 0x07 << 4 ) | 16;		// DUALSHOCK2 id + data length.

	m_data.m_control_data[2] = 0xFF;					// Turn off all buttons by default.
	m_data.m_control_data[3] = 0xFF;					// Turn off all buttons by default.

	m_data.m_control_data[2] ^= ( xis.Gamepad.wButtons & XINPUT_GAMEPAD_START ) ? ( 1 << 3 ) : 0;		// XBox 'Start' = PS2 'Start'.
	m_data.m_control_data[2] ^= ( xis.Gamepad.wButtons & XINPUT_GAMEPAD_BACK ) ? ( 1 << 0 ) : 0;		// XBox 'Back' = PS2 'Select'.
	m_data.m_control_data[2] ^= ( xis.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB ) ? ( 1 << 1 ) : 0;	// Xbox 'Left Stick Button' = PS2 'Left Stick Button'.
	m_data.m_control_data[2] ^= ( xis.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB ) ? ( 1 << 2 ): 0;	// Xbox 'Right Stick Button' = PS2 'Right Stick Button'.

	// TRC 1.4-1-26 to eliminate crosstalk must disregard values below 0x20.
	m_data.m_control_data[3] ^= ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_Y] >= 0x20 ) ? ( 1 << 4 ) : 0;	// XBox 'Y' = PS2 'Triangle'.
	m_data.m_control_data[3] ^= ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_B] >= 0x20 ) ? ( 1 << 5 ) : 0;	// XBox 'B' = PS2 'Circle'.
	m_data.m_control_data[3] ^= ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_A] >= 0x20 ) ? ( 1 << 6 ) : 0;	// XBox 'A' = PS2 'X'.
	m_data.m_control_data[3] ^= ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_X] >= 0x20 ) ? ( 1 << 7 ) : 0;	// XBox 'X' = PS2 'Square'.
		
	if( Ed::CParkEditor::Instance()->EditingCustomPark())
	{
		// In the Park Editor, black and white buttons are L1 and L2, triggers are R1 and R2... unless the gap name keyboard is active :(
		bool							keyboard_active		= false;
		Front::CScreenElementManager	*p_screen_elem_man	= Front::CScreenElementManager::Instance();
		if( p_screen_elem_man )
		{
			Front::CScreenElement		*p_keyboard			= p_screen_elem_man->GetElement( Script::GenerateCRC( "keyboard_anchor" ) , Front::CScreenElementManager::DONT_ASSERT );
			if( p_keyboard )
			{
				keyboard_active	= true;
			}
		}
		
		if( keyboard_active	)
		{
			m_data.m_control_data[3] ^= ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] > 0x20 ) ? ( 1 << 1 ) : 0;		// XBox  'Right Trigger'= PS2 'R2'.
		}
		else
		{
			m_data.m_control_data[3] ^= ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] > 0x20 ) ? ( 1 << 2 ) : 0;		// XBox  'Right Trigger'= PS2 'L1'.
		}
		
		m_data.m_control_data[3] ^= ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] > 0x20 ) ? ( 1 << 0 ) : 0;			// XBox 'Left Trigger' = PS2 'L2'.
		// New! Black and white are no longer mapped to R2 and L2 but are treated as seperate buttons
		//m_data.m_control_data[3] ^= ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_WHITE] > 0x20 ) ? ( 1 << 1 ) : 0;					// XBox 'White' = PS2 'R2'.
		//m_data.m_control_data[3] ^= ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_BLACK] > 0x20 ) ? ( 1 << 3 ) : 0;					// XBox 'Black' = PS2 'R1'.
	}
	else
	{
		// New! Black and white are no longer mapped to R2 and L2 but are treated as seperate buttons

		// Outside of the Park Editor, either black or white buttons act as L1 and R1...
		//m_data.m_control_data[3] ^= ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_WHITE] > 0x20 ) ? (( 1 << 2 ) | ( 1 << 3 )) : 0;	// XBox 'White' = PS2 'L1' + PS2 'R1'.
//		m_data.m_control_data[3] ^= ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_BLACK] > 0x20 ) ? (( 1 << 2 ) | ( 1 << 3 )) : 0;	// XBox 'Black'= PS2 'L1' + PS2 'R1'.
		//m_data.m_control_data[3] ^= ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_BLACK] > 0x20 ) ? (( 1 << 0 ) | ( 1 << 1 )) : 0;	// XBox 'Black'= PS2 'L2' + PS2 'R2'.
		
		// ...triggers function as both L1/L2 and R1/R2.		
		m_data.m_control_data[3] ^= ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] > 128 ) ? ( 1 << 0 ) : 0;				// XBox 'Left Trigger pressed down > halfway' = PS2 'L2'.
		m_data.m_control_data[3] ^= ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] > 128 ) ? ( 1 << 1 ) : 0;			// XBox 'Right Trigger pressed down > halfway' = PS2 'R2'.

		m_data.m_control_data[3] ^= ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] > 0x20 ) ? ( 1 << 2 ) : 0;				// XBox 'Left Trigger' = PS2 'L1'.
		m_data.m_control_data[3] ^= ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] > 0x20 ) ? ( 1 << 3 ) : 0;				// XBox 'Right Trigger' = PS2 'R1'.
	}

	// Make analog buttons full depression if pressed.
	// Xbox analog buttons return analog value in range [0, 255].
	m_data.m_control_data[12] = ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_Y] > 0x20 ) ? xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_Y] : 0;							// XBox 'Y' = PS2 Analog 'Triangle'.
	m_data.m_control_data[13] = ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_B] > 0x20 ) ? xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_B] : 0;							// XBox 'B' = PS2 Analog 'Circle'.
	m_data.m_control_data[14] = ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_A] > 0x20 ) ? xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_A] : 0;							// XBox 'A' = PS2 Analog 'X'.
	m_data.m_control_data[15] = ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_X] > 0x20 ) ? xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_X] : 0;							// XBox 'X' = PS2 'Square'.
	m_data.m_control_data[16] = ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] > 0x20 ) ? xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] : 0;	// XBox 'Left Trigger' = PS2 Analog 'L1'.
	m_data.m_control_data[17] = ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] > 0x20 ) ? xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] : 0;	// XBox 'Right Trigger' = PS2 Analog 'R1'.
	// Black and white are no longer mapped to R2 and L2, but are treated as separate buttons.
	//m_data.m_control_data[18] = ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_WHITE] > 0x20 ) ? xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_WHITE] : 0;					// XBox 'White' = PS2 Analog 'L2'.
	//m_data.m_control_data[19] = ( xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_BLACK] > 0x20 ) ? xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_BLACK] : 0;					// XBox 'Black' = PS2 Analog 'R2'.
	m_data.m_control_data[18]=0;
	m_data.m_control_data[19]=0;

	// Zero out the d-pad pressure values.
	m_data.m_control_data[8]	= 0x00;
	m_data.m_control_data[9]	= 0x00;
	m_data.m_control_data[10]	= 0x00;
	m_data.m_control_data[11]	= 0x00;

	// Handle 8 position d-pad.
	m_data.m_control_data[2] ^= ( xis.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP ) ? ( 1 << 4 ) : 0;		// XBox 'DPad Up' = PS2 'DPad Up'.
	m_data.m_control_data[10] = ( xis.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP ) ? 0xFF : 0;			// XBox 'DPad Up' = PS2 'DPad Up'.
	m_data.m_control_data[2] ^= ( xis.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT ) ? ( 1 << 5 ) : 0;	// XBox 'DPad Right' = PS2 'DPad Right'.
	m_data.m_control_data[8] = ( xis.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT ) ? 0xFF : 0;			// XBox 'DPad Right' = PS2 'DPad Right'.
	m_data.m_control_data[2] ^= ( xis.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN ) ? ( 1 << 6 ) : 0;	// XBox 'DPad Down' = PS2 'DPad Down'.
	m_data.m_control_data[11] = ( xis.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN ) ? 0xFF : 0;			// XBox 'DPad Down' = PS2 'DPad Down'.
	m_data.m_control_data[2] ^= ( xis.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT ) ? ( 1 << 7 ) : 0;	// XBox 'DPad Left' = PS2 'DPad Left'.
	m_data.m_control_data[9] = ( xis.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT ) ? 0xFF : 0;			// XBox 'DPad Left' = PS2 'DPad Left'.

	// Xbox thumbsticks return analog value in range [-32767, 32767].
	m_data.m_control_data[4] = XboxThumbToPS2Thumb( xis.Gamepad.sThumbRX );	// Analog stick right (X direction).
	m_data.m_control_data[5] = XboxThumbToPS2Thumb( -xis.Gamepad.sThumbRY );	// Analog stick right (Y direction).
	m_data.m_control_data[6] = XboxThumbToPS2Thumb( xis.Gamepad.sThumbLX );	// Analog stick left (X direction).
	m_data.m_control_data[7] = XboxThumbToPS2Thumb( -xis.Gamepad.sThumbLY );	// Analog stick left (Y direction).

	// K: Use m_control_data[20] to store the state of the black & white buttons.
	m_data.m_control_data[20]=0;
	if (xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_BLACK] > 0x20)
	{
		m_data.m_control_data[20] |= (1<<0);
	}	
	if (xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_WHITE] > 0x20)
	{
		m_data.m_control_data[20] |= (1<<1);
	}	

	
	uint32 buttons = 0xFFFF ^ (( m_data.m_control_data[2] << 8 ) | m_data.m_control_data[3] );

	// Skate3 specific code, removed for now.
	if( buttons )
	{
		m_pressed = true;	
	}
	else
	{
		m_pressed = false;
	}	
		

	if(( xis.Gamepad.wButtons & XINPUT_GAMEPAD_START) || xis.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_A] >= 0x20 )
	{
		m_start_or_a_pressed = true;
	}
	else
	{
		m_start_or_a_pressed = false;
	}	
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

	if( m_data.m_handle == NULL )
	{
		++m_unplugged_counter;

		// Retry to connect every second or so.
		if(( m_unplugged_counter & 0x3F ) == m_unplugged_retry )
		{
			m_data.m_handle = XInputOpen( XDEVICE_TYPE_GAMEPAD, m_data.m_port, XDEVICE_NO_SLOT, NULL );
		}
		
		if( m_data.m_handle == NULL )
		{
			return;
		}
	}

    status = get_status();
                        
    if(( status == vCONNECTED ) || ( status == vREADY ))
    {
        // Sucessful.  Now query the controller for capabilities.
		m_unplugged_counter = 0;
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
	// Currently assumes standard XBox controller.
    int id = vANALOG_CTRL;
    
    m_data.m_type = id;

	switch( id )
	{   
		case vANALOG_CTRL:
		{
			m_data.m_caps.SetMask( mANALOG_BUTTONS );

			m_data.m_num_actuators		= 2;
			m_data.m_actuator_max[0]	= 255;
			m_data.m_actuator_max[1]	= 255;
			m_data.m_caps.SetMask( mACTUATORS );

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
	if( m_data.m_handle )
	{
		XINPUT_STATE state;
		if( XInputGetState( m_data.m_handle, &state ) == ERROR_SUCCESS )
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
	
	// Random retry for unplugged controllers.
	m_unplugged_retry = rand() & 0x3F;

//	memset( m_data.m_dma_buff, 0, sizeof( uint128 ) * scePadDmaBufferMax );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
Device::~Device ( void )
{
	Dbg_AssertType ( m_node, Lst::Node< Device > );
//	Mem::Free( m_data.m_prealbuffer );

	XInputClose( m_data.m_handle );

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
		// Acquire device handle.
		m_data.m_handle = XInputOpen( XDEVICE_TYPE_GAMEPAD, m_data.m_port, XDEVICE_NO_SLOT, NULL );

//		if( m_data.m_handle )
		{
			// Store capabilites of the device
//			XINPUT_CAPABILITIES caps;
//			XInputGetCapabilities( m_data.m_handle, &caps );

//			memset( m_data.m_control_data, 0, CTRL_BUFFER_LENGTH );
			m_data.m_valid = false;
			m_state = vACQUIRING;
			return;
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
		m_data.m_handle = NULL;
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
    
    // First, make sure we're in a ready state and our controller has actuators
	read_data();
    if(( m_state == vACQUIRED ) && ( m_data.m_caps.TestMask( mACTUATORS )))
    {
        if(( act_num >= 0 ) && ( act_num < m_data.m_num_actuators ))
        {
			unsigned short left_motor, right_motor;           
			float act_strength						= ((float)percent * m_data.m_actuator_max[act_num] ) * 0.01f;
			m_data.m_actuator_direct[act_num]		= (unsigned char)act_strength;

			// Scale the values from [0,255] to [0,65535].
			if( act_num == 0 )
			{
				left_motor	= (unsigned short)( act_strength * 256.0f );
				right_motor	= (unsigned short)m_data.m_actuator_direct[1] * 256;
			}
			else
			{
				left_motor	= (unsigned short)m_data.m_actuator_direct[0] * 256;
				right_motor	= (unsigned short)( act_strength * 256.0f );
			}

			set_xbox_actuators( m_data.m_handle, left_motor, right_motor );
        }
    }
}

/******************************************************************/
/* These disable or enable pad vibration.
/******************************************************************/
void Device::DisableActuators()
{
	// If disabled already do nothing.
	if( m_data.m_actuators_disabled )
	{
		return;
	}
		
	// Run through all the actuators and make sure they're off.
	if(( m_state == vACQUIRED ) && ( m_data.m_caps.TestMask( mACTUATORS )))
    {
		for( int i = 0; i < m_data.m_num_actuators; ++i )
		{
			// Switch it off.
			m_data.m_actuator_direct[i] = 0;
		}	
		set_xbox_actuators( m_data.m_handle, 0, 0 );
	}	
	
	// Set the flag.
	m_data.m_actuators_disabled = true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void Device::EnableActuators( void )
{
	m_data.m_actuators_disabled = false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void Device::ResetActuators( void )
{
	if( m_data.m_actuators_disabled )
	{
		// If disabled, then should be off anyway but toggle the states to make sure.
		EnableActuators();
		DisableActuators();
	}
	else
	{
		// If enabled, then we disable them, which switched them off and then enable them again (in the off position).
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

void Device::Pause( void )
{
	// If paused already do nothing.
	if( !m_data.m_paused_ref.InUse())
	{		
	    // First, make sure we're in a ready state and our controller has actuators
	    if(( m_state == vACQUIRED ) && ( m_data.m_caps.TestMask( mACTUATORS )))
		{
			for( int i = 0; i<m_data.m_num_actuators; ++i )
			{
				// Save the old actuator vibration strength.
				m_data.m_actuator_old_direct[i] = m_data.m_actuator_direct[i];

				// Then switch it off.
				m_data.m_actuator_direct[i] = 0;
			}	
			set_xbox_actuators( m_data.m_handle, 0, 0 );
		}	
	}
	m_data.m_paused_ref.Acquire();
}



/******************************************************************/
/*                                                                */
/* Restores the vibration status.                                 */
/*                                                                */
/******************************************************************/

void Device::UnPause( void )
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
					m_data.m_actuator_direct[i] = m_data.m_actuator_old_direct[i];
				}
				set_xbox_actuators( m_data.m_handle,
									(unsigned short)m_data.m_actuator_direct[0] * 256,
									(unsigned short)m_data.m_actuator_direct[1] * 256 );
			}	
		}
	}
}



/******************************************************************/
/* Switches off vibration, and also zeros the saved				  */
/* vibration levels too so that unpausing will not				  */
/* switch them on again for goodness sake.						  */
/******************************************************************/
void Device::StopAllVibrationIncludingSaved()
{
	// First, make sure we're in a ready state and our controller has actuators
	if(( m_state == vACQUIRED ) && ( m_data.m_caps.TestMask( mACTUATORS )))
	{
		for( int i = 0; i < m_data.m_num_actuators; ++i )
		{
			m_data.m_actuator_direct[i]		= 0;
			m_data.m_actuator_old_direct[i]	= 0;
		}	
		set_xbox_actuators( m_data.m_handle, 0, 0 );
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
