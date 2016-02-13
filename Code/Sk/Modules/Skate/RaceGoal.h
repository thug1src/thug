#ifndef __SK_MODULES_SKATE_RACEGOAL_H__
#define __SK_MODULES_SKATE_RACEGOAL_H__

#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

#include <sk/modules/skate/GoalManager.h>

namespace Game
{

class CRaceGoal : public CGoal
{
	friend class CGoalManager;

public:
					CRaceGoal( Script::CStruct* pParams );
	virtual			~CRaceGoal();

	bool			ShouldExpire();
	bool			IsExpired();
	void			Expire();
	bool			NextRaceWaypoint( uint32 goalId );
};

}

#endif
