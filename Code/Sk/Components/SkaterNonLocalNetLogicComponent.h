//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterNonLocalNetLogicComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/11/3
//****************************************************************************

#ifndef __COMPONENTS_SKATERNONLOCALNETLOGICCOMPONENT_H__
#define __COMPONENTS_SKATERNONLOCALNETLOGICCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#include <sk/objects/skater.h>

#define		CRC_SKATERNONLOCALNETLOGIC CRCD(0x850eef66, "SkaterNonLocalNetLogic")

#define		GetSkaterNonLocalNetLogicComponent() ((Obj::CSkaterNonLocalNetLogicComponent*)GetComponent(CRC_SKATERNONLOCALNETLOGIC))
#define		GetSkaterNonLocalNetLogicComponentFromObject(pObj) ((Obj::CSkaterNonLocalNetLogicComponent*)(pObj)->GetComponent(CRC_SKATERNONLOCALNETLOGIC))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CSkaterStateHistoryComponent;
	class CSkaterCorePhysicsComponent;
	class CSkaterStateComponent;
	class CSkaterEndRunComponent;
	class CSkaterFlipAndRotateComponent;
	class CShadowComponent;
	class CRailNode;
	
enum
{
	vMAG_HISTORY_LENGTH = 20
};

class CSkaterNonLocalNetLogicComponent : public CBaseComponent
{
public:
    CSkaterNonLocalNetLogicComponent();
    virtual ~CSkaterNonLocalNetLogicComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    virtual void            		Finalize();
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	void							Resync();
	
	CSkater*						GetSkater() { return static_cast< CSkater* >(GetObject()); }
	
private:
	void							setup_brightness_and_shadow (   );
	void							interpolate_client_position (   );
	void							extrapolate_client_position (   );
	void							extrapolate_rail_position( void );
	CRailNode*						travel_on_rail( CRailNode* start_node, float frame_length );
	float 							rotate_away_from_wall( Mth::Vector normal, float &turn_angle );
	void 							bounce_off_wall( const Mth::Vector& normal, const Mth::Vector& point );
	void 							snap_to_ground( void  );
	void							extrapolate_position( void );
	void							do_client_animation_update (   );
	
private:
 	Tmr::Time						m_client_initial_update_time;
	//Tmr::Time						m_client_initial_anm_update_time;
	Tmr::Time						m_server_initial_update_time;
	//Tmr::Time						m_server_initial_anm_update_time;
	Tmr::Time						m_last_anm_update_time;
	int								m_last_pos_index;
	
	Mth::Vector						m_interp_pos;
	Mth::Vector						m_old_interp_pos;
	Mth::Vector						m_old_extrap_pos;
	Mth::Vector						m_current_normal;
	float							m_mag_history[vMAG_HISTORY_LENGTH];
	float							m_ratio_history[vMAG_HISTORY_LENGTH];
	float							m_extrap_coeff_history[vMAG_HISTORY_LENGTH];
	Mth::Vector						m_vel_history[vMAG_HISTORY_LENGTH];
	int								m_num_mags;
	
	float							m_frame_length;
	
	bool							m_last_driving;
	
	CSkaterStateHistoryComponent*	mp_state_history_component;
	CAnimationComponent*			mp_animation_component;
	CModelComponent*				mp_model_component;
	CSkaterStateComponent*			mp_state_component;
	CSkaterEndRunComponent*			mp_endrun_component;
	CSkaterFlipAndRotateComponent*	mp_flip_and_rotate_component;
    CShadowComponent*				mp_shadow_component;
};

}

#endif
