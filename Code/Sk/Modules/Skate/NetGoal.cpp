#include <sk/objects/skater.h>
#include <sk/modules/skate/skate.h>
#include <sk/modules/skate/victorycond.h>
#include <sk/modules/skate/gamemode.h>
#include <sk/modules/skate/GoalManager.h>
#include <sk/modules/skate/NetGoal.h>
#include <sk/components/skaterendruncomponent.h>

#include <sk/gamenet/gamenet.h>

namespace Game
{

CNetGoal::CNetGoal( Script::CStruct* pParams ) : CGoal( pParams )
{
	m_endRunType = vENDOFRUN;
	m_initialized = false;
}

CNetGoal::~CNetGoal()
{
	// nothing yet
}

bool CNetGoal::Update()
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	if ( !m_initialized )
	{
		Init();
		m_initialized = true;
	}

	if( skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netgoalattack" ))
	{
		if( GoalAttackComplete())
		{
			if( IsActive())
			{
				Expire();

				return true;
			}
		}
	}

	return CGoal::Update();
}

bool CNetGoal::Activate( void )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	gamenet_man->ClearGameOver();

	return CGoal::Activate();
}

bool CNetGoal::Deactivate( bool force, bool affect_tree )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	if( force == false )
	{
		if( skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netgoalattack" ))
		{
			return false;
		}
	}

	return CGoal::Deactivate( force, affect_tree );
}

bool CNetGoal::IsExpired()
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	if( skate_mod->GetGameMode()->GetNameChecksum() == Script::GenerateCRC( "netgoalattack" ))
	{
		return GoalAttackComplete() && AllSkatersAtEndOfRun();
	}
	else
	{
		GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
		GameNet::PlayerInfo* player;
		Lst::Search< GameNet::PlayerInfo > sh;
	
		// If no players are left, the goal should expire
		if( gamenet_man->GetNumPlayers() == 0 )
		{
			//Dbg_Printf( "IsExpired: 1\n" );
			return true;
		}
						
		// If it's an unlimited game, don't expire the goal
		if ( m_unlimitedTime )
		{
			//Dbg_Printf( "IsExpired: 2\n" );
			return false;
		}
		
		if( ( gamenet_man->HaveReceivedFinalScores() == false ) && 
			( gamenet_man->InNetGame()))
		{
			//Dbg_Printf( "IsExpired: 3\n" );
			return false;
		}

		// if we're not supposed to use end of run, we need only check
		// the time
		if ( m_endRunType == vNONE )
		{
			//Dbg_Printf( "Timeleft: %d\n", (int) m_timeLeft );
			return ( (int)m_timeLeft <= 0 );
		}
	
		// check if the skaters have reached end of run BEFORE
		// setting the exception, or you'll never catch it!
		if ( AllSkatersAtEndOfRun() && (int)m_timeLeft <= 0 )
		{
			//Dbg_Printf( "IsExpired: 4\n" );
			return true;
		}
	
		// reset all players' scores
		for( player = gamenet_man->FirstPlayerInfo( sh ); player; player = gamenet_man->NextPlayerInfo( sh ))
		{
			Obj::CSkater *p_Skater = player->m_Skater;
			Dbg_Assert( p_Skater );

			Obj::CSkaterEndRunComponent* p_skater_endrun_component = GetSkaterEndRunComponentFromObject(p_Skater);
			Dbg_Assert( p_skater_endrun_component );

			if( p_skater_endrun_component->RunHasEnded() || p_Skater->IsPaused())
			{
				continue;
			}
			// if time is up, set the end of run exception
			// if ( m_shouldEndRun && !m_endRunCalled && (int)m_timeLeft <= 0 )
			if ( m_endRunType != vNONE && (int)m_timeLeft <= 0 )
			{
				m_endRunCalled = true;
				//Dbg_Printf( "EXPIRING GOAL: %d\n", GetGoalId());
				// printf("calling EndRun\n");
				p_skater_endrun_component->EndRun();
			}
		}

		//Dbg_Printf( "IsExpired: 6 : %d\n", gamenet_man->GameIsOver());
		return gamenet_man->GameIsOver();

	}
}

void	CNetGoal::Expire( void )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	gamenet_man->MarkGameOver();
	return CGoal::Expire();
}

}	// namespace Game
