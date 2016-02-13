//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterRunTimerComponent.h
//* OWNER:          Dan
//* CREATION DATE:  7/17/3
//****************************************************************************

#ifndef __COMPONENTS_SKATERRUNTIMERCOMPONENT_H__
#define __COMPONENTS_SKATERRUNTIMERCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/objtrack.h>

#include <gel/object/basecomponent.h>

#include <sk/objects/skater.h>

#define		CRC_SKATERRUNTIMER CRCD(0x8bce7e66, "SkaterRunTimer")

#define		GetSkaterRunTimerComponent() ((Obj::CSkaterRunTimerComponent*)GetComponent(CRC_SKATERRUNTIMER))
#define		GetSkaterRunTimerComponentFromObject(pObj) ((Obj::CSkaterRunTimerComponent*)(pObj)->GetComponent(CRC_SKATERRUNTIMER))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CWalkComponent;

class CSkaterRunTimerComponent : public CBaseComponent
{
	enum EState
	{
		INACTIVE,
		ACTIVE_RUNNING,
		ACTIVE_PAUSED,
		ACTIVE_TIME_UP
	};
	
	// number of divisions of the timer; you always lose at least one chunk of timer per runout
	enum { vRT_NUM_TIME_CHUNKS = 8 };
	
public:
    CSkaterRunTimerComponent();
    virtual ~CSkaterRunTimerComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    virtual void            		Finalize();
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );
	
	CSkater*						GetSkater() { return static_cast< CSkater* >(GetObject()); }
	
	void							ComboStarted (   );
	void							ComboEnded (   );

	static CBaseComponent*			s_create();
	
private:
	void							update_timer (   );
	void							pause (   );
	void							unpause (   );
	bool							is_timer_up (   );
	void							set_state ( EState state );
	uint32							get_run_timer_controller_id (   );
	
private:
	EState							m_state;
	
	float							m_timer;
	
	short							m_unpause_count;
	
	CWalkComponent*					mp_walk_component;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CSkaterRunTimerComponent::set_state ( EState state )
{
	m_state = state;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline uint32 CSkaterRunTimerComponent::get_run_timer_controller_id (   )
{
	return CRCD(0x83321dea, "RunTimerController") + GetSkater()->GetSkaterNumber();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CSkaterRunTimerComponent::ComboStarted (   )
{
	m_timer = GetSkater()->GetScriptedStat(CRCD(0xb84f532, "Physics_RunTimer_Duration"));
	m_unpause_count = 0;
	
	set_state(ACTIVE_PAUSED);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CSkaterRunTimerComponent::update_timer (   )
{
	m_timer -= Tmr::FrameLength();
	
	if (m_timer < 0.0f)
	{
		set_state(ACTIVE_TIME_UP);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CSkaterRunTimerComponent::pause (   )
{
	if (m_state == INACTIVE) return;
	
	set_state(ACTIVE_PAUSED);
	
	CTracker::Instance()->LaunchEvent(CRCD(0x813cc576, "HideRunTimer"), get_run_timer_controller_id(), GetObject()->GetID());
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CSkaterRunTimerComponent::is_timer_up (   )
{
	Dbg_Assert(m_state != INACTIVE);
	
	return m_state == ACTIVE_TIME_UP;
}

}

#endif
