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
**	File name:		Lobby.cpp												**
**																			**
**	Created by:		04/4/02	-	spg											**
**																			**
**	Description:	Gamespy peer lobby implementation						**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/thread.h>
#include <core/singleton.h>

#include <gel/mainloop.h>

#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/net/server/netserv.h>
#include <gel/net/client/netclnt.h>

#include <sk/gamenet/gamenet.h>
#include <sk/gamenet/lobby.h>
#include <sk/gamenet/ngps/p_buddy.h>
#include <sk/gamenet/ngps/p_stats.h>
#include <sk/modules/skate/gamemode.h>                                       
#include <sk/scripting/cfuncs.h>

#include <peer/peer.h>


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

#define vPOST_SERVER_TIMEOUT		Tmr::Seconds( 90 )
#define vPOST_SERVER_RETRY_TIME		Tmr::Seconds( 5 )
#define vGAME_VERSION				5

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

enum
{
	vGAMESPY_NO_DATA,
	vGAMESPY_QUERY_DATA,
	vGAMESPY_NAT_DATA,
};

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

static Tmr::Time	s_last_refresh_time = 0;
static Tmr::Time	s_time_of_connection = 0;
static	int		s_gamespy_data_type = vGAMESPY_NO_DATA;
static	char	s_gamespy_parse_data[2048];
static	int		s_gamespy_parse_data_len = 0;
static	struct sockaddr s_gamespy_sender;
static	bool	s_got_gamespy_callback;
static	Tmr::Time	s_game_start_time;
static	Tmr::Time	s_last_post_time;
static	bool		s_notified_user_not_connected;

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::add_lobby_to_menu( LobbyInfo* lobby, int index )
{
	Script::CStruct* p_item_params;
	char lobby_title[64];
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	if( lobby->m_Full )
	{
		sprintf( lobby_title, "%s - %d/%d (%s)", lobby->m_Name, lobby->m_NumServers, lobby->m_MaxServers, Script::GetString( "net_lobby_full" ));
	}
	else
	{
		sprintf( lobby_title, "%s - %d/%d", lobby->m_Name, lobby->m_NumServers, lobby->m_MaxServers );
	}

	p_item_params = new Script::CStruct;	
	p_item_params->AddString( "text", lobby_title );
	if( lobby->m_Full || lobby->m_OffLimits )
	{
		p_item_params->AddChecksum( NONAME, CRCD( 0x1d944426, "not_focusable" ) );
		p_item_params->AddChecksum( "pad_choose_script",Script::GenerateCRC("selected_lobby_full"));
	}
	else
	{
		p_item_params->AddChecksum( "pad_choose_script",Script::GenerateCRC("choose_selected_lobby"));
		p_item_params->AddChecksum( "focus_script",Script::GenerateCRC("regions_menu_set_focus"));
	}

	// create the parameters that are passed to the X script
	Script::CStruct *p_script_params= new Script::CStruct;
	p_script_params->AddInteger( "index", index );	
	
	if( !( lobby->m_Full || lobby->m_OffLimits ))
	{
		p_item_params->AddStructure("pad_choose_params",p_script_params);			
	}

	Script::RunScript("regions_menu_add_item",p_item_params);
	delete p_item_params;
	delete p_script_params;

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::add_player_to_menu( LobbyPlayerInfo* player )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Script::CStruct *p_script_params;
	Script::CStruct *p_unfocus_params;
	Script::CStruct* p_item_params;
	Script::CArray* p_colors;
	int rank;
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	p_script_params = new Script::CStruct;
	p_item_params = new Script::CStruct;	
	p_colors = new Script::CArray;
	p_unfocus_params = new Script::CStruct;

	p_item_params->AddString( "text", PlayerName( player->m_Name ));
	p_colors->SetSizeAndType( 4, ESYMBOLTYPE_INTEGER );

	if( ( player->m_Profile ) && 
		( gamenet_man->mpBuddyMan->IsLoggedIn()))
	{           
		//Dbg_Printf( "Adding Player %s of %d to list\n", player->m_Name, player->m_Profile );
		
		// If it's me (or a buddy), color it differently and don't let me add myself to my own buddy list
		if( player->m_Profile == gamenet_man->mpBuddyMan->GetProfile() || gamenet_man->mpBuddyMan->IsAlreadyMyBuddy( player->m_Profile ))
		{
			if( gamenet_man->mpBuddyMan->GetProfile() == player->m_Profile )
			{
				p_colors->SetInteger( 0, 0 );
				p_colors->SetInteger( 1, 0 );
				p_colors->SetInteger( 2, 128 );
				p_colors->SetInteger( 3, 128 );

				p_item_params->AddChecksum( "pad_choose_script",Script::GenerateCRC("cant_add_self_to_buddy_prompt"));
			}
			else
			{
				p_colors->SetInteger( 0, 128 );
				p_colors->SetInteger( 1, 128 );
				p_colors->SetInteger( 2, 0 );
				p_colors->SetInteger( 3, 128 );

				p_item_params->AddChecksum( "pad_choose_script",Script::GenerateCRC("already_buddy_prompt"));
			}
		}
		else
		{
			p_colors->SetInteger( 0, 0 );
			p_colors->SetInteger( 1, 128 );
			p_colors->SetInteger( 2, 0 );
			p_colors->SetInteger( 3, 128 );

			p_script_params->AddInteger( "profile", player->m_Profile );	
			p_script_params->AddString( "nick", player->m_Name );	
			p_script_params->AddString( "net_name", PlayerName( player->m_Name ));  
			p_item_params->AddStructure( "pad_choose_params", p_script_params );

			if( gamenet_man->mpBuddyMan->NumBuddies() < vMAX_BUDDIES )
			{
				p_item_params->AddChecksum( "pad_choose_script", Script::GenerateCRC( "add_buddy_prompt" ));
			}
			else
			{
				p_item_params->AddChecksum( "pad_choose_script", Script::GenerateCRC( "cant_add_buddy_prompt_3" ));
			}
		}
	}
	else
	{
		//Dbg_Printf( "Adding Player %s, no profile\n", player->m_Name );

		p_colors->SetInteger( 0, 128 );
		p_colors->SetInteger( 1, 128 );
		p_colors->SetInteger( 2, 128 );
		p_colors->SetInteger( 3, 128 );
		
		if( gamenet_man->mpBuddyMan->IsLoggedIn())
		{
			p_item_params->AddChecksum( "pad_choose_script",Script::GenerateCRC("cant_add_buddy_prompt_1"));
		}
		else
		{
			p_item_params->AddChecksum( "pad_choose_script",Script::GenerateCRC("cant_add_buddy_prompt_2"));
		}
		
	}
	
	rank = (int) (((float) player->m_Rating / (float) vMAX_RATING ) * (float) vMAX_RANK );
	p_item_params->AddInteger( "rank", rank );
	p_item_params->AddChecksum( "id", Script::GenerateCRC( player->m_Name ));
	p_item_params->AddChecksum( "parent_menu_id", Script::GenerateCRC( "lobby_player_list_menu" ));
	p_item_params->AddFloat( "scale", 1.1f );

	p_unfocus_params->AddArray( "rgba", p_colors );
	p_item_params->AddArray( "rgba", p_colors );
	p_item_params->AddStructure( "unfocus_params", p_unfocus_params );
	//Script::RunScript("make_text_sub_menu_item", p_item_params );
	
	Script::RunScript("player_list_add_item", p_item_params );
	
	delete p_colors;
	delete p_script_params;
	delete p_item_params;
	delete p_unfocus_params;
	

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::refresh_player_list( void )
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	clear_player_list();
	Script::RunScript( "destroy_lobby_user_list_children" );
	peerEnumPlayers( m_peer, GroupRoom, s_enum_players_callback, this );
	
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::clear_player_list( void )
{
	Lst::Search< LobbyPlayerInfo > sh;
	LobbyPlayerInfo* player, *next;

	for( player = sh.FirstItem( m_players ); player; player = next )
	{
		next = sh.NextItem();
		delete player;
	}

	Script::RunScript( "update_lobby_player_list" );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::fill_player_list( void )
{
	Lst::Search< LobbyPlayerInfo > sh;
	LobbyPlayerInfo* player;

	Script::RunScript( "destroy_lobby_user_list_children" );

    for( player = sh.FirstItem( m_players ); player; player = sh.NextItem())
	{
		if( player->m_GotInfo )
		{
			add_player_to_menu( player );
		}
	}
	
	Script::RunScript( "update_lobby_player_list" );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::add_buddy_to_menu( LobbyPlayerInfo* player )
{
	Script::CStruct* p_item_params;
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	//Dbg_Printf( "Adding %s of %d to the list\n", PlayerName( player->m_Name ), player->m_Profile );
	p_item_params = new Script::CStruct;	
	p_item_params->AddString( "text", PlayerName( player->m_Name ));
	p_item_params->AddChecksum( "pad_choose_script",Script::GenerateCRC("lobby_add_buddy"));
	p_item_params->AddChecksum( "parent_menu_id", Script::GenerateCRC( "lobby_buddy_list_menu" ));

	// create the parameters that are passed to the X script
	Script::CStruct *p_script_params= new Script::CStruct;
	p_script_params->AddInteger( "profile", player->m_Profile );	
	p_script_params->AddString( "nick", player->m_Name );	
	p_script_params->AddString( "net_name", PlayerName( player->m_Name ));  
	p_item_params->AddStructure( "pad_choose_params", p_script_params );

	Script::RunScript("make_text_sub_menu_item",p_item_params);

	delete p_item_params;
	delete p_script_params;

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::fill_prospective_buddy_list( void )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Lst::Search< LobbyPlayerInfo > sh;
	LobbyPlayerInfo* player;

	Script::RunScript( "destroy_lobby_buddy_list_children" );

	for( player = sh.FirstItem( m_players ); player; player = sh.NextItem())
	{
		if( player->m_Profile == gamenet_man->mpBuddyMan->GetProfile())
		{
			continue;
		}

		if(	( player->m_Profile > 0 ) && 
			( gamenet_man->mpBuddyMan->IsAlreadyMyBuddy( player->m_Profile ) == false ))
		{
			add_buddy_to_menu( player );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::s_threaded_peer_connect( LobbyMan* lobby_man )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

	// Register this thread with the sockets API
	sockAPIregthr();

	Prefs::Preferences* prefs;
	char lobby_name[128];
	
	prefs = gamenet_man->GetNetworkPreferences();
    const char* network_name = prefs->GetPreferenceString( Script::GenerateCRC( "network_id" ), Script::GenerateCRC( "ui_string" ));
    
	// Use our unique Gamespy profile ID as our "unique number", if one is available
	if( gamenet_man->mpBuddyMan->IsLoggedIn())
	{
		sprintf( lobby_name, "a%d_%s", gamenet_man->mpBuddyMan->GetProfile(), network_name );
	}
	else
	{
		sprintf( lobby_name, "a%d_%s", (int) Tmr::GetTime(), network_name );
	}

	lobby_man->m_connection_in_progress = true;
    peerConnect( lobby_man->m_peer, lobby_name, gamenet_man->mpBuddyMan->GetProfile(), s_nick_error_callback, s_connect_callback, lobby_man, false );

	// Deregister this thread with the sockets API
	sockAPIderegthr();
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void LobbyMan::s_disconnected_callback( PEER peer, const char * reason, void * param )
{
	LobbyMan* man = (LobbyMan*) param;
	Tmr::Time elapsed_time;

	elapsed_time = Tmr::GetTime() - s_time_of_connection;
	printf( "Disconnected at %d after %d ms. Reason: %s\n", (int) Tmr::GetTime(), (int) elapsed_time, reason );

	if( man->m_connection_in_progress )
	{
		printf( "******* Connect Callback FAILED!\n" );
		Script::RunScript( "create_gamespy_connection_failure_dialog" );
		man->m_connection_in_progress = false;
	}
	else if( man->m_expecting_disconnect == false )
	{
		printf( "Wasn't expecting discon.  Attempting to stop hosting game\n" );
		Script::RunScript( "lost_connection_to_gamespy" );
	}
	else
	{
		printf( "Was expecting discon. Ignore\n" );
	}

	man->m_expecting_disconnect = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void LobbyMan::s_room_message_callback( PEER peer, RoomType roomType, const char * nick, const char * message,
										MessageType messageType, void * param )
{
	Script::CStruct* p_params;
	char msg[1024];

	LobbyMan* lobby_man = (LobbyMan*) param;


	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());
	sprintf( msg, "%s: %s", lobby_man->PlayerName((char*) nick ), message );
	p_params = new Script::CStruct;	
	p_params->AddString( "text", msg );
	Script::RunScript("create_console_message", p_params );
	delete p_params;
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void LobbyMan::s_player_message_callback( PEER peer, const char * nick, const char * message, MessageType messageType,
												void * param )
{
	//printf( "Player Message. Nick: %s Message: %s\n", nick, message );
	//LobbyMan* man = (LobbyMan*) param;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void LobbyMan::s_player_proile_id_callback( PEER peer, PEERBool success,  const char * nick, int profileID, void * param )
{
	Lst::Search< LobbyPlayerInfo > sh;
	LobbyPlayerInfo* player;
	LobbyMan* lobby_man = (LobbyMan*) param;

	//Dbg_Printf( "Got a player profile id callback on %s : %d\n", nick, profileID );
	for( player = sh.FirstItem( lobby_man->m_players ); player; player = sh.NextItem())
	{
		if( stricmp( player->m_Name, nick ) == 0 )
		{
			player->m_Profile = profileID;
			break;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void LobbyMan::s_player_joined_callback( PEER peer, RoomType roomType, const char * nick, void * param )
{
	Script::CStruct* p_params;
	char msg[1024];
	Lst::Search< LobbyPlayerInfo > sh;
	LobbyPlayerInfo* player;
	
	LobbyMan* lobby_man = (LobbyMan*) param;

	//printf( "%s joined the room\n", nick );

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	for( player = sh.FirstItem( lobby_man->m_players ); player; player = sh.NextItem())
	{
		if( stricmp( player->m_Name, nick ) == 0 )
		{
			peerGetPlayerInfoNoWait( peer, nick, NULL, &player->m_Profile );
			Mem::Manager::sHandle().PopContext();
			player->m_GotInfo = true;
			return;	// already have this player
		}
	}

	player = new LobbyPlayerInfo;
	strcpy( player->m_Name, nick );
	peerGetPlayerInfoNoWait( peer, nick, NULL, &player->m_Profile );
	player->m_GotInfo = true;

	lobby_man->m_players.AddToTail( player );

	Script::RunScript( "prepare_lobby_user_list_for_new_children" );
	lobby_man->add_player_to_menu( player );
	Script::RunScript( "update_lobby_player_list" );
		
	sprintf( msg, "%s %s", lobby_man->PlayerName((char*) nick ), Script::GetString( "lobby_status_joined" ));

	p_params = new Script::CStruct;	
	p_params->AddString( "text", msg );
    p_params->AddChecksum( "join", 0 );
	Script::RunScript("create_console_message", p_params );
	delete p_params;

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void LobbyMan::s_player_info_callback( PEER peer, RoomType roomType, const char * nick, unsigned int IP, int profileID,  
                                                void * param )
{
	Lst::Search< LobbyPlayerInfo > sh;
	LobbyPlayerInfo* player;
	LobbyMan* lobby_man = (LobbyMan*) param;
	
	if( nick == NULL )
	{
		return;
	}
	
	//Dbg_Printf( "Got info on %s : %d\n", nick, profileID );

	for( player = sh.FirstItem( lobby_man->m_players ); player; player = sh.NextItem())
	{
		if( stricmp( player->m_Name, nick ) == 0 )
		{
			// If we don't already have this player's profile
			if( player->m_Profile == 0 )
			{
				player->m_Profile = profileID;
			}
			player->m_GotInfo = true;
			lobby_man->add_player_to_menu( player );
            Script::RunScript( "update_lobby_player_list" );
			break;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void LobbyMan::s_player_left_callback( PEER peer, RoomType roomType, const char * nick, const char * reason, 
												void * param )
{
	Script::CStruct* p_params;
	char msg[1024];
	Lst::Search< LobbyPlayerInfo > sh;
	LobbyPlayerInfo* player;
	LobbyMan* lobby_man = (LobbyMan*) param;
	
	//printf( "%s left the room\n", nick );

	for( player = sh.FirstItem( lobby_man->m_players ); player; player = sh.NextItem())
	{
		if( stricmp( player->m_Name, nick ) == 0 )
		{
			p_params = new Script::CStruct;

			p_params->AddChecksum( "user_id", Script::GenerateCRC( nick ));
			Script::RunScript( "destroy_lobby_user", p_params );

			delete player;
			delete p_params;

			Script::RunScript( "update_lobby_player_list" );
			break;
		}
	}

	sprintf( msg, "%s %s", lobby_man->PlayerName((char*) nick ), Script::GetString( "lobby_status_left" ));

	p_params = new Script::CStruct;	
	p_params->AddString( "text", msg );
    p_params->AddChecksum( "left", 0 );
	Script::RunScript("create_console_message", p_params );
	delete p_params;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void 	LobbyMan::s_get_room_keys_callback( PEER peer, PEERBool success, RoomType roomType, 
											const char * nick, int num, char ** keys, char ** values, 
											void * param )
{
	int i;
	LobbyMan* lobby_man = (LobbyMan*) param;

	for( i = 0; i < num; i++ )
	{
		//Dbg_Printf( "Got Key: %s\n", keys[i] );
	}
	if( num > 0 )
	{
		int rank;
		Script::CStruct* p_item_params;
		Lst::Search< LobbyPlayerInfo > sh;
		LobbyPlayerInfo* player;

		if(( keys[0] != NULL )&& stricmp( keys[0], "b_rating" ) == 0 )
		{
			for( player = sh.FirstItem( lobby_man->m_players ); player; player = sh.NextItem())
			{
				if( stricmp( player->m_Name, nick ) == 0 )
				{
					Dbg_Printf( "Got rating %d for %s\n", player->m_Rating, nick );
					player->m_Rating = atoi( values[0] );
					rank = (int) (((float) player->m_Rating / (float) vMAX_RATING ) * (float) vMAX_RANK );
			
					p_item_params = new Script::CStruct;
					p_item_params->AddChecksum( "id", Script::GenerateCRC( player->m_Name ));
					p_item_params->AddInteger( "rank", rank );
					Script::RunScript( "player_list_update_rank", p_item_params );
					delete p_item_params;
				}
			}
		}
	}
	
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	LobbyMan::s_enum_players_callback( PEER peer, PEERBool success, RoomType roomType, int index, const char * nick,  // The nick of the player.
												int flags, void * param )
{
	LobbyMan* lobby_man = (LobbyMan*) param;
	Lst::Search< LobbyPlayerInfo > sh;
	LobbyPlayerInfo* player;

	if( success == false )
	{
		//printf( "Failed to enum players\n" );
		return;
	}

	//Dbg_Printf( "Enumerating players\n" );
	if( index == -1 )
	{
		// List is finished. 
		//lobby_man->fill_player_list();
        return;
	}
	
	for( player = sh.FirstItem( lobby_man->m_players ); player; player = sh.NextItem())
	{
		if( stricmp( player->m_Name, nick ) == 0 )
		{
			return;	// already have this player in our list
		}
	}
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

	player = new LobbyPlayerInfo;
	strcpy( player->m_Name, nick );
	//Dbg_Printf( "Before getting player info on %s : %d\n", nick, player->m_Profile );
	if( peerGetPlayerInfoNoWait( peer, nick, NULL, &player->m_Profile ))
	{
		//Dbg_Printf( "***** Got Player Info for %s : %d\n", nick, player->m_Profile );
	}
	else
	{
		//Dbg_Printf( "***** Failed to Get Player Info for %s : %d\n", nick, player->m_Profile );
	}

	lobby_man->m_players.AddToTail( player );

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void LobbyMan::s_new_player_list_callback( PEER peer, RoomType roomType, void * param )
{
	LobbyMan* lobby_man = (LobbyMan*) param;
    
	//printf( "New Player List\n" );

	lobby_man->refresh_player_list();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void LobbyMan::s_room_key_callback( PEER peer, RoomType roomType, const char * nick, 
									const char * key, const char * value, void * param )
{
	LobbyMan* lobby_man = (LobbyMan*) param;
	Lst::Search< LobbyPlayerInfo > sh;
	LobbyPlayerInfo* player;

	for( player = sh.FirstItem( lobby_man->m_players ); player; player = sh.NextItem())
	{
		if( stricmp( player->m_Name, nick ) == 0 )
		{
			int rank;
			Script::CStruct* p_item_params;

			player->m_Rating = atoi( value );
			rank = (int) (((float) player->m_Rating / (float) vMAX_RATING ) * (float) vMAX_RANK );

			Dbg_Printf( "Got room key for %s. Value: %d\n", nick, player->m_Rating );

			p_item_params = new Script::CStruct;
			p_item_params->AddChecksum( "id", Script::GenerateCRC( player->m_Name ));
			p_item_params->AddInteger( "rank", rank );
			Script::RunScript( "player_list_update_rank", p_item_params );
			delete p_item_params;
			break;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void LobbyMan::s_nat_negotiate_callback( PEER peer, int cookie, void * param )
{
    NegotiateError err;
	Manager * gamenet_man = Manager::Instance();
	Mlp::Manager * mlp_manager = Mlp::Manager::Instance();
	Net::Server* server;

	if( gamenet_man->mpLobbyMan->m_nat_count.InUse())
	{
		Dbg_Printf( "******* Ignoring new nat negotiation request\n" );
		return;
	}

	server = gamenet_man->GetServer();
	Dbg_Assert( server );
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	Dbg_Printf( "******* Beginning new nat negotiation\n" );
	err = NNBeginNegotiationWithSocket( server->GetSocket(), cookie, 0, s_nat_negotiate_progress,
										s_nat_negotiate_complete, (void*) cookie );
	if( err != ne_noerror )
	{
		Dbg_Printf( "******* NAT Neg Error: %d\n", err );
		NNCancel( cookie );
		NNThink();
		Mem::Manager::sHandle().PopContext();
		return;
	}
	
	if( gamenet_man->mpLobbyMan->m_nat_count.InUse() == false )
	{
		mlp_manager->AddLogicTask( *gamenet_man->mpLobbyMan->m_nat_negotiate_task );
	}

	gamenet_man->mpLobbyMan->m_nat_count.Acquire();

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void LobbyMan::s_server_key_callback( PEER peer, int key, qr2_buffer_t buffer, void * param )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	const char* server_name;
	Prefs::Preferences* pPreferences;
	Script::CScriptStructure* pStructure;
    char* password;
	
	//Dbg_Printf( "***** SERVER KEY CALLBACK\n" );
	pPreferences = gamenet_man->GetNetworkPreferences();

    switch(key)
	{
		case HOSTNAME_KEY:
			pStructure = pPreferences->GetPreference( Script::GenerateCRC("server_name") );
			pStructure->GetText( "ui_string", &server_name, true );
			//Dbg_Printf( "Server name is %s\n", server_name );
			qr2_buffer_add(buffer, server_name );
			break;
		case GAMENAME_KEY:
			qr2_buffer_add(buffer, "thps5ps2" );
			break;
		case GAMEMODE_KEY:
			qr2_buffer_add(buffer, gamenet_man->GetGameModeName());
			break;
		case HOSTPORT_KEY:
			qr2_buffer_add_int(buffer, vHOST_PORT );
			break;
		case MAPNAME_KEY:
			qr2_buffer_add(buffer, gamenet_man->GetLevelName());
			break;
		case GAMETYPE_KEY:
			qr2_buffer_add(buffer, gamenet_man->GetGameModeName());
			break;
		case TEAMPLAY_KEY:
			qr2_buffer_add_int(buffer, 0);
			break;
		case NUMPLAYERS_KEY:
			qr2_buffer_add_int(buffer, gamenet_man->GetNumPlayers());
			break;
		case MAXPLAYERS_KEY:
			qr2_buffer_add_int(buffer, gamenet_man->GetMaxPlayers());
			break;
		case PASSWORD_KEY:
			password = gamenet_man->GetPassword();
			if( password[0] == '\0' )
			{
				qr2_buffer_add_int(buffer, 0 );
			}
			else
			{
				qr2_buffer_add_int(buffer, 1 );
			}
			break;
			
		case NUMOBSERVERS_KEY:
			qr2_buffer_add_int(buffer, gamenet_man->GetNumObservers());
			break;
		case MAXOBSERVERS_KEY:
			qr2_buffer_add_int(buffer, gamenet_man->GetMaxObservers());
			break;
		case SKILLLEVEL_KEY:
			qr2_buffer_add_int(buffer, gamenet_man->GetSkillLevel());
			break;
		case STARTED_KEY:
			if( skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netlobby" ))
			{
				qr2_buffer_add_int(buffer, 0 );
			}
			else
			{
				qr2_buffer_add_int(buffer, 1 );
			}
			break;
		case HOSTED_MODE_KEY:
			qr2_buffer_add_int(buffer, gamenet_man->GetHostMode());
			break;
		case RANKED_KEY:
			qr2_buffer_add_int(buffer, gamenet_man->mpBuddyMan->IsLoggedIn());
			break;
		default:
			qr2_buffer_add(buffer, "");
			break;
	}

	s_got_gamespy_callback = true;
}

void LobbyMan::s_player_key_callback( PEER peer, int key, int index, qr2_buffer_t buffer, void * param )
{
	s_got_gamespy_callback = true;

	//Dbg_Printf( "***** PLAYER KEY %d CALLBACK : for player %d\n", key, index );
	switch(key)
	{
		case RATING__KEY:
		{
			GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
			Lst::Search< PlayerInfo > sh;
			PlayerInfo* player;
			Lst::Search< NewPlayerInfo > new_sh;
			NewPlayerInfo* np;
			int i;
			
			i = 0;
			for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; 
					player = gamenet_man->NextPlayerInfo( sh, true ))
			{
				if( !player->IsObserving() || player->IsPendingPlayer())
				{
					if( i == index )
					{
						//printf( "(2) Added %s's rating: %d to list\n", player->m_Name, player->m_Rating );
						qr2_buffer_add_int(buffer, player->m_Rating );
						return;
					}
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
					if( i == index )
					{
						//printf( "(2) Added %s's rating: %d to list\n", np->Name, np->Rating );
						qr2_buffer_add_int(buffer, np->Rating );
						return;
					}
					i++;
				}                 
			}
			
			//printf( "Added empty string to player list\n" );
			// If we're here, we don't have a player == index
            qr2_buffer_add(buffer, "");
			break;
		}
		case PLAYER__KEY:
		{
			GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
			Lst::Search< PlayerInfo > sh;
			PlayerInfo* player;
			Lst::Search< NewPlayerInfo > new_sh;
			NewPlayerInfo* np;
			int i;
			
			i = 0;
			for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; 
					player = gamenet_man->NextPlayerInfo( sh, true ))
			{
				if( !player->IsObserving() || player->IsPendingPlayer())
				{
					if( i == index )
					{
						//printf( "Added %s to player list\n", player->m_Name );
						qr2_buffer_add(buffer, player->m_Name);
						return;
					}
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
					if( i == index )
					{
						//printf( "(2) Added %s to player list\n", player->m_Name );
						qr2_buffer_add(buffer, np->Name );
						return;
					}
					i++;
				}                 
			}
			
			//printf( "Added empty string to player list\n" );
			// If we're here, we don't have a player == index
            qr2_buffer_add(buffer, "");
			break;
		}

		case PING__KEY:
			qr2_buffer_add(buffer, "");
			break;
	}
}

void LobbyMan::s_public_address_callback( PEER peer, unsigned int ip, unsigned short port, 
											   void * param )
{
	char location[1024];
	const char *server_name;
	Script::CScriptStructure* pStructure;
	Prefs::Preferences* pPreferences;
	Manager * gamenet_man = Manager::Instance();
	BuddyMan* buddy_man;
	LobbyMan* lobby_man;

	lobby_man = (LobbyMan*) param;
	buddy_man = gamenet_man->mpBuddyMan;

	Dbg_Printf( "***** Got public address: %s : %d\n", inet_ntoa(*(struct in_addr *) &ip ), port );
	if( gamenet_man->OnServer() && lobby_man->GetPeer())
	{
		gamenet_man->SetJoinPort( port );
		gamenet_man->SetJoinIP( ip );
		gamenet_man->SetJoinPrivateIP( peerGetPrivateIP( lobby_man->GetPeer()) );
		pPreferences = gamenet_man->GetNetworkPreferences();
		pStructure = pPreferences->GetPreference( Script::GenerateCRC("server_name") );
		pStructure->GetText( "ui_string", &server_name, true );
	
		sprintf( location, "%d:%d:%d:%s (%s)", ip, peerGetPrivateIP( lobby_man->GetPeer()), port, server_name, lobby_man->GetLobbyName());
		if( buddy_man->IsLoggedIn())
		{
			buddy_man->SetStatusAndLocation( GP_PLAYING, (char*) Script::GetString( "homie_status_hosting" ), location );
		}
	}	
}

void LobbyMan::s_add_error_callback( PEER peer, qr2_error_t error, char * errorString, void * param )
{
	Dbg_Printf( "***** Got an error (%d) adding the server: %s\n", (int) error, errorString );
}

int LobbyMan::s_key_count_callback( PEER peer, qr2_key_type type, void * param )
{
	//Dbg_Printf( "*** GOT KEY COUNT CALLBACK\n" );
	switch( type )
	{
		case key_server:
		{
			//Dbg_Printf( "*** Server Key Count Request: 15\n" );
			return 16;
		}
		case key_player:
		{
			GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
			//Dbg_Printf( "*** Player Key Count Request: %d\n", gamenet_man->GetNumPlayers() );
			return gamenet_man->GetNumPlayers();
		}
		case key_team:
		default:
			return 0;
	}

	return 0;
}

void LobbyMan::s_key_list_callback( PEER peer, qr2_key_type type, qr2_keybuffer_t keyBuffer, 
									void * param )
{
	//Dbg_Printf( "***** Got a key list callback\n" );
	// register the keys we use
	switch(type)
	{
		case key_server:
			//Dbg_Printf( "***** Got a request for server keys\n" );
			
			qr2_keybuffer_add(keyBuffer, GAMEMODE_KEY);
			qr2_keybuffer_add(keyBuffer, GAMENAME_KEY);
			qr2_keybuffer_add(keyBuffer, HOSTNAME_KEY);
			qr2_keybuffer_add(keyBuffer, HOSTPORT_KEY);
			qr2_keybuffer_add(keyBuffer, MAPNAME_KEY);
			qr2_keybuffer_add(keyBuffer, GAMETYPE_KEY);
			qr2_keybuffer_add(keyBuffer, TEAMPLAY_KEY);
			qr2_keybuffer_add(keyBuffer, NUMPLAYERS_KEY);
			qr2_keybuffer_add(keyBuffer, MAXPLAYERS_KEY);
			qr2_keybuffer_add(keyBuffer, PASSWORD_KEY);
			
			qr2_keybuffer_add(keyBuffer, NUMOBSERVERS_KEY);
			qr2_keybuffer_add(keyBuffer, MAXOBSERVERS_KEY);
			qr2_keybuffer_add(keyBuffer, RANKED_KEY);
			
			qr2_keybuffer_add(keyBuffer, SKILLLEVEL_KEY);
			qr2_keybuffer_add(keyBuffer, STARTED_KEY);
			qr2_keybuffer_add(keyBuffer, HOSTED_MODE_KEY);
			break;
		case key_player:
			//Dbg_Printf( "***** Got a request for player keys\n" );
			qr2_keybuffer_add(keyBuffer, PLAYER__KEY);
			qr2_keybuffer_add(keyBuffer, RATING__KEY);
			break;
		case key_team:
			//Dbg_Printf( "***** Got a request for team keys\n" );
			// no custom team keys
			break;
		default:
			break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	LobbyMan::s_nick_error_callback( PEER peer, int type, const char* nick, void *param )
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	if( type == PEER_IN_USE )
	{
		GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		char new_nick[128];
		Prefs::Preferences* prefs;
	
		prefs = gamenet_man->GetNetworkPreferences();
		const char* network_name = prefs->GetPreferenceString( Script::GenerateCRC( "network_id" ), Script::GenerateCRC( "ui_string" ));
        
		printf( "Nick Error. Nick %s In Use\n", nick );
		sprintf( new_nick, "a%d_%s", (int) Tmr::GetTime(), network_name );
		peerRetryWithNick( peer, (const char*) new_nick );
	}
	else if( type == PEER_INVALID )
	{
		char new_nick[128];
		
		printf( "Nick Error. Nick %s is Invalid\n", nick );
        chatFixNick( new_nick, nick );
		peerRetryWithNick( peer, (const char*) new_nick );
	}

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	LobbyMan::s_connect_callback( PEER peer, PEERBool success, void* param )
{
	LobbyMan* lobby_man = (LobbyMan*) param;

	lobby_man->m_connection_in_progress = false;
	if( success )
	{
		Net::Manager * net_man = Net::Manager::Instance();

		//Mlp::Manager * mlp_man = Mlp::Manager::Instance();
		//GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		//mlp_man->AddLogicTask( *lobby_man->m_lobby_logic_task );
		
		printf( "****** Connect Callback Successful\n" );

		net_man->SetPublicIP( peerGetLocalIP( peer ));
		CFuncs::ScriptStartLobbyList( NULL, NULL );
		//gamenet_man->SetServerListState( vSERVER_LIST_STATE_STARTING_LOBBY_LIST );
		lobby_man->m_expecting_disconnect = false;
		s_time_of_connection = Tmr::GetTime();
	}
	else
	{
		printf( "******* Connect Callback FAILED!\n" );
		Script::RunScript( "create_gamespy_connection_failure_dialog" );
	}
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	LobbyMan::s_group_rooms_callback( PEER peer, PEERBool success, int groupID, SBServer server,  
									const char * name, int numWaiting, int maxWaiting, int numGames, int numPlaying,
									void* param )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	LobbyMan* lobby_man = (LobbyMan*) param;
	// Check for the terminator
	if( success )
	{
		if( groupID == 0 )
		{   
			gamenet_man->SetServerListState( vSERVER_LIST_STATE_GOT_LOBBY_LIST );
		}
		else
		{
			int max_players, official_key;
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

			LobbyInfo* lobby_info;
			lobby_info = new LobbyInfo;
			strncpy( lobby_info->m_Name, name, 63 );
			lobby_info->m_Name[63] = '\0';
			lobby_info->m_NumServers = numGames;
			lobby_info->m_GroupId = groupID;
			lobby_info->m_MaxServers = maxWaiting;
            max_players = SBServerGetIntValue( server, "maxplayers", 0 );
			if( numWaiting >= max_players )
			{
				lobby_info->m_Full = true;
			}

			lobby_info->m_MaxRating = SBServerGetIntValue( server, "maxrating", 100000 );
			lobby_info->m_MinRating = SBServerGetIntValue( server, "minrating", 0 );

			Dbg_Printf( "Got lobby: %s, max: %d min: %d, rating: %d\n", name, lobby_info->m_MaxRating, lobby_info->m_MinRating, gamenet_man->mpStatsMan->GetStats()->GetRating());

			if( gamenet_man->mpStatsMan->GetStats()->GetRating() > lobby_info->m_MaxRating )
			{
				lobby_info->m_OffLimits = true;
			}

			if( gamenet_man->mpStatsMan->GetStats()->GetRating() < lobby_info->m_MinRating )
			{
				lobby_info->m_OffLimits = true;
			}

			official_key = 0;
			official_key = SBServerGetIntValue( server, "neversoft", 0 );
			if( official_key )
			{
				lobby_info->m_Official = true;
			}
            
			//printf( "Got lobby %d: %s\n", groupID, name );
			//printf( "Max Servers of lobby %d: %d\n", lobby_info->m_GroupId, lobby_info->m_MaxServers );
			if( lobby_info->m_NumServers > lobby_info->m_MaxServers )
			{
				lobby_info->m_MaxServers = lobby_info->m_NumServers;
			}
	
			lobby_info->SetPri( -groupID );
			lobby_man->m_lobbies.AddNode( lobby_info );

			Mem::Manager::sHandle().PopContext();
		}
	}
	else
	{
		gamenet_man->SetServerListState( vSERVER_LIST_STATE_FAILED_LOBBY_LIST );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	LobbyMan::s_join_room_callback( PEER peer, PEERBool success, PEERJoinResult result, RoomType roomType, 
										void* param )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	LobbyMan* lobby_man = (LobbyMan*) param;

	
	Dbg_Assert( roomType == GroupRoom );

	if( success )
	{
		//printf( "Entered group room\n" );
		Script::RunScript( "dialog_box_exit" );
		Script::RunScript( "create_network_select_games_menu" );
		lobby_man->m_in_group_room = true;

		lobby_man->refresh_player_list();
		if( gamenet_man->mpBuddyMan->IsLoggedIn())
		{
			char rating_str[64];
			char key[64];
			const char* keys;
			const char* values;

			gamenet_man->mpBuddyMan->SetStatusAndLocation( GP_CHATTING, (char*) Script::GetString( "homie_status_chatting" ), 
																		lobby_man->GetLobbyName());
			sprintf( key, "b_rating" );
			sprintf( rating_str, "%d", gamenet_man->mpStatsMan->GetStats()->GetRating());
			keys = key;
			values = rating_str;
			peerSetRoomKeys(peer, roomType, peerGetNick( peer ), 1, &keys, &values );
			peerGetRoomKeys( peer, roomType, "*", 1, &keys, s_get_room_keys_callback, 
							 lobby_man, false );
		}
	}
	else
	{
		switch( result )
		{
			case PEERFullRoom:
				//printf( "Full Room\n" );
				break;
			case PEERInviteOnlyRoom:
				//printf( "Invite Only\n" );
				break;
			case PEERBannedFromRoom:
				//printf( "Banned from room\n" );
				break;
			case PEERBadPassword:
				//printf( "Bad Password\n" );
				break;
			case PEERAlreadyInRoom:
				//printf( "Already in Room\n" );
				break;
			case PEERNoTitleSet:
				//printf( "No Title Set\n" );
				break;
			case PEERJoinFailed:
				//printf( "Generic: Join Failed\n" );
				break;
			default:
				break;

		};
		
		Script::RunScript( "CreateJoinLobbyFailedDialog" );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	LobbyMan::s_lobby_logic_code( const Tsk::Task< LobbyMan >& task )
{
	LobbyMan& man = task.GetData();

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());
	peerThink( man.m_peer );
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	LobbyMan::s_nat_negotiate_think_code( const Tsk::Task< LobbyMan >& task )
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());
	NNThink();
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void gamespy_data_handler( char* packet_data, int len, struct sockaddr* sender )
{
	

	//Dbg_Printf( "Got gamepsy data of length %d\n", len );
	//Dbg_Printf( "Data[0] = %c\n", packet_data[0] );
	//Dbg_Printf( "Data[1] = %c\n", packet_data[1] );

	Dbg_Assert( len < 2047 );
	// backslash as the first character signifies a gamespy packet
	if( ( (unsigned char) packet_data[0] == QR_MAGIC_1 ) &&
		( (unsigned char) packet_data[1] == QR_MAGIC_2 ))
	{
		//Dbg_Printf( "************ GOT GAMESPY QUERY CALL!!!!!!!!\n" );
		memcpy( s_gamespy_parse_data, packet_data, len );
		s_gamespy_sender = *sender;
		s_gamespy_parse_data_len = len;
		s_gamespy_data_type = vGAMESPY_QUERY_DATA;
	}
	else
	{
		if( len >= NATNEG_MAGIC_LEN )
		{
			//Dbg_Printf( "Got Nat Neg data of length %d\n", len );
			if(	( (unsigned char) packet_data[0] == NN_MAGIC_0 ) &&
				( (unsigned char) packet_data[1] == NN_MAGIC_1 ) &&
				( (unsigned char) packet_data[2] == NN_MAGIC_2 ) &&
				( (unsigned char) packet_data[3] == NN_MAGIC_3 ) &&
				( (unsigned char) packet_data[4] == NN_MAGIC_4 ) &&
				( (unsigned char) packet_data[5] == NN_MAGIC_5 ))
			{
				//Dbg_Printf( "Magic number matches\n" );
                memcpy( s_gamespy_parse_data, packet_data, len );
				s_gamespy_sender = *sender;
				s_gamespy_parse_data_len = len;
				s_gamespy_data_type = vGAMESPY_NAT_DATA;
			}
		}

	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	LobbyMan::s_process_gamespy_queries_code( const Tsk::Task< LobbyMan >& task )
{
	LobbyMan& man = task.GetData();
	Tmr::Time current_time;

	current_time = Tmr::GetTime();
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	//if( s_gamespy_parse_data[0] != '\0' )
	if( s_gamespy_data_type == vGAMESPY_QUERY_DATA )
	{
		//Dbg_Printf( "***** Parsing Gamespy Packet Data...\n" ); 
		//qr_parse_query( NULL, s_gamespy_parse_data, &s_gamespy_sender );
		peerParseQuery( man.GetPeer(), s_gamespy_parse_data, s_gamespy_parse_data_len, &s_gamespy_sender );
		s_gamespy_parse_data[0] = '\0';
		s_gamespy_parse_data_len = 0;
		s_gamespy_data_type = vGAMESPY_NO_DATA;
	}
	else if( s_gamespy_data_type == vGAMESPY_NAT_DATA )
	{
		//Dbg_Printf( "Processing Nat Neg data\n" );
        NNProcessData( s_gamespy_parse_data, s_gamespy_parse_data_len, (sockaddr_in*) &s_gamespy_sender );
		s_gamespy_parse_data[0] = '\0';
		s_gamespy_parse_data_len = 0;
		s_gamespy_data_type = vGAMESPY_NO_DATA;
	}
	else if( s_got_gamespy_callback == false )
	{
		Manager * gamenet_man = Manager::Instance();

		if( gamenet_man->InNetGame() && gamenet_man->OnServer())
		{
			// After N seconds, if we have yet to be contacted by GameSpy, notify the user that his server
			// was not properly posted to GameSpy
			if( ( current_time - s_game_start_time ) > vPOST_SERVER_TIMEOUT )
			{
				
				if( s_notified_user_not_connected == false )
				{
					Script::RunScript( "CreateNotPostedDialog" );
					s_notified_user_not_connected = true;
				}
			}
			else if(( current_time - s_last_post_time ) > vPOST_SERVER_RETRY_TIME )
			{
				// Gamespy still hasn't queried us for our options. Let's tell them again
				// that our server is up
				peerStateChanged( man.GetPeer());
				s_last_post_time = current_time;
			}
		}
	}
	
	//qr_process_queries( NULL );

	Mem::Manager::sHandle().PopContext();
}

/*****************************************************************************
**							  Handler Functions								**
*****************************************************************************/

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

LobbyMan::LobbyMan( void )
{
	m_in_group_room = false;
	m_expecting_disconnect = true;
	m_peer = NULL;

	m_lobby_logic_task = new Tsk::Task< LobbyMan > ( s_lobby_logic_code, *this );
	m_nat_negotiate_task = new Tsk::Task< LobbyMan > ( s_nat_negotiate_think_code, *this );
	m_process_gamespy_queries_task = new Tsk::Task< LobbyMan > ( s_process_gamespy_queries_code, *this );
	qr2_register_key( NUMOBSERVERS_KEY, "numobservers" );
	qr2_register_key( MAXOBSERVERS_KEY, "maxobservers" );
	qr2_register_key( SKILLLEVEL_KEY, "skilllevel" );
	qr2_register_key( STARTED_KEY, "started" );
	qr2_register_key( HOSTED_MODE_KEY, "hostmode" );
	qr2_register_key( RANKED_KEY, "ranked" );
	qr2_register_key( RATING__KEY, "rating_" );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

LobbyMan::~LobbyMan( void )
{
	delete m_lobby_logic_task;
	delete m_nat_negotiate_task;
	delete m_process_gamespy_queries_task;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::StartReportingGame( void )
{
	Mlp::Manager * mlp_manager = Mlp::Manager::Instance();
	Manager * gamenet_man = Manager::Instance();
	Net::Server* server;
	PEERBool result;

	server = gamenet_man->GetServer();
	Dbg_Assert( server );
	
	server->SetForeignPacketHandler( gamespy_data_handler );
	s_gamespy_parse_data[0] = '\0';
	s_gamespy_data_type = vGAMESPY_NO_DATA;
    
	result = peerStartReportingWithSocket( m_peer, server->GetSocket(), vHOST_PORT );
	if( result == true )
	{
		peerStateChanged( m_peer );
	}

	s_game_start_time = Tmr::GetTime();
	s_last_post_time = s_game_start_time;
	s_notified_user_not_connected = false;
	s_got_gamespy_callback = false;
	mlp_manager->AddLogicTask( *m_process_gamespy_queries_task );
	m_reported_game = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::StopReportingGame( void )
{
	if( m_peer )
	{
        peerStopGame( m_peer );
		m_process_gamespy_queries_task->Remove();
	}

	m_reported_game = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		LobbyMan::ReportedGame( void )
{
	return m_reported_game;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::StartLobbyList( void )
{
	Manager * gamenet_man = Manager::Instance();
	//Thread::PerThreadStruct	net_thread_data;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

    gamenet_man->SetServerListState( vSERVER_LIST_STATE_GETTING_LOBBY_LIST );

	// Reset the server list refresh time so that the 1-second minimum rule for refreshing server
	// lists won't apply when going back and forth really quickly to/from lobby/server lists
	s_last_refresh_time = 0;

	peerListGroupRooms( m_peer, "\\maxplayers\\neversoft\\maxrating\\minrating", s_group_rooms_callback, this, false );
    
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::StopLobbyList( void )
{
	Lst::Search< LobbyInfo > sh;
	LobbyInfo* lobby, *next;
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	if( m_peer )
	{
		peerStopListingGames( m_peer );
	}

	for( lobby = sh.FirstItem( m_lobbies ); lobby; lobby = next )
	{
		next = sh.NextItem();
		delete lobby;
	}

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::FillLobbyList( void )
{
	Lst::Search< LobbyInfo > sh;
	LobbyInfo* lobby;
	int index;

	Script::RunScript( "dialog_box_exit" );
	Script::RunScript( "create_lobby_list_menu" );
	
	index = 0;
	for( lobby = sh.FirstItem( m_lobbies ); lobby; lobby = sh.NextItem())
	{
		add_lobby_to_menu( lobby, index++ );    
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

LobbyInfo*	LobbyMan::GetLobbyInfo( int index )
{
	Lst::Search< LobbyInfo > sh;
	LobbyInfo* lobby;

	for( lobby = sh.FirstItem( m_lobbies ); lobby; lobby = sh.NextItem())
	{
		if( index == 0 )
		{
			return lobby;
		}

		index--;
	}
	
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::SetLobbyId( int id )
{
	m_lobby_id = id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int			LobbyMan::GetLobbyId( void )
{
	return m_lobby_id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::SetLobbyNumServers( int num_servers )
{
	m_lobby_num_servers = num_servers;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int			LobbyMan::GetLobbyNumServers( void )
{
	return m_lobby_num_servers;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::SetLobbyMaxServers( int max_servers )
{
	m_lobby_max_servers = max_servers;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int			LobbyMan::GetLobbyMaxServers( void )
{
	return m_lobby_max_servers;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::SetLobbyName( char* name )
{
	Dbg_Assert( name );

	strcpy( m_lobby_name, name );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*		LobbyMan::GetLobbyName( void )
{
	return m_lobby_name;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::SetServerName( char* name )
{
	strcpy( m_server_name, name );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*		LobbyMan::GetServerName( void )
{
	return m_server_name;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		LobbyMan::SetOfficialLobby( bool official )
{
	m_official = official;
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		LobbyMan::IsLobbyOfficial( void )
{
	return m_official;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void 	LobbyMan::s_nat_negotiate_complete( NegotiateResult result, SOCKET gamesocket, 
												struct sockaddr_in *remoteaddr, void *userdata )
{
	Manager* gamenet_man = Manager::Instance();
	int cookie;

	cookie = (int) userdata;

	Dbg_Printf( "**** NAT Negotiation Complete: %d\n", result );
	switch( result )
	{
		case nr_success:
		{
			Dbg_Printf( "**** NAT Negotiation Complete: Success!\n" );

			if( !gamenet_man->OnServer())
			{
				bool observe_only;
				Dbg_Printf( "**** IP: %s Port: %d\n", inet_ntoa( remoteaddr->sin_addr ), ntohs( remoteaddr->sin_port ));
				observe_only = ( gamenet_man->GetJoinMode() == GameNet::vJOIN_MODE_OBSERVE );
				gamenet_man->JoinServer( observe_only, (int) remoteaddr->sin_addr.s_addr, ntohs( remoteaddr->sin_port ), 0 );
			}
			break;
		}
		case nr_deadbeatpartner:
			Dbg_Printf( "**** NAT Negotiation Complete: Deadbeat Partner\n" );
			break;
		case nr_inittimeout:
			Dbg_Printf( "**** NAT Negotiation Complete: Timeout\n" );
			break;
		case nr_unknownerror:
		default:
			Dbg_Printf( "**** NAT Negotiation Complete: Unknown Error\n" );
			break;
	}

	if( gamenet_man->OnServer())
	{
		gamenet_man->mpLobbyMan->m_nat_count.Release();
		if( gamenet_man->mpLobbyMan->m_nat_count.InUse() == false )
		{
			gamenet_man->mpLobbyMan->m_nat_negotiate_task->Remove();
		}
	}
	else
	{
		Net::Client* client;

		NNCancel( cookie );
		NNThink();
		NNFreeNegotiateList();

		client = gamenet_man->GetClient( 0 );
		client->SetForeignPacketHandler( NULL );
		gamenet_man->mpLobbyMan->m_nat_negotiate_task->Remove();
		gamenet_man->mpLobbyMan->m_process_gamespy_queries_task->Remove();

		if( result != nr_success )
		{
			Script::RunScript( "show_nat_timeout" );
		}
	}

	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void 	LobbyMan::s_nat_negotiate_progress( NegotiateState state, void *userdata )
{
    switch( state )
	{
		case ns_initack:
			Dbg_Printf( "**** NAT Negotiation : Init Ack State\n" );
			break;
		case ns_connectping:
			Dbg_Printf( "**** NAT Negotiation : Connect Ping State\n" );
			break;
		default:
			Dbg_Printf( "**** NAT Negotiation : Unknown State\n" );
			break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	LobbyMan::ScriptCancelNatNegotiation(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager* gamenet_man = Manager::Instance();

	NNCancel( gamenet_man->mpLobbyMan->m_cookie );
	NNThink();
	NNFreeNegotiateList();
	
	gamenet_man->mpLobbyMan->m_nat_negotiate_task->Remove();
	gamenet_man->mpLobbyMan->m_process_gamespy_queries_task->Remove();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	LobbyMan::ScriptStartNatNegotiation(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager* gamenet_man = Manager::Instance();
	LobbyMan* lobby_man;
	Mlp::Manager* mlp_manager = Mlp::Manager::Instance();
	Net::Client* client;
	const char *server_ip = NULL;
	int port = 0;
	NegotiateError err;
    
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	lobby_man = gamenet_man->mpLobbyMan;
	pParams->GetText( NONAME, &server_ip );
	pParams->GetInteger( NONAME, &port );
	pParams->GetChecksum( CRCD( 0x751f4599, "cookie" ), (uint32*) &lobby_man->m_cookie );
	
    //port = 9959;
	Dbg_Printf( "******* Starting NAT Negotiation for server : %s port %p *******", server_ip, port );
	
	client = gamenet_man->SpawnClient( false, false, true, 0 );
    peerSendNatNegotiateCookie( lobby_man->GetPeer(), inet_addr( server_ip ), 
								port, lobby_man->m_cookie );
    err = NNBeginNegotiationWithSocket( client->GetSocket(), lobby_man->m_cookie, 1, s_nat_negotiate_progress, 
										s_nat_negotiate_complete, (void*) lobby_man->m_cookie );
	if( err != ne_noerror )
	{
		Dbg_Printf( "******* NAT Neg Error: %d\n", err );
		NNFreeNegotiateList();
		Mem::Manager::sHandle().PopContext();
		return false;
	}
	Dbg_Assert( err == ne_noerror );

	Dbg_Printf( "*** Added NAT Task\n" );
	mlp_manager->AddLogicTask( *lobby_man->m_nat_negotiate_task );
	client->SetForeignPacketHandler( gamespy_data_handler );
	mlp_manager->AddLogicTask( *lobby_man->m_process_gamespy_queries_task );
	s_gamespy_parse_data[0] = '\0';

	Mem::Manager::sHandle().PopContext();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	LobbyMan::ScriptRejoinLobby(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager* gamenet_man = Manager::Instance();
	
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	peerJoinGroupRoom( gamenet_man->mpLobbyMan->m_peer, gamenet_man->mpLobbyMan->GetLobbyId(), s_join_room_callback, gamenet_man->mpLobbyMan, false );

	Mem::Manager::sHandle().PopContext();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	LobbyMan::ScriptChooseLobby(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	LobbyInfo* p_lobby;
	Manager* gamenet_man = Manager::Instance();
	int index = 0;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	pParams->GetInteger( Script::GenerateCRC("index"), &index );
				
	p_lobby = gamenet_man->mpLobbyMan->GetLobbyInfo( index );
	Dbg_Assert( p_lobby );
								
	//Dbg_Printf( "Choosing lobby %d\n", p_lobby->m_GroupId );
	gamenet_man->mpLobbyMan->SetLobbyId( p_lobby->m_GroupId );
	gamenet_man->mpLobbyMan->SetLobbyNumServers( p_lobby->m_NumServers );
	gamenet_man->mpLobbyMan->SetLobbyMaxServers( p_lobby->m_MaxServers );
	gamenet_man->mpLobbyMan->SetLobbyName( p_lobby->m_Name );
	gamenet_man->mpLobbyMan->SetOfficialLobby( p_lobby->m_Official );
	
    peerJoinGroupRoom( gamenet_man->mpLobbyMan->m_peer, gamenet_man->mpLobbyMan->GetLobbyId(), s_join_room_callback, gamenet_man->mpLobbyMan, false );

	// Don't need the lobby list anymore. We have recorded all its data
	gamenet_man->mpLobbyMan->StopLobbyList();

	Mem::Manager::sHandle().PopContext();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	LobbyMan::ScriptGetNumPlayersInLobby(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Manager* gamenet_man = Manager::Instance();
	Script::CStruct* p_return_params;

	p_return_params = pScript->GetParams();
	p_return_params->AddInteger( "num_players", gamenet_man->mpLobbyMan->m_players.CountItems());
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	LobbyMan::ScriptLeaveLobby(Script::CScriptStructure *pParams, Script::CScript* pScript )
{
	Manager* gamenet_man = Manager::Instance();
	LobbyMan* lobby_man;
	BuddyMan* buddy_man;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	lobby_man = gamenet_man->mpLobbyMan;
	buddy_man = gamenet_man->mpBuddyMan;

	peerLeaveRoom( lobby_man->m_peer, GroupRoom, NULL );
	lobby_man->m_in_group_room = false;

	if( buddy_man->IsLoggedIn())
	{
		if( !pParams->ContainsComponentNamed( CRCD(0xb2ab4dab,"preserve_status")))
		{
			buddy_man->SetStatusAndLocation( GP_ONLINE, 	(char*) Script::GetString( "homie_status_online" ), 
														(char*) Script::GetString( "homie_status_logging_in" ));
		}
	}

	// Maintain our association with our group room so that, if we're hosting a game, it will
	// report it to the correct group room
	peerSetGroupID( lobby_man->m_peer, lobby_man->GetLobbyId());

	Mem::Manager::sHandle().PopContext();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	LobbyMan::ScriptLobbyConnect(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	PEERBool ping_rooms[NumRooms] = {false, true, false};
	PEERBool x_ping_rooms[NumRooms] = {false, false, false};
	PEER peer;
	Thread::PerThreadStruct	net_thread_data;
    
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());
	
	gamenet_man->mpLobbyMan->Initialize();
	
	peer = gamenet_man->mpLobbyMan->m_peer;
	peerSetTitle( peer, 
				  "thps5ps2", 
				  "G2k8cF", 
				  "thps5ps2", 
				  "G2k8cF", 
				  vGAME_VERSION,
				  15, 
				  true,
				  ping_rooms, 
				  x_ping_rooms );

	Dbg_Printf( "Connecting to lobby 1\n" );
	net_thread_data.m_pEntry = s_threaded_peer_connect;
	net_thread_data.m_iInitialPriority = vSOCKET_THREAD_PRIORITY;
	net_thread_data.m_pStackBase = gamenet_man->GetNetThreadStack();
	net_thread_data.m_iStackSize = vNET_THREAD_STACK_SIZE;
	net_thread_data.m_utid = 0x152;
	Thread::CreateThread( &net_thread_data );
	gamenet_man->SetNetThreadId( net_thread_data.m_osId );
	StartThread( gamenet_man->GetNetThreadId(), gamenet_man->mpLobbyMan );
	Dbg_Printf( "Connecting to lobby 2\n" );
	mlp_man->AddLogicTask( *gamenet_man->mpLobbyMan->m_lobby_logic_task );
	
	Mem::Manager::sHandle().PopContext();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	LobbyMan::ScriptLobbyDisconnect(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	Dbg_Printf( "******************************* DISCONNECTING FROM PEER!!!! \n" );

	while( gamenet_man->mpLobbyMan->m_nat_count.InUse())
	{
		gamenet_man->mpLobbyMan->m_nat_count.Release();
	}

	gamenet_man->mpLobbyMan->StopLobbyList();
	gamenet_man->mpLobbyMan->clear_player_list();
	gamenet_man->mpLobbyMan->m_lobby_logic_task->Remove();
	gamenet_man->mpLobbyMan->m_nat_negotiate_task->Remove();
	gamenet_man->mpLobbyMan->m_in_group_room = false;
	gamenet_man->mpLobbyMan->m_expecting_disconnect = true;
	if( gamenet_man->mpLobbyMan->m_peer )
	{
		peerStopGame( gamenet_man->mpLobbyMan->m_peer );
		peerClearTitle( gamenet_man->mpLobbyMan->m_peer );
		peerDisconnect( gamenet_man->mpLobbyMan->m_peer );
	}
	gamenet_man->mpLobbyMan->Shutdown();
	//peerShutdown( gamenet_man->mpLobbyMan->m_peer );
	//gamenet_man->mpLobbyMan->m_peer = NULL;

	//Dbg_Printf( "******************************* DISCONNECTED FROM PEER!!!! \n" );
	Mem::Manager::sHandle().PopContext();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	LobbyMan::ScriptSetQuietMode( Script::CScriptStructure* pParams, Script::CScript* pScript )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());
	if( pParams->ContainsFlag( "off" ))
	{
		peerSetQuietMode( gamenet_man->mpLobbyMan->m_peer, false );
	}
	else
	{
		peerSetQuietMode( gamenet_man->mpLobbyMan->m_peer, true );
	}
	Mem::Manager::sHandle().PopContext();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	LobbyMan::ScriptFillPlayerList( Script::CScriptStructure* pParams, Script::CScript* pScript )
{
	Spt::SingletonPtr< GameNet
		::Manager > gamenet_man;

	gamenet_man->mpLobbyMan->fill_player_list();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	LobbyMan::ScriptFillLobbyProspectiveBuddyList( Script::CScriptStructure* pParams, Script::CScript* pScript )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	gamenet_man->mpLobbyMan->fill_prospective_buddy_list();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	LobbyMan::ScriptCanHostGame( Script::CScriptStructure* pParams, Script::CScript* pScript )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	if( gamenet_man->mpLobbyMan->IsLobbyOfficial())
	{
		if( gamenet_man->mpBuddyMan->IsLoggedIn())
		{
			int profile;

			profile = gamenet_man->mpBuddyMan->GetProfile();
			if(	( profile == 27747931 ) ||
				( profile == 27747977 ) ||
				( profile == 27748011 ) ||
				( profile == 27748097 ) ||
				( profile == 27748044 ) ||
				( profile == 27748142 ) ||
				( profile == 27748187 ) ||
				( profile == 27748211 ) ||
				( profile == 27748244 ) ||
				( profile == 27748273 ))
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	return( gamenet_man->mpLobbyMan->GetLobbyNumServers() < gamenet_man->mpLobbyMan->GetLobbyMaxServers());
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	LobbyMan::ScriptSendMessage( Script::CScriptStructure* pParams, Script::CScript* pScript )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	const char *p_text;
	
	if( pParams->GetText( "text", &p_text ))
	{
		if( pParams->ContainsFlag( "system_message" ))
		{
			Script::CStruct* p_params;
			char msg[1024];
		
			sprintf( msg, "<System> %s", p_text );
			p_params = new Script::CStruct;	
			p_params->AddString( "text", msg );
			Script::RunScript("create_console_message", p_params );
			delete p_params;
		}
		else
		{
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());
			peerMessageRoom( gamenet_man->mpLobbyMan->m_peer, GroupRoom, p_text, NormalMessage );
			Mem::Manager::sHandle().PopContext();
		}
	}

	return true;
}
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	LobbyMan::Initialize( void )
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());

	PEERCallbacks callbacks;

	memset( &callbacks, 0, sizeof( PEERCallbacks ));
	callbacks.disconnected = s_disconnected_callback;
	callbacks.roomMessage = s_room_message_callback;
	callbacks.playerMessage = s_player_message_callback;
	callbacks.playerJoined = s_player_joined_callback;
	callbacks.playerLeft = s_player_left_callback;
	callbacks.playerInfo = s_player_info_callback;
	callbacks.newPlayerList = s_new_player_list_callback;
	callbacks.roomKeyChanged = s_room_key_callback;

    callbacks.qrNatNegotiateCallback = s_nat_negotiate_callback;
	callbacks.qrKeyList = s_key_list_callback;
	callbacks.qrServerKey = s_server_key_callback;
	callbacks.qrPlayerKey = s_player_key_callback;
	callbacks.qrCount = s_key_count_callback;
	callbacks.qrAddError = s_add_error_callback;
	callbacks.qrPublicAddressCallback = s_public_address_callback;
	callbacks.param = this;
	
	m_peer = peerInitialize( &callbacks );
	Dbg_Assert( m_peer );
	
	m_in_group_room = false;
	m_expecting_disconnect = true;
	m_reported_game = false;
	m_official = false;

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	LobbyMan::Shutdown( void )
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	m_lobby_logic_task->Remove();
	m_nat_negotiate_task->Remove();
	m_process_gamespy_queries_task->Remove();

	if( m_peer )
	{
		peerShutdown( m_peer );
	}
	m_peer = NULL;

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

PEER	LobbyMan::GetPeer( void )
{
	return m_peer;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


bool	LobbyMan::InGroupRoom( void )
{
	return m_in_group_room;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*	LobbyMan::PlayerName( char* lobby_name )
{
	char* player_name;

	player_name = strchr( lobby_name, '_' );
	if( player_name == NULL )
	{
		return lobby_name;
	}
	return ( player_name + 1 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

LobbyPlayerInfo::LobbyPlayerInfo( void ) : Lst::Node< LobbyPlayerInfo > ( this )
{
	m_Profile = 0;
	m_Rating = 0;
	m_GotInfo = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


} // namespace GameNet




