// horse goal subclass
#ifndef __SK_MODULES_SKATE_HORSEGOAL_H__
#define __SK_MODULES_SKATE_HORSEGOAL_H__

#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

#include <sk/modules/skate/GoalManager.h>

namespace Game
{


class CHorseGoal : public CGoal
{

public:
					CHorseGoal( Script::CStruct* pParams );
	virtual			~CHorseGoal();
	
	void			CheckScore();
	bool			ShouldExpire();
	bool			IsExpired();
	void			Expire();
};

}

#endif
