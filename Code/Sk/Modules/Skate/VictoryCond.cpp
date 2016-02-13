/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Skate													**
**																			**
**	File name:		Skate/VictoryCond.cpp									**
**																			**
**	Created by:		02/07/01	-	gj										**
**																			**
**	Description:	Defines various victory conditions for a game			**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <gel/scripting/checksum.h>
#include <sk/modules/skate/score.h>
#include <sk/modules/skate/skate.h>
#include <sk/modules/skate/gamemode.h>
#include <sk/modules/skate/goalmanager.h>
#include <sk/modules/skate/victorycond.h>
#include <sk/objects/skater.h>
#include <sk/gamenet/gamenet.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Game
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

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
**							   PrivateFunctions								**
*****************************************************************************/

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CVictoryCondition::CVictoryCondition() //: Lst::Node< CVictoryCondition >( this )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CHighestScoreVictoryCondition::CHighestScoreVictoryCondition() : CVictoryCondition()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CHighestScoreVictoryCondition::ConditionComplete()
{
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CHighestScoreVictoryCondition::PrintDebugInfo()
{
//	printf( "Found CHighestScoreVictoryCondition\n" );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CHighestScoreVictoryCondition::IsTerminal()
{
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CHighestScoreVictoryCondition::IsWinner( uint32 skater_num )
{

	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	for ( uint32 i = 0; i < skate_mod->GetNumSkaters(); i++)
	{
		if ( i != skater_num )
		{
			Mdl::Score* pTestThisScore = skate_mod->GetSkater(skater_num)->GetScoreObject();

			// score object
			Mdl::Score* pScore = skate_mod->GetSkater(i)->GetScoreObject();

			if ( pTestThisScore->GetTotalScore() < pScore->GetTotalScore() )
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

CScoreReachedVictoryCondition::CScoreReachedVictoryCondition( uint32 target_score ) : CVictoryCondition()
{
	m_TargetScore = target_score;

//	PrintDebugInfo();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CScoreReachedVictoryCondition::ConditionComplete()
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	if( skate_mod->GetGameMode()->IsTeamGame())
	{
		GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		Lst::Search< GameNet::PlayerInfo > sh;
		GameNet::PlayerInfo* player;
		int i;
		int total_score[GameNet::vMAX_TEAMS] = {0};

		for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
		{
			Mdl::Score* pScore = player->m_Skater->GetScoreObject();

			total_score[player->m_Team] += pScore->GetTotalScore();
		}

		for( i = 0; i < GameNet::vMAX_TEAMS; i++ )
		{
			if( total_score[i] >= (int32)m_TargetScore )
			{
				return true;
			}
		}
	}
	else
	{
		for ( uint32 i = 0; i < skate_mod->GetNumSkaters(); i++)
		{
			// score object
			Mdl::Score* pScore = skate_mod->GetSkater(i)->GetScoreObject();
			
			if ( pScore->GetTotalScore() >= (int32)m_TargetScore )
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

bool
CScoreReachedVictoryCondition::PrintDebugInfo()
{
	printf( "Found CScoreReachedVictoryCondition %d\n", m_TargetScore );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	CScoreReachedVictoryCondition::GetTargetScore( void )
{
	return m_TargetScore;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CScoreReachedVictoryCondition::IsTerminal()
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CScoreReachedVictoryCondition::IsWinner( uint32 skater_num )
{

	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	for ( uint32 i = 0; i < skate_mod->GetNumSkaters(); i++)
	{
		if ( i != skater_num )
		{
			Mdl::Score* pTestThisScore = skate_mod->GetSkater(skater_num)->GetScoreObject();

			// score object
			Mdl::Score* pScore = skate_mod->GetSkater(i)->GetScoreObject();

			if ( pTestThisScore->GetTotalScore() < pScore->GetTotalScore() )
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

CGoalsCompletedVictoryCondition::CGoalsCompletedVictoryCondition( void )
: CVictoryCondition()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool GoalAttackComplete()
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Game::CGoalManager* pGoalManager;
	
	pGoalManager = Game::GetGoalManager();
	if( skate_mod->GetGameMode()->IsTeamGame())
	{
		int i, num_selected_goals;

		num_selected_goals = pGoalManager->GetNumSelectedGoals();
		for( i = 0; i < skate_mod->GetGameMode()->NumTeams(); i++ )
		{
			if( gamenet_man->GetTeamScore( i ) >= num_selected_goals )
			{
				return true;
			}
		}
	}
	else
	{
		uint32 i, j, num_goals;
		bool completed_all;
		Game::CGoal* pGoal;
		
		num_goals = pGoalManager->GetNumGoals();
		for( i = 0; i < skate_mod->GetNumSkaters(); i++ )
		{
			Obj::CSkater* skater = skate_mod->GetSkater(i);
			completed_all = true;
			for( j = 0; j < num_goals; j++ )
			{
				pGoal = pGoalManager->GetGoalByIndex( j );

				if( pGoal->GetParents()->m_relative != 0 )
				{
					continue;
				}
				if( pGoal->IsSelected())
				{
					if( pGoal->HasWonGoal( skater->GetID()) == false )
					{
						completed_all = false;
						break;
					}
				}
			}    

			if( completed_all )
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

bool
CGoalsCompletedVictoryCondition::ConditionComplete()
{
	return GoalAttackComplete();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CGoalsCompletedVictoryCondition::IsWinner( uint32 skater_num )
{
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CGoalsCompletedVictoryCondition::IsTerminal()
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CGoalsCompletedVictoryCondition::PrintDebugInfo()
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBestComboVictoryCondition::CBestComboVictoryCondition( uint32 num_combos ) : CVictoryCondition()
{
	m_NumCombos = num_combos;

//	PrintDebugInfo();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CBestComboVictoryCondition::ConditionComplete()
{
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CBestComboVictoryCondition::PrintDebugInfo()
{
	printf( "Found CBestComboVictoryCondition %d\n", m_NumCombos );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CBestComboVictoryCondition::IsTerminal()
{
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool
CBestComboVictoryCondition::IsWinner( uint32 skater_num )
{

	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	for ( uint32 i = 0; i < skate_mod->GetNumSkaters(); i++)
	{
		if ( i != skater_num )
		{
			Mdl::Score* pTestThisScore = skate_mod->GetSkater(skater_num)->GetScoreObject();

			// score object
			Mdl::Score* pScore = skate_mod->GetSkater(i)->GetScoreObject();

			if ( pTestThisScore->GetTotalScore() < pScore->GetTotalScore() )
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

CVictoryCondition*	CreateVictoryConditionInstance( Script::CScriptStructure* p_structure )
{
	

	Dbg_Assert( p_structure );

	uint32 condition_type;
	p_structure->GetChecksum( "type", &condition_type, true );

	CVictoryCondition* pVictoryCondition = NULL;
	
	switch ( condition_type )
	{
		case vVICTORYCOND_HIGHESTSCORE:
		{
			pVictoryCondition = new CHighestScoreVictoryCondition( );
		}
		break;
		case vVICTORYCOND_TARGETSCORE:
		{
			int target_score;
			p_structure->GetInteger( "score", &target_score, true );
			pVictoryCondition = new CScoreReachedVictoryCondition( (uint32)target_score );
		}
		break;
		case vVICTORYCOND_BESTCOMBO:
		{
			int num_combos;
			p_structure->GetInteger( "num_combos", &num_combos, true );
			pVictoryCondition = new CBestComboVictoryCondition( (uint32)num_combos );
		}
		break;
		case vVICTORYCOND_COMPLETEGOALS:
		{
			pVictoryCondition = new CGoalsCompletedVictoryCondition;
		}
		break;
		case vVICTORYCOND_LASTMANSTANDING:
		{
			pVictoryCondition = new CLastManStandingVictoryCondition;
		}
		break;
		default:
		{
			Dbg_MsgAssert( 0,( "Unrecognized victory condition type %08x %s", condition_type, Script::FindChecksumName( condition_type ) ));
		}
		break;
	}

	return pVictoryCondition;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CLastManStandingVictoryCondition::CLastManStandingVictoryCondition( void )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CLastManStandingVictoryCondition::ConditionComplete()
{
	GameNet::Manager* gamenet_man = GameNet::Manager::Instance();
	GameNet::PlayerInfo* player;

	// This logic may seem strange. Basically, when ConditionComplete() returns true,
	// the local skater ends his run and he can observe other players. So, it will return
	// true when his hit points have been depleted rather than when he has won.

	player = gamenet_man->GetLocalPlayer();
	if( player && player->m_Skater )
	{
		Mdl::Score* pScore = player->m_Skater->GetScoreObject();

		if( pScore->GetTotalScore() <= 0 )
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

bool	CLastManStandingVictoryCondition::PrintDebugInfo()
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CLastManStandingVictoryCondition::IsTerminal()
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CLastManStandingVictoryCondition::IsWinner( uint32 skater_num )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	// score object
	Mdl::Score* pScore = skate_mod->GetSkater(skater_num)->GetScoreObject();
	if( pScore && ( pScore->GetTotalScore() > 0 ))
	{
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Game

