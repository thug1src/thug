// Create-A-Trick goal!

#include <sk/modules/skate/catgoal.h>
#include <sk/modules/skate/GoalManager.h>

namespace Game
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCatGoal::CCatGoal( Script::CStruct* pParams ) : CGoal( pParams )
{
	// uh...
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCatGoal::~CCatGoal()
{
	// hmm...
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CCatGoal::Activate()
{
	return CGoal::Activate();
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCatGoal::Deactivate( bool force, bool affect_tree )
{
	return CGoal::Deactivate( force, affect_tree );
}


}	// namespace game
