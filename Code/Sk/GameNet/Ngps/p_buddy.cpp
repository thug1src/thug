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
**	File name:		p_buddy.cpp												**
**																			**
**	Created by:		02/28/02	-	SPG										**
**																			**
**	Description:	PS2 Buddy List Code 									**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <gel/mainloop.h>
#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>

#include <GameNet/GameNet.h>
#include <GameNet/Ngps/p_buddy.h>
#include <GameNet/lobby.h>

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

#define vBUDDY_CONNECT_TIMEOUT Tmr::Seconds( 10 )

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

static 	Tmr::Time	s_connect_start_time = 0;

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

void BuddyMan::s_error_callback( GPConnection* connection, void* arg, void* param )
{
    GPErrorArg* error;
	BuddyMan* buddy_man;

	error = (GPErrorArg*) arg;
	buddy_man = (BuddyMan*) param;
	
	Dbg_Printf( "******** ERROR: %d %s\n", error->errorCode, error->errorString );

	if( error->fatal )
	{
		Dbg_Printf( "******** ERROR IS FATAL\n" );
	}
	
	switch( error->errorCode )
	{
		case GP_NEWUSER_BAD_PASSWORD:
			Dbg_Printf( "Bad password\n" );
			Script::RunScript( "create_wrong_profile_password_dialog" );
			buddy_man->Disconnect();
			break;
		case GP_NEWUSER_BAD_NICK:
			Dbg_Printf( "Bad nick (already exists?) Trying to connect now\n" );
			buddy_man->m_state = vSTATE_BEGIN_LOGIN;
			break;
		case GP_ADDBUDDY_ALREADY_BUDDY:
			Script::RunScript( "failed_add_buddy_already_buddy" );
			break;
		case GP_LOGIN_CONNECTION_FAILED:
			buddy_man->Disconnect();
			Script::RunScript( "CreateBuddyLoginFailedDialog" );
			break;
		default:
			Dbg_Printf( "Not handled: %d\n", error->errorCode );
			break;
		
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void BuddyMan::s_buddy_request( GPConnection* connection, void* arg, void* param )
{
	BuddyMan* buddy_man;
	GPRecvBuddyRequestArg* request;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());
	
	buddy_man = (BuddyMan*) param;
	request = (GPRecvBuddyRequestArg*) arg;
	
	// Always accept
	gpAuthBuddyRequest( &buddy_man->m_gp_conn, request->profile );

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void BuddyMan::s_buddy_status( GPConnection* connection, void* arg, void* param )
{
	BuddyMan* buddy_man;
	GPRecvBuddyStatusArg* status_arg;
	GPBuddyStatus status;
	Lst::Search< BuddyInfo > sh;
	BuddyInfo* buddy;
	char* join_info;
	char* location_str;
	bool handled;
	int i;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

	buddy_man = (BuddyMan*) param;
	status_arg = (GPRecvBuddyStatusArg*) arg;
    
	gpGetBuddyStatus( &buddy_man->m_gp_conn, status_arg->index, &status );

	//Dbg_Printf( "Got buddy status %d for %d : %s, %s\n", status.status, status.profile, status.statusString, status.locationString );

	handled = false;
	for( buddy = sh.FirstItem( buddy_man->m_buddies ); buddy; buddy = sh.NextItem())
	{
		if( buddy->m_Profile == status.profile )
		{
			Script::CStruct* p_status_params;

			buddy->m_Status = status.status;
			
			strcpy( buddy->m_StatusString, status.statusString );
			if( buddy->m_Status == GP_PLAYING )
			{
				location_str = strchr( status.locationString, ':' );
				location_str += 1;
				location_str = strchr( location_str, ':' );
				location_str += 1;
				location_str = strchr( location_str, ':' );
				location_str += 1;
	
				strcpy( buddy->m_Location, location_str );
				join_info = strtok( status.locationString, ":" );
				Dbg_Assert( join_info != NULL );
				buddy->m_JoinIP = atoi( join_info );
				join_info = strtok( NULL, ":" );
				Dbg_Assert( join_info != NULL );
				buddy->m_JoinPrivateIP = atoi( join_info );
				join_info = strtok( NULL, ":" );
				Dbg_Assert( join_info != NULL );
				buddy->m_JoinPort = atoi( join_info );
			}
			else
			{
				strcpy( buddy->m_Location, status.locationString );
			}

			if( buddy->m_Pending )
			{
				// This was a pending addition.  Add him to our permanent list
				GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
				Prefs::Preferences*	prefs;
				Script::CArray* buddy_array, *new_array;
				Script::CStruct* new_buddy;
				Script::CStruct* params;
                      
				params = new Script::CStruct;	
				params->AddString( "net_name", buddy->m_Nick );

				Script::RunScript( "added_buddy", params );
				delete params;

				buddy->m_Pending = false;

				prefs = gamenet_man->GetNetworkPreferences();
				buddy_array = prefs->GetPreferenceArray( Script::GenerateCRC( "buddy_array" ));

				new_array = new Script::CArray;
				new_array->SetSizeAndType( buddy_array->GetSize() + 1, ESYMBOLTYPE_STRUCTURE );

				for( i = 0; i < (int) buddy_array->GetSize(); i++ )
				{
					Script::CStruct* entry, *new_entry;

					entry = buddy_array->GetStructure( i );
					new_entry = new Script::CStruct;
					*new_entry = *entry;
					new_array->SetStructure( i, new_entry );
				}

				new_buddy = new Script::CStruct;
				new_buddy->AddInteger( "profile", buddy->m_Profile );
				new_buddy->AddString( "nick", buddy->m_Nick );
				new_array->SetStructure( i, new_buddy );
				
				prefs->SetPreference( Script::GenerateCRC( "buddy_array" ), new_array );
			}

			p_status_params = new Script::CScriptStructure;
			p_status_params->AddComponent( Script::GenerateCRC( "name" ), ESYMBOLTYPE_STRING, buddy->m_Nick );
			p_status_params->AddComponent( Script::GenerateCRC( "status" ), ESYMBOLTYPE_STRING, buddy->m_StatusString );
			p_status_params->AddComponent( Script::GenerateCRC( "location" ), ESYMBOLTYPE_STRING, buddy->m_Location );
			p_status_params->AddComponent( Script::GenerateCRC( "id" ), ESYMBOLTYPE_NAME, Script::GenerateCRC( "first_lobby_buddy" ) + buddy->m_Profile );
			Script::RunScript( "update_buddy_status", p_status_params );
			delete p_status_params;

			handled = true;
			break;
		}
	}
	
	if( !handled )
	{
		GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		Prefs::Preferences*	prefs;
		Script::CArray* buddy_array;
		int profile;

		prefs = gamenet_man->GetNetworkPreferences();

		buddy = new BuddyInfo;
		buddy->m_Profile = status.profile;
		buddy->m_Status = status.status;
		strcpy( buddy->m_StatusString, status.statusString );
		sprintf( buddy->m_Nick, "ProSkater" );

		if( buddy->m_Status == GP_PLAYING )
		{
			location_str = strchr( status.locationString, ':' );
			location_str += 1;
			location_str = strchr( location_str, ':' );
			location_str += 1;
			location_str = strchr( location_str, ':' );
			location_str += 1;
	
			strcpy( buddy->m_Location, location_str );
			join_info = strtok( status.locationString, ":" );
			Dbg_Assert( join_info );
			buddy->m_JoinIP = atoi( join_info );
			join_info = strtok( NULL, ":" );
			Dbg_Assert( join_info );
			buddy->m_JoinPrivateIP = atoi( join_info );
			join_info = strtok( NULL, ":" );
			Dbg_Assert( join_info );
			buddy->m_JoinPort = atoi( join_info );
		}
		else
		{
			strcpy( buddy->m_Location, status.locationString );
		}

		buddy_man->m_buddies.AddToTail( buddy );

		// We now need to find the name by which we know this buddy.  When we added them to our list
		// we added an entry into our network preferences with a matching profile ID and the player's
		// network name
		buddy_array = prefs->GetPreferenceArray( Script::GenerateCRC( "buddy_array" ));
		if( buddy_array )
		{
			Script::CStruct* entry;
			for( i = 0; i < (int) buddy_array->GetSize(); i++ )
			{
				entry = buddy_array->GetStructure( i );
				entry->GetInteger( "profile", &profile, Script::ASSERT );
				if( profile == status.profile )
				{
					const char* nick = NULL;
					entry->GetString( "nick", &nick, Script::ASSERT );

					strcpy( buddy->m_Nick, nick );
				}
			}
		}
	}

    Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void BuddyMan::s_buddy_message( GPConnection* connection, void* arg, void* param )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void BuddyMan::s_game_invite( GPConnection* connection, void* arg, void* param )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void BuddyMan::s_transfer_callback( GPConnection* connection, void* arg, void* param )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void BuddyMan::s_connect_callback( GPConnection* connection, void* arg, void* param )
{
	BuddyMan* buddy_man;
    GPConnectResponseArg* response;

	buddy_man = (BuddyMan*) param;
	response = (GPConnectResponseArg*) arg;
	switch( response->result )
	{
		case GP_NO_ERROR:
			//Dbg_Printf( "*************** CONNECTED, NO ERROR\n" );
			buddy_man->Connected( response->profile );
			break;
		case GP_MEMORY_ERROR:
			//Dbg_Printf( "*************** DIDNT CONNECT: MEMORY ERROR\n" );
			break;
		case GP_PARAMETER_ERROR:
			//Dbg_Printf( "*************** DIDNT CONNECT: PARAMETER ERROR\n" );
			break;
		case GP_NETWORK_ERROR:
			//Dbg_Printf( "*************** DIDNT CONNECT: NETWORK ERROR\n" );
			break;
		case GP_SERVER_ERROR:
			//Dbg_Printf( "*************** DIDNT CONNECT: SERVER ERROR\n" );
			break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	BuddyMan::s_buddy_logic_code( const Tsk::Task< BuddyMan >& task )
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());
	BuddyMan& man = task.GetData();
	static Tmr::Time s_last_process_time = 0;

	switch( man.m_state )
	{
		case vSTATE_WAIT:
			break;
		case vSTATE_BEGIN_LOGIN:
			man.Connect();
			break;
		case vSTATE_CREATING_PROFILE:
		case vSTATE_CONNECTING:
			if(( Tmr::GetTime() - s_connect_start_time ) > vBUDDY_CONNECT_TIMEOUT )
			{
				//Dbg_Printf( "Timing buddy server connection out\n" );
				man.Disconnect();
				Script::RunScript( "CreateBuddyLoginFailedDialog" );
			}
			else
			{
				gpProcess( &man.m_gp_conn );
			}
			break;
		case vSTATE_LOGGED_IN:
			if(( Tmr::GetTime() - s_last_process_time ) > Tmr::Seconds( 1 ))
			{
				gpProcess( &man.m_gp_conn );
				s_last_process_time = Tmr::GetTime();
			}
			break;
		default:
			Dbg_Assert( 0 );
			break;

	}

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void 	BuddyMan::s_threaded_buddy_connect( BuddyMan* buddy_man )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Prefs::Preferences*	prefs;
	GPResult result;
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

	// Register this thread with the sockets API
	sockAPIregthr();

	prefs = gamenet_man->GetNetworkPreferences();
	const char* email = prefs->GetPreferenceString( Script::GenerateCRC( "profile_email" ), Script::GenerateCRC("ui_string") );
	const char* password = prefs->GetPreferenceString( Script::GenerateCRC( "profile_password" ), Script::GenerateCRC("ui_string") );

	Dbg_Printf( "Connecting to gp with %s : %s\n", email, password );
	result = gpConnect( &buddy_man->m_gp_conn, "proskater", email, password, GP_FIREWALL, GP_NON_BLOCKING, s_connect_callback, buddy_man );

	// Deregister this thread with the sockets API
	sockAPIderegthr();

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void 	BuddyMan::s_threaded_buddy_create( BuddyMan* buddy_man )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Prefs::Preferences*	prefs;
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

	// Register this thread with the sockets API
	sockAPIregthr();

	prefs = gamenet_man->GetNetworkPreferences();
	const char* email = prefs->GetPreferenceString( Script::GenerateCRC( "profile_email" ), Script::GenerateCRC("ui_string") );
	const char* password = prefs->GetPreferenceString( Script::GenerateCRC( "profile_password" ), Script::GenerateCRC("ui_string") );

	//Dbg_Printf( "************ Creating profile: Proskater for account %s, password %s\n", email, password );
	gpConnectNewUser( &buddy_man->m_gp_conn, "proskater", email, password, GP_FIREWALL, GP_NON_BLOCKING, s_connect_callback, buddy_man );

	/*if( buddy_man->m_command == vCOMMAND_LOGIN )
	{
		//Dbg_Printf( "Connecting instead\n" );
		Script::RunScript( "log_in_profile" );
		buddy_man->m_command = vCOMMAND_CONNECT;
		s_connect_start_time = Tmr::GetTime();
		result = gpConnect( &buddy_man->m_gp_conn, "proskater", email, password, GP_FIREWALL, GP_NON_BLOCKING, s_connect_callback, buddy_man );
	}*/
	//Dbg_Printf( "called gpconnect\n" );

	// Deregister this thread with the sockets API
	sockAPIderegthr();

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		BuddyMan::add_player_to_menu( BuddyInfo* buddy, bool allow_join, bool allow_remove )
{
	Script::CStruct* p_item_params;
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	p_item_params = new Script::CStruct;	
	char upper_nick[1024];

	//Dbg_Printf( "****** add_player_to_menu. Allow Join %d\n", (int) allow_join );

	sprintf( upper_nick, buddy->m_Nick );

	p_item_params->AddString( "name", buddy->m_Nick );
	p_item_params->AddString( "status", buddy->m_StatusString );
	p_item_params->AddString( "location", buddy->m_Location );
	p_item_params->AddChecksum( "id", Script::GenerateCRC( "first_lobby_buddy" ) + buddy->m_Profile );
	if( allow_join || allow_remove )
	{
		// create the parameters that are passed to the X script
		Script::CStruct *p_script_params= new Script::CStruct;
		p_script_params->AddInteger( "profile", buddy->m_Profile );	
		p_script_params->AddString( "nick", strupr( upper_nick ));	
		p_script_params->AddString( "status", buddy->m_StatusString );	
		p_script_params->AddString( "location", buddy->m_Location );
		p_script_params->AddInteger( "join_ip", buddy->m_JoinIP );	
		p_script_params->AddInteger( "join_private_ip", buddy->m_JoinPrivateIP );	
		p_script_params->AddInteger( "join_port", buddy->m_JoinPort );	

		if( allow_remove )
		{
			p_script_params->AddChecksum( NONAME, Script::GenerateCRC( "allow_remove" ));	
		}

		if( allow_join && ( buddy->m_Status == GP_PLAYING ))
		{
			//Dbg_Printf( "****** GP_PLAYING\n" );
			p_script_params->AddChecksum( NONAME, Script::GenerateCRC( "allow_join" ));	
		}

		if( allow_join )
		{
			p_script_params->AddChecksum( NONAME, Script::GenerateCRC( "in_lobby" ));	
		}
		else
		{
			p_script_params->AddChecksum( "back_script", Script::GenerateCRC( "back_from_shell_buddy_options" ));   
			p_script_params->AddChecksum( "remove_script", Script::GenerateCRC( "shell_remove_buddy" ));   
		}
		p_item_params->AddStructure( "pad_choose_params", p_script_params );
		p_item_params->AddChecksum( "pad_choose_script",Script::GenerateCRC("present_buddy_options"));
	}
	else
	{
		p_item_params->AddChecksum( "pad_choose_script",Script::GenerateCRC("do_nothing"));
	}
	
	p_item_params->AddChecksum( "parent_menu_id", Script::GenerateCRC( "lobby_buddy_list_menu" ));
	if( allow_join )
	{
		p_item_params->AddFloat( "scale", 1.1f );
	}                                            

	//Script::RunScript("make_text_sub_menu_item",p_item_params);
	Script::RunScript("homie_list_add_item",p_item_params);
	delete p_item_params;

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		BuddyMan::fill_buddy_list( bool clear_list, bool allow_join, bool allow_remove )
{
	Lst::Search< BuddyInfo > sh;
	BuddyInfo* buddy;

	if( clear_list )
	{
		//Dbg_Printf( "Clearing buddy list menu\n" );
		Script::RunScript( "destroy_lobby_buddy_list_children" );
	}

	for( buddy = sh.FirstItem( m_buddies ); buddy; buddy = sh.NextItem())
	{
		if( buddy->m_Pending )
		{
			continue;
		}
		add_player_to_menu( buddy, allow_join, allow_remove );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		BuddyMan::fill_prospective_buddy_list( void )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Lst::Search< PlayerInfo > sh;
	PlayerInfo* player;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
	{
		if( player->IsLocalPlayer())
		{
			continue;
		}
		if( player->m_Profile && !IsAlreadyMyBuddy( player->m_Profile ))
		{
			Script::CStruct* p_item_params;
	
			//Dbg_Printf( "Adding %s of %d to the list\n", player->m_Name, player->m_Profile );
			p_item_params = new Script::CStruct;	
			p_item_params->AddString( "text", player->m_Name );
			p_item_params->AddChecksum( "id", player->m_Profile );
			p_item_params->AddChecksum( "pad_choose_script",Script::GenerateCRC("add_buddy"));
			p_item_params->AddChecksum( "parent_menu_id", Script::GenerateCRC( "lobby_buddy_list_menu" ));
		
			// create the parameters that are passed to the X script
			Script::CStruct *p_script_params= new Script::CStruct;
			p_script_params->AddInteger( "profile", player->m_Profile );	
			p_script_params->AddString( "nick", player->m_Name );	
			p_item_params->AddStructure( "pad_choose_params", p_script_params );
			p_item_params->AddFloat( "scale", 1.1f );

			Script::RunScript("make_text_sub_menu_item",p_item_params);

			delete p_item_params;
			delete p_script_params;
		}
	}

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	BuddyMan::add_buddy( char *name, int profile )
{
	GPResult result;
	BuddyInfo* new_buddy;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

	new_buddy = new BuddyInfo;
	strcpy( new_buddy->m_Nick, name );
	new_buddy->m_Profile = profile;
	new_buddy->m_Status = 0;
	new_buddy->m_Pending = true;

	m_buddies.AddToTail( new_buddy );
    
	result = gpSendBuddyRequest( &m_gp_conn, profile, "" );

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	BuddyMan::cancel_add_buddy( void )
{
	BuddyInfo* buddy;
	Lst::Search< BuddyInfo > sh;

	for( buddy = sh.FirstItem( m_buddies ); buddy; buddy = sh.NextItem())
	{
		if( buddy->m_Pending )
		{
			delete buddy;
			return;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	BuddyMan::remove_buddy( int profile )
{
	BuddyInfo* buddy;
	Lst::Search< BuddyInfo > sh;
	Script::CStruct* p_struct;

	for( buddy = sh.FirstItem( m_buddies ); buddy; buddy = sh.NextItem())
	{
		if( buddy->m_Profile == profile )
		{
			delete buddy;
			break;
		}
	}

	gpDeleteBuddy( &m_gp_conn, profile );
                      
	p_struct = new Script::CStruct;
	p_struct->AddChecksum( "id", Script::GenerateCRC( "first_lobby_buddy" ) + profile );

	Script::RunScript( "removed_buddy", p_struct );
	delete p_struct;
}

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

BuddyMan::BuddyMan( void )
{
	GPResult result;
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());
	
	result = gpInitialize( &m_gp_conn, vGAMESPY_PRODUCT_ID );
	Dbg_Assert( result == GP_NO_ERROR );

	result = gpSetCallback( &m_gp_conn, GP_ERROR, s_error_callback, this );
	Dbg_Assert( result == GP_NO_ERROR );

	result = gpSetCallback( &m_gp_conn, GP_RECV_BUDDY_REQUEST, s_buddy_request, this );
	Dbg_Assert( result == GP_NO_ERROR );

	result = gpSetCallback( &m_gp_conn, GP_RECV_BUDDY_STATUS, s_buddy_status, this );
	Dbg_Assert( result == GP_NO_ERROR );

	result = gpSetCallback( &m_gp_conn, GP_RECV_BUDDY_MESSAGE, s_buddy_message, this );
	Dbg_Assert( result == GP_NO_ERROR );

	result = gpSetCallback( &m_gp_conn, GP_RECV_GAME_INVITE, s_game_invite, this );
	Dbg_Assert( result == GP_NO_ERROR );

	result = gpSetCallback( &m_gp_conn, GP_TRANSFER_CALLBACK, s_transfer_callback, this );
	Dbg_Assert( result == GP_NO_ERROR );

	m_buddy_logic_task = new Tsk::Task< BuddyMan > ( s_buddy_logic_code, *this );

	m_logged_in = false;
	m_state = vSTATE_WAIT;
	m_profile = 0;

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

BuddyMan::~BuddyMan( void )
{
	delete m_buddy_logic_task;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	BuddyMan::Connected( GPProfile profile )
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	m_logged_in = true;
	m_profile = profile;
	m_state = vSTATE_LOGGED_IN;

	//Dbg_Printf( "******************** PROFILE IS %d\n", m_profile );
	Script::RunScript( "profile_logged_in" );
    gpSetStatus( &m_gp_conn, GP_ONLINE, (char*) Script::GetString( "homie_status_online" ), 
										(char*) Script::GetString( "homie_status_logging_in" ));

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	BuddyMan::IsLoggedIn( void )
{
	return m_logged_in;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	BuddyMan::Connect( void )
{   
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();
	Thread::PerThreadStruct	net_thread_data;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());
	
	m_state = vSTATE_CONNECTING;
	s_connect_start_time = Tmr::GetTime();

	Script::RunScript( "log_in_profile" );

    net_thread_data.m_pEntry = s_threaded_buddy_connect;
	net_thread_data.m_iInitialPriority = vSOCKET_THREAD_PRIORITY;
	net_thread_data.m_pStackBase = gamenet_man->GetNetThreadStack();
	net_thread_data.m_iStackSize = vNET_THREAD_STACK_SIZE;
	net_thread_data.m_utid = 0x152;
	Thread::CreateThread( &net_thread_data );
	gamenet_man->SetNetThreadId( net_thread_data.m_osId );
	StartThread( gamenet_man->GetNetThreadId(), this );

	if( !m_buddy_logic_task->InList())
	{
		mlp_man->AddLogicTask( *m_buddy_logic_task );
	}
	
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	BuddyMan::CreateProfile( void )
{   
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();
	Thread::PerThreadStruct	net_thread_data;

	m_state = vSTATE_CREATING_PROFILE;
	s_connect_start_time = Tmr::GetTime();

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());
	
	net_thread_data.m_pEntry = s_threaded_buddy_create;
	net_thread_data.m_iInitialPriority = vSOCKET_THREAD_PRIORITY;
	net_thread_data.m_pStackBase = gamenet_man->GetNetThreadStack();
	net_thread_data.m_iStackSize = vNET_THREAD_STACK_SIZE;
	net_thread_data.m_utid = 0x152;
	Thread::CreateThread( &net_thread_data );
	gamenet_man->SetNetThreadId( net_thread_data.m_osId );
	StartThread( gamenet_man->GetNetThreadId(), this );

	mlp_man->AddLogicTask( *m_buddy_logic_task );

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	BuddyMan::Disconnect( void )
{   
	BuddyInfo* buddy, *next;
	Lst::Search< BuddyInfo > sh;
	
	//Dbg_Printf( "***** Disconnecting from presence system\n" );
	gpDisconnect( &m_gp_conn );
	m_buddy_logic_task->Remove();
	m_logged_in = false;

	/*SetStatusAndLocation( GP_OFFLINE, 	(char*) Script::GetString( "homie_status_offline" ), 
										"" );*/
	for( buddy = sh.FirstItem( m_buddies ); buddy; buddy = next )
	{
		next = sh.NextItem();
		delete buddy;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		BuddyMan::GetProfile( void )
{
	return m_profile;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	BuddyMan::IsAlreadyMyBuddy( int profile )
{
	BuddyInfo* buddy;
	Lst::Search< BuddyInfo > sh;
	
	for( buddy = sh.FirstItem( m_buddies ); buddy; buddy = sh.NextItem())
	{
		if( buddy->m_Profile == profile )
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

int		BuddyMan::NumBuddies( void )
{
	return m_buddies.CountItems();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	BuddyMan::SetStatusAndLocation( int status, char* status_string, char* location )
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());
	//Dbg_Printf( "Setting status to %d : %s , %s\n", status, status_string, location );
	gpSetStatus( &m_gp_conn, (GPEnum) status, status_string, location );
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	BuddyMan::ScriptSetUniqueId(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Script::CStruct* new_params;
	Prefs::Preferences*	prefs;
	Tmr::Time time;
	int ms_time;
	char str_time[32];

	time = Tmr::GetTime();
	ms_time = (int) time;

	//Dbg_Printf( "*************** (Getting Time) Setting unique id to %d\n", ms_time );
	
	prefs = gamenet_man->GetNetworkPreferences();
	new_params = new Script::CStruct;
	sprintf( str_time, "%d", ms_time );
	new_params->AddString( "ui_string", str_time );
	prefs->SetPreference( Script::GenerateCRC( "unique_id" ), new_params );
	delete new_params;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	BuddyMan::ScriptProfileLoggedIn(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	BuddyMan* buddy_man;

	buddy_man = gamenet_man->mpBuddyMan;
	return buddy_man->IsLoggedIn();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	BuddyMan::ScriptProfileLogIn(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Prefs::Preferences*	prefs;
	
	prefs = gamenet_man->GetNetworkPreferences();
	uint32 success = prefs->GetPreferenceChecksum( Script::GenerateCRC( "profile_success" ), Script::GenerateCRC( "checksum" ));
	if( success != Script::GenerateCRC( "boolean_true" ))
	{
		return false;
	}

	gamenet_man->mpBuddyMan->Connect();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	BuddyMan::ScriptProfileLogOff(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	BuddyMan* buddy_man;

	buddy_man = gamenet_man->mpBuddyMan;
	buddy_man->Disconnect();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	BuddyMan::ScriptCreateProfile(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	
	gamenet_man->mpBuddyMan->CreateProfile();
    
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	BuddyMan::ScriptFillBuddyList(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	bool clear_list, allow_join, allow_remove;

	clear_list = pParams->ContainsFlag( "clear_list" );
	allow_join = pParams->ContainsFlag( "allow_join" );
	allow_remove = pParams->ContainsFlag( "allow_remove" );

	//Dbg_Printf( "****** ScripFillBuddyList. Allow Join %d\n", (int) allow_join );
	gamenet_man->mpBuddyMan->fill_buddy_list( clear_list, allow_join, allow_remove );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	BuddyMan::ScriptFillProspectiveBuddyList(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	
	gamenet_man->mpBuddyMan->fill_prospective_buddy_list();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	BuddyMan::ScriptAddBuddy(Script::CStruct *pParams, Script::CScript *pScript)
{   
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	int profile;
	const char* nick;

	pParams->GetInteger( "profile", &profile, Script::ASSERT );
	pParams->GetString( "nick", &nick, Script::ASSERT );

	gamenet_man->mpBuddyMan->add_buddy((char *) nick, profile );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	BuddyMan::ScriptCancelAddBuddy(Script::CStruct *pParams, Script::CScript *pScript)
{                                                               
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
                                                                
	gamenet_man->mpBuddyMan->cancel_add_buddy();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	BuddyMan::ScriptRemoveBuddy(Script::CStruct *pParams, Script::CScript *pScript)
{
	int profile;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
                                                                
	pParams->GetInteger( "profile", &profile, Script::ASSERT );
	gamenet_man->mpBuddyMan->remove_buddy( profile );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	BuddyMan::ScriptJoinBuddy(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	char location[1024];
	int join_ip, join_private_ip, join_port;
	const char* location_str;
	bool observing, local;
	Script::CStruct* p_struct;

	observing = pParams->ContainsFlag( "observe" );
	pParams->GetString( "location", &location_str, Script::ASSERT );
	pParams->GetInteger( "join_ip", &join_ip, Script::ASSERT );
	pParams->GetInteger( "join_private_ip", &join_private_ip, Script::ASSERT );
	pParams->GetInteger( "join_port", &join_port, Script::ASSERT );
	sprintf( location, "%d:%d:%d:%s", join_ip, join_private_ip, join_port, location_str );

	local = false;
	if((unsigned int) join_ip == peerGetLocalIP( gamenet_man->mpLobbyMan->GetPeer()))
	{
		join_ip = join_private_ip;
		join_port = vHOST_PORT;
		local = true;
	}
	
	if( observing )
	{
		gamenet_man->SetJoinMode( vJOIN_MODE_OBSERVE );
		gamenet_man->mpBuddyMan->SetStatusAndLocation( GP_PLAYING, (char*) Script::GetString( "homie_status_observing" ), location );
	}
	else
	{
		gamenet_man->SetJoinMode( vJOIN_MODE_PLAY );
		gamenet_man->mpBuddyMan->SetStatusAndLocation( GP_PLAYING, (char*) Script::GetString( "homie_status_playing" ), location );
	}

	char *ip_addr;
	struct in_addr addr;
	uint32 cookie;
		
	p_struct = new Script::CStruct;

	addr.s_addr = join_ip;
	ip_addr = inet_ntoa( addr );
	p_struct->AddComponent( NONAME, ESYMBOLTYPE_INTEGER, join_port );
	p_struct->AddComponent( NONAME, ESYMBOLTYPE_STRING, ip_addr );
		
	if( !local )
	{
		cookie = Mth::Rnd( 65535 );
		p_struct->AddComponent( CRCD( 0x751f4599, "cookie" ), ESYMBOLTYPE_NAME, cookie );
	}
		
	Script::RunScript( "net_chosen_join_server", p_struct );
	delete p_struct;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	BuddyMan::ScriptHasBuddies(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	return ( gamenet_man->mpBuddyMan->NumBuddies() > 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	BuddyMan::ScriptBuddyListFull(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	return ( gamenet_man->mpBuddyMan->NumBuddies() >= vMAX_BUDDIES );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	BuddyMan::ScriptSetLobbyStatus(Script::CStruct *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	Dbg_Assert( gamenet_man->mpBuddyMan->IsLoggedIn());
	gamenet_man->mpBuddyMan->SetStatusAndLocation( GP_CHATTING, (char*) Script::GetString( "homie_status_chatting" ), 
																gamenet_man->mpLobbyMan->GetLobbyName());

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

BuddyInfo::BuddyInfo( void )
: Lst::Node< BuddyInfo > ( this )
{
	m_Pending = false;
	m_Status = 0;
	m_Location[0] = '\0';
	sprintf( m_StatusString, "Offline" );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace GameNet




