// find x gaps

#ifndef __SK_MODULES_SKATE_FINDGAPSGOAL_H__
#define __SK_MODULES_SKATE_FINDGAPSGOAL_H__

#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

#include <sk/modules/skate/GoalManager.h>

namespace Game
{

class CFindGapsGoal : public CGoal
{

public:
						CFindGapsGoal( Script::CStruct* pParams );
	virtual				~CFindGapsGoal();

	bool				Activate();
	bool				Deactivate( bool force = false, bool affect_tree = true );

	void				CheckGaps();
protected:
	int					m_numGapsToFind;
};

}	// namespace game

#endif

