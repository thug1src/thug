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
**	File name:		GameHandler.cpp											**
**																			**
**	Created by:		08/09/01	-	spg										**
**																			**
**	Description:	Game-Side Network Handlers								**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <sys/mcman.h>
#include <sys/SIO/keyboard.h>

#include <gel/net/server/netserv.h>
#include <gel/net/client/netclnt.h>
#include <gel/mainloop.h>

#include <gfx/vecfont.h>

#include <sk/gamenet/gamenet.h>
#include <sk/parkeditor2/edmap.h>

#include <sk/modules/skate/skate.h>
#include <sk/modules/skate/gamemode.h>
#include <sk/modules/skate/goalmanager.h>
#include <sk/modules/frontend/frontend.h>

#include <sk/objects/skaterprofile.h>
#include <sk/objects/proxim.h>
#include <sk/objects/crown.h>

#include <sk/components/skaterscorecomponent.h>
#include <sk/components/GoalEditorComponent.h>
#include <sk/components/RailEditorComponent.h>

#include <sk/scripting/cfuncs.h>
#include <gel/scripting/script.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <sk/scripting/nodearray.h>
#include <sk/objects/skater.h>		   // 	 various high level network game things

#ifdef __PLAT_NGPS__
#include <sk/gamenet/ngps/p_buddy.h>
#include <sk/gamenet/lobby.h>
#include <sk/gamenet/ngps/p_stats.h>
#endif

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

#define vTIME_BEFORE_CHANGING_LEVEL		Tmr::Seconds( 3 )

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

static Tmr::Time	s_time_change_level;

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

/*****************************************************************************
**							  Handler Functions								**
*****************************************************************************/

/******************************************************************/
/* The server has told us to create a new skater and associate it */
/* with a player                                                  */
/******************************************************************/

int Manager::s_handle_new_player( Net::MsgHandlerContext* context )
{
	MsgNewPlayer* msg;
	Manager* gamenet_man;
	
	msg = (MsgNewPlayer *) context->m_Msg;
	gamenet_man = (Manager *) context->m_Data;

	NewPlayerInfo new_player;

	strcpy( new_player.Name, msg->m_Name );
	new_player.ObjID = msg->m_ObjId;
	new_player.Conn = NULL;
	new_player.Flags = msg->m_Flags;
	new_player.Team = msg->m_Team;
	new_player.Profile = msg->m_Profile;
	new_player.Rating = msg->m_Rating;
	new_player.Score = msg->m_Score;
	new_player.VehicleControlType = msg->m_VehicleControlType;

	if( new_player.Flags & PlayerInfo::mLOCAL_PLAYER )
	{
		Dbg_Message( "(%d) Got local new player with ID %d!\n", context->m_App->m_FrameCounter,
															msg->m_ObjId );
	}
	else
	{
		Dbg_Message( "(%d) Got new player with ID %d!\n", context->m_App->m_FrameCounter,
															msg->m_ObjId );
	}

	// Observers and local players do not receive their own appearance data
	if( !( new_player.Flags & ( PlayerInfo::mOBSERVER | PlayerInfo::mLOCAL_PLAYER )))
	{
		// GJ:  the appearance is transmitted inside the message,
		// so copy it to the "new_player" structure
		new_player.mpSkaterProfile->ReadFromBuffer( msg->m_AppearanceData );
	}
	
	// If this is all part of our join packet, just load players immediately instead of deferring it
	if( gamenet_man->ReadyToPlay() == false )
	{
		Dbg_Printf( "Calling DeferredNewPlayer with no wait\n" );
		gamenet_man->DeferredNewPlayer( &new_player, 0 );
	}
	else
	{
		gamenet_man->CreateNetPanelMessage( false, Script::GenerateCRC("net_message_new_player"), 
											new_player.Name );

		gamenet_man->DeferredNewPlayer( &new_player );
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Manager::s_handle_wait_n_seconds( Net::MsgHandlerContext* context )
{
	char *num_seconds = (char*) context->m_Msg;

	context->m_Conn->UpdateCommTime( Tmr::Seconds( *num_seconds ));
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Manager::s_handle_player_info_ack_req( Net::MsgHandlerContext* context )
{
	MsgIntInfo* msg;
	Manager* gamenet_man;
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	Net::MsgDesc msg_desc;
	int i;

	Dbg_Printf( "Handling PlayerInfo ack request\n" );
	// At this point, we've received the batch of player info's representing the players
	// that were in the game at the time of our join. Advance our state machine
	gamenet_man = (Manager *) context->m_Data;

	gamenet_man->SetJoinState( GameNet::vJOIN_STATE_GOT_PLAYERS );
    
	msg = (MsgIntInfo*) context->m_Msg;

	msg_desc.m_Id = MSG_ID_PLAYER_INFO_ACK;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;

	context->m_App->EnqueueMessageToServer( &msg_desc );

	if( !gamenet_man->OnServer())
	{
		for( i = 0; i < 2; i++ )
		{
			Net::Client* client;
	
			if(( client = gamenet_man->GetClient( i )))
			{
				skate_mod->AddNetworkMsgHandlers( client, i );
			}
		}
	}
                                                            
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Manager::s_handle_player_info_ack( Net::MsgHandlerContext* context )
{   
	Net::Manager * net_man = Net::Manager::Instance();
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	PlayerInfo* player;
	Manager* gamenet_man;

	gamenet_man = (Manager*) context->m_Data;
	player = gamenet_man->GetPlayerByConnection( context->m_Conn );
	if( player )
	{
		// tell them what level to load
		if( !( player->m_flags.TestMask( PlayerInfo::mHAS_PLAYER_INFO )))
		{
			Lst::Search< TriggerEvent > trigger_sh;
			TriggerEvent* trigger;
			MsgStartInfo start_info_msg;
			Net::MsgDesc start_desc;
			PlayerInfo* king;
			MsgReady ready_msg;
		
			ready_msg.m_Time = Tmr::GetTime();
	
			player->m_flags.SetMask( PlayerInfo::mHAS_PLAYER_INFO );
			
			start_info_msg.m_MaxPlayers = gamenet_man->GetMaxPlayers();
			if( player->IsLocalPlayer())
			{
				gamenet_man->SetReadyToPlay(true);
			}
			else
			{
				// If the game is over, just tell the client to join in net_lobby mode
				if( gamenet_man->GameIsOver())
				{
					start_info_msg.m_GameMode = Script::GenerateCRC( "netlobby" );
					start_info_msg.m_TimeLeft = 0;
					start_info_msg.m_TimeLimit = 0;
					start_info_msg.m_TargetScore = 0;
				}
				else
				{
					start_info_msg.m_GameMode = skate_mod->GetGameMode()->GetNameChecksum();
					start_info_msg.m_TimeLeft = Tmr::InSeconds( skate_mod->GetGameMode()->GetTimeLeft());
					//start_info_msg.m_TimeLimit = gamenet_man->m_network_preferences.GetPreferenceValue( Script::GenerateCRC("time_limit"), Script::GenerateCRC("time") );
					start_info_msg.m_TimeLimit = skate_mod->GetGameMode()->GetTimeLimit();
					start_info_msg.m_TargetScore = gamenet_man->m_network_preferences.GetPreferenceValue( Script::GenerateCRC("target_score"), Script::GenerateCRC("score") );
				}
				start_info_msg.m_GameId = gamenet_man->GetNetworkGameId();
				start_info_msg.m_LevelId = gamenet_man->GetNetworkLevelId();
				start_info_msg.m_TeamMode = skate_mod->GetGameMode()->NumTeams();
				start_info_msg.m_CrownSpawnPoint = gamenet_man->m_crown_spawn_point;
				start_info_msg.m_ProSetFlags = gamenet_man->m_proset_flags;
				memcpy( start_info_msg.m_StartPoints, gamenet_man->m_skater_starting_points, 
						Mdl::Skate::vMAX_SKATERS * sizeof( int ));
				if( net_man->GetConnectionType() == Net::vCONN_TYPE_MODEM )
				{
					start_info_msg.m_Broadband = 0;
				}
				else
				{
					start_info_msg.m_Broadband = 1;
				}

				start_desc.m_Data = &start_info_msg;
				start_desc.m_Length = sizeof( MsgStartInfo );
				start_desc.m_Id = MSG_ID_START_INFO;
				start_desc.m_Queue = Net::QUEUE_SEQUENCED;
				start_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
				gamenet_man->m_server->EnqueueMessage( 	player->GetConnHandle(), &start_desc );

				if( gamenet_man->UsingCreatedGoals())
				{
					gamenet_man->LoadGoals( gamenet_man->GetNetworkLevelId());
					gamenet_man->m_server->StreamMessage( player->GetConnHandle(), MSG_ID_GOALS_DATA, gamenet_man->GetGoalsDataSize(), 
												 gamenet_man->GetGoalsData(), "goals data", vSEQ_GROUP_PLAYER_MSGS,
												 false, true );
				}

				// tell them to execute the queue of scripts
				for( trigger = trigger_sh.FirstItem( gamenet_man->m_trigger_events ); trigger; 
						trigger = trigger_sh.NextItem())
				{
					MsgSpawnAndRunScript script_msg;
					Net::MsgDesc script_desc;
	
					script_msg.m_Node = trigger->m_Node;
					script_msg.m_ObjID = trigger->m_ObjID;
					script_msg.m_ScriptName = trigger->m_Script;
					script_msg.m_Permanent = 0;
	
					script_desc.m_Data = &script_msg;
					script_desc.m_Length = sizeof( MsgSpawnAndRunScript );
					script_desc.m_Id = MSG_ID_SPAWN_AND_RUN_SCRIPT;
					script_desc.m_Queue = Net::QUEUE_SEQUENCED;
					script_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
					gamenet_man->m_server->EnqueueMessage( 	player->GetConnHandle(), &script_desc );
				}
	
				if( player->IsLocalPlayer() == false )
				{
					skate_mod->SendCheatList( player );
				}

				// Update the observers with graffiti status
				if( player->IsObserving())
				{
					if( skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netgraffiti" ))
					{
						MsgInitGraffitiState init_graffiti_state_msg;
						Net::MsgDesc obs_msg_desc;
						uint32 msg_size = skate_mod->GetTrickObjectManager()->SetInitGraffitiStateMessage( &init_graffiti_state_msg );
						Dbg_Printf( "Sending graffiti state message %d\n", msg_size );
		
						obs_msg_desc.m_Data = &init_graffiti_state_msg;
						obs_msg_desc.m_Length = msg_size;
						obs_msg_desc.m_Id = MSG_ID_OBSERVER_INIT_GRAFFITI_STATE;
						obs_msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
						obs_msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;

						gamenet_man->m_server->StreamMessage( player->GetConnHandle(), obs_msg_desc.m_Id, obs_msg_desc.m_Length, 
						   obs_msg_desc.m_Data, "TrickObj Buffer", GameNet::vSEQ_GROUP_PLAYER_MSGS );
						//gamenet_man->m_server->EnqueueMessage( 	player->GetConnHandle(), &obs_msg_desc );
                        
					}
				}
								
				// Compile the list of selected goals to send to the client
				{
					int i, num_goals;
					Game::CGoalManager* pGoalManager;
					Game::CGoal* pGoal;
					Net::MsgMax msg;
					Net::MsgDesc msg_desc;
					uint32 goal_id;
					char* data;
					 
					pGoalManager = Game::GetGoalManager();
			
					data = msg.m_Data;
					*data++ = 0;	// This indicates that they should not bring up the goal summary dialog
					num_goals = pGoalManager->GetNumGoals();
					for( i = 0; i < num_goals; i++ )
					{
						pGoal = pGoalManager->GetGoalByIndex( i );
						Dbg_Assert( pGoal );
						if( pGoal->IsSelected())
						{
							goal_id = pGoal->GetGoalId();
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
					gamenet_man->m_server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
				}
				
				// Update the player with the king of the hill status
				if(( king = gamenet_man->GetKingOfTheHill()))
				{
					GameNet::MsgByteInfo msg;
					Net::MsgDesc msg_desc;
	
					Dbg_MsgAssert( skate_mod->GetGameMode()->GetNameChecksum() == CRCD(0x6ef8fda0,"netking")  ||
								   skate_mod->GetGameMode()->GetNameChecksum() == CRCD(0x5d32129c,"king") ,
									( "King exists in non-king of the hill game" ));
	
					msg.m_Data = king->m_Skater->GetID();

					msg_desc.m_Data = &msg;
					msg_desc.m_Length = sizeof( GameNet::MsgByteInfo );
					msg_desc.m_Id = MSG_ID_NEW_KING;
					msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
					msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
					gamenet_man->m_server->EnqueueMessage(	player->GetConnHandle(), &msg_desc );
				}
			}
			
			Net::MsgDesc proceed_desc;

			proceed_desc.m_Id = MSG_ID_PROCEED_TO_PLAY;
			proceed_desc.m_Queue = Net::QUEUE_SEQUENCED;
			proceed_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
			// Send this to let the client know when he has received the full set of initial data
			gamenet_man->m_server->EnqueueMessage( 	player->GetConnHandle(), &proceed_desc );

			if( gamenet_man->InNetGame())
			{
				gamenet_man->SendFaceDataToPlayer( player );
			}
		}
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	Manager::s_handle_ready_query( Net::MsgHandlerContext* context )
{
	MsgReady* msg;
	Manager* gamenet_man;

	gamenet_man = (Manager*) context->m_Data;
	msg = (MsgReady*) context->m_Msg;
	
	gamenet_man->m_latest_ready_query = msg->m_Time;

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	Manager::s_handle_ready_response( Net::MsgHandlerContext* context )
{
	PlayerInfo* player;
	Manager* gamenet_man;
	MsgReady* ready_msg;

	gamenet_man = (Manager*) context->m_Data;
	ready_msg = (MsgReady* ) context->m_Msg;
	player = gamenet_man->GetPlayerByConnection( context->m_Conn );
	if( player )
	{   
		player->MarkAsReady( ready_msg->m_Time );
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Respond to a client who is looking for games to join on the LAN*/
/* Tell them info about the game                                  */
/******************************************************************/

int Manager::s_handle_find_server( Net::MsgHandlerContext* context )
{
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;
	Lst::Search< NewPlayerInfo > new_sh;
	NewPlayerInfo* np;
    MsgServerDescription msg;
	MsgFindServer* find_msg;
	Manager* gamenet_man;
	Net::Server* server;
	int i;
	char name[vMAX_SERVER_NAME_LEN + 1];
	const char *server_name;
	Script::CScriptStructure* pStructure;
	Prefs::Preferences* pPreferences;
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();   

	//Dbg_Printf( "Got find server broadcast\n" );
	if( context->m_MsgLength != sizeof( MsgFindServer ))
	{
		return Net::HANDLER_MSG_DONE;
	}

	gamenet_man = (Manager*) context->m_Data;
	if( gamenet_man->InInternetMode())
	{
		return Net::HANDLER_MSG_DONE;
	}

	pPreferences = gamenet_man->GetNetworkPreferences();
	pStructure = pPreferences->GetPreference( Script::GenerateCRC("server_name") );
	pStructure->GetText( "ui_string", &server_name, true );
	
	find_msg = (MsgFindServer *) context->m_Msg;
    
	strncpy( name, server_name, vMAX_SERVER_NAME_LEN );
	name[ vMAX_SERVER_NAME_LEN ] = '\0';
	
	server = gamenet_man->GetServer();
	Dbg_Assert( server != NULL );

	msg.m_SkillLevel = gamenet_man->GetSkillLevel();
	msg.m_NumPlayers = gamenet_man->GetNumPlayers();
	msg.m_MaxPlayers = gamenet_man->GetMaxPlayers();
	msg.m_NumObservers = gamenet_man->GetNumObservers();
	msg.m_MaxObservers = gamenet_man->GetMaxObservers();
    
	// Handle the cases where you've just changed the limits and your current number of players/observers
	// exceeds the limits
	if( msg.m_MaxPlayers < msg.m_NumPlayers )
	{
		msg.m_MaxPlayers = msg.m_NumPlayers;
	}
	if( msg.m_MaxObservers < msg.m_NumObservers )
	{
		msg.m_MaxObservers = msg.m_NumObservers;
	}

	if( strcmp( gamenet_man->GetPassword(), "" ) == 0 )
	{
		msg.m_Password = 0;
	}
	else
	{
		msg.m_Password = 1;
	}

	if( skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netlobby" ))
	{
		msg.m_GameStarted = false;
	}
	else
	{
		msg.m_GameStarted = true;
	}

	msg.m_HostMode = gamenet_man->GetHostMode();
#ifdef __PLAT_NGPS__	
	msg.m_Ranked = false;
	if( gamenet_man->InInternetMode())
	{
		if( gamenet_man->mpBuddyMan->IsLoggedIn())
		{
			msg.m_Ranked = true;
		}
	}
#endif
	
	strcpy( msg.m_Name, name );
	sprintf( msg.m_Level, "%s", gamenet_man->GetLevelName());
	sprintf( msg.m_Mode, "%s", gamenet_man->GetGameModeName());
	msg.m_Timestamp = find_msg->m_Timestamp;
#ifdef __PLAT_XBOX__
	memcpy( msg.m_Nonce, find_msg->m_Nonce, 8 );
	memcpy( &msg.m_XboxKeyId, &server->m_XboxKeyId, sizeof( XNKID ));
	memcpy( &msg.m_XboxKey, &server->m_XboxKey, sizeof( XNKEY ));
	memcpy( &msg.m_XboxAddr, &server->m_XboxAddr, sizeof( XNADDR ));
#endif // __PLAT_XBOX__

	i = 0;

	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; 
			player = gamenet_man->NextPlayerInfo( sh, true ))
	{
		if( !player->IsObserving() || player->IsPendingPlayer())
		{
			sprintf( msg.m_PlayerNames[i], player->m_Name );
			i++;
		}
	}
    
	for( np = gamenet_man->FirstNewPlayerInfo( new_sh ); np; np = gamenet_man->NextNewPlayerInfo( new_sh ))
	{
		// Pending players count, observers don't
		if(	( np->Flags & PlayerInfo::mPENDING_PLAYER ) ||
			( np->Flags & PlayerInfo::mJUMPING_IN ) ||
			!( np->Flags & PlayerInfo::mOBSERVER ))
		{
			sprintf( msg.m_PlayerNames[i], np->Name );
			i++;
		}                 
	}
	
#ifdef __PLAT_XBOX__	
	server->SendMessageTo( Net::MSG_ID_SERVER_RESPONSE, sizeof( MsgServerDescription ), &msg,
							INADDR_BROADCAST, context->m_Conn->GetPort(), 0 );
#else
	server->SendMessageTo( Net::MSG_ID_SERVER_RESPONSE, sizeof( MsgServerDescription ), &msg,
							context->m_Conn->GetIP(), context->m_Conn->GetPort(), 0 );	
#endif		

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Got a connection request: For now, just accept it, create a    */
/* skater for the requester, and tell them to create one too      */
/******************************************************************/

int	Manager::s_handle_connection( Net::MsgHandlerContext* context )
{   
	Manager* gamenet_man;
	MsgConnectInfo* connect_msg;
	int reason;
	
	Dbg_Assert( context );

	gamenet_man = (Manager*) context->m_Data;
	connect_msg = (MsgConnectInfo*) context->m_Msg;
    
	if( context->m_Conn->IsRemote())
	{
		MsgProceed msg;
		const char *server_name;
		Script::CScriptStructure* pStructure;
		Prefs::Preferences* pPreferences;
		Net::Manager* net_man = Net::Manager::Instance();
		
		if( !gamenet_man->ok_to_join( reason, connect_msg, context->m_Conn ))
		{
			MsgEmbedded msg;
	
			msg.m_SubMsgId = reason;

			context->m_App->SendMessageTo( MSG_ID_JOIN_REFUSED, sizeof( MsgEmbedded ), &msg, 
										   context->m_Conn->GetIP(), context->m_Conn->GetPort(), 0 );
			return Net::HANDLER_HALT;
		}

		Dbg_Printf( "Sending Join Proceed Message to IP: %x Port: %d MaxPlayers: %d\n", context->m_Conn->GetIP(), context->m_Conn->GetPort(), gamenet_man->GetNumPlayers());
		msg.m_MaxPlayers = gamenet_man->GetMaxPlayers();
		if(	net_man->GetConnectionType() == Net::vCONN_TYPE_MODEM )
		{
			msg.m_Broadband = 0;
		}
		else
		{
			msg.m_Broadband = 1;
		}
		
		if( gamenet_man->InInternetMode())
		{
			msg.m_Port = gamenet_man->GetJoinPort();
			msg.m_PrivateIP = gamenet_man->GetJoinPrivateIP();
			msg.m_PublicIP = gamenet_man->GetJoinIP();
		}
		
		pPreferences = gamenet_man->GetNetworkPreferences();
		pStructure = pPreferences->GetPreference( Script::GenerateCRC("server_name") );
		pStructure->GetText( "ui_string", &server_name, true );
	
		strncpy( msg.m_ServerName, server_name, vMAX_SERVER_NAME_LEN );
		msg.m_ServerName[ vMAX_SERVER_NAME_LEN ] = '\0';

		context->m_App->SendMessageTo( 	MSG_ID_JOIN_PROCEED, sizeof( MsgProceed), &msg,
										context->m_Conn->GetIP(), context->m_Conn->GetPort(), 0 );
	}
	else
	{
		MsgProceed msg;
		Net::MsgDesc msg_desc;

		msg.m_MaxPlayers = gamenet_man->GetMaxPlayers();
		msg_desc.m_Id = MSG_ID_JOIN_PROCEED;
		msg_desc.m_Data = &msg;
		msg_desc.m_Length = sizeof( MsgProceed );
		context->m_App->EnqueueMessage( context->m_Conn->GetHandle(), &msg_desc );
		//context->m_App->SendData();
	}
	
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Client wants to join a connected server						  */
/*																  */
/******************************************************************/

int	Manager::s_handle_join_request( Net::MsgHandlerContext* context )
{   
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	NewPlayerInfo new_player;
	Manager* gamenet_man;
	MsgJoinInfo* join_msg;
	int reason;
	
	Dbg_Assert( context );

	Dbg_Printf( "Got Join Request from ip %x port %d\n", context->m_Conn->GetIP(), context->m_Conn->GetPort());

	gamenet_man = (Manager*) context->m_Data;
	join_msg = (MsgJoinInfo*) context->m_Msg;

	// We'll just accept local clients
	if( context->m_Conn->IsRemote())
	{
		// Rule out redundancy
		if( context->m_PacketFlags & Net::mHANDLE_FOREIGN )
		{
			Net::Conn* conn;

			// Are we currently accepting players?
			if( context->m_App->AcceptsForeignConnections() == false )
			{
				return Net::HANDLER_MSG_DONE;
			}
			
			// Create a more permanent connection
			conn = context->m_App->NewConnection( context->m_Conn->GetIP(), context->m_Conn->GetPort());
			if( join_msg->m_Broadband )
			{
				conn->SetBandwidthType( Net::Conn::vBROADBAND );
				conn->SetSendInterval( Tmr::VBlanks( 2 ));
			}
			else
			{
				conn->SetBandwidthType( Net::Conn::vNARROWBAND );
				conn->SetSendInterval( Tmr::VBlanks( 2 ));
			}
			
			
			context->m_Conn = conn;	// the rest of the chain will use this new, valid connection
			
			if( !gamenet_man->ok_to_join( reason, join_msg, conn ))
			{
				MsgEmbedded msg;
				Net::MsgDesc msg_desc;
		
				msg.m_SubMsgId = reason;
		
				msg_desc.m_Data = &msg;
				msg_desc.m_Length = sizeof( MsgEmbedded );
				msg_desc.m_Id = MSG_ID_JOIN_REFUSED;
				context->m_App->EnqueueMessage( context->m_Conn->GetHandle(), &msg_desc );
				context->m_Conn->Invalidate();
		
				return Net::HANDLER_HALT;
			}
		}
		else
		{
			return Net::HANDLER_MSG_DONE;
		}
	}
	
	strcpy( new_player.Name, join_msg->m_Name );
	new_player.ObjID = gamenet_man->GetNextPlayerObjectId();
	new_player.Conn = context->m_Conn;
	new_player.Flags = 0;
	new_player.Profile = join_msg->m_Profile;
	new_player.Rating = join_msg->m_Rating;
	new_player.VehicleControlType = join_msg->m_VehicleControlType;
	if( join_msg->m_Observer != 0 )
	{
		new_player.Flags |= PlayerInfo::mOBSERVER;
	}
	else if(( skate_mod->GetGameMode()->GetNameChecksum() != CRCD(0x1c471c60, "netlobby") ) &&
			( gamenet_man->GameIsOver() == false ))
	{
		new_player.Flags |= ( PlayerInfo::mPENDING_PLAYER | PlayerInfo::mOBSERVER );
	}
    
	Dbg_Printf( "====================== GOT JOIN REQUEST FROM PLAYER %d\n", new_player.ObjID );
	if( context->m_Conn->IsLocal())
	{
		// GJ:  copy the contents of the skater profile from one to the other
		// it'd be nice to just use a copy constructor, but i couldn't
		// get it to work for some reason
		uint8* pTempBuffer = new uint8[vMAX_APPEARANCE_DATA_SIZE];
		skate_mod->GetProfile( new_player.ObjID )->WriteToBuffer(pTempBuffer, vMAX_APPEARANCE_DATA_SIZE);
		new_player.mpSkaterProfile->ReadFromBuffer(pTempBuffer);
		delete pTempBuffer;

		new_player.Flags = PlayerInfo::mLOCAL_PLAYER;
		gamenet_man->DeferredNewPlayer( &new_player, 0 );
	}
	else
	{
		if( !join_msg->m_Observer )
		{
			// GJ:  the appearance is transmitted inside the message,
			// so copy it to the "new_player" structure
			new_player.mpSkaterProfile->ReadFromBuffer(join_msg->m_AppearanceData);
		}
		gamenet_man->DeferredNewPlayer( &new_player );

		// Flag this new player's connection as busy so he doesn't get game-related messages
		// in the ineterum
		context->m_Conn->ClearStatus( Net::Conn::mSTATUS_READY );
		context->m_Conn->SetStatus( Net::Conn::mSTATUS_BUSY );
	}
	
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Client wants to enter observer mode	  						  */
/*																  */
/******************************************************************/

int	Manager::s_handle_observe( Net::MsgHandlerContext* context )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Manager* gamenet_man;
	PlayerInfo* player;
	Obj::CSkater* skater;
		
	gamenet_man = (Manager *) context->m_Data;

	Dbg_Printf( "Got observe request\n" );

	player = gamenet_man->GetPlayerByConnection( context->m_Conn );
	if( !player || player->IsObserving())
	{
		return Net::HANDLER_MSG_DONE;
	}

	// Allow the server to go into observer mode without restriction
	if( ( player->IsLocalPlayer() == false ) &&
		( gamenet_man->GetNumObservers() >= gamenet_man->GetMaxObservers()))
	{
		Net::MsgDesc msg_desc;

		Dbg_Printf( "Not enough observer spots\n" );
		msg_desc.m_Id = MSG_ID_OBSERVE_REFUSED;
		gamenet_man->m_server->EnqueueMessage( context->m_Conn->GetHandle(), &msg_desc );
	}
	else
	{
		Net::MsgDesc msg_desc;

        // Only do this player->observer conversion here for remote players.
		// For our local player, let the observe_proceed message run its course
		if( player->IsLocalPlayer() == false )
		{
			skater = player->m_Skater;
			gamenet_man->DropPlayer( player, vREASON_OBSERVING );
			skate_mod->remove_skater( skater );
			player->m_flags.SetMask( PlayerInfo::mOBSERVER );
			player->m_Skater = NULL;
			if( player->IsServerPlayer())
			{
				player->MarkAsNotServerPlayer();
				gamenet_man->ChooseNewServerPlayer();
			}
			// If we're destroying the player we're looking at, change to the next player
			if( player == gamenet_man->m_cam_player )
			{
				gamenet_man->m_cam_player = gamenet_man->GetNextPlayerToObserve();
				if( player == gamenet_man->m_cam_player )
				{
					gamenet_man->m_cam_player = NULL;
				}
				
				gamenet_man->ObservePlayer( gamenet_man->m_cam_player );
			}
			
		}

		Dbg_Printf( "Sent Proceed to Observe\n" );
		msg_desc.m_Id = MSG_ID_OBSERVE_PROCEED;
		gamenet_man->m_server->EnqueueMessage( context->m_Conn->GetHandle(), &msg_desc );
	}
    
	return Net::HANDLER_CONTINUE;
}   

/******************************************************************/
/* Got a disconnection request. Accept it at face value for now	  */
/*																  */
/******************************************************************/

int	Manager::s_handle_disconn_request( Net::MsgHandlerContext* context )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Manager* gamenet_man;
	PlayerInfo* quitter;
		
	

    gamenet_man = (Manager *) context->m_Data;
	quitter = gamenet_man->GetPlayerByConnection( context->m_Conn );
	if( quitter )
	{
		if (!quitter->IsLocalPlayer())
		{
			Obj::CSkater* quitting_skater;
			bool observing;

			quitting_skater = quitter->m_Skater;
			observing = quitter->IsObserving();

			gamenet_man->DropPlayer( quitter, vREASON_QUIT );
			if( !observing )
			{
				skate_mod->remove_skater( quitting_skater );
			}
		}
	}
	else
	{
		Lst::Node< NewPlayerInfo > *node, *next;
		NewPlayerInfo* new_player;
			
		
	
		for( node = gamenet_man->m_new_players.GetNext(); node; node = next )
		{
			next = node->GetNext();
	
			new_player = node->GetData();
			
			// 
			if( new_player->Conn == context->m_Conn )
			{
				Dbg_Printf( "Removing %s\n", new_player->Name );
				context->m_App->TerminateConnection( new_player->Conn );
				
				delete node;
				delete new_player;
				
				// Return this value to signify that we've destroyed the message (done by
				// TerminateConnection())
				return Net::HANDLER_MSG_DESTROYED;
			}
		}
		// Pay no attention to any other pending messages from ths client
		return Net::HANDLER_HALT;
	}
    
	// Return this value to signify that we've destroyed the message (done by
	// TerminateConnection())
	return Net::HANDLER_MSG_DESTROYED;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	Manager::s_handle_client_proceed( Net::MsgHandlerContext* context )
{
	Mlp::Manager * mlp_manager = Mlp::Manager::Instance();
	Manager* gamenet_man;
	
	
	Dbg_Printf( "(%d) Got proceed instruction from server\n", context->m_App->m_FrameCounter );

	gamenet_man = (Manager *) context->m_Data;
	gamenet_man->SetReadyToPlay(true);
	
	if( !context->m_App->IsLocal())
	{
		mlp_manager->AddLogicTask( *gamenet_man->m_client_add_new_players_task );
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Manager::s_handle_observe_proceed( Net::MsgHandlerContext* context )
{
	Manager* gamenet_man;
		
	Dbg_Printf( "Entering Observer Mode\n" );	

	gamenet_man = (Manager *) context->m_Data;
	ScriptExitSurveyorMode( NULL, NULL );
	gamenet_man->EnterObserverMode(); 
	
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Manager::s_handle_observe_refused( Net::MsgHandlerContext* context )
{
	Manager* gamenet_man;
	
    gamenet_man = (Manager *) context->m_Data;
	Script::CStruct* p_structure = new Script::CStruct;

	p_structure->AddChecksum( "reason", Script::GenerateCRC( "net_reason_full_observers" ));
	p_structure->AddChecksum( "just_dialog", 0 );
	Script::RunScript( "CreateJoinRefusedDialog", p_structure );
	delete p_structure;

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Manager::s_handle_join_refused( Net::MsgHandlerContext* context )
{
	Manager* gamenet_man;
	MsgEmbedded* msg;
    
	

	gamenet_man = (Manager *) context->m_Data;
	msg = (MsgEmbedded *) context->m_Msg;


	switch( msg->m_SubMsgId )
	{
		case JOIN_REFUSED_ID_BANNED:
			gamenet_man->m_conn_refused_reason = Script::GenerateCRC("net_reason_banned");
			break;
		case JOIN_REFUSED_ID_FULL:
			gamenet_man->m_conn_refused_reason = Script::GenerateCRC("net_reason_full");
			break;
		case JOIN_REFUSED_ID_FULL_OBSERVERS:
			gamenet_man->m_conn_refused_reason = Script::GenerateCRC("net_reason_full_observers");
			break;
		case JOIN_REFUSED_ID_PW:
		{   
			if( gamenet_man->GetJoinState() == vJOIN_STATE_TRYING_PASSWORD )
			{
				gamenet_man->m_conn_refused_reason = Script::GenerateCRC("net_reason_wrong_password");
			}
			else
			{
				Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());

				Script::RunScript( "CreateEnterPasswordControl" );
				/*
				dlg->GoBack();
							
				Front::MenuFactory* menu_factory = Front::MenuFactory::Instance();
				Front::MenuElement *net_menu = menu_factory->GetMenuElement(Script::GenerateCRC("net_menu"));
				Front::MenuEvent event;
				
				event.SetTypeAndTarget( Front::MenuEvent::vLINK, Script::GenerateCRC("enter_password_control") );
				net_menu->LaunchEvent( &event );
				*/

				// Temporarily disable timeout
				gamenet_man->m_join_timeout_task->Remove();

				Mem::Manager::sHandle().PopContext();
				return Net::HANDLER_CONTINUE;
			}
			
			break;
		}
		case JOIN_REFUSED_ID_IN_PROGRESS:
		{
			Dbg_Printf( "Refused: Joining a game in progress\n" );
			
			Script::RunScript( "CreateGameInProgressDialog" );
            
			gamenet_man->m_join_timeout_task->Remove();
			return Net::HANDLER_CONTINUE;
		}
		case JOIN_REFUSED_ID_VERSION:
			gamenet_man->m_conn_refused_reason = Script::GenerateCRC("net_reason_version" );
			break;
		default:
			gamenet_man->m_conn_refused_reason = Script::GenerateCRC("net_reason_default" );
			break;
	}
	
	Dbg_Printf( "(%d) Host refused join request: Reason %d\n", 	context->m_App->m_FrameCounter,
																msg->m_SubMsgId );
	
	gamenet_man->CancelJoinServer();
	gamenet_man->SetJoinState( vJOIN_STATE_REFUSED );
	
	return Net::HANDLER_HALT;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Manager::s_handle_join_proceed( Net::MsgHandlerContext* context )
{
	Manager* gamenet_man;
	Net::Manager * net_man = Net::Manager::Instance();
	MsgJoinInfo msg;
    Net::MsgDesc msg_desc;
	int size;
	Script::CScriptStructure* pStructure;
	Prefs::Preferences* pPreferences;
	const char* network_id;
	MsgProceed* proceed_msg;
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	bool ignore_face_data;

	gamenet_man = (Manager *) context->m_Data;
		
	proceed_msg = (MsgProceed*) context->m_Msg;
	if( context->m_Conn->IsRemote())
	{
		// Note whether or not the server is narrowband/broadband
		if( proceed_msg->m_Broadband )
		{
			context->m_Conn->SetBandwidthType( Net::Conn::vBROADBAND );
		}
		else
		{
			context->m_Conn->SetBandwidthType( Net::Conn::vNARROWBAND );
		}

		gamenet_man->m_join_ip = proceed_msg->m_PublicIP;
		gamenet_man->m_join_private_ip = proceed_msg->m_PrivateIP;
		gamenet_man->SetJoinPort( proceed_msg->m_Port );

		Dbg_Printf( "**** Setting public IP to %s\n", inet_ntoa(*(struct in_addr *) &gamenet_man->m_join_ip ));
#ifdef __PLAT_NGPS__
		if( gamenet_man->mpBuddyMan->IsLoggedIn())
		{
			char location[1024];

			gamenet_man->mpLobbyMan->SetServerName( proceed_msg->m_ServerName );
			sprintf( location, "%d:%d:%d:%s (%s)", gamenet_man->m_join_ip, gamenet_man->m_join_private_ip, gamenet_man->GetJoinPort(), 
					 gamenet_man->mpLobbyMan->GetServerName(), gamenet_man->mpLobbyMan->GetLobbyName());
			if( gamenet_man->GetJoinMode() == vJOIN_MODE_PLAY )
			{
				gamenet_man->mpBuddyMan->SetStatusAndLocation( GP_PLAYING, (char*) Script::GetString( "homie_status_playing" ), location );
			}
			else
			{
				gamenet_man->mpBuddyMan->SetStatusAndLocation( GP_PLAYING, (char*) Script::GetString( "homie_status_observing" ), location );
			}
		}
#endif

	}
	skate_mod->GetGameMode()->SetMaximumNumberOfPlayers( proceed_msg->m_MaxPlayers );

	/*Dbg_Printf( "******* Got Join Proceed from: %s, private: %s, port: %d\n", 
				inet_ntoa(*(struct in_addr *) &gamenet_man->m_join_ip ), 
				inet_ntoa(*(struct in_addr *) &gamenet_man->m_join_private_ip ),
				gamenet_man->m_join_port );*/
	
	pPreferences = gamenet_man->GetNetworkPreferences();
	pStructure = pPreferences->GetPreference( Script::GenerateCRC("network_id") );
	pStructure->GetText( "ui_string", &network_id, true );

#ifdef __PLAT_NGPS__
	msg.m_Profile = gamenet_man->mpBuddyMan->GetProfile();
	msg.m_Rating = gamenet_man->mpStatsMan->GetStats()->GetRating();
#endif
	msg.m_Observer = ( gamenet_man->GetJoinMode() == vJOIN_MODE_OBSERVE );
	msg.m_Version = vVERSION_NUMBER;
	msg.m_WillingToWait = 1;
	if(	net_man->GetConnectionType() == Net::vCONN_TYPE_MODEM )
	{
		ignore_face_data = true;
		msg.m_Broadband = 0;
	}
	else
	{
		ignore_face_data = false;
		msg.m_Broadband = 1;
	}
			
	strcpy( msg.m_Name, network_id );
	msg.m_Password[0] = '\0';
	size = 0;
	if( !msg.m_Observer )
	{
		// GJ:  transmit the way you look (slot 0 of the
		// skater profile manager) to the server
		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		Obj::CSkaterProfile* pSkaterProfile = skate_mod->GetProfile(0);
		Net::Conn *server_conn;
		Lst::Search< Net::Conn > sh;
	
		server_conn = context->m_App->FirstConnection( &sh );
		if( server_conn->GetBandwidthType() == Net::Conn::vNARROWBAND )
		{
			ignore_face_data = true;
		}

		size = pSkaterProfile->WriteToBuffer(msg.m_AppearanceData, vMAX_APPEARANCE_DATA_SIZE,
												ignore_face_data );
		Dbg_Assert( size < vMAX_APPEARANCE_DATA_SIZE );
		Dbg_Printf("\n\n******************* MsgJoinInfo (%d) appearance data size = %d %d Broadband %d\n", MSG_ID_JOIN_REQ,
				size, sizeof(MsgJoinInfo) - vMAX_APPEARANCE_DATA_SIZE + size, ignore_face_data );
	}

	msg_desc.m_Data = &msg;
	msg_desc.m_Length = sizeof(MsgJoinInfo) - vMAX_APPEARANCE_DATA_SIZE + size;
	msg_desc.m_Id = MSG_ID_JOIN_REQ;
	if( context->m_Conn->IsRemote())
	{
		msg_desc.m_Queue = Net::QUEUE_IMPORTANT;
	}

	context->m_App->EnqueueMessageToServer( &msg_desc );
	
	/*context->m_App->StreamMessageToServer( GameNet::MSG_ID_JOIN_REQ, msg_desc.m_Length, 
					   msg_desc.m_Data, "appearance data", vSEQ_GROUP_PLAYER_MSGS, false );*/

	gamenet_man->SetJoinState( vJOIN_STATE_JOINING );

	return Net::HANDLER_MSG_DONE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Manager::s_handle_join_accepted( Net::MsgHandlerContext* context )
{
	Dbg_Message( "(%d) Got Join Accepted!\n", context->m_App->m_FrameCounter );

	// Consider all of our messages up to this point to have been received
	//context->m_Conn->AckAllMessages();
	//context->m_Conn->DestroyMessageQueues();
	if( context->m_Conn->IsRemote())
	{
		context->m_Conn->DestroyImportantMessageQueues();
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	Manager::s_handle_new_king( Net::MsgHandlerContext* context )
{
	Manager* gamenet_man;
	MsgByteInfo* msg;
	PlayerInfo* player;

	msg = (MsgByteInfo*) context->m_Msg;

	gamenet_man = (Manager *) context->m_Data;
	
	player = gamenet_man->GetKingOfTheHill();
	if( player )
	{
		player->MarkAsKing( false );
	}
	else
	{
		NewPlayerInfo* new_player;
		Lst::Search< NewPlayerInfo > sh;

		// Maybe we just haven't officially added the previous king. Search our list of
		// queued up new players
		for( new_player = gamenet_man->FirstNewPlayerInfo( sh ); new_player;
				new_player = gamenet_man->NextNewPlayerInfo( sh ))
		{
			if( new_player->Flags & PlayerInfo::mKING_OF_THE_HILL )
			{
				new_player->Flags &= ~PlayerInfo::mKING_OF_THE_HILL;
				break;
			}
		}
	}

	player = gamenet_man->GetPlayerByObjectID( msg->m_Data );
	if( player )
	{
		player->MarkAsKing( true );
	}
	else
	{
		NewPlayerInfo* new_player;
		Lst::Search< NewPlayerInfo > sh;

		// Maybe we just haven't officially added this new king player. Search our list of
		// queued up new players
		for( new_player = gamenet_man->FirstNewPlayerInfo( sh ); new_player;
				new_player = gamenet_man->NextNewPlayerInfo( sh ))
		{
			if( new_player->ObjID == msg->m_Data )
			{
				new_player->Flags |= PlayerInfo::mKING_OF_THE_HILL;
				break;
			}
		}
	}
    
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	Manager::s_handle_stole_flag( Net::MsgHandlerContext* context )
{
	Manager* gamenet_man;
	MsgFlagMsg* msg;
	PlayerInfo* player;

	gamenet_man = (Manager *) context->m_Data;
	msg = (MsgFlagMsg*) context->m_Msg;

	player = gamenet_man->GetPlayerByObjectID( msg->m_ObjId );
	if( player )
	{
		player->StoleFlag( msg->m_Team );
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	Manager::s_handle_took_flag( Net::MsgHandlerContext* context )
{
	Manager* gamenet_man;
	MsgFlagMsg* msg;
	PlayerInfo* player;

	gamenet_man = (Manager *) context->m_Data;
	msg = (MsgFlagMsg*) context->m_Msg;

	player = gamenet_man->GetPlayerByObjectID( msg->m_ObjId );
	if( player )
	{
		player->TookFlag( msg->m_Team );
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	Manager::s_handle_captured_flag( Net::MsgHandlerContext* context )
{
	Manager* gamenet_man;
	MsgFlagMsg* msg;
	PlayerInfo* player;

	gamenet_man = (Manager *) context->m_Data;
	msg = (MsgFlagMsg*) context->m_Msg;

	player = gamenet_man->GetPlayerByObjectID( msg->m_ObjId );
	if( player )
	{
		player->CapturedFlag( msg->m_Team );
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	Manager::s_handle_retrieved_flag( Net::MsgHandlerContext* context )
{
	Manager* gamenet_man;
	MsgFlagMsg* msg;
	PlayerInfo* player;

	gamenet_man = (Manager *) context->m_Data;
	msg = (MsgFlagMsg*) context->m_Msg;

	player = gamenet_man->GetPlayerByObjectID( msg->m_ObjId );
	if( player )
	{
		player->RetrievedFlag();
	}
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* A player has restarted in koth mode. If it is the king, send	  */
/* out a message signifying that the king has lost his crown	  */
/******************************************************************/

int	Manager::s_handle_player_restarted( Net::MsgHandlerContext* context )
{
	Manager* gamenet_man;
	PlayerInfo* player;
	Mdl::Skate * skate_mod;
	 
	skate_mod =  Mdl::Skate::Instance();
	gamenet_man = (Manager *) context->m_Data;
	player = gamenet_man->GetPlayerByConnection( context->m_Conn );
	if( player )
	{
		player->m_flags.ClearMask( PlayerInfo::mRESTARTING );
	}

	if(	skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netctf" ))
	{
		if( player && player->HasCTFFlag())
		{
			Lst::Search< PlayerInfo > sh;
			PlayerInfo* send_player, *local_player;
			char team_str[64];
			int team;
			Script::CStruct* pParams;
			MsgFlagMsg msg;
			Net::MsgDesc msg_desc;
			
			team = player->HasWhichFlag();
			sprintf( team_str, "team_%d_name", team + 1 );
			pParams = new Script::CStruct;
	
			pParams->AddInteger( "team", team );
			pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, Script::GetString( team_str ));
			Script::RunScript( "flag_returned", pParams );
	
			delete pParams;
			msg.m_Team = team;
			msg.m_ObjId = player->m_Skater->GetID();

			// If it's me, remove the message that says "return the flag to your base"
			if( player->IsLocalPlayer())
			{
				Script::RunScript( "destroy_ctf_panel_message" );
			}
			local_player = gamenet_man->GetLocalPlayer();
			if( local_player && !local_player->IsObserving())
			{
				if( local_player->m_Team == team )
				{
					Script::RunScript( "hide_ctf_arrow" );
				}
			}

			player->ClearCTFState();

			msg_desc.m_Data = &msg;
			msg_desc.m_Length = sizeof( MsgFlagMsg );
			msg_desc.m_Id = MSG_ID_PLAYER_DROPPED_FLAG;
			msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
			msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
			for( send_player = gamenet_man->FirstPlayerInfo( sh, true ); send_player; 
					send_player = gamenet_man->NextPlayerInfo( sh, true ))
			{
				if( send_player->IsLocalPlayer())
				{
					continue;
				}
				context->m_App->EnqueueMessage( send_player->GetConnHandle(), &msg_desc );
			}
		}
	}
	else if(( skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netking" )) ||
			( skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "king" )))
	{
		if( player && player->IsKing())
		{
			Lst::Search< PlayerInfo > sh;
			Net::MsgDesc msg_desc;
			char point;
	
			player->MarkAsKing( false );
			gamenet_man->m_crown_spawn_point = Mth::Rnd( vNUM_CROWN_SPAWN_POINTS );
            point = (char) gamenet_man->m_crown_spawn_point;
			
			msg_desc.m_Data = &point;
			msg_desc.m_Length = sizeof( char );
			msg_desc.m_Id = MSG_ID_KING_DROPPED_CROWN;
			msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
			msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
			for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; 
					player = gamenet_man->NextPlayerInfo( sh, true ))
			{
				context->m_App->EnqueueMessage( player->GetConnHandle(), &msg_desc );
			}
		}
	}
	
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* The server is notifying the client that he's in an auto-serving*/
/* server									  					  */
/******************************************************************/

int	Manager::s_handle_auto_server_notification( Net::MsgHandlerContext* context )
{
	Script::RunScript( "launch_auto_server_notification" );
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* The server is notifying the client that he is the new		  */
/* operating server	in this fcfs server		  					  */
/******************************************************************/

int	Manager::s_handle_fcfs_assignment( Net::MsgHandlerContext* context )
{
	Manager* gamenet_man;
	PlayerInfo* player;
	uint32 *checksum;
	Script::CStruct* pParams;
	
	gamenet_man = (Manager *) context->m_Data;

	player = gamenet_man->GetLocalPlayer();
	Dbg_Assert( player );

	checksum = (uint32*) context->m_Msg;
	player->MarkAsServerPlayer();
	pParams = new Script::CStruct;
	pParams->AddChecksum( "checksum", *checksum );
	Script::RunScript( "launch_fcfs_notification", pParams );
	delete pParams;

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* The fcfs client is requesting to perform a server operation	  */
/* 								  								  */
/******************************************************************/

int	Manager::s_handle_fcfs_request( Net::MsgHandlerContext* context )
{
	PlayerInfo* player;
	Manager* gamenet_man;

	gamenet_man = (Manager *) context->m_Data;

	Dbg_Printf( "GOT FCFS REQUEST\n" );
	// First, ensure we even have an fcfs server
	player = gamenet_man->GetServerPlayer();
	if( player == NULL )
	{
		return Net::HANDLER_MSG_DONE;
	}

	// Next, make sure the requesting client matches the fcfs client
	if( player->m_Conn != context->m_Conn )
	{
		return Net::HANDLER_MSG_DONE;
	}

	Dbg_Printf( "GOT FCFS REQUEST 2\n" );
	switch( context->m_MsgId )
	{
		case MSG_ID_FCFS_START_GAME:
		{
			MsgStartGameRequest* msg;
			Prefs::Preferences* pPreferences;
			Script::CStruct* pTempStructure;
			const char* ui_string;
			int score, time, fireball_level;
	
			msg = (MsgStartGameRequest*) context->m_Msg;
			pPreferences = gamenet_man->GetNetworkPreferences();

			// First, apply the options chosen by the fcfs
			pTempStructure = new Script::CStruct;
			ui_string = gamenet_man->GetNameFromArrayEntry( "net_game_type_info", msg->m_GameMode );
			Dbg_Printf( "Got Game Mode UI String of %s\n", ui_string );
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, ui_string );
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, msg->m_GameMode );
			pPreferences->SetPreference( Script::GenerateCRC( "game_type" ), pTempStructure );
			delete pTempStructure;

			pTempStructure = new Script::CStruct;
			ui_string = gamenet_man->GetNameFromArrayEntry( "skill_level_info", msg->m_SkillLevel );
			Dbg_Printf( "Got Skill Level UI String of %s\n", ui_string );
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, ui_string );
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, msg->m_SkillLevel );
			pPreferences->SetPreference( Script::GenerateCRC( "skill_level" ), pTempStructure );
			delete pTempStructure;

			pTempStructure = new Script::CStruct;
			ui_string = gamenet_man->GetNameFromArrayEntry( "fireball_level_info", msg->m_FireballLevel );
			fireball_level = gamenet_man->GetIntFromArrayEntry( "fireball_level_info", msg->m_FireballLevel, 
																CRCD(0xce87e4e3,"fireball_level") );
			Dbg_Printf( "Got Fireball Level UI String of %s\n", ui_string );
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, ui_string );
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, msg->m_FireballLevel );
			pPreferences->SetPreference( Script::GenerateCRC( "fireball_difficulty" ), pTempStructure );
			delete pTempStructure;

			pTempStructure = new Script::CStruct;
			ui_string = gamenet_man->GetNameFromArrayEntry( "on_off_types", msg->m_PlayerCollision );
			Dbg_Printf( "Got Player Collision UI String of %s\n", ui_string );
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, ui_string );
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, msg->m_PlayerCollision );
			pPreferences->SetPreference( Script::GenerateCRC( "player_collision" ), pTempStructure );
			delete pTempStructure;

			pTempStructure = new Script::CStruct;
			ui_string = gamenet_man->GetNameFromArrayEntry( "on_off_types", msg->m_FriendlyFire );
			Dbg_Printf( "Got Friendly Fire UI String of %s\n", ui_string );
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, ui_string );
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, msg->m_FriendlyFire );
			pPreferences->SetPreference( Script::GenerateCRC( "friendly_fire" ), pTempStructure );
			delete pTempStructure;

			pTempStructure = new Script::CStruct;
			ui_string = gamenet_man->GetNameFromArrayEntry( "boolean_types", msg->m_StopAtZero );
			Dbg_Printf( "Got Stop At Zero UI String of %s\n", ui_string );
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, ui_string );
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, msg->m_StopAtZero );
			pPreferences->SetPreference( Script::GenerateCRC( "stop_at_zero" ), pTempStructure );
			delete pTempStructure;

			pTempStructure = new Script::CStruct;
			ui_string = gamenet_man->GetNameFromArrayEntry( "ctf_type", msg->m_CTFType );
			Dbg_Printf( "Got CTF Type UI String of %s\n", ui_string );
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, ui_string );
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, msg->m_CTFType );
			pPreferences->SetPreference( Script::GenerateCRC( "ctf_game_type" ), pTempStructure );
			delete pTempStructure;

            pTempStructure = new Script::CStruct;
			ui_string = gamenet_man->GetNameFromArrayEntry( "time_limit_options", msg->m_TimeLimit );
			time = gamenet_man->GetIntFromArrayEntry( "time_limit_options", msg->m_TimeLimit, CRCD( 0x906b67ba, "time" ) );
			Dbg_Printf( "Got Time Limit UI String of %s\n", ui_string );
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, ui_string );
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, msg->m_TimeLimit );
			pTempStructure->AddComponent( Script::GenerateCRC("time"), ESYMBOLTYPE_INTEGER, time );
			pPreferences->SetPreference( Script::GenerateCRC( "time_limit" ), pTempStructure );
			delete pTempStructure;

			pTempStructure = new Script::CStruct;

			score = 0;
			ui_string = gamenet_man->GetNameFromArrayEntry( "target_score_options", msg->m_TargetScore );
			score = gamenet_man->GetIntFromArrayEntry( "target_score_options", msg->m_TargetScore, CRCD( 0xcd66c8ae, "score" ));
			// Our "target score" comes from either the target score options array or the time limit options array, depending on
			// whether the game type is score challenge or koth
			if( ui_string == NULL )
			{
				ui_string = gamenet_man->GetNameFromArrayEntry( "capture_options", msg->m_TargetScore );
				score = gamenet_man->GetIntFromArrayEntry( "capture_options", msg->m_TargetScore, CRCD( 0xcd66c8ae, "score" ));
				if( ui_string == NULL )
				{
					ui_string = gamenet_man->GetNameFromArrayEntry( "time_limit_options", msg->m_TargetScore );
					score = gamenet_man->GetIntFromArrayEntry( "time_limit_options", msg->m_TargetScore, CRCD( 0x906b67ba, "time" ));
					score *= 1000;
				}
			}
			Dbg_Printf( "Got Target Score %08x UI String of %s, score %d\n", msg->m_TargetScore, ui_string, score );
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, ui_string );
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, msg->m_TargetScore );
			if( score > 0 )
			{
				pTempStructure->AddComponent( Script::GenerateCRC("score"), ESYMBOLTYPE_INTEGER, score );
			}
			pPreferences->SetPreference( Script::GenerateCRC( "target_score" ), pTempStructure );

			delete pTempStructure;

			Script::RunScript( "chosen_start_game" );
			break;
		}
		case MSG_ID_FCFS_BAN_PLAYER:
		{
			MsgRemovePlayerRequest* msg;
			PlayerInfo* player, *target_player;
			Lst::Search< PlayerInfo > sh;
			int i;

			msg = (MsgRemovePlayerRequest*) context->m_Msg;

			i = 0;
			target_player = NULL;
			for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
			{
				if( msg->m_Index == i )
				{
					if( stricmp( msg->m_Name, player->m_Name ) == 0 )
					{
						target_player = player;
					}
					break;
				}
				i++;
			}

			if( target_player == NULL )
			{
				return Net::HANDLER_MSG_DONE;
			}

			if( msg->m_Ban == 1 )
			{
				Obj::CSkater* quitting_skater;
				bool observing;
	
				quitting_skater = target_player->m_Skater;
				observing = target_player->IsObserving();
				gamenet_man->DropPlayer( target_player, vREASON_BANNED );
				if( !observing )
				{
					Mdl::Skate * skate_mod = Mdl::Skate::Instance();
					skate_mod->remove_skater( quitting_skater );
				}
			}
			else
			{
				Obj::CSkater* quitting_skater;
				bool observing;
	
				quitting_skater = target_player->m_Skater;
				observing = target_player->IsObserving();
				gamenet_man->DropPlayer( target_player, vREASON_KICKED );
				if( !observing )
				{
					Mdl::Skate * skate_mod = Mdl::Skate::Instance();
					skate_mod->remove_skater( quitting_skater );
				}
			}
			break;
		}
		case MSG_ID_FCFS_CHANGE_LEVEL:
		{
			MsgIntInfo* msg;
			Script::CStruct* pStructure;

			msg = (MsgIntInfo *) context->m_Msg;
			pStructure = new Script::CStruct;
	
			pStructure->AddComponent( Script::GenerateCRC( "level" ), ESYMBOLTYPE_NAME, (uint32) msg->m_Data );
			pStructure->AddChecksum( "from_fcfs", Script::GenerateCRC( "from_fcfs" ));
			pStructure->AddChecksum( "show_warning", Script::GenerateCRC( "show_warning" ));

			Script::RunScript( "change_level", pStructure );

			delete pStructure;
			break;
		}
		case MSG_ID_FCFS_TOGGLE_PROSET:
		{
			MsgToggleProSet* msg;

			msg = (MsgToggleProSet *) context->m_Msg;
			gamenet_man->ToggleProSet( msg->m_Bit, msg->m_ParamId );
			Script::RunScript( "toggle_proset_flag", Script::GetStructure( msg->m_ParamId, Script::ASSERT ));
			Script::RunScript( "toggle_geo_nomenu", Script::GetStructure( msg->m_ParamId, Script::ASSERT ));
			break;
		}
		case MSG_ID_FCFS_TOGGLE_GOAL_SELECTION:
		{
			Game::CGoalManager* pGoalManager = Game::GetGoalManager();
			MsgIntInfo* msg;
			
			msg = (MsgIntInfo*) context->m_Msg;

			pGoalManager->ToggleGoalSelection( msg->m_Data );
			break;
		}
		case MSG_ID_FCFS_END_GAME:
		{
			Net::Server* server;
			Lst::Search< PlayerInfo > sh;
			Net::MsgDesc msg_desc;

			server = gamenet_man->GetServer();

			Script::RunScript( "fcfc_end_game_selected" );
	
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
			break;
		}

		case MSG_ID_FCFS_SET_NUM_TEAMS:
		{
			Prefs::Preferences* prefs;
			Script::CStruct* pParams, *pPrefStruct;
			MsgByteInfo* msg;
            
			msg = (MsgByteInfo*) context->m_Msg;
			Dbg_Printf( "GOT FCFS REQUEST : NUM TEAMS %d\n", msg->m_Data );
			pParams = new Script::CStruct;
			pParams->AddInteger( NONAME, msg->m_Data );

			pPrefStruct = new Script::CScriptStructure;
			switch( msg->m_Data )
			{
				case 0:
					pPrefStruct->AddString( "ui_string", "None" );
					pPrefStruct->AddChecksum( "checksum", Script::GenerateCRC( "teams_none" ));
					break;
				case 2:
					pPrefStruct->AddString( "ui_string", "2" );
					pPrefStruct->AddChecksum( "checksum", Script::GenerateCRC( "teams_two" ));
					break;
				case 3:
					pPrefStruct->AddString( "ui_string", "3" );
					pPrefStruct->AddChecksum( "checksum", Script::GenerateCRC( "teams_three" ));
					break;
				case 4:
					pPrefStruct->AddString( "ui_string", "4" );
					pPrefStruct->AddChecksum( "checksum", Script::GenerateCRC( "teams_four" ));
					break;
				default:
					Dbg_Assert( 0 );
					break;
			}
			
			prefs = gamenet_man->GetNetworkPreferences();
			prefs->SetPreference( Script::GenerateCRC("team_mode"), pPrefStruct );
			delete pPrefStruct;

			ScriptSetNumTeams( pParams, NULL );

			delete pParams;
			break;
		}
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* The king has lost his crown. Spawn a new one in the world at   */
/* the spawn point specified									  */
/******************************************************************/

int	Manager::s_handle_dropped_crown( Net::MsgHandlerContext* context )
{
	Manager* gamenet_man;
	PlayerInfo* king, *other_player;
	char spawn_pt;
	Obj::CCrown* crown;
	
	gamenet_man = (Manager *) context->m_Data;
	king = gamenet_man->GetKingOfTheHill();
	memcpy( &spawn_pt, context->m_Msg, sizeof( char ));
	if( king )
	{
		// He is no longer king
		king->MarkAsKing( false );
	}
	
	if( king && king->IsLocalPlayer())
	{
		gamenet_man->CreateNetPanelMessage( false, Script::GenerateCRC("net_message_dropped_crown_you"),
											NULL, NULL, king->m_Skater );
	}
	else
	{   
		if( gamenet_man->InNetGame())
		{
			gamenet_man->CreateNetPanelMessage( false, Script::GenerateCRC("net_message_dropped_crown_other"));
		}
		else if( king && king->m_Skater )
		{   
			int other_id;

			// NOTE: This code only works for 2-player splitscreen. If we ever go to four,
			// it needs to be more sophisticated
			if( king->m_Skater->GetID() == 0 )
			{
				other_id = 1;
			}
			else
			{ 
				other_id = 0;
			}

			other_player = gamenet_man->GetPlayerByObjectID( other_id );
			if( other_player )
			{
				gamenet_man->CreateNetPanelMessage( false, Script::GenerateCRC("net_message_dropped_crown_other"),
													NULL, NULL, other_player->m_Skater );
			}
		}
	}

	crown = gamenet_man->GetCrown();
	if( crown )
	{
#if 1
		int loop_count = 0;
		bool node_found = false;
		
		// KLUDGE: keep looping through incase spawn_pt is larger than the number of crowns in the node array
		while (!node_found)
#endif
		{
#if 1
			// KLUDGE: make sure we exit this loop at some point
			loop_count++;
			if (loop_count == 50) break;
#endif

			int i;
			
			Script::CArray *pNodeArray=Script::GetArray("NodeArray");
	
			// Make sure there's a node array
			Dbg_Assert( pNodeArray != NULL );
							 
			i = 0;
			// scan through it for a crown spawn point
			while( i < (int)pNodeArray->GetSize() )
			{
				uint32	TypeChecksum;
				Mth::Vector pos;
				Script::CScriptStructure *pNode=pNodeArray->GetStructure(i);
				
				TypeChecksum = 0;
				pNode->GetChecksum("Type",&TypeChecksum);
				if( TypeChecksum == 0xaf86421b )	// checksum of "Crown"
				{
					// We want the Nth crown spawn pt
					if( spawn_pt == 0 )
					{
						SkateScript::GetPosition( pNode, &pos );
						crown->SetPosition( pos );
#if 1
						node_found = true;
#endif
						break;
					}
					spawn_pt--;
				}
				i++;
			}
		}

		crown->RemoveFromKing();
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* The player has dropped a ctf flag.  Relocate it.				  */
/*																  */
/******************************************************************/

int	Manager::s_handle_dropped_flag( Net::MsgHandlerContext* context )
{
	Manager* gamenet_man;
	MsgFlagMsg* msg;
	PlayerInfo* player, *local_player;

	gamenet_man = (Manager*) context->m_Data;
	msg = (MsgFlagMsg*) context->m_Msg;
	player = gamenet_man->GetPlayerByObjectID( msg->m_ObjId );
	if( player && player->HasCTFFlag())
	{
		char team_str[64];
		int team;
		Script::CStruct* pParams;
		
		team = player->HasWhichFlag();
		sprintf( team_str, "team_%d_name", team + 1 );
		pParams = new Script::CStruct;

		pParams->AddInteger( "team", team );
		pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, Script::GetString( team_str ));
		Script::RunScript( "flag_returned", pParams );

		delete pParams;

		// If it's me, remove the message that says "return the flag to your base"
		if( player->IsLocalPlayer())
		{
			Script::RunScript( "destroy_ctf_panel_message" );
		}

		local_player = gamenet_man->GetLocalPlayer();
		if( local_player && !local_player->IsObserving())
		{
			if( local_player->m_Team == team )
			{
				Script::RunScript( "hide_ctf_arrow" );
			}
		}

		player->ClearCTFState();
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Streams the level data to the client    						  */
/*                                                 				  */
/******************************************************************/

int	Manager::s_handle_request_level( Net::MsgHandlerContext* context )
{
	Net::Server* server;
	MsgRequestLevel* msg;
	Manager* gamenet_man;
	PlayerInfo* player;

	gamenet_man = (Manager*) context->m_Data;
	player = gamenet_man->GetPlayerByConnection( context->m_Conn );
	msg = (MsgRequestLevel*) context->m_Msg;
		 
	server = (Net::Server*) context->m_App;
	Dbg_Printf( "************** STREAMING LEVEL DATA ***************\n" );
	
	server->StreamMessage( context->m_Conn->GetHandle(), MSG_ID_LEVEL_DATA, Ed::CParkManager::COMPRESSED_MAP_SIZE, 
						   Ed::CParkManager::sInstance()->GetCompressedMapBuffer(), "level data", vSEQ_GROUP_PLAYER_MSGS,
						   false, true );
						   
	server->StreamMessage( context->m_Conn->GetHandle(), MSG_ID_RAIL_DATA, Obj::GetRailEditor()->GetCompressedRailsBufferSize(), 
						   Obj::GetRailEditor()->GetCompressedRailsBuffer(), "rail data", vSEQ_GROUP_PLAYER_MSGS,
						   false, true );
						   
						   
	if( gamenet_man->UsingCreatedGoals())
	{
		gamenet_man->LoadGoals( msg->m_LevelId );
		server->StreamMessage( context->m_Conn->GetHandle(), MSG_ID_GOALS_DATA, gamenet_man->GetGoalsDataSize(),
							   gamenet_man->GetGoalsData(), "goals data", vSEQ_GROUP_PLAYER_MSGS, false, true );
	}

	if( msg->m_Source == MSG_ID_CHANGE_LEVEL )
	{
		GameNet::MsgReady ready_msg;
		MsgChangeLevel change_msg;
		Net::MsgDesc msg_desc;

		Dbg_Printf( "************** SENDING CHANGE LEVEL DATA ***************\n" );
			
		change_msg.m_Level = msg->m_LevelId;
		change_msg.m_ShowWarning = 0;
			
		ready_msg.m_Time = Tmr::GetTime();

		msg_desc.m_Data = &change_msg;
		msg_desc.m_Length = sizeof(MsgChangeLevel);
		msg_desc.m_Id = MSG_ID_CHANGE_LEVEL;
		msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
		msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
		server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
	
		// Don't send them any non-important messages until they're finished loading
		msg_desc.m_Data = &ready_msg;
		msg_desc.m_Length = sizeof(MsgReady);
		msg_desc.m_Id = MSG_ID_READY_QUERY;
		server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
					
		player->MarkAsNotReady( ready_msg.m_Time );
		
		server->SendData();		// Mick, (true) because we want to send it immediatly
		
#ifdef __PLAT_NGPS__
		//server->WaitForAsyncCallsToFinish();
#endif
	} 

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Handles level data from server    						  	  */
/*                                                 				  */
/******************************************************************/

int	Manager::s_handle_level_data( Net::MsgHandlerContext* context )
{
    Dbg_Printf( "received streamed message! size %d\n", (int)context->m_MsgLength ); 
	
	Ed::CParkManager::sInstance()->SetCompressedMapBuffer((uint8*) context->m_Msg, true );

	uint32 crc;
	crc = Crc::GenerateCRCCaseSensitive((char*) Ed::CParkManager::sInstance()->GetCompressedMapBuffer(), Ed::CParkManager::COMPRESSED_MAP_SIZE );
	Dbg_Printf( "******************** CHECKSUM OF MAP : %08x\n", crc );
	
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Handles rail data from server    						  	  */
/*                                                 				  */
/******************************************************************/

int	Manager::s_handle_rail_data( Net::MsgHandlerContext* context )
{
	Manager* gamenet_man = (Manager*) context->m_Data;
    
	Dbg_Printf( "received streamed message! size %d\n", (int)context->m_MsgLength ); 

	if( context->m_PacketFlags & Net::mHANDLE_CRC_MISMATCH )
	{
		Mdl::Skate* skate_mod = Mdl::Skate::Instance();
		Net::MsgDesc msg_desc;

		if( skate_mod->m_cur_level == CRCD(0x9f2bafb7,"Load_Skateshop"))
		{
			Script::CStruct* params;

			params = new Script::CStruct;
			params->AddChecksum( NONAME, CRCD(0x19eca78e,"show_timeout"));

			// Set it back to the joining state so that cancel_join_server does not
			// opt out early
			gamenet_man->m_join_state_task->Remove();
			gamenet_man->SetJoinState( vJOIN_STATE_JOINING );

			Dbg_Printf( "************** received bad rails data!! Cancelling join.\n" );
			Script::RunScript( CRCD(0x60b653db,"cancel_join_server" ), params );
			delete params;

			return Net::HANDLER_MSG_DESTROYED;
		}
		else
		{
			Dbg_Printf( "************** received bad rails data!! Re-requesting it.\n" );
			Dbg_Printf( "*** Level was 0x%x : %s\n", skate_mod->m_cur_level, Script::FindChecksumName(skate_mod->m_cur_level));

			msg_desc.m_Id = MSG_ID_REQUEST_RAILS_DATA;
			msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
			msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	
			context->m_App->EnqueueMessageToServer( &msg_desc );

			return Net::HANDLER_MSG_DONE;
		}
	}

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
	
	Obj::GetRailEditor()->SetCompressedRailsBuffer((uint8*) context->m_Msg);
    
    // InitUsingCompressedRailsBuffer will do an initial cleanup of any existing rails, which may result in
    // a call to UpdateSuperSectors (due to sectors being deleted).  
    // Trouble is, UpdateSuperSectors will cause the park to disappear from under the skater, so disable
    // it temporarily.
   
	Obj::CRailEditorComponent::sUpdateSuperSectorsAfterDeletingRailSectors=false;   
    Obj::GetRailEditor()->InitUsingCompressedRailsBuffer();
	Obj::CRailEditorComponent::sUpdateSuperSectorsAfterDeletingRailSectors=true;
		
	Mem::Manager::sHandle().PopContext();

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Handles goals data from server    						  	  */
/*                                                 				  */
/******************************************************************/

int Manager::s_handle_goals_data( Net::MsgHandlerContext* context )
{
	uint32 level;
	uint8* goals_data;
	Manager* gamenet_man;
	Script::CStruct* params;
	Net::MsgDesc msg_desc;
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();

	gamenet_man = (Manager *) context->m_Data;

	Dbg_Printf( "received goals data\n" );
	if( context->m_PacketFlags & Net::mHANDLE_CRC_MISMATCH )
	{
		if( skate_mod->m_cur_level == CRCD(0x9f2bafb7,"Load_Skateshop"))
		{
			Script::CStruct* params;
			Dbg_Printf( "************** received bad goals data!! Cancelling join server.\n" );

			// Set it back to the joining state so that cancel_join_server does not
			// opt out early
			gamenet_man->m_join_state_task->Remove();
			gamenet_man->SetJoinState( vJOIN_STATE_JOINING );

			params = new Script::CStruct;
			params->AddChecksum( NONAME, CRCD(0x19eca78e,"show_timeout"));
			Script::RunScript( CRCD(0x60b653db,"cancel_join_server" ), params );
			delete params;
			return Net::HANDLER_MSG_DESTROYED;
		}
		else
		{
			Dbg_Printf( "************** received bad goals data!! Re-requesting it.\n" );
			
			msg_desc.m_Id = MSG_ID_REQUEST_GOALS_DATA;
			msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
			msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	
			context->m_App->EnqueueMessageToServer( &msg_desc );
			return Net::HANDLER_MSG_DONE;
		}
	}
	
	memcpy( &level, context->m_Msg, sizeof( uint32 ));

	//Dbg_Printf( "*** Level: 0x%x  requested level: 0x%x\n", level, Mdl::Skate::Instance()->m_requested_level );
	//skate_mod->m_requested_level = level;
	//skate_mod->m_cur_level = level;

	goals_data = (uint8*) ( context->m_Msg + sizeof( uint32 ));
	Obj::GetGoalEditor()->ReadFromBuffer( level, goals_data );

	

	params = new Script::CStruct;
	params->AddChecksum( NONAME, CRCD(0x88001327,"DoNotCreateGoalPeds"));
	Script::RunScript( CRCD(0x6f4180d0,"InitialiseCreatedGoals"), params );
	delete params;

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Handles any sort of change of level. Makes sure we actually    */
/* Have that level                                                */
/******************************************************************/

int	Manager::s_handle_new_level( Net::MsgHandlerContext* context )
{
	Dbg_Printf( "***************** Handling new level!!!\n" );
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Manager::s_handle_change_level( Net::MsgHandlerContext* context )
{
	Manager* gamenet_man;
	MsgChangeLevel* pMsg;

	gamenet_man = (Manager *) context->m_Data;
	
	pMsg = (MsgChangeLevel*) context->m_Msg;
	
	Dbg_Printf( "********** GameNet:: Got change level\n" );
	gamenet_man->ClearTriggerEventList();
	gamenet_man->m_game_over = false;
	
	if( pMsg->m_ShowWarning )
	{
		Dbg_Printf( "******** GameNet:: IN NET GAME!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
		Mlp::Manager * mlp_man = Mlp::Manager::Instance();
				
		// Don't allow two quick change level commands
		if( gamenet_man->m_change_level_task->InList())
		{
			if( !gamenet_man->OnServer())
			{
				gamenet_man->m_level_id = pMsg->m_Level;
			}
			
			return Net::HANDLER_MSG_DONE;
		}

		gamenet_man->m_level_id = pMsg->m_Level;
		s_time_change_level = Tmr::GetTime() + vTIME_BEFORE_CHANGING_LEVEL;

		if( gamenet_man->OnServer())
		{                                    
			Script::CScriptStructure* pTempStructure;
			Prefs::Preferences* pPreferences;
			
			pTempStructure = new Script::CScriptStructure;
			pTempStructure->AddComponent( Script::GenerateCRC("ui_string"), ESYMBOLTYPE_STRING, 
										  gamenet_man->GetLevelName( false ));
			pTempStructure->AddComponent( Script::GenerateCRC("checksum"), ESYMBOLTYPE_NAME, (int) gamenet_man->m_level_id );
			pPreferences = gamenet_man->GetNetworkPreferences();
			pPreferences->SetPreference( Script::GenerateCRC( "level"), pTempStructure );
			delete pTempStructure;

			// Allow a little extra time for the transmission of level data
			if( gamenet_man->m_level_id == CRCD(0xb664035d,"Load_Sk5Ed_gameplay"))
			{
				s_time_change_level += Tmr::Seconds( 1 );
			}
		}
			
		mlp_man->AddLogicTask( *gamenet_man->m_change_level_task );

		gamenet_man->CreateNetPanelMessage( false, Script::GenerateCRC("net_message_changing_levels"),
											gamenet_man->GetLevelName(), NULL, NULL, NULL, false, Tmr::Seconds( 5 ));
		
		Script::RunScript("hide_console_window");
		// Don't let this message pass through the Skate. We don't want to change levels just yet
		return Net::HANDLER_MSG_DONE;
	}
	
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Manager::s_handle_kill_flags( Net::MsgHandlerContext* context )
{
	Script::RunScript( "Kill_Team_Flags" );
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	Manager::s_handle_game_info( Net::MsgHandlerContext* context )
{   
    Manager* gamenet_man;
	MsgGameInfo* msg;
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Script::CArray* mode_array;
	int i;
    
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());

	msg = (MsgGameInfo*) context->m_Msg;
	gamenet_man = (Manager *) context->m_Data;

	Dbg_Printf( "Got game info\n" );

	gamenet_man->SetSkaterStartingPoints( msg->m_StartPoints );
	gamenet_man->m_crown_spawn_point = msg->m_CrownSpawnPoint;
	gamenet_man->m_waiting_for_game_to_start = false;

	// Invalidate old positions. Player will be re-added to the world when we get a
	// sufficient number of object updates
	for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
	{
		if( player->IsLocalPlayer() == false )
		{
			player->m_Skater->RemoveFromCurrentWorld();
			player->m_Skater->Resync();
		}
	}

	// set up the game mode for the client cause we're now in the lobby
	// anything that's lobby-dependent should go after this line
	skate_mod->GetGameMode()->LoadGameType( msg->m_GameMode );
	
	// Remove all currently-running goals
	skate_mod->GetGoalManager()->DeactivateAllGoals();	

	if( gamenet_man->InNetGame())
	{
		mode_array = Script::GetArray( "net_game_type_info" );
	}
	else
	{
		mode_array = Script::GetArray( "mp_game_type_info" );
	}
	
	Dbg_Assert( mode_array );

	{
		Script::CScriptStructure* pTempStructure = new Script::CScriptStructure;

		// In king of the hill, interpret time limit as a target time
		if(	( msg->m_TimeLimit == 0 ) &&
			( msg->m_GameMode != Script::GenerateCRC( "netgoalattack" )))
		{
			pTempStructure->AddComponent( Script::GenerateCRC("default_time_limit"), ESYMBOLTYPE_INTEGER, (int) 0 );
			if( ( msg->m_GameMode != CRCD(0x3d6d444f,"firefight")) &&
				( msg->m_GameMode != CRCD(0xbff33600,"netfirefight")))
			{
				Script::CArray* pArray = new Script::CArray;
				Script::CopyArray(pArray,Script::GetArray("targetScoreArray") );
				Script::CScriptStructure* pSubStruct = pArray->GetStructure(0);
				Dbg_Assert(pSubStruct);
				pSubStruct->AddComponent(Script::GenerateCRC("score"),ESYMBOLTYPE_INTEGER, msg->m_TargetScore );
				
				pTempStructure->AddComponent( Script::GenerateCRC("victory_conditions"), pArray );
			}
		}
		else
		{
			pTempStructure->AddComponent( Script::GenerateCRC("default_time_limit"), ESYMBOLTYPE_INTEGER, (int) msg->m_TimeLimit );
			
			if( msg->m_GameMode == Script::GenerateCRC( "netctf" ))
			{
				Script::CArray* pArray = new Script::CArray;
				Script::CopyArray(pArray,Script::GetArray("highestScoreArray") );
				pTempStructure->AddComponent( Script::GenerateCRC("victory_conditions"), pArray );
			}
		}

		Dbg_Printf( "***** GOT FIREBALL LEVEL OF %d\n", msg->m_FireballLevel );
		pTempStructure->AddComponent( CRCD(0xce87e4e3,"fireball_level"), ESYMBOLTYPE_INTEGER, msg->m_FireballLevel );
		pTempStructure->AddComponent( CRCD(0x48e748b5,"stop_at_zero"), ESYMBOLTYPE_INTEGER, msg->m_StopAtZero );
	    Dbg_Printf( "******************** Setting time limit to %d\n", (int)msg->m_TimeLimit );
		skate_mod->SetTimeLimit( msg->m_TimeLimit );
		skate_mod->GetGameMode()->OverrideOptions( pTempStructure );
		skate_mod->GetGameMode()->SetNumTeams( msg->m_TeamMode );
		delete pTempStructure;
	}

	Script::RunScript( "StartingNewNetGame" );
	
	// the following sets up the panel and stuff
	skate_mod->LaunchGame();
	gamenet_man->ResetPlayers();
	gamenet_man->m_game_pending = false;
	gamenet_man->m_cheating_occurred = false;
	gamenet_man->MarkReceivedFinalScores( false );
	gamenet_man->SetCurrentLeader( NULL );
	gamenet_man->SetCurrentLeadingTeam( vNO_TEAM );
	gamenet_man->SetNetworkGameId( msg->m_GameId );
	if( msg->m_GameMode == Script::GenerateCRC( "netgoalattack" ))
	{
		skate_mod->GetGoalManager()->InitializeAllSelectedGoals();
	}
	gamenet_man->m_game_over = false;
	
	// Clear all scores
	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; player = gamenet_man->NextPlayerInfo( sh, true ))
	{
		Mdl::Score* score;
		
		player->ClearCTFState();
		player->ResetProjectileVulnerability();
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
    
	// Clear king of the hill data
	player = gamenet_man->GetKingOfTheHill();
	if( player )
	{
		player->MarkAsKing( false );
	}
	
	for( i = 0; i < (int)mode_array->GetSize(); i++ )
	{   
		uint32 value, script;
		Script::CStruct* mode_struct;
		 
		mode_struct = mode_array->GetStructure( i );
		Dbg_Assert( mode_struct );
		
		mode_struct->GetChecksum( "checksum", &value, true );
		if( value == msg->m_GameMode )
		{
			Script::CStruct* params;
						
			params = new Script::CStruct;
			params->AddInteger( "time", msg->m_TimeLimit );
			params->AddInteger( "score", msg->m_TargetScore );
			if( msg->m_TimeLimit == 0 )
			{
				params->AddInteger( CRCD(0xf0e712d2,"unlimited_time"), 1 );
			}
			else
			{
				params->AddInteger( CRCD(0xf0e712d2,"unlimited_time"), 0 );
			}
			mode_struct->GetChecksum( "goal_script", &script, true );
			Script::RunScript( script, params );
			delete params;
			break;
		}
	}

	Mem::Manager::sHandle().PopContext();
	
	
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Manager::s_handle_select_goals( Net::MsgHandlerContext* context )
{
	Net::MsgMax* msg;
	Game::CGoalManager* pGoalManager;
	Game::CGoal* pGoal;
	char* data;
	uint32 goal_id;
	char show_summary;
		
	msg = (Net::MsgMax*) context->m_Msg;	 
	data = msg->m_Data;
	pGoalManager = Game::GetGoalManager();
	pGoalManager->DeselectAllGoals();
	show_summary = *data++;
	memcpy( &goal_id, data, sizeof( uint32 ));
	data+= sizeof( uint32 );
	while( goal_id )
	{
		pGoal = pGoalManager->GetGoal( goal_id );
		Dbg_Assert( pGoal );

		pGoal->UnBeatGoal();
		pGoal->Select();
		
		memcpy( &goal_id, data, sizeof( uint32 ));
		data+= sizeof( uint32 );
	}

	if( show_summary == 1 )
	{
		Script::CStruct* pParams;
		pParams = new Script::CStruct;
		pParams->AddChecksum( "goal_summary", Script::GenerateCRC( "goal_summary" ));
		#ifdef __NOPT_ASSERT__
		Script::CScript *p_script=Script::SpawnScript( "wait_and_create_view_selected_goals_menu", pParams );
		p_script->SetCommentString("Spawned from Manager::s_handle_select_goals");
		#else
		Script::SpawnScript( "wait_and_create_view_selected_goals_menu", pParams );
		#endif
		delete pParams;
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Manager::s_handle_panel_message( Net::MsgHandlerContext* context )
{
    GameNet::MsgPanelMessage *msg;
	Manager* gamenet_man;
    
	msg = (GameNet::MsgPanelMessage*) context->m_Msg;
	gamenet_man = (Manager *) context->m_Data;
	
	gamenet_man->CreateNetPanelMessage( false, msg->m_StringId, msg->m_Parm1, msg->m_Parm2, NULL, NULL, false, msg->m_Time );

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	Manager::s_handle_run_ended( Net::MsgHandlerContext* context )
{   
	Manager* gamenet_man;
	PlayerInfo* player;
	bool all_done;
	unsigned char game_id;

	gamenet_man = (Manager *) context->m_Data;
	player = gamenet_man->GetPlayerByConnection( context->m_Conn );
	memcpy( &game_id, context->m_Msg, sizeof( unsigned char ));
	//Dbg_Printf( "**** Got end of run from player\n" );

	if( player )
	{
		Lst::Search< PlayerInfo > sh;
		if( player->m_flags.TestMask( PlayerInfo::mRUN_ENDED ))
		{
			//Dbg_Printf( "**** Run had already ended\n" );
			return Net::HANDLER_MSG_DONE;
		}

		// Make sure this message pertains to the current game
		if( game_id != gamenet_man->GetNetworkGameId())
		{
			//Dbg_Printf( "**** Wrong game id\n" );
			return Net::HANDLER_MSG_DONE;
		}
		
		player->m_flags.SetMask( PlayerInfo::mRUN_ENDED );
		
		all_done = true;
		for( player = gamenet_man->FirstPlayerInfo( sh ); player;
				player = gamenet_man->NextPlayerInfo( sh ))
		{
			if( !player->m_flags.TestMask( PlayerInfo::mRUN_ENDED))
			{
				all_done = false;
				break;
			}
		}

		if( all_done )
		{
			Net::Server* server;
			Net::MsgDesc msg_desc;

			server = gamenet_man->GetServer();
			Dbg_Assert( server );

			msg_desc.m_Id = MSG_ID_GAME_OVER;
			msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
			msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
			for( player = gamenet_man->FirstPlayerInfo( sh, true ); player;
				player = gamenet_man->NextPlayerInfo( sh, true ))
			{
				server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
			}
		}
		else
		{
			//Dbg_Printf( "**** Not all done id\n" );
		}
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	Manager::s_handle_game_over( Net::MsgHandlerContext* context )
{   
	Manager* gamenet_man;
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();

	gamenet_man = (Manager *) context->m_Data;
	gamenet_man->MarkGameOver();

	if( ( CFuncs::ScriptIsObserving( NULL, NULL )) &&
		( skate_mod->GetGameMode()->GetNameChecksum() == CRCD(0xec200eaa,"netgoalattack" )))
	{
		Script::RunScript( CRCD(0xfce4f72d,"create_rankings"));
	}
	
	//Script::RunScript( "set_lobby_mode" );

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	Manager::s_handle_end_game( Net::MsgHandlerContext* context )
{   
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CTrickObjectManager* manager = skate_mod->GetTrickObjectManager();

	manager->DeleteAllTrickObjects();
	Script::RunScript( "create_game_ended_dialog" );

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Just calculate the length of the message and pass it back to   */
/* the dispatcher                                                 */
/******************************************************************/

int	Manager::s_handle_object_update( Net::MsgHandlerContext* context )
{   
	Manager* gamenet_man;
	int update_flags;
	unsigned char obj_id, obj_id_mask;
	Net::BitStream stream;
    
	Dbg_Assert( context );
     
	gamenet_man = (Manager *) context->m_Data;
	stream.SetInputData( context->m_Msg, 1024 );
	obj_id_mask = stream.ReadUnsignedValue( sizeof( char ) * 8 );
		
	for( obj_id = 0; obj_id < Mdl::Skate::vMAX_SKATERS; obj_id++ )
	{
		int value;

		// If the bit for this player number is not set, that means there is no 
		// object update for them. Continue
		if(( obj_id_mask & ( 1 << obj_id )) == 0 )
		{
			continue;
		}

		value = stream.ReadUnsignedValue( sizeof( uint16 ) * 8 );
		//Dbg_Printf( "Length after timestamp: %d\n", stream.GetByteLength());
		
		update_flags = stream.ReadUnsignedValue( 9 );
		//Dbg_Printf( "Length after update flags: %d\n", stream.GetByteLength());
		
		if( update_flags & GameNet::mUPDATE_FIELD_POS_X )
		{   
			value = stream.ReadSignedValue( sizeof( short ) * 8 );
			//Dbg_Printf( "Length after pos X: %d\n", stream.GetByteLength());
		}
		if( update_flags & GameNet::mUPDATE_FIELD_POS_Y )
		{
			value = stream.ReadSignedValue( sizeof( short ) * 8 );
			//Dbg_Printf( "Length after pos Y: %d\n", stream.GetByteLength());
		}
		if( update_flags & GameNet::mUPDATE_FIELD_POS_Z )
		{
			value = stream.ReadSignedValue( sizeof( short ) * 8 );
			//Dbg_Printf( "Length after pos Z: %d\n", stream.GetByteLength());
		}   
				
		if( update_flags & GameNet::mUPDATE_FIELD_ROT_X )
		{
			value = stream.ReadSignedValue( sizeof( short ) * 8 );
			//Dbg_Printf( "Length after rot X: %d\n", stream.GetByteLength());
		}
		if( update_flags & GameNet::mUPDATE_FIELD_ROT_Y )
		{
			value = stream.ReadSignedValue( sizeof( short ) * 8 );
			//Dbg_Printf( "Length after rot Y: %d\n", stream.GetByteLength());
		}
		if( update_flags & GameNet::mUPDATE_FIELD_ROT_Z )
		{
			value = stream.ReadSignedValue( sizeof( short ) * 8 );
			//Dbg_Printf( "Length after rot Z: %d\n", stream.GetByteLength());
		}
        
		if( update_flags & GameNet::mUPDATE_FIELD_STATE )
		{   
			value = stream.ReadUnsignedValue( 4 );
			//Dbg_Printf( "Length after state1: %d\n", stream.GetByteLength());
			value = stream.ReadUnsignedValue( 6 );
			value = stream.ReadUnsignedValue( 1 );
			value = stream.ReadUnsignedValue( 1 );
			//Dbg_Printf( "Length after state2: %d\n", stream.GetByteLength());
		}

		if( update_flags & GameNet::mUPDATE_FIELD_FLAGS )
		{
			value = stream.ReadUnsignedValue( 8 );
			//Dbg_Printf( "Length after flags: %d\n", stream.GetByteLength());
		}

		if( update_flags & GameNet::mUPDATE_FIELD_RAIL_NODE )
		{
			value = stream.ReadSignedValue( sizeof( sint16 ) * 8 );
			//Dbg_Printf( "Length After Rail Node: %d\n", stream.GetByteLength());
		}
	}
			
	// Tell the dispatcher the size of this message because it does not know (optimization)
	context->m_MsgLength = stream.GetByteLength();
	//Dbg_Printf( "Message Length %d\n", context->m_MsgLength );

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Someone has requested to change teams						  */
/*                                                  			  */
/******************************************************************/

int	Manager::s_handle_team_change_request( Net::MsgHandlerContext* context )
{
	Manager* gamenet_man;
	PlayerInfo* change_player, *player;
	MsgChangeTeam* msg;
	Net::Server* server;
	MsgChangeTeam change_msg;
	Net::MsgDesc msg_desc;
	Lst::Search< PlayerInfo > sh;
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		
	gamenet_man = (Manager *) context->m_Data;
	
	msg = (MsgChangeTeam*) context->m_Msg;
	change_player = gamenet_man->GetPlayerByObjectID( msg->m_ObjID );
	if(( change_player == NULL ) ||
	   ( change_player->m_Skater == NULL ) ||
	   ( skate_mod->GetGameMode()->GetNameChecksum() != Script::GenerateCRC( "netlobby" )))	// ignore unless in lobby
	{
		return Net::HANDLER_MSG_DONE;
	}
	change_msg.m_ObjID = msg->m_ObjID;
	change_msg.m_Team = msg->m_Team;

	server = gamenet_man->GetServer();
	Dbg_Assert( server );

	msg_desc.m_Data = &change_msg;
	msg_desc.m_Length = sizeof( MsgChangeTeam );
	msg_desc.m_Id = MSG_ID_TEAM_CHANGE;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; 
				player = gamenet_man->NextPlayerInfo( sh, true ))
	{
		server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Someone has changed teams  									  */
/*                                                  			  */
/******************************************************************/

int	Manager::s_handle_team_change( Net::MsgHandlerContext* context )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Manager* gamenet_man;
	PlayerInfo* player, *other_player;
	Lst::Search< PlayerInfo > sh;
	MsgChangeTeam* msg;
	int num_other_players;
		
	gamenet_man = (Manager *) context->m_Data;
	msg = (MsgChangeTeam*) context->m_Msg;
	
	player = gamenet_man->GetPlayerByObjectID( msg->m_ObjID );
	if( player )
	{
		char team_str[64];
		Script::CStruct* pParams;
		bool all_same_team;
		
		player->m_Team = msg->m_Team;
		sprintf( team_str, "team_%d_name", msg->m_Team + 1 );
		pParams = new Script::CStruct;
		if( player->IsLocalPlayer())
		{
			if( skate_mod->GetGameMode()->IsTeamGame())
			{
				pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, Script::GetString( team_str ));
				Script::RunScript( "joined_team_you", pParams );
			}
		}
		else
		{
			if( skate_mod->GetGameMode()->IsTeamGame())
			{
				pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, player->m_Name );
				pParams->AddComponent( Script::GenerateCRC("String1"), ESYMBOLTYPE_STRING, Script::GetString( team_str ));
				Script::RunScript( "joined_team_other", pParams );
			}
		}
		
		delete pParams;

		all_same_team = true;
		num_other_players = 0;
		for( other_player = gamenet_man->FirstPlayerInfo( sh ); other_player; 
				other_player = gamenet_man->NextPlayerInfo( sh ))
		{
			if( other_player != player )
			{
				num_other_players++;
				if( msg->m_Team != other_player->m_Team )
				{
					all_same_team = false;
				}
			}
		}

		if( all_same_team && ( num_other_players > 0 ))
		{
			if( skate_mod->GetGameMode()->NumTeams() > 0 )
			{
				Script::RunScript( CRCD(0x16c9b0dc,"warn_all_same_team"));
			}
		}
	}
	
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Client beat a goalattack goal. Verify that no other teammate	  */
/* has beaten it and relay it to all clients involved  			  */
/******************************************************************/

int Manager::s_handle_beat_goal( Net::MsgHandlerContext* context )
{
	MsgBeatGoal *msg;
	Manager* gamenet_man;
	Game::CGoalManager* pGoalManager;
	Game::CGoal* pGoal;    
		
	gamenet_man = (Manager *) context->m_Data;
	msg = (MsgBeatGoal*) context->m_Msg;
    
	Dbg_Printf( "*** Got s_handle_beat_goal: 0x%x\n", msg->m_GoalId );
	// Make sure this message pertains to the current game
	if( msg->m_GameId != gamenet_man->GetNetworkGameId())
	{
		return Net::HANDLER_MSG_DONE;
	}

	pGoalManager = Game::GetGoalManager();
	pGoal = pGoalManager->GetGoal( msg->m_GoalId );
	if( pGoal )
	{
		PlayerInfo* player;
		bool already_beaten;

		player = gamenet_man->GetPlayerByConnection( context->m_Conn );
		Dbg_Assert( player );

		already_beaten = false;
		if( player->m_Skater )
		{   
			Mdl::Skate * skate_mod = Mdl::Skate::Instance();

			// If we're in a team game, flag the goal as completed for all team members just in case someone quits later.
			// Also, uninitialize the goal
			if( skate_mod->GetGameMode()->IsTeamGame())
			{
				PlayerInfo* team_player;
				Lst::Search< PlayerInfo > sh;

				for( team_player = gamenet_man->FirstPlayerInfo( sh ); team_player; 
						team_player = gamenet_man->NextPlayerInfo( sh ))
				{
					if( player->m_Team == team_player->m_Team )
					{
						if( pGoal->HasWonGoal( team_player->m_Skater->GetID()))
						{
							already_beaten = true;
							break;
						}
					}
				}
				
				if( !already_beaten )
				{
					pGoal->MarkBeatenBy( player->m_Skater->GetID());
					// Notify all team members of the fact that the goal was beaten
					for( team_player = gamenet_man->FirstPlayerInfo( sh ); team_player; 
							team_player = gamenet_man->NextPlayerInfo( sh ))
					{
						MsgBeatGoalRelay goal_msg;
						Net::MsgDesc msg_desc;

						goal_msg.m_GoalId = msg->m_GoalId;
						goal_msg.m_ObjId = player->m_Skater->GetID();
						
						msg_desc.m_Data = &goal_msg;
						msg_desc.m_Length = sizeof( MsgBeatGoalRelay );
						msg_desc.m_Id = MSG_ID_BEAT_GOAL;
						msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
						msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
						
						context->m_App->EnqueueMessage( team_player->GetConnHandle(), &msg_desc );
					}
				}
			}
			else
			{
				if( pGoal->HasWonGoal( player->m_Skater->GetID()) == false )
				{
					PlayerInfo* send_player;
					Lst::Search< PlayerInfo > sh;
					MsgBeatGoalRelay goal_msg;
					Net::MsgDesc msg_desc;

					pGoal->MarkBeatenBy( player->m_Skater->GetID());

					goal_msg.m_GoalId = msg->m_GoalId;
					goal_msg.m_ObjId = player->m_Skater->GetID();

					msg_desc.m_Data = &goal_msg;
					msg_desc.m_Length = sizeof( MsgBeatGoalRelay );
					msg_desc.m_Id = MSG_ID_BEAT_GOAL;
					msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
					msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
					for( send_player = gamenet_man->FirstPlayerInfo( sh ); send_player; 
							send_player = gamenet_man->NextPlayerInfo( sh ))
					{
						context->m_App->EnqueueMessage( send_player->GetConnHandle(), &msg_desc );
					}
				}
			}
		}
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Client beat a goalattack goal						  	  	  */
/*                                                  			  */
/******************************************************************/

int Manager::s_handle_beat_goal_relay( Net::MsgHandlerContext* context )
{
	MsgBeatGoalRelay *msg;
	Manager* gamenet_man;
	Game::CGoalManager* pGoalManager;
	Game::CGoal* pGoal, *pCurrentGoal;
	int current_goal_index;
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
    
	gamenet_man = (Manager *) context->m_Data;
	msg = (MsgBeatGoalRelay*) context->m_Msg;
    
	pGoalManager = Game::GetGoalManager();
	pGoal = pGoalManager->GetGoal( msg->m_GoalId );
	pCurrentGoal = NULL;
	current_goal_index = pGoalManager->GetActiveGoal( true );
	if( current_goal_index >= 0 )
	{
		pCurrentGoal = pGoalManager->GetGoalByIndex( current_goal_index );
	}

	if( pGoal )
	{
		PlayerInfo* local_player, *player;

		player = gamenet_man->GetPlayerByObjectID( msg->m_ObjId );
		local_player = gamenet_man->GetLocalPlayer();
		Dbg_Assert( player );

		pGoal->MarkBeatenByTeam( player->m_Team );

		if( player->m_Skater )
		{
			pGoal->MarkBeatenBy( player->m_Skater->GetID());

			// If it's not us, it must be a teammate
			if( local_player == player )
			{
				int num_goals, num_beaten;
				Script::CStruct* pParams;
					
				num_goals = pGoalManager->GetNumSelectedGoals();
				num_beaten = pGoalManager->NumGoalsBeaten();
		
				if( num_goals > num_beaten )
				{
					pParams = new Script::CStruct;
					pParams->AddInteger( "NumGoalsLeft", num_goals - num_beaten );
					Script::RunScript( "goal_attack_completed_goal", pParams );
					delete pParams;
				}

				pGoal->mp_goalPed->DestroyGoalPed();
			}
			else
			{
				int num_goals, num_beaten;
				Script::CStruct* pParams;
				bool in_same_goal;
					
				in_same_goal = 	( pCurrentGoal != NULL ) && 
								( pGoal->GetRootGoalId() == pCurrentGoal->GetRootGoalId());
				if( ( skate_mod->GetGameMode()->IsTeamGame()) && 
					( local_player->m_Team == player->m_Team ))
				{
					if( in_same_goal )
					{
						if( pGoal->GetChildren()->m_relative != 0 )
						{
							Game::CGoal* pChildGoal = pGoal;	
							
							// get the root node
							Game::CGoalLink* p_child = pGoal->GetChildren();
							while ( p_child && p_child->m_relative != 0 )
							{
								pChildGoal = pGoalManager->GetGoal( p_child->m_relative );
								p_child = pChildGoal->GetChildren();
							}
						
							pChildGoal->Win();
							if( pChildGoal != pCurrentGoal )
							{
								pCurrentGoal->Deactivate();
							}
						}
						else
						{
							pGoal->Win();
						}

						pCurrentGoal->mp_goalPed->DestroyGoalPed();
					}
					else
					{
						pGoal->mp_goalPed->DestroyGoalPed();
					}
					
					pGoal->MarkBeaten();
	
					num_goals = pGoalManager->GetNumSelectedGoals();
					num_beaten = pGoalManager->NumGoalsBeaten();
			
					if( num_goals > num_beaten )
					{
						const char* p_view_goals_text = NULL;
	
						pGoal->GetViewGoalsText( &p_view_goals_text );
						pParams = new Script::CStruct;
						pParams->AddInteger( CRCD(0x684a8396, "NumGoalsLeft"), num_goals - num_beaten );
						pParams->AddString( CRCD(0x9196d920, "PlayerName"), player->m_Name );
						pParams->AddString( CRCD(0xb8a88b50, "GoalText"), p_view_goals_text );
						Script::RunScript( CRCD(0xf4748e47, "goal_attack_completed_goal_other_same_team"), pParams );
						delete pParams;
					}

					
				}
			}
		}
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Client started a goalattack goal. 							  */
/* Relay it to all clients			  			  				  */
/******************************************************************/

int Manager::s_handle_started_goal( Net::MsgHandlerContext* context )
{
	MsgStartedGoal *msg;
	Manager* gamenet_man;
	Game::CGoalManager* pGoalManager;
	Game::CGoal* pGoal;    
		
	gamenet_man = (Manager *) context->m_Data;
	msg = (MsgStartedGoal*) context->m_Msg;
    
	// Make sure this message pertains to the current game
	if( msg->m_GameId != gamenet_man->GetNetworkGameId())
	{
		return Net::HANDLER_MSG_DONE;
	}

	pGoalManager = Game::GetGoalManager();
	pGoal = pGoalManager->GetGoal( msg->m_GoalId );
	if( pGoal )
	{
		PlayerInfo* player;

		player = gamenet_man->GetPlayerByConnection( context->m_Conn );
		Dbg_Assert( player );

		if( player->m_Skater )
		{   
			PlayerInfo* other_player;
			Lst::Search< PlayerInfo > sh;
			MsgStartedGoalRelay goal_msg;
			Net::MsgDesc msg_desc;

			goal_msg.m_GoalId = msg->m_GoalId;
			goal_msg.m_ObjId = player->m_Skater->GetID();

			msg_desc.m_Data = &goal_msg;
			msg_desc.m_Length = sizeof( MsgStartedGoalRelay );
			msg_desc.m_Id = MSG_ID_STARTED_GOAL;
			/* Dan: used to only sent to teammates
			for( team_player = gamenet_man->FirstPlayerInfo( sh ); team_player; 
					team_player = gamenet_man->NextPlayerInfo( sh ))
			{
				if( player == team_player )
				{
					continue;
				}
				if( player->m_Team == team_player->m_Team )
				{
					context->m_App->EnqueueMessage( team_player->GetConnHandle(), &msg_desc );
				}
			}
			*/
			for( other_player = gamenet_man->FirstPlayerInfo( sh ); other_player; 
					other_player = gamenet_man->NextPlayerInfo( sh ))
			{
				if( player == other_player )
				{
					continue;
				}
				context->m_App->EnqueueMessage( other_player->GetConnHandle(), &msg_desc );
			}
		}
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Client started a goalattack goal						  	  	  */
/*                                                  			  */
/******************************************************************/

int Manager::s_handle_started_goal_relay( Net::MsgHandlerContext* context )
{
	MsgStartedGoalRelay *msg;
	Manager* gamenet_man;
	Game::CGoalManager* pGoalManager;
	Game::CGoal* pGoal;
    
	gamenet_man = (Manager *) context->m_Data;
	msg = (MsgStartedGoalRelay*) context->m_Msg;
    
	pGoalManager = Game::GetGoalManager();
	pGoal = pGoalManager->GetGoal( msg->m_GoalId );
	if( pGoal )
	{
		Script::CStruct* pParams;
		const char* p_view_goals_text = NULL;
		PlayerInfo* player;

		player = gamenet_man->GetPlayerByObjectID( msg->m_ObjId );
		Dbg_Assert( player );

		if (gamenet_man->GetLocalPlayer()->m_Team == player->m_Team)
		{
			pGoal->GetViewGoalsText( &p_view_goals_text );
			pParams = new Script::CStruct;
			pParams->AddString( CRCD(0x9196d920, "PlayerName"), player->m_Name );
			pParams->AddString( CRCD(0xb8a88b50, "GoalText"), p_view_goals_text );
			Script::RunScript( CRCD(0xa77758de, "goal_attack_started_goal_other_same_team"), pParams );
			delete pParams;
		}
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Toggle this proset						  	  				  */
/*                                                  			  */
/******************************************************************/

int Manager::s_handle_toggle_proset( Net::MsgHandlerContext* context )
{
	MsgToggleProSet* msg;
	Script::CStruct* pParams;
	Manager* gamenet_man;

	msg = (MsgToggleProSet*) context->m_Msg;
	gamenet_man = (Manager *) context->m_Data;
	pParams = new Script::CStruct;

	Dbg_Printf( "********************* handle toggle proset" );

	Script::RunScript( "toggle_proset_flag", Script::GetStructure( msg->m_ParamId, Script::ASSERT ));
	Script::RunScript( "toggle_geo_nomenu", Script::GetStructure( msg->m_ParamId, Script::ASSERT ));

	if( gamenet_man->OnServer() == false )
	{
		gamenet_man->m_proset_flags.Toggle( msg->m_Bit );
	}

	delete pParams;

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Someone has sent us a chat message						  	  */
/*                                                  			  */
/******************************************************************/

int Manager::s_handle_chat( Net::MsgHandlerContext* context )
{
	MsgChat* chat_msg;
	Script::CStruct* p_params;
	char final_msg[256];
	
	chat_msg = (MsgChat*) context->m_Msg;
	p_params = new Script::CStruct;	
	sprintf( final_msg, "\\c%i%s\\c0 : %s", ( chat_msg->m_ObjId + 2 ), chat_msg->m_Name, chat_msg->m_ChatMsg );
	p_params->AddString( "text", final_msg );
	Script::RunScript("create_console_message", p_params );
	delete p_params;

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Server has changed the number of teams						  */
/*                                                  			  */
/******************************************************************/

int	Manager::s_handle_set_num_teams( Net::MsgHandlerContext* context )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Manager* gamenet_man;
	MsgByteInfo* msg;
	int num_teams;
    
	gamenet_man = (Manager *) context->m_Data;
	msg = (MsgByteInfo*) context->m_Msg;
	num_teams = msg->m_Data;

	if( ( num_teams > 0 ) &&
		( !skate_mod->GetGameMode()->IsTeamGame()))
	{
		skate_mod->GetGameMode()->SetNumTeams( num_teams );

		Script::RunScript( "ChooseTeamMessage" );
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

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Server requests client's cheat checksum				  		  */
/*                                                  			  */
/******************************************************************/

int	Manager::s_handle_cheat_checksum_request( Net::MsgHandlerContext* context )
{
	MsgCheatChecksum* msg;
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Net::MsgDesc msg_desc;

	msg = (MsgCheatChecksum*) context->m_Msg;
	msg->m_ClientChecksum = skate_mod->GetCheatChecksum();

	msg_desc.m_Id = MSG_ID_CHEAT_CHECKSUM_RESPONSE;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	msg_desc.m_Data = msg;
	msg_desc.m_Length = sizeof( MsgCheatChecksum );

	context->m_App->EnqueueMessageToServer( &msg_desc );

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Server has sent a GameSpy stats challenge					  */
/*                                                  			  */
/******************************************************************/

int	Manager::s_handle_challenge( Net::MsgHandlerContext* context )
{
#ifdef __PLAT_NGPS__
	Net::MsgDesc msg_desc;
	char* challenge;
	char response[33];
	Prefs::Preferences* pPreferences;
	Manager* gamenet_man;
	
	gamenet_man = (Manager*) context->m_Data;

	pPreferences = gamenet_man->GetNetworkPreferences();
	const char* password = pPreferences->GetPreferenceString( Script::GenerateCRC( "profile_password" ), Script::GenerateCRC("ui_string") );

	challenge = context->m_Msg;
	gamenet_man->mpStatsMan->GenerateAuthResponse( challenge, (char*) password, response );

	msg_desc.m_Id = MSG_ID_CHALLENGE_RESPONSE;
	msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
	msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
	msg_desc.m_Data = response;
	msg_desc.m_Length = 33;

	context->m_App->EnqueueMessageToServer( &msg_desc );
#endif
	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Client has answered the GameSpy stats challenge				  */
/*                                                  			  */
/******************************************************************/

int	Manager::s_handle_challenge_response( Net::MsgHandlerContext* context )
{
#ifdef __PLAT_NGPS__
	PlayerInfo* player;
	Manager* gamenet_man;
	
	gamenet_man = (Manager*) context->m_Data;

	player = gamenet_man->GetPlayerByConnection( context->m_Conn );
	gamenet_man->mpStatsMan->AuthorizePlayer( player->m_Skater->GetID(), context->m_Msg );
#endif

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Client has requested that we re-send level data to them		  */
/*                                                  			  */
/******************************************************************/

int Manager::s_handle_level_data_request( Net::MsgHandlerContext* context )
{
	Net::Server* server;
	Manager* gamenet_man;

	server = (Net::Server*) context->m_App;
	gamenet_man = (Manager*) context->m_Data;

	if( context->m_MsgId == MSG_ID_REQUEST_RAILS_DATA )
	{
		Dbg_Printf( "***** Client requested rails data\n" );
		server->StreamMessage( context->m_Conn->GetHandle(), MSG_ID_RAIL_DATA, Obj::GetRailEditor()->GetCompressedRailsBufferSize(), 
							   Obj::GetRailEditor()->GetCompressedRailsBuffer(), "rail data", vSEQ_GROUP_PLAYER_MSGS,
							   false, true );
							   
	}
	else if( context->m_MsgId == MSG_ID_REQUEST_GOALS_DATA )
	{
		Dbg_Printf( "***** Client requested goals data\n" );
		if( gamenet_man->UsingCreatedGoals())
		{
			server->StreamMessage( context->m_Conn->GetHandle(), MSG_ID_GOALS_DATA, gamenet_man->GetGoalsDataSize(),
								   gamenet_man->GetGoalsData(), "goals data", vSEQ_GROUP_PLAYER_MSGS, false, true );
		}
	}

	return Net::HANDLER_CONTINUE;
}

/******************************************************************/
/* Client has responded to the cheat checksum request			  */
/*                                                  			  */
/******************************************************************/

int	Manager::s_handle_cheat_checksum_response( Net::MsgHandlerContext* context )
{
	MsgCheatChecksum* msg;
	PlayerInfo* player;
	Manager* gamenet_man;

	gamenet_man = (Manager*) context->m_Data;
	player = gamenet_man->GetPlayerByConnection( context->m_Conn );
	msg = (MsgCheatChecksum*) context->m_Msg;
	if(( msg->m_ServerChecksum ^ 0xDEADFACE ) != msg->m_ClientChecksum )
	{
		Dbg_Printf( "***** CHEATING! 0x%x  0x%x\n", msg->m_ServerChecksum ^ 0xDEADFACE, msg->m_ClientChecksum );
		if( gamenet_man->HasCheatingOccurred() == false )
		{
			Script::CStruct* p_params;
	
			p_params = new Script::CStruct;
			p_params->AddString( CRCD(0xa4b08520,"String0"), player->m_Name );
			Script::RunScript( CRCD(0x62ddbe0a,"notify_client_cheating"), p_params );
			delete p_params;
			
			gamenet_man->CheatingOccured();
		}
	}

	return Net::HANDLER_CONTINUE;
}

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::s_change_level_code( const Tsk::Task< Manager >& task )
{
	Manager& man = task.GetData();
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;

	if(( Tmr::GetTime() < s_time_change_level ))
	{
		return;
	}

	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	skate_mod->ChangeLevel( man.m_level_id );

	// Clear the king of the hill
	if(( player = man.GetKingOfTheHill()))
	{
		player->MarkAsKing( false );
	}

	for( player = man.FirstPlayerInfo( sh, true ); player; player = man.NextPlayerInfo( sh, true ))
	{
		player->ClearCTFState();
	}

	//man.RespondToReadyQuery();
	man.m_change_level_task->Remove();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		Manager::ok_to_join( int& reason, MsgConnectInfo* connect_msg, Net::Conn* conn )
{
	char* password;
	
	if( m_server->IsConnectionBanned( conn ))
	{
		reason = JOIN_REFUSED_ID_BANNED;
		return false;
	}
	if( connect_msg->m_Observer )
	{
		if( GetNumObservers() >= GetMaxObservers())
		{
			reason = JOIN_REFUSED_ID_FULL_OBSERVERS;
			return false;
		}
	}
	else
	{
		if( GetNumPlayers() >= GetMaxPlayers())
		{
			reason = JOIN_REFUSED_ID_FULL;
			return false;
		}
	}
		
	// Version check
	if( connect_msg->m_Version != vVERSION_NUMBER )
	{
		reason = JOIN_REFUSED_ID_VERSION;
		Dbg_Printf( "Client had the wrong version. Dropping connection\n" );
		return false;
	}
		
	// Game-in-progress check
	if( !connect_msg->m_WillingToWait && !connect_msg->m_Observer )
	{
		Mdl::Skate * skate_mod = Mdl::Skate::Instance();

		if(( skate_mod->GetGameMode()->GetNameChecksum() != Script::GenerateCRC( "netlobby" )) &&
		   ( GameIsOver() == false ))
		{
			reason = JOIN_REFUSED_ID_IN_PROGRESS;
			return false;
		}
	}

	// Check if we're password-protected. If so, compare passwords
	password = GetPassword();
	if( password[0] != '\0' )
	{
		Dbg_Printf( "Comparing passwords %s and %s\n", password, connect_msg->m_Password );
		if( strcmp( password, connect_msg->m_Password ))
		{          
			reason = JOIN_REFUSED_ID_PW;

			Dbg_Printf( "Client gave the wrong password. Dropping connection\n" );
			return false;
		}
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace GameNet




