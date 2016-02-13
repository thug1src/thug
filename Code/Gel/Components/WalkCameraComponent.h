//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       WalkCameraComponent.h
//* OWNER:          Dan
//* CREATION DATE:  4/9/3
//****************************************************************************

#ifndef __COMPONENTS_WALKCAMERACOMPONENT_H__
#define __COMPONENTS_WALKCAMERACOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>
#include <gel/components/camerautil.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

#define		CRC_WALKCAMERA CRCD(0xc7d37ec2, "WalkCamera")

#define		GetWalkCameraComponent() ((Obj::CWalkCameraComponent*)GetComponent(CRC_WALKCAMERA))
#define		GetWalkCameraComponentFromObject(pObj) ((Obj::CWalkCameraComponent*)(pObj)->GetComponent(CRC_WALKCAMERA))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CWalkComponent;
	class CCameraLookAroundComponent;
	class CCameraComponent;
	class CSkaterCameraComponent;
	class CSkaterPhysicsControlComponent;
	
class CWalkCameraComponent : public CBaseComponent
{
public:
    CWalkCameraComponent();
    virtual ~CWalkCameraComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    virtual void            		Finalize();
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	void							ReadyForActivation ( const SCameraState& state );
	void							GetCameraState ( SCameraState& state );
	void							SetOverrides ( const Mth::Vector& facing, float slerp_rate );
	void							UnsetOverrides (   );
	
	void							FlushRequest (   );
	void							Reset (   );
	
	void							SetSkater ( CCompositeObject* p_skater );
	
private:
	Mth::Vector						get_tripod_pos( bool instantly );
	void							calculate_zoom ( float& above, float& behind );
	
	void							set_target ( CCompositeObject* p_target );
	
	float							s_get_param ( uint32 checksum );
	
private:
	int								m_instant_count;
	bool							m_flush_request_active;
	bool							m_reset;
	
	float							m_current_zoom;
	
	Mth::Vector						m_last_tripod_pos;
	Mth::Matrix						m_last_actual_matrix;
	float							m_last_dot;
	
	bool							m_override_active;
	Mth::Vector						m_override_facing;
	float							m_override_slerp_rate;
	
	CCameraLookAroundComponent*		mp_lookaround_component;
	CCameraComponent*				mp_camera_component;
	CSkaterCameraComponent*			mp_skater_camera_component;
	
	CCompositeObject*				mp_target;
	CWalkComponent*					mp_target_walk_component;
	CSkaterPhysicsControlComponent*	mp_target_physics_control_component;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float CWalkCameraComponent::s_get_param ( uint32 checksum )
{
	Script::CStruct* p_walk_params = Script::GetStructure(CRCD(0x1cda02ae, "walk_camera_parameters"));
	Dbg_Assert(p_walk_params);
	
	float param;
	p_walk_params->GetFloat(checksum, &param, Script::ASSERT);
	return param;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CWalkCameraComponent::SetOverrides ( const Mth::Vector& facing, float slerp_rate )
{
	m_override_active = true;
	m_override_facing = facing;
	m_override_slerp_rate = slerp_rate;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CWalkCameraComponent::UnsetOverrides (   )
{
	m_override_active = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CWalkCameraComponent::SetSkater ( CCompositeObject* p_skater )
{
	set_target(p_skater);
	m_instant_count = 3;
}

}

#endif
