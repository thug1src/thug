//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterEndRunComponent.h
//* OWNER:          Dan
//* CREATION DATE:  3/28/3
//****************************************************************************

#ifndef __COMPONENTS_SKATERENDRUNCOMPONENT_H__
#define __COMPONENTS_SKATERENDRUNCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#define		CRC_SKATERENDRUN CRCD(0xb1d5a3e1, "SkaterEndRun")

#define		GetSkaterEndRunComponent() ((Obj::CSkaterEndRunComponent*)GetComponent(CRC_SKATERENDRUN))
#define		GetSkaterEndRunComponentFromObject(pObj) ((Obj::CSkaterEndRunComponent*)(pObj)->GetComponent(CRC_SKATERENDRUN))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CSkaterEndRunComponent : public CBaseComponent
{
public:
    CSkaterEndRunComponent();
    virtual ~CSkaterEndRunComponent();
	
	enum EFlagType
	{
		STARTED_END_OF_RUN,
		FINISHED_END_OF_RUN,
		IS_ENDING_RUN,
		STARTED_GOAL_END_OF_RUN,
		FINISHED_GOAL_END_OF_RUN,
		IS_ENDING_GOAL
		// If adding more than 8 flags, tell Steve
	};

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	void							EndRun ( bool force_end = false );
	void							EndGoalRun ( bool force_end = false );
	bool							RunHasEnded (   );
	bool							GoalRunHasEnded (   );
	bool							StartedEndOfRun (   );
	void							ClearStartedEndOfRunFlag (   );
	bool							StartedGoalEndOfRun (   );
	void							ClearStartedGoalEndOfRunFlag (   );
	bool							IsEndingRun (   );
	void							ClearIsEndingRun (   );
	Flags<int>						GetFlags( void );
	void							SetFlags( Flags<int> flags );

	
private:
	Flags<int>						m_flags;
	
};

}

#endif
