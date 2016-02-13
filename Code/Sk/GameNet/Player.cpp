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
**	File name:		Player.cpp												**
**																			**
**	Created by:		08/09/01	-	spg										**
**																			**
**	Description:	PlayerInfo functionality								**
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
#include <gfx/2d/screenelemman.h>

#include <sk/gamenet/gamenet.h>

#include <sk/modules/skate/skate.h>
#include <sk/modules/skate/gamemode.h>
#include <sk/modules/skate/goalmanager.h>
#include <sk/modules/frontend/frontend.h>
//#include <sk/modules/frontend/mainmenu.h>

#include <sk/objects/skaterprofile.h>
#include <sk/objects/proxim.h>
#include <sk/objects/crown.h>
#include <sk/objects/skatercam.h>
#include <sk/components/skaterstatehistorycomponent.h>
#include <sk/components/skaterscorecomponent.h>

#include <sk/scripting/cfuncs.h>
#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>

#include <sk/objects/skater.h>		   // mostly getting score


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

enum
{
	vINVULNERABILITY_PERIOD = 10,		// in frames
	vINVULNERABILITY_BAIL_SECONDS = 1,	// in seconds
	vPROJECTILE_INVULNERABILITY_INTERVAL = 5000,// in ms
};

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

int 	Manager::GetTeamScore( int team_id )
{
	PlayerInfo* player, *local_player;
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Lst::Search< PlayerInfo > sh;
	Mdl::Score* score_obj;
	int score;
	bool in_goal_attack;
	 
	score = 0;

	local_player = GetLocalPlayer();
	in_goal_attack = skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netgoalattack" );
	// Next create a sorted list of the players (sorted in order of highest to lowest score)
	for( player = FirstPlayerInfo( sh ); player; player = NextPlayerInfo( sh ))
	{
		// Only show the names/scores of players that are fully in the game
		if((player->IsFullyIn() == false ) || ( player->m_Team != team_id ))
		{
			continue;
		}

		if( in_goal_attack && !local_player->IsObserving())
		{
			Game::CGoalManager* pGoalManager;
			 
			pGoalManager = Game::GetGoalManager();
			return pGoalManager->NumGoalsBeatenByTeam( team_id );
		}
		else
		{
			Obj::CSkaterScoreComponent* p_skater_score_component = GetSkaterScoreComponentFromObject(player->m_Skater);
			Dbg_Assert(p_skater_score_component);
			
			score_obj = p_skater_score_component->GetScore();
			score += score_obj->GetTotalScore();
		}
	}

	return score;
}

int 	Manager::GetPlayerScore( int obj_id )
{
	bool in_goal_attack;
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	GameNet::PlayerInfo* local_player;
	
	local_player = GetLocalPlayer();
	in_goal_attack = skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netgoalattack" );
	if( in_goal_attack && !local_player->IsObserving())
	{
		Game::CGoalManager* pGoalManager;
			 
		pGoalManager = Game::GetGoalManager();
		return pGoalManager->NumGoalsBeatenBy( obj_id );
	}
	else
	{
		Mdl::Score* score_obj;
		PlayerInfo* player;

		player = GetPlayerByObjectID( obj_id );
		Dbg_Assert( player );
		
		Obj::CSkaterScoreComponent* p_skater_score_component = GetSkaterScoreComponentFromObject(player->m_Skater);
		Dbg_Assert(p_skater_score_component);
	
		score_obj = p_skater_score_component->GetScore();
		return score_obj->GetTotalScore();
	}
}

void	Manager::s_render_scores_code( const Tsk::Task< Manager >& task )
{
	Front::CScreenElementManager* p_screen_elem_man = Front::CScreenElementManager::Instance();
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Script::CScriptStructure *pParams;
	Game::CGameMode* pGameMode;
	PlayerInfo* player;
	Lst::Search< PlayerInfo > sh;
	static Tmr::Time last_update = 0;
	Tmr::Time cur_time;
	Lst::Head< ScoreRank > rank_list;
	Lst::Search< ScoreRank > rank_sh;
	ScoreRank* rank, *next;
	bool show_score, king_mode;

	Manager& man = task.GetData();
    // Only draw the scores in modes in which the score accumulates
	pGameMode = skate_mod->GetGameMode();
	show_score =  skate_mod->GetGameMode()->GetNameChecksum() != CRCD( 0x1c471c60, "netlobby" );

	// in King of the hill games, "score" is actually milliseconds with the crown
	king_mode =	( skate_mod->GetGameMode()->GetNameChecksum() == CRCD( 0x6ef8fda0, "netking" )) ||
				( skate_mod->GetGameMode()->GetNameChecksum() == CRCD( 0x5d32129c, "king" ));
    
	cur_time = Tmr::GetTime();
	if(( cur_time - last_update ) >= Tmr::Seconds( 1 ))
	{
		int i, j;
		uint32 id;
		
		last_update = cur_time;

		// First clear all previous entries
		Script::RunScript( "clear_scores" );
        
		if( show_score && man.ShouldDisplayTeamScores())
		{
			ScoreRank* ranks[vMAX_TEAMS];
            
			for( j = 0; j < skate_mod->GetGameMode()->NumTeams(); j++ )
			{
				if( man.NumTeamMembers( j ) > 0 )
				{
					uint32 team_name_checksum;
					
					switch( j )
					{
					case 0:
						team_name_checksum = CRCD( 0x3224348d, "team_1_name" );
						break;
					case 1:
						team_name_checksum = CRCD( 0xb4b04623, "team_2_name" );
						break;
					case 2:
						team_name_checksum = CRCD( 0x7fec9586, "team_3_name" );
						break;
					case 3:
						team_name_checksum = CRCD( 0x62e9a53e, "team_4_name" );
						break;
					default:
						Dbg_Assert( 0 );
						team_name_checksum = 0;
						break;
					}
	
					ranks[j] = new ScoreRank;
					strcpy( ranks[j]->m_Name, Script::GetString( team_name_checksum ));
					ranks[j]->m_IsKing = false;
					ranks[j]->m_HasFlag = false;
					ranks[j]->m_WhichFlags = 0;
					ranks[j]->m_ColorIndex = j + 2;
					ranks[j]->m_TotalScore = man.GetTeamScore( j );
				}
			}

			// Next create a sorted list of the players (sorted in order of highest to lowest score)
			for( player = man.FirstPlayerInfo( sh ); player; player = man.NextPlayerInfo( sh ))
			{
				// Only show the names/scores of players that are fully in the game
				if( !( player->IsFullyIn()))
				{
					continue;
				}
				if( player->IsLocalPlayer())
				{
					char new_name[64];

					// This is my team.  Make that obvious to the players
					sprintf( new_name, "> %s", ranks[player->m_Team]->m_Name );
					strcpy( ranks[player->m_Team]->m_Name, new_name );
				}
	
				if( player->IsKing())
				{
					ranks[player->m_Team]->m_IsKing = true;
				}
				if( player->HasCTFFlag())
				{
					ranks[player->m_Team]->m_HasFlag = true;
					ranks[player->m_Team]->m_WhichFlags |= ( 1 << player->HasWhichFlag());
				}
			}

			for( j = 0; j < skate_mod->GetGameMode()->NumTeams(); j++ )
			{
				if( man.NumTeamMembers( j ) > 0 )
				{
					// Lists sort based on node's priority so set the node's priority to that of
					// the team's score and add it to the list
					ranks[j]->SetPri( ranks[j]->m_TotalScore );
					rank_list.AddNode( ranks[j] );
				}
			}
		}
		else
		{
			// Next create a sorted list of the players (sorted in order of highest to lowest score)
			for( player = man.FirstPlayerInfo( sh ); player; player = man.NextPlayerInfo( sh ))
			{
				// Only show the names/scores of players that are fully in the game
				if( !( player->IsFullyIn()))
				{
					continue;
				}
	
				rank = new ScoreRank;
				if( player->IsLocalPlayer())
				{
					// This is me.  Make that obvious.
					sprintf( rank->m_Name, "> %s", player->m_Name );
				}
				else
				{
					strcpy( rank->m_Name, player->m_Name );
				}
				
				rank->m_IsKing = player->IsKing();
				if( skate_mod->GetGameMode()->IsTeamGame())
				{
					rank->m_ColorIndex = player->m_Team + 2;
				}
				else
				{
					rank->m_ColorIndex = player->m_Skater->GetID() + 2;
				}
				if( player->HasCTFFlag())
				{
					rank->m_HasFlag = true;
					rank->m_WhichFlags |= ( 1 << player->HasWhichFlag());
				}
				
				if( show_score )
				{
					// Lists sort based on node's priority so set the node's priority to that of
					// the player's score and add him to the list
					rank->SetPri( man.GetPlayerScore( player->m_Skater->GetID()));
					rank_list.AddNode( rank );
				}
				else
				{
					// Just add the player to the list so that the order is the same regardless of score
					rank_list.AddToTail( rank );
				}
			}
		}
		

		// Now loop through the sorted list in order of highest to lowest score and print them out
		i = 0;
		for( rank = rank_sh.FirstItem( rank_list ); rank; rank = next )
		{   
			char score_str[64];
			char title[16];

			title[0]='\0';
			if( rank->m_IsKing )
			{
#if ENGLISH == 0
				sprintf( title, Script::GetLocalString( "player_str_king_abbreviation" ));
#else 
				sprintf( title, "K" );
#endif
			}
			else
			{
				if( rank->m_HasFlag )
				{
					if( rank->m_WhichFlags & 1 )
					{
						strcat( title, "\\s0" );
					}
					if( rank->m_WhichFlags & 2 )
					{
						strcat( title, "\\s1" );
					}
					if( rank->m_WhichFlags & 4 )
					{
						strcat( title, "\\s2" );
					}
					if( rank->m_WhichFlags & 8 )
					{
						strcat( title, "\\s3" );
					}
				}
				else
				{
					title[0] = '\0';
				}
			}

			next = rank_sh.NextItem();
			if( show_score )
			{
				if( king_mode )
				{
					int seconds;

					seconds = Tmr::InSeconds( rank->GetPri() );
					if( seconds >= 3600 )
					{
						sprintf( score_str, "\\c4%s\\c%d %s \\c%d %d:%.2d:%.2d", 
								   								title,
																rank->m_ColorIndex, 
																rank->m_Name,
																rank->m_ColorIndex,
																seconds / 3600,
																( seconds % 3600 ) / 60,
																seconds % 60 );
					}
					else
					{
						sprintf( score_str, "\\c4%s\\c%d %s \\c%d %d:%.2d", 
										   						title,
																rank->m_ColorIndex, 
																rank->m_Name,
																rank->m_ColorIndex,
																seconds / 60,
																seconds % 60 );
					}
					
				}
				else
				{
					sprintf( score_str, "%s\\c%d %s \\c%d %d", 	title,
																rank->m_ColorIndex, 
																rank->m_Name,
																rank->m_ColorIndex,
																rank->GetPri());
				}

			}
			else
			{
				sprintf( score_str, "\\c%d%s", 	rank->m_ColorIndex, rank->m_Name );
			}
            
			switch( i )
			{
			case 0:
				id = CRCD( 0x2b083809, "net_score_1" );
				break;
			case 1:
				id = CRCD( 0xb20169b3, "net_score_2" );
				break;
			case 2:
				id = CRCD( 0xc5065925, "net_score_3" );
				break;
			case 3:
				id = CRCD( 0x5b62cc86, "net_score_4" );
				break;
			case 4:
				id = CRCD( 0x2c65fc10, "net_score_5" );
				break;
			case 5:
				id = CRCD( 0xb56cadaa, "net_score_6" );
				break;
			case 6:
				id = CRCD( 0xc26b9d3c, "net_score_7" );
				break;
			case 7:
				id = CRCD( 0x52d480ad, "net_score_8" );
				break;
			default:
				Dbg_Assert( 0 );
				id = 0;
				break;
			}
			
			pParams = new Script::CScriptStructure;
			pParams->AddComponent( CRCD( 0xc4745838, "text" ), ESYMBOLTYPE_STRING, score_str );
			pParams->AddComponent( CRCD( 0x40c698af, "id" ), ESYMBOLTYPE_NAME, id );
			
			
			Front::CScreenElement* p_name_elem = p_screen_elem_man->GetElement( id, Front::CScreenElementManager::DONT_ASSERT );
			// If any operable menus are up, ignore input
			if( p_name_elem )
			{
				//make sure the scores are enabled
				//if(!Mdl::Skate::Instance()->GetCareer()->GetGlobalFlag(Script::GetInteger(CRCD(0xfbb66146,"NO_DISPLAY_NET_SCORES")))))
					p_name_elem->SetProperties( pParams );
			}
			Script::RunScript( "update_score", pParams );

			delete pParams;
			delete rank;
			i++;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

PlayerInfo::PlayerInfo( int flags )
: Lst::Node< PlayerInfo > ( this )
{
	int i, j;

	sprintf( m_Name, "<No Name>" );

	for( i = 0; i < Mdl::Skate::vMAX_SKATERS; i++ )
	{
		m_LastSentProps.m_LastSkaterFlagsUpdate[i] = -1;
		m_LastSentProps.m_LastSkaterStateUpdate[i] = -1;
		m_LastSentProps.m_LastDoingTrickUpdate[i] = -1;
		for( j = 0; j < 3; j++ )
		{
			m_LastSentProps.m_LastSkaterPosUpdate[i][j] = -1;
			m_LastSentProps.m_LastSkaterRotUpdate[i][j] = -1;
		}
	}

	m_flags = flags;
	m_observer_logic_task = NULL;
	m_jump_in_frame = 0;
	m_last_object_update_id = vMAX_PLAYERS - 1;
	m_face_data = NULL;

	m_Skater = NULL;
	mp_SkaterProfile = NULL;
	m_last_bail_time = 0;
	m_last_hit_time = 0;
	m_Team = vTEAM_RED;
	m_Profile = 0;
	m_Rating = 0;
	m_VehicleControlType = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

PlayerInfo::~PlayerInfo( void )
{
	

	if ( mp_SkaterProfile )
		delete mp_SkaterProfile;

	if( m_observer_logic_task )
	{
		delete m_observer_logic_task;
	}

	if( m_face_data )
	{
		delete [] m_face_data;
		m_face_data = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::SetSkater( Obj::CSkater* skater )
{
	Manager * gamenet_man = Manager::Instance();
	
	m_Skater = skater;
	if( !IsLocalPlayer())
	{
		if( gamenet_man->GetCurrentlyObservedPlayer() == this )
		{
			gamenet_man->ObservePlayer( this );
		}
	}
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::SetFaceData( uint8* face_data, int size )
{
	uint8 player_id;

	Dbg_Assert( m_face_data == NULL );
	Dbg_Assert( face_data != NULL );
	Dbg_Assert( m_Skater != NULL );

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetMiscHeap());

	player_id = m_Skater->GetID();
		 
	// Store the player's skater id as the first byte of the face data
	m_face_data = new uint8[size+1];
	*m_face_data = player_id;
	memcpy( m_face_data + 1, face_data, size );

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// The player's skater id is the first byte of the face data
uint8*	PlayerInfo::GetFaceData( void )
{
	return m_face_data;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::CopyProfile( Obj::CSkaterProfile* pSkaterProfile )
{
	

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());

	Dbg_Assert( pSkaterProfile );

#if 0
	// copy it to the server's player info data
	uint8* pTempBuffer = new uint8[vMAX_APPEARANCE_DATA_SIZE];
	uint32 size = pSkaterProfile->WriteToBuffer(pTempBuffer, vMAX_APPEARANCE_DATA_SIZE);
	Dbg_Printf("Appearance data size is %d bytes\n", size);
	mp_SkaterProfile = new Obj::CSkaterProfile;
	Dbg_Assert( mp_SkaterProfile );
	mp_SkaterProfile->ReadFromBuffer(pTempBuffer);
	delete pTempBuffer;
#endif

	// now uses assignment operator to copy over the info as well
	mp_SkaterProfile = new Obj::CSkaterProfile;
	*mp_SkaterProfile = *pSkaterProfile;

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	PlayerInfo::IsLocalPlayer( void )
{
	return m_flags.TestMask( mLOCAL_PLAYER );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	PlayerInfo::IsServerPlayer( void )
{
	return m_flags.TestMask( mSERVER_PLAYER );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	PlayerInfo::IsObserving( void )
{
	return m_flags.TestMask( mOBSERVER );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	PlayerInfo::IsSurveying( void )
{
	return m_flags.TestMask( mSURVEYING );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::MarkAsRestarting( void )
{
	m_flags.SetMask( mRESTARTING );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


void	PlayerInfo::MarkAsNonCollidable( void )
{
	m_last_bail_time = Tmr::GetTime();
	return m_flags.SetMask( mNON_COLLIDABLE );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::ClearNonCollidable( void )
{
	return m_flags.ClearMask( mNON_COLLIDABLE );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	PlayerInfo::IsNonCollidable( void )
{
	Mdl::Skate * skate_mod =  Mdl::Skate::Instance();

	if( !m_Conn->TestStatus( Net::Conn::mSTATUS_READY ))
	{
		return true;
	}

	if( m_Skater->IsInWorld() == false )
	{
		return true;
	}

	// Make skaters invulnerable for the first 10 frames of the game
	if( m_Skater->GetStateHistory()->GetNumPosUpdates() < vINVULNERABILITY_PERIOD )
	{
		return true;
	}

	if(( Tmr::GetTime() - m_last_bail_time ) < Tmr::Seconds( vINVULNERABILITY_BAIL_SECONDS ))
	{
		return true;
	}

	// Don't collide with eliminated players in firefight
	if( skate_mod->GetGameMode()->GetNameChecksum() == CRCD(0xbff33600,"netfirefight"))
	{
		Mdl::Score* score_obj;
		Obj::CSkaterScoreComponent* p_score_component = GetSkaterScoreComponentFromObject(m_Skater);
		Dbg_Assert(p_score_component);
		
		score_obj = p_score_component->GetScore();
		if( score_obj->GetTotalScore() <= 0 )
		{
			return true;
		}
	}
	
	// If they've been in this state for more than 5 seconds, that means that
	// for one reason or another, we didn't get their "NotifyBailDone" message.
	// This will rectify things.
	if(( Tmr::GetTime() - m_last_bail_time ) > Tmr::Seconds( 5 ))
	{
		ClearNonCollidable();
		return false;
	}

	if( m_flags.TestMask( mNON_COLLIDABLE ))
	{
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::MarkAsFullyIn( void )
{
	Manager * gamenet_man = Manager::Instance();

	m_flags.SetMask( mFULLY_IN );

	if( gamenet_man->InNetGame() && 
		gamenet_man->OnServer() &&
		!IsLocalPlayer())
	{
		switch( gamenet_man->GetHostMode())
		{
			case vHOST_MODE_SERVE:
				break;
			case vHOST_MODE_AUTO_SERVE:
			{
				Net::Server* server;
				Net::MsgDesc msg_desc;

				server = gamenet_man->GetServer();
				Dbg_Assert( server );

				msg_desc.m_Id = MSG_ID_AUTO_SERVER_NOTIFICATION;
				msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
				msg_desc.m_GroupId = vSEQ_GROUP_PLAYER_MSGS;
				// Tell the client that they're in an auto-serving server
				server->EnqueueMessage( GetConnHandle(), &msg_desc );
				break;
			}
			case vHOST_MODE_FCFS:
			{
				PlayerInfo* player;

				player = gamenet_man->GetServerPlayer();

				// If we don't currently have a player acting as the host
				// make this new player the host
				if( player == NULL )
				{
					gamenet_man->ChooseNewServerPlayer();
				}
				break;
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	PlayerInfo::IsFullyIn( void )
{
	return m_flags.TestMask( mFULLY_IN | mLOCAL_PLAYER );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::MarkAsServerPlayer( void )
{
	m_flags.SetMask( mSERVER_PLAYER );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::MarkAsNotServerPlayer( void )
{
	m_flags.ClearMask( mSERVER_PLAYER );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	PlayerInfo::HasCTFFlag( void )
{
	return m_flags.TestMask( mHAS_ANY_FLAG );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		PlayerInfo::HasWhichFlag( void )
{
	if( m_flags.TestMask( mHAS_RED_FLAG ))
	{
		return 0;
	}
	if( m_flags.TestMask( mHAS_BLUE_FLAG ))
	{
		return 1;
	}
	if( m_flags.TestMask( mHAS_GREEN_FLAG ))
	{
		return 2;
	}
	if( m_flags.TestMask( mHAS_YELLOW_FLAG ))
	{
		return 3;
	}

	return -1;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::ClearCTFState( void )
{
	m_flags.ClearMask( mHAS_ANY_FLAG );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::CapturedFlag( int team )
{   
	Manager * gamenet_man = Manager::Instance();
	PlayerInfo* local_player;
	char team_str[64];
	Script::CStruct* pParams;
		
	sprintf( team_str, "team_%d_name", team + 1 );

	local_player = gamenet_man->GetLocalPlayer();

	// Make sure they have a flag on them
	Dbg_Assert( HasCTFFlag());

	pParams = new Script::CStruct;
	
	if( IsLocalPlayer())
	{
		pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, Script::GetString( team_str ));
		pParams->AddInteger( "team", team );
		Script::RunScript( "captured_flag_you", pParams );
		
	}
	else
	{
		Dbg_Printf( "***** Captured Flag : My Team %d, Flag Team %d\n", local_player->m_Team, team );
		if( ( !local_player->IsObserving()) && 
			( local_player->m_Team == team ))
		{
			Dbg_Printf( "***** Captured Flag 2\n" );
			pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, m_Name );
			pParams->AddInteger( "team", team );
			Script::RunScript( "hide_ctf_arrow" );
			Script::RunScript( "captured_your_flag", pParams );
		}
		else
		{
			pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, m_Name );
			pParams->AddComponent( Script::GenerateCRC("String1"), ESYMBOLTYPE_STRING, Script::GetString( team_str ));
			pParams->AddInteger( "team", team );
			Script::RunScript( "captured_flag_other", pParams );
		}                                                      
	}
	
	m_flags.ClearMask( mHAS_ANY_FLAG );

	delete pParams;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::StoleFlag( int team )
{
	// Shouldn't have any flag already
	Dbg_Assert( !HasCTFFlag());

	Dbg_Printf( "********* In StoleFlag\n" );
	Manager * gamenet_man = Manager::Instance();
	PlayerInfo* local_player, *player;
	Lst::Search< PlayerInfo > sh;
	char team_str[64];
	Script::CStruct* pParams;
		
	sprintf( team_str, "team_%d_name", team + 1 );
	pParams = new Script::CStruct;

	local_player = gamenet_man->GetLocalPlayer();

	
	if( IsLocalPlayer())
	{
		pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, Script::GetString( team_str ));
		Script::RunScript( "stole_flag_you", pParams );
		
	}
	else
	{
		Dbg_Printf( "***** FLAG STOLEN: Team %d, I had %d\n", team, local_player->HasWhichFlag() );
		if( !local_player->IsObserving() && ( local_player->HasWhichFlag() == team ))
		{

			pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, m_Name );
			pParams->AddComponent( Script::GenerateCRC("String1"), ESYMBOLTYPE_STRING, Script::GetString( team_str ));
			Script::RunScript( "stole_flag_from_you", pParams );
		}
		else
		{
			pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, m_Name );
			pParams->AddComponent( Script::GenerateCRC("String1"), ESYMBOLTYPE_STRING, Script::GetString( team_str ));
			Script::RunScript( "stole_flag_other", pParams );
		}
	}

	delete pParams;
	
	for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
	{
		if( player->HasWhichFlag() == team )
		{
			player->m_flags.ClearMask(( mHAS_RED_FLAG << team ));
			player->ClearCTFState();
			Dbg_Assert( !player->HasCTFFlag());
		}
	}

	m_flags.SetMask(( mHAS_RED_FLAG << team ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::TookFlag( int team )
{
	// Shouldn't have any flag already
	Dbg_Assert( !HasCTFFlag());

	Manager * gamenet_man = Manager::Instance();
	PlayerInfo* local_player;
	char team_str[64];
	Script::CStruct* pParams;
		
	sprintf( team_str, "team_%d_name", team + 1 );
	pParams = new Script::CStruct;

	local_player = gamenet_man->GetLocalPlayer();

    if( IsLocalPlayer())
	{
		pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, Script::GetString( team_str ));
		Script::RunScript( "took_flag_you", pParams );
		
	}
	else
	{
		Script::CStruct* arrow_params;

		if(( !local_player->IsObserving()) && ( team == local_player->m_Team ))
		{
			arrow_params = new Script::CStruct;
			arrow_params->AddInteger( "team", local_player->m_Team );
			Script::RunScript( "show_ctf_arrow", arrow_params );
			delete arrow_params;

			pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, m_Name );
			Script::RunScript( "took_flag_yours", pParams );
		}
		else
		{
			pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, m_Name );
			pParams->AddComponent( Script::GenerateCRC("String1"), ESYMBOLTYPE_STRING, Script::GetString( team_str ));
			Script::RunScript( "took_flag_other", pParams );
		}
	}

	delete pParams;
	m_flags.SetMask(( mHAS_RED_FLAG << team ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::RetrievedFlag( void )
{
	Manager * gamenet_man = Manager::Instance();
	GameNet::PlayerInfo* player, *local_player;
	Lst::Search< PlayerInfo > sh;
	char team_str[64];
	Script::CStruct* pParams;
		
	sprintf( team_str, "team_%d_name", m_Team + 1 );
	pParams = new Script::CStruct;

	if( IsLocalPlayer())
	{
		pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, Script::GetString( team_str ));
		pParams->AddInteger( "team", m_Team );
		Script::RunScript( "hide_ctf_arrow" );
		Script::RunScript( "retrieved_flag_you", pParams );
		
	}
	else
	{
		local_player = gamenet_man->GetLocalPlayer();
		if( local_player && !local_player->IsObserving())
		{
			if( local_player->m_Team == m_Team )
			{
				Script::RunScript( "hide_ctf_arrow" );
			}
		}
		pParams->AddComponent( Script::GenerateCRC("String0"), ESYMBOLTYPE_STRING, m_Name );
		pParams->AddComponent( Script::GenerateCRC("String1"), ESYMBOLTYPE_STRING, Script::GetString( team_str ));
		pParams->AddInteger( "team", m_Team );
		Script::RunScript( "retrieved_flag_other", pParams );
	}

	for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
	{
		if( player->HasWhichFlag() == m_Team )
		{
			player->ClearCTFState();
			break;

		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::MarkAsKing( bool mark )
{    
	Manager * gamenet_man = Manager::Instance();
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
    
	if( mark )
	{   
		PlayerInfo* former_king;

		// There can only be one king, so dethrone the last
		former_king = gamenet_man->GetKingOfTheHill();
		if( former_king )
		{
			former_king->MarkAsKing( false );
		}

		if( !m_flags.TestMask( mKING_OF_THE_HILL ))
		{
			m_flags.SetMask( mKING_OF_THE_HILL );

			if( IsLocalPlayer())
			{
				Script::CStruct* pParams;

				pParams = new Script::CStruct;
				if( m_Skater->GetHeapIndex() == 0 )
				{
					pParams->AddChecksum( "player_1", Script::GenerateCRC( "player_1"));
				}
				else
				{
					pParams->AddChecksum( "player_2", Script::GenerateCRC( "player_2"));
				}
				// If we just picked up the crown, hide the arrow
				Script::RunScript( "hide_crown_arrow", pParams );
				delete pParams;
				
				/*gamenet_man->CreateNetPanelMessage( false, Script::GenerateCRC("net_message_new_king_you"),
													NULL, NULL, m_Skater );*/
				Script::RunScript( "NewKingYou" );
				
			}
			else
			{
				Script::CStruct* pParams;

				pParams = new Script::CStruct;
				pParams->AddChecksum( "player_1", Script::GenerateCRC( "player_1"));
				// If someone else just picked up the crown, show the arrow
				Script::RunScript( "show_crown_arrow", pParams );
				
				pParams->AddString( "String0", m_Name );
				Script::RunScript( "NewKingOther", pParams );
				/*gamenet_man->CreateNetPanelMessage( false, Script::GenerateCRC( "net_message_new_king_other"),
													m_Name, NULL );*/
				delete pParams;

				
			}

			if( !gamenet_man->InNetGame())
			{
				int other_id;
				PlayerInfo* other_player;

				// NOTE: This code only works for 2-player splitscreen. If we ever go to four,
				// it needs to be more sophisticated
				if( m_Skater->GetID() == 0 )
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
					Script::CStruct* pParams;

					pParams = new Script::CStruct;
					pParams->AddString( "String0", (char*)m_Skater->GetDisplayName());
					Script::RunScript( "NewKingOther", pParams );
					/*gamenet_man->CreateNetPanelMessage( false, Script::GenerateCRC("net_message_new_king_other"),
														(char*)m_Skater->GetDisplayName(), NULL, other_player->m_Skater );*/
					delete pParams;
				}
			}
			
			
			m_flags.SetMask( mKING_OF_THE_HILL );
		}
        
        Obj::CCrown* crown;
		crown = gamenet_man->GetCrown();
		if( crown )
		{
			crown->PlaceOnKing( m_Skater );
		}
	}
	else
	{
		if( m_flags.TestMask( mKING_OF_THE_HILL ))
		{
			if( IsLocalPlayer())
			{
				// If we lost the crown, show the arrow again, but only if we're still in koth mode
				if( skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netking" ) ||
					skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "king" ))
				{
					Script::CStruct* pParams;

					pParams = new Script::CStruct;
					pParams->AddChecksum( "player_1", Script::GenerateCRC( "player_1"));
					pParams->AddChecksum( "player_2", Script::GenerateCRC( "player_2"));

					Script::RunScript( "show_crown_arrow", pParams );
					delete pParams;
				}
			}
			m_flags.ClearMask( mKING_OF_THE_HILL );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	PlayerInfo::IsKing( void )
{
	return m_flags.TestMask( mKING_OF_THE_HILL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::MarkAsNotReady( int time )
{
	if( m_Conn && m_Conn->IsRemote())
	{
		if( time != 0 )
		{
			m_latest_ready_query = time;
		}
		m_Conn->ClearStatus( Net::Conn::mSTATUS_READY );
		m_Conn->SetStatus( Net::Conn::mSTATUS_BUSY );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::SetReadyQueryTime( int time )
{
	m_latest_ready_query = time;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::MarkAsReady( int time )
{
	if( m_latest_ready_query == time )
	{
		if( m_Conn )
		{   
			m_Conn->ClearStatus( Net::Conn::mSTATUS_BUSY );
			m_Conn->SetStatus( Net::Conn::mSTATUS_READY );

			// Now that they're ready, we need to keep track of how many times we're resending to
			// them so that we can detect bad connections
			m_Conn->ClearNumResends();
		}
		m_latest_ready_query = 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		PlayerInfo::IsVulnerable( void )
{
	return (( Tmr::GetTime() - m_last_hit_time ) > vPROJECTILE_INVULNERABILITY_INTERVAL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		PlayerInfo::ResetProjectileVulnerability( void )
{
	m_last_hit_time = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		PlayerInfo::SetHitTime( Tmr::Time hit_time )
{
	m_last_hit_time = hit_time;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		PlayerInfo::GetConnHandle( void )
{
	if( m_Conn )
	{
		return m_Conn->GetHandle();
	}
	return Net::Conn::vHANDLE_INVALID;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		PlayerInfo::GetSkaterNumber( void )
{
	Dbg_Assert( m_Skater );

	return m_Skater->GetSkaterNumber();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		PlayerInfo::GetLastObjectUpdateID( void )
{
	return m_last_object_update_id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	PlayerInfo::SetLastObjectUpdateID( int id )
{
	m_last_object_update_id = id;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		PlayerInfo::GetMaxObjectUpdates( void )
{
	if( m_Conn->GetBandwidthType() == Net::Conn::vBROADBAND )
	{
		return 2;
	}
	
	return 1;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	PlayerInfo::IsPendingPlayer( void )
{
	return m_flags.TestMask( mPENDING_PLAYER );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

NewPlayerInfo::NewPlayerInfo(void)
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());

	mpSkaterProfile = new Obj::CSkaterProfile;
	JumpInFrame = 0;
	Profile = 0;
	Rating = 0;
	Score = 0;
	VehicleControlType = 0;
	
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

NewPlayerInfo::~NewPlayerInfo(void)
{
	

	Dbg_Assert( mpSkaterProfile );

	delete mpSkaterProfile;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace GameNet




