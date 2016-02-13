//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterCorePhysicsComponent.h
//* OWNER:          Dan
//* CREATION DATE:  3/21/3
//****************************************************************************

#ifndef __COMPONENTS_SKATERCOREPHYSICSCOMPONENT_H__
#define __COMPONENTS_SKATERCOREPHYSICSCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>
#include <core/math/slerp.h>

#include <gel/object/basecomponent.h>
#include <sk/engine/feeler.h>
#include <sk/objects/skater.h>
#include <sk/objects/skaterflags.h>
#include <sk/objects/manual.h>
#include <sk/objects/rail.h>
#include <sk/components/skaterstatecomponent.h>
#include <sk/components/skatersoundcomponent.h>
#include <sk/components/skaterphysicscontrolcomponent.h>

#define		CRC_SKATERCOREPHYSICS CRCD(0x5bd63b29, "SkaterCorePhysics")

#define		GetSkaterCorePhysicsComponent() ((Obj::CSkaterCorePhysicsComponent*)GetComponent(CRC_SKATERCOREPHYSICS))
#define		GetSkaterCorePhysicsComponentFromObject(pObj) ((Obj::CSkaterCorePhysicsComponent*)(pObj)->GetComponent(CRC_SKATERCOREPHYSICS))

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
	class CInputComponent;
	class CTriggerComponent;
	class CSkaterLoopingSoundComponent;
	class CTrickComponent;
	class CSkaterRotateComponent;
	class CSkaterScoreComponent;
	class CMovableContactComponent;
	
	struct SRailData;

class CSkaterCorePhysicsComponent : public CBaseComponent
{
	friend Mdl::Skate;
	friend CSkater;
	friend CSkaterCam;
	friend CManual;
	friend CSkaterCameraComponent;
	friend CSkaterAdjustPhysicsComponent;
	friend CSkaterFinalizePhysicsComponent;
	friend CSkaterNonLocalNetLogicComponent;
	friend CSkaterRotateComponent;
	friend CSkaterPhysicsControlComponent;
	friend CSkaterMatrixQueriesComponent;
	friend CSkaterStateHistoryComponent;
	friend CWalkComponent;
	
public:
    CSkaterCorePhysicsComponent();
    virtual ~CSkaterCorePhysicsComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    virtual void            		Finalize();
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	CSkater*						GetSkater() { return static_cast< CSkater* >(GetObject()); }
	
public:
	void							Reset (   );
	void							ReadySkateState ( bool to_ground_state, const SRailData* p_rail_data, const SAcidDropData* p_acid_drop_data );
	void							CleanUpSkateState (   );
	
	EStateType						GetState (   );
	void							SetState ( EStateType state );
	bool							GetFlag ( ESkaterFlag flag );
	void							SetFlag ( ESkaterFlag flag, bool state = true );
	void							SetFlagTrue ( ESkaterFlag flag );
	void							SetFlagFalse ( ESkaterFlag flag );
	void							ToggleFlag ( ESkaterFlag flag );
	Tmr::Time						GetFlagTime ( ESkaterFlag flag );
	Tmr::Time						GetFlagElapsedTime ( ESkaterFlag flag );
	void							ResetFlags (   );
	void							ResetSpecialFrictionIndex (   );
	void							ReverseFacing (   );
	bool							HaveLandedThisFrame (   );
	ETerrainType					GetTerrain (   );
	ETerrainType					GetLastGroundTerrain (   );
	uint32							GetLastNodeChecksum (   );
	bool							GetTrueLandedFromVert (   );
	void							ResetLerpingMatrix (   );
	void							StopSkitch (   );
	bool							IsSwitched (   );
	void							ResetGrindTweak (   );
	sint16							GetRailNode( void );
	
	void							CollideWithOtherSkaterLost ( CCompositeObject* p_other_skater );
	
	Mth::Vector&					GetPos (   ) { return GetObject()->m_pos; }
	Mth::Vector&					GetOldPos (   ) { return GetObject()->m_old_pos; }
	Mth::Matrix&					GetMatrix (   ) { return GetObject()->m_matrix; }
	Mth::Vector&					GetVel (   ) { return GetObject()->m_vel; }
	
private:
	void							do_on_ground_physics (   );
	void							do_in_air_physics (   );
	void							do_wallride_physics (   );
	void							do_wallplant_physics (   );
	void							do_lip_physics (   );
	void							do_rail_physics (   );
	
	void							limit_speed (   );
	bool							is_trying_to_brake (   );
	bool							is_braking (   );
	void							do_brake (   );
	bool							on_steep_slow_slope (   );
	bool							can_kick (   );
	void							do_kick (   );
	void							handle_ground_friction (   );
	void							handle_wind_resistance (   );
	void							handle_rolling_resistance (   );
	bool							slide_off_slow_steep_slope (   );
	void							apply_wind_resistance ( float friction );
	void							apply_rolling_friction (   );
	void							push_away_from_walls (   );
	void							bounce_off_wall ( const Mth::Vector& normal );
	void							check_for_wall_push ( const Mth::Vector& start, const Mth::Vector& end, int index );
	void							snap_to_ground (   );
	void							maybe_straight_up (   );
	void							maybe_break_vert (   );
	bool							maybe_spine_transfer (   );
	bool							look_for_transfer_target ( const Mth::Vector& search_dir, const Mth::Vector& start_normal, bool& hip_transfer, Mth::Vector& target, Mth::Vector& target_normal );
	bool							maybe_acid_drop ( bool skated_off_edge, const Mth::Vector &pos, const Mth::Vector& old_pos, Mth::Vector& vel, SAcidDropData& acid_drop_data );
	void							enter_acid_drop ( const SAcidDropData& acid_drop_data );
	void							handle_post_transfer_limit_overrides (   );
	void							handle_ground_rotation (   );
	void							handle_air_rotation (   );
	void							remove_sideways_velocity ( Mth::Vector& vel );
	void							check_side_collisions (   );
	bool							check_side ( float Side, float SideCol );
	void							handle_forward_collision (   );
	float							rotate_away_from_wall ( const Mth::Vector& normal, float& turn_angle, float lerp = 1.0f );
	float							get_air_gravity (   );
	void							check_leaning_into_wall (   );
	float							calculate_time_to_reach_height ( float target_height, float pos_Y, float vel_Y );
	void							flip_if_skating_backwards (   );
	bool							maybe_flag_ollie_exception (   );
	void							handle_air_lean (   );
	void							handle_transfer_slerping (   );
	void							handle_air_vert_recovery (   );
	void							rotate_upright (   );
	bool							handle_forward_collision_in_air ( const Mth::Vector& start_pos );
	bool							check_for_air_snap_up ( const Mth::Vector& start_pos );
	bool							handle_upward_collision_in_air (   );
	bool							handle_upward_collision_in_wallride (   );
	void							ollie_off_rail_rotate (   );
	void							skate_off_rail ( const Mth::Vector& off_point );
	bool							check_for_wallpush (   );
	bool							check_for_wallplant (   );
	bool							check_for_wallride (   );
	void							maybe_skitch (   );
	void							move_to_skitch_point (   );
	void							check_movable_contact (   );
	void							new_normal ( Mth::Vector normal );
	void							adjust_normal (   );
	void							maybe_trip_rail_trigger ( uint32 type );
	void							handle_tensing (   );
	void							maybe_stick_to_rail ( bool override_air = false );
	bool							will_take_rail ( const CRailNode* pNode, CRailManager* p_rail_man, bool from_walk = false );
	void							got_rail ( const Mth::Vector& rail_pos, const CRailNode* pNode, CRailManager* p_rail_man, bool no_lip_tricks = false, bool from_walk = false );
	bool							get_member_feeler_collision ( uint16 ignore_1 = mFD_NON_COLLIDABLE, uint16 ignore_0 = 0 );
	void							start_skitch (   );
	void							do_grind_trick ( uint Direction, bool Right, bool Parallel, bool Backwards, bool Regular );
	void							set_terrain ( ETerrainType terrain );
	void							set_last_ground_feeler ( CFeeler& feeler );
	void							setup_default_collision_cache (   );
	bool							is_vert_for_transfers ( const Mth::Vector& normal );
	void							update_special_friction_index (   );
	
public:
	void							do_jump ( Script::CStruct *pParams );
	
private:
	Mth::Matrix						m_lerping_display_matrix;
	Mth::Vector						m_display_normal;				// current normal the skater is oriented to
	Mth::Vector						m_current_normal;   			// normal of current polygon 
	Mth::Vector						m_last_display_normal;   		// last value of m_display_normal
	float							m_normal_lerp;					// counter 1 to 0 for amount of normal lerp
	
	float							m_frame_length;
	
	Tmr::Time						m_state_change_timestamp;		// timestamp of the latest state change
	EStateType						m_previous_state;				// the skater's state previous to the current state
	
	Mth::Vector						m_safe_pos;						// Safe world position, to which we can be moved in an emergency
	
	CFeeler							m_feeler;	   					// collision data from last collision check 
	CFeeler							m_last_ground_feeler;			// and from last time we were in valid contact with ground
	Mth::Vector						m_col_start, m_col_end;			// start and end of collision line segment
	bool							m_col_flag_skatable;			// true is last face was skatable
	bool							m_col_flag_vert;				// true if last face was flagged vert
	bool							m_col_flag_wallable;			// true if last face was flagged wall-ridable
	
	// Mth::Vector						m_movement;						// distance we want to move this frame
																	// used when moving over ground so we don't travel short distances
																	// when we hit a concave surface which causes collisions				  
	
	const CRailNode*				mp_rail_node;					// pointer to current rail node
	CRailManager*					mp_rail_man;					// pointer to manager of this rail
	Tmr::Time						m_rail_time;					// last time we were on a rail
	Tmr::Time						m_lip_time;						// last time we were on a lip
	Tmr::Time						m_rerail_time;					// time that must elapse before we can get back on the same rail
	uint32							m_last_rail_node_name;
	uint32							m_last_rail_trigger_node_name;
	bool							mLedge;                         // Whether it's a ledge. Calculated in MaybeStickToRail.
	bool							mBadLedge;                      // Whether this is a bad ledge. Calculated in MaybeStickToRail, and 
																	// used by the BadLedge script command.
	
	
	bool							m_moving_to_skitch; 			// true if in initial movment to skitch
	int								m_skitch_index;					// index of the node number on the object we are skitching
	float							m_skitch_dir;					// direction (-1.0f or 1.0f) we are moving in
	
	bool							m_landed_this_frame;			// true if we just landed this frame
	
	bool							m_began_frame_in_transfer;
	bool							m_began_frame_in_lip_state;

	Mth::Vector						m_pre_lip_pos;					// position we were at before we snapped to a lip

	Mth::Vector						m_vert_pos;						// point below us on a vert face
	Mth::Vector						m_vert_normal;					// normal of that face

	Mth::Vector						m_fall_line;					// unit vector that points "down" the plane

	float							m_vert_upstep;					// amount we try to move up each frame when tracking vert

	Mth::Vector						m_extra_ground_push;			// extra vector to apply to next "ground" movement

	Mth::Vector						m_wall_push; 					// direction to push away from wall
	float							m_wall_dist;					// shortest distance from wall
	float							m_wall_push_radius;				// radius to check
	float							m_wall_push_speed;				// speed at which to push out
	float 							m_wall_rotate_speed;			// speed at which we rotate away

	float							m_last_time;					// time at last frame
	
public:
	bool							m_auto_kick;   					// true if auto kick on
	bool							m_spin_taps;					// true when 180 spin taps are enabled.
	
protected:
	float							m_tap_turns;					// How much tap-turn rotation remains.

	bool							m_force_cankick_off;			// Set by the CanKickOff script command, so that Scott can
																	// force CanKick to return false;

	bool							m_pressing_down_brakes;			// Set/Reset by the CanBrakeOn and CanBrakeOff
																	// commands.


	float							m_rolling_friction;				// Friction, set by SetRollingFriction script command.
	int								m_special_friction_index;		// Index into an array of special friction values, set by the
																	// SetSpecialFriction command (used by reverts)
	Tmr::Time						m_special_friction_decrement_time_stamp;
	float							m_special_friction_duration;	// When m_rolling_friction is set via special friction, it must be given a duration;
																	// after which it reverts to default.  Calls to set m_rolling_friction to default
																	// are ignored during this duration.

	bool							m_braking;						// true if currently braking
	Tmr::Time						m_push_time[4];					// time of last side push

	Mth::Vector						m_last_ground_pos; 				// Set in DoJump(), & used when calculating which side of rail skater is on.
	bool							mRail_Backwards;

																	// Flags and value to override the speed limits set on the skater
	float							m_override_limits_time;			// 0.0f if not overriding, else time in seconds to override for
	float							m_override_max;					// override value for max_speed
	float							m_override_max_max;				// override value for max_max_speed
	float							m_override_air_friction;    	// override value for air friction
	float							m_override_down_gravity;		// override value for ground gravity (only applied when not going down)

	Obj::CSmtPtr<Obj::CCompositeObject>	mp_skitch_object;

	bool							mNoSpin;						// Gets set and reset by NoSpin and CanSpin.
																	// Disables left-right pad control of the spin.
	bool							m_bail;							// Set by BailOn, reset by BailOff and tested using BailIsOn
	
	bool							m_front_truck_sparks;			// True if sparks come from the front trucks; false if from the rear

	bool							mLandedFromSpine;				// True if landed from a transfer.
	bool							mLandedFromVert;				// True if landed from vert. Can be set & reset by script commands.
	
	bool							m_true_landed_from_vert;		// This is the 'true' landed from vert flag, which only gets set by the C-code on landing. Scripts
																	// cannot change its value.
																	// Used by the Clearpanel_Landed C-code to determine whether or not to count an ollie as a trick.
																	// 180 Ollies are not counted as tricks if you landed from vert, only spins of at least 360 count.
																	// mLandedFromVert can't be used because the scripts have already cleared it by the time the 
																	// Clearpanel_Landed command is executed.
	
	bool							mAllowLipNoGrind;               // When this is set, then lip tricks will always happen when sticking to a rail,
																	// and grinds will not be allowed.
																	// Only gets set or reset by script commands, AllowLipNoGrind and ClearAllowLipNoGrind
	
	int								m_tense_time;                   // The time that the player was tensed for. Gets recorded by MaybeFlagOllieException,
																	// and used by the DoJump function.
	
	float							m_transfer_target_height;		// target point for transfer
	
																	// These next two needed by the AirTimeLessThan and AirTimeGreaterThan script functions.
	Tmr::Time						m_went_airborne_time;			// The time when the state change to AIR
	Tmr::Time						m_landed_time; 					// The time when the state changed from AIR to something else.
	
	bool							m_lock_velocity_direction;		// Ken: Switched on/off by the LockVelocityDirection command.
																	// This is a way to allow the skater to be able to cross a gap in a loop whilst
																	// inverted and technically being on the ground (on an invisible poly) but such that he 
																	// looks he's in the air. He will be able to be rotated, but his velocity will not turn
																	// with his rotation.
	
	Mth::Vector						mWallNormal;
	uint32							mWallrideTime; 					// The time the wall-ride was triggered. Used to stop the player triggering
																	// another one too soon after the last.

	bool							mNoRailTricks;                  // K: No grinds or lip tricks are allowed if this flag is set. This is switched on and off by
																	// the NoGrind and AllowGrind script commands. These are used when jumping out of
																	// a lip trick to disallow going straight into a grind.
	
	bool							mYAngleIncreased;				// Keeps track of whether the y angle last increased or decreased.
																	// Used by the LastSpinWas command.
	
	int								mGrindTweak;
	
	float							m_transfer_overrides_factor;	// Control the overriding of speed limits after an acid drop.
	
	Mth::Matrix						m_acid_drop_camera_matrix;
	
	Mth::SlerpInterpolator			m_transfer_slerper;				// Variables controlling the slerping of the skater's matrix during an acid drop
	float							m_transfer_slerp_timer;
	float							m_transfer_slerp_duration;
	Mth::Vector						m_transfer_goal_facing;
	Mth::Matrix						m_transfer_slerp_previous_matrix;

	Tmr::Time						m_last_wallplant_time_stamp;	// Timestamps wallplants in order to not allow a second one for a short duration.
	Tmr::Time						m_last_wallpush_time_stamp;		// Timestamps wallpushes in order to not allow a second one for a short duration.
	
	Tmr::Time						m_last_jump_time_stamp;			// Timestamps jumps
	
	bool							m_vert_air_last_frame;			// Used to broadcast vert air enter and exit events
	
	Nx::CCollCache*					mp_coll_cache;
	
private:
	// peer components
	CInputComponent*				mp_input_component;
	CTriggerComponent*				mp_trigger_component;
	CSkaterSoundComponent*			mp_sound_component;
	CTrickComponent*				mp_trick_component;
	CSkaterRotateComponent*			mp_rotate_component;
	CSkaterScoreComponent*			mp_score_component;
	CSkaterBalanceTrickComponent*	mp_balance_trick_component;
	CSkaterStateComponent*			mp_state_component;
	CMovableContactComponent*		mp_movable_contact_component;
	CSkaterPhysicsControlComponent*	mp_physics_control_component;
	CWalkComponent*					mp_walk_component;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline bool CSkaterCorePhysicsComponent::is_braking (   )
{
	 return m_braking;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline bool CSkaterCorePhysicsComponent::is_vert_for_transfers ( const Mth::Vector& normal )
{
	// cull out non-vert vert polys when looking for spine transfer and acid drop triggers; allows designers to be a little sloppier
	return Mth::Abs(normal[Y]) < 0.707f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline EStateType CSkaterCorePhysicsComponent::GetState (   )
{
	return mp_state_component->m_state;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Tmr::Time CSkaterCorePhysicsComponent::GetFlagTime ( ESkaterFlag flag )
{
	return mp_state_component->m_skater_flags[flag].GetTime();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Tmr::Time CSkaterCorePhysicsComponent::GetFlagElapsedTime ( ESkaterFlag flag )
{
	return mp_state_component->m_skater_flags[flag].GetElapsedTime();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CSkaterCorePhysicsComponent::GetFlag ( ESkaterFlag flag )
{
	return mp_state_component->m_skater_flags[flag].Get(); 
} 								 

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
								 
inline void CSkaterCorePhysicsComponent::SetFlag ( ESkaterFlag flag, bool state )
{
	mp_state_component->m_skater_flags[flag].Set(state);
	if (flag == RAIL_SLIDING)
	{
		mp_sound_component->SetIsRailSliding(state);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CSkaterCorePhysicsComponent::SetFlagTrue ( ESkaterFlag flag )
{
	SetFlag(flag, true);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CSkaterCorePhysicsComponent::SetFlagFalse ( ESkaterFlag flag )
{
	SetFlag(flag, false);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CSkaterCorePhysicsComponent::ToggleFlag ( ESkaterFlag flag )
{
	SetFlag(flag, !GetFlag(flag));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
 								 
inline bool CSkaterCorePhysicsComponent::IsSwitched (   )
{
	return GetSkater()->m_isGoofy ? GetFlag(FLIPPED) : !GetFlag(FLIPPED);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
 								 
inline bool CSkaterCorePhysicsComponent::HaveLandedThisFrame (   )
{
	return m_landed_this_frame;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline ETerrainType CSkaterCorePhysicsComponent::GetLastGroundTerrain (   )
{
	return m_last_ground_feeler.GetTerrain();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline uint32 CSkaterCorePhysicsComponent::GetLastNodeChecksum (   )
{
	return m_last_ground_feeler.GetNodeChecksum();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CSkaterCorePhysicsComponent::GetTrueLandedFromVert (   )
{
	return m_true_landed_from_vert;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CSkaterCorePhysicsComponent::ResetSpecialFrictionIndex (   )
{
 	m_special_friction_index = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
 								 
inline ETerrainType CSkaterCorePhysicsComponent::GetTerrain (   )
{
	return mp_state_component->m_terrain;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline sint16	CSkaterCorePhysicsComponent::GetRailNode( void )
{
	if( mp_rail_node )
	{
		return mp_rail_node->GetNode();
	}
	else
	{
		return -1;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
 								 
inline void CSkaterCorePhysicsComponent::ResetGrindTweak (   )
{
	mGrindTweak = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline void CSkaterCorePhysicsComponent::ResetLerpingMatrix (   )
{
	m_lerping_display_matrix = GetObject()->m_matrix;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
 								 
inline void CSkaterCorePhysicsComponent::set_terrain ( ETerrainType terrain )
{
	mp_state_component->m_terrain = terrain;
	mp_sound_component->SetTerrain(terrain);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
 								 
inline void CSkaterCorePhysicsComponent::set_last_ground_feeler ( CFeeler& feeler )
{
	m_last_ground_feeler = feeler;
	mp_sound_component->SetLastTerrain(m_last_ground_feeler.GetTerrain());
}

}


#endif
