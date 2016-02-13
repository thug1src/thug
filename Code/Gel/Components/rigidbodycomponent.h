//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       RigidBodyComponent.h
//* OWNER:          Dan
//* CREATION DATE:  1/22/3
//****************************************************************************

#ifndef __COMPONENTS_RIGIDBODYCOMPONENT_H__
#define __COMPONENTS_RIGIDBODYCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>
#include <gel/collision/collcache.h>

#define		CRC_RIGIDBODY CRCD(0xffb39248, "RigidBody")
#define		GetRigidBodyComponent() ((Obj::CRigidBodyComponent*)GetComponent(CRC_RIGIDBODY))
#define		GetRigidBodyComponentFromObject(pObj) ((Obj::CRigidBodyComponent*)(pObj)->GetComponent(CRC_RIGIDBODY))

#define vRP_GRAVITATIONAL_ACCELERATION	 					(-386.4f)
#define vRP_DEFAULT_MASS_OVER_MOMENT						(0.003f)
#define vRP_DEFAULT_COEFF_RESTITUTION	  					(0.4f)
#define vRP_DEFAULT_COEFF_FRICTION	    					(0.5f)
#define vRP_DEFAULT_SPRING_CONST    						(10.0f)
#define vRP_DEFAULT_SKATER_COLLISION_RADIUS	  				(12.0f + 36.0f)
#define vRP_DEFAULT_SKATER_COLLISION_APPLICATION_RADIUS		(12.0f)
#define vRP_DEFAULT_SKATER_COLLISION_ASSENT					(25.0f)
#define vRP_DEFAULT_SKATER_COLLISION_IMPULSE_FACTOR			(1.0f)
#define vRP_DEFAULT_SKATER_COLLISION_ROTATION_FACTOR		(1.0f)
#define vRP_SKATER_COLLISION_VELOCITY_FACTOR 		   		(1.0f)
#define vRP_SKATER_COLLISION_VELOCITY_THRESHOLD_INCREMENT	(50.0f)
#define vRP_DEFAULT_LINEAR_VELOCITY_SLEEP_POINT	   			(10.0f)
#define vRP_DEFAULT_ANGULAR_VELOCITY_SLEEP_POINT   			(0.4f)
#define vRP_DEFAULT_IGNORE_SKATER_DURATION					(5.0f / 60.0f)
#define vRP_DEFAULT_COLLIDE_MUTE_DELAY						(1000)
#define vRP_DEFAULT_GLOBAL_COLLIDE_MUTE_DELAY				(100)
#define vRP_DEFAULT_BOUNCE_VELOCITY_CALLBACK_THRESHOLD		(20.0f)
#define vRP_DEFAULT_BOUNCE_VELOCITY_FULL_SPEED				(300.0f)

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CSoundComponent;

class CRigidBodyComponent : public CBaseComponent
{
	enum ERigidBodyStateType
	{
		ASLEEP, AWAKE
	};

	enum EObjectGeometryType
	{
		BOX, PYRAMID, CYLINDER, TRIANGLE, NO_GEOMETRY
	};
	
	enum EFlagType
	{
		DIE_UPON_SLEEP,
		PLAYER_COLLISION_DISABLED
	};

	struct SContact
	{
		// defining element of a rigidbody contact point; position
		Mth::Vector p;
		
		// the axis of friction
		Mth::Vector friction_direction;
		
		// if friction at this contact occurs only along a single axis
		char directed_friction;
	};
	
	struct SContactState
	{
		// offset to the contact point in world space
		Mth::Vector p_world;

		// normal of the surface is it colliding with
		Mth::Vector normal;

		// the effective mass of this contact for impulses applied along the normal
		float normal_eff_mass;

		// keeps track of the total impulse magnitude of the normal force in a frame; used to limit the frictional impulse
		float normal_impulse_magnitude;
		
		// depth of the contact; negative corresponds to interpenetration
		float depth;

		// is this contact in collision
		char collision;

		// false before the normal_inv_mass has been set
		char normal_eff_mass_set;
	};
	
	// if the number of free voices is this number or less, don't play bounce sounds
	static const int vRP_NUM_UNTOUCHABLE_VOICES = 12;
	
	// maximum number of contacts allowed on a single rigidbody
	static const unsigned int vRP_MAX_NUM_CONTACTS = 24;

public:
    CRigidBodyComponent();
    virtual ~CRigidBodyComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
	virtual void					Finalize();
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	static void 					sToggleDrawRigidBodyDebugLines (   );

private:

	void							setup_contact_states (   );
	void							handle_skater_collisions (   );
	void							update_dynamic_state ( float time_step );
	void							consider_sleeping (   );
	void							wake (   );
	void							sleep (   );
	bool							detect_collisions (   );
	float							calculate_effective_mass ( const Mth::Vector& direction, const SContactState& contact_state ) const;
	void							resolve_collisions (   );
	void							draw_debug_lines (   ) const;
	void							get_sound_setup ( uint32 sound_type_id );
	void							setup_contacts_as_box ( const Mth::Vector& top_half_dimensions, const Mth::Vector& bottom_half_dimensions );
	void							setup_contacts_as_pyramid ( float radius, float half_depth );
	void							setup_contacts_as_cylinder ( float top_radius, float bottom_radius, float half_depth, int edges = 6 );
	void							setup_contacts_as_triangle ( const Mth::Vector& half_dimensions );
	void							setup_contacts_from_array ( Script::CArray *pArray );

private:

	// properties:

	// object's don't have masses or moments of inertia;
	// instead, a single scalar is used for the moment of inertia and only the ratio of mass over moment is used;
	// this ratio controls the object's relative tendency to move linearly or rotationally;
	// if proper external forces are ever applied to rigidbodies, they will need a mass;
	// units are inverse inches squared; should be on the order of the inverse of the square of the characteristic
	// distance between the contact points and the center of mass
	float m_mass_over_moment;

	// object's acceleration due to constant force on its center of mass (gravity)
	float m_const_acc;

	// coefficient of restitution; bounce factor
	float m_coeff_restitution;

	// coefficient of friction
	float m_coeff_friction;
	
	// spring constant used to prevent interpenetration
	float m_spring_const;

	// effectively the sum of the object radius and the skater radius; when the center-to-center distance is below this, there is a collision
	float m_skater_collision_radius;

	// effectively the radius of the object; the impulse is applied at this distance from the object; to have reasonable rotations, m_mass_over_moment
	// should on the order of the inverse of this radius squared; also used in skater collisions to check their Y-interception
	float m_skater_collision_application_radius;

	// sine and cosine of the angle of assent of the skater collision impulse
	float m_cos_skater_collision_assent;
	float m_sin_skater_collision_assent;

	// factor which adjusts the magnitude of skater collision impulses; sort of like an inverse mass
	float m_skater_collision_impulse_factor;
	
	// scales the rotation the object gains from a collision with the skater
	float m_skater_collision_rotation_factor;

	// if the velocities are below these points, the object may go to sleep
	float m_linear_velocity_sleep_point_sqr;
	float m_angular_velocity_sleep_point_sqr;

	// currently, we assume a box; the algorithm is general enough to do anything
	SContact* mp_contacts;
	unsigned short m_num_contacts;
	
	// center of mass of the object
	Mth::Vector m_center_of_mass;
	
	// offset from the center of mass to the origin of the model
	Mth::Vector m_model_offset;

	// number of frames after a skater collision before we begin checking for skater collisions again
	float m_ignore_skater_duration;
	
	// callback script names
	struct SScriptCallbackNames
	{
		uint32 collide;
		uint32 bounce;
		uint32 settle;
		uint32 stuck;
	} m_script_names;
	
	// parameter structure passed to callback scripts
	Script::CStruct* mp_script_params;
	
	// sound setup
	struct SSoundSetup
	{
		Script::CStruct* collide_sound;
		Script::CStruct* bounce_sound;
		Tmr::Time collide_mute_delay;
		Tmr::Time global_collide_mute_delay;
		float bounce_velocity_callback_threshold;
		float bounce_velocity_full_speed;
	} m_sound_setup;
	
	// time when the next collide sound is allowed
	Tmr::Time m_collide_sound_allowed_time;
	
	// time when the any next collide sound is allowed
	static Tmr::Time s_collide_sound_allowed_time;
	
	// used to allow dynamic updating of sounds
	#ifdef __NOPT_ASSERT__
	uint32 m_sound_type_id;
	#endif
	
	// property flags
	Flags<EFlagType> m_flags;
	
	// position at which we were last woke
	Mth::Vector m_wake_pos;
	
	// countdown to dying; not active if set to -1.0f
	float m_die_countdown;
	
	// primary state variables:

	// object's state; objects sleep when they are not active
	ERigidBodyStateType m_state;

	// object's center of mass position
	Mth::Vector m_pos;

	// object's velocity
	Mth::Vector m_vel;

	// object's orientation around center of mass
	Mth::Quat m_orientation;

	// object's angular velocity
	Mth::Vector m_rotvel;

	// dependent state variables:

	// rotation matrix based on our orientation
	Mth::Matrix m_matrix;

	// number of contacts in current collision
	char m_num_collisions;

	// count down used to time the interval between a skater collision and the point when we begin looking for skater collisions again
	float m_ignore_skater_countdown;

	// work variables:

	// collision cache; used to improve collision detection turn-around time; shared by all rigidbodies
	static Nx::CCollCache s_collision_cache;

	// longest distance between a contact point and the center of mass; used to generate the collision cache's bounding box
	float m_largest_contact_extent;

	// debug variables
	static bool s_debug_lines_on;
	static bool s_draw_skater_collision_circles;

	// height of the skater; used when determining skater collisions and resulting impulse point of application
	static float s_skater_head_height;
	
	// to save memory, the rigidbodies share an array of structures which hold the state of their contacts during collision resolution
	static SContactState sp_contact_states [ vRP_MAX_NUM_CONTACTS ];
	
	#ifdef __NOPT_DEBUG__
	// insures that only a single rigidbody is using sp_contact_states at a time
	static bool s_contact_states_in_use;
	#endif
	
	// peer components
	CSoundComponent* mp_sound_component;
};

}

#endif
