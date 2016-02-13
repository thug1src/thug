//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       WalkComponent.h
//* OWNER:          Dan
//* CREATION DATE:  4/2/3
//****************************************************************************

#ifndef __COMPONENTS_WALKCOMPONENT_H__
#define __COMPONENTS_WALKCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>
#include <core/timestampedflag.h>

#include <gel/object/basecomponent.h>
#include <gel/object/compositeobject.h>
#include <gel/components/inputcomponent.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

#include <sk/components/skaterphysicscontrolcomponent.h>
#include <sk/engine/feeler.h>

#ifdef __NOPT_ASSERT__
#define WHEADER { printf("%i:%s:%i: ", Tmr::GetRenderFrame(), __FILE__ + 15, __LINE__); }
#define DUMP_WPOSITION { if (Script::GetInteger(CRCD(0x029ade47, "debug_skater_pos"))) { WHEADER; printf("m_pos = %g, %g, %g\n", m_pos[X], m_pos[Y], m_pos[Z]); } }
#else
#define DUMP_WPOSITION {   }
#endif

#define		CRC_WALK CRCD(0x726e85aa, "Walk")

#define		GetWalkComponent() ((Obj::CWalkComponent*)GetComponent(CRC_WALK))
#define		GetWalkComponentFromObject(pObj) ((Obj::CWalkComponent*)(pObj)->GetComponent(CRC_WALK))

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
	class CWalkCameraComponent;
	class CSkaterPhysicsControlComponent;
	class CSkaterStateComponent;
	class CMovableContactComponent;
    class CRailNode;
    class CRailManager;

	struct SHangRailData
	{
		Mth::Vector					hang_point;
		Mth::Vector					hang_facing;
		float						along_rail_factor;
		float						horizontal_hang_offset;
		float						vertical_hang_offset;
		uint32						initial_animation;
		const CRailNode*			p_rail_node;
	};
	
	struct SLadderRailData
	{
		Mth::Vector					climb_point;
		Mth::Vector					climb_facing;
		float						along_rail_factor;
	};
	
class CWalkComponent : public CBaseComponent
{
	friend class CSkaterCorePhysicsComponent;
	
public:
	enum EStateType
	{
		WALKING_GROUND = 0,
		WALKING_AIR,
		// WALKING_HOP,
		WALKING_HANG,
		WALKING_LADDER,
		WALKING_ANIMWAIT,
	};
	
	enum { NUM_WALKING_STATES = 5 };
	
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
		vNUM_BARRIER_HEIGHT_FEELERS = 9
	};
	
	typedef void (Obj::CWalkComponent::*AnimWaitCallbackPtr) (   );
	
private:
	static const uint32 sp_state_names [ NUM_WALKING_STATES ];
	
public:
    CWalkComponent();
    virtual ~CWalkComponent();

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
	
	void							go_on_ground_state (   );
	void							calculate_horizontal_speed_and_facing ( float &horizontal_speed );
	bool							adjust_horizonal_vel_for_environment (   );
	void							adjust_facing_for_adjusted_horizontal_vel (   );
	float							adjust_desired_speed_for_slope ( float desired_speed );
	void							respond_to_ground (   );
	void							adjust_curb_float_height (   );
	void							account_for_movable_contact (   );
	void							jump (   );
	void							adjust_jump_for_ceiling_obstructions (   );
	void							go_in_air_state (   );
	void							control_horizontal_vel (   );
	void							adjust_vertical_vel_for_ceiling (   );
	void							check_for_landing ( const Mth::Vector& previous_pos, const Mth::Vector& final_pos );
	void							maybe_wallplant (   );
	void							uber_frig (   );
	void							lerp_upright (   );
	void							transition_lerp_upright (   );
	bool							check_for_wall_push (   );
	void							leave_movable_contact_for_air ( Mth::Vector& horizontal_vel, float& vertical_vel );
	bool							maybe_hop_to_hang (   );
	bool							maybe_drop_to_hang (   );
	bool							maybe_grab_to_hang ( Mth::Vector start_pos, Mth::Vector end_pos );
	bool							investigate_hang_rail_vicinity ( CFeeler& feeler, const Mth::Vector& preliminary_hang_point, SHangRailData& rail_data );
	bool							vertical_hang_obstruction_check ( CFeeler& feeler, SHangRailData& rail_data, const Mth::Vector& check_offset_direction );
	void							go_hop_state (   );
	void							go_hang_state (   );
	void							hang_movement ( float movement );
	bool							check_environment_for_hang_movement_collisions ( const Mth::Vector& facing, float along_rail_factor, const Mth::Vector& pos, bool left, const Mth::Vector& perp_facing, Mth::Vector rail, const CRailNode* p_rail_start, const CRailNode* p_rail_end );
	void							move_onto_neighboring_hang_rail ( float movement, bool descending, const Mth::Vector& old_rail );
	bool							find_next_rail ( float& along_rail_factor, const CRailNode* p_current_start, const CRailNode* p_current_end, const CRailNode*& p_new_start, const CRailNode*& p_new_end );
	void							set_pos_from_hang_rail_state (   );
	bool							maybe_pull_up_from_hang ( Mth::Vector& proposed_stand_pos );
	bool							maybe_jump_low_barrier (   );
	bool							maybe_in_air_acid_drop (   );
	bool							maybe_jump_to_acid_drop (   );
	bool							maybe_climb_up_ladder ( bool user_requested = false );
	bool							maybe_climb_down_ladder (   );
	bool							maybe_grab_to_ladder ( const Mth::Vector& start_pos, const Mth::Vector& end_pos );
	void							go_ladder_state (   );
	void							ladder_movement (   );
	void							off_ladder_bottom (   );
	void							off_ladder_top (   );
	void							calculate_anim_wait_facing_drift_parameters ( const Mth::Vector& goal_facing, float rotate_factor = 1.0f );
    void                            go_anim_wait_state (   );
    void                            anim_wait_complete (   );
	bool							maybe_stick_to_rail (   );
	void							setup_collision_cache (   );
	void							update_critical_point_offset (   );
	void							update_display_offset (   );
	void							adjust_pos_for_false_wall (   );
	void							extract_state_from_object (   );
	void							copy_state_into_object (   );
	void							get_controller_input (   );
	float							calculate_desired_speed (   );
	Mth::Vector						calculate_feeler_offset_direction ( int contact );
	bool							determine_stand_pos ( const Mth::Vector& proposed_stand_pos, Mth::Vector& stand_pos, CFeeler& feeler );
	float							get_run_speed (   );
	float							get_gravity (   );
	void							update_run_speed (   );
	void							update_run_speed_factor (   );
	void							smooth_anim_speed ( uint32 previous_frame_event );
	void							set_camera_overrides (   );
	void							update_anim_speeds (   );
	float							get_hang_display_offset (   );
	
	void							drop_to_hang_complete (   );
	void							pull_up_from_hang_to_ground_complete (   );
	void							pull_up_from_hang_to_air_complete (   );
	float							get_hang_critical_point_vert_offset (   );
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
	
	void							CollideWithOtherSkaterLost ( CCompositeObject* p_other_skater );
    
	Mth::Vector						GetCameraCollisionTargetOffset (   );
	
	void							ReadyWalkState ( bool to_ground_state );
	void							CleanUpWalkState (   );
	const SRailData*				GetRailData (   );
	const SAcidDropData*			GetAcidDropData (   );
    
	bool							FilterHangRail ( const Mth::Vector& pos_a, const Mth::Vector& pos_b, const Mth::Vector& preliminary_hang_point, const Mth::Vector& grab_from_point, float along_rail_factor, SHangRailData& rail_data, bool drop_to_hang );
	bool							FilterLadderUpRail ( const Mth::Vector& bottom_pos, const Mth::Vector& top_pos, const Mth::Matrix& orientation, SLadderRailData& rail_data );
	bool							FilterLadderDownRail ( const Mth::Vector& bottom_pos, const Mth::Vector& top_pos, const Mth::Matrix& orientation, SLadderRailData& rail_data );
	bool							FilterLadderAirGrabRail ( const Mth::Vector& bottom_pos, const Mth::Vector& top_pos, const Mth::Matrix& orientation, const Mth::Vector& preliminary_climb_point, float along_rail_factor, SLadderRailData& rail_data );
	
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
	bool							m_analog_control;
	
	// The grab button is the same as the transition button, and is thus ignored after a transition until it is released
	bool							m_ignore_grab_button;
	
	// After transitioning from skating to walking in air, you are not allowed to acid drop until you first switch out of the air state.
	bool							m_disallow_acid_drops;
	
	// The offset from the skater's true origin at which to display the skater.  Used to get CAS scaled skaters looking correct.
	float							m_display_offset;
	
	struct SContact
	{
		// If this contact is in collision with the environment.
		bool						in_collision;
		
		// The horizontal normal of the collided geometry.
		Mth::Vector					normal;
		
		// If the collision in this direction is determined to be low enough to jump.
		bool						jumpable;
		
		// Movement of the object we're in contact with (not accounting for its rotation) along its horizontal normal.
		float						movement;
		
		// Must store the full feeler in order to trip triggers and get movable contact objects
		CFeeler						feeler;
		
	// The walker looks around him in vNUM_FEELERS directions and stores the results here.  The final contact is felt for along the movement of
	// the origin, and is only used while in the air.
	} mp_contacts [ vNUM_FEELERS + 1 ];
	
	// Type of animation speed scaling to do.
	EAnimScaleSpeed					m_anim_scale_speed;
	
	// Horizontal speed for the purposes of animation speed scaling.
	float							m_anim_effective_speed;
	float							m_last_frame_anim_effective_speed;
	
	#ifdef SCRIPT_WALK_DRAG
	// Factor multiplied by all velocities.  Set by script.
	float							m_script_drag_factor;
	#endif
	
	// Used to lock control in the forward direction (during a camera flush request)
	bool							m_forward_control_lock;
	
	// When on ground, the walker is often floated above the ground in an attempt to smooth out stair/curb climbs.
	float							m_curb_float_height;
	
	// If m_curb_float_height was adjusted this frame.  If not, we lerp it to zero.
	bool							m_curb_float_height_adjusted;
	
	// The critical point is a position relative to the skater which MUST ALWAYS STAY ON THE CORRECT SIDE OF COLLIDABLE GEO.  Normally, this is simply the
	// origin.  However, sometimes this must be adjusted (during hanging).  Once hanging is over, the critical point is pushed back to the origin.
	Mth::Vector						m_critical_point_offset;
	
	// Counts down while we're slerping upright after a transition from skating
	float m_rotate_upright_timer;
	
	// Angle over and axis around which to slerp to upright
	float m_rotate_upright_angle;
	Mth::Vector m_rotate_upright_axis;
	
	// The matrix's [Z][Y] component last time facing was extracted from the matrix.  Stored so it can be restored when rebuilding the matrix, thus
	// preventing unwanted rotations.
	float m_frame_initial_facing_Y;
	
	struct SSpecialTransitionData
	{
		// The type of transition causing the switch to skating.
		ESpecialTransitionType		type;
		
		// The state data for that transition.
		SRailData					rail_data;
		SAcidDropData				acid_drop_data;
		
	// When the walker transitions to skating in some manner which requires additional state information, this structure holds that information before it
	// is passed off to the skate physics.
	}								m_special_transition_data;
	
	// The collision cache used.
	Nx::CCollCache*					mp_collision_cache;
	
	// Associated camera.
	CCompositeObject*				mp_camera;
	CWalkCameraComponent*			mp_camera_component;
	
    // If associated with a rail, this is the rail node and rail manager.
    const CRailNode*              	mp_rail_start;
    const CRailNode*              	mp_rail_end;
    CRailManager*                   mp_rail_manager;
	
	// Peer components.
	CInputComponent*				mp_input_component;
	CAnimationComponent*			mp_animation_component;
	CModelComponent*				mp_model_component;
	CTriggerComponent*				mp_trigger_component;
	CSkaterPhysicsControlComponent*	mp_physics_control_component;
	CMovableContactComponent*		mp_movable_contact_component;
	CSkaterStateComponent*			mp_state_component;
	CSkaterCorePhysicsComponent*	mp_core_physics_component;
    
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
	
	// lerp factor between standard run speed and combo run speed
	float							m_run_speed_factor;
	
	// If true, no transitional period of reduced deceleration is used from skating.
	// bool							m_cancel_transition_momentum;
	
	// Set to true when the walker's speeds is significantly reduced due to interactions with walls.  Causes the rotation rate of the skater to be dropped
	// to walking levels next frame.  A beter solution would be to reduce rotation rate THIS frame.
	bool							m_drop_rotation_rate;
	
	// Latest duration over which the controller has been continuously pegged.  Only used on the ground and reset to a large value whenever you enter ground.
	float							m_control_pegged_duration;
	
	// True if the controller was pegged last frame.  Only updated while on the ground.
	bool							m_last_frame_controller_pegged;
	
	/////////////////////////////////
	// WALKING_AIR state variables
	
	// The forward horizontal direction of the jump which, when coming out of skating or because of in-air velocity control, diverges from the direction of velocity
	Mth::Vector						m_primary_air_direction;
	
	// This term of our horizontal velocity is due to leaping from a movable contact, and is not adjustable via in-air control.
	Mth::Vector						m_uncontrollable_air_horizontal_vel;
	
	// When non-zero, it counts down and suppresses in air velocity control.
	float							m_in_air_control_suppression_timer;
	
	// Extra drift velocity used while in the air.
	Mth::Vector						m_in_air_drift_vel;
	
	// Extra drift is turned off after the skater reaches this height.
	float							m_in_air_drift_stop_height;
	
	struct SFalseWall
	{
		// This tracks if a false wall is active.
		bool						active;
		
		// The false wall is defined by a normal.
		Mth::Vector					normal;
		
		// And a scalar.
		float						distance;
		
		// The false wall is automatically turned off when this height is exceeded.
		float						cancel_height;
		
	// A false wall is sometimes set up while in the air to prevent movement in a certain direction.
	}								m_false_wall;
	
	/////////////////////////////////
	// WALKING_HANG state variables
	
	// Measures our position along the current rail as a parameter running from 0.0 to 1.0.
	float							m_along_rail_factor;
	
	// Measures the hang position offset from the actual rail we are hanging from.
	float							m_vertical_hang_offset;
	float							m_horizontal_hang_offset;
	
	// Checksum describing the type of initial hang animation to play.
	uint32							m_initial_hang_animation;
	
	// Speed the skater is shimmying at
	float							m_hang_move_vel;
    
	/////////////////////////////////
	// WALKING_HOP state variables
	
    // The position we will have at the end of the hop.
    // Mth::Vector                     m_goal_hop_pos;
    
    // The facing we will have at the end of the hop.
    // Mth::Vector                     m_goal_hop_facing;
	
	// Hang state variable.  Set at hop start.
	// float						m_along_rail_factor;
	
	// Hang state variable.  Set at hop start.
	// uint32						m_initial_hang_animation;
    
	/////////////////////////////////
	// WALKING_LADDER state variables
	
	// Shared with hang state.
	// float						m_along_rail_factor;
	
	// Only ladder animation speed changes are sent to the server are updated in net games.  This holds the last animimation speed sent.
	float							m_last_ladder_anim_speed;
    
	/////////////////////////////////
	// WALKING_ANIMWAIT state variables
	
	// Position at the start of the animation wait.
	Mth::Vector						m_anim_wait_initial_pos;
	
	// Facing at the start of the aniamtion wait.
	Mth::Vector						m_anim_wait_initial_facing;
	
    // Amount to move the object at the end of the animation wait.
    Mth::Vector                     m_anim_wait_goal_pos;
	
	// Goal display offset of the animation wait.
	float							m_drift_initial_display_offset;
	float							m_drift_goal_display_offset;
	
	// Goal facing of the position drift.
	Mth::Vector						m_drift_goal_facing;
	
	// Factor of animation in which to do the rotation in.
	float							m_drift_goal_rotate_factor;
	
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
	
	// WALKING_AIR state variable partially setup at the start of an animation wait
	// struct SFalseWall			m_false_wall;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float CWalkComponent::s_get_param ( uint32 checksum )
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

inline float CWalkComponent::get_run_speed (   )
{
	return Mth::Lerp(s_get_param(CRCD(0xcc461b87, "run_speed")), s_get_param(CRCD(0x7213ef07, "combo_run_speed")), m_run_speed_factor);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CWalkComponent::IsRunning (   )
{
	return m_run_toggle;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CWalkComponent::SetForwardControlLock ( bool forward_control_lock )
{
	m_forward_control_lock = forward_control_lock;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const SRailData* CWalkComponent::GetRailData (   )
{
	return m_special_transition_data.type == RAIL_TRANSITION ? &m_special_transition_data.rail_data : NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const SAcidDropData* CWalkComponent::GetAcidDropData (   )
{
	return m_special_transition_data.type == ACID_DROP_TRANSITION ? &m_special_transition_data.acid_drop_data : NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CWalkComponent::UseDPadCamera (   )
{
	
	return mp_input_component->GetControlPad().m_leftX == 0.0f && mp_input_component->GetControlPad().m_leftY == 0.0f && m_control_pegged;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Mth::Vector CWalkComponent::GetCameraCollisionTargetOffset (   )
{
	switch (m_state)
	{
		case WALKING_HANG:
		case WALKING_LADDER:
		case WALKING_ANIMWAIT:
			return -18.0f * m_facing;
		
		case WALKING_AIR:
			if ((m_previous_state == WALKING_HANG || m_previous_state ==WALKING_LADDER || m_previous_state ==WALKING_LADDER)
				&& Tmr::ElapsedTime(m_state_timestamp) < 150)
			{
				return -18.0f * m_facing;
			}
			else
			{
				return Mth::Vector(0.0f, 0.0f, 0.0f);
			}
			
		case WALKING_GROUND:
		default:
			return Mth::Vector(0.0f, 0.0f, 0.0f);
	}
}

}

#endif

