#ifndef __SK_MODULES_SKATE_MINIGAME_H__
#define __SK_MODULES_SKATE_MINIGAME_H__

#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

#include <sk/modules/skate/GoalManager.h>

namespace Game
{


class CMinigame : public CGoal
{

public:						
					CMinigame( Script::CStruct* pParams );
	virtual			~CMinigame();

	bool			Activate();

	void			LoadSaveData( Script::CStruct* pFlags );
	void			GetSaveData( Script::CStruct* pFlags );
	
	bool			ShouldUseTimer();
	bool			AddTime( int amount );
	bool			CountAsActive() { return false; }
	bool			IsExpired();
	bool			CheckRecord( int value );
	int				GetRecord();
	bool			CanRetry();
	void			SetStartTime();
	void			UpdateTimer();

	bool			Update();
	void			UpdateMinigameTimer();

	bool			AwardCash( int amount );

protected:
	int				m_record;
	int				m_cashLimit;
};

}

#endif
