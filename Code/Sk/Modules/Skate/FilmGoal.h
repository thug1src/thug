// film goal.

#ifndef __SK_MODULES_SKATE_FILMGOAL_H__
#define __SK_MODULES_SKATE_FILMGOAL_H__

#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

#include <sk/modules/skate/GoalManager.h>

namespace Game
{

class CFilmGoal : public CGoal
{

public:
						CFilmGoal( Script::CStruct* pParams );
	virtual				~CFilmGoal();

	bool				Activate();
	bool				Deactivate( bool force = false, bool affect_tree = true );
	bool				Update();

	void				CheckpointHit();
	void				StartFilming();
protected:
	bool				target_object_visible();

	bool				m_filming;
	
	Tmr::Time           m_timeOnCamera;
	Tmr::Time			m_timeRequired;
	Tmr::Time			m_totalTime;

	int					m_numShotsRequired;
	int					m_numShotsAchieved;

};

}

#endif

