#include <sk/modules/skate/goalmanager.h>
#include <sk/modules/skate/racegoal.h>

namespace Game
{


CRaceGoal::CRaceGoal( Script::CStruct* pParams ) : CGoal( pParams )
{
	// nothing yet
}

CRaceGoal::~CRaceGoal()
{
	// nope
}

bool CRaceGoal::ShouldExpire()
{
	if ( !m_unlimitedTime && (int)m_timeLeft <= 0)
	{
		return true;
	}
	return false;
}

bool CRaceGoal::IsExpired()
{
	if ( !m_unlimitedTime && (int)m_timeLeft <= 0)
	{
		CGoal::IsExpired();
		return true;
	}
	return false;
}

void CRaceGoal::Expire()
{
	RunCallbackScript( vEXPIRED );
	Deactivate();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CRaceGoal::NextRaceWaypoint( uint32 goalId )
{
    if ( IsExpired() )
	{
		printf("expired!\n");
		return false;
	}
	Script::CArray* p_waypoints;
    mp_params->GetArray( "race_waypoints", &p_waypoints, Script::ASSERT );

    Script::CStruct* p_waypoint;
    for ( uint32 i = 0; i < p_waypoints->GetSize(); i++ )
    {
        p_waypoint = p_waypoints->GetStructure( i );

        // is this waypoint done?
        uint32 flagId;
        p_waypoint->GetChecksum( "flag", &flagId, Script::ASSERT );
        if ( GoalFlagEquals( flagId, 1 ) )
        {
            continue;
        }
        
		if ( m_flagsSet == 0)
		{
			m_timeLeft = 0;
			p_waypoint->AddInteger( "first_waypoint", 1 );
		}
        
		// add the goalId to the params
        p_waypoint->AddChecksum( "goal_id", goalId );

		// add quick start flag
		if ( mp_params->ContainsFlag( CRCD(0x38221df4,"quick_start") ) )
			p_waypoint->AddChecksum( NONAME, CRCD(0x38221df4,"quick_start") );
		else
			p_waypoint->RemoveFlag( CRCD(0x38221df4,"quick_start") );

		#ifdef __NOPT_ASSERT__
        Script::CScript *p_script=Script::SpawnScript( "goal_race_next_waypoint", p_waypoint);
		p_script->SetCommentString("Spawned from CRaceGoal::NextRaceWaypoint");
		#else
        Script::SpawnScript( "goal_race_next_waypoint", p_waypoint);
		#endif

		// set the timer
		if ( !m_unlimitedTime )
		{
			int time;
			if ( !p_waypoint->GetInteger( "time", &time, Script::NO_ASSERT ) )
			{
				Dbg_MsgAssert( 0, ("Each race waypoint must have a 'time' param") );
			}
			CGoal::AddTime( time );
		}

/*        uint32 waypoint_script;
        if ( p_waypoint->GetChecksum( "scr", &waypoint_script, Script::NO_ASSERT ) )
            Script::SpawnScript( waypoint_script );
*/
        return true;
    }
    return true;
}

}
