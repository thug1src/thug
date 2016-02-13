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
#include <libpad.h>

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								DBG Defines									**
*****************************************************************************/

namespace SIO
{
  


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

	case vPRESS_MODE_ON_COMPLETE:
		m_data.m_button_mode = vANALOG;
		m_state = vACQUIRED;
		break;

	case vPRESS_MODE_OFF_COMPLETE:
		m_data.m_button_mode = vDIGITAL;
		m_state = vACQUIRED;
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

    if( scePadGetReqState( m_data.m_port, m_data.m_slot ) != scePadReqStateBusy )
    {
        if( get_status() != vNOTREADY )
        {
            m_state = m_next_state;
            m_next_state = vIDLE;
        }
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Device::read_data ( void )
{
    

    int state;

    state = scePadGetState( m_data.m_port, m_data.m_slot );

    switch ( state )
    {
    case scePadStateFindCTP1:
    case scePadStateStable:
        if( scePadRead( m_data.m_port, m_data.m_slot, m_data.m_control_data ) > 0 )
        {
            m_data.m_valid = true;
			m_plugged_in=true;
        }
        else
        {
            m_data.m_valid = false;
        }
        break;

    case scePadStateDiscon:
        // We lost connection to the controller. Try to reacquire it
        // Also, display message if it is required.
		m_plugged_in=false;
        Unacquire();
        Acquire();
        break;
    }                  
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Device::IsPluggedIn()
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
    

    int id, ex_id;

    // Get controller ID
    id = scePadInfoMode( m_data.m_port, m_data.m_slot, InfoModeCurID, 0 );
    
    if( id == 0 )
    {
        Dbg_Warning( "Unsupported controller or controller disconnected" );
        Unacquire();
        return;
    }

    // Get extended ID, if one exists
    ex_id = scePadInfoMode( m_data.m_port, m_data.m_slot, InfoModeCurExID, 0 );
    // if we have a valid extended ID, use it instead
    if( ex_id > 0 )
    { 
        id = ex_id;
    }
    
    m_data.m_type = id;
	m_data.m_button_mode = vDIGITAL;

    switch( id )
    {   
    case vNEGI_COM:
        Dbg_Message( "Detected NeGi-CON Controller in %d:%d", m_data.m_port, m_data.m_slot );
        break;

    case vKONAMI_GUN:
        Dbg_Message( "Detected GunCON(Konami) Controller in %d:%d", m_data.m_port, m_data.m_slot );
        break;

    case vDIGITAL_CTRL:
        Dbg_Message( "Detected Digital Controller in %d:%d", m_data.m_port, m_data.m_slot );
        // Switch to Analog if it's supported
        if( scePadInfoMode( m_data.m_port, m_data.m_slot, InfoModeCurExID, 0 ) != 0 )
        {   
            if( scePadSetMainMode( m_data.m_port, m_data.m_slot, 1, 3 ) == 0 )
            {
                Dbg_Warning( "scePadSetMainMode not sent properly" );
            }
			// Return so that we stay in this state and re-query as an analog controller
			return;
        }
        
        break;

    case vJOYSTICK:
        Dbg_Message( "Detected Analog Joystick in %d:%d", m_data.m_port, m_data.m_slot );
        break;

    case vNAMCO_GUN:
        Dbg_Message( "Detected GunCON(NAMCO) Controller in %d:%d", m_data.m_port, m_data.m_slot );
        break;


    case vANALOG_CTRL:
        {
            int i, j;

            Dbg_Message( "Detected Analog Controller in %d:%d", m_data.m_port, m_data.m_slot );
            m_data.m_caps.SetMask( mANALOG );
            // check if it's a dual shock 2
			if( scePadInfoPressMode( m_data.m_port, m_data.m_slot ) == 1 )
			{
				m_data.m_caps.SetMask( mANALOG_BUTTONS );
			}

            // check for actuator support
            if(( m_data.m_num_actuators = scePadInfoAct( m_data.m_port, m_data.m_slot, -1, 0 )) > 0 )
            {
                m_data.m_caps.SetMask( mACTUATORS );

                for( i = 0; i < m_data.m_num_actuators; i++ )
                {
                    int power;

                    power = scePadInfoAct( m_data.m_port, m_data.m_slot, i, InfoActSize );
                    if( power == 0 )
                    {
                        m_data.m_actuator_max[i] = 1;
                    }
                    else
                    {
                        m_data.m_actuator_max[i] = 255;
                    }
	            }

                for( i = 0; i < m_data.m_num_actuators; i++ )
                {
                    m_data.m_actuator_align[ i ] = i;
                }

                for( j = i; j < 6; j++ )
                {
                    m_data.m_actuator_align[ j ] = 0xFF;
                }
                
                if( scePadSetActAlign( m_data.m_port, m_data.m_slot, m_data.m_actuator_align ) == 0 )
                {
                    Dbg_Warning( "scePadSetActAlign not sent properly" );
                }

                m_state = vBUSY;
                m_next_state = vACQUIRED;
                return;
            }
            break;
        }
        
    case vFISHING_CTRL:
        Dbg_Message( "Detected TSURI-CON(fishing) Controller in %d:%d", m_data.m_port, m_data.m_slot );
        break;
        
    case vJOG_CTRL:
        Dbg_Message( "Detected JOG-CON Controller in %d:%d", m_data.m_port, m_data.m_slot );
        break;

    default:
        Dbg_Message( "Detected Controller of unknown type in %d:%d", m_data.m_port, m_data.m_slot );
        break;
    }

    m_state = vACQUIRED;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Device::get_status( void )
{
    

    int state;

    state = scePadGetState( m_data.m_port, m_data.m_slot );
    switch ( state )
    {
        
    case scePadStateFindCTP1:
    	return vREADY;
		
	case scePadStateStable:   
        return vCONNECTED;

    case scePadStateError:
    case scePadStateDiscon:
	case scePadStateClosed:
		m_plugged_in = false;
        // problems with controller connection
		return vDISCONNECTED;

    case scePadStateFindPad:
    case scePadStateExecCmd:
		return vNOTREADY;

    default:
        Dbg_MsgAssert( 0,( "Unhandled Controller Pad State" ));
        return vREADY;
    }                 
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
	m_data.m_button_mode = vDIGITAL;
    m_data.m_valid = false;

	m_plugged_in = true;
    
    
    memset( m_data.m_actuator_direct, 0, ACTUATOR_BUFFER_LENGTH );
    memset( m_data.m_actuator_align, 0xFF, ACTUATOR_BUFFER_LENGTH );
    m_data.m_actuator_align[0] = 0;
    m_data.m_actuator_align[1] = 1;

    memset( m_data.m_control_data, 0, CTRL_BUFFER_LENGTH );
	m_data.m_prealbuffer = Mem::Malloc( scePadDmaBufferMax * sizeof( uint128) + 63 );
	m_data.m_dma_buff =  (unsigned char*)((((uint32)m_data.m_prealbuffer)+63)&~63);
	
    memset( m_data.m_dma_buff, 0, sizeof( uint128 ) * scePadDmaBufferMax );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
Device::~Device ( void )
{
	

	Dbg_AssertType ( m_node, Lst::Node< Device > );
	Mem::Free( m_data.m_prealbuffer );
	delete m_node;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Device::Acquire ( void )
{
    
    
    Dbg_Message( "Acquiring controller port %d slot %d\n", m_data.m_port, m_data.m_slot );

    if( m_state == vIDLE )
    {
        if( scePadPortOpen( m_data.m_port, m_data.m_slot, (uint128*)m_data.m_dma_buff ) == 1 )
        {   
            memset( m_data.m_control_data, 0, CTRL_BUFFER_LENGTH );
            
            m_data.m_valid = false;
            m_state = vACQUIRING;
        }
        else
        {
            Dbg_Warning( "failed to open controller port %d slot %d\n", m_data.m_port, m_data.m_slot );
        }
    }
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
        if( scePadPortClose( m_data.m_port, m_data.m_slot ) != 1 )
        {
            Dbg_Warning( "failed to close controller port %d slot %d\n", m_data.m_port, m_data.m_slot );
        }
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
	if (m_data.m_actuators_disabled)
	{
		return;
	}	
	
    float act_strength;
    
    // First, make sure we're in a ready state and our controller has actuators
    if(( m_state == vACQUIRED ) && ( m_data.m_caps.TestMask( mACTUATORS )))
    {
        if(( act_num >= 0 ) && ( act_num < m_data.m_num_actuators ))
        {
            act_strength = ((float) percent * m_data.m_actuator_max[act_num] ) / 100.0f;
            // for lack of a rounding function, perform this check here
            if( m_data.m_actuator_max[act_num] == 1 )
            {
                if( act_strength > 0.001f )
                {
                    m_data.m_actuator_direct[act_num] = 1;
                }
                else
                {
                    m_data.m_actuator_direct[act_num] = 0;
                }
            }
            else
            {
                m_data.m_actuator_direct[act_num] = (unsigned char) act_strength;
            }
            
            scePadSetActDirect( m_data.m_port, m_data.m_slot, m_data.m_actuator_direct );
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
        scePadSetActDirect( m_data.m_port, m_data.m_slot, m_data.m_actuator_direct );
	}	
	
	// Set the flag.
	m_data.m_actuators_disabled=true;
}

void Device::EnableActuators()
{
    
	m_data.m_actuators_disabled=false;
}


void Device::ResetActuators()
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
    

	if (!m_data.m_paused_ref.InUse())
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
			scePadSetActDirect( m_data.m_port, m_data.m_slot, m_data.m_actuator_direct );
		}	
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
    
	
	if (m_data.m_paused_ref.InUse())
	{
		m_data.m_paused_ref.Release();
		if (!m_data.m_paused_ref.InUse())
		{
			// First, make sure we're in a ready state and our controller has actuators
			if(( m_state == vACQUIRED ) && ( m_data.m_caps.TestMask( mACTUATORS )))
			{
				for (int i=0; i<m_data.m_num_actuators; ++i)
				{
					// Restore the saved vibration strength.
					m_data.m_actuator_direct[i]=m_data.m_actuator_old_direct[i];
				}	
				scePadSetActDirect( m_data.m_port, m_data.m_slot, m_data.m_actuator_direct );
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
		scePadSetActDirect( m_data.m_port, m_data.m_slot, m_data.m_actuator_direct );
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
        if( scePadEnterPressMode( m_data.m_port, m_data.m_slot ) == 1 )
        {
            m_state = vBUSY;
            m_next_state = vPRESS_MODE_ON_COMPLETE;
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
        if( scePadExitPressMode( m_data.m_port, m_data.m_slot ) == 1 )
        {
            m_state = vBUSY;
            m_next_state = vPRESS_MODE_OFF_COMPLETE;
        }
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
} // namespace SIO
