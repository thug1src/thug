// start autoduck documentation
// @DOC goal
// @module goal | None
// @subindex Scripting Database
// @index script | goal
							   
#include <sk/modules/skate/goalmanager.h>

#include <sk/modules/skate/gamemode.h>

#include <core/string/stringutils.h>

#include <gel/scripting/script.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/string.h>
#include <gel/scripting/component.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>

#include <gel/net/client/netclnt.h>
#include <gel/object/compositeobjectmanager.h>

#include <sk/gamenet/gamenet.h>
								
#include <sk/objects/skater.h>
#include <sk/objects/trickobject.h>
#include <sk/objects/skaterprofile.h>
#include <sk/objects/skatercareer.h>
#include <sk/objects/playerprofilemanager.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/components/skaterendruncomponent.h>
#include <sk/components/skaterscorecomponent.h>
					   
#include <sk/modules/skate/score.h>
#include <sk/modules/skate/skate.h>
#include <sk/scripting/nodearray.h>

#include <sk/scripting/cfuncs.h>
#include <sk/scripting/skfuncs.h>


namespace Front
{
    extern void HideTimeTHPS4();
}

namespace Game
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGoalLink::CGoalLink()
{
	m_relative = 0;
	mp_next = NULL;
	m_beaten = false;
}

CGoalLink::~CGoalLink()
{
	// nothing yet
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGoal::CGoal( Script::CStruct* pParams ) : Lst::Node<CGoal>( this )
{
	mp_children = new Game::CGoalLink();
	mp_parents = new Game::CGoalLink();
	
	m_active = false;
    m_hasSeen = false;
    m_paused = false;
    m_unlimitedTime = true;
    m_isbeat = false;
	m_netIsBeat = false;
	m_beatenFlags = 0;
	m_teamBeatenFlags = 0;
	m_isselected = false;
	m_isInitialized = false;

	m_numTimesLost = 0;
	m_numTimesWon = 0;
	m_numTimesStarted = 0;

	m_deactivateOnExpire = true;

    m_timeLeft  = 0;

	m_chapter = m_stage = -1;

	mp_params = new Script::CStruct;
	mp_params->AppendStructure( pParams );

	mp_goalPed = new Game::CGoalPed( mp_params );

	mp_goalFlags = new Script::CStruct;
    
	m_numflags = 0;
    m_flagsSet = 0;

	m_elapsedTime = 0;
	m_elapsedTimeRecord = vINT32_MAX;
	m_winScoreRecord = 0;

	m_endRunType = vGOALENDOFRUN;

    // set the level number
    Mdl::Skate * skate_mod = Mdl::Skate::Instance();
    Obj::CSkaterCareer* pCareer = skate_mod->GetCareer();
    m_levelNum = pCareer->GetLevel();
    
	CreateGoalFlags( pParams );

	if ( pParams->ContainsFlag( CRCD( 0x4ab5f14, "single_timer" ) ) )
	{
		// Dbg_MsgAssert( mp_children->m_relative, ( "single_timer flag used on a goal without children" ) );
		m_inherited_flags |= GOAL_INHERITED_FLAGS_ONE_TIMER;
	}

	if ( pParams->ContainsFlag( CRCD( 0xc6b4258f, "retry_stage" ) ) )
	{
		m_inherited_flags |= GOAL_INHERITED_FLAGS_RETRY_STAGE;
	}
    
    // TODO: make goal lock stuff work right
	if ( pParams->ContainsFlag( vUNLOCKED_BY_ANOTHER ) )
    {
        m_isLocked = true;
    }
    else if ( pParams->ContainsFlag( vPRO_GOAL ) )
    {
        m_isLocked = true;
    }
/*	else if ( pParams->ContainsFlag( vPRO_CHALLENGE ) )
	{
		m_isLocked = true;
	}
*/
    else
    {
        m_isLocked = false;
    }
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::Init()
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());

	// overwrite any params with difficulty level params
	AppendDifficultyLevelParams();

	// make sure there's a goal description so that the intro cam anims can
	// be killed and Gary will be happy
	Dbg_MsgAssert( mp_params->ContainsComponentNamed( CRCD(0xc5d7e6b,"goal_description") ), ( "No goal description defined for %s\n", Script::FindChecksumName( GetGoalId() ) ) );
	
	RunCallbackScript( vINIT );
	ReplaceTrickText();
	m_isInitialized = true;
	
	// Dan, TT12965.  If a server switched between time limit and capture limit ctf games, m_unlimitedTime was not being reset to true.  Thus, ShouldExpire
	// was returning true during the capture limit game, if the time on the previous time limit game had been allowed to run down.
	int unlimited_time = 0;
	mp_params->GetInteger( CRCD(0xf0e712d2,"unlimited_time"), &unlimited_time );
	if ( unlimited_time )
	{
		m_unlimitedTime = true;
	}
	
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::AddParent( uint32 id )
{
	// printf("adding parents\n");
	if ( mp_parents->m_relative == 0 )
	{
		mp_parents->m_relative = id;
	}
	else
	{
		Game::CGoalLink* p_parent = new Game::CGoalLink();
		p_parent->m_relative = id;
		p_parent->mp_next = mp_parents;
		mp_parents = p_parent;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::CreateGoalFlags( Script::CStruct* pParams )
{
    // set any goal flags specified in the params
    Script::CArray *pTempArray;
    mp_goalFlags->Clear();
	m_numflags = 0;
	if ( pParams->GetArray( CRCD(0xcc3c4cc4,"goal_flags"), &pTempArray ) )
    {
        for ( uint32 c = 0; c < pTempArray->GetSize(); c++)
        {
            mp_goalFlags->AddInteger( pTempArray->GetNameChecksum( c ), 0 );
            m_numflags++;
        }
    }
	mp_params->AddInteger( CRCD(0xb3790f33,"num_flags"), m_numflags );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::SetInheritableFlags( int recurse_level )
{
	// printf("SetInheritableFlags called on %s\n", Script::FindChecksumName( GetGoalId() ) );
	Dbg_MsgAssert( recurse_level < 100, ( "SetInheritableFlags recursed too much" ) );
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	CGoalLink* p_child = mp_children;
	while ( p_child && p_child->m_relative != 0 )
	{
		CGoal *pGoal = pGoalManager->GetGoal( p_child->m_relative );
		if ( pGoal )
		{
			pGoal->m_inherited_flags |= m_inherited_flags;
			pGoal->SetInheritableFlags( recurse_level + 1 );
		}
		p_child = p_child->mp_next;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::AddChild( uint32 id )
{
	// printf("adding child\n");	
	// any current children?
	if ( mp_children->m_relative == 0 )
	{
		mp_children->m_relative = id;
	}
	else
	{		
		// make sure this child isn't already in the list
		bool found = false;
		Game::CGoalLink* p_test = mp_children;
		while ( p_test && p_test->m_relative != 0 )
		{
			if ( p_test->m_relative == id )
			{
				found = true;
				break;
			}
			p_test = p_test->mp_next;
		}
		
		if ( found )
		{
			Dbg_MsgAssert( 0, ( "AddChild failed.  %s already a child of %s\n", Script::FindChecksumName( id ), Script::FindChecksumName( GetGoalId() ) ) );
		}
		else
		{
			// add new child to head of list
			Game::CGoalLink* p_child = new Game::CGoalLink();
			p_child->m_relative = id;
			p_child->mp_next = mp_children;
			mp_children = p_child;
		}
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::ResetGoalLinks()
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
	
	CGoalLink* p_link = mp_children;
	while ( p_link && p_link->m_relative != 0 )
	{
		p_link->m_beaten = false;
		CGoal* pGoal = pGoalManager->GetGoal( p_link->m_relative );
		if ( pGoal )
		{
			// recurse!
			pGoal->ResetGoalLinks();
		}
		
		p_link = p_link->mp_next;
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::DeleteLinks( CGoalLink* p_list )
{
	CGoalLink *p_a, *p_b;
	for ( p_a = p_list; p_a; p_a = p_b )
	{
		p_b = p_a->mp_next;
		delete p_a;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::Uninit( bool propogate, int recurse_level )
{
	Dbg_MsgAssert( recurse_level < 20, ( "CGoal::Uninit recursed too deep" ) );
	
	// uint32 trigger_obj_id = 0;

	if ( IsActive() )
	{
		Deactivate( true );
	}

	RunCallbackScript( vUNINIT );
	
	m_isInitialized = false;
	
/*	
	Script::RunScript( Script::GenerateCRC( "goal_kill_proset_geom" ), mp_params );
	mp_params->GetChecksum( CRCD( 0x2d7e03d, "trigger_obj_id" ), &trigger_obj_id );
	if ( trigger_obj_id )
	{
		Obj::CMovingObject* p_pos_obj = NULL;
		// p_pos_obj = Obj::CMovingObject::m_hash_table.GetItem( trigger_obj_id );
		p_pos_obj = (Obj::CMovingObject*) Obj::CCompositeObjectManager::Instance()->GetObjectByID( trigger_obj_id );
		if ( p_pos_obj )
		{
			delete p_pos_obj;
		}
	}
*/
	if ( propogate )
	{
		CGoalManager* pGoalManager = GetGoalManager();
		Dbg_Assert( pGoalManager );

		CGoalLink* pGoalLink = mp_children;
		while ( pGoalLink && pGoalLink->m_relative != 0 )
		{
			CGoal* pChildGoal = pGoalManager->GetGoal( pGoalLink->m_relative );
			Dbg_Assert( pChildGoal );
			pChildGoal->Uninit( propogate, recurse_level + 1 );
			pGoalLink = pGoalLink->mp_next;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CGoal::~CGoal()
{
	// does this really need to be run?
	// (you may not necessarily want to go through the
	// same destructor logic if we're clearing out the
	// goals at the end of the level)
    
    RunCallbackScript( vDESTROY );
	
	delete mp_goalPed;

	Dbg_Assert( mp_params );
	delete mp_params;    
	delete mp_goalFlags;

	// delete all children and parent trees
	DeleteLinks( mp_children );
	DeleteLinks( mp_parents );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
Script::CStruct* CGoal::GetParams()
{
	return mp_params;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::SetTimer()
{
	int unlimited_time = 0;

	mp_params->GetInteger( CRCD(0xf0e712d2,"unlimited_time"), &unlimited_time );

	// check for unlimited time
	if ( unlimited_time == 1 )
	{
		return;
	}
	
	// don't set default time for minigames.
	if ( IsMinigame() )
	{
		if ( !mp_params->ContainsComponentNamed( CRCD(0x906b67ba,"time") ) )
			return;
	}

	// default time limit of 2 minutes
    m_timeLeft = 120;
	int time;
	if ( mp_params->GetInteger( CRCD(0x906b67ba,"time"), &time, Script::NO_ASSERT ) )
	{
		m_timeLeft = (Tmr::Time)time;
	}
/*	else
	{
		Script::CArray* pTimeLimits;
		if ( mp_params->GetArray( CRCD(0x906b67ba,"time"), &pTimeLimits, Script::NO_ASSERT ) )
		{
			CGoalManager* pGoalManager = GetGoalManager();
			Dbg_Assert( pGoalManager );
			uint32 difficulty_level = pGoalManager->GetDifficultyLevel();
			Dbg_Assert( difficulty_level > 0 && difficulty_level < pTimeLimits->GetSize() );
			m_timeLeft = (Tmr::Time)( pTimeLimits->GetInteger( difficulty_level ) );
		}
	}
*/
	// convert to milliseconds
	m_timeLeft *= 1000;
    
	// bump up timer so clock starts with expected amount of time left
	m_timeLeft += 20;
	// printf("timer set to %i\n", (int)m_timeLeft);

	m_unlimitedTime = false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::SetTimer( int time )
{
	m_timeLeft = (Tmr::Time)time * 1000;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	CGoal::GetTimeLeft( void )
{
	return m_timeLeft;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::AddTime( int time )
{
	// bump up time by 20ms so that it displays right
	m_timeLeft += ( (Tmr::Time)time * 1000 ) + 20;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::Activate()
{
    if ( !IsActive() )
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
		// Warning! Make sure there are no 'return's between the above pushcontext and the 
		// popcontext below ...
		
		m_elapsedTime = 0;
		
		m_numTimesStarted++;

		// printf("goal is not active - now activating\n");
		GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		GameNet::PlayerInfo* player;
		Lst::Search< GameNet::PlayerInfo > sh;
						
		// TODO: reset just this skater's score
		for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ) )
		{
			Obj::CSkater *p_Skater = player->m_Skater;
			Dbg_Assert( p_Skater );
			
			Obj::CSkaterScoreComponent* p_SkaterScoreComponent = GetSkaterScoreComponentFromObject(p_Skater);
			Dbg_Assert( p_SkaterScoreComponent );
			
			Mdl::Score * p_Score = ( p_SkaterScoreComponent->GetScore() );
			Dbg_Assert( p_Score );
	
			// don't reset the score when starting a minigame
			if ( !IsMinigame() )
				p_Score->Reset();
		}

		// Warning! Make sure there are no 'return's between the above pushcontext and the 
		// popcontext below ...
        
		if ( mp_parents->m_relative != 0 )
		{
			CGoalManager* pGoalManager = GetGoalManager();
			Dbg_Assert( pGoalManager );

			CGoal* pParentGoal = NULL;

			// get the root node and check the timer
			CGoalLink* p_parent = mp_parents;
			while ( p_parent && p_parent->m_relative != 0 )
			{
				pParentGoal = pGoalManager->GetGoal( p_parent->m_relative );
				p_parent = pParentGoal->GetParents();
			}
			
			if ( pParentGoal )
			{
				if ( m_inherited_flags & GOAL_INHERITED_FLAGS_ONE_TIMER )
					SetTimer( pParentGoal->GetTimeLeft() / 1000.0f );
				else
					SetTimer();

				if ( pParentGoal->IsInitialized() )
				{
					pParentGoal->Uninit();
					
					// reinitialize the goal in case uninitializing the parent
					// screwed anything up.
					if ( IsInitialized() )
					{
						Uninit();
						Init();
					}
				}
			}
			else
			{
				// Dbg_Message( "Goal supposed to use a single timer but couldn't find parent" );
				SetTimer();
			}
		}
		else
		{
			SetTimer();
		}
		
		if ( !IsInitialized() )
		{
			Init();
		}
		
		m_endRunCalled = false;

		// Warning! Make sure there are no 'return's between the above pushcontext and the 
		// popcontext below ...

		mp_params->RemoveFlag( CRCD(0x7100155d,"combo_started") );

        // activate script
		RunCallbackScript( vACTIVATE );
        
		if( gamenet_man->InNetGame() == false )
		{
			m_hasSeen = true;
		}
        
		//if ( !mp_params->ContainsFlag( "quick_start" ) )
			//PauseGoal();
		SetActive();
		Mem::Manager::sHandle().PopContext();
		
		return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::SetActive()
{
	m_active = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::Deactivate( bool force, bool affect_tree )
{
	if ( IsActive() )
	{
		// special case for special trick goal...get rid of
		// the temp key_combos array
		if ( IsSpecialGoal() )
			mp_params->RemoveComponent( CRCD(0x79704516,"key_combos") );

		Front::HideTimeTHPS4();
		
		SetInactive();
        
        ResetGoalFlags();

		ResetGoalLinks();

        RunCallbackScript( vDEACTIVATE );

		if (!mp_params->ContainsFlag( CRCD( 0x38221df4, "quick_start" ) ))
		{
			// If not retrying, then remove the testing_goal flag
			// The presence of the testing_goal flag prevents the goal from being marked as
			// beaten when it is beaten during testing.
			mp_params->RemoveFlag(CRCD(0x54d3cac1,"testing_goal"));
		}	
		
		if ( !mp_params->ContainsFlag( CRCD( 0xc309cad1, "just_won_goal" ) )
			 && !mp_params->ContainsFlag( CRCD( 0x38221df4, "quick_start" ) ) )
		{
			Script::RunScript(CRCD(0x719ae783, "ready_skater_for_early_end_current_goal"), mp_params);
			
			if ( affect_tree && mp_parents && mp_parents->m_relative )
			{
				Uninit();
	
				// find parent node and reinitialize
				CGoalManager* pGoalManager = GetGoalManager();
				Dbg_Assert( pGoalManager );
				CGoalLink* p_link = mp_parents;
				CGoal* pParentGoal = NULL;
				while ( p_link && p_link->m_relative )
				{
					pParentGoal = pGoalManager->GetGoal( p_link->m_relative );
					Dbg_Assert( pParentGoal );
					p_link = pParentGoal->GetParents();
				}
				if ( pParentGoal && !pParentGoal->HasWonGoal() )
				{
					// printf("initializing goal: %s\n", Script::FindChecksumName( pParentGoal->GetGoalId() ) );
					pParentGoal->Init();
				}
			}
		}

        return true;
    }
	return false;
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::SetInactive()
{
	m_active = false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::Win( void )
{
	mp_params->AddInteger( CRCD(0xc22a2b72,"win_record"), 0 );
	// see if it's a leaf node
	if ( mp_children->m_relative == 0 )
	{
		mp_params->AddChecksum( NONAME, CRCD( 0xc309cad1, "just_won_goal" ) );			

		GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		
		m_numTimesWon++;
		
		// look for a new record if we're not in a net game
		bool set_new_record = false;
		uint32 record_type;
		if ( !gamenet_man->InNetGame() && mp_params->GetChecksum( CRCD(0x96963902,"record_type"), &record_type, Script::NO_ASSERT ) )
		{
			// printf("record type is %s\n", Script::FindChecksumName( record_type ) );
			if ( record_type == CRCD(0xcd66c8ae,"score") )
			{
				// check for custom score record
				int custom_record;
				if ( mp_params->GetInteger( CRCD(0xfc9630ab,"custom_score_record"), &custom_record, Script::NO_ASSERT ) )
				{
					if ( custom_record > m_winScoreRecord )
					{
						m_winScoreRecord = custom_record;
						mp_params->AddInteger( CRCD(0xc22a2b72,"win_record"), m_winScoreRecord );
						set_new_record = true;
					}
				}
				else
				{
					// printf("found a score record type\n");
					Mdl::Skate * skate_mod = Mdl::Skate::Instance();
					Obj::CSkater *pSkater = skate_mod->GetLocalSkater();
					Dbg_Assert( pSkater );
					
					Obj::CSkaterScoreComponent* pSkaterScoreComponent = GetSkaterScoreComponentFromObject(pSkater);
					Dbg_Assert( pSkaterScoreComponent );
		
					Mdl::Score * pScore = ( pSkaterScoreComponent->GetScore() );
					Dbg_Assert( pScore );
		
					int total_score;
					if ( mp_params->ContainsFlag( CRCD(0xc63432a7,"high_combo") ) )
						 total_score = pScore->GetLastScoreLanded();
					else
						 total_score = pScore->GetTotalScore();

					// printf("got a total_score of %i, had a previous record of %i\n", total_score, m_winScoreRecord );
					if ( total_score > m_winScoreRecord )
					{
						m_winScoreRecord = total_score;
						mp_params->AddInteger( CRCD(0xc22a2b72,"win_record"), m_winScoreRecord );
						set_new_record = true;
					}
				}
			}
			else if ( record_type == CRCD(0x906b67ba,"time") )
			{
				// printf("got a time of %i, previous record was %i\n", (int)m_elapsedTime, (int)m_elapsedTimeRecord);
				if ( m_elapsedTime < m_elapsedTimeRecord )
				{
					m_elapsedTimeRecord = m_elapsedTime;
					mp_params->AddInteger( CRCD(0xc22a2b72,"win_record"), (int)m_elapsedTimeRecord );
					set_new_record = true;
				}
			}
		}
	#ifdef __NOPT_ASSERT__
		CGoalManager* pGoalManager = GetGoalManager();
		Dbg_Assert( pGoalManager );
	#endif		// __NOPT_ASSERT__
		
		if ( !HasWonGoal() && !gamenet_man->InNetGame() )
		{
			// printf("getting rewards struct\n");
			Script::CStruct* p_reward_params = new Script::CStruct();
			GetRewardsStructure( p_reward_params, true );
			if ( set_new_record )
				p_reward_params->AddChecksum( NONAME, CRCD(0xe7b8254f,"set_new_record") );
	
			mp_params->AddStructurePointer( CRCD(0x128b3088,"reward_params"), p_reward_params );
	
			UnlockRewardGoals();
		}
		else
			mp_params->RemoveComponent( CRCD(0x128b3088,"reward_params") );
		
		// update set_new_record flag
		if ( set_new_record )
			mp_params->AddChecksum( NONAME, CRCD(0xe7b8254f,"set_new_record") );
		else
			mp_params->RemoveFlag( CRCD(0xe7b8254f,"set_new_record") );
		
		RunCallbackScript( vSUCCESS );

		bool testing_goal=mp_params->ContainsFlag(CRCD(0x54d3cac1,"testing_goal"));
		
		if ( IsActive() )
		{
			// add a flag so the deactivate script knows that we just beat the goal
			Deactivate();
		}
	
		// The testing_goal flag is set when a created goal is being test played.
		// Don't want it to be crossed off the view goals menu in that case. (TT3366)
		if (!testing_goal)
		{
			MarkBeaten();
		}

		if ( gamenet_man->InNetGame() )
		{
			Uninit();
		}
				
		mp_params->RemoveFlag( CRCD( 0xc309cad1, "just_won_goal" ) );
	
		return true;
	}
	else
	{
		// it's a link to another goal

		// TODO: we need a way of deciding which child to pick
		// for now, we'll just take the first child
		Game::CGoalLink* p_link = mp_children;

		CGoalManager* pGoalManager = GetGoalManager();
		Dbg_Assert( pGoalManager );
		Deactivate();
		Uninit();
		CGoal* pChildGoal = pGoalManager->GetGoal( p_link->m_relative );
		pChildGoal->Init();
		pGoalManager->ActivateGoal( p_link->m_relative, false );
		
		// mark the link beaten
		p_link->m_beaten = true;

		return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::Lose( void )
{
	// Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	// Obj::CSkater *p_Skater = skate_mod->GetLocalSkater();
	// Dbg_Assert( p_Skater );

	// p_Skater->ClearAllExceptions();

	if ( IsActive() )
	{
		m_numTimesLost++;
		
		RunCallbackScript( vFAIL );
        
		Deactivate( true );

		return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::IsExpired()
{
	return IsExpired( false );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::ShouldExpire()
{
	// returns true if the goal would expire if the skater was not in a combo
	
	// Dan: this logic needs to mimic IsExpired()
	
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Obj::CSkater* p_skater;
	
	p_skater = skate_mod->GetLocalSkater();
	if ( p_skater == NULL )
	{
		return true;
	}
	
#ifdef __NOPT_ASSERT__
	Obj::CSkaterEndRunComponent* p_skater_endrun_component = GetSkaterEndRunComponentFromObject(p_skater);
	Dbg_Assert(p_skater_endrun_component);
#endif		// __NOPT_ASSERT__
	
	// If no players are left, the goal should expire
	if ( gamenet_man->GetNumPlayers() == 0 )
	{
		return true;
	}
	
	// If it's an unlimited game, don't expire the goal
	if ( m_unlimitedTime )
	{
		return false;
	}
	
	if ( (int)m_timeLeft <= 0 )
	{
		// Dbg_Printf( "**** Already Expired\n" );
		return true;
	}
	
	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::IsExpired( bool force_end )
{
    Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Obj::CSkater* p_skater;

	p_skater = skate_mod->GetLocalSkater();
	if ( p_skater == NULL )
	{
		return true;
	}
	
	Obj::CSkaterEndRunComponent* p_skater_endrun_component = GetSkaterEndRunComponentFromObject(p_skater);
	Dbg_Assert(p_skater_endrun_component);

	// If no players are left, the goal should expire
	if ( gamenet_man->GetNumPlayers() == 0 )
	{
		return true;
	}
					
	// If it's an unlimited game, don't expire the goal
	if ( m_unlimitedTime )
	{
		return false;
	}
	
	// check if the skaters have reached end of run BEFORE
	// setting the exception, or you'll never catch it!
	bool expired = false;
	 
	switch ( m_endRunType )
	{
	case vENDOFRUN:
		expired = p_skater_endrun_component->RunHasEnded();
		break;
	case vGOALENDOFRUN:
		expired = p_skater_endrun_component->GoalRunHasEnded();
		break;
	case vNONE:
		// if we're not supposed to use end of run, we need only check
		// the time
		return ( (int)m_timeLeft <= 0 );
		break;
	default:
		Dbg_Assert( 0 );
	}

	if ( expired && (int)m_timeLeft <= 0 )
	{
		// Dbg_Printf( "**** Already Expired\n" );
		return true;
	}

	// if time is up, set the end of run exception
	if ( (int)m_timeLeft <= 0 )
	{
		// Dbg_Printf( "EXPIRING GOAL: %d\n", GetGoalId());
		// printf("&&&&&&&&&&&&&&&&& calling EndRun\n");
		switch ( m_endRunType )
		{
		case vENDOFRUN:
			m_endRunCalled = true;
			// Dbg_Printf( "**** Calling End Run\n" );
			p_skater_endrun_component->EndRun( force_end );
			break;
		case vGOALENDOFRUN:
			// printf("reached goalendofrun\n");
			m_endRunCalled = true;
			p_skater_endrun_component->EndGoalRun( force_end );
			break;
		default:
			Dbg_Assert( 0 );
		}
	}

    // printf("AtEndOfRun returned %i\n", all_expired);
	// return ( all_expired && ( (int)m_timeLeft <= 0 ) );
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::AllSkatersAtEndOfRun()
{
	// check all the goals
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();

	GameNet::PlayerInfo* player;
	Lst::Search< GameNet::PlayerInfo > sh;
	
	for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
	{
		Obj::CSkater *p_Skater = player->m_Skater;
		// int skater_num = p_Skater->GetSkaterNumber();
		Dbg_Assert( p_Skater );
		
		Obj::CSkaterEndRunComponent* p_skater_endrun_component = GetSkaterEndRunComponentFromObject(p_Skater);
		Dbg_Assert( p_skater_endrun_component );

		if( skate_mod->GetGameMode()->GetNameChecksum() == CRCD( 0x9d65d0e7, "horse" ))
		{
			if( p_Skater->IsPaused() || p_Skater->IsHidden())
			{
				continue;
			}
		}
		switch ( m_endRunType )
		{
		case vENDOFRUN:
			if ( !p_skater_endrun_component->RunHasEnded() )
			{
				return false;
			}
			break;
		case vGOALENDOFRUN:
			if ( !p_skater_endrun_component->GoalRunHasEnded() )
			{
				return false;
			}
			break;
		default:
			Dbg_Assert( 0 );
		}
	}    
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::EditParams( Script::CStruct* pParams )
{
	mp_params->AppendStructure( pParams );

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::Update( void )
{
    if ( IsActive() && !IsPaused() )
    {
        RunCallbackScript( vACTIVE );
    }
    
    // the active script may have deactivated the goal, so we check 
    // if it's active again...don't check mini games
    if ( IsActive() && !IsPaused())
	{
		if( ShouldUseTimer()  )
		{
			if ( ( (int)m_timeLeft >= 1 ) )
			{
				int old_time_left = (int)m_timeLeft;
				m_timeLeft -= (Tmr::Time)( Tmr::FrameLength() * 1000 );
				m_elapsedTime += (Tmr::Time)( Tmr::FrameLength() * 1000 );

				if ( (int)m_timeLeft < 10000 && ( (int)( old_time_left / 1000 ) > (int)( (int)m_timeLeft / 1000 ) ) )
				{
					if (Mdl::Skate::Instance()->GetGameMode()->ShouldTimerBeep())
					{
						Script::RunScript( CRCD(0xcfc7c3ad,"timer_runout_beep") );
					}
				}
			}
		}

		if ( ShouldRequireCombo() )
		{
			if ( !mp_params->ContainsFlag( CRCD( 0x7100155d, "combo_started" ) ) )
			{
				Mdl::Skate *skate_mod =  Mdl::Skate::Instance();
				Obj::CSkater* pSkater = skate_mod->GetLocalSkater();
				if ( pSkater )
				{
					Mdl::Score* pScore = pSkater->GetScoreObject();
					if ( pScore && pScore->GetScorePotValue() > 0 )
					{
						mp_params->AddChecksum( NONAME, CRCD(0x7100155d,"combo_started") );
					}
				}
			}
		}
		
		// add the amount of time left
		// mp_params->AddInteger( "time_left", m_timeLeft );        
		if ( IsExpired() )
		{
			Expire();
		}
	}
	
// haven't used the inactive yet	
/*	else
	{
		if ( !IsLocked() )
        {
            // runs its inactive script (if any)
    		RunCallbackScript( vINACTIVE );
        }
	}	
*/
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::Expire()
{
	RunCallbackScript( vEXPIRED );
	
	// TODO: fix after e3
	// this was done to make the horse and combo letter goals work right
	if ( m_deactivateOnExpire )
	{
		Deactivate();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::IsActive( void )
{
	return ( m_active == true );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::HasSeenGoal( void )
{
	return m_hasSeen;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::PauseGoal( void )
{
	m_paused = true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::UnPauseGoal( void )
{
	m_paused = false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
Tmr::Time CGoal::GetTime( void )
{
	return m_timeLeft;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
/*void CGoal::CreateGoalFlag( uint32 flag )
{
    mp_goalFlags->AddInteger( flag, 0 );
    m_numflags++;
}*/


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::SetGoalFlag( uint32 flag, int value )
{
    if ( !IsActive() )
    {
        return false;
    }
    
	if ( mp_goalFlags->ContainsComponentNamed( flag ) )
	{
		// remember the old value
		int old_value;
		mp_goalFlags->GetInteger( flag, &old_value, Script::ASSERT );
		
		mp_goalFlags->AddInteger( flag, value );
		if ( old_value != value )
		{
			if ( value == 0 )
				m_flagsSet--;
			else
			{
				m_flagsSet++;
				if ( !mp_params->ContainsFlag( CRCD(0x524566e3,"no_midgoal_sound") ) )
					Script::RunScript( CRCD(0x5ed34deb,"goal_got_flag_sound") );
			}
			if ( mp_params->ContainsFlag( CRCD(0xfd7ae1f4,"use_hud_counter") ) )
				Script::RunScript( CRCD(0xf2e85a34,"goal_update_counter"), mp_params );

			mp_params->AddInteger( CRCD(0xcf6cd3c1,"num_flags_set"), m_flagsSet );
		}		
		// printf("setting goal flag %s, %i of %i set\n", Script::FindChecksumName( flag ), m_flagsSet, m_numflags );
		return true;
	}
    
    Dbg_MsgAssert( 0, ("SetGoalFlag couldn't find flag\n" ) );
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::GetCreatedGoalGap(int gapIndex)
{
	Script::CArray *p_required_gaps=NULL;
	if (!mp_params->GetArray(CRCD(0x52d4489e,"required_gaps"),&p_required_gaps))
	{
		return false;
	}

	const char *p_required_trick_name="";
	mp_params->GetString(CRCD(0x730844a3,"required_trick_name"),&p_required_trick_name);
	if (p_required_trick_name[0])
	{
		Mdl::Skate *p_skate_mod =  Mdl::Skate::Instance();
		Obj::CSkater *p_skater = p_skate_mod->GetLocalSkater();
		if (!p_skater)
		{
			return false;
		}	
		Mdl::Score *p_score = p_skater->GetScoreObject();
	
		uint32 trick_checksum=Script::GenerateCRC(p_required_trick_name);
		
		// Also check for fs and bs versions (TT9784)
		uint32 bs_trick_checksum=CRCD(0x7491335e,"bs ");
		bs_trick_checksum=Crc::ExtendCRCWithString(bs_trick_checksum, p_required_trick_name);
		uint32 fs_trick_checksum=CRCD(0x73989b82,"fs ");
		fs_trick_checksum=Crc::ExtendCRCWithString(fs_trick_checksum, p_required_trick_name);
		
		if (p_score->GetCurrentNumberOfOccurrencesByName(trick_checksum, 0, false) == 0 && 
			p_score->GetCurrentNumberOfOccurrencesByName(bs_trick_checksum, 0, false) == 0 &&
			p_score->GetCurrentNumberOfOccurrencesByName(fs_trick_checksum, 0, false) == 0)
		{
			return false;
		}
	}		
	
	for (uint i=0; i<p_required_gaps->GetSize(); ++i)
	{
		if (gapIndex==p_required_gaps->GetInteger(i))
		{
			char p_foo[20];
			sprintf(p_foo,"got_%d",i+1);
			SetGoalFlag(Script::GenerateCRC(p_foo),1);
			return true;
		}
	}
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::GoalFlagEquals( uint32 flag, int value )
{
	if ( mp_goalFlags->ContainsComponentNamed( flag ) )
	{
		int f;
		mp_goalFlags->GetInteger( flag, &f, Script::ASSERT );
		return ( value == f );
	}
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::ResetGoalFlags()
{
	// reset the goal flags
	Script::CComponent* p_comp = mp_goalFlags->GetNextComponent( NULL );
	while ( p_comp )
	{
		Script::CComponent* p_next = mp_goalFlags->GetNextComponent( p_comp );
		p_comp->mIntegerValue = 0;
		// SetGoalFlag( p_comp->mNameChecksum, 0 );
		
		// printf("resetting goal flag %s\n", Script::FindChecksumName( flag_name ) );
		p_comp = p_next;
	}
	m_flagsSet = 0;
	mp_params->AddInteger( CRCD( 0xcf6cd3c1, "num_flags_set" ), 0 );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::AllFlagsSet()
{
//    printf("%i flags, %i set\n", m_numflags, m_flagsSet);
	int num_flags_to_win = m_numflags;
	mp_params->GetInteger( CRCD(0x1171a555,"num_flags_to_win"), &num_flags_to_win, Script::NO_ASSERT );
	
	// K: Fix to TT9785, where created gap goals would succeed straight away if no gaps were chosen,
	// causing an assert.
	if (num_flags_to_win == 0)
	{
		return false;
	}	
	return ( m_flagsSet >= num_flags_to_win );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::GoalFlagSet( uint32 flag )
{
	int flag_value;
	mp_goalFlags->GetInteger( flag, &flag_value, Script::ASSERT );
	return ( flag_value != 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::IsLocked()
{
    return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::UnlockGoal()
{
    if ( m_isLocked )
    {
        m_isLocked = false;

		// don't initialize goals automatically in net games
		GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		if ( gamenet_man->InNetGame() )
			return;

		// initialize goal
        Init();
        
        // call the unlock script 
        RunCallbackScript( vUNLOCK );
    }
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::NextTourSpot()
{
    Script::CArray* p_waypoints;
    // backward compatible with the horse_spots array naming convention
	if ( !mp_params->GetArray( CRCD(0x160b3220,"tour_spots"), &p_waypoints, Script::NO_ASSERT ) )
		mp_params->GetArray( CRCD(0x92c6e839,"horse_spots"), &p_waypoints, Script::ASSERT );

    Script::CStruct* p_waypoint;
    for ( uint32 i = 0; i < p_waypoints->GetSize(); i++ )
    {
        p_waypoint = p_waypoints->GetStructure( i );

        // is this waypoint done?
        uint32 flagId;
        p_waypoint->GetChecksum( CRCD(0x2e0b1465,"flag"), &flagId, Script::ASSERT );
        if ( GoalFlagEquals( flagId, 1 ) )
            continue;
        
        // pause the goal and set the time for this tour spot
        PauseGoal();
		int time;
		p_waypoint->GetInteger( CRCD(0x906b67ba,"time"), &time, Script::ASSERT );
        // add time if necessary
		if ( mp_params->ContainsFlag( CRCD(0xfb11cb01,"cumulative_timer") ) )
		{
			AddTime( time );
		}
		else
		{
			mp_params->AddInteger( CRCD(0x906b67ba,"time"), time );
			SetTimer();
		}
            
        // add the goalId and trigger id to the params
        p_waypoint->AddChecksum( CRCD(0x9982e501,"goal_id"), GetGoalId() );

		// send a flag if this is the first spot
		if ( m_flagsSet == 0 )
			p_waypoint->AddChecksum( NONAME, CRCD( 0xb13f3ab8, "first_spot" ) );

		// send a flag if this is a restart
		if ( mp_params->ContainsFlag( CRCD(0x38221df4,"quick_start") ) )
			p_waypoint->AddChecksum( NONAME, CRCD(0x38221df4,"quick_start") );
		else
			p_waypoint->RemoveFlag( CRCD(0x38221df4,"quick_start") );

        
		#ifdef __NOPT_ASSERT__
		// backward compatible with horse goal
		Script::CScript *p_script=NULL;
		if ( mp_params->ContainsFlag( CRCD(0x9d65d0e7,"horse") ) )
			p_script=Script::SpawnScript( CRCD(0x681bc776,"goal_horse_next_spot"), p_waypoint );
		else
			p_script=Script::SpawnScript( CRCD(0x4c938553,"goal_tour_next_spot"), p_waypoint );
		p_script->SetCommentString("Spawned from CGoal::NextTourSpot()");
		#else	
		// backward compatible with horse goal
		if ( mp_params->ContainsFlag( CRCD(0x9d65d0e7,"horse") ) )
			Script::SpawnScript( CRCD(0x681bc776,"goal_horse_next_spot"), p_waypoint );
		else
			Script::SpawnScript( CRCD(0x4c938553,"goal_tour_next_spot"), p_waypoint );
		#endif			


		// remove restart flag
		p_waypoint->RemoveFlag( CRCD(0x38221df4,"quick_start") );


/*        // move the skater to the node
        uint32 reset_checksum;
        p_waypoint->GetChecksum( "id", &reset_checksum, Script::ASSERT );
        Mdl::Skate * p_skate = Mdl::Skate::Instance();
        p_skate->ResetSkaters( SkateScript::FindNamedNode( reset_checksum ) );

        uint32 waypoint_script;
        if ( p_waypoint->GetChecksum( "scr", &waypoint_script, Script::NO_ASSERT ) )
            Script::SpawnScript( waypoint_script );
*/        
        return true;

    }
    return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::InitTrickObjects()
{
    // get the object id's
    Script::CArray* p_trickObjectArray = NULL;
    mp_params->GetArray( CRCD(0x2eb84a54,"trick_objects"), &p_trickObjectArray, Script::ASSERT );
    
    Mdl::Skate* skate_mod = Mdl::Skate::Instance();
    Obj::CTrickObjectManager* p_TrickObjectManager = skate_mod->GetTrickObjectManager();

    for ( uint32 i = 0; i < p_trickObjectArray->GetSize(); i++ )
    {
        // add the trick object
        uint32 goal_object;
        Script::CStruct* p_trickObject = p_trickObjectArray[i].GetStructure( i );
        p_trickObject->GetChecksum( CRCD(0x40c698af,"id"), &goal_object, Script::ASSERT );
        
        // p_TrickObjectManager->RequestLogTrick( 1, 
        p_TrickObjectManager->ModulateTrickObjectColor( goal_object, 0 );
    }
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::GotTrickObject( uint32 clusterId )
{
	// do we care?
	if ( mp_params->ContainsFlag( CRCD( 0xf268c278, "SkateTheLine" ) ) )
	{
		// we do
		uint32 current;
		mp_params->GetChecksum( CRCD(0xb03713ee,"current_cluster"), &current, Script::ASSERT );
		if ( current == clusterId )
			Script::RunScript( CRCD( 0x62b12a8e, "Goal_SkateTheLine_got_node" ), mp_params );
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::IsGraffitiGoal()
{
	return ( mp_params->ContainsFlag( CRCD(0xc8a82b5a,"graffiti") ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::ShouldLogTrickObject()
{
	if ( mp_params->ContainsFlag( CRCD(0xc8a82b5a,"graffiti") ) || mp_params->ContainsFlag( CRCD(0xf268c278,"SkateTheLine") ) )
	{
		return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::GotGraffitiCluster( uint32 clusterId, int score )
{
    // find the cluster in our cluster array
    Script::CArray* pClusterArray;
    mp_params->GetArray( CRCD(0x191f66e9,"kill_clusters"), &pClusterArray, Script::ASSERT );

    for ( uint32 i = 0; i < pClusterArray->GetSize(); i++ )
    {
        Script::CStruct* pCluster = pClusterArray->GetStructure( i );
        Dbg_MsgAssert( pCluster, ( "got null cluster from kill_cluster array\n" ) );

        uint32 id;
        pCluster->GetChecksum( "id", &id, Script::ASSERT );
		
		int minScore;
		// get the score for this cluster, or the default score
		if ( !pCluster->GetInteger( CRCD(0xcd66c8ae,"score"), &minScore, Script::NO_ASSERT ) )
			mp_params->GetInteger( CRCD(0xcd66c8ae,"score"), &minScore, Script::ASSERT );
		
		// printf("minScore: %i, score: %i\n", minScore, score);

        if ( id == clusterId && score > minScore )
        {
			Mdl::Skate * skate_mod = Mdl::Skate::Instance();
			Obj::CTrickObjectManager* p_trickObjectMan = skate_mod->GetTrickObjectManager();

			// printf("modulating color\n");
			if ( !p_trickObjectMan->ModulateTrickObjectColor( clusterId, 0 ) )
				printf("ModulateTrickObjectColor didn't work\n");

			uint32 flag;
            pCluster->GetChecksum( CRCD(0x2e0b1465,"flag"), &flag, Script::ASSERT );

            SetGoalFlag( flag, 1 );
            return;
        }
    }
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::IsSpecialGoal()
{
    return ( mp_params->ContainsFlag( CRCD(0xb394c01c,"special") ) );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::CheckSpecialGoal()
{
    Mdl::Skate * skate_mod = Mdl::Skate::Instance();
    Obj::CSkater *p_Skater = skate_mod->GetLocalSkater();
    Dbg_Assert( p_Skater );
	
	Obj::CSkaterScoreComponent* p_SkaterScoreComponent = GetSkaterScoreComponentFromObject(p_Skater);
	Dbg_Assert( p_SkaterScoreComponent );
    
    Mdl::Score * p_Score = ( p_SkaterScoreComponent->GetScore() );
    Dbg_Assert( p_Score );

    const char* trick_string;
    mp_params->GetString( CRCD(0x3a54cc4b,"trick_string"), &trick_string, Script::ASSERT );

	// if it's a grind, we need to check for the FS and BS version too
	uint32 trick_type;
	mp_params->GetChecksum( CRCD(0x3d929b66,"trick_type"), &trick_type, Script::ASSERT );
	if ( trick_type == CRCD( 0x255ed86f, "grind" ) )
	{
		char test_trick[128];
		// frontside
		strcpy( test_trick, "FS " );
		strcat( test_trick, trick_string );
		// printf("testing FS version - %s\n", test_trick);
		if ( p_Score->GetCurrentNumberOfOccurrencesByName( Script::GenerateCRC( test_trick ) ) )
		{
			// printf("got it\n");
			return true;
		}

		// backside
		strcpy( test_trick, "BS " );
		strcat( test_trick, trick_string );
		// printf("testing BS version - %s\n", test_trick);
		if ( p_Score->GetCurrentNumberOfOccurrencesByName( Script::GenerateCRC( test_trick ) ) )
		{
			// printf("got it\n");
			return true;
		}
	}

	// check the bare trick string
	if ( mp_params->ContainsFlag( CRCD(0x8e6014f6,"create_a_trick") ) )
	{
		// printf("testing cat version %s\n", new_string );
		if ( p_Score->GetCurrentNumberOfOccurrencesByName( Script::GenerateCRC( trick_string ) ) > 0 )
		{
			return true;
		}
	}
	else
	{
		// printf("testing normal version - %s\n", trick_string);
		if ( p_Score->GetCurrentNumberOfOccurrencesByName( Script::GenerateCRC( trick_string ) ) > 0 )
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

bool CGoal::IsTetrisGoal()
{
    return ( mp_params->ContainsFlag( CRCD(0x4147922b,"tetris") ) );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CGoal::GetRandomIndexFromKeyCombos( Script::CArray* p_keyCombos )
{
	Dbg_MsgAssert( p_keyCombos->GetType() == ESYMBOLTYPE_NAME ||
				   p_keyCombos->GetType() == ESYMBOLTYPE_STRUCTURE,
				   ( "GetRandomIndexFromKeyCombos got array of the wrong type" ) );

	// get the global trick mapping
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	Obj::CSkaterProfile* p_SkaterProfile = skate_mod->GetCurrentProfile();
	Script::CStruct* p_trickMappings = p_SkaterProfile->GetTrickMapping( CRCD(0xd544aa2d,"trick_mapping") );

	// figure number of components in mapping
	uint32 size = p_keyCombos->GetSize();

	// safety first
	int max_tries = 100;
	
	uint32 trick_name_checksum = 0;
	uint32 key_combo;
	while ( !trick_name_checksum )
	{        
		int i = Mth::Rnd( size );
		if ( p_keyCombos->GetType() == ESYMBOLTYPE_NAME )
		{
			key_combo = p_keyCombos->GetChecksum( i );
		}
		else
			p_keyCombos->GetStructure( i )->GetChecksum( CRCD(0xacfdb27a,"key_combo"), &key_combo, Script::ASSERT );

		// see if this is a valid key_combo
		int cat_index;
		p_trickMappings->GetChecksum( key_combo, &trick_name_checksum, Script::NO_ASSERT );
		if ( trick_name_checksum )
		{
			return i;
		}
		else if ( p_trickMappings->GetInteger( key_combo, &cat_index, Script::NO_ASSERT ) )
		{
			return i;
		}
		// reset it to 0
		trick_name_checksum = 0;
		max_tries--;
		if ( max_tries == 0 )
		{
			// they've probably un-mapped all their keys.  check all the tricks in order
			// just to be safe
			for ( uint32 j = 0; j < size; j++ )
			{
				if ( p_keyCombos->GetType() == ESYMBOLTYPE_NAME )
					key_combo = p_keyCombos->GetChecksum( j );
				else
					p_keyCombos->GetStructure( j )->GetChecksum( CRCD(0xacfdb27a,"key_combo"), &key_combo, Script::ASSERT );
				p_trickMappings->GetChecksum( key_combo, &trick_name_checksum, Script::NO_ASSERT );
				if ( trick_name_checksum )
				{
					return i;
				}
				trick_name_checksum = 0;
			}
			// they did un-map all their keys.  Fuck 'em
			NoValidKeyCombos();
			break;
		}
	}
	return -1;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::NoValidKeyCombos()
{
	Script::CStruct* p_temp_params = new Script::CStruct();
	p_temp_params->AddChecksum( CRCD(0x9982e501,"goal_id"), GetGoalId() );
	Script::RunScript( CRCD( 0xf3c20af5, "goal_no_valid_key_combos" ), p_temp_params );
	delete p_temp_params;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
uint32 CGoal::GetRootGoalId()
{
	CGoalManager* pGoalManager = GetGoalManager();
	CGoal* pParentGoal = this;	
	Dbg_Assert( pGoalManager );
	
	// get the root node
	CGoalLink* p_parent = mp_parents;
	while ( p_parent && p_parent->m_relative != 0 )
	{
		pParentGoal = pGoalManager->GetGoal( p_parent->m_relative );
		p_parent = pParentGoal->GetParents();
	}

	return pParentGoal->GetGoalId();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
uint32 CGoal::GetGoalId()
{
    uint32 goalId = 0;
    mp_params->GetChecksum( "goal_id", &goalId, Script::ASSERT );
    return goalId;
}

bool CGoal::IsEditedGoal()
{
	Dbg_MsgAssert(mp_params,("NULL mp_params"));
	return mp_params->ContainsFlag(CRCD(0x981d3ad0,"edited_goal"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::HasWonGoal()
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();	
	if ( gamenet_man->InNetGame() && 
		 ( skate_mod->GetGameMode()->GetNameChecksum() == CRCD( 0xec200eaa, "netgoalattack" )) ||
		 ( skate_mod->GetGameMode()->GetNameChecksum() == CRCD(0x1c471c60,"netlobby")))
	{
		return m_netIsBeat;
	}
	return m_isbeat;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::HasWonStage()
{
	if ( HasWonGoal() )
		return true;	
	else
	{
		CGoalLink* pLink = mp_children;
		while ( pLink )
		{
			if ( pLink->m_beaten )
				return true;
			else
				pLink = pLink->mp_next;
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::IsSelected()
{
	return m_isselected;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::GotCounterObject()
{
	int number_collected = 0;
    mp_params->GetInteger( CRCD(0x129daa10,"number_collected"), &number_collected, Script::NO_ASSERT );
    // printf("alredy collected %i\n", number_collected);
    number_collected++;

    mp_params->AddInteger( CRCD(0x129daa10,"number_collected"), number_collected );

	if ( mp_params->ContainsFlag( CRCD(0xfd7ae1f4,"use_hud_counter") ) )
		Script::RunScript( CRCD(0xf2e85a34,"goal_update_counter"), mp_params );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::CounterGoalDone()
{
    int number_collected = 0;
    mp_params->GetInteger( CRCD(0x129daa10,"number_collected"), &number_collected, Script::NO_ASSERT );

    int number;
    mp_params->GetInteger( CRCD(0x696fe0ab,"number"), &number, Script::ASSERT );

    return ( number_collected >= number );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::MarkInvalid()
{
    m_invalid = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::IsInvalid()
{
    return m_invalid;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::HasProset( const char* proset_prefix )
{
    const char* p_local_proset;
    if ( mp_params->GetString( CRCD(0x955c6305,"proset_prefix"), &p_local_proset, Script::NO_ASSERT ) )
    {
        if ( stricmp( p_local_proset, proset_prefix ) == 0 )
            return true;
    }
    return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::SetHasSeen()
{
    m_hasSeen = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::MarkBeaten()
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	
	if( gamenet_man->InNetGame()  && skate_mod->GetGameMode()->GetNameChecksum() == CRCD( 0xec200eaa, "netgoalattack" ) )
	{
		m_netIsBeat = true;
	}
	else
	{
		m_isbeat = true;
	}
	// mark all goals in the tree beaten
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	Game::CGoalLink* p_link = mp_parents;
	while ( p_link && p_link->m_relative != 0 )
	{
		CGoal* pGoal = pGoalManager->GetGoal( p_link->m_relative );
		pGoal->MarkBeaten();
		p_link = p_link->mp_next;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::MarkBeatenBy( int id )
{
	Dbg_Printf( "**** Marking goal 0x%x as beaten\n", GetGoalId());
	m_beatenFlags.Set( id );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::MarkBeatenByTeam( int team_id )
{
	m_teamBeatenFlags.Set( team_id );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::HasWonGoal( int id )
{
	return m_beatenFlags.Test( id );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::HasTeamWonGoal( int team_id )
{
	return m_teamBeatenFlags.Test( team_id );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::Select()
{
	m_isselected = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::Deselect()
{
	m_isselected = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::SetUnlocked()
{
    m_isLocked = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::SetLevelNum( int levelNum )
{
    m_levelNum = levelNum;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
int CGoal::GetLevelNum()
{
    return m_levelNum;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::CheckMinigameRecord( int value )
{
	int record = 0;
	mp_params->GetInteger( CRCD(0x78ffb85c,"minigame_record"), &record, Script::NO_ASSERT );

	mp_params->AddInteger( CRCD(0x819fc1d1,"last_attempt"), value );
	if ( value > record )
	{
		mp_params->AddInteger( CRCD(0x78ffb85c,"minigame_record"), value );
		return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
int CGoal::GetMinigameRecord()
{
	int minigame_record = -1;
	mp_params->GetInteger( CRCD(0x78ffb85c,"minigame_record"), &minigame_record, Script::NO_ASSERT );
	return minigame_record;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::CanRetry()
{
	return ( !mp_params->ContainsFlag( CRCD(0xa612f12c,"no_restart") ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::IsMinigame()
{
	return ( mp_params->ContainsFlag( CRCD(0x6bae094c,"minigame") ) );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::SetStartTime()
{
	mp_params->AddInteger( CRCD(0x5fd80edc,"current_time"), 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::UpdateComboTimer()
{
	if ( !IsPaused() )
	{
		Tmr::Time current_time = 0;
		int time;
		mp_params->GetInteger( CRCD(0x5fd80edc,"current_time"), &time, Script::NO_ASSERT );
		current_time = (Tmr::Time)time;
	
		current_time +=  (Tmr::Time)( Tmr::FrameLength() * 1000 );
	
		mp_params->AddInteger( CRCD(0x5fd80edc,"current_time"), (int)current_time );

		// add normal units for display
		mp_params->AddInteger( CRCD(0x7b1fe9a4,"current_time_seconds"), (int)( current_time / 1000 ) );
		mp_params->AddInteger( CRCD(0xb4db9cb7,"current_time_fraction"), (int)( ( current_time / 10 ) % 100 ) );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::SetStartHeight()
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CSkater *p_skater = skate_mod->GetLocalSkater();
	Dbg_Assert( p_skater );

	int start_height = static_cast< int >(GetSkaterStateComponentFromObject(p_skater)->GetHeight());
	mp_params->AddInteger( CRCD(0xc07d8d60,"start_height"), start_height );

	// printf("setting start height to %i\n", start_height);
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::CheckHeightRecord()
{
	if ( !IsPaused() )
	{
		int record = 0;
		mp_params->GetInteger( CRCD(0x78ffb85c,"minigame_record"), &record, Script::NO_ASSERT );

		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		Obj::CSkater *p_skater = skate_mod->GetLocalSkater();
		Dbg_Assert( p_skater );

		int start_height = 10000;
		mp_params->GetInteger( CRCD(0xc07d8d60,"start_height"), &start_height, Script::NO_ASSERT );
		
		int current_height = static_cast< int >(GetSkaterStateComponentFromObject(p_skater)->GetHeight()) - start_height;

		//printf("current_height: %i\n", current_height);

		if ( current_height < 0 )
			current_height = 0;		

		int feet = (int)( current_height / 12 );
		int inches = current_height % 12;

		mp_params->AddInteger( CRCD(0x50815d42,"current_height_feet"), feet );
		mp_params->AddInteger( CRCD(0xb0c26927,"current_height_inches"), inches );
		
		if ( current_height > record )
		{
			mp_params->AddInteger( CRCD(0x78ffb85c,"minigame_record"), current_height );
			
			mp_params->AddInteger( CRCD(0x24bf3ae4,"record_feet"), feet );
			mp_params->AddInteger( CRCD(0x9b49f28d,"record_inches"), inches );
			return true;
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::CheckDistanceRecord()
{
	// find the object in question
	uint32 objId;
	mp_params->GetChecksum( CRCD(0x8e18f3b3,"distance_record_object"), &objId, Script::ASSERT );

	// grab the object
	Obj::CMovingObject* pObj = NULL;
//	pObj = Obj::CMovingObject::m_hash_table.GetItem( objId );
	pObj = (Obj::CMovingObject*) Obj::CCompositeObjectManager::Instance()->GetObjectByID( objId);

	// grab the skater
	Obj::CSkater* pSkater = Mdl::Skate::Instance()->GetLocalSkater();
	
	if ( pSkater && pObj )
	{
		float distSquared = Mth::DistanceSqr( pObj->m_pos, pSkater->m_pos );
		
		// get the height of the two
		float objHeight = pObj->m_pos.GetY();
		float skaterHeight = pSkater->m_pos.GetY();

		// get the base of the right triangle formed by the three points
		float distance = sqrtf(	distSquared - ( Mth::Sqr( skaterHeight - objHeight ) ) );
		// printf("distance = %f\n", distance);

		int feet = (int)( distance / 12 );
		int inches = (int)distance % 12;
		
		mp_params->AddInteger( CRCD(0xcb48c8e8,"current_distance_feet"), feet );
		mp_params->AddInteger( CRCD(0x644fc146,"current_distance_inches"), inches );
		
		int record = 0;
		mp_params->GetInteger( CRCD(0x78ffb85c,"minigame_record"), &record, Script::NO_ASSERT );		
		if ( distance > record )
		{
			mp_params->AddInteger( CRCD(0x78ffb85c,"minigame_record"), distance );
		
			mp_params->AddInteger( CRCD(0x24bf3ae4,"record_feet"), feet );
			mp_params->AddInteger( CRCD(0x9b49f28d,"record_inches"), inches );
			return true;
		}
	}	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::SetQuickStartFlag()
{
	mp_params->AddChecksum( NONAME, CRCD( 0x38221df4, "quick_start" ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::UnsetQuickStartFlag()
{
	mp_params->RemoveFlag( CRCD(0x38221df4,"quick_start") );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::IsProGoal()
{
	return ( mp_params->ContainsComponentNamed( CRCD(0xd303a1a3,"pro_goal") ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::IsNetGoal()
{
	return ( mp_params->ContainsComponentNamed( CRCD(0xd15ea00,"net") ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::IsCompetition()
{
	return ( mp_params->ContainsFlag( CRCD(0x4af5d34e,"competition") ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::IsPaused()
{
	return m_paused;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::UnBeatGoal()
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	if( gamenet_man->InNetGame() )
	{
		m_beatenFlags = 0;
		m_teamBeatenFlags = 0;
		if( m_netIsBeat )
		{
			m_netIsBeat = false;
			return true;
		}
	}
	else
	{
		if ( m_isbeat )
		{
			m_isbeat = false;
			return true;
		}
	}    
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::GetViewGoalsText( const char** p_view_goals_text )
{
	mp_params->GetText( "view_goals_text", p_view_goals_text, Script::NO_ASSERT );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::IsBettingGame()
{
	return ( mp_params->ContainsFlag( CRCD(0x8add4306,"betting_game") ) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::IsBettingGuy()
{
	return ( mp_params->ContainsFlag( CRCD(0x8cfee956,"betting_guy") ) );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::EndRunCalled()
{
	return m_endRunCalled;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::ClearEndRun( Script::CScript* pScript )
{
	// printf("CGoal::ClearEndRun called\n");
	m_endRunCalled = false;

	// TODO: this should pick the right skater somehow
	
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CSkater* p_skater = skate_mod->GetLocalSkater();

	Obj::CSkaterEndRunComponent* p_skater_endrun_component = GetSkaterEndRunComponentFromObject(p_skater);
	Dbg_Assert(p_skater_endrun_component);

	switch ( m_endRunType )
	{
	case vENDOFRUN:
// Mick: the way these type of exception is handled has changed
// I'm not sure if it still needs to be cleared here any more
// as it should have been executed immediately
// I'm removing this line for now, as the ExceptionComponent has gone  
//		GetExceptionComponentFromObject(p_skater)->ClearException( CRCD( 0x822e13a9, "RunHasEnded" ));
		p_skater->CallMemberFunction( CRCD( 0x4c58771e, "EndOfRunDone" ), NULL, pScript );
		p_skater_endrun_component->ClearStartedEndOfRunFlag();
		break;
	case vGOALENDOFRUN:
//		GetExceptionComponentFromObject(p_skater)->ClearException( CRCD( 0xab676175, "GoalHasEnded" ) );
		p_skater->CallMemberFunction( CRCD( 0x69a9e37f, "Goal_EndOfRunDone" ), NULL, pScript );
		p_skater_endrun_component->ClearStartedGoalEndOfRunFlag();
		break;
	default:
		Dbg_Assert( 0 );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::FinishedEndOfRun()
{
	// TODO: make this pick the right skater
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CSkater* p_skater = skate_mod->GetLocalSkater();

	Obj::CSkaterEndRunComponent* p_skater_endrun_component = GetSkaterEndRunComponentFromObject(p_skater);
	Dbg_Assert(p_skater_endrun_component);

	switch ( m_endRunType )
	{
	case vENDOFRUN:
		return p_skater_endrun_component->RunHasEnded();
		break;
	case vGOALENDOFRUN:
		return p_skater_endrun_component->GoalRunHasEnded();
		break;
	default:
		Dbg_Assert( 0 );
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::StartedEndOfRun()
{
	// TODO: make this choose the right skater
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CSkater* p_skater = skate_mod->GetLocalSkater();

	Obj::CSkaterEndRunComponent* p_skater_endrun_component = GetSkaterEndRunComponentFromObject(p_skater);
	Dbg_Assert(p_skater_endrun_component);

	switch ( m_endRunType )
	{
	case vENDOFRUN:
		return p_skater_endrun_component->StartedEndOfRun();
		break;
	case vGOALENDOFRUN:
		// return p_skater->StartedGoalEndOfRun();
		return m_endRunCalled;
		break;
	default:
		Dbg_Assert( 0 );
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// void CGoal::DisableEndOfRun()
// {
//	m_shouldEndRun = false;
// }


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::AddTempSpecialTrick()
{
	// get the new special trick name and type	
	uint32 trickType;
	bool is_cat = mp_params->ContainsFlag( CRCD(0x8e6014f6,"create_a_trick") );
	
	mp_params->GetChecksum( "trick_type", &trickType, Script::ASSERT );
	uint32 trickName;	
	if ( is_cat )
	{
		int tempTrickName;
		mp_params->GetInteger( CRCD(0xb502200,"premade_cat_index"), &tempTrickName, Script::ASSERT );
		trickName = (uint32)tempTrickName;
	}
	else
		mp_params->GetChecksum( "trick_name", &trickName, Script::ASSERT );
	
	// get the local string version of the trick and store it
	if ( is_cat )
	{
		Script::CArray* pPremadeCatArray = Script::GetArray( CRCD(0xffd7913f,"Premade_CATS"), Script::ASSERT );
		Script::CStruct* pPremadeCat = pPremadeCatArray->GetStructure( trickName );
		const char* pPremadeCatName;
		pPremadeCat->GetString( CRCD(0xa1dc81f9,"name"), &pPremadeCatName, Script::ASSERT );
		mp_params->AddString( CRCD(0x3a54cc4b,"trick_string"), pPremadeCatName );
	}
	else
	{
		const char* trickName_string; 
		Script::CStruct* p_trick = Script::GetStructure( trickName, Script::ASSERT );
		Script::CStruct* p_trick_params;
		p_trick->GetStructure( "Params", &p_trick_params, Script::ASSERT );
		p_trick_params->GetLocalString( "Name", &trickName_string, Script::ASSERT );
		mp_params->AddString( "trick_string", trickName_string );
	}
	
	// get the list of valid key bindings for this trick type
	Script::CStruct* p_keyBindings = NULL;
	switch ( trickType )
	{
		case CRCC( 0x61a1bc57, "cat" ):
		case CRCC( 0x439f4704, "air" ):
			p_keyBindings = Script::GetStructure( CRCD( 0x4764231d, "goal_special_tricks_air" ), Script::ASSERT );
			break;
		case CRCC( 0xa549b57b, "lip" ):
			p_keyBindings = Script::GetStructure( CRCD( 0xa1b2d162, "goal_special_tricks_lip" ), Script::ASSERT );
			break;
		case CRCC( 0x255ed86f, "grind" ):
			p_keyBindings = Script::GetStructure( CRCD( 0xf481d0cd, "goal_special_tricks_grind" ), Script::ASSERT );
			break;
		case CRCC( 0xef24413b, "manual" ):
			p_keyBindings = Script::GetStructure( CRCD( 0xd72d5cf7, "goal_special_tricks_manual" ), Script::ASSERT );
			break;
		default:
			Dbg_MsgAssert( p_keyBindings, ( "AddTempSpecialTrick got weird trick type\n" ) );
	}

	Obj::CPlayerProfileManager* pPlayerProfileManager = Mdl::Skate::Instance()->GetPlayerProfileManager();
	Obj::CSkaterProfile* pSkaterProfile = pPlayerProfileManager->GetCurrentProfile();
	// Script::CStruct* p_trickMapping = p_SkaterProfile->GetTrickMapping( CRCD(0xd544aa2d,"trick_mapping") );

	int numTricks = pSkaterProfile->GetNumSpecialTrickSlots();
	
	// we need to remember the key combo
	uint32 key_combo = 0;

	// search through current special tricks
	bool found_trick = false;
	for ( int i = 0; i < numTricks; i++ )
	{
		Obj::SSpecialTrickInfo trick_info = pSkaterProfile->GetSpecialTrickInfo( i );
		
		if ( !is_cat && trick_info.m_TrickName == trickName )
		{
			// printf("the special trick is already bound!\n");
			found_trick = true;
			key_combo = trick_info.m_TrickSlot;
			break;
		}
		p_keyBindings->RemoveFlag( trick_info.m_TrickSlot );
	}

	// if we didn't find the trick, grab the first available, unused key combo
	if ( !found_trick )
	{
		// printf("binding special trick\n");
		Script::CComponent* p_comp = p_keyBindings->GetNextComponent();
		Dbg_MsgAssert( p_comp, ( "Could not find an open key binding\n" ) );
		uint32 trickSlot = p_comp->mChecksum;
		Dbg_MsgAssert( trickSlot, ( "Could not find key binding in structure" ) );
		
		// give everyone a new slot
		/*int num_profiles = pPlayerProfileManager->GetNumProfileTemplates();
		for ( int i = 0; i < num_profiles; i++ )
		{
			Obj::CSkaterProfile* pTempProfile = pPlayerProfileManager->GetProfileTemplateByIndex( i );
			pTempProfile->AwardSpecialTrickSlot();
		}*/
		pSkaterProfile->AwardSpecialTrickSlot();

		key_combo = trickSlot;
		if ( is_cat )
		{
			// assign cat to trickslot
			Script::CStruct* pScriptParams = new Script::CStruct();
			pScriptParams->AddChecksum( CRCD(0xa92a2280,"trickSlot"), trickSlot );
			pScriptParams->AddInteger( CRCD(0x7f8c98fe,"index"), trickName );
			pScriptParams->AddInteger( CRCD(0x7f264be9,"special_trick_index"), numTricks );
			Script::RunScript( CRCD(0x466576c8,"load_premade_cat"), pScriptParams );
			delete pScriptParams;
		}
		else
		{
			// add new trick to new slot
			Obj::SSpecialTrickInfo trickInfo;
			trickInfo.m_TrickSlot = trickSlot;
			trickInfo.m_TrickName = trickName;
			pSkaterProfile->SetSpecialTrickInfo( numTricks, trickInfo );
		}

		// make a note of this so we can remove it later
		mp_params->AddInteger( CRCD( 0xbc678826, "should_remove_trick" ), 1 );
	}

	// testing
	// Script::PrintContents( p_SkaterProfile->GetSpecialTricksStructure() );

	// replace the key combo array
	mp_params->RemoveComponent( "key_combos" );
	Script::CArray* p_key_combos = new Script::CArray();
	p_key_combos->SetSizeAndType( 1, ESYMBOLTYPE_NAME );

	p_key_combos->SetChecksum( 0, key_combo );

	mp_params->AddArrayPointer( "key_combos", p_key_combos );
	// printf("added special trick\n");
	// Script::PrintContents( mp_params );
	ReplaceTrickText();
	
	return !found_trick;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::RemoveTempSpecialTrick()
{
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	Obj::CPlayerProfileManager* pPlayerProfileManager = skate_mod->GetPlayerProfileManager();
	Obj::CSkaterProfile* pSkaterProfile = pPlayerProfileManager->GetCurrentProfile();

	// do we need to take out the trick we added?
	int should_remove;
	mp_params->GetInteger( "should_remove_trick", &should_remove, Script::ASSERT );
	if ( should_remove == 1 )
	{
		// printf("removing special trick\n");
		// find it and take it out
		uint32 trickName;
		
		if ( mp_params->ContainsFlag( CRCD(0x8e6014f6,"create_a_trick") ) )
		{
			// int premade_cat_index;
			// mp_params->GetInteger( CRCD(0xb502200,"premade_cat_index"), &premade_cat_index, Script::ASSERT );
			// trickName = (uint32)premade_cat_index;
			
			// temp cat tricks are always on 0
			trickName = 0;
		}
		else
			mp_params->GetChecksum( CRCD( 0xef6fb249, "trick_name" ), &trickName, Script::ASSERT );

		// int numTricks = p_SkaterProfile->GetNumSpecialTrickSlots();
		// just look through all tricks
		for ( int i = 0; i < Obj::CSkaterProfile::vMAXSPECIALTRICKSLOTS; i++ )
		{
			Obj::SSpecialTrickInfo trick_info = pSkaterProfile->GetSpecialTrickInfo( i );
			// printf("m_TrickName: %x, trickName: %x\n", trick_info.m_TrickName, trickName );
			if ( trick_info.m_TrickName == trickName )
			{
				// printf("Removing special trick\n");
				Obj::SSpecialTrickInfo new_info;
				new_info.m_TrickName = CRCD( 0xf60c9090, "unassigned" );
				new_info.m_TrickSlot = CRCD( 0xf60c9090, "unassigned" );
				pSkaterProfile->SetSpecialTrickInfo( i, new_info );
				mp_params->AddInteger( CRCD( 0xbc678826, "should_remove_trick" ), 0 );
				break;
			}
		}
		
		// take a slot away from everyone
		/*int num_profiles = pPlayerProfileManager->GetNumProfileTemplates();
		for ( int i = 0; i < num_profiles; i++ )
		{
			Obj::CSkaterProfile* pTempProfile = pPlayerProfileManager->GetProfileTemplateByIndex( i );
			pTempProfile->AwardSpecialTrickSlot( -1 );
		}*/
		pSkaterProfile->AwardSpecialTrickSlot( -1 );
	}
	
	mp_params->RemoveComponent( CRCD(0x79704516,"key_combos") );

	Dbg_MsgAssert( (int)pSkaterProfile->GetNumSpecialTrickSlots() <= Script::GetInteger( CRCD(0x20297d6f,"max_number_of_special_trickslots") ), ( "You have too many trickslots!" ) );
	// Script::CStruct* pTest = pSkaterProfile->GetSpecialTricksStructure();
	// printf("done removing\n");
	// Script::PrintContents( pTest );

	// remove the display string just in case
	mp_params->RemoveComponent( "key_combo_string" );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::ShouldUseTimer()
{
	int unlimited_time = 0;

	mp_params->GetInteger( CRCD(0xf0e712d2,"unlimited_time"), &unlimited_time );

	return ( unlimited_time == 0 );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::CountAsActive()
{
	return IsActive();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::LoadSaveData( Script::CStruct *pFlags )
{
	int hasBeaten = 0;
	pFlags->GetInteger( "hasBeaten", &hasBeaten, Script::ASSERT );
	if ( hasBeaten != 0 )
		MarkBeaten();
	
	int isLocked = 1;
	pFlags->GetInteger( "isLocked", &isLocked, Script::ASSERT );
	if ( isLocked == 0 )
	{
		// special case for pro challenges
/*		if ( mp_params->ContainsFlag( "pro_specific_challenge" ) && hasBeaten == 0 )
		{
			if ( mp_goalPed->ProIsCurrentSkater() )
				SetUnlocked();
		}
		else
*/
		SetUnlocked();
	}

	int hasSeen = 0;
	pFlags->GetInteger( "hasSeen", &hasSeen, Script::ASSERT );
	if ( hasSeen != 0 )
		SetHasSeen();
	
	
	// check for a completion record
	int record = 0;
	pFlags->GetInteger( CRCD( 0xc22a2b72, "win_record" ), &record, Script::ASSERT );
	mp_params->AddInteger( "win_record", record );
	uint32 record_type;
	if ( mp_params->GetChecksum( "record_type", &record_type, Script::NO_ASSERT ) )
	{
		if ( record_type == Script::GenerateCRC( "time" ) )
			m_elapsedTimeRecord = record;
		else if ( record_type == Script::GenerateCRC( "score" ) )
			m_winScoreRecord = record;
	}

	pFlags->GetInteger( "timesLost", &m_numTimesLost, Script::ASSERT );
	pFlags->GetInteger( "timesWon", &m_numTimesWon, Script::ASSERT );
	pFlags->GetInteger( "timesStarted", &m_numTimesStarted, Script::ASSERT );

	uint32 team_pro;
	pFlags->GetChecksum( CRCD(0xced8a3a4,"team_pro"), &team_pro, Script::ASSERT );
	if ( team_pro != 0 )
	{
		mp_params->AddChecksum( CRCD(0xced8a3a4,"team_pro"), team_pro );
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::GetSaveData( Script::CStruct* pFlags )
{
	pFlags->AddInteger( "levelNum", GetLevelNum() );

	int isLocked = IsLocked();
	pFlags->AddInteger( "isLocked", isLocked );

	int hasSeen = HasSeenGoal();
	pFlags->AddInteger( "hasSeen", hasSeen );

	int hasBeaten = HasWonGoal();
	pFlags->AddInteger( "hasBeaten", hasBeaten );
	
/*	int isProSpecificChallenge = 0;
	if ( mp_params->ContainsFlag( "pro_specific_challenge" ) )
		isProSpecificChallenge = 1;
	pFlags->AddInteger( "isProSpecificChallenge", isProSpecificChallenge );
*/

	int record = 0;
	uint32 record_type;
	if ( mp_params->GetChecksum( "record_type", &record_type, Script::NO_ASSERT ) )
	{
		if ( record_type == Script::GenerateCRC( "time" ) )
			record = (int)m_elapsedTimeRecord;
		else if ( record_type == Script::GenerateCRC( "score" ) )
			record = m_winScoreRecord;
	}
	pFlags->AddInteger( "win_record", record );

	// TODO: should this be removed for the release?
	pFlags->AddInteger( "timesLost", m_numTimesLost );
	pFlags->AddInteger( "timesWon", m_numTimesWon );
	pFlags->AddInteger( "timesStarted", m_numTimesStarted );

	uint32 team_pro = 0;
	mp_params->GetChecksum( CRCD(0xced8a3a4,"team_pro"), &team_pro, Script::NO_ASSERT );
	pFlags->AddChecksum( CRCD(0xced8a3a4,"team_pro"), team_pro );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// utility used by ReplaceTrickText
bool fill_trick_and_key_combo_arrays( Script::CArray* p_key_combos, Script::CArray* p_key_combo_strings, Script::CArray* p_trick_names, int premade_cat_index )
{	
	// make sure the arrays are the right size and type
	Dbg_MsgAssert( p_key_combos->GetSize() == p_key_combo_strings->GetSize(), ( "key combo string array wrong size" ) );
	Dbg_MsgAssert( p_key_combos->GetSize() == p_trick_names->GetSize(),	( "trick name array wrong size" ) );
	Dbg_MsgAssert( p_key_combo_strings->GetType() == ESYMBOLTYPE_STRING, ( "array is the wrong type" ) );
	Dbg_MsgAssert( p_trick_names->GetType() == ESYMBOLTYPE_STRING, ( "array is the wrong type" ) );
	
	Script::CStruct* p_key_combo_map = Script::GetStructure( CRCD(0x8856c817,"goal_tetris_trick_text"), Script::ASSERT );
	
	Mdl::Skate * pSkate = Mdl::Skate::Instance();
	Obj::CSkaterProfile* pSkaterProfile = pSkate->GetCurrentProfile();
	Script::CStruct* pTricks = pSkaterProfile->GetTrickMapping( CRCD(0xd544aa2d,"trick_mapping") );

	// Script::PrintContents( pTricks );

	int size = p_key_combos->GetSize();
	
	for ( int i = 0; i < size; i++ )
	{
		uint32 key_combo = p_key_combos->GetChecksum( i );
		// printf("checking for key combo %s\n", Script::FindChecksumName( key_combo ) );
		uint32 trick_checksum = 0;
		bool found_a_match = false;
		int cat_trick_index = 0;
		bool found_cat = false;
		if ( pTricks->GetInteger( key_combo, &cat_trick_index, Script::NO_ASSERT ) )
		{
			// load cat info
			printf("found a cat trick - cat_trick_index = %i\n", cat_trick_index);
			found_cat = true;
			found_a_match = true;
		}
		else if ( pTricks->GetChecksum( key_combo, &trick_checksum, Script::NO_ASSERT ) )
		{
			found_a_match = true;
		}
		else
		{
			// look for a special key combo
			int numSpecials = pSkaterProfile->GetNumSpecialTrickSlots();
			// printf("checking special trick slots\n");
			for ( int j = 0; j < numSpecials; j++ )
			{
				Obj::SSpecialTrickInfo trick_info = pSkaterProfile->GetSpecialTrickInfo( j );
				// printf("trickslot: %s\n", Script::FindChecksumName( trick_info.m_TrickSlot ) );
				if ( trick_info.m_TrickSlot == key_combo )
				{
					// printf("found a match\n");
					trick_checksum = trick_info.m_TrickName;
					found_a_match = true;
					break;
				}
			}
		}

		if ( !found_a_match )
		{
			// Script::PrintContents( pTricks );
			// this key combo isn't assigned!  bail out...
			Dbg_MsgAssert( 0, ( "key combo %s not assigned!\n", Script::FindChecksumName( key_combo ) ) );
			return false;
		}

		const char* p_key_combo_string;
		p_key_combo_map->GetString( key_combo, &p_key_combo_string, Script::ASSERT );
		const char* p_trick_name;
		if ( found_cat )
		{            
			// printf("getting cat params\n");
			Obj::CSkater* pSkater = pSkate->GetLocalSkater();
			if ( pSkater )
			{
				// printf("got skater\n");
				Game::CCreateATrick* pCreatedTrick = pSkater->m_created_trick[cat_trick_index];
				Dbg_Assert( pCreatedTrick );
				// Script::PrintContents( pCreatedTrick->mp_other_params );
				pCreatedTrick->mp_other_params->GetString( CRCD( 0xa1dc81f9, "name" ), &p_trick_name, Script::ASSERT );
			}
		}
		else
		{
			if ( premade_cat_index > -1 )
			{
				Script::CArray* pPremadeCats = Script::GetArray( CRCD(0xffd7913f,"Premade_CATS"), Script::ASSERT );
				Script::CStruct* pPremadeCat = pPremadeCats->GetStructure( premade_cat_index );
				pPremadeCat->GetString( CRCD(0xa1dc81f9,"name"), &p_trick_name, Script::ASSERT );
			}
			else
			{
				Script::CStruct* p_trick_struct = Script::GetStructure( trick_checksum, Script::ASSERT );
				Script::CStruct* p_trick_params;
				p_trick_struct->GetStructure( CRCD(0x7031f10c,"Params"), &p_trick_params, Script::ASSERT );
				p_trick_params->GetLocalString( CRCD(0xa1dc81f9,"Name"), &p_trick_name, Script::ASSERT );
			}
		}

		// printf("setting strings: %s\n%s\n", p_key_combo_string, p_trick_name );
		
		p_key_combo_strings->SetString( i, Script::CreateString( p_key_combo_string ) );
		p_trick_names->SetString( i, Script::CreateString( p_trick_name ) );
	}
	return true;
}

// utility used by CGoal::ReplaceTrickText...finds and replaces \t and \k with
// the trick name and key combo, respectively
void find_and_replace_trick_string( const char* p_old, char* p_out, Script::CArray* p_key, Script::CArray* p_trick, uint out_buffer_size )
{
	#ifdef	__NOPT_ASSERT__
	const char *p_out_start = p_out;
	#endif
	const char *p_in = p_old;	
	while( *p_in )
	{
		int tag_num;
		if ( *p_in == '\\' && *(p_in+1) == 't' && ( *(p_in+2) <= '9' && *(p_in+2) >= '0' ) )
		{
			// printf("found the trick tag\n");
			
			// get the index
			tag_num = *(p_in+2) - 48;
			if ( *(p_in+3) <= '9' && *(p_in+3) >= '0' )
			{
				tag_num *= 10;
				tag_num += *(p_in+3) - 48;
				
				// move ahead an extra char
				p_in++;
			}
			// printf( "found a tag num of %i\n", tag_num );
			Dbg_MsgAssert( ( tag_num > 0 ) && tag_num <= (int)p_trick->GetSize(), ( "Tag value too large" ) );
			const char* p_trick_string = p_trick->GetString( tag_num - 1 );

			Dbg_MsgAssert( p_trick_string, ( "ReplaceTrickText found a \\t tag but no key combo." ) );

			sprintf( p_out, p_trick_string );
			int length = strlen( p_trick_string );
			for ( int i = 0; i < length; i++ )
				p_out++;
			
			// move past the tag
			p_in++;
			p_in++;
			p_in++;
		}
		else if ( *p_in == '\\' && *(p_in+1) == 'k' && ( *(p_in+2) <= '9' && *(p_in+2) >= '0' ) )
		{
			// printf("found the key tag\n");
			
			// get the index
			tag_num = *(p_in+2) - 48;
			if ( *(p_in+3) <= '9' && *(p_in+3) >= '0' )
			{
				tag_num *= 10;
				tag_num += *(p_in+3) - 48;
				
				// move ahead an extra char
				p_in++;
			}
			// printf( "found a tag num of %i\n", tag_num );
			Dbg_MsgAssert( ( tag_num > 0 ) && tag_num <= (int)p_key->GetSize(), ( "Tag value too large" ) );
			const char* p_key_string = p_key->GetString( tag_num - 1 );

			Dbg_MsgAssert( p_key_string, ( "ReplaceTrickText found a \\k tag but no key combo." ) );

			sprintf( p_out, p_key_string );
			int length = strlen( p_key_string );
			for ( int i = 0; i < length; i++ )
				p_out++;
			
			// move past the tag
			p_in++;
			p_in++;
			p_in++;
		}
		else
		{
			*p_out++ = *p_in++;
		}
	}
	*p_out++ = '\0';
	Dbg_MsgAssert(strlen(p_out_start) < out_buffer_size,("String overflow with\n\n%s\n\nMake it shorter than %d chars, or get a bigger buffer",p_out_start, out_buffer_size));
}

bool CGoal::ReplaceTrickText()
{
	// printf("ReplaceTrickText %s\n", Script::FindChecksumName( GetGoalId() ) );
	// see if there's a key_combo associated with this goal
	Script::CArray* p_key_combo = NULL;
	if ( !mp_params->GetArray( "key_combos", &p_key_combo, Script::NO_ASSERT ) )
	{
		// printf("no key combos\n");
		return false;
	}
	
	// Make sure we have copies of the original text.  We want
	// to update properly if they change their trick bindings.
	if ( !mp_params->ContainsComponentNamed( "original_goal_description" ) )
	{
		const char* p_ogd;
		Script::CArray* p_ogd_array;
		if ( mp_params->GetString( "goal_description", &p_ogd, Script::NO_ASSERT ) )
			mp_params->AddString( "original_goal_description", p_ogd );
		
		else if ( mp_params->GetArray( "goal_description", &p_ogd_array, Script::ASSERT ) )
			mp_params->AddArray( "original_goal_description", p_ogd_array );
	}
	if ( !mp_params->ContainsComponentNamed( "original_view_goals_text" ) )
	{
		const char* p_ovgt;
		if ( mp_params->GetString( "view_goals_text", &p_ovgt, Script::NO_ASSERT ) )
			mp_params->AddString( "original_view_goals_text", p_ovgt );
	}
	if ( !mp_params->ContainsComponentNamed( "original_goal_text" ) )
	{
		const char* p_ogt;
		if ( mp_params->GetString( "goal_text", &p_ogt, Script::NO_ASSERT ) )
			mp_params->AddString( "original_goal_text", p_ogt );
	}
	if ( !mp_params->ContainsComponentNamed( "original_key_combo_text" ) )
	{
		const char* p_okct;
		if ( mp_params->GetString( "key_combo_text", &p_okct, Script::NO_ASSERT ) )
			mp_params->AddString( "original_key_combo_text", p_okct );
	}

	// special case for cat goal
	int premade_cat_index = -1;
	if ( mp_params->ContainsFlag( CRCD(0x8e6014f6,"create_a_trick") ) )
	{
		mp_params->GetInteger( CRCD(0xb502200,"premade_cat_index"), &premade_cat_index, Script::ASSERT );
	}

	// get the new string arrays
	Script::CArray* p_key = new Script::CArray();
	Script::CArray* p_trick = new Script::CArray();
	int size = p_key_combo->GetSize();
	p_key->SetSizeAndType( size, ESYMBOLTYPE_STRING );
	p_trick->SetSizeAndType( size, ESYMBOLTYPE_STRING );
	if ( !fill_trick_and_key_combo_arrays( p_key_combo, p_key, p_trick, premade_cat_index ) )
	{
		Script::CleanUpArray( p_key );
		Script::CleanUpArray( p_trick );
		delete p_key;
		delete p_trick;
		return false;
	}
	
	const char* p_string;
	Script::CArray* p_string_array = NULL;

	char new_string[Game::NEW_STRING_LENGTH];
	
	// replace the goal description
	if ( mp_params->GetString( "original_goal_description", &p_string, Script::NO_ASSERT ) )
	{
		find_and_replace_trick_string( p_string, new_string, p_key, p_trick, Game::NEW_STRING_LENGTH );
		mp_params->AddString( "goal_description", p_string );
	}
	else if ( mp_params->GetArray( "original_goal_description", &p_string_array, Script::ASSERT ) )
	{
		int size = p_string_array->GetSize();
		Script::CArray* p_new_string_array = new Script::CArray();
		p_new_string_array->SetSizeAndType( size, ESYMBOLTYPE_STRING );
		for ( int i = 0; i < size; i++ )
		{
			p_string = p_string_array->GetString( i );
			find_and_replace_trick_string( p_string, new_string, p_key, p_trick, Game::NEW_STRING_LENGTH );
			p_new_string_array->SetString( i, Script::CreateString( new_string ) );
		}
		mp_params->AddArray( "goal_description", p_new_string_array );
		Script::CleanUpArray( p_new_string_array );
		delete p_new_string_array;
	}

	// replace the view goals text
	if ( mp_params->GetString( "original_view_goals_text", &p_string, Script::NO_ASSERT ) )
	{
		find_and_replace_trick_string( p_string, new_string, p_key, p_trick, Game::NEW_STRING_LENGTH );
		mp_params->AddString( "view_goals_text", new_string );
	}
	
	// replace the goal text
	if ( mp_params->GetString( "original_goal_text", &p_string, Script::NO_ASSERT ) )
	{
		find_and_replace_trick_string( p_string, new_string, p_key, p_trick, Game::NEW_STRING_LENGTH );
		mp_params->AddString( "goal_text", new_string );
	}

	// replace the key combo text
	if ( mp_params->GetString( "original_key_combo_text", &p_string, Script::NO_ASSERT ) )
	{
		find_and_replace_trick_string( p_string, new_string, p_key, p_trick, Game::NEW_STRING_LENGTH );
		mp_params->AddString( "key_combo_text", new_string );
	}

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
	
int CGoal::GetNumberCollected()
{
	int number_collected = 0;
	
	// check for special counter goal num
	if ( mp_params->GetInteger( CRCD(0x129daa10,"number_collected"), &number_collected, Script::NO_ASSERT ) )
		return number_collected;

	// return the number of flags set
	return m_flagsSet;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
int CGoal::GetNumberOfFlags()
{
	int num_flags_to_win = m_numflags;
	mp_params->GetInteger( CRCD(0x1171a555,"num_flags_to_win"), &num_flags_to_win, Script::NO_ASSERT );
	if ( num_flags_to_win != 0 )
		return num_flags_to_win;
	return -1;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::ColorTrickObjects( int seqIndex, bool clear )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Obj::CTrickObjectManager* p_trickObjMan = skate_mod->GetTrickObjectManager();
	
	Script::CArray* p_clusters = NULL;
	if ( !mp_params->GetArray( "kill_clusters", &p_clusters, Script::NO_ASSERT ) )
		return;

	int numClusters = p_clusters->GetSize();
	for ( int i = 0; i < numClusters; i++ )
	{
		Script::CStruct* p_temp = p_clusters->GetStructure( i );
		uint32 cluster_id;
		p_temp->GetChecksum( "id", &cluster_id, Script::ASSERT );
		
		Obj::CTrickCluster* p_trickCluster = p_trickObjMan->GetTrickCluster( cluster_id );
		if ( p_trickCluster )
		{
			if ( clear )
			{
				// printf("clearing cluster %s\n", Script::FindChecksumName( cluster_id ) );
				Obj::CTrickCluster* p_trickCluster = p_trickObjMan->GetTrickCluster( cluster_id );
				if ( p_trickCluster )
					p_trickCluster->ClearCluster( seqIndex );
			}
			else
				p_trickObjMan->GetTrickCluster( cluster_id )->ModulateTrickObjectColor( seqIndex );
		}
		else
			Dbg_Message( "WARNING: cluster %s not found\n", Script::FindChecksumName( cluster_id ) );
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
int CGoal::GetNumberOfTimesGoalStarted()
{
	return m_numTimesStarted;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::GetRewardsStructure( Script::CStruct *p_structure, bool give_cash )
{
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );

	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	
	if ( !mp_params->ContainsFlag( CRCD(0x981d3ad0,"edited_goal") ) )
		pGoalManager->AddGoalPoint();

	p_structure->AddChecksum( NONAME, CRCD( 0xc7fef529, "awarded_goal_point" ) );

	// No rewards in net games, only fanfare
	if( gamenet_man->InNetGame() == false )
	{
		// special case for special trick goal
		if ( mp_params->ContainsFlag( "reward_trickslot" ) && !mp_params->ContainsFlag( CRCD( 0x8e6014f6, "create_a_trick" ) ) )
		{
			// printf("found trickslot flag\n");
			Mdl::Skate* skate_mod = Mdl::Skate::Instance();
			Obj::CSkaterProfile* p_SkaterProfile = skate_mod->GetCurrentProfile();
			int num_slots = p_SkaterProfile->GetNumSpecialTrickSlots();
			if ( num_slots <= Script::GetInteger( CRCD(0x20297d6f,"max_number_of_special_trickslots") ) )
			{
				// printf("has less than the max number of slots\n");
				int should_remove = 0;
				mp_params->GetInteger( "should_remove_trick", &should_remove, Script::NO_ASSERT );
				if ( should_remove == 1 )
				{
					// printf("was going to remove the special trick, but don't!\n");
					mp_params->AddInteger( "should_remove_trick", 0 );
					// p_reward_params->AddChecksum( NONAME, CRCD( 0x65ca5ffc, "already_added_trickslot" ) );
					mp_params->AddChecksum( NONAME, CRCD( 0x65ca5ffc, "already_added_trickslot" ) );
					mp_params->AddInteger( CRCD(0xb45fd352,"just_added_trickslot"), 1 );
				}					
			}
		}		
		
		// ************************
		// new global reward stuff
		// ************************
		// grab the list of rewards
		Script::CStruct* p_all_rewards = Script::GetStructure( "goal_rewards", Script::ASSERT );
		Script::CStruct* p_rewards = NULL;
		p_all_rewards->GetStructure( GetGoalId(), &p_rewards, Script::NO_ASSERT );
		if ( p_rewards )
		{
			// map the reward_stat flag to an int
			if ( p_rewards->ContainsFlag( "reward_stat" ) )
			{
				p_rewards->RemoveComponent( "reward_stat" );
				p_rewards->AddInteger( "reward_stat", 1 );
			}

			p_structure->AppendStructure( p_rewards );
			
			if ( give_cash )
			{
				int reward_cash;
				if ( p_rewards->GetInteger( "reward_cash", &reward_cash, Script::NO_ASSERT ) )
					p_structure->AddInteger( "cash", reward_cash );
			}
		}
		// **************************
		// old reward stuff
		// **************************
		else
		{
			if ( give_cash )
			{
				int reward_cash;
				// give 'em cash
				if ( mp_params->GetInteger( "reward_cash", &reward_cash, Script::NO_ASSERT ) )
					p_structure->AddInteger( "cash", reward_cash );
			}

			// stat points
			int stat_points;
			if ( mp_params->ContainsFlag( "reward_stat" ) )
				p_structure->AddInteger( "reward_stat", 1 );
			else if ( mp_params->GetInteger( "reward_stat", &stat_points, Script::NO_ASSERT ) )
				p_structure->AddInteger( "reward_stat", stat_points );

		}
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::UnlockRewardGoals()
{
	CGoalManager* pGoalManager = GetGoalManager();
	
	// unlock a new goal
	Script::CArray* p_new_goal_array = NULL;
	uint32 new_goal_checksum = 0;

	if ( mp_params->GetChecksum( "reward_goal", &new_goal_checksum, Script::NO_ASSERT ) )
		pGoalManager->UnlockGoal( new_goal_checksum );

	// check for an array of goals to unlock
	else if ( mp_params->GetArray( "reward_goal", &p_new_goal_array, Script::NO_ASSERT ) )
	{
		int num_goals_to_unlock = p_new_goal_array->GetSize();
		for ( int i = 0; i < num_goals_to_unlock; i++ )
		{
			new_goal_checksum = p_new_goal_array->GetChecksum( i );
			if ( new_goal_checksum )
				pGoalManager->UnlockGoal( new_goal_checksum );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::SetEndRunType( EndRunType newEndRunType )
{
	m_endRunType = newEndRunType;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CGoal::SetShouldDeactivateOnExpire( bool should_deactivate )
{
	m_deactivateOnExpire = should_deactivate;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::IsHorseGoal()
{
	return mp_params->ContainsFlag( CRCD(0x9d65d0e7,"horse") );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::IsFindGapsGoal()
{
    return ( mp_params->ContainsFlag( CRCD( 0xc5ec08e6, "findGaps" ) ) );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CGoal::ShouldRequireCombo()
{
	return mp_params->ContainsFlag( CRCD(0xf8e31760,"require_combo") );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CGoal::AppendDifficultyLevelParams()
{
	Script::CArray* pLevelParams;
	if ( mp_params->GetArray( CRCD( 0x85104ab3, "difficulty_level_params" ), &pLevelParams, Script::NO_ASSERT ) )
	{
		CGoalManager* pGoalManager = GetGoalManager();
		Dbg_Assert( pGoalManager );
		Game::GOAL_MANAGER_DIFFICULTY_LEVEL difficulty_level = pGoalManager->GetDifficultyLevel();
		Dbg_MsgAssert( difficulty_level < (int)pLevelParams->GetSize(), ( "Difficulty params array has wrong size in goal %s", Script::FindChecksumName( GetGoalId() ) ) );
		mp_params->AppendStructure( pLevelParams->GetStructure( difficulty_level ) );
		CreateGoalFlags( mp_params );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CGoal::RunCallbackScript( uint32 script_name )
{
#if 1
	uint32 checksum;
	if ( mp_params->GetChecksum( script_name, &checksum, Script::NO_ASSERT ) )
	{
		Script::RunScript( checksum, mp_params );
		return true;
    }
#else
	Script::CStruct* pStruct;
	if ( mp_params->GetStructure( script_name, &pStruct, Script::NO_ASSERT ) )
	{
		// target script name is required
		uint32 checksum;
		pStruct->GetChecksum( CRCD(0xb990d003,"target"), &checksum, Script::ASSERT );

		// but not the params...
		Script::CStruct* pSubStruct = NULL;
		pStruct->GetStructure( CRCD(0x7031f10c,"params"), &pSubStruct, Script::NO_ASSERT );

		if ( pParams )
			pSubStruct->AppendStructure( pParams );
		
		Script::RunScript( checksum, pSubStruct );
		return true;
	}
#endif
	else
	{
		return false;
	}
}

}	// namespace Game
