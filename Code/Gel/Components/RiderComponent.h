//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       RiderComponent.h
//* OWNER:          Dave
//* CREATION DATE:  5/30/03
//****************************************************************************

#ifndef __COMPONENTS_RIDERCOMPONENT_H__
#define __COMPONENTS_RIDERCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>
#include <core/timestampedflag.h>

#include <gel/object/basecomponent.h>
#include <gel/object/compositeobject.h>
#include <gel/components/inputcomponent.h>
#include <gel/components/horsecomponent.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

#include <sk/components/skaterphysicscontrolcomponent.h>
#include <sk/engine/feeler.h>

#define		CRC_RIDER CRCD(0x15beefca,"Rider")

#define		GetRiderComponent()					((Obj::CRiderComponent*)GetComponent(CRC_RIDER))
#define		GetRiderComponentFromObject(pObj)	((Obj::CRiderComponent*)(pObj)->GetComponent(CRC_RIDER))

class CFeeler;
		
namespace Script
{
    class CScript;
    class CStruct;
}

namespace Nx
{
	class CCollCache;
}
              
namespace Obj
{
	class CAnimationComponent;
	class CModelComponent;
	class CTriggerComponent;
	class CRiderCameraComponent;
	class CSkaterPhysicsControlComponent;
	class CMovableContactComponent;
	
class CRiderComponent : public CBaseComponent
{
public:
	enum EStateType
	{
		WALKING_GROUND,
		WALKING_AIR,
		WALKING_HOP,
		WALKING_HANG,
		WALKING_LADDER,
		WALKING_ANIMWAIT
	};
	
	enum EAnimScaleSpeed
	{
		OFF,
		RUNNING,
        HANGMOVE,
		LADDERMOVE
	};
	
	enum
	{
		vNUM_FEELERS = 7,
		vNUM_BARRIER_HEIGHT_FEELERS = 7
	};
	
	typedef void (Obj::CRiderComponent::*AnimWaitCallbackPtr) (   );
	
public:
    CRiderComponent();
    virtual ~CRiderComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
	virtual void					Finalize();
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	EStateType						GetState (   ) { return m_state; }
	
private:
	void							set_state ( EStateType state );
	
	void							go_on_horse_state( void );

	void							go_on_ground_state (   );
	void							calculate_horizontal_speed_and_facing ( float &horizontal_speed );
	bool							adjust_horizonal_vel_for_environment ( bool wall_push_active );
	void							adjust_facing_for_adjusted_horizontal_vel (   );
	float							adjust_desired_speed_for_slope ( float desired_speed );
	void							respond_to_ground (   );
	void							adjust_curb_float_height (   );
	void							account_for_movable_contact (   );
	void							jump ( float strength, bool hang_jump = false );
	void							go_in_air_state (   );
	void							control_horizontal_vel (   );
	void							adjust_vertical_vel_for_ceiling (   );
	void							check_for_landing ( const Mth::Vector& previous_pos, const Mth::Vector& final_pos );
	void							uber_frig (   );
	void							lerp_upright (   );
	bool							check_for_wall_push (   );
	void							leave_movable_contact_for_air ( Mth::Vector& horizontal_vel, float& vertical_vel );
	bool							maybe_hop_to_hang (   );
	bool							maybe_drop_to_hang (   );
	bool							maybe_grab_to_hang ( Mth::Vector start_pos, Mth::Vector end_pos );
	void							go_hop_state (   );
	void							go_hang_state (   );
	void							hang_movement (   );
	void							find_neighboring_hang_rail ( float movement, bool descending );
	void							set_pos_from_hang_rail_state (   );
	bool							maybe_pull_up_from_hang (   );
	bool							maybe_jump_low_barrier (   );
	bool							maybe_climb_up_ladder ( bool user_requested = false );
	bool							maybe_climb_down_ladder (   );
	bool							maybe_grab_to_ladder ( const Mth::Vector& start_pos, const Mth::Vector& end_pos );
	void							go_ladder_state (   );
	void							ladder_movement (   );
	void							off_ladder_bottom (   );
	void							off_ladder_top (   );
	void							calculate_anim_wait_facing_drift_parameters ( const Mth::Vector& goal_facing );
    void                            go_anim_wait_state (   );
    void                            anim_wait_complete (   );
	bool							maybe_stick_to_rail (   );
	void							setup_collision_cache (   );
	void							copy_state_into_object (   );
	void							get_controller_input (   );
	void							adjust_control_for_forced_run (   );
	float							calculate_desired_speed (   );
	Mth::Vector						calculate_feeler_offset_direction ( int contact );
	bool							forced_run (   );
	bool							determine_stand_pos ( const Mth::Vector& proposed_stand_pos, Mth::Vector& stand_pos, CFeeler& feeler );
	bool							should_bail_from_frame (   );
	
	void							drop_to_hang_complete (   );
	void							pull_up_from_hang_complete (   );
	void							move_to_ladder_bottom_complete (   );
	void							move_to_ladder_top_complete (   );
	void							off_ladder_bottom_complete (   );
	void							off_ladder_top_to_ground_complete (   );
	void							off_ladder_top_to_air_complete (   );
	
	static float					s_get_param ( uint32 checksum );
	
public:
	bool							IsRunning (   );
	bool							UseDPadCamera (   );
	void							SetForwardControlLock ( bool forward_control_lock );
	void							SetAssociatedCamera ( CCompositeObject* camera_obj );
    
    const Mth::Vector&              GetEffectivePos (   );
	
	bool							ReadyRiderState ( bool to_ground_state );
	
private:
	EStateType						m_state;
	
	// Time stamp of when our state last changed.
	Tmr::Time						m_state_timestamp;
	
	// State we changed from when m_state_timestamp was last set.
	EStateType						m_previous_state;
	
	// Extracted from the object at frame start and back into the object at frame end.
	Mth::Vector						m_pos;
	Mth::Vector						m_horizontal_vel;
	float							m_vertical_vel;
	Mth::Vector						m_facing;
	Mth::Vector						m_upward;
	
	// Cache our position at the frame's start
	Mth::Vector						m_frame_start_pos;
	
	// Script event reporting our current state.  Set during each frame update and sent at the end of the update.
	uint32							m_frame_event;
	
	// Last frame's frame event.
	uint32							m_last_frame_event;
	
	// Current frame's frame length.
	float							m_frame_length;
	
	// Current frame's control input.
	Mth::Vector						m_control_direction;
	float							m_control_magnitude;
	bool							m_control_pegged;
	
	struct SContact
	{
		// If this contact is in collision with the environment.
		bool						in_collision;
		
		// The horizontal normal of the collided geometry.
		Mth::Vector					normal;
		
		// Distance from the collision as a factor of the feeler length.
		float						distance;
		
		// If the collision in this direction is determined to be low enough to jump.
		bool						jumpable;
		
		// Movement of the object we're in contact with (not accounting for its rotation) along its horizontal normal.
		float						movement;
		
	// The walker looks around him in vNUM_FEELERS directions and stores the results here.  The final contact is felt for along the movement of
	// the origin, and is only used while in the air.
	} mp_contacts [ vNUM_FEELERS + 1 ];
	
	// Type of animation speed scaling to do.
	EAnimScaleSpeed					m_anim_scale_speed;
	
	// Horizontal speed for the purposes of animation speed scaling.
	float							m_anim_effective_speed;
	
	// Factor multiplied by all velocities.  Set by script.
	float							m_script_drag_factor;
	
	// Used to lock control in the forward direction (during a camera flush request)
	bool							m_forward_control_lock;
	
	// When on ground, the walker is often floated above the ground in an attempt to smooth out stair/curb climbs.
	float							m_curb_float_height;
	
	// If m_curb_float_height was adjusted this frame.  If not, we lerp it to zero.
	bool							m_curb_float_height_adjusted;
	
	// The collision cache used.
	Nx::CCollCache*					mp_collision_cache;
	
	// Associated camera.
	CCompositeObject*				mp_camera;
	CRiderCameraComponent*			mp_camera_component;
	CHorseComponent*				mp_horse_component;		// This points to the CHorseComponent in the separate CCompositeObject that is the horse being ridden.
	
    // If associated with a rail, this is the rail node and rail manager.
    const CRailNode*              	mp_rail_node;
    CRailManager*                   mp_rail_manager;
	
	// Peer components.
	CInputComponent*				mp_input_component;
	CAnimationComponent*			mp_animation_component;
	CModelComponent*				mp_model_component;
	CTriggerComponent*				mp_trigger_component;
	CSkaterPhysicsControlComponent*	mp_physics_control_component;
	CMovableContactComponent*		mp_movable_contact_component;
    
	/////////////////////////////////
	// WALKING_GROUND state variables
	
	// Normal of the ground on which we are standing.  Only meaningful when on the ground.  Set the previous frame.
	Mth::Vector						m_ground_normal;
	
	// Certain things such as rotation rate and camera lerp are increased when this is set.
	bool							m_run_toggle;
	
	struct SWallPushTest
	{
		// Whether we currently testing.
		bool						active;
		
		// Time at which this test began.
		Tmr::Time					test_start_time;
		
	// Data used to detect when a player is pushing into a wall in order to climb it.
	}								m_wall_push_test;
	
	// When doing anim speed scaling when running, this is set to the run speed which corresponds to the current animation playing at standard speed.
	float							m_anim_standard_speed;
	
	// A copy of the latest feeler which contacted the groud under us.
	CFeeler							m_last_ground_feeler;
	bool							m_last_ground_feeler_valid;
	
	/////////////////////////////////
	// WALKING_AIR state variables
	
	// The forward horizontal direction of the jump which, when coming out of skating or because of in-air velocity control, diverges from the direction of velocity
	Mth::Vector						m_primary_air_direction;
	
	// This term of our horizontal velocity is due to leaping from a movable contact, and is not adjustable via in-air control.
	Mth::Vector						m_uncontrollable_air_horizontal_vel;
	
	/////////////////////////////////
	// WALKING_HANG state variables
	
	// Measures our position along the current rail as a parameter running from 0.0 to 1.0.
	float							m_along_rail_factor;
	
	// Measures the hang position offset from the actual rail we are hanging from.
	float							m_vertical_hang_offset;
	float							m_horizontal_hang_offset;
	
	// Checksum describing the type of initial hang animation to play.
	uint32							m_initial_hang_animation;
    
	/*
	/////////////////////////////////
	// WALKING_HOP state variables
	
    // The position we will have at the end of the hop.
    Mth::Vector                     m_goal_hop_pos;
    
    // The facing we will have at the end of the hop.
    Mth::Vector                     m_goal_hop_facing;
	
	// Hang state variable.  Set at hop start.
	// float						m_along_rail_factor;
	
	// Hang state variable.  Set at hop start.
	// uint32							m_initial_hang_animation;
	*/
    
	/////////////////////////////////
	// WALKING_LADDER state variables
	
	// Shared with hang state.
	// float						m_along_rail_factor;
    
	/////////////////////////////////
	// WALKING_ANIMWAIT state variables
	
	// Position at the start of the animation wait.
	Mth::Vector						m_anim_wait_initial_pos;
	
	// Facing at the start of the aniamtion wait.
	Mth::Vector						m_anim_wait_initial_facing;
	
    // Amount to move the object at the end of the animation wait.
    Mth::Vector                     m_anim_wait_goal_pos;
	
	// Goal facing of the position drift.
	Mth::Vector						m_drift_goal_facing;
	
	// The angle over which the facing must drift.
	float							m_drift_angle;
    
	// Accumulates position offset due to a movable contact during an animation wait.
	Mth::Vector						m_offset_due_to_movable_contact;
	
	// The behavior of the camera during the animation wait.
	enum EAnimWaitCameraModeType			
	{
		AWC_CURRENT,
		AWC_GOAL
	}								m_anim_wait_camera_mode;
	
	// Callback function to call upon completion of the animation wait.
	AnimWaitCallbackPtr				mp_anim_wait_complete_callback;
	
	// WALKING_GROUND state variable sometimes set at start of animation wait based on the animation end ground situation
	// Mth::Vector					m_ground_normal;
	// CFeeler						m_last_ground_feeler;
	// bool							m_last_ground_feeler_valid;
	
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CRiderComponent::set_state ( EStateType state )
{
    if (state == m_state) return;
	
	m_previous_state = m_state;
    
    if (m_state == WALKING_GROUND)
    {
        m_wall_push_test.active = false;
		m_run_toggle = false;
    }
	
	m_state = state;
	m_state_timestamp = Tmr::GetTime();
	
	if (m_state == WALKING_ANIMWAIT)
	{
		m_offset_due_to_movable_contact.Set();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float CRiderComponent::s_get_param ( uint32 checksum )
{
	Script::CStruct* p_walk_params = Script::GetStructure(CRCD(0x6775e538, "walk_parameters"));
	Dbg_Assert(p_walk_params);
	
	float param;
	p_walk_params->GetFloat(checksum, &param, Script::ASSERT);
	return param;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CRiderComponent::forced_run (   )
{
	return false;
	if (!mp_input_component->GetControlPad().m_x.GetPressed()) return false;
	
	uint32 pressed_time = mp_input_component->GetControlPad().m_x.GetPressedTime();
	return pressed_time > m_state_timestamp || pressed_time > s_get_param(CRCD(0xa7f5ba13, "forced_run_delay"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CRiderComponent::should_bail_from_frame (   )
{
	// for now, all restarts switch us to skating; so, only bail from frame if we're ever switch out of walking via script
//	return !mp_physics_control_component->IsWalking();
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CRiderComponent::IsRunning (   )
{
	return m_run_toggle;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CRiderComponent::SetForwardControlLock ( bool forward_control_lock )
{
	m_forward_control_lock = forward_control_lock;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const Mth::Vector& CRiderComponent::GetEffectivePos (   )
{
    if (m_state != WALKING_ANIMWAIT)
    {
        return GetObject()->GetPos();
    }
    else
    {
        return GetObject()->GetPos();
    }
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CRiderComponent::UseDPadCamera (   )
{
	
	return mp_input_component->GetControlPad().m_leftX == 0.0f && mp_input_component->GetControlPad().m_leftY == 0.0f && m_control_pegged;
}

}

#endif
