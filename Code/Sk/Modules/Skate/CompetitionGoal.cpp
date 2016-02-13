#include <sk/modules/skate/GoalManager.h>
#include <sk/modules/skate/CompetitionGoal.h>

#include <sk/modules/skate/skate.h>
#include <sk/modules/skate/competition.h>

#include <sk/objects/skaterprofile.h>

#include <gel/scripting/script.h>
#include <gel/scripting/utils.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/struct.h>

#include <sk/gamenet/gamenet.h>

namespace Game
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CCompetitionGoal::CCompetitionGoal( Script::CStruct* pParams )
	: CGoal( pParams )
{
	// uhh...necessary?
	m_compPaused = false;
	m_gotGold = m_gotSilver = m_gotBronze = false;
	m_rewardGiven = 0;

	m_winScoreRecord = vINT_MAX;

	m_endRunType = vENDOFRUN;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CCompetitionGoal::~CCompetitionGoal()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCompetitionGoal::Expire()
{
	// don't deactivate the goal!
	RunCallbackScript( Game::vEXPIRED );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CCompetitionGoal::IsActive()
{
	return m_active;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCompetitionGoal::SetActive()
{
	// printf("setting to active\n");
	m_active = true;
	m_compPaused = false;
	m_paused = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCompetitionGoal::SetInactive()
{
	m_active = false;
	m_compPaused = false;
	m_paused = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CCompetitionGoal::HasWonGoal()
{
	return m_isbeat;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCompetitionGoal::MarkBeaten()
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Mdl::CCompetition* pComp = skate_mod->GetCompetition();
	
	if ( pComp->PlaceIs( 1 ) )
	{
		m_gotGold = true;
		m_gotSilver = true;
		m_gotBronze = true;
	}
	else if ( pComp->PlaceIs( 2 ) )
	{
		m_gotSilver = true;
		m_gotBronze = true;
	}
	else if ( pComp->PlaceIs( 3 ) )
	{
		m_gotBronze = true;
	}

	// m_gotGold = true;
	// m_gotSilver = true;
	// m_gotBronze = true;
	m_isbeat = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CCompetitionGoal::UnBeatGoal()
{
	m_isbeat = false;
	m_gotBronze = false;
	m_gotSilver = false;
	if ( HasWonGoal() )
	{
		m_gotGold = false;
		return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CCompetitionGoal::Win()
{
	mp_params->AddChecksum( NONAME, CRCD( 0xc309cad1, "just_won_goal" ) );
	RunCallbackScript( Game::vDEACTIVATE );
	mp_params->RemoveFlag( CRCD( 0xc309cad1, "just_won_goal" ) );
	SetInactive();
	
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	
	if ( !gamenet_man->InNetGame() )
	{
		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		Mdl::CCompetition* pComp = skate_mod->GetCompetition();
		
		int place = 999;
		if ( pComp->PlaceIs( 1 ) )
			place = 1;
		else if ( pComp->PlaceIs( 2 ) )
			place = 2;
		else if ( pComp->PlaceIs( 3 ) )
			place = 3;

		if ( place < m_winScoreRecord )
		{
			mp_params->AddInteger( "win_record", place );
			m_winScoreRecord = place;
		}
	}

#ifdef __NOPT_ASSERT__
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
#endif		// __NOPT_ASSERT__
		
	Script::CStruct* p_reward_params = new Script::CStruct();
	
	if ( !HasWonGoal() )
	{
		// don't add cash
		Script::CStruct* pTempStruct = new Script::CStruct();
		GetRewardsStructure( pTempStruct, true );
		p_reward_params->AppendStructure( pTempStruct );
		delete pTempStruct;
		UnlockRewardGoals();
	}
	else
		mp_params->RemoveComponent( "reward_params" );

	// award any cash
	// AwardCash( p_reward_params );

	// mp_params->RemoveComponent( "reward_params" );
	mp_params->AddStructure( "reward_params", p_reward_params );
	RunCallbackScript( Game::vSUCCESS );
	
	delete p_reward_params;

	// now it's safe to mark as beaten
	if ( !HasWonGoal() )
		MarkBeaten();
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCompetitionGoal::AwardCash( Script::CStruct* p_reward_params )
{
#ifdef __NOPT_ASSERT__
	CGoalManager* pGoalManager = GetGoalManager();
	Dbg_Assert( pGoalManager );
#endif		// __NOPT_ASSERT__
	
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	Mdl::CCompetition* pComp = skate_mod->GetCompetition();
	
	// figure any cash reward
	if ( pComp->PlaceIs( 1 ) || pComp->PlaceIs( 2 ) || pComp->PlaceIs( 3 ) )
	{		
		int total_reward;
		int current_reward = 0;
		if ( mp_params->GetInteger( "reward_cash", &total_reward, Script::NO_ASSERT ) )
		{			
			if ( pComp->PlaceIs( 1 ) && !m_gotGold )
			{
				current_reward = total_reward - m_rewardGiven;
				m_gotGold = true;
				m_gotSilver = true;
				m_gotBronze = true;
			}
			else if ( pComp->PlaceIs( 2 ) && !m_gotSilver )
			{
				current_reward = total_reward * 2 / 3;
				if ( m_gotBronze )
					current_reward -= total_reward / 3;

				m_gotSilver = true;
				m_gotBronze = true;				

			}
			else if ( pComp->PlaceIs( 3 ) && !m_gotBronze )
			{
				current_reward = total_reward / 3;
				m_gotBronze = true;
			}

			if ( current_reward > 0 )
			{
				// pGoalManager->AddCash( current_reward );
				// Script::RunScript( "goal_got_cash" );
				p_reward_params->AddInteger( "cash", current_reward );
				m_rewardGiven += current_reward;
			}
		}
	}   
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCompetitionGoal::PauseCompetition()
{
	m_compPaused = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCompetitionGoal::UnPauseCompetition()
{
	m_compPaused = false;
	m_endRunCalled = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CCompetitionGoal::IsPaused()
{
	return ( m_paused || m_compPaused );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCompetitionGoal::LoadSaveData( Script::CStruct* pFlags )
{
	int gotBronze;
	pFlags->GetInteger( "gotBronze", &gotBronze, Script::NO_ASSERT );
	if ( gotBronze != 0 )
		m_gotBronze = true;

	int gotSilver;
	pFlags->GetInteger( "gotSilver", &gotSilver, Script::NO_ASSERT );
	if ( gotSilver != 0 )
		m_gotBronze = true;

	int gotGold;
	pFlags->GetInteger( "gotGold", &gotGold, Script::NO_ASSERT );
	if ( gotGold != 0 )
		m_gotBronze = true;

	int reward_given;
	pFlags->GetInteger( "reward_given", &reward_given, Script::NO_ASSERT );
	m_rewardGiven = reward_given;

	int record;
	if ( pFlags->GetInteger( "win_record", &record, Script::NO_ASSERT ) )
		m_winScoreRecord = record;
	
	CGoal::LoadSaveData( pFlags );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCompetitionGoal::GetSaveData( Script::CStruct* pFlags )
{
	CGoal::GetSaveData( pFlags );
	
	if ( m_gotBronze )
		pFlags->AddInteger( "gotBronze", 1 );
	else
		pFlags->AddInteger( "gotBronze", 0 );

	if ( m_gotSilver )
		pFlags->AddChecksum( "gotSilver", 1 );
	else
		pFlags->AddChecksum( "gotSilver", 0 );

	if ( m_gotGold )
		pFlags->AddChecksum( "gotGold", 1 );
	else
		pFlags->AddChecksum( "gotGold", 0 );

	pFlags->AddInteger( "win_record", m_winScoreRecord );
	pFlags->AddInteger( "reward_given", m_rewardGiven );
}

}
