//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       VehicleComponent.h
//* OWNER:          ???
//* CREATION DATE:  ??/??/??
//****************************************************************************

#ifndef __COMPONENTS_VEHICLECOMPONENT_H__
#define __COMPONENTS_VEHICLECOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/collision/collcache.h>
#include <gel/components/skeletoncomponent.h>
#include <gel/components/triggercomponent.h>
#include <gel/object/basecomponent.h>

#include <sk/engine/rectfeeler.h>
#include <sk/engine/feeler.h>

#define		CRC_VEHICLE CRCD(0xe47f1b79, "Vehicle")
#define		GetVehicleComponent() ((Obj::CVehicleComponent*)GetComponent(CRC_VEHICLE))
#define		GetVehicleComponentFromObject(pObj) ((Obj::CVehicleComponent*)(pObj)->GetComponent(CRC_VEHICLE))
		 
namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CInputComponent;
	class CAnimationComponent;
	class CSkaterCorePhysicsComponent;
	class CModelComponent;

#define vVP_GRAVITATIONAL_ACCELERATION								(386.4f)
#define vVP_MAX_NUM_GEARS											(6)
#define vVP_NUM_COLLIDERS											(2)

#define vVP_FLIP_DURATION											(5000)
#define vVP_FLIP_DELAY												(300)
#define vVP_FLIP_RATE												(3.0f)
#define vVP_FLIP_GRAVITY_FACTOR										(0.3f)
#define vVP_SLEEP_VEL												(1.0f)
#define vVP_SLEEP_ROTVEL											(0.01f)

class CVehicleComponent : public CBaseComponent
{
	friend class CVehicleSoundComponent;
	
	// number of frames of normal force history to record for each tire
	enum { vVP_NORMAL_FORCE_HISTORY_LENGTH = 10 };
	
	// HACK for control
	struct SControls {
		// steering; between -1.0 and 1.0
		float steering;
		
		// throttle
		bool throttle;
		
		// brake
		bool brake;
		
		// handbrake
		bool handbrake;
		
		// reverse
		bool reverse;
	};
	
	struct SEngine
	{
		// state variables
		// int gear;
		
		// constant characteristics
		
		// engine's base torque output before applying gear ratios
		float drive_torque;
		float drag_torque;
		
		int num_gears;
		
		float p_gear_ratios[vVP_MAX_NUM_GEARS];
		
		float reverse_torque_ratio;
		
		float differential_ratio;
		
		// float differential_stiffness;
		
		// float downshift_rotvel;
		
		float upshift_rotvel;
	};
	
	struct SBrake
	{
		// state variables
		
		// brake torque is applied both after engine torque and friction torque; here we track the unused torque between the applications
		float spare_torque;
		
		// constant characteristics
		
		// braking torque per wheel
		float torque;
		
		// strength of the handbrake torque when throttle is down and steering is forward
		float handbrake_torque;
	};
	
	struct SWheel
	{
		
		enum EStateType {
			NO_STATE, OUT_OF_CONTACT, UNDER_GRIPPING, GRIPPING, SLIPPING, SKIDDING, HANDBRAKE_THROTTLE, HANDBRAKE_LOCKED
		};
		
		enum ESteeringType {
			FIXED, LEFT, RIGHT
		};
		
		// state variables
		
		// offset along Y-axis of wheel's position in vehicle's frame
		float y_offset;
		
		// is the wheel out of contact, gripped, skidding, etc
		EStateType state;
		
		// angular velocity about the axle
		float rotvel;
		
		// angular position about the axle
		float orientation;
		
		// angle of deflection from forward
		float steering_angle;
		
		// displayed angle of deflection from forward; lerps behind steering_angle
		float steering_angle_display;
		
		// dependent variables
		
		// world frame position of the bottom of the wheel
		Mth::Vector pos_world;
		
		// world frame velocity of the bottom of the wheel, not counting rotation
		Mth::Vector vel_world;
		
		// magnitude of the normal force applied by the suspension through the tire; set in apply_suspension_forces(); used in apply_friction_forces()
		float normal_force;
		
		// magnitude of the normal force for the last few frames; friction uses a smoothed normal force to even out control
		float normal_force_history [ vVP_NORMAL_FORCE_HISTORY_LENGTH ];
		
		// normal of the wheel's contact surface
		Mth::Vector contact_normal;
		
		// friction coefficient this frame
		float friction_coefficient;
		
		// ground detector feeler start and end positions in world frame
		Mth::Vector feeler_start_world;
		Mth::Vector feeler_end_world;
		
		// NOTE: used only to draw skid indicators; remove if not used for anything else
		float slip_vel;
		
		// accumulators
		
		// total rotational acceleration applied so far this frame
		float rotacc;
		
		// constant characteristics
		
		// start of the wheel's downward pointing collision feeler; basically the top of the wheel's position in the vehicle's frame
		Mth::Vector pos;
		
		// type of wheel with respect to steering
		ESteeringType steering;
		
		// true if the wheel is a drive wheel
		bool drive;
		
		// hang point; wheel's y_offset will drop only to this point; the effective equilibrium point for the spring, if one accounts for the wheel's
		// weight ahead of time
		float y_offset_hang;
		
		// the wheels are never rendered above this y threshold; this has no affect on their location from the physics code's perspective
		float max_draw_y_offset;
		
		// radius of wheel and tire (in)
		float radius;
		
		// inverse moment of the wheel and tire around the axis (1 / lb / in^2)
		float inv_moment;
		
		// suspension's spring constant (lb / in); spring rate
		float spring;
		
		// suspension's damping constant (lb s / in); bump rate
		float damping;
		
		// the friction coefficient is a set of concatenated lines; the transitions are made as these velocities; a more sophisticated
		float min_static_velocity;
		float max_static_velocity;
		float min_dynamic_velocity;
		
		// static friction coefficient
		float static_friction;
		
		// dynamic friction coefficient
		float dynamic_friction;
		
		// friction coefficient during handbraking
		float handbrake_throttle_friction;
		float handbrake_locked_friction;
		
		// subelements
		
		// brake
        SBrake brake;
		
		// caches to avoid repeating calculations between friction coefficient calculation and friction application
		Mth::Vector cache_projected_direction;
		Mth::Vector cache_projected_vel;
		
		// the last feeler for this wheel's height which touched the ground
		CFeeler last_ground_feeler;
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
	// inorder to interface with models and skeletons, the number of wheels must be hardcoded to four
	enum { vVP_NUM_WHEELS = 4 };
	
public:
    CVehicleComponent();
    virtual ~CVehicleComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
	virtual void					Finalize (   );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	void							MoveToNode ( Script::CStruct* p_node );
	void							ApplyArtificialCollision ( const Mth::Vector& offset );
	
	
	const Mth::Vector&				GetVel (   ) const { return m_vel; }
	int								GetNumWheelsInContact (   ) const { return m_num_wheels_in_contact; }
	bool							IsOnGround (   ) const;
	
	uint32							GetSoundSetupChecksum (   ) const;
	
private:
	
	void							get_input (   );
	void							zero_input (   );
	
	void							update_wheel_from_structure ( SWheel& wheel, Script::CStruct* p_wheel_struct );
	
	void							update_dynamic_state (   );
	void							update_wheel_dynamic_state (   );
	void							update_dependent_variables (   );
	void							update_velocities (   );
	void							update_collision_cache (   );
	
	void							damp_rotation (   );
	
	void							calculate_inverse_moment (   );
	float							calculate_friction_coefficient ( SWheel& wheel, float velocity ) const;
	void							calculate_friction_coefficients (   );
	Mth::Vector						calculate_body_point_velocity ( const Mth::Vector& pos ) const;
	float							calculate_body_point_effective_mass ( const Mth::Vector& pos, const Mth::Vector& direction ) const;
	
	int								determine_effective_gear ( float wheel_rotvel );
	void							update_wheel_heights (   );
	void							update_steering_angles (   );
	
	void							accumulate_forces (   );
	void							apply_gravitational_forces (   );
	void							apply_suspension_forces (   );
	void							apply_engine_forces (   );
	void							apply_drag_forces (   );
	void							apply_friction_forces (   );
	void							apply_brake_forces (   );
	void							apply_handbrake_forces ( float application_factor );
	void							apply_spare_brake_forces (   );
	
	void							accumulate_force ( const Mth::Vector& force, const Mth::Vector& location, uint32 color = MAKE_RGB(255, 255, 255) );
	void							accumulate_collision_force ( const Mth::Vector& force, const Mth::Vector& location );
	void							apply_wheel_torque ( SWheel& wheel, float torque );
	float							calculate_stopping_torque ( const SWheel& wheel, float rotvel ) const;
	
	void							update_flip (   );
	void							slerp_to_face_velocity (  );
	void							consider_sleeping (   );
	bool							in_artificial_collision (    );
	
	
									// environment collision members
	float							calculate_collision_depth ( const SCollisionPoint& collision_point, const SCollider& collider ) const;
	float							calculate_collision_depth ( const CLineFeeler& line_feeler ) const;
	bool							check_for_capping ( SCollisionPoint& collision_point, const CRectFeeler& rect_feeler, const SCollider& collider, int collision_line_idx, int collision_point_end_idx ) const;
	bool							consider_culling_point ( const SCollisionPoint& collision_point ) const;
	bool							very_close ( const Mth::Vector p, const Mth::Vector q ) const;
	bool							reset_this_frame (   ) const;
	void							apply_environment_collisions (   );
	void							update_pos_with_uber_frig ( const Mth::Vector& movement );
	void							apply_impulse ( const Mth::Vector& impulse, const Mth::Vector& location);
	
	void							trip_trigger ( ESkaterTriggerType trigger_type, CLineFeeler &feeler );
	void							trip_trigger ( ESkaterTriggerType trigger_type, CRectFeeler &feeler );
	
	void							control_skater (   );
	
	void							update_skeleton (   );
	
	void							draw_shadow (   );

	Mth::Vector						wheel_point ( const SWheel& wheel, int i, bool side ) const;
    void							draw_debug_rendering (   ) const;
	
private:
	
	// dynamic state variables
	
	// position
	Mth::Vector m_pos;
	
	// linear momentum
	Mth::Vector m_mom;
	
	// angular orientation
	Mth::Quat m_orientation;
	
	// rotational momentum
	Mth::Vector m_rotmom;
	
	// like a rigidbody, the vehicle can go to sleep
	enum EStateType
	{
		ASLEEP, AWAKE
	}
	m_state;
	
	// counting the number of consecutive sleep-worthy frames; after a certain number, we will go to sleep
	int m_consider_sleeping_count;
	
	// number of collision contacts during latest collision test
	int m_num_collision_points;
	
	// dependent state variables
	
	// angular orientation in 3x3 matrix form
	Mth::Matrix m_orientation_matrix;
	
	// linear velocity
	Mth::Vector m_vel;
	
	// rotational velocity
	Mth::Vector m_rotvel;
	
	// inverse moment of inertia in world frame
	Mth::Matrix m_inv_moment;
	
	// offset of center of mass used for suspension and friction, but not collisions
	Mth::Vector m_suspension_center_of_mass_world;
	
	// counts the number of wheels in contact with the ground
	unsigned int m_num_wheels_in_contact;
	
	// pointer to the next normal force history to use (the oldest one)
	char m_next_normal_force_history_idx;

	// if true, we were reset from a trigger script and should bail from the frame's update logic
	bool m_reset_this_frame;
	
	// if true, we are currently in the midst of flipping the car over
	bool m_in_flip;
	
	// if we're flipping the car, this is a time stamp of when the flipping began
	Tmr::Time m_flip_start_time_stamp;
	
	// during an artificial collision, this timer counts down to the end of the duration
	float m_artificial_collision_timer;
	
	// accumulators
	
	// accumulates the force on the body in a frame
	Mth::Vector m_force;
	
	// accumulates the torque on the body in a frame
	Mth::Vector m_torque;
	
	// length of current frame
	float m_time_step;
	
	// constant characteristics
	
	// vehicle's center of mass
	Mth::Vector m_suspension_center_of_mass;
	
	// inverse mass (1 / lb)
	float m_inv_mass;
	
	// inverse moment of inertia in vehicle's frame (1 / lb / in^2)
	Mth::Vector m_inv_moment_body;
	
	// coefficient of restitution of the body
	float m_body_restitution;
	
	// coefficient of friction of the body
	float m_body_friction;
	
	// spring constant of the collision penalty system
	float m_body_spring;
	
	// coefficient of friction of the body used when the vehicle is not upright
	float m_body_wipeout_friction;
	
	// factor of normal collision impulse which you can control via your steering
	float m_collision_control;
	
	// horizontal velocity cutoff below which no in-air slerping to face velocity occurs
	float m_in_air_slerp_vel_cutoff;
	
	// time over which in-air slerping lerps to full strength after takeoff
	float m_in_air_slerp_time_delay;
	
	// in-air slerping strength
	float m_in_air_slerp_strength;
	
	// special slerping at low speeds
	bool m_vert_correction;

	// number of wheels
	unsigned int m_num_wheels;
	
	// number of drive wheels; engine torque is divided between this number of wheels
	unsigned int m_num_drive_wheels;
	
	// maximum steering angle
	float m_max_steering_angle;
	
	// effective distance between fixed and steering axles (in); used when determining turning radius
	float m_cornering_wheelbase;
	
	// distance between center of steering wheels (in); used when determining steering angles
	float m_cornering_axle_length;
	
	// constant and quadratic rotational damp coefficients
	float m_const_rotvel_damping;
	float m_quad_rotvel_damping;
	
	// vehicles have rectangular feelers which they use to detect collisions with their environment
	SCollider mp_colliders[vVP_NUM_COLLIDERS];
	
	// offset from the vehicle's center of mass the model's origin
	// float m_body_model_offset;
	
	// body model's position
	Mth::Vector m_body_pos;
	
	// if true, triangle acts as a brake and throws an ExitCar exception when the car stops
	bool m_exitable;
	
	// if true, the car has no handbrake
	bool m_no_handbrake;
	
	// subelements
	
	// wheels
	SWheel* mp_wheels;
	
	// engine
	SEngine m_engine;
	
	// input state
	SControls m_controls;
	
	// collision cache
	Nx::CCollCache m_collision_cache;
	
	// parameters controlling skater while vehicle is active
	
	// true if the skater should be visible while in the vehicle
	bool m_skater_visible;
	
	// position offset of skater model
	Mth::Vector m_skater_pos;
	
	// skater animation to use while in the vehicle
	uint32 m_skater_anim;
	
	// controls the steering used for the displayed wheels and driving animation; lerps to m_controls.steering
	float m_steering_display;
	
	// name of the sound setup structure from PlayerVehicleSounds that which vehicle uses
	uint32 m_sound_setup_checksum;
	
	// dynamic script-controllable parameters
	
	// the effective gravity can be modified via script; graviy is multiplied by m_gravity_fraction over a duration of m_gravity_override_timer
	float m_gravity_override_fraction;
	float m_gravity_override_timer;
	
	// if true, the brakes are forced to be on
	bool m_force_brake;
	
	// state observers
	
	// time since we last had a wheel on the ground (plus no body contact)
	float m_air_time;
	float m_air_time_no_collision;
	
	// latest updates maximum normal collision impulse
	float m_max_normal_collision_impulse;
	
	// peer components
	
	CSkeletonComponent* mp_skeleton_component;
	CInputComponent* mp_input_component;
	CModelComponent* mp_model_component;
	
	// shared objects
	
	// shared collision point array
	static SCollisionPoint sp_collision_points[4 * (Nx::MAX_NUM_2D_COLLISIONS_REPORTED + 1)];
	
	// debug
	int m_draw_debug_lines;
	
	// if true, no collision detection is done and the skeleton is not updated; this is used to allow a car to settle on its suspension "off camera"
	bool m_update_suspension_only;
	
	// the driver
	CCompositeObject* mp_skater;
	CAnimationComponent* mp_skater_animation_component;
	CSkaterCorePhysicsComponent* mp_skater_core_physics_component;
	CTriggerComponent* mp_skater_trigger_component;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CVehicleComponent::IsOnGround (   ) const
{
	return m_num_wheels_in_contact > 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline uint32 CVehicleComponent::GetSoundSetupChecksum (   ) const
{
	return m_sound_setup_checksum;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CVehicleComponent::update_velocities (   )
{
	m_vel = m_inv_mass * m_mom;

	// Zero the W component here to avoid possible NaN propogation issues.
	m_vel[W] = 0.0f;
	m_rotvel = m_inv_moment.Rotate(m_rotmom);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Mth::Vector CVehicleComponent::calculate_body_point_velocity ( const Mth::Vector& pos ) const
{
	return m_vel + Mth::CrossProduct(m_rotvel, pos);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CVehicleComponent::accumulate_force ( const Mth::Vector& force, const Mth::Vector& location, uint32 color )
{
	m_force += force;
	m_torque += Mth::CrossProduct(location - m_suspension_center_of_mass_world, force);
	
	#if 0
	if (m_draw_debug_lines)
	{
		Mth::Vector adjusted_location = location;
		adjusted_location[Y] += 1.0f; // pull out of the ground
		Gfx::AddDebugLine(m_pos + adjusted_location, m_pos + adjusted_location + 0.05f * force, color, color, 1);
	}
	#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CVehicleComponent::accumulate_collision_force ( const Mth::Vector& force, const Mth::Vector& location )
{
	m_force += force;
	m_torque += Mth::CrossProduct(location, force);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CVehicleComponent::apply_wheel_torque ( SWheel& wheel, float torque )
{
	float rotacc = wheel.inv_moment * torque;
	
	wheel.rotvel += rotacc * m_time_step;
	
	wheel.rotacc += rotacc;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float CVehicleComponent::calculate_stopping_torque ( const SWheel& wheel, float rotvel ) const
{
	return -rotvel / (m_time_step * wheel.inv_moment);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
			
inline float CVehicleComponent::calculate_collision_depth ( const CLineFeeler& line_feeler ) const
{
	return (1.0f - line_feeler.GetDist()) * Mth::DotProduct(line_feeler.GetNormal(), line_feeler.m_start - line_feeler.m_end);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CVehicleComponent::apply_impulse ( const Mth::Vector& impulse, const Mth::Vector& location )
{
	m_mom += impulse;
	m_rotmom += Mth::CrossProduct(location, impulse);
	
	update_velocities();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CVehicleComponent::very_close ( const Mth::Vector p, const Mth::Vector q ) const
{
	return Mth::Abs(p[X] - q[X]) < 0.1f && Mth::Abs(p[Y] - q[Y]) < 0.1f && Mth::Abs(p[Z] - q[Z]) < 0.1f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CVehicleComponent::reset_this_frame (   ) const
{
	return m_reset_this_frame || GetObject()->IsDead();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CVehicleComponent::trip_trigger ( ESkaterTriggerType trigger_type, CLineFeeler &feeler )
{                            
	mp_skater_trigger_component->CheckFeelerForTrigger(trigger_type, feeler);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CVehicleComponent::trip_trigger ( ESkaterTriggerType trigger_type, CRectFeeler &feeler )
{                            
	mp_skater_trigger_component->CheckFeelerForTrigger(trigger_type, feeler);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CVehicleComponent::in_artificial_collision (    )
{                            
	return m_artificial_collision_timer > 0.0f;
}

}

#endif
