//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       HorseComponent.h
//* OWNER:          ???
//* CREATION DATE:  ??/??/??
//****************************************************************************

#ifndef __COMPONENTS_HORSECOMPONENT_H__
#define __COMPONENTS_HORSECOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/collision/collcache.h>
#include <gel/components/skeletoncomponent.h>
#include <gel/components/modelcomponent.h>
#include <gel/components/animationcomponent.h>
#include <gel/components/movablecontactcomponent.h>
#include <gel/object/basecomponent.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

#include <sk/engine/rectfeeler.h>
#include <sk/engine/feeler.h>
#include <sk/objects/navigation.h>

#define		CRC_HORSE CRCD(0x9d65d0e7,"Horse")
#define		GetHorseComponent() ((Obj::CHorseComponent*)GetComponent(CRC_HORSE))
#define		GetHorseComponentFromObject(pObj) ((Obj::CHorseComponent*)(pObj)->GetComponent(CRC_HORSE))
		 
namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CInputComponent;

#define vVP_GRAVITATIONAL_ACCELERATION								(386.4f)
#define vVP_MAX_NUM_GEARS											(6)
#define vVP_NUM_COLLIDERS											(2)

class CHorseComponent : public CBaseComponent
{
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

	// inorder to interface with models and skeletons, the number of wheels must be hardcoded to four
	enum { vVP_NUM_WHEELS = 4 };
	
	// HACK for control
	struct SControls {
		// steering; between -1.0 and 1.0
		float steering;
		
		// Spurring the horse on, or reining it in, values in range [-1.0, 1.0].
		float spur_rein;

		// throttle
		bool throttle;
		
		// brake
		bool brake;
		
		// handbrake
		bool handbrake;
		
		// reverse
		bool reverse;
	};
	
	
	struct SCollisionPoint
	{
		// world position
		Mth::Vector pos;
		
		// impact normal
		Mth::Vector normal;
		
		// depth of collision as defined as the dot between the displacement from the nearest collider corner and the collision normal
		float depth;
		
		// normal impulse accumulator; used to set the maximum friction
		float normal_impulse;
		
		bool line;
	};
	
	struct SCollider
	{
		// the rectangle defining the collider in body space
		Mth::Rectangle body;
		
		// that rectangle transformed into world space
		Mth::Rectangle world;
		Mth::Vector first_edge_direction_world;
		Mth::Vector second_edge_direction_world;
		
		// a few precomputable values
		float first_edge_length;
		float second_edge_length;
	};
	
public:
    CHorseComponent();
    virtual ~CHorseComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
	virtual void					Finalize (   );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	void							MoveToNode ( Script::CStruct* p_node );
	
	const Mth::Vector&				GetPos( void )						{ return GetObject()->GetPos(); }
	const Mth::Vector&				GetVel( void )				const	{ return m_vel; }

	Mth::Matrix&					GetMatrix( void )					{ return GetObject()->GetMatrix(); }
	Mth::Matrix&					GetDisplayMatrix( void )			{ return GetObject()->GetDisplayMatrix(); }

	bool							AcceptRiderMount( CCompositeObject*	p_rider );
	bool							AcceptRiderDismount( CCompositeObject*	p_rider );
	bool							ShouldUpdateCamera( void );
	CCompositeObject*				GetRider( void )					{ return mp_rider; }

	uint32							GetAnimation( void );

private:
	
    void							draw_debug_rendering (   ) const;
	
private:
	
	Obj::CNavNode**					mp_nav_nodes;			// List of pointers to navigation nodes if following a path.
	int								m_nav_node_index;		// Current index within the list (moves to zero as path progesses).

	// dynamic state variables
	
	// dependent state variables
	
	// angular orientation in 3x3 matrix form
	Mth::Matrix m_orientation_matrix;
	
	// linear velocity
	Mth::Vector m_vel;
	
	// rotational velocity
	Mth::Vector m_rotvel;
	
	// accumulators
	
	
	// length of current frame
	float m_time_step;
	
	// constant characteristics
	
	
	// subelements
	
	// input state
//	SControls m_controls;
	
	// collision cache
	Nx::CCollCache m_collision_cache;
	
	
	// shared objects
	
	// shared collision point array
	static SCollisionPoint sp_collision_points[4 * (Nx::MAX_NUM_2D_COLLISIONS_REPORTED + 1)];
	
	CCompositeObject*				mp_rider;

	// peer components
	CSkeletonComponent*				mp_skeleton_component;
	CInputComponent*				mp_input_component;
	CModelComponent*				mp_model_component;
	CAnimationComponent*			mp_animation_component;
	CMovableContactComponent*		mp_movable_contact_component;
	
	// debug
	int m_draw_debug_lines;

	// Stuff from CWalkComponent.

	static float					s_get_param ( uint32 checksum );


	enum
	{
		vNUM_FEELERS = 7,
		vNUM_BARRIER_HEIGHT_FEELERS = 7
	};

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

	/////////////////////////////////
	// WALKING_AIR state variables
	
	// The forward horizontal direction of the jump which, when coming out of skating or because of in-air velocity control, diverges from the direction of velocity
	Mth::Vector						m_primary_air_direction;
	
	// This term of our horizontal velocity is due to leaping from a movable contact, and is not adjustable via in-air control.
	Mth::Vector						m_uncontrollable_air_horizontal_vel;

	void							get_controller_input( void );
	void							go_on_ground_state( void );
	void							go_in_air_state( void );
	void							lerp_upright( void );
	void							uber_frig( void );
	void							account_for_movable_contact( void );
	void							setup_collision_cache( void );
	float							calculate_desired_speed( void );
	float							adjust_desired_speed_for_slope( float desired_speed );
	bool							adjust_horizontal_vel_for_environment( bool wall_push_active );
	void							copy_state_into_object( void );
	void							adjust_facing_for_adjusted_horizontal_vel( void );
	void							calculate_horizontal_speed_and_facing( float &horizontal_speed );
	Mth::Vector						calculate_feeler_offset_direction ( int contact );
	void							adjust_curb_float_height( void );
	bool							forced_run( void );
	void							respond_to_ground( void );
	void							leave_movable_contact_for_air( Mth::Vector& horizontal_vel, float& vertical_vel );
	void							jump( float strength );
	void							set_state( EStateType state );
	void							control_horizontal_vel( void );
	void							check_for_landing( const Mth::Vector& previous_pos, const Mth::Vector& final_pos );
	void							adjust_vertical_vel_for_ceiling( void );
	void							position_rider( void );

	void							do_path_following( void );
	void							switch_to_node_based_path_following( void );
	void							cleanup_node_based_path_following( void );
	void							do_node_based_path_following( void );

	int								m_path_follow_panic_counter;

	// Accumulates position offset due to a movable contact during an animation wait.
	Mth::Vector						m_offset_due_to_movable_contact;

	EStateType						m_state;

	// Time stamp of when our state last changed.
	Tmr::Time						m_state_timestamp;
	
	// State we changed from when m_state_timestamp was last set.
	EStateType						m_previous_state;

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

	// Normal of the ground on which we are standing.  Only meaningful when on the ground.  Set the previous frame.
	Mth::Vector						m_ground_normal;

	// Used to slerp the display matrix towards the actual matrix (can't use CCompositeObject::m_display_matrix since that is reset every frame).
	Mth::Matrix						m_display_slerp_matrix;

	// The collision cache used.
	Nx::CCollCache*					mp_collision_cache;

	// When doing anim speed scaling when running, this is set to the run speed which corresponds to the current animation playing at standard speed.
	float							m_anim_standard_speed;
	
	// A copy of the latest feeler which contacted the groud under us.
	CFeeler							m_last_ground_feeler;
	bool							m_last_ground_feeler_valid;

	// The target fraction of maximum speed [0.0, 1.0] as modified by let analog stick up down movement.
	float							m_target_speed;
	float							m_target_speed_adjustment;

	// Extracted from the object at frame start and back into the object at frame end.
	Mth::Vector						m_pos;
	Mth::Vector						m_horizontal_vel;
	float							m_vertical_vel;
	Mth::Vector						m_facing;
	Mth::Vector						m_upward;

	// The control-based change in heading this frame. Used mainly for path following decisions.
	float							m_delta_angle;

	// Type of animation speed scaling to do.
	EAnimScaleSpeed					m_anim_scale_speed;
	
	// Horizontal speed for the purposes of animation speed scaling.
	float							m_anim_effective_speed;
	
	// Factor multiplied by all velocities.  Set by script.
	float							m_script_drag_factor;

	// Current frame's frame length.
	float							m_frame_length;

	// Current frame's control input.
	Mth::Vector						m_control_direction;
	float							m_control_magnitude;
	bool							m_control_pegged;

 	// Keep track of the previous state of the dpad controls so that we can ramp up to a run speed over time.
	bool							m_dpad_used_last_frame;
	
	// Keep track of when we first started using the dpad so that we can ramp up to a run speed over time.
	Tmr::Time						m_dpad_use_time_stamp;

	// Cache our position at the frame's start
	Mth::Vector						m_frame_start_pos;
	
	// Script event reporting our current state.  Set during each frame update and sent at the end of the update.
	uint32							m_frame_event;
	
	// Last frame's frame event.
	uint32							m_last_frame_event;

	// Used to lock control in the forward direction (during a camera flush request)
	bool							m_forward_control_lock;

	// When on ground, the walker is often floated above the ground in an attempt to smooth out stair/curb climbs.
	float							m_curb_float_height;
	
	// If m_curb_float_height was adjusted this frame.  If not, we lerp it to zero.
	bool							m_curb_float_height_adjusted;

};


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
inline float CHorseComponent::s_get_param( uint32 checksum )
{
	Script::CStruct* p_params = Script::GetStructure(CRCD(0x1b38036f,"GunslingerHorseParameters"));
	Dbg_Assert(p_params);
	
	float param;
	p_params->GetFloat(checksum, &param, Script::ASSERT);
	return param;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
inline bool CHorseComponent::forced_run( void )
{
	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
inline void CHorseComponent::set_state ( EStateType state )
{
    if (state == m_state) return;
	
	m_previous_state = m_state;
    
    if (m_state == WALKING_GROUND)
    {
        m_wall_push_test.active = false;
		m_run_toggle = false;
    }
	
	if (m_state != WALKING_GROUND && m_state != WALKING_AIR)
	{
		m_dpad_used_last_frame = false;
	}
	
	m_state = state;
	m_state_timestamp = Tmr::GetTime();
	
	if (m_state == WALKING_ANIMWAIT)
	{
		m_offset_due_to_movable_contact.Set();
	}
}

}

#endif
