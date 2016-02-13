#ifndef __SK_MODULES_SKATE_COMPETITIONGOAL_H__
#define __SK_MODULES_SKATE_COMPETITIONGOAL_H__

#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

#include <sk/modules/skate/GoalManager.h>

namespace Game
{


class CCompetitionGoal : public CGoal
{

public:
					CCompetitionGoal( Script::CStruct* pParams );
	virtual			~CCompetitionGoal();

	void			Expire();
	bool			IsActive();
	void			SetActive();
	void 			SetInactive();

	void			LoadSaveData( Script::CStruct* pFlags );
	void			GetSaveData( Script::CStruct* pFlags );

	bool			Win();
	bool			HasWonGoal();
	void			MarkBeaten();
	bool			UnBeatGoal();

	void			AwardCash( Script::CStruct* p_reward_params );

	void			PauseCompetition();
	void			UnPauseCompetition();

	bool			IsPaused();
protected:

	bool			m_compPaused;
	bool			m_gotGold;
	bool			m_gotSilver;
	bool			m_gotBronze;
	int				m_rewardGiven;
};

}

#endif
