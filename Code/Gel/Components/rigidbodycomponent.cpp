//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       RigidBodyComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  1/22/3
//****************************************************************************

// An object component which dictates the motion of an object using an approximation of rigidbody physics.

#include <gel/components/rigidbodycomponent.h>
#include <gel/components/soundcomponent.h>

#include <core/math/matrix.h>

#include <gfx/nx.h>

#include <sk/engine/feeler.h>
#include <sk/modules/skate/skate.h>
#include <sk/objects/skater.h>
#include <sk/scripting/nodearray.h>

#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/collision/collcache.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/component.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/utils.h>

namespace Obj
{

Nx::CCollCache CRigidBodyComponent::s_collision_cache;
bool CRigidBodyComponent::s_debug_lines_on = false;
bool CRigidBodyComponent::s_draw_skater_collision_circles = false;
float CRigidBodyComponent::s_skater_head_height;
Tmr::Time CRigidBodyComponent::s_collide_sound_allowed_time = 0;
CRigidBodyComponent::SContactState CRigidBodyComponent::sp_contact_states [ CRigidBodyComponent::vRP_MAX_NUM_CONTACTS ];

#ifdef __NOPT_DEBUG__
bool CRigidBodyComponent::s_contact_states_in_use = false;
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CRigidBodyComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CRigidBodyComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CRigidBodyComponent::CRigidBodyComponent() : CBaseComponent()
{
	SetType( CRC_RIGIDBODY );

	mp_sound_component = NULL;
	
	// setup defaults
	
	m_const_acc = vRP_GRAVITATIONAL_ACCELERATION;
	m_mass_over_moment = vRP_DEFAULT_MASS_OVER_MOMENT;
	m_coeff_restitution = vRP_DEFAULT_COEFF_RESTITUTION;
	m_coeff_friction = vRP_DEFAULT_COEFF_FRICTION;
	m_spring_const = vRP_DEFAULT_SPRING_CONST;
	m_linear_velocity_sleep_point_sqr = vRP_DEFAULT_LINEAR_VELOCITY_SLEEP_POINT * vRP_DEFAULT_LINEAR_VELOCITY_SLEEP_POINT;
	m_angular_velocity_sleep_point_sqr = vRP_DEFAULT_ANGULAR_VELOCITY_SLEEP_POINT * vRP_DEFAULT_ANGULAR_VELOCITY_SLEEP_POINT;
	m_skater_collision_impulse_factor = vRP_DEFAULT_SKATER_COLLISION_IMPULSE_FACTOR;
	m_skater_collision_rotation_factor = vRP_DEFAULT_SKATER_COLLISION_ROTATION_FACTOR;
	m_skater_collision_radius = vRP_DEFAULT_SKATER_COLLISION_RADIUS;
	m_skater_collision_application_radius = vRP_DEFAULT_SKATER_COLLISION_APPLICATION_RADIUS;
	m_cos_skater_collision_assent = cosf(DEGREES_TO_RADIANS(vRP_DEFAULT_SKATER_COLLISION_ASSENT));
	m_sin_skater_collision_assent = sinf(DEGREES_TO_RADIANS(vRP_DEFAULT_SKATER_COLLISION_ASSENT));
	m_ignore_skater_duration = vRP_DEFAULT_IGNORE_SKATER_DURATION;
	m_model_offset.Set(0.0f, 0.0f, 0.0f);
	
	m_num_contacts = 0;
	
	m_script_names.collide = m_script_names.bounce = m_script_names.settle = m_script_names.stuck = 0;
	mp_script_params = NULL;
	
	m_sound_setup.collide_sound = m_sound_setup.bounce_sound = NULL;
	m_sound_setup.collide_mute_delay = vRP_DEFAULT_COLLIDE_MUTE_DELAY;
	m_sound_setup.global_collide_mute_delay = vRP_DEFAULT_GLOBAL_COLLIDE_MUTE_DELAY;
	m_sound_setup.bounce_velocity_callback_threshold = vRP_DEFAULT_BOUNCE_VELOCITY_CALLBACK_THRESHOLD;
	m_sound_setup.bounce_velocity_full_speed = vRP_DEFAULT_BOUNCE_VELOCITY_FULL_SPEED;
	
	m_collide_sound_allowed_time = 0;
	
	m_die_countdown = -1.0f;
	
	mp_contacts = NULL;
	
	#ifdef __NOPT_ASSERT__
	m_sound_type_id = 0;
	#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CRigidBodyComponent::~CRigidBodyComponent()
{
	delete [] mp_contacts;
	delete mp_script_params;
	delete m_sound_setup.bounce_sound;
	delete m_sound_setup.collide_sound;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::InitFromStructure( Script::CStruct* pParams )
{
	// NOTE: right now rigidbodies ignore moving collidables; if we want to change this, we should account for the moving collidables' velocity
	// in the collision resolution code
	// NOTE: probably want to add some small random rotation when the skater hits the object; two reasons; one, when you hit a line of cones, right
	// now it seems too orderly; two, when you push a cone ahead of you, the cone's dynamics should be more jerky and less smooth
	
	// NOTE: don't make any contacts within a foot of the center of mass or the object is in danger of falling through the ground

	// IDEA: for a skateboard rigidbody, have four of the contacts only feel friction parallel to the wheel's axis

	// potential optimization; collision detect only every other frame, unless the last-ditch feeler says to do it this frame

	m_state = ASLEEP;

	m_vel.Set(0.0f, 0.0f, 0.0f);
	m_rotvel.Set(0.0f, 0.0f, 0.0f);
	
	m_matrix.Ident();
	
	m_pos = GetObject()->GetPos();
	
	m_orientation = GetObject()->GetMatrix();
	m_orientation.Normalize();
	m_orientation.GetMatrix(m_matrix);
	
	pParams->GetVector(CRCD(0xc4c809e, "vel"), &m_vel);
	
	pParams->GetVector(CRCD(0xfb1a83b2, "rotvel"), &m_rotvel);

	pParams->GetVector(CRCD(0x737612c6, "center_of_mass"), &m_center_of_mass);

	pParams->GetFloat(CRCD(0x6fae5c9a, "const_acc"), &m_const_acc);

	pParams->GetFloat(CRCD(0x60b4860d, "coeff_restitution"), &m_coeff_restitution);

	pParams->GetFloat(CRCD(0xf9d75930, "coeff_friction"), &m_coeff_friction);
	
	pParams->GetFloat(CRCD(0xf1ecb30f, "spring_const"), &m_spring_const);

	if (pParams->ContainsComponentNamed(CRCD(0xc11e4dd8, "linear_velocity_sleep_point")))
	{
		pParams->GetFloat(CRCD(0xc11e4dd8, "linear_velocity_sleep_point"), &m_linear_velocity_sleep_point_sqr);
		m_linear_velocity_sleep_point_sqr *= m_linear_velocity_sleep_point_sqr;
	}

	if (pParams->ContainsComponentNamed(CRCD(0xa457ece3, "angular_velocity_sleep_point")))
	{
		pParams->GetFloat(CRCD(0xa457ece3, "angular_velocity_sleep_point"), &m_angular_velocity_sleep_point_sqr);
		m_angular_velocity_sleep_point_sqr *= m_angular_velocity_sleep_point_sqr;
	}

	pParams->GetFloat(CRCD(0x3390d1e5, "skater_collision_impulse_factor"), &m_skater_collision_impulse_factor);
	
	pParams->GetFloat(CRCD(0x5a18f61f, "skater_collision_rotation_factor"), &m_skater_collision_rotation_factor);

	pParams->GetFloat(CRCD(0x85f112c1, "skater_collision_radius"), &m_skater_collision_radius);

	pParams->GetFloat(CRCD(0x8ab2db68, "skater_collision_application_radius"), &m_skater_collision_application_radius);

	if (pParams->ContainsComponentNamed(CRCD(0xe9156e6d, "skater_collision_assent")))
	{
		float skater_collision_assent;
		pParams->GetFloat(CRCD(0xe9156e6d, "skater_collision_assent"), &skater_collision_assent);
		m_cos_skater_collision_assent = cosf(DEGREES_TO_RADIANS(skater_collision_assent));
		m_sin_skater_collision_assent = sinf(DEGREES_TO_RADIANS(skater_collision_assent));
	}

	pParams->GetFloat(CRCD(0x1f4d0ca5, "ignore_skater_duration"), &m_ignore_skater_duration);
	
	pParams->GetChecksum(CRCD(0x58cdc0f0, "CollideScript"), &m_script_names.collide);
	pParams->GetChecksum(CRCD(0x2d075f90, "BounceScript"), &m_script_names.bounce);
	pParams->GetChecksum(CRCD(0x571917fa, "SettleScript"), &m_script_names.settle);
	pParams->GetChecksum(CRCD(0xccb84047, "StuckScript"), &m_script_names.stuck);
	
	if (pParams->ContainsComponentNamed(CRCD(0xdd85bda, "CallbackParams")))
	{
		Script::CStruct* p_struct;
		pParams->GetStructure(CRCD(0xdd85bda, "CallbackParams"), &p_struct, Script::ASSERT);
		
		mp_script_params = new Script::CStruct;
		*mp_script_params += *p_struct;
	}
	
	// get sound setup
	if (pParams->ContainsComponentNamed(CRCD(0x94956e48, "SoundType")))
	{
		uint32 sound_type_id;
		pParams->GetChecksum(CRCD(0x94956e48, "SoundType"), &sound_type_id, Script::ASSERT);
		
		get_sound_setup(sound_type_id);
		
		#ifdef __NOPT_ASSERT__
		m_sound_type_id = sound_type_id;
		#endif
	}
	
	m_flags.Set(DIE_UPON_SLEEP, pParams->ContainsFlag(CRCD(0x5e51924f, "DieUponSettling")));

	// there are currently two ways to setup the contact points; either you pass them in as an array of offsets from the center of mass
	// or you specify a generic geometry and pass parameters to that geometry
	
	EObjectGeometryType object_geometry = NO_GEOMETRY;
	if (m_num_contacts == 0)
	{
		object_geometry = BOX;
	}
	
	if (pParams->ContainsFlag(CRCD(0xf756b7c5, "box")))
	{
		object_geometry = BOX;
	}
	else if (pParams->ContainsFlag(CRCD(0x1cb1a2b6, "pyramid")))
	{
		object_geometry	= PYRAMID;
	}
	else if (pParams->ContainsFlag(CRCD(0x64fba415, "cylinder")))
	{
		object_geometry	= CYLINDER;
	}
	else if (pParams->ContainsFlag(CRCD(0x20689278, "triangle")))
	{
		object_geometry	= TRIANGLE;
	}

	// setup contacts using an array
	if (pParams->ContainsComponentNamed(CRCD(0xccbfea8c, "contacts")))
	{
		Script::CArray* p_array;
		pParams->GetArray(CRCD(0xccbfea8c, "contacts"), &p_array);
		setup_contacts_from_array(p_array);
		object_geometry = NO_GEOMETRY;
	}
	
	// setup contacts using a generic geometry
	switch (object_geometry)
	{
		// a rectangular prism with contacts at each corner
		case BOX:
		{
			Mth::Vector top_half_dimensions(32.0f, 32.0f, 16.0f);
			Mth::Vector bottom_half_dimensions;

			pParams->GetVector("dimensions", &top_half_dimensions);
			bottom_half_dimensions = top_half_dimensions;
			pParams->GetVector("top_dimensions", &top_half_dimensions);
			pParams->GetVector("bottom_dimensions", &bottom_half_dimensions);
			top_half_dimensions /= 2.0f;
			bottom_half_dimensions /= 2.0f;

			setup_contacts_as_box(top_half_dimensions, bottom_half_dimensions);
			
			break;
		}

		// a pyramid with contacts at each corner along the bottom and one on the top
		case PYRAMID:
		{
			float half_height = 42.0f;
			float half_depth = 32.0f;
			
			pParams->GetFloat(CRCD(0xab21af0,"height"), &half_height);
			pParams->GetFloat(CRCD(0x55ce396,"depth"), &half_depth);
			half_height /= 2.0f;
			half_depth /= 2.0f;
			
			setup_contacts_as_pyramid(half_height, half_depth);
			
			break;
		}

		// a cylinder with six contacts around the top and bottom
		case CYLINDER:
		{
			float top_radius = 12.0f;
			float bottom_radius;
			float half_height = 60.0f;
			int edges = 6;
			
			pParams->GetFloat(CRCD(0xc48391a5,"radius"), &top_radius);
			bottom_radius = top_radius;
			
			pParams->GetFloat(CRCD(0x937e3b29,"top_radius"), &top_radius);
			pParams->GetFloat(CRCD(0xe2ac8c3a,"bottom_radius"), &bottom_radius);
			pParams->GetFloat(CRCD(0xab21af0,"height"), &half_height);
			pParams->GetInteger(CRCD(0x4055f24a,"edges"), &edges);
			half_height /= 2.0f;
			
			setup_contacts_as_cylinder(top_radius, bottom_radius, half_height, edges);
			
			break;
		}

		// a triangle with three contacts on the left and right
		case TRIANGLE:
		{
			Mth::Vector half_dimensions(32.0f, 32.0f, 16.0f);
			
			pParams->GetVector(CRCD(0x1d82745a, "dimensions"), &half_dimensions);
			half_dimensions /= 2.0f;
			
			setup_contacts_as_triangle(half_dimensions);
	
			break;
		}
		
		// don't use a generic geometry
		case NO_GEOMETRY:
			Dbg_MsgAssert(m_num_contacts, ("Contact geometry not setup for rigidbody"));
			break;
	}
	
	// setup directional friction
	if (pParams->ContainsComponentNamed(CRCD(0xc35cac0d, "directed_friction")))
	{
		Script::CArray* p_array;
		pParams->GetArray(CRCD(0xc35cac0d, "directed_friction"), &p_array);
		
		Dbg_MsgAssert(p_array->GetSize() == m_num_contacts, ("Array of directed friction specifications must equal the number of contacts"));
		
		for (int n = p_array->GetSize(); n--; )
		{
			Script::CStruct* m_struct = p_array->GetStructure(n);
			if (m_struct->ContainsComponentNamed(CRCD(0x806fff30, "none")))
			{
				mp_contacts[n].directed_friction = false;
			}
			else
			{
				mp_contacts[n].directed_friction = true;
				m_struct->GetVector(NO_NAME, &mp_contacts[n].friction_direction, Script::ASSERT);
			}
		}
	}
			 	
	// adjust contacts based on extra angles
	if (pParams->ContainsComponentNamed(CRCD(0x17b93077, "rotate_contacts")))
	{
		Mth::Vector angles;
		pParams->GetVector(CRCD(0x17b93077, "rotate_contacts"), &angles, Script::ASSERT);
		Mth::Matrix matrix;
		matrix.SetFromAngles(angles);
		
		for (int n = m_num_contacts; n--; )
		{
			mp_contacts[n].p = matrix.Rotate(mp_contacts[n].p);
		}
				 }

	// adjust contacts based on node array angles
	if (pParams->ContainsComponentNamed(CRCD(0x9d2d0915, "Angles")))
	{
		Mth::Vector angles;
		SkateScript::GetAngles(pParams, &angles);
		Mth::Matrix matrix;
		matrix.SetFromAngles(angles);
		
		for (int n = m_num_contacts; n--; )
		{
			mp_contacts[n].p = matrix.Rotate(mp_contacts[n].p);
		}
	}
	
	// setup model offset
	Mth::Vector center_of_mass(0.0f, 0.0f, 0.0f);
	if (pParams->ContainsComponentNamed(CRCD(0x737612c6, "center_of_mass")))
	{
		if (pParams->ContainsComponentNamed(CRCD(0x737612c6, "center_of_mass")))
		{
			pParams->GetVector(CRCD(0x737612c6, "center_of_mass"), &center_of_mass);
		}
	}
		
	// if not set (or set to default), calculate the center of mass
	if (center_of_mass[X] == 0.0f && center_of_mass[Y] == 0.0f && center_of_mass[Z] == 0.0f);
	{
		for (int n = m_num_contacts; n--; )
		{
			center_of_mass += mp_contacts[n].p;
		}
		center_of_mass *= (1.0f / m_num_contacts);
	}
	
	m_model_offset = -center_of_mass;
	for (int n = m_num_contacts; n--; )
	{
		mp_contacts[n].p -= center_of_mass;
	}
	
	
	if (!pParams->GetFloat(CRCD(0x967cd3ae, "mass_over_moment"), &m_mass_over_moment))
	{
		// calculate an appropriate mass over moment adjustment; otherwise, you can get some really weird behavior
		float max_dist = 0.0f;
		for (int n = m_num_contacts; n--; )
		{
			max_dist = Mth::Max(max_dist, mp_contacts[n].p.Length());
		}
		if (max_dist > 30.0f)
		{
			m_mass_over_moment /= Mth::Sqr((max_dist / 30.0f));
		}
	}
	
	// setup static variables
	s_skater_head_height = GetPhysicsFloat(CRCD(0x542cf0c7, "Skater_default_head_height"));

	// determine the largest feeler extent; used with the collision cache system
	m_largest_contact_extent = 0.0f;
	for (int n = m_num_contacts; n--; )
	{
		SContact& contact = mp_contacts[n];

		float contact_extent_sqr = contact.p.LengthSqr();
		if (contact_extent_sqr > m_largest_contact_extent)
		{
			m_largest_contact_extent = contact_extent_sqr;
		}
	}
	m_largest_contact_extent = sqrtf(m_largest_contact_extent) + 1.0f;

	m_ignore_skater_countdown = 0.0f;
	
	m_pos -= m_matrix.Rotate(m_model_offset);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::Finalize()
{
	mp_sound_component = GetSoundComponentFromObject(GetObject());
	
	Dbg_Assert(mp_sound_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::Update()
{
	#ifdef __NOPT_DEBUG__
	Dbg_MsgAssert(!s_contact_states_in_use, ("Two CRigidBodyComponents updating simultaneously"));
	s_contact_states_in_use = true;
	#endif
	
	float full_time_step = Tmr::FrameLength();
	
	setup_contact_states();
	
	// only check for skater collisions if we haven't collided with the skater recently; this insures that the stupid skater doesn't cram us
	// into corners and the like
	if (m_ignore_skater_countdown > 0.0f)
	{
		m_ignore_skater_countdown -= full_time_step;
	}
	else if (!m_flags.Test(PLAYER_COLLISION_DISABLED))
	{
		handle_skater_collisions();
	}
	
	if (m_die_countdown >= 0.0f)
	{
		if (m_die_countdown <= full_time_step)
		{
			GetObject()->MarkAsDead();
		}
		else
		{
			m_die_countdown -= full_time_step;
		}
	}
	
	// if we are asleep
	if (m_state == ASLEEP)
	{
		GetObject()->SetDisplayMatrix(m_matrix);

		if (s_debug_lines_on)
		{
			draw_debug_lines();
		}

		// do just about nothing
			
		#ifdef __NOPT_DEBUG__
		s_contact_states_in_use = false;
		#endif

		return;
	}

	int main_update_loop_count = 0;
	bool last_ditch_collision = false;
	float remaining_time_step = full_time_step;
	do {
		float time_step = remaining_time_step;

		// send a feeler from object's center of mass to our new center of mass; most collisions will not be caught in this manner;
		// this is a last-ditch feeler to insure that we don't fly through objects at high speeds
		CFeeler feeler;
		feeler.m_start = m_pos;
		feeler.m_end = m_pos + time_step * m_vel;
		feeler.m_end[Y] += 0.5f * time_step * time_step * m_const_acc;

		last_ditch_collision = feeler.GetCollision(false);
	
		// if we have such a collision, move only to it
		if (last_ditch_collision)
		{
			time_step = (feeler.GetDist() * feeler.Length() - 6.0f) / m_vel.Length();
			if (time_step < 0.0f)
			{
				time_step = 0.0f;
			}
		}
	
		update_dynamic_state(time_step);

		// we do a very simple collision
		if (last_ditch_collision)
		{
			m_vel -= (1.0f + m_coeff_restitution) * Mth::DotProduct(m_vel, feeler.GetNormal()) * feeler.GetNormal();
		}

		remaining_time_step -= time_step;

		// make sure we're not in an infinite loop; a weirdly hanging box is better than a frozen game
		if (++main_update_loop_count == 5)
		{
			MESSAGE("too many last-ditch contacts in a single frame; going to sleep");
			sleep();
			break;
		}
	} while (last_ditch_collision);
	
	if (detect_collisions())
	{
		resolve_collisions();
	}

	consider_sleeping();

	// if we've gotten here, we need to update the object
	
	GetObject()->SetPos(m_pos + m_matrix.Rotate(m_model_offset));
	
	GetObject()->SetMatrix(m_matrix);
	GetObject()->SetDisplayMatrix(m_matrix);
	
	GetObject()->SetVel(m_vel);

	if (s_debug_lines_on)
	{
		draw_debug_lines();
	}
	
	#ifdef __NOPT_DEBUG__
	s_contact_states_in_use = false;
	#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CRigidBodyComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		// @script | RigidBody_IgnoreSkater | rigidbody ignores the skater for a given duration
        // @uparm 0.0 | duration (defaults to seconds)
        // @flag frames | time is given in frames
		case CRCC(0xcc12cf87, "RigidBody_IgnoreSkater"):
		{
			pParams->GetFloat(NO_NAME, &m_ignore_skater_countdown, Script::ASSERT);
			if (pParams->ContainsFlag(CRCD(0x19176c5, "frames")) || pParams->ContainsFlag(CRCD(0x4a07c332, "frame")))
			{
				m_ignore_skater_countdown *= (1.0f / 60.0f);
			}
			break;
		}
		
		// @script | RigidBody_Wake | wakes up the rigidbody
		case CRCC(0x9599c10f, "RigidBody_Wake"):
			wake();
			break;

		// @script | RigidBody_Sleep | puts the rigidbody to sleep
		case CRCC(0xcd5ba67b, "RigidBody_Sleep"):
			sleep();
			break;
		
		// @script | RigidBody_Kick | wakes the rigidbody with a kick
		case CRCC(0xae24df9f, "RigidBody_Kick"):
		{
			Mth::Vector v;
			if (pParams->GetVector(CRCD(0xc4c809e, "vel"), &v))
			{
				GetObject()->SetVel(m_vel += v);
			}
			if (pParams->GetVector(CRCD(0xfb1a83b2, "rotvel"), &v))
			{
				m_rotvel += v;
			}
			if (pParams->GetVector(CRCD(0x7f261953, "pos"), &v))
			{
				GetObject()->SetPos(m_pos += v);
			}
			wake();
			break;
		}

		// @script : RigidBody_Reset | reset any parameters of the rigidbody
		case CRCC(0x92f5db9a, "RigidBody_Reset"):
			InitFromStructure(pParams);
			break;
			
		// @script : RigidBody_DisablePlayerCollision
		case CRCC(0xa2bbb3a, "RigidBody_DisablePlayerCollision"):
			m_flags.Set(PLAYER_COLLISION_DISABLED);
			break;
		
		// @script : RigidBody_EnablePlayerCollision
		case CRCC(0x49f44585, "RigidBody_EnablePlayerCollision"):
			m_flags.Clear(PLAYER_COLLISION_DISABLED);
			break;
			
		// @script : RigidBody_EnablePlayerCollision
		case CRCC(0x949be8a9, "RigidBody_MatchVelocityTo"):
		{
			Script::CComponent* p_component = pParams->GetNextComponent(NULL);
			Dbg_MsgAssert(p_component->mType == ESYMBOLTYPE_NAME, ("RigidBody_MatchVelocityTo requires an object name as its first parameter"));
			CCompositeObject* p_composite_object = static_cast< CCompositeObject* >(CCompositeObjectManager::Instance()->GetObjectByID(p_component->mChecksum));
			Dbg_MsgAssert(p_composite_object, ("RigidBody_MatchVelocityTo requires a composite object name as its first parameter"));
			
			bool apply_random_adjustment = pParams->ContainsFlag(CRCD(0x8ace8a0f, "ApplyRandomAdjustment"));
			
			m_vel = p_composite_object->m_vel;
			if (apply_random_adjustment)
			{
				m_vel *= 1.0f + Mth::PlusOrMinus(0.4f);
			}
			GetObject()->SetVel(m_vel);
			
			if (m_state == ASLEEP && Mth::Abs(m_vel[Y]) < 1.0f)
			{
				m_vel[Y] = 1.0f;
			}
			
			// use the rotational velocity also, if it's a rigidbody
			CRigidBodyComponent* p_rigid_body_component = GetRigidBodyComponentFromObject(p_composite_object);
			if (p_rigid_body_component)
			{
				m_rotvel = p_rigid_body_component->m_rotvel;
				if (apply_random_adjustment)
				{
					m_rotvel *= 1.0f * Mth::PlusOrMinus(0.4f);
				}
			}
			
			wake();
			
			break;
		}
		
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info, ("NULL p_info sent to CRigidBodyComponent::GetDebugInfo"));

	uint32 state_checksums[] =
	{
		CRCD(0x4484b712, "ASLEEP"), CRCD(0x99a34896, "AWAKE")
	};
	p_info->AddChecksum("m_state", state_checksums[m_state]);

	p_info->AddFloat("m_orientation", m_orientation.GetScalar());
	p_info->AddVector("m_orientation", m_orientation.GetVector());

	p_info->AddVector("m_vel", m_vel);

	p_info->AddVector("m_rotvel", m_rotvel);

	p_info->AddFloat("m_const_acc", m_const_acc);

	p_info->AddFloat("m_mass_over_moment", m_mass_over_moment);

	p_info->AddFloat("m_coeff_restitution", m_coeff_restitution);

	p_info->AddFloat("m_coeff_friction", m_coeff_friction);
	
	p_info->AddFloat("m_spring_const", m_spring_const);

	p_info->AddFloat("m_skater_collision_impulse_factor", m_skater_collision_impulse_factor);
	
	p_info->AddFloat("m_skater_collision_rotation_factor", m_skater_collision_rotation_factor);
	
	p_info->AddFloat("m_skater_collision_radius", m_skater_collision_radius);

    p_info->AddFloat("m_num_collisions", m_num_collisions);
	
	p_info->AddVector("m_model_offset", m_model_offset);
	
	p_info->AddFloat("CollideMuteDelay", m_sound_setup.collide_mute_delay);
	
	p_info->AddFloat("GlobalCollideMuteDelay", m_sound_setup.global_collide_mute_delay);
	
	p_info->AddFloat("BounceVelocityCallbackThreshold", m_sound_setup.bounce_velocity_callback_threshold);
	
	p_info->AddFloat("BounceVelocityFullSpeed", m_sound_setup.bounce_velocity_full_speed);
	
	Script::CArray* p_array = new Script::CArray;
	p_array->SetSizeAndType(m_num_contacts, ESYMBOLTYPE_VECTOR);
	for (int n = m_num_contacts; n--; )
	{
		Script::CVector* p_vector = new Script::CVector;
		p_vector->mX = mp_contacts[n].p[X];
		p_vector->mY = mp_contacts[n].p[Y];
		p_vector->mZ = mp_contacts[n].p[Z];
		p_array->SetVector(n, p_vector);
	}
	p_info->AddArrayPointer("m_contacts", p_array);
	
	p_info->AddChecksum("CollideScript", m_script_names.collide);
	p_info->AddChecksum("BounceScript", m_script_names.bounce);
	p_info->AddChecksum("SettleScript", m_script_names.settle);
	p_info->AddChecksum("StuckScript", m_script_names.stuck);
	
	if (mp_script_params)
	{
		p_info->AddStructure("CallbackParams", mp_script_params);
	}

	p_info->AddChecksum("DieUponSettling", m_flags.Test(DIE_UPON_SLEEP) ? CRCD(0x203b372, "true") : CRCD(0xd43297cf, "false"));
	p_info->AddChecksum("PlayerCollisionDisabled", m_flags.Test(PLAYER_COLLISION_DISABLED) ? CRCD(0x203b372, "true") : CRCD(0xd43297cf, "false"));
	
	if (m_sound_setup.bounce_sound)
	{
		Script::CStruct* p_struct = new Script::CStruct;
		*p_struct += *m_sound_setup.bounce_sound;
		p_info->AddStructurePointer(CRCD(0x1fb2f60c, "BounceSound"), p_struct);
	}
	if (m_sound_setup.collide_sound)
	{
		Script::CStruct* p_struct = new Script::CStruct;
		*p_struct += *m_sound_setup.collide_sound;
		p_info->AddStructurePointer(CRCD(0xbf8e0ace, "CollideSound"), p_struct);
	}
	
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::sToggleDrawRigidBodyDebugLines (   )
{
	if (s_debug_lines_on)
	{
		/*
		if (s_draw_skater_collision_circles)
		{
			s_debug_lines_on = s_draw_skater_collision_circles = false;
		}
		else
		{
			s_draw_skater_collision_circles = true;
		}
		*/
		s_debug_lines_on = false;
	}
	else
	{
		s_debug_lines_on = true;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::setup_contact_states (   )
{
	for (int n = m_num_contacts; n--; )
	{
		sp_contact_states[n].p_world = m_matrix.Rotate(mp_contacts[n].p);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::handle_skater_collisions (   )
{
	for (int n = Mdl::Skate::Instance()->GetNumSkaters(); n--; )
	{
		CSkater& skater = *Mdl::Skate::Instance()->GetSkater(n);

		float radius_boost = skater.GetRigidBodyCollisionRadiusBoost();
		
		// detect collisions within a cylinder of the skater; if objects end up having significant extent in some directions and not others,
		// this detection code may have to become more sophisticated

		// check distance to the skater in the X-Z plane
		float distance_sqr = (m_pos[X] - skater.GetPos()[X]) * (m_pos[X] - skater.GetPos()[X])
			+ (m_pos[Z] - skater.GetPos()[Z]) * (m_pos[Z] - skater.GetPos()[Z]);
		if (distance_sqr > Mth::Sqr(m_skater_collision_radius + radius_boost)) continue;

		// check Y-offset to the skater
		float y_offset = skater.GetPos()[Y] - m_pos[Y];
		if (y_offset > m_skater_collision_application_radius + radius_boost
			|| y_offset < -(m_skater_collision_application_radius + radius_boost + s_skater_head_height)) continue;

		// apply the skater collision impulse; the impulse occurs at a slight assent and in a direction halfway between of the skater's
		// velocity and the line to the object; the magnitude is roughly proportional to the component of the skater's velocity along the
		// line to the object; the impulse will be applied a few inches above the skater's lowest point, along the line between the object
		// and the skater at distance equal to the collision radius of the object

		// unit vector pointing from the skater to the object in the X-Z plane
		Mth::Vector r = m_pos;
		r -= skater.GetPos();
		// r[Y] = 0.0f; // optimized out as r[Y] never used
		float eff_length = sqrtf(r[X] * r[X] + r[Z] * r[Z]);
		r[X] /= eff_length;
		r[Z] /= eff_length;
		
		// skater's relative velocity in the X-Z plane
		Mth::Vector skater_vel = skater.GetVel();
		skater_vel -= m_vel;
		skater_vel[Y] = 0.0f;

		if(( fabsf( skater_vel[X] ) < 0.01f ) && ( fabsf( skater_vel[Z] ) < 0.01f ))
		{
			// Small enough not to worry about - smaller can cause NaN decomposition.
			return;
		}
		
		// unadjusted impulse magnitude
		float magnitude = skater_vel[X] * r[X] + skater_vel[Z] * r[Z];
		if (magnitude <= 0.0f) continue;

		// we adjust the impulse magnitude so the object will be vaulted in front of the skater so as to give the player a nice view of the object's
		// dynamics; also, at low skater velocities, we want only a very small impulse

		// fast collisions
		if (magnitude > (2.0f * vRP_SKATER_COLLISION_VELOCITY_THRESHOLD_INCREMENT))
		{
			// increase the impulse by a constant
			magnitude += ((1.0f + vRP_SKATER_COLLISION_VELOCITY_THRESHOLD_INCREMENT) * vRP_SKATER_COLLISION_VELOCITY_FACTOR);
		}
		// medium collisions
		else if (magnitude > vRP_SKATER_COLLISION_VELOCITY_THRESHOLD_INCREMENT) {
			// connect the functions continuously
			magnitude += (vRP_SKATER_COLLISION_VELOCITY_FACTOR * vRP_SKATER_COLLISION_VELOCITY_THRESHOLD_INCREMENT)
				+ (magnitude - vRP_SKATER_COLLISION_VELOCITY_THRESHOLD_INCREMENT);
		}
		else
		// slow collisions
		{
			// increase the impulse by a factor
			magnitude *= (1.0f + vRP_SKATER_COLLISION_VELOCITY_FACTOR);
		}

		// impulse direction in the X-Z plane is the average of the skater velocity and r directions
		// we know the Y component for both is zero
		Mth::Vector impulse = skater_vel;
		eff_length = sqrtf(impulse[X] * impulse[X] + impulse[Z] * impulse[Z]);
		impulse[X] = impulse[X] / eff_length + r[X];
		impulse[Z] = impulse[Z] / eff_length + r[Z];
		eff_length = sqrtf(impulse[X] * impulse[X] + impulse[Z] * impulse[Z]);
		impulse[X] = impulse[X] / eff_length;
		impulse[Z] = impulse[Z] / eff_length;

		// adjust the impulse direction upwards
		impulse[X] *= m_cos_skater_collision_assent;
		impulse[Y] = m_sin_skater_collision_assent;
		impulse[Z] *= m_cos_skater_collision_assent;

		// factor in the magnitude
		impulse *= m_skater_collision_impulse_factor * magnitude;

		// the impulse's point of application in the XZ-plane is along the line between the skater and the object at the object's impulse application
		// radius

		// we calculate the point of application with respect to the center of mass
        Mth::Vector application_point = skater.GetPos();
		application_point[X] -= m_pos[X];
		application_point[Y] = 0.0f;
		application_point[Z] -= m_pos[Z];
		application_point.Normalize(m_skater_collision_application_radius);

		// calculate the Y-component of the point of application
		if (y_offset > 0.0f)
		{
			// collide with the skater's feet
			application_point[Y] = skater.GetPos()[Y] - m_pos[Y];
		}
		else if (y_offset < -s_skater_head_height)
		{
			// collide with the skater's head
			application_point[Y] = skater.GetPos()[Y] + s_skater_head_height - m_pos[Y];
		}
		else
		{
			// collide with the skater's body; use an offset from the center of mass to cause rotation
			application_point[Y] = -12.0f;
		}

		// apply the impulse; artifically reduce the initial rotation; it looks nice when the objects tumbles more after their first bounce
		m_vel += impulse;
		m_rotvel += m_skater_collision_rotation_factor * 0.6f * m_mass_over_moment * Mth::CrossProduct(application_point, impulse);

		wake();

		// start a countdown; while this countdown is complete, we will ignore the skater
		m_ignore_skater_countdown = m_ignore_skater_duration;
		
		// call the bounce script callback
		if (m_script_names.collide)
		{
			GetObject()->SpawnScriptPlease(m_script_names.collide, mp_script_params)->Update();
		}
		
		// kill the object's shadow
		if (Nx::CSector *p_shadow_sector = Nx::CEngine::sGetMainScene()->GetSector(Crc::ExtendCRCWithString(GetObject()->GetID(), "_Shadow")))
		{
			p_shadow_sector->SetActive(false);
		}
		
		Tmr::Time time = Tmr::GetTime();
		if ((int) (time - m_collide_sound_allowed_time) > 0 && (int) (time - s_collide_sound_allowed_time) > 0)
		{
			#ifdef __NOPT_ASSERT__
			if (m_sound_type_id && Script::GetInteger(CRCD(0xd634a297, "DynamicRigidbodySounds")))
			{
				get_sound_setup(m_sound_type_id);
			}
			#endif
			
			if (m_sound_setup.collide_sound)
			{
				float percent = (100.0f / 1000.0f) * impulse.Length();
				m_sound_setup.collide_sound->AddFloat(CRCD(0x9e497fc6, "Percent"), percent);
				mp_sound_component->PlayScriptedSound(m_sound_setup.collide_sound);
				
				// delay the next collision sound
				m_collide_sound_allowed_time = Tmr::GetTime() + (50 + Mth::Rnd(100)) * m_sound_setup.collide_mute_delay / 100;
				s_collide_sound_allowed_time = Tmr::GetTime() + (50 + Mth::Rnd(100)) * m_sound_setup.global_collide_mute_delay / 100;
			}
		}

		// collide with only a single skater
		break;
	} // END loop over skaters
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::update_dynamic_state ( float time_step )
{
	// a massively shoe-string handling of contact forces
	if (m_num_collisions < 3)
	{
		Mth::Vector delta_pos = m_vel;
		delta_pos[Y] += 0.5f * time_step * m_const_acc;
		delta_pos *= time_step;
		m_pos += delta_pos;

		m_vel[Y] += time_step * m_const_acc;
	}
	else
	{
		m_pos += time_step * m_vel;
	}

	Mth::Quat delta_orientation = m_orientation;
	delta_orientation *= Mth::Quat(-0.5f * m_rotvel[X], -0.5f * m_rotvel[Y], -0.5f * m_rotvel[Z], 0.0f);
	delta_orientation *= time_step;
	m_orientation += delta_orientation;

	m_orientation.Normalize();

	m_orientation.GetMatrix(m_matrix);

	for (int n = m_num_contacts; n--; )
	{
		sp_contact_states[n].p_world = m_matrix.Rotate(mp_contacts[n].p);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::consider_sleeping (   )
{
	// it is possible that we may want to check these conditions over a period of several frames to insure that sleeping is warranted

	// only sleep if we're experiencing three or more collisions; hopefully, a surface-to-surface contact
	if (m_num_collisions < 3) return;

	// only sleep if we're moving slow
	if (m_vel.LengthSqr() > m_linear_velocity_sleep_point_sqr) return;

	// only sleep if we're moving slow
	if (m_rotvel.LengthSqr() > m_angular_velocity_sleep_point_sqr) return;

	sleep();
	
	// if we're not suppose to when going to sleep
	if (!m_flags.Test(DIE_UPON_SLEEP)) return;
	
	// only die if we've moved substantially from our starting point
	if ((m_pos - m_wake_pos).LengthSqr() < 48.0f * 48.0f) return;
	
	m_die_countdown = 1.5f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::wake (   )
{
	if (m_state != AWAKE)
	{
		m_wake_pos = GetObject()->GetPos();
		m_state = AWAKE;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::sleep (   )
{
	if (m_script_names.settle)
	{
		GetObject()->SpawnScriptPlease(m_script_names.settle, mp_script_params)->Update();
	}
	
	m_vel.Set(0.0f, 0.0f, 0.0f);
	m_rotvel.Set(0.0f, 0.0f, 0.0f);
	m_state = ASLEEP;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CRigidBodyComponent::detect_collisions (   )
{
	CFeeler feeler;

	// set up a bounding box around the space within which all collision detection will occur
	Mth::CBBox bounding_box(m_pos - Mth::Vector(m_largest_contact_extent, m_largest_contact_extent, m_largest_contact_extent),
		m_pos + Mth::Vector(m_largest_contact_extent, m_largest_contact_extent, m_largest_contact_extent));
	s_collision_cache.Update(bounding_box);
	feeler.SetCache(&s_collision_cache);

	// loop over the contact points
	m_num_collisions = 0;
	for (int n = m_num_contacts; n--; )
	{
		SContactState& contact_state = sp_contact_states[n];

		// run a feeler from the object's center to the contact point
		feeler.m_start = m_pos;
		feeler.m_end = m_pos;
		feeler.m_end += contact_state.p_world;

		contact_state.collision = feeler.GetCollision(false);

		if (!contact_state.collision) continue;

		contact_state.normal = feeler.GetNormal();
		contact_state.normal_eff_mass_set = false;
		contact_state.normal_impulse_magnitude = 0.0f;
		
		contact_state.depth = Mth::DotProduct(contact_state.normal, (1.0f - feeler.GetDist()) * (feeler.m_end - feeler.m_start));
		contact_state.collision = contact_state.depth <= 0.0f;
		
		if (contact_state.collision)
		{
			m_num_collisions++;
		}
	}

	return m_num_collisions != 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CRigidBodyComponent::calculate_effective_mass ( const Mth::Vector& direction, const SContactState& contact_state ) const
{
	// calculated the effective mass of the object at a contact for forces in a given direction

	Mth::Vector delta_rotvel = Mth::CrossProduct(contact_state.p_world, direction);
	delta_rotvel *= m_mass_over_moment;
	return 1.0f / Mth::DotProduct(direction, direction + Mth::CrossProduct(delta_rotvel, contact_state.p_world));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::resolve_collisions (   )
{
	// collision resolution code of my own devise; simplified and not wholly accurate, it is good enough because we only have a single object
	// colliding with a (mostly) static background; multiple collisions are not resolved simultaneously; instead, an iterative procedure is used to
	// resolve the normal forces; this can cause objects which are colliding face-to-face with a surface to bounce off with odd rotation,
	// as the required collision impulse will not be spread evenly across the contacts (as would be correct); also, friction is not handled in closed
	// form; instead, after the collision is resolved and the normal forces are determined, we go back in and apply frictional forces (using the
	// velocity of the object before the normal forces are applied; this generates friction which is incorrect, but visually reasonable; the friction
	// forces could ruin the resolution of the collision, but we just ignore that possibility, and leave the problem to be fixed on the next frame

	// cache the velocity and use it when determining the frictional forces
	Mth::Vector cache_vel = m_vel;
	Mth::Vector cache_rotvel = m_rotvel;
	
	// maximum collision velocity; used to determine if a bounce callback is warranted
	float max_collision_velocity = 0.0f;

	// loop until the collisions are fully resolved
	int collision_resolution_pass_count = 0;
	bool clean_pass;
	do {
		clean_pass = true;

		// loop over the collision points
		for (int n = m_num_contacts; n--; )
		{
			SContactState& contact_state = sp_contact_states[n];
			if (!contact_state.collision) continue;
	
			// calculate the normal velocity at the collision point
			Mth::Vector vel = m_vel;
			vel += Mth::CrossProduct(m_rotvel, contact_state.p_world);
			float normal_vel = Mth::DotProduct(contact_state.normal, vel);
	
			// skip if we're not colliding
			if (normal_vel > 0.0f) continue;
			clean_pass = false;
			
			if (-normal_vel > max_collision_velocity)
			{
				max_collision_velocity = -normal_vel;
			}
	
			// calculate the goal velocity
			float goal_normal_vel = -m_coeff_restitution * normal_vel;

			// calculate normal effective mass
			if (!contact_state.normal_eff_mass_set)
			{
				contact_state.normal_eff_mass = calculate_effective_mass(contact_state.normal, contact_state);
				contact_state.normal_eff_mass_set = true;
			}
	
			// calculate the normal impulse required to reach the goal velocity
			float normal_impulse = (goal_normal_vel - normal_vel) * contact_state.normal_eff_mass;

			// calculate normal impulse
			Mth::Vector impulse = contact_state.normal;
			impulse *= normal_impulse;

			// accumulate the total normal impulse applied at his contact
			contact_state.normal_impulse_magnitude += normal_impulse;

			// apply the normal impulse
			m_vel += impulse;
			m_rotvel += m_mass_over_moment * Mth::CrossProduct(contact_state.p_world, impulse);
		} // END loop over collision points

		if (++collision_resolution_pass_count == 20)
		{
			Dbg_Message("CRigidBodyComponent::resolve_collisions: unresolveable collision; zeroing velocity");
			// call the sleep script callback
			if (m_script_names.stuck)
			{
				GetObject()->SpawnScriptPlease(m_script_names.stuck, mp_script_params)->Update();
			}
			else
			{
				m_vel.Set(0.0f, 0.0f, 0.0f);
				m_rotvel.Set(0.0f, 0.0f, 0.0f);
			}
			return;
		}
		
		// if there's only one collision point, we need to make only a single pass
		if (m_num_contacts == 1) break;
	} while (!clean_pass);
	// END loop until collision is resolved

	// we've resolved the collision; now calculate and apply the friction using the cached velocities

	// loop over the collision points
	for (int n = m_num_contacts; n--; )
	{
		SContact& contact = mp_contacts[n];
		SContactState& contact_state = sp_contact_states[n];
		if (!contact_state.collision) continue;

		// calculate the tangent direction and velocity
		Mth::Vector vel = cache_vel;
		vel += Mth::CrossProduct(cache_rotvel, contact_state.p_world);
		float normal_vel = Mth::DotProduct(contact_state.normal, vel);
		Mth::Vector tangent = vel + -normal_vel * contact_state.normal;
		
		if (contact.directed_friction)
		{
			Mth::Vector friction_direction_world = m_matrix.Rotate(contact.friction_direction);
			friction_direction_world -= Mth::DotProduct(contact_state.normal, friction_direction_world) * contact_state.normal;
			
			float length = friction_direction_world.Length();
			if (length > 0.001f)
			{
				friction_direction_world *= (1.0f / length);
				tangent.ProjectToNormal(friction_direction_world);
			}
		}
		
		float tangent_vel = tangent.Length();

		// if the tangential velocity is too small, it's direction will be meaningless anyway
		if (tangent_vel < 0.001f) continue;

		// normalize by hand
		tangent /= tangent_vel;

		float tangent_eff_mass = calculate_effective_mass(tangent, contact_state);

		// calculate the tangential impulse required to stop the tangential	velocity
		float tangent_impulse = tangent_vel * tangent_eff_mass;

		// the frictional impulse is only allowed to be so big
		float max_tangent_impulse = m_coeff_friction * contact_state.normal_impulse_magnitude;
		if (tangent_impulse > max_tangent_impulse)
		{
			tangent_impulse = max_tangent_impulse;
		}
		
		// calculate the tangent impulse
		Mth::Vector impulse = tangent;
		impulse *= -tangent_impulse;
		
		if (s_draw_skater_collision_circles)
		{
			if (contact.directed_friction)
			{
				Gfx::AddDebugLine(contact_state.p_world + m_pos + Mth::Vector(0.0f, 1.0f, 0.0f), contact_state.p_world + m_pos + 6.0f * impulse + Mth::Vector(0.0f, 1.0f, 0.0f), MAKE_RGB(255, 255, 0), MAKE_RGB(255, 255, 0), 1);
			}
			else
			{
				Gfx::AddDebugLine(contact_state.p_world + m_pos + Mth::Vector(0.0f, 1.0f, 0.0f), contact_state.p_world + m_pos + 6.0f * impulse + Mth::Vector(0.0f, 1.0f, 0.0f), MAKE_RGB(255, 0, 255), MAKE_RGB(255, 0, 255), 1);
			}
		}

		// apply the tangent impulse to both the true velocity and the velocity we are using to determine the frictional forces

		Mth::Vector delta_rotvel = m_mass_over_moment * Mth::CrossProduct(contact_state.p_world, impulse);

		m_vel += impulse;
		m_rotvel += delta_rotvel;

		cache_vel += impulse;
		cache_rotvel += delta_rotvel;
	} // END loop over collision points
	
	// apply penalty forces to prevent interpenetration
	
	float time_step = Tmr::FrameLength();
	
	for (int n = m_num_contacts; n--; )
	{
		SContactState& contact_state = sp_contact_states[n];
		if (!contact_state.collision) continue;
		
		Mth::Vector impulse = -m_spring_const * contact_state.depth * time_step * contact_state.normal;
		
		Mth::Vector delta_rotvel = m_mass_over_moment * Mth::CrossProduct(contact_state.p_world, impulse);
		
		m_vel += impulse;
		m_rotvel += delta_rotvel;
		
		if (s_draw_skater_collision_circles)
		{
			Gfx::AddDebugLine(contact_state.p_world + m_pos, contact_state.p_world + m_pos + 60.0f * impulse, MAKE_RGB(255, 255, 0), MAKE_RGB(255, 255, 255), 1);
		}
	}
	
	// script callback
	if (max_collision_velocity > m_sound_setup.bounce_velocity_callback_threshold)
	{
		if (m_script_names.bounce)
		{
			GetObject()->SpawnScriptPlease(m_script_names.bounce, mp_script_params)->Update();
		}
		
		#ifdef __NOPT_ASSERT__
		if (m_sound_type_id && Script::GetInteger(CRCD(0xd634a297, "DynamicRigidbodySounds")))
		{
			get_sound_setup(m_sound_type_id);
		}
		#endif
		
		// don't use up the last vRP_NUM_UNTOUCHABLE_VOICES voices
		if (m_sound_setup.bounce_sound && NUM_VOICES - Sfx::CSfxManager::Instance()->GetNumSoundsPlaying() > vRP_NUM_UNTOUCHABLE_VOICES)
		{
			float percent = Mth::Clamp(100.0f * max_collision_velocity / m_sound_setup.bounce_velocity_full_speed, 0.0f, 100.0f);
			m_sound_setup.bounce_sound->AddFloat(CRCD(0x9e497fc6, "Percent"), percent);
			mp_sound_component->PlayScriptedSound(m_sound_setup.bounce_sound);
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::get_sound_setup ( uint32 sound_type_id )
{

	Script::CStruct* p_base_struct;
	p_base_struct = Script::GetStructure(CRCD(0xb9738eba, "RigidBodySounds"), Script::ASSERT);

	Script::CStruct* p_sound_struct = NULL;
	p_base_struct->GetStructure(sound_type_id, &p_sound_struct);
	Dbg_MsgAssert(p_sound_struct, ("Sound type '%s' not found in RigidBodySounds", Script::FindChecksumName(sound_type_id)));

	Script::CStruct* p_struct;

	if (p_sound_struct->ContainsComponentNamed(CRCD(0xbf8e0ace, "CollideSound")))
	{
		p_sound_struct->GetStructure(CRCD(0xbf8e0ace, "CollideSound"), &p_struct, Script::ASSERT);
		if (m_sound_setup.collide_sound)
		{
			delete m_sound_setup.collide_sound;
		}
		m_sound_setup.collide_sound = new Script::CStruct;
		*m_sound_setup.collide_sound += *p_struct;
	}

	if (p_sound_struct->ContainsComponentNamed(CRCD(0x1fb2f60c, "BounceSound")))
	{
		p_sound_struct->GetStructure(CRCD(0x1fb2f60c, "BounceSound"), &p_struct, Script::ASSERT);
		if (m_sound_setup.bounce_sound)
		{
			delete m_sound_setup.bounce_sound;
		}
		m_sound_setup.bounce_sound = new Script::CStruct;
		*m_sound_setup.bounce_sound += *p_struct;

		if (p_sound_struct->ContainsComponentNamed(CRCD(0x3818ee47, "CollideMuteDelay")))
		{
			int time;
			p_sound_struct->GetInteger(CRCD(0x3818ee47, "CollideMuteDelay"), &time);
			m_sound_setup.collide_mute_delay = static_cast< Tmr::Time >(time);
		}
		if (p_sound_struct->ContainsComponentNamed(CRCD(0xad67a34d, "GlobalCollideMuteDelay")))
		{
			int time;
			p_sound_struct->GetInteger(CRCD(0xad67a34d, "GlobalCollideMuteDelay"), &time);
			m_sound_setup.global_collide_mute_delay = static_cast< Tmr::Time >(time);
		}
		p_sound_struct->GetFloat(CRCD(0x4511ca8d, "BounceVelocityCallbackThreshold"), &m_sound_setup.bounce_velocity_callback_threshold);
		p_sound_struct->GetFloat(CRCD(0x8fc6519f, "BounceVelocityFullSpeed"), &m_sound_setup.bounce_velocity_full_speed);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::setup_contacts_from_array ( Script::CArray* pArray )
{
	m_num_contacts = pArray->GetSize();
	Dbg_MsgAssert(m_num_contacts <= vRP_MAX_NUM_CONTACTS, ("Number of contacts in rigidbody exceeds limit of %i contacts", vRP_MAX_NUM_CONTACTS));
	
	if (mp_contacts)
	{
		delete [] mp_contacts;
	}
	mp_contacts = new SContact[m_num_contacts];
	
	for (unsigned int n = 0; n < m_num_contacts; n++)
	{
		Script::CVector* p_v = pArray->GetVector(n);
        mp_contacts[n].p[X] = p_v->mX;
        mp_contacts[n].p[Y] = p_v->mY;
        mp_contacts[n].p[Z] = p_v->mZ;
		mp_contacts[n].p[W] = 0.0f;
		mp_contacts[n].directed_friction = false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
		
void CRigidBodyComponent::setup_contacts_as_box ( const Mth::Vector& top_half_dimensions, const Mth::Vector& bottom_half_dimensions )
{
	m_num_contacts = 8;

	if (mp_contacts)
	{
		delete [] mp_contacts;
	}
	mp_contacts = new SContact[m_num_contacts];
	
	int n = 0;
	bool x = false;
	do {
		bool y = false;
		do {
			bool z = false;
			do {
				mp_contacts[n].p[X] = (x ? 1.0f : -1.0f) * (y ? top_half_dimensions[X] : bottom_half_dimensions[X]);
				mp_contacts[n].p[Y] = (y ? top_half_dimensions[Y] : -bottom_half_dimensions[Y]);
				mp_contacts[n].p[Z] = (z ? 1.0f : -1.0f) * (y ? top_half_dimensions[Z] : bottom_half_dimensions[Z]);
				mp_contacts[n].p[W] = 1.0f;
				mp_contacts[n].directed_friction = false;
				n++;
				z = !z;
			} while (z);
			y = !y;
		} while (y);
		x = !x;
	} while (x);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::setup_contacts_as_pyramid ( float half_height, float half_depth )
{
	m_num_contacts = 5;

	if (mp_contacts)
	{
		delete [] mp_contacts;
	}
	mp_contacts = new SContact[m_num_contacts];
	
	mp_contacts[0].p[X] = half_depth;
	mp_contacts[0].p[Y] = -0.6f * half_height;
	mp_contacts[0].p[Z] = half_depth;
	mp_contacts[0].p[W] = 1.0f;
	mp_contacts[0].directed_friction = false;

	mp_contacts[1].p[X] = -half_depth;
	mp_contacts[1].p[Y] = -0.6f * half_height;
	mp_contacts[1].p[Z] = half_depth;
	mp_contacts[1].p[W] = 1.0f;
	mp_contacts[0].directed_friction = false;

	mp_contacts[2].p[X] = -half_depth;
	mp_contacts[2].p[Y] = -0.6f * half_height;
	mp_contacts[2].p[Z] = -half_depth;
	mp_contacts[2].p[W] = 1.0f;
	mp_contacts[0].directed_friction = false;

	mp_contacts[3].p[X] = half_depth;
	mp_contacts[3].p[Y] = -0.6f * half_height;
	mp_contacts[3].p[Z] = -half_depth;
	mp_contacts[3].p[W] = 1.0f;
	mp_contacts[0].directed_friction = false;

	mp_contacts[4].p[X] = 0.0f;
	mp_contacts[4].p[Y] = 1.6f * half_height;
	mp_contacts[4].p[Z] = 0.0f;
	mp_contacts[4].p[W] = 1.0f;
	mp_contacts[0].directed_friction = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::setup_contacts_as_cylinder ( float top_radius, float bottom_radius, float half_height, int edges )
{
	m_num_contacts = 2 * edges;

	if (mp_contacts)
	{
		delete [] mp_contacts;
	}
	mp_contacts = new SContact[m_num_contacts];
	
	int i = 0;
	bool top = false;
	do {
		for (int n = edges; n--; )
		{
			mp_contacts[i].p[X] = (top ? top_radius : bottom_radius) * cosf(n * 2.0f * 3.1415f / edges);
			mp_contacts[i].p[Y] = (top ? half_height : -half_height);
			mp_contacts[i].p[Z] = (top ? top_radius : bottom_radius) * sinf(n * 2.0f * 3.1415f / edges);
			mp_contacts[i].p[W] = 1.0f;
			mp_contacts[i].directed_friction = false;
			i++;
		}
		top = !top;
	} while (top);
}
			

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::setup_contacts_as_triangle ( const Mth::Vector& half_dimensions )
{
	m_num_contacts = 6;

	if (mp_contacts)
	{
		delete [] mp_contacts;
	}
	mp_contacts = new SContact[m_num_contacts];
	
	int i = 0;
	bool left = false;
	do {
		mp_contacts[i].p[X] = half_dimensions[X];
		mp_contacts[i].p[Y] = -half_dimensions[Y];
		mp_contacts[i].p[Z] = (left ? -1.0f : 1.0f) * half_dimensions[Z];
		mp_contacts[i].p[W] = 1.0f;
		mp_contacts[i].directed_friction = false;
		i++;
		
		mp_contacts[i].p[X] = -half_dimensions[X];
		mp_contacts[i].p[Y] = -half_dimensions[Y];
		mp_contacts[i].p[Z] = (left ? -1.0f : 1.0f) * half_dimensions[Z];
		mp_contacts[i].p[W] = 1.0f;
		mp_contacts[i].directed_friction = false;
		i++;
		
		mp_contacts[i].p[X] = 0.0f;
		mp_contacts[i].p[Y] = half_dimensions[Y];
		mp_contacts[i].p[Z] = (left ? -1.0f : 1.0f) * half_dimensions[Z];
		mp_contacts[i].p[W] = 1.0f;
		mp_contacts[i].directed_friction = false;
		i++;
		
		left = !left;
	} while (left);
}			

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CRigidBodyComponent::draw_debug_lines (   ) const
{
	int color = 0;
	bool collidable = false;
	switch (m_state)
	{
		case ASLEEP:
			color = MAKE_RGB(0, 0, 200);
			collidable = true;
			break;
			
		case AWAKE:
			if (m_ignore_skater_countdown > 0.0f)
			{
				color = MAKE_RGB(0, 200, 0);
			}
			else
			{
				color = MAKE_RGB(200, 0, 0);
				collidable = true;
			}
			break;
	}
	
	if (collidable && s_draw_skater_collision_circles)
	{
		Gfx::AddDebugCircle(m_pos + Mth::Vector(0.0f, m_skater_collision_application_radius, 0.0f), 8, m_skater_collision_radius, MAKE_RGB(200, 200, 0), 1);
		Gfx::AddDebugCircle(m_pos + Mth::Vector(0.0f, -m_skater_collision_application_radius, 0.0f), 8, m_skater_collision_radius, MAKE_RGB(200, 200, 0), 1);
	}
	
	if (s_draw_skater_collision_circles)
	{
		Gfx::AddDebugStar(m_pos, 36.0f, MAKE_RGB(200, 0, 200), 1);
	}
	
	// draw debug lines in less pretty yet general manner
	for (int n = m_num_contacts; n--; )
	{
		for (int m = 0; m < n; m++)
		{
			Gfx::AddDebugLine(sp_contact_states[n].p_world + m_pos, sp_contact_states[m].p_world + m_pos, color, color, 1);
		}
	}
}

}
