//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterPhysicsControlComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/7/3
//****************************************************************************

#include <sk/components/skaterphysicscontrolcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/components/triggercomponent.h>
#include <gel/components/walkcomponent.h>
#include <gel/components/skatercameracomponent.h>
#include <gel/components/walkcameracomponent.h>

#ifdef TESTING_GUNSLINGER
#include <gel/components/ridercomponent.h>
#endif

#include <sk/modules/skate/skate.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/components/skaterrotatecomponent.h>
#include <sk/components/skateradjustphysicscomponent.h>
#include <sk/components/skaterfinalizephysicscomponent.h>
#include <sk/components/skaterstatecomponent.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CSkaterPhysicsControlComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterPhysicsControlComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterPhysicsControlComponent::CSkaterPhysicsControlComponent() : CBaseComponent()
{
	SetType( CRC_SKATERPHYSICSCONTROL );
	
	mp_core_physics_component = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterPhysicsControlComponent::~CSkaterPhysicsControlComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterPhysicsControlComponent::InitFromStructure( Script::CStruct* pParams )
{
	Dbg_MsgAssert(GetObject()->GetType() == SKATE_TYPE_SKATER, ("CSkaterPhysicsControlComponent added to non-skater composite object"));
	
	m_physics_suspended = false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterPhysicsControlComponent::Finalize(  )
{
	mp_core_physics_component = GetSkaterCorePhysicsComponentFromObject(GetObject());
	mp_state_component = GetSkaterStateComponentFromObject(GetObject());
	
	Dbg_Assert(mp_core_physics_component);
	Dbg_Assert(mp_state_component);
	
	mp_state_component->m_physics_state = NO_STATE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterPhysicsControlComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterPhysicsControlComponent::Update()
{
	m_restarted_this_frame = false;
	
	// SkaterPhysicsControlComponent is inactive when physics is NOT suspended.  When physics is suspended SkaterPhysicsControlComponent handles
	// setting the skater's display matrix.
	if (!m_physics_suspended) return;
	
	switch (mp_state_component->m_physics_state)
	{
		case SKATING:
			GetObject()->SetDisplayMatrix(mp_core_physics_component->m_lerping_display_matrix);
			break;
		
		case WALKING:
			GetObject()->SetDisplayMatrix(GetObject()->GetMatrix());
			break;
			
		default:
			Dbg_MsgAssert(false, ("No skater physics state switched on before first skater update"));
	}
	
	// If the skater is unsuspended, the physics components will unsuspend as well.  We must check for this and resuspend
	if (!mp_core_physics_component->IsSuspended())
	{
		suspend_physics(true);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterPhysicsControlComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		// @script | PausePhysics | PausePhysics will stop all programmatic movement and rotation of the skater
		case CRCC(0x3522d612, "PausePhysics"):
			if (!m_physics_suspended)
			{
				suspend_physics(true);
			}
			break;
			
        // @script | UnPausePhysics | 
		case CRCC(0x595627c, "UnPausePhysics"):
			if (m_physics_suspended)
			{
				suspend_physics(false);
			}
			break;
			
		case CRCC(0x57bfbae8, "Walking"):
			return IsWalking() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
		case CRCC(0xf2813ee5, "Skating"):
			return IsSkating() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
		case CRCC(0xee584cbc, "Driving"):
			return IsDriving() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
		case CRCC(0xb32e9eae, "SetDriving"):
			mp_state_component->m_driving = true;
			break;
			
		case CRCC(0x953a7b6d, "UnsetDriving"):
			mp_state_component->m_driving = false;
			break;
			
		case CRCC(0x9569d5b1, "GetTimeSincePhysicsSwitch"):
			pScript->GetParams()->AddFloat(CRCD(0x27f48e93, "TimeSincePhysicsSwitch"), Tmr::ElapsedTime(m_physics_state_switch_time_stamp) * (1.0f / 1000.0f));
			break;
			
		case CRCC(0xa887285a, "GetPreviousPhysicsStateDuration"):
			pScript->GetParams()->AddFloat(CRCD(0x24d061ba, "PreviousPhysicsStateDuration"), m_previous_physics_state_duration * (1.0f / 1000.0f));
			break;
			
		case CRCC(0x5c038f9b, "SkaterPhysicsControl_SwitchWalkingToSkating"):
			switch_walking_to_skating();
			break;
			
		case CRCC(0x6e8e39b3, "SkaterPhysicsControl_SwitchSkatingToWalking"):
			switch_skating_to_walking();
			break;
			
		case CRCC(0x9366a509, "SetBoardMissing"):
			m_board_missing = true;
			break;
			
		case CRCC(0x4830e80, "UnsetBoardMissing"):
			m_board_missing = false;
			break;
		
		case CRCC(0xd5a9f889, "IsBoardMissing"):
			return m_board_missing ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
#		ifdef TESTING_GUNSLINGER
		case CRCC(0x14c4f16b,"SkaterPhysicsControl_SwitchWalkingToRiding"):
			switch_walking_to_riding();
			break;

		case CRCC(0x82604c1e,"SkaterPhysicsControl_SwitchRidingToWalking"):
			switch_riding_to_walking();
			break;
#		endif

		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterPhysicsControlComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterPhysicsControlComponent::GetDebugInfo"));
	
	p_info->AddChecksum("m_physics_suspended", m_physics_suspended ? CRCD(0x203b372, "true") : CRCD(0xd43297cf, "false"));
	p_info->AddInteger("m_physics_state", mp_state_component->m_physics_state);

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterPhysicsControlComponent::suspend_physics ( bool suspend )
{
	m_physics_suspended = suspend;
	
	// If the skater is suspended, we don't want to unsuspend the physics components.  We only want to change the CSkaterPhysicsControlComponent state.
	if (!suspend && GetObject()->IsSuspended()) return;
    
	switch (mp_state_component->m_physics_state)
	{
		case SKATING:
			Dbg_Assert(mp_core_physics_component);
			Dbg_Assert(GetSkaterRotateComponentFromObject(GetObject()));
			Dbg_Assert(GetSkaterAdjustPhysicsComponentFromObject(GetObject()));
			Dbg_Assert(GetSkaterFinalizePhysicsComponentFromObject(GetObject()));
			
			mp_core_physics_component->Suspend(suspend);
			GetSkaterRotateComponentFromObject(GetObject())->Suspend(suspend);
			GetSkaterAdjustPhysicsComponentFromObject(GetObject())->Suspend(suspend);
			GetSkaterFinalizePhysicsComponentFromObject(GetObject())->Suspend(suspend);
			break;

		case WALKING:
			Dbg_Assert(GetWalkComponentFromObject(GetObject()));
			
			GetWalkComponentFromObject(GetObject())->Suspend(suspend);
			break;
			
		default:
			Dbg_Assert(false);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterPhysicsControlComponent::switch_walking_to_skating (   )
{
	if (mp_state_component->m_physics_state == SKATING) return;
	
	m_previous_physics_state_duration = Tmr::ElapsedTime(m_physics_state_switch_time_stamp);
	m_physics_state_switch_time_stamp = Tmr::GetTime();
	
	CWalkComponent* p_walk_component = GetWalkComponentFromObject(GetObject());
	CCompositeObject* p_skater_cam = get_skater_camera();
	CSkaterCameraComponent* p_skater_camera_component = GetSkaterCameraComponentFromObject(p_skater_cam);
	CWalkCameraComponent* p_walk_camera_component = GetWalkCameraComponentFromObject(p_skater_cam);
	
	Dbg_Assert(p_walk_component);
	Dbg_Assert(p_skater_camera_component);
	Dbg_Assert(p_walk_camera_component);
	
	// switch off walking
	
	p_walk_component->CleanUpWalkState();
	p_walk_component->Suspend(true);
	
	p_walk_camera_component->Suspend(true);
	
	// switch on skating

	mp_state_component->m_physics_state = SKATING;
	
	mp_core_physics_component->Suspend(false);
	mp_core_physics_component->ReadySkateState(p_walk_component->GetState() == CWalkComponent::WALKING_GROUND, p_walk_component->GetRailData(), p_walk_component->GetAcidDropData());
	
	GetSkaterRotateComponentFromObject(GetObject())->Suspend(false);
	GetSkaterAdjustPhysicsComponentFromObject(GetObject())->Suspend(false);
	GetSkaterFinalizePhysicsComponentFromObject(GetObject())->Suspend(false);
	
	p_skater_camera_component->Suspend(false);
	
	// exchange camera states
	SCameraState camera_state;
	p_walk_camera_component->GetCameraState(camera_state);
	p_skater_camera_component->ReadyForActivation(camera_state);
	
	// reapply the physics suspend state
	if (m_physics_suspended)
	{
		suspend_physics(m_physics_suspended);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterPhysicsControlComponent::switch_skating_to_walking (   )
{
	if (mp_state_component->m_physics_state == WALKING) return;
	
	m_previous_physics_state_duration = Tmr::ElapsedTime(m_physics_state_switch_time_stamp);
	m_physics_state_switch_time_stamp = Tmr::GetTime();
	
	CWalkComponent* p_walk_component = GetWalkComponentFromObject(GetObject());
	CCompositeObject* p_skater_cam = get_skater_camera();
	CSkaterCameraComponent* p_skater_camera_component = GetSkaterCameraComponentFromObject(p_skater_cam);
	CWalkCameraComponent* p_walk_camera_component = GetWalkCameraComponentFromObject(p_skater_cam);
	
	Dbg_Assert(p_walk_component);
	Dbg_Assert(p_skater_camera_component);
	Dbg_Assert(p_walk_camera_component);
	
	// switch off skating
	
	bool ground = mp_core_physics_component->GetState() == GROUND;
	mp_core_physics_component->CleanUpSkateState();
	mp_core_physics_component->Suspend(true);

	GetSkaterRotateComponentFromObject(GetObject())->Suspend(true);
	GetSkaterAdjustPhysicsComponentFromObject(GetObject())->Suspend(true);
	GetSkaterFinalizePhysicsComponentFromObject(GetObject())->Suspend(true);

	p_skater_camera_component->Suspend(true);
	
	// switch on walking
		
	mp_state_component->m_physics_state = WALKING;
	
	p_walk_component->Suspend(false);
	p_walk_component->ReadyWalkState(ground);
	
	p_walk_camera_component->Suspend(false);
	
	// exchange camera states
	SCameraState camera_state;
	p_skater_camera_component->GetCameraState(camera_state);
	p_walk_camera_component->ReadyForActivation(camera_state);
	
	// reapply the physics suspend state
	if (m_physics_suspended)
	{
		suspend_physics(m_physics_suspended);
	}
}


#ifdef TESTING_GUNSLINGER
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkaterPhysicsControlComponent::switch_walking_to_riding( void )
{
	if (mp_state_component->m_physics_state == SKATING) return;
	if (mp_state_component->m_physics_state == RIDING) return;
	
	CRiderComponent*		p_rider_component			= GetRiderComponentFromObject(GetObject());

	// Use the rider component to check that this switch is valid.
	if(	p_rider_component->ReadyRiderState( true ))
	{
		CWalkComponent*			p_walk_component			= GetWalkComponentFromObject(GetObject());
		CCompositeObject*		p_skater_cam				= get_skater_camera();
		CSkaterCameraComponent*	p_skater_camera_component	= GetSkaterCameraComponentFromObject(p_skater_cam);
		CWalkCameraComponent*	p_walk_camera_component		= GetWalkCameraComponentFromObject(p_skater_cam);
	
		Dbg_Assert(p_walk_component);
		Dbg_Assert(p_skater_camera_component);
		Dbg_Assert(p_walk_camera_component);
	
		// switch off walking
		p_walk_component->Suspend(true);
		p_walk_camera_component->Suspend(true);
	
		// switch on riding
		mp_state_component->m_physics_state = RIDING;
	
//		mp_core_physics_component->Suspend(false);
//		mp_core_physics_component->ReadySkateState(p_walk_component->GetState() == CWalkComponent::WALKING_GROUND, p_walk_component->GetRailData());
		p_rider_component->Suspend( false );
	
//		GetSkaterRotateComponentFromObject(GetObject())->Suspend(false);
//		GetSkaterAdjustPhysicsComponentFromObject(GetObject())->Suspend(false);
//		GetSkaterFinalizePhysicsComponentFromObject(GetObject())->Suspend(false);
//		GetSkaterCleanupStateComponentFromObject(GetObject())->Suspend(false);
	
//		p_skater_camera_component->Suspend(false);
	
		// exchange camera states
//		SCameraState camera_state;
//		p_walk_camera_component->GetCameraState(camera_state);
//		p_skater_camera_component->ReadyForActivation(camera_state);
	
		// reapply the physics suspend state
		if (m_physics_suspended)
		{
			suspend_physics(m_physics_suspended);
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkaterPhysicsControlComponent::switch_riding_to_walking( void )
{
//	if (mp_state_component->m_physics_state == WALKING) return;
	
	CWalkComponent*			p_walk_component = GetWalkComponentFromObject(GetObject());
	CRiderComponent*		p_rider_component			= GetRiderComponentFromObject(GetObject());
	CCompositeObject*		p_skater_cam = get_skater_camera();
	CSkaterCameraComponent*	p_skater_camera_component = GetSkaterCameraComponentFromObject(p_skater_cam);
	CWalkCameraComponent*	p_walk_camera_component = GetWalkCameraComponentFromObject(p_skater_cam);
	
	Dbg_Assert(p_walk_component);
	Dbg_Assert(p_skater_camera_component);
	Dbg_Assert(p_walk_camera_component);
	
	// switch off skating
	mp_core_physics_component->Suspend(true);

	GetSkaterRotateComponentFromObject(GetObject())->Suspend(true);
	GetSkaterAdjustPhysicsComponentFromObject(GetObject())->Suspend(true);
	GetSkaterFinalizePhysicsComponentFromObject(GetObject())->Suspend(true);
//	GetSkaterCleanupStateComponentFromObject(GetObject())->Suspend(true);

	p_skater_camera_component->Suspend(true);
	
	// Switch off riding.
	p_rider_component->Suspend( true );

	// switch on walking
	mp_state_component->m_physics_state = WALKING;
	
	p_walk_component->Suspend(false);
	p_walk_component->ReadyWalkState(mp_core_physics_component->GetState() == GROUND);
	p_walk_camera_component->Suspend( false );
	
	// exchange camera states
//	SCameraState camera_state;
//	p_skater_camera_component->GetCameraState( camera_state );
//	p_walk_camera_component->ReadyForActivation( camera_state );
	
	// reapply the physics suspend state
	if (m_physics_suspended)
	{
		suspend_physics(m_physics_suspended);
	}
}



#endif




}
