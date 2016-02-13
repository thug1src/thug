/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Skate4													**
**																			**
**	Module:			GameNet					 								**
**																			**
**	File name:		GameNet.cpp												**
**																			**
**	Created by:		02/01/01	-	spg										**
**																			**
**	Description:	Game-Side Network Functionality							**
**																			**
*****************************************************************************/

// start autoduck documentation
// @DOC gamenet
// @module gamenet | None
// @subindex Scripting Database
// @index script | gamenet
                          
/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <sys/mcman.h>
#include <sys/SIO/keyboard.h>

#include <gel/net/server/netserv.h>
#include <gel/net/client/netclnt.h>
#include <gel/mainloop.h>
#include <gel/mainloop.h>
#include <gel/objtrack.h>
#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/soundfx/soundfx.h>

#include <gfx/nxviewman.h>
#include <gfx/skeleton.h>
#include <gfx/2d/screenelemman.h>
#include <gfx/FaceTexture.h>
#include <gfx/FaceMassage.h>

#include <sk/gamenet/gamenet.h>

#include <sk/modules/skate/skate.h>
#include <sk/modules/frontend/frontend.h>
#include <sk/modules/skate/gamemode.h>
#include <sk/modules/skate/goalmanager.h>
#include <sk/ParkEditor2/EdMap.h>
#include <sk/ParkEditor2/parked.h>
//#include <sk/modules/frontend/mainmenu.h>

#include <sk/components/skaterscorecomponent.h>
#include <sk/components/GoalEditorComponent.h>
#include <sk/components/skaterstatehistoryComponent.h>
#include <sk/components/skaterphysicscontrolComponent.h>

#include <sk/objects/crown.h>
#include <sk/objects/restart.h>
#include <sk/objects/proxim.h>
#include <sk/objects/skatercam.h>
#include <sk/objects/skaterprofile.h>

#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/array.h>
#include <gel/components/skatercameracomponent.h>
#include <gel/components/walkcameracomponent.h>

#include <sk/scripting/cfuncs.h>
#include <sk/scripting/nodearray.h>
#include <sk/components/RailEditorComponent.h>

#ifdef __PLAT_NGPS__
#include <gamenet/lobby.h>
#include <gamenet/ngps/p_content.h>
#include <gamenet/ngps/p_buddy.h>
#include <gamenet/ngps/p_stats.h>
#endif

#ifdef __PLAT_XBOX__
#include <Sk/GameNet/Xbox/p_auth.h>
#include <Sk/GameNet/Xbox/p_buddy.h>
#include <Sk/GameNet/Xbox/p_voice.h>
#endif

#include <sk/objects/skater.h>		   // we do various things with the skater, network play related


/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

extern bool g_CheatsEnabled;

namespace GameNet
{



DefineSingletonClass( Manager, "Game Network Manager" )

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

#ifndef __PLAT_NGC__
extern	bool	 gEnteringFromMainMenu;
#endif		// __PLAT_NGC__

/*****************************************************************************
**								   Defines									**
*****************************************************************************/
#define USE_DNS	1
enum
{   
	vMAX_TOLERABLE_RESENDS = 20	// we'll allow 20 reseonds of messages to ready coonections but that's our limit
};

#define vTIME_BEFORE_STARTING_NETGAME   Tmr::Seconds( 3 )
#define vTIME_BEFORE_STARTING_GOALATTACK   Tmr::Seconds( 8 )
#define vAUTO_START_INTERVAL			Tmr::Seconds( 5 )
#define vLINK_CHECK_FREQUENCY			Tmr::Seconds( 1 )
#define vTAUNT_INTERVAL			Tmr::Seconds( 2 )
#define vMAX_FACE_DATA_SIZE		( Gfx::CFaceTexture::vTOTAL_CFACETEXTURE_SIZE )

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

static Tmr::Time	s_time_to_start_game;
static Tmr::Time	s_auto_serve_start_time;
//static bool			s_waiting_for_game_to_start = false;
//static Tmr::Time	s_usage_tracking_start_time;

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

Manager::Manager( void )
{
	Net::Manager * net_man = Net::Manager::Instance();
	Mlp::Manager * mlp_manager = Mlp::Manager::Instance();
	int i;

	

	net_man->SetMessageName( MSG_ID_SKATER_INPUT, "Skater Input" );
	net_man->SetMessageName( MSG_ID_PLAYER_CREATE, "Player Create" );
	net_man->SetMessageName( MSG_ID_JOIN_ACCEPTED, "Join Accepted" );
	net_man->SetMessageName( MSG_ID_OBJ_UPDATE_STREAM, "Object Update Stream" );
	net_man->SetMessageName( MSG_ID_NEW_ALIAS, "New Alias" );
	net_man->SetMessageName( MSG_ID_READY_QUERY, "Ready Query" );
	net_man->SetMessageName( MSG_ID_READY_RESPONSE, "Ready Response" );
	net_man->SetMessageName( MSG_ID_PAUSE, "Pause" );
	net_man->SetMessageName( MSG_ID_UNPAUSE, "Unpause" );
	net_man->SetMessageName( MSG_ID_PRIM_ANIM_START, "Primary Animation" );
	net_man->SetMessageName( MSG_ID_FLIP_ANIM, "Flip Animation" );
	net_man->SetMessageName( MSG_ID_ROTATE_SKATEBOARD, "Rotate Skateboard" );
	net_man->SetMessageName( MSG_ID_ROTATE_DISPLAY, "Rotate Display" );
	net_man->SetMessageName( MSG_ID_CLEAR_ROTATE_DISPLAY, "Clear Rotate Display" );
	net_man->SetMessageName( MSG_ID_CREATE_SPECIAL_ITEM, "Create Item" );
	net_man->SetMessageName( MSG_ID_DESTROY_SPECIAL_ITEM, "Destroy Item" );
	net_man->SetMessageName( MSG_ID_SET_WOBBLE_TARGET, "Set Wobble Target" );
	net_man->SetMessageName( MSG_ID_SET_WOBBLE_DETAILS, "Set Wobble Details" );
	net_man->SetMessageName( MSG_ID_SET_LOOPING_TYPE, "Set Looping Type" );
	net_man->SetMessageName( MSG_ID_SET_HIDE_ATOMIC, "Hide Atomic" );
	net_man->SetMessageName( MSG_ID_SET_ANIM_SPEED, "Set anim speed" );
	net_man->SetMessageName( MSG_ID_PLAYER_QUIT, "Player Quit" );	
	net_man->SetMessageName( MSG_ID_HOST_PROCEED, "Proceed Hosting" );
	net_man->SetMessageName( MSG_ID_GAMELIST_RESPONSE, "Gamelist" );
	net_man->SetMessageName( MSG_ID_HOST_QUIT, "Host Shutdown" );
	net_man->SetMessageName( MSG_ID_START_INFO, "Start Info" );
	net_man->SetMessageName( MSG_ID_GAME_INFO, "Game Info" );
	net_man->SetMessageName( MSG_ID_PLAY_SOUND, "Play Sound" );
	net_man->SetMessageName( MSG_ID_PLAY_LOOPING_SOUND, "Play Looping Sound" );
	net_man->SetMessageName( MSG_ID_SPARKS_ON, "Sparks On" );
	net_man->SetMessageName( MSG_ID_SPARKS_OFF, "Sparks Off" );
	net_man->SetMessageName( MSG_ID_BLOOD_ON, "Blood On" );
	net_man->SetMessageName( MSG_ID_BLOOD_OFF, "Blood Off" );
	net_man->SetMessageName( MSG_ID_SCORE, "Score Msg" );
	net_man->SetMessageName( MSG_ID_PANEL_MESSAGE, "Panel Message" );
	net_man->SetMessageName( MSG_ID_SCORE_UPDATE, "Score Update" );
	net_man->SetMessageName( MSG_ID_PROCEED_TO_PLAY, "Proceed To Play" );
	net_man->SetMessageName( MSG_ID_SKATER_COLLIDE_LOST, "Skater Col Lost" );
	net_man->SetMessageName( MSG_ID_SKATER_COLLIDE_WON, "Skater Col Win" );
	net_man->SetMessageName( MSG_ID_SKATER_HIT_BY_PROJECTILE, "Hit by projectile" );
	net_man->SetMessageName( MSG_ID_SKATER_PROJECTILE_HIT_TARGET, "Projectile Hit" );
	net_man->SetMessageName( MSG_ID_BAIL_DONE, "Bail done" );
	net_man->SetMessageName( MSG_ID_RUN_SCRIPT, "Run Script" );
	net_man->SetMessageName( MSG_ID_SPAWN_AND_RUN_SCRIPT, "SpawnRun Script" );
	net_man->SetMessageName( MSG_ID_OBSERVE, "Observe" );
	net_man->SetMessageName( MSG_ID_STEAL_MESSAGE, "Steal message" );
	net_man->SetMessageName( MSG_ID_CHAT, "Chat Message" );
	net_man->SetMessageName( MSG_ID_OBSERVE_PROCEED, "Proceed to Observe" );
	net_man->SetMessageName( MSG_ID_OBSERVE_REFUSED, "Observe Refused" );
	net_man->SetMessageName( MSG_ID_JOIN_PROCEED, "Proceed to Join" );
	net_man->SetMessageName( MSG_ID_JOIN_REQ, "Join Request" );
	net_man->SetMessageName( MSG_ID_KICKED, "Kick Message" );
	net_man->SetMessageName( MSG_ID_LANDED_TRICK, "Landed Trick" );
	net_man->SetMessageName( MSG_ID_PLAYER_INFO_ACK_REQ, "Player Ack Req" );
	net_man->SetMessageName( MSG_ID_PLAYER_INFO_ACK, "Player Info Ack" );
	net_man->SetMessageName( MSG_ID_NEW_KING, "New King" );
	net_man->SetMessageName( MSG_ID_LEFT_OUT, "Left Out" );
	net_man->SetMessageName( MSG_ID_MOVED_TO_RESTART, "Moved to Restart" );
	net_man->SetMessageName( MSG_ID_KING_DROPPED_CROWN, "Dropped Crown" );
	net_man->SetMessageName( MSG_ID_PLAYER_DROPPED_FLAG, "Dropped Flag" );
	net_man->SetMessageName( MSG_ID_RUN_HAS_ENDED, "Run ended" );
	net_man->SetMessageName( MSG_ID_GAME_OVER, "Game over" );
	net_man->SetMessageName( MSG_ID_OBSERVER_LOG_TRICK_OBJ, "Observer Log Trick Object" );
	net_man->SetMessageName( MSG_ID_OBSERVER_INIT_GRAFFITI_STATE, "Observer Init Graffiti State" );
	net_man->SetMessageName( MSG_ID_AUTO_SERVER_NOTIFICATION, "Auto-Server Notification" );
	net_man->SetMessageName( MSG_ID_FCFS_ASSIGNMENT, "FCFS Assignment" );
	net_man->SetMessageName( MSG_ID_FCFS_END_GAME, "FCFS End Game" );
	net_man->SetMessageName( MSG_ID_FCFS_SET_NUM_TEAMS, "FCFS Num Teams" );
	net_man->SetMessageName( MSG_ID_END_GAME, "End Game" );
	net_man->SetMessageName( MSG_ID_SELECT_GOALS, "Select Goals" );
	net_man->SetMessageName( MSG_ID_BEAT_GOAL, "Beat Goal" );
	net_man->SetMessageName( MSG_ID_STARTED_GOAL, "Started Goal" );
	net_man->SetMessageName( MSG_ID_TOGGLE_PROSET, "Toggle proset" );
	net_man->SetMessageName( MSG_ID_FCFS_TOGGLE_PROSET, "FCFS Toggle proset" );
	net_man->SetMessageName( MSG_ID_FCFS_TOGGLE_GOAL_SELECTION, "FCFS Toggle Goal" );
	net_man->SetMessageName( MSG_ID_KILL_TEAM_FLAGS, "Kill Team Flags" );
	net_man->SetMessageName( MSG_ID_WAIT_N_SECONDS, "Wait N Seconds" );
	net_man->SetMessageName( MSG_ID_CHEAT_CHECKSUM_REQUEST, "CC Request" );
	net_man->SetMessageName( MSG_ID_CHEAT_CHECKSUM_RESPONSE, "CC Response" );
    
	net_man->SetMessageFlags( MSG_ID_OBJ_UPDATE_STREAM, Net::mMSG_SIZE_UNKNOWN );
		
	for( i = 0; i < vMAX_LOCAL_CLIENTS; i++ )
	{
		m_client[i] = NULL;
		m_match_client = NULL;
	}
	
	m_server = NULL;
    
	m_metrics_on = false;
	m_scores_on = false;
	m_cheating_occurred = false;
	m_draw_player_names = true;
	m_sort_key = vSORT_KEY_NAME;
	m_host_mode = vHOST_MODE_SERVE;
	m_server_list_state = vSERVER_LIST_STATE_SHUTDOWN;
	m_next_server_list_state = vSERVER_LIST_STATE_SHUTDOWN;
    
	// initialize the network preferences
	m_network_preferences.Load( Script::GenerateCRC("default_network_preferences" ) );
	m_taunt_preferences.Load( Script::GenerateCRC("default_taunt_preferences" ) );

	m_timeout_connections_task = new Tsk::Task< Manager > ( s_timeout_connections_code, *this,
										Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_TIMEOUT_CONNECTIONS );
	m_render_metrics_task = new Tsk::Task< Manager > ( s_render_metrics_code, *this );
    m_auto_refresh_task = new Tsk::Task< Manager > ( s_auto_refresh_code, *this );
	m_auto_server_task = new Tsk::Task< Manager > ( s_auto_server_code, *this );
	m_render_scores_task = new Tsk::Task< Manager > ( s_render_scores_code, *this );
	m_client_add_new_players_task = new Tsk::Task< Manager > ( s_client_add_new_players_code, *this, 
										Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_CLIENT_ADD_NEW_PLAYERS );
	m_server_add_new_players_task = new Tsk::Task< Manager > ( s_server_add_new_players_code, *this, 
										Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_SERVER_ADD_NEW_PLAYERS );
    m_enter_chat_task = new Tsk::Task< Manager > ( s_enter_chat_code, *this );
	m_modem_state_task = new Tsk::Task< Manager > ( s_modem_state_code, *this );
	m_join_timeout_task = new Tsk::Task< Manager > ( s_join_timeout_code, *this );
	m_server_list_state_task = new Tsk::Task< Manager > ( s_server_list_state_code, *this );
	m_join_state_task = new Tsk::Task< Manager > ( s_join_state_code, *this );
	m_start_network_game_task = new Tsk::Task< Manager > ( s_start_network_game_code, *this );
	m_change_level_task = new Tsk::Task< Manager > ( s_change_level_code, *this );
	m_ctf_logic_task = new Tsk::Task< Manager > ( s_ctf_logic_code, *this );

	mlp_manager->AddLogicTask( *m_timeout_connections_task );

	m_last_modem_state = Net::vMODEM_STATE_DISCONNECTED;
	m_goals_data_size = 0;
	m_goals_level = 0;

#ifdef __PLAT_XBOX__
	m_XboxKeyRegistered = false;
	mpAuthMan = new AuthMan;
	mpBuddyMan = new BuddyMan;
	mpVoiceMan = new VoiceMan;
#endif

#ifdef __PLAT_NGPS__
	m_got_motd = false;
	mpLobbyMan = new LobbyMan;
	mpContentMan = new ContentMan;
	mpBuddyMan = new BuddyMan;
	mpStatsMan = new StatsMan;
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Manager::~Manager( void )
{
#ifdef __PLAT_NGPS__
	delete mpLobbyMan;
	delete mpContentMan;
	delete mpBuddyMan;
	delete mpStatsMan;
#endif

	delete m_client_add_new_players_task;
	delete m_server_add_new_players_task;
	delete m_timeout_connections_task;
	delete m_render_metrics_task;
	delete m_enter_chat_task;
	delete m_server_list_state_task;
	delete m_join_state_task;
	delete m_start_network_game_task;
	delete m_change_level_task;
    delete m_auto_refresh_task;
#ifdef __PLAT_XBOX__
	delete mpAuthMan;
	delete mpBuddyMan;
	delete mpVoiceMan;
#endif // __PLAT_XBOX__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::free_all_players( void )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	PlayerInfo *player, *next;
	Lst::Search< PlayerInfo > sh;

	for( player = FirstPlayerInfo( sh, true ); player; player = next )
	{
		next = NextPlayerInfo( sh, true );
		// if this player has a skater
		if( player->m_Skater )
		{
			// and the skater is not on the default heap, or it's not a local player
			if ( player->m_Skater->GetHeapIndex() != 0 || !player->IsLocalPlayer() )
			{
				// then remove it (deletes the CSkater, which will unload any model)
				skate_mod->remove_skater( player->m_Skater );
			}
		}
		DestroyPlayer( player );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::free_all_pending_players( void )
{
	Lst::Node< NewPlayerInfo > *node, *next;
	NewPlayerInfo* new_info;
		
	

	Dbg_Printf( "Destroying Pending Players\n" );
	for( node = m_new_players.GetNext(); node; node = next )
	{
		next = node->GetNext();

		new_info = node->GetData();

		if( m_server )
		{
			Dbg_Printf( "Removing %s\n", new_info->Name );
			m_server->TerminateConnection( new_info->Conn );
		}
		
		delete node;
		delete new_info;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::s_observer_logic_code ( const Tsk::Task< PlayerInfo >& task )
{
	Manager * gamenet_man = Manager::Instance();

	// Dan: Proxim_Update use is now done through CProximTriggerComponents attached to the camera CCompositeObjects.  Thus, the observer's
	// camera should handle proxim updating	on its own.
	
	// PlayerInfo* current_player;
	// current_player = gamenet_man->GetCurrentlyObservedPlayer();
	// if( current_player )
	// {
		// Obj::CSkater* skater;
		// skater = local_player.m_cam->GetSkater();
	
		// if( skater )
		// {   
			// Mdl::Skate * skate_mod = Mdl::Skate::Instance();
			// if( current_player->IsFullyIn())
			// {
				// local_player.m_cam->Update( false );
			
				// if( skate_mod->mpProximManager )
				// {   
					// if( local_player.m_cam )
					// {
						// Obj::Proxim_Update( skater, local_player.m_cam );
					// }
				// }
			// }
		// }
	// }

	if( gamenet_man->GetObserverCommand() == vOBSERVER_COMMAND_NEXT )
	{
		Dbg_Printf( "Got observer next command.....\n" );

		PlayerInfo*	player;
		
		player = gamenet_man->GetNextPlayerToObserve();
		gamenet_man->ObservePlayer( player );

		gamenet_man->SetObserverCommand( vOBSERVER_COMMAND_NONE );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::s_observer_input_logic_code ( const Inp::Handler < Manager >& handler )
{
    

	Manager&	man = handler.GetData();
	
	Front::CScreenElementManager* p_screen_elem_man = Front::CScreenElementManager::Instance();
	Front::CScreenElement* p_root_window = p_screen_elem_man->GetElement( Script::GenerateCRC( "root_window" ) , 
																			Front::CScreenElementManager::DONT_ASSERT );
	// If any operable menus are up, ignore input
	if( p_root_window )
	{
		Script::CStruct* pTags = new Script::CStruct();
		p_root_window->CopyTagsToScriptStruct( pTags );
		uint32 menu_state;
		pTags->GetChecksum( "menu_state", &menu_state, Script::NO_ASSERT );
		delete pTags;
		if ( menu_state == Script::GenerateCRC( "on" ) )
		{
			return;
		}
	}

	if( handler.m_Input->m_Makes & Inp::Data::mD_X )
	{
		Dbg_Printf( "Got X, inserting command....\n" );
		man.SetObserverCommand( vOBSERVER_COMMAND_NEXT );
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::s_auto_server_code( const Tsk::Task< Manager >& task )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Manager& man = task.GetData();

	// Give it a couple of seconds before activating
	if(( Tmr::GetTime() - s_auto_serve_start_time ) < Tmr::Seconds( 2 ))
	{
		return;
	}
	if( ( skate_mod->GetGameMode()->GetNameChecksum() != Script::GenerateCRC( "netlobby" )))
	{
		return;
	}

	if( man.GetNumPlayers() < 2 )
	{
		return;
	}

	if( man.m_waiting_for_game_to_start )
	{
		return;
	}

	if(( Tmr::GetTime() - man.m_lobby_start_time ) > vAUTO_START_INTERVAL )
	{
        Dbg_Printf( "Starting Network Game....\n" );
		Script::RunScript( "LoadPendingPlayers" );
		Script::RunScript( "StartNetworkGame" );
		man.m_waiting_for_game_to_start = true;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::s_enter_chat_code( const Tsk::Task< Manager >& task )
{
	int num_chars;
	char makes[256];
	static Tmr::Time s_last_taunt_time = 0;
		
	num_chars = SIO::KeyboardRead( makes );

	if( num_chars > 0 )
	{
		// Space and enter bring up the chat interface
		if( ( makes[0] == SIO::vKB_ENTER ) ||
			( makes[0] == 32 ))
		{
			Script::CStruct* pParams;
			
			pParams = new Script::CStruct;
			pParams->AddChecksum( Script::GenerateCRC( "id" ), Script::GenerateCRC( "keyboard_anchor" ));
			
			// Enter and space act as "choose" only if you're not currently using the on-screen keyboard
			if( Obj::ScriptObjectExists( pParams, NULL ) == false )
			{
				pParams->Clear();
				pParams->AddChecksum( Script::GenerateCRC( "id" ), Script::GenerateCRC( "current_menu_anchor" ));
				if( Obj::ScriptObjectExists( pParams, NULL ) == false )
				{
					pParams->Clear();
					pParams->AddChecksum( Script::GenerateCRC( "id" ), Script::GenerateCRC( "dialog_box_anchor" ));
					if( Obj::ScriptObjectExists( pParams, NULL ) == false )
					{
						Script::RunScript( "enter_kb_chat" );
						SIO::KeyboardClear();
					}
				}
			}
		}
		else if( makes[0] == SIO::vKB_F1 )
		{
			if(( Tmr::GetTime() - s_last_taunt_time ) > vTAUNT_INTERVAL )
			{
				Script::CStruct* pParams;

				pParams = new Script::CStruct;
				pParams->AddChecksum( CRCD(0xb53d0e0f,"string_id"), CRCD(0xe5fd359,"props_string"));
				Script::RunScript( "SendTauntMessage", pParams );
				delete pParams;

				s_last_taunt_time = Tmr::GetTime();
			}
		}
		else if( makes[0] == SIO::vKB_F2 )
		{
			if(( Tmr::GetTime() - s_last_taunt_time ) > vTAUNT_INTERVAL )
			{
				Script::CStruct* pParams;

				pParams = new Script::CStruct;
				pParams->AddChecksum( CRCD(0xb53d0e0f,"string_id"), CRCD(0xbeea3518,"your_daddy_string"));
				Script::RunScript( "SendTauntMessage", pParams );
				delete pParams;

				s_last_taunt_time = Tmr::GetTime();
			}
		}
		else if( makes[0] == SIO::vKB_F3 )
		{
			if(( Tmr::GetTime() - s_last_taunt_time ) > vTAUNT_INTERVAL )
			{
				Script::CStruct* pParams;

				pParams = new Script::CStruct;
				pParams->AddChecksum( CRCD(0xb53d0e0f,"string_id"), CRCD(0x4525adbd,"get_some_string"));
				Script::RunScript( "SendTauntMessage", pParams );
				delete pParams;

				s_last_taunt_time = Tmr::GetTime();
			}
		}
		else if( makes[0] == SIO::vKB_F4 )
		{
			if(( Tmr::GetTime() - s_last_taunt_time ) > vTAUNT_INTERVAL )
			{
				Script::CStruct* pParams;

				pParams = new Script::CStruct;
				pParams->AddChecksum( CRCD(0xb53d0e0f,"string_id"), CRCD(0xa36dbee1,"no_way_string"));
				Script::RunScript( "SendTauntMessage", pParams );
				delete pParams;

				s_last_taunt_time = Tmr::GetTime();
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::s_join_state_code( const Tsk::Task< Manager >& task )
{
	Manager& man = task.GetData();
		
	switch( man.GetJoinState())
	{   
		case vJOIN_STATE_TRYING_PASSWORD:
		case vJOIN_STATE_WAITING:
		{   
			break;
		}
		case vJOIN_STATE_CONNECTING:
		{   
			Script::RunScript( "CreateConnectingDialog" );
			
			man.SetJoinState( vJOIN_STATE_WAITING );
			break;
		}
		case vJOIN_STATE_JOINING:
		{
			Net::Client* client;

			client = man.GetClient( 0 );
			if( !client->IsLocal())
			{
				Script::RunScript( "CreateJoiningDialog" );
			}
					
			man.SetJoinState( vJOIN_STATE_WAITING );
			break;
		}
		case vJOIN_STATE_JOINING_WITH_PASSWORD:
		{
			Script::RunScript( "CreateTryingPasswordDialog" );
			
			man.SetJoinState( vJOIN_STATE_TRYING_PASSWORD );
			break;
		}
		case vJOIN_STATE_REFUSED:
		{
			Script::CStruct* p_structure = new Script::CStruct;
			p_structure->AddChecksum( "reason", man.m_conn_refused_reason );

			man.m_join_state_task->Remove();

			Script::RunScript( "CreateJoinRefusedDialog", p_structure );
			delete p_structure;
			break;
		}
		case vJOIN_STATE_GOT_PLAYERS:
		{
			Lst::Search< NewPlayerInfo > sh;
			NewPlayerInfo* new_player;		
			
			// If we're observing, we need to remove our skater
			for( new_player = man.FirstNewPlayerInfo( sh ); new_player; new_player = man.NextNewPlayerInfo( sh ))
			{
				if( new_player->Flags & PlayerInfo::mLOCAL_PLAYER )
				{
					if( new_player->Flags & PlayerInfo::mOBSERVER )
					{
						Mdl::Skate * skate_mod = Mdl::Skate::Instance();

						Obj::CSkater* skater;
						skater = skate_mod->GetLocalSkater();
						Dbg_Assert( skater );
						skate_mod->remove_skater( skater );
						man.ObservePlayer( NULL );
					}
					break;
				}
			}
			
			Script::RunScript( "entered_network_game" );

			man.SetJoinState( vJOIN_STATE_WAITING_FOR_START_INFO );
			break;
		}
		case vJOIN_STATE_WAITING_FOR_START_INFO:
			break;
		case vJOIN_STATE_CONNECTED:
		{
			Script::RunScript( "dialog_box_exit" );
			man.m_join_timeout_task->Remove();
		}
		// Fall-through intentional
		case vJOIN_STATE_FINISHED:
		{
			man.m_join_state_task->Remove();
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

void	Manager::s_ctf_logic_code( const Tsk::Task< Manager >& task )
{
	Manager * gamenet_man = Manager::Instance();
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;

	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	if( skate_mod->GetGameMode()->GetNameChecksum() != Script::GenerateCRC( "netctf" ))
	{
		Dbg_MsgAssert( 0, ( "Shouldn't be running CTF logic in non-ctf game" ));
		return;
	}
	
	for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
	{
		if( player->HasCTFFlag())
		{
			int team;
			uint32 obj_id;
			Obj::CMovingObject* obj;
			Obj::CMovingObject* moving_obj;

			team = player->HasWhichFlag();
			switch( team )
			{
				case vTEAM_RED:
					obj_id = Script::GenerateCRC( "TRG_CTF_Red" );
					break;
				case vTEAM_BLUE:
					obj_id = Script::GenerateCRC( "TRG_CTF_Blue" );
					break;
				case vTEAM_GREEN:
					obj_id = Script::GenerateCRC( "TRG_CTF_Green" );
					break;
			case vTEAM_YELLOW:
					obj_id = Script::GenerateCRC( "TRG_CTF_Yellow" );
					break;
				default:
					Dbg_Assert( 0 );
					obj_id = 0;
					break;
			}

			obj = (Obj::CMovingObject *) Obj::ResolveToObject( obj_id );
			if( obj == NULL )
			{
				return;
			}

			moving_obj = static_cast<Obj::CMovingObject *>( obj );

			Gfx::CSkeleton* pSkeleton;
			pSkeleton = player->m_Skater->GetSkeleton();
			Dbg_Assert( pSkeleton );

			Mth::Matrix& flag_matrix = moving_obj->GetMatrix();
			pSkeleton->GetBoneMatrix( CRCD(0xddec28af,"Bone_Head"), &flag_matrix );
		
			flag_matrix.RotateXLocal( 90.0f );

			// raise the flag by 1 foot
			flag_matrix.TranslateLocal( Mth::Vector( 0.0f, FEET(1.0f), 0.0f, 0.0f ) );
		
			Mth::Vector flag_offset = flag_matrix[Mth::POS];
			flag_matrix[Mth::POS] = Mth::Vector( 0.0f, 0.0f, 0.0f, 1.0f );
		
			// for some reason, the below doesn't work as expected.
			// so instead, use the skater's root matrix for the orientation
			flag_matrix = flag_matrix * player->m_Skater->GetMatrix();
			//		crown.m_matrix = crown.mp_king->GetMatrix();

			moving_obj->m_pos = player->m_Skater->GetMatrix().Transform( flag_offset );
			moving_obj->m_pos += player->m_Skater->m_pos;

// Mick:  Removed this as it no longer does anything apart from copying over the display matrix
// if that's still needed then do it here
// Might also possibly have some problem with shadows being left behind, but that just indicates the need
// for a proper LOD system for shadows
//			moving_obj->MovingObj_Update();
// Mick:  Like this, but I don't think it's needed  		
//			moving_obj->m_display_matrix = moving_obj->m_matrix;
		
	//		Gfx::AddDebugStar( crown.m_pos, 10, 0x0000ffff, 1 );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::s_start_network_game_code( const Tsk::Task< Manager >& task )
{
	//Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Manager& man = task.GetData();
	static bool showing_wait_screen = false;
                                                                                             
	if( man.NumPartiallyLoadedPlayers() > 0 )
	{
		if( man.OnServer())
		{
			if( showing_wait_screen == false )
			{
				Front::CScreenElementManager* pManager = Front::CScreenElementManager::Instance();
				Front::CScreenElementPtr p_elem = pManager->GetElement( CRCD(0xf53d1d83,"current_menu_anchor"), Front::CScreenElementManager::DONT_ASSERT );
				if( !p_elem )
				{
					Script::RunScript( "CreateWaitForPlayersDialog" );
					showing_wait_screen = true;
				}
			}
		}
		else
		{
			if( showing_wait_screen == false )
			{
				man.CreateNetPanelMessage( true, Script::GenerateCRC("net_message_game_will_start"),
										   NULL, NULL, NULL, "netexceptionprops" );
				showing_wait_screen = true;
			}
		}

		return;
	}
    
	if( ScriptAllPlayersAreReady( NULL, NULL ) == false )
	{
		return;
	}

	if(( Tmr::GetTime() < s_time_to_start_game ))
	{
		return;
	}

	showing_wait_screen = false;
	man.StartNetworkGame();
	man.m_start_network_game_task->Remove();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::s_modem_state_code( const Tsk::Task< Manager >& task )
{
	Net::Manager * net_man = Net::Manager::Instance();
	int new_modem_state;
	Manager& man = task.GetData();

	new_modem_state = net_man->GetModemState();
	
	// Check for positive edge
	if( new_modem_state != man.m_last_modem_state )
	{
		Script::CStruct* pStructure = new Script::CStruct;
		
		switch( new_modem_state )
		{
			case Net::vMODEM_STATE_DIALING:
			{
				char dial_msg[128];

                if( net_man->GetConnectionType() == Net::vCONN_TYPE_PPPOE )
				{
					sprintf( dial_msg, "%s", Script::GetString("net_modem_state_conencting"));
				}
				else
				{
					sprintf( dial_msg, "%s %s", Script::GetString("net_modem_state_dialing"), 
				   							net_man->GetISPPhoneNumber());
				}

				pStructure->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, dial_msg );
				Script::RunScript( "create_modem_state_dialog", pStructure );
                
				Dbg_Printf( "In modem state code. Got state dialing\n" );
				break;
			}
			case Net::vMODEM_STATE_CONNECTED:
			{
				pStructure->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, Script::GetString( "net_modem_state_connected" ));
				Script::RunScript( "create_modem_state_dialog", pStructure );

				Dbg_Printf( "In modem state code. Got state connected\n" );
				break;
			}
			case Net::vMODEM_STATE_LOGGED_IN:
			{
				char conn_msg[128];

				Dbg_Printf( "In modem state code. Got state logged in\n" );
				if( net_man->GetConnectionType() == Net::vCONN_TYPE_MODEM )
				{
					sprintf( conn_msg, "%s %d bps", Script::GetString("net_modem_state_logged_in"), 
										net_man->GetModemBaudRate());
				}
				else
				{
					sprintf( conn_msg, "%s", Script::GetString("net_modem_state_logged_in"));
				}
				
				pStructure->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, conn_msg );
				Script::RunScript( "create_modem_final_state_dialog", pStructure );
				break;
			}
			case Net::vMODEM_STATE_DISCONNECTING:
			{
				pStructure->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, Script::GetString( "net_modem_state_disconnecting" ));
				Script::RunScript( "create_modem_status_dialog", pStructure );
				

				Dbg_Printf( "In modem state code. Got state disconnecting\n" );
				break;
			}
			case Net::vMODEM_STATE_HANGING_UP:
			{
				pStructure->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, Script::GetString( "net_modem_state_hanging_up" ));
				Script::RunScript( "create_modem_status_dialog", pStructure );

				Dbg_Printf( "In modem state code. Got state hanging up\n" );
				break;
			}
			case Net::vMODEM_STATE_DISCONNECTED:
			{
				pStructure->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, Script::GetString( "net_modem_state_disconnected" ));
				Script::RunScript( "create_modem_final_state_dialog", pStructure );

				Dbg_Printf( "In modem state code. Got state disconnected\n" );
				break;
			}
			case Net::vMODEM_STATE_READY_TO_TRANSMIT:
			{
#ifdef __PLAT_NGPS__
				Dbg_Printf( "Removing modem state task\n" );
				man.RemoveModemStateTask();
				
				if( man.m_connection_success_script )
				{
					Script::RunScript( man.m_connection_success_script );
				}
#endif		// __PLAT_NGPS__
				break;
			}
			case Net::vMODEM_STATE_ERROR:
			{
#ifdef __PLAT_NGPS__
				uint32 modem_error;

				switch( net_man->GetModemError())
				{
					case SNTC_ERR_NOMODEM:
						modem_error = Script::GenerateCRC("net_modem_error_no_modem");
						break;
					case SNTC_ERR_TIMEOUT:
						modem_error = Script::GenerateCRC("net_modem_error_timeout");
						break;
					case SNTC_ERR_CONNECT:
					{
						Prefs::Preferences* prefs;
						uint32 config_type;

						prefs = man.GetNetworkPreferences();
						config_type = prefs->GetPreferenceChecksum( CRCD(0xe381f426,"config_type"), 
																	CRCD(0x21902065,"checksum") );
						if( config_type == CRCD(0x99452df9,"config_manual"))
						{
							modem_error = Script::GenerateCRC("net_modem_error_during_connect");
						}
						else
						{
							modem_error = Script::GenerateCRC("net_modem_error_during_connect_ync");
						}
						
						break;
					}
					case SNTC_ERR_BUSY:
						modem_error = Script::GenerateCRC("net_modem_error_busy");
						break;
					case SNTC_ERR_NOCARRIER:
					case SNTC_ERR_NOANSWER:
						modem_error = Script::GenerateCRC("net_modem_error_no_connect");
						break;
					case SNTC_ERR_NODIALTONE:
						modem_error = Script::GenerateCRC("net_modem_error_no_dialtone");
						break;
					default:
						modem_error = Script::GenerateCRC("net_modem_error_no_connect");
						break;
				}

				Dbg_Printf( "In modem state error: %s\n", Script::GetString( modem_error ));
				pStructure->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, Script::GetString( modem_error ));
				Script::RunScript( "create_modem_final_state_dialog", pStructure );
#endif		// __PLAT_NGPS__
				break;
			}
			default:
				Dbg_Assert( 0 );
				break;
		}
		
		delete pStructure;
	}

	man.m_last_modem_state = new_modem_state;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::s_client_add_new_players_code( const Tsk::Task< Manager >& task )
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());
	
	Manager& man = task.GetData();
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CSkater* skater;
	Lst::Node< NewPlayerInfo > *node, *next;
	NewPlayerInfo* new_player;
	Net::Conn* server_conn;
	Lst::Search< Net::Conn > sh;
	bool added_player;
	int i;
                         
	added_player = false;
	for( node = man.m_new_players.GetNext(); node; node = next )
	{
		next = node->GetNext();

		new_player = node->GetData();

		if( new_player->AllowedJoinFrame > man.m_client[0]->m_FrameCounter )
		{
			break;
		}

		// GJ:  the "new_player" structure should already
		// contain the desired appearance

		man.ClientAddNewPlayer( new_player );

		delete new_player;
		delete node;

		added_player = true;
	}

	if( added_player )
	{
        for( i = 0; i < vMAX_LOCAL_CLIENTS; i++ )
		{
			if( man.m_client[i] )
			{
				server_conn = man.m_client[i]->FirstConnection( &sh );
				if( server_conn )
				{
					server_conn->UpdateCommTime();	// update the current comm time so it doesn't time out after
													// laoding the skater
				}
			}
		}
		for( i = 0; i < Mdl::Skate::vMAX_SKATERS; i++ )
		{
			skater = skate_mod->GetSkater( i );
			if( skater )
			{   
				skater->Resync();
			}
		}
        
		man.RespondToReadyQuery();
	}
	Mem::Manager::sHandle().PopContext();
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::s_server_add_new_players_code( const Tsk::Task< Manager >& task )
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());
	Manager& man = task.GetData();
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Lst::Node< NewPlayerInfo > *node, *next;
	NewPlayerInfo* new_info;
	Net::MsgDesc msg_desc;
	Net::Manager* net_man = Net::Manager::Instance();

	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	for( node = man.m_new_players.GetNext(); node; node = next )
	{
		Obj::CSkater* skater;
		MsgNewPlayer new_player_msg;
		Lst::Search< PlayerInfo > sh;
		PlayerInfo* player, *new_player;
		MsgReady ready_msg;
		bool narrowband;
				
		next = node->GetNext();

		new_info = node->GetData();
	
		if( new_info->AllowedJoinFrame > man.m_server->m_FrameCounter )
		{
			break;
		}

		narrowband = ( new_info->Conn->GetBandwidthType() == Net::Conn::vNARROWBAND ) ||
						( net_man->GetConnectionType() == Net::vCONN_TYPE_MODEM );

		// Mick: moved onto skater info heap, to prevent fragmentation
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());
		new_player = man.NewPlayer( NULL, new_info->Conn, new_info->Flags );
		Mem::Manager::sHandle().PopContext();
		
		strcpy( new_player->m_Name, new_info->Name );
		new_player->CopyProfile( new_info->mpSkaterProfile );
		new_player->m_jump_in_frame = new_info->JumpInFrame;
		new_player->m_Profile = new_info->Profile;
		new_player->m_Rating = new_info->Rating;
		new_player->m_VehicleControlType = new_info->VehicleControlType;

		man.AddPlayerToList( new_player );
		ready_msg.m_Time = Tmr::GetTime();
        
		if( !new_player->IsLocalPlayer())
		{
			// Send the new player's create skater command in such a way that he knows it's his own skater
			new_player_msg.m_ObjId = new_info->ObjID;
			new_player_msg.m_Team = new_player->m_Team;
			new_player_msg.m_Profile = new_player->m_Profile;
			new_player_msg.m_Rating = new_player->m_Rating;
			new_player_msg.m_VehicleControlType = new_player->m_VehicleControlType;
			new_player_msg.m_Score = 0;
			strcpy( new_player_msg.m_Name, new_info->Name );
            
			new_player_msg.m_Flags = PlayerInfo::mLOCAL_PLAYER;
			if( new_player->m_flags.TestMask( PlayerInfo::mJUMPING_IN ))
			{
				new_player_msg.m_Flags |= PlayerInfo::mJUMPING_IN;
			}
			if( new_player->IsObserving())
			{
				new_player_msg.m_Flags |= PlayerInfo::mOBSERVER;
			}
			
			msg_desc.m_Data = NULL;
			msg_desc.m_Length = 0;
			msg_desc.m_Id = MSG_ID_JOIN_ACCEPTED;
			msg_desc.m_Priority = Net::HIGHEST_PRIORITY;
			man.m_server->EnqueueMessage( new_player->GetConnHandle(), &msg_desc );
		
			msg_desc.m_Data = &new_player_msg;
			msg_desc.m_Length = sizeof( MsgNewPlayer ) - vMAX_APPEARANCE_DATA_SIZE;
			msg_desc.m_Id = MSG_ID_PLAYER_CREATE;
			man.m_server->EnqueueMessage( new_player->GetConnHandle(), &msg_desc );
		}
		

		if( !new_player->IsObserving())
		{
			// Send the new player's info to existing players
			for( player = man.FirstPlayerInfo( sh, true ); player; 
					player = man.NextPlayerInfo( sh, true ))
			{
				// don't send the new player's info to the new player. We sent it separately and specially
				// Also, don't send to the server (ourselves)
				if(( new_player != player ) &&
				   ( !player->IsLocalPlayer()))
				{   
					new_player_msg.m_ObjId = new_info->ObjID;
					new_player_msg.m_Flags = 0;
					new_player_msg.m_Team = new_player->m_Team;
					new_player_msg.m_Profile = new_player->m_Profile;
					new_player_msg.m_Rating = new_player->m_Rating;
					new_player_msg.m_VehicleControlType = new_player->m_VehicleControlType;
					new_player_msg.m_Score = 0;
					strcpy( new_player_msg.m_Name, new_player->m_Name );
					Dbg_Assert( new_player->mp_SkaterProfile );
					uint32 data_size = new_player->mp_SkaterProfile->WriteToBuffer(new_player_msg.m_AppearanceData, vMAX_APPEARANCE_DATA_SIZE );

					msg_desc.m_Data = &new_player_msg;
					msg_desc.m_Length = sizeof( MsgNewPlayer ) - vMAX_APPEARANCE_DATA_SIZE + data_size;
					msg_desc.m_Id = MSG_ID_PLAYER_CREATE;
					msg_desc.m_Priority = Net::NORMAL_PRIORITY;
					msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
					msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
					man.m_server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
				}
			}
		}

		// Players that are "jumping in" already have current players' info
		// Only send them other players who jumped in at the same time
		if( new_player->m_flags.TestMask( PlayerInfo::mJUMPING_IN ))
		{
			for( player = man.FirstPlayerInfo( sh ); player; 
					player = man.NextPlayerInfo( sh ))
			{
				if( ( new_player != player ) &&
					( player->m_jump_in_frame == new_player->m_jump_in_frame ))
				{   
					new_player_msg.m_ObjId = player->m_Skater->GetID();
					new_player_msg.m_Flags = ( player->m_flags & ( PlayerInfo::mKING_OF_THE_HILL | PlayerInfo::mHAS_ANY_FLAG ));
					new_player_msg.m_Team = player->m_Team;
					new_player_msg.m_Profile = player->m_Profile;
					new_player_msg.m_Rating = player->m_Rating;
					new_player_msg.m_VehicleControlType = player->m_VehicleControlType;
					new_player_msg.m_Score = 0;
					strcpy( new_player_msg.m_Name, player->m_Name );
					Dbg_Assert( player->mp_SkaterProfile );
					uint32 data_size = player->mp_SkaterProfile->WriteToBuffer(new_player_msg.m_AppearanceData, 
																	vMAX_APPEARANCE_DATA_SIZE, narrowband );

					msg_desc.m_Data = &new_player_msg;
					msg_desc.m_Length = sizeof( MsgNewPlayer ) - vMAX_APPEARANCE_DATA_SIZE + data_size;
					msg_desc.m_Id = MSG_ID_PLAYER_CREATE;
					msg_desc.m_Priority = Net::NORMAL_PRIORITY;
					msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
					msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
					man.m_server->EnqueueMessage( new_player->GetConnHandle(), &msg_desc );
				}
			}
		}
		else
		{
			// Send the new player the info on all current players( except himself )
			for( player = man.FirstPlayerInfo( sh ); player; 
					player = man.NextPlayerInfo( sh ))
			{
				// don't send the new player's info to the new player. We sent it separately and specially
				// also only send these messages to remote clients as we've already loaded the player here
				if( ( new_player != player ) &&
					( !new_player->IsLocalPlayer()))
				{   
					new_player_msg.m_ObjId = player->m_Skater->GetID();
					new_player_msg.m_Flags = ( player->m_flags & ( PlayerInfo::mKING_OF_THE_HILL | PlayerInfo::mHAS_ANY_FLAG ));
					new_player_msg.m_Team = player->m_Team;
					new_player_msg.m_Profile = player->m_Profile;
					new_player_msg.m_Rating = player->m_Rating;
					new_player_msg.m_VehicleControlType = player->m_VehicleControlType;
					new_player_msg.m_Score = man.GetPlayerScore( player->m_Skater->GetID() );
					strcpy( new_player_msg.m_Name, player->m_Name );
					Dbg_Assert( player->mp_SkaterProfile );
					uint32 data_size = player->mp_SkaterProfile->WriteToBuffer(new_player_msg.m_AppearanceData, 
																		vMAX_APPEARANCE_DATA_SIZE, narrowband );

					msg_desc.m_Data = &new_player_msg;
					msg_desc.m_Length = sizeof( MsgNewPlayer ) - vMAX_APPEARANCE_DATA_SIZE + data_size;
					msg_desc.m_Id = MSG_ID_PLAYER_CREATE;
					msg_desc.m_Priority = Net::NORMAL_PRIORITY;
					msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
					msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
					man.m_server->EnqueueMessage( new_player->GetConnHandle(), &msg_desc );
				}
			}
			
			MsgIntInfo level_msg;
			uint32 level;

			level = (int) man.GetNetworkLevelId();
			if( !new_player->IsLocalPlayer())
			{
				if( level == CRCD(0xb664035d,"Load_Sk5Ed_gameplay"))
				{
					Ed::CParkManager::sInstance()->WriteCompressedMapBuffer();

					man.m_server->StreamMessage( new_player->GetConnHandle(), MSG_ID_LEVEL_DATA, Ed::CParkManager::COMPRESSED_MAP_SIZE, 
							   Ed::CParkManager::sInstance()->GetCompressedMapBuffer(), "level data", vSEQ_GROUP_PLAYER_MSGS,
												 false, true );
												 
					man.m_server->StreamMessage( new_player->GetConnHandle(), MSG_ID_RAIL_DATA, Obj::GetRailEditor()->GetCompressedRailsBufferSize(), 
							   Obj::GetRailEditor()->GetCompressedRailsBuffer(), "rail data", vSEQ_GROUP_PLAYER_MSGS,
												 false, true );
												 
				}
			}

			level_msg.m_Data = (int) level;
			Dbg_Printf( "Sending player info ack request\n" );

			msg_desc.m_Data = &level_msg;
			msg_desc.m_Length = sizeof( MsgIntInfo );
			msg_desc.m_Id = MSG_ID_PLAYER_INFO_ACK_REQ;
			msg_desc.m_Priority = Net::NORMAL_PRIORITY;
			msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
			msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
			man.m_server->EnqueueMessage( new_player->GetConnHandle(), &msg_desc );
		}
		
		if( !new_player->IsObserving())
		{   
			int skater_num;

			// GJ:  the "new_info" should contain the desired
			// appearance by this point
			
			// Mick: if this is a local player in a net game
			// then it must be skater 0
			if( new_player->IsLocalPlayer() && man.InNetGame())
			{
				skater_num = 0;
			}
			else
			{
				// Otherwise, for 2P games, nonlocal net players, and P1, 
				skater_num = skate_mod->GetNumSkaters();
			}
			skater = skate_mod->add_skater( new_info->mpSkaterProfile, new_player->m_Conn->IsLocal(), 
											new_info->ObjID, skater_num );
			new_player->SetSkater( skater );
			if( !new_player->IsLocalPlayer())
			{
				skater->RemoveFromCurrentWorld();
			}

			for( player = man.FirstPlayerInfo( sh, true ); player; player = man.NextPlayerInfo( sh, true ))
			{
				if( new_player == player )
				{
					continue;
				}

				msg_desc.m_Data = &ready_msg;
				msg_desc.m_Length = sizeof( MsgReady );
				msg_desc.m_Id = MSG_ID_READY_QUERY;
				msg_desc.m_Priority = Net::NORMAL_PRIORITY;
				msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
				msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
				man.m_server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
				if( !player->IsLocalPlayer() && !player->IsObserving())
				{
					player->m_Skater->Resync();
					player->MarkAsNotReady( ready_msg.m_Time );
				}
			}
		}

		// Mark the client as "not ready" so we don't bombard him with non-important messages
		// until he has fully loaded all that he needs to load
		msg_desc.m_Data = &ready_msg;
		msg_desc.m_Length = sizeof( MsgReady );
		msg_desc.m_Id = MSG_ID_READY_QUERY;
		msg_desc.m_Priority = Net::NORMAL_PRIORITY;
		msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
		msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
		man.m_server->EnqueueMessage( new_player->GetConnHandle(), &msg_desc );
		new_player->MarkAsNotReady( ready_msg.m_Time );
        
		delete new_info;
		delete node;
	}
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::s_render_metrics_code( const Tsk::Task< Manager >& task )
{
	Lst::Search< Net::Conn > sh;
	Net::Conn* conn;
	int conn_num;
	Net::Metrics* metrics_in, *metrics_out;
	char buff[ 1024 ];
	int i;

    Manager& man = task.GetData();


    for( i = 0; i < 2; i++ )
	{
		if( man.m_client[0] && !man.m_client[0]->IsLocal())
		{
			conn_num = 0;
			
	
			for( conn = man.m_client[0]->FirstConnection( &sh ); conn;
					conn = man.m_client[0]->NextConnection( &sh ))
			{
				if( conn->IsLocal())
				{
					continue;
				}
				metrics_in = conn->GetInboundMetrics();
				metrics_out = conn->GetOutboundMetrics();
				
				sprintf( buff, "(%d) Lag: %d Resends: %d Bps: %d %d", 
						 conn_num, 
						 conn->GetAveLatency(),
						 conn->GetNumResends(),
						 metrics_in->GetBytesPerSec(),
						 metrics_out->GetBytesPerSec());
					
				Script::CStruct* pParams;
				pParams = new Script::CStruct;
				pParams->AddString( "text", buff );
				Script::RunScript( "update_net_metrics", pParams );

				delete pParams;
				conn_num++;
			}
		}
		
		if( man.m_server )
		{
			conn_num = 0;
			for( conn = man.m_server->FirstConnection( &sh ); conn;
					conn = man.m_server->NextConnection( &sh ))
			{
				if( conn->IsLocal())
				{
					continue;
				}

				metrics_in = conn->GetInboundMetrics();
				metrics_out = conn->GetOutboundMetrics();
				
				sprintf( buff, "(%d) Lag: %d Resends: %d Bps: %d %d", 
						 conn_num, 
						 conn->GetAveLatency(),
						 conn->GetNumResends(),
						 metrics_in->GetBytesPerSec(),
						 metrics_out->GetBytesPerSec());

				Script::CStruct* pParams;
				pParams = new Script::CStruct;
				pParams->AddString( "text", buff );
				Script::RunScript( "update_net_metrics", pParams );

				delete pParams;
					
				conn_num++;
			}
		}
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::s_timeout_connections_code( const Tsk::Task< Manager >& task )
{
	Lst::Search< PlayerInfo > sh;
	PlayerInfo *player, *next;
	Manager& man = task.GetData();
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	bool time_out;
	
#ifdef __PLAT_XBOX__
	static Tmr::Time s_last_link_check = 0;
	
	if(	( man.InNetMode()) &&
		(( Tmr::GetTime() - s_last_link_check ) > vLINK_CHECK_FREQUENCY ))
	{
		if( man.InNetGame())
		{
			s_last_link_check = Tmr::GetTime();
			DWORD dwStatus = XNetGetEthernetLinkStatus();
			
        	if(( dwStatus & XNET_ETHERNET_LINK_ACTIVE ) == 0 )
			{
				Front::CScreenElementManager* pManager = Front::CScreenElementManager::Instance();
				Front::CScreenElementPtr p_elem = pManager->GetElement( Script::GenerateCRC( "link_lost_dialog_anchor" ), Front::CScreenElementManager::DONT_ASSERT );
				if( !p_elem )
				{
					Script::RunScript( "create_link_unplugged_dialog" );
				}
			}
		}
		else
		{
			s_last_link_check = Tmr::GetTime();
			DWORD dwStatus = XNetGetEthernetLinkStatus();
			
        	if(( dwStatus & XNET_ETHERNET_LINK_ACTIVE ) == 0 )
			{
				Front::CScreenElementManager* pManager = Front::CScreenElementManager::Instance();
				Front::CScreenElementPtr p_elem = pManager->GetElement( CRCD(0x3b56e746,"dialog_box_anchor"), Front::CScreenElementManager::DONT_ASSERT );
				if( !p_elem )
				{
					Script::RunScript( "create_link_unplugged_front_end_dialog" );
				}
			}
		}
	}
	
#endif	

	if( man.OnServer())
	{   
		for( player = man.FirstPlayerInfo( sh, true ); player; player = next )
		{   
			time_out = false;
			next = man.NextPlayerInfo( sh, true );
			if( player->m_Conn->IsLocal())
			{
				continue;
			}

			if( player->m_Conn->TestStatus( Net::Conn::mSTATUS_READY ))
			{   
				if( player->m_Conn->GetTimeElapsedSinceCommunication() > vCONNECTION_TIMEOUT )
				{
					time_out = true;
				}
				else
				{
					if( player->m_Conn->GetNumResends() > vMAX_TOLERABLE_RESENDS )
					{
						Obj::CSkater* skater;
						bool observing;

						skater = player->m_Skater;
						observing = player->IsObserving();
						man.DropPlayer( player, vREASON_BAD_CONNECTION );
						if( !observing)
						{
							skate_mod->remove_skater( skater );
						}
						continue;
					}
				}
			}
			else
			{
				if( player->m_Conn->GetTimeElapsedSinceCommunication() > vNOT_READY_TIMEOUT )
				{
					time_out = true;
				}
			}
			if( time_out )
			{
				Obj::CSkater* skater;
				bool observing;

#ifdef __NOPT_ASSERT__
				unsigned int cur_time = Tmr::GetTime();
				unsigned int last_time = player->m_Conn->GetLastCommTime();
				Dbg_Printf( "Gamenet : Timing client out. Current time (%d) last comm time (%d)\n",
								cur_time, last_time );
#endif				
				skater = player->m_Skater;
				observing = player->IsObserving();
				man.DropPlayer( player, vREASON_TIMEOUT );
				if( !observing)
				{
					skate_mod->remove_skater( skater );
				}
			}
		}
	}
	else
	{
		Net::Conn* server_conn;
		Lst::Search< Net::Conn > sh;
		
		if( man.m_client[0] )
		{   
            server_conn = man.m_client[0]->FirstConnection( &sh );
			if( server_conn == NULL )
			{
				return;
			}
			if(( man.GetJoinState() == vJOIN_STATE_CONNECTED ) ||
			   ( man.GetJoinState() == vJOIN_STATE_WAITING_FOR_START_INFO ) || 
			   ( man.GetJoinState() == vJOIN_STATE_GOT_PLAYERS ))
			{
				if( server_conn->GetTimeElapsedSinceCommunication() > vCONNECTION_TIMEOUT )
				{  
#ifdef __NOPT_ASSERT__
					unsigned int cur_time = Tmr::GetTime();
					unsigned int last_time = server_conn->GetLastCommTime();
#endif					
#ifdef __NOPT_ASSERT__
					Dbg_Printf( "Gamenet : Timing server out. Current time (%d) last comm time (%d)\n",
									cur_time, last_time );
#endif              
					
					if( skate_mod->m_cur_level == CRCD(0x9f2bafb7,"Load_Skateshop"))
					{						
                        // If we got partially in, cancel the join and remove partially-loaded players
						// Shut the client down so that it doesn't bombard the server ip with messages
						
						Script::RunScript( "ShowJoinTimeoutNotice" );

						//man.CancelJoinServer();
						man.m_join_state_task->Remove();
						man.SetJoinState( vJOIN_STATE_FINISHED );
					}
					else
					{
// On the xbox, don't show the lost conn dialog if the quit dialog box is up					
#ifdef __PLAT_XBOX__
						Front::CScreenElementManager* pManager = Front::CScreenElementManager::Instance();
						Front::CScreenElementPtr p_elem = pManager->GetElement( CRCD(0x4c8bf619,"quit_dialog_anchor"), Front::CScreenElementManager::DONT_ASSERT );
						if( !p_elem )
#endif
						{
							Script::RunScript( "CreateLostConnectionDialog" );
	
							man.CleanupPlayers();
							man.ClientShutdown();
						}
					}
				}
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::s_join_timeout_code( const Tsk::Task< Manager >& task )
{
	Manager& man = task.GetData();

	if(( Tmr::GetTime() - man.m_join_start_time ) > vJOIN_TIMEOUT )
	{
		//Mdl::FrontEnd* front = Mdl::FrontEnd::Instance();
		
		//man.CancelJoinServer();
		//man.SetJoinState( vJOIN_STATE_FINISHED );
        
		man.m_join_state_task->Remove();
		Script::RunScript( "ShowJoinTimeoutNotice" );
	}
}

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

void				Manager::SetNetworkMode( NetworkMode mode )
{
	m_flags.ClearMask( mNETWORK_MODE);
	
	if( mode == vLAN_MODE )
	{
		Dbg_Printf( "************* Setting Lan Mode\n" );
		m_flags.SetMask( mLAN );
	}
	else if( mode == vINTERNET_MODE )
	{
		Dbg_Printf( "************* Setting Internet Mode\n" );
		m_flags.SetMask( mINTERNET );
	}
	else
	{
		Dbg_Printf( "************* Clearing Net Mode\n" );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				Manager::SetJoinMode( JoinMode mode )
{
	m_join_mode = mode;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

JoinMode			Manager::GetJoinMode( void )
{
	return m_join_mode;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				Manager::SetHostMode( HostMode mode )
{
	// Only accept if it is actually a change in modes
	if( mode == m_host_mode )
	{
		return;
	}
	
	m_host_mode = mode;
	
	switch( m_host_mode )
	{
		case vHOST_MODE_SERVE:
		{
			// Do nothing. This is the standard mode
			Dbg_Printf( "Got mode serve\n" );
			break;
		}
		case vHOST_MODE_AUTO_SERVE:
		{
			RequestObserverMode();
			SpawnAutoServer();

			// Should start a game and also spawn a task that starts new games
			// after X seconds in a lobby
			break;
		}
		
		case vHOST_MODE_FCFS:
		{
			RequestObserverMode();
			ChooseNewServerPlayer();

			// Should start a game and also spawn a task that starts new games
			// after X seconds in a lobby
			break;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

	HostMode			Manager::GetHostMode( void )
{
	return m_host_mode;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				Manager::SetServerMode( bool on )
{
	if( on )
	{
		m_flags.SetMask( mSERVER );
	}
	else
	{
		m_flags.ClearMask( mSERVER );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				Manager::InNetMode( void )
{
	return m_flags.TestMask( mNETWORK_MODE);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				Manager::InNetGame( void )
{
	#ifdef	__NOPT_ASSERT__
	// Mick:  Pretend we are in a net game when we are actually in
	// regular single player
	// This allows designer to test net games without starting a server
	if (Script::GetInt(CRCD(0xcbc1a46,"fake_net")))
	{
		return true;
	}
	#endif
	
	
	if( !InNetMode())
	{
		return false;
	}

	if( m_server && !m_server->IsLocal())
	{
		return true;
	}

	if( m_client[0] && !m_client[0]->IsLocal())
	{
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				Manager::InLanMode( void )
{
	return m_flags.TestMask( mLAN );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				Manager::InInternetMode( void )
{
    return m_flags.TestMask( mINTERNET );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				Manager::OnServer( void )
{
	return m_flags.TestMask( mSERVER );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				Manager::OnClient( void )
{
	return m_flags.TestMask( mCLIENT );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int					Manager::GetSkillLevel( void )
{
	uint32 checksum;
	Script::CStruct* pStructure;
		
	pStructure = m_network_preferences.GetPreference( Script::GenerateCRC("skill_level") );
	pStructure->GetChecksum( "Checksum", &checksum, true );
	if( checksum == Script::GenerateCRC( "num_1" ))
	{
		return vSKILL_LEVEL_1;
	}
	else if( checksum == Script::GenerateCRC( "num_2" ))
	{
		return vSKILL_LEVEL_2;
	}
	else if( checksum == Script::GenerateCRC( "num_3" ))
	{
		return vSKILL_LEVEL_3;
	}                                              
	else if( checksum == Script::GenerateCRC( "num_4" ))
	{
		return vSKILL_LEVEL_4;
	}                                              
	else if( checksum == Script::GenerateCRC( "num_5" ))
	{
		return vSKILL_LEVEL_5;
	}                                              

	return vSKILL_LEVEL_DEFAULT;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int					Manager::GetMaxPlayers( void )
{
	uint32 checksum;
	int max_players = 2;
	Script::CStruct* pStructure;
		
	pStructure = m_network_preferences.GetPreference( Script::GenerateCRC("num_players") );
	pStructure->GetChecksum( "Checksum", &checksum, true );
	if( checksum == Script::GenerateCRC( "num_2" ))
	{
		max_players = 2;
	}
	else if( checksum == Script::GenerateCRC( "num_3" ))
	{
		max_players = 3;
	}
	else if( checksum == Script::GenerateCRC( "num_4" ))
	{
		max_players = 4;
	}                                              
	else if( checksum == Script::GenerateCRC( "num_5" ))
	{
		max_players = 5;
	}
	else if( checksum == Script::GenerateCRC( "num_6" ))
	{
		max_players = 6;
	}
	else if( checksum == Script::GenerateCRC( "num_7" ))
	{
		max_players = 7;
	}
	else if( checksum == Script::GenerateCRC( "num_8" ))
	{
		max_players = 8;
	}

	return max_players;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				Manager::CreateTeamFlags( int num_teams )
{
	Script::CStruct* pParams;
	
	// First destroy all team flags
	Script::RunScript( "Kill_Team_Flags" );

	// Now only create the number of flags we need
	if( num_teams == 0 )
	{
		return;
	}

	pParams = new Script::CStruct;
	pParams->AddInteger( "num_teams", num_teams );
	Script::RunScript( "Create_Team_Flags", pParams );
	delete pParams;
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int					Manager::GetNumTeams( void )
{
	uint32 checksum;
	int num_teams = vMAX_TEAMS;
	Script::CStruct* pStructure;
		
	pStructure = m_network_preferences.GetPreference( Script::GenerateCRC("team_mode") );
	pStructure->GetChecksum( "Checksum", &checksum, true );
	if( checksum == Script::GenerateCRC( "teams_none" ))
	{
		num_teams = 0;
	}
	else if( checksum == Script::GenerateCRC( "teams_two" ))
	{
		num_teams = 2;
	}
	else if( checksum == Script::GenerateCRC( "teams_three" ))
	{
		num_teams = 3;
	}                                              
	else if( checksum == Script::GenerateCRC( "teams_four" ))
	{
		num_teams = 4;
	}

	return num_teams;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int					Manager::NumTeamMembers( int team )
{
	int num_players;
	Lst::Search< PlayerInfo > sh;
	PlayerInfo* player;
	Lst::Search< NewPlayerInfo > new_sh;
	NewPlayerInfo* np;

	num_players = 0;

	for( player = FirstPlayerInfo( sh ); player; player = NextPlayerInfo( sh ))
	{
		if( player->m_Team == team )
		{
			num_players++;
		}
	}

	for( np = new_sh.FirstItem( m_new_players ); np; np = new_sh.NextItem())
	{
		// Pending players count, observers don't
		if(	( np->Flags & PlayerInfo::mPENDING_PLAYER ) ||
			( np->Flags & PlayerInfo::mJUMPING_IN ) ||
			!( np->Flags & PlayerInfo::mOBSERVER ))
		{
			if( np->Team == team )
			{
				num_players++;
			}
		}                 
	}

	
	return num_players;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				Manager::SetMaxPlayers( int max_players )
{
	Script::CStruct* pTempStructure;
	Prefs::Preferences* pPreferences;
	uint32 checksum;
	char check_string[32];
	char name_string[32];
	
	Dbg_Assert(( max_players >= 2 ) && ( max_players <= vMAX_PLAYERS ));
	
	sprintf( check_string, "num_%d", max_players );
	checksum = Script::GenerateCRC( check_string );

#if ENGLISH == 0
	sprintf( name_string, "%d %s", max_players, Script::GetLocalString( "netmenu_str_players" ));
#else
	sprintf( name_string, "%d Players", max_players );
#endif

	pTempStructure = new Script::CStruct;
	pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, 
								  name_string );
	pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, (int) checksum );
	pPreferences = GetNetworkPreferences();
	pPreferences->SetPreference( Script::GenerateCRC( "num_players"), pTempStructure );
}
                                        
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int					Manager::GetMaxObservers( void )
{
	uint32 checksum;
	int max_observers = 2;
	Script::CStruct* pStructure;
		
	pStructure = m_network_preferences.GetPreference( Script::GenerateCRC("num_observers"));
	pStructure->GetChecksum( "Checksum", &checksum, true );
	if( checksum == Script::GenerateCRC( "num_0" ))
	{
		max_observers = 0;
	}
	else if( checksum == Script::GenerateCRC( "num_1" ))
	{
		max_observers = 1;
	}
	else if( checksum == Script::GenerateCRC( "num_2" ))
	{
		max_observers = 2;
	}
	else if( checksum == Script::GenerateCRC( "num_3" ))
	{
		max_observers = 3;
	}
	else if( checksum == Script::GenerateCRC( "num_4" ))
	{
		max_observers = 4;
	}                                              
	else if( checksum == Script::GenerateCRC( "num_5" ))
	{
		max_observers = 5;
	}
	else if( checksum == Script::GenerateCRC( "num_6" ))
	{
		max_observers = 6;
	}
	else if( checksum == Script::GenerateCRC( "num_7" ))
	{
		max_observers = 7;
	}
	else if( checksum == Script::GenerateCRC( "num_8" ))
	{
		max_observers = 8;
	}                                              

	return max_observers;
}
                                        
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				Manager::SetMaxObservers( int max_observers )
{
	Script::CStruct* pTempStructure;
	Prefs::Preferences* pPreferences;
	uint32 checksum;
	char check_string[32];
	char name_string[32];
	
    sprintf( check_string, "num_%d", max_observers );
	checksum = Script::GenerateCRC( check_string );
	
#if ENGLISH == 0
	if( max_observers == 0 )
	{
		sprintf( name_string, Script::GetLocalString( "netmenu_str_no_observers" ));
	}
	else
	{
		if( max_observers == 1 )
		{
			sprintf( name_string, "1 %s", Script::GetLocalString( "netmenu_str_observer" ));
		}
		else
		{
			sprintf( name_string, "%d %s", max_observers, Script::GetLocalString( "netmenu_str_observers" ));
		}
	}
#else
	if( max_observers == 0 )
	{
		sprintf( name_string, "No Observers" );
	}
	else
	{
		if( max_observers == 1 )
		{
			sprintf( name_string, "1 Observer" );
		}
		else
		{
			sprintf( name_string, "%d Observers", max_observers );
		}
	}
#endif
	pTempStructure = new Script::CStruct;
	pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, 
								  name_string );
	pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, (int) checksum );
	pPreferences = GetNetworkPreferences();
	pPreferences->SetPreference( Script::GenerateCRC( "num_observers"), pTempStructure );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*				Manager::GetPassword( void )
{
	Prefs::Preferences* pPreferences;
	Script::CStruct* pStructure;
	const char* password;
		
	pPreferences = GetNetworkPreferences();
	pStructure = pPreferences->GetPreference( Script::GenerateCRC("password") );
	pStructure->GetText( "ui_string", &password, true );

	return (char*) password;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*				Manager::GetLevelName( bool get_created_park_name )
{
	Script::CArray* pArray = Script::GetArray("level_select_menu_level_info");
	Dbg_Assert( pArray );

	for ( int i = 0; i < (int)pArray->GetSize(); i++ )
	{   
		uint32 checksum;
		Script::CStruct* pStructure = pArray->GetStructure( i );
		Dbg_Assert( pStructure );

        pStructure->GetChecksum( "level", &checksum );
		if( GetNetworkLevelId() == checksum )
		{
			const char* level_name;

			if( get_created_park_name && ( checksum == CRCD(0xb664035d,"Load_Sk5Ed_gameplay")))
			{
				return (char*) Ed::CParkManager::sInstance()->GetParkName();
			}
			pStructure->GetText( "text", &level_name, true );
			return (char *) level_name;
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*				Manager::GetGameModeName( void )
{
	Script::CArray* pArray = Script::GetArray("mode_info");
	Dbg_Assert( pArray );

	for ( int i = 0; i < (int)pArray->GetSize(); i++ )
	{   
		uint32 checksum;
		uint32 game_mode_id;
		Script::CStruct* pStructure = pArray->GetStructure( i );
		Dbg_Assert( pStructure );
	
		game_mode_id = GetGameTypeFromPreferences();//skate_mod->GetGameMode()->GetNameChecksum();
		pStructure->GetChecksum( "checksum", &checksum );
		if( game_mode_id == checksum )
		{
			const char* mode_name;

			pStructure->GetText( "name", &mode_name, true );
			return (char *) mode_name;
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				Manager::PlayerCollisionEnabled( void )
{
	uint32 collision_pref;
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	 
	if(	( GameIsOver()) ||
		( skate_mod->GetGameMode()->GetNameChecksum() == CRCD( 0x34861a16, "freeskate" )) ||
		( skate_mod->GetGameMode()->GetNameChecksum() == CRCD( 0x9d65d0e7, "horse" )))
	{
		// Allow collision in lobbies now, if the option is on
		if( skate_mod->GetGameMode()->GetNameChecksum() != CRCD(0x1c471c60,"netlobby"))
		{
			return false;
		}
	}                

	// Option is always on in splitscreen games
	if( CFuncs::ScriptInSplitScreenGame( NULL, NULL ))
	{
		return true;
	}

	// If this option is on, we definitely collide
	collision_pref = m_network_preferences.GetPreferenceChecksum( CRCD( 0x43ee978, "player_collision"), CRCD( 0x21902065, "checksum" ));
	if( collision_pref == CRCD( 0xf81bc89b, "boolean_true" ))
	{
		return true;
	}

	// Even if it's turned off, we might still enable collision because player collision is the basis
	// of some game modes
	if( ( skate_mod->GetGameMode()->GetNameChecksum() == CRCD( 0xf9d5d933, "netslap" )) ||
		( skate_mod->GetGameMode()->GetNameChecksum() == CRCD( 0xca1f360f, "slap" )))
	{
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ShouldDisplayTeamScores( void )
{
	uint32 collision_pref;
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	 
	// Always just show players in lobbies
	if( skate_mod->GetGameMode()->GetNameChecksum() == CRCD( 0x1c471c60, "netlobby" ))
	{
		return false;
	}

	if(	skate_mod->GetGameMode()->IsTeamGame() == false )
	{
		return false;
	}                

	// If this option is on, we definitely collide
	collision_pref = m_network_preferences.GetPreferenceChecksum( CRCD( 0x92a6a8c8, "score_display"), CRCD( 0x21902065, "checksum") );
	return ( collision_pref == CRCD( 0xd071112f, "score_teams" ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Net::Server*		Manager::SpawnServer( bool local, bool secure )
{
	
	int flags;

	Net::Manager * net_man = Net::Manager::Instance();
	Mlp::Manager * mlp_manager = Mlp::Manager::Instance();

	Dbg_MsgAssert( m_server == NULL,( "Failed to spawn new server. Old server still running!\n" ));

	flags = Net::App::mBROADCAST |
			Net::App::mDYNAMIC_RESEND |
			Net::App::mACCEPT_FOREIGN_CONN;

	if( local )
	{
		flags |= Net::App::mLOCAL;
	}
	if( secure )
	{
		flags |= Net::App::mSECURE;
	}

	m_server = net_man->CreateNewAppServer( 0, "Skate4", vMAX_CONNECTIONS, 
											vHOST_PORT, inet_addr( net_man->GetLocalIP()), 
											flags );
    
	SetHostMode( vHOST_MODE_SERVE );

	if( m_server )
	{
		mlp_manager->AddLogicTask( *m_server_add_new_players_task );
		// These handlers will be removed after the server actually starts (i.e. after the lobby shuts down)
		m_server->m_Dispatcher.AddHandler( Net::MSG_ID_CONNECTION_REQ, 
											s_handle_connection, 
											Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN,
											this, Net::LOWEST_PRIORITY );
		m_server->m_Dispatcher.AddHandler( MSG_ID_JOIN_REQ,
										   s_handle_join_request,
										   Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN,
										   this );
		m_server->m_Dispatcher.AddHandler( Net::MSG_ID_FIND_SERVER, 
											s_handle_find_server, 
											Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN , this );
		m_server->m_Dispatcher.AddHandler( Net::MSG_ID_DISCONN_REQ, s_handle_disconn_request, 
										   Net::mHANDLE_LATE, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_OBSERVE, s_handle_observe, 
										   Net::mHANDLE_LATE, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_READY_RESPONSE, s_handle_ready_response, 
											Net::mHANDLE_LATE, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_PLAYER_INFO_ACK, s_handle_player_info_ack,
										   0, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_MOVED_TO_RESTART, s_handle_player_restarted,
										   0, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_RUN_HAS_ENDED, s_handle_run_ended, 0, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_FCFS_START_GAME, s_handle_fcfs_request, Net::mHANDLE_LATE, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_FCFS_BAN_PLAYER, s_handle_fcfs_request, Net::mHANDLE_LATE, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_FCFS_CHANGE_LEVEL, s_handle_fcfs_request, Net::mHANDLE_LATE, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_FCFS_TOGGLE_PROSET, s_handle_fcfs_request, Net::mHANDLE_LATE, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_FCFS_TOGGLE_GOAL_SELECTION, s_handle_fcfs_request, Net::mHANDLE_LATE, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_FCFS_END_GAME, s_handle_fcfs_request, Net::mHANDLE_LATE, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_FCFS_SET_NUM_TEAMS, s_handle_fcfs_request, Net::mHANDLE_LATE, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_REQUEST_CHANGE_TEAM, s_handle_team_change_request, Net::mHANDLE_LATE, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_REQUEST_LEVEL, s_handle_request_level, Net::mHANDLE_LATE, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_BEAT_GOAL, s_handle_beat_goal, Net::mHANDLE_LATE, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_STARTED_GOAL, s_handle_started_goal, Net::mHANDLE_LATE, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_LEVEL_DATA, s_handle_level_data, 0, this, Net::HIGHEST_PRIORITY );
		m_server->m_Dispatcher.AddHandler( MSG_ID_RAIL_DATA, s_handle_rail_data, 0, this, Net::HIGHEST_PRIORITY );
		m_server->m_Dispatcher.AddHandler( MSG_ID_CHALLENGE_RESPONSE, s_handle_challenge_response, 0, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_CHEAT_CHECKSUM_RESPONSE, s_handle_cheat_checksum_response,
											  0, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_REQUEST_RAILS_DATA, s_handle_level_data_request, 0, this );
		m_server->m_Dispatcher.AddHandler( MSG_ID_REQUEST_GOALS_DATA, s_handle_level_data_request, 0, this );

		m_last_score_update = 0;
		m_waiting_for_game_to_start = false;
        	
		RandomizeSkaterStartingPoints();
		m_observer_input_handler = NULL;		// record and handle button inputs
	}
	else
	{
#ifdef __NOPT_ASSERT__
		int error = net_man->GetLastError();
		Dbg_Printf( "Spawn Server: Error %d\n", error );
#endif
		// Here output appropriate error message to the screen
	}

    return m_server;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Net::Client*		Manager::SpawnClient( bool broadcast, bool local, bool secure, int index )
{
	

	Net::Manager * net_man = Net::Manager::Instance();
	int flags;

	Dbg_MsgAssert( m_client[index] == NULL,( "Failed to spawn new client. Old client still running!\n" ));

	flags = 0;
	if( broadcast )
	{
		flags |= Net::App::mBROADCAST;
	}
	if( local )
	{
		flags |= Net::App::mLOCAL;
	}
	else
	{
		flags |= Net::App::mDYNAMIC_RESEND | Net::App::mACCEPT_FOREIGN_CONN;
	}
	if( secure )
	{
		flags |= Net::App::mSECURE;
	}

	m_client[index] = net_man->CreateNewAppClient( index, "Skate4", 
											vJOIN_PORT, inet_addr( net_man->GetLocalIP()),
											flags );
		
	if(( index == 0) && ( m_client[index] ))
	{  
		m_latest_ready_query = 0;
		m_proset_flags = 0;
		m_cam_player = NULL;
		SetReadyToPlay(false);
	}
	else
	{
#ifdef __NOPT_ASSERT__
		int error = net_man->GetLastError();
		Dbg_Printf( "Spawn Client: Error %d\n", error );
#endif
		// Here output appropriate error message to the screen
	}

	m_observer_input_handler = NULL;
	m_game_over = false;
	m_game_pending = false;
	return m_client[index];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Net::Client*		Manager::SpawnMatchClient( void )
{
	Net::Manager * net_man = Net::Manager::Instance();
	int flags;

	flags = Net::App::mBROADCAST | Net::App::mDYNAMIC_RESEND | Net::App::mACCEPT_FOREIGN_CONN | Net::App::mSECURE;

	m_match_client = net_man->CreateNewAppClient( 0, "Match Skate4", 
											vJOIN_PORT, inet_addr( net_man->GetLocalIP()),
											flags );
	m_match_client->m_Dispatcher.AddHandler( 	Net::MSG_ID_SERVER_RESPONSE, 
												s_handle_server_response, 
												Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN,
												this );

	return m_match_client;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SpawnAutoServer( void )
{
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();

	s_auto_serve_start_time = Tmr::GetTime();
	mlp_man->AddLogicTask( *m_auto_server_task );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::JoinServer( bool observe_only, unsigned long ip, unsigned short port, int index )
{
    
	Net::Conn* conn;
	MsgConnectInfo msg;
	Net::MsgDesc msg_desc;
	uint32 size;
	const char* network_id;
	Script::CStruct* pStructure;
	Prefs::Preferences* pPreferences;
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();
	//Net::Manager * net_man = Net::Manager::Instance();
		
	Dbg_MsgAssert( m_client[index] != NULL,( "Can't join server : Client has not been spawned\n" ));

#ifndef __PLAT_NGC__
	//Dbg_Printf( "Joining server at %s %d\n", inet_ntoa( *(struct in_addr*) &ip ), port );
#endif		// __PLAT_NGC__

	pPreferences = GetNetworkPreferences();
	pStructure = pPreferences->GetPreference( Script::GenerateCRC("network_id") );
	pStructure->GetText( "ui_string", &network_id, true );

	if( m_client[index]->IsLocal())
	{
		Net::Conn* server_conn;

		Dbg_Assert( m_server );

		server_conn = m_server->NewConnection( ip, port, Net::Conn::mLOCAL );
		conn = m_client[index]->NewConnection( ip, port, Net::Conn::mLOCAL );
        m_client[index]->AliasConnections( server_conn, conn );
	}
	else
	{   
		m_client[index]->ConnectToServer( ip, port );
		conn = m_client[index]->NewConnection( ip, port );

		m_join_start_time = Tmr::GetTime();
		mlp_man->AddLogicTask( *m_join_timeout_task );
		SetJoinState( vJOIN_STATE_CONNECTING );
		mlp_man->AddLogicTask( *m_join_state_task );
		
		SetJoinIP( ip );
		SetJoinPort( port );
	}   
	
	msg.m_Observer = observe_only;
	size = 0;
	
	msg.m_Version = vVERSION_NUMBER;
	msg.m_Password[0] = '\0';
	msg.m_WillingToWait = 0;
	
	msg_desc.m_Data = &msg;
	msg_desc.m_Length = sizeof(MsgConnectInfo);
	msg_desc.m_Id = Net::MSG_ID_CONNECTION_REQ;

	m_client[index]->EnqueueMessageToServer( &msg_desc );
	m_client[index]->SendData();

	m_client[index]->m_Dispatcher.AddHandler( 	MSG_ID_PROCEED_TO_PLAY, 
												s_handle_client_proceed, Net::mHANDLE_LATE, this );
	m_client[index]->m_Dispatcher.AddHandler( 	MSG_ID_OBSERVE_PROCEED, 
												s_handle_observe_proceed, Net::mHANDLE_LATE, this );
	m_client[index]->m_Dispatcher.AddHandler( 	MSG_ID_OBSERVE_REFUSED, 
												s_handle_observe_refused, Net::mHANDLE_LATE, this );
	m_client[index]->m_Dispatcher.AddHandler( 	MSG_ID_JOIN_REFUSED, 
												s_handle_join_refused, Net::mHANDLE_LATE, this );
	m_client[index]->m_Dispatcher.AddHandler( 	MSG_ID_JOIN_PROCEED, 
												s_handle_join_proceed, Net::mHANDLE_LATE, this );
	m_client[index]->m_Dispatcher.AddHandler( 	MSG_ID_READY_QUERY, s_handle_ready_query, 
												Net::mHANDLE_LATE, this );
	m_client[index]->m_Dispatcher.AddHandler( 	MSG_ID_READY_RESPONSE, s_handle_ready_response, 
												Net::mHANDLE_LATE, this );
	m_client[index]->m_Dispatcher.AddHandler( 	MSG_ID_PLAYER_INFO_ACK_REQ, s_handle_player_info_ack_req, 
												Net::mHANDLE_LATE, this );
	m_client[index]->m_Dispatcher.AddHandler(	MSG_ID_KING_DROPPED_CROWN, s_handle_dropped_crown, 0, this );
	m_client[index]->m_Dispatcher.AddHandler(	MSG_ID_PLAYER_DROPPED_FLAG, s_handle_dropped_flag, 0, this );
	m_client[index]->m_Dispatcher.AddHandler(	MSG_ID_PANEL_MESSAGE, s_handle_panel_message, 
												Net::mHANDLE_LATE, this );
	m_client[index]->m_Dispatcher.AddHandler( 	MSG_ID_AUTO_SERVER_NOTIFICATION, s_handle_auto_server_notification,
												Net::mHANDLE_LATE, this );
	m_client[index]->m_Dispatcher.AddHandler( 	MSG_ID_FCFS_ASSIGNMENT, s_handle_fcfs_assignment,
												Net::mHANDLE_LATE, this );
	m_client[index]->m_Dispatcher.AddHandler( 	MSG_ID_TEAM_CHANGE, s_handle_team_change,
												Net::mHANDLE_LATE, this );
	m_client[index]->m_Dispatcher.AddHandler( 	MSG_ID_SET_NUM_TEAMS, s_handle_set_num_teams,
												Net::mHANDLE_LATE, this );
	m_client[index]->m_Dispatcher.AddHandler( 	MSG_ID_CHAT, s_handle_chat, Net::mHANDLE_LATE );
	m_client[index]->m_Dispatcher.AddHandler(	MSG_ID_CHALLENGE, s_handle_challenge,
												Net::mHANDLE_LATE, this ); 
	
	if( index == 0 )
	{   
		m_client[0]->m_Dispatcher.AddHandler( MSG_ID_SELECT_GOALS, s_handle_select_goals, 0, this );
		m_client[0]->m_Dispatcher.AddHandler( MSG_ID_GAME_INFO, s_handle_game_info, 0, this );
		m_client[0]->m_Dispatcher.AddHandler( MSG_ID_KILL_TEAM_FLAGS, s_handle_kill_flags, 0, this );
		m_client[0]->m_Dispatcher.AddHandler( MSG_ID_BEAT_GOAL, s_handle_beat_goal_relay, 0, this );
		m_client[0]->m_Dispatcher.AddHandler( MSG_ID_STARTED_GOAL, s_handle_started_goal_relay, 0, this );
		m_client[0]->m_Dispatcher.AddHandler( MSG_ID_TOGGLE_PROSET, s_handle_toggle_proset, 0, this );
		m_client[0]->m_Dispatcher.AddHandler( MSG_ID_CHANGE_LEVEL, s_handle_change_level, 0, this,
												Net::HIGHEST_PRIORITY );
		m_client[0]->m_Dispatcher.AddHandler( MSG_ID_CHANGE_LEVEL, s_handle_new_level, 0, this,
												Net::HIGHEST_PRIORITY );
		m_client[0]->m_Dispatcher.AddHandler( MSG_ID_LEVEL_DATA, s_handle_level_data, 0, this,
												Net::HIGHEST_PRIORITY );
		m_client[0]->m_Dispatcher.AddHandler( MSG_ID_RAIL_DATA, s_handle_rail_data, Net::mHANDLE_CRC_MISMATCH, this,
												Net::HIGHEST_PRIORITY );
		m_client[0]->m_Dispatcher.AddHandler( MSG_ID_GOALS_DATA, s_handle_goals_data, Net::mHANDLE_CRC_MISMATCH, this,
												Net::HIGHEST_PRIORITY );
		m_client[0]->m_Dispatcher.AddHandler( MSG_ID_CHEAT_CHECKSUM_REQUEST, s_handle_cheat_checksum_request,
											  0, this );
		
		
		if( !m_client[0]->IsLocal())
		{
			m_client[index]->m_Dispatcher.AddHandler( MSG_ID_PLAYER_CREATE, s_handle_new_player, 0, this );
			m_client[index]->m_Dispatcher.AddHandler( MSG_ID_JOIN_ACCEPTED, s_handle_join_accepted, 0, this );
			
		}

		m_client[index]->m_Dispatcher.AddHandler( MSG_ID_WAIT_N_SECONDS, s_handle_wait_n_seconds, 0, this );
		m_client[index]->m_Dispatcher.AddHandler( MSG_ID_NEW_KING, s_handle_new_king, 0, this );
		m_client[index]->m_Dispatcher.AddHandler( MSG_ID_CAPTURED_FLAG, s_handle_captured_flag, 0, this );
		m_client[index]->m_Dispatcher.AddHandler( MSG_ID_STOLE_FLAG, s_handle_stole_flag, 0, this );
		m_client[index]->m_Dispatcher.AddHandler( MSG_ID_TOOK_FLAG, s_handle_took_flag, 0, this );
		m_client[index]->m_Dispatcher.AddHandler( MSG_ID_RETRIEVED_FLAG, s_handle_retrieved_flag, 0, this );
		m_client[index]->m_Dispatcher.AddHandler( MSG_ID_GAME_OVER, s_handle_game_over, 0, this );
		m_client[index]->m_Dispatcher.AddHandler( MSG_ID_END_GAME, s_handle_end_game, 0, this );
		
		// Add this low-priority object update handler just to advance the dispatcher's stream pointer.
		// Since we no longer send the size of the object update stream with the stream, we need
		// some code to return to the dispatcher the length of the message. If, because of timing,
		// we get one of these messages before our Mdl::Skate object update handler is instantiated
		// this handler will figure out the length of the message and pass it back
		m_client[index]->m_Dispatcher.AddHandler( MSG_ID_OBJ_UPDATE_STREAM, s_handle_object_update, 
												  Net::mHANDLE_LATE | Net::mHANDLE_FOREIGN, this,
												  Net::HIGHEST_PRIORITY );
	}

	SetObserverCommand( vOBSERVER_COMMAND_NONE );
}
    
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::ReattemptJoin( Net::App* client )
{
	MsgConnectInfo msg;
	Net::MsgDesc msg_desc;
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();
		
	msg.m_Observer = ( GetJoinMode() == vJOIN_MODE_OBSERVE );
	msg.m_Version = vVERSION_NUMBER;
	msg.m_WillingToWait = 1;
	msg.m_Password[0] = '\0';
	
	msg_desc.m_Data = &msg;
	msg_desc.m_Length = sizeof(MsgConnectInfo);
	msg_desc.m_Id = Net::MSG_ID_CONNECTION_REQ;
	client->EnqueueMessageToServer( &msg_desc );
	client->SendData();
	
	// Reset join timeout
	m_join_start_time = Tmr::GetTime();
	mlp_man->AddLogicTask( *m_join_timeout_task );

	SetJoinState( vJOIN_STATE_JOINING );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::CancelJoinServer( void )
{
	Net::Client* client;
    Net::MsgDesc msg_desc;

	// These are "finished" states. Don't redundantly cancel our join if we've already cancelled
	if(	( GetJoinState() == vJOIN_STATE_FINISHED ) ||
		( GetJoinState() == vJOIN_STATE_CONNECTED ) ||
		( GetJoinState() == vJOIN_STATE_REFUSED ))
	{
		return;
	}

	client = GetClient( 0 );
	Dbg_Assert( client );

    // First, wait for any pending network tasks to finish before sending out final disconn message
#ifdef __PLAT_NGPS__
	//client->WaitForAsyncCallsToFinish();
#endif  		
	Dbg_Printf( "Leaving server\n" );
			
	// Send off last message, notifying server that I'm about to quit
	msg_desc.m_Id = Net::MSG_ID_DISCONN_REQ;
	client->EnqueueMessageToServer( &msg_desc );
	// Wake the network thread up to send the data
	client->SendData();
		
	// Wait for that final send to complete
#ifdef __PLAT_NGPS__
	//client->WaitForAsyncCallsToFinish();
#endif  
    
	ClientShutdown();

	m_join_timeout_task->Remove();

	// We may have made it halfway in, so free up any pending players.
	free_all_pending_players(); 

#ifdef __PLAT_NGPS__
	if( mpBuddyMan->IsLoggedIn())
	{
		mpBuddyMan->SetStatusAndLocation( GP_CHATTING, (char*) Script::GetString( "homie_status_chatting" ), 
														mpLobbyMan->GetLobbyName());
	}
#endif
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::ReattemptJoinWithPassword( char* password )
{
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();
	Net::Manager * net_man = Net::Manager::Instance();
	Script::CStruct* pStructure;
	Prefs::Preferences* pPreferences;
	const char* network_id;
	MsgJoinInfo msg;
	Net::MsgDesc msg_desc;
	int size;
	bool ignore_face_data;

	Dbg_Printf( "************* Trying password : %s\n", password );
		
	pPreferences = GetNetworkPreferences();
	pStructure = pPreferences->GetPreference( Script::GenerateCRC("network_id") );
	pStructure->GetText( "ui_string", &network_id, true );

#ifdef __PLAT_NGPS__
	msg.m_Profile = mpBuddyMan->GetProfile();
	msg.m_Rating = mpStatsMan->GetStats()->GetRating();
#endif
	msg.m_Observer = ( GetJoinMode() == vJOIN_MODE_OBSERVE );
	msg.m_Version = vVERSION_NUMBER;
	msg.m_WillingToWait = 1;
	if(	net_man->GetConnectionType() == Net::vCONN_TYPE_MODEM )
	{
		msg.m_Broadband = 0;
		ignore_face_data = true;
	}
	else
	{
		msg.m_Broadband = 1;
		ignore_face_data = false;
	}
	strcpy( msg.m_Name, network_id );
	strcpy( msg.m_Password, password );
	size = 0;
	if( !msg.m_Observer )
	{
		// GJ:  transmit the way you look (slot 0 of the
		// skater profile manager) to the server
		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		Obj::CSkaterProfile* pSkaterProfile = skate_mod->GetProfile(0);
		Net::Conn *server_conn;
		Lst::Search< Net::Conn > sh;
	
		server_conn = m_client[0]->FirstConnection( &sh );
		if( server_conn->GetBandwidthType() == Net::Conn::vNARROWBAND )
		{
			ignore_face_data = true;
		}
	
		size = pSkaterProfile->WriteToBuffer(msg.m_AppearanceData, vMAX_APPEARANCE_DATA_SIZE,
												ignore_face_data );
		Dbg_Assert( size < vMAX_APPEARANCE_DATA_SIZE );
		Dbg_Printf("\n\n******************* MsgJoinInfo appearance data size = %d %d\n", size, sizeof(MsgJoinInfo) - vMAX_APPEARANCE_DATA_SIZE + size);
	}

	msg_desc.m_Data = &msg;
	msg_desc.m_Length = sizeof(MsgJoinInfo) - vMAX_APPEARANCE_DATA_SIZE + size;
	msg_desc.m_Id = MSG_ID_JOIN_REQ;
	msg_desc.m_Queue = Net::QUEUE_IMPORTANT;
	m_client[0]->EnqueueMessageToServer( &msg_desc );
    
	//m_client[0]->SendData();

	// Reset join timeout
	m_join_start_time = Tmr::GetTime();
	mlp_man->AddLogicTask( *m_join_timeout_task );

	SetJoinState( vJOIN_STATE_JOINING_WITH_PASSWORD );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::ServerShutdown( void )
{
	
	Net::Manager * net_man = Net::Manager::Instance();

	Dbg_Message( "Shutting Server Down\n" );

	if( m_server )
	{
#ifndef __PLAT_XBOX__
#ifndef __PLAT_NGC__
		if( !m_server->IsLocal() && InInternetMode())
		{
			mpLobbyMan->StopReportingGame();
			NNFreeNegotiateList();
		}
#endif
#endif    
		net_man->DestroyApp( m_server );
		m_server_add_new_players_task->Remove();
		m_start_network_game_task->Remove();
		m_server = NULL;
	}

	m_flags.ClearMask( mSERVER );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	  Manager::ClientShutdown( void )
{
	
	
	Net::Manager * net_man = Net::Manager::Instance();
	int i;

#ifdef	__PLAT_XBOX__	 
	if( m_client[0] && !m_client[0]->IsLocal())
	{
		if( m_XboxKeyRegistered )
		{
			if( XNetUnregisterKey( &m_XboxKeyId ) == 0 )
			{
				m_XboxKeyRegistered = false;
			}
		}
	}
#endif

	for( i = 0; i < vMAX_LOCAL_CLIENTS; i++ )
	{
		if( m_client[i] )
		{
			if(( i == 0 ) && !m_client[i]->IsLocal())
			{
				m_client_add_new_players_task->Remove();
			}

			net_man->DestroyApp( m_client[i] );
			m_client[i] = NULL;
		}
	}

	if( m_observer_input_handler )
	{
		delete m_observer_input_handler;
		m_observer_input_handler = NULL;
	}

	m_enter_chat_task->Remove();
	m_render_scores_task->Remove();
	m_change_level_task->Remove();

	ClearTriggerEventList();
	SetReadyToPlay(false);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::MatchClientShutdown( void )
{
	if( m_match_client )
	{
		Net::Manager * net_man = Net::Manager::Instance();

		net_man->DestroyApp( m_match_client );
		m_match_client = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::AutoServerShutdown( void )
{
	m_auto_server_task->Remove();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Manager::GetJoinIP( void )
{
	return m_join_ip;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Manager::GetJoinPrivateIP( void )
{
	return m_join_private_ip;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Manager::GetJoinPort( void )
{
	return m_join_port;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetJoinIP( int ip )
{
	Dbg_Printf( "**** In SetJoinIP: %s\n", inet_ntoa(*(struct in_addr *) &ip ));
	m_join_ip = ip;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetJoinPrivateIP( int ip )
{
	m_join_private_ip = ip;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetJoinPort( int port )
{
	m_join_port = port;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Net::Server*  Manager::GetServer( void )
{
	return m_server;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Net::Client*   Manager::GetClient( int index )
{
	

	Dbg_Assert( index < vMAX_LOCAL_CLIENTS );
   	
	return m_client[index];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Net::Client*   Manager::GetMatchClient( void )
{   
	return m_match_client;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Manager::GetNumPlayers( void )
{
	int num_players;
	Lst::Search< PlayerInfo > sh;
	PlayerInfo* player;
	Lst::Search< NewPlayerInfo > new_sh;
	NewPlayerInfo* np;

	num_players = 0;

	for( player = FirstPlayerInfo( sh, true ); player; player = NextPlayerInfo( sh, true ))
	{
		if( !player->IsObserving() || player->m_flags.TestMask( PlayerInfo::mPENDING_PLAYER ))
		{
			num_players++;
		}
	}
    
	for( np = new_sh.FirstItem( m_new_players ); np; np = new_sh.NextItem())
	{
		// Pending players count, observers don't
		if(	( np->Flags & PlayerInfo::mPENDING_PLAYER ) ||
			( np->Flags & PlayerInfo::mJUMPING_IN ) ||
			!( np->Flags & PlayerInfo::mOBSERVER ))
		{
			num_players++;
		}                 
	}
	
	return num_players;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Manager::GetNumObservers( void )
{
	int num_observers;
	Lst::Search< PlayerInfo > sh;
	PlayerInfo* player;
	Lst::Search< NewPlayerInfo > new_sh;
	NewPlayerInfo* np;

	num_observers = 0;

	for( player = FirstPlayerInfo( sh, true ); player; player = NextPlayerInfo( sh, true ))
	{
		// Pending players don't count as observers
		if( ( player->IsObserving()) && 
			( !player->IsPendingPlayer()))
		{   
			// Don't count a server who is sitting out as an observer
			if(	( player->IsLocalPlayer() && 
				( GetHostMode() != vHOST_MODE_SERVE )))
			{
				continue;
			}
			num_observers++;
		}
	}
    
	for( np = new_sh.FirstItem( m_new_players ); np; np = new_sh.NextItem())
	{
		// Don't count pending players as observers
		if(( np->Flags & PlayerInfo::mOBSERVER ) &&
		   ( !( np->Flags & PlayerInfo::mPENDING_PLAYER )))
		{
			num_observers++;
		}                 
	}
	
	return num_observers;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

PlayerInfo*		Manager::NewPlayer( Obj::CSkater* skater, Net::Conn* conn, int flags )
{
	PlayerInfo* new_player;
		
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());

	new_player = new PlayerInfo( flags );

	new_player->m_Skater = skater;
	new_player->m_Conn = conn;
		
	// By default, count the Local player as our currently observed player
	if( new_player->IsLocalPlayer())
	{
		if( new_player->IsObserving())
		{   
			Mlp::Manager * mlp_manager = Mlp::Manager::Instance();
			Inp::Manager * inp_manager = Inp::Manager::Instance();
			
			/*new_player->m_cam = new Obj::CSkaterCam( 0 );
			Nx::CViewportManager::sSetCamera( 0, new_player->m_cam );
			new_player->m_cam->SetMode( Obj::CSkaterCam::SKATERCAM_MODE_NORMAL_MEDIUM, 0.0f );*/
						
			Script::RunScript( "hide_panel_stuff" );
			ObservePlayer( GetNextPlayerToObserve());

			new_player->m_observer_logic_task = 
					new Tsk::Task< PlayerInfo > ( PlayerInfo::s_observer_logic_code, *new_player );
			mlp_manager->AddLogicTask( *new_player->m_observer_logic_task );
			m_observer_input_handler = new Inp::Handler< Manager > ( 0,  s_observer_input_logic_code, *this, 
															 Tsk::BaseTask::Node::vHANDLER_PRIORITY_OBSERVER_INPUT_LOGIC );
			inp_manager->AddHandler( *m_observer_input_handler );
		}
		else
		{
			m_cam_player = NULL;
		}
	}
	else
	{
		PlayerInfo* local_player;

		local_player = GetLocalPlayer();
		Dbg_MsgAssert( local_player,( "Should have local player by now" ));

		if( local_player->IsObserving())
		{
			if( m_cam_player == NULL )
			{
				ObservePlayer( new_player );
			}
		}
	}

	Dbg_Printf( "Player added : %p with conn %p\n", new_player, new_player->m_Conn );
    
	Mem::Manager::sHandle().PopContext();

	return new_player;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			Manager::SendFaceDataToServer( void )
{
	bool sent_data;
	Net::Manager* net_man = Net::Manager::Instance();

	Dbg_Assert( InNetGame());

	// Don't send face data to other players if I'm on a modem
	if(	net_man->GetConnectionType() == Net::vCONN_TYPE_MODEM )
	{
		return false;
	}

	Obj::CSkaterProfile* pSkaterProfile = Mdl::Skate::Instance()->GetProfile( 0 );
	Gfx::CModelAppearance* pAppearance = pSkaterProfile->GetAppearance();
	Gfx::CFaceTexture* pFaceTexture = pAppearance->GetFaceTexture();

	sent_data = false;
	if( pFaceTexture && pFaceTexture->IsValid())
	{
		if( OnServer())
		{
			PlayerInfo* local_player;

			local_player = GetLocalPlayer();
			
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetMiscHeap());
			local_player->m_face_data = new uint8[vMAX_FACE_DATA_SIZE + 1];
			Mem::Manager::sHandle().PopContext();

			local_player->m_face_data[0] = 0;	// player id
			pFaceTexture->WriteToBuffer( &local_player->m_face_data[1], vMAX_FACE_DATA_SIZE );
			sent_data = true;
		}
		else
		{
			Net::Conn *server_conn;
			Lst::Search< Net::Conn > sh;
		
			server_conn = GetClient(0)->FirstConnection( &sh );
			if( server_conn->GetBandwidthType() == Net::Conn::vBROADBAND )
			{
				Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());
				uint8* buffer = new uint8[vMAX_FACE_DATA_SIZE];
				Mem::Manager::sHandle().PopContext();
	
				pFaceTexture->WriteToBuffer( buffer, vMAX_FACE_DATA_SIZE );
				GetClient(0)->StreamMessageToServer( MSG_ID_FACE_DATA, vMAX_FACE_DATA_SIZE, buffer, "Face Data",
													 vSEQ_GROUP_FACE_MSGS, false, false );
				delete[] buffer;
				sent_data = true;
			}
		}
	}

	return sent_data;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SendFaceDataToPlayer( PlayerInfo* player )
{
	PlayerInfo* face_player;
	Lst::Search< PlayerInfo > sh;
	Net::Manager* net_man = Net::Manager::Instance();

	Dbg_Assert( OnServer());

	// Don't send face data to other players if I'm on a modem
	if(	net_man->GetConnectionType() == Net::vCONN_TYPE_MODEM )
	{
		return;
	}

	for( face_player = FirstPlayerInfo( sh ); face_player; face_player = NextPlayerInfo( sh ))
	{
		bool has_face_data;

		if( face_player == player )
		{
			continue;
		}
		// Don't send face data to modem players for bandwidth reasons
		if( player->m_Conn->GetBandwidthType() == Net::Conn::vNARROWBAND )
		{
			continue;
		}

		has_face_data = false;
		if( face_player->GetFaceData())
		{
			has_face_data = true;
		}
		else
		{
			if( face_player->IsLocalPlayer())
			{
				has_face_data = SendFaceDataToServer();
			}
		}

		if( has_face_data == false )
		{
			continue;
		}

		
		m_server->StreamMessageToConn( player->m_Conn, MSG_ID_FACE_DATA, vMAX_FACE_DATA_SIZE + 1, face_player->GetFaceData(), "Face Data",
									   vSEQ_GROUP_FACE_MSGS, false, true );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::ClientAddNewPlayer( NewPlayerInfo* new_player )
{
	Obj::CSkater* skater;
	PlayerInfo* player_info;
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
    
	Dbg_Assert( !OnServer());

	skater = NULL;
    
	if( !( new_player->Flags & PlayerInfo::mOBSERVER ))
	{
		Obj::CSkaterProfile* pSkaterProfile;

		if ( CFuncs::ScriptInSplitScreenGame( NULL, NULL ) )
		{
			// in splitscreen games, we have to figure out
			// which player is currently being added (either 0 or 1)
			pSkaterProfile = skate_mod->GetProfile(new_player->ObjID);
		}
		else
		{
			// in other types of games, "your player" is always
			// in slot 0 of the local machine's skater profile manager
			pSkaterProfile = skate_mod->GetProfile(0);
		}
		if( InNetGame())
		{
			if( new_player->Flags & PlayerInfo::mLOCAL_PLAYER )
			{
				skater = skate_mod->add_skater( pSkaterProfile, true, new_player->ObjID, 0);
			}
			else
			{
				int skater_num;

				
				if( skate_mod->GetLocalSkater())
				{
					skater_num = skate_mod->GetNumSkaters();
				}
				else
				{
					skater_num = ( skate_mod->GetNumSkaters() + 1 ) % Mdl::Skate::vMAX_SKATERS;
				}

				skater = skate_mod->add_skater( new_player->mpSkaterProfile, false, new_player->ObjID,
													skater_num );
				
				if (new_player->VehicleControlType)
				{
					Obj::CSkaterStateHistoryComponent* p_skater_state_history_component = GetSkaterStateHistoryComponentFromObject(skater);
					Dbg_Assert(p_skater_state_history_component);
					
					p_skater_state_history_component->SetCurrentVehicleControlType(new_player->VehicleControlType);
				}
			}
		}
		else
		{
			skater = skate_mod->add_skater( pSkaterProfile, true, new_player->ObjID, skate_mod->GetNumSkaters() );
		}
	}

	player_info = GetPlayerByObjectID( new_player->ObjID );
	if( new_player->Flags & PlayerInfo::mJUMPING_IN )
	{   
		if( player_info == NULL )
		{
			player_info = GetLocalPlayer();
		}

		if( player_info )
		{
			DestroyPlayer( player_info );
			player_info = NULL;
		}
	}

	if( player_info == NULL )
	{
		int flags;

		flags = ( new_player->Flags & (	PlayerInfo::mLOCAL_PLAYER | PlayerInfo::mOBSERVER | PlayerInfo::mHAS_ANY_FLAG ));
										
		player_info = NewPlayer( skater, new_player->Conn, flags );
        strcpy( player_info->m_Name, new_player->Name );
		player_info->m_Team = new_player->Team;
		player_info->m_Profile = new_player->Profile;
		player_info->m_Rating = new_player->Rating;
		player_info->m_VehicleControlType = new_player->VehicleControlType;
		AddPlayerToList( player_info );
		if( new_player->Flags & PlayerInfo::mKING_OF_THE_HILL )
		{
			player_info->MarkAsKing( true );
		}
	}
	
	// We get this message when we're added to a network lobby. So go ahead and start it up
	// on our end
	if( new_player->Flags & PlayerInfo::mJUMPING_IN )
	{
		// We're jumping into a game at the lobby so start the lobby game flow.
		// Also send our face texture data to all other players.
		SendFaceDataToServer();
		skate_mod->GetTrickObjectManager()->DeleteAllTrickObjects();
		Script::RunScript( "do_backend_retry" );
	}

	if( player_info->IsLocalPlayer() == false )
	{
		Mdl::Score* score = GetSkaterScoreComponentFromObject( skater )->GetScore();

		skater->RemoveFromCurrentWorld();
		score->SetTotalScore( new_player->Score );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::DeferredNewPlayer( NewPlayerInfo* new_player, int num_frames )
{
	
	
	Lst::Node< NewPlayerInfo > *player;
	Lst::Search< NewPlayerInfo > new_sh;
	NewPlayerInfo* np;
    
	// First, make sure we don't already have this new player queued up
	for( np = new_sh.FirstItem( m_new_players ); np; np = new_sh.NextItem())
	{
		if( np->ObjID == new_player->ObjID )
		{
			return;
		}
	}
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());

	np = new NewPlayerInfo;

	strcpy( np->Name, new_player->Name );
    
	np->Conn = new_player->Conn;
	np->ObjID = new_player->ObjID;
	np->Flags = new_player->Flags;
	np->Team = new_player->Team;
	np->Profile = new_player->Profile;
	np->Rating = new_player->Rating;
	np->Score = new_player->Score;
	np->VehicleControlType = new_player->VehicleControlType;
	if(( np->Flags & PlayerInfo::mJUMPING_IN ) && OnServer())
	{
		np->JumpInFrame = m_server->m_FrameCounter;
	}

	// Pure observers don't have skater profile date
	if( (!( np->Flags & PlayerInfo::mOBSERVER ) || ( np->Flags & PlayerInfo::mPENDING_PLAYER )))
	{
		// GJ:  copy the contents of the skater profile from one to the other
		// it'd be nice to just use a copy constructor, but the compiler
		// doesn't seem to like it.
		uint8* pTempBuffer = new uint8[vMAX_APPEARANCE_DATA_SIZE];
		new_player->mpSkaterProfile->WriteToBuffer(pTempBuffer, vMAX_APPEARANCE_DATA_SIZE);
		np->mpSkaterProfile->ReadFromBuffer(pTempBuffer);
		delete[] pTempBuffer;
		
		// GJ 2/12/03:  the above was written sometime during THPS3,
		// and our current compiler seems to be okay now.  However,
		// i'm afraid of breaking anything by changing this now...
//		*np->mpSkaterProfile = *new_player->mpSkaterProfile;
	}
						 
	if( OnServer())
	{
        np->AllowedJoinFrame = m_server->m_FrameCounter + num_frames;

		if( !( np->Flags & PlayerInfo::mLOCAL_PLAYER ))
		{
			if( np->Flags & PlayerInfo::mOBSERVER )
			{
				uint32 msg_checksum;

				if( np->Flags & PlayerInfo::mPENDING_PLAYER )
				{
					msg_checksum = Script::GenerateCRC("net_message_join_pending");
				}
				else
				{
					msg_checksum = Script::GenerateCRC("net_message_observing");
				}
				CreateNetPanelMessage( true, msg_checksum, new_player->Name );
			}
			else
			{
				CreateNetPanelMessage( false, Script::GenerateCRC("net_message_joining"),
									   new_player->Name );
			}
		}
	}
	else
	{
		np->AllowedJoinFrame = m_client[0]->m_FrameCounter + num_frames;
	}
	
	player = new Lst::Node< NewPlayerInfo > ( np );

	m_new_players.AddToTail( player );
	
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::AddPlayerToList( PlayerInfo* player )
{
	

	Dbg_Assert( player );

    m_players.AddToTail( player );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::DestroyPlayer( PlayerInfo* player )
{
	// If we're destroying the player we're looking at, change to the next player
	if( player == m_cam_player )
	{
		PlayerInfo* local_player;

		m_cam_player = GetNextPlayerToObserve();
		if( player == m_cam_player )
		{
			m_cam_player = NULL;
		}
        
		local_player = GetLocalPlayer();
		if( local_player && local_player->m_Skater )
		{
			ObservePlayer( local_player );
		}
		else
		{
			ObservePlayer( m_cam_player );
		}
	}

	if( player->IsKing())
	{
		if( m_crown )
		{
			m_crown->RemoveFromKing();
		}
	}

	if( player->HasCTFFlag())
	{
		char team_str[64];
		int team;
		Script::CStruct* pParams;
		PlayerInfo* local_player;
		
		team = player->HasWhichFlag();
		sprintf( team_str, "team_%d_name", team + 1 );
		pParams = new Script::CStruct;

		pParams->AddInteger( "team", team );
		pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, Script::GetString( team_str ));
		Script::RunScript( "flag_returned", pParams );

		delete pParams;

		local_player = GetLocalPlayer();
		if( local_player && !local_player->IsObserving())
		{
			if( local_player->m_Team == team )
			{
				Script::RunScript( "hide_ctf_arrow" );		
			}
		}
		player->ClearCTFState();
	}

	delete player;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

PlayerInfo*		Manager::GetCurrentlyObservedPlayer( void )
{
	return m_cam_player;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

PlayerInfo*		Manager::GetNextPlayerToObserve( void )
{
	Lst::Search< PlayerInfo > sh;
	PlayerInfo* new_cam_player;

	

	new_cam_player = NULL;
	for( new_cam_player = FirstPlayerInfo( sh ); new_cam_player;
			new_cam_player = NextPlayerInfo( sh ))
	{
		// If we had no 'current' target, just return the first player
		if( m_cam_player == NULL )
		{
			break;
		}

		// Ok, we found our current cam_player. Return the next one, if there is a next one
		if( new_cam_player == m_cam_player )
		{
			new_cam_player = NextPlayerInfo( sh );
			if( new_cam_player == NULL )
			{
				// We've reached the end of the list. Return the first one
				new_cam_player = FirstPlayerInfo( sh );
			}
			break;
		}
	}

	return new_cam_player;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Manager::ObservePlayer( PlayerInfo* player )
{   
	Obj::CSkaterCameraComponent* skater_cam;
	Obj::CWalkCameraComponent* walk_cam;
	Obj::CCompositeObject* cam_obj;
	
	cam_obj = (Obj::CCompositeObject*) Obj::CCompositeObjectManager::Instance()->GetObjectByID(CRCD(0x967c138c,"skatercam0"));
	skater_cam = GetSkaterCameraComponentFromObject( cam_obj );
	walk_cam = GetWalkCameraComponentFromObject( cam_obj );
	
	if (!player || !player->m_Skater)
	{
		Dbg_Printf( "Attempting to observe NULL player\n" );
		
		skater_cam->SetSkater( NULL );
		walk_cam->SetSkater( NULL );
		
		m_cam_player = NULL;
		
		return;
	}
	
	Manager* gamenet_man = Manager::Instance();
	PlayerInfo* local_player;
	local_player = gamenet_man->GetLocalPlayer();
	
	if (player != local_player)
	{
		// observer camera only use the skater camera logic
		
		Dbg_Printf( "Observing player %d\n", player->m_Skater->GetID());
		skater_cam->SetSkater( player->m_Skater );
		
		walk_cam->SetSkater( NULL );
		
		skater_cam->Suspend( false );
		walk_cam->Suspend( true );
	}
	else
	{
		// restore the correct camera state for a normal skate camera
		
		Obj::CSkaterPhysicsControlComponent* physics_control_comp;
		
		physics_control_comp = GetSkaterPhysicsControlComponentFromObject( player->m_Skater );
		Dbg_MsgAssert(physics_control_comp, ("Local player has no SkaterPhysicsControlComponent"));
		
		Dbg_Printf( "Restoring standard camera to player %d\n", player->m_Skater->GetID());
		
		skater_cam->SetSkater( player->m_Skater );
		walk_cam->SetSkater( player->m_Skater );
		
		if (physics_control_comp->IsSkating())
		{
			skater_cam->Suspend( false );
			walk_cam->Suspend( true );
		}
		else
		{
			skater_cam->Suspend( true );
			walk_cam->Suspend( false );
			walk_cam->Reset();
		}
	}
	
	m_cam_player = player;

	/*if( player == NULL )
	{
		Mth::Vector rot;
		Mth::Matrix cam_matrix;
		Mth::Vector node_pos;
		int node;

		node = Obj::GetRestartNode( Script::GenerateCRC( "Player1" ), 0 );
		Script::CStruct* pNodeData = SkateScript::GetNode( node ); 
		
		cam_matrix.Ident( );	   		// Set identity before we decided if we need to rotate or not, in case we don't.
		
		SkateScript::GetPosition( node, &node_pos );
		node_pos[Y] += FEET( 5 );
		Dbg_Printf( "************************ Got Position %f %f %f\n", node_pos[X], node_pos[Y], node_pos[Z] );
		
		SkateScript::GetAngles( pNodeData, &rot );
		cam_matrix.RotateY( rot[ Y ] );
		cam_matrix[Z] = -cam_matrix[Z];
		cam_matrix[X] = -cam_matrix[X];
		Dbg_Printf( "************************ Rots X: %f Y: %f Z: %f\n", rot[X], rot[Y], rot[Z] );
		
		Dbg_Assert( local_player );
		Dbg_Assert( local_player->m_cam );

		local_player->m_cam->SetMatrix( cam_matrix );
		local_player->m_cam->SetPos( node_pos );
		local_player->m_cam->SetSkater( NULL );
		return;
	}

	// Only works in network games
	if( InNetGame())
	{   
		if( local_player && local_player->IsObserving())
		{   
			m_cam_player = player;
			local_player->m_cam->SetSkater( m_cam_player->m_Skater );
		}
	}*/
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Manager::RequestObserverMode( void )
{
	PlayerInfo* local_player;
    Net::MsgDesc msg_desc;
	
	local_player = GetLocalPlayer();
	Dbg_Assert( local_player );

	if( local_player->IsObserving())
	{
		return;
	}
	
	Dbg_Printf( "Sent request to enter observer mode\n" );
	msg_desc.m_Id = MSG_ID_OBSERVE;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	m_client[0]->EnqueueMessageToServer( &msg_desc );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Manager::ChooseNewServerPlayer( void )
{
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;
	Net::MsgDesc msg_desc;

	Dbg_Assert(( GetServerPlayer() == NULL ));	// Shouldn't call this function if one already exists
	Dbg_Assert( m_server );

	for( player = FirstPlayerInfo( sh ); player; player = NextPlayerInfo( sh ))
	{
		Script::CStruct* pStructure;
		uint32 checksum;

		// For now, just select the first non-server player info
		if( player->IsLocalPlayer())
		{
			continue;
		}

		// If we don't have any "fully in" players at this point, that's ok. Don't choose a server player.
		// When they are marked as being "fully in" they will be chosen as the fcfs and notified
		if( player->IsFullyIn() == false )
		{
			continue;
		}

		player->MarkAsServerPlayer();
		
		pStructure = m_network_preferences.GetPreference( Script::GenerateCRC("player_collision"));
		pStructure->GetChecksum( "Checksum", &checksum, true );
		
		msg_desc.m_Data = &checksum;
		msg_desc.m_Length = sizeof( uint32 );
		msg_desc.m_Id = MSG_ID_FCFS_ASSIGNMENT;
		msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
		msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
		m_server->EnqueueMessage( player->GetConnHandle(), &msg_desc );

		break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Manager::EnterObserverMode( void )
{
	PlayerInfo* local_player;
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Mlp::Manager * mlp_manager = Mlp::Manager::Instance();
	Inp::Manager * inp_manager = Inp::Manager::Instance();
	Lst::Search< PlayerInfo > first_player;
	
	Dbg_Printf( "Entering Observer Mode\n" );
	
	local_player = GetLocalPlayer();
	Dbg_Assert( local_player );

    if( local_player->IsObserving())
	{
		return;
	}
	
	if( OnServer())
	{
		DropPlayer( local_player, vREASON_OBSERVING );
	}

	if( local_player->IsKing())
	{
		local_player->MarkAsKing( false );
		if( m_crown )
		{
			m_crown->RemoveFromKing();
		}
	}
	if( local_player->HasCTFFlag())
	{
		char team_str[64];
		int team;
		Script::CStruct* pParams;
		
		team = local_player->HasWhichFlag();
		sprintf( team_str, "team_%d_name", team + 1 );
		pParams = new Script::CStruct;

		pParams->AddInteger( "team", team );
		pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, Script::GetString( team_str ));
		Script::RunScript( "flag_returned", pParams );

		delete pParams;

		Script::RunScript( "hide_ctf_arrow" );		
		local_player->ClearCTFState();
	}

	local_player->m_flags.SetMask( PlayerInfo::mOBSERVER );


	skate_mod->remove_skater( local_player->m_Skater );
	local_player->m_Skater = NULL;
	//skate_mod->HideSkater( local_player->m_Skater );

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterHeap( 0 ));

	//Gfx::Camera* p_camera = &(viewer_mod->GetCamera( 0 )->camera );
	/*local_player->m_cam = new Obj::CSkaterCam( 0 );
	Dbg_Assert( local_player->m_cam );
	Nx::CViewportManager::sSetCamera( 0, local_player->m_cam );
	local_player->m_cam->SetMode( Obj::CSkaterCam::SKATERCAM_MODE_NORMAL_MEDIUM, 0.0f );*/

	ObservePlayer( GetNextPlayerToObserve());

	local_player->m_observer_logic_task = 
			new Tsk::Task< PlayerInfo > ( PlayerInfo::s_observer_logic_code, *local_player );
	mlp_manager->AddLogicTask( *local_player->m_observer_logic_task );
	m_observer_input_handler = new Inp::Handler< Manager > ( 0,  s_observer_input_logic_code, *this, 
															 Tsk::BaseTask::Node::vNORMAL_PRIORITY-1 );
	inp_manager->AddHandler( *m_observer_input_handler );
	SetObserverCommand( vOBSERVER_COMMAND_NONE );

	Script::RunScript( "ShowAllObjects" );
	Script::RunScript( "hide_panel_stuff" );

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Manager::GetNextPlayerObjectId( void )
{
	Lst::Search< PlayerInfo > sh;
	Lst::Search< NewPlayerInfo > new_sh;
	PlayerInfo* player;
	NewPlayerInfo* new_player;
	int i;
	bool taken;

	

	Dbg_MsgAssert( m_flags.TestMask( mSERVER ),( "Only the server should be assigning ids!\n" ));
	
	for( i = 0; i < 16; i++ )
	{
		taken = false;
		for( player = FirstPlayerInfo( sh ); player; player = NextPlayerInfo( sh ))
		{
			if( player->m_Skater->GetID() == (uint32)i )
			{
				taken = true;
				break;
			}
		}

		if( taken == false )
		{
			for( new_player = FirstNewPlayerInfo( new_sh ); new_player;
					new_player = NextNewPlayerInfo( new_sh ))
			{
				// Pure observers don't and won't have object ids
				if(( new_player->Flags & PlayerInfo::mOBSERVER ) &&
				   ( !( new_player->Flags & PlayerInfo::mPENDING_PLAYER )))
				{
					continue;
				}
				if( new_player->ObjID == i )
				{
					taken = true;
					break;
				}
			}
		}

		if( taken == false )
		{
			return i;
		}
	}

	Dbg_MsgAssert( 0,( "Ran out of player object id's. More than 16 players in game?\n" ));
	return 16;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CSkater*	Manager::GetSkaterByConnection( Net::Conn* conn )
{
	Lst::Search< PlayerInfo > sh;
	PlayerInfo *player;

	for( player = FirstPlayerInfo( sh ); player; player = NextPlayerInfo( sh ))
	{
		if( player->m_Conn == conn )
		{
			return player->m_Skater;
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

PlayerInfo*		Manager::GetPlayerByConnection( Net::Conn* conn )
{
	Lst::Search< PlayerInfo > sh;
	PlayerInfo *player;

	for( player = FirstPlayerInfo( sh, true ); player; player = NextPlayerInfo( sh, true ))
	{
		if( player->m_Conn == conn )
		{
			return player;
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

PlayerInfo*		Manager::GetPlayerByObjectID( unsigned short obj_id )
{
	Lst::Search< PlayerInfo > sh;
	PlayerInfo *player;

	for( player = FirstPlayerInfo( sh ); player; player = NextPlayerInfo( sh ))
	{
		if( player->m_Skater )
		{
			if( player->m_Skater->GetID() == obj_id )
			{
				return player;
			}
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::FindServersOnLAN( void )
{
	if( m_match_client )
	{
		MsgFindServer msg;
        
		msg.m_Timestamp = Tmr::GetTime();
        
#if (defined(__PLAT_NGPS__)||defined(__PLAT_NGC__))
		m_match_client->SendMessageTo( Net::MSG_ID_FIND_SERVER, sizeof( MsgFindServer ), &msg, 
									0xFFFFFFFF, vHOST_PORT, 0 );
#else
		XNetRandom( m_match_client->m_Nonce, sizeof( m_match_client->m_Nonce ));
		memcpy( msg.m_Nonce, &m_match_client->m_Nonce, sizeof( m_match_client->m_Nonce ));
		m_match_client->SendMessageTo( Net::MSG_ID_FIND_SERVER, sizeof( MsgFindServer ), &msg,
									INADDR_BROADCAST, vHOST_PORT, 0 );
#endif
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

PlayerInfo*			Manager::FirstPlayerInfo( Lst::Search< PlayerInfo > &sh, bool include_observers )
{
	PlayerInfo* player;

	player = sh.FirstItem( m_players );
	if( !include_observers && player && player->IsObserving())
	{
		player = NextPlayerInfo( sh, include_observers );
	}
	
	return player;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

PlayerInfo*			Manager::NextPlayerInfo( Lst::Search< PlayerInfo > &sh, bool include_observers )
{
	PlayerInfo* player;

	player = sh.NextItem();
	if( !include_observers && player && player->IsObserving())
	{
		player = NextPlayerInfo( sh, include_observers );
	}
	
	return player;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

NewPlayerInfo*		Manager::FirstNewPlayerInfo( Lst::Search< NewPlayerInfo > &sh )
{
	NewPlayerInfo* player;

	player = sh.FirstItem( m_new_players );
	
	return player;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

NewPlayerInfo*		Manager::NextNewPlayerInfo( Lst::Search< NewPlayerInfo > &sh )
{
	NewPlayerInfo* player;

	player = sh.NextItem();
	
	return player;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

NewPlayerInfo*		Manager::GetNewPlayerInfoByObjectID( unsigned short obj_id )
{
	NewPlayerInfo* player;
	Lst::Search< NewPlayerInfo > sh;

	for( player = FirstNewPlayerInfo( sh ); player; player = NextNewPlayerInfo( sh ))
	{
		if( player->ObjID == obj_id )
		{
			return player;
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Manager::DestroyNewPlayer( NewPlayerInfo* new_player )
{
	Lst::Node< NewPlayerInfo > *node;
	NewPlayerInfo* new_info;

	Dbg_Assert( new_player );
	Dbg_Printf( "Destroying Pending Player %d\n", new_player->ObjID );
	for( node = m_new_players.GetNext(); node; node = node->GetNext())
	{
		new_info = node->GetData();
		if( new_info == new_player )
		{
			if( m_server )
			{
				Dbg_Printf( "Removing %s\n", new_info->Name );
				m_server->TerminateConnection( new_info->Conn );
			}
			else
			{
				Dbg_Printf( "Removing %s\n", new_info->Name );
				if( new_info->Conn )
				{
					m_client[0]->TerminateConnection( new_info->Conn );
				}
			}
			
			delete node;
			delete new_info;
			break;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

PlayerInfo*			Manager::GetLocalPlayer( void )
{
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;

	for( player = FirstPlayerInfo( sh, true ); player;
			player = NextPlayerInfo( sh, true ))
	{
        if( player->IsLocalPlayer())
		{
			return player;
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

PlayerInfo*			Manager::GetServerPlayer( void )
{
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;

	for( player = FirstPlayerInfo( sh, true ); player;
			player = NextPlayerInfo( sh, true ))
	{
        if( player->IsServerPlayer())
		{
			return player;
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

PlayerInfo*			Manager::GetKingOfTheHill( void )
{
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;

	for( player = FirstPlayerInfo( sh, true ); player;
			player = NextPlayerInfo( sh, true ))
	{
        if( player->IsKing())
		{
			return player;
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32				Manager::GetNetworkLevelId( void )
{
	return m_level_id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::DisconnectFromServer( void )
{
	Dbg_Message( "Disconnecting from server\n" );
    
	CleanupPlayers();
	
	ClientShutdown();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::CleanupPlayers( void )
{
	free_all_players();
	free_all_pending_players();	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Manager::NumPartiallyLoadedPlayers( void )
{
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;
	int num;

	num = 0;
	for( player = FirstPlayerInfo( sh ); player; player = NextPlayerInfo( sh ))
	{
		if( player->IsFullyIn() == false )
		{
			num++;
		}
	}

	return num;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::DropPartiallyLoadedPlayers( void )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	PlayerInfo* player, *next;
	Lst::Search< PlayerInfo > sh;

	Dbg_Printf( "****** DROPPING PARTIALLY-LOADED SKATERS\n" );
	for( player = FirstPlayerInfo( sh ); player; player = next )
	{
		next = NextPlayerInfo( sh );
		if( player->IsFullyIn() == false )
		{
			Obj::CSkater* quitting_skater;
						
			quitting_skater = player->m_Skater;
						
			Dbg_Printf( "****** DROPPING PLAYER %d\n", quitting_skater->GetID());
			DropPlayer( player, vREASON_LEFT_OUT );
            skate_mod->remove_skater( quitting_skater );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::DropPlayer( PlayerInfo* info, DropReason reason )
{
	
	char player_name[vMAX_PLAYER_NAME_LEN + 1];

	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	Dbg_Assert( info );
	Dbg_Message( "Removing player %s from the game\n", info->m_Name );

    strcpy( player_name, info->m_Name );
	
	if( !info->IsObserving())
	{
		// if we're playing a game of graffiti, then free up all of the trick objects accumulated by the dropped client
		if ( skate_mod->GetGameMode()->IsTrue( "should_modulate_color" ) )
		{
			skate_mod->GetTrickObjectManager()->FreeTrickObjects( info->m_Skater->GetID() );
		}
	}

	if( OnServer())
	{
		if( !info->IsObserving())
		{
			// Inform all others of the quitter's departure
			for( player = FirstPlayerInfo( sh, true ); player;
					player = NextPlayerInfo( sh, true ))
			{
				if( !player->IsLocalPlayer())
				{
					MsgPlayerQuit msg;
					Net::MsgDesc msg_desc;
					
					msg.m_ObjId = info->m_Skater->GetID();
					msg.m_Reason = (char) reason;
			
					msg_desc.m_Data = &msg;
					msg_desc.m_Length = sizeof( MsgPlayerQuit );
					msg_desc.m_Id = MSG_ID_PLAYER_QUIT;
					msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
					msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
					m_server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
				}
			}

#ifdef __PLAT_NGPS__
			if( ( reason == vREASON_QUIT ) ||
				( reason == vREASON_TIMEOUT ) ||
				( reason == vREASON_BAD_CONNECTION ))
			{
				if( mpStatsMan->IsLoggedIn())
				{
					mpStatsMan->PlayerLeft( info->m_Skater->GetID());
				}
			}
#endif
		}

		if(	( reason == vREASON_BANNED ) ||
			( reason == vREASON_KICKED ))
		{
			m_server->SendMessageTo( MSG_ID_KICKED, 0, NULL,
									   info->m_Conn->GetIP(), info->m_Conn->GetPort(), 0 );
			if( reason == vREASON_BANNED )
			{
				m_server->BanConnection( info->m_Conn );
			}
#ifdef __PLAT_NGPS__
			//m_server->WaitForAsyncCallsToFinish();
#endif // __PLAT_NGPS__

		}
		else if( reason == vREASON_LEFT_OUT )
		{
			m_server->SendMessageTo( MSG_ID_LEFT_OUT, 0, NULL,
									   info->m_Conn->GetIP(), info->m_Conn->GetPort(), 0 );
#ifdef __PLAT_NGPS__
			//m_server->WaitForAsyncCallsToFinish();
#endif // __PLAT_NGPS__
		}
		if( reason != vREASON_OBSERVING )
		{
			if( info->m_Conn )
			{
				info->m_Conn->Invalidate();
				//Dbg_Printf( "DestroyingAllMessageData 1\n" );
				info->m_Conn->DestroyAllMessageData();
			}
		}

		uint32 reason_checksum;
		
		switch( reason )
		{
			case vREASON_QUIT:
			{
				reason_checksum = Script::GenerateCRC("net_message_player_quit");
				break;
			}
			case vREASON_TIMEOUT:
			{
				reason_checksum = Script::GenerateCRC("net_message_player_timed_out");
				break;
			}
			case vREASON_OBSERVING:
			{
				reason_checksum = Script::GenerateCRC("net_message_player_now_observing");
				break;
			}
			case vREASON_BANNED:
			{
				reason_checksum = Script::GenerateCRC("net_message_player_banned");
				break;
			}
			case vREASON_KICKED:
			{
				reason_checksum = Script::GenerateCRC("net_message_player_kicked");
				break;
			}
			case vREASON_LEFT_OUT:
			{
				reason_checksum = Script::GenerateCRC("net_message_player_left_out");
				break;
			}
			case vREASON_BAD_CONNECTION:
			{
				reason_checksum = Script::GenerateCRC("net_message_player_dropped");
				break;
			}
			default:
				reason_checksum = Script::GenerateCRC("net_message_player_timed_out");
				break;
		}

		// If the info is about the server player, no need to print a panel message.
		// Currently this would only happen if the server decided to sit out
		if( info->IsLocalPlayer() == false )
		{	   
			CreateNetPanelMessage( true, reason_checksum, player_name );
		}
		if( reason == vREASON_OBSERVING )
		{
			if( info->IsKing())
			{
				if( m_crown )
				{
					m_crown->RemoveFromKing();
				}
				info->MarkAsKing( false );
			}
			if( info->HasCTFFlag())
			{
				char team_str[64];
				int team;
				Script::CStruct* pParams;
				PlayerInfo* local_player;
				
				team = info->HasWhichFlag();
				sprintf( team_str, "team_%d_name", team + 1 );
				pParams = new Script::CStruct;
		
				pParams->AddInteger( "team", team );
				pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, Script::GetString( team_str ));
				Script::RunScript( "flag_returned", pParams );
		
				delete pParams;
		
				local_player = GetLocalPlayer();
				if( local_player && !local_player->IsObserving())
				{
					if( local_player->m_Team == team )
					{
						Script::RunScript( "hide_ctf_arrow" );		
					}
				}
				info->ClearCTFState();
			}
		}
		else
		{
			bool is_server_player;
			
			is_server_player = info->IsServerPlayer();
			
			Dbg_Printf( "Destroying Player %s\n", info->m_Name );
			DestroyPlayer( info );
			if( is_server_player )
			{
				ChooseNewServerPlayer();
				if( GetServerPlayer() == NULL )
				{
					if( ( skate_mod->GetGameMode()->GetNameChecksum() != Script::GenerateCRC( "netlobby" )))
					{
						Script::RunScript( "network_end_game_selected" );
					}
				}
			}
		}

		// Now see if the leaving of this player satisfies ending conditions
		bool all_done = true;
		for( player = FirstPlayerInfo( sh ); player;
				player = NextPlayerInfo( sh ))
		{
			if( !player->m_flags.TestMask( PlayerInfo::mRUN_ENDED))
			{
				all_done = false;
				break;
			}
		}
	
		if( all_done )
		{
			Net::MsgDesc msg_desc;

			msg_desc.m_Id = MSG_ID_GAME_OVER;
			msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
			msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
			for( player = FirstPlayerInfo( sh, true ); player;
					player = NextPlayerInfo( sh, true ))
			{
				m_server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
			}
		}
	}
	else
	{
		DestroyPlayer( info );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Prefs::Preferences* Manager::GetNetworkPreferences( void )
{
	return &m_network_preferences;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Prefs::Preferences* Manager::GetTauntPreferences( void )
{
	return &m_taunt_preferences;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const char*			Manager::GetNameFromArrayEntry( char* array_name, uint32 checksum )
{
	Script::CArray* pArray;
	const char* string = NULL;
	int i;

	pArray = Script::GetArray( array_name );
	Dbg_Assert( pArray );

	for( i = 0; i < (int)pArray->GetSize(); i++ )
	{   
		uint32 value;
		Script::CStruct* pStructure;
		
		 
		pStructure = pArray->GetStructure( i );
		Dbg_Assert( pStructure );
		
		pStructure->GetChecksum( "checksum", &value, true );
		if( value == checksum )
		{
			pStructure->GetText( "name", &string, true );
		}
	}

	return string;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int					Manager::GetIntFromArrayEntry( char* array_name, uint32 checksum, uint32 field_checksum )
{
	Script::CArray* pArray;
	int int_val;
	int i;

	pArray = Script::GetArray( array_name );
	Dbg_Assert( pArray );

	int_val = 0;
	for( i = 0; i < (int)pArray->GetSize(); i++ )
	{   
		uint32 value;
		Script::CStruct* pStructure;
		
		 
		pStructure = pArray->GetStructure( i );
		Dbg_Assert( pStructure );
		
		pStructure->GetChecksum( "checksum", &value, true );
		if( value == checksum )
		{
			pStructure->GetInteger( field_checksum, &int_val, true );
		}
	}

	return int_val;
}

/******************************************************************/
/* Make use of the network preferences we currently have stored	  */
/*                                                                */
/******************************************************************/

void	Manager::UsePreferences( void )
{
	Net::Manager * net_man = Net::Manager::Instance();
	Prefs::Preferences* pPreferences;
	Script::CStruct* pStructure;
	Script::CArray* pArray;
	const char* ip, *phone, *login, *password, *host, *domain;
	uint32 dev_type, ip_assignment, checksum;
	bool use_dhcp, use_authentication, use_pcmcia_card;
	int i;

	// Initialize the master server list
	for( i = 0; i < vMAX_MASTER_SERVERS; i++ )
	{
		m_master_servers[i][0] = '\0';
	}

	pPreferences = GetNetworkPreferences();

	pArray = Script::GetArray("default_master_servers");
	Dbg_Assert( pArray );

	pStructure = pPreferences->GetPreference( Script::GenerateCRC("use_default_master_servers"));
	pStructure->GetChecksum( "Checksum", &checksum, true );
	if( checksum == Script::GenerateCRC( "boolean_true" ))
	{
		for( i = 0; i < (int)pArray->GetSize(); i++ )
		{   
			Script::CStruct* pStructure = pArray->GetStructure( i );
			Dbg_Assert( pStructure );
            
			pStructure->GetText( "name", &ip, true );
			strcpy( m_master_servers[ i ], ip );
		}
	}
	else
	{
		int num_servers;

		num_servers = 0;

		// First, add the manually-entered master servers to the list, if they're not
		// 0.0.0.0
		pStructure = pPreferences->GetPreference( Script::GenerateCRC("master_server1") );
		pStructure->GetText( "ui_string", &ip, true );
		if( strcmp( ip, "0.0.0.0" ))
		{
			strcpy( m_master_servers[ num_servers ], ip );
			num_servers++;
		}
				
		pStructure = pPreferences->GetPreference( Script::GenerateCRC("master_server2") );
		pStructure->GetText( "ui_string", &ip, true );
		if( strcmp( ip, "0.0.0.0" ))
		{
			strcpy( m_master_servers[ num_servers ], ip );
			num_servers++;
		}

		// Now tack on our default servers, just in case
		for( i = 0; i < (int)pArray->GetSize(); i++ )
		{   
			Script::CStruct* pStructure = pArray->GetStructure( i );
			Dbg_Assert( pStructure );
            
			pStructure->GetText( "name", &ip, true );
			strcpy( m_master_servers[ num_servers + i ], ip );
		}
	}

	pStructure = pPreferences->GetPreference( Script::GenerateCRC("broadband_type") );
	pStructure->GetChecksum( "Checksum", &ip_assignment, true );
	use_dhcp = false;
	if( ip_assignment == Script::GenerateCRC( "ip_dhcp" ))
	{
		Dbg_Printf( "************* Supposed to use DHCP here !! ***************\n" );
		use_dhcp = true;
	}

	// Override IP settings with the viewer_ip script variable, used to launch a server for real-time communication
#ifdef __PLAT_XBOX__
	const char* viewer_ip = Script::GetString( "xbox_viewer_ip" );
#endif
#ifdef __PLAT_NGPS__
	const char* viewer_ip = Script::GetString( "viewer_ip" );
#endif
#ifdef __PLAT_NGC__
	const char* viewer_ip = Script::GetString( "ngc_viewer_ip" );
#endif
	const char* viewer_gateway = Script::GetString( "viewer_gateway" );
	if( viewer_ip && !*viewer_ip )
	{
		viewer_ip = NULL;
	}
	if( viewer_ip )
	{
		Dbg_Printf( "************* Got a viewer IP !! ***************\n" );
		Prefs::Preferences* prefs;
        Script::CScriptStructure* pTempStructure;

		use_dhcp = false;

        pTempStructure = new Script::CScriptStructure;
		pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, 
								  viewer_ip );
        prefs = GetNetworkPreferences();
		prefs->SetPreference( Script::GenerateCRC("ip_address"), pTempStructure );
		delete pTempStructure;

		if( viewer_gateway && *viewer_gateway )
		{
			pTempStructure = new Script::CScriptStructure;
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, 
									  viewer_gateway );
			prefs = GetNetworkPreferences();
			prefs->SetPreference( Script::GenerateCRC("gateway"), pTempStructure );
			delete pTempStructure;
		}
	}
		
	net_man->SetDHCP( use_dhcp );

    if( use_dhcp )
	{
		pStructure = pPreferences->GetPreference( Script::GenerateCRC("host_name") );
		pStructure->GetText( "ui_string", &host, true );
		net_man->SetHostName((char*) host );

		pStructure = pPreferences->GetPreference( Script::GenerateCRC("domain_name") );
		pStructure->GetText( "ui_string", &domain, true );
		net_man->SetDomainName((char*) domain );
	}
	else
	{
		pStructure = pPreferences->GetPreference( Script::GenerateCRC("ip_address") );
		pStructure->GetText( "ui_string", &ip, true );
		Dbg_Printf( "****************** Setting Local IP to %s ********** \n", ip );
		net_man->SetLocalIP((char*) ip );
        
		pStructure = pPreferences->GetPreference( Script::GenerateCRC("gateway") );
		pStructure->GetText( "ui_string", &ip, true );
		Dbg_Printf( "****************** Setting Gateway to %s ********** \n", ip );
		net_man->SetGateway((char*) ip );

		pStructure = pPreferences->GetPreference( Script::GenerateCRC("subnet_mask") );
		pStructure->GetText( "ui_string", &ip, true );
		Dbg_Printf( "****************** Setting Subnet to %s ********** \n", ip );
		net_man->SetSubnetMask((char*) ip );
	}

#ifdef USE_DNS
	pStructure = pPreferences->GetPreference( Script::GenerateCRC("auto_dns") );
	pStructure->GetChecksum( "Checksum", &checksum, true );
	if( checksum == Script::GenerateCRC( "boolean_true" ) && ( viewer_ip == NULL ))
	{
		Dbg_Printf( "****************** Setting DNS to 0.0.0.0 ******** \n" );
		net_man->SetDNSServer( 0, "" );
		net_man->SetDNSServer( 1, "" );
		net_man->SetDNSServer( 2, "" );
	}
	else
	{
		if( viewer_ip )
		{
			net_man->SetDNSServer( 0, "205.147.0.100" );
			net_man->SetDNSServer( 1, "205.147.0.102" );
		}
		else
		{
			pStructure = pPreferences->GetPreference( Script::GenerateCRC("dns_server") );
			pStructure->GetText( "ui_string", &ip, true );
			Dbg_Printf( "****************** Setting DNS 1 to %s ******** \n", ip );
			net_man->SetDNSServer( 0, (char*) ip );
		
			pStructure = pPreferences->GetPreference( Script::GenerateCRC("dns_server2") );
			pStructure->GetText( "ui_string", &ip, true );
			Dbg_Printf( "****************** Setting DNS 2 to %s ******** \n", ip );
			net_man->SetDNSServer( 1, (char*) ip );
		}
	}
#else
#ifdef USE_FREE_DNS
	net_man->SetDNSServer( 0, "204.80.125.130" );	// Free DNS server in LA
	net_man->SetDNSServer( 1, "204.57.55.100" );	// Free DNS server in Boston
	net_man->SetDNSServer( 2, "" );
#else
	net_man->SetDNSServer( 0, "" );	// Free DNS server in LA
	net_man->SetDNSServer( 1, "" );	// Free DNS server in Boston
	net_man->SetDNSServer( 2, "" );
#endif // USE_FREE_DNS
#endif // USE_DNS

	pStructure = pPreferences->GetPreference( Script::GenerateCRC("dialup_number") );
	pStructure->GetText( "ui_string", &phone, true );
	net_man->SetISPPhoneNumber((char*) phone );

	pStructure = pPreferences->GetPreference( Script::GenerateCRC("dialup_username") );
	pStructure->GetText( "ui_string", &login, true );
	net_man->SetISPLogin((char*) login );

	pStructure = pPreferences->GetPreference( Script::GenerateCRC("dialup_password") );
	pStructure->GetText( "ui_string", &password, true );
	net_man->SetISPPassword((char*) password );
	
	pStructure = pPreferences->GetPreference( Script::GenerateCRC("device_type") );
	pStructure->GetChecksum( "Checksum", &dev_type, true );
	const char* use_pcmcia = Script::GetString( "use_pcmcia" );
	use_pcmcia_card = use_pcmcia[0] != '\0';
	
	if(( dev_type == Script::GenerateCRC( "device_broadband_pc" )) || use_pcmcia_card ) 
	{
		if( viewer_ip )
		{
			Script::RunScript( "spoof_pcmcia_adaptor_setup" );
		}
		net_man->SetDeviceType( Net::vDEV_TYPE_PC_ETHERNET );
		net_man->SetConnectionType( Net::vCONN_TYPE_ETHERNET );
	}
	else if(( dev_type == Script::GenerateCRC( "device_broadband_usb" )) || ( viewer_ip ))
	{
		if( viewer_ip )
		{
			Script::RunScript( "spoof_usb_adaptor_setup" );
		}
		net_man->SetDeviceType( Net::vDEV_TYPE_USB_ETHERNET );
		net_man->SetConnectionType( Net::vCONN_TYPE_ETHERNET );
	}
	else if( dev_type == Script::GenerateCRC( "device_broadband_usb_pppoe" ))
	{
		net_man->SetDeviceType( Net::vDEV_TYPE_USB_ETHERNET );
		net_man->SetConnectionType( Net::vCONN_TYPE_PPPOE );
	}
	else if( dev_type == Script::GenerateCRC( "device_usb_modem" ))
	{
		net_man->SetDeviceType( Net::vDEV_TYPE_USB_MODEM );
		net_man->SetConnectionType( Net::vCONN_TYPE_MODEM );
		if( GetMaxPlayers() > 3 )
		{
			SetMaxPlayers( 3 );
		}
		SetMaxObservers( 0 );
	}
	else if( dev_type == Script::GenerateCRC( "device_sony_modem" ))
	{
		net_man->SetDeviceType( Net::vDEV_TYPE_SONY_MODEM );
		net_man->SetConnectionType( Net::vCONN_TYPE_MODEM );
		if( GetMaxPlayers() > 3 )
		{
			SetMaxPlayers( 3 );
		}
		SetMaxObservers( 0 );
	}
	else if( dev_type == Script::GenerateCRC( "device_broadband_pc_pppoe" ))
	{
		net_man->SetDeviceType( Net::vDEV_TYPE_PC_ETHERNET );
		net_man->SetConnectionType( Net::vCONN_TYPE_PPPOE );
	}
	else
	{   
		net_man->SetConnectionType( Net::vCONN_TYPE_NONE );
		net_man->SetDeviceType( Net::vDEV_TYPE_NONE );
	}

	pStructure = pPreferences->GetPreference( Script::GenerateCRC("dialup_authentication") );
	pStructure->GetChecksum( "Checksum", &checksum, true );
	use_authentication = false;
	if( checksum == Script::GenerateCRC( "boolean_true" ))
	{
		use_authentication = true;
	}
	net_man->SetDialupAuthentication( use_authentication );
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::RandomizeSkaterStartingPoints( void )
{
	int i, j, offset, max_skaters;

	// choose a random offset for these starting points
	offset = Mth::Rnd( 256 );

	if( InNetGame())
	{
		max_skaters = Mdl::Skate::vMAX_SKATERS;
	}
	else
	{
		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		max_skaters = skate_mod->GetGameMode()->GetMaximumNumberOfPlayers();
		
		// PATCH  (Mick)
		// If playing a splitscreen game in the park editor
		// then set the Maximum number of players to 2
		// so we only use the 1P and 2P start points
		// oterwise, we will use all the auto-horse points
		// which end up in stupid positions
		if (Ed::CParkEditor::Instance()->UsingCustomPark() && !skate_mod->GetGameMode()->IsTrue("is_horse")) 		// is it a custom park and not horse mode?
		{
			max_skaters = 2;
			offset = 0;	   // reset the offset, so they always get used the same way
		}
		
		
	}

	for( i = 0; i < max_skaters; i++ )
	{
		m_skater_starting_points[i] = -1;
	}

	// For each skater, choose a random starting point
	for( i = 0; i < max_skaters; i++ )
	{
		bool unique;
        
		// Choose a random starting point until it's unique
		do
		{
			m_skater_starting_points[i] = Mth::Rnd( max_skaters );
			unique = true;
			for( j = 0; j < max_skaters; j++ )
			{
				// skip myself
				if( i == j )
				{
					continue;
				}

				if( m_skater_starting_points[i] == m_skater_starting_points[j] )
				{
					unique = false;
					break;
				}
			}
		} while( !unique );
	}

	// Offset these random sequential numbers by a random number so that we pick
	// four sequential numbers within a possibly larger range of starting points
	for( i = 0; i < max_skaters; i++ )
	{
		m_skater_starting_points[i] += offset;
	}

	m_crown_spawn_point = Mth::Rnd( vNUM_CROWN_SPAWN_POINTS );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetSkaterStartingPoints( int* start_points )
{
	int i;

	for( i = 0; i < Mdl::Skate::vMAX_SKATERS; i++ )
	{
		m_skater_starting_points[i] = start_points[i];
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Manager::GetSkaterStartingPoint( int skater_id )
{
	return m_skater_starting_points[skater_id];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::LoadPendingPlayers( void )
{
	PlayerInfo* player, *next;
	Lst::Search< PlayerInfo > sh;
	int num_new_players;

	Dbg_Assert( m_server );

	num_new_players = 0;

#ifdef __PLAT_XBOX__
	// Since XBOX has a quicker timeout, we need to tell clients that this process (loading pending players) may take much longer
	// than the timeout allows. Clients will allow the server N seconds to get back to them after receiving this message.
	// NOTE: This will only be guaranteed to work in LAN mode, because if the packet is lost, the original timeout will stand
	bool players_pending;

	players_pending = false;
	// Add all pending players to the game
	for( player = FirstPlayerInfo( sh, true ); player; player = NextPlayerInfo( sh, true ))
	{
		if( player->IsPendingPlayer())
		{
			players_pending = true;
			break;
		}
	}
	if( players_pending )
	{
		Net::MsgDesc msg_desc;
		for( player = FirstPlayerInfo( sh, true ); player; player = NextPlayerInfo( sh, true ))
		{
			if( !player->IsLocalPlayer())
			{
				char num_seconds;

				num_seconds = 15;

				msg_desc.m_Data = &num_seconds;
				msg_desc.m_Length = sizeof( char );
				msg_desc.m_Id = MSG_ID_WAIT_N_SECONDS;
				msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
				msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
				m_server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
			}
		}		
		m_server->SendData();
	}
	
#endif	

	// Add all pending players to the game
	for( player = FirstPlayerInfo( sh, true ); player; player = next )
	{
		next = NextPlayerInfo( sh, true );
		if( player->m_flags.TestMask( PlayerInfo::mPENDING_PLAYER ))
		{
			NewPlayerInfo new_player;

			Dbg_Assert( player->IsObserving());

			num_new_players++;

			strcpy( new_player.Name, player->m_Name );
			new_player.ObjID = GetNextPlayerObjectId();
			new_player.Conn = player->m_Conn;
			new_player.Flags = PlayerInfo::mJUMPING_IN;
			new_player.Profile = player->m_Profile;
			new_player.Rating = player->m_Rating;
			new_player.VehicleControlType = player->m_VehicleControlType;
			
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());
			*new_player.mpSkaterProfile = *player->mp_SkaterProfile;
			Mem::Manager::sHandle().PopContext();
            
			DestroyPlayer( player );

			DeferredNewPlayer( &new_player, ( num_new_players + 1 ) * vDEFERRED_JOIN_FRAMES );

		}
	}

	// Invalidate old positions. Player will be re-added to the world when we get a
	// sufficient number of object updates
	for( player = FirstPlayerInfo( sh ); player; player = NextPlayerInfo( sh ))
	{
		if( player->IsLocalPlayer())
		{
			continue;
		}
		
		player->m_Skater->RemoveFromCurrentWorld();
		player->m_Skater->Resync();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::StartNetworkLobby( void )
{
	int num_teams;	

	// This version of "StartNetworkLobby"
	// exits the game mode, and enters the lobby
	
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;
    
	if( OnServer())
	{
		m_level_id = GetLevelFromPreferences();
		num_teams = GetNumTeams();
	}
	else
	{
		num_teams = skate_mod->GetGameMode()->NumTeams();
	}

	// set the game type for the server
	m_gamemode_id = Script::GenerateCRC( "netlobby" );
	skate_mod->GetGameMode()->LoadGameType( m_gamemode_id );
	skate_mod->GetGameMode()->SetNumTeams( num_teams );

	m_game_over = false;

    // launches the game for this client
	// (should really wait until the clients are ready)
	
	//Mdl::ScriptRetry( NULL, NULL );

	ResetPlayers();

	Dbg_Printf( "Starting network lobby\n" );
	
	// Clear the king of the hill
	if(( player = GetKingOfTheHill()))
	{
		player->MarkAsKing( false );
	}

	for( player = FirstPlayerInfo( sh, true ); player; player = NextPlayerInfo( sh, true ))
	{
		player->ClearCTFState();
	}

	ClearTriggerEventList();

	m_lobby_start_time = Tmr::GetTime();

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ShouldStopAtZero( void )
{
	uint32 should_stop;

	if( ( m_gamemode_id == Script::GenerateCRC( "netgoalattack" )) ||
		( m_gamemode_id == Script::GenerateCRC( "king" )) ||
		( m_gamemode_id == Script::GenerateCRC( "netking" )) ||
		( m_gamemode_id == Script::GenerateCRC( "netscorechallenge" )) ||
		( m_gamemode_id == Script::GenerateCRC( "scorechallenge" )))
	{
		return true;
	}

	if( InNetGame() == false )
	{
		return false;
	}

	if( m_gamemode_id == Script::GenerateCRC( "netctf" ))
	{
		uint32 mode;

		mode = GetNetworkPreferences()->GetPreferenceChecksum( Script::GenerateCRC("ctf_game_type"), Script::GenerateCRC("checksum") );
		if( mode != CRCD(0x938ab24b,"timed_ctf"))
		{
			return true;
		}
	}

	should_stop = GetNetworkPreferences()->GetPreferenceChecksum( Script::GenerateCRC("stop_at_zero"), Script::GenerateCRC("checksum") );
	if( should_stop == CRCD(0xf81bc89b,"boolean_true"))
	{
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::StartNetworkGame( void )
{
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	int time_limit;
	Prefs::Preferences* pPreferences;
	Net::MsgDesc msg_desc;

	Dbg_MsgAssert( OnServer(),( "Server-only function called on client" ));

	pPreferences = NULL;
	if( InNetGame())
	{
		pPreferences = GetNetworkPreferences();
	}
	else 
	{
		pPreferences = skate_mod->GetSplitScreenPreferences();
	}

	

	Net::Server* server;
	server = GetServer();
	Dbg_MsgAssert( server,( "Server has not been started\n" ));

	// Sorry boys, you didn't quite make it in
	free_all_pending_players();

	// set the game type for the server, from the network preferences
	m_gamemode_id = GetGameTypeFromPreferences();
	//skate_mod->GetGameMode()->LoadGameType( m_gamemode_id );

	// Mick: Moved this to after the setting of the game type, so we can check for horse mode in it
	// (used to explicitly set 1P and 2P start points in 2P split screen (not horse) game)
	RandomizeSkaterStartingPoints();


	if( m_gamemode_id == Script::GenerateCRC( "horse" ))
	{
		time_limit = pPreferences->GetPreferenceValue( Script::GenerateCRC("horse_time_limit"), Script::GenerateCRC("time") );
	}
	else
	{
		time_limit = pPreferences->GetPreferenceValue( Script::GenerateCRC("time_limit"), Script::GenerateCRC("time") );
	}

	Dbg_Printf("Starting network game of type %s with time limit %d\n", Script::FindChecksumName(m_gamemode_id), time_limit);
	m_level_id = GetLevelFromPreferences();
	
	MsgGameInfo game_info_msg;
	game_info_msg.m_GameMode = m_gamemode_id;
	game_info_msg.m_FireballLevel = GetFireballLevelFromPreferences();
	Dbg_Printf( "***** FIREBALL LEVEL: %d\n", game_info_msg.m_FireballLevel );
	if( ( m_gamemode_id == CRCD(0xec200eaa,"netgoalattack")) ||
		( m_gamemode_id == CRCD(0x5d32129c,"king")) ||
		( m_gamemode_id == CRCD(0x6ef8fda0,"netking")) ||
		( m_gamemode_id == CRCD(0x1498240a,"netscorechallenge")) ||
		( m_gamemode_id == CRCD(0xf135ecb6,"scorechallenge")) ||
		( m_gamemode_id == CRCD(0x3d6d444f,"firefight")) ||
		( m_gamemode_id == CRCD(0xbff33600,"netfirefight")))
	{
		game_info_msg.m_TimeLimit = 0;
	}
	else
	{

		if( m_gamemode_id == Script::GenerateCRC( "netctf" ))
		{
			uint32 mode;

            mode = pPreferences->GetPreferenceChecksum( Script::GenerateCRC("ctf_game_type"), Script::GenerateCRC("checksum") );
			if( mode == CRCD(0x938ab24b,"timed_ctf"))
			{
				game_info_msg.m_TimeLimit = time_limit;
			}
			else
			{
				game_info_msg.m_TimeLimit = 0;
			}
		}
		else
		{
			game_info_msg.m_TimeLimit = time_limit;
		}
	}
	
	game_info_msg.m_StopAtZero = (char) ShouldStopAtZero();
	game_info_msg.m_TargetScore = pPreferences->GetPreferenceValue( Script::GenerateCRC("target_score"), Script::GenerateCRC("score") );
	Dbg_Printf( "Target score %d\n", (int)game_info_msg.m_TargetScore );
	game_info_msg.m_TeamMode = GetNumTeams();

	// Here, make sure we get a game id that does not match our last game id
	do
	{
		game_info_msg.m_GameId = 1 + (unsigned char) Mth::Rnd( 255 );
	} while( game_info_msg.m_GameId == m_last_end_of_run_game_id );

	game_info_msg.m_CrownSpawnPoint = m_crown_spawn_point;
	memcpy( game_info_msg.m_StartPoints, m_skater_starting_points, Mdl::Skate::vMAX_SKATERS * sizeof( int ));

	msg_desc.m_Data = &game_info_msg;
	msg_desc.m_Length = sizeof( MsgGameInfo );
	msg_desc.m_Id = MSG_ID_GAME_INFO;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	for( player = FirstPlayerInfo( sh, true ); player; player = NextPlayerInfo( sh, true ))
	{
		player->m_BestCombo = 0;
		player->ClearCTFState();
		player->m_flags.ClearMask( PlayerInfo::mRESTARTING );
		server->EnqueueMessage( player->GetConnHandle(), &msg_desc );

		// In splitscreen games, just send it to the first client
		if( InNetGame() == false )
		{
			break;
		}
	}

	// Clear the king of the hill
	if(( player = GetKingOfTheHill()))
	{
		player->MarkAsKing( false );
	}
	SetCurrentLeader( NULL );
	SetCurrentLeadingTeam( vNO_TEAM );
	ClearTriggerEventList();
	ResetPlayers();
	g_CheatsEnabled = false;

#ifdef __PLAT_NGPS__
	if( mpStatsMan->IsLoggedIn())
	{
		mpStatsMan->StartNewGame();
	}
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::ResetPlayers( void )
{
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;

	for( player = FirstPlayerInfo( sh, true ); player;
			player = NextPlayerInfo( sh, true ))
	{
		Mdl::Score* score;
		player->m_flags.ClearMask( PlayerInfo::mRUN_ENDED );
		if( player->m_Skater )
		{
			Obj::CSkaterScoreComponent* p_skater_score_component = GetSkaterScoreComponentFromObject(player->m_Skater);
			Dbg_Assert(p_skater_score_component);
			score = p_skater_score_component->GetScore();
			
			if( score )
			{
				score->Reset();
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::SetLevel( uint32 level_id )
{
	m_level_id = level_id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetNetworkGameId( unsigned char id )
{
	m_game_id = id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

unsigned char	Manager::GetNetworkGameId( void )
{
	return m_game_id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 Manager::GetGameTypeFromPreferences( void )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	Prefs::Preferences* pPreferences;
	 
	pPreferences = NULL;
	if( InNetGame())
	{
		pPreferences = GetNetworkPreferences();
	}
	else 
	{
		pPreferences = skate_mod->GetSplitScreenPreferences();
	}

	Script::CStruct* pStructure = pPreferences->GetPreference( Script::GenerateCRC("game_type") );

	Dbg_Assert( pStructure );

	uint32 gametype_id;
	pStructure->GetChecksum( "checksum", &gametype_id, true );

	return gametype_id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 Manager::GetLevelFromPreferences( void )
{
	uint32 level_id;

	Prefs::Preferences* pPreferences;

	pPreferences = GetNetworkPreferences();
	Script::CStruct* pStructure = pPreferences->GetPreference( Script::GenerateCRC("level") );

	Dbg_Assert( pStructure );
    pStructure->GetChecksum( "checksum", &level_id, true );
	
	return level_id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Manager::GetFireballLevelFromPreferences( void )
{
	uint32 checksum;
	Script::CStruct* pStructure;
		
	pStructure = m_network_preferences.GetPreference( CRCD(0xce238f5d,"fireball_difficulty") );
	pStructure->GetChecksum( "Checksum", &checksum, true );
	if( checksum == Script::GenerateCRC( "num_1" ))
	{
		return 1;
	}
	else if( checksum == Script::GenerateCRC( "num_2" ))
	{
		return 2;
	}
	else if( checksum == Script::GenerateCRC( "num_3" ))
	{
		return 3;
	}                                              
	else if( checksum == Script::GenerateCRC( "num_4" ))
	{
		return 4;
	}                                              
	else if( checksum == Script::GenerateCRC( "num_5" ))
	{
		return 5;
	}                                              

	return 3;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::ToggleMetrics( void )
{
	Mlp::Manager * mlp_manager = Mlp::Manager::Instance();

	m_metrics_on ^= 1;

	if( m_metrics_on )
	{
		mlp_manager->AddLogicTask( *m_render_metrics_task );
	}
	else
	{
		m_render_metrics_task->Remove();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::ToggleScores( void )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Game::CGameMode* pGameMode;
	bool show_score;

	pGameMode = skate_mod->GetGameMode();
	show_score = pGameMode->ShouldAccumulateScore();

	m_scores_on ^= 1;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::TogglePlayerNames( void )
{
	m_draw_player_names ^= 1;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ShouldDrawPlayerNames( void )
{
	Prefs::Preferences* pPreferences;
	Script::CStruct* pStructure;
	uint32 checksum = 0;
		
	pPreferences = GetNetworkPreferences();
	pStructure = pPreferences->GetPreference( CRCD(0xf7dd263,"show_names") );
	pStructure->GetChecksum( CRCD(0x21902065,"checksum"), &checksum, false );
	return( checksum == CRCD(0xf81bc89b,"boolean_true") );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::RemoveModemStateTask( void )
{
	m_modem_state_task->Remove();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ConnectToInternet( uint32 success_script, uint32 failure_script )
{
#ifdef __PLAT_NGPS__
	Net::Manager * net_man = Net::Manager::Instance();
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();
	bool was_online, is_online;

	m_connection_success_script = success_script;
	m_connection_failure_script = failure_script;
    
	Dbg_Printf( "Connecting to the internet\n" );
	was_online = net_man->IsOnline();
	if( was_online )
	{
		is_online = true;
	}
	else
	{
		m_last_modem_state = -1;
		is_online = net_man->ConnectToInternet();
	}
	
	if( is_online )
	{   
		if( m_connection_success_script > 0 )
		{
			Script::RunScript( m_connection_success_script );
		}
	}
	else if(( net_man->GetConnectionType() == Net::vCONN_TYPE_MODEM ) ||
			( net_man->GetConnectionType() == Net::vCONN_TYPE_PPPOE ))
	{   
		mlp_man->AddLogicTask( *m_modem_state_task );
	}
	
	return is_online;
#else
#ifdef __PLAT_XBOX__
	mpAuthMan->UserLogon();
#endif		// __PLAT_XBOX__
	
	return false;
#endif		// __PLAT_NGPS__	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::CancelConnectToInternet( void )
{
	
	Net::Manager * net_man = Net::Manager::Instance();
	Manager * gamenet_man = Manager::Instance();
	
	switch( net_man->GetModemState())
	{
		case Net::vMODEM_STATE_DIALING:
		case Net::vMODEM_STATE_CONNECTED:
			gamenet_man->RemoveModemStateTask();
			gamenet_man->DisconnectFromInternet();
			break;
		case Net::vMODEM_STATE_ERROR:
#ifdef __PLAT_NGPS__
			if( net_man->GetModemError() == SNTC_ERR_NOMODEM )
			{
				Script::RunScript( gamenet_man->m_connection_failure_script );
			}
			else
			{
				gamenet_man->RemoveModemStateTask();
				gamenet_man->DisconnectFromInternet();
				break;
			}
#endif		// __PLAT_NGPS__
			gamenet_man->RemoveModemStateTask();
			break;;
		case Net::vMODEM_STATE_DISCONNECTED:
			Dbg_Printf( "Removing modem state task\n" );
			gamenet_man->RemoveModemStateTask();
			Script::RunScript( gamenet_man->m_connection_failure_script );
			break;
		case Net::vMODEM_STATE_LOGGED_IN:
		{
			net_man->SetModemState( Net::vMODEM_STATE_READY_TO_TRANSMIT );
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

void	Manager::DisconnectFromInternet( uint32 callback_script )
{
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();
	Net::Manager * net_man = Net::Manager::Instance();

	Dbg_Assert(	( net_man->GetConnectionType() == Net::vCONN_TYPE_MODEM ) ||
				( net_man->GetConnectionType() == Net::vCONN_TYPE_PPPOE ));
	   
	m_last_modem_state = -1;
	if( callback_script != 0 )
	{
        m_connection_failure_script = callback_script;
	}

	mlp_man->AddLogicTask( *m_modem_state_task );
	SetServerListState( vSERVER_LIST_STATE_SHUTDOWN );
	net_man->DisconnectFromInternet();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::CreateNetPanelMessage( bool broadcast, uint32 message_checksum, char* string_parm_1,
										char* string_parm_2, Obj::CMovingObject* object, char* properties, bool important,
										int time )
{
	if( broadcast )
	{
		PlayerInfo* player;
		Lst::Search< PlayerInfo > sh;
		MsgPanelMessage msg;
		Net::MsgDesc msg_desc;

		msg.m_Parm1[0] = '\0';
		msg.m_Parm2[0] = '\0';
		msg.m_StringId = message_checksum;
		msg.m_Time = time;
		msg.m_PropsStructureCRC = (int) Script::GenerateCRC( properties );
		if( string_parm_1 )
		{
			strcpy( msg.m_Parm1, string_parm_1 );
		}
		if( string_parm_2 )
		{
			strcpy( msg.m_Parm2, string_parm_2 );
		}

		msg_desc.m_Data = &msg;
		msg_desc.m_Length = sizeof( MsgPanelMessage );
		msg_desc.m_Id = MSG_ID_PANEL_MESSAGE;
		if( important )
		{
			msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
			msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
		}

		Dbg_Assert( m_server );
        for( player = FirstPlayerInfo( sh, true ); player;
				player = NextPlayerInfo( sh, true ))
		{
			m_server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
		}
	}
	else
	{   
		Script::CScriptStructure* pStructure = new Script::CScriptStructure;
		const char* text;	
		char temp_msg[256];
		char final_msg[256];
		char* format_start;

		text = Script::GetLocalString( message_checksum );
		strcpy( temp_msg, text );
		if(( format_start = strstr( temp_msg, "%s0" )))
		{
			*format_start = '\0';

			strcpy( final_msg, temp_msg );
			Dbg_Assert( string_parm_1 );	// Need a parameter for this format string
			strcat( final_msg, string_parm_1 );
			format_start += 3;	// skip the format tag
			strcat( final_msg, format_start );

			pStructure->AddComponent( Script::GenerateCRC( "Text" ), ESYMBOLTYPE_STRING, final_msg );
		}
		else
		{
			pStructure->AddComponent( Script::GenerateCRC( "Text" ), ESYMBOLTYPE_STRING, text );
		}
		
		pStructure->AddComponent( Script::GenerateCRC( "id" ), ESYMBOLTYPE_NAME, Script::GenerateCRC( "net_panel_msg" ));
		pStructure->AddInteger( Script::GenerateCRC( "msg_time" ), time );
		Script::RunScript( "create_net_panel_message", pStructure );
		
		delete pStructure;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ShouldSendScoreUpdates( void )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	Game::CGameMode* pGameMode = skate_mod->GetGameMode();
	
	// We need to send score updates in goal attack
	if( ( pGameMode->ShouldAccumulateScore() == false ) &&
		( pGameMode->ShouldTrackBestCombo() == false ) &&
		( pGameMode->GetNameChecksum() != CRCD(0xbff33600,"netfirefight")) &&
		( pGameMode->GetNameChecksum() != CRCD(0xec200eaa,"netgoalattack")) &&
		( pGameMode->GetNameChecksum() != CRCD(0x6c5ff266,"netctf")))
	{
		return false;
	}
	
	if( pGameMode->EndConditionsMet() && ( pGameMode->GetNameChecksum() != CRCD(0xbff33600,"netfirefight")))
	{
		return false;
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::SetCurrentLeader( PlayerInfo* player )
{
	m_leader = player;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

PlayerInfo*		Manager::GetCurrentLeader( void )
{
	return m_leader;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::SetCurrentLeadingTeam( int team )
{
	m_leading_team = team;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int				Manager::GetCurrentLeadingTeam( void )
{
	return m_leading_team;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetLastScoreUpdateTime( int time )
{
	m_last_score_update = time;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Manager::GetLastScoreUpdateTime( void )
{
	return m_last_score_update;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::MarkReceivedFinalScores( bool received )
{
	m_received_final_scores = received;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::HaveReceivedFinalScores( void )
{
	return m_received_final_scores;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::AddSpawnedTriggerEvent( int node, int obj_id, uint32 script )
{
	TriggerEvent* queued_script;
	Lst::Search< TriggerEvent > sh;
	
	// Check to make sure we don't queue duplicate scripts
	for( queued_script = sh.FirstItem( m_trigger_events ); queued_script; 
			queued_script = sh.NextItem())
	{
		if( queued_script->m_Script == script )
		{
			return;
		}
	}

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());

	TriggerEvent* event = new TriggerEvent;

	event->m_Node = node;
	event->m_ObjID = obj_id;
	event->m_Script = script;

	m_trigger_events.AddToTail( event );

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::ClearTriggerEventList( void )
{
	Lst::Search< TriggerEvent > sh;
	TriggerEvent* event, *next;

	for( event = sh.FirstItem( m_trigger_events ); event; event = next )
	{
		next = sh.NextItem();
		delete event;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetProSetFlags( char flags )
{
	m_proset_flags = flags;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SendChatMessage( char* message )
{
	
	MsgChat msg;
	int size;
	PlayerInfo* player;
	Script::CStruct* p_params;
	char final_msg[256];
	Net::MsgDesc msg_desc;

    Dbg_Assert( message );
	Dbg_Assert( m_client[0] );

	// No point in printing/sending an empty message
	if( message[0] == '\0' )
	{
		return;
	}

	player = GetLocalPlayer();
	Dbg_Assert( player );
    
	strcpy( msg.m_Name, player->m_Name );
	strcpy( msg.m_ChatMsg, message );
	if( player->IsObserving() )
	{
		msg.m_ObjId = Mdl::Skate::vMAX_SKATERS; 	// Signifies "observer"
	}
	else
	{
		msg.m_ObjId = player->m_Skater->GetID();
	}

	size = sizeof( char ) + ( vMAX_PLAYER_NAME_LEN + 1 ) + strlen( message ) + 1;

	msg_desc.m_Data = &msg;
	msg_desc.m_Length = size;
	msg_desc.m_Id = MSG_ID_CHAT;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	m_client[0]->EnqueueMessageToServer( &msg_desc );
	
	p_params = new Script::CStruct;	
	sprintf( final_msg, "%s : %s", player->m_Name, message );
	p_params->AddString( "text", final_msg );
	Script::RunScript("create_console_message", p_params );
	delete p_params;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::SetJoinState( JoinState state )
{
	m_join_state = state;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

JoinState		Manager::GetJoinState( void )
{
	return m_join_state;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::SetObserverCommand( ObserverCommand command )
{
	m_observer_command = command;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

ObserverCommand		Manager::GetObserverCommand( void )
{
	return m_observer_command;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::LaunchQueuedScripts( void )
{
	TriggerEvent* queued_script, *next;
	Lst::Search< TriggerEvent > sh;
	Sfx::CSfxManager * pSfxManager = Sfx::CSfxManager::Instance();
	Mdl::Skate * mod = Mdl::Skate::Instance();
	Script::CScript* pScript;
    
    Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
	
	mod->SetLaunchingQueuedScripts();
	// Remember the original volume, then turn sound off so scripts don't make noise
	float orig_volume = pSfxManager->GetMainVolume();
	pSfxManager->SetMainVolume( 0.0f );

    Dbg_Printf( "********************* SPAWNING QUEUED SCRIPTS ******************\n" );
	
	Script::RunScript( "PreRunQueuedScripts" );
	for( queued_script = sh.FirstItem( m_trigger_events ); queued_script; queued_script = next )
	{
		Script::EScriptReturnVal ret_val;

		next = sh.NextItem();
	
		pScript = Script::SpawnScript( queued_script->m_Script, NULL, 0, NULL, 
									   queued_script->m_Node ); // K: The 0,NULL bit means no callback script specified
		#ifdef __NOPT_ASSERT__
		pScript->SetCommentString("Spawned from Manager::LaunchQueuedScripts");
		#endif
		pScript->mpObject = mod->GetObjectManager()->GetObjectByID( queued_script->m_ObjID );; 

		// Now run script to completion
		do 
		{
			ret_val = pScript->Update();
			// Script must not get blocked, otherwise it'll hang in this loop forever.
			Dbg_MsgAssert( ret_val!=Script::ESCRIPTRETURNVAL_BLOCKED,("\n%s\nScript got blocked when being run by RunScript.",pScript->GetScriptInfo()));
		
		} while( ret_val != Script::ESCRIPTRETURNVAL_FINISHED );
		Script::KillSpawnedScript(pScript);
		delete queued_script;
	}

	Script::RunScript( "PostRunQueuedScripts" );

	Dbg_Printf( "********************* FINISHED SPAWNING QUEUED SCRIPTS ******************\n" );
	 
	// Back to normal volume
	pSfxManager->SetMainVolume( orig_volume );
	mod->ClearLaunchingQueuedScripts();

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CCrown*		Manager::GetCrown( void )
{
	return m_crown;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::CleanupObjects( void )
{
	// Nullify references to now-dead objects
	m_crown = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::SetCrownSpawnPoint( char spawn_point )
{
	m_crown_spawn_point = spawn_point;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*			Manager::GetNetThreadStack( void )
{
	return m_net_thread_stack;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::SetNetThreadId( int net_thread_id )
{
	m_net_thread_id = net_thread_id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int				Manager::GetNetThreadId( void )
{
	return m_net_thread_id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::SendEndOfRunMessage( void )
{
	Net::Client* client;
	Net::MsgDesc msg_desc;

	Dbg_Assert( HaveSentEndOfRunMessage() == false );

	client = m_client[0];
	Dbg_Assert( client );

	msg_desc.m_Data = &m_game_id;
	msg_desc.m_Length = sizeof( char );
	msg_desc.m_Id = MSG_ID_RUN_HAS_ENDED;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	client->EnqueueMessageToServer( &msg_desc );

	m_last_end_of_run_game_id = m_game_id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			Manager::HaveSentEndOfRunMessage( void )
{
	if( m_last_end_of_run_game_id == m_game_id )
	{
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			Manager::GameIsOver( void )
{
	return m_game_over;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::MarkGameOver( void )
{
	m_game_over = true;
}
                                          
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::ClearGameOver( void )
{
	m_game_over = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			Manager::HasCheatingOccurred( void )
{
	return m_cheating_occurred;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::CheatingOccured( void )
{
	m_cheating_occurred = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::ClearCheating( void )
{
	m_cheating_occurred = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::RespondToReadyQuery( void )
{
	MsgReady msg;
	Net::MsgDesc msg_desc;
	int i;

	msg.m_Time = m_latest_ready_query;

	msg_desc.m_Data = &msg;
	msg_desc.m_Length = sizeof( MsgReady );
	msg_desc.m_Id = MSG_ID_READY_RESPONSE;
	msg_desc.m_Queue = Net::QUEUE_IMPORTANT;
	for( i = 0; i < 2; i++ )
	{
		if( m_client[i] )
		{
			m_client[i]->EnqueueMessageToServer( &msg_desc );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::ToggleProSet( char bit, uint32 param_id )
{
	MsgToggleProSet msg;
	Net::MsgDesc msg_desc;
	Net::Server* server;
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;

	m_proset_flags.Toggle( bit );
	
	msg.m_Bit = bit;
	msg.m_ParamId = param_id;

	msg_desc.m_Data = &msg;
	msg_desc.m_Length = sizeof( MsgToggleProSet );
	msg_desc.m_Id = MSG_ID_TOGGLE_PROSET;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;

	server = GetServer();
	for( player = FirstPlayerInfo( sh, true ); player; player = NextPlayerInfo( sh, true ))
	{
		if( player->IsLocalPlayer())
		{
			continue;
		}
		server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		Manager::ScriptStartCTFGame(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();

	mlp_man->AddLogicTask( *gamenet_man->m_ctf_logic_task );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		Manager::ScriptEndCTFGame(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();

	gamenet_man->m_ctf_logic_task->Remove();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptTookFlag(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	PlayerInfo* player, *capture_player;
	Net::Server* server;
	Lst::Search< PlayerInfo > sh;
	MsgFlagMsg msg;
	Net::MsgDesc msg_desc;
	int obj_id, team;

	pParams->GetInteger( "player", &obj_id );
	pParams->GetInteger( "flag_team", &team );
	
	capture_player = gamenet_man->GetPlayerByObjectID( obj_id );
	if( capture_player == NULL )
	{
		return true;
	}

	msg.m_ObjId = (char) obj_id;
	msg.m_Team = (char) team;
		
	msg_desc.m_Data = &msg;
	msg_desc.m_Length = sizeof( MsgFlagMsg );
	msg_desc.m_Id = MSG_ID_TOOK_FLAG;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;

	server = gamenet_man->GetServer();
	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
	{
		if( player->IsLocalPlayer())
		{
			capture_player->TookFlag( team );
			continue;
		}
		server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptCapturedFlag(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	PlayerInfo* player, *capture_player;
	Net::Server* server;
	Lst::Search< PlayerInfo > sh;
	MsgFlagMsg msg;
	Net::MsgDesc msg_desc;
	int obj_id, team;

	pParams->GetInteger( "player", &obj_id );
	capture_player = gamenet_man->GetPlayerByObjectID( obj_id );
	if(( capture_player == NULL ) || ( capture_player->m_flags.TestMask( PlayerInfo::mRESTARTING )))
	{
		return true;
	}

	team = capture_player->HasWhichFlag();
	msg.m_ObjId = (char) obj_id;
	msg.m_Team = (char) team;
		
	msg_desc.m_Data = &msg;
	msg_desc.m_Length = sizeof( MsgFlagMsg );
	msg_desc.m_Id = MSG_ID_CAPTURED_FLAG;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;

	server = gamenet_man->GetServer();
	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
	{
		if( player->IsLocalPlayer())
		{
			Mdl::Score* score_obj;
			
			Obj::CSkaterScoreComponent* p_skater_score_component = GetSkaterScoreComponentFromObject(capture_player->m_Skater);
			Dbg_Assert(p_skater_score_component);
			score_obj = p_skater_score_component->GetScore();
			
			Dbg_Assert(score_obj);
			score_obj->SetTotalScore( score_obj->GetTotalScore() + 1 );

			capture_player->CapturedFlag( team );
			continue;
		}
		server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptRetrievedFlag(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	PlayerInfo* player, *retrieve_player;
	Net::Server* server;
	Lst::Search< PlayerInfo > sh;
	MsgFlagMsg msg;
	Net::MsgDesc msg_desc;
	int obj_id, team;

	pParams->GetInteger( "player", &obj_id );
	pParams->GetInteger( "flag_team", &team );
	
	retrieve_player = gamenet_man->GetPlayerByObjectID( obj_id );
	if( retrieve_player == NULL )
	{
		return true;
	}

	msg.m_ObjId = (char) obj_id;
	msg.m_Team = (char) team;
		
	msg_desc.m_Data = &msg;
	msg_desc.m_Length = sizeof( MsgFlagMsg );
	msg_desc.m_Id = MSG_ID_CAPTURED_FLAG;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;

	server = gamenet_man->GetServer();
	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
	{
		if( player->IsLocalPlayer())
		{
			retrieve_player->CapturedFlag( team );
			continue;
		}
		server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptHasFlag(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	PlayerInfo* player;
	int obj_id;

	pParams->GetInteger( "player", &obj_id );
	
	player = gamenet_man->GetPlayerByObjectID( obj_id );
	if( player == NULL )
	{
		return true;
	}

	return player->HasCTFFlag();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptDisplayFlagBaseWarning(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	static Tmr::Time s_last_show = 0;

	if(( Tmr::GetTime() - s_last_show ) > Tmr::Seconds( 3 ))
	{
		Script::RunScript( CRCD(0x55af7c02,"display_flag_base_warning"), NULL );
		s_last_show = Tmr::GetTime();
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptTeamFlagTaken(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;
	int team;

	pParams->GetInteger( "team", &team );
	
	for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
	{
		if( player->HasWhichFlag() == team )
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

// @script | SpawnCrown | spawn the king of the hill crown
bool	Manager::ScriptSpawnCrown(Script::CStruct *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	// Spawn the king of the hill crown
	gamenet_man->m_crown = new Obj::CCrown;

	int i, spawn_pt;
		
	Script::CArray *pNodeArray=Script::GetArray("NodeArray");

	// if there is no node array, then there are no restart points
	if (!pNodeArray)
	{
		return false;
	}
						 
	i = 0;
	spawn_pt = gamenet_man->m_crown_spawn_point;
		
	// collect a table of all crown nodes
	int max_crown_node = 0;
	Script::CStruct *pCrownNodeTab[vNUM_CROWN_SPAWN_POINTS];
	int crown_nodes[vNUM_CROWN_SPAWN_POINTS];
	while( i < (int)pNodeArray->GetSize() )
	{
		uint32	TypeChecksum;
		Script::CStruct *pNode=pNodeArray->GetStructure(i);
		
		TypeChecksum = 0;
		pNode->GetChecksum("Type",&TypeChecksum);
		if( TypeChecksum == 0xaf86421b )	// checksum of "Crown"
		{
			Dbg_MsgAssert( max_crown_node < vNUM_CROWN_SPAWN_POINTS, ( "Too many crown spawn points" ));
			crown_nodes[max_crown_node] = i;
			pCrownNodeTab[max_crown_node++] = pNode;
		}
		i++;
	}
	Dbg_MsgAssert(max_crown_node, ("no crown nodes found"));
		
	// there may be fewer crown points than the one we want, so
	// truncate the request number if so
	spawn_pt = spawn_pt % max_crown_node;

	// now set up crown point in world
	Mth::Vector pos;
	SkateScript::GetPosition( pCrownNodeTab[spawn_pt], &pos );
    
	gamenet_man->m_crown->InitCrown( skate_mod->GetObjectManager(), pCrownNodeTab[spawn_pt] );

    PlayerInfo* king;
	//gamenet_man->m_crown->AddToWorld( viewer_mod->GetCurrentWorld(), &pos );
	
	// We may have a king "queued" up (i.e. we joined a KOTH game in progress.
	// If so, coronate the king straight away
	king = gamenet_man->GetKingOfTheHill();
	if( king )
	{
		gamenet_man->m_crown->PlaceOnKing( king->m_Skater );
	}
    
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | NetworkGamePending | 
bool Manager::ScriptNetworkGamePending(Script::CStruct *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
    
	return gamenet_man->m_game_pending;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | StartNetworkGame | 
bool Manager::ScriptStartNetworkGame(Script::CStruct *pParams, Script::CScript *pScript)
{
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Manager * gamenet_man = Manager::Instance();
    
	if( gamenet_man->m_start_network_game_task->InList())
	{
		return true;
	}
	if( gamenet_man->m_change_level_task->InList())
	{
		return true;
	}

	gamenet_man->m_game_pending = true;
	if( gamenet_man->InNetGame())
	{ 
		if( gamenet_man->GetGameTypeFromPreferences() == Script::GenerateCRC( "netgoalattack" ))
		{
			s_time_to_start_game = Tmr::GetTime() + vTIME_BEFORE_STARTING_GOALATTACK;
			
		}
		else
		{
			s_time_to_start_game = Tmr::GetTime() + vTIME_BEFORE_STARTING_NETGAME;
		}
	}
	else
	{
		s_time_to_start_game = Tmr::GetTime();
	}
			
	mlp_man->AddLogicTask( *gamenet_man->m_start_network_game_task );
	
	if( gamenet_man->InNetGame())
	{
		if( gamenet_man->GetHostMode() == vHOST_MODE_AUTO_SERVE )
		{
			gamenet_man->CreateNetPanelMessage( true, Script::GenerateCRC("net_message_auto_starting_game"),
											 gamenet_man->GetGameModeName(), NULL, NULL, "netstatusprops", true,
												Tmr::Seconds( 3 ));    
		}
		else
		{
			gamenet_man->CreateNetPanelMessage( true, Script::GenerateCRC("net_message_starting_game"),
										 gamenet_man->GetGameModeName(), NULL, NULL, "netstatusprops", true,
											Tmr::Seconds( 3 ));
		}

		if( skate_mod->GetGameMode()->IsTeamGame())
		{
			Net::Server* server;
			PlayerInfo* player;
			Lst::Search< PlayerInfo > sh;
			Net::MsgDesc msg_desc;
		 
			server = gamenet_man->GetServer();
			Dbg_Assert( server );

			msg_desc.m_Id = MSG_ID_KILL_TEAM_FLAGS;
			msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
			msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
			for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
			{
				server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
			}
		}
	}
	
	// Compile the list of selected goals to send to all clients at the start of goal attack games
	if( gamenet_man->InNetGame() && ( gamenet_man->GetGameTypeFromPreferences() == Script::GenerateCRC( "netgoalattack" )))
	{
		int i, num_goals;
		Game::CGoalManager* pGoalManager;
		Game::CGoal* pGoal;
		Net::MsgMax msg;
		Net::MsgDesc msg_desc;
		uint32 goal_id;
		char* data;
		Net::Server* server;
		PlayerInfo* player;
		Lst::Search< PlayerInfo > sh;
		 
		pGoalManager = Game::GetGoalManager();
		server = gamenet_man->GetServer();
		Dbg_Assert( server );

		data = msg.m_Data;
		*data++ = 1;	// This indicates that they should bring up the goal summary dialog
		num_goals = pGoalManager->GetNumGoals();
		Dbg_Printf( "*** Num Goals: %d\n" );
		for( i = 0; i < num_goals; i++ )
		{
			pGoal = pGoalManager->GetGoalByIndex( i );
			Dbg_Assert( pGoal );
			if( pGoal->IsSelected())
			{
				pGoal->UnBeatGoal();
				goal_id = pGoal->GetGoalId();
				Dbg_Printf( "*** Goals %d selected, id 0x%x\n", i, goal_id );
				memcpy( data, &goal_id, sizeof( uint32 ));
				data += sizeof( uint32 );
			}
		}

		// zero-terminate the list of goals
		goal_id = 0;
		memcpy( data, &goal_id, sizeof( uint32 ));
		data += sizeof( uint32 );

		msg_desc.m_Data = &msg;
		msg_desc.m_Length = (int) ( data - msg.m_Data );
		msg_desc.m_Id = MSG_ID_SELECT_GOALS;
		msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
		msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
		for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
		{
			// No need to send to server
			if( player->IsLocalPlayer())
			{
				continue;
			}
			server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
		}
		
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptSendGameOverToObservers(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;
	Manager * gamenet_man = Manager::Instance();

	Dbg_Printf( "**** In ScriptSendGameOverToObservers\n" );
	if( gamenet_man->OnServer())
	{
		Net::Server* server;
		Net::MsgDesc msg_desc;

		server = gamenet_man->GetServer();

		msg_desc.m_Id = MSG_ID_GAME_OVER;
		msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
		msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
		for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
		{
			if( player->IsObserving())
			{
				Dbg_Printf( "**** Sending message to observer\n" );
                server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
			}
		}
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		Manager::ScriptEndNetworkGame(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;
	Manager * gamenet_man = Manager::Instance();

	if( gamenet_man->OnServer())
	{
		Net::Server* server;
		Net::MsgDesc msg_desc;

		server = gamenet_man->GetServer();

		Script::RunScript( "do_backend_retry" );
	
		msg_desc.m_Id = MSG_ID_END_GAME;
		msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
		msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
		for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
		{
			if( player->IsLocalPlayer())
			{
				continue;
			}
	
			server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
		}
	}
	else
	{
		Net::Client* client;
		Net::MsgDesc msg_desc;

		client = gamenet_man->GetClient( 0 );

		msg_desc.m_Id = MSG_ID_FCFS_END_GAME;
		msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
		msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
		client->EnqueueMessageToServer( &msg_desc );
	}

	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetJoinMode | 
// @uparm 1 | mode (0 for play, anything else for observe)
bool Manager::ScriptSetJoinMode(Script::CStruct *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	int mode = 0;
    
	mode = 0;
	pParams->GetInteger( NONAME, &mode );
	if( mode == 0 )
	{
		gamenet_man->SetJoinMode( vJOIN_MODE_PLAY );
	}
	else
	{
		gamenet_man->SetJoinMode( vJOIN_MODE_OBSERVE );
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | SetHostMode | 
// @uparm 1 | mode (0 for server, 1 for auto_serve, 2 for fcfs)
bool Manager::ScriptSetHostMode(Script::CStruct *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	int mode = 0;
    
	mode = 0;
	pParams->GetInteger( NONAME, &mode );

	gamenet_man->SetHostMode( (HostMode) mode );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptGetNumTeams(Script::CStruct *pParams, Script::CScript *pScript)
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Script::CStruct* pass_back_params;
	int num_teams;
	
	num_teams = skate_mod->GetGameMode()->NumTeams();
	pass_back_params = pScript->GetParams();
	pass_back_params->AddInteger( "num_teams", num_teams );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptGetNumPlayersOnTeam(Script::CStruct *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	Script::CStruct* pass_back_params;
	int team_num = 0;
	int num_players = 0;
	
	pParams->GetInteger( "team", &team_num, Script::ASSERT );
	num_players = gamenet_man->NumTeamMembers( team_num );
	pass_back_params = pScript->GetParams();
	pass_back_params->AddInteger( "num_members", num_players );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptGetMyTeam(Script::CStruct *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	Script::CStruct* pass_back_params;
	PlayerInfo *player;
	
	player = gamenet_man->GetLocalPlayer();
	if( player )
	{
		pass_back_params = pScript->GetParams();
		pass_back_params->AddInteger( "team", player->m_Team );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptSetNumTeams(Script::CStruct *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	PlayerInfo* team_player, *player;
	Lst::Search< PlayerInfo > team_sh, sh;
	MsgChangeTeam msg;
	MsgByteInfo num_teams_msg;
	Net::MsgDesc team_msg_desc;
	int num_teams = 0;
	Net::Server* server;
    
	pParams->GetInteger( NONAME, &num_teams );

	if( !gamenet_man->OnServer())
	{
		Net::Client* client;
		MsgByteInfo team_msg;
		Net::MsgDesc msg_desc;

		client = gamenet_man->GetClient( 0 );

		team_msg.m_Data = num_teams;
		Dbg_Printf( "Telling server to create %d teams\n", num_teams );

		msg_desc.m_Data = &team_msg;
		msg_desc.m_Length = sizeof( MsgByteInfo );
		msg_desc.m_Id = MSG_ID_FCFS_SET_NUM_TEAMS;
		msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
		msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
		client->EnqueueMessageToServer( &msg_desc );
		return true;
	}

	if( ( num_teams > 0 ) &&
		( !skate_mod->GetGameMode()->IsTeamGame()))
	{
		skate_mod->GetGameMode()->SetNumTeams( num_teams );
		if( skate_mod->GetLocalSkater())
		{
			Script::RunScript( "PrepareSkaterForMove", NULL, skate_mod->GetLocalSkater());
			skate_mod->move_to_restart_point( skate_mod->GetLocalSkater());
		}
	}
	else
	{
		skate_mod->GetGameMode()->SetNumTeams( num_teams );
	}
	
	server = gamenet_man->GetServer();
	if( server == NULL )
	{
		return true;
	}

	num_teams_msg.m_Data = num_teams;

	team_msg_desc.m_Data = &num_teams_msg;
	team_msg_desc.m_Length = sizeof( MsgByteInfo );
	team_msg_desc.m_Id = MSG_ID_SET_NUM_TEAMS;
	team_msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	team_msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	
	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
	{
		if( player->IsLocalPlayer())
		{
			continue;
		}
		server->EnqueueMessage( player->GetConnHandle(), &team_msg_desc );
	}

	team_msg_desc.m_Data = &msg;
	team_msg_desc.m_Length = sizeof( MsgChangeTeam );
	team_msg_desc.m_Id = MSG_ID_TEAM_CHANGE;
	team_msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	team_msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	for( team_player = gamenet_man->FirstPlayerInfo( team_sh ); team_player; team_player = gamenet_man->NextPlayerInfo( team_sh ))
	{
		if( team_player->m_Team >= num_teams )
		{
			msg.m_Team = (unsigned char) vTEAM_RED;
			msg.m_ObjID = (unsigned char) team_player->m_Skater->GetID();

			for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
			{
				if( player->IsLocalPlayer())
				{
					continue;
				}
				server->EnqueueMessage( player->GetConnHandle(), &team_msg_desc );
			}
			team_player->m_Team = vTEAM_RED;
		}
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptJoinTeam(Script::CStruct *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	PlayerInfo* player;
	MsgChangeTeam msg;
	Net::MsgDesc msg_desc;
	Net::Client* client;
	uint32 team_checksum;
	int team;
	static Tmr::Time s_last_join_time = 0;

	player = gamenet_man->GetLocalPlayer();
	if( ( player == NULL ) || 
		( player->m_Skater == NULL ))
	{
		return false;
	}

	pParams->GetChecksum( NONAME, &team_checksum );
	if( team_checksum == Script::GenerateCRC( "blue" ))
	{
		team = vTEAM_BLUE;
	}
	else if( team_checksum == Script::GenerateCRC( "red" ))
	{
		team = vTEAM_RED;
	}
	else if( team_checksum == Script::GenerateCRC( "green" ))
	{
		team = vTEAM_GREEN;
	}
	else 
	{
		team = vTEAM_YELLOW;
	}
    
	msg.m_Team = (unsigned char) team;
	msg.m_ObjID = (unsigned char) player->m_Skater->GetID();
	client = gamenet_man->GetClient( 0 );
	if(( Tmr::GetTime() - s_last_join_time ) < 150 )
	{
		return true;
	}
	s_last_join_time = Tmr::GetTime();

	msg_desc.m_Data = &msg;
	msg_desc.m_Length = sizeof( MsgChangeTeam );
	msg_desc.m_Id = MSG_ID_REQUEST_CHANGE_TEAM;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	client->EnqueueMessageToServer( &msg_desc );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptConnectedToPeer(Script::CStruct *pParams, Script::CScript *pScript)
{
#ifdef __PLAT_NGPS__
	Manager * gamenet_man = Manager::Instance();
	PEER peer;

	peer = gamenet_man->mpLobbyMan->GetPeer();

	return ( peer && peerIsConnected( peer ));
#else
	return false;
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptInGroupRoom(Script::CStruct *pParams, Script::CScript *pScript)
{
#ifndef __PLAT_NGPS__
	return false;
#else
	Manager * gamenet_man = Manager::Instance();
	return gamenet_man->mpLobbyMan->InGroupRoom();
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptIsHost(Script::CStruct *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	PlayerInfo* player;

	if( gamenet_man->InNetGame() == false )
	{
		return false;
	}

	if( gamenet_man->OnServer())
	{
		return( gamenet_man->GetHostMode() == vHOST_MODE_SERVE );
	}

	player = gamenet_man->GetLocalPlayer();
	if( player && player->IsServerPlayer())
	{
		return true;
	}
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptIsAutoServing(Script::CStruct *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
		
	if( gamenet_man->InNetGame() == false )
	{
		return false;
	}

	if( gamenet_man->OnServer() == false )
	{
		return false;
		
	}

	return( gamenet_man->GetHostMode() == vHOST_MODE_AUTO_SERVE );
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptChangeLevelPending(Script::CStruct *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	
	return( gamenet_man->m_change_level_task->InList());
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptDownloadMotd(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();

	gamenet_man->SetNextServerListState( vSERVER_LIST_STATE_STARTING_MOTD );
	gamenet_man->SetServerListState( vSERVER_LIST_STATE_INITIALIZE );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptAlreadyGotMotd(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
#ifdef __PLAT_NGPS__
	Manager * gamenet_man = Manager::Instance();

	return gamenet_man->m_got_motd;
#else
	return true;
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptAlreadySignedIn(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
#ifdef __PLAT_XBOX__
	Manager * gamenet_man = Manager::Instance();

	return gamenet_man->mpAuthMan->SignedIn();
#else
	return false;
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptSignOut(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
#ifdef __PLAT_XBOX__
	Manager * gamenet_man = Manager::Instance();

	gamenet_man->mpAuthMan->SignOut();
#endif
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptAllPlayersAreReady(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	Lst::Search< PlayerInfo > sh;
	PlayerInfo* player;

	if( gamenet_man->OnServer())
	{
		for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
		{
			if(( player->m_Conn->GetStatus() & Net::Conn::mSTATUS_READY ) == 0 )
			{
				return false;
			}
		}
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptJoinServerComplete(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();

	return gamenet_man->ReadyToPlay();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptJustStartedNetGame(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	if(( Tmr::GetTime() - s_time_to_start_game ) < Tmr::Seconds( 5 ))
	{
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptEnteredNetworkGame(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	Mlp::Manager * mlp_manager = Mlp::Manager::Instance();
    
	mlp_manager->AddLogicTask( *gamenet_man->m_enter_chat_task );
	mlp_manager->AddLogicTask( *gamenet_man->m_render_scores_task );
	gamenet_man->m_scores_on = true;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptChooseAccount(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
#ifdef __PLAT_XBOX__
	Manager * gamenet_man = Manager::Instance();
	int index;

	index = 0;
	pParams->GetInteger( Script::GenerateCRC("index"), &index );
	gamenet_man->mpAuthMan->SelectAccount( index );
#endif	// __PLAT_XBOX__
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptCreatePlayerOptions(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	Script::CScriptStructure* pStructure;
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;
	int index;
	const char* name;

	index = 0;
	name = NULL;
	pParams->GetInteger( Script::GenerateCRC("index"), &index );
	pParams->GetString( Script::GenerateCRC("name"), &name );
	
	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
	{
		if( index == 0 )
		{
			if( stricmp( player->m_Name, name ) == 0 )
			{
				player->m_flags.SetMask( PlayerInfo::mMARKED_PLAYER );

				pStructure = new Script::CScriptStructure;

#ifdef __PLAT_NGPS__	
				if( gamenet_man->InInternetMode())
				{
					if( ( player->m_Profile ) && 
						( gamenet_man->mpBuddyMan->IsLoggedIn()))
					{         
						pStructure->AddInteger( "profile", player->m_Profile );	
						pStructure->AddString( "nick", player->m_Name );    
				
						if( gamenet_man->mpBuddyMan->IsAlreadyMyBuddy( player->m_Profile ))
						{
							pStructure->AddChecksum( NONAME, Script::GenerateCRC( "allow_remove_homie" ));
						}
						else
						{
							pStructure->AddChecksum( NONAME, Script::GenerateCRC( "allow_add_homie" ));
						}
					}
				}
#endif
				pStructure->AddComponent( Script::GenerateCRC( "name" ), ESYMBOLTYPE_STRING, name );
	
				Script::RunScript( "create_player_options_dialog", pStructure );
	
				delete pStructure;
			}
			break;
		}
		
		index--;
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
#ifdef __PLAT_NGPS__
bool Manager::ScriptFillPlayerListMenu( Script::CStruct* pParams, Script::CScript* pScript )
{
	Manager * gamenet_man = Manager::Instance();
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;
	Script::CStruct* p_item_params, *p_unfocus_params;
	Script::CArray* p_colors;
	int i;
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());

	i = 0;
	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
	{
		if( player->IsLocalPlayer())
		{
			i++;
			continue;
		}
		
		p_colors = new Script::CArray;
		p_colors->SetSizeAndType( 4, ESYMBOLTYPE_INTEGER );

		if( ( player->m_Profile ) && 
			( gamenet_man->mpBuddyMan->IsLoggedIn()))
		{         
			if( gamenet_man->mpBuddyMan->IsAlreadyMyBuddy( player->m_Profile ))
			{
				p_colors->SetInteger( 0, 128 );
				p_colors->SetInteger( 1, 128 );
				p_colors->SetInteger( 2, 0 );
				p_colors->SetInteger( 3, 128 );
			}
			else
			{
				p_colors->SetInteger( 0, 0 );
				p_colors->SetInteger( 1, 128 );
				p_colors->SetInteger( 2, 0 );
				p_colors->SetInteger( 3, 128 );
			}
		}
		else
		{
			p_colors->SetInteger( 0, 128 );
			p_colors->SetInteger( 1, 128 );
			p_colors->SetInteger( 2, 128 );
			p_colors->SetInteger( 3, 128 );
		}
		
		p_item_params = new Script::CStruct;	
		p_item_params->AddString( "text", player->m_Name );
		p_item_params->AddChecksum( "centered", CRCD(0x2a434d05,"centered") );
		p_item_params->AddChecksum( "id", 123456 + i );
		p_item_params->AddChecksum( "pad_choose_script",Script::GenerateCRC("choose_selected_player"));
		
		p_unfocus_params = new Script::CStruct;
		p_unfocus_params->AddArray( "rgba", p_colors );
		p_item_params->AddArray( "rgba", p_colors );
		p_item_params->AddStructure( "unfocus_params", p_unfocus_params );
	
		// create the parameters that are passed to the X script
		Script::CStruct *p_script_params= new Script::CStruct;
		p_script_params->AddInteger( "index", i );	
		p_script_params->AddString( "name", player->m_Name );	
		p_item_params->AddStructure("pad_choose_params",p_script_params);			
	
		Script::RunScript("theme_menu_add_item",p_item_params);
		
		delete p_item_params;
		delete p_script_params;
		delete p_unfocus_params;
		delete p_colors;
		i++;
	}

	Mem::Manager::sHandle().PopContext();
	
	return true;
}
#endif
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptRemovePlayer(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	Script::CScriptStructure* pStructure;
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;
	int index;
	const char* name;

	index = 0;
	name = NULL;
	pParams->GetInteger( Script::GenerateCRC("index"), &index );
	pParams->GetString( Script::GenerateCRC("name"), &name );
	
	for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
	{
		if( index == 0 )
		{
			if( stricmp( player->m_Name, name ) == 0 )
			{
				player->m_flags.SetMask( PlayerInfo::mMARKED_PLAYER );

				pStructure = new Script::CScriptStructure;
				pStructure->AddComponent( Script::GenerateCRC( "name" ), ESYMBOLTYPE_STRING, name );
	
				Script::RunScript( "create_kick_ban_menu", pStructure );
	
				delete pStructure;
			}
			break;
		}
		
		index--;
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptCancelRemovePlayer(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;

	for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
	{
		player->m_flags.ClearMask( PlayerInfo::mMARKED_PLAYER );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptKickPlayer(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;
	int i;

	i = 0;
	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
	{
		if( player->m_flags.TestMask( PlayerInfo::mMARKED_PLAYER ))
		{
			if( gamenet_man->OnServer())
			{
				Obj::CSkater* quitting_skater;
				bool observing;
	
				quitting_skater = player->m_Skater;
				observing = player->IsObserving();
				gamenet_man->DropPlayer( player, vREASON_KICKED );
				if( !observing )
				{
					Mdl::Skate * skate_mod = Mdl::Skate::Instance();
					skate_mod->remove_skater( quitting_skater );
				}
			}
			else
			{
				Net::Client* client;
				MsgRemovePlayerRequest msg;
				Net::MsgDesc msg_desc;

				client = gamenet_man->GetClient( 0 );
				Dbg_Assert( client );

				msg.m_Ban = 0;
				msg.m_Index = i;
				strcpy( msg.m_Name, player->m_Name );

				msg_desc.m_Data = &msg;
				msg_desc.m_Length = sizeof( MsgRemovePlayerRequest );
				msg_desc.m_Id = MSG_ID_FCFS_BAN_PLAYER;
				msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
				msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
				client->EnqueueMessageToServer( &msg_desc );
												
			}
			break;
		}
		i++;
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::ScriptBanPlayer(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;
	int i;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());
	i = 0;
	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
	{
		if( player->m_flags.TestMask( PlayerInfo::mMARKED_PLAYER ))
		{
			if( gamenet_man->OnServer())
			{
				Obj::CSkater* quitting_skater;
				bool observing;
	
				quitting_skater = player->m_Skater;
				observing = player->IsObserving();
				gamenet_man->DropPlayer( player, vREASON_BANNED );
				if( !observing )
				{
					Mdl::Skate * skate_mod = Mdl::Skate::Instance();
					skate_mod->remove_skater( quitting_skater );
				}
			}
			else
			{
				Net::Client* client;
				MsgRemovePlayerRequest msg;
				Net::MsgDesc msg_desc;

				client = gamenet_man->GetClient( 0 );
				Dbg_Assert( client );

				msg.m_Ban = 1;
				msg.m_Index = i;
				strcpy( msg.m_Name, player->m_Name );

				msg_desc.m_Data = &msg;
				msg_desc.m_Length = sizeof( MsgRemovePlayerRequest );
				msg_desc.m_Id = MSG_ID_FCFS_BAN_PLAYER;
				msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
				msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
				client->EnqueueMessageToServer( &msg_desc );
			
			}
			break;
		}
		i++;
	}

	Mem::Manager::sHandle().PopContext();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptFCFSRequestStartGame(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	MsgStartGameRequest msg;
	Net::MsgDesc msg_desc;
	Manager * gamenet_man = Manager::Instance();
	Net::Client* client;
	Prefs::Preferences* pPreferences;
	
	pPreferences = gamenet_man->GetNetworkPreferences();
	msg.m_SkillLevel = pPreferences->GetPreferenceChecksum( Script::GenerateCRC("skill_level"), Script::GenerateCRC("checksum") );
	msg.m_FireballLevel = pPreferences->GetPreferenceChecksum( Script::GenerateCRC("fireball_difficulty"), Script::GenerateCRC("checksum") );
	msg.m_GameMode = gamenet_man->GetGameTypeFromPreferences();
	msg.m_TimeLimit = pPreferences->GetPreferenceChecksum( Script::GenerateCRC("time_limit"), Script::GenerateCRC("checksum") );
	msg.m_TargetScore = 0;
	msg.m_TargetScore = pPreferences->GetPreferenceChecksum( Script::GenerateCRC("target_score"), Script::GenerateCRC("checksum") );
	Dbg_Printf( "************ SENDING TARGET SCORE OF %08x\n", msg.m_TargetScore );
	msg.m_PlayerCollision = pPreferences->GetPreferenceChecksum( Script::GenerateCRC("player_collision"), Script::GenerateCRC("checksum") );
	msg.m_FriendlyFire = pPreferences->GetPreferenceChecksum( Script::GenerateCRC("friendly_fire"), Script::GenerateCRC("checksum") );
	msg.m_StopAtZero = pPreferences->GetPreferenceChecksum( Script::GenerateCRC("stop_at_zero"), Script::GenerateCRC("checksum") );
	msg.m_CTFType = pPreferences->GetPreferenceChecksum( Script::GenerateCRC("ctf_game_type"), Script::GenerateCRC("checksum") );
	
	// FCFS functions should only happen on clients
	Dbg_Assert( !gamenet_man->OnServer());

	client = gamenet_man->GetClient( 0 );
	Dbg_Assert( client );

	msg_desc.m_Data = &msg;
	msg_desc.m_Length = sizeof( MsgStartGameRequest );
	msg_desc.m_Id = MSG_ID_FCFS_START_GAME;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	client->EnqueueMessageToServer( &msg_desc );

	gamenet_man->m_game_pending = true;
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptFCFSRequestChangeLevel(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	uint32 level;
	Manager * gamenet_man = Manager::Instance();
	MsgIntInfo msg;
	Net::MsgDesc msg_desc;
	Net::Client* client;

	// Need to limit this option. Make sure a second request doesn't go out while waiting for the first
	// to complete

	// FCFS functions should only happen on clients
	Dbg_Assert( !gamenet_man->OnServer());

	level = 0;
	pParams->GetChecksum( Script::GenerateCRC( "level" ), &level );
    msg.m_Data = (int) level;

	client = gamenet_man->GetClient( 0 );
	Dbg_Assert( client );

	if( level == CRCD(0xb664035d,"Load_Sk5Ed_gameplay"))
	{
		client->StreamMessageToServer( MSG_ID_LEVEL_DATA, Ed::CParkManager::COMPRESSED_MAP_SIZE, 
						   Ed::CParkManager::sInstance()->GetCompressedMapBuffer(), "level data", vSEQ_GROUP_PLAYER_MSGS );
						   
		client->StreamMessageToServer( MSG_ID_RAIL_DATA, Obj::GetRailEditor()->GetCompressedRailsBufferSize(), 
						   Obj::GetRailEditor()->GetCompressedRailsBuffer(), "rail data", vSEQ_GROUP_PLAYER_MSGS );
    
	}

	msg_desc.m_Data = &msg;
	msg_desc.m_Length = sizeof( MsgIntInfo );
	msg_desc.m_Id = MSG_ID_FCFS_CHANGE_LEVEL;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	client->EnqueueMessageToServer( &msg_desc );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptFoundServers(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	
	return( gamenet_man->m_servers.CountItems() > 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptCancelJoinServer(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();

	gamenet_man->CancelJoinServer();
	gamenet_man->SetJoinState( vJOIN_STATE_FINISHED );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptReattemptJoinServer(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();

	gamenet_man->ReattemptJoin( gamenet_man->GetClient( 0 ));

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptDropPendingPlayers(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();

	gamenet_man->DropPartiallyLoadedPlayers();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptToggleProSet(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	int bit;
	uint32 param_id;
	Manager * gamenet_man = Manager::Instance();

	pParams->GetInteger( "bit", &bit, Script::ASSERT );
	pParams->GetChecksum( "param_id", &param_id, Script::ASSERT );
    
	gamenet_man->ToggleProSet( bit, param_id );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptFCFSRequestToggleProSet(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	Net::Client* client;
	MsgToggleProSet msg;
	Net::MsgDesc msg_desc;
	uint32 param_id;
	int bit;

	pParams->GetInteger( "bit", &bit, Script::ASSERT );
	pParams->GetChecksum( "param_id", &param_id, Script::ASSERT );

	client = gamenet_man->GetClient( 0 );
	Dbg_Assert( client );

	msg.m_Bit = bit;
	msg.m_ParamId = param_id;

	msg_desc.m_Data = &msg;
	msg_desc.m_Length = sizeof( MsgToggleProSet );
	msg_desc.m_Id = MSG_ID_FCFS_TOGGLE_PROSET;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	client->EnqueueMessageToServer( &msg_desc );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptFCFSRequestToggleGoalSelection(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    uint32 goalId;
	Manager * gamenet_man = Manager::Instance();
	Net::Client* client;
	MsgIntInfo msg;
	Net::MsgDesc msg_desc;

    pParams->GetChecksum( "name", &goalId, Script::ASSERT );
	
	client = gamenet_man->GetClient( 0 );
	Dbg_Assert( client );

	msg.m_Data = goalId;

	msg_desc.m_Data = &msg;
	msg_desc.m_Length = sizeof( MsgIntInfo );
	msg_desc.m_Id = MSG_ID_FCFS_TOGGLE_GOAL_SELECTION;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	client->EnqueueMessageToServer( &msg_desc );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptResetProSetFlags(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	int i;

	for( i = 0; i < vMAX_NUM_PROSETS; i++ )
	{
		Script::CStruct* pSetParams;
		pSetParams = new Script::CStruct;

		pSetParams->AddInteger( "flag", Script::GetInt( "FLAG_PROSET1_GEO_ON" ) + i );
		if( gamenet_man->m_proset_flags.Test( i ))
		{
			Script::RunScript( "SetFlag", pSetParams );
		}
		else
		{
			Script::RunScript( "UnSetFlag", pSetParams );
		}

		delete pSetParams;
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptHasSignedDisclaimer(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	Prefs::Preferences*	prefs;
	
	prefs = gamenet_man->GetNetworkPreferences();
	uint32 has_signed = prefs->GetPreferenceChecksum( Script::GenerateCRC( "signed_disclaimer" ), Script::GenerateCRC( "checksum" ));
	
	return ( has_signed == Script::GenerateCRC( "boolean_true" ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptEnterSurveyorMode(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Inp::Manager * inp_manager = Inp::Manager::Instance();
	Mlp::Manager* mlp_man = Mlp::Manager::Instance();
	Manager * gamenet_man = Manager::Instance();
	PlayerInfo* local_player;

	local_player = gamenet_man->GetLocalPlayer();
	if( local_player->IsObserving() || local_player->IsSurveying())
	{
		return false;
	}

	Dbg_Printf( "Entering surveyor mode\n" );
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());

	local_player->m_observer_logic_task = 
					new Tsk::Task< PlayerInfo > ( PlayerInfo::s_observer_logic_code, *local_player );
	mlp_man->AddLogicTask( *local_player->m_observer_logic_task );
	gamenet_man->m_observer_input_handler = new Inp::Handler< Manager > ( 0,  s_observer_input_logic_code, *gamenet_man, 
															 Tsk::BaseTask::Node::vHANDLER_PRIORITY_OBSERVER_INPUT_LOGIC );
	inp_manager->AddHandler( *gamenet_man->m_observer_input_handler );

	local_player->m_flags.SetMask( PlayerInfo::mSURVEYING );

	Mem::Manager::sHandle().PopContext();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ScriptExitSurveyorMode(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager * gamenet_man = Manager::Instance();
	PlayerInfo* local_player;

	local_player = gamenet_man->GetLocalPlayer();
	if( !local_player )
	{
		return false;
	}

	Dbg_Assert( !( local_player->IsObserving()));

	if( local_player->IsSurveying() == false )
	{
		return true;
	}

	Dbg_Printf( "Exiting surveyor mode\n" );
	gamenet_man->ObservePlayer( local_player );
	delete local_player->m_observer_logic_task;
	local_player->m_observer_logic_task = NULL;
	delete gamenet_man->m_observer_input_handler;
	gamenet_man->m_observer_input_handler = NULL;

	local_player->m_flags.ClearMask( PlayerInfo::mSURVEYING );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetReadyToPlay(bool ready_to_play)
{
	if (ready_to_play == m_ready_to_play) return;
	
	m_ready_to_play = ready_to_play;
	
	// Dan: moved from the beginning of CSkater::DoServerLogic().  I'm sort of just moving a hack, instead of fixing it...
	
	// keep the skater from skating around before he it ready
	if (!Mdl::Skate::Instance()->GetGameMode()->IsTrue( "is_frontend" ))
	{
		if (m_ready_to_play)
		{
			Mdl::Skate* skate_mod = Mdl::Skate::Instance();
			uint32 NumSkaters = skate_mod->GetNumSkaters();
			for (uint32 i = 0; i < NumSkaters; ++i)
			{
			   Obj::CSkater *pSkater = skate_mod->GetSkater(i);
			   if (pSkater)
			   {
				   pSkater->UnPause();
			   }	
			}	
		}
		else
		{
			Mdl::Skate* skate_mod = Mdl::Skate::Instance();
			uint32 NumSkaters = skate_mod->GetNumSkaters();
			for (uint32 i = 0; i < NumSkaters; ++i)
			{
			   Obj::CSkater *pSkater = skate_mod->GetSkater(i);
			   if (pSkater)
			   {
				   pSkater->Pause();
			   }	
			}	
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32		Manager::GetGoalsLevel( void )
{
	return m_goals_level;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		Manager::UsingCreatedGoals( void )
{
	/*uint32 goals_pref;

	goals_pref = m_network_preferences.GetPreferenceChecksum( CRCD(0x38dbe1d0,"goals"), CRCD( 0x21902065, "checksum" ));
	if( goals_pref == CRCD(0xe9354db8,"goals_created"))
	{
		return true;
	}*/

	if( Obj::GetGoalEditor()->CountGoalsForLevel( 0 ) > 0 )
	{
		return true;
	}


	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Manager::LoadGoals( uint32 level )
{
	m_goals_level = level;

	// Store the level as the first 4 bytes
	memcpy( m_goals_data, &level, sizeof( uint32 ));
	m_goals_data_size = Obj::GetGoalEditor()->WriteToBuffer( level, (uint8*) ( m_goals_data + sizeof( uint32 )), 
															 vMAX_GOAL_SIZE );
	m_goals_data_size += sizeof( uint32 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*		Manager::GetGoalsData( void )
{
	return m_goals_data;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int			Manager::GetGoalsDataSize( void )
{
	return m_goals_data_size;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Manager::SetGoalsData( char* data, uint32 level, int size )
{
	Dbg_Assert( size < vMAX_GOAL_SIZE );

	memcpy( m_goals_data, data, size );
	m_goals_data_size = size;
	m_goals_level = level;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace GameNet




