/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Skate3													**
**																			**
**	Module:			GameNet					 								**
**																			**
**	File name:		ServerList.cpp											**
**																			**
**	Created by:		06/131/01	-	spg										**
**																			**
**	Description:	Server List functionality								**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/thread.h>

#include <sys/mcman.h>

#include <gel/net/server/netserv.h>
#include <gel/net/client/netclnt.h>
#include <gel/mainloop.h>
#include <gel/music/music.h>

#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>

#include <sk/gamenet/gamenet.h>

#include <sk/modules/frontend/frontend.h>
//#include <sk/modules/frontend/mainmenu.h>

#ifdef __PLAT_NGPS__
#include <gamenet/lobby.h>
//#include <cengine/goaceng.h>
#include <qr2/qr2.h>
#include <ghttp/ghttp.h>
#include <pt/pt.h>
#include <sk/gamenet/ngps/p_buddy.h>
#endif

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

#ifndef __PLAT_XBOX__
#ifndef __PLAT_NGC__
// Gamespy frame counter
//extern unsigned long goa_frame_ct;

//extern "C" 
//{
//////////////////////////////////
// Global IP addresses for gamespy
//extern char	qr_hostname[64];
//extern char ServerListHostname[64];

//}

extern void GOASetUniqueID( const char* id );

#endif
#endif

namespace GameNet
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define vMOTD_WAIT_THRESHOLD		Tmr::Seconds( 7 )
#define vTIME_BETWEEN_AUTO_REFRESH	Tmr::Seconds( 3 )
#define vTIME_BETWEEN_MANUAL_REFRESH	Tmr::Seconds( 5 )

#define	vLAN_SERVER_LIST_TIMEOUT	500
#define	vHTTP_TIMEOUT				Tmr::Seconds( 15 )

/*****************************************************************************
**								Private Types								**
*****************************************************************************/
#ifdef __PLAT_NGPS__

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

static int			s_motd_retry = 0;
#endif // __PLAT_NGPS__

static Tmr::Time 	s_last_refresh_time = 0;
static bool 		s_refresh_pending = false;

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

bool	gEnteringFromMainMenu;

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

#ifdef __PLAT_NGPS__
//static bool s_no_servers_dialog_box_handler( Front::EDialogBoxResult result );

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

/*SortKeyInfo* GetSortKey( int key_type )
{
	int i;

	for( i = 0; i < vNUM_SORT_KEYS; i++ )
	{
		if( s_sort_keys[i].KeyType == key_type )
		{
			return &s_sort_keys[i];
		}
	}

	return NULL;
}*/
#endif // __PLAT_NGPS__

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#if 0
static bool not_posted_dialog_box_handler( Front::EDialogBoxResult result )
{
    Mdl::FrontEnd* front = Mdl::FrontEnd::Instance();

	front->SetActive( false );

	return false;
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::s_server_list_state_code( const Tsk::Task< Manager >& task )
{
#ifdef __PLAT_NGPS__
	Manager& man = task.GetData();

	switch( man.GetServerListState())
	{   
		case vSERVER_LIST_STATE_WAIT:
			// A wait time of zero indicates we should wait for an outside force to change the state
			if( man.m_server_list_wait_time == 0 )
			{
				break;
			}
			if( Tmr::GetTime() > man.m_server_list_wait_time )
			{
				man.SetServerListState( man.GetNextServerListState());
			}
			break;

		case vSERVER_LIST_STATE_RETRY_MOTD:
		{
			//Mdl::FrontEnd* front = Mdl::FrontEnd::Instance();
			//Front::DialogBox* dlg;

			//dlg = front->GetDialogBox();
			//dlg->GoBack();

			// fall-through intentional
		}
		case vSERVER_LIST_STATE_STARTING_MOTD:
		{
			Thread::PerThreadStruct	net_thread_data;
			
			//Dbg_Printf( "Serverlist State: Starting MotD\n" );

			// Pause the music & streams before doing DNS lookup
			Pcm::PauseStream( 1 );
			
            
			// First, show the dialog box
			if( s_motd_retry == 0 )
			{
				Script::SpawnScript( "launch_motd_wait_dialog" );
			}
			else
			{
				char msg[256];
				Script::CScriptStructure* pStructure;
			
				pStructure = new Script::CScriptStructure;
				sprintf( msg, "(%d) %s", 	s_motd_retry,
											Script::GetLocalString( "net_status_retry_motd" ));
				pStructure->AddComponent( Script::GenerateCRC( "message" ), ESYMBOLTYPE_STRING, msg );
				Script::SpawnScript( "CreateMotdRetryDialog", pStructure );
				delete pStructure;
			}

			man.SetServerListState( vSERVER_LIST_STATE_WAIT );
			man.m_server_list_wait_time = 0;

			// Now spawn the thread that gets the message of the day
			net_thread_data.m_pEntry = s_threaded_get_message_of_the_day;
			net_thread_data.m_iInitialPriority = vSOCKET_THREAD_PRIORITY;
			net_thread_data.m_pStackBase = man.GetNetThreadStack();
			net_thread_data.m_iStackSize = vNET_THREAD_STACK_SIZE;
			net_thread_data.m_utid = 0x151;
			Thread::CreateThread( &net_thread_data );
			man.SetNetThreadId( net_thread_data.m_osId );
			StartThread( man.GetNetThreadId(), &man );
			
			break;
		}
		case vSERVER_LIST_STATE_GETTING_MOTD:
		{
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());
			if(( Tmr::GetTime() - man.m_ghttp_start_time ) > vHTTP_TIMEOUT )
			{
				ghttpCancelRequest( man.m_ghttp_request );
				man.SetServerListState( vSERVER_LIST_STATE_FAILED_MOTD );
			}
			else
			{
				ghttpThink(); 
			}
			
			Mem::Manager::sHandle().PopContext();
			break;
		}
		case vSERVER_LIST_STATE_TRACK_USAGE:
		{
			Thread::PerThreadStruct	net_thread_data;
			
			ghttpCleanup();

			//Dbg_Printf( "Serverlist State: Starting track usage\n" );

			// Pause the music & streams before doing DNS lookup
			Pcm::PauseStream( 1 );
            
			// Now spawn the thread that gets the message of the day
			net_thread_data.m_pEntry = s_threaded_track_usage;
			net_thread_data.m_iInitialPriority = vSOCKET_THREAD_PRIORITY;
			net_thread_data.m_pStackBase = man.GetNetThreadStack();
			net_thread_data.m_iStackSize = vNET_THREAD_STACK_SIZE;
			net_thread_data.m_utid = 0x151;
			Thread::CreateThread( &net_thread_data );
			man.SetNetThreadId( net_thread_data.m_osId );
			StartThread( man.GetNetThreadId(), &man );
			break;
		}
		case vSERVER_LIST_STATE_TRACKING_USAGE:
		{
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());
			if(( Tmr::GetTime() - man.m_ghttp_start_time ) > vHTTP_TIMEOUT )
			{
				ghttpCancelRequest( man.m_ghttp_request );
				man.SetServerListState( vSERVER_LIST_STATE_SHOW_MOTD );
			}
			else
			{
				ghttpThink(); 
			}
			Mem::Manager::sHandle().PopContext();
			break;
		}
		case vSERVER_LIST_STATE_SHOW_MOTD:
		{
			Script::CScriptStructure* pStructure;
			
			//Dbg_Printf( "Serverlist State: Got MotD\n" );

			ghttpCleanup();

			man.RemoveMessageOfTheDay();

			pStructure = new Script::CScriptStructure;
			pStructure->AddComponent( Script::GenerateCRC( "message" ), ESYMBOLTYPE_STRING, man.m_motd );
			
			Script::RunScript( "create_motd_menu", pStructure );
			
			delete pStructure;
			
			man.SetServerListState( vSERVER_LIST_STATE_WAIT );
			man.m_server_list_wait_time = 0;
			break;
		}
		case vSERVER_LIST_STATE_FAILED_MOTD:
		{
			Dbg_Printf( "In vSERVER_LIST_STATE_FAILED_MOTD\n" );
			ghttpCleanup();
			Script::RunScript( "CreateMotdFailedDialog" );

			// NOTE: USED TO CALL THIS FROM THE "OK" DIALOG BOX HANDLER
			// gamenet_man->SetServerListState( vSERVER_LIST_STATE_STARTING_LOBBY_LIST )
						
			//man.SetServerListState( vSERVER_LIST_STATE_WAIT );
			man.SetServerListState( vSERVER_LIST_STATE_SHUTDOWN );
			man.m_server_list_wait_time = 0;
			break;
		}
		case vSERVER_LIST_STATE_STARTING_LOBBY_LIST:
		{
			// First, show the dialog box
			Script::RunScript( "CreateGettingLobbyListDialog" );

			// Free up the previous lobby list
			man.mpLobbyMan->StopLobbyList();
			
			// Now spawn the lobby list -- this function also changes the state
			man.mpLobbyMan->StartLobbyList();
			break;
		}
		case vSERVER_LIST_STATE_GETTING_LOBBY_LIST:
		{

			//Dbg_Printf( "Serverlist State: Getting Lobby List\n" );
            
            man.SetServerListState( vSERVER_LIST_STATE_WAIT );
			man.m_server_list_wait_time = 0;
			break;
		}
		case vSERVER_LIST_STATE_FAILED_LOBBY_LIST:
		{
			Script::CScriptStructure* pStructure;
			
			//Dbg_Printf( "Failed lobby list\n" );
			man.mpLobbyMan->StopLobbyList();
		
			pStructure = new Script::CScriptStructure;
			pStructure->AddComponent( Script::GenerateCRC( "message" ), ESYMBOLTYPE_STRING, Script::GetLocalString( "net_status_gamespy_no_connect" ));

			Script::RunScript( "CreateFailedLobbyListDialog", pStructure );

			delete pStructure;
					
			man.SetServerListState( vSERVER_LIST_STATE_SHUTDOWN );
			break;
		}
		case vSERVER_LIST_STATE_GOT_LOBBY_LIST:
		{
			// If we didn't get any lobbies, that either means the matchmaker's down or their
			// connection isn't valid
			if( man.mpLobbyMan->GetLobbyInfo( 0 ) == NULL )
			{
				Script::CScriptStructure* pStructure = new Script::CScriptStructure;
				
				pStructure->AddComponent( Script::GenerateCRC( "message" ), ESYMBOLTYPE_STRING, Script::GetLocalString( "net_status_gamespy_no_connect" ));

				Script::RunScript( "CreateFailedLobbyListDialog", pStructure );

				delete pStructure;
				
				man.SetServerListState( vSERVER_LIST_STATE_SHUTDOWN );
			}
			else
			{
				man.SetServerListState( vSERVER_LIST_STATE_FILL_LOBBY_LIST );
			}
			
			Pcm::PauseStream( 0 );
			break;
		}
		case vSERVER_LIST_STATE_FILL_LOBBY_LIST:
		{
			man.mpLobbyMan->FillLobbyList();
			man.SetServerListState( vSERVER_LIST_STATE_SHUTDOWN );
			break;
		}
		
		default:
			break;
	}
#endif

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::s_auto_refresh_code( const Tsk::Task< Manager >& task )
{
	Manager& man = task.GetData();
	static Tmr::Time s_refresh_time = 0;

	if(( Tmr::GetTime() - s_refresh_time ) > vTIME_BETWEEN_AUTO_REFRESH )
	{
		s_refresh_time = Tmr::GetTime();
#ifdef __PLAT_XBOX__
		man.RefreshServerList( true );
#endif
	}
	else
	{
		if( s_refresh_pending )
		{
			if(( Tmr::GetTime() - s_last_refresh_time ) > vLAN_SERVER_LIST_TIMEOUT )
			{
				Lst::Search< ServerInfo > sh;
				ServerInfo* server, *next;

				man.ClearServerList();
						
				for( server = sh.FirstItem( man.m_temp_servers ); server; server = next )
				{
					next = sh.NextItem();
					server->Remove();

					man.AddServerToMenu( server, man.m_servers.CountItems());
					man.m_servers.AddToTail( server );					
				}
				Script::RunScript( "update_lobby_server_list" );
				
				s_refresh_pending = false;
			}
		}
	}
}

/*****************************************************************************
**							  Handler Functions								**
*****************************************************************************/

/******************************************************************/
/* Respond to a client who is looking for games to join on the LAN*/
/* Tell them info about the game                                  */
/******************************************************************/

int Manager::s_handle_server_response( Net::MsgHandlerContext* context )
{
	MsgServerDescription* msg;
	ServerInfo *server_info;
	Manager* manager = (Manager*) context->m_Data;
	int i;

	//Dbg_Printf( "GOT SERVER RESPONSE!\n" );

	msg = (MsgServerDescription*) context->m_Msg;

	if( context->m_MsgLength != sizeof( MsgServerDescription ))
	{
		return Net::HANDLER_MSG_DONE;
	}
	
#ifdef	__PLAT_XBOX__
    if( manager->ServerAlreadyInList( msg->m_Name, &msg->m_XboxAddr ))
	{
		return Net::HANDLER_MSG_DONE;
	}
#else
	if( manager->ServerAlreadyInList( msg->m_Name, context->m_Conn->GetIP()))
	{
		return Net::HANDLER_MSG_DONE;
	}
#endif

	
#ifdef	__PLAT_XBOX__
	if( memcmp( msg->m_Nonce, context->m_App->m_Nonce, 8 ))
	{
		return Net::HANDLER_MSG_DONE;
	}
#endif

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());

	server_info = new ServerInfo;
	
	strcpy( server_info->m_Name, msg->m_Name );
	server_info->m_Ip = context->m_Conn->GetIP();
	server_info->m_Latency = Tmr::GetTime() - msg->m_Timestamp;
	strcpy( server_info->m_Level, msg->m_Level );
	strcpy( server_info->m_Mode, msg->m_Mode );
	server_info->m_MaxPlayers = msg->m_MaxPlayers;
	server_info->m_NumPlayers = msg->m_NumPlayers;
	server_info->m_MaxObservers = msg->m_MaxObservers;
	server_info->m_NumObservers = msg->m_NumObservers;
	server_info->m_Password = msg->m_Password;
	server_info->m_GameStarted = msg->m_GameStarted;
	server_info->m_HostMode = msg->m_HostMode;
	server_info->m_Ranked = msg->m_Ranked;
	server_info->m_SkillLevel = msg->m_SkillLevel;
	server_info->m_Port = vHOST_PORT;
#ifdef __PLAT_XBOX__
	memcpy( &server_info->m_XboxKeyId, &msg->m_XboxKeyId, sizeof( XNKID ));
	memcpy( &server_info->m_XboxKey, &msg->m_XboxKey, sizeof( XNKEY ));	
	memcpy( &server_info->m_XboxAddr, &msg->m_XboxAddr, sizeof( XNADDR ));	
#endif    
    
	for( i = 0; i < server_info->m_NumPlayers; i++ )
	{
		server_info->AddPlayer( msg->m_PlayerNames[i] );
	}

	//manager->AddServerToMenu( server_info, manager->m_servers.CountItems());

	//manager->m_servers.AddToTail( server_info );
	manager->m_temp_servers.AddToTail( server_info );
    
	Mem::Manager::sHandle().PopContext();

	return Net::HANDLER_CONTINUE;
}

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
#ifdef __PLAT_NGPS__

void	Manager::s_create_game_callback( PEER peer, PEERBool success, PEERJoinResult result, RoomType roomType, 
											void * param )
{
	if( success )
	{
		//Dbg_Printf( "Joined Created Room\n" );
	}
	else
	{
		switch( result )
		{
			case PEERFullRoom:
				//Dbg_Printf( "Couldnt join created room. Full\n" );
				break;
			case PEERInviteOnlyRoom:
				//Dbg_Printf( "Couldnt join created room. Invite Only\n" );
				break;
			case PEERBannedFromRoom:
				//Dbg_Printf( "Couldnt join created room. Banned\n" );
				break;
			case PEERBadPassword:
				//Dbg_Printf( "Couldnt join created room. Bad Password\n" );
				break;
			case PEERAlreadyInRoom:
				//Dbg_Printf( "Couldnt join created room. Already in room\n" );
				break;
			case PEERNoTitleSet:
				//Dbg_Printf( "Couldnt join created room. No Title Set\n" );
				break;
			default:
				//Dbg_Printf( "Couldnt join created room. General error\n" );
				break;
		}
	}
}

#endif // __PLAT_NGPS__
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::PostGame( void )
{
#ifdef __PLAT_NGPS__
	Net::Manager * net_man = Net::Manager::Instance();
	
    Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());
	
	mpLobbyMan->StartReportingGame();
	if( mpBuddyMan->IsLoggedIn())
	{
		char location[1024];
		const char *server_name;
		Script::CScriptStructure* pStructure;
		Prefs::Preferences* pPreferences;

		pPreferences = GetNetworkPreferences();
		pStructure = pPreferences->GetPreference( Script::GenerateCRC("server_name") );
		pStructure->GetText( "ui_string", &server_name, true );

		if( mpLobbyMan->GetPeer())
		{
			sprintf( location, "%d:%d:%d:%s (%s)", net_man->GetPublicIP(), peerGetPrivateIP( mpLobbyMan->GetPeer()), vHOST_PORT, server_name, mpLobbyMan->GetLobbyName());
		}
		mpBuddyMan->SetStatusAndLocation( GP_PLAYING, (char*) Script::GetString( "homie_status_hosting" ), location );
	}
	Mem::Manager::sHandle().PopContext();
#endif
}


/*void Manager::RequestMatchmakerConnect( void )
{
	Mlp::Manager * mlp_manager = Mlp::Manager::Instance();

#ifdef __PLAT_NGPS__
	int result;

	m_server->SetForeignPacketHandler( gamespy_data_handler );
	s_gamespy_parse_data[0] = '\0';

	// Hardcode the IP of the master server until we get the message of the day squared away
	result = qr_init_socket( NULL, m_server->GetSocket(), "thps4ps2", "H2r8W1", basic_callback,
							 info_callback, rules_callback, players_callback, this );
	// Uncomment for media burns
	//result = qr_init_socket( NULL, m_server->GetSocket(), "thps3media", "tRKg39", basic_callback,
							 //info_callback, rules_callback, players_callback, this );
	 
	switch( result )
	{
		case 0:
			// success:
			break;
		case E_GOA_WSOCKERROR:
			Dbg_MsgAssert( 0, ( "Failed to initialize GameSpy query report toolkit due to Socket Error\n" ));
			break;
		case E_GOA_BINDERROR:
			Dbg_MsgAssert( 0, ( "Failed to initialize GameSpy query report toolkit due to Bind Error\n" ));
			break;
		case E_GOA_CONNERROR:
			Dbg_MsgAssert( 0, ( "Failed to initialize GameSpy query report toolkit due to Connection Error\n" ));
			break;
		default:
			Dbg_MsgAssert( 0, ( "Failed to initialize Gamespy query report toolkit\n" ));
			break;
	}

#endif
	
#ifdef __PLAT_NGPS__
	s_game_start_time = Tmr::GetTime();
	s_notified_user_not_connected = false;
	s_retried_game_post = false;
	gGotGamespyCallback = false;
#endif		// __PLAT_NGC__
	mlp_manager->AddLogicTask( *m_process_gamespy_queries_task );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetServerFocus( ServerInfo* server )
{
	ClearServerFocus();
	server->m_InFocus = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::ClearServerFocus( void )
{
	Lst::Search< ServerInfo > sh;
	ServerInfo* server;

	for( server = sh.FirstItem( m_servers ); server; server = sh.NextItem())
	{
		server->m_InFocus = false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

ServerInfo*		Manager::GetServerFocus( void )
{
	Lst::Search< ServerInfo > sh;
	ServerInfo* server;

	for( server = sh.FirstItem( m_servers ); server; server = sh.NextItem())
	{
		if( server->m_InFocus )
		{
			return server;
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	Manager::NumServersListed( void )
{
	Lst::Search< ServerInfo > sh;
	ServerInfo* server;
	int num_listed;

	num_listed = 0;
	for( server = sh.FirstItem( m_servers ); server; server = sh.NextItem())
	{
		if( server->m_Listed )
		{
			num_listed++;
		}
	}

	return num_listed;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::AddServerToMenu( ServerInfo* server, int index )
{   
	Script::CStruct* p_item_params;
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
	uint32 server_id;
		
	// If this is our first listed server, destroy the "no servers found" message
	if( NumServersListed() == 0 )
	{
		Script::RunScript( "destroy_server_menu_children" );
	}

	Script::RunScript( "prepare_server_menu_for_new_children" );
		
	p_item_params = new Script::CStruct;	
	p_item_params->AddString( "text", server->m_Name );
	p_item_params->AddString( "mode", server->m_Mode );
	printf("The game mode is: %s\n", server->m_Mode );
#ifdef __PLAT_NGPS__
	if( InInternetMode())
	{
		server_id = (uint32) server->m_GServer;
		
	}
	else
#endif
	{
		server_id = (uint32) server;
	}
	
	p_item_params->AddChecksum( "id", server_id );
	p_item_params->AddChecksum( "parent", Script::GenerateCRC( "server_list_menu" ));
	p_item_params->AddChecksum( "pad_choose_script",Script::GenerateCRC("choose_selected_server"));
	p_item_params->AddChecksum( "focus_script",Script::GenerateCRC("describe_selected_server"));

	// create the parameters that are passed to the X script
	Script::CStruct *p_script_params= new Script::CStruct;
	p_script_params->AddChecksum( "id", server_id );	
	p_item_params->AddStructure("pad_choose_params",p_script_params);			
	p_item_params->AddStructure("focus_params",p_script_params);

	Script::RunScript("server_list_menu_add_item",p_item_params);
	
	server->m_Listed = true;

	delete p_item_params;
	delete p_script_params;

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
#ifdef __PLAT_NGPS__

GHTTPBool	Manager::MotdComplete( GHTTPRequest request, GHTTPResult result, char* buffer, int buffer_len, 
						  void* param )
{
	Manager * gamenet_man = Manager::Instance();
	bool succeeded;

	
    
	succeeded = false;
	if( result == GHTTPSuccess )
	{   
		char* token;
		char* message, *ip_ptr;
		char temp_motd[4096];
		int server_ip;

		strncpy( temp_motd, buffer, 4095 );
		temp_motd[4095] = '\0';
		//Dbg_Printf( "Got Message of the day: %s\n", temp_motd );
		
		// Checksum comes first and is delimited by a space
		token = strtok( temp_motd, " " );
		if( token )
		{
		 	uint32 checksum;

			checksum = atoi( token );
			//Dbg_Printf( "checksum is %d\n", checksum );
			ip_ptr = token + strlen( token ) + 1;	// skip the checksum
			if( checksum == Script::GenerateCRC( ip_ptr ))
			{
				//Dbg_Printf( "Matched!\n" );
				// Next token is the IP address for the gamespy servers
				ip_ptr = strtok( NULL, " " );
				if( ip_ptr )
				{
					//Dbg_Printf( "Got an IP Pointer of %s!\n", ip_ptr );
					message = strtok( NULL, "\n" );
					if( message )
					{
						//Dbg_Printf( "Got a message of %s\n", message );
						strncpy( gamenet_man->m_motd, message, 1023 );
						gamenet_man->m_motd[1023] = '\0';

						server_ip = atoi( ip_ptr );
						
                        //sprintf( ServerListHostname, "%s", inet_ntoa(*(struct in_addr*) &server_ip ));
						//sprintf( qr_hostname, "%s", inet_ntoa(*(struct in_addr*) &server_ip ));

						//Dbg_Printf( "GameSpy Server is %s\n", ServerListHostname );
						succeeded = true;
					}
				}
			}
		}

		if( succeeded )
		{   
			gamenet_man->m_got_motd = true;
			gamenet_man->SetServerListState( vSERVER_LIST_STATE_TRACK_USAGE );
		}
	}

	if( !succeeded )
	{
		Dbg_Printf( "Error Downloading motd: %d\n", result );

		// No retries in THPS4. We're just going to use our neversoft website and that's it
		/*s_motd_retry = s_motd_retry + 1;
		if( gamenet_man->m_master_servers[ s_motd_retry ][0] != '\0' )
		{
			gamenet_man->SetServerListState( vSERVER_LIST_STATE_RETRY_MOTD );
			return GHTTPTrue;
		}*/

		gamenet_man->SetServerListState( vSERVER_LIST_STATE_FAILED_MOTD );
	}

	return GHTTPTrue;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void MotdProgress( GHTTPRequest request, GHTTPState state, const char * buffer, int bufferLen,
					int bytesReceived, int totalSize, void * param )
{
	

	switch( state )
	{
		case GHTTPHostLookup:
			Dbg_Printf( "Lookin up HTTP host...\n" );
			break;
		case GHTTPConnecting:
			Dbg_Printf( "Connecting to HTTP host...\n" );
			break;
		case GHTTPSendingRequest:
			Dbg_Printf( "Sending Request to HTTP host...\n" );
			break;
		case GHTTPPosting:
			break;
		case GHTTPWaiting:
			Dbg_Printf( "Waiting for response from HTTP host...\n" );
			break;
		case GHTTPReceivingStatus:
			Dbg_Printf( "Receiving status....\n" );
			break;
		case GHTTPReceivingHeaders:
			Dbg_Printf( "Receiving headers....\n" );
			break;
		case GHTTPReceivingFile:
			Dbg_Printf( "Receiving file: %d bytes....\n", bytesReceived );
			break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

GHTTPBool	Manager::TrackUsageComplete( GHTTPRequest request, GHTTPResult result, char* buffer, int buffer_len, 
						  void* param )
{
	Manager * gamenet_man = Manager::Instance();

	//Dbg_Printf( "Track usage complete!!!\n" );

	gamenet_man->SetServerListState( vSERVER_LIST_STATE_SHOW_MOTD );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Manager::s_threaded_get_message_of_the_day( Manager* gamenet_man )
{
	char motd_path[256];

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

	// Register this thread with the sockets API
	sockAPIregthr();

	// Wait a bit before starting the http transfer 
	msleep( 100 );

	gamenet_man->SetServerListState( vSERVER_LIST_STATE_GETTING_MOTD );
	
	gamenet_man->m_ghttp_start_time = Tmr::GetTime();
	sprintf( motd_path, "http://www.thugonline.com/motd.txt" );
	//sprintf( motd_path, "http://www.neversoft.com/thps5/motd.txt" );
	gamenet_man->m_ghttp_request = ghttpGetEx( 	motd_path,
												NULL,
												gamenet_man->m_motd,
												1024,
												NULL,
												GHTTPFalse,
												GHTTPFalse,
												MotdProgress,
												MotdComplete,
												gamenet_man );

	// Deregister this thread with the sockets API
	sockAPIderegthr();
		
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Manager::s_threaded_track_usage( Manager* gamenet_man )
{
	Prefs::Preferences*	prefs;
	Script::CStruct* pStructure;
	const char* unique_id = "123456789";

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

	// Register this thread with the sockets API
	sockAPIregthr();
        
	prefs = gamenet_man->GetNetworkPreferences();
	pStructure = prefs->GetPreference( Script::GenerateCRC("unique_id"));
	pStructure->GetString( "ui_string", &unique_id );
	//GOASetUniqueID( unique_id );

	gamenet_man->m_ghttp_start_time = Tmr::GetTime();
	gamenet_man->m_ghttp_request = ptTrackUsage( 	0,	// int userID ( if not using Presence & Messaging, send zero )
													vGAMESPY_PRODUCT_ID,	// int productID,
													"1.0",	//const char * versionUniqueID,
													0,	//int distributionID,
													GHTTPFalse,
													TrackUsageComplete );	//PTBool blocking

	gamenet_man->SetServerListState( vSERVER_LIST_STATE_TRACKING_USAGE );

	// Deregister this thread with the sockets API
	sockAPIderegthr();
		
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::RemoveMessageOfTheDay( void )
{
	//ghttpCleanup();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::s_server_list_callback( PEER peer, PEERBool success, const char * name, SBServer server, PEERBool staging, 
									  int msg, int progress, void * param )
{
	Manager* man = (Manager*) param;
	if( success )
	{
		switch( msg )
		{
		
			case PEER_ADD:
			{
				ServerInfo* server_info;
				int i;
				
				Dbg_Printf( "****** ADDING SERVER %p : %s\n", server, name );
				Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());
	
				server_info = new ServerInfo;
				server_info->m_GServer = server;
				strncpy( server_info->m_Name, name, vMAX_SERVER_NAME_LEN );
				server_info->m_Name[vMAX_SERVER_NAME_LEN] = '\0';
				server_info->m_Latency = SBServerGetPing( server );

				// Check if we're behind the same NAT
                if( SBServerHasPrivateAddress( server ))
				{
					// If so, we'll have the same public IP
                    if( SBServerGetPublicInetAddress( server ) == peerGetLocalIP( peer ))
					{
						server_info->m_Ip = SBServerGetPrivateInetAddress( server );						
						server_info->m_Port = vHOST_PORT;
					}
					else
					{
						// If we're not behind the same NAT, check if they are behind
						// a promiscuous NAT or, perhaps, not behind a NAT at all.  If so
						// we can directly connect to their public IP
						if( SBServerDirectConnect( server ))
						{
							server_info->m_Ip = SBServerGetPublicInetAddress( server );
							server_info->m_Port = SBServerGetPublicQueryPort( server );
						}
						else
						{
							server_info->m_Ip = SBServerGetPublicInetAddress( server );
							server_info->m_Port = SBServerGetPublicQueryPort( server );
							server_info->m_CanDirectConnect = false;
							server_info->m_Latency = 9999;	// We can't get the ping of servers behind non-promiscuous NATs
						}
					}
				}
				else
				{
					if( SBServerDirectConnect( server ))
					{
						server_info->m_Ip = SBServerGetPublicInetAddress( server );
						server_info->m_Port = SBServerGetPublicQueryPort( server );
					}
					else
					{
						server_info->m_Ip = SBServerGetPublicInetAddress( server );
						server_info->m_Port = SBServerGetPublicQueryPort( server );
						server_info->m_CanDirectConnect = false;
						server_info->m_Latency = 9999;	// We can't get the ping of servers behind non-promiscuous NATs
					}
				}
				
				Dbg_Printf( "Percent Done: %d : Server %s ping (%d)\n", (int) progress, server_info->m_Name, server_info->m_Latency );
				strcpy( server_info->m_Level, SBServerGetStringValue( server, "mapname","..."));
				strcpy( server_info->m_Mode, SBServerGetStringValue( server, "gametype","..."));
				server_info->m_NumPlayers = SBServerGetIntValue( server, "numplayers", 1 );
				server_info->m_MaxPlayers = SBServerGetIntValue( server, "maxplayers", 8 );
				server_info->m_NumObservers = SBServerGetIntValue( server, "numobservers", 0 );
				server_info->m_MaxObservers = SBServerGetIntValue( server, "maxobservers", 1 );
				server_info->m_Password = SBServerGetBoolValue( server, "password", false );
				server_info->m_GameStarted = SBServerGetBoolValue( server, "started", false );
				server_info->m_HostMode = SBServerGetIntValue( server, "hostmode", vHOST_MODE_SERVE );
				server_info->m_Ranked = SBServerGetIntValue( server, "ranked", 0 );
				server_info->m_SkillLevel = SBServerGetIntValue( server, "skilllevel", vSKILL_LEVEL_DEFAULT );

				Dbg_Printf( "Numplayers %d\n", server_info->m_NumPlayers )
				for( i = 0; i < server_info->m_NumPlayers; i++ )
				{
					int rating;

					rating = SBServerGetPlayerIntValue( server, i, "rating", 0 );
					Dbg_Printf( "Adding player %s with rating %d\n", (char*) SBServerGetPlayerStringValue( server, i, "player","..." ), rating );
					server_info->AddPlayer((char*) SBServerGetPlayerStringValue( server, i, "player","..." ), rating );
				}
				
				man->m_servers.AddToTail( server_info );
				if( SBServerHasBasicKeys( server ))
				{
					man->AddServerToMenu( server_info, man->m_servers.CountItems() - 1 );
					Script::RunScript( "update_lobby_server_list" );
					server_info->m_HasBasicInfo = true;
				}

				Mem::Manager::sHandle().PopContext();
				break;
			}
	
			//   Remove this server from the list.  The server object for this server
			//   should not be used again after this callback returns.
			case PEER_REMOVE:
			{
				Lst::Search< ServerInfo > sh;
				ServerInfo* server_info, *next;
				Script::CStruct* pParams;
				
				Dbg_Printf( "****** REMOVING SERVER %p\n", server );

				pParams = new Script::CStruct;
				pParams->AddChecksum( "server_id", (uint32) server );
				Script::RunScript( "destroy_lobby_server", pParams );
				delete pParams;
	    
				for( server_info = sh.FirstItem( man->m_servers ); server_info; server_info = next )
				{
					next = sh.NextItem();
					if( server_info->m_GServer == server )
					{
						delete server_info;
						break;
					}
				}

				Script::RunScript( "update_lobby_server_list" );
                break;
			}

			//   Clear the list.  This has the same effect as if this was called
			//   with PEER_REMOVE for every server listed.
			case PEER_CLEAR:
			{
				Dbg_Printf( "****** CLEAR SERVER LIST\n" );

				man->ClearServerList( false );
				man->FreeServerList();

				Script::RunScript( "update_lobby_server_list" );
				break;
			}
			//   This server is already on the list, and its been updated.
			case PEER_UPDATE:
			{
				Lst::Search< ServerInfo > sh;
				ServerInfo* server_info;
				int i;
				
				if( SBServerHasFullKeys( server ))
				{
					Dbg_Printf( "****** FULLY UPDATING SERVER %p\n", server );
				}
				else
				{
					Dbg_Printf( "****** PARTIALLY UPDATING SERVER %p\n", server );
				}
				
	    
				for( server_info = sh.FirstItem( man->m_servers ); server_info; server_info = sh.NextItem())
				{
					if( server_info->m_GServer == server )
					{
						strncpy( server_info->m_Name, name, vMAX_SERVER_NAME_LEN );
						server_info->m_Name[vMAX_SERVER_NAME_LEN] = '\0';
						//server_info->m_Ip = SBServerGetPublicInetAddress( server );
						if( server_info->m_CanDirectConnect )
						{
							server_info->m_Latency = SBServerGetPing( server );
						}
						Dbg_Printf( "Percent Done: %d : Server %s ping (%d)\n", (int) progress, server_info->m_Name, SBServerGetPing(server));
						strcpy( server_info->m_Level, SBServerGetStringValue( server, "mapname","..."));
						strcpy( server_info->m_Mode, SBServerGetStringValue( server, "gametype","..."));
						server_info->m_NumPlayers = SBServerGetIntValue( server, "numplayers", 1 );
						server_info->m_MaxPlayers = SBServerGetIntValue( server, "maxplayers", 8 );
						server_info->m_NumObservers = SBServerGetIntValue( server, "numobservers", 0 );
						server_info->m_MaxObservers = SBServerGetIntValue( server, "maxobservers", 1 );
						server_info->m_Password = SBServerGetBoolValue( server, "password", false );
						server_info->m_GameStarted = SBServerGetBoolValue( server, "started", false );
						server_info->m_HostMode = SBServerGetIntValue( server, "hostmode", vHOST_MODE_SERVE );
						server_info->m_Ranked = SBServerGetIntValue( server, "ranked", 0 );
						server_info->m_SkillLevel = SBServerGetIntValue( server, "skilllevel", vSKILL_LEVEL_DEFAULT );
						//server_info->m_Port = SBServerGetPublicQueryPort( server );
						server_info->ClearPlayerNames();
						Dbg_Printf( "Game Mode: %s\n", server_info->m_Mode );
						Dbg_Printf( "Numplayers %d\n", server_info->m_NumPlayers )
						for( i = 0; i < server_info->m_NumPlayers; i++ )
						{
							int rating;

							rating = SBServerGetPlayerIntValue( server, i, "rating", 0 );
							Dbg_Printf( "Adding player %s with rating %d\n", (char*) SBServerGetPlayerStringValue( server, i, "player","..." ), rating );
							server_info->AddPlayer((char*) SBServerGetPlayerStringValue( server, i, "player","..." ), rating );
						}
						
						if( server_info->m_HasBasicInfo == false )
						{
							if( SBServerHasBasicKeys( server ))
							{
								man->AddServerToMenu( server_info, 0 );
								Script::RunScript( "update_lobby_server_list" );
								server_info->m_HasBasicInfo = true;
							}
						}

						if( server_info->m_HasFullInfo == false )
						{
							if( SBServerHasFullKeys( server ))
							{
								server_info->m_HasFullInfo = true;
								if( server_info->m_InFocus )
								{
									Script::CStruct* params;

									params = new Script::CStruct;
									params->AddChecksum( "id", (uint32) server );
									Script::RunScript( "describe_selected_server", params );
									delete params;
								}
							}
						}
						break;
					}
				}
                break;
			}
		}
	}
}

#endif 

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptGetNumServersInLobby(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	Script::CStruct* p_return_params;

	p_return_params = pScript->GetParams();
	p_return_params->AddInteger( "num_servers", gamenet_man->m_servers.CountItems());

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::StartServerList( void )
{
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();

	if( InLanMode())
	{
		Net::Client* client;

		MatchClientShutdown();

		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());

		client = SpawnMatchClient();

		Mem::Manager::sHandle().PopContext();

		if( client == NULL )
		{
			return false;
		}

        if( !m_auto_refresh_task->InList())
		{
			mlp_man->AddLogicTask( *m_auto_refresh_task );
			return true;
		}
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::StopServerList( void )
{
	

	if( InInternetMode())
	{   
#ifdef __PLAT_NGPS__
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());
		Dbg_Printf( "******** STOP LISTING GAMES **********\n" );
		if( mpLobbyMan->GetPeer())
		{
			peerStopListingGames( mpLobbyMan->GetPeer());
		}
		Mem::Manager::sHandle().PopContext();
#endif
	}
	else
	{
		Net::Client* client;

		client = GetMatchClient();
		if( client )
		{
			client->m_Dispatcher.Deinit();
		}

        m_auto_refresh_task->Remove();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::FreeServerList( void )
{
	Lst::Search< ServerInfo > sh;
	ServerInfo* server, *next;
	    
	for( server = sh.FirstItem( m_temp_servers ); server; server = next )
	{
		next = sh.NextItem();
		delete server;
	}

	for( server = sh.FirstItem( m_servers ); server; server = next )
	{
		next = sh.NextItem();
		delete server;
	}

	if( InLanMode())
	{
		MatchClientShutdown();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::ClearServerList( bool refocus )
{   
	Script::CStruct* pParams;
	
	pParams = new Script::CStruct;
	if( refocus )
	{
		pParams->AddChecksum( "refocus", Script::GenerateCRC( "refocus" ));
	}
	Script::RunScript( "destroy_server_menu_children", pParams );
	Script::RunScript( "add_no_servers_found_message", pParams );
	delete pParams;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::RefreshServerList( bool force_refresh )
{
	//Dbg_Printf( "In Refresh server list\n" );
	if( force_refresh == false )
	{
		//Dbg_Printf( "No force\n" );
		// Don't let people refresh like crazy
		if(( Tmr::GetTime() - s_last_refresh_time ) < vTIME_BETWEEN_MANUAL_REFRESH )
		{
			//Dbg_Printf( "Not enough time\n" );
			return;
		}
	}

	s_last_refresh_time = Tmr::GetTime();
	s_refresh_pending = true;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());

	StopServerList();
	FreeServerList();
	StartServerList();

	if( InInternetMode())
	{
#ifdef __PLAT_NGPS__
		unsigned char key_list[] = {NUMPLAYERS_KEY,
									MAXPLAYERS_KEY,
									HOSTPORT_KEY,
									GAMETYPE_KEY };

		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());
		
		Dbg_Printf( "******** START LISTING GAMES **********\n" );
		peerStartListingGames( mpLobbyMan->GetPeer(), key_list, sizeof(key_list), NULL, 
								s_server_list_callback, this );
		Mem::Manager::sHandle().PopContext();
#endif
	}
	else
	{   
		FindServersOnLAN();
	}
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#ifdef __PLAT_XBOX__
bool		Manager::ServerAlreadyInList( char* name, XNADDR* xbox_addr )
{
	Lst::Search< ServerInfo > sh;
	ServerInfo* server;

	for( server = sh.FirstItem( m_temp_servers ); server; server = sh.NextItem())
	{
		if(	( memcmp( &server->m_XboxAddr, xbox_addr, sizeof( XNADDR ) == 0 )) &&
			( stricmp( server->m_Name, name ) == 0 ))
		{
			return true;
		}
	}
	return false;
}

#else

bool		Manager::ServerAlreadyInList( char* name, int ip )
{
	Lst::Search< ServerInfo > sh;
	ServerInfo* server;

	for( server = sh.FirstItem( m_servers ); server; server = sh.NextItem())
	{
		if(	( server->m_Ip == ip ) &&
			( stricmp( server->m_Name, name ) == 0 ))
		{
			return true;
		}
	}

	for( server = sh.FirstItem( m_temp_servers ); server; server = sh.NextItem())
	{
		if(	( server->m_Ip == ip ) &&
			( stricmp( server->m_Name, name ) == 0 ))
		{
			return true;
		}
	}

	
	return false;
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Manager::SortPreviousKey( void )
{
	if( m_sort_key == 0 )
	{
		m_sort_key = vNUM_SORT_KEYS - 1;
	}
	else
	{
		m_sort_key--;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Manager::SortNextKey( void )
{
	m_sort_key = ( m_sort_key + 1 ) % vNUM_SORT_KEYS;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Manager::SortServerList( void )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

ServerInfo*	Manager::GetServerInfo( uint32 id )
{
	Lst::Search< ServerInfo > sh;
	ServerInfo* server;

	for( server = sh.FirstItem( m_servers ); server; server = sh.NextItem())
	{
#ifdef __PLAT_NGPS__
		if( InInternetMode())
		{
			if( id == (uint32) server->m_GServer )
			{
				return server;
			}
		}
		else
#endif
		{
			if( id == (uint32) server )
		{
			return server;
		}
		}
	}
	
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

ServerInfo::ServerInfo( void ) : Lst::Node< ServerInfo > ( this )
{
	m_Listed = false;
	m_InFocus = false;
	m_Latency = 0;
	sprintf( m_Level, "None" );
	sprintf( m_Mode, "None" );
	m_MaxPlayers = 8;
	m_NumPlayers = 1;
	m_MaxObservers = 1;
	m_NumObservers = 0;
	m_SkillLevel = vSKILL_LEVEL_DEFAULT;
	m_HostMode = vHOST_MODE_SERVE;
	m_Ranked = false;
	m_GameStarted = false;
	m_CanDirectConnect = true;
	m_HasBasicInfo = false;
	m_HasFullInfo = false;
#ifdef __PLAT_NGPS__
	m_GServer = NULL;
#endif
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void ServerInfo::ClearPlayerNames( void )
{
	Lst::Search< PlayerInfo > sh;
	PlayerInfo* player, *next;
	Manager * gamenet_man = Manager::Instance();

	for( player = sh.FirstItem( m_players ); player; player = next )
	{
		next = sh.NextItem();
		gamenet_man->DestroyPlayer( player );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

ServerInfo::~ServerInfo( void )
{
	ClearPlayerNames();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	ServerInfo::AddPlayer( char* name, int rating )
{   
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	PlayerInfo* player;

	if( gamenet_man->InInternetMode())
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());
	}
	else
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());
	}
	
	player = new PlayerInfo( 0 );
	player->m_Rating = rating;
	strncpy( player->m_Name, name, vMAX_PLAYER_NAME_LEN );
	player->m_Name[ vMAX_PLAYER_NAME_LEN ] = '\0';
	m_players.AddToTail( player );
	Mem::Manager::sHandle().PopContext();
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*	ServerInfo::GetPlayerName( int index )
{
	Lst::Search< PlayerInfo > sh;
	PlayerInfo* player;

	for( player = sh.FirstItem( m_players ); player; player = sh.NextItem())
	{
		if( index == 0 )
		{
			return player->m_Name;
		}
		index--;
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		ServerInfo::GetPlayerRating( int index )
{
	Lst::Search< PlayerInfo > sh;
	PlayerInfo* player;

	for( player = sh.FirstItem( m_players ); player; player = sh.NextItem())
	{
		if( index == 0 )
		{
			return player->m_Rating;
		}
		index--;
	}

	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetServerListState( ServerListState state )
{
	//Dbg_Printf( "Setting server list state to %d from %d\n", state, m_server_list_state );
	m_server_list_state = state;
	
	switch( m_server_list_state )
	{
		case vSERVER_LIST_STATE_INITIALIZE:
		{
			Mlp::Manager * mlp_man = Mlp::Manager::Instance();
			
			//Dbg_Printf( "Serverlist State: Initializing\n" );

			mlp_man->AddLogicTask( *m_server_list_state_task );
			SetServerListState( GetNextServerListState());
#ifdef __PLAT_NGPS__
			s_motd_retry = 0;
#endif // __PLAT_NGPS__
			break;
		}
		
		case vSERVER_LIST_STATE_SHUTDOWN:
		{
			//Dbg_Printf( "Serverlist State: Shutting down\n" );

			m_server_list_state_task->Remove();
			break;
		}
		
		default:
			break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetNextServerListState( ServerListState state )
{
	m_next_server_list_state = state;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

ServerListState Manager::GetServerListState( void )
{
	return m_server_list_state;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

ServerListState Manager::GetNextServerListState( void )
{
	return m_next_server_list_state;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		Manager::ScriptRetrieveServerInfo(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager* gamenet_man = Manager::Instance();

	if( gamenet_man->InLanMode())
	{
		// We have the data and can display it immediately
		return true;
	}
	else
	{
#ifdef __PLAT_NGPS__
		uint32 id;
		ServerInfo* p_server;

		pParams->GetChecksum( Script::GenerateCRC("id"), &id );

		p_server = gamenet_man->GetServerInfo( id );
		Dbg_Assert( p_server );

		gamenet_man->SetServerFocus( p_server );
		if( p_server->m_HasFullInfo )
		{
			// We have the data and can display it immediately
			return true;
		}
		else
		{
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

			peerUpdateGame( gamenet_man->mpLobbyMan->GetPeer(), p_server->m_GServer, PEERTrue );

			Mem::Manager::sHandle().PopContext();

			// We have to request it from the server
			return false;
		}
#else
		return false;
#endif
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		Manager::ScriptDescribeServer(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Script::CStruct* p_item_params;
	ServerInfo* p_server;
	Manager* gamenet_man = Manager::Instance();
	int i;
	char p_string[128];
	Script::CArray* p_skill_array;
	Script::CScriptStructure* p_skill_struct;
	const char* p_skill_string;
	uint32 id;

	pParams->GetChecksum( Script::GenerateCRC("id"), &id );
				
	p_server = gamenet_man->GetServerInfo( id );
	Dbg_Assert( p_server );

	Script::RunScript( "destroy_server_desc_children" );

    Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());

	if( p_server->m_GameStarted )
	{
        // In Progress
        p_item_params = new Script::CStruct;
        sprintf( p_string, "%s:", Script::GetLocalString( "netoptions_str_game_in_progress" ) );
        p_item_params->AddString( "name", p_string );
        Script::RunScript("net_game_info_add_item", p_item_params );
    	delete p_item_params;
	}

	if( p_server->m_HostMode == vHOST_MODE_AUTO_SERVE )
	{
		// Auto Serve
        p_item_params = new Script::CStruct;
        sprintf( p_string, "%s:", Script::GetLocalString( "netoptions_str_auto_serve" ) );
        p_item_params->AddString( "name", p_string );
        Script::RunScript("net_game_info_add_item", p_item_params );
    	delete p_item_params;
	}
	else if( p_server->m_HostMode == vHOST_MODE_FCFS )
	{
		// FCFS
        p_item_params = new Script::CStruct;
        sprintf( p_string, "%s:", Script::GetLocalString( "netoptions_str_fcfs" ) );
        p_item_params->AddString( "name", p_string );
        Script::RunScript("net_game_info_add_item", p_item_params );
    	delete p_item_params;
	}

#ifdef __PLAT_NGPS__
	// Ping
    p_item_params = new Script::CStruct;
    sprintf( p_string, "%s:", Script::GetLocalString( "sort_title_ping" ) );
    p_item_params->AddString( "name", p_string );
	if( p_server->m_Latency == 9999 )
	{
		sprintf( p_string, "..." );
	}
	else
	{
		sprintf( p_string, "%d", p_server->m_Latency );
	}
    
    p_item_params->AddString( "value", p_string );
    Script::RunScript("net_game_info_add_item", p_item_params );
	delete p_item_params;
#endif

	// Mode
    p_item_params = new Script::CStruct;
    sprintf( p_string, "%s:", Script::GetLocalString( "sort_title_mode" ) );
    p_item_params->AddString( "name", p_string );
    sprintf( p_string, "%s", p_server->m_Mode );
    p_item_params->AddString( "value", p_string );
    Script::RunScript("net_game_info_add_item", p_item_params );
	delete p_item_params;
    
	// Skill Level
    p_skill_array = Script::GetArray( "skill_level_info" );
	p_skill_struct = p_skill_array->GetStructure( p_server->m_SkillLevel );
	p_skill_struct->GetText( "name", &p_skill_string );
	p_item_params = new Script::CStruct;	
    sprintf( p_string, "%s:", Script::GetLocalString( "sort_title_skill" ) );
    p_item_params->AddString( "name", p_string );
    sprintf( p_string, "%s", p_skill_string );
    p_item_params->AddString( "value", p_string );
	Script::RunScript("net_game_info_add_item", p_item_params );
	delete p_item_params;

	// Level
    p_item_params = new Script::CStruct;
    sprintf( p_string, "%s:", Script::GetLocalString( "sort_title_level" ) );
    p_item_params->AddString( "name", p_string );
    sprintf( p_string, "%s", p_server->m_Level );
    p_item_params->AddString( "value", p_string );
    Script::RunScript("net_game_info_add_item", p_item_params );
	delete p_item_params;

	// Goals
#ifndef __PLAT_XBOX__
	if( gamenet_man->InInternetMode())
	{
		p_item_params = new Script::CStruct;
		sprintf( p_string, "%s:", Script::GetLocalString( "sort_title_ranked" ) );
		p_item_params->AddString( "name", p_string );
		if( p_server->m_Ranked )
		{
			sprintf( p_string, "%s", Script::GetLocalString( "netoptions_str_yes" ));
		}
		else
		{
			sprintf( p_string, "%s", Script::GetLocalString( "netoptions_str_no" ));
		}
		
		p_item_params->AddString( "value", p_string );
		Script::RunScript("net_game_info_add_item", p_item_params );
		delete p_item_params;
	}
#endif

	// Player Ratio
    p_item_params = new Script::CStruct;
    sprintf( p_string, ": %d/%d", p_server->m_NumPlayers, p_server->m_MaxPlayers );
    p_item_params->AddString( "text", p_string );
    Script::RunScript("net_game_info_update_player_title", p_item_params );
	delete p_item_params;
    
    // List of Players
    for( i = 0; i < p_server->m_NumPlayers; i++ )
	{
		int rank;

		rank = (int) (((float) p_server->GetPlayerRating( i ) / (float) vMAX_RATING ) * (float) vMAX_RANK );
		p_item_params = new Script::CStruct;
        sprintf( p_string, "%d:", i+1 );
		p_item_params->AddInteger( "rank", rank );
        p_item_params->AddString( "name", p_string );
        sprintf( p_string, "%s", p_server->GetPlayerName( i ) );
        p_item_params->AddString( "value", p_string );
		Script::RunScript("net_game_info_add_player", p_item_params );
		delete p_item_params;
	}

	Mem::Manager::sHandle().PopContext();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		Manager::ScriptChooseServer(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	ServerInfo* p_server;
	Manager* gamenet_man = Manager::Instance();
	bool server_full;
	uint32 reason_checksum;		
	uint32 id;

	pParams->GetChecksum( Script::GenerateCRC("id"), &id );
				
	gamenet_man->ClearServerFocus();

	p_server = gamenet_man->GetServerInfo( id );
	Dbg_Assert( p_server );
								
	server_full = false;
	reason_checksum = 0;
	if( gamenet_man->GetJoinMode() == vJOIN_MODE_PLAY )
	{
		if( p_server->m_NumPlayers >= p_server->m_MaxPlayers )
		{
			server_full = true;
			reason_checksum = Script::GenerateCRC("net_reason_full");
		}
	}
	else if( gamenet_man->GetJoinMode() == vJOIN_MODE_OBSERVE )
	{
		if( p_server->m_NumObservers >= p_server->m_MaxObservers )
		{
			server_full = true;
			reason_checksum = Script::GenerateCRC("net_reason_full_observers");
		}
	}

	if( server_full )
	{   
		Script::CStruct* p_structure = new Script::CStruct;
		p_structure->AddChecksum( "reason", reason_checksum );
		p_structure->AddChecksum( "just_dialog", 0 );

		gamenet_man->m_join_state_task->Remove();
		Script::RunScript( "CreateJoinRefusedDialog", p_structure );
		delete p_structure;
	}
	else
	{
		Script::CScriptStructure* p_structure = new Script::CScriptStructure;

#ifdef __PLAT_NGPS__							
        if( gamenet_man->mpBuddyMan->IsLoggedIn())
		{
			gamenet_man->mpLobbyMan->SetServerName( p_server->m_Name );
		}
#endif

#ifdef __PLAT_XBOX__							
		IN_ADDR host_addr;		
		
		// Register the session key.
		if( XNetRegisterKey( &p_server->m_XboxKeyId, 
			&p_server->m_XboxKey ) != NO_ERROR )
		{
			OutputDebugString( "**** Failed to register key\n" );
			return false;
		}
		
		gamenet_man->m_XboxKeyRegistered = true;
		// Translate the host’s XNADDR to an IN_ADDR.			
		if( XNetXnAddrToInAddr( &p_server->m_XboxAddr, &p_server->m_XboxKeyId,
				&host_addr ) != NO_ERROR )
		{
			OutputDebugString( "**** Failed to get inaddr from xnaddr\n" );
			return false;
		}
		
		memcpy( &gamenet_man->m_XboxKeyId, &p_server->m_XboxKeyId, sizeof( XNKID ));
		
		p_structure->AddComponent( Script::GenerateCRC( "Address" ), 
								ESYMBOLTYPE_INTEGER, (int) host_addr.s_addr );
		p_structure->AddComponent( Script::GenerateCRC( "Port" ), 
								ESYMBOLTYPE_INTEGER, p_server->m_Port );
#else
		char *ip_addr;
		struct in_addr addr;
		uint32 cookie;
		
		addr.s_addr = p_server->m_Ip;
		ip_addr = inet_ntoa( addr );
		if( p_server->m_CanDirectConnect == false )
		{
			cookie = Mth::Rnd( vINT32_MAX );
			p_structure->AddComponent( CRCD( 0x751f4599, "cookie" ), ESYMBOLTYPE_NAME, cookie );
		}
        p_structure->AddComponent( NONAME, ESYMBOLTYPE_INTEGER, p_server->m_Port );
		p_structure->AddComponent( NONAME, ESYMBOLTYPE_STRING, ip_addr );
#endif

		p_structure->AddInteger( "MaxPlayers", p_server->m_MaxPlayers );
		Script::RunScript( "net_chosen_join_server", p_structure );
		delete p_structure;

	}	
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
#ifdef __PLAT_NGPS__
bool 		Manager::ScriptPostGame( Script::CStruct* pParams, Script::CScript* pScript )
{
	Manager* gamenet_man = Manager::Instance();

	if( gamenet_man->mpLobbyMan->ReportedGame() == false )
	{
		gamenet_man->PostGame();
	}

	return true;
}

#endif //__PLAT_NGPS__
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

LobbyInfo::LobbyInfo( void ) : Lst::Node< LobbyInfo > ( this )
{
	m_Full = false;
	m_Official = false;
	m_OffLimits = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


} // namespace GameNet




