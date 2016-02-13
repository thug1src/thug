//****************************************************************************
//* MODULE:         Sk/Modules/Skate
//* FILENAME:       GoalManager.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  1/24/2001
//****************************************************************************

// start autoduck documentation
// @DOC goalmanager
// @module goalmanager | None
// @subindex Scripting Database
// @index script | goalmanager
							   
#include <sk/modules/skate/goalmanager.h>

#include <sk/modules/skate/Minigame.h>
#include <sk/modules/skate/CompetitionGoal.h>
#include <sk/modules/skate/NetGoal.h>
#include <sk/modules/skate/RaceGoal.h>
#include <sk/modules/skate/BettingGuy.h>
#include <sk/modules/skate/SkatetrisGoal.h>
#include <sk/modules/skate/gamemode.h>
#include <sk/modules/skate/HorseGoal.h>
#include <sk/modules/skate/FindGapsGoal.h>
#include <sk/modules/skate/FilmGoal.h>
#include <sk/modules/skate/CatGoal.h>

#include <core/string/stringutils.h>

#include <gfx/2D/ScreenElemMan.h>       // for tetris tricks
#include <gfx/2D/ScreenElement2.h>
#include <gfx/2D/TextElement.h>
#include <gfx/2D/SpriteElement.h>

#include <gel/scripting/script.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/string.h>
#include <gel/scripting/component.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>

#include <gel/music/music.h>
#include <gel/net/client/netclnt.h>
#include <gel/components/trickcomponent.h>
#include <gel/object/compositeobjectmanager.h>


#include <sk/gamenet/gamenet.h>
								
#include <sk/objects/skater.h>
#include <sk/objects/trickobject.h>
#include <sk/objects/skaterprofile.h>
#include <sk/objects/skatercareer.h>
#include <sk/objects/playerprofilemanager.h>


#include <sk/modules/skate/score.h>
#include <sk/modules/skate/skate.h>
#include <sk/modules/skate/competition.h>
#include <sk/scripting/nodearray.h>

#include <sk/scripting/cfuncs.h>
#include <sk/scripting/skfuncs.h>

namespace Game
{

	extern bool fill_trick_and_key_combo_arrays( Script::CArray* p_key_combos, Script::CArray* p_key_combo_strings, Script::CArray* p_trick_names, int premade_cat_index );
	extern void find_and_replace_trick_string( const char* p_old, char* p_out, Script::CArray* p_key, Script::CArray* p_trick, uint out_buffer_size );
	
// TODO:
// Uber time limit should be its own goal?
// Each of the skate letters can either be its own goal
// or maybe we can grab the goal state in script somehow

// maybe there's some kind of goal factory

// TODO:  Maybe the script should be spawned so that it can wait a while...
// or maybe it could run a script that spawns a script...
// or maybe a combination of the two?

// TODO:  Edit goal.  changes the state of the goal, maybe to track flags and such?
// or maybe a thing can be made of 5 subgoals?

// TODO:  Need a way of binding the goal to a specific player 

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
/*****************************************************************************
**								Public Functions							**
*****************************************************************************/


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CGoalManager::CGoalManager()
{
    m_lastGoal = 0;
    m_graffitiMode = 0;
    m_goalPoints = 0;
	m_totalGoalPointsEarned = 0;
    m_cash = 0;
	m_numGoalsBeaten = 0;
	m_isPro = false;
	// m_proChallengesUnlocked = false;
	m_canStartGoal = true;

	m_currentChapter = 0;
	m_currentStage = 0;
	m_sponsor = 0;
	mp_team = new Script::CStruct();

    mp_goalFlags = new Script::CStruct();
	// mp_proSpecificChallenges = new Script::CStruct();
	// mp_proSpecificChallenges->AppendStructure( Script::GetStructure( "goal_pro_specific_challenges_beaten", Script::ASSERT ) );

	mp_difficulty_levels = new Script::CStruct();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CGoalManager::~CGoalManager()
{
	// delete all the goals in the list
	RemoveAllGoals();
    delete mp_goalFlags;
	// delete mp_proSpecificChallenges;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::AddGoal( uint32 goalId, Script::CStruct* pParams )
{
	// don't add non-minigames in free skate
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	bool free_skate = skate_mod->GetGameMode()->IsTrue( CRCD(0x337e7779,"is_freeskate") );
	if ( !CFuncs::ScriptInMultiplayerGame( NULL, NULL ) && free_skate && !pParams->ContainsFlag( CRCD(0x6bae094c,"minigame") ) )
		return false;

	if ( skate_mod->GetGameMode()->IsTrue( CRCD( 0x353961d7, "is_creategoals" ) ) && !pParams->ContainsFlag( CRCD( 0x981d3ad0, "edited_goal" ) ) )
		return false;

	// printf("addGoal called with goalId %x\n", goalId);
	
	Dbg_MsgAssert( !m_goalList.GetItem( goalId ), ( "duplicate goal id 0x%x (%s), please check your script", goalId, Script::FindChecksumName( goalId ) ) );
	pParams->AddChecksum( CRCD(0x9982e501,"goal_id"), goalId );
	
	// figure the type
	CGoal* pGoal = NULL;
	if ( pParams->ContainsFlag( CRCD(0xc18fef36,"career_only") ) )
	{   
		if( CFuncs::ScriptInMultiplayerGame( NULL, NULL ))
		{
			return false;
		}
	}
	if ( pParams->ContainsFlag( CRCD(0x4af5d34e,"competition") ) )
	{
		// printf("adding comp goal\n");
		pGoal = new CCompetitionGoal( pParams );
	}
	else if ( pParams->ContainsFlag( CRCD(0xd15ea00,"net") ) )
	{
		// printf("adding net goal\n");
		pGoal = new CNetGoal( pParams );
	}
	else if ( pParams->ContainsFlag( CRCD(0x25904450,"race") ) )
	{
		// printf("adding race goal\n");
		pGoal = new CRaceGoal( pParams );
	}
	else if ( pParams->ContainsFlag( CRCD(0x6bae094c,"minigame") ) )
	{
		// printf("adding minigame\n");
		pGoal = new CMinigame( pParams );
	}
	else if ( pParams->ContainsFlag( CRCD(0x8cfee956,"betting_guy") ) )
	{
		// printf("adding betting guy\n");
		pGoal = new CBettingGuy( pParams );
	}
	else if ( pParams->ContainsFlag( CRCD(0x4147922b,"tetris") ) )
	{
		// printf("adding skatetris goal\n");
		pGoal = new CSkatetrisGoal( pParams );
	}
	else if ( pParams->ContainsFlag( CRCD(0x9d65d0e7,"horse") ) )
	{
		// printf("adding horse goal\n");
		pGoal = new CHorseGoal( pParams );
	}
	else if ( pParams->ContainsFlag( CRCD( 0xc5ec08e6, "findGaps" ) ) )
	{
		pGoal = new CFindGapsGoal( pParams );
	}
	else if ( pParams->ContainsFlag( CRCD( 0x7dbb41dd, "Film" ) ) )
	{
		pGoal = new CFilmGoal( pParams );
	}
	else if ( pParams->ContainsFlag( CRCD( 0x8e6014f6, "create_a_trick" ) ) )
	{
		pGoal = new CCatGoal( pParams );
	}
	else
	{
		pGoal = new CGoal( pParams );
	}
	Dbg_Assert( pGoal );

	m_goalList.PutItem( goalId, pGoal );

	pGoal->AppendDifficultyLevelParams();
	Script::CStruct* pGoalParams = pGoal->GetParams();

	// add children
	uint32 childId;
	Script::CArray* p_children;
	if ( pGoalParams->GetArray( CRCD( 0x5e684e45, "children" ), &p_children, Script::NO_ASSERT ) )
	{
		int size = p_children->GetSize();
		Dbg_MsgAssert( size > 0, ( "0 size children array for goal %s", Script::FindChecksumName( goalId ) ) );
		Dbg_MsgAssert( p_children->GetType() == ESYMBOLTYPE_NAME, ( "children array has wrong type for goal %s", Script::FindChecksumName( goalId ) ) );
		for ( int i = 0; i < size; i++ )
		{
			FindCircularLinks( goalId, p_children->GetChecksum( i ) );
			pGoal->AddChild( p_children->GetChecksum( i ) );
		}
	}
	else if ( pGoalParams->GetChecksum( CRCD( 0xdd4cabd6, "child" ), &childId, Script::NO_ASSERT ) )
	{
		FindCircularLinks( goalId, childId );
		pGoal->AddChild( childId );
	}
	
	if ( CFuncs::ScriptInMultiplayerGame( NULL, NULL ))
	{
		// SKATE SPECIFIC
		pGoal->GetParams()->AddString( CRCD(0x944b2900,"pro"), "NetJudge" );
		pGoal->GetParams()->AddString( CRCD(0x243b9c3b,"full_name"), Script::GetString( CRCD(0xa500bc5f,"judge_full_name") ) );
		// END SKATE SPECIFIC
	}
	else
	{
		// SKATE SPECIFIC
		const char* pFirstName;
		uint32 pro_name;
		if ( pGoal->GetParams()->GetString( CRCD(0x944b2900,"pro"), &pFirstName, Script::NO_ASSERT ) )
		{
			// printf("pro string: %s\n", pFirstName );
			Script::CStruct* pLastNames = Script::GetStructure( CRCD(0x621d1828,"goal_pro_last_name_checksums") );
			if ( pLastNames->GetChecksum( Script::GenerateCRC( pFirstName ), &pro_name, Script::NO_ASSERT ) )
			{
				// printf("got a last name: %s\n", Script::FindChecksumName( pro_name ) );
				// get the current skater's name
				Mdl::Skate * skate_mod = Mdl::Skate::Instance();
				Obj::CPlayerProfileManager* pPlayerProfileManager = skate_mod->GetPlayerProfileManager();
				Obj::CSkaterProfile* pSkaterProfile = pPlayerProfileManager->GetCurrentProfile();
				uint32 current_skater = pSkaterProfile->GetSkaterNameChecksum();
				// printf("current_skater: %s\n", Script::FindChecksumName( current_skater ) );
				if ( current_skater == pro_name )
				{
					const char* p_generic_pro_first_name = Script::GetString( CRCD(0x2245b93c,"generic_pro_first_name"), Script::ASSERT );
					pGoal->GetParams()->AddString( CRCD(0x944b2900,"pro"), p_generic_pro_first_name );
					const char* p_generic_pro_full_name = Script::GetString( CRCD(0xfe6ef7b8,"generic_pro_full_name"), Script::ASSERT );
					pGoal->GetParams()->AddString( CRCD(0x243b9c3b,"full_name"), p_generic_pro_full_name );					
				}
			}
		}
		// END SKATE SPECIFIC
	}

	return ( pGoal != NULL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGoal* CGoalManager::GetGoal( uint32 goalId, bool assert )
{
	CGoal* pGoal = m_goalList.GetItem( goalId );
	if ( assert )
		Dbg_MsgAssert( pGoal, ( "Could not find goal %s\n", Script::FindChecksumName( goalId ) ) );
	
	return pGoal;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGoal* CGoalManager::GetGoal( Script::CStruct* pParams, bool assert )
{
	uint32 goalId = GetGoalIdFromParams( pParams, assert );
	if ( goalId )
		return GetGoal( goalId, assert );
	return NULL;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGoal*	CGoalManager::GetGoalByIndex( int index )
{
	return m_goalList.GetItemByIndex( index, NULL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::RemoveGoal( uint32 goalId )
{
	// TODO:  Maybe add an "AssertOnFail" flag

	CGoal* pGoal = m_goalList.GetItem( goalId );
	Dbg_Assert( pGoal );
		
	pGoal->mp_goalPed->DestroyGoalPed();
	m_goalList.FlushItem( goalId );

	if (m_lastGoal == goalId)
	{
		m_lastGoal=0;
	}
		
	// The FlushItem does not delete the CGoal* itself, it only deletes its own entry for
	// it (which is a LookupItem<CGoal>*) so we need to delete the pGoal here.
	delete pGoal;	
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::ActivateGoal( uint32 goalId, bool dontAssert )
{
	CGoal* pGoal = GetGoal( goalId );

    bool success = false;

	if ( !pGoal )
	{
		if ( dontAssert )
			return false;			
		else
			Dbg_MsgAssert( 0, ( "ActivateGoal could not find the goal" ) );
	}
	
    if ( pGoal )
    {
		success = pGoal->Activate();
		// set the last goal
        if ( success && pGoal->CanRetry() )
		{
			// only set this for the root node
			// BB - changed!  the retry goal option now
			// always gives you the last stage.
			
			// Game::CGoalLink* p_parents = pGoal->GetParents();
			// if ( p_parents->m_relative == 0 )
			m_lastGoal = goalId;
		}
    }
	
    // move it back and forth from the inactive to the active list
	// TODO:  Maybe add an "AssertOnFail" flag
	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::DeactivateGoal( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	if ( pGoal )
	{
		pGoal->Deactivate();
	}

	// move it back and forth from the inactive to the active list
	// would be an optimization?

 	// TODO:  Maybe add an "AssertOnFail" flag

	return ( pGoal != NULL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::ClearLastGoal( void )
{
	m_lastGoal = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::WinGoal( uint32 goalId )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	if ( pGoal )
	{
        // don't let them retry this goal
		// only if it's a leaf node
        if ( pGoal->GetChildren()->m_relative == 0 )
			m_lastGoal = 0;

		// we always want to call win, but if they
		// already beat it, we don't want to give them 
		// double credit
		if ( !pGoal->HasWonGoal() )
		{
			// Report winning leaf node goals
			if( ( pGoal->GetChildren()->m_relative == 0 ) &&
				( gamenet_man->InNetGame()))
			{
				Net::Client* client;
				Net::MsgDesc msg_desc;

				Dbg_Printf( "*** Reporting won goal\n" );
				// TODO: This should use the appropriate client for the skater completing this trick.
				// Assuming client Zero will probably cause problems in split-screen games
				client = gamenet_man->GetClient( 0 );
				Dbg_Assert( client );

				GameNet::MsgBeatGoal msg;

				msg.m_GameId = gamenet_man->GetNetworkGameId();
				msg.m_GoalId = pGoal->GetRootGoalId();
				
				Dbg_Printf( "**** Beat GOAL 0x%x  ROOT: 0x%x\n", pGoal->GetGoalId(), pGoal->GetRootGoalId());

				msg_desc.m_Data = &msg;
				msg_desc.m_Length = sizeof( GameNet::MsgBeatGoal );
				msg_desc.m_Id = GameNet::MSG_ID_BEAT_GOAL;
				msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
				msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
				client->EnqueueMessageToServer( &msg_desc );
			}
			else
			{
				// increase num goals beaten if this is a leaf node.
				if ( pGoal->GetChildren()->m_relative == 0 )
					if ( !pGoal->GetParams()->ContainsFlag( CRCD(0x981d3ad0,"edited_goal") ) )
						m_numGoalsBeaten++;
			}
			
		}
		
		// In net games, "won" goals need to be approved by the server
		// Update: Win goals locally now to close the window where time can expire waiting for server's approval
		pGoal->Win();
	}

	/*
	if( gamenet_man->InNetGame() == false )
	{
		if ( !m_isPro && ( m_numGoalsBeaten >= m_numGoalsToPro ) )
		{
			TurnPro();
		}
	}
	*/

	return ( pGoal != NULL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::LoseGoal( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	// expire takes priority over fail
	if ( pGoal )
	{
		pGoal->Lose();
	}

	return ( pGoal != NULL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::EditGoal( uint32 goalId, Script::CStruct* pParams )
{
	CGoal* pGoal = GetGoal( goalId );

	if ( pGoal )
	{
		pGoal->EditParams( pParams );
	}
			 
	// should edit the state of the goal here...
	
	// TODO:  Maybe add an "AssertOnFail" flag

	return ( pGoal != NULL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::SetGoalTimer( uint32 goalId, int time )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	if ( pGoal )
	{
		if (time == -1)
		{
			pGoal->SetTimer();
		}
		else
		{
			pGoal->SetTimer(time);
		}
	}
			 
	// should edit the state of the goal here...
	
	// TODO:  Maybe add an "AssertOnFail" flag

	return ( pGoal != NULL );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::GoalIsActive( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId, false );

	if ( pGoal )
	{
		return pGoal->IsActive();
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CGoalManager::GetActiveGoal( bool ignore_net_goals )
{
	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );

		if( ignore_net_goals && pGoal->IsNetGoal())
		{
			continue;
		}
		if ( pGoal->IsActive() )
		{
            return i;
        }
    }
    return -1;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CGoalManager::GetNumGoals()
{
	return m_goalList.getSize();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CGoalManager::GetNumActiveGoals( bool count_all )
{
	int numActiveGoals = 0;

	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );

		if( pGoal->IsNetGoal() && !count_all )
		{
			continue;
		}
		if ( count_all && pGoal->IsActive() )
			numActiveGoals++;
		else if ( pGoal->CountAsActive() )
			numActiveGoals++;
	}
	return numActiveGoals;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CGoalManager::GetNumSelectedGoals()
{
	int num_selected = 0;

	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );

		// this is now primarily used for determining if the timer should be on-screen
		if ( pGoal->IsSelected() )
		{
			num_selected++;
		}
	}
	return num_selected;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::RemoveAllGoals()
{
	// can't retry!
    m_lastGoal = 0;
    
	// kill the ped
	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
		
		pGoal->mp_goalPed->DestroyGoalPed();
	}

	// delete all the goals in the list
	Lst::LookupTableDestroyer<CGoal> destroyer( &m_goalList );	
	destroyer.DeleteTableContents();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::DeactivateAllGoals( bool include_net_goals )
{
	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
		if ( pGoal->IsActive() )
		{
			pGoal->Deactivate( include_net_goals );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::UninitializeGoal( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );
	pGoal->Uninit();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::UninitializeGoalTree( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );
	CGoalLink* pGoalLink = pGoal->GetParents();
	while ( pGoalLink && pGoalLink->m_relative != 0 )
	{
		pGoal = GetGoal( pGoalLink->m_relative );
		Dbg_Assert( pGoal );
		pGoalLink = pGoal->GetParents();
	}
	
	// the resulting goal should be the root node of the family tree
	pGoal->Uninit( true );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::UninitializeAllGoals()
{
	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
		pGoal->Uninit();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::DeactivateAllMinigames()
{
	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
		if ( pGoal->IsMinigame() && pGoal->IsActive() )
			pGoal->Deactivate();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::InitializeAllMinigames()
{
	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
		if( pGoal->IsMinigame())
		{
			pGoal->RunCallbackScript( vINIT );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::InitializeAllGoals()
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
			
		if( gamenet_man->InNetGame() && !pGoal->IsNetGoal())
		{
			continue;
		}

		if ( !pGoal->IsLocked() && !pGoal->HasWonGoal() )
		{
			// check if this is a child of another goal
			CGoalLink* pGoalParents = pGoal->GetParents();

			// support an always_initialize_goal flag, which
			// allows designers to have goals that aren't tied to
			// a chapter/stage
			Script::CStruct* pGoalParams = pGoal->GetParams();
			
			int goal_chapter = pGoal->GetChapter();
			int goal_stage = pGoal->GetStage();
			
			// printf("%s: %i.%i\n", Script::FindChecksumName( pGoal->GetGoalId() ), goal_chapter, goal_stage );
			if ( pGoalParents->m_relative == 0 )
			{
				// if the goal isn't listed in chapter_info.q at all,
				// the chapter and stage will default to -1.
				// Always create these goals
				if ( ( goal_chapter == m_currentChapter && goal_stage == m_currentStage ) 
					 || pGoalParams->ContainsFlag( CRCD( 0x4e863e93, "always_initialize_goal" ) )
					 || pGoalParams->ContainsFlag( CRCD( 0x981d3ad0, "edited_goal" ) ) )
				{
					pGoal->Init();
				}
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::InitializeAllSelectedGoals()
{
	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
			
		if ( pGoal->IsSelected())
		{       
			Script::RunScript( Script::GenerateCRC( "goal_create_proset_geom" ), pGoal->GetParams());
			pGoal->Init();
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::DeselectAllGoals()
{
	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
			
		pGoal->Deselect();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// TODO: make this smarter...it just deactivates the first
// non-minigame goal it finds
void CGoalManager::DeactivateCurrentGoal()
{
	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
		if ( pGoal->IsActive() && !pGoal->IsNetGoal() && !pGoal->IsMinigame() )
		{
			pGoal->Deactivate();
			return;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::Update()
{
	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );

		if ( pGoal->IsActive() )
		{
            pGoal->Update();
        }
    }

	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );

        if ( pGoal->IsInvalid() )
        {
            //Dbg_Printf("************** removing goal %p\n", pGoal->GetGoalId());
            m_goalList.FlushItem( pGoal->GetGoalId() );
            delete pGoal;
        }
/*        else 
            {
                pGoal->Update();
            }
*/
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::HasSeenGoal( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );

	if ( pGoal )
	{
		return pGoal->HasSeenGoal();
	}

	return false;

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::PauseAllGoals()
{
    for ( int i = 0; i < GetNumGoals(); i++ )
    {
        uint32 key;
        CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
        Dbg_Assert( pGoal );
    
        pGoal->PauseGoal();
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::UnPauseAllGoals()
{
    for ( int i = 0; i < GetNumGoals(); i++ )
    {
        uint32 key;
        CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
        Dbg_Assert( pGoal );
    
        pGoal->UnPauseGoal();
    }
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Tmr::Time CGoalManager::GetGoalTime()
{
    // TODO: allow some sort of goal priority 
    // right now this just finds the fist active goal
    // and returns that timer
    for (int i = 0; i < GetNumGoals(); i++) {
        uint32 key;
        CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
        Dbg_Assert( pGoal );
        
        if ( pGoal->IsActive() && pGoal->ShouldUseTimer() && !pGoal->IsExpired() )
        {
            Tmr::Time timeLeft = pGoal->GetTime();
            if ( (int)timeLeft < 0 )
                return 0;
            return timeLeft;
        }
    }
    // there are no active goals.
    return 0;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::CanRetryGoal()
{
    if ( m_lastGoal != 0 )
	{
		CGoal* pGoal = GetGoal( m_lastGoal );
		return pGoal->CanRetry();

	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::QuickStartGoal( uint32 goalId, bool dontAssert )
{
	CGoal* pGoal = GetGoal( goalId );
	if ( !pGoal )
	{
		if ( dontAssert )
			return false;
		else
			Dbg_MsgAssert( 0, ( "QuickStartGoal unable to find goal" ) );
	}
	pGoal->SetQuickStartFlag();
	bool success = pGoal->Activate();
	
	// set the last goal
	if ( success && pGoal->CanRetry() )
		m_lastGoal = goalId;

	pGoal->UnsetQuickStartFlag();
	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::RestartLastGoal() 
{
	if ( m_lastGoal != 0 )
    {
		CGoal* pGoal = GetGoal( m_lastGoal );
		Dbg_Assert( pGoal );

		// set the restart flag so the (de)activate scripts knows not to show
		// cam anims, etc.
		pGoal->SetQuickStartFlag();

		// if ( pGoal->IsActive() )
			// pGoal->Deactivate();
		DeactivateAllGoals();

		// SKATE SPECIFIC
		// kill any sk3_killscripts, as they will screw up the reset skater call
		Script::KillSpawnedScriptsThatReferTo( ( CRCD(0xdea3057b,"SK3_KillSkater") ) );
		Script::KillSpawnedScriptsThatReferTo( ( CRCD(0xb38ed6b,"SK3_Killskater_Finish") ) );
		// END SKATE SPECIFIC

		// kill any blur effect
		Script::RunScript(( CRCD(0xbda66e7d,"kill_blur") ) );
		
		// uint32 reset_checksum = 0;
        // pGoal->GetParams()->GetChecksum( "restart_node", &reset_checksum, Script::ASSERT );        
        // Mdl::Skate * p_skate = Mdl::Skate::Instance();
        // p_skate->ResetSkaters( SkateScript::FindNamedNode( reset_checksum ) );

		// clear the trigger exceptions
		if ( !pGoal->HasWonGoal() )
		{
			Script::CStruct* p_params = new Script::CStruct();
			p_params->AddChecksum( "goal_id", pGoal->GetGoalId() );
			Script::RunScript( CRCD( 0x17c2e09f, "GoalManager_ResetGoalTrigger" ), p_params );
			delete p_params;
		}
		
		pGoal->Activate();
		pGoal->UnsetQuickStartFlag();
    }
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::RestartStage()
{
    int activeGoal = GetActiveGoal();
	if ( activeGoal == -1 )
		return;

	uint32 key;
	CGoal* pGoal = m_goalList.GetItemByIndex( activeGoal, (int*)&key );
	Dbg_Assert( pGoal );

	pGoal->SetQuickStartFlag();

	if ( pGoal->IsActive() )
	{
		pGoal->Deactivate( false, false );

		// SKATE SPECIFIC
		// kill any sk3_killscripts, as they will screw up the reset skater call
		Script::KillSpawnedScriptsThatReferTo(  CRCD(0xdea3057b,"SK3_KillSkater")  );
		Script::KillSpawnedScriptsThatReferTo( ( CRCD(0xb38ed6b,"SK3_Killskater_Finish") ) );
		// END SKATE SPECIFIC

		// kill any blur effect
		Script::RunScript( ( CRCD(0xbda66e7d,"kill_blur") ) );
		
		// uint32 reset_checksum = 0;
        // pGoal->GetParams()->GetChecksum( "restart_node", &reset_checksum, Script::ASSERT );        
        // Mdl::Skate * p_skate = Mdl::Skate::Instance();
        // p_skate->ResetSkaters( SkateScript::FindNamedNode( reset_checksum ) );

		// clear the trigger exceptions
		if ( !pGoal->HasWonGoal() )
		{
			Script::CStruct* p_params = new Script::CStruct();
			p_params->AddChecksum( "goal_id", pGoal->GetGoalId() );
			Script::RunScript( CRCD( 0x17c2e09f, "GoalManager_ResetGoalTrigger" ), p_params );
			delete p_params;
		}
		
		pGoal->Activate();
		pGoal->UnsetQuickStartFlag();
    }
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*void CGoalManager::CreateGoalFlag( uint32 goalId, uint32 flag )
{
	CGoal* pGoal = GetGoal( goalId );
    Dbg_Assert( pGoal );
    
    pGoal->CreateGoalFlag( flag );
}*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::SetGoalFlag( uint32 goalId, uint32 flag, int value )
{
    CGoal* pGoal = GetGoal( goalId );

    if ( !pGoal )
        return false;
    if ( pGoal->SetGoalFlag( flag, value ) ) return true;
    
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::AllFlagsSet( uint32 goalId )
{
    CGoal* pGoal = GetGoal( goalId );

    Dbg_Assert( pGoal );
    return pGoal->AllFlagsSet();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::GoalFlagSet( uint32 goalId, uint32 flag )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );
	return pGoal->GoalFlagSet( flag );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CGoalManager::CreatedGapGoalIsActive()
{
	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* p_goal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( p_goal );
		
		if (p_goal->IsActive())
		{
			Script::CStruct *p_params=p_goal->GetParams();
			Script::CArray *p_dummy=NULL;
			if (p_params->GetArray(CRCD(0x52d4489e,"required_gaps"),&p_dummy))
			{
				return true;
			}
		}	
	}	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::GetCreatedGoalGap(int gapIndex)
{
	bool return_value=false;
	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* p_goal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( p_goal );
		
		return_value=p_goal->GetCreatedGoalGap(gapIndex);
	}	
	return return_value;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct* CGoalManager::GetGoalParams( uint32 goalId )
{
	// printf("trying to get params for goalId %x\n", goalId);
	CGoal* pGoal = GetGoal( goalId );
    Dbg_Assert( pGoal );

    return pGoal->GetParams( );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::CanStartGoal()
{
    return m_canStartGoal;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::NextRaceWaypoint( uint32 goalId )
{
    CRaceGoal* pGoal = (CRaceGoal*)GetGoal( goalId );
	Dbg_MsgAssert( pGoal, ( "Couldn't find goal: %x\n", goalId ) );

    return pGoal->NextRaceWaypoint( goalId );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::NextTourSpot( uint32 goalId )
{
    CGoal* pGoal = GetGoal( goalId );
    Dbg_Assert( pGoal );

    return pGoal->NextTourSpot();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::InitTrickObjects( uint32 goalId )
{
    CGoal* pGoal = GetGoal( goalId );
    Dbg_Assert( pGoal );

    pGoal->InitTrickObjects();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::SetGraffitiMode( int mode )
{
    m_graffitiMode = mode;

/*    // do we need to reset the scores?
    if ( mode == 0 )
    {
        Mdl::Skate* skate_mod = Mdl::Skate::Instance();
        Obj::CTrickObjectManager* p_TrickObjectManager = skate_mod->GetTrickObjectManager();
        p_TrickObjectManager->ResetAllTrickObjects();
    }
*/
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::ShouldLogTrickObject()
{
    int activeGoal = GetActiveGoal( true );
	if ( activeGoal == -1 )
		return false;

	uint32 key;
	CGoal* pGoal = m_goalList.GetItemByIndex( activeGoal, (int*)&key );
	Dbg_Assert( pGoal );
	return pGoal->ShouldLogTrickObject();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::GotTrickObject( uint32 clusterId, int score )
{
	int activeGoal = GetActiveGoal( true );
	if ( activeGoal == -1 )	// no active goal
		return;
	
	uint32 key;
	CGoal* pGoal = m_goalList.GetItemByIndex( activeGoal, (int*)&key );
	Dbg_Assert( pGoal );

	if ( pGoal->IsGraffitiGoal() )
		pGoal->GotGraffitiCluster( clusterId, score );
	else
		pGoal->GotTrickObject( clusterId );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::CheckTrickText()
{
    int activeGoal = GetActiveGoal( true );
    
    if ( activeGoal == -1 )
        return;

    uint32 key;
    CGoal* pGoal = m_goalList.GetItemByIndex( activeGoal, (int*)&key );

    Dbg_Assert( pGoal );

    Script::CStruct* p_goal_params = pGoal->GetParams();
	if ( pGoal->IsSpecialGoal() || p_goal_params->ContainsFlag( CRCD(0x8e6014f6,"create_a_trick") ) )
    {		
		// don't check special goals that are setup with gaps
		if ( !p_goal_params->ContainsFlag( "special_gap" ) )
		{
			if ( pGoal->CheckSpecialGoal() )
			{
				uint32 goalId = pGoal->GetGoalId();
				WinGoal( goalId );
			}
		}
    }
    else if ( pGoal->IsTetrisGoal() )
    {
        CSkatetrisGoal* p_temp = (CSkatetrisGoal*)pGoal;
		p_temp->CheckTetrisTricks();
    }
	else if ( pGoal->IsFindGapsGoal() )
	{
		CFindGapsGoal *p_temp = (CFindGapsGoal*)pGoal;
		p_temp->CheckGaps();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::UnlockGoal( uint32 goalId )
{
    CGoal* pGoal = GetGoal( goalId );
    Dbg_MsgAssert( pGoal, ("Couldn't find goal to unlock") );

    pGoal->UnlockGoal();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::HasWonGoal( uint32 goalId )
{
    CGoal* pGoal = GetGoal( goalId, false );

    if ( pGoal )
		return pGoal->HasWonGoal();
	else
	{
		// check goal manager params struct in case the goal was in
		// another level
		int hasWonGoal = 0;
		Script::CStruct* pGoalParams;
		if ( mp_goalFlags->GetStructure( goalId, &pGoalParams, Script::NO_ASSERT ) )
			pGoalParams->GetInteger( CRCD(0x49807745,"hasBeaten"), &hasWonGoal, Script::ASSERT );

		return ( hasWonGoal != 0 );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::GoalIsSelected( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );
    Dbg_Assert( pGoal );

    return pGoal->IsSelected();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::ToggleGoalSelection( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );
    Dbg_Assert( pGoal );

	if( pGoal->IsSelected())
	{
		pGoal->Deselect();
	}
	else
	{
		pGoal->Select();
	}                   
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::GotCounterObject( uint32 goalId )
{
    CGoal* pGoal = GetGoal( goalId );
    Dbg_Assert( pGoal );

    if ( pGoal->IsActive() )
		pGoal->GotCounterObject();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::CounterGoalDone( uint32 goalId )
{
    CGoal* pGoal = GetGoal( goalId );
    Dbg_Assert( pGoal );

    return pGoal->CounterGoalDone();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::SpendGoalPoints( int num )
{
    m_goalPoints -= num;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::AddGoalPoint()
{
    m_goalPoints++;
	m_totalGoalPointsEarned++;
	Script::RunScript( CRCD(0x888ef05e,"GoalManager_ShowGoalPoints") );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::HasGoalPoints( int num )
{
    return ( m_goalPoints >= num );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CGoalManager::GetNumberOfGoalPoints()
{
    return m_goalPoints;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CGoalManager::GetTotalNumberOfGoalPointsEarned()
{
    return m_totalGoalPointsEarned;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::ClearGoalPoints()
{
	m_goalPoints = 0;
	m_totalGoalPointsEarned = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::AddCash( int amount )
{
    m_cash += amount;
	m_totalCash += amount;
	if ( GetNumActiveGoals() < 1 )
	{
		Script::RunScript( CRCD(0x888ef05e,"GoalManager_ShowGoalPoints") );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::SpendCash( int amount )
{
    if ( amount <= m_cash)
    {
        m_cash -= amount;
        return true;
    } 
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CGoalManager::GetCash( bool get_total )
{
    if ( get_total )
		return m_totalCash;
	return m_cash;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::HasBeatenGoalWithProset( const char* proset_prefix )
{
	for ( int i = 0; i < GetNumGoals(); i++ )
    {
        uint32 key;
        CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
        Dbg_Assert( pGoal );

        if ( pGoal->HasWonGoal() && pGoal->HasProset( proset_prefix ) )
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

void CGoalManager::LevelUnload()
{
	// don't do this in net games!
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	if ( gamenet_man->InNetGame() )
		return;
	
	// save goal manager params
	Script::CStruct *p_manager_params = new Script::CStruct();

	p_manager_params->AddInteger( CRCD(0x6abb6fcb,"goalPoints"), m_goalPoints );
	p_manager_params->AddInteger( CRCD(0x79ed60aa,"totalGoalPointsEarned"), m_totalGoalPointsEarned );
	p_manager_params->AddInteger( CRCD(0xf9461a46,"cash"), m_cash );
	p_manager_params->AddInteger( CRCD(0x875e89bb,"total_cash"), m_totalCash );
	// p_manager_params->AddInteger( "numGoalsToPro", m_numGoalsToPro );
	p_manager_params->AddInteger( CRCD(0x46f71d38,"numGoalsBeaten"), m_numGoalsBeaten );

	p_manager_params->AddInteger( CRCD(0xf884773c,"currentChapter"), m_currentChapter );
	p_manager_params->AddInteger( CRCD(0xaf1575eb,"currentStage"), m_currentStage );
	p_manager_params->AddChecksum( CRCD(0x7e73362b,"sponsor"), m_sponsor );
	p_manager_params->AddStructure( CRCD(0x3b1f59e0,"team"), mp_team );
	p_manager_params->AddStructure( CRCD(0xb13d98d3,"difficulty_levels"), mp_difficulty_levels );
	
/*	int proChallengesUnlocked = 0;
	if ( m_proChallengesUnlocked )
		proChallengesUnlocked = 1;
	p_manager_params->AddInteger( CRCD(0x24c226fa,"proChallengesUnlocked"), proChallengesUnlocked );
*/
	int isPro = 0;
	if ( m_isPro )
		isPro = 1;
	p_manager_params->AddInteger( CRCD(0x40801d41,"isPro"), isPro );

	// p_manager_params->AddStructure( CRCD(0xf8478579,"proSpecificChallenges"), mp_proSpecificChallenges );

	mp_goalFlags->AddStructure( CRCD(0x23d4170a,"GoalManager_Params"), p_manager_params );

	delete p_manager_params;

	for ( int i = 0; i < GetNumGoals(); i++ )
    {
        uint32 key;
        CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
        Dbg_Assert( pGoal );
		Script::CStruct* pGoalParams = pGoal->GetParams();
		
		if ( !pGoalParams->ContainsFlag( CRCD(0x3d1cab0b,"null_goal") ) && !pGoal->IsEditedGoal() ) // K: Don't add edited goals
		{
			Script::CStruct* p_temp = new Script::CStruct();
			pGoal->GetSaveData( p_temp );
			mp_goalFlags->AddStructure( pGoal->GetGoalId(), p_temp );
			delete p_temp;
		}	
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::LevelLoad()
{
    Script::CComponent* p_comp = NULL;
    p_comp = mp_goalFlags->GetNextComponent( p_comp );

    while ( p_comp )
    {
        Script::CComponent* p_next = mp_goalFlags->GetNextComponent( p_comp );

		// special goal manager params structure
		if ( p_comp->mNameChecksum == CRCD( 0x23d4170a, "GoalManager_Params" ) )
		{
			p_comp->mpStructure->GetInteger( CRCD(0x6abb6fcb,"goalPoints"), &m_goalPoints, Script::NO_ASSERT );
			p_comp->mpStructure->GetInteger( CRCD(0x79ed60aa,"totalGoalPointsEarned"), &m_totalGoalPointsEarned, Script::NO_ASSERT );
			p_comp->mpStructure->GetInteger( CRCD(0xf9461a46,"cash"), &m_cash, Script::NO_ASSERT );
			p_comp->mpStructure->GetInteger( CRCD(0x875e89bb,"total_cash"), &m_totalCash, Script::NO_ASSERT );
			// p_comp->mpStructure->GetInteger( "numGoalsToPro", &m_numGoalsToPro, Script::NO_ASSERT );
			p_comp->mpStructure->GetInteger( CRCD(0x46f71d38,"numGoalsBeaten"), &m_numGoalsBeaten, Script::NO_ASSERT );

			int auto_change_chapter_and_stage = 0;
			auto_change_chapter_and_stage = Script::GetInteger( CRCD(0x5a1d8804,"auto_change_chapter_and_stage"), Script::NO_ASSERT );
			if ( auto_change_chapter_and_stage == 0 )
			{
				p_comp->mpStructure->GetInteger( CRCD(0xf884773c,"currentChapter"), &m_currentChapter, Script::NO_ASSERT );
				p_comp->mpStructure->GetInteger( CRCD(0xaf1575eb,"currentStage"), &m_currentStage, Script::NO_ASSERT );				
			}
			p_comp->mpStructure->GetChecksum( CRCD(0x7e73362b,"sponsor"), &m_sponsor, Script::ASSERT );
			
			Script::CStruct* p_copy;
			p_comp->mpStructure->GetStructure( CRCD(0x3b1f59e0,"team"), &p_copy, Script::ASSERT );
			mp_team->Clear();
			mp_team->AppendStructure( p_copy );

			Script::CStruct* p_diff_copy;
			p_comp->mpStructure->GetStructure( CRCD(0xb13d98d3,"difficulty_levels"), &p_diff_copy, Script::ASSERT );
			mp_difficulty_levels->Clear();
			mp_difficulty_levels->AppendStructure( p_diff_copy );
			
			int isPro;
			p_comp->mpStructure->GetInteger( CRCD(0x40801d41,"isPro"), &isPro, Script::ASSERT );
			if ( isPro != 0 )
				TurnPro();
			
/*			int proChallengesUnlocked;
			p_comp->mpStructure->GetInteger( CRCD(0x24c226fa,"proChallengesUnlocked"), &proChallengesUnlocked, Script::ASSERT );
			if ( proChallengesUnlocked != 0 )
				m_proChallengesUnlocked = true;

			Script::CStruct* p_copy;
			p_comp->mpStructure->GetStructure( CRCD(0xf8478579,"proSpecificChallenges"), &p_copy, Script::ASSERT );
			mp_proSpecificChallenges->AppendStructure( p_copy );
*/
		}
		else 
		{
			CGoal* pGoal = GetGoal( p_comp->mNameChecksum, false );
			if ( pGoal )
				pGoal->LoadSaveData( p_comp->mpStructure );
		}
        
		p_comp = p_next;
    }
	
	// make sure to unlock any pro goals (they may have turned pro in another level)
	if ( m_isPro )
		TurnPro();

	// ...and unlock any pro challenges
	// if ( m_proChallengesUnlocked )
		// UnlockProSpecificChallenges();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CGoalManager::NumGoalsBeatenBy( int obj_id )
{
	int i, num_beaten;
	CGoal* pGoal;

	num_beaten = 0;
	for ( i = 0; i < GetNumGoals(); i++ )
    {
        uint32 key;
        pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
        Dbg_Assert( pGoal );

		if( pGoal->GetParents()->m_relative != 0 )
		{
			continue;
		}

		if( pGoal->HasWonGoal( obj_id ))
		{
			num_beaten++;
		}
	}

	return num_beaten;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CGoalManager::NumGoalsBeatenByTeam( int team_id )
{
	int i, num_beaten;
	CGoal* pGoal;

	num_beaten = 0;
	for ( i = 0; i < GetNumGoals(); i++ )
    {
        uint32 key;
        pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
        Dbg_Assert( pGoal );

		if( pGoal->GetParents()->m_relative != 0 )
		{
			continue;
		}
		if( pGoal->HasTeamWonGoal( team_id ))
		{
			num_beaten++;
		}
	}

	return num_beaten;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CGoalManager::NumGoalsBeaten()
{
	int i, num_beaten;
	CGoal* pGoal;

	num_beaten = 0;
	for ( i = 0; i < GetNumGoals(); i++ )
    {
        uint32 key;
        pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
        Dbg_Assert( pGoal );

		if( pGoal->GetParents()->m_relative != 0 )
		{
			continue;
		}
		if( pGoal->HasWonGoal())
		{
			num_beaten++;
		}
	}

	return num_beaten;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
int CGoalManager::NumGoalsBeatenInLevel( int levelNum, bool count_pro_specific_challenges )
{
    // store all the goal info in the goalmanager's struct
    // so we can grab it all in one place
    LevelUnload();
    
    int numbeat = 0;
    Script::CComponent* p_comp = mp_goalFlags->GetNextComponent( NULL );
    Script::CComponent* p_next = NULL;
    while ( p_comp )
    {
        // printf("checking p_comp\n");
        p_next = mp_goalFlags->GetNextComponent( p_comp );

        int p_comp_levelNum = 0;
        if ( p_comp->mpStructure->GetInteger( CRCD(0x1a4cde06,"levelNum"), &p_comp_levelNum, Script::NO_ASSERT ) )
        {
            // printf("found levelNum\n");
/*			if ( !count_pro_specific_challenges )
			{
				int isProSpecificChallenge;
				p_comp->mpStructure->GetInteger( CRCD(0x3dbe3121,"isProSpecificChallenge"), &isProSpecificChallenge, Script::ASSERT );
				if ( isProSpecificChallenge == 1 )
				{
					p_comp = p_next;
					continue;
				}
			}
*/
			int hasBeaten = 0;
			p_comp->mpStructure->GetInteger( CRCD(0x49807745,"hasBeaten"), &hasBeaten, Script::NO_ASSERT );
            if ( p_comp_levelNum == levelNum && hasBeaten )
            {
                numbeat++;
            }
        }
        p_comp = p_next;
    }
    return numbeat;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoalManager::UnlockAllGoals()
{
    m_isPro = true;
	// m_proChallengesUnlocked = true;

	for ( int i = 0; i < GetNumGoals(); i++ )
    {
        uint32 key;
        CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
        Dbg_Assert( pGoal );
        // pGoal->UnlockGoal();
		pGoal->SetHasSeen();
    }
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::TurnPro()
{
	m_isPro = true;
	// m_proChallengesUnlocked = true;

	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
		
		if ( pGoal->IsProGoal() && pGoal->IsLocked() )
		{
			pGoal->UnlockGoal();
		}
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoalManager::SetStartTime( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	pGoal->SetStartTime();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoalManager::CheckMinigameRecord( uint32 goalId, int value )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	return pGoal->CheckMinigameRecord( value );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::UpdateComboTimer( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	pGoal->UpdateComboTimer();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::SetStartHeight( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	pGoal->SetStartHeight();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::CheckHeightRecord( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	return pGoal->CheckHeightRecord();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::CheckDistanceRecord( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	return pGoal->CheckDistanceRecord();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::PlayGoalStartStream( Script::CStruct* pParams )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	pGoal->mp_goalPed->PlayGoalStartStream( pParams );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::PlayGoalWaitStream( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	pGoal->mp_goalPed->PlayGoalWaitStream();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::PlayGoalStream( Script::CStruct* pParams )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	// if we got a specific stream, just play it and quit
	uint32 stream_checksum = 0;
	pParams->GetChecksum( CRCD(0x1a4e4fb9,"vo"), &stream_checksum, Script::NO_ASSERT );
	if ( stream_checksum )
	{
		pGoal->mp_goalPed->PlayGoalStream( stream_checksum );
		return;
	}

	// check for type
	const char* type = NULL;
	if ( pParams->GetString( CRCD(0x7321a8d6,"type"), &type, Script::NO_ASSERT ) )
	{		
		pGoal->mp_goalPed->PlayGoalStream( type, pParams );
		return;
	}		
	
	// you gotta have one or the other
	if ( !type && !stream_checksum )
		Dbg_MsgAssert( 0, ("GoalManager_PlayGoalStream called without a type or vo checksum") );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::StopCurrentStream( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	pGoal->mp_goalPed->StopCurrentStream();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::PlayGoalWinStream( Script::CStruct* pParams )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	pGoal->mp_goalPed->PlayGoalWinStream( pParams );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::PauseGoal( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	pGoal->PauseGoal();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::UnPauseGoal( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	pGoal->UnPauseGoal();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::PauseCompetition( uint32 goalId )
{
	Game::CCompetitionGoal* pComp = (Game::CCompetitionGoal*)GetGoal( goalId );
	Dbg_Assert( pComp );

	pComp->PauseCompetition();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::UnPauseCompetition( uint32 goalId )
{
	Game::CCompetitionGoal* pComp = (Game::CCompetitionGoal*)GetGoal( goalId );
	Dbg_Assert( pComp );

	pComp->UnPauseCompetition();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::UnBeatAllGoals()
{	
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );

		if ( pGoal->UnBeatGoal() && !gamenet_man->InNetGame() )
		{
			// m_numGoalsToPro++;
			m_numGoalsBeaten--;
			m_goalPoints--;
			m_totalGoalPointsEarned--;
		}
	}
	// clear the goal manager's memory of goals too
	if ( !gamenet_man->InNetGame() )
		mp_goalFlags->Clear();

	if ( GetNumActiveGoals() == 0 )
	{
		if( CFuncs::ScriptInMultiplayerGame( NULL, NULL ) == false )
		{
			Script::RunScript( CRCD(0x888ef05e,"GoalManager_ShowGoalPoints") );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::AddViewGoalsList()
{
	
	int numGoals = GetNumGoals();
	Script::CArray* p_ordered_goals = new Script::CArray();
	Script::CArray* p_unordered_goals = new Script::CArray();	
	p_ordered_goals->SetSizeAndType( numGoals, ESYMBOLTYPE_NAME );
	p_unordered_goals->SetSizeAndType( numGoals, ESYMBOLTYPE_NAME );
	
	int num_ordered_goals = 0;
	int num_unordered_goals = 0;
	
	// divide the list into goals with ordinals and goals without
	for ( int i = 0; i < numGoals; i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );

		if ( pGoal->GetParams()->ContainsComponentNamed( CRCD(0x9b048fc,"ordinal") ) )
		{
			p_ordered_goals->SetChecksum( num_ordered_goals, pGoal->GetGoalId() );
			num_ordered_goals++;
		}
		else
		{
			p_unordered_goals->SetChecksum( num_unordered_goals, pGoal->GetGoalId() );
			num_unordered_goals++;
		}			
	}

	// sort the list of goals with ordinals
	// printf("there are %i ordered goals\n", num_ordered_goals);
	for ( int i = 0; i < num_ordered_goals; i++ )
	{
		// inefficient sorting, but there's only ~20 things - brad
		for ( int j = i; j < num_ordered_goals; j++ )
		{
			uint32 temp = p_ordered_goals->GetChecksum( j );
			CGoal* p_temp_goal = GetGoal( temp );
			Dbg_Assert( p_temp_goal );
			int ordinal_check;
			p_temp_goal->GetParams()->GetInteger( CRCD(0x9b048fc,"ordinal"), &ordinal_check, Script::ASSERT );
			Dbg_MsgAssert( ordinal_check - 1 < num_ordered_goals, ( "Ordinal value %i too big.", ordinal_check ) );
			if ( ordinal_check - 1 == i )
			{
				if ( j == i )
					break;

				// swap them				
				uint32 temp2 = p_ordered_goals->GetChecksum( i );
				int double_check;
				GetGoal( temp2 )->GetParams()->GetInteger( CRCD(0x9b048fc,"ordinal"), &double_check, Script::ASSERT );
				Dbg_MsgAssert( double_check != i, ( "Found ordinal %i twice", double_check ) );

				p_ordered_goals->SetChecksum( i, temp );
				p_ordered_goals->SetChecksum( j, temp2 );
				break;
			}
		}
	}
	
	// tack on the list of unordered goals
	for ( int i = 0; i < num_unordered_goals; i++ )
		p_ordered_goals->SetChecksum( i + num_ordered_goals, p_unordered_goals->GetChecksum( i ) );
	
	// populate the menu
	for ( int i = 0; i < numGoals; i++ )
	{
		uint32 goalId = p_ordered_goals->GetChecksum( i );
		CGoal* pGoal = GetGoal( goalId );
		Dbg_Assert( pGoal );
		Script::CStruct* pGoalParams = pGoal->GetParams();

		const char* p_view_goals_text = NULL;
		pGoal->GetViewGoalsText( &p_view_goals_text );
		
		// skip goals without an associated text (eg, minigames)
		if ( !p_view_goals_text )
			continue;

		// skip pro goals if we're not pro
		if ( !IsPro() && pGoalParams->ContainsFlag( CRCD(0xd303a1a3,"pro_goal") ) )
			continue;

		// skip pro challenges if we haven't unlocked them.
		// if ( !ProSpecificChallengesUnlocked() && pGoalParams->ContainsFlag( CRCD(0xe0f96410,"pro_specific_challenge") ) )
			// continue;
		
		// skip pro specific challenges if we're not the right skater
/*		if ( !pGoal->HasWonGoal() && !IsPro() && pGoalParams->ContainsFlag( CRCD(0xe0f96410,"pro_specific_challenge") ) )
		{
			// check the current skater against the listed pro
			Script::CStruct* pLastNames = Script::GetStructure( CRCD(0x621d1828,"goal_pro_last_name_checksums"), Script::ASSERT );
			const char* pProChallengeProName;
			pGoalParams->GetString( CRCD(0x4a91eceb,"pro_challenge_pro_name"), &pProChallengeProName, Script::ASSERT );
			uint32 last_name = 0;
			pLastNames->GetChecksum( Script::GenerateCRC( pProChallengeProName ), &last_name, Script::NO_ASSERT );
			Mdl::Skate * pSkate = Mdl::Skate::Instance();
			Obj::CSkaterProfile* pSkaterProfile = pSkate->GetCurrentProfile();
			if ( last_name && last_name != pSkaterProfile->GetSkaterNameChecksum() )
				continue;
		}
*/
		if( pGoal->IsNetGoal())
			continue;
		
		// create the screen element
		Script::CStruct* p_elem_params = new Script::CStruct();
		uint32 id = pGoal->GetGoalId();
		
		// add the id so we can use the same struct when calling the set_events script below
		p_elem_params->AddChecksum( CRCD(0x9982e501,"goal_id"), id );
		p_elem_params->AddString( CRCD(0xc4745838,"text"), p_view_goals_text );

		// check for a win record
		uint32 record_type;
		if ( pGoal->HasWonGoal() )
		{
			int record = 0;
			char record_string[128];

			// special case for competition
			if ( pGoal->IsCompetition() )
			{
				pGoalParams->GetInteger( CRCD(0xc22a2b72,"win_record"), &record, Script::NO_ASSERT );
				sprintf( record_string, "%i", record );
				switch ( record )
				{
					case 1:
						strcat( record_string, "st place" );
						break;
					case 2:
						strcat( record_string, "nd place" );
						break;
					case 3:
						strcat( record_string, "rd place" );
						break;
					default:
						Dbg_Message( "WARNING: competition place is %i", record );
						break;
				}						
				p_elem_params->AddString( CRCD(0xbb48af8b,"win_record_string"), record_string );
			}			
			else if ( pGoalParams->GetChecksum( CRCD(0x96963902,"record_type"), &record_type, Script::NO_ASSERT ) )
			{
				// For the purposes of supporting different record types in the future,
				// I've left some repeated code within each block.
				if ( record_type == ( CRCD(0xcd66c8ae,"score") ) )
				{
					pGoalParams->GetInteger( CRCD(0xc22a2b72,"win_record"), &record, Script::NO_ASSERT );
					sprintf( record_string, "%s", Str::PrintThousands( record ) );

					p_elem_params->AddString( CRCD(0xbb48af8b,"win_record_string"), record_string );
				}
				else if ( record_type == ( CRCD(0x906b67ba,"time") ) )
				{
					int record = 0;
					pGoalParams->GetInteger( CRCD(0xc22a2b72,"win_record"), &record, Script::NO_ASSERT );
					
					// break down into minutes, seconds, fraction
					int min, sec, fraction;
					sec = record / 1000;
					min = ( sec / 60 );
					sec -= ( min * 60 );
					fraction = ( ( record % 1000 ) / 10 );
					sprintf( record_string, "%i:%02i.%02i", min, sec, fraction );

					p_elem_params->AddString( CRCD(0xbb48af8b,"win_record_string"), record_string );
				}
			}
			else
			{
				p_elem_params->AddChecksum( NONAME,  CRCD(0xc80e196,"no_record_type")  );
			}
		}

		// create the item
		Script::RunScript( CRCD(0x380aea65,"view_goals_menu_add_item"), p_elem_params );

		delete p_elem_params;
	}
	
	// cleanup!
	Script::CleanUpArray( p_ordered_goals );
	Script::CleanUpArray( p_unordered_goals );
	delete p_ordered_goals;
	delete p_unordered_goals;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::AddGoalChoices( bool selected_only )
{
	
	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );

		const char* p_view_goals_text = NULL;
		pGoal->ReplaceTrickText();
		pGoal->GetViewGoalsText( &p_view_goals_text );
		
		// skip goals without an associated text (eg, minigames)
		if ( !p_view_goals_text )
		{
			continue;
		}

		if( pGoal->IsNetGoal())
		{
			continue;
		}

		if( pGoal->IsMinigame())
		{
			continue;
		}

        if( pGoal->GetParents()->m_relative != 0 )
		{
			continue;
		}

		if( GameNet::Manager::ScriptIsHost( NULL, NULL ))
		{
			// if( pGoal->HasSeenGoal() == false )
			// {
				// continue;
			// }
	
			// if( pGoal->IsLocked())
			// {
				// continue;
			// }
		}

		if( selected_only )
		{
			if( !pGoal->IsSelected())
			{
				
				continue;
			}
		}
		
		// create the screen element
		Script::CStruct* p_elem_params = new Script::CStruct();
		uint32 id = pGoal->GetGoalId();
		

		// add the id so we can use the same struct when calling the set_events script below
		p_elem_params->AddChecksum( CRCD(0x9982e501,"goal_id"), id );
		p_elem_params->AddChecksum( CRCD(0xc2719fb0,"parent"), CRCD( 0x4d49ac0a, "current_menu" ) );
		p_elem_params->AddString( CRCD(0xc4745838,"text"), p_view_goals_text );
		p_elem_params->AddChecksum( CRCD(0x2f6bf72d,"font"), CRCD( 0x8aba15ec, "small" ) );		
		p_elem_params->AddInteger( CRCD(0x13b9da7b,"scale"), 0 );
		
		
		// set initial color and events for this item
		if( selected_only )
		{
			Script::RunScript( CRCD(0xe704c86c,"view_selected_goals_menu_set_events"), p_elem_params);
		}
		else
		{
			Script::RunScript( CRCD(0x6790ac11,"choose_goals_menu_set_events"), p_elem_params);
		}
		// normally, script won't be updated until next frame -- we want it NOW
		// p_new_script->Update();

		delete p_elem_params;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::GoalIsLocked( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId, false );
	
	if ( pGoal )
	{
		return pGoal->IsLocked();
	}
	else
	{
		// check goal manager params struct in case the goal was in
		// another level
		int goalLocked = 0;
		Script::CStruct* pGoalParams;
		if ( mp_goalFlags->GetStructure( goalId, &pGoalParams, Script::NO_ASSERT ) )
			pGoalParams->GetInteger( CRCD(0xd3e93882,"isLocked"), &goalLocked, Script::ASSERT );

		return ( goalLocked != 0 );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGoal* CGoalManager::IsInCompetition()
{
	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );

		if ( pGoal->IsCompetition() && pGoal->IsActive() )
		{
			return pGoal;
		}
	}
	return NULL;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::GetGoalAnimations( uint32 goalId, const char* type, Script::CScript* pScript )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	uint32 array_name = pGoal->mp_goalPed->GetGoalAnimations( type );
	// Dbg_Assert( p_anims_array );

	// never know
	pScript->GetParams()->RemoveComponent( CRCD(0x18653764,"pro_stuff") );
	if ( array_name )
		pScript->GetParams()->AddChecksum( CRCD(0x18653764,"pro_stuff"), array_name );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CGoalManager::GetRandomBettingMinigame()
{
	// create an index array and randomize
	int numGoals = GetNumGoals();
	int* indexes = new int[numGoals];
	for ( int i = 0; i < numGoals; i++ )
		indexes[i] = i;
	for ( int i = 0; i < numGoals * 5; i++ )
	{
		int aIndex = Mth::Rnd( numGoals );
		int bIndex = Mth::Rnd( numGoals );
		int a = indexes[aIndex];
		indexes[aIndex] = indexes[bIndex];
		indexes[bIndex] = a;
	}
	
	// look through until we find one that's suitable
	for ( int i = 0; i < numGoals; i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( indexes[i], (int*)&key );
		Dbg_Assert( pGoal );

		if ( pGoal->IsBettingGame() )
		{
			delete[] indexes;
			return pGoal->GetGoalId();
		}
	}
	delete[] indexes;
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::EndRunCalled( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	return pGoal->EndRunCalled();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CGoalManager::GetBettingGuyId()
{
	// find the betting guy
	int numGoals = GetNumGoals();
	for ( int i = 0; i < numGoals; i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );

		if ( pGoal->IsBettingGuy() )
		{			
			return pGoal->GetGoalId();
		}
	}
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::DeactivateMinigamesWithTimer()
{
	int numGoals = GetNumGoals();
	for ( int i = 0; i < numGoals; i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );

		if ( pGoal->IsMinigame() && pGoal->IsActive() && pGoal->ShouldUseTimer() )
			pGoal->Deactivate();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct* CGoalManager::GetGoalManagerParams()
{
	return mp_goalFlags;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::ReplaceTrickText( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	return pGoal->ReplaceTrickText();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::ReplaceTrickText( uint32 goalId, const char* p_old_string, char* p_new_string, uint out_buffer_size )
{
#ifdef __NOPT_ASSERT__
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );
#endif		// __NOPT_ASSERT__

	// get the key combo array
	Script::CStruct* p_goal_params = GetGoalParams( goalId );
	Script::CArray* p_key_combo;
	p_goal_params->GetArray( CRCD(0x79704516,"key_combos"), &p_key_combo, Script::ASSERT );

	// build the string arrays
	Script::CArray* p_key = new Script::CArray();
	Script::CArray* p_trick = new Script::CArray();
	int size = p_key_combo->GetSize();
	p_key->SetSizeAndType( size, ESYMBOLTYPE_STRING );
	p_trick->SetSizeAndType( size, ESYMBOLTYPE_STRING );

	int premade_cat_index = -1;
	if ( p_goal_params->ContainsFlag( CRCD(0x8e6014f6,"create_a_trick") ) )
	{
		p_goal_params->GetInteger( CRCD(0x5b077ce1,"trickName"), &premade_cat_index, Script::ASSERT );
	}
	
	if ( !fill_trick_and_key_combo_arrays( p_key_combo, p_key, p_trick, premade_cat_index ) )
	{
		Script::CleanUpArray( p_key );
		Script::CleanUpArray( p_trick );
		delete p_key;
		delete p_trick;
		return false;
	}

	find_and_replace_trick_string( p_old_string, p_new_string, p_key, p_trick, out_buffer_size );
	
	// cleanup!
	Script::CleanUpArray( p_key );
	Script::CleanUpArray( p_trick );
	delete p_key;
	delete p_trick;

	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::ReplaceAllTrickText()
{
	int numGoals = GetNumGoals();
	for ( int i = 0; i < numGoals; i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );

		if ( pGoal->IsInitialized() )
			pGoal->ReplaceTrickText();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
void CGoalManager::UnlockProSpecificChallenges()
{
	m_proChallengesUnlocked = true;
	int numGoals = GetNumGoals();
	for ( int i = 0; i < numGoals; i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
		
		if ( pGoal->GetParams()->ContainsFlag( CRCD(0xe0f96410,"pro_specific_challenge") ) )
		{
			// only unlock it if it's beaten or we're using the right skater
			if ( pGoal->HasWonGoal() || pGoal->mp_goalPed->ProIsCurrentSkater() )
				pGoal->UnlockGoal();
		}
	}
}
*/
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
bool CGoalManager::ProSpecificChallengesUnlocked()
{
	return m_proChallengesUnlocked;
}
*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CGoalManager::GetNumberCollected( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId, true );

	return pGoal->GetNumberCollected();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CGoalManager::GetNumberOfFlags( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId, true );

	return pGoal->GetNumberOfFlags();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::IsPro()
{
	return m_isPro;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::ResetGoalFlags( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );

	pGoal->ResetGoalFlags();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::ColorTrickObjects( uint32 goalId, int seqIndex, bool clear )
{
	CGoal* pGoal = GetGoal( goalId );

	pGoal->ColorTrickObjects( seqIndex, clear );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CGoalManager::GetNumberOfTimesGoalStarted( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId );

	return pGoal->GetNumberOfTimesGoalStarted();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::GoalExists( uint32 goalId )
{
	CGoal* pGoal = GetGoal( goalId, false );
	if ( pGoal )
		return true;
	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct* CGoalManager::GetGoalAttemptInfo()
{
	int numGoals = GetNumGoals();
	int totalStarts = 0;
	Script::CStruct* attemptInfo = new Script::CStruct();

	for ( int i = 0; i < numGoals; i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
		if ( !pGoal->IsMinigame() && !pGoal->IsNetGoal() && pGoal->HasWonGoal() )
			totalStarts += pGoal->GetNumberOfTimesGoalStarted();
	}
	attemptInfo->AddInteger( CRCD(0xd023adbe,"totalStarts"), totalStarts );
	return attemptInfo;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::SetCanStartGoal( int value )
{
	if ( value != 0 )
		m_canStartGoal = true;
	else
		m_canStartGoal = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::CheckTrickObjects()
{
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	Obj::CSkater* pSkater = skate_mod->GetLocalSkater();

	Mdl::Score* pScore = pSkater->GetScoreObject();
	int score = pScore->GetLastScoreLanded();
	
	Obj::CTrickComponent* pTrickComponent = GetTrickComponentFromObject(pSkater);
	Dbg_Assert(pTrickComponent);

	Obj::CPendingTricks pendingTricks = pTrickComponent->m_pending_tricks;

	int max_tricks = pendingTricks.m_NumTrickItems;
	if ( max_tricks > pendingTricks.vMAX_PENDING_TRICKS )
		max_tricks = pendingTricks.vMAX_PENDING_TRICKS;

	for ( int i = 0; i < max_tricks; i++ )
	{
		GotTrickObject( pendingTricks.m_Checksum[i], score );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
void CGoalManager::MarkProSpecificChallengeBeaten( uint32 skater_name, int beaten )
{
	Dbg_MsgAssert( mp_proSpecificChallenges->ContainsComponentNamed( skater_name ), ("MarkProSpecificChallengeBeaten called with unknown skater") );
	mp_proSpecificChallenges->AddInteger( skater_name, beaten );
}
*/
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
bool CGoalManager::SkaterHasBeatenProSpecificChallenge( uint32 skater_name )
{
	// Dbg_MsgAssert( mp_proSpecificChallenges->ContainsComponentNamed( skater_name ), ("SkaterHasBeatenProSpecificChallenge called with unknown skater %s", Script::FindChecksumName( skater_name ) ) );
	if ( mp_proSpecificChallenges->ContainsComponentNamed( skater_name ) )
	{
		int beaten;
		mp_proSpecificChallenges->GetInteger( skater_name, &beaten, Script::ASSERT );
		return ( beaten != 0 );
	}
	return false;
}
*/
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::SetEndRunType( uint32 goalId, Game::EndRunType newEndRunType )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	pGoal->SetEndRunType( newEndRunType );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::ShouldUseTimer()
{
	int numGoals = GetNumGoals();
	for ( int i = 0; i < numGoals; i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
		
		if ( pGoal->IsActive() && pGoal->ShouldUseTimer() )
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

void CGoalManager::SetShouldDeactivateOnExpire( uint32 goalId, bool should_deactivate )
{
	CGoal* pGoal = GetGoal( goalId );
	Dbg_Assert( pGoal );

	pGoal->SetShouldDeactivateOnExpire( should_deactivate );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoalManager::Land()
{
	if ( ShouldLogTrickObject() )
		CheckTrickObjects();
	else 
	{
		int numGoals = GetNumGoals();
		for ( int i = 0; i < numGoals; i++ )
		{
			uint32 key;
			CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
			Dbg_Assert( pGoal );

			if ( pGoal->IsActive() && pGoal->IsHorseGoal() )
			{
				Game::CHorseGoal* pHorseGoal = (Game::CHorseGoal*)pGoal;
				pHorseGoal->CheckScore();
				break;
			}
			else if ( pGoal->IsActive() && pGoal->GetParams()->ContainsFlag( CRCD(0xc63432a7,"high_combo") ) )
			{
				// TODO: maybe the high combo goal should be a subclass rather than
				// having a special case here?				
				Obj::CSkater* pSkater = Mdl::Skate::Instance()->GetLocalSkater();
				if ( pSkater )
				{
					Mdl::Score* pScore = pSkater->GetScoreObject();
					if ( pScore->GetLastScoreLanded() > 0 )
					{
						Script::CStruct* pScriptParams = new Script::CStruct();
						pScriptParams->AddChecksum( CRCD(0x9982e501,"goal_id"), pGoal->GetGoalId() );
						pScriptParams->AddInteger( CRCD(0xb9072b65,"last_score_landed"), pScore->GetLastScoreLanded() );
						Script::RunScript( CRCD(0x6b755ed8,"goal_highcombo_check_score"), pScriptParams );
						delete pScriptParams;
					}
				}
			}
			else if ( pGoal->IsActive() && pGoal->ShouldRequireCombo() && pGoal->GetParams()->ContainsFlag( CRCD(0x7100155d,"combo_started") ) )
			{
				// lose the goal next frame
				Script::CStruct* pScriptParams = new Script::CStruct();
				pScriptParams->AddChecksum( CRCD(0x9982e501,"goal_id"), pGoal->GetGoalId() );
				Script::SpawnScript( CRCD(0xfefda11,"goal_lose_next_frame"), pScriptParams );
				delete pScriptParams;
			}
			else if ( pGoal->IsActive() && pGoal->GetParams()->ContainsFlag( CRCD( 0x293b95a7, "score_goal" ) ) )
			{
				Obj::CSkater* pSkater = Mdl::Skate::Instance()->GetLocalSkater();
				if ( pSkater )
				{
					Mdl::Score* pScore = pSkater->GetScoreObject();
					int required_score;
					pGoal->GetParams()->GetInteger( CRCD(0xcd66c8ae,"score"), &required_score, Script::ASSERT );
					if ( pScore->GetTotalScore() + pScore->GetScorePotValue() >= required_score )
					{
						pGoal->GetParams()->AddInteger( CRCD(0x3da891b6,"winning_score"), pScore->GetTotalScore() + pScore->GetScorePotValue() );
						WinGoal( pGoal->GetGoalId() );
						pGoal->GetParams()->RemoveComponent( CRCD(0x3da891b6,"winning_score") );
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
	
void CGoalManager::Bail()
{
	int numGoals = GetNumGoals();
	for ( int i = 0; i < numGoals; i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );

		if ( pGoal->IsActive() && pGoal->ShouldRequireCombo() && pGoal->GetParams()->ContainsFlag( CRCD(0x7100155d,"combo_started") ) )
		{
			// lose the goal next frame
			Script::CStruct* pScriptParams = new Script::CStruct();
			pScriptParams->AddChecksum( CRCD(0x9982e501,"goal_id"), pGoal->GetGoalId() );
			Script::SpawnScript( CRCD(0xfefda11,"goal_lose_next_frame"), pScriptParams );
			delete pScriptParams;
		}
		if ( pGoal->IsActive() && pGoal->IsTetrisGoal() )
		{
			int trick_flag;
			if ( pGoal->GetParams()->GetInteger( CRCD( 0x60b827d5, "trick_flag" ), &trick_flag, Script::NO_ASSERT ) )
			{
				if ( trick_flag != 0 )
				{
					pGoal->GetParams()->AddInteger( CRCD(0x60b827d5,"trick_flag"), 0 );
				}
			}
		}
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::AwardMinigameCash( uint32 goalId, int amount )
{
	CGoal* pGoal = GetGoal( goalId, true );
	if ( pGoal && pGoal->IsMinigame() )
	{
		CMinigame* pMinigame = (CMinigame*)pGoal;
		return pMinigame->AwardCash( amount );
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::ResetCareer()
{
	// reset all of the goal manager's memory
	mp_goalFlags->Clear();

	m_goalPoints = 0;
	m_totalGoalPointsEarned = 0;
	m_cash = 0;
	m_totalCash = 0;
	m_numGoalsBeaten = 0;
	// m_proChallengesUnlocked = false;
	m_isPro = false;
	
	// mp_proSpecificChallenges->AppendStructure( Script::GetStructure( CRCD(0xd8b84847,"goal_pro_specific_challenges_beaten"), Script::ASSERT ) );
	m_currentChapter = m_currentStage = 0;

	mp_difficulty_levels->Clear();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::AwardAllGoalCash()
{
	int numGoals = GetNumGoals();
	for ( int i = 0; i < numGoals; i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
		
		if ( !pGoal->IsMinigame() && !pGoal->HasWonGoal() )
		{
			// printf("found an unbeaten goal\n");
			pGoal->MarkBeaten();
			Script::CStruct* pRewardParams = new Script::CStruct();
			pGoal->GetRewardsStructure( pRewardParams );
			int cash;
			if ( pRewardParams->GetInteger(	 "cash", &cash, Script::NO_ASSERT ) )
			{
				const char* p_goal_name;
				pGoal->GetParams()->GetString( CRCD(0xbfecc45b,"goal_name"), &p_goal_name, Script::ASSERT );
				// printf("adding $%i for goal %s\n", cash, p_goal_name);
				AddCash( cash );
			}
			delete pRewardParams;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::UpdateFamilyTrees()
{
	int numGoals = GetNumGoals();
	
	// update all the parent lists
	for ( int i = 0; i < numGoals; i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
		uint32 parent_id = pGoal->GetGoalId();
		Game::CGoalLink* p_child = pGoal->GetChildren();
		
		while ( p_child && p_child->m_relative != 0 )
		{
			// printf("goal has a child\n");
			uint32 child_id = p_child->m_relative;
			// get the child
			CGoal* pChildGoal = GetGoal( child_id, false );
			Dbg_MsgAssert( pChildGoal, ( "Failed to find goal %s when binding child to parent\n", Script::FindChecksumName( child_id ) ) );

			// search the parent list and add if necessary
			Game::CGoalLink* p_parentNode = pChildGoal->GetParents();
			bool found = false;
			while ( p_parentNode && p_parentNode->m_relative != 0 )
			{
				if ( p_parentNode->m_relative == parent_id )
				{
					found = true;
					break;
				}
				p_parentNode = p_parentNode->mp_next;
			}

			if ( !found )
				pChildGoal->AddParent( parent_id );
			
			p_child = p_child->mp_next;
		}
	}

	// send inherited params down the tree from root parent
	for ( int i = 0; i < numGoals; i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
		
		// SetInheritableFlags will automatically traverse down the tree,
		// so we only need to call it on nodes without parents - ie, the root 
		// node of the tree.
		if ( pGoal->GetParents()->m_relative == 0 )
		{
			pGoal->SetInheritableFlags();
		}
	}
	
	// debugging shit.
	for ( int i = 0; i < numGoals; i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
	
		// print out children list
		Game::CGoalLink* p_test = pGoal->GetChildren();
		bool print_newline = false;
		if ( p_test->m_relative != 0 )
			// printf("children of %s\n", Script::FindChecksumName( pGoal->GetGoalId() ) );
		while ( p_test && p_test->m_relative != 0 )
		{
			print_newline = true;
			// printf("\t%s\n", Script::FindChecksumName( p_test->m_relative ) );
			p_test = p_test->mp_next;
		}
		
		// print out the parent list
		p_test = pGoal->GetParents();
		if ( p_test->m_relative != 0 )
			// printf("parents of %s\n", Script::FindChecksumName( pGoal->GetGoalId() ) );
		while ( p_test && p_test->m_relative != 0 )
		{
			print_newline = true;
			// printf("\t%s\n", Script::FindChecksumName( p_test->m_relative ) );
			p_test = p_test->mp_next;
		}
		// if ( print_newline )
			// printf("\n");
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::FindCircularLinks( uint32 parent, uint32 child )
{
#ifdef __NOPT_ASSERT__
	CGoal* pGoal = GetGoal( child, false );
	if ( pGoal )
	{
		CGoalLink* p_link = pGoal->GetChildren();
		while ( p_link && p_link->m_relative != 0 )
		{
			if ( p_link->m_relative == parent )
				Dbg_MsgAssert( 0, ( "Circular goal link path found from %s to %s\n", Script::FindChecksumName( parent ), Script::FindChecksumName( child ) ) );
			if ( p_link->m_relative == child )
				Dbg_MsgAssert( 0, ( "Goal (%s) has itself as a child\n", Script::FindChecksumName( child ) ) );

			// recurse!
			FindCircularLinks( parent, p_link->m_relative );
			p_link = p_link->mp_next;
		}
	}
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::CallMemberFunction( uint32 checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( checksum )
	{
		case 0x3ae9211b: // IsRootNode
		{
			CGoal* pGoal = GetGoal( pParams );
			if ( pGoal )
				return ( pGoal->GetParents()->m_relative == 0 );
			return false;
			break;
		}
	
		case 0xd782f8de: // IsLeafNode
		{
			CGoal* pGoal = GetGoal( pParams );
			if ( pGoal )
				return ( pGoal->GetChildren()->m_relative == 0 );
			return false;
			break;
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CGoalManager::GetGoalIdFromParams( Script::CStruct* pParams, bool assert )
{
	uint32 goalId = 0;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, assert );
	return goalId;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::CreateGoalLevelObjects( void )
{
	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		Script::CStruct* pParams;
		Script::CArray *pGeomArray;
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
		pParams = pGoal->GetParams();

		// Create the associated pieces of geometry
		if ( pParams->GetArray( CRCD(0xf4eb5c60,"create_geometry"), &pGeomArray ) )
		{
			uint32 j, geometry;
			for( j = 0; j < pGeomArray->GetSize(); j++ )
			{
				geometry = pGeomArray->GetChecksum( j );
				CFuncs::ScriptCreateFromNodeIndex( SkateScript::FindNamedNode( geometry ));
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::SetDifficultyLevel( int level )
{
	Dbg_MsgAssert( level >= 0 && level <= vMAXDIFFICULTYLEVEL, ( "SetDifficultyLevel called with level %i", level ) );
	Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
	uint32 gameMode = skate_mod->GetGameMode()->GetNameChecksum();
	mp_difficulty_levels->AddInteger( gameMode, level );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

GOAL_MANAGER_DIFFICULTY_LEVEL CGoalManager::GetDifficultyLevel( bool career )
{
	uint32 gameMode;
    if ( career)
    {
        gameMode = CRCD(0x4da4937b,"career");
    }
    else
    {
        Mdl::Skate * skate_mod =  Mdl::Skate::Instance();
    	gameMode = skate_mod->GetGameMode()->GetNameChecksum();
    }
	
    int level;
	
	// if no diff level assigned for this game mode yet, assume normal
	if ( !mp_difficulty_levels->GetInteger( gameMode, &level, Script::NO_ASSERT ) )
		level = 1;
	
	GOAL_MANAGER_DIFFICULTY_LEVEL diff_level = GOAL_MANAGER_DIFFICULTY_MEDIUM;
	
    switch ( level )
	{
		case 0:
			diff_level = GOAL_MANAGER_DIFFICULTY_LOW;
			break;
		case 1:
			diff_level = GOAL_MANAGER_DIFFICULTY_MEDIUM;
			break;
		case 2:
			diff_level = GOAL_MANAGER_DIFFICULTY_HIGH;
			break;
		default:
			Dbg_MsgAssert( 0, ( "bad difficulty level %i", level ) );
			break;
	}

	return diff_level;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoalManager::CanRestartStage()
{
	printf("STUBBED\n");
	return false;
/*
	int activeGoal = GetActiveGoal();
	if ( activeGoal != -1 )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( activeGoal, (int*)&key );
		Dbg_Assert( pGoal );
	
		if ( pGoal->m_inherited_flags & GOAL_INHERITED_FLAGS_RETRY_STAGE )
		{
			return true;
		}
	}
	return false;
*/
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::SetGoalChaptersAndStages()
{
	// grab the chapter info array
	Script::CArray* pChapterGoalsArray = Script::GetArray( CRCD( 0xb892776f, "chapter_goals" ), Script::ASSERT );
	Dbg_Assert( pChapterGoalsArray->GetType() == ESYMBOLTYPE_ARRAY );

	int chapter_goals_array_size = pChapterGoalsArray->GetSize();
	for ( int chapter_index = 0; chapter_index < chapter_goals_array_size; chapter_index++ )
	{
		Script::CArray* pStageArray = pChapterGoalsArray->GetArray( chapter_index );
		Dbg_Assert( pStageArray->GetType() == ESYMBOLTYPE_ARRAY );
		int stage_array_size = pStageArray->GetSize();
		for ( int stage_index = 0; stage_index < stage_array_size; stage_index++ )
		{
			Script::CArray* pStage = pStageArray->GetArray( stage_index );
			Dbg_Assert( pStage->GetType() == ESYMBOLTYPE_STRUCTURE );
			int num_goals = pStage->GetSize();
			for ( int goal_index = 0; goal_index < num_goals; goal_index++ )
			{
				Script::CStruct* pGoalStruct = pStage->GetStructure( goal_index );
				uint32 goal_id;
				if ( pGoalStruct->GetChecksum( CRCD( 0x9982e501, "goal_id" ), &goal_id, Script::NO_ASSERT ) )
				{
					CGoal* pGoal = GetGoal( goal_id, false );
					if ( pGoal )
					{
						pGoal->SetChapter( chapter_index );
						pGoal->SetStage( stage_index );
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

bool CGoalManager::AdvanceStage( bool force, uint32 just_won_goal_id )
{
	// printf("CGoalManager::AdvanceStage\n");
	// DeactivateAllGoals();
	
	// K: This was to fix an assert when winning a created goal in the park editor.
	// Fixed by just not doing any advance-stage logic when in created goals mode.
	Mdl::Skate *p_skate_mod = Mdl::Skate::Instance();
	if ( p_skate_mod->GetGameMode()->IsTrue( CRCD( 0x353961d7, "is_creategoals" ) ) )
	{
		return false;
	}	
	
	// grab the chapter info array
	Script::CArray* pChapterGoalsArray = Script::GetArray( CRCD( 0xb892776f, "chapter_goals" ), Script::ASSERT );
	Dbg_Assert( pChapterGoalsArray->GetType() == ESYMBOLTYPE_ARRAY );
	
	// grab num goals to complete array
	Script::CArray* pChapterNumGoalsToComplete = Script::GetArray( CRCD( 0x846f65f5, "CHAPTER_NUM_GOALS_TO_COMPLETE" ), Script::ASSERT );
	Dbg_Assert( pChapterNumGoalsToComplete->GetType() == ESYMBOLTYPE_ARRAY );
	
	if ( m_currentChapter >= (int)pChapterGoalsArray->GetSize() )
	{
		// they beat the game
		return false;
	}
	// printf("m_currentChapter = %i, m_currentStage = %i\n", m_currentChapter, m_currentStage);
	Dbg_MsgAssert( m_currentChapter < (int)pChapterGoalsArray->GetSize(), ( "the current chapter is %i, but the chapter goals array has size %i.", m_currentChapter, pChapterGoalsArray->GetSize() ) );
	Script::CArray* pCurrentChapter = pChapterGoalsArray->GetArray( m_currentChapter );
	Dbg_MsgAssert( m_currentStage < (int)pCurrentChapter->GetSize(), ( "The current stage is set to %i, but the chapter array only has %i stage(s).", m_currentStage, pCurrentChapter->GetSize() ) );
	Script::CArray* pCurrentStage = pCurrentChapter->GetArray( m_currentStage );

	Script::CArray* pChapterTotals = pChapterNumGoalsToComplete->GetArray( m_currentChapter );
	int stage_total_target = pChapterTotals->GetInteger( m_currentStage );
	int num_stages = pChapterTotals->GetSize();

	// resolve just won goal to parent goal, which is what's listed
	// in the chapter info array
	if ( just_won_goal_id )
	{
		// printf("just won goal %s\n", Script::FindChecksumName( just_won_goal_id ) );
		CGoal* pJustWonGoal = GetGoal( just_won_goal_id );
		CGoalLink* pJustWonParents = pJustWonGoal->GetParents();
		while ( pJustWonParents && pJustWonParents->m_relative != 0 )
		{
			just_won_goal_id = pJustWonParents->m_relative;
			pJustWonGoal = GetGoal( pJustWonParents->m_relative );
			pJustWonParents = pJustWonGoal->GetParents();
		}
	}
	
	// check the required total
	int num_goals = pCurrentStage->GetSize();
	int stage_total = 0;

	for ( int goal_index = 0; goal_index < num_goals; goal_index++ )
	{
		Script::CStruct* pGoalStruct = pCurrentStage->GetStructure( goal_index );
		uint32 goal_id;
		if ( pGoalStruct->GetChecksum( CRCD( 0x9982e501, "goal_id" ), &goal_id, Script::NO_ASSERT ) )
		{
			CGoal* pGoal = GetGoal( goal_id, false );
			
			if ( pGoal )
			{
				// resolve to parent goal id
				CGoalLink* pParents = pGoal->GetParents();
				while ( pParents && pParents->m_relative != 0 )
				{
					goal_id = pParents->m_relative;
					pGoal = GetGoal( pParents->m_relative );
					pParents = pGoal->GetParents();				
				}
			}

			if ( pGoal )
			{
				if ( ( goal_id == just_won_goal_id && !pGoal->HasWonGoal() ) || ( pGoal->HasWonGoal() ) )
				{
					stage_total++;
					// printf( "you've won %s, stage_total = %i\n", Script::FindChecksumName( goal_id ), stage_total );
				}
			}
			else
			{
				// look for goal in other level
				Script::CStruct* pTempGoalParams;
				if ( mp_goalFlags->GetStructure( goal_id, &pTempGoalParams, Script::NO_ASSERT ) )
				{
					int hasBeaten = 0;
					pTempGoalParams->GetInteger( "hasBeaten", &hasBeaten, Script::ASSERT );
					if ( hasBeaten != 0 )
					{
						// printf("you've beaten %s in another level\n", Script::FindChecksumName( p_comp->mNameChecksum ) );
						stage_total++;
					}					
				}
			}
		}
	}
	
	// printf("stage_total = %i, stage_total_target = %i\n", stage_total, stage_total_target );
	if ( stage_total >= stage_total_target || force )
	{
		// we've beaten the stage
		if ( m_currentStage + 1 == num_stages )
		{
			// we've finished the chapter
			m_currentChapter++;
			m_currentStage = 0;
		}
		else
		{
			m_currentStage++;
		}

		// printf("m_currentStage = %i\n", m_currentStage);
		// reinitialize the chapter and stage
		UninitializeAllGoals();
		InitializeAllGoals();

		return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::RunLastStageScript()
{
	// DeactivateAllGoals();
	
	// don't do anything if they beat the game
	Script::CArray* pChapterGoalsArray = Script::GetArray( CRCD( 0xb892776f, "chapter_goals" ), Script::ASSERT );
	Dbg_Assert( pChapterGoalsArray->GetType() == ESYMBOLTYPE_ARRAY );
	if ( m_currentChapter >= (int)pChapterGoalsArray->GetSize() )
	{
		return;
	}

	// K: This was to fix an assert when winning a created goal in the park editor.
	// Fixed by just not doing any advance-stage logic when in created goals mode.
	Mdl::Skate *p_skate_mod = Mdl::Skate::Instance();
	if ( p_skate_mod->GetGameMode()->IsTrue( CRCD( 0x353961d7, "is_creategoals" ) ) )
	{
		return;
	}	
	
	// grab the chapter scripts arrray
	Script::CArray* pChapterScriptsArray = Script::GetArray( CRCD(0x9f00892d,"CHAPTER_COMPLETION_SCRIPTS"), Script::ASSERT );
	Dbg_Assert( pChapterScriptsArray->GetType() == ESYMBOLTYPE_ARRAY );
	
	int last_stage_index = m_currentStage;
	int last_chapter_index = m_currentChapter;

	if ( last_stage_index == 0 )
	{
		if ( last_chapter_index == 0 )
		{
			// we're on the first chapter and stage - don't do anything
			return;
		}
		last_chapter_index--;

		Dbg_MsgAssert( last_chapter_index < (int)pChapterScriptsArray->GetSize(), ( "last_chapter_index %i too big", last_chapter_index ) );

		// figure the stage index
		Script::CArray* pChapterArray = pChapterScriptsArray->GetArray( last_chapter_index );
		Dbg_Assert( pChapterArray );
		last_stage_index = pChapterArray->GetSize() - 1;
		// [ { script_name=NJ_StageSwitch_0_to_1_1 }, { script_name=NJ_StageSwitch_1_1_to_1_2 }, { script_name=NJ_StageSwitch_1_2_to_2_1 } ]
	}
	else
		last_stage_index--;

	uint32 script_name;
	Script::CArray* pChapterParamsArray = pChapterScriptsArray->GetArray( last_chapter_index );
	Dbg_Assert( pChapterParamsArray );
	Script::CStruct* pStageScriptStruct = pChapterParamsArray->GetStructure( last_stage_index );
	Dbg_Assert( pStageScriptStruct );
	if ( pStageScriptStruct->GetChecksum( CRCD(0x6166f3ad,"script_name"), &script_name, Script::NO_ASSERT ) )
	{
		Script::SpawnScript( script_name );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::SetTeamMember( uint32 pro, bool remove, bool mark_used )
{
	if ( remove )
		mp_team->RemoveComponent( pro );
	else
	{
		if ( mark_used )
			mp_team->AddInteger( pro, 1 );	
		else
			mp_team->AddInteger( pro, 0 );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::KillTeamMembers( )
{
	mp_team->Clear();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::SetTeamName( const char* pTeamName )
{
	mp_team->AddString( CRCD(0x703d7582,"team_name"), pTeamName );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoalManager::HideAllGoalPeds( bool hide )
{
	for ( int i = 0; i < GetNumGoals(); i++ )
	{
		uint32 key;
		CGoal* pGoal = m_goalList.GetItemByIndex( i, (int*)&key );
		Dbg_Assert( pGoal );
		pGoal->mp_goalPed->Hide( hide );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGoalManager* GetGoalManager()
{
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	return skate_mod->GetGoalManager();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_AddGoal | 
// @parm name | name | the goal id
// @parm structure | params | goal parameters
bool ScriptAddGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	Script::CStruct* pSubParams;
	pParams->GetStructure( CRCD(0x7031f10c,"params"), &pSubParams, Script::ASSERT );

	return pGoalManager->AddGoal( goalId, pSubParams );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_RemoveGoal | 
// @parm name | name | the goal id
bool ScriptRemoveGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );
	pGoalManager->RemoveGoal( goalId );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetGoal | currently does nothing at all!
// @parm name | name | the goal id
bool ScriptGetGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
	#if 0
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );
	CGoal* pGoal = pGoalManager->GetGoal( goalId );
	if ( pGoal )
	{
	}
	#endif
	
	// TODO: return it in the script components

	// flags are not allowed as params,
	// otherwise there will be a leak
	// as things get appended, and re-appended

	// activate goal->
	// should turn off the score degrader
	// and the score accumulation script 

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_EditGoal | this will append the given structure
// onto the goal's existing params structure
// @parm name | name | the goal id
// @parm structure | params | structure to append
bool ScriptEditGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	Script::CStruct* pSubParams;
	pParams->GetStructure( CRCD(0x7031f10c,"params"), &pSubParams, Script::ASSERT );

	return pGoalManager->EditGoal( goalId, pSubParams );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_ActivateGoal |
// @parm name | name | the goal id
// @flag dontAssert | pass this in if you don't want the goal manager 
// to assert if it can't find the specified goal (not recommended)
bool ScriptActivateGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

    // put in for testing
    bool dontAssert = false;
    if ( pParams->ContainsFlag( CRCD(0x9ac8d54f,"dontAssert") ) )
        dontAssert = true;
	
	return pGoalManager->ActivateGoal( goalId, dontAssert );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_DeactivateGoal | 
// @parm name | name | the goal id
bool ScriptDeactivateGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );
	return pGoalManager->DeactivateGoal( goalId );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_DeactivateGoal | 
bool ScriptClearLastGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	
	pGoalManager->ClearLastGoal();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_WinGoal | 
// @parm name | name | the goal id
bool ScriptWinGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
	#ifdef	__NOPT_ASSERT__
	bool had_object = false;
	uint32	old_id = 0;
	Obj::CObjectPtr p_obj;
	if (pScript->mpObject)
	{
		had_object = true;
		p_obj = pScript->mpObject;
		old_id = p_obj->GetID();
	}
	#endif
	
	
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );
	bool result = pGoalManager->WinGoal( goalId );

	#ifdef	__NOPT_ASSERT__
	if (had_object)
	{
		Dbg_MsgAssert(p_obj,("%s\nObject %s was deleted indirectly \nWhen running a script via GoalManager_WinGoal\n You probably use KILL PREFIX = ...., Maybe you could use DIE instead",pScript->GetScriptInfo(),Script::FindChecksumName(old_id)));
	}
	#endif
		
	return result;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_LoseGoal | 
// @parm name | name | the goal id
bool ScriptLoseGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
	
	#ifdef	__NOPT_ASSERT__
	bool had_object = false;
	uint32	old_id = 0;
	Obj::CObjectPtr p_obj;
	if (pScript->mpObject)
	{
		had_object = true;
		p_obj = pScript->mpObject;
		old_id = p_obj->GetID();
	}
	#endif
	
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );
	bool result = pGoalManager->LoseGoal( goalId );

	#ifdef	__NOPT_ASSERT__
	if (had_object)
	{
		Dbg_MsgAssert(p_obj,("%s\nObject %s was deleted indirectly when running a script via GoalManager_LoseGoal\n You probaly use KILL PREFIX = ...., Maybe you could use DIE instead",pScript->GetScriptInfo(),Script::FindChecksumName(old_id)));
	}
	#endif
	
	
	return result;
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_RemoveAllGoals | this will completely destroy
// all the goals currently in existence...make sure you call GoalManager_LevelUnload
// first if you want to remember anything about the state of the goals before
// destroying them.
bool ScriptRemoveAllGoals(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	pGoalManager->RemoveAllGoals();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_DeactivateAllGoals | 
bool ScriptDeactivateAllGoals(Script::CStruct *pParams, Script::CScript *pScript)
{
	bool including_net_goals;
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	including_net_goals = false;
	if( pParams->ContainsComponentNamed( CRCD(0xcc365298,"force_all")))
	{
		including_net_goals = true;
	}

	pGoalManager->DeactivateAllGoals( including_net_goals );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | UninitializeAllGoals |
bool ScriptUninitializeAllGoals(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	pGoalManager->UninitializeAllGoals();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_UninitializeGoal |
// @parm name | name | the goal id
bool ScriptUninitializeGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );
	if ( pParams->ContainsFlag( CRCD(0x687de558,"affect_tree") ) )
	{
		pGoalManager->UninitializeGoalTree( goalId );
	}
	else
	{
		pGoalManager->UninitializeGoal( goalId );
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_InitializeAllGoals | 
bool ScriptInitializeAllGoals(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	pGoalManager->InitializeAllGoals();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_InitializeAllSelectedGoals | 
bool ScriptInitializeAllSelectedGoals(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	pGoalManager->InitializeAllSelectedGoals();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_UpdateAllGoals | This forces the goal manager to run
// an update cycle.
bool ScriptUpdateAllGoals(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	pGoalManager->Update();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_HasActiveGoals | returns true if there is at least one
// active goal (excluding minigames and other goals that don't count towards the total)
bool ScriptHasActiveGoals(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	bool count_all = pParams->ContainsFlag( CRCD(0xa21884b5,"count_all") );

	return ( pGoalManager->GetNumActiveGoals( count_all ) > 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GoalIsActive | returns true if the specified goal
// is currently active
// @parm name | name | the goal id
bool ScriptGoalIsActive(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );
	return pGoalManager->GoalIsActive( goalId );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_SetGoalTimer | resets the goal timer to it's inital
// value.
// @parm name | name | the goal id
bool ScriptSetGoalTimer(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );
	
	return pGoalManager->SetGoalTimer( goalId );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_ZeroGoalTimer | zeros the goal timer; added to allow a hack to fix split screen horse end runs to be minimally invasive
// @parm name | name | the goal id
bool ScriptZeroGoalTimer(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );
	
	return pGoalManager->SetGoalTimer( goalId, 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_PauseAllGoals | 
bool ScriptPauseAllGoals(Script::CStruct *pParams, Script::CScript *pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    pGoalManager->PauseAllGoals();
    return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_UnPauseAllGoals | 
bool ScriptUnPauseAllGoals(Script::CStruct *pParams, Script::CScript *pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    pGoalManager->UnPauseAllGoals();
    return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_SetGoalFlag | for setting the special goal flags
// @parm name | name | the goal id
// @uparm name | the name of the flag to set
// @uparm 1 | value to use 
bool ScriptSetGoalFlag(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );
	
	uint32 flagChecksum;
	pParams->GetChecksum( NONAME, &flagChecksum, Script::ASSERT );

	int flagValue;
	pParams->GetInteger( NONAME, &flagValue, Script::ASSERT );

    bool success = pGoalManager->SetGoalFlag( goalId, flagChecksum, flagValue );

    return success;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_HasSeenGoal | returns true if the player has
// activated the specified goal at least once
// @parm name | name | the goal id
bool ScriptHasSeenGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    uint32 goalId;
    pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

    return ( pGoalManager->HasSeenGoal( goalId ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_RestartLastGoal | this will restart the last goal
// It will fail silently if there is no goal to retry
bool ScriptRestartLastGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    pGoalManager->RestartLastGoal();

    return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*// @script | GoalManager_CreateGoalFlag | 
// @parm name | name | the goal id
// @parm name | flag | the name of the flag to create
bool ScriptCreateGoalFlag(Script::CStruct *pParams, Script::CScript *pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    uint32 name;
    pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &name, Script::ASSERT );

    uint32 flag;
    pParams->GetChecksum( CRCD(0x2e0b1465,"flag"), &flag, Script::ASSERT ); 

    pGoalManager->CreateGoalFlag( name, flag );
    return true;
}*/


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_AllFlagsSet | 
// @parm name | name | the goal id
bool ScriptAllFlagsSet(Script::CStruct *pParams, Script::CScript *pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    uint32 name;
    pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &name, Script::ASSERT );

    return pGoalManager->AllFlagsSet( name );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GoalFlagSet | true if a goal flag has been set
// @parm name | name | the goal id
// @parm name | flag | the name of the flag to check
bool ScriptGoalFlagSet(Script::CStruct *pParams, Script::CScript *pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );
	
	uint32 flag;
	pParams->GetChecksum( CRCD(0x2e0b1465,"flag"), &flag, Script::ASSERT );

	return pGoalManager->GoalFlagSet( goalId, flag );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_CanRetryGoal | returns true if there is a goal
// to retry
bool ScriptCanRetryGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    return pGoalManager->CanRetryGoal();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetGoalParams | this will append the goal's 
// params onto the scripts params.  The goal manager scripts use this
// extensively to avoid passing large structures back and forth.
// @parm name | name | the goal id
bool ScriptGetGoalParams(Script::CStruct *pParams, Script::CScript *pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    uint32 goalId;
    pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );
    
    pScript->GetParams()->AppendStructure( pGoalManager->GetGoalParams( goalId ) );

    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_CanStartGoal |
// @parm name | name | the goal id
bool ScriptCanStartGoal(Script::CStruct *pParams, Script::CScript *pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    // uint32 goalId;
    // pParams->GetChecksum( "name", &goalId, Script::ASSERT );

    return pGoalManager->CanStartGoal();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_NextRaceWaypoint | 
// @parm name | name | the goal id
bool ScriptNextRaceWaypoint(Script::CStruct *pParams, Script::CScript *pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    uint32 goalId;
    pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

    return pGoalManager->NextRaceWaypoint( goalId );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_NextTourSpot | 
// @parm name | name | the goal id
bool ScriptNextTourSpot(Script::CStruct *pParams, Script::CScript *pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    uint32 goalId;
    pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

    return pGoalManager->NextTourSpot( goalId );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_InitTrickObjects |
// @parm name | name | the goal id
bool ScriptInitTrickObjects( Script::CStruct* pParams, Script::CScript* pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );
    
    uint32 goalId;
    pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

    pGoalManager->InitTrickObjects( goalId );

    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_CreateGoalName | 
// @parm string | goal_type | the goal type
bool ScriptCreateGoalName( Script::CStruct* pParams, Script::CScript* pScript )
{
    char goal_prefix[128];
    const char* goal_type;

    pParams->GetText( CRCD(0x6d11ed74,"goal_type"), &goal_type, Script::ASSERT );

	// Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	// Obj::CSkaterCareer* pCareer = skate_mod->GetCareer();

	// create the level prefix
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	Script::CStruct* p_level_structure = Script::GetStructure( pGoalManager->GetLevelStructureName(), Script::ASSERT );
	const char* p_level_prefix;
	p_level_structure->GetString( CRCD(0x651533ec,"level"), &p_level_prefix, Script::ASSERT );
	strcpy( goal_prefix, p_level_prefix );
	strcat( goal_prefix, "_goal_" );

/*	// figure the level
	switch ( level )
	{
	case 1:                                 // school
		strcpy( goal_prefix, "Sch_goal_" );
		break;
	case 2:                                 // san francisco
		strcpy( goal_prefix, "Sf2_goal_" );
		break;
	case 3:                                 // alcatraz
		strcpy( goal_prefix, "Alc_goal_" );
		break;
	case 4:                                 // Kona park
		strcpy( goal_prefix, "Kon_goal_" );
		break;
	case 5:                                 // junkyard
		strcpy( goal_prefix, "Jnk_goal_" );
		break;
	case 6:                                 // london
		strcpy( goal_prefix, "Lon_goal_" );
		break;
	case 7:                                 // zoo
		strcpy( goal_prefix, "Zoo_goal_" );
		break;
	case 8:									// carnival
		strcpy( goal_prefix, "Cnv_Goal_" );
		break;
	case 9:									// chicago
		strcpy( goal_prefix, "Hof_goal_" );
		break;
	case 10:								// default test level
		strcpy( goal_prefix, "Default_goal_" );
		break;
	default:
		Dbg_MsgAssert( false, ( "GetGoalName couldn't figure level number" ) );
	}
*/
    
    // create the goal name
    strcat( goal_prefix, (char*)goal_type );

    // add to pParams
    pScript->GetParams()->AddChecksum( CRCD(0x9982e501,"goal_id"), Script::GenerateCRC( goal_prefix ) );
	pScript->GetParams()->AddString( CRCD(0xbfecc45b,"goal_name"), goal_prefix );

    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetLevelPrefix | returns the current level's prefix in level_prefix
bool ScriptGetLevelPrefix( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	Script::CStruct* p_level_structure = Script::GetStructure( pGoalManager->GetLevelStructureName(), Script::ASSERT );
	const char* p_level_prefix;
	p_level_structure->GetString( CRCD(0x651533ec,"level"), &p_level_prefix, Script::ASSERT );
	
	pScript->GetParams()->AddString( CRCD(0x15f818d0,"level_prefix"), p_level_prefix);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_SetGraffitiMode |
// @uparmopt 1 | turn on or off with 1 or 0
bool ScriptSetGraffitiMode( Script::CStruct* pParams, Script::CScript* pScript )
{
    CGoalManager* pGoalManager = GetGoalManager();

    int mode = 1;
    pParams->GetInteger( NONAME, &mode, Script::NO_ASSERT ); 

    return pGoalManager->SetGraffitiMode( mode );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_HasWonGoal | true if you've beat this goal
// @parm name | name | the goal id
bool ScriptHasWonGoal( Script::CStruct* pParams, Script::CScript* pScript )
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    uint32 goalId;
    pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

    return pGoalManager->HasWonGoal( goalId );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGoalIsSelected(Script::CStruct* pParams, Script::CScript* pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    uint32 goalId;
    pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

    return pGoalManager->GoalIsSelected( goalId );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptToggleGoalSelection(Script::CStruct* pParams, Script::CScript* pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    uint32 goalId;
    pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

    pGoalManager->ToggleGoalSelection( goalId );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptGoalsAreSelected(Script::CStruct* pParams, Script::CScript* pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

	return ( pGoalManager->GetNumSelectedGoals() > 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GotCounterObject | 
// @parm name | name | the goal id
bool ScriptGotCounterObject( Script::CStruct* pParams, Script::CScript* pScript )
{
    uint32 goalId;
    if ( pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::NO_ASSERT ) )
    {
        CGoalManager* pGoalManager = GetGoalManager();
        pGoalManager->GotCounterObject( goalId );
        return true;
    }
    else 
    {
        // printf("didn't get goalId\n");
    }
    return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_CounterGoalDone | returns true if the counter goal
// is complete
// @parm name | name | the goal id
bool ScriptCounterGoalDone(Script::CStruct* pParams, Script::CScript* pScript)
{
    uint32 goalId;
    pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

    CGoalManager* pGoalManager = GetGoalManager();

    return pGoalManager->CounterGoalDone( goalId );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_AddGoalPoint | adds one goal point
bool ScriptAddGoalPoint(Script::CStruct* pParams, Script::CScript* pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	pGoalManager->AddGoalPoint();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_SpendGoalPoints | 
// @uparm 1 | number to spend
bool ScriptSpendGoalPoints(Script::CStruct* pParams, Script::CScript* pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    int num;
    pParams->GetInteger( NONAME, &num, Script::ASSERT );

    pGoalManager->SpendGoalPoints( num );
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_HasGoalPoints | 
// @uparm 1 | number to check for
bool ScriptHasGoalPoints(Script::CStruct* pParams, Script::CScript* pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    int num;
    pParams->GetInteger( NONAME, &num, Script::ASSERT );
    
    return pGoalManager->HasGoalPoints( num );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptClearGoalPoints(Script::CStruct* pParams, Script::CScript* pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    pGoalManager->ClearGoalPoints();
    
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetNumberOfGoalPoints | places the number of
// goal points in a variable named "goal_points" in the script params
bool ScriptGetNumberOfGoalPoints(Script::CStruct* pParams, Script::CScript* pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    int goal_points = 0;
	if ( pParams->ContainsFlag( CRCD(0x3dd01ea1,"total") ) )
		goal_points = pGoalManager->GetTotalNumberOfGoalPointsEarned();
	else
		goal_points = pGoalManager->GetNumberOfGoalPoints();

    pScript->GetParams()->AddInteger( CRCD(0x42c20492,"goal_points"), goal_points );
    return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_AddCash | gives the player more cash
// @uparm 1 | amount of cash to give
bool ScriptAddCash(Script::CStruct* pParams, Script::CScript* pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    int amount;
    pParams->GetInteger( NONAME, &amount, Script::ASSERT );

    pGoalManager->AddCash( amount );
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_SpendCash | spends some of the player's cash
// @uparm 1 | amount to spend
bool ScriptSpendCash(Script::CStruct* pParams, Script::CScript* pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    int amount;
    pParams->GetInteger( NONAME, &amount, Script::ASSERT );

    return pGoalManager->SpendCash( amount );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetCash | places the current player's cash amount
// in a parameter called "cash" in the script params
bool ScriptGetCash(Script::CStruct* pParams, Script::CScript* pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

	bool get_total = pParams->ContainsFlag( CRCD(0x3dd01ea1,"total") );
    
    pScript->GetParams()->AddInteger( CRCD(0xf9461a46,"cash"), pGoalManager->GetCash( get_total ) );
    return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_HasWonGoalWithProset | 
// @uparm "string" | the proset_prefix you wanna check
bool ScriptHasBeatenGoalWithProset(Script::CStruct* pParams, Script::CScript* pScript)
{
    const char* p_proset_prefix;
    pParams->GetString( NONAME, &p_proset_prefix, Script::ASSERT );

    CGoalManager* pGoalManager = GetGoalManager();
    return pGoalManager->HasBeatenGoalWithProset( p_proset_prefix );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetProsetNotPrefix | 
// @uparm "string" | the proset_prefix you wanna check
bool ScriptGetProsetNotPrefix(Script::CStruct* pParams, Script::CScript* pScript)
{
    const char* p_proset_prefix;
    pParams->GetString( NONAME, &p_proset_prefix, Script::ASSERT );

    char temp_string[128];
    strcpy( temp_string, "");

    int size = 0;    
    while ( p_proset_prefix[size + 1] )
    {
        temp_string[size] = p_proset_prefix[size];
        size++;
    }
	temp_string[size] = '\0';

    // now we're at the last char
    strcat( temp_string, "NOT_" );

    pScript->GetParams()->AddString( CRCD(0xc60ce720,"proset_not_prefix"), temp_string );
    return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_LevelLoad | called when the player enters a level.
// Loads data about goals seen, completed, etc. in a previous game
bool ScriptLevelLoad(Script::CStruct* pParams, Script::CScript* pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    pGoalManager->LevelLoad();
    return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_LevelUnload | called when the player leaves a level.
// Prompts the goal manager to save state data about all goals
bool ScriptLevelUnload(Script::CStruct* pParams, Script::CScript* pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    pGoalManager->LevelUnload();
    return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_NumGoalsBeatenInLevel | This will return the number
// of beaten goals in the scripts params as num_beaten
// @uparm 1 | level number
// @flag count_pro_specific_challenges | false by default
bool ScriptNumGoalsBeatenInLevel(Script::CStruct* pParams, Script::CScript* pScript)
{
    int levelNum;
    pParams->GetInteger( NONAME, &levelNum, Script::ASSERT );

    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    int numbeat = pGoalManager->NumGoalsBeatenInLevel( levelNum, pParams->ContainsFlag( CRCD(0x8690a9cf,"count_pro_specific_challenges") ) );
    pScript->GetParams()->AddInteger( CRCD(0x8110dcdf,"num_beaten"), numbeat );
    return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_UnlockAllGoals | Unlocks all locked goals
bool ScriptUnlockAllGoals(Script::CStruct* pParams, Script::CScript* pScript)
{
    CGoalManager* pGoalManager = GetGoalManager();
    Dbg_Assert( pGoalManager );

    pGoalManager->UnlockAllGoals();
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_CheckMinigameRecord | checks the current minigame
// value against the one stored in minigame_record in the goal params
// @parm name | name | the goal id
// @uparm 100 | the value you want to check against the current record
bool ScriptCheckMinigameRecord(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	int value;
	pParams->GetInteger( NONAME, &value, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	return pGoalManager->CheckMinigameRecord( goalId, value );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_DeactivateCurrentGoal | Deactivates the currently
// active goal (this function cannot always be trusted).
bool ScriptDeactivateCurrentGoal(Script::CStruct* pParams, Script::CScript* pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->DeactivateCurrentGoal();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_SetStartTime | Records the start time of a combo
// (only used for the timedcombo miingame right now).
// @parm name | name | the goal id
bool ScriptSetStartTime(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );
	
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->SetStartTime( goalId );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_UpdateComboTimer | Used for the timedcombo minigame
// @parm name | name | the goal id
bool ScriptUpdateComboTimer(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->UpdateComboTimer( goalId );
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_SetStartHeight | used for the height minigame
// @parm name | name | the goal id
bool ScriptSetStartHeight(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->SetStartHeight( goalId );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_CheckHeightRecord | used for height minigame
// @parm name | name | the goal id
bool ScriptCheckHeightRecord(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	return pGoalManager->CheckHeightRecord( goalId );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_CheckDistanceRecord | used for height minigame
// @parm name | name | the goal id
bool ScriptCheckDistanceRecord(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	return pGoalManager->CheckDistanceRecord( goalId );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_PlayGoalStartStream | plays the intro vo for the goal
// @parm name | name | the goal id
bool ScriptPlayGoalStartStream(Script::CStruct* pParams, Script::CScript* pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->PlayGoalStartStream( pParams );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_PlayGoalWaitStream | plays the wait vo for the goal
// @parm name | name | the goal id
bool ScriptPlayGoalWaitStream(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->PlayGoalWaitStream( goalId );
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_PlayGoalWinStream | plays the success vo for the goal
// @parm name | name | the goal ID
bool ScriptPlayGoalWinStream(Script::CStruct* pParams, Script::CScript* pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->PlayGoalWinStream( pParams );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_StopCurrentStream | stops any vo being played by
// the goal trigger
// @parm name | name | the goal ID
bool ScriptStopCurrentStream(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->StopCurrentStream( goalId );
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_TurnPro | instantly turns you pro, unlocks all 
// the pro goals, and runs the "you just turned pro" script
bool ScriptTurnPro(Script::CStruct* pParams, Script::CScript* pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->TurnPro();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_DeactivateAllMinigames | 
bool ScriptDeactivateAllMinigames(Script::CStruct* pParams, Script::CScript* pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->DeactivateAllMinigames();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_InitializeAllMinigames | 
bool ScriptInitializeAllMinigames(Script::CStruct* pParams, Script::CScript* pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->InitializeAllMinigames();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_PauseGoal | pauses an individual goal
// @parm name | name | the goal id
bool ScriptPauseGoal( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->PauseGoal( goalId );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_UnPauseGoal | unpauses an individual goal
// @parm name | name | the goal id
bool ScriptUnPauseGoal( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->UnPauseGoal( goalId );
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_PauseCompetition | Pauses a competition
// @parm name | name | the goal id
bool ScriptPauseCompetition( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->PauseCompetition( goalId );
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_UnPauseCompetition | The competition uses a secondary
// pause mechanism...this is for unpausing the comp between rounds
// @parm name | name | the goal id
bool ScriptUnPauseCompetition( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->UnPauseCompetition( goalId );
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_UnBeatAllGoals | marks all goals as unbeaten and 
// takes away any goal points you earned for beating them.  You have to 
// reload the level to re-initialize the goals, however
bool ScriptUnBeatAllGoals( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_MsgAssert( pGoalManager, ("couldn't get goal manager") );

	pGoalManager->UnBeatAllGoals();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_AddViewGoalsList | adds the list of goals to the 
// view goals menu.  This is called from one place within the create_view_Goals_menu
// script.
bool ScriptAddViewGoalsList( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_MsgAssert( pGoalManager, ("couldn't get goal manager") );

	pGoalManager->AddViewGoalsList();
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_AddGoalChoices | Adds the list of available goals to the "Choose Goals" menu
bool ScriptAddGoalChoices( Script::CStruct* pParams, Script::CScript* pScript )
{
	bool selected_only;
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_MsgAssert( pGoalManager, ("couldn't get goal manager") );

	selected_only = pParams->ContainsFlag( CRCD(0x5685c1b7,"selected_only") );

	pGoalManager->AddGoalChoices( selected_only );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GoalIsLocked | true if the goal is locked
// @parm name | name | the goal id
bool ScriptGoalIsLocked( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	return pGoalManager->GoalIsLocked( goalId );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_IsInCompetition | returns true if the player is
// in a competition
bool ScriptIsInCompetition( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	CGoal* pGoal = pGoalManager->IsInCompetition();
	if ( pGoal )
	{
		pScript->GetParams()->AddChecksum( CRCD(0x9982e501,"goal_id"), pGoal->GetGoalId() );
		return true;
	}
	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetGoalAnimations | Gets the appropriate animations
// array from script.  It wil first search for an array specific to this goal and 
// type, then for an array specific to this pro, then for the generic array
// @parm name | name | the goal id
// @parmopt string | type | | the goal state - wait, start, midgoal, or success
bool ScriptGetGoalAnimations( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	const char* type = NULL;
	pParams->GetString( CRCD(0x7321a8d6,"type"), &type, Script::NO_ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->GetGoalAnimations( goalId, type, pScript );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_PlayGoalStream | plays the appropriate stream for this
// goal/pro.  The appropriate anim arrays must be setup in script for this to work
// @parm name | name | the goal id
bool ScriptPlayGoalStream( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->PlayGoalStream( pParams );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_EndRunCalled | returns true if the goal manager
// called endrun for this goal
// @parm name | name | the goal id
bool ScriptEndRunCalled(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	return pGoalManager->EndRunCalled( goalId );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_ClearEndRun | This will clear the skater's endrun exception
// as well as any others.  It's used by the combo goal, which needs to clear
// the exception if you bail.  In general, this function should not be used -
// clearing the endrun exception is not usually a good idea.
// @parm name | name | the goal id
bool ScriptClearEndRun(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );
	
	CGoalManager* pGoalManager = GetGoalManager();
	pGoalManager->GetGoal( goalId )->ClearEndRun( pScript );
	
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_FinishedEndOfRun | returns true if EndOfRun has
// finished
// @parm name | name | the goal id
bool ScriptFinishedEndOfRun(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	return pGoalManager->GetGoal( goalId )->FinishedEndOfRun();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_StartedEndOfRun | this returns true if EndOfRun 
// has started
// @parm name | name | the goal id
bool ScriptStartedEndOfRun(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	return pGoalManager->GetGoal( goalId )->StartedEndOfRun();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_EndBetAttempt | call this to end a bet attempt. 
// If the goal id does not match the current bet, the call will be ignored.
// @parm name | name | the goal id
bool ScriptEndBetAttempt(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 id = pGoalManager->GetBettingGuyId();
	if ( id )
	{
		CBettingGuy* pGoal = (CBettingGuy*)pGoalManager->GetGoal( id );
		Dbg_Assert( pGoal );
		return pGoal->EndBetAttempt( goalId );
	}
	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_StartBetAttempt | called by any minigame that is
// also used as a betting game
// @parm name | name | the goal id
bool ScriptStartBetAttempt(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 id = pGoalManager->GetBettingGuyId();
	if ( id )
	{
		CBettingGuy* pGoal = (CBettingGuy*)pGoalManager->GetGoal( id );
		Dbg_Assert( pGoal );
		return pGoal->StartBetAttempt( goalId );
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_WinBet | calling this will win the bet.
// The goal id is used to verify that the specified bet is in fact
// the active bet.  If it's not, the goal manager will ignore the call
// @parm name | name | the goal id
bool ScriptWinBet(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 id = pGoalManager->GetBettingGuyId();
	if ( id )
	{
		CBettingGuy* pGoal = (CBettingGuy*)pGoalManager->GetGoal( id );
		Dbg_Assert( pGoal );
		pGoal->Win( goalId );
	}
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_BetOffered | called once the betting guy makes 
// an offer
bool ScriptBetOffered( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 id = pGoalManager->GetBettingGuyId();
	if ( id )
	{	
		CBettingGuy* pGoal = (CBettingGuy*)pGoalManager->GetGoal( id );
		Dbg_Assert( pGoal );
		pGoal->OfferMade();
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_BetRefused | called when the player declines a betting
// guy bet
bool ScriptBetRefused( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 id = pGoalManager->GetBettingGuyId();
	if ( id )
	{	
		CBettingGuy* pGoal = (CBettingGuy*)pGoalManager->GetGoal( id );
		Dbg_Assert( pGoal );
		pGoal->OfferRefused();
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_MoveBettingGuyNow | this will force the betting guy to
// immediately move to a new, random betting game.  This is useful when the betting guy
// is created and you don't care where the skater is (otherwise the betting guy will avoid
// moving where the skater could see him).
bool ScriptMoveBettingGuyNow( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 id = pGoalManager->GetBettingGuyId();
	if ( id )
	{
		CBettingGuy* pGoal = (CBettingGuy*)pGoalManager->GetGoal( id );
		Dbg_Assert( pGoal );
		pGoal->MoveToNewNode( true );
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_BetAccepted | called when the player accepts a bet
bool ScriptBetAccepted(Script::CStruct* pParams, Script::CScript* pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 id = pGoalManager->GetBettingGuyId();
	if ( id )
	{
		CBettingGuy* pGoal = (CBettingGuy*)pGoalManager->GetGoal( id );
		Dbg_Assert( pGoal );
		pGoal->OfferAccepted();
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_BetIsActive | returns true if the bet is currently
// active - ie, the bet guy has offered the bet and the player accepted
// @parm name | name | the goal id
bool ScriptBetIsActive(Script::CStruct* pParams, Script::CScript* pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	uint32 betting_guy_id = pGoalManager->GetBettingGuyId();
	if ( betting_guy_id )
	{
		CBettingGuy* pBetGuy = (CBettingGuy*)pGoalManager->GetGoal( betting_guy_id );
		Dbg_Assert( pBetGuy );
		return pBetGuy->BetIsActive( goalId );
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_AddMinigameTime | adds the specified amount of time
// to the minigame.  
// @parm name | name | the minigame id (usually goal_id)
// @uparm 1 | the amount of time to add in seconds
bool ScriptAddMinigameTime(Script::CStruct* pParams, Script::CScript* pScript)
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );	
	
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	int amount;
	pParams->GetInteger( NONAME, &amount, Script::ASSERT );

	CGoal* pGoal = pGoalManager->GetGoal( goalId );
	Dbg_MsgAssert( pGoal->IsMinigame(), ("This is not a minigame") );
	CMinigame* pMinigame = (CMinigame*)pGoal;
	return pMinigame->AddTime( amount );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_AddTempSpecialTrick | This is used only by the 
// special goal.
// @parm name | name | the goal id
bool ScriptAddTempSpecialTrick(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	CGoal* pGoal = pGoalManager->GetGoal( goalId );
	Dbg_Assert( pGoal );

	return pGoal->AddTempSpecialTrick();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_RemoveTempSpecialTrick | this is used only by the 
// special goal.
// @parm name | name | the goal id
bool ScriptRemoveTempSpecialTrick(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	CGoal* pGoal = pGoalManager->GetGoal( goalId );
	Dbg_Assert( pGoal );

	pGoal->RemoveTempSpecialTrick();
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetTrickFromKeyCombo | this will find the trick
// currently bound to the given key combo.  the trick name (checksum value) will
// be placed in the script params as trick_checksum.  The trick text
// will be placed in the params as trick_string.  If no trick is bound to the 
// key combo, the function will return false
// @parm name | key_combo | the key combo to search for
bool ScriptGetTrickFromKeyCombo(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 key_combo;
	pParams->GetChecksum( CRCD(0xacfdb27a,"key_combo"), &key_combo, Script::ASSERT );

	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Obj::CSkaterProfile* pSkaterProfile = pSkate->GetCurrentProfile();
	Script::CStruct* pTricks = pSkaterProfile->GetTrickMapping( CRCD(0xd544aa2d,"trick_mapping") );
	
	uint32 trick_checksum;
    int cat_num;
	if ( !pParams->ContainsFlag( CRCD(0xb394c01c,"special") ) )
    {
        if( pTricks->GetInteger( key_combo, &cat_num, Script::NO_ASSERT ) )
        {
            // printf("it's a created trick! %i\n", cat_num );
    		pScript->GetParams()->AddInteger( CRCD(0xa75b8581,"cat_num"), cat_num );
            return true;
        }
        
        if( pTricks->GetChecksum( key_combo, &trick_checksum, Script::NO_ASSERT ) )
    	{
    		if ( trick_checksum == ( CRCD(0xf60c9090,"Unassigned") ) )
               	return false;
            
    		// return the name and checksum
    		const char* p_trick_string;
    		Script::CStruct* p_trick = Script::GetStructure( trick_checksum, Script::ASSERT );
    		Script::CStruct* p_trick_params;
    		p_trick->GetStructure( CRCD(0x7031f10c,"Params"), &p_trick_params, Script::ASSERT );
    		p_trick_params->GetLocalString( CRCD(0xa1dc81f9,"Name"), &p_trick_string, Script::ASSERT );
    
    		pScript->GetParams()->AddString( CRCD(0x3a54cc4b,"trick_string"), p_trick_string );
    		pScript->GetParams()->AddChecksum( CRCD(0x4443a4da,"trick_checksum"), trick_checksum );
    
    		// look for any double tap tricks
    		uint32 extra_trick = 0;
    		if ( !p_trick_params->GetChecksum( CRCD(0x6e855102,"ExtraTricks"), &extra_trick, Script::NO_ASSERT ) )
    		{
    			// look for an array of extra tricks and grab the first one
    			Script::CArray* p_extra_tricks;
    			if ( p_trick_params->GetArray( CRCD(0x6e855102,"ExtraTricks"), &p_extra_tricks, Script::NO_ASSERT ) )
    				extra_trick = p_extra_tricks->GetChecksum( 0 );
    		}
    
    		// add any extra trick info
    		if ( extra_trick )
    		{
    			// grab this extra trick
    			Script::CArray* p_temp = Script::GetArray( extra_trick, Script::ASSERT );
    			Script::CStruct* p_extra_trick = p_temp->GetStructure( 0 );
    			Dbg_Assert( p_extra_trick );
    			Script::CStruct* p_extra_trick_params;
    			p_extra_trick->GetStructure( CRCD(0x7031f10c,"Params"), &p_extra_trick_params, Script::ASSERT );
    			const char* p_extra_trick_string;
    			p_extra_trick_params->GetString( CRCD(0xa1dc81f9,"Name"), &p_extra_trick_string, Script::ASSERT );
    			pScript->GetParams()->AddString( CRCD(0x3daaf58b,"extra_trick_string"), p_extra_trick_string );
    			pScript->GetParams()->AddChecksum( CRCD(0xd0bcfc62,"extra_trick_checksum"), extra_trick );
    		}
    		return true;
    	}
    }
	else if ( pParams->ContainsFlag( CRCD(0xb394c01c,"special") ) )
	{		
		// check the special tricks
		int numTricks = pSkaterProfile->GetNumSpecialTrickSlots();

		// search through current special tricks
		for ( int i = 0; i < numTricks; i++ )
		{
            Obj::SSpecialTrickInfo trick_info = pSkaterProfile->GetSpecialTrickInfo( i );
		
			if ( trick_info.m_TrickSlot == key_combo )
			{
				if ( trick_info.m_TrickName == ( CRCD(0xf60c9090,"Unassigned") ) )
					return false;
				
				// printf("the special trick is already bound!\n");
				if ( Script::CStruct* p_trick = Script::GetStructure( trick_info.m_TrickName ) )
                {
                    const char* p_trick_string;
                    Script::CStruct* p_trick_params;
    				p_trick->GetStructure( CRCD(0x7031f10c,"Params"), &p_trick_params, Script::ASSERT );
    				p_trick_params->GetLocalString( CRCD(0xa1dc81f9,"Name"), &p_trick_string, Script::ASSERT );
                    pScript->GetParams()->AddString( CRCD(0x3a54cc4b,"trick_string"), p_trick_string );
                }
                
                if ( trick_info.m_isCat )
                {
                    pScript->GetParams()->AddInteger( CRCD(0x4443a4da,"trick_checksum"), trick_info.m_TrickName );
                }
                else
                {
                    pScript->GetParams()->AddChecksum( CRCD(0x4443a4da,"trick_checksum"), trick_info.m_TrickName );
                }
                pScript->GetParams()->AddInteger( CRCD(0xdb92effa,"current_index"), i );
				return true;
			}
		}
	}
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_UnlockGoal | unlocks the specified goal
// @parm name | name | the goal id
bool ScriptUnlockGoal(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	CGoal* pGoal = pGoalManager->GetGoal( goalId );

	pGoal->UnlockGoal();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_QuickStartGoal | starts a goal without going
// through the intro movie, etc.
// @parm name | name | the goal id
// @flag dontAssert | used by autotester
bool ScriptQuickStartGoal(Script::CStruct* pParams, Script::CScript* pScript)
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	bool dontAssert = false;
	if ( pParams->ContainsFlag( CRCD(0x9ac8d54f,"dontAssert") ) )
		dontAssert = true;

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	return pGoalManager->QuickStartGoal( goalId, dontAssert );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_InitializeGoal | initialize the specified goal
// @parm name | name | the goal id
bool ScriptInitializeGoal( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	CGoal* pGoal = pGoalManager->GetGoal( goalId );
	Dbg_Assert( pGoal );

	pGoal->Init();
	return true;
}
// TODO:  it would be nice to have the cfuncs call the goalmanager's
// member functions directly...

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_AddTime | adds the specified amount of time to the goal
// @parm name | name | the goal id
// @uparm 10 | time to add, in seconds
bool ScriptAddTime( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	int time;
	pParams->GetInteger( NONAME, &time, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	CGoal* pGoal = pGoalManager->GetGoal( goalId );
	Dbg_Assert( pGoal );

	pGoal->AddTime( time );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_ReplaceTrickText | replaces the trick name and 
// key combo with the appropriate text.  The key combo should be represented 
// in the string by \k, and the text by \t
// @parmopt name | name | | the goal id
// @flag all | in place of a goal id - update all goals
bool ScriptReplaceTrickText( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 goalId;
	if ( pParams->ContainsFlag( CRCD(0xc4e78e22,"all") ) )
	{
		pGoalManager->ReplaceAllTrickText();
		return true;
	}
	else if ( pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::NO_ASSERT ) )
	{
		const char* p_old_string;
		if ( pParams->GetString( CRCD(0xc4745838,"text"), &p_old_string, Script::NO_ASSERT ) )
		{
			char new_string[NEW_STRING_LENGTH];
			if ( pGoalManager->ReplaceTrickText( goalId, p_old_string, new_string, NEW_STRING_LENGTH ) )
			{
                pScript->GetParams()->RemoveComponent( CRCD(0x3eafa520,"TrickText") );
				pScript->GetParams()->AddString( CRCD(0x3eafa520,"TrickText"), new_string );
			}
		}
		else 
			return pGoalManager->ReplaceTrickText( goalId );
	}
	else
		Dbg_MsgAssert( 0, ( "GoalManager_ReplaceTrickText requires a goal id or the 'all' flag." ) );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
// @script | GoalManager_UnlockProSpecificChallenges | called once, when you 
// first get enough goals to unlock the pro challenge
bool ScriptUnlockProSpecificChallenges( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->UnlockProSpecificChallenges();
	return true;
}
*/
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
// @script | GoalManager_ProSpecificChallengesUnlocked | true if the pro challenges 
// have been unlocked
bool ScriptProSpecificChallengesUnlocked( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	return pGoalManager->ProSpecificChallengesUnlocked();
}
*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetNumberCollected | returns the number of objects
// collected so far in this goal.  If the goal is not a counter goal, it just returns
// the number of flags set.  The value will be stored in the number_collected var.
// @parm name | name | the goal id
bool ScriptGetNumberCollected( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	int number_collected = pGoalManager->GetNumberCollected( goalId );
	pScript->GetParams()->AddInteger( CRCD(0x129daa10,"number_collected"), number_collected );
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetNumberOfFlags | returns the number of flags in this
// goal.  If there are no flags, it returns false
// @parm name | name | the goal id
bool ScriptGetNumberOfFlags( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );
	
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	int number_of_flags = pGoalManager->GetNumberOfFlags( goalId );
	if ( number_of_flags != -1 )
	{
		pScript->GetParams()->AddInteger( CRCD(0xcb9d1224,"number_of_flags"), number_of_flags );
		return true;
	}
	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_IsPro | returns true if you're pro
bool ScriptIsPro( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	return pGoalManager->IsPro();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_ResetGoalFlags | resets all of the goals flags
bool ScriptResetGoalFlags( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->ResetGoalFlags( goalId );
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_ColorTrickObjects | colors all trick objects 
// associated with this goal
// @parm name | name | the goal id
// @flag clear | pass this in if you want to clear the color
// @parmopt int | skater | 0 | the skater num
bool ScriptColorTrickObjects( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	bool clear = pParams->ContainsFlag( CRCD(0x1a4e0ef9,"clear") );

	int skaterId = 0;
	pParams->GetInteger( CRCD(0x5b8ab877,"skater"), &skaterId, Script::NO_ASSERT );

	int seqIndex = skaterId + 1;

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->ColorTrickObjects( goalId, seqIndex, clear );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetNumberOfTimesGoalStarted | gets the total number
// of times the player has lost this goal.  The number will be stored in
// the calling scripts params as times_lost
// @parm name | name | the goal id
bool ScriptGetNumberOfTimesGoalStarted( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	int numTimesLost = pGoalManager->GetNumberOfTimesGoalStarted( goalId );
	pScript->GetParams()->AddInteger( CRCD(0xff61ba23,"times_started"), numTimesLost );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GoalExists | returns true if the goal exists
// @parm name | name | the goal id
bool ScriptGoalExists( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	return pGoalManager->GoalExists( goalId );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetGoalAttemptInfo | grabs the number of times each
// goal has been lost, and stores all the data in your times_lost_info
bool ScriptGetGoalAttemptInfo( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	pScript->GetParams()->AddStructurePointer( CRCD(0xd23ad129,"goal_attempt_info"), pGoalManager->GetGoalAttemptInfo() );
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_SetCanStartGoal | tell the goal manager that it can
// or cannot start any new goals
// @parmopt int | | 1 | 0 to block new goals
bool ScriptSetCanStartGoal( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	int value = 1;
	pParams->GetInteger( NONAME, &value, Script::NO_ASSERT );
	pGoalManager->SetCanStartGoal( value );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetLastGoalId | returns the last unbeaten goal
// as a checksum param goal_id.  If the player hasn't played a goal yet
// or beat the last goal, the function will return false.
bool ScriptGetLastGoalId( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 goalId = pGoalManager->GetLastGoalId();
	if ( goalId )
	{
		pScript->GetParams()->AddChecksum( CRCD(0x9982e501,"goal_id"), goalId );
		return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_ClearTetrisTricks | clears the entire stack of
// tetris tricks.
bool ScriptClearTetrisTricks( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoal* pGoal = pGoalManager->GetGoal( goalId );
	Dbg_MsgAssert( pGoal->IsTetrisGoal(), ( "ClearTetrisTricks called on a non-tetris goal" ) );

	CSkatetrisGoal* pSkatetrisGoal = (CSkatetrisGoal*)pGoal;
	pSkatetrisGoal->ClearAllTricks();
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
bool ScriptMarkProSpecificChallengeBeaten( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 skater_name;
	pParams->GetChecksum( CRCD(0x5b8ab877,"skater"), &skater_name, Script::ASSERT );

	int beaten = 1;
	pParams->GetInteger( NONAME, &beaten, Script::NO_ASSERT );
	
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->MarkProSpecificChallengeBeaten( skater_name, beaten );
	return true;
}
*/
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
bool ScriptSkaterHasBeatenProSpecificChallenge( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 skater_name;
	pParams->GetChecksum( CRCD(0x5b8ab877,"skater"), &skater_name, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	return pGoalManager->SkaterHasBeatenProSpecificChallenge( skater_name );
}
*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptAnnounceGoalStarted( Script::CStruct* pParams, Script::CScript* pScript )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Net::Client* client;
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoal* pGoal = pGoalManager->GetGoal( goalId );
	Dbg_Assert( pGoal != NULL );

	if( pGoal->IsNetGoal())
	{
		return true;
	}
	if( pGoal->GetParents()->m_relative != 0 )
	{
		return true;
	}
	
	client = gamenet_man->GetClient( 0 );
	Dbg_Assert( client );
	GameNet::MsgStartedGoal msg;
	GameNet::PlayerInfo* local_player;
	Net::MsgDesc msg_desc;

	local_player = gamenet_man->GetLocalPlayer();
	Dbg_Assert( local_player );
	
	msg.m_GameId = gamenet_man->GetNetworkGameId();
	msg.m_GoalId = pGoal->GetGoalId();

	msg_desc.m_Data = &msg;
	msg_desc.m_Length = sizeof( GameNet::MsgStartedGoal );
	msg_desc.m_Id = GameNet::MSG_ID_STARTED_GOAL;
	client->EnqueueMessageToServer( &msg_desc );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_SetEndRunType | one of the flags is required
// @parm name | name | the goal id
// @flag Goal_EndOfRun | use GoalEndOfRun
// @flag EndOfRun | use EndOfRun
// @flag none | don't use end of run
bool ScriptSetEndRunType( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	Game::EndRunType newEndRunType = Game::vENDOFRUN;
	if ( pParams->ContainsFlag( CRCD(0x54585419,"Goal_EndOfRun") ) )
		newEndRunType = Game::vGOALENDOFRUN;
	else if ( pParams->ContainsFlag( CRCD(0xe022c12c,"EndOfRun") ) )
		newEndRunType = Game::vENDOFRUN;
	else if ( pParams->ContainsFlag( CRCD(0x806fff30,"none") ) )
		newEndRunType = Game::vNONE;
	else
		Dbg_MsgAssert( 0, ( "GoalManager_SetEndRunType called without type" ) );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->SetEndRunType( goalId, newEndRunType );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_SetShouldDeactivateOnExpire | 
// @uparm 1 | 0 for no, anything else for yes
bool ScriptSetShouldDeactivateOnExpire( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	int should_deactivate;
	pParams->GetInteger( NONAME, &should_deactivate, Script::ASSERT );
	
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->SetShouldDeactivateOnExpire( goalId, should_deactivate != 0 );
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetActiveGoalId | returns the active goal's goal_id.
// Returns false if there are no active goals
bool ScriptGetActiveGoalId( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId = 0;
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	if ( pGoalManager )
	{
		int activeGoal = pGoalManager->GetActiveGoal( true );
		if ( activeGoal == -1 )
			return false;

		CGoal* pGoal = pGoalManager->GetGoalByIndex( activeGoal );
		Dbg_Assert( pGoal );
		if ( pGoal )
		{
			goalId = pGoal->GetGoalId();
			pScript->GetParams()->AddChecksum( "goal_id", goalId );
			return true;
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_AwardMinigameCash | 
// @parm name | name | the goal id
// @uparm 25 | amount of cash to award
bool ScriptAwardMinigameCash( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	int amount;
	pParams->GetInteger( NONAME, &amount, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	if ( pGoalManager )
	{
		return pGoalManager->AwardMinigameCash( goalId, amount );
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_ResetCareer | 
bool ScriptResetCareer( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->ResetCareer();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptAwardAllGoalCash( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->AwardAllGoalCash();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_UpdateFamilyTrees |
bool ScriptUpdateFamilyTrees( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pGoalManager->UpdateFamilyTrees();
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_IsLeafNode | returns true if the passed goal
// is a leaf node in the goal tree
// @parm name | name | the goal id
bool ScriptIsLeafNode( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	return pGoalManager->CallMemberFunction( ( CRCD(0xd782f8de,"IsLeafNode") ), pParams, pScript );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_IsRootNode | returns true if the passed
// goal is the root node of a family tree.  This is true if the goal
// is the only goal in a tree.
// @parm name | name | the goal id
bool ScriptIsRootNode( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	return pGoalManager->CallMemberFunction( ( CRCD(0x3ae9211b,"IsRootNode") ), pParams, pScript );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptSuspendGoalPedLogic( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	int suspend = 1;
	pParams->GetInteger( CRCD(0xce0ca665,"suspend"), &suspend, Script::NO_ASSERT );

	pGoalManager->GetGoal( pParams )->mp_goalPed->SuspendGoalLogic( suspend != 0 );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptRememberLevelStructureName( Script::CStruct *pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 level;
	pParams->GetChecksum( CRCD(0x63b1214e,"level_structure_name"), &level, Script::ASSERT );
	pGoalManager->RememberLevelStructureName( level );
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_SetDifficultyLevel | sets the current level
// @uparm 1 | difficulty level (starts at 0)
bool ScriptSetDifficultyLevel( Script::CStruct* pParams, Script::CScript* pScript )
{
	int level;
	pParams->GetInteger( NONAME, &level, Script::ASSERT );
	Dbg_MsgAssert( level >= 0 && level <= vMAXDIFFICULTYLEVEL, ( "GoalManager_SetDifficultyLevel called with level of %i", level ) );
	
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	pGoalManager->SetDifficultyLevel( level );
	return ( level != 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetDifficultyLevel | returns the current
// difficulty level as an integer param called difficulty_level
bool ScriptGetDifficultyLevel( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	int level = (int)pGoalManager->GetDifficultyLevel();
	pScript->GetParams()->AddInteger( CRCD(0x3671f748,"difficulty_level"), level );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
bool ScriptGetGoalParam( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );
	CGoal* pGoal = pGoalManager->GetGoal( goalId );

	uint32 type_checksum;
	Script::ESymbolType type = ESYMBOLTYPE_NONE;
	pParams->GetChecksum( CRCD(0x7321a8d6,"type"), &type_checksum, Script::ASSERT );

	// convert type
	switch ( type_checksum )
	{
		case CRCC(0x21902065,"checksum"):
			type=ESYMBOLTYPE_NAME;
			break;
		case CRCC(0x365aa16a,"float"):
			type=ESYMBOLTYPE_FLOAT;
			break;
		case CRCC(0xa5f054f4,"integer"):
			type=ESYMBOLTYPE_INTEGER;
			break;
		case CRCC(0x90fec815,"structure"):
			type=ESYMBOLTYPE_STRUCTURE;
			break;
		case CRCC(0x5ef31148,"array"):
			type=ESYMBOLTYPE_ARRAY;
			break;
		default:
			Dbg_MsgAssert( 0, ( "Uknown type %x", type ) );
			break;
	}

	uint32 name;
	pParams->GetChecksum( NONAME, &name, Script::ASSERT );

	// printf("looking for %s with type %s\n", Script::FindChecksumName( name ), Script::FindChecksumName( type ) );

	return pGoal->ExtractDifficultyLevelParam( type, name, pScript->GetParams() );
}
*/
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_RestartStage | restarts the current/last stage
bool ScriptRestartStage( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	pGoalManager->RestartStage();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_CanRestartStage | returns true if the player
// can retry a goal stage.
bool ScriptCanRestartStage( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	return pGoalManager->CanRestartStage();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptSetGoalChaptersAndStages( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	pGoalManager->SetGoalChaptersAndStages();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_AdvanceStage | returns true if the stage/chapter
// advances, false if you haven't reached an advancement point yet.
// @flag force | force the stage to advance
// @flag just_won_goal | this flag needs to be passed in if the function is called
// from a success script, as the goal is not marked beaten until after the success
// script finishes.  Therefore, the total number of goals beaten will be one more
// than the number returned by searching through the goals.
bool ScriptAdvanceStage( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	uint32 just_won_goal_id = 0;
	pParams->GetChecksum( CRCD(0x5d82c486,"just_won_goal_id"), &just_won_goal_id, Script::NO_ASSERT );
	return pGoalManager->AdvanceStage( pParams->ContainsFlag( CRCD( 0x68977cd7, "force" ) ), just_won_goal_id );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetCurrentChapterAndStage | returns the 
// current chapter and stage as currentChapter and currentStage
bool ScriptGetCurrentChapterAndStage( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	int chapter = pGoalManager->GetCurrentChapter();
	int stage = pGoalManager->GetCurrentStage();
	
	// return the last chapter if they beat the game
	Script::CArray* pChapterGoalsArray = Script::GetArray( CRCD( 0xb892776f, "chapter_goals" ), Script::ASSERT );
	Dbg_Assert( pChapterGoalsArray->GetType() == ESYMBOLTYPE_ARRAY );
	if ( chapter >= (int)pChapterGoalsArray->GetSize() )
	{
		chapter = pChapterGoalsArray->GetSize() - 1;
	}
	pScript->GetParams()->AddInteger( CRCD( 0xf884773c, "CurrentChapter" ), chapter );
	pScript->GetParams()->AddInteger( CRCD( 0xaf1575eb, "CurrentStage" ), stage );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_SetCurrentChapterAndStage | 
// @parmopt int | chapter | current | 
// @parmopt int | stage | current |
bool ScriptSetCurrentChapterAndStage( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	int chapter = pGoalManager->GetCurrentChapter();
	int stage = pGoalManager->GetCurrentStage();
	pParams->GetInteger( CRCD(0x67e4ad1,"chapter"), &chapter, Script::NO_ASSERT );
	pParams->GetInteger( CRCD(0x3d836c96,"stage"), &stage, Script::NO_ASSERT );
	pGoalManager->SetCurrentChapter( chapter );
	pGoalManager->SetCurrentStage( stage );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// @script | GoalManager_FilmGoalCheckpoint | Call this when the filming
// target reaches a checkpoint.  This is only used for filming goals that
// require the skater to catch X of Y checkpoints, not goals which requrie
// the skater to film the target for X of Y seconds.
// @parm name | name | the goal_id
bool ScriptFilmGoalCheckpoint( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	CFilmGoal* pFilmGoal = (CFilmGoal*)pGoalManager->GetGoal( pParams, true );

	pFilmGoal->CheckpointHit();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// @script | GoalManager_StartFilming | Called when the start cam anims finish.
// @parm name | name | the goal_id
bool ScriptStartFilming( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	CFilmGoal* pFilmGoal = (CFilmGoal*)pGoalManager->GetGoal( pParams, true );

	pFilmGoal->StartFilming();
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// @script | GoalManager_GoalShouldExpire | setup for endrun exceptions
bool ScriptGoalShouldExpire( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	if ( pGoalManager )
	{
		int activeGoal = pGoalManager->GetActiveGoal();
		if ( activeGoal == -1 )
			return false;

		CGoal* pGoal = pGoalManager->GetGoalByIndex( activeGoal );
		Dbg_Assert( pGoal );
		if ( pGoal )
		{
			return pGoal->ShouldExpire();
		}
	}
	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_SetSponsor | 
// @uparm sponsor_name | the checksum of the sponsor name
bool ScriptSetSponsor( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	uint32 sponsor;
	pParams->GetChecksum( NONAME, &sponsor, Script::ASSERT );
	pGoalManager->SetSponsor( sponsor );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetSponsor | returns sponsor as param "sponsor"
// the sponsor will be 0 if not set
bool ScriptGetSponsor( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	pScript->GetParams()->AddChecksum( CRCD(0x7e73362b,"sponsor"), pGoalManager->GetSponsor() );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GetTeam | returns team structure as param named
// team.  It also returns the current number of team members, in a param 
// named num_team_members
bool ScriptGetTeam( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	Script::CStruct* pTeam = pGoalManager->GetTeam();
	pScript->GetParams()->AddStructure( CRCD(0x3b1f59e0,"team"), pTeam );
	
	// throw in the number of team members
    Script::CComponent* p_comp = NULL;
    p_comp = pTeam->GetNextComponent( p_comp );

    int total = 0;
	while ( p_comp )
    {
        Script::CComponent* p_next = pTeam->GetNextComponent( p_comp );
		total++;
		p_comp = p_next;
	}
	pScript->GetParams()->AddInteger( CRCD(0xeb8dd330,"num_team_members"), total );
	
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_SetTeamMember | add or remove a team member
// @parm name | pro | name of the pro to add or remove
// @flag remove | use this flag to remove the pro - will add by default
bool ScriptSetTeamMember( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	uint32 pro;
	pParams->GetChecksum( CRCD(0x944b2900,"pro"), &pro, Script::ASSERT );
	bool remove = pParams->ContainsFlag( CRCD(0x977fe2cf,"remove") );
	bool mark_used = pParams->ContainsFlag( CRCD(0x531ff702,"mark_used") );
	Dbg_MsgAssert( !( remove && mark_used ), ( "GoalManager_SetTeamMember called with remove and mark_used flags." ) );
	pGoalManager->SetTeamMember( pro, remove, mark_used );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_KillTeamMembers | removes all team members
bool ScriptKillTeamMembers( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	pGoalManager->KillTeamMembers( );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_SetTeamName | set the team name
// @uparm "team name" | team name
bool ScriptSetTeamName( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	const char* p_string;
	pParams->GetString( NONAME, &p_string, Script::ASSERT );
	pGoalManager->SetTeamName( p_string );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool ScriptRunLastStageScript( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	pGoalManager->RunLastStageScript();
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_GoalInitialized | returns true if the goal
// is initialized
// @parm name | name | goal id
bool ScriptGoalInitialized( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	CGoal* pGoal = pGoalManager->GetGoal( goalId );
	Dbg_Assert( pGoal );

	return pGoal->IsInitialized();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_UnloadLastFam | unloads the last fam file 
// loaded for the goal ped.  If the fam has already been unloaded, it
// will return
bool ScriptUnloadLastFam( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	CGoal* pGoal = pGoalManager->GetGoal( goalId );
	Dbg_Assert( pGoal );

	pGoal->mp_goalPed->UnloadLastFam();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_StopLastStream | stops the last stream played 
// for a cam anim
bool ScriptStopLastSream( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 goalId;
	pParams->GetChecksum( CRCD(0xa1dc81f9,"name"), &goalId, Script::ASSERT );

	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	CGoal* pGoal = pGoalManager->GetGoal( goalId );
	Dbg_Assert( pGoal );

	pGoal->mp_goalPed->StopLastStream();
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | GoalManager_HideAllGoalPeds | hide/unhide all goal peds
// @uparm 0 | 0 to unhide, anything else to hide
bool ScriptHideAllGoalPeds( Script::CStruct* pParams, Script::CScript* pScript )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	int hide;
	pParams->GetInteger( NONAME, &hide, Script::ASSERT );
	pGoalManager->HideAllGoalPeds( ( hide != 0 ) );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
} // namespace Game

