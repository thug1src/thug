//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       VehicleComponent.cpp
//* OWNER:          Dan Nelson
//* CREATION DATE:  1/31/3
//****************************************************************************

#include <core/defines.h>
#include <core/math.h>

#include <sys/config/config.h>

#include <gel/components/vehiclecomponent.h>
#include <gel/components/inputcomponent.h>
#include <gel/components/animationcomponent.h>
#include <gel/components/soundcomponent.h>
#include <gel/components/modelcomponent.h>
#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/collision/collcache.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/vecpair.h>
#include <gel/scripting/symboltable.h>

#include <gfx/nxmodel.h>
#include <gfx/nxhierarchy.h>
#include <gfx/skeleton.h>
#include <gfx/nxviewman.h>
#include <gfx/nxmiscfx.h>

#include <sk/engine/feeler.h>
#include <sk/modules/skate/skate.h>
#include <sk/scripting/nodearray.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/objects/restart.h>
#include <sk/parkeditor2/parked.h>
#include <sk/gamenet/gamenet.h>

#include <core/math/slerp.h>

#define MESSAGE(a) { printf("M:%s:%i: %s\n", __FILE__ + 15, __LINE__, a); }
#define DUMPI(a) { printf("D:%s:%i: " #a " = %i\n", __FILE__ + 15, __LINE__, a); }
#define DUMPF(a) { printf("D:%s:%i: " #a " = %g\n", __FILE__ + 15, __LINE__, a); }
#define DUMPE(a) { printf("D:%s:%i: " #a " = %e\n", __FILE__ + 15, __LINE__, a); }
#define DUMPS(a) { printf("D:%s:%i: " #a " = %s\n", __FILE__ + 15, __LINE__, a); }
#define DUMPP(a) { printf("D:%s:%i: " #a " = %p\n", __FILE__ + 15, __LINE__, a); }
#define DUMPV(a) { printf("D:%s:%i: " #a " = %g, %g, %g\n", __FILE__ + 15, __LINE__, (a)[X], (a)[Y], (a)[Z]); }
#define DUMP4(a) { printf("D:%s:%i: " #a " = %g, %g, %g, %g\n", __FILE__ + 15, __LINE__, (a)[X], (a)[Y], (a)[Z], (a)[W]); }
#define DUMPM(a) { DUMP4(a[X]); DUMP4(a[Y]); DUMP4(a[Z]); DUMP4(a[W]); }
#define MARK { printf("K:%s:%i: %s\n", __FILE__ + 15, __LINE__, __PRETTY_FUNCTION__); }
#define PERIODIC(n) for (static int p__ = 0; (p__ = ++p__ % (n)) == 0; )

// THOUGHTS:
//
// - BUG: solve negative collision depth issue for rect collisions; must project collision normal into rect plane before calculating depth (i think)
// - BUG: bad wipeout behavior; IDEAS:
//   - use only line feelers
//   - turn off graivty with > 2 contacts
//   - sleep like a rigidbody
//   - compare algorithm with rigidbody
// - Triggers.
// - Lock motion if very slow and there's no input.
// - Vehicle camera needs to reports its old position to get sounds' pitch correctly.
//
// - two center of masses (collision at zero and suspension) is an issue; actual rotation is done around zero; what sort of poor behavior does this cause
//   when the car is on two wheels? what way is there to retain stable suspension behavior but use a common center of mass?	perhaps simply damp rotational
//   impulses due to the suspension; this may help with the "vert, linear-to-angular energy" freak-out issue as well
// - body-body collisions; treat as boxes
// - prehaps reduce wheel friction when very near vertical to prevent wall riding
// - states: ASLEEP, AWAKE (full brake; rigidbody), DRIVEN, DRONE (?)
// - vertical side rectangle feelers
// - wheel camber <sp> extracted from model
// - side slippage when stopped on hills

namespace Obj
{
	
CVehicleComponent::SCollisionPoint CVehicleComponent::sp_collision_points[4 * (Nx::MAX_NUM_2D_COLLISIONS_REPORTED + 1)];
								   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CVehicleComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CVehicleComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CVehicleComponent::CVehicleComponent() : CBaseComponent()
{
	SetType( CRC_VEHICLE );
	
	mp_input_component = NULL;
	
	m_draw_debug_lines = 0;
	
	m_update_suspension_only = false;
	
	m_steering_display = 0.0f;
	
	m_skater_pos.Set();
	
	mp_wheels = NULL;
	
	m_sound_setup_checksum = CRCD(0x1ca1ff20, "Default");
	
	m_num_collision_points = 0;
	
	m_artificial_collision_timer = 0.0f;
	
	m_state = AWAKE;
	
	m_consider_sleeping_count = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CVehicleComponent::~CVehicleComponent()
{
	delete [] mp_wheels;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::InitFromStructure( Script::CStruct* pParams )
{
	// extract constant parameters from struct; parameters not included in the script are left unchanged
	
	// toggles through the debug line drawing states
	if (pParams->ContainsFlag(CRCD(0x935ab858, "debug")))
	{
		m_draw_debug_lines = ++m_draw_debug_lines % 3;
	}
	else if (pParams->ContainsFlag(CRCD(0xe6b5fcd9, "full_debug")))
	{
		m_draw_debug_lines = (m_draw_debug_lines + 2) % 3;
	}
	else if (pParams->ContainsFlag(CRCD(0x751da48b, "no_debug")))
	{
		m_draw_debug_lines = 0;
	}
	
	// center of mass location (in)
	// the point around which the car rotates; a lower center of mass reduces roll tendency; the XZ location affects the
	// vehicle's behavior during jumps
	if (pParams->ContainsComponentNamed(CRCD(0x93346b8c, "suspension_center_of_mass")))
	{
		float suspension_center_of_mass_offset;
		pParams->GetFloat(CRCD(0x93346b8c, "suspension_center_of_mass"), &suspension_center_of_mass_offset);
		m_suspension_center_of_mass.Set(0.0f, suspension_center_of_mass_offset, 0.0f);
	}
	
	// mass (lbs)
	// affects acceleration, suspension behavior, and resistance to drag
	if (pParams->ContainsComponentNamed(CRCD(0x93fca499, "mass")))
	{
		pParams->GetFloat(CRCD(0x93fca499, "mass"), &m_inv_mass);
		m_inv_mass = vVP_GRAVITATIONAL_ACCELERATION / m_inv_mass;
	}

	// moment of inertia (in^2 lbs)
	// resistance to changes in rotation along the three axes; natural value is about the weight of the car times the
	// square of the average radius of the car along the axes in question
	// X: resistance to rolling
	// Y: resistance to turning
	// Z: resistance to pitcing during acceleration, braking, and jumps
	if (pParams->ContainsComponentNamed(CRCD(0x8c6473ec, "moment_of_inertia")))
	{
		pParams->GetVector(CRCD(0x8c6473ec, "moment_of_inertia"), &m_inv_moment_body);
		m_inv_moment_body[X] = vVP_GRAVITATIONAL_ACCELERATION / m_inv_moment_body[X];
		m_inv_moment_body[Y] = vVP_GRAVITATIONAL_ACCELERATION / m_inv_moment_body[Y];
		m_inv_moment_body[Z] = vVP_GRAVITATIONAL_ACCELERATION / m_inv_moment_body[Z];
	}
	
	// vehicle body's coefficient of restitution
	pParams->GetFloat(CRCD(0x79c3b862, "body_restitution"), &m_body_restitution);
	
	// vehicle body's coefficient of friction
	pParams->GetFloat(CRCD(0xf476273c, "body_friction"), &m_body_friction);
	
	// vehicle body's coefficient of friction
	pParams->GetFloat(CRCD(0x4a23a53c, "body_wipeout_friction"), &m_body_wipeout_friction);
	
	// vehicle body's penalty-method interpenetration-prevention spring constant
	pParams->GetFloat(CRCD(0x1dd4890, "body_spring"), &m_body_spring);
	
	// factor of normal impulse over which you can control via steering
	pParams->GetFloat(CRCD(0xb9f026ed, "collision_control"), &m_collision_control);
	
	// horizontal velocity cutoff below which no in-air slerping to face velocity occurs
	pParams->GetFloat(CRCD(0x8c14709e, "in_air_slerp_velocity_cutoff"), &m_in_air_slerp_vel_cutoff);
	
	// time over which in-air slerping lerps to full strength after takeoff
	pParams->GetFloat(CRCD(0x2c91e49b, "in_air_slerp_time_delay"), &m_in_air_slerp_time_delay);
	
	// in-air slerping strength
	pParams->GetFloat(CRCD(0x280f0e0b, "in_air_slerp_strength"), &m_in_air_slerp_strength);
	
	// special slerping below standard velocity threshold
	m_vert_correction = pParams->ContainsFlag(CRCD(0x5341806a, "vert_correction"));
	
	// maximum steering angle (degrees)
	if (pParams->ContainsComponentNamed(CRCD(0xc1e5abdd, "max_steering_angle")))
	{
        pParams->GetFloat(CRCD(0xc1e5abdd, "max_steering_angle"), &m_max_steering_angle);
		m_max_steering_angle = DEGREES_TO_RADIANS(m_max_steering_angle);
	}
	
	// rotational damping parameters which prevent fishtailing and drift
	pParams->GetFloat(CRCD(0x386837db, "constant_rotational_damping"), &m_const_rotvel_damping);
	pParams->GetFloat(CRCD(0x469cf79a, "quadratic_rotational_damping"), &m_quad_rotvel_damping);
	
	// if the car can be exited via triangle
	m_exitable = pParams->ContainsFlag(CRCD(0xc2a136cc, "exitable"));
	
	// if the car has a handbrke
	m_no_handbrake = pParams->ContainsFlag(CRCD(0xad6c0a46, "no_handbrake"));
	
	// the vehicle's body's interaction with the environment is expressed as an array of rectangular colliders
	if (pParams->ContainsComponentNamed(CRCD(0xfc8c4ac6, "colliders")))
	{
		Script::CArray* p_colliders_array;
		pParams->GetArray(CRCD(0xfc8c4ac6, "colliders"), &p_colliders_array, Script::ASSERT);
		
		float average_distance = 0.0f;
		
		int num_colliders = p_colliders_array->GetSize();
		// Dbg_MsgAssert(num_colliders == vVP_NUM_COLLIDERS, ("Number of colliders for CVehicleComponent is incorrect"));
		
		for (int collider_idx = num_colliders; collider_idx--; )
		{
            SCollider& collider = mp_colliders[collider_idx];
			
			Script::CArray* p_corner_array = p_colliders_array->GetArray(collider_idx);
			Dbg_MsgAssert(p_corner_array->GetSize() == 3, ("Incorrect number of corner vectors in collider array"));
			
			Script::CVector* p_corner_vector;
            p_corner_vector = p_corner_array->GetVector(0);
            collider.body.m_corner[X] = p_corner_vector->mX;
            collider.body.m_corner[Y] = p_corner_vector->mY;
            collider.body.m_corner[Z] = p_corner_vector->mZ;
			p_corner_vector = p_corner_array->GetVector(1);
			collider.body.m_first_edge[X] = p_corner_vector->mX;
			collider.body.m_first_edge[Y] = p_corner_vector->mY;
			collider.body.m_first_edge[Z] = p_corner_vector->mZ;
			collider.body.m_first_edge -= collider.body.m_corner;
			p_corner_vector = p_corner_array->GetVector(2);
			collider.body.m_second_edge[X] = p_corner_vector->mX;
			collider.body.m_second_edge[Y] = p_corner_vector->mY;
			collider.body.m_second_edge[Z] = p_corner_vector->mZ;
			collider.body.m_second_edge -= collider.body.m_corner;
			
			// update maximum_distance
			average_distance += collider.body.m_corner.Length();
			average_distance += (collider.body.m_corner + collider.body.m_first_edge).Length();
			average_distance += (collider.body.m_corner + collider.body.m_second_edge).Length();
			
			// calculate the precomputable values
			collider.first_edge_length = collider.body.m_first_edge.Length();
			collider.second_edge_length = collider.body.m_second_edge.Length();
			
			Dbg_MsgAssert(collider.first_edge_length > 0.0f, ("Collider of zero area"));
			Dbg_MsgAssert(collider.second_edge_length > 0.0f, ("Collider of zero area"));
		}
		
		// set the skater's rigid body collision radius while in the car
		Mdl::Skate::Instance()->GetLocalSkater()->SetRigidBodyCollisionRadiusBoost(average_distance / 3.0f / num_colliders - 48.0f);
	}
	
	if (pParams->ContainsComponentNamed(CRCD(0x1757e572, "engine")))
	{
		Script::CStruct* p_engine_struct;
		pParams->GetStructure(CRCD(0x1757e572, "engine"), &p_engine_struct, Script::ASSERT);
		
		// base drive torque of the engine; multiplied by the gear and differential ratio (ft-lbs)
		if (p_engine_struct->ContainsComponentNamed(CRCD(0x9aa9faee, "drive_torque")))
		{
			p_engine_struct->GetFloat(CRCD(0x9aa9faee, "drive_torque"), &m_engine.drive_torque);
			m_engine.drive_torque *= 12.0f;
		}
		
		// base drag torque of the engine; multiplied by the square of the gear ratio (ft-lbs)
		if (p_engine_struct->ContainsComponentNamed(CRCD(0xb2268648, "drag_torque")))
		{
			p_engine_struct->GetFloat(CRCD(0xb2268648, "drag_torque"), &m_engine.drag_torque);
			m_engine.drag_torque *= 12.0f;
		}
		
		// when rpm reaches this speed, the transmition upshifts (rpm)
		if (p_engine_struct->ContainsComponentNamed(CRCD(0x33921583, "upshift_rpm")))
		{
			p_engine_struct->GetFloat(CRCD(0x33921583, "upshift_rpm"), &m_engine.upshift_rotvel);
			m_engine.upshift_rotvel = RPM_TO_RADIANS_PER_SECOND(m_engine.upshift_rotvel);
		}
		
		// base gear ratio; torque and rpm are multiplied by this when translated up and down the drive train
		p_engine_struct->GetFloat(CRCD(0x94ea6601, "differential_ratio"), &m_engine.differential_ratio);
		
		// instead of having a true reverse gear, all gears are scaled down by this ratio when in reverse
		p_engine_struct->GetFloat(CRCD(0x82bcbb10, "reverse_torque_ratio"), &m_engine.reverse_torque_ratio);
		
		// gear ratios; torque and rpm are multiplied by these when translated up and down the drive train
		if (p_engine_struct->ContainsComponentNamed(CRCD(0xc023fa75, "gear_ratios")))
		{
			Script::CArray* p_engine_gear_ratios_array;
			p_engine_struct->GetArray(CRCD(0xc023fa75, "gear_ratios"), &p_engine_gear_ratios_array, Script::ASSERT);
			
			m_engine.num_gears = p_engine_gear_ratios_array->GetSize();
			Dbg_MsgAssert(m_engine.num_gears <= vVP_MAX_NUM_GEARS, ("Number of gears exceeds the maximum allowed number"));
			
			for (int n = 0; n < m_engine.num_gears; n++)
			{
				m_engine.p_gear_ratios[n] = p_engine_gear_ratios_array->GetFloat(n);
				
				if (n == 0) continue;
				
				Dbg_MsgAssert(n == 0 || m_engine.p_gear_ratios[n - 1] > m_engine.p_gear_ratios[n],
					("Low gear has lower gear ratio than high gear"));
			}
		}
	}
	
	// you can setup all wheels at once or each individually
	
	// an array of wheel structures
	if (pParams->ContainsComponentNamed(CRCD(0xb3f8557e, "wheels")))
	{
		Script::CArray* p_wheels_array = NULL;
		pParams->GetArray(CRCD(0xb3f8557e, "wheels"), &p_wheels_array, Script::ASSERT);
		
		Dbg_MsgAssert(!mp_wheels || m_num_wheels == p_wheels_array->GetSize(), ("Changed number of wheels"));
		
		m_num_wheels = p_wheels_array->GetSize();
			
		if (!mp_wheels)
		{
			mp_wheels = new SWheel[m_num_wheels];
		}
			
		for (int n = m_num_wheels; n--; )
		{
			Script::CStruct* p_wheel_struct = NULL;
			p_wheel_struct = p_wheels_array->GetStructure(n);
			
			// see update_wheel_from_structure() for documentation on wheel parameters
			update_wheel_from_structure(mp_wheels[n], p_wheel_struct);
		} // END loop over wheels
	} // END if wheel array specified

	Dbg_MsgAssert(mp_wheels, ("Wheels not created"));
	
	// a structure of wheel parameters which acts on all wheels
	if (pParams->ContainsComponentNamed(CRCD(0x371badff, "all_wheels")))
	{
		Script::CStruct* p_wheel_struct;
		pParams->GetStructure(CRCD(0x371badff, "all_wheels"), &p_wheel_struct, Script::ASSERT);
		
		for (int n = m_num_wheels; n--; )
		{
			// see update_wheel_from_structure() for documentation on wheel parameters
			update_wheel_from_structure(mp_wheels[n], p_wheel_struct);
		} // END loop over wheels
	} // END if all_wheel structure specified
	
	// calculate dependent constant characteristics
	
	// count the number of drive wheels
	m_num_drive_wheels = 0;
	for (int n = m_num_wheels; n--; )
	{
		if (mp_wheels[n].drive)
		{
			m_num_drive_wheels++;
		}
	}
	
	// setup position and orientation based on the object's state
	m_orientation = GetObject()->GetMatrix();
	m_orientation.Normalize();
	if (pParams->ContainsComponentNamed(CRCD(0xaa99c521, "save")))
	{
		Mth::Vector orientation_vector;
		orientation_vector.Set();
		pParams->GetVector(CRCD(0xc97f3aa9, "orientation"), &orientation_vector);
		m_orientation.SetVector(orientation_vector);
		m_orientation.Normalize();
		m_orientation.GetMatrix(m_orientation_matrix);
		m_orientation_matrix[W].Set();
		GetObject()->SetMatrix(m_orientation_matrix);
		GetObject()->SetDisplayMatrix(m_orientation_matrix);
	}
	
	m_pos = GetObject()->GetPos();
	if (pParams->ContainsComponentNamed(CRCD(0x7f261953, "pos")))
	{
		pParams->GetVector(CRCD(0x7f261953, "pos"), &m_pos);
		GetObject()->SetPos(m_pos);
	}
	
	m_skater_visible = m_skater_visible || pParams->ContainsFlag(CRCD(0x2ed67657, "make_skater_visible"));
	if (pParams->ContainsFlag(CRCD(0x2ed67657, "make_skater_visible")))
	{
		pParams->GetVector(CRCD(0xec86ef7a, "skater_pos"), &m_skater_pos, Script::ASSERT);
		pParams->GetChecksum(CRCD(0xda75a33e, "skater_anim"), &m_skater_anim, Script::ASSERT);
	}
	
	pParams->GetChecksum(CRCD(0xedcf90e, "Sounds"), &m_sound_setup_checksum);
	
	// zero velocities and accumulators
	
	m_mom.Set(0.0f, 0.0f, 0.0f);
	m_rotmom.Set(0.0f, 0.0f, 0.0f);
	
	m_force.Set(0.0f, 0.0f, 0.0f);
	m_torque.Set(0.0f, 0.0f, 0.0f);
	
	for (int n = m_num_wheels; n--; )
	{
		SWheel& wheel = mp_wheels[n];
		
		wheel.state = SWheel::OUT_OF_CONTACT;
 		wheel.orientation = 0.0f;
		wheel.rotvel = 0.0f;
		wheel.y_offset = wheel.y_offset_hang;
		wheel.steering_angle = 0.0f;
		wheel.steering_angle_display = 0.0f;
		
		wheel.rotacc = 0.0f;

		// set normal force history to their default values
		for (int i = vVP_NORMAL_FORCE_HISTORY_LENGTH; i--; )
		{
			wheel.normal_force_history[i] = vVP_GRAVITATIONAL_ACCELERATION / m_inv_mass / m_num_wheels;
		}
	}
	m_next_normal_force_history_idx = 0;
	
	m_gravity_override_timer = 0.0f;
	m_gravity_override_fraction = 1.0f;
	
	m_in_flip = false;
	
	// grab a pointer to the vehicle's skeleton
	mp_skeleton_component = static_cast< CSkeletonComponent* >(GetObject()->GetComponent(CRC_SKELETON));
	Dbg_MsgAssert(mp_skeleton_component, ("Vehicle component has no peer skeleton component."));
	Dbg_MsgAssert(mp_skeleton_component->GetSkeleton()->GetNumBones() == static_cast< int >(2 + m_num_wheels), ("Vehicle component's peer skeleton component has an unexpected number of bones"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	Dbg_Assert(false);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::Finalize (   )
{
	mp_input_component = GetInputComponentFromObject(GetObject());
	mp_model_component = GetModelComponentFromObject(GetObject());
	
	Dbg_Assert(mp_input_component);
	Dbg_Assert(mp_model_component);
	
	// extract information about the car from the model
	
	Dbg_MsgAssert(m_num_wheels == vVP_NUM_WHEELS, ("Number of wheels must equal CVehicleComponent::vVP_NUM_WHEELS"));
	
	CModelComponent* p_model_component = static_cast< CModelComponent* >(GetModelComponentFromObject(GetObject()));
	Dbg_Assert(p_model_component);
	Nx::CModel* p_model = p_model_component->GetModel();
	Dbg_Assert(p_model);
	Nx::CHierarchyObject* p_hierarchy_objects = p_model->GetHierarchy();
	Dbg_Assert(p_hierarchy_objects);
	
	for (int n = vVP_NUM_WHEELS; n--; )
	{
		SWheel& wheel = mp_wheels[n];
		
		Mth::Matrix wheel_matrix = (p_hierarchy_objects + 2 + n)->GetSetupMatrix();
		
		// rotate out of max coordinate system
		wheel.pos[X] = -wheel_matrix[W][X];
		wheel.pos[Y] = wheel_matrix[W][Z];
		wheel.pos[Z] = -wheel_matrix[W][Y];
		wheel.pos[W] = 1.0f;
	}
	
	Mth::Matrix body_matrix = (p_hierarchy_objects + 1)->GetSetupMatrix();
	m_body_pos[X] = -body_matrix[W][X];
	m_body_pos[Y] = body_matrix[W][Z];
	m_body_pos[Z] = -body_matrix[W][Y];
	m_body_pos[W] = 1.0f;
	
	// extract axle and wheelbase information
	int left_steering_tire = -1;
	int right_steering_tire = -1;
	int rear_tire = -1;
	for (int n = vVP_NUM_WHEELS; n--; )
	{
		switch (mp_wheels[n].steering)
		{
			case SWheel::LEFT:
				left_steering_tire = n;
				break;
			case SWheel::RIGHT:
				right_steering_tire = n;
				break;
			case SWheel::FIXED:
				rear_tire = n;
				break;
		}
	}
	Dbg_Assert(left_steering_tire != -1);
	Dbg_Assert(right_steering_tire != -1);
	Dbg_Assert(rear_tire != -1);
	m_cornering_wheelbase = Mth::Abs(
		(p_hierarchy_objects + 2 + left_steering_tire)->GetSetupMatrix()[W][Y] - (p_hierarchy_objects + 2 + rear_tire)->GetSetupMatrix()[W][Y]
	);
	m_cornering_axle_length = Mth::Abs(
		(p_hierarchy_objects + 2 + left_steering_tire)->GetSetupMatrix()[W][X] - (p_hierarchy_objects + 2 + right_steering_tire)->GetSetupMatrix()[W][X]
	);
	
	// calculate the true wheel positions based on the desired wheel positions with vehicle weight applied
	for (int n = m_num_wheels; n--; )
	{
		SWheel& wheel = mp_wheels[n];
	
		// place the wheel in the desired location based on the vehicle mass and the suspension spring rate; the suspension feeler starts at two
		// radii above the desired position
		float desired_y_pos = wheel.pos[Y];
		wheel.pos[Y] = desired_y_pos + 2.0f * wheel.radius;
		wheel.y_offset_hang = -wheel.pos[Y] + desired_y_pos - vVP_GRAVITATIONAL_ACCELERATION / (m_inv_mass * m_num_wheels * wheel.spring);
	}
	
	// determine the lowest collider point
	float lowest_collider_height = mp_colliders[0].body.m_corner[Y];
	for (int n = vVP_NUM_COLLIDERS; n--; )
	{
		lowest_collider_height = Mth::Min(mp_colliders[n].body.m_corner[Y], lowest_collider_height);
		lowest_collider_height = Mth::Min(mp_colliders[n].body.m_corner[Y] + mp_colliders[n].body.m_first_edge[Y], lowest_collider_height);
		lowest_collider_height = Mth::Min(mp_colliders[n].body.m_corner[Y] + mp_colliders[n].body.m_second_edge[Y], lowest_collider_height);
	}
	
	// ready the skater for control
	mp_skater = Mdl::Skate::Instance()->GetLocalSkater();
	mp_skater_core_physics_component = GetSkaterCorePhysicsComponentFromObject(mp_skater);
	mp_skater_trigger_component = GetTriggerComponentFromObject(mp_skater);
	Dbg_Assert(mp_skater_core_physics_component);
	Dbg_Assert(mp_skater_trigger_component);
	
	if (!m_skater_visible)
	{
		mp_skater->Hide(true);
	}
	else
	{
		mp_skater_animation_component = GetAnimationComponentFromObject(mp_skater);
		Dbg_Assert(mp_skater_animation_component);
	}
	
	// calculate the center of mass we will use based on the wheel locations
	Mth::Vector center_of_mass(0.0f, 0.0f, 0.0f, 0.0f);
	for (int n = m_num_wheels; n--; )
	{
		center_of_mass += mp_wheels[n].pos;
	}
	center_of_mass /= m_num_wheels;
	center_of_mass[Y] = lowest_collider_height;
	center_of_mass[W] = 0.0f;
	
	// move wheels and colliders so that they are relative to the correct center of mass
	for (int n = m_num_wheels; n--; )
	{
		mp_wheels[n].pos -= center_of_mass;
	}
	for (int n = vVP_NUM_COLLIDERS; n--; )
	{
		mp_colliders[n].body.m_corner -= center_of_mass;
	}
	m_body_pos -= center_of_mass;
	m_skater_pos -= center_of_mass;
	
	update_dependent_variables();
	
	GetObject()->SetPos(m_pos);
	GetObject()->SetVel(m_vel);
	GetObject()->SetMatrix(m_orientation_matrix);
	GetObject()->SetDisplayMatrix(GetObject()->GetMatrix());
	
	update_skeleton();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::Update()
{
	m_reset_this_frame = false;
	
	if (!m_update_suspension_only)
	{
		get_input();
	}
	else
	{
		zero_input();
	}
	
	if (m_state == ASLEEP)
	{
		// reduced work set if we're asleep
		GetObject()->SetDisplayMatrix(GetObject()->GetMatrix());
		update_steering_angles();
		if (m_controls.brake)
		{
			for (int n = m_num_wheels; n--; )
			{
				mp_wheels[n].rotvel = 0.0f;
			}
		}
		update_wheel_dynamic_state();
		update_skeleton();
		draw_shadow();
		control_skater();
		return;
	}
	
	int num_time_steps;
	float frame_length = Tmr::FrameLength();
	
	if (in_artificial_collision())
	{
		m_artificial_collision_timer -= frame_length;
	}
	
	// count down timers
	if (m_gravity_override_timer != 0.0f)
	{
		m_gravity_override_timer -= frame_length;
		if (m_gravity_override_timer <= 0.0f || m_num_wheels_in_contact > 1)
		{
			MESSAGE("Ending vehicle gravity override");
			m_gravity_override_timer = 0.0f;
			m_gravity_override_fraction = 1.0f;
		}
	}
	
	// the physics is unstable at low frame rates, so we take multiple physics steps during long frame; if vehicle physics is a significant fraction
	// of CPU time, this could exacerbate whatever frame rate problems are occuring
	if (frame_length >= (1.0f / 30.0f))
	{
		num_time_steps = static_cast< int >(ceilf(frame_length / (1.0f / 60.0f)));
		if (num_time_steps > 6)
		{
			num_time_steps = 6;
		}
		m_time_step = frame_length / num_time_steps;
		Dbg_Message("CVehicleComponent::Update: using %i steps this frame", num_time_steps);
	}
	else
	{
		num_time_steps = 1;
		m_time_step = frame_length;
	}
	
	for (int step = num_time_steps; step--; )
	{
		update_dynamic_state();
		
		m_force.Set(0.0f, 0.0f, 0.0f);
		m_torque.Set(0.0f, 0.0f, 0.0f);
		for (int n = m_num_wheels; n--; )
		{
			mp_wheels[n].rotacc = 0.0f;
		}
		
		if (!m_update_suspension_only)
		{
			apply_environment_collisions();
			if (reset_this_frame()) return;
		}
		
		// teleport wheels to the ground, if within reach of the suspension
		update_wheel_heights();
		if (reset_this_frame()) return;
		
		// update the steering wheels' angles
		update_steering_angles();
		
		// damp out rotations to create a more drivable car
		damp_rotation();
		
		// accumulate all forces and torques on the body and wheels
		accumulate_forces();

		/////////////
		// update state observers

		if (m_num_wheels_in_contact > 0)
		{
			m_air_time = 0.0f;
		}
		else
		{
			m_air_time += m_time_step;
		}
		
		if (m_num_wheels_in_contact > 0 || m_max_normal_collision_impulse > 0.0f)
		{
			m_air_time_no_collision = 0.0f;
		}
		else
		{
			m_air_time_no_collision += m_time_step;
		}
	}
	
	if (m_update_suspension_only) return;
	
	// draw debug lines
	draw_debug_rendering();
	
	// update object's position and orientation
	GetObject()->SetPos(m_pos);
	GetObject()->SetVel(m_vel);
	GetObject()->SetMatrix(m_orientation_matrix);
	GetObject()->SetDisplayMatrix(GetObject()->GetMatrix());
	
	consider_sleeping();
	
	update_skeleton();

	// Hack to draw shadow
	draw_shadow();
	
	#if 0
	Mth::Vector forward(0.0f, 0.0f, 1.0f);
	forward = m_orientation_matrix.Rotate(forward);
	float vel = IPS_TO_MPH(Mth::Abs(Mth::DotProduct(m_vel, forward)));
	PERIODIC(60) DUMPF(vel);
	#endif
	
	// HACK: get player proximity checks, triggers, driving animations, and the like working
	control_skater();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CVehicleComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		// @script | Vehicle_Kick | kicks the vehicle with a force and torque
		case CRCC(0x93b713a6, "Vehicle_Kick"):
		{
			Mth::Vector linear;
			Mth::Vector angular;
			
			m_state = AWAKE;
			m_consider_sleeping_count = 0;
			
			pParams->GetVector("linear", &linear);
			pParams->GetVector("angular", &angular);
			
			linear = m_orientation_matrix.Rotate(linear);
			angular = m_orientation_matrix.Rotate(angular);
			
			m_mom += linear / m_inv_mass;
			m_rotmom += angular / m_inv_moment_body[X];
			
			update_velocities();
			
			break;
		}
		
		// @script | Vehicle_Wake | wakes the vehicle if asleep
		case CRCC(0xa80a0d36, "Vehicle_Wake"):
			m_state = AWAKE;
			m_consider_sleeping_count = 0;
			break;

		// @script | Vehicle_MoveToRestart | teleport the vehicle to the restart node
		case CRCC(0x4b0b27dd, "Vehicle_MoveToRestart"):
		{
			uint32 node_name_checksum;
			if (pParams->GetChecksum(NO_NAME, &node_name_checksum))
			{
				// a node is specifically specified
				MoveToNode(SkateScript::GetNode(SkateScript::FindNamedNode(node_name_checksum, Script::ASSERT)));
			}
			else
			{
				// find a linked restart node
				int node = pScript->mNode;
				Dbg_MsgAssert(node !=  -1,( "Vehicle_MoveToRestart called from non-node script with no target node specified"));
				{
					int num_links = SkateScript::GetNumLinks(node);
					for (int n = 0; n < num_links; n++)
					{
						int linked_node = SkateScript::GetLink(node, n);
						if (IsRestart(linked_node))
						{
							MoveToNode(SkateScript::GetNode(linked_node));
							return CBaseComponent::MF_TRUE;
						}
					}
					if (Ed::CParkEditor::Instance()->UsingCustomPark())
					{
						MoveToNode(SkateScript::GetNode(Mdl::Skate::Instance()->find_restart_node(0)));
					}
					else
					{
						Dbg_MsgAssert(0, ("Vehicle_MoveToRestart called but node %d not linked to restart", node));			
					}
				}
			}			
			break;
		}
			
		// @script | Vehicle_PlaceBeforeCamera | moves the object before the active camera
		case CRCC(0xc33608e4, "Vehicle_PlaceBeforeCamera"):
		{
			Gfx::Camera* camera = Nx::CViewportManager::sGetActiveCamera(0);
			if (camera)
			{
				Mth::Vector& cam_pos = camera->GetPos();
				Mth::Matrix& cam_mat = camera->GetMatrix();

				m_pos = cam_pos;
				m_pos += cam_mat[Y] * 12.0f * 12.0f;
				m_pos -= cam_mat[Z] * 12.0f * 12.0f;
				GetObject()->SetPos(m_pos);
				
				m_orientation_matrix[X] = -cam_mat[X];
				m_orientation_matrix[Y] = cam_mat[Y];
				m_orientation_matrix[Z] = -cam_mat[Z];
				m_orientation = m_orientation_matrix;
				m_orientation.Normalize();
				m_orientation.GetMatrix(m_orientation_matrix);
				m_orientation_matrix[W].Set();
				GetObject()->SetMatrix(m_orientation_matrix);
				
				update_dependent_variables();
			}
			break;
		}
			
		// @script | Vehicle_AdjustGravity | adjusts effective gravity for a given duration or until the vehicle has two or more wheels on the ground,
		// whichever occurs first
		// @parm float | Percent | Percent of standard gravity.
		// @parm float | Duration | Duration in seconds over which to override gravity.
		case CRCC(0xdb35aad8, "Vehicle_AdjustGravity"):
			pParams->GetFloat(CRCD(0x9e497fc6, "Percent"), &m_gravity_override_fraction, Script::ASSERT);
			m_gravity_override_fraction *= 1.0f / 100.0f;
			pParams->GetFloat(CRCD(0x79a07f3f, "Duration"), &m_gravity_override_timer, Script::ASSERT);
			Dbg_MsgAssert(m_gravity_override_timer > 0.0f, ("Vehicle_AdjustGravity must have positive Duration"));
			MESSAGE("Initiating vehicle gravity override");
			break;
			
		// @script | Vehicle_ForceBrake | forces on the brake
		// case CRCC(0x1ad6b6bc, "Vehicle_ForceBrake"):
			// m_force_brake = true;
			// break;
			
		// @script | Vehicle_HandbrakeActive | returns true if the car has a handbrake
		case CRCC(0x5008b253, "Vehicle_HandbrakeActive"):
			return m_force_brake ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
		// @script | Vehicle_AllWheelsAreInContact | returns true if all of the wheels are in contact with geo
		case CRCC(0x279602ed, "Vehicle_AllWheelsAreInContact"):
			return m_num_wheels_in_contact == m_num_wheels ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;

		// @script | Vehicle_LostCollision | lost a net collision; respond appropriately
		case CRCC(0x4c73526b, "Vehicle_LostCollision"):
		{
			Mth::Vector offset;
			pParams->GetVector(CRCD(0xa6f5352f, "Offset"), &offset, Script::ASSERT);
			ApplyArtificialCollision(offset);
			break;
		}
		
		// @script | Vehicle_IsSkaterVisible | true if skater should be visible while driving
		case CRCC(0x81faac21, "Vehicle_IsSkaterVisible"):
			return m_skater_visible ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE; 
			
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}

    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::GetDebugInfo ( Script::CStruct *p_info )
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info, ("NULL p_info sent to CVehicleComponent::GetDebugInfo"));
	
	p_info->AddVector("m_pos", m_pos);
	p_info->AddVector("m_mom", m_mom);
	p_info->AddVector("m_rotmom", m_rotmom);
	p_info->AddFloat("m_orientation", m_orientation.GetScalar());
	p_info->AddVector("m_orientation", m_orientation.GetVector());
	
	p_info->AddVector("m_force", m_force);
	p_info->AddVector("m_torque", m_torque);
	
	p_info->AddVector("m_vel", m_vel);
	p_info->AddVector("m_rotvel", m_rotvel);
	
	p_info->AddVector("m_suspension_center_of_mass", m_suspension_center_of_mass);
	p_info->AddFloat("mass", vVP_GRAVITATIONAL_ACCELERATION / m_inv_mass);
	p_info->AddVector("moment_of_inertia",
		vVP_GRAVITATIONAL_ACCELERATION / m_inv_moment_body[X],
		vVP_GRAVITATIONAL_ACCELERATION / m_inv_moment_body[Y],
		vVP_GRAVITATIONAL_ACCELERATION / m_inv_moment_body[Z]
	);
	p_info->AddFloat("body_restitution", m_body_restitution);
	p_info->AddFloat("body_friction", m_body_friction);
	p_info->AddFloat("body_spring", m_body_spring);
	p_info->AddFloat("collision_control", m_collision_control);
	p_info->AddFloat("in_air_slerp_velocity_cutoff", m_in_air_slerp_vel_cutoff);
	p_info->AddFloat("in_air_slerp_time_delay", m_in_air_slerp_time_delay);
	p_info->AddFloat("in_air_slerp_strength", m_in_air_slerp_strength);
	p_info->AddChecksum("vert_correction", m_vert_correction ? CRCD(0x203b372, "true") : CRCD(0xd43297cf, "false"));
	
	p_info->AddFloat("wipeout_body_friction", m_body_wipeout_friction);
	p_info->AddFloat("max_steering_angle", RADIANS_TO_DEGREES(m_max_steering_angle));
	p_info->AddFloat("cornering_wheelbase", m_cornering_wheelbase);
	p_info->AddFloat("cornering_axle_length", m_cornering_axle_length);
	p_info->AddFloat("constant_rotational_damping", m_const_rotvel_damping);
	p_info->AddFloat("quadratic_rotational_damping", m_quad_rotvel_damping);
	
	Script::CStruct* p_engine_info = new Script::CStruct;
	p_engine_info->AddFloat("drive_torque", m_engine.drive_torque / 12.0f);
	p_engine_info->AddFloat("drag_torque", m_engine.drag_torque / 12.0f);
	p_engine_info->AddFloat("upshift_rpm", RADIANS_PER_SECOND_TO_RPM(m_engine.upshift_rotvel));
	p_engine_info->AddFloat("differential_ratio", m_engine.differential_ratio);
	p_engine_info->AddFloat("reverse_torque_ratio", m_engine.reverse_torque_ratio);
	Script::CArray* p_engine_gear_ratio_info = new Script::CArray;
	p_engine_gear_ratio_info->SetSizeAndType(m_engine.num_gears, ESYMBOLTYPE_FLOAT);
	for (int n = m_engine.num_gears; n--; )
	{
		p_engine_gear_ratio_info->SetFloat(n, m_engine.p_gear_ratios[n]);
	}
	p_engine_info->AddArrayPointer("gear_ratios", p_engine_gear_ratio_info);
	p_info->AddStructurePointer("m_engine", p_engine_info);
	
	Script::CArray* p_wheels_info = new Script::CArray;
	p_wheels_info->SetSizeAndType(m_num_wheels, ESYMBOLTYPE_STRUCTURE);
	for (int n = m_num_wheels; n--; )
	{
		SWheel& wheel = mp_wheels[n];
		Script::CStruct* p_local_info = new Script::CStruct;
		
		uint32 state_checksums[] =
		{
			CRCC(0xba2b31d7, "NO_STATE"),
			CRCC(0xcd419ee6, "OUT_OF_CONTACT"),
			CRCC(0x927c0fed, "UNDER_GRIPPING"),
			CRCC(0xe9bc148a, "GRIPPING"),
			CRCC(0x26acccc8, "SLIPPING"),
			CRCC(0xa91003cc, "SKIDDING"),
			CRCC(0x2e7ef449, "HANDBRAKE_THROTTLE"),
			CRCC(0xbf6d6529, "HANDBRAKE_LOCKED"),
		};
		uint32 steering_checksums[] =
		{
			CRCC(0x613631cd, "FIXED"),
			CRCC(0x85981897, "LEFT"),
			CRCC(0x4b358aeb, "RIGHT")
		};
		p_local_info->AddChecksum("state", state_checksums[wheel.state]);
		p_local_info->AddFloat("rotvel", wheel.rotvel);
        p_local_info->AddFloat("y_offset", wheel.y_offset);
        p_local_info->AddFloat("steering_angle", RADIANS_TO_DEGREES(wheel.steering_angle));
        p_local_info->AddFloat("steering_angle_display", RADIANS_TO_DEGREES(wheel.steering_angle_display));
        p_local_info->AddFloat("rotacc", wheel.rotacc);
		p_local_info->AddFloat("orientation", wheel.orientation);
		p_local_info->AddVector("pos", wheel.pos);
		p_local_info->AddFloat("y_offset_hang", wheel.y_offset_hang);
		p_local_info->AddFloat("max_draw_y_offset", wheel.max_draw_y_offset);
		p_local_info->AddChecksum("steering", steering_checksums[wheel.steering]);
		p_local_info->AddInteger("drive", wheel.drive);
		p_local_info->AddFloat("radius", wheel.radius);
		p_local_info->AddFloat("moment", vVP_GRAVITATIONAL_ACCELERATION / wheel.inv_moment);
		p_local_info->AddFloat("spring_rate", wheel.spring);
		p_local_info->AddFloat("damping_rate", wheel.damping);
		p_local_info->AddFloat("static_friction", wheel.static_friction);
		p_local_info->AddFloat("dynamic_friction", wheel.dynamic_friction);
		p_local_info->AddFloat("handbrake_throttle_friction", wheel.handbrake_throttle_friction);
		p_local_info->AddFloat("handbrake_locked_friction", wheel.handbrake_locked_friction);
		p_local_info->AddFloat("min_static_grip_velocity", IPS_TO_MPH(wheel.min_static_velocity));
		p_local_info->AddFloat("max_static_grip_velocity", IPS_TO_MPH(wheel.max_static_velocity));
		p_local_info->AddFloat("min_dynamic_grip_velocity", IPS_TO_MPH(wheel.min_dynamic_velocity));
		p_local_info->AddFloat("brake_torque", wheel.brake.torque / 12.0f);
		p_local_info->AddFloat("handbrake_torque", wheel.brake.handbrake_torque / 12.0f);
		
		p_wheels_info->SetStructure(n, p_local_info);
	}
	p_info->AddArrayPointer("mp_wheels", p_wheels_info);
	
	Script::CStruct* p_controls_info = new Script::CStruct;
	p_controls_info->AddFloat("steering", m_controls.steering);
	p_controls_info->AddInteger("throttle", m_controls.throttle);
	p_controls_info->AddInteger("brake", m_controls.brake);
	p_controls_info->AddInteger("handbrake", m_controls.handbrake);
	p_controls_info->AddInteger("reverse", m_controls.reverse);
	p_info->AddStructurePointer("m_controls", p_controls_info);
	
	if (m_skater_visible)
	{
		p_info->AddVector(CRCD(0xec86ef7a, "skater_pos"), m_skater_pos);
		p_info->AddChecksum(CRCD(0xda75a33e, "skater_anim"), m_skater_anim);
	}

	// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::MoveToNode ( Script::CStruct* p_node )
{
	// move to the position relative to the given node that a ped car would be at
	
	Mth::Vector restart_pos;
	SkateScript::GetPosition(p_node, &restart_pos);
	
	Mth::Vector restart_angles;
	SkateScript::GetAngles(p_node, &restart_angles);
	Mth::Matrix restart_matrix;
	restart_matrix.SetFromAngles(restart_angles);
	
	// calculate appropriate offset from the ground based on estimated wheel offsets
	float avg_ground_offset = 0.0f;
	for (int n = vVP_NUM_WHEELS; n--; )
	{
		avg_ground_offset += mp_wheels[n].y_offset_hang + mp_wheels[n].pos[Y] + vVP_GRAVITATIONAL_ACCELERATION / (m_inv_mass * m_num_wheels * mp_wheels[n].spring);
		avg_ground_offset -= mp_wheels[n].radius;
	}
	avg_ground_offset /= vVP_NUM_WHEELS;
	
	// find ground height
	CFeeler feeler(restart_pos + Mth::Vector(0.0f, 24.0f, 0.0f), restart_pos + Mth::Vector(0.0f, -240.0f, 0.0f));
	if (feeler.GetCollision(false))
	{
		restart_pos[Y] = feeler.GetPoint()[Y] - avg_ground_offset;
	}
	
	// ped cars have their origin between the rear wheels
	int rear_wheel_idx = -1;
	for (int n = vVP_NUM_WHEELS; n--; )
	{
		if (mp_wheels[n].steering == SWheel::FIXED)
		{
			rear_wheel_idx = n;
			break;
		}
	}
	Dbg_Assert(rear_wheel_idx != -1);
	restart_pos -= restart_matrix[Z] * mp_wheels[rear_wheel_idx].pos[Z];

	// move the car to the restart position and allow it to settle on its suspension
	
	m_pos = restart_pos;
	m_orientation = restart_matrix;
	
	m_mom.Set(0.0f, 0.0f, 0.0f);
	m_rotmom.Set(0.0f, 0.0f, 0.0f);
	
	// zero wheels
	
	m_update_suspension_only = true;
	for (int n = 60; n--; )
	{
		// lock wheels each frame
		for (int n = m_num_wheels; n--; )
		{
			mp_wheels[n].rotvel = 0.0f;
		}
		
		Update();
	}
	m_update_suspension_only = false;
	
	m_mom.Set(0.0f, 0.0f, 0.0f);
	m_rotmom.Set(0.0f, 0.0f, 0.0f);
	
	m_reset_this_frame = true;
	
	m_state = ASLEEP;
	
	// update object's position and orientation
	GetObject()->SetPos(m_pos);
	GetObject()->SetVel(m_vel);
	GetObject()->SetMatrix(m_orientation_matrix);
	GetObject()->SetDisplayMatrix(GetObject()->GetMatrix());
	
	update_skeleton();
	
	control_skater();
	
	GetObject()->SetTeleported();
	mp_skater->SetTeleported();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::ApplyArtificialCollision ( const Mth::Vector& offset )
{
	Mth::Vector impulse_direction = offset;
	impulse_direction[Y] = 0.0f;
	impulse_direction.Normalize();
	
	Mth::Vector sideways(-impulse_direction[Z], 0.0f, impulse_direction[X]);
	
	float impulse_forward = (1.0f + Mth::PlusOrMinus(0.5f)) * Script::GetFloat(CRCD(0x3db3fa83, "vehicle_physics_netcoll_forward_impulse"));
	float impulse_sideways = Mth::PlusOrMinus(1.5f) * Script::GetFloat(CRCD(0x39e99fec, "vehicle_physics_netcoll_sideways_impulse"));
	float impulse_upwards = (1.0f + Mth::PlusOrMinus(0.2f)) * Script::GetFloat(CRCD(0x738500fd, "vehicle_physics_netcoll_upwards_impulse"));
	
	Mth::Vector impulse = impulse_forward * impulse_direction + impulse_sideways * sideways;
	impulse[Y] += impulse_upwards;
	
	float rotate_spin = Mth::PlusOrMinus(1.5f) * Script::GetFloat(CRCD(0x14ddb3ef, "vehicle_physics_netcoll_spin_impulse"));
	float rotate_flip = (1.0f + Mth::PlusOrMinus(0.8f)) * Script::GetFloat(CRCD(0x4088e6ec, "vehicle_physics_netcoll_flip_impulse"));
	
	Mth::Vector rotate = rotate_flip * sideways;
	rotate[Y] += rotate_spin;
	
	m_mom += impulse / m_inv_mass;
	m_rotmom += rotate / m_inv_moment_body[X];

	update_velocities();
	
	m_artificial_collision_timer = Script::GetFloat(CRCD(0x771922a6, "vehicle_physics_artificial_collision_duration"));
	
	m_state = AWAKE;
	m_consider_sleeping_count = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::update_flip (   )
{
	m_orientation.GetMatrix(m_orientation_matrix);
	m_orientation_matrix[W].Set();
	
	if (m_orientation_matrix[Y][Y] > 0.999f) // || !m_controls.throttle)
	{
		m_in_flip = false;
		return;
	}
	
	bool flipping_appropriate_this_frame = m_vel.LengthSqr() < Mth::Sqr(300.0f)
		&& m_rotvel.LengthSqr() < Mth::Sqr(Mth::PI / 4.0f)
		&& m_max_normal_collision_impulse > 0.0f
		&& m_orientation_matrix[Y][Y] < 0.25f
		&& (m_num_wheels_in_contact == 0 || m_orientation_matrix[Y][Y] < 0.0f)
		&& m_controls.throttle;

	if (!m_in_flip && !flipping_appropriate_this_frame) return;
	
	// starting a flip
	if (!m_in_flip)
	{
		m_mom.Set();
		m_rotmom.Set();
		
		m_in_flip = true;
		m_flip_start_time_stamp = Tmr::GetTime();
	}
	
	// check for maximum flip duration
	if (Tmr::ElapsedTime(m_flip_start_time_stamp) > vVP_FLIP_DURATION + vVP_FLIP_DELAY)
	{
		m_in_flip = false;
		return;
	}
	
	// put a short delay before the flip actually has an effect
	if (Tmr::ElapsedTime(m_flip_start_time_stamp) < vVP_FLIP_DELAY)
	{
		return;
	}
	
	// setup this frame's slerp
	Mth::Matrix goal_orientation;
	goal_orientation[Z] = m_orientation_matrix[Z];
	goal_orientation[Z][Y] = 0.0f;
	goal_orientation[Z].Normalize();
	goal_orientation[Y].Set(0.0f, 1.0f, 0.0f);
	goal_orientation[X].Set(goal_orientation[Z][Z], 0.0f, -goal_orientation[Z][X]);
	Mth::SlerpInterpolator slerper;
	slerper.setMatrices(&m_orientation_matrix, &goal_orientation);
	
	// calculate the new orientation matrix
	slerper.getMatrix(
		&m_orientation_matrix,
		Mth::ClampMax(vVP_FLIP_RATE * Tmr::FrameLength() / slerper.getRadians(), 1.0f)
	);
	m_orientation = m_orientation_matrix;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::update_wheel_from_structure ( SWheel& wheel, Script::CStruct* p_wheel_struct )
{
	uint32 checksum;
	
	// wheel's steering type
	if (p_wheel_struct->ContainsComponentNamed(CRCD(0x7560e63, "steering")))
	{
		p_wheel_struct->GetChecksum(CRCD(0x7560e63, "steering"), &checksum);
		switch (checksum)
		{
			case CRCC(0x85981897, "left"):
				wheel.steering = SWheel::LEFT;
				break;
			case CRCC(0x4b358aeb, "right"):
				wheel.steering = SWheel::RIGHT;
				break;
			case CRCC(0x613631cd, "fixed"):
				wheel.steering = SWheel::FIXED;
				break;
			default:
				Dbg_MsgAssert(false, ("Bad wheel steering setting"));
		}
	}

	// if the wheel is a drive wheel
	if (p_wheel_struct->ContainsComponentNamed(CRCD(0x97e20a70, "drive")))
	{
		p_wheel_struct->GetChecksum(CRCD(0x97e20a70, "drive"), &checksum);
		switch (checksum)
		{
			case CRCC(0x8a18ca56, "yes"):
				wheel.drive = true;
				break;
			case CRCC(0x9855d7e0, "no"):
				wheel.drive = false;
				break;
			default:
				Dbg_MsgAssert(false, ("Bad wheel drive setting"));
		}
	}

	// suspension spring rate (lbs / in)
	// strength of the suspension spring; multiplied by the compresion to get the spring's force; must be scaled
	// with the vehicle weight; large spring rates give stiffer suspensions; large values may cause instability in
	// physics model
	p_wheel_struct->GetFloat(CRCD(0x9e931fdf, "spring_rate"), &wheel.spring);

	// suspension damping; bump rate (lbs s / in)
	// bounce damping strength of the suspension; smaller values cause bouncier suspensions; very large values may
	// cause instability in the physics model
	p_wheel_struct->GetFloat(CRCD(0xafa960ff, "damping_rate"), &wheel.damping);

	// wheel radius (in)
	// larger radius wheels are harder for the engine to rotate and the brake and rolling resistance to stop
	p_wheel_struct->GetFloat(CRCD(0xc48391a5, "radius"), &wheel.radius);

	if (p_wheel_struct->ContainsComponentNamed(CRCD(0xca73775d, "moment")))
	{
		p_wheel_struct->GetFloat(CRCD(0xca73775d, "moment"), &wheel.inv_moment);
		wheel.inv_moment = vVP_GRAVITATIONAL_ACCELERATION / wheel.inv_moment;
	}
	
	p_wheel_struct->GetFloat(CRCD(0xd87d0183, "max_draw_y_offset"), &wheel.max_draw_y_offset);

	// the following parameters affect the shape of the tire's friction curve
	// the friction curve gives the strength of the frictional force as a function of the tire's slip velocity; our
	// simple curve rises linearly from zero at zero slip velocity, reaches static_friction at a slip velocity of
	// min_static_velocity, remains at static_friction up to a slip velocity of max_static_velocity, drops linearly
	// to dynamic_friction until a slip velocity of min_dynamic_velocity, and then remains constant at dynamic_friction

	// normalized friction strength when the wheel is not skidding
	// low values will cause the vehicle to be spin-out and skid easily; high values cause the tires to feel sticky;
	// real world values are on the order of 1.3; fun values are much higher
	p_wheel_struct->GetFloat(CRCD(0xf36b97c8, "static_friction"), &wheel.static_friction);

	// normalized friction strength when the wheel is skidding
	// should be lower than static_friction; values much lower than static_friction cause the vehicle to be unforgiving;
	// once you lose grip, it is harder to regain control of the vehicle; values near static_friction cause the vehicle
	// to be very forgiving, with skidding causing little loss of handling 
	p_wheel_struct->GetFloat(CRCD(0x674852f6, "dynamic_friction"), &wheel.dynamic_friction);
	
	// normalized friction strength when the handbrake is applied
	// currently specially tuned in script to force a fishtail when the handbrake and throttle are both applied and spin-outs when only handbrake is applied
	p_wheel_struct->GetFloat(CRCD(0x52242919, "handbrake_throttle_friction"), &wheel.handbrake_throttle_friction);
	p_wheel_struct->GetFloat(CRCD(0x6d615f67, "handbrake_locked_friction"), &wheel.handbrake_locked_friction);

	// slip speed below which tires undergrip (mph)
	// should be around 1 to 2 mph
	if (p_wheel_struct->ContainsComponentNamed(CRCD(0x60b2c097, "min_static_grip_velocity")))
	{
		p_wheel_struct->GetFloat(CRCD(0x60b2c097, "min_static_grip_velocity"), &wheel.min_static_velocity);
		wheel.min_static_velocity = MPH_TO_IPS(wheel.min_static_velocity);
	}

	// maximum slip speed before tires begin to lose their grip (mph)
	// should be larger than min_static_velocity; with a low value the vehicle will skid more readily; a high value
	// causes there to be a large range of slip velocties with maximum tire grip
	if (p_wheel_struct->ContainsComponentNamed(CRCD(0xbb0d9370, "max_static_grip_velocity")))
	{
		p_wheel_struct->GetFloat(CRCD(0xbb0d9370, "max_static_grip_velocity"), &wheel.max_static_velocity);
		wheel.max_static_velocity = MPH_TO_IPS(wheel.max_static_velocity);
	}

	// minimum slip speed before tires begin skidding (imph)
	// should be larger than max_static_velocity; with a low values, tires are very unforgiving and begin to skid as soon
	// as they begin to slip; with a larger value, the loss of control and onset of skidding will be more gradual
	if (p_wheel_struct->ContainsComponentNamed(CRCD(0xb71803ad, "min_dynamic_grip_velocity")))
	{
		p_wheel_struct->GetFloat(CRCD(0xb71803ad, "min_dynamic_grip_velocity"), &wheel.min_dynamic_velocity);
		wheel.min_dynamic_velocity = MPH_TO_IPS(wheel.min_dynamic_velocity);
	}

	// torque which the brake exerts on the wheel (ft-lbs)
	if (p_wheel_struct->ContainsComponentNamed(CRCD(0x3bccbadc, "brake_torque")))
	{
		p_wheel_struct->GetFloat(CRCD(0x3bccbadc, "brake_torque"), &wheel.brake.torque);
		wheel.brake.torque *= 12.0f;
	}

	// torque which the handbrake exerts on the wheel when steering is straight and the throttle is down (ft-lbs)
	if (p_wheel_struct->ContainsComponentNamed(CRCD(0x9439d144, "handbrake_torque")))
	{
		p_wheel_struct->GetFloat(CRCD(0x9439d144, "handbrake_torque"), &wheel.brake.handbrake_torque);
		wheel.brake.handbrake_torque *= 12.0f;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::update_dynamic_state (   )
{
	// update body state
	
	// calculate accelerations
	
	Mth::Vector acc = m_inv_mass * m_force;
	Mth::Vector rotacc = m_inv_moment.Rotate(m_torque);
	
	// update state variables
	
	m_mom += m_time_step * m_force;
	update_pos_with_uber_frig(m_time_step * (m_vel + 0.5f * m_time_step * acc));
	
	Mth::Vector delta_orientation_vector = -0.5f * m_time_step * (m_rotvel + 0.5f * m_time_step * rotacc);
	Mth::Quat delta_orientation = m_orientation * Mth::Quat(delta_orientation_vector[X], delta_orientation_vector[Y], delta_orientation_vector[Z], 0.0f);
	m_orientation += delta_orientation;
	m_orientation.Normalize();

    m_rotmom += m_time_step * m_torque;
	
	update_wheel_dynamic_state();
	
    // if we're in the air and moving fast enough horizontally
	if (m_num_wheels_in_contact == 0 && m_max_normal_collision_impulse == 0.0f)
	{
		slerp_to_face_velocity();
	}
	
	update_flip();
    
	update_dependent_variables();
	
#if 0
	PERIODIC(60) {
		float K = 0.5f * Mth::DotProduct(m_vel, m_mom) + 0.5f * Mth::DotProduct(m_rotvel, m_rotmom);
		float U = vVP_GRAVITATIONAL_ACCELERATION * m_pos[Y] / m_inv_mass;
		DUMPF(K + U);
		Mth::Vector a, b;
		a = m_rotmom;
		b = m_rotvel;
		DUMPF(Mth::DotProduct(a.Normalize(), b.Normalize()));
		// DUMPF(m_rotvel.Length());
		// DUMPV(m_rotvel);
	}
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::update_wheel_dynamic_state (   )
{
	for (int n = m_num_wheels; n--; )
	{
		SWheel& wheel = mp_wheels[n];
		
		// wheel rotvel updates as torques are applied; thus, wheel rotvel is currently the frame's final rotvel 
		wheel.orientation += m_time_step * (wheel.rotvel - 0.5f * m_time_step * wheel.rotacc);
		
		if (wheel.orientation > (200.0f * Mth::PI))
		{
			wheel.orientation -= (100.0f * Mth::PI);
		}
		else if (wheel.orientation < 0.0f)
		{
			wheel.orientation += (100.0f * Mth::PI);
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::update_dependent_variables (   )
{
	m_orientation.GetMatrix(m_orientation_matrix);
	m_orientation_matrix[W].Set();
	
	m_suspension_center_of_mass_world = m_orientation_matrix.Rotate(m_suspension_center_of_mass);
	
	calculate_inverse_moment();
	
	update_velocities();
	
	// update wheel feeler endpoint positions
	for	(int n = m_num_wheels; n--; )
	{
		SWheel& wheel = mp_wheels[n];
		
		wheel.feeler_start_world = m_pos + m_orientation_matrix.Rotate(wheel.pos);
		wheel.feeler_end_world = wheel.pos;
		wheel.feeler_end_world[Y] += wheel.y_offset_hang - wheel.radius;
		wheel.feeler_end_world = m_pos + m_orientation_matrix.Rotate(wheel.feeler_end_world);
	}
	
	// update collider world positions
	for (int collider_idx = vVP_NUM_COLLIDERS; collider_idx--; )
	{
		SCollider& collider = mp_colliders[collider_idx];
		
		collider.world.m_corner = m_pos + m_orientation_matrix.Rotate(collider.body.m_corner);
		collider.world.m_first_edge = m_orientation_matrix.Rotate(collider.body.m_first_edge);
        collider.world.m_second_edge = m_orientation_matrix.Rotate(collider.body.m_second_edge);
		
		collider.first_edge_direction_world = collider.world.m_first_edge / collider.first_edge_length;
		collider.second_edge_direction_world = collider.world.m_second_edge / collider.second_edge_length;
	}
	
	update_collision_cache();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CVehicleComponent::calculate_friction_coefficient ( SWheel& wheel, float velocity ) const
{
	// friction based on section 2.4 and 2.7 of Race Car Vehicle Dynamics by Milliken
	
	// the tire friction coefficient curve is very simple; just four concatenated lines
	
	if (m_controls.handbrake)
	{
		// handbrake friction is a total scam
		
		float friction;
		if (m_controls.throttle)
		{
			friction = wheel.handbrake_throttle_friction;
			wheel.state = SWheel::HANDBRAKE_THROTTLE;
		}
		else
		{
			friction = wheel.handbrake_locked_friction;
			wheel.state = SWheel::HANDBRAKE_LOCKED;
		}
		
		if (velocity < wheel.min_static_velocity)
		{
			// under gripped handbrake "skidding"
			wheel.state = SWheel::UNDER_GRIPPING;
			return friction * velocity / wheel.min_static_velocity;
		}
		else
		{
			// handbrake "skidding"
			return friction;
		}
	}
	
	float multiplier = m_controls.brake ? 2.0f : 1.0f;
	
	if (velocity < wheel.min_static_velocity)
	{
		// under gripped
		wheel.state = SWheel::UNDER_GRIPPING;
		return wheel.static_friction * velocity / wheel.min_static_velocity;
	}
	else if (velocity < wheel.max_static_velocity)
	{
		// maximum grip
		wheel.state = SWheel::GRIPPING;
		return multiplier * wheel.static_friction;
	}
	else if (velocity < wheel.min_dynamic_velocity)
	{
		// on the verge of skidding
		wheel.state = SWheel::SLIPPING;
		return multiplier * wheel.dynamic_friction + (wheel.static_friction - wheel.dynamic_friction)
			* (velocity - wheel.max_static_velocity) / (wheel.min_dynamic_velocity - wheel.max_static_velocity);
	}
	else
	{
		// skidding
		wheel.state = SWheel::SKIDDING;
		return multiplier * wheel.dynamic_friction;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::calculate_friction_coefficients (   )
{
	// calculate the wheels' friction coefficients this frame
	for (int n = m_num_wheels; n--; )
	{
		SWheel& wheel = mp_wheels[n];
		
		if (wheel.state == SWheel::OUT_OF_CONTACT) continue;
		
		// project the wheel's velocity due to the body's motion into the plane of the contact surface
		Mth::Vector projected_vel = wheel.vel_world;
		projected_vel.ProjectToPlane(wheel.contact_normal);
	
		// calculate the forward direction of the wheel
		Mth::Vector wheel_direction(sinf(wheel.steering_angle), 0.0f, cosf(wheel.steering_angle));
		wheel_direction = m_orientation_matrix.Rotate(wheel_direction);
		
		// project that forward direction onto the contact surface
		wheel_direction.ProjectToPlane(wheel.contact_normal);
		wheel_direction.Normalize();
		
		// NOTE: from this point and up, the calculation is repeated exactly in apply_friction_forces(); we cache the rotated and projected
		// wheel direction and the wheel velocity due to body velocity; because the wheel velocity will change before apply_friction_forces(),
		// the calculation below must be repeated
		
		wheel.cache_projected_direction = wheel_direction;
		wheel.cache_projected_vel = projected_vel;
	
		// subtract the contact patch velocity; positive wheel rotational velocity equals negative contact patch velocity
		projected_vel -= wheel.rotvel * wheel.radius * wheel_direction;
	
		// calculate the friction coefficient
		wheel.friction_coefficient = calculate_friction_coefficient(wheel, projected_vel.Length());
	}
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::calculate_inverse_moment (   )
{
	// m_inv_moment = m_orientation_matrix * m_inv_moment_body_diag_matrix * m_orientation_matrix_transpose
	
	m_inv_moment[X][X] = m_inv_moment_body[X] * m_orientation_matrix[X][X];
	m_inv_moment[X][Y] = m_inv_moment_body[Y] * m_orientation_matrix[X][Y];
	m_inv_moment[X][Z] = m_inv_moment_body[Z] * m_orientation_matrix[X][Z];
	m_inv_moment[Y][X] = m_inv_moment_body[X] * m_orientation_matrix[Y][X];
	m_inv_moment[Y][Y] = m_inv_moment_body[Y] * m_orientation_matrix[Y][Y];
	m_inv_moment[Y][Z] = m_inv_moment_body[Z] * m_orientation_matrix[Y][Z];
	m_inv_moment[Z][X] = m_inv_moment_body[X] * m_orientation_matrix[Z][X];
	m_inv_moment[Z][Y] = m_inv_moment_body[Y] * m_orientation_matrix[Z][Y];
	m_inv_moment[Z][Z] = m_inv_moment_body[Z] * m_orientation_matrix[Z][Z];
	
	Mth::Matrix orientation_matrix_transpose;
	orientation_matrix_transpose.Transpose(m_orientation_matrix);
	m_inv_moment = m_inv_moment * orientation_matrix_transpose;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CVehicleComponent::determine_effective_gear ( float wheel_rotvel )
{
	int gear = 0;
	
	do
	{
		float engine_rotvel = wheel_rotvel * m_engine.differential_ratio * m_engine.p_gear_ratios[gear];
		
		if (engine_rotvel > m_engine.upshift_rotvel)
		{
			if (gear < m_engine.num_gears - 1)
			{
				gear++;
				continue;
			}
		}
		return gear;
	}
	while (true);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::update_collision_cache (   )
{
	Mth::CBBox collision_bbox;
	
	// add wheel location feelers
	for	(int n = m_num_wheels; n--; )
	{
		SWheel& wheel = mp_wheels[n];
		
		collision_bbox.AddPoint(wheel.feeler_start_world);
		collision_bbox.AddPoint(wheel.feeler_end_world);
	}
	
	// add body collision feelers
	for (int collider_idx = vVP_NUM_COLLIDERS; collider_idx--; )
	{
		SCollider& collider = mp_colliders[collider_idx];
		
        collision_bbox.AddPoint(collider.world.m_corner);
		collision_bbox.AddPoint(collider.world.m_corner + collider.world.m_first_edge);
		collision_bbox.AddPoint(collider.world.m_corner + collider.world.m_second_edge);
		collision_bbox.AddPoint(collider.world.m_corner + collider.world.m_first_edge + collider.world.m_second_edge);
	}
	
	m_collision_cache.Update(collision_bbox);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::update_wheel_heights (   )
{
	// We crap out and don't do real wheel dynamics.  Instead, if the ground is within a wheel's hang point, we teleport the wheel to the
	// ground.  If not, we teleport the wheel to its hang point.
	
	CFeeler feeler;
	
	feeler.SetCache(&m_collision_cache);
	
	m_num_wheels_in_contact = 0;
	for	(int n = m_num_wheels; n--; )
	{
		SWheel& wheel = mp_wheels[n];
		
		// y_offset a feeler from the top of the wheel's y_offset to the wheel's hang point plus its radius
		feeler.m_start = wheel.feeler_start_world;
		feeler.m_end = wheel.feeler_end_world;
		feeler.SetIgnore(mFD_NON_COLLIDABLE, 0);
		
		if (feeler.GetCollision(false))
		{
			// trip any triggers
			if (wheel.state != SWheel::OUT_OF_CONTACT)
			{
				if (feeler.GetSector() != wheel.last_ground_feeler.GetSector())
				{
					trip_trigger(TRIGGER_SKATE_OFF, wheel.last_ground_feeler);
					if (reset_this_frame()) return;
					trip_trigger(TRIGGER_SKATE_ONTO, feeler);
					if (reset_this_frame()) return;
				}
			}
			else
			{
				trip_trigger(TRIGGER_SKATE_ONTO, feeler);
				if (reset_this_frame()) return;
			}
			
			wheel.last_ground_feeler = feeler;
			
			wheel.state = SWheel::NO_STATE;
			wheel.contact_normal = feeler.GetNormal();
			wheel.y_offset = feeler.GetDist() * (wheel.y_offset_hang - wheel.radius) + wheel.radius;
			
			m_num_wheels_in_contact++;
			
			feeler.m_end = feeler.GetPoint();
		}
		else
		{
			// trip any triggers
			if (wheel.state != SWheel::OUT_OF_CONTACT)
			{
				trip_trigger(TRIGGER_SKATE_OFF, wheel.last_ground_feeler);
				if (reset_this_frame()) return;
			}
			
			wheel.state = SWheel::OUT_OF_CONTACT;
			wheel.y_offset = wheel.y_offset_hang;
		}
		
		// Allows CAP kill planes to work on the car.
		if (Ed::CParkEditor::Instance()->UsingCustomPark())
		{
			// now check for non-collidable trigger polys
			feeler.SetIgnore(0, mFD_NON_COLLIDABLE | mFD_TRIGGER);
			if (feeler.GetCollision(false))
			{
				trip_trigger(TRIGGER_BONK, feeler);
				if (reset_this_frame()) return;
			}
		}
		
	} // END loop over wheels
	
	// update the wheels' dependent variables which depend of the wheel heights
	for (int n = m_num_wheels; n--; )
	{
		SWheel& wheel = mp_wheels[n];
		
		wheel.pos_world = wheel.pos;
		wheel.pos_world[Y] += wheel.y_offset - wheel.radius;
		wheel.pos_world = m_orientation_matrix.Rotate(wheel.pos_world);
		
		wheel.vel_world = calculate_body_point_velocity(wheel.pos_world);
	} // END loop over wheels
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::update_steering_angles (   )
{
	// NOTE: could easily use a table here
	
	m_steering_display = Mth::Lerp(m_steering_display, m_controls.steering, 10.0f * m_time_step);
	
	float left_steering_angle;
	float right_steering_angle;
	float left_steering_angle_display;
	float right_steering_angle_display;
	
	if (Mth::Abs(m_controls.steering) > 0.001f)
	{
		float turning_radius = m_cornering_wheelbase / tanf(m_controls.steering * m_max_steering_angle);
		 
		if (turning_radius > 0.0f)
		{
			left_steering_angle = -atan2f(m_cornering_wheelbase, turning_radius - m_cornering_axle_length / 2.0f);
			right_steering_angle = -atan2f(m_cornering_wheelbase, turning_radius + m_cornering_axle_length / 2.0f);
		}
		else
		{
			left_steering_angle = atan2f(m_cornering_wheelbase, -turning_radius + m_cornering_axle_length / 2.0f);
			right_steering_angle = atan2f(m_cornering_wheelbase, -turning_radius - m_cornering_axle_length / 2.0f);
		}
	}
	else
	{
		left_steering_angle = right_steering_angle = 0.0f;
	}
		
	if (Mth::Abs(m_steering_display) > 0.001f)
	{
		float turning_radius_display = m_cornering_wheelbase / tanf(m_steering_display * m_max_steering_angle);
		 
		if (turning_radius_display > 0.0f)
		{
			left_steering_angle_display = -atan2f(m_cornering_wheelbase, turning_radius_display - m_cornering_axle_length / 2.0f);
			right_steering_angle_display = -atan2f(m_cornering_wheelbase, turning_radius_display + m_cornering_axle_length / 2.0f);
		}
		else
		{
			left_steering_angle_display = atan2f(m_cornering_wheelbase, -turning_radius_display + m_cornering_axle_length / 2.0f);
			right_steering_angle_display = atan2f(m_cornering_wheelbase, -turning_radius_display - m_cornering_axle_length / 2.0f);
		}
	}
	else
	{
		left_steering_angle_display = right_steering_angle_display = 0.0f;
	}
	
	for (int n = m_num_wheels; n--; )
	{
		SWheel& wheel = mp_wheels[n];
		
		if (wheel.steering == SWheel::FIXED) continue;
		
        wheel.steering_angle = (wheel.steering == SWheel::LEFT ? left_steering_angle : right_steering_angle);
        wheel.steering_angle_display = (wheel.steering == SWheel::LEFT ? left_steering_angle_display : right_steering_angle_display);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::damp_rotation (   )
{
	// no damping when handbraking on the ground
	if (m_controls.handbrake && m_num_wheels_in_contact > 2) return;
	
	if (!m_controls.throttle && m_controls.steering != 0.0f && m_num_wheels_in_contact > 2) return;
	
	if (in_artificial_collision()) return;
	
	// apply quadtratic damping
	
	// quadratic damping in the air makes jump recovery much easier; the vehicle lands on its wheels much more often
	float quad_damping = m_time_step * m_quad_rotvel_damping * m_rotmom.LengthSqr();
	
	// prevent reversal
	if (quad_damping > m_rotmom.Length())
	{
		m_rotmom.Set(0.0f, 0.0f, 0.0f);
	}
	else
	{
		Mth::Vector direction = m_rotmom;
		direction.Normalize();
		m_rotmom -= quad_damping * direction;
	}

	// apply constant damping; only applied in the Y direction
	
	// apply only if we're not steering, we're on the ground, and we're relatively upright
	if (m_controls.steering == 0.0f && m_num_wheels_in_contact > 2 && m_orientation_matrix[Y][Y] > 0.5f)
	{
		float const_damping = m_time_step * m_const_rotvel_damping;;
		
		// prevent reversal
		if (Mth::Abs(const_damping) > Mth::Abs(m_rotmom[Y]))
		{
			m_rotmom[Y] = 0.0f;
		}
		else
		{
			m_rotmom[Y] += (m_rotmom[Y] > 0.0f ? -const_damping : const_damping);
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::accumulate_forces (   )
{
	// forces and torques on the body accumulate and are applied at the end of the frame; torques on the wheels are too large to accumulate;
	// they are applied to wheel velocity as they occur; thus, the order of application of wheel torques is critical
	
	apply_gravitational_forces();
	
	apply_suspension_forces();
	
    // we calculate the friction coefficients before applying engine torques so that friction will get the chance to counteract rotational
	// accelerations using static friction before the wheels spin out due to engine torque
	calculate_friction_coefficients();
	
	if (!m_controls.handbrake)
	{
		apply_engine_forces();
		
		apply_drag_forces();
		
		// give the brakes a chance to cancel the rotation due to engine torque and to lock up the wheels before friction effects
		apply_brake_forces();
		
		apply_friction_forces();
		
		// after friction, the brakes are given the opportunity to apply additional torque and relock the wheels
		apply_spare_brake_forces();
	}
	else
	{
		if (m_controls.throttle)
		{
			apply_engine_forces();
			
            // scale the handbrake's braking effect with the steering factor; thus, you power through sharp handbrake turns; yet, the handbrake
			// still brakes on straight aways
			apply_handbrake_forces(1.0f - Mth::Abs(m_controls.steering));

			apply_friction_forces();

			apply_spare_brake_forces();
		}
		else
		{
			apply_friction_forces();
			
			// lock wheels
			for (int n = m_num_wheels; n--; )
			{
				mp_wheels[n].rotvel = 0.0f;
				mp_wheels[n].rotacc = 0.0f;
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::apply_gravitational_forces (   )
{
	// use reduced gravity when flipping the car
	if (m_in_flip && Tmr::ElapsedTime(m_flip_start_time_stamp) > vVP_FLIP_DELAY)
	{
		m_force[Y] -= vVP_FLIP_GRAVITY_FACTOR * m_gravity_override_fraction * vVP_GRAVITATIONAL_ACCELERATION / m_inv_mass;
	}
	else
	{
		m_force[Y] -= m_gravity_override_fraction * vVP_GRAVITATIONAL_ACCELERATION / m_inv_mass;
	}
	
	// we do our own force accumulation for this simple and constant force, having the same effect as the following code
	// Mth::Vector force(0.0f, -vVP_GRAVITATIONAL_ACCELERATION / m_inv_mass, 0.0f);
	// Mth::Vector location = m_suspension_center_of_mass;
	// accumulate_force(force, location, MAKE_RGB(50, 50, 0));
	
	if (m_draw_debug_lines)
	{
		Gfx::AddDebugLine(m_pos, m_pos + 0.05f * Mth::Vector(0.0f, -vVP_GRAVITATIONAL_ACCELERATION / m_inv_mass, 0.0f), MAKE_RGB(50, 50, 0), MAKE_RGB(50, 50, 0), 1);
	}
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::apply_suspension_forces (   )
{
	for (int n = m_num_wheels; n--; )
	{
		SWheel& wheel = mp_wheels[n];
		
		// if not in contact with the ground, y_offset == y_offset_hang
		if (wheel.state == SWheel::OUT_OF_CONTACT)
		{
			// set this frame's history to the default value to smooth out landings
			wheel.normal_force_history[m_next_normal_force_history_idx] = vVP_GRAVITATIONAL_ACCELERATION / m_inv_mass / m_num_wheels;
			continue;
		}
		
		// direction of the suspension line; commented version is slower but explicit
		// Mth::Vector direction = m_orientation_matrix.Rotate(Mth::Vector(0.0f, 1.0f, 0.0f));
		Mth::Vector direction = m_orientation_matrix[Y];
		
		// magnitude of the spring force along the contact normal
		float magnitude = -wheel.spring * (wheel.y_offset_hang - wheel.y_offset) * Mth::DotProduct(wheel.contact_normal, direction);
		
		// velocity of the wheel projected into the contact normal direction
		float projected_vel = Mth::DotProduct(wheel.vel_world, wheel.contact_normal);
		
		// magnitude adjusted for damping
		magnitude += -wheel.damping * projected_vel;
		
		// suspension force
		Mth::Vector force = magnitude * wheel.contact_normal;
		
		accumulate_force(force, wheel.pos_world, MAKE_RGB(100, 100, 0));
		
		// set the normal force magnitude; used in the friction force code
		
		wheel.normal_force_history[m_next_normal_force_history_idx] = magnitude;
		
		wheel.normal_force = 0.0f;
		for (int i = vVP_NORMAL_FORCE_HISTORY_LENGTH; i--; )
		{
			wheel.normal_force += wheel.normal_force_history[i];
		}
		wheel.normal_force /= vVP_NORMAL_FORCE_HISTORY_LENGTH;
	}
	
	m_next_normal_force_history_idx = ++m_next_normal_force_history_idx % vVP_NORMAL_FORCE_HISTORY_LENGTH;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::apply_friction_forces (   )
{
	// NOTE: we've handled longitudinal friction reversal successfully; however, tangential friction reversal is still a significant issue for large
	// friction coefficients and high centers of mass
	
	// we use a simple tire model were the friction force is opposite the velocity of the tire contact and proportional to the normal force and a
	// friction coefficient; the friction coefficient is a function of the velocity of the contact with the following form:
	// 0 < v < min_static_velocity: rises linearly from zero to static_friction
	// min_static_velocity < v < max_static_velocity: constant at static_friction
	// max_static_velocity < v < min_dynamic_velocity: falls linearly from static_friction to dynamic_friction
	// min_dynamic_velocity < v: constant at dynamic_friction
	
	// NOTE: because friction is zero at zero slip velocity, hill slippage is an issue
	
	for (int n = m_num_wheels; n--; )
	{
		SWheel& wheel = mp_wheels[n];
		
		if (wheel.state == SWheel::OUT_OF_CONTACT) continue;
		
		// grab the cached wheel direction and velocity projected into the contact plane
		Mth::Vector projected_wheel_direction = wheel.cache_projected_direction;
		Mth::Vector projected_vel = wheel.cache_projected_vel;
		
		// subtract the contact patch velocity; positive wheel rotational velocity equals negative contact patch velocity
		projected_vel -= wheel.rotvel * wheel.radius * projected_wheel_direction;
		
		// store the slip vel for later display
		wheel.slip_vel = projected_vel.Length();
		
		// if the velocity is too small, its direction is meaningless
		if (wheel.slip_vel < 0.000001) continue;
		
		// propose a frictional force
		Mth::Vector vel_direction = projected_vel;
		vel_direction.Normalize();
		Mth::Vector proposed_force = -wheel.friction_coefficient * wheel.normal_force * vel_direction;
		
		// now we must check that this force is not more that the force which would stop the relative longitudinal velocity
		
		// calculate the torque required to stick the wheel contact patch to the ground
		float longitudinal_vel = Mth::DotProduct(projected_vel, projected_wheel_direction);
		float stopping_rotvel = longitudinal_vel / wheel.radius;
		float stopping_torque = -calculate_stopping_torque(wheel, stopping_rotvel);
		
		// calculate the proposed torque
		float proposed_torque = -wheel.radius * Mth::DotProduct(proposed_force, projected_wheel_direction);
		
		Mth::Vector force;
		float torque;
		
		// if the proposed torque is less than what is required to stop the rotation
		if (Mth::Abs(proposed_torque) < Mth::Abs(stopping_torque))
		{
			// we're ok
			force = proposed_force;
			torque = proposed_torque;
		}
		else
		{
			// otherwise, reduce the proposed longitudinal force to only the stopping force
			float proposed_longitudinal_force = Mth::DotProduct(proposed_force, projected_wheel_direction);
			float stopping_longitudinal_force = -stopping_torque / wheel.radius;
			force = proposed_force + (stopping_longitudinal_force - proposed_longitudinal_force) * projected_wheel_direction;
			
			torque = stopping_torque;
		}
		
		// apply the force to the body
		accumulate_force(force, wheel.pos_world, MAKE_RGB(0, 0, 255));
		
		// apply the torque to the wheel
		apply_wheel_torque(wheel, torque);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::apply_drag_forces (   )
{
	if (m_controls.throttle || m_controls.reverse || m_controls.brake) return;
	
	for (int n = m_num_wheels; n--; )
	{
		SWheel& wheel = mp_wheels[n];
		
		if (!wheel.drive || wheel.state == SWheel::OUT_OF_CONTACT) continue;
		
		// determine the effective engine gear
		int effective_gear = determine_effective_gear(Mth::Abs(wheel.rotvel));
		
		// drag torque scales like the square of the gear ratio
		float torque = m_engine.drag_torque * m_engine.p_gear_ratios[effective_gear] * m_engine.p_gear_ratios[effective_gear] / m_num_drive_wheels;
		
		// prevent reversal
		float maximum_torque = Mth::Abs(calculate_stopping_torque(wheel, wheel.rotvel));
		if (torque > maximum_torque)
		{
			torque = maximum_torque;
		}
		
		apply_wheel_torque(wheel, wheel.rotvel < 0.0f ? torque : -torque);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::apply_engine_forces (   )
{
	// NOTE: test
	float wall_climb_factor = Mth::ClampMax(m_orientation_matrix[Y][Y] * m_orientation_matrix[Y][Y] / 0.9f, 1.0f);
	
	if ((!m_controls.throttle && !m_controls.reverse) || m_controls.brake) return;
	
	for (int n = m_num_wheels; n--; )
	{
		SWheel& wheel = mp_wheels[n];
		
		if (!wheel.drive) continue;
		
		// turn off power to front wheels when power handbraking
		if (m_controls.handbrake && wheel.steering != SWheel::FIXED) continue;
		
		// determine effective engine gear
		int effective_gear;
		if (!m_controls.reverse)
		{
			effective_gear = determine_effective_gear(wheel.rotvel > 0.0f ? wheel.rotvel : 0.0f);
		}
		else
		{
			effective_gear = determine_effective_gear(wheel.rotvel < 0.0f ? -wheel.rotvel : 0.0f);
		}
		
		float torque = m_engine.drive_torque * m_engine.differential_ratio * m_engine.p_gear_ratios[effective_gear] / m_num_drive_wheels;
		torque *= wall_climb_factor;
		
		// reverse gears are scaled down by a factor
		if (m_controls.reverse)
		{
			torque *= -m_engine.reverse_torque_ratio;
		}
		
		// account for low of power in front wheels in four-wheel drive car when power handbraking
		if (m_controls.handbrake && m_num_drive_wheels == 4) torque *= 2.0f;
		
		apply_wheel_torque(wheel, torque);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::apply_brake_forces (   )
{
	if (!m_controls.brake) return;
	
	for (int n = m_num_wheels; n--; )
	{
		SWheel& wheel = mp_wheels[n];
		
		float torque = wheel.brake.torque;
		
		float maximum_torque = Mth::Abs(calculate_stopping_torque(wheel, wheel.rotvel));
		
		if (torque > maximum_torque)
		{
			wheel.brake.spare_torque = torque - maximum_torque;
			torque = maximum_torque;
		}
		else
		{
			wheel.brake.spare_torque = 0.0f;
		}
		
		apply_wheel_torque(wheel, wheel.rotvel < 0.0f ? torque : -torque);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::apply_handbrake_forces ( float application_factor )
{
	Dbg_Assert(m_controls.handbrake);
	
	for (int n = m_num_wheels; n--; )
	{
		SWheel& wheel = mp_wheels[n];
		
		float torque = application_factor * wheel.brake.handbrake_torque;
		
		float maximum_torque = Mth::Abs(calculate_stopping_torque(wheel, wheel.rotvel));
		
		if (torque > maximum_torque)
		{
			wheel.brake.spare_torque = torque - maximum_torque;
			torque = maximum_torque;
		}
		else
		{
			wheel.brake.spare_torque = 0.0f;
		}
		
		apply_wheel_torque(wheel, wheel.rotvel < 0.0f ? torque : -torque);
	}
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::apply_spare_brake_forces (   )
{
	if (!m_controls.brake) return;
	
	for (int n = m_num_wheels; n--; )
	{
		SWheel& wheel = mp_wheels[n];
		
		if (wheel.brake.spare_torque == 0.0f) continue;
		
		float torque = wheel.brake.spare_torque;
		
		float maximum_torque = Mth::Abs(calculate_stopping_torque(wheel, wheel.rotvel));
		
		if (torque > maximum_torque)
		{
			torque = maximum_torque;
		}
		
		apply_wheel_torque(wheel, wheel.rotvel < 0.0f ? torque : -torque);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
			
float CVehicleComponent::calculate_collision_depth ( const SCollisionPoint& collision_point, const SCollider& collider ) const
{
	enum EDepthDirection
	{
		WITH_FIRST_EDGE, AGAINST_FIRST_EDGE, WITH_SECOND_EDGE, AGAINST_SECOND_EDGE
	};
	
	// calculate displacement from base corner
	Mth::Vector displacement = collision_point.pos - collider.world.m_corner;
	
	// determine the minimum depth and correponding depth direction
	
	float min_depth = Mth::DotProduct(displacement, collider.first_edge_direction_world);
	EDepthDirection depth_direction = WITH_FIRST_EDGE;
	
	float depth = collider.first_edge_length - min_depth;
	if (depth < min_depth)
	{
		min_depth = depth;
		depth_direction = AGAINST_FIRST_EDGE;
	}
	
	depth = Mth::DotProduct(displacement, collider.second_edge_direction_world);
	if (depth < min_depth)
	{
		min_depth = depth;
		depth_direction = WITH_SECOND_EDGE;
	}
	
	depth = collider.second_edge_length - depth;
	if (depth < min_depth)
	{
		min_depth = depth;
		depth_direction = AGAINST_SECOND_EDGE;
	}
	
	if (min_depth < 0.01f)
	{
		return 0.0f;
	}
	
	switch (depth_direction)
	{
		case WITH_FIRST_EDGE:
			depth = min_depth / Mth::DotProduct(collider.first_edge_direction_world, collision_point.normal);
			break;
			
		case AGAINST_FIRST_EDGE:
			depth = -min_depth / Mth::DotProduct(collider.first_edge_direction_world, collision_point.normal);
			break;
		
		case WITH_SECOND_EDGE:
			depth = min_depth / Mth::DotProduct(collider.second_edge_direction_world, collision_point.normal);
			break;
		
		case AGAINST_SECOND_EDGE:
			depth = -min_depth / Mth::DotProduct(collider.second_edge_direction_world, collision_point.normal);
			break;
	}
	
	#ifdef __USER_DAN__
	if (depth < 0.0f)
	{
		MESSAGE("negative depth bug");
	}
	#endif
	
	return depth;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
			
bool CVehicleComponent::check_for_capping ( SCollisionPoint& collision_point, const CRectFeeler& rect_feeler, const SCollider& collider, int collision_line_idx, int collision_point_end_idx ) const
{
	// check to see if this culled collision is the back of a cappable collision with a 2D object
	for (int check_line_idx = rect_feeler.GetNumMergedCollisionSurfaces(); check_line_idx--; )
	{
		if (check_line_idx == collision_line_idx) continue;
		
		for (int end = 2; end--; )
		{
			if (very_close(collision_point.pos, rect_feeler.GetMergedCollisionSurface(check_line_idx).ends[end].point)
				&& Mth::DotProduct(collision_point.normal, rect_feeler.GetMergedCollisionSurface(check_line_idx).normal) < -0.99)
			{
				// we'll use this point to cap the collision with the 2D object; the tangent cross the normal will point out of the triangle
				collision_point.normal = Mth::CrossProduct(
					rect_feeler.GetMergedCollisionSurface(collision_line_idx).ends[collision_point_end_idx].tangent,
					collision_point.normal
				);
				collision_point.depth = calculate_collision_depth(collision_point, collider);
				
				if (m_draw_debug_lines)
				{
					Gfx::AddDebugLine(
						collision_point.pos,
						collision_point.pos + (72) * collision_point.normal,
						MAKE_RGB(255, 100, 255), MAKE_RGB(255, 100, 255), 1
					);
				}
				
				return true;
			}
		} // END loop over checked collision line ends
	} // END loop over all other collision lines

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CVehicleComponent::consider_culling_point ( const SCollisionPoint& collision_point ) const
{
	// cull collision points which have a normal direction along their body offset
	
	Mth::Vector offset = collision_point.pos - m_pos;
	bool cull = Mth::DotProduct(offset, collision_point.normal) >= 0.0f;
	if (m_draw_debug_lines && cull)
	{
		uint32 c = MAKE_RGB(0, 0, 0);
		Gfx::AddDebugLine(collision_point.pos, collision_point.pos + 48.0f * collision_point.normal, c, c, 1);
	}
	return cull;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::apply_environment_collisions (   )
{
	m_max_normal_collision_impulse = 0.0f;
	
	CRectFeeler rect_feeler;
	CLineFeeler line_feeler;
	
	rect_feeler.SetCache(&m_collision_cache);
	line_feeler.SetCache(&m_collision_cache);
	
	m_num_collision_points = 0;
	for (int collider_idx = vVP_NUM_COLLIDERS; collider_idx--; )
	{
		SCollider& collider = mp_colliders[collider_idx];
		
		// first we use a rectangular feeler
		
		// setup the feeler
		rect_feeler.SetRectangle(collider.world);
		
		// detect collisions
		if (rect_feeler.GetCollision())
		{
			trip_trigger(TRIGGER_BONK, rect_feeler);
			if (reset_this_frame()) return;
			
			rect_feeler.MergeCollisionSurfaces();
			
			// copy out the collision points from the feeler output
			for (int collision_line_idx = rect_feeler.GetNumMergedCollisionSurfaces(); collision_line_idx--; )
			{
				for (int end = 2; end--; )
				{
					// add the collision line's start to the list of collision points
					SCollisionPoint& new_collision_point = sp_collision_points[m_num_collision_points];
					
					new_collision_point.line = false;
					
					new_collision_point.pos = rect_feeler.GetMergedCollisionSurface(collision_line_idx).ends[end].point;
					new_collision_point.normal = rect_feeler.GetMergedCollisionSurface(collision_line_idx).normal;
					if (!consider_culling_point(new_collision_point))
					{
						// if this is a keeper, calculate the collision depth
						if (rect_feeler.GetMergedCollisionSurface(collision_line_idx).ends[end].tangent_exists)
						{
							new_collision_point.depth = calculate_collision_depth(new_collision_point, collider);
						}
						else
						{
							new_collision_point.depth = 0.0f;
						}
						m_num_collision_points++;
					}
					else if (rect_feeler.GetMergedCollisionSurface(collision_line_idx).ends[end].tangent_exists)
					{
						if (check_for_capping(new_collision_point, rect_feeler, collider, collision_line_idx, end))
						{
							m_num_collision_points++;
						}
					}
				} // END loop over collision line ends
			} // END loop over collision surfaces
		} // END if we had a collision this collider
    
		if (m_draw_debug_lines)
		{
			rect_feeler.DebugLines(255, 255, 255, 1);
		}
		
		// line feelers

		line_feeler.m_start = m_pos;

		line_feeler.m_end = rect_feeler.m_corner;
		if (line_feeler.GetCollision(false))
		{
			trip_trigger(TRIGGER_BONK, line_feeler);
			if (reset_this_frame()) return;
			
			sp_collision_points[m_num_collision_points].pos = line_feeler.GetPoint();
			sp_collision_points[m_num_collision_points].normal = line_feeler.GetNormal();
			if (!consider_culling_point(sp_collision_points[m_num_collision_points]))
			{
				sp_collision_points[m_num_collision_points].depth = calculate_collision_depth(line_feeler);
				m_num_collision_points++;
				sp_collision_points[m_num_collision_points].line = true;
			}
		}
		if (m_draw_debug_lines)
		{
			line_feeler.DebugLine(255, 255, 255, 1);
		}


		line_feeler.m_end += rect_feeler.m_first_edge;
		if (line_feeler.GetCollision(false))
		{
			trip_trigger(TRIGGER_BONK, line_feeler);
			if (reset_this_frame()) return;
			
			sp_collision_points[m_num_collision_points].pos = line_feeler.GetPoint();
			sp_collision_points[m_num_collision_points].normal = line_feeler.GetNormal();
			if (!consider_culling_point(sp_collision_points[m_num_collision_points]))
			{
				sp_collision_points[m_num_collision_points].depth = calculate_collision_depth(line_feeler);
				m_num_collision_points++;
				sp_collision_points[m_num_collision_points].line = true;
			}
		}
		if (m_draw_debug_lines)
		{
			line_feeler.DebugLine(255, 255, 255, 1);
		}

		line_feeler.m_end += rect_feeler.m_second_edge;
		if (line_feeler.GetCollision(false))
		{
			trip_trigger(TRIGGER_BONK, line_feeler);
			if (reset_this_frame()) return;
			
			sp_collision_points[m_num_collision_points].pos = line_feeler.GetPoint();
			sp_collision_points[m_num_collision_points].normal = line_feeler.GetNormal();
			if (!consider_culling_point(sp_collision_points[m_num_collision_points]))
			{
				sp_collision_points[m_num_collision_points].depth = calculate_collision_depth(line_feeler);
				m_num_collision_points++;
				sp_collision_points[m_num_collision_points].line = true;
			}
		}
		if (m_draw_debug_lines)
		{
			line_feeler.DebugLine(255, 255, 255, 1);
		}

		line_feeler.m_end -= rect_feeler.m_first_edge;
		if (line_feeler.GetCollision(false))
		{
			trip_trigger(TRIGGER_BONK, line_feeler);
			if (reset_this_frame()) return;
			
			sp_collision_points[m_num_collision_points].pos = line_feeler.GetPoint();
			sp_collision_points[m_num_collision_points].normal = line_feeler.GetNormal();
			if (!consider_culling_point(sp_collision_points[m_num_collision_points]))
			{
				sp_collision_points[m_num_collision_points].depth = calculate_collision_depth(line_feeler);
				m_num_collision_points++;
				sp_collision_points[m_num_collision_points].line = true;
			}
		}
		if (m_draw_debug_lines)
		{
			line_feeler.DebugLine(255, 255, 255, 1);
		}
	} // END loop over colliders
	
	if (m_num_collision_points == 0) return;
	
	// zero accumulators
	for (int collision_idx = m_num_collision_points; collision_idx--; )
	{
		sp_collision_points[collision_idx].normal_impulse = 0.0f;
	}
	
	// cache the momentums and velocities before normal impulses are applied; they are used when determining the frictional forces
	Mth::Vector cache_mom = m_mom;
	Mth::Vector cache_rotmom = m_rotmom;
	Mth::Vector cache_vel = m_vel;
	Mth::Vector cache_rotvel = m_rotvel;
	
	// apply normal impulses
	
	// loop over normal impulse points until all collisions are fully resolved
	int pass_count = 0;
	bool clean_pass;
	do
	{
		clean_pass = true;
		
		for (int collision_idx = m_num_collision_points; collision_idx--; )
		{
			SCollisionPoint& collision = sp_collision_points[collision_idx];
			
			// calculate the collision position in body space
			Mth::Vector pos_local = collision.pos - m_pos;
			
			// calculate the velocity and normal velocity of the collision point
			Mth::Vector vel = calculate_body_point_velocity(pos_local);
			float normal_vel = Mth::DotProduct(vel, collision.normal);
			
			// ignore if we're not impacting
			if (normal_vel > 0.0f) continue;
			clean_pass = false;
			
			// calculate the goal velocity of the collision
			float goal_normal_velocity = -m_body_restitution * normal_vel;
			
			// calculate the effective mass of the collision point
			float effective_mass = calculate_body_point_effective_mass(pos_local, collision.normal);
			
			// calculate the normal impulse required to reach the body velocity
			float normal_impulse = (goal_normal_velocity - normal_vel) * effective_mass;
			
			// accumulate the total normal impulse applied to this collision point
			collision.normal_impulse += normal_impulse;
			
			// setup the required impulse vector
			Mth::Vector impulse = normal_impulse * collision.normal;
			
			// apply the normal impulse immediately
			apply_impulse(impulse, pos_local);
		}

		if (++pass_count > 5)
		{
			#ifdef __USER_DAN__
			MESSAGE("environment collision resolution pass count limit exceeded");
			#endif
			break;
		}
	} while (!clean_pass);
	
    if (m_draw_debug_lines)
	{
		for (int collision_idx = m_num_collision_points; collision_idx--; )
		{
			// SCollisionPoint& collision = sp_collision_points[collision_idx];
			// Gfx::AddDebugLine(
				// collision.pos + collision.normal,
				// collision.pos + collision.normal + collision.normal_impulse / 2.0f * collision.normal,
				// MAKE_RGB(0, 100, 100), MAKE_RGB(0, 100, 100), 1
			// );
		}
	} // END loop over collision points

	// apply penalty forces to prevent interpenetration
	for (int collision_idx = m_num_collision_points; collision_idx--; )
	{
		SCollisionPoint& collision = sp_collision_points[collision_idx];
		
		// collision normal is facing opposite the depth
		// BUG: too many rect feeler collisions have negative depth!!!
		if (collision.depth <= 0.0f) continue;
		
		// extremely deep collisions are mostly collisions with the underbody which are actually shallow in the Y direction; for now we'll cap such
		// collisions and perhaps allow the car to sink down into geometry which comes from below but misses the wheels
		if (collision.depth > 2.0f)
		{
			collision.depth = 2.0f;
		}
		
		Mth::Vector force = m_body_spring * collision.depth * collision.normal;
		
		accumulate_collision_force(force, collision.pos - m_pos);
		
		// Gfx::AddDebugLine(
			// collision.pos,
			// collision.pos + (2 * collision.depth) * collision.normal,
			// MAKE_RGB(255, 255, 0), MAKE_RGB(255, 255, 0), 1
		// );
	}
	
	float body_friction = m_orientation_matrix[Y][Y] > 0.707f ? m_body_friction : m_body_wipeout_friction;
	
	// apply a friction impulse at each point
	for (int collision_idx = m_num_collision_points; collision_idx--; )
	{
		SCollisionPoint& collision = sp_collision_points[collision_idx];
		
		if (collision.normal_impulse == 0.0f) continue;
		
		// calculate the local collision position
		Mth::Vector pos_local = collision.pos - m_pos;
		
		// calculate the tangential velocity
		Mth::Vector	vel = cache_vel + Mth::CrossProduct(cache_rotvel, pos_local);
		Mth::Vector tanjent = vel - Mth::DotProduct(vel, collision.normal) * collision.normal;
		float tanjent_vel = tanjent.Length();
		if (tanjent_vel < 0.001f) continue;
		tanjent /= tanjent_vel;
		
		// calculate the effective mass of the collision point
		float effective_mass = calculate_body_point_effective_mass(pos_local, tanjent);
		
		// calculate the tanjential impulse required to stop the tanjential velocity
		float tanjent_impulse = tanjent_vel * effective_mass;
		
		// cap the frictional force at the normal force times the coefficient of friction
		float max_tanjent_impulse = body_friction * collision.normal_impulse;
		
		if (tanjent_impulse > max_tanjent_impulse)
		{
			tanjent_impulse = max_tanjent_impulse;
		}
		
		// setup the tanjent impulse vector
		Mth::Vector impulse = -tanjent_impulse * tanjent;
		
		// apply the friction impulse to the true state
		apply_impulse(impulse, pos_local);
		
		if (m_draw_debug_lines)
		{
			// Gfx::AddDebugLine(
				// collision.pos + collision.normal,
				// collision.pos + collision.normal + impulse / 2.0f,
				// MAKE_RGB(100, 0, 100), MAKE_RGB(100, 0, 100), 1
			// );
		}
		
		// apply the friction impulse to the state we are using for friction calculations
		cache_mom += impulse;
		cache_rotmom += Mth::CrossProduct(pos_local, impulse);
		cache_vel = m_inv_mass * cache_mom;
		cache_rotvel = m_inv_moment.Rotate(cache_rotmom);
	} // END loop over collision points
	
	// determine the maximum normal impulse this frame
	for (int collision_idx = m_num_collision_points; collision_idx--; )
	{
		SCollisionPoint& collision = sp_collision_points[collision_idx];
		
		m_max_normal_collision_impulse = Mth::Max(collision.normal_impulse, m_max_normal_collision_impulse);
	}
	
	// no player controlled impulses in the air	or with handbrake on
	if (m_controls.steering == 0.0f || m_num_wheels_in_contact < 3 || m_controls.handbrake) return;
	
	// extra player controled collision impulse; allows one slight control over the direction on collision impulses
	for (int collision_idx = m_num_collision_points; collision_idx--; )
	{
		SCollisionPoint& collision = sp_collision_points[collision_idx];
				
		Mth::Vector pos_local = collision.pos - m_pos;
		
		float strength = m_controls.steering * m_collision_control * Mth::DotProduct(m_orientation_matrix[Z], collision.normal) * collision.normal_impulse;
		
		Mth::Vector direction(-collision.normal[Z], 0.0f, collision.normal[X]);
		
		apply_impulse(strength * direction, pos_local);
		
		if (m_draw_debug_lines)	  
		{
			Gfx::AddDebugLine(
				collision.pos + collision.normal,
				collision.pos + collision.normal + strength * direction / 5.0f,
				MAKE_RGB(255, 0, 100), MAKE_RGB(255, 0, 100), 1
			);
		}
	}
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::update_pos_with_uber_frig ( const Mth::Vector& movement )
{
	// The vehicle has two uber frigs.  The first treats the c.o.m. as a point particle with respect to the geo.  The seconds is a traditional uber frig.
	
	Mth::Vector frame_start_pos = m_pos;
	
	// look for collisions along the origin's movement; only do a single bounce, assuming the remaining movement is ok
	CFeeler feeler(m_pos, m_pos + movement);
	if (feeler.GetCollision(false))
	{
		// bounce off the surface
		MESSAGE("less than uber frig");
		
		m_pos = feeler.GetPoint() + feeler.GetNormal();
		
		Mth::Vector remaining_movement = feeler.GetDist() * movement;
		remaining_movement -= 2.0f * Mth::DotProduct(remaining_movement, feeler.GetNormal()) * feeler.GetNormal();
		m_pos += remaining_movement;
		
		// use coeff of restit of 0.5
		m_mom -= 1.5f * Mth::DotProduct(m_mom, feeler.GetNormal()) * feeler.GetNormal();
	}
	else
	{
		m_pos += movement;
	}
	
	// now, a traditional uber frig
	feeler.m_start = m_pos;
	feeler.m_end = feeler.m_start;
	feeler.m_end[Y] -= FEET(400.0f);
	if (!feeler.GetCollision(false))
	{
		MESSAGE("uber frig");
		m_pos = frame_start_pos;
		m_mom *= 1.0f;
		m_rotmom *= 1.0f;
	}
	else
	{
		mp_model_component->ApplyLightingFromCollision(feeler);
	}
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::slerp_to_face_velocity (  )
{
	// slerp to face our velocity
	
	if (m_in_flip) return;
	
	if (in_artificial_collision()) return;
	
	// only slerp if horizontal velocity is below a given threshold
	bool vel_below_threshold = m_vel[X] * m_vel[X] + m_vel[Z] * m_vel[Z] < m_in_air_slerp_vel_cutoff * m_in_air_slerp_vel_cutoff;
	if (vel_below_threshold && !m_vert_correction) return;

	// extract the current orientation matrix
	m_orientation.GetMatrix(m_orientation_matrix);
	m_orientation_matrix[W].Set();

	// construct our target matrix
	Mth::Matrix target;
	if (vel_below_threshold)
	{
		target[Z] = m_orientation_matrix[Z];
		target[Z][Y] = 0.0f;
		target[Z].Normalize();
	}
	else
	{
		target[Z] = m_vel;
		target[Z].Normalize();
	}
	if (Mth::DotProduct(target[Z], m_orientation_matrix[Z]) < 0.0f)
	{
		target[Z] *= -1.0f;
	}
	target[Y].Set(0.0f, 1.0f, 0.0f);
	target[X] = Mth::CrossProduct(target[Y], target[Z]);
	target[Y] = Mth::CrossProduct(target[Z], target[X]);
	target[W].Set();

	// setup the slerp
	Mth::SlerpInterpolator slerper;
	slerper.setMatrices(&m_orientation_matrix, &target);

	// lerp up to full strength over time
	if (m_vel[X] * m_vel[X] + m_vel[Z] * m_vel[Z] > 350.0f * 350.0f)
	{
		slerper.getMatrix(&m_orientation_matrix, Mth::LinearMap(
			0.0f,
			m_in_air_slerp_strength * m_time_step,
			Mth::Min(m_air_time_no_collision, m_in_air_slerp_time_delay),
			0.0f,
			m_in_air_slerp_time_delay
		));
	}
	else
	{
		slerper.getMatrix(&m_orientation_matrix, Mth::LinearMap(
			0.0f,
			0.3f * m_in_air_slerp_strength * m_time_step,
			Mth::Min(m_air_time_no_collision, m_in_air_slerp_time_delay),
			0.0f,
			m_in_air_slerp_time_delay
		));
	}
	m_orientation = m_orientation_matrix;
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CVehicleComponent::calculate_body_point_effective_mass ( const Mth::Vector& pos, const Mth::Vector& direction ) const
{
	// calculate the effect on momentums due to a unit impulse applied at pos towards direction
	Mth::Vector delta_mom = direction;
	Mth::Vector delta_rotmom = Mth::CrossProduct(pos, direction);
	
	// calculate the resulting change in velocities
	Mth::Vector delta_vel = m_inv_mass * delta_mom;
	Mth::Vector delta_rotvel = m_inv_moment.Rotate(delta_rotmom);
	
	// calculate the corresponding change in the body point's velocity
	Mth::Vector delta_vel_body_point = delta_vel + Mth::CrossProduct(delta_rotvel, pos);
	
	// extract the change in velocity along the direction of interest
	float delta_vel_direction = Mth::DotProduct(direction, delta_vel_body_point);
	
	// return the effective mass of the body point in the direction of interest
	Dbg_Assert(delta_vel_direction != 0.0f);
	return 1.0f / delta_vel_direction;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::consider_sleeping (   )
{
	int last_consider_sleeping_count = m_consider_sleeping_count;
	m_consider_sleeping_count = 0;
	
	if (m_controls.throttle) return;
	
	if (m_controls.reverse && m_num_wheels_in_contact != 0) return;
	
	// only sleep if we're experiencing three or more collisions; hopefully, a surface-to-surface contact
	if (m_num_wheels_in_contact + m_num_collision_points < 3) return;
	
	// only sleep if we've got four-on-the-floor or are completely flipped
	if (m_num_wheels_in_contact != 0 && m_num_wheels_in_contact != m_num_wheels) return;

	// only sleep if we're moving slow
	if (m_vel.LengthSqr() > Mth::Sqr(vVP_SLEEP_VEL)) return;
	#ifdef __USER_DAN__
	DUMPF(m_vel.Length());
	#endif

	// only sleep if we're moving slow
	if (m_rotvel.LengthSqr() > Mth::Sqr(vVP_SLEEP_ROTVEL)) return;
	#ifdef __USER_DAN__
	DUMPF(m_rotvel.Length());
	#endif
	
	// never sleep during an artificial collision period
	if (in_artificial_collision()) return;
	
	m_consider_sleeping_count = last_consider_sleeping_count + 1;
	if (m_consider_sleeping_count < 3)
	{
		return;
	}
	
	// sleep
	m_mom.Set();
	m_rotmom.Set();
	m_force.Set(0.0f, 0.0f, 0.0f);
	m_torque.Set(0.0f, 0.0f, 0.0f);
	for (int n = m_num_wheels; n--; )
	{
		mp_wheels[n].rotacc = 0.0f;
	}
	m_state = ASLEEP;
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::get_input (   )
{
	CControlPad& control_pad = mp_input_component->GetControlPad();
	
	// analog control
	// m_controls.steering = control_pad.m_scaled_leftX;
	m_controls.steering = control_pad.m_scaled_leftX * Mth::Abs(control_pad.m_scaled_leftX);
	
	// dpad control
	if (m_controls.steering == 0.0f)
	{
		if (control_pad.m_left.GetPressed() && !control_pad.m_right.GetPressed())
		{
			m_controls.steering = -1.0f;
		}
		else if (control_pad.m_right.GetPressed() && !control_pad.m_left.GetPressed())
		{
			m_controls.steering = 1.0f;
		}
	}
	
	m_controls.throttle = control_pad.m_x.GetPressed();
	m_controls.handbrake = control_pad.m_R1.GetPressed() && !m_no_handbrake;
	
	bool brake_pressed = control_pad.m_square.GetPressed() || (m_exitable && control_pad.m_triangle.GetPressed());
	bool exit_request = brake_pressed && !control_pad.m_square.GetPressed();
	
	// decide between brake and reverse
	if (brake_pressed)
	{
		if ((m_controls.reverse || (Mth::DotProduct(m_vel, m_orientation_matrix[Z]) < 300.0f && m_num_wheels_in_contact != 0))
			&& (m_state != ASLEEP || m_num_wheels_in_contact != 0))
		{
			// reverse
			m_controls.reverse = true;
			m_controls.brake = false;
		}
		else
		{
			// brake
			m_controls.brake = true;
			m_controls.reverse = false;
		}
		
		if (exit_request)
		{
			if (Mth::DotProduct(m_vel, m_orientation_matrix[Z]) < 0.0f)
			{
				// brake
				m_controls.brake = true;
				m_controls.reverse = false;
			}
			
			if (Mth::Abs(Mth::DotProduct(m_vel, m_orientation_matrix[Z])) < 30.0f)
			{
				// signal an exit
				GetObject()->BroadcastEvent(CRCD(0xcbaa3476, "ExitVehicleRequest"));
			}
		}
	}
	else
	{
		m_controls.reverse = false;
		m_controls.brake = false;
	}
	
	if (m_force_brake)
	{
		m_controls.brake = true;
		m_controls.reverse = false;
	}
	
	if (m_state == ASLEEP && (m_controls.throttle || m_controls.reverse))
	{
		m_state = AWAKE;
		m_consider_sleeping_count = 0;
	}
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::zero_input (   )
{
	m_controls.brake = false;
	m_controls.handbrake = false;
	m_controls.throttle = false;
	m_controls.reverse = false;
	m_controls.steering = 0.0f;
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::update_skeleton (   )
{
	Mth::Matrix* p_matrices = mp_skeleton_component->GetSkeleton()->GetMatrices();
	for (int i = mp_skeleton_component->GetSkeleton()->GetNumBones(); i--; )
	{
		Mth::Matrix& matrix = p_matrices[i];
		
		// setup the matrix for each bone in the skeleton
		
		if (m_draw_debug_lines == 2)
		{
			matrix.Zero();
			continue;
		}
		
		// shadow
		if (i == 0)
		{
			matrix.Zero();
		}
		
		// body
		else if (i == 1)
		{
			matrix.Zero();
			matrix[X][X] = 1.0f;
			matrix[Y][Z] = -1.0f;
			matrix[Z][Y] = 1.0f;
			matrix[W] = m_body_pos;
		}
		
		// wheel
		else
		{
			SWheel& wheel = mp_wheels[i - 2];
			
			matrix.Zero();
			matrix[X][X] = -1.0f;
			matrix[Y][Z] = 1.0f;
			matrix[Z][Y] = 1.0f;
			
			matrix.RotateZLocal(wheel.steering_angle_display);
			matrix.RotateXLocal(-wheel.orientation);
			matrix[W] = wheel.pos;
			#ifdef __NOPT_ASSERT__
			if (Script::GetInteger("use_max_y_offset"))
			{
				matrix[W][Y] += wheel.max_draw_y_offset;
			}
			else
			{
				matrix[W][Y] += Mth::ClampMax(wheel.y_offset, wheel.max_draw_y_offset);
			}
			#else
			matrix[W][Y] += Mth::ClampMax(wheel.y_offset, wheel.max_draw_y_offset);
			#endif
			matrix[W][W] = 1.0f;
		}
	}
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::draw_shadow (   )
{
	//Dbg_Message("Drawing shadow for car");

#if 0	// Experiment to draw a shadow using texture splats
	Mth::Vector start_pos = GetObject()->GetPos();
	Mth::Vector end_pos = start_pos;
	start_pos[Y] += 12.0f;
	end_pos[Y] -= 120.0f;

	float y_rot = 360.0f - m_orientation.GetVector()[Y];
	while (y_rot < 0.0f)
		y_rot += 360.0f;

	Nx::TextureSplat( start_pos, end_pos, 120.0f, 100.0f, 2.0f / (float) Config::FPS(), "blood_01", y_rot);
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::control_skater (   )
{
    // HACKY; setup the skater's position, orientation, and animation each frame
	
    mp_skater->SetPos(GetObject()->GetPos() + GetObject()->GetMatrix().Rotate(m_skater_pos));
	mp_skater->SetMatrix(GetObject()->GetMatrix());
	mp_skater_core_physics_component->ResetLerpingMatrix();
	mp_skater->SetVel(GetObject()->GetVel());
	
	if (!m_skater_visible) return;
	
	float target_time = Mth::LinearMap(
		0.0f,
		mp_skater_animation_component->AnimDuration(m_skater_anim),
		mp_skater_core_physics_component->GetFlag(FLIPPED) ? m_steering_display : -m_steering_display,
		-1.0f,
		1.0f
	);
	
	if (!GameNet::Manager::Instance()->InNetGame())
	{
		mp_skater_animation_component->PlayPrimarySequence(m_skater_anim, false, target_time, 1000.0f, Gfx::LOOPING_CYCLE, 0.0f, 0.0f);
	}
	else
	{
		// HACK to reduce number of animation events when driving cars
		static int anim_send_count = 0;
		if (++anim_send_count == 15)
		{
			anim_send_count = 0;
			mp_skater_animation_component->PlayPrimarySequence(m_skater_anim, true, target_time, 1000.0f, Gfx::LOOPING_CYCLE, 0.0f, 0.0f);
		}
		else
		{
			mp_skater_animation_component->PlayPrimarySequence(m_skater_anim, false, target_time, 1000.0f, Gfx::LOOPING_CYCLE, 0.0f, 0.0f);
		}
	}
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const int num_edges = 12;

Mth::Vector CVehicleComponent::wheel_point ( const SWheel& wheel, int i, bool side ) const
{
	Mth::Vector point(
		(side ? 1.0f : -1.0f) * 3.85f,
		wheel.radius * cosf(2.0f * 3.141592f * i / num_edges + wheel.orientation),
		wheel.radius * sinf(2.0f * 3.141592f * i / num_edges + wheel.orientation)
	);
	Mth::Matrix steering_rotation(Mth::Vector(0.0f, 1.0f, 0.0f), wheel.steering_angle);
	point = steering_rotation.Rotate(point);
	point[Y] += wheel.y_offset;
	point += wheel.pos;
	point = m_pos + m_orientation_matrix.Rotate(point);
	return point;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVehicleComponent::draw_debug_rendering (   ) const
{
	if (m_draw_debug_lines)
	{
		for (int n = m_num_wheels; n--; )
		{
			SWheel& wheel = mp_wheels[n];
			
			int32 color = 0;
			switch (wheel.state)
			{
				case SWheel::UNDER_GRIPPING:
					color = MAKE_RGB(0, 255, 0);
					break;
				case SWheel::GRIPPING:
					color = MAKE_RGB(0, 0, 0);
					break;
				case SWheel::SLIPPING:
					color = MAKE_RGB(255, 200, 0);
					break;
				case SWheel::SKIDDING:
					color = MAKE_RGB(255, 0, 0);
					break;
				case SWheel::HANDBRAKE_THROTTLE:
					color = MAKE_RGB(150, 0, 150);
					break;
				case SWheel::HANDBRAKE_LOCKED:
					color = MAKE_RGB(0, 150, 150);
					break;
				case SWheel::NO_STATE:
				case SWheel::OUT_OF_CONTACT:
				default:
					color = MAKE_RGB(255, 255, 255);
					break;
			}
			
			for (int i = num_edges; i--; )
			{
				Mth::Vector start = wheel_point(wheel, i, true);
				Mth::Vector end = wheel_point(wheel, i + 1, true);
				Gfx::AddDebugLine(start, end, color, color, 1);
	
				start = wheel_point(wheel, i, false);
				end = wheel_point(wheel, i + 1, false);
				Gfx::AddDebugLine(start, end, color, color, 1);
				
				start = wheel_point(wheel, i, true);
				end = wheel_point(wheel, i, false);
				Gfx::AddDebugLine(start, end, color, color, 1);
				
				#if 1
				// draw axis of rotation lines
				Mth::Matrix steering_rotation(Mth::Vector(0.0f, 1.0f, 0.0f), wheel.steering_angle);
				start.Set(-240.0f, 0.0f, 0.0f);
				start = steering_rotation.Rotate(start);
				start += wheel.pos;
				start[Y] += wheel.y_offset;
				start = m_pos + m_orientation_matrix.Rotate(start);
				end.Set(240.0f, 0.0f, 0.0f);
				end = steering_rotation.Rotate(end);
				end += wheel.pos;
				end[Y] += wheel.y_offset;
				end = m_pos + m_orientation_matrix.Rotate(end);
				Gfx::AddDebugLine(start, end, MAKE_RGB(0, 0, 0), MAKE_RGB(0, 0, 0), 1);
				#endif
			}
		} // END loop over wheels
	}
	
	#if 1
	if (m_draw_debug_lines)
	{
		Gfx::AddDebugStar(m_pos + m_suspension_center_of_mass_world, 12.0f, MAKE_RGB(100, 0, 100), 1);
		Gfx::AddDebugStar(m_pos, 12.0f, MAKE_RGB(0, 100, 100), 1);
	}
	#endif
}

}
