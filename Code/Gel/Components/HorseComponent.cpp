//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       HorseComponent.cpp
//* OWNER:          Dan Nelson
//* CREATION DATE:  1/31/3
//****************************************************************************

#include <core/defines.h>
#include <core/math.h>
#include <core/math/slerp.h>
										
#include <gel/components/horsecomponent.h>
#include <gel/components/floatinglabelcomponent.h>
#include <gel/components/inputcomponent.h>
#include <gel/components/camerautil.h>
#include <gel/components/horsecameracomponent.h>
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
#include <gfx/camera.h>
#include <gfx/nxviewman.h>

#include <sk/engine/feeler.h>
#include <sk/modules/viewer/viewer.h>
#include <sk/scripting/cfuncs.h>
#include <sk/scripting/nodearray.h>

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
	
CHorseComponent::SCollisionPoint CHorseComponent::sp_collision_points[4 * (Nx::MAX_NUM_2D_COLLISIONS_REPORTED + 1)];
								   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CHorseComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CHorseComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CHorseComponent::CHorseComponent() : CBaseComponent()
{
	SetType( CRC_HORSE );
	
	mp_input_component				= NULL;
	mp_animation_component			= NULL;
	mp_movable_contact_component	= NULL;

	mp_collision_cache = Nx::CCollCacheManager::sCreateCollCache();
	
	m_draw_debug_lines = 0;

	mp_rider						= NULL;

	m_vel.Set( 0.0f, 0.0f, 0.0f, 0.0f );
	m_upward.Set( 0.0f, 1.0f, 0.0f, 0.0f );
	m_horizontal_vel.Set( 0.0f, 0.0f, 0.0f, 0.0f );
	m_control_direction					= m_horizontal_vel;
	m_primary_air_direction				= m_horizontal_vel;
	m_uncontrollable_air_horizontal_vel = m_horizontal_vel;

	m_orientation_matrix.Ident();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CHorseComponent::~CHorseComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::InitFromStructure( Script::CStruct* pParams )
{
	m_script_drag_factor	= 1.0f;

	// extract constant parameters from struct; parameters not included in the script are left unchanged
	
	// toggles through the debug line drawing states
	if (pParams->ContainsFlag(CRCD(0x935ab858, "debug")))
	{
		m_draw_debug_lines = ++m_draw_debug_lines % 3;
	}
	else if (pParams->ContainsFlag(CRCD(0x751da48b, "no_debug")))
	{
		m_draw_debug_lines = 0;
	}

	m_pos = GetObject()->GetPos();

	if( pParams->ContainsComponentNamed(CRCD(0x7f261953, "pos" )))
	{
		pParams->GetVector(CRCD(0x7f261953, "pos"), &m_pos);
		GetObject()->SetPos(m_pos);
	}

	// Grab a pointer to the skeleton.
	mp_skeleton_component = static_cast< CSkeletonComponent* >(GetObject()->GetComponent(CRC_SKELETON));
	Dbg_MsgAssert(mp_skeleton_component, ("HorseComponent has no peer skeleton component."));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure( pParams );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::Finalize( void )
{
	mp_model_component				= GetModelComponentFromObject( GetObject());
	mp_input_component				= GetInputComponentFromObject( GetObject());
	mp_animation_component			= GetAnimationComponentFromObject( GetObject());
	mp_movable_contact_component	= GetMovableContactComponentFromObject( GetObject());

	Dbg_Assert( mp_model_component );
	Dbg_Assert( mp_input_component );
	Dbg_Assert( mp_animation_component );
	Dbg_Assert( mp_movable_contact_component );

	// Enable blending for the Horse by default.
	mp_animation_component->EnableBlending( true );

	m_display_slerp_matrix = GetObject()->GetMatrix();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::Update( void )
{
	// zero the frame event
	m_last_frame_event = m_frame_event;
	m_frame_event = 0;
	
	// get input
    get_controller_input();
	
	// extract initial state for this frame from the object
	m_frame_start_pos = m_pos = GetObject()->GetPos();
	
	m_horizontal_vel = GetObject()->GetVel();
	m_horizontal_vel[Y] = 0.0f;
	m_vertical_vel = GetObject()->GetVel()[Y];
	
	// note that m_facing and m_upward will often not be orthogonal, but will always span a plan
	
	// generally straight up, but now after a transition from skating
	m_upward = GetObject()->GetMatrix()[Y];
	
	m_facing = GetObject()->GetMatrix()[Z];
	m_facing[Y] = 0.0f;
	float length = m_facing.Length();
	if (length < 0.001f)
	{
		// upward facing orientation matrix
		m_facing = -GetObject()->GetMatrix()[Y];
		m_facing[Y] = 0.0f;
		m_facing.Normalize();
		
		// since m_upward is now in the same plan as m_facing, push m_upward up a touch
		m_upward[Y] += 0.01f;
		m_upward.Normalize();
	}
	else
	{
		m_facing /= length;
	}
	
	// Set the frame length.
	m_frame_length = Tmr::FrameLength();
	
	// go to our true Y position
	m_curb_float_height_adjusted = false;
	m_pos[Y] -= m_curb_float_height;
	
	// switch logic based on walking state
	switch (m_state)
	{
		case WALKING_GROUND:
			go_on_ground_state();
			break;

		case WALKING_AIR:
			go_in_air_state();
			break;
																	  
		case WALKING_HOP:
//			go_hop_state();
			break;
																	  
		case WALKING_HANG:
//			go_hang_state();
			break;
            
		case WALKING_LADDER:
//			go_ladder_state();
            break;
			
		case WALKING_ANIMWAIT:
//			go_anim_wait_state();
			break;
	}
	
	// the there's no curb to adjust due to, lerp down to zero
	if (!m_curb_float_height_adjusted)
	{
		m_curb_float_height = Mth::Lerp(m_curb_float_height, 0.0f, s_get_param(CRCD(0x9b3388fa, "curb_float_lerp_down_rate")) * m_frame_length);
	}
	
	// adjust back to our curb float Y position
	m_pos[Y] += m_curb_float_height;
	
	// scripts may have restarted us / switched us to skating
//	if (should_bail_from_frame()) return;
	
	// Keep the object from falling through holes in the geometry.
	if( m_state == WALKING_GROUND || m_state == WALKING_AIR )
	{
		uber_frig();
	}
	
	// rotate to upright
	lerp_upright();
	
	// setup the object based on this frame's walking
	copy_state_into_object();
	
	// Position the rider correctly, if one exists.
	position_rider();

	Dbg_Assert(m_frame_event);
	GetObject()->SelfEvent(m_frame_event);
	
	// set the animation speeds
/*	switch (m_anim_scale_speed)
	{
		case RUNNING:
			if (m_anim_standard_speed > 0.0f)
			{
				mp_animation_component->SetAnimSpeed(m_anim_effective_speed / m_anim_standard_speed, false, false);
			}
			break;
			
		case HANGMOVE:
			mp_animation_component->SetAnimSpeed(m_anim_effective_speed / s_get_param(CRCD(0xd77ee881, "hang_move_speed")), false, false);
			break;
					
		case LADDERMOVE:
			mp_animation_component->SetAnimSpeed(m_anim_effective_speed / s_get_param(CRCD(0xab2db54, "ladder_move_speed")), false, false);
			break;
	
		default:
			break;
	}
*/
	
	// camera controls
	// NOTE: script parameters
/*
	switch (m_frame_event)
	{
		case CRCC(0xf41aba21, "Hop"):
			mp_camera_component->SetOverrides(m_facing, 0.05f);
			break;
		
		case CRCC(0x2d9815c3, "HangMoveLeft"):
		{
			Mth::Vector facing = m_facing;
			facing.RotateY(-0.95f);
			mp_camera_component->SetOverrides(facing, 0.05f);
			break;
		}
			
		case CRCC(0x279b1f0b, "HangMoveRight"):
		{
			Mth::Vector facing = m_facing;
			facing.RotateY(0.95f);
			mp_camera_component->SetOverrides(facing, 0.05f);
			break;
		}
					
		case CRCC(0x4194ecca, "Hang"):
			mp_camera_component->SetOverrides(m_facing, 0.05f);
			break;
		
		case CRCC(0xc84243da, "Ladder"):
		case CRCC(0xaf5abc82, "LadderMoveUp"):
		case CRCC(0xfec9dded, "LadderMoveDown"):
			mp_camera_component->SetOverrides(m_facing, 0.05f);
			break;
					
		case CRCC(0x4fe6069c, "AnimWait"):
			if (m_anim_wait_camera_mode == AWC_CURRENT)
			{
				mp_camera_component->SetOverrides(m_facing, 0.05f);
			}
			else
			{
				mp_camera_component->SetOverrides(m_drift_goal_facing, 0.05f);
			}
			break;
	
		default:
			mp_camera_component->UnsetOverrides();
			break;
	}
*/
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CBaseComponent::EMemberFunctionResult CHorseComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		// @script | Walk_Ground |
		case CRCC( 0x893213e5, "Walk_Ground" ):
		{
			return m_state == WALKING_GROUND ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			break;
		}
	
		case CRCC( 0x88bd4924, "Horse_GetSpeedScale" ):
		{
			uint32 checksum;
			if( m_anim_effective_speed < s_get_param( CRCD(0xf3649996, "max_slow_walk_speed")))
			{
				checksum = CRCD(0x20c5a299,"HORSE_WALK");
			}
			else if( m_anim_effective_speed < s_get_param( CRCD(0x6a5805d8, "max_fast_walk_speed")))
			{
				checksum = CRCD(0x8a354e68,"HORSE_TROT");
			}
			else if( m_anim_effective_speed < s_get_param( CRCD(0x1c94cc9c, "max_slow_run_speed")))
			{
				checksum = CRCD(0x1bee30eb,"HORSE_CANTER");
			}
			else
			{
				checksum = CRCD(0x2ca2c118,"HORSE_GALLOP");
			}
			pScript->GetParams()->AddChecksum( CRCD( 0x92c388f, "SpeedScale" ), checksum );
			break;
		}

		// @script | Horse_Jump |
		case CRCC( 0xae7deda, "Horse_Jump" ):
		{
			// Jump strength scales with the length the jump button has been held.
			jump( Mth::Lerp( s_get_param( CRCD( 0x246d0bf3, "min_jump_factor" )), 1.0f,	Mth::ClampMax( mp_input_component->GetControlPad().m_x.GetPressedTime() / s_get_param( CRCD( 0x12333ebd, "hold_time_for_max_jump" )), 1.0f )));
			break;
		}

		// @script | Vehicle_Kick | kicks the vehicle with a force and torque
		case CRCC(0x93b713a6, "Vehicle_Kick"):
		{
			break;
		}

		// @script : Vehicle_Reset | reset any parameters of the vehicle
		case CRCC(0xcdcdc05e, "Vehicle_Reset"):
			RefreshFromStructure(pParams);
			Finalize();
			break;
			
		// @script : Vehicle_MoveToRestart | teleport the vehicle to the restart node
		case CRCC(0x4b0b27dd, "Vehicle_MoveToRestart"):
		{
			uint32 node_name_checksum;
			pParams->GetChecksum(NO_NAME, &node_name_checksum, Script::ASSERT);
			MoveToNode(SkateScript::GetNode(SkateScript::FindNamedNode(node_name_checksum, Script::ASSERT)));
			break;
		}
			
		// @script : Vehicle_PlaceBeforeCamera | moves the object before the active camera
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
				GetObject()->SetMatrix( m_orientation_matrix );
			}
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

void CHorseComponent::GetDebugInfo ( Script::CStruct *p_info )
{
	Dbg_MsgAssert(p_info, ("NULL p_info sent to CHorseComponent::GetDebugInfo"));

	// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::MoveToNode( Script::CStruct* p_node )
{
	// move to the position relative to the given node that a ped car would be at
	Mth::Vector restart_pos;
	SkateScript::GetPosition(p_node, &restart_pos);
	
	Mth::Vector restart_angles;
	SkateScript::GetAngles( p_node, &restart_angles );
	Mth::Matrix restart_matrix;
	restart_matrix.SetFromAngles( restart_angles );
	
	// find ground hieght
	CFeeler feeler( restart_pos + Mth::Vector( 0.0f, 24.0f, 0.0f, 0.0f ), restart_pos + Mth::Vector( 0.0f, -800.0f, 0.0f, 0.0f ));
	if (feeler.GetCollision())
	{
		restart_pos[Y] = feeler.GetPoint()[Y];
	}
	
	// move the car to the restart position and allow it to settle on its suspension
	m_pos = restart_pos;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint32 CHorseComponent::GetAnimation( void )
{
	return mp_animation_component->GetCurrentSequence();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CHorseComponent::AcceptRiderMount( CCompositeObject* p_rider )
{
	// This is where we want to enable the associated camera.
	CHorseCameraComponent*	p_cam_component = GetHorseCameraComponentFromObject( GetObject());
	if( p_cam_component )
	{
		p_cam_component->Suspend( false );
	}

	// Also want to set the horse camera as active.
	Script::RunScript( CRCD( 0xb08469a2, "MountHorse" ));

	mp_rider = p_rider;

	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CHorseComponent::ShouldUpdateCamera( void )
{
	if( mp_rider )
		return true;
	else
		return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CHorseComponent::AcceptRiderDismount( CCompositeObject* p_rider )
{
	// This is where we want to disable the associated camera.
	CHorseCameraComponent*	p_cam_component = GetHorseCameraComponentFromObject( GetObject());
	if( p_cam_component )
	{
		p_cam_component->Suspend( true );
	}

	// Also want to set the horse camera as inactive.
	Script::RunScript( CRCD( 0x400b95f5, "DismountHorse" ));

	mp_rider = NULL;
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::draw_debug_rendering (   ) const
{
	if (m_draw_debug_lines)
	{
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::account_for_movable_contact( void )
{
/*
	if (!mp_movable_contact_component->UpdateContact(m_pos))
		return;
	
	m_pos += mp_movable_contact_component->GetContact()->GetMovement();
	
	if (mp_movable_contact_component->GetContact()->IsRotated())
	{
		m_facing = mp_movable_contact_component->GetContact()->GetRotation().Rotate(m_facing);
		if (m_facing[Y] != 0.0f)
		{
			DUMPF(m_facing[Y]);
			m_facing[Y] = 0.0f;
			m_facing.Normalize();
		}
	}
*/
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::jump( float strength )
{
	// switch to air state and give the object an upwards velocity
	
	// if we're jumping from the ground, trip the ground's triggers
	if (m_state == WALKING_GROUND && m_last_ground_feeler_valid)
	{
//		mp_trigger_component->CheckFeelerForTrigger(TRIGGER_JUMP_OFF, m_last_ground_feeler);
//		if (should_bail_from_frame()) return;
	}
	
	m_primary_air_direction = m_facing;
	
	// Called by script from outside of the component update, so m_vertical_vel is not used.
	GetObject()->GetVel()[Y] = strength * s_get_param( CRCD( 0x63d62a21, "jump_velocity" ));
	
	leave_movable_contact_for_air( GetObject()->GetVel(), GetObject()->GetVel()[Y] );
	
	set_state( WALKING_AIR );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::setup_collision_cache( void )
{
	float horizontal_reach = 1.0f + s_get_param(CRCD(0x99978d2b, "feeler_length"));
	float vertical_height = 1.0f + s_get_param(CRCD(0x9ea1974a, "walker_height"));;
	float vertical_depth = 1.0f + s_get_param(CRCD(0xaf3e4251, "snap_down_height"));
	
	Mth::CBBox bbox(
		GetObject()->GetPos() - Mth::Vector(horizontal_reach, vertical_depth, horizontal_reach, 0.0f),
		GetObject()->GetPos() + Mth::Vector(horizontal_reach, vertical_height, horizontal_reach, 0.0f)
	);
	
	mp_collision_cache->Update(bbox);
	CFeeler::sSetDefaultCache(mp_collision_cache);
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
float CHorseComponent::calculate_desired_speed( void )
{
	// forced run
//	adjust_control_for_forced_run();
	
//	float walk_point = s_get_param(CRCD(0xc1528f7f, "walk_point"));

	if( m_control_magnitude > 0.0f )
	{
		// Stick is in non-zero position, so adjust target speed.
		// We may want two sets of these, one for speeding up, one for slowing down.
		float speed_ka = 1.0f; /* Script::GetFloat( CRCD( 0xcac0c1d4, "GunslingerLookaroundTiltKa" ), Script::ASSERT ); */
		float speed_ea = 2.0f; /* Script::GetFloat( CRCD( 0x5443ec5a, "GunslingerLookaroundTiltEa" ), Script::ASSERT ); */

		float speed_ks = 1.0f; /* Script::GetFloat( CRCD( 0x3979b09c, "GunslingerLookaroundTiltKs" ), Script::ASSERT ); */
		float speed_es = 1.0f; /* Script::GetFloat( CRCD( 0xa7fa9d12, "GunslingerLookaroundTiltEs" ), Script::ASSERT ); */

		float target = m_control_direction[X] * m_control_magnitude;

		// Calculate acceleration.
		float a = speed_ka * powf( Mth::Abs( target ), speed_ea ) * (( target > 0.0f ) ? 1.0f : ( target < 0.0f ) ? -1.0f : 0.0f );

		// Calculate max speed.
		float s = speed_ks * powf( Mth::Abs( target ), speed_es ) * (( target > 0.0f ) ? 1.0f : ( target < 0.0f ) ? -1.0f : 0.0f );

		m_target_speed_adjustment += a;

		if( s == 0.0f )
		{
			m_target_speed_adjustment = 0.0f;
		}
		else if((( s > 0.0f ) && ( m_target_speed_adjustment > s )) || (( s < 0.0f ) && ( m_target_speed_adjustment < s )))
		{
			m_target_speed_adjustment = s;
		}

		m_target_speed += m_target_speed_adjustment * m_frame_length;

		// Constrain to [0.0, 1.0] limits.
		m_target_speed = ( m_target_speed < 0.0f ) ? 0.0f : (( m_target_speed > 1.0f ) ? 1.0f : m_target_speed );
	}

	return s_get_param( CRCD( 0xcc461b87, "run_speed" )) * m_target_speed;

/*
	{
		float walk_point			= s_get_param(CRCD(0xc1528f7f, "walk_point"));
		float velocity_magnitude	= m_control_direction[X] * m_control_magnitude;

		if( velocity_magnitude <= walk_point )
		{
			m_run_toggle = false;
			return Mth::LinearMap( 0.0f, s_get_param( CRCD( 0x79d182ad, "walk_speed" )), velocity_magnitude, 0.3f, walk_point );
		}
		else
		{
			m_run_toggle = true;
			return Mth::LinearMap( s_get_param( CRCD( 0x79d182ad, "walk_speed" )), s_get_param( CRCD( 0xcc461b87, "run_speed" )), velocity_magnitude, walk_point, 1.0f );
		}
	}
*/
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
float CHorseComponent::adjust_desired_speed_for_slope( float desired_speed )
{
	// Slow velocity up and down slopes.
	
	// skip if there is no appreciable slope
	if (m_ground_normal[Y] > 0.95f) return desired_speed;
	
	// skip if not running
	if (desired_speed <= s_get_param(CRCD(0x79d182ad, "walk_speed"))) return desired_speed;
	
	// calculate a horizontal vector up the slope
	Mth::Vector up_slope = m_ground_normal;
	up_slope[Y] = 0.0f;
	up_slope.Normalize();
	
	// horizontal factor of velocity if the velocity were pointing along the slope (instead of along the horizontal)
	float movement_factor = m_ground_normal[Y];
	
	// factor of velocity pointing up the slope
	float dot = Mth::Abs(Mth::DotProduct(m_facing, up_slope));
	
	// scale the up-the-slope element of velocity based on the slope strength
	return (1.0f - dot) * desired_speed + dot * movement_factor * desired_speed;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::calculate_horizontal_speed_and_facing( float &horizontal_speed )
{
	// calculate user's desired speed
	float desired_speed = calculate_desired_speed();
	
	// adjust speed by the script set drag factor
	desired_speed *= m_script_drag_factor;
			   	
	// setup frame's event
	if (desired_speed <= s_get_param(CRCD(0x74e8227d, "max_stand_speed")))
	{
		m_frame_event = CRCD(0x9b46e749, "Stand");
	}
	else
	{
		m_frame_event = CRCD(0xaf895b3f, "Run");
	}

	bool special_acceleration = false;
	
	// HorseComponent version.
	// The required rate of turn depends on the x axis component of the control direction.
//	if( horizontal_speed < s_get_param( CRCD( 0x52582d5b, "max_rotate_in_place_speed" )))
	if( 1 )
	{
		// Low speed rotate to desired orientation with no speed change.
		m_delta_angle = Mth::DegToRad( s_get_param( CRCD( 0xb557804b, "rotate_in_place_rate" ))) * -m_control_direction[Z] * m_control_magnitude * m_frame_length;
		if( !m_run_toggle )
		{
			m_delta_angle *= s_get_param( CRCD( 0x7b446c98, "walk_rotate_factor" ));
		}
			
		if( m_delta_angle != 0.0f )
		{
			float cos_delta_angle	= cosf( m_delta_angle );
			float sin_delta_angle	= sinf( m_delta_angle );
			float adjusted_vel		= cos_delta_angle * m_facing[X] + sin_delta_angle * m_facing[Z];

			m_facing[Z]				= -sin_delta_angle * m_facing[X] + cos_delta_angle * m_facing[Z];
			m_facing[X]				= adjusted_vel;
			
			// check for overturn
//			if (left_turn != (-m_facing[Z] * m_control_direction[X] + m_facing[X] * m_control_direction[Z] < 0.0f))
//			{
//				m_facing = m_control_direction;
//			}
			
			// no acceleration until we reach the desired orientation
//			special_acceleration = true;
			
			// setup the event
			m_frame_event = ( m_delta_angle < 0.0f ) ? CRCD( 0xf28adbfc, "RotateLeft" ) : CRCD( 0x912220f8, "RotateRight" );
		}
	}
	
	if (special_acceleration) return;
	
	// Store desired speed for animation speed scaling.
	m_anim_effective_speed = desired_speed;
	
	// adjust desired speed for slope
	desired_speed = adjust_desired_speed_for_slope( desired_speed );
	
	// linear acceleration; exponential deceleration
	if (horizontal_speed > desired_speed)
	{
		horizontal_speed = Mth::Lerp(horizontal_speed, desired_speed, s_get_param(CRCD(0xacfa4e0c, "decel_factor")) * m_frame_length);
		
		if (desired_speed == 0.0f)
		{
			if (horizontal_speed > s_get_param(CRCD(0x79d182ad, "walk_speed")))
			{
				m_frame_event = CRCD(0x1d537eff, "Skid");
			}
			else if (m_last_frame_event == CRCD(0x1d537eff, "Skid") && horizontal_speed > s_get_param(CRCD(0x311d02b2, "stop_skidding_speed")))
			{
				m_frame_event = CRCD(0x1d537eff, "Skid");
			}
		}
	}
	else
	{
		if (m_run_toggle)
		{
			horizontal_speed += s_get_param(CRCD(0x4f47c998, "run_accel_rate")) * m_frame_length;
		}
		else
		{
			horizontal_speed += s_get_param(CRCD(0x6590a49b, "walk_accel_rate")) * m_frame_length;
		}
		if (horizontal_speed > desired_speed)
		{
			horizontal_speed = desired_speed;
		}
	}
}




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CHorseComponent::adjust_horizontal_vel_for_environment( bool wall_push_active )
{
	// We send out feeler rays to find nearby walls.  We limit velocity to be flush with the first wall found.  If two or more non-parallel walls
	// are found, velocity is zeroed.
	
	float feeler_length = s_get_param(CRCD(0x99978d2b, "feeler_length"));
	float feeler_height = s_get_param(CRCD(0x6da7f696, "feeler_height"));
	
	CFeeler feeler;
	
	bool contact = false;
	for (int n = 0; n < vNUM_FEELERS + 1; n++)
	{
		// setup the the feeler
		
		if (n == vNUM_FEELERS)
		{
			// final feeler is for air state only and is at the feet; solves situations in which the feet impact with vertical surfaces which the
			// wall feelers are too high to touch
			if (m_state != WALKING_AIR)
			{
				mp_contacts[vNUM_FEELERS].in_collision = false;
				continue;
			}
			
			feeler.m_start = m_pos;
			feeler.m_end = m_pos + m_horizontal_vel * m_frame_length;
			feeler.m_end[Y] += m_vertical_vel * m_frame_length + 0.5f * -s_get_param(CRCD(0xa5e2da58, "gravity")) * Mth::Sqr(m_frame_length);
		}
		else
		{
			feeler.m_start = m_pos;
			feeler.m_start[Y] += feeler_height;
			feeler.m_end = m_pos + feeler_length * calculate_feeler_offset_direction(n);
			feeler.m_end[Y] += feeler_height;
		}
		
		mp_contacts[n].in_collision = feeler.GetCollision();
		
		if (!mp_contacts[n].in_collision)
		{
			#ifdef __USER_DAN__
			if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
			{
				feeler.DebugLine(0, 0, 255, 1);
			}
			#endif
			continue;
		}
		
		contact = true;
		
		#ifdef __USER_DAN__
		if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
		{
			feeler.DebugLine(255, 0, 0, 1);
		}
		#endif

		// grab the horizontal normal of the contacted wall
		mp_contacts[n].normal = feeler.GetNormal();
		mp_contacts[n].normal[Y] = 0.0f;
		mp_contacts[n].normal.Normalize();
		
		// grab the distance of the contact from the object
		mp_contacts[n].distance = feeler.GetDist();
		
		// if we're on the moving object, don't count its movement when doing collision detection, as the walker's velocity is already measured
		// relative to its movable contact's
		if (feeler.IsMovableCollision()
			&& (!mp_movable_contact_component->HaveContact() || mp_movable_contact_component->GetContact()->GetObject() != feeler.GetMovingObject()))
		{
			mp_contacts[n].movement = Mth::DotProduct(feeler.GetMovingObject()->GetVel(), mp_contacts[n].normal);
		}
		else
		{
			mp_contacts[n].movement = 0.0f;
		}
	}
	
	// check for wall push
	if (m_state == WALKING_GROUND)
	{
//		if (wall_push_active && check_for_wall_push())
		if (false)
		{
			// if we're wall pushing, we may decide to switch states based on our environment
			
			if (Tmr::ElapsedTime(m_wall_push_test.test_start_time) > s_get_param(CRCD(0x928e6775, "hop_delay")))
			{
//				if (maybe_climb_up_ladder() || maybe_hop_to_hang() || maybe_jump_low_barrier()) return false;
			}
			else if (Tmr::ElapsedTime(m_wall_push_test.test_start_time) > s_get_param(CRCD(0x38d36700, "barrier_jump_delay")))
			{
//				if (maybe_climb_up_ladder() || maybe_jump_low_barrier()) return false;
			}
		}
//		else if (mp_input_component->GetControlPad().m_R1.GetPressed() && !m_ignore_grab_button)
//		{
//			if (maybe_climb_up_ladder(true)) return false;
//		}
	}
	
	if (!contact) return false;
	
	// push away from walls
	for (int n = 0; n < vNUM_FEELERS + 1; n++)
	{
		if (!mp_contacts[n].in_collision) continue;
		                
		if (mp_contacts[n].distance < s_get_param(CRCD(0xa20c43b7, "push_feeler_length")) / feeler_length)
		{
			m_pos += s_get_param(CRCD(0x4d16f37d, "push_strength")) * m_frame_length * mp_contacts[n].normal;
		}
	}
	
	// from here on we ignore collisions we're moving out of
	contact = false;
	for (int n = 0; n < vNUM_FEELERS + 1; n++)
	{
		if (!mp_contacts[n].in_collision) continue;
		
		// don't count collisions we're moving out of
		if (Mth::DotProduct(mp_contacts[n].normal, m_horizontal_vel) >= mp_contacts[n].movement)
		{
			mp_contacts[n].in_collision = false;
		}
		else
		{
			contact = true;
		}
	}
	if (!contact) return false;
	
	// Now we calculate how our movement is effected by our collisions.  The movement must have a non-negative dot product with all collision normals.
	// The algorithm used should be valid for all convex environments.
	
	// if any of the colllision normals are more than right angles to one another, no movement is possible
	// NOTE: not valid with movable contacts; could cause jerky movement in corners where walls are movable
	for (int n = 0; n < vNUM_FEELERS + 1; n++)
	{
		if (!mp_contacts[n].in_collision) continue;
		for (int m = n + 1; m < vNUM_FEELERS + 1; m++)
		{
			if (!mp_contacts[m].in_collision) continue;
			if (Mth::DotProduct(mp_contacts[n].normal, mp_contacts[m].normal) <= 0.0f)
			{
				m_horizontal_vel.Set();
				m_anim_effective_speed = Mth::Min(s_get_param(CRCD(0xbd6a05d, "min_anim_run_speed")), m_anim_effective_speed);
				return true;
			}
		}
	}
	
	// direction of proposed movement
	Mth::Vector movement_direction = m_horizontal_vel;
	movement_direction.Normalize();
	
	Mth::Vector adjusted_vel = m_horizontal_vel;
	
	// loop over the contacts (from backward to forward)
	const int contact_idxs[] = { 7, 4, 3, 5, 2, 6, 1, 0 };
	for (int i = 0; i < vNUM_FEELERS + 1; i++)
	{
		int n = contact_idxs[i];
		
		if (!mp_contacts[n].in_collision) continue;
		
		// check to see if the movement still violates this constraint
		float normal_vel = Mth::DotProduct(adjusted_vel, mp_contacts[n].normal);
		if (normal_vel >= mp_contacts[n].movement) continue;
		
		// adjust the movement to the closest direction allowed by this contraint
		adjusted_vel -= (normal_vel - mp_contacts[n].movement) * mp_contacts[n].normal;
		
		// if the mvoement direction no longer points in the direction of the proposed movement, no movement occurs
		if (Mth::DotProduct(adjusted_vel, m_horizontal_vel) <= 0.0f)
		{
			m_horizontal_vel.Set();
			m_anim_effective_speed = Mth::Min(s_get_param(CRCD(0xbd6a05d, "min_anim_run_speed")), m_anim_effective_speed);
			return true;
		}
	}
	
	// insure that the adjusted velocity in the final direction is not larger than the projection of the initial velocity into that direction
	float adjusted_speed = adjusted_vel.Length();
	Mth::Vector adjusted_vel_direction = adjusted_vel;
	adjusted_vel_direction *= 1.0f / adjusted_speed;
	float projected_vel = Mth::DotProduct(m_horizontal_vel, adjusted_vel_direction);
	
	if (adjusted_speed > projected_vel)
	{
		adjusted_vel = adjusted_vel_direction * projected_vel;
	}
	
	// only the velocity along the movement direction is retained
	m_horizontal_vel = adjusted_vel;
	
	float final_horiz_vel = m_horizontal_vel.Length();
	if (m_anim_effective_speed > s_get_param(CRCD(0xbd6a05d, "min_anim_run_speed")))
	{
		m_anim_effective_speed = final_horiz_vel;
		m_anim_effective_speed = Mth::Max(s_get_param(CRCD(0xbd6a05d, "min_anim_run_speed")), m_anim_effective_speed);
	}
	
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::adjust_facing_for_adjusted_horizontal_vel( void  )
{
	// We adjust facing due to adjustment in horizontal velocity due to environment.
	// Basically, we want to object to turn to face the velocity that the environment has forced upon it.
	
	// IDEA: shift to basing turn amount on angle difference and not speed
	
	float horizontal_speed = m_horizontal_vel.Length();
	
	// the new facing is in the direction of our adjusted velocity
	Mth::Vector new_facing = m_horizontal_vel;
	new_facing.Normalize();
	
	// Smoothly transition between no wall turning to full wall turning.
	float turn_ratio;
	if( horizontal_speed > s_get_param( CRCD( 0xe6c1cd0d, "max_wall_turn_speed_threshold" )))
	{
		turn_ratio = s_get_param( CRCD( 0x7a583b9b, "wall_turn_factor" )) * m_frame_length;
	}
	else
	{
		turn_ratio = Mth::LinearMap(	0.0f,
										s_get_param( CRCD( 0x7a583b9b, "wall_turn_factor")) * m_frame_length,
										horizontal_speed,
//										s_get_param( CRCD( 0x0515a933, "wall_turn_speed_threshold")),
										0.0f,
										s_get_param( CRCD( 0xe6c1cd0d, "max_wall_turn_speed_threshold" )));
	}
	
	// Exponentially approach new facing.
	if( turn_ratio >= 1.0f )
	{
		m_facing = new_facing;
	}
	else
	{
		m_facing = Mth::Lerp( m_facing, new_facing, turn_ratio );
		m_facing.Normalize();
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::go_on_ground_state( void )
{
	account_for_movable_contact();
	
	setup_collision_cache();
	
	// Calculate initial horizontal speed.
	float horizontal_speed = m_horizontal_vel.Length();
	
	calculate_horizontal_speed_and_facing( horizontal_speed );
	
	// Calculate this frame's movement.
	m_horizontal_vel = horizontal_speed * m_facing;
	
	// Prevent movement into walls.
	if( adjust_horizontal_vel_for_environment( true ))
	{
		// Turn to face newly adjusted velocity.
		adjust_facing_for_adjusted_horizontal_vel();
	}
	
	// If we are wall pushing, we may have decided to switch states during adjust_horizontal_vel_for_environment based on our environment.
//	if (m_state != WALKING_GROUND || should_bail_from_frame())
	if( m_state != WALKING_GROUND )
	{
		CFeeler::sClearDefaultCache();
		return;
	}
	
	// Apply movement for this frame.
	m_pos += m_horizontal_vel * m_frame_length;
	
	// Snap up and down curbs and perhaps switch to air.
	respond_to_ground();

//	if (m_state != WALKING_GROUND || should_bail_from_frame())
	if( m_state != WALKING_GROUND )
	{
		CFeeler::sClearDefaultCache();
		return;
	}
	
	adjust_curb_float_height();
	
	// Deal with intelligent path following.
	do_path_following();

	// Ensure that we do not slip through the cracks in the collision geometry which are a side-effect of moving collidable objects.
	if (CCompositeObject* p_inside_object = mp_movable_contact_component->CheckInsideObjects(m_pos, m_frame_start_pos))
	{
		MESSAGE("WALKING_GROUND, within moving object");
		
		// allow it to push us forward, causing a bit of a stumble
		m_horizontal_vel = p_inside_object->GetVel();
		m_horizontal_vel[Y] = 0.0f;
		m_vertical_vel = 0.0f;
		
		float speed_sqr = m_horizontal_vel.LengthSqr();
		if (speed_sqr > (10.0f * 10.0f))
		{
			m_facing = m_horizontal_vel * (1.0f / sqrtf(speed_sqr));
		}
	}
	
	CFeeler::sClearDefaultCache();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::go_in_air_state( void )
{
	setup_collision_cache();
	
	// default air event
	m_frame_event = CRCD(0x439f4704, "Air");
	
	// user control of horizontal velocity
	control_horizontal_vel();
	
	// prevent movement into walls
	adjust_horizontal_vel_for_environment(false);
//	if (should_bail_from_frame()) return;
	
	// check for head bonking
	adjust_vertical_vel_for_ceiling();
	
	// apply movement and acceleration for this frame
	m_pos += m_horizontal_vel * m_frame_length;
	m_pos[Y] += m_vertical_vel * m_frame_length + 0.5f * -s_get_param(CRCD(0xa5e2da58, "gravity")) * Mth::Sqr(m_frame_length);
	
	m_vertical_vel += -s_get_param(CRCD(0xa5e2da58, "gravity")) * m_frame_length;
	
	// see if we've landed yet
	check_for_landing(m_frame_start_pos, m_pos);
//	if (m_state != WALKING_AIR || should_bail_from_frame()) return;
	if (m_state != WALKING_AIR ) return;
	
	// maybe grab a rail; delay regrabbing of hang rails
//	if (mp_input_component->GetControlPad().m_R1.GetPressed() && !m_ignore_grab_button
//		&& ((m_previous_state != WALKING_HANG && m_previous_state != WALKING_LADDER) || Tmr::ElapsedTime(m_state_timestamp) > s_get_param(CRCD(0xe6e0c0a4, "rehang_delay"))))
//	{
//		if (m_previous_state == WALKING_LADDER)
//		{
//			// can't regrab ladders
//			if (maybe_grab_to_hang(m_frame_start_pos, m_pos))
//			{
//				CFeeler::sClearDefaultCache();
//				return;
//			}
//		}
//		else
//		{
//			if (maybe_grab_to_hang(m_frame_start_pos, m_pos) || maybe_grab_to_ladder(m_frame_start_pos, m_pos))
//			{
//				CFeeler::sClearDefaultCache();
//				return;
//			}
//		}
//	}
	
//	if (mp_input_component->GetControlPad().m_triangle.GetPressed())
//	{
//		if (maybe_stick_to_rail())
//		{
//			CFeeler::sClearDefaultCache();
//			return;
//		}
//	}
	
	// insure that we do not slip through the cracks in the collision geometry which are a side-effect of moving collidable objects
	Mth::Vector previous_pos = m_pos;
	if (CCompositeObject* p_inside_object = mp_movable_contact_component->CheckInsideObjects(m_pos, m_frame_start_pos))
	{
		MESSAGE("WALKING_AIR, within moving object");
		
		m_horizontal_vel.Set();
		m_vertical_vel = 0.0f;
		check_for_landing(m_pos, previous_pos);
	}
	
	CFeeler::sClearDefaultCache();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::get_controller_input( void )
{
	// If no rider, ignore input.
	if( mp_rider == NULL )
	{
		// Set it as if we are trying to slow downn to the max.
		m_control_direction[X]	= -1.0f;
		m_control_direction[Z]	= 0.0f;
		m_control_magnitude		= 1.0f;
		return;
	}

	CControlPad& control_pad = mp_input_component->GetControlPad();
	
//	Dbg_Assert(mp_camera);
	
	// rotate controller direction into camera's frame
//	Mth::Vector camera_forward = -mp_camera->m_matrix[Z];
//	camera_forward[Y] = 0.0f;
//	camera_forward.Normalize();
	
	// allow a tolerance range for pressing directly forward
	float angle = control_pad.m_leftAngle;
	if (Mth::Abs(angle) < Mth::DegToRad(s_get_param(CRCD(0x4676a268, "forward_tolerance"))))
	{
		angle = 0.0f;
	}
	
	float sin_angle = sinf(angle);
	float cos_angle = cosf(angle);
//	m_control_direction[X] = cos_angle * camera_forward[X] - sin_angle * camera_forward[Z];
//	m_control_direction[Z] = sin_angle * camera_forward[X] + cos_angle * camera_forward[Z];
	m_control_direction[X] = cos_angle;
	m_control_direction[Z] = sin_angle;
	
	// different control schemes for analog stick and d-pad
#	if 0
	if (control_pad.m_leftX == 0.0f && control_pad.m_leftY == 0.0f)
	{
		// d-pad control
		if (control_pad.m_leftLength == 0.0f)
		{
			m_control_magnitude = 0.0f;
			m_control_pegged = false;
			
			// don't reset dpad in the air
			if (m_state != WALKING_AIR)
			{
				m_dpad_used_last_frame = false;
			}
		}
		else
		{
			if (!m_dpad_used_last_frame)
			{
				m_dpad_use_time_stamp = Tmr::GetTime();
			}
			m_dpad_used_last_frame = true;
			
			if (m_state == WALKING_GROUND)
			{
				// slowly ramp up to a full run
				
				Tmr::Time elapsed_time = Tmr::ElapsedTime(m_dpad_use_time_stamp);
				
				Tmr::Time full_run_dpad_delay = static_cast< Tmr::Time >(s_get_param(CRCD(0x1832588c, "full_run_dpad_delay")));
				Tmr::Time start_run_dpad_delay = static_cast< Tmr::Time >(s_get_param(CRCD(0x2c386a43, "start_run_dpad_delay")));
				
				if (elapsed_time < start_run_dpad_delay)
				{
					m_control_magnitude = s_get_param(CRCD(0xc1528f7f, "walk_point"));
				}
				else if (elapsed_time < full_run_dpad_delay)
				{
					m_control_magnitude = Mth::SmoothMap(
						s_get_param(CRCD(0xc1528f7f, "walk_point")),
						1.0f,
						elapsed_time,
						start_run_dpad_delay,
						full_run_dpad_delay
					);
				}
				else
				{
					m_control_magnitude = 1.0f;
				}
			}
			else
			{
				m_control_magnitude = 1.0f;
			}
            m_control_pegged = true;
		}
        
		// damp dpad control directions towards forward when running
        if (m_state == WALKING_GROUND && Mth::Abs(angle) < Mth::DegToRad(90.0f + 5.0f) && (forced_run() || m_control_magnitude > s_get_param(CRCD(0xc1528f7f, "walk_point"))))
        {
//			if (forced_run() || m_control_magnitude == 1.0f)
//			{
//				m_control_direction += s_get_param(CRCD(0x3c581621, "dpad_control_damping_factor")) * camera_forward;
//			}
//			else
//			{
//				// smoothly interpolate between damping and no damping
//				m_control_direction += Mth::SmoothMap(0.0f, s_get_param(CRCD(0x3c581621, "dpad_control_damping_factor")), m_control_magnitude, s_get_param(CRCD(0xc1528f7f, "walk_point")), 1.0f)
//					* camera_forward;
//			}
			m_control_direction.Normalize();
        }
	}
	else
#	endif
	{
		// analog stick control
		m_control_magnitude = control_pad.GetScaledLeftAnalogStickMagnitude() / 0.85f;
		if (m_control_magnitude >= 1.0f)
		{
			m_control_magnitude = 1.0f;
			m_control_pegged = true;
		}
		else
		{
			m_control_pegged = false;
		}
	}
	
	// during a forward control lock, ignore all input not in the forward direction
	if (m_state == WALKING_GROUND && m_forward_control_lock)
	{
		m_control_magnitude = Mth::ClampMin(m_control_magnitude * Mth::DotProduct(m_control_direction, m_facing), 0.0f);
		m_control_direction = m_facing;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::uber_frig( void )
{
	// Ensure that we don't fall to the center of the earth, even if there are holes in the geometry.
	// Also, do lighting since we've got the feeler anyway.
	CFeeler feeler;
	feeler.m_start = m_pos;
	feeler.m_start[Y] += 1.0f;
	feeler.m_end = m_pos;
	feeler.m_end[Y] -= FEET(400);
	
	if( feeler.GetCollision())
	{
		mp_model_component->ApplyLightingFromCollision( feeler );
		return;
	}
	
	// Teleport us back to our position at the frame's start; not pretty, but this isn't supposed to be.
	m_pos = m_frame_start_pos;
	
	// Zero our velocity too.
	m_horizontal_vel.Set();
	m_vertical_vel = 0.0f;
	
	// Set our state to ground
	set_state( WALKING_GROUND );
			  	
	m_last_ground_feeler_valid = false;
	
	m_ground_normal.Set(0.0f, 1.0f, 0.0f);

	// Reset our script state.
	m_frame_event = CRCD(0x57ff2a27, "Land");
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::lerp_upright( void )
{
//    if (m_upward[Y] == 1.0f) return;
	
//	if (m_upward[Y] > 0.999f)
//	{
//		m_upward.Set(0.0f, 1.0f, 0.0f);
//		return;
//	}
	
//	m_upward = Mth::Lerp(m_upward, Mth::Vector(0.0f, 1.0f, 0.0f), s_get_param(CRCD(0xf22c135, "lerp_upright_rate")) * Tmr::FrameLength());
//	m_upward.Normalize();

	m_upward = m_ground_normal;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::copy_state_into_object( void )
{
	// Build the object's matrix based on our facing.
	Mth::Matrix matrix;
	
	// A horse requires special consideration, since it will necessarily tilt to match it's environment along it's long axis,
	// but will need to remain upright (or very nearly so) along it's shortest axis, since otherwise given it's high center
	// of gravity, it would topple over when standing sideways on a steep slope.
	matrix[X]		= Mth::CrossProduct( m_upward, m_facing );
	matrix[X][Y]	= 0.0f;
	matrix[X].Normalize();

	matrix[Y]		= m_upward;
	matrix[Z]		= Mth::CrossProduct( matrix[X], matrix[Y] );
	matrix[Z].Normalize();

	matrix[Y]		= Mth::CrossProduct( matrix[Z], matrix[X] );

	matrix[W].Set( 0.0f, 0.0f, 0.0f, 1.0f );

	GetObject()->SetPos( m_pos );
	GetObject()->SetMatrix( matrix );

	// The display matrix is slerped towards the current matrix.
	Mth::SlerpInterpolator slerper( &m_display_slerp_matrix, &matrix );
			
	// Apply the slerping.
	slerper.getMatrix( &m_display_slerp_matrix, GetTimeAdjustedSlerp( 0.1f, m_frame_length ));
	
	GetObject()->SetDisplayMatrix( m_display_slerp_matrix );

	// Construct the object's velocity.
	GetObject()->SetVel( m_horizontal_vel );
	GetObject()->GetVel()[Y] = m_vertical_vel;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Mth::Vector CHorseComponent::calculate_feeler_offset_direction( int contact )
{
	float angle = contact * (2.0f * Mth::PI / vNUM_FEELERS);
	float cos_angle = cosf(angle);
	float sin_angle = sinf(angle);

	Mth::Vector end_offset_direction;
	end_offset_direction[X] = cos_angle * m_facing[X] - sin_angle * m_facing[Z];
	end_offset_direction[Y] = 0.0f;
	end_offset_direction[Z] = sin_angle * m_facing[X] + cos_angle * m_facing[Z];
	end_offset_direction[W] = 0.0f;
	
	return end_offset_direction;
}




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::adjust_curb_float_height( void )
{
	// adjust m_curb_float_height to smooth out moving up stairs
	
	// When facing a curb, we smoothly increase m_curb_float_height to the height of the curb.  When we snap up the curb, m_curb_float_height is then
	// reduced by an amount equal to the snap distance.
	// When we snap down a curb, m_curb_float_height is increased by the snap distance.  We then drop m_curb_float_height smoothly to zero.
	
	// determine appropriate direction to search for a curb
	Mth::Vector feeler_direction = m_facing;
	feeler_direction.ProjectToPlane(m_ground_normal);
	feeler_direction[Y] = Mth::ClampMin(feeler_direction[Y], 0.0f);
	feeler_direction.Normalize();
	
	// look for a curb
	CFeeler feeler;
	feeler.m_start = m_pos;
	feeler.m_start[Y] += 0.5f;
	feeler.m_end = m_pos + s_get_param(CRCD(0x11edcc52, "curb_float_feeler_length")) * feeler_direction;
    feeler.m_end[Y] += 0.5f;
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
	{
		feeler.DebugLine(0, 255, 0, 1);
	}
	#endif
	
	if (feeler.GetCollision())
	{
		// grab the distance to the curb
		float distance_to_curb = feeler.GetDist();
		
		// look up from the curb to find the curb height
		feeler.m_end = feeler.GetPoint() + 0.5f * feeler_direction;
		feeler.m_start = feeler.m_end;
		feeler.m_start[Y] = m_pos[Y] + s_get_param(CRCD(0xcee3a3e1, "snap_up_height"));
		#ifdef __USER_DAN__
		if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
		{
			feeler.DebugLine(0, 255, 255, 1);
		}
		#endif
		
		if (feeler.GetCollision())
		{
			// calculate the m_curb_float_height we should have based on the curb height and distance
			float appropriate_curb_float_height = (1.0f - distance_to_curb) * (feeler.GetPoint()[Y] - m_pos[Y]);
			
			if (Mth::Abs(m_curb_float_height) < 0.01f && m_control_magnitude == 0.0f && m_horizontal_vel.LengthSqr() < Mth::Sqr(s_get_param(CRCD(0x227d72ee, "min_curb_height_adjust_vel"))))
			{
				// don't update the curb height if we're on the ground and standing still; this is mostly to prevent snapping up right after landing a jump
			}
			else
			{
				// lerp to the appropriate height
				m_curb_float_height = Mth::Lerp(m_curb_float_height, appropriate_curb_float_height, s_get_param(CRCD(0x856a80d3, "curb_float_lerp_up_rate")) * m_frame_length);
			}
			
			m_curb_float_height_adjusted = true;
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::respond_to_ground( void )
{
	// Reset ground normal to straight up.
	m_ground_normal.Set( 0.0f, 1.0f, 0.0f, 0.0f );

	// Look for the ground below us.  If we find it, snap to it.  If not, go to air state.
	CFeeler feeler;
	feeler.m_start = m_pos;
	feeler.m_start[Y] += s_get_param(CRCD(0xcee3a3e1, "snap_up_height"));
	feeler.m_end = m_pos;
	feeler.m_end[Y] -= s_get_param(CRCD(0xaf3e4251, "snap_down_height"));
	
	if (!feeler.GetCollision())
	{
		// no ground
		
//		if (m_last_ground_feeler_valid)
//		{
//			mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_OFF_EDGE, m_last_ground_feeler);
//			if (should_bail_from_frame()) return;
//		}
		
		// go to air state
		set_state(WALKING_AIR);
		m_primary_air_direction = m_facing;
		leave_movable_contact_for_air(m_horizontal_vel, m_vertical_vel);
		m_frame_event = CRCD(0xabf1f6ac, "WalkOffEdge");
		return;
	}
	
	float snap_distance = feeler.GetPoint()[Y] - m_pos[Y];
	
	// no not send event for very small snaps
	if (Mth::Abs(snap_distance) > s_get_param(CRCD(0xd3193d8e, "max_unnoticed_ground_snap")))
	{
		GetObject()->SelfEvent(snap_distance > 0.0f ? CRCD(0x93fcf3ed, "SnapUpEdge") : CRCD(0x56e21153, "SnapDownEdge"));
	}
	
	// snap position to the ground
	m_pos[Y] = feeler.GetPoint()[Y];
	
	// adjust stair float distance
	m_curb_float_height = Mth::ClampMin(m_curb_float_height - snap_distance, 0.0f);
	
	// see if we've changed sectors
	if (m_last_ground_feeler.GetSector() != feeler.GetSector())
	{
//		if (m_last_ground_feeler_valid)
//		{
//			mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_OFF, m_last_ground_feeler);
//			if (should_bail_from_frame()) return;
//		}
//		mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_ONTO, feeler);
//		if (should_bail_from_frame()) return;
	}
	
	// stash the ground feeler so that we can trip the group's triggers at a later time
	m_last_ground_feeler = feeler;
	m_last_ground_feeler_valid = true;
	
	// set the ground normal for next frame's velocity slope adjustment
	m_ground_normal = feeler.GetNormal();

	// NOTE: need to repeat this code anywhere we enter the ground state
	mp_movable_contact_component->CheckForMovableContact(feeler);
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::leave_movable_contact_for_air( Mth::Vector& horizontal_vel, float& vertical_vel )
{
	// use movement from the latest movable contact update call
	if (mp_movable_contact_component->HaveContact())
	{
		// keep track of the horizontal velocity due to our old contact
		m_uncontrollable_air_horizontal_vel = mp_movable_contact_component->GetContact()->GetObject()->GetVel();

		if (Mth::DotProduct(m_uncontrollable_air_horizontal_vel, horizontal_vel) > 0.0f)
		{
			// extra kicker; dangerous as there's no collision detection; without this slight extra movement, when we walk off the front of a movable object,
			// the object will move back under us before our next frame, and we will clip its edge and land on it
			m_pos += m_uncontrollable_air_horizontal_vel * m_frame_length;
		}
		
		// add movable contact's velocity into our launch velocity
		vertical_vel += m_uncontrollable_air_horizontal_vel[Y];
		m_uncontrollable_air_horizontal_vel[Y] = 0.0f;
		horizontal_vel += m_uncontrollable_air_horizontal_vel;
	}
	else
	{
		m_uncontrollable_air_horizontal_vel.Set();
	}

	mp_movable_contact_component->LoseAnyContact();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::control_horizontal_vel( void )
{
	// We allow user control over the object's in air velocity.  The algorithm is complicated by the fact that the forward velocity of the jump needs
	// to be accounted for when allowing for velocity adjustment.  It is assumed that the jump direction is the same as the facing.
	
	// remove uncontrollable velocity term
	m_horizontal_vel -= m_uncontrollable_air_horizontal_vel;
	
	// forced run still works in the air
//	adjust_control_for_forced_run();
	
	// adjust speed by the script set drag factor
	float adjust_magnitude = m_control_magnitude * m_script_drag_factor;
	
	// adjust velocity perpendicular to jump direction
	
	// direction perpendicular to jump direction
	Mth::Vector perp_direction( -m_primary_air_direction[Z], 0.0f, m_primary_air_direction[X], 0.0f );
	
	// desired perpendicular velocity
	float perp_desired_vel = s_get_param(CRCD(0x896c8888, "jump_adjust_speed")) * adjust_magnitude * Mth::DotProduct(m_control_direction, perp_direction);
	
	// current perpendicular velocity
	float perp_vel = Mth::DotProduct(m_horizontal_vel, perp_direction);
	
	// exponentially approach desired velocity
	perp_vel = Mth::Lerp(perp_vel, perp_desired_vel, s_get_param(CRCD(0xf085443b, "jump_accel_factor")) * m_frame_length);
		
	// adjust velocity parallel to jump direction
	
	// desired parallel velocity
	float para_desired_vel = s_get_param(CRCD(0x896c8888, "jump_adjust_speed")) * adjust_magnitude * Mth::DotProduct(m_control_direction, m_primary_air_direction);
	
	// current parallel velocity
	float para_vel = Mth::DotProduct(m_horizontal_vel, m_primary_air_direction);
	
    // if desired velocity if forward and forward velocity already exceeds adjustment velocity
	if (para_desired_vel >= 0.0f && para_vel > para_desired_vel)
	{
		// do nothing; don't slow down the jump
	}
	else
	{
		// adjust desired velocity to center around current velocity to insure that our in air stopping ability is not too amazing
		if (para_desired_vel < 0.0f && para_vel > 0.0f)
		{
			para_desired_vel += para_vel;
		}
		
		// expondentially approach desired velocity
		para_vel = Mth::Lerp(para_vel, para_desired_vel, s_get_param(CRCD(0xf085443b, "jump_accel_factor")) * m_frame_length);
	}
		
	// rebuild horizontal velocity from parallel and perpendicular components
	// Dave note - not sure about controlling the horse velocity in midair, so disable for now.
//	m_horizontal_vel = para_vel * m_primary_air_direction + perp_vel * perp_direction;
	
	// reinstitute uncontrollable velocity term
	m_horizontal_vel += m_uncontrollable_air_horizontal_vel;

}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::check_for_landing( const Mth::Vector& previous_pos, const Mth::Vector& final_pos )
{
	// See if our feet have passed through geometry.  If so, snap to it and go to ground state.
	
	CFeeler feeler;
	feeler.m_start = previous_pos;
	feeler.m_end = final_pos;
	if (!feeler.GetCollision()) return;
	
	// snap to the collision point
	m_pos = feeler.GetPoint();
	
	// zero vertical velocity
	m_vertical_vel = 0.0f;
	
	// change to ground state
	set_state(WALKING_GROUND);
	
	// stash the feeler
	m_last_ground_feeler = feeler;
	m_last_ground_feeler_valid = true;
	
	// trip any land trigger
//	mp_trigger_component->CheckFeelerForTrigger(TRIGGER_LAND_ON, m_last_ground_feeler);
//	if (should_bail_from_frame()) return;
	
	// setup our ground normal for next frames velocity slope adjustment
	m_ground_normal = feeler.GetNormal();
	
	// check for a moving contact
	mp_movable_contact_component->CheckForMovableContact(feeler);
	if (mp_movable_contact_component->HaveContact())
	{
		m_horizontal_vel -= mp_movable_contact_component->GetContact()->GetObject()->GetVel();
		m_horizontal_vel[Y] = 0.0f;
	}
	
	// retain only that velocity which is parallel to our facing and forward
	if (Mth::DotProduct(m_horizontal_vel, m_facing) > 0.0f)
	{
		m_horizontal_vel.ProjectToNormal(m_facing);
	}
	else
	{
		m_horizontal_vel.Set();
	}
	
	// clear any jump requests
	mp_input_component->GetControlPad().m_x.ClearRelease();
	
	m_frame_event = CRCD(0x57ff2a27, "Land");
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::adjust_vertical_vel_for_ceiling( void )
{
	// If we hit our head, zero vertical velocity
	
	// only worry about the ceiling if we're moving upwards
	if (m_vertical_vel <= 0.0f) return;
	
	// look for a collision up through the body to the head
	CFeeler feeler;
	feeler.m_start = m_pos;
	feeler.m_end = m_pos;
	feeler.m_end[Y] += s_get_param(CRCD(0x9ea1974a, "walker_height"));
	if (!feeler.GetCollision()) return;
	
	// zero upward velocity
	m_vertical_vel = 0.0f;
	
	GetObject()->SelfEvent(CRCD(0x6e84acf3, "HitCeiling"));
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::position_rider( void )
{
	// HACK: get player proximity checks, triggers, and the like working
//	CCompositeObject* p_skater = static_cast< CCompositeObject* >(CCompositeObjectManager::Instance()->GetObjectByID( 0 ));
//	p_skater->SetPos( GetObject()->GetPos());
}




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool get_collision( Mth::Vector& start, Mth::Vector& direction, float length, float* p_worst_length )
{
	CFeeler feeler;
	
	feeler.m_start			= start;
	feeler.m_end			= start + length * direction;

	// No collision as yet.
	bool collision	= false;
	*p_worst_length	= length;

	Gfx::AddDebugLine( feeler.m_start, feeler.m_end, MAKE_RGB( 0, 128, 0 ), MAKE_RGB( 0, 128, 0 ), 1 );

	if( feeler.GetCollision())
	{
		if( Mth::Abs( feeler.GetNormal()[Y] ) < 0.2f )
		{
			collision = true;
			*p_worst_length = length * feeler.GetDist();
			Gfx::AddDebugLine( feeler.m_start, feeler.m_end, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 1 );
		}
	}

	Mth::Vector offset( -direction[Z], 0.0f, direction[X], 0.0f );

	// Try 4 inches to the left.
	feeler.m_start			+= offset * 4.0f;
	feeler.m_end			+= offset * 4.0f;
	if( feeler.GetCollision())
	{
		if( Mth::Abs( feeler.GetNormal()[Y] ) < 0.2f )
		{
			collision = true;
			if(( length * feeler.GetDist()) < *p_worst_length )
			{
				*p_worst_length = length * feeler.GetDist();
				Gfx::AddDebugLine( feeler.m_start, feeler.m_end, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 1 );
			}
		}
	}

	// Try 8 inches to the left.
	feeler.m_start			+= offset * 4.0f;
	feeler.m_end			+= offset * 4.0f;
	if( feeler.GetCollision())
	{
		if( Mth::Abs( feeler.GetNormal()[Y] ) < 0.2f )
		{
			collision = true;
			if(( length * feeler.GetDist()) < *p_worst_length )
			{
				*p_worst_length = length * feeler.GetDist();
				Gfx::AddDebugLine( feeler.m_start, feeler.m_end, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 1 );
			}
		}
	}

	// Try 12 inches to the left.
	feeler.m_start			+= offset * 4.0f;
	feeler.m_end			+= offset * 4.0f;
	if( feeler.GetCollision())
	{
		if( Mth::Abs( feeler.GetNormal()[Y] ) < 0.2f )
		{
			collision = true;
			if(( length * feeler.GetDist()) < *p_worst_length )
			{
				*p_worst_length = length * feeler.GetDist();
				Gfx::AddDebugLine( feeler.m_start, feeler.m_end, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 1 );
			}
		}
	}

	// Try 4 inches to the right.
	feeler.m_start			-= offset * 16.0f;
	feeler.m_end			-= offset * 16.0f;
	if( feeler.GetCollision())
	{
		if( Mth::Abs( feeler.GetNormal()[Y] ) < 0.2f )
		{
			collision = true;
			if(( length * feeler.GetDist()) < *p_worst_length )
			{
				*p_worst_length = length * feeler.GetDist();
				Gfx::AddDebugLine( feeler.m_start, feeler.m_end, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 1 );
			}
		}
	}

	// Try 8 inches to the right.
	feeler.m_start			-= offset * 4.0f;
	feeler.m_end			-= offset * 4.0f;
	if( feeler.GetCollision())
	{
		if( Mth::Abs( feeler.GetNormal()[Y] ) < 0.2f )
		{
			collision = true;
			if(( length * feeler.GetDist()) < *p_worst_length )
			{
				*p_worst_length = length * feeler.GetDist();
				Gfx::AddDebugLine( feeler.m_start, feeler.m_end, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 1 );
			}
		}
	}

	// Try 12 inches to the right.
	feeler.m_start			-= offset * 4.0f;
	feeler.m_end			-= offset * 4.0f;
	if( feeler.GetCollision())
	{
		if( Mth::Abs( feeler.GetNormal()[Y] ) < 0.2f )
		{
			collision = true;
			if(( length * feeler.GetDist()) < *p_worst_length )
			{
				*p_worst_length = length * feeler.GetDist();
				Gfx::AddDebugLine( feeler.m_start, feeler.m_end, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 1 );
			}
		}
	}

	return collision;
}







const float AVOID_COLLISION_SCORE			= 1000.0f;
const float	TURN_ANGLE_SCORE_MULT			= 1.0f;
const float DISTANCE_PERCENTAGE_SCORE_MULT	= 0.1f;

const float	PANIC_SCORE						= 900.0f;


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::do_path_following( void )
{
	// Get the horse camera component for this horse. Messy.
	Obj::CHorseCameraComponent* p_cam_component = static_cast<Obj::CHorseCameraComponent*>( Obj::CCompositeObjectManager::Instance()->GetFirstComponentByType( CRC_HORSECAMERA ));
	while( p_cam_component )
	{
		if( p_cam_component->GetSubject() == this )
		{
			break;
		}
		p_cam_component = static_cast<Obj::CHorseCameraComponent*>( p_cam_component->GetNextSameType());
	}

	if( p_cam_component )
	{
		if( p_cam_component->GetState() == CHorseCameraComponent::STATE_NORMAL )
		{
			// No path following in normal state. Cleanup if we were doing path following.
			if( mp_nav_nodes )
			{
				cleanup_node_based_path_following();
			}
			return;
		}

		// Did we turn this frame? If so, all bets are off...
		if( m_delta_angle != 0.0f )
		{
			// Cleanup if we were doing path following.
			if( mp_nav_nodes )
			{
				cleanup_node_based_path_following();
			}
			return;
		}

		// Are we path following? In which case, follow the path.
		if( mp_nav_nodes )
		{
			do_node_based_path_following();
			return;
		}

		// See if we are going to hit something soon if we stay on this course.
		float feeler_height = 36.0f;
		float feeler_length	= 50.0f * 12.0f;

		float		angle		= 0.0f;
		float		cos_angle	= cosf( angle );
		float		sin_angle	= sinf( angle );
		float		worst_length;

		Mth::Vector	feeler_start;
		Mth::Vector	feeler_end;
		Mth::Vector	end_offset_direction;

		end_offset_direction[X] = cos_angle * m_facing[X] - sin_angle * m_facing[Z];
		end_offset_direction[Y] = 0.0f;
		end_offset_direction[Z] = sin_angle * m_facing[X] + cos_angle * m_facing[Z];
		end_offset_direction[W] = 0.0f;

		feeler_start			= Mth::Vector( m_pos[X], m_pos[Y] + feeler_height, m_pos[Z], m_pos[W] );
		feeler_end				= feeler_start + feeler_length * end_offset_direction;
		
		if( get_collision( feeler_start, end_offset_direction, feeler_length, &worst_length ))
//		if( feeler.GetCollision())
		{
			float dist = worst_length;

			// We need to figure out if turning a few degrees will enable us to obtain a clear path for
			// a significantly further distance.
			feeler_length *= 2.0f;

			float	best_score					= 0.0f;
			float	best_angle					= Mth::DegToRad( 90.0f );

			// Scan through angles from +9 degrees to -9 degrees in 3 degree steps.
			float step_angle = Mth::DegToRad( -9.0f );
			while( step_angle <= Mth::DegToRad( 9.0f ))
			{
				float score = 0.0f;

				cos_angle	= cosf( step_angle );
				sin_angle	= sinf( step_angle );

				end_offset_direction[X] = cos_angle * m_facing[X] - sin_angle * m_facing[Z];
				end_offset_direction[Y] = 0.0f;
				end_offset_direction[Z] = sin_angle * m_facing[X] + cos_angle * m_facing[Z];
				end_offset_direction[W] = 0.0f;

				feeler_end				= feeler_start + feeler_length * end_offset_direction;

				// Score for avoiding collision.
				bool collision = get_collision( feeler_start, end_offset_direction, feeler_length, &worst_length );

				if( !collision )
				{
					score += AVOID_COLLISION_SCORE;
				}

				// Score for minimal turn distance.
				score -= Mth::Abs( step_angle ) * TURN_ANGLE_SCORE_MULT;

				if( collision )
				{
					// Score for larger distances, use the percentage of this distance over the original collision distance.
					float this_dist	= worst_length;
					if( this_dist > dist )
					{
						float pc = ( this_dist / dist ) * 100.0f;
						score += pc * DISTANCE_PERCENTAGE_SCORE_MULT;
					}
				}

				// Score for being a turn in the same direction as last time.

				// Is this the best score yet?
				if( score > best_score )
				{
					best_score = score;
					best_angle = step_angle;
				}
				step_angle += Mth::DegToRad( 3.0f );
			}

			// Check for being in a potentially nasty situation.
			if( best_score < PANIC_SCORE )
			{
				// Increment panic counter, give a small chance to fix itself.
				++m_path_follow_panic_counter;
				if( m_path_follow_panic_counter > 0 )
				{
					m_path_follow_panic_counter = 0;
					switch_to_node_based_path_following();
					return;
				}
			}

			// Test - Immediately snap the direction to the best alternative.
			if( best_score > 0.0f )
			{
				cos_angle	= cosf( best_angle );
				sin_angle	= sinf( best_angle );

				end_offset_direction[X] = cos_angle * m_facing[X] - sin_angle * m_facing[Z];
				end_offset_direction[Z] = sin_angle * m_facing[X] + cos_angle * m_facing[Z];

				printf( "Changing heading by %f degrees\n", best_angle );

				m_facing[X]				= end_offset_direction[X];
				m_facing[Z]				= end_offset_direction[Z];
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::cleanup_node_based_path_following( void )
{
	delete [] mp_nav_nodes;
	mp_nav_nodes = NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::switch_to_node_based_path_following( void )
{
	// Try various distances, the further the better.
	float target_distance = FEET_TO_INCHES( 500.0f );

	while( target_distance > FEET_TO_INCHES( 50.0f ))
	{
		// Generate a distant target position.
		Mth::Vector start_pos	= m_pos + Mth::Vector( 0.0f, 60.0f, 0.0f, 0.0f );
		Mth::Vector target_pos	= start_pos + ( m_facing * target_distance );

		CFeeler feeler;
		feeler.m_start	= target_pos + Mth::Vector( 0.0f, 12000.0f, 0.0f, 0.0f );
		feeler.m_end	= target_pos - Mth::Vector( 0.0f, 12000.0f, 0.0f, 0.0f );

		if( feeler.GetCollision())
		{
			// Adjust the target position to be 5 feet above ground level.
			target_pos = feeler.GetPoint() + Mth::Vector(  0.0f, 60.0f, 0.0f, 0.0f );
		}
		else
		{
			// Hmm, maybe should try different distances here.
			// For now, Just go with what we started with.
		}

		CNavNode* p_target_node = CalculateNodePath( start_pos, target_pos );

		if( p_target_node == NULL )
		{
			// Try with a shorter distance.
			target_distance -= FEET_TO_INCHES( 50.0f );
			continue;
		}
		
		// A path was found, so set up the state accordingly. First count how many nodes on the path.
		CNavNode*	p_node_counter	= p_target_node;
		int			num_nodes		= 1;
		while( p_node_counter->GetAStarNode()->mp_parent != NULL )
		{
			++num_nodes;
			p_node_counter = p_node_counter->GetAStarNode()->mp_parent;
		}

		// Allocate memory for the path.
		if( mp_nav_nodes )
		{
			delete [] mp_nav_nodes;
		}
		mp_nav_nodes = new CNavNode*[num_nodes];

		// Fill in the array.
		m_nav_node_index = 0;
		while( p_target_node )
		{
			mp_nav_nodes[m_nav_node_index++] = p_target_node;
			p_target_node = p_target_node->GetAStarNode()->mp_parent;
		}
		--m_nav_node_index;

		// mp_nav_nodes[m_nav_node_index] now points to the first node we need to visit.
		break;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseComponent::do_node_based_path_following( void )
{
	// Are we within a reasonable distance of our current node?
	Mth::Vector target_node_pos = mp_nav_nodes[m_nav_node_index]->GetPos() + Mth::Vector( 0.0f, 36.0f, 0.0f, 0.0f );
	float dist = Mth::Distance( m_pos, target_node_pos );
	if( dist < ( 12.0f * 6.0f ))
	{
		// Yes we are. Exit if no more nodes.
		if( m_nav_node_index == 0 )
		{
			cleanup_node_based_path_following();
			return;
		}

		// Switch to the next node.
		--m_nav_node_index;
	}


	for( int l = m_nav_node_index; l >= 0; --l )
	{
		Gfx::AddDebugStar( mp_nav_nodes[l]->GetPos(), 12.0f, MAKE_RGB( 255, 200, 0 ), 1 );

		if( l > 0 )
		{
			Gfx::AddDebugLine( mp_nav_nodes[l]->GetPos(), mp_nav_nodes[l - 1]->GetPos(), MAKE_RGB( 255, 255, 255 ), MAKE_RGB( 255, 255, 255 ), 1 );
		}
	}

	// Head to the next node.
	Mth::Vector heading = mp_nav_nodes[m_nav_node_index]->GetPos() - m_pos;
	heading[Y] = 0.0f;
	heading[W] = 0.0f;
	heading.Normalize();

	m_facing[X]	= heading[X];
	m_facing[Z]	= heading[Z];
}


}
