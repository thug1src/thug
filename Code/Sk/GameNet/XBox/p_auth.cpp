/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2002 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Skate4													**
**																			**
**	Module:			GameNet			 										**
**																			**
**	File name:		p_auth.cpp												**
**																			**
**	Created by:		02/28/02	-	SPG										**
**																			**
**	Description:	XBox Online Authorization Code		 					**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <sys/McMan.h>
#include <gel/mainloop.h>
#include <gel/scripting/script.h>
#include <sk/gamenet/gamenet.h>
#include <GameNet/Xbox/p_auth.h>
#include <GameNet/Xbox/p_buddy.h>
#include <GameNet/Xbox/p_voice.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace GameNet
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

const DWORD vMAX_MEMORY_UNITS  = 2 * XGetPortCount();
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
**							  Private Functions								**
*****************************************************************************/

void	AuthMan::s_logon_state_code( const Tsk::Task< AuthMan >& task )
{
#	if 0
	AuthMan& man = task.GetData();
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	switch( man.m_state )
	{
		case vSTATE_FILL_ACCOUNT_LIST:		
			if( man.gather_user_list() == 0 )
			{
				man.m_state = vSTATE_CREATE_ACCOUNT;
			}
			else
			{
				man.fill_user_list();
				man.m_state = vSTATE_SELECT_ACCOUNT;
			}
			break;				
		case vSTATE_SELECT_ACCOUNT:
			break;			
		case vSTATE_CREATE_ACCOUNT:		
			man.create_account();
			break;
		
		case vSTATE_GET_PIN:
			Script::RunScript( "launch_pin_entry_menu" );
			man.m_pin_entry.BeginInput( man.m_chosen_account );
			man.m_state = vSTATE_WAIT;
			break;
		case vSTATE_PIN_COMPLETE:
			man.PinAttempt( man.m_pin_entry.GetPin());
			break;
		case vSTATE_PIN_CANCELLED:
			man.m_pin_entry.EndInput();
			man.m_state = vSTATE_FILL_ACCOUNT_LIST;
			break;
		case vSTATE_PIN_CLEARED:
			man.m_state = vSTATE_GET_PIN;
			man.m_pin_entry.ClearInput();
			break;
		case vSTATE_LOG_ON:
			man.logon();
			man.m_state = vSTATE_LOGGING_ON;
			break;
		case vSTATE_LOGGING_ON:		
			man.check_logon_progress();			
			break;
		case vSTATE_SUCCESS:
			Script::RunScript( "launch_xbox_online_menu" );
			man.m_signed_in = true;
			man.m_state = vSTATE_LOGGED_IN;
			gamenet_man->mpVoiceMan->Startup();
			gamenet_man->mpBuddyMan->SpawnBuddyList( man.m_chosen_account );
			break;
		case vSTATE_LOGGED_IN:
			man.pump_logon_task();
			break;
		case vSTATE_CANCEL:
		{
			Lst::DynamicTableDestroyer<XONLINE_USER> destroyer( &man.m_user_list );
			destroyer.DeleteTableContents();

			delete man.mp_logon_state_task;
			man.mp_logon_state_task = NULL;
			man.m_state = vSTATE_WAIT;		
			break;
		}
		case vSTATE_WAIT:
			break;
		case vSTATE_ERROR:
			man.m_state = man.m_next_state;
			switch( man.m_error )
			{
				case vERROR_WRONG_PIN:
					Script::RunScript( "launch_wrong_pin_dialog_box" );
					break;
				case vERROR_GENERAL_LOGIN:
					break;
				case vERROR_LOGIN_FAILED:
					break;
			}			
			break;
	}	
#	endif
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static DWORD	s_get_mem_unit_slot( DWORD i )
{
	return( ( i % 2 ) ? XDEVICE_BOTTOM_SLOT : XDEVICE_TOP_SLOT );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static DWORD	s_get_mem_unit_port( DWORD i )
{
	return( i / 2 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static DWORD	s_get_mem_unit_mask( DWORD i )
{
	// The XGetDevices bitmask is formatted as follows:
    //
    //      0x00000001      port 0 top slot         i = 0
    //      0x00010000      port 0 bottom slot          1
    //      0x00000002      port 1 top slot             2
    //      0x00020000      port 1 bottom slot          3
    //      0x00000004      port 2 top slot             4
    //      0x00040000      port 2 bottom slot          5
    //      0x00000008      port 3 top slot             6
    //      0x00080000      port 3 bottom slot          7

    DWORD dwMask = 1 << s_get_mem_unit_port( i );
    if( s_get_mem_unit_slot( i ) == XDEVICE_BOTTOM_SLOT )
        dwMask <<= 16;

    return( dwMask );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		AuthMan::gather_user_list( void )
{
    // Get accounts stored on the hard disk
    DWORD num_users = 0;

#	if 0
	// On input, the list must have room for XONLINE_MAX_STORED_ONLINE_USERS
    // accounts
    XONLINE_USER user_list[ XONLINE_MAX_STORED_ONLINE_USERS ];

    HRESULT hr = XOnlineGetUsers( user_list, &num_users);
    if( SUCCEEDED(hr) )
    {
        for( DWORD i = 0; i < num_users; ++i )
		{
			XONLINE_USER* new_user;
			
			new_user = new XONLINE_USER;
			*new_user = user_list[i];
            m_user_list.Add( new_user );
		}
    }
#	endif

	return num_users;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	AuthMan::fill_user_list( void )
{
	int i;
	
	Script::RunScript( "create_account_list_menu" );	

	for( i = 0; i < m_user_list.GetSize(); i++ )
	{
		Script::CStruct* p_item_params = new Script::CStruct;
		XONLINE_USER* user;
		
		user = &m_user_list[i];

		//p_item_params->AddString( "text", user->gamertag );
		p_item_params->AddChecksum( "id", 123456 + i );		
		p_item_params->AddChecksum( "pad_choose_script",Script::GenerateCRC("choose_selected_account"));

		// create the parameters that are passed to the X script
		Script::CStruct *p_script_params= new Script::CStruct;
		p_script_params->AddInteger( "index", i );	
		p_item_params->AddStructure("pad_choose_params",p_script_params);			

		Script::RunScript("make_text_sub_menu_item",p_item_params);
		delete p_item_params;
		delete p_script_params;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	AuthMan::create_account( void )
{
	Script::RunScript( "launch_no_accounts_dialog" );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	AuthMan::logon( void )
{	
#	if 0
	XONLINE_USER user_list[ XGetPortCount() ] = { 0 };	
	
	CopyMemory( &user_list[ 0 ], &m_user_list[ m_chosen_account ], sizeof( XONLINE_USER ));

    // Initiate the login process. XOnlineTaskContinue() is used to poll
    // the status of the login.
    HRESULT hr = XOnlineLogon( user_list, mp_services, vNUM_SERVICES, 
                               NULL, &mh_online_task );

    if( FAILED(hr) || mh_online_task == NULL )
    {
		if( mh_online_task != NULL )
		{
			hr = XOnlineTaskClose( mh_online_task );
			mh_online_task = NULL;
		}        
        m_state = vSTATE_ERROR;
		m_next_state = vSTATE_WAIT;
		m_error = vERROR_GENERAL_LOGIN;        
    }
#	endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	AuthMan::xbox_has_voice_device( void )
{
	return false;
	
//	DWORD dwConnectedMicrophones = XGetDevices( XDEVICE_TYPE_VOICE_MICROPHONE );
//	DWORD dwConnectedHeadphones = XGetDevices( XDEVICE_TYPE_VOICE_HEADPHONE );

    // Voice is available if there's at least one mike and one headphone
//	return( dwConnectedMicrophones >= 1 && dwConnectedHeadphones  >= 1 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	AuthMan::pump_logon_task( void )
{
#	if 0
	// Pump the task
	HRESULT hr;
	hr = XOnlineTaskContinue( mh_online_task );
	
	// Check status
	if( FAILED( hr ))
	{	
		//m_UI.SetErrorStr( L"Connection was lost. Must relogin" );
	}
#	endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	AuthMan::check_logon_progress( void )
{
#	if 0
	// Pump the task
	HRESULT hr;
	hr = XOnlineTaskContinue( mh_online_task );
	
	// Check status
	if ( hr != XONLINETASK_S_RUNNING )
	{	
		if ( hr != XONLINE_S_LOGON_CONNECTION_ESTABLISHED  )
        {
            XOnlineTaskClose( mh_online_task );

			m_state = vSTATE_ERROR;
			m_error = vERROR_LOGIN_FAILED;				
            return;
        }		
	
		// Check login status; partial results indicate that login itself
		// has completed.
		BOOL success = TRUE;
		BOOL service_err = FALSE;
		HRESULT hrService = S_OK;
		DWORD i = 0;
		
		// Next, check if the user was actually logged on
		PXONLINE_USER pLoggedOnUsers = XOnlineGetLogonUsers();
		Dbg_Assert( pLoggedOnUsers );

		hr = pLoggedOnUsers[ m_chosen_account ].hr;

		// Check for general errors
		if( FAILED(hr) )
		{
			XOnlineTaskClose( mh_online_task );

			m_state = vSTATE_ERROR;
			m_error = vERROR_LOGIN_FAILED;				
            return;
		}
		
		// Check for service errors
		for( i = 0; i < vNUM_SERVICES; ++i )
		{
			if( FAILED( hrService = XOnlineGetServiceInfo( mp_services[i], NULL) ))
			{
				success = FALSE;
				service_err = TRUE;
				break;
			}		
		}

		// If no errors, login was successful
		m_state = success ? vSTATE_SUCCESS : vSTATE_ERROR;

		if( !success )
		{
			if( service_err )
			{
				m_error = vERROR_SERVICE_DOWN;
				/*m_UI.SetErrorStr( L"Login failed.\n\n"
								  L"Error %x logging into service %d",
								  mp_services[i].hr, mp_services[i].dwServiceID );*/
			}
			else
			{
				m_error = vERROR_LOGIN_FAILED;
				/*m_UI.SetErrorStr( L"Login failed.\n\n"
								  L"Error %x returned by "
								  L"XOnlineTaskContinue", hr );*/					
			}
			XOnlineTaskClose( mh_online_task );
			m_state = vSTATE_ERROR;
			m_error = vERROR_LOGIN_FAILED;				
		}			
		else
		{
			DWORD player_state;

			m_service_info_list.Reset();
			for( i = 0; i < vNUM_SERVICES; ++i )
			{
				// Stored service information for UI
				XONLINE_SERVICE_INFO* service_info;			

				service_info = new XONLINE_SERVICE_INFO;                
                XOnlineGetServiceInfo( mp_services[i], service_info );
				m_service_info_list.Add( service_info );
			}

			// Notify the world of our state change
			player_state = XONLINE_FRIENDSTATE_FLAG_ONLINE;
			if( xbox_has_voice_device())
			{
				player_state |= XONLINE_FRIENDSTATE_FLAG_VOICE;
			}

			XOnlineNotificationSetState( m_chosen_account, player_state,
                                         XNKID(), 0, NULL );			
		}
	}
#	endif
}

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

AuthMan::AuthMan( void )
: m_user_list( 1 ), m_service_info_list( 1 ), m_signed_in( false )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	AuthMan::UserLogon( void )
{	
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();

	mp_logon_state_task = new Tsk::Task< AuthMan > ( s_logon_state_code, *this,
										Tsk::BaseTask::Node::vNORMAL_PRIORITY );
	mlp_man->AddLogicTask( *mp_logon_state_task );
	m_state = vSTATE_FILL_ACCOUNT_LIST;	
	m_chosen_account = 0;
	mp_services[0] = XONLINE_MATCHMAKING_SERVICE;
    mp_services[1] = XONLINE_BILLING_OFFERING_SERVICE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	AuthMan::CancelLogon( void )
{
	m_state = vSTATE_CANCEL;		
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	AuthMan::SelectAccount( int index )
{
	m_chosen_account = index;

#	if 0
	DWORD pin_required = m_user_list[ m_chosen_account ].dwUserOptions & XONLINE_USER_OPTION_REQUIRE_PIN;
	if( pin_required )
	{
		m_state = vSTATE_GET_PIN;
	}
	else
	{
		m_state = vSTATE_LOG_ON;
	}
#	endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	AuthMan::PinAttempt( BYTE* pin )
{	
#	if 0
	if(( memcmp( pin, m_user_list[m_chosen_account].pin, XONLINE_PIN_LENGTH ) == 0 ))
	{
		m_state = vSTATE_LOG_ON;		
	}
	else
	{
		m_state = vSTATE_ERROR;
		m_next_state = vSTATE_CANCEL;
		m_error = vERROR_WRONG_PIN;
	}
#	endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	AuthMan::SetState( LogonState state )
{
	m_state = state;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	AuthMan::SignedIn( void )
{
	return m_signed_in;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	AuthMan::SignOut( void )
{
#	if 0
	XOnlineTaskClose( mh_online_task );
	mh_online_task = NULL;
	m_signed_in = false;
#	endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PinEntry::s_pin_entry_input_logic_code ( const Inp::Handler < PinEntry >& handler )
{	
	PinEntry&	entry = handler.GetData();

	// 'B' and 'Back' cancel the PIN entry
	if(	( handler.m_Input->m_Buttons & Inp::Data::mD_SELECT ) ||
		( handler.m_Input->m_Buttons & Inp::Data::mD_CIRCLE ))
	{
		entry.m_cancelled = true;
		return;
	}

	// 'A' and 'Start' are ignored
	if(	( handler.m_Input->m_Buttons & Inp::Data::mD_X ) ||
		( handler.m_Input->m_Buttons & Inp::Data::mD_START ))
	{
		return;
	}

	XINPUT_GAMEPAD pad = {0};	

	if( handler.m_Input->m_Buttons & Inp::Data::mD_L3 )
	{
		entry.m_cleared = true;
		return;
	}

	if( handler.m_Input->m_Buttons & Inp::Data::mD_LEFT )
	{
		pad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
	}
	if( handler.m_Input->m_Buttons & Inp::Data::mD_RIGHT )
	{
		pad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
	}
	if( handler.m_Input->m_Buttons & Inp::Data::mD_UP )
	{
		pad.wButtons |= XINPUT_GAMEPAD_DPAD_UP;
	}
	if( handler.m_Input->m_Buttons & Inp::Data::mD_DOWN )
	{
		pad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
	}
	if( handler.m_Input->m_Buttons & Inp::Data::mD_TRIANGLE )
	{
		pad.bAnalogButtons[XINPUT_GAMEPAD_Y] = 255;		
	}
	if( handler.m_Input->m_Buttons & Inp::Data::mD_SQUARE )
	{
		pad.bAnalogButtons[XINPUT_GAMEPAD_X] = 255;		
	}
	if( handler.m_Input->m_Buttons & Inp::Data::mD_L1 )
	{
		pad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] = 255;		
	}
	if( handler.m_Input->m_Buttons & Inp::Data::mD_R1 )
	{
		pad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] = 255;		
	}
	if( handler.m_Input->m_Buttons & Inp::Data::mD_L2 )
	{
		pad.bAnalogButtons[XINPUT_GAMEPAD_WHITE] = 255;		
	}
	if( handler.m_Input->m_Buttons & Inp::Data::mD_R2 )
	{
		pad.bAnalogButtons[XINPUT_GAMEPAD_BLACK] = 255;		
	}		

	XINPUT_STATE xis;
    CopyMemory( &xis.Gamepad, &pad, sizeof( pad ));

#	if 0
    HRESULT hr = XOnlinePINDecodeInput( entry.m_pin_input, &xis, &entry.m_frame_entry );
    if( hr == S_OK )
    {
		entry.m_got_entry = true;        
    }
#	endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PinEntry::s_pin_entry_logic_code( const Tsk::Task< PinEntry >& task )
{
#	if 0
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	PinEntry& entry = task.GetData();

	if( entry.m_cleared )
	{
		gamenet_man->mp_AuthMan->SetState( AuthMan::vSTATE_PIN_CLEARED );		
	}
	else if( entry.m_cancelled )
	{		
		gamenet_man->mp_AuthMan->SetState( AuthMan::vSTATE_PIN_CANCELLED );		
	}
	else if( entry.m_got_entry )
	{
		entry.m_pin[entry.m_pin_length++] = entry.m_frame_entry;
		entry.m_got_entry = false;
		// Here update the UI with the latest pin entry

		if( entry.Complete())
		{
			entry.EndInput();
			
			// Submit this entry as a our final pin
			GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
			gamenet_man->mp_AuthMan->SetState( AuthMan::vSTATE_PIN_COMPLETE );
		}
	}
#	endif
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PinEntry::ClearInput( void )
{
	m_pin_length = 0;
	m_got_entry = false;
	m_cleared = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PinEntry::BeginInput( int controller )
{
#	if 0
	int i;

	Mlp::Manager * mlp_man = Mlp::Manager::Instance();
	Inp::Manager * inp_manager = Inp::Manager::Instance();

	mp_pin_entry_logic_task = new Tsk::Task< PinEntry > ( s_pin_entry_logic_code, *this,
										Tsk::BaseTask::Node::vNORMAL_PRIORITY );
	mlp_man->AddLogicTask( *mp_pin_entry_logic_task );
	mp_pin_entry_input_handler = new Inp::Handler< PinEntry > ( controller,  
														s_pin_entry_input_logic_code, *this, 
														Tsk::BaseTask::Node::vNORMAL_PRIORITY-1 );
	inp_manager->AddHandler( *mp_pin_entry_input_handler );	

	for( i = 0; i < XONLINE_PIN_LENGTH; i++ )
	{
		m_pin[i] = 0;
	}

	m_pin_length = 0;
	m_frame_entry = 0;
	m_cancelled = false;
	m_got_entry = false;
	m_cleared = false;

	XINPUT_STATE xis = {0};
    m_pin_input = XOnlinePINStartInput( &xis );	
#	endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	PinEntry::Complete( void )
{
//	return( m_pin_length == XONLINE_PIN_LENGTH );
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PinEntry::EndInput( void )
{
	delete mp_pin_entry_logic_task;
	mp_pin_entry_logic_task = NULL;
	delete mp_pin_entry_input_handler;
	mp_pin_entry_input_handler = NULL;

//	XOnlinePINEndInput( m_pin_input );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

BYTE*	PinEntry::GetPin( void )
{
//	return m_pin;
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace GameNet


	

