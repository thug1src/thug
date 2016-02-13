//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterPhysicsControlComponent.h
//* OWNER:          Dan
//* CREATION DATE:  3/7/3
//****************************************************************************

#ifndef __COMPONENTS_SKATERPHYSICSCONTROLCOMPONENT_H__
#define __COMPONENTS_SKATERPHYSICSCONTROLCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#include <sk/objects/skater.h>
#include <sk/components/skaterstatecomponent.h>

#define		CRC_SKATERPHYSICSCONTROL CRCD(0xd9151d4d, "SkaterPhysicsControl")

#define		GetSkaterPhysicsControlComponent() ((Obj::CSkaterPhysicsControlComponent*)GetComponent(CRC_SKATERPHYSICSCONTROL))
#define		GetSkaterPhysicsControlComponentFromObject(pObj) ((Obj::CSkaterPhysicsControlComponent*)(pObj)->GetComponent(CRC_SKATERPHYSICSCONTROL))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CSkaterCorePhysicsComponent;
	class CSkaterStateComponent;
	class CRailManager;
	class CRailNode;
	
	enum ESpecialTransitionType
	{
		NO_SPECIAL_TRANSITION,
		RAIL_TRANSITION,
		ACID_DROP_TRANSITION
	};

	struct SRailData
	{
		Mth::Vector					rail_pos;
		CRailManager*				p_rail_man;
		CRailNode*					p_node;
		CCompositeObject*			p_movable_contact;
	};
	
	struct SAcidDropData
	{
		Mth::Vector					target_pos;
		Mth::Vector					target_normal;
		float						true_target_height;
	};

class CSkaterPhysicsControlComponent : public CBaseComponent
{
public:
    CSkaterPhysicsControlComponent();
    virtual ~CSkaterPhysicsControlComponent();

public:
    virtual void            		Update();
    virtual void            		Finalize();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
			 	
	CSkater*						GetSkater() { return static_cast< CSkater* >(GetObject()); }

	void							SuspendPhysics ( bool suspend );
	bool							IsPhysicsSuspended (   ) { return m_physics_suspended; }
	
	bool							IsSkating (   );
	bool							IsWalking (   );
	bool							IsDriving (   );
	
	Tmr::Time						GetStateSwitchTime (   );
	Tmr::Time						GetPreviousPhysicsStateDuration (   );
	
	bool							IsBoardMissing (   ) { return m_board_missing; }
	bool							HaveBeenReset (   ) { return m_restarted_this_frame; }
	void							NotifyReset (   ) { m_restarted_this_frame = true; }
	
private:
	CCompositeObject*				get_skater_camera (   );
	void							suspend_physics ( bool suspend );
	
	void							switch_skating_to_walking (   );
	void							switch_walking_to_skating (   );

#	ifdef TESTING_GUNSLINGER
	void							switch_walking_to_riding (   );
	void							switch_riding_to_walking (   );
#	endif


private:
	bool							m_physics_suspended;
	
	Tmr::Time						m_physics_state_switch_time_stamp;
	Tmr::Time						m_previous_physics_state_duration;
	
	bool							m_board_missing;
	
	bool							m_restarted_this_frame;  		// Set if a restart occured, so we can early out
	
	// peer components
	CSkaterCorePhysicsComponent*	mp_core_physics_component;
	CSkaterStateComponent*			mp_state_component;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CSkaterPhysicsControlComponent::IsDriving (   )
{
	return mp_state_component->m_driving;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CSkaterPhysicsControlComponent::IsSkating (   )
{
	return !IsDriving() && mp_state_component->m_physics_state == SKATING;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CSkaterPhysicsControlComponent::IsWalking (   )
{
	return !IsDriving() && mp_state_component->m_physics_state == WALKING;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Tmr::Time CSkaterPhysicsControlComponent::GetStateSwitchTime (   )
{
	return m_physics_state_switch_time_stamp;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Tmr::Time CSkaterPhysicsControlComponent::GetPreviousPhysicsStateDuration (   )
{
	return m_previous_physics_state_duration;
}

/*****************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CSkaterPhysicsControlComponent::SuspendPhysics ( bool suspend )
{
	if (m_physics_suspended == suspend) return;
	
	suspend_physics(suspend);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline CCompositeObject* CSkaterPhysicsControlComponent::get_skater_camera (   )
{
	
	return static_cast< CSkater* >(GetObject())->GetCamera();
}

}

#endif
