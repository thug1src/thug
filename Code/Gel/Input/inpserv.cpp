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
**	File name:		inpserv.cpp												**
**																			**
**	Created:		05/31/2000	-	spg										**
**																			**
**	Description:	Input server code										**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/math.h>



#ifdef __NOPT_DEBUG__
#include <sys/timer.h>
#endif

#include <sys/demo.h>
#include <sys/sioman.h>
#include <sys/siodev.h>
 
#include <gel/inpman.h>

#include <string.h>

#ifdef	__PLAT_NGPS__
#include <gfx/ngps/p_memview.h>	  		// Mick:  needed for low level input patch bypassing task system
#include <libscedemo.h>					// Mick: needed for low level inactivity timeout on PS2 demo disk
#elif defined( __PLAT_NGC__ )
#include <gfx/ngc/p_memview.h>
#endif

#include <sys/config/config.h>

#ifdef	__PLAT_NGPS__
extern	sceDemoEndReason demo_exit_reason;
extern  int		inactive_time;
extern  int		inactive_countdown;
extern  int		gameplay_time;
#endif

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

// Used to map digital input to analog events in case of no analog button support
static  int adc_table[][2] = 
{
    { Data::mD_L3,        Data::vA_L3 },
    { Data::mD_R3,        Data::vA_R3 },
    { Data::mD_L2,        Data::vA_L2 },
    { Data::mD_R2,        Data::vA_R2 },
    { Data::mD_L1,        Data::vA_L1 },
    { Data::mD_R1,        Data::vA_R1 },
    { Data::mD_TRIANGLE,  Data::vA_TRIANGLE },
    { Data::mD_CIRCLE,    Data::vA_CIRCLE },
    { Data::mD_X,         Data::vA_X },
    { Data::mD_SQUARE,    Data::vA_SQUARE },
    { Data::mD_UP,        Data::vA_UP },
    { Data::mD_RIGHT,     Data::vA_RIGHT },
    { Data::mD_DOWN,      Data::vA_DOWN },
    { Data::mD_LEFT,      Data::vA_LEFT },
    { Data::mD_BLACK,     Data::vA_BLACK },
    { Data::mD_WHITE,     Data::vA_WHITE },
    { Data::mD_Z,         Data::vA_Z },
//	{ Data::mD_SELECT,	  Data::vA_SELECT },
    { 0,                  0 }
};

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

// Mick: Controller info is made public for quick and easy debug test
// it sould not be used for any shippable code....
uint32	gDebugButtons[SIO::vMAX_DEVICES];
uint32	gDebugBreaks[SIO::vMAX_DEVICES];
uint32	gDebugMakes[SIO::vMAX_DEVICES];

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

void Server::service_handlers( void )
{
//    
    
    unsigned char *control_data;
	Data *handler_data;
    Flags<SIO::Capabilities> caps;

	// If we are getting our input from recorded data, set all depended controller data from recorded data   
	if( m_data_in && m_data_in->m_valid )
	{
		int i;
		
		m_data.m_prev = m_data.m_cur;
		m_data.m_cur = m_data_in->m_cur;		// Get digital input from buffer
		m_data.m_new = m_data.m_prev ^ m_data.m_cur;
		m_data.m_Buttons = m_data.m_cur;
		m_data.m_Makes = m_data.m_new & m_data.m_cur;
		m_data.m_Breaks = m_data.m_new & ~m_data.m_cur;
		
		for( i = 0; i < Data::vMAX_ANALOG_EVENTS; i++ )	// Get analog input from buffer
		{      
			m_data.m_Event[i] = m_data_in->m_event[i];
		}
		
		m_handler_stack.Process();
		
		m_data_in++;
		return;
	}
			
	if( ( m_device ) &&
		( m_device->HasValidControlData()))
    {   
		Inp::Manager * inp_manager = Inp::Manager::Instance();

        control_data = m_device->GetControlData();
        handler_data = &m_data;
        caps = m_device->GetCapabilities();
        Dbg_Assert( control_data );
		
		if( m_data_out )
		{
			m_data_out->m_valid = FALSE;
		}

        if( control_data[0] == 0 )  // Valid controller communication
        {
            unsigned char controller_type;
            
            controller_type = control_data[1] >> 4;  // top 4 bits contain controller type
            handler_data->m_prev = handler_data->m_cur;
			// Regular buttons
            handler_data->m_cur = 0xFFFF ^ (( control_data[2] << 8 ) | control_data[3] );
			// The XBox black & white and NGC Z buttons.
			handler_data->m_cur |= (control_data[20]<<16);
			
			
			handler_data->m_new = handler_data->m_prev ^ handler_data->m_cur;
			
			handler_data->m_Buttons = handler_data->m_cur;         
            handler_data->m_Makes = handler_data->m_new & handler_data->m_cur;
            handler_data->m_Breaks = handler_data->m_new & ~handler_data->m_cur;
            
            switch( controller_type )
            {
            case SIO::Device::vNEGI_COM:
                // TODO: Handle 
                Dbg_MsgAssert( 0,( "Unsupported Device Type\n" ));
                break;

            case SIO::Device::vKONAMI_GUN:
                // TODO: Handle 
                Dbg_MsgAssert( 0,( "Unsupported Device Type\n" ));
                break;

            case SIO::Device::vDIGITAL_CTRL:
                handler_data->ConvertDigitalToAnalog();
                break;                                                   

            case SIO::Device::vNAMCO_GUN:
                // TODO: Handle 
                Dbg_MsgAssert( 0,( "Unsupported Device Type\n" ));
                break;

            case SIO::Device::vJOYSTICK:
                handler_data->m_Event[Data::vA_RIGHT_X] = control_data[4];
                handler_data->m_Event[Data::vA_RIGHT_Y] = control_data[5];
                handler_data->m_Event[Data::vA_LEFT_X] = control_data[6];
                handler_data->m_Event[Data::vA_LEFT_Y] = control_data[7];
                handler_data->ConvertDigitalToAnalog();
				if( inp_manager->ShouldAnalogStickOverride())
				{
					handler_data->OverrideAnalogPadWithStick();
				}
                break;

            case SIO::Device::vANALOG_CTRL:
                //if( caps.TestMask( SIO::mANALOG_BUTTONS ))
				if( m_device->GetButtonMode() == SIO::vANALOG )
                {
                    memcpy( handler_data->m_Event, &control_data[4], Data::vMAX_ANALOG_EVENTS );
                }
                else
                {
                    handler_data->m_Event[Data::vA_RIGHT_X] = control_data[4];
                    handler_data->m_Event[Data::vA_RIGHT_Y] = control_data[5];
                    handler_data->m_Event[Data::vA_LEFT_X] = control_data[6];
                    handler_data->m_Event[Data::vA_LEFT_Y] = control_data[7];
                    handler_data->ConvertDigitalToAnalog();
                    handler_data->handle_analog_tolerance();
					if( inp_manager->ShouldAnalogStickOverride())
					{
						handler_data->OverrideAnalogPadWithStick();
					}
                }
                break;

            case SIO::Device::vFISHING_CTRL:
                // TODO: Handle 
                Dbg_MsgAssert( 0,( "Unsupported Device Type\n" ));
                break;
                
            case SIO::Device::vJOG_CTRL:        
                // TODO: Handle 
                Dbg_MsgAssert( 0,( "Unsupported Device Type\n" ));
                break;

            }
            
			if( m_data_out )
			{
				int i;
				
				m_data_out->m_cur = handler_data->m_cur;
				m_data_out->m_valid = TRUE;
				for( i = 0; i < Data::vMAX_ANALOG_EVENTS; i++ )	// Get analog input from buffer
				{      
					m_data_out->m_event[i] = handler_data->m_Event[i];
				}
				
				m_data_out++;
			}

			// Make simple controller buttons accessible at global level
			// for debugging tools			
			
			if (m_device)
			{
				gDebugButtons[m_device->GetIndex()] = m_data.m_Buttons; 							
				gDebugMakes[m_device->GetIndex()] = m_data.m_Makes; 							
				gDebugBreaks[m_device->GetIndex()] = m_data.m_Breaks; 							
								
				#ifdef	__PLAT_NGPS__
				MemView_Input(m_data.m_Buttons, m_data.m_Makes, m_data.m_Breaks );
							
				// Mick: added test for "Select" which will now exit the game if in demo mode
				
				if (Config::Bootstrap())
				{

					bool exit = false;
		
					if (m_data.m_Buttons ||			
						(handler_data->m_Event[Data::vA_RIGHT_X] !=128)||
						(handler_data->m_Event[Data::vA_RIGHT_Y] !=128)	||
						(handler_data->m_Event[Data::vA_LEFT_X] !=128) 	||
						(handler_data->m_Event[Data::vA_LEFT_Y] !=128))
					{
						//printf ("%d activity....\n", inactive_countdown);
						inactive_countdown = inactive_time;				
					}
		
					if (m_device->GetIndex() == 0)
					{
						
						if (inactive_countdown)
						{
							inactive_countdown --;
							if (inactive_countdown == 0)
							{
								printf ("Exiting due to inactivity\n");
								exit = true;  
								demo_exit_reason = SCE_DEMO_ENDREASON_PLAYABLE_INACTIVITY_TIMEOUT;
							}
						}
						if (gameplay_time)
						{
							gameplay_time--;
							if (gameplay_time == 0)
							{
								printf ("Exiting due to gameplay timer exiting\n");
								exit = true;  
								demo_exit_reason = SCE_DEMO_ENDREASON_PLAYABLE_GAMEPLAY_TIMEOUT;
							}
						}
					}
					
					if ((m_data.m_Buttons & Inp::Data::mD_SELECT) && (m_data.m_Buttons & Inp::Data::mD_START))
					{
						exit = true;  
						demo_exit_reason = SCE_DEMO_ENDREASON_PLAYABLE_QUIT;
					}
					
					if ( exit )
					{
                        Sys::ExitDemo();
					}
				}
				#endif				  
			}
			
            // At this point, we have obtained and converted valid controller information
            // Now propogate it to clients
            m_handler_stack.Process();
        }
    }            
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Data::handle_analog_tolerance( void )
{

	// copy unclamped values over before we clamp them										  
	m_Event[Data::vA_RIGHT_X_UNCLAMPED] = m_Event[Data::vA_RIGHT_X];	
	m_Event[Data::vA_RIGHT_Y_UNCLAMPED] = m_Event[Data::vA_RIGHT_Y];	
	m_Event[Data::vA_LEFT_X_UNCLAMPED] = m_Event[Data::vA_LEFT_X];	
	m_Event[Data::vA_LEFT_Y_UNCLAMPED] = m_Event[Data::vA_LEFT_Y];	
					 
					 
	if( Mth::Abs( vANALOGUE_CENTER - m_Event[Data::vA_RIGHT_X] ) <= vANALOGUE_TOL )
    {
        m_Event[Data::vA_RIGHT_X] = vANALOGUE_CENTER;
    }

    if( Mth::Abs( vANALOGUE_CENTER - m_Event[Data::vA_RIGHT_Y] ) <= vANALOGUE_TOL )
    {
        m_Event[Data::vA_RIGHT_Y] = vANALOGUE_CENTER;
    }

    if( Mth::Abs( vANALOGUE_CENTER - m_Event[Data::vA_LEFT_X] ) <= vANALOGUE_TOL )
    {
        m_Event[Data::vA_LEFT_X] = vANALOGUE_CENTER;
    }

    if( Mth::Abs( vANALOGUE_CENTER - m_Event[Data::vA_LEFT_Y] ) <= vANALOGUE_TOL )
    {
        m_Event[Data::vA_LEFT_Y] = vANALOGUE_CENTER;
    }

// "unclamped", only zero the controllers if BOTH X and Y are in the dead zone
// that way, we can smoothly go around the periphery, and get much better aiming ability
// (improves the look-around camera, and the walking)

	if( Mth::Abs( vANALOGUE_CENTER - m_Event[Data::vA_RIGHT_X_UNCLAMPED] ) <= vANALOGUE_TOL 
	&& Mth::Abs( vANALOGUE_CENTER - m_Event[Data::vA_RIGHT_Y_UNCLAMPED] ) <= vANALOGUE_TOL )
    {
        m_Event[Data::vA_RIGHT_X_UNCLAMPED] = vANALOGUE_CENTER;
        m_Event[Data::vA_RIGHT_Y_UNCLAMPED] = vANALOGUE_CENTER;
    }

    if( Mth::Abs( vANALOGUE_CENTER - m_Event[Data::vA_LEFT_X_UNCLAMPED] ) <= vANALOGUE_TOL 
	&& Mth::Abs( vANALOGUE_CENTER - m_Event[Data::vA_LEFT_Y_UNCLAMPED] ) <= vANALOGUE_TOL )
    {
        m_Event[Data::vA_LEFT_X_UNCLAMPED] = vANALOGUE_CENTER;
        m_Event[Data::vA_LEFT_Y_UNCLAMPED] = vANALOGUE_CENTER;
    }
	
	
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

Server::Server( void )
{
//    
    
	
	m_data_in = NULL;
	m_data_out = NULL;
	m_data_in_end = NULL;
	m_data_out_end = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Server::RecordInput( RecordedData *data_buffer )
{
//    
    
	
	Dbg_Assert( data_buffer );
	
	m_data_out = data_buffer;   
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Server::PlaybackInput( RecordedData *recorded_input )
{
//    
    
	
	Dbg_Assert( recorded_input );
	
	m_data_in = recorded_input;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
    
void Data::MaskDigitalInput( int button_mask )
{
//    
    

	m_Buttons &= ~button_mask;   
	
#if 0 	// old way of masking
	m_new = m_prev ^ m_Buttons;
	m_Makes = m_new & m_Buttons;
	m_Breaks = m_new & ~m_Buttons;
#endif
	
	m_Makes &= ~button_mask;
	m_Breaks &= ~button_mask;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Data::MaskAnalogInput( int button_mask )
{
//    
    

	int i, mask;   
		
    mask = 1;
	for( i = 0; i < vMAX_ANALOG_EVENTS; i++, mask <<= 1 )
	{
		if( button_mask & mask )
		{
            // Analog stick is neutral at 128, all other buttons are neutral at 0
            if( 
				( i == vA_RIGHT_X ) ||
                ( i == vA_RIGHT_Y ) ||
                ( i == vA_LEFT_X  ) ||
                ( i == vA_LEFT_Y  )	||
				( i == vA_RIGHT_X_UNCLAMPED ) ||
                ( i == vA_RIGHT_Y_UNCLAMPED ) ||
                ( i == vA_LEFT_X_UNCLAMPED  ) ||
                ( i == vA_LEFT_Y_UNCLAMPED  ))
            {   
                m_Event[i] = 128;
            }
            else
            {
                m_Event[i] = 0;
            }
		}
	}
}

/******************************************************************/
/* Overrides Analog Pad with Analog Stick values if pad is not in */
/* in use                                                         */
/******************************************************************/

void Data::OverrideAnalogPadWithStick( void )
{
	

	// Only override analog pad if no analog input is present
	if( m_Buttons & ( mD_UP | mD_RIGHT | mD_DOWN | mD_LEFT ))
	{
		return;
	}

	if( m_Event[vA_LEFT_X] > vANALOGUE_CENTER )
	{
		m_Event[vA_RIGHT] = m_Event[vA_LEFT_X] - vANALOGUE_CENTER;
		m_Buttons |= mD_RIGHT;
	}
	else if( m_Event[vA_LEFT_X] < vANALOGUE_CENTER )
	{
		m_Event[vA_LEFT] = Mth::Abs( vANALOGUE_CENTER - m_Event[vA_LEFT_X] );
		m_Buttons |= mD_LEFT;
	}
    
	if( m_Event[vA_LEFT_Y] < vANALOGUE_CENTER )
	{
		m_Event[vA_UP] = m_Event[Data::vA_LEFT_Y] - vANALOGUE_CENTER;
		m_Buttons |= mD_UP;
	}
	else if( m_Event[vA_LEFT_Y] > vANALOGUE_CENTER )
	{
		m_Event[vA_DOWN] = Mth::Abs( vANALOGUE_CENTER - m_Event[vA_LEFT_Y] );
		m_Buttons |= mD_DOWN;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Data::ConvertDigitalToAnalog( void )
{
	int i;

    for( i = 0; adc_table[i][0] > 0; i++ )
	{
		if( m_Buttons & adc_table[i][0] )  // If we have digital "down" convert to fully-pressed analog input
		{
			m_Event[adc_table[i][1]] = vMAX_ANALOG_VALUE;
		}
        else
        {
            m_Event[adc_table[i][1]] = 0;
        }
	}  
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Data::Data( void )
{
    memset( m_Event, 0, vMAX_ANALOG_EVENTS );
    m_Buttons = 0;
	m_cur = 0;
    m_new = 0;
    m_prev = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Inp
