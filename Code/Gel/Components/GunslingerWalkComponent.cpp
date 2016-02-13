//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       WalkComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  4/2/3
//****************************************************************************

#include <gel/components/walkcomponent.h>
#include <gel/components/inputcomponent.h>
#include <gel/components/animationcomponent.h>
#include <gel/components/walkcameracomponent.h>
#include <gel/components/modelcomponent.h>
#include <gel/components/triggercomponent.h>
#include <gel/components/movablecontactcomponent.h>
#include <gel/components/railmanagercomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/collision/collcache.h>

#include <sk/objects/rail.h>
#include <sk/engine/feeler.h>
#include <sk/modules/skate/skate.h>
#include <sk/components/skaterphysicscontrolcomponent.h>


/*
 * - Take out manual, take out manual, take out manual.
 * - Switch over to no-origin-teleport animations.
 * - Catching on curbs right after jump (use snap up code from skater)
 * - Moving contacts (and sticking to moving rails).
 * - Control Feel
 *    - Perhaps X equals run only during run outs.
 * - Camera
 *    - Flush requests don't quite complete using the timer method.  Perhaps with forward control locking, the goal method may once again be useful.
 *    - Transition out of vert facing camera.  Perhaps flush camera at first landing after an air transition.
 *    - Rewrite camera with "forward locking" affecting target matrix instead of turning off lerping.  Lookaround when walking shouldn't reset to behind
 *      during walking.
 * - Shadow update.
 * - Height update.
 * - Enter/exit walk behavior (always rotates to along slope, run out to walk behavior still looks odd).
 * - Turn off stance panel nollie.
 * - Step up/down (use feeler to look ahead).  Delay WalkOffEdge animation change.
 * - Animations:
 *    - Pull-up-from-hang has no feet!
 *    - Trot walking animation.
 *    - Drop-to-hang animation.
 *    - Hop-to-hang animation.
 *    - Three walk/run-to-stand animations.
 *    - Rotate walk/run animation.
 *    - Two ladder idle animations.
 *    - Onto-ladder-top animation. 
 *    - Off-ladder-bottom animation.
 *    - Ladder-to-hang/hang-to-ladder animations.
 *    - idle skater to stand
 *    - land to walk issues
 *    - special pull-up-to-wire anim
 *    - off-ladder-bottom anim to allow blending at anim start / teleport at anim end
 *    - onto-ladder-top anim to allow blending at anim start / teleport at anim end
 *    - grab-to-hang swing and wall anims suck
 *    - grab-to-hang sideways swing
 *    - Onto-ladder-top animation.  Currently play Off-ladder-top backwards.
 *    - Fall-air before jump-air anim.
 * - Animation comments:
 *    - running land doesn't really work (no weight), especially when you land and then immediately run off an edge and then land again
 *    - full run is odd looking
 * - Walk to stand animation will require at least three versions.
 * - Hang
 *    - Rail-to-rail ledge transition issues.
 *    - Jerky rail corners.
 *    - Non horizontal hang movement animations.
 *    - Camera pop during pull-up (caused by camera collision).
 *    - Jump off hang.
 * - Ladder
 *    - Jump off ladder.
 *    - Jump onto ladder.
 *    - You can grind ladders.
 * - Grab rail in air.
 * - Little talkie boxes don't always work.
 * - Gaps still trigger, but based on skate state.  Maybe run gap component while walking and include walking gap flag.
 * - How can we deal with a low snap-up height, yet allow for steep slope walking?
 * - Grind transition animation in Grind script (check to see if its playing).
 * - Clean 1/3 second delay before X running.
 * - Upper body interpenetrates stuff.
 * - Hang from bottom of ladder.
 * - Pull up collision restrictions are too restrictive (use push feeler distance?).
 * - Make it easier to grab a rail when you walk off an edge.
 * - Ladder to rail across top.  Rail across top to ladder.
 * - Hand-plants, drop-ins, etc.
 * BUGS:
 * - Snap to hangs can cause the origin to go to bad places?  NJ by your roof.
 * - Fall through verts with low frame rate.
 * - Pop in transition from last to first frames of slow cycling walk animations.
 */

namespace Obj
{
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static float s_get_gunslinger_param( uint32 checksum )
{
	Script::CStruct* p_walk_params = Script::GetStructure( CRCD( 0x1c33e162, "GunslingerWalkParameters" ));
	Dbg_Assert(p_walk_params);
	
	float param;
	p_walk_params->GetFloat(checksum, &param, Script::ASSERT);
	return param;
}

	
	
	
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CWalkComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CWalkComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CWalkComponent::CWalkComponent() : CBaseComponent()
{
	SetType( CRC_WALK );
	
	mp_collision_cache = Nx::CCollCacheManager::sCreateCollCache();
	
	mp_input_component = NULL;
	mp_animation_component = NULL;
	mp_movable_contact_component = NULL;

	m_facing.Set( 0.0f, 0.0f, 0.0f, 0.0f );
	m_control_direction.Set( 0.0f, 0.0f, 0.0f, 0.0f );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CWalkComponent::~CWalkComponent()
{
	Nx::CCollCacheManager::sDestroyCollCache(mp_collision_cache);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CWalkComponent::Finalize()
{
	mp_input_component = GetInputComponentFromObject(GetObject());
	mp_animation_component = GetAnimationComponentFromObject(GetObject());
	mp_model_component = GetModelComponentFromObject(GetObject());
	mp_trigger_component = GetTriggerComponentFromObject(GetObject());
	mp_physics_control_component = GetSkaterPhysicsControlComponentFromObject(GetObject());
	mp_movable_contact_component = GetMovableContactComponentFromObject(GetObject());
	
	Dbg_Assert(mp_input_component);
	Dbg_Assert(mp_animation_component);
	Dbg_Assert(mp_model_component);
	Dbg_Assert(mp_trigger_component);
	Dbg_Assert(mp_physics_control_component);
	Dbg_Assert(mp_movable_contact_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::InitFromStructure( Script::CStruct* pParams )
{
	uint32 camera_id;
	if (pParams->GetChecksum(CRCD(0xc4e311fa, "camera"), &camera_id))
	{
		CCompositeObject* p_camera = static_cast< CCompositeObject* >(CCompositeObjectManager::Instance()->GetObjectByID(camera_id));
		Dbg_MsgAssert(mp_camera, ("No such camera object"));
		SetAssociatedCamera(p_camera);
	}
}
		 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::Update()
{
	// TEMP: debounce R1 after a transition
	if (m_ignore_grab_button && !mp_input_component->GetControlPad().m_R1.GetPressed())
	{
		m_ignore_grab_button = false;
	}

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
	
	// set the frame length
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
																	  
//		case WALKING_HOP:
//			go_hop_state();
//			break;
																	  
		case WALKING_HANG:
			go_hang_state();
			break;
            
		case WALKING_LADDER:
			go_ladder_state();
            break;
			
		case WALKING_ANIMWAIT:
			go_anim_wait_state (   );
			break;
	}
	
	// the there's no curb to adjust due to, lerp down to zero
	if (!m_curb_float_height_adjusted)
	{
		m_curb_float_height = Mth::Lerp(m_curb_float_height, 0.0f, s_get_gunslinger_param(CRCD(0x9b3388fa, "curb_float_lerp_down_rate")) * m_frame_length);
	}
	
	// adjust back to our curb float Y position
	m_pos[Y] += m_curb_float_height;
	
	// scripts may have restarted us / switched us to skating
//	if (should_bail_from_frame()) return;
	
	// keep the object from falling through holes in the geometry
	if (m_state == WALKING_GROUND || m_state == WALKING_AIR)
	{
		uber_frig();
	}
	
	// rotate to upright
	lerp_upright();
	
	// setup the object based on this frame's walking
	copy_state_into_object();
	
	Dbg_Assert(m_frame_event);
	GetObject()->SelfEvent(m_frame_event);
	
	// set the animation speeds
	switch (m_anim_scale_speed)
	{
		case RUNNING:
			if (m_anim_standard_speed > 0.0f)
			{
				mp_animation_component->SetAnimSpeed(m_anim_effective_speed / m_anim_standard_speed, false, false);
			}
			break;
			
		case HANGMOVE:
			mp_animation_component->SetAnimSpeed(m_anim_effective_speed / s_get_gunslinger_param(CRCD(0xd77ee881, "hang_move_speed")), false, false);
			break;
					
		case LADDERMOVE:
			mp_animation_component->SetAnimSpeed(m_anim_effective_speed / s_get_gunslinger_param(CRCD(0xab2db54, "ladder_move_speed")), false, false);
			break;
	
		default:
			break;
	}
	
	// camera controls
	// NOTE: script parameters
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
	
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
	{
		Gfx::AddDebugStar(GetObject()->GetPos(), 36.0f, MAKE_RGB(255, 255, 255), 1);
	}
	#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CWalkComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		// @script | Walk_Ground |
		case CRCC(0x893213e5, "Walk_Ground"):
			return m_state == WALKING_GROUND ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
		// @script | Walk_Air |
		case CRCC(0x5012082e, "Walk_Air"):
			return m_state == WALKING_AIR ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
		// @script | Walk_Hang |
		case CRCC(0x9a3ca853, "Walk_Hang"):
			return m_state == WALKING_HANG ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
		// @script | Walk_Ladder |
		case CRCC(0x19702ca8, "Walk_Ladder"):
			return m_state == WALKING_LADDER ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
		// @script | Walk_GetStateTime | Loads the time in milliseconds since last state change.
		case CRCC(0xce64576c, "Walk_GetStateTime"):
			pScript->GetParams()->AddInteger(CRCD(0x5ab23cc9, "StateTime"), Tmr::ElapsedTime(m_state_timestamp));
			break;
		
		// @script | Walk_Jump |
		case CRCC(0x83e4bd70, "Walk_Jump"):
		{
			// jump strength scales with the length the jump button has been held
			jump();
//			jump(Mth::Lerp(
//				s_get_gunslinger_param(CRCD(0x246d0bf3, "min_jump_factor")), 
//				1.0f,
//				Mth::ClampMax(mp_input_component->GetControlPad().m_x.GetPressedTime() / s_get_gunslinger_param(CRCD(0x12333ebd, "hold_time_for_max_jump")), 1.0f)
//			));
			break;
		}
		
//		case CRCC(0xeb0d763b, "Walk_HangJump"):
//		{
//			// jump strength scales with the length the jump button has been held
//			jump(s_get_gunslinger_param(CRCD(0xf2fa5845, "hang_jump_factor")), true);
//			break;
//		}
		
		// @script | Walk_SetDragFactor |
		case CRCC(0xc6100a7d, "Walk_SetDragFactor"):
			break;
			
		case CRCC(0x4e4fae43, "Walk_ResetDragFactor"):
			break;
			
		case CRCC(0xaf04b983, "Walk_GetSpeedScale"):
		{
			uint32 checksum;
			if (m_anim_effective_speed < s_get_gunslinger_param(CRCD(0xf3649996, "max_slow_walk_speed")))
			{
				checksum = CRCD(0x1150cabb, "WALK_SLOW");
			}
			else if (m_anim_effective_speed < s_get_gunslinger_param(CRCD(0x6a5805d8, "max_fast_walk_speed")))
			{
				checksum = CRCD(0x131f2a2, "WALK_FAST");
			}
			else if (m_anim_effective_speed < s_get_gunslinger_param(CRCD(0x1c94cc9c, "max_slow_run_speed")))
			{
				checksum = CRCD(0x5606d106, "RUN_SLOW");
			}
			else
			{
				checksum = CRCD(0x4667e91f, "RUN_FAST");
			}
			pScript->GetParams()->AddChecksum(CRCD(0x92c388f, "SpeedScale"), checksum);
			
			break;
		}
		
		// @script | Walk_ScaleAnimSpeed | Sets the manner in which the walk animations speeds should be scaled.
		// @flag Off | No animation speed scaling.
		// @flag Run | Scale animation speeds against running speed.
		// @flag Walk | Scale animation speeds against walking speed.
		case CRCC(0x56112c03, "Walk_ScaleAnimSpeed"):
			if (pParams->ContainsFlag(CRCD(0xd443a2bc, "Off")))
			{
				if (m_anim_scale_speed != OFF)
				{
					m_anim_scale_speed = OFF;
					mp_animation_component->SetAnimSpeed(1.0f, false, true);
				}
			}
			else if (pParams->ContainsFlag(CRCD(0xaf895b3f, "Run")))
			{
				m_anim_scale_speed = RUNNING;
			}
			else if (pParams->ContainsFlag(CRCD(0x6384f1da, "HangMove")))
			{
				m_anim_scale_speed = HANGMOVE;
			}
			else if (pParams->ContainsFlag(CRCD(0xa2bfe505, "LadderMove")))
			{
				m_anim_scale_speed = LADDERMOVE;
			}
			else
			{
				Dbg_MsgAssert(false, ("Walk_ScaleAnimSpeed requires Off, Run, or Walk flag"));
			}
			
			pParams->GetFloat(CRCD(0xb2d59baf, "StandardSpeed"), &m_anim_standard_speed);
			break;
			
		// @script | Walk_AnimWaitComplete | Signal from script that the walk component should leave its animation wait
		case CRCC(0x9d3eebe8, "Walk_AnimWaitComplete"):
			anim_wait_complete();
			break;
				   			
		// @script | Walk_GetHangInitAnimType | Determine which type of initial hang animation should be played
		case CRCC(0xc6cd659e, "Walk_GetHangInitAnimType"):
			// m_initial_hang_animation is set when the hang rail is filtered
			pScript->GetParams()->AddChecksum(CRCD(0x85fa9ac4, "HangInitAnimType"), m_initial_hang_animation);
			break;

		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::GetDebugInfo(Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info sent to CWalkComponent::GetDebugInfo"));
	
	switch (m_state)
	{
		case WALKING_GROUND:
			p_info->AddChecksum(CRCD(0x109b9260, "m_state"), CRCD(0x58007c97, "GROUND"));
			break;
		case WALKING_AIR:
			p_info->AddChecksum(CRCD(0x109b9260, "m_state"), CRCD(0x439f4704, "AIR"));
			break;
//		case WALKING_HOP:
//			p_info->AddChecksum(CRCD(0x109b9260, "m_state"), CRCD(0xf41aba21, "HOP"));
//			break;										 
		case WALKING_HANG:
			p_info->AddChecksum(CRCD(0x109b9260, "m_state"), CRCD(0x4194ecca, "HANG"));
			break;										 
        case WALKING_LADDER:
			p_info->AddChecksum(CRCD(0x109b9260, "m_state"), CRCD(0xc84243da, "LADDER"));
			break;										 
        case WALKING_ANIMWAIT:
			p_info->AddChecksum(CRCD(0x109b9260, "m_state"), CRCD(0x4fe6069c, "ANIMWAIT"));
			break;										 
	}

	CBaseComponent::GetDebugInfo(p_info);	  
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::SetAssociatedCamera ( CCompositeObject* camera_obj )
{
	mp_camera = camera_obj;
	Dbg_Assert(mp_camera);
	mp_camera_component = GetWalkCameraComponentFromObject(mp_camera);
	Dbg_MsgAssert(mp_camera_component, ("No WalkCameraComponent in camera object"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::ReadyWalkState ( bool to_ground_state )
{
	// setup the state in preparation for being in walking mode next object update
	MARK;
	
    // always reset the state timestamp
    m_state_timestamp = Tmr::GetTime();

	if (to_ground_state)
	{
		set_state(WALKING_GROUND);
		
		// will be incorrect for one frame
		m_ground_normal.Set(0.0f, 1.0f, 0.0);
		
		m_last_ground_feeler_valid = false;
		
		GetObject()->GetVel()[Y] = 0.0f;
	}
	else
	{
		set_state(WALKING_AIR);
		
		// set primary air direction in the direction of velocity
		m_primary_air_direction = GetObject()->GetVel();
		m_primary_air_direction[Y] = 0.0f;
		float length = m_primary_air_direction.Length();
		if (length < 0.001f)
		{
			// or facing
			m_primary_air_direction = GetObject()->GetMatrix()[Z];
			m_primary_air_direction[Y] = 0.0f;
			length = m_primary_air_direction.Length();
			if (length < 0.001f)
			{
				// or future facing
				m_primary_air_direction = -GetObject()->GetMatrix()[Y];
				m_primary_air_direction[Y] = 0.0f;
				length = m_primary_air_direction.Length();
			}
		}
		m_primary_air_direction /= length;
		
		leave_movable_contact_for_air(GetObject()->GetVel(), GetObject()->GetVel()[Y]);
	}

	m_curb_float_height = 0.0f;
	
	m_last_frame_event = 0;
	
	// TEMP: debounce R1 after a transition
	m_ignore_grab_button = true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::CleanUpWalkState (   )
{
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::go_on_ground_state( void )
{
	// Check for trying to mount a horse, triggered on triangle.
	if( mp_input_component->GetControlPad().m_triangle.GetTriggered())
	{
		mp_input_component->GetControlPad().m_triangle.ClearTrigger();

		// Get the control component.
		CSkaterPhysicsControlComponent*	p_control_component = GetSkaterPhysicsControlComponentFromObject( GetObject());
		p_control_component->CallMemberFunction( CRCD( 0x14c4f16b, "SkaterPhysicsControl_SwitchWalkingToRiding"), NULL, NULL );

		// Send the 'Ride' exception.
		m_frame_event = CRCD( 0x64c2832f, "Ride" );
		GetObject()->SelfEvent( m_frame_event );
		return;
	}

	account_for_movable_contact();
	
	setup_collision_cache();
	
	// calculate initial horizontal speed
	float horizontal_speed = m_horizontal_vel.Length();
	
	calculate_horizontal_speed_and_facing(horizontal_speed);
	
	// calculate this frame's movement
	m_horizontal_vel = horizontal_speed * m_facing;
	
	// prevent movement into walls
	if (adjust_horizonal_vel_for_environment())
	{
		// turn to face newly adjusted velocity
		adjust_facing_for_adjusted_horizontal_vel();
	}
	
	// if we are wall pushing, we may have decided to switch states during adjust_horizonal_vel_for_environment based on our environment
	if (m_state != WALKING_GROUND /*|| should_bail_from_frame()*/)
	{
		CFeeler::sClearDefaultCache();
		return;
	}
	
	// apply movement for this frame
	m_pos += m_horizontal_vel * m_frame_length;
	
	// snap up and down curbs and perhaps switch to air
	respond_to_ground();
	if (m_state != WALKING_GROUND /*|| should_bail_from_frame()*/)
	{
		CFeeler::sClearDefaultCache();
		return;
	}
	
	adjust_curb_float_height();
	
	// insure that we do not slip through the cracks in the collision geometry which are a side-effect of moving collidable objects
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

void CWalkComponent::CollideWithOtherSkaterLost ( CCompositeObject* p_other_skater )
{
	set_state(WALKING_AIR);
	m_primary_air_direction = m_facing;
	
	leave_movable_contact_for_air(GetObject()->GetVel(), GetObject()->GetVel()[Y]);
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::calculate_horizontal_speed_and_facing ( float &horizontal_speed )
{
	// calculate user's desired speed
	float desired_speed = calculate_desired_speed();
			   	
	// setup frame's event
	if (desired_speed <= s_get_gunslinger_param(CRCD(0x74e8227d, "max_stand_speed")))
	{
		m_frame_event = CRCD(0x9b46e749, "Stand");
	}
	else
	{
		m_frame_event = CRCD(0xaf895b3f, "Run");
	}

	bool special_acceleration = false;
	
	// adjust facing based on input
	
	if( m_control_magnitude > 0.0f )
	{
		float dot = Mth::DotProduct( m_facing, m_control_direction );
		
		if (horizontal_speed < s_get_gunslinger_param(CRCD(0x52582d5b, "max_rotate_in_place_speed")) && dot < cosf(Mth::DegToRad(s_get_gunslinger_param(CRCD(0x5dff96a4, "max_rotate_in_place_angle")))))
		{
			// low speed rotate to desired orientation with no speed change
			
			float delta_angle = Mth::DegToRad(s_get_gunslinger_param(CRCD(0xb557804b, "rotate_in_place_rate"))) * m_control_magnitude * m_frame_length;
			bool left_turn = -m_facing[Z] * m_control_direction[X] + m_facing[X] * m_control_direction[Z] < 0.0f;
			
			if (!m_run_toggle)
			{
				delta_angle *= s_get_gunslinger_param(CRCD(0x7b446c98, "walk_rotate_factor"));
			}
			
			float cos_delta_angle = cosf(left_turn ? delta_angle : -delta_angle);
			float sin_delta_angle = sinf(left_turn ? delta_angle : -delta_angle);
			float adjusted_vel = cos_delta_angle * m_facing[X] + sin_delta_angle * m_facing[Z];
			m_facing[Z] = -sin_delta_angle * m_facing[X] + cos_delta_angle * m_facing[Z];
			m_facing[X] = adjusted_vel;
			
			// check for overturn
			if (left_turn != (-m_facing[Z] * m_control_direction[X] + m_facing[X] * m_control_direction[Z] < 0.0f))
			{
				m_facing = m_control_direction;
			}
			
			// no acceleration until we reach the desired orientation
			special_acceleration = true;
			
			// setup the event
			m_frame_event = left_turn ? CRCD(0xf28adbfc, "RotateLeft") : CRCD(0x912220f8, "RotateRight");
		}
		else
		{
			if (dot > -cosf(Mth::DegToRad(s_get_gunslinger_param(CRCD(0x2d571c0f, "max_reverse_angle")))))
			{
				// if the turn angle is soft
				
				// below a speed threshold, scale up the turn rate
				float turn_factor;
				if (horizontal_speed < s_get_gunslinger_param(CRCD(0x27815f69, "max_pop_speed")))
				{
					// quick turn
					turn_factor = Mth::Lerp(s_get_gunslinger_param(CRCD(0xb278405d, "best_turn_factor")), s_get_gunslinger_param(CRCD(0x6cb2e5de, "worse_turn_factor")), horizontal_speed / s_get_gunslinger_param(CRCD(0x27815f69, "max_pop_speed"))); 
				}
				else
				{
					// slower turn
					turn_factor = s_get_gunslinger_param(CRCD(0x6cb2e5de, "worse_turn_factor"));
				}
				turn_factor *= m_control_magnitude;
				
				// exponentially approach the new facing
				float turn_ratio = turn_factor * m_frame_length;
				if (turn_ratio >= 1.0f)
				{
					m_facing = m_control_direction;
				}
				else
				{
					m_facing = Mth::Lerp(m_facing, m_control_direction, turn_ratio);
					m_facing.Normalize();
				}
			}
			else
			{
				// the turn angle is hard
				
				if (horizontal_speed > s_get_gunslinger_param(CRCD(0xf1e97e45, "min_skid_speed")))
				{
					// if moving fast enough to require a skidding stop
					special_acceleration = true;
					horizontal_speed -= s_get_gunslinger_param(CRCD(0x9661ed7, "skid_accel")) * m_frame_length;
					horizontal_speed = Mth::ClampMin(horizontal_speed, 0.0f);
					m_frame_event = CRCD(0x1d537eff, "Skid");
				}
				else
				{
					// if max_rotate_in_place_speed is larger than min_skid_speed and max_reverse_angle points farther in reverse than
					// max_rotate_in_place_angle, as they should, then this code should never be run
					Dbg_Message("Unexpected state in CWalkComponent::calculate_horizontal_speed_and_facing");
					
					// to be safe, pop to the new facing
					m_facing = m_control_direction;
				}
			}
		}
	}
	
	if (special_acceleration) return;
	
	// store desired speed for animation speed scaling
	m_anim_effective_speed = desired_speed;
	
	// adjust desired speed for slope
	desired_speed = adjust_desired_speed_for_slope(desired_speed);
	
	// linear acceleration; exponential deceleration
	if (horizontal_speed > desired_speed)
	{
		horizontal_speed = Mth::Lerp(horizontal_speed, desired_speed, s_get_gunslinger_param(CRCD(0xacfa4e0c, "decel_factor")) * m_frame_length);
		
		if (desired_speed == 0.0f)
		{
			if (horizontal_speed > s_get_gunslinger_param(CRCD(0x79d182ad, "walk_speed")))
			{
				m_frame_event = CRCD(0x1d537eff, "Skid");
			}
			else if (m_last_frame_event == CRCD(0x1d537eff, "Skid") && horizontal_speed > s_get_gunslinger_param(CRCD(0x311d02b2, "stop_skidding_speed")))
			{
				m_frame_event = CRCD(0x1d537eff, "Skid");
			}
		}
	}
	else
	{
		if (m_run_toggle)
		{
			horizontal_speed += s_get_gunslinger_param(CRCD(0x4f47c998, "run_accel_rate")) * m_frame_length;
		}
		else
		{
			horizontal_speed += s_get_gunslinger_param(CRCD(0x6590a49b, "walk_accel_rate")) * m_frame_length;
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

bool  CWalkComponent::adjust_horizonal_vel_for_environment ( void )
{
	// We send out feeler rays to find nearby walls.  We limit velocity to be flush with the first wall found.  If two or more non-parallel walls
	// are found, velocity is zeroed.
	
	float feeler_length = s_get_gunslinger_param(CRCD(0x99978d2b, "feeler_length"));
	float feeler_height = s_get_gunslinger_param(CRCD(0x6da7f696, "feeler_height"));
	
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
			feeler.m_end[Y] += m_vertical_vel * m_frame_length + 0.5f * -s_get_gunslinger_param(CRCD(0xa5e2da58, "gravity")) * Mth::Sqr(m_frame_length);
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
		if (check_for_wall_push())
		{
			// if we're wall pushing, we may decide to switch states based on our environment
			
			if (Tmr::ElapsedTime(m_wall_push_test.test_start_time) > s_get_gunslinger_param(CRCD(0x928e6775, "hop_delay")))
			{
				if (maybe_climb_up_ladder() || /*maybe_hop_to_hang() ||*/ maybe_jump_low_barrier()) return false;
			}
			else if (Tmr::ElapsedTime(m_wall_push_test.test_start_time) > s_get_gunslinger_param(CRCD(0x38d36700, "barrier_jump_delay")))
			{
				if (maybe_climb_up_ladder() || maybe_jump_low_barrier()) return false;
			}
		}
		else if (mp_input_component->GetControlPad().m_R1.GetPressed() && !m_ignore_grab_button)
		{
			if (maybe_climb_up_ladder(true)) return false;
		}
	}
	
	if (!contact) return false;
	
	// push away from walls
	for (int n = 0; n < vNUM_FEELERS + 1; n++)
	{
		if (!mp_contacts[n].in_collision) continue;
		                
		if (mp_contacts[n].feeler.GetDist() < s_get_param(CRCD(0xa20c43b7, "push_feeler_length")) / feeler_length)
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
				m_anim_effective_speed = Mth::Min(s_get_gunslinger_param(CRCD(0xbd6a05d, "min_anim_run_speed")), m_anim_effective_speed);
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
			m_anim_effective_speed = Mth::Min(s_get_gunslinger_param(CRCD(0xbd6a05d, "min_anim_run_speed")), m_anim_effective_speed);
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
	if (m_anim_effective_speed > s_get_gunslinger_param(CRCD(0xbd6a05d, "min_anim_run_speed")))
	{
		m_anim_effective_speed = final_horiz_vel;
		m_anim_effective_speed = Mth::Max(s_get_gunslinger_param(CRCD(0xbd6a05d, "min_anim_run_speed")), m_anim_effective_speed);
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CWalkComponent::adjust_facing_for_adjusted_horizontal_vel (   )
{
	// We adjust facing due to adjustment in horizontal velocity due to environment.  Basically, we want to object to turn to face the velocity
	// that the environment has forced upon it.
	
	// IDEA: shift to basing turn amount on angle difference and not speed
	
	float horizontal_speed = m_horizontal_vel.Length();
	
	if (horizontal_speed < s_get_gunslinger_param(CRCD(0x515a933, "wall_turn_speed_threshold"))) return;
	
	// the new facing is in the direction of our adjusted velocity
	Mth::Vector new_facing = m_horizontal_vel;
	new_facing.Normalize();
	
	// smoothly transition between no wall turning to full wall turning
	float turn_ratio;
	if (horizontal_speed > s_get_gunslinger_param(CRCD(0xe6c1cd0d, "max_wall_turn_speed_threshold")))
	{
		turn_ratio = s_get_gunslinger_param(CRCD(0x7a583b9b, "wall_turn_factor")) * m_frame_length;
	}
	else
	{
		turn_ratio = Mth::LinearMap(
			0.0f,
			s_get_gunslinger_param(CRCD(0x7a583b9b, "wall_turn_factor")) * m_frame_length,
			horizontal_speed,
			s_get_gunslinger_param(CRCD(0x0515a933, "wall_turn_speed_threshold")),
			s_get_gunslinger_param(CRCD(0xe6c1cd0d, "max_wall_turn_speed_threshold"))
		);
	}
	
	// exponentially approach new facing
	if (turn_ratio >= 1.0f)
	{
		m_facing = new_facing;
	}
	else
	{
		m_facing = Mth::Lerp(m_facing, new_facing, turn_ratio);
		m_facing.Normalize();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CWalkComponent::adjust_desired_speed_for_slope ( float desired_speed )
{
	// Slow velocity up and down slopes.
	
	// skip if there is no appreciable slope
	if (m_ground_normal[Y] > 0.95f) return desired_speed;
	
	// skip if not running
	if (desired_speed <= s_get_gunslinger_param(CRCD(0x79d182ad, "walk_speed"))) return desired_speed;
	
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
	
void CWalkComponent::respond_to_ground (   )
{
	// Look for the ground below us.  If we find it, snap to it.  If not, go to air state.
	
	CFeeler feeler;
	feeler.m_start = m_pos;
	feeler.m_start[Y] += s_get_gunslinger_param(CRCD(0xcee3a3e1, "snap_up_height"));
	feeler.m_end = m_pos;
	feeler.m_end[Y] -= s_get_gunslinger_param(CRCD(0xaf3e4251, "snap_down_height"));
	
	if (!feeler.GetCollision())
	{
		// no ground
		
		if (m_last_ground_feeler_valid)
		{
			mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_OFF_EDGE, m_last_ground_feeler);
//			if (should_bail_from_frame()) return;
		}
		
		if (mp_input_component->GetControlPad().m_triangle.GetPressed())
		{
			// need to give the player a change to rail before climbing down ladders and such
			
			// if we just climbed up to a rail and are immediately falling, we need some minute amount of movement in order to find a rail
			if (m_pos == m_frame_start_pos)
			{
				m_frame_start_pos[Y] += 0.001f;
			}
			
			if (maybe_stick_to_rail()) return;
		}
		
		if (maybe_climb_down_ladder() || maybe_drop_to_hang()) return;
		
		// go to air state
		set_state(WALKING_AIR);
		m_primary_air_direction = m_facing;
		leave_movable_contact_for_air(m_horizontal_vel, m_vertical_vel);
		m_frame_event = CRCD(0xabf1f6ac, "WalkOffEdge");
		return;
	}
	
	float snap_distance = feeler.GetPoint()[Y] - m_pos[Y];
	
	// no not send event for very small snaps
	if (Mth::Abs(snap_distance) > s_get_gunslinger_param(CRCD(0xd3193d8e, "max_unnoticed_ground_snap")))
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
		if (m_last_ground_feeler_valid)
		{
			mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_OFF, m_last_ground_feeler);
//			if (should_bail_from_frame()) return;
		}
		mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_ONTO, feeler);
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
	
void CWalkComponent::adjust_curb_float_height (   )
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
	feeler.m_end = m_pos + s_get_gunslinger_param(CRCD(0x11edcc52, "curb_float_feeler_length")) * feeler_direction;
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
		feeler.m_start[Y] = m_pos[Y] + s_get_gunslinger_param(CRCD(0xcee3a3e1, "snap_up_height"));
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
			
			if (Mth::Abs(m_curb_float_height) < 0.01f && m_control_magnitude == 0.0f && m_horizontal_vel.LengthSqr() < Mth::Sqr(s_get_gunslinger_param(CRCD(0x227d72ee, "min_curb_height_adjust_vel"))))
			{
				// don't update the curb height if we're on the ground and standing still; this is mostly to prevent snapping up right after landing a jump
			}
			else
			{
				// lerp to the appropriate height
				m_curb_float_height = Mth::Lerp(m_curb_float_height, appropriate_curb_float_height, s_get_gunslinger_param(CRCD(0x856a80d3, "curb_float_lerp_up_rate")) * m_frame_length);
			}
			
			m_curb_float_height_adjusted = true;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::account_for_movable_contact (   )
{
	if (!mp_movable_contact_component->UpdateContact(m_pos)) return;
	
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
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CWalkComponent::jump (   )
{
	// switch to air state and give the object an upwards velocity
	
	float strength = 0.0f;
	switch (m_state)
	{
		case WALKING_GROUND:
		case WALKING_AIR:
			// jump strength scales with the length the jump button has been held
			strength = Mth::Lerp(
				s_get_param(CRCD(0x246d0bf3, "min_jump_factor")), 
				1.0f,
				Mth::ClampMax(mp_input_component->GetControlPad().m_x.GetPressedTime() / s_get_param(CRCD(0x12333ebd, "hold_time_for_max_jump")), 1.0f)
			);
			break;
			
		case WALKING_HANG:
		case WALKING_LADDER:
		case WALKING_ANIMWAIT:
			strength = s_get_param(CRCD(0xf2fa5845, "hang_jump_factor"));
			break;
	}
	
	// if we're jumping from the ground, trip the ground's triggers
	if (m_state == WALKING_GROUND && m_last_ground_feeler_valid)
	{
		mp_trigger_component->CheckFeelerForTrigger(TRIGGER_JUMP_OFF, m_last_ground_feeler);
		if (mp_physics_control_component->HaveBeenReset()) return;
	}
	
	// Called by script from outside of the component update, so m_vertical_vel is not used.
	GetObject()->GetVel()[Y] = strength * s_get_param(CRCD(0x63d62a21, "jump_velocity"));
	
	// jumps for ladders and hanging get a backwards velocity
	switch (m_state)
	{
		case WALKING_GROUND:
		case WALKING_AIR:
		{
//			extract_state_from_object();
	
			if (m_control_magnitude)
			{
				float min_launch_speed = calculate_desired_speed()
					* s_get_param(CRCD(0x839fe542, "jump_horiz_speed")) / get_run_speed();
				
				if (Mth::DotProduct(m_horizontal_vel, m_control_direction) > 0.0f)
				{
					m_horizontal_vel.ProjectToNormal(m_control_direction);
					if (m_horizontal_vel.Length() < min_launch_speed)
					{
						m_horizontal_vel.Normalize(min_launch_speed);
					}
				}
				else
				{
					m_horizontal_vel = min_launch_speed * m_control_direction;
				}
				
				m_primary_air_direction = m_control_direction;
				m_facing = m_control_direction;
			}
			else
			{
				m_primary_air_direction = m_facing;
			}
			
//			adjust_jump_for_ceiling_obstructions();
			
			// setup primary air direction
			float length_sqr = m_horizontal_vel.LengthSqr();
			if (length_sqr > 0.01f)
			{
				m_primary_air_direction = m_horizontal_vel;
				m_primary_air_direction /= sqrtf(length_sqr);
			}
			else
			{
				m_primary_air_direction = m_facing;
			}
			
			copy_state_into_object();
			
			break;
		}
		
		case WALKING_ANIMWAIT:
			m_false_wall.active = true;
			m_false_wall.distance = Mth::DotProduct(m_false_wall.normal, m_pos + m_critical_point_offset);
			
			GetObject()->GetVel()[X] = 0.0f;
			GetObject()->GetVel()[Z] = 0.0f;
			m_primary_air_direction = m_facing;
			break;
			
		case WALKING_HANG:
			m_false_wall.active = true;
			m_false_wall.normal = m_facing;
			m_false_wall.distance = Mth::DotProduct(m_false_wall.normal, m_pos + m_critical_point_offset);
			m_false_wall.cancel_height = m_pos[Y] - m_vertical_hang_offset;
			
			GetObject()->GetVel()[X] = 0.0f;
			GetObject()->GetVel()[Z] = 0.0f;
			m_primary_air_direction = m_facing;
			break;

		case WALKING_LADDER:
			GetObject()->GetVel()[X] = 0.0f;
			GetObject()->GetVel()[Z] = 0.0f;
			m_primary_air_direction = m_facing;
			break;
	}
	
	leave_movable_contact_for_air(GetObject()->GetVel(), GetObject()->GetVel()[Y]);
	
	set_state(WALKING_AIR);
	
	GetObject()->BroadcastEvent(CRCD(0x8687163a, "SkaterJump"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CWalkComponent::go_in_air_state (   )
{
	setup_collision_cache();
	
	// default air event
	m_frame_event = CRCD(0x439f4704, "Air");
	
	// user control of horizontal velocity
	control_horizontal_vel();
	
	// prevent movement into walls
	adjust_horizonal_vel_for_environment();
//	if (should_bail_from_frame()) return;
	
	// check for head bonking
	adjust_vertical_vel_for_ceiling();
	
	// apply movement and acceleration for this frame
	m_pos += m_horizontal_vel * m_frame_length;
	m_pos[Y] += m_vertical_vel * m_frame_length + 0.5f * -s_get_gunslinger_param(CRCD(0xa5e2da58, "gravity")) * Mth::Sqr(m_frame_length);
	
	m_vertical_vel += -s_get_gunslinger_param(CRCD(0xa5e2da58, "gravity")) * m_frame_length;
	
	// see if we've landed yet
	check_for_landing(m_frame_start_pos, m_pos);
	if (m_state != WALKING_AIR /*|| should_bail_from_frame()*/) return;
	
	// maybe grab a rail; delay regrabbing of hang rails
	if (mp_input_component->GetControlPad().m_R1.GetPressed() && !m_ignore_grab_button
		&& ((m_previous_state != WALKING_HANG && m_previous_state != WALKING_LADDER) || Tmr::ElapsedTime(m_state_timestamp) > s_get_gunslinger_param(CRCD(0xe6e0c0a4, "rehang_delay"))))
	{
		if (m_previous_state == WALKING_LADDER)
		{
			// can't regrab ladders
			if (maybe_grab_to_hang(m_frame_start_pos, m_pos))
			{
				CFeeler::sClearDefaultCache();
				return;
			}
		}
		else
		{
			if (maybe_grab_to_hang(m_frame_start_pos, m_pos) || maybe_grab_to_ladder(m_frame_start_pos, m_pos))
			{
				CFeeler::sClearDefaultCache();
				return;
			}
		}
	}
	
	if (mp_input_component->GetControlPad().m_triangle.GetPressed())
	{
		if (maybe_stick_to_rail())
		{
			CFeeler::sClearDefaultCache();
			return;
		}
	}
	
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

void CWalkComponent::control_horizontal_vel (   )
{
	// We allow user control over the object's in air velocity.  The algorithm is complicated by the fact that the forward velocity of the jump needs
	// to be accounted for when allowing for velocity adjustment.  It is assumed that the jump direction is the same as the facing.
	
	// remove uncontrollable velocity term
	m_horizontal_vel -= m_uncontrollable_air_horizontal_vel;
	
	// forced run still works in the air
	adjust_control_for_forced_run();
	
	// adjust speed by the script set drag factor
	float adjust_magnitude = m_control_magnitude;
	
	// adjust velocity perpendicular to jump direction
	
	// direction perpendicular to jump direction
	Mth::Vector perp_direction(-m_primary_air_direction[Z], 0.0f, m_primary_air_direction[X]);
	
	// desired perpendicular velocity
	float perp_desired_vel = s_get_gunslinger_param(CRCD(0x896c8888, "jump_adjust_speed")) * adjust_magnitude * Mth::DotProduct(m_control_direction, perp_direction);
	
	// current perpendicular velocity
	float perp_vel = Mth::DotProduct(m_horizontal_vel, perp_direction);
	
	// exponentially approach desired velocity
	perp_vel = Mth::Lerp(perp_vel, perp_desired_vel, s_get_gunslinger_param(CRCD(0xf085443b, "jump_accel_factor")) * m_frame_length);
		
	// adjust velocity parallel to jump direction
	
	// desired parallel velocity
	float para_desired_vel = s_get_gunslinger_param(CRCD(0x896c8888, "jump_adjust_speed")) * adjust_magnitude * Mth::DotProduct(m_control_direction, m_primary_air_direction);
	
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
		para_vel = Mth::Lerp(para_vel, para_desired_vel, s_get_gunslinger_param(CRCD(0xf085443b, "jump_accel_factor")) * m_frame_length);
	}
		
	// rebuild horizontal velocity from parallel and perpendicular components
	m_horizontal_vel = para_vel * m_primary_air_direction + perp_vel * perp_direction;
	
	// reinstitute uncontrollable velocity term
	m_horizontal_vel += m_uncontrollable_air_horizontal_vel;
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::adjust_vertical_vel_for_ceiling (   )
{
	// If we hit our head, zero vertical velocity
	
	// only worry about the ceiling if we're moving upwards
	if (m_vertical_vel <= 0.0f) return;
	
	// look for a collision up through the body to the head
	CFeeler feeler;
	feeler.m_start = m_pos;
	feeler.m_end = m_pos;
	feeler.m_end[Y] += s_get_gunslinger_param(CRCD(0x9ea1974a, "walker_height"));
	if (!feeler.GetCollision()) return;
	
	// zero upward velocity
	m_vertical_vel = 0.0f;
	
	GetObject()->SelfEvent(CRCD(0x6e84acf3, "HitCeiling"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CWalkComponent::check_for_landing ( const Mth::Vector& previous_pos, const Mth::Vector& final_pos )
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
	mp_trigger_component->CheckFeelerForTrigger(TRIGGER_LAND_ON, m_last_ground_feeler);
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

void CWalkComponent::uber_frig (   )
{
	// insure that we don't fall to the center of the earth, even if there are holes in the geometry; also, do lighting since we've got the feeler anyway
	
	CFeeler feeler;
	feeler.m_start = m_pos;
	feeler.m_start[Y] += 1.0f;
	feeler.m_end = m_pos;
	feeler.m_end[Y] -= FEET(400);
	
	if (feeler.GetCollision())
	{
		mp_model_component->ApplyLightingFromCollision(feeler);
		return;
	}
	
	MESSAGE("applying uber frig");
	
	// teleport us back to our position at the frame's start; not pretty, but this isn't supposed to be
	m_pos = m_frame_start_pos;
	
	// zero our velocity too
	m_horizontal_vel.Set();
	m_vertical_vel = 0.0f;
	
	// set our state to ground
	set_state(WALKING_GROUND);
			  	
	m_last_ground_feeler_valid = false;
	
	m_ground_normal.Set(0.0f, 1.0f, 0.0f);

	// reset our script state
	m_frame_event = CRCD(0x57ff2a27, "Land");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CWalkComponent::lerp_upright (   )										 
{
    if (m_upward[Y] == 1.0f) return;
	
	if (m_upward[Y] > 0.999f)
	{
		m_upward.Set(0.0f, 1.0f, 0.0f);
		return;
	}
	
	m_upward = Mth::Lerp(m_upward, Mth::Vector(0.0f, 1.0f, 0.0f), s_get_gunslinger_param(CRCD(0xf22c135, "lerp_upright_rate")) * Tmr::FrameLength());
	m_upward.Normalize();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::check_for_wall_push (   )
{
	// ensure we have a forward contact
	if (!mp_contacts[0].in_collision && !mp_contacts[1].in_collision && !mp_contacts[vNUM_FEELERS - 1].in_collision)
	{
		return m_wall_push_test.active = false;
	}
	
	// ensure that control is maxed out
	if (!m_control_pegged)
	{
		return m_wall_push_test.active = false;
	}
	
	if (!m_wall_push_test.active)
	{
		// if we're not testing, simply start the test
		m_wall_push_test.test_start_time = Tmr::GetTime();
		m_wall_push_test.active = true;
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::maybe_jump_low_barrier (   )
{
	// For each forward contact in collision, we check to see if any lacks a collision at the maximum jump height.  For any that do, we find the first height
	// at which they are in contact.  The lowest such height is used at the target jump height.
	
	const int p_forward_contact_idxs[] = { 0, 1, vNUM_FEELERS - 1 };
	
	// we use the lowest hang height as the maximum autojump barrier height
	float top_feeler_height = s_get_gunslinger_param(CRCD(0x2c942693, "lowest_hang_height"));
	float feeler_length = 2.0f * s_get_gunslinger_param(CRCD(0x99978d2b, "feeler_length"));
	float height_increment = (top_feeler_height - s_get_gunslinger_param(CRCD(0x6da7f696, "feeler_height"))) / vNUM_BARRIER_HEIGHT_FEELERS;
	
	CFeeler feeler;
	
	// setup collision cache
	Mth::CBBox bbox(
		m_pos - Mth::Vector(feeler_length + 1.0f, 0.0f, feeler_length + 1.0f),
		m_pos + Mth::Vector(feeler_length + 1.0f, top_feeler_height + 1.0f, feeler_length + 1.0f)
	);
	Nx::CCollCache* p_coll_cache = Nx::CCollCacheManager::sCreateCollCache();
	p_coll_cache->Update(bbox);
	feeler.SetCache(p_coll_cache);
	
	// loop over forward collisions and check to see if the barrier in each of their directions is jumpable
	bool jumpable = false;
	for (int i = 0; i < 3; i++)
	{
		int n = p_forward_contact_idxs[i];
		
		if (!mp_contacts[n].in_collision) continue;
		
		// first check to see if the collision normal is not too transverse to the control direction, making a autojump unlikely to succeed
		if (Mth::DotProduct(mp_contacts[n].normal, m_control_direction) > -cosf(Mth::DegToRad(s_get_gunslinger_param(CRCD(0x78e6a5ec, "barrier_jump_max_angle")))))
		{
			mp_contacts[n].jumpable = false;
			continue;
		}
		
		feeler.m_start = m_pos;
		feeler.m_start[Y] += top_feeler_height;
		feeler.m_end = feeler.m_start + feeler_length * calculate_feeler_offset_direction(n);
		
		mp_contacts[n].jumpable = !feeler.GetCollision();
		
        if (mp_contacts[n].jumpable)
		{
			jumpable = true;
			#ifdef __USER_DAN__
			if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
			{
				feeler.DebugLine(0, 0, 255, 0);
			}
			#endif
		}
		else
		{
			#ifdef __USER_DAN__
			if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
			{
				feeler.DebugLine(255, 0, 0, 0);
			}
			#endif
		}
	}
	
	// if the barrier is not jumpable
	if (!jumpable)
	{
		// no autojump
		Nx::CCollCacheManager::sDestroyCollCache(p_coll_cache);
		return false;
	}
	
	// loop over the jumpable collision directions
	float lowest_height = m_pos[Y] + top_feeler_height;
	for (int i = 0; i < 3; i++)
	{
		int n = p_forward_contact_idxs[i];
		
		if (!mp_contacts[n].in_collision) continue;
		if (!mp_contacts[n].jumpable) continue;
		
		feeler.m_start = m_pos;
		feeler.m_start[Y] += top_feeler_height;
		feeler.m_end = feeler.m_start + feeler_length * calculate_feeler_offset_direction(n);
		
		// look for the barrier height
		for (int h = vNUM_BARRIER_HEIGHT_FEELERS - 1; h--; )
		{
			feeler.m_start[Y] -= height_increment;
			feeler.m_end[Y] -= height_increment;
			if (feeler.GetCollision())
			{
				#ifdef __USER_DAN__
				if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
				{
					feeler.DebugLine(255, 0, 0, 0);
				}
				#endif
				break;
			}
			else
			{
				#ifdef __USER_DAN__
				if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
				{
					feeler.DebugLine(0, 255, 0, 0);
				}
				#endif
			}
		}
		
		// find the lowest barrier of the jumpable directions
		if (lowest_height > feeler.m_start[Y])
		{
			lowest_height = feeler.m_start[Y];
		}
	}
	
	Nx::CCollCacheManager::sDestroyCollCache(p_coll_cache);
	
	// caluclate the velocity required to clear the barrier
	float jump_height = lowest_height + height_increment + s_get_gunslinger_param(CRCD(0x72660978, "barrier_jump_min_clearance")) - m_pos[Y];
	float required_vertical_velocity = sqrtf(2.0f * s_get_gunslinger_param(CRCD(0xa5e2da58, "gravity")) * jump_height);
	
	// setup the new walking state
	
	m_vertical_vel = required_vertical_velocity;
	
	set_state(WALKING_AIR);
	
	m_primary_air_direction = m_facing;
	leave_movable_contact_for_air(m_horizontal_vel, m_vertical_vel);
	
	m_frame_event = CRCD(0x584cf9e9, "Jump");
	
	
	// stop late jumps after an autojump
	GetObject()->RemoveEventHandler(CRCD(0x6b9ca247, "JumpRequested"));
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::leave_movable_contact_for_air ( Mth::Vector& horizontal_vel, float& vertical_vel )
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

void CWalkComponent::calculate_anim_wait_facing_drift_parameters ( const Mth::Vector& goal_facing )
{
	float initial_angle = atan2f(m_facing[X], m_facing[Z]);
	float goal_angle = atan2f(goal_facing[X], goal_facing[Z]);
	
	m_drift_angle = goal_angle - initial_angle;
	if (Mth::Abs(m_drift_angle) > Mth::PI)
	{
		m_drift_angle -= Mth::Sgn(m_drift_angle) * (2.0f * Mth::PI);
	}
	
	m_drift_goal_facing = goal_facing;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::go_anim_wait_state (   )
{
	if (mp_movable_contact_component->UpdateContact(m_pos))
	{
		m_offset_due_to_movable_contact += mp_movable_contact_component->GetContact()->GetMovement();
		if (mp_movable_contact_component->GetContact()->IsRotated())
		{
			m_drift_goal_facing = mp_movable_contact_component->GetContact()->GetRotation().Rotate(m_drift_goal_facing);
			if (m_drift_goal_facing[Y] != 0.0f)
			{
				m_drift_goal_facing[Y] = 0.0f;
				m_drift_goal_facing.Normalize();
			}
			
		}
	}
	
	float start, current, end;
	mp_animation_component->GetPrimaryAnimTimes(&start, &current, &end);
    float animation_completion_factor = Mth::LinearMap(0.0f, 1.0f, current, start, end);
	
	// smoothly drift the position
	m_pos = m_offset_due_to_movable_contact + Mth::Lerp(m_anim_wait_initial_pos, m_anim_wait_goal_pos, animation_completion_factor);
	
	float angle = Mth::Lerp(-m_drift_angle, 0.0f, animation_completion_factor);
	m_facing = m_drift_goal_facing;
	m_facing.RotateY(angle);
	
	m_frame_event = CRCD(0x4fe6069c, "AnimWait");
}
		
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::anim_wait_complete (   )
{
	GetObject()->SetPos(m_anim_wait_goal_pos + m_offset_due_to_movable_contact);
    
    Mth::Matrix matrix;
    matrix[Z] = m_drift_goal_facing;
    matrix[Y].Set(0.0f, 1.0f, 0.0f);
    matrix[X].Set(m_drift_goal_facing[Z], 0.0f, -m_drift_goal_facing[X]);
	GetObject()->SetMatrix(matrix);
	
    if (mp_anim_wait_complete_callback)
    {
        (this->*mp_anim_wait_complete_callback)();
    }
}
		
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::maybe_stick_to_rail (   )
{
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
	
void CWalkComponent::setup_collision_cache (   )
{
	float horizontal_reach = 1.0f + s_get_gunslinger_param(CRCD(0x99978d2b, "feeler_length"));
	float vertical_height = 1.0f + s_get_gunslinger_param(CRCD(0x9ea1974a, "walker_height"));;
	float vertical_depth = 1.0f + s_get_gunslinger_param(CRCD(0xaf3e4251, "snap_down_height"));
	
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
	
void CWalkComponent::copy_state_into_object (   )
{
	// build the object's matrix based on our facing
	Mth::Matrix matrix;
	
	// basically, rotate Z upward to perpendicular with m_upward
	matrix[X] = Mth::CrossProduct(m_upward, m_facing);
	matrix[X].Normalize();
	matrix[Y] = m_upward;
	matrix[Z] = Mth::CrossProduct(matrix[X], matrix[Y]);
	matrix[W].Set( 0.0f, 0.0f, 0.0f, 1.0f );
	
	GetObject()->SetPos( m_pos );
	GetObject()->SetMatrix( matrix );
	GetObject()->SetDisplayMatrix( matrix );
	
	// construct the object's velocity
	GetObject()->SetVel(m_horizontal_vel);
	GetObject()->GetVel()[Y] = m_vertical_vel;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CWalkComponent::get_controller_input (   )
{
	CControlPad& control_pad = mp_input_component->GetControlPad();
	
	Dbg_Assert(mp_camera);
	
	// rotate controller direction into camera's frame
	
	Mth::Vector camera_forward = -mp_camera->m_matrix[Z];
	camera_forward[Y] = 0.0f;
	camera_forward.Normalize();
	
	// allow a tolerance range for pressing directly forward
	float angle = control_pad.m_leftAngle;
	if (Mth::Abs(angle) < Mth::DegToRad(s_get_gunslinger_param(CRCD(0x4676a268, "forward_tolerance"))))
	{
		angle = 0.0f;
	}
	
	float sin_angle = sinf(angle);
	float cos_angle = cosf(angle);
	m_control_direction[X] = cos_angle * camera_forward[X] - sin_angle * camera_forward[Z];
	m_control_direction[Z] = sin_angle * camera_forward[X] + cos_angle * camera_forward[Z];
	
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
				
				Tmr::Time full_run_dpad_delay = static_cast< Tmr::Time >(s_get_gunslinger_param(CRCD(0x1832588c, "full_run_dpad_delay")));
				Tmr::Time start_run_dpad_delay = static_cast< Tmr::Time >(s_get_gunslinger_param(CRCD(0x2c386a43, "start_run_dpad_delay")));
				
				if (elapsed_time < start_run_dpad_delay)
				{
					m_control_magnitude = s_get_gunslinger_param(CRCD(0xc1528f7f, "walk_point"));
				}
				else if (elapsed_time < full_run_dpad_delay)
				{
					m_control_magnitude = Mth::SmoothMap(
						s_get_gunslinger_param(CRCD(0xc1528f7f, "walk_point")),
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
        if (m_state == WALKING_GROUND && Mth::Abs(angle) < Mth::DegToRad(90.0f + 5.0f) && (forced_run() || m_control_magnitude > s_get_gunslinger_param(CRCD(0xc1528f7f, "walk_point"))))
        {
			if (forced_run() || m_control_magnitude == 1.0f)
			{
				m_control_direction += s_get_gunslinger_param(CRCD(0x3c581621, "dpad_control_damping_factor")) * camera_forward;
			}
			else
			{
				// smoothly interpolate between damping and no damping
				m_control_direction += Mth::SmoothMap(0.0f, s_get_gunslinger_param(CRCD(0x3c581621, "dpad_control_damping_factor")), m_control_magnitude, s_get_gunslinger_param(CRCD(0xc1528f7f, "walk_point")), 1.0f)
					* camera_forward;
			}
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

void CWalkComponent::adjust_control_for_forced_run (   )
{
	if (!forced_run()) return;
	
	// if no direction is pressed
	if (m_control_magnitude == 0.0f)
	{
		// run in the direction of our current facing
		m_control_direction = m_facing;
	}

	// run full speed
	m_control_magnitude = 1.0f;
	m_control_pegged = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CWalkComponent::calculate_desired_speed (   )
{
	// forced run
	adjust_control_for_forced_run();
	
	if (m_control_magnitude == 0.0f) return 0.0f;
	
	float walk_point = s_get_gunslinger_param(CRCD(0xc1528f7f, "walk_point"));
	if (m_control_magnitude <= walk_point)
	{
		m_run_toggle = false;
		return Mth::LinearMap(0.0f, s_get_gunslinger_param(CRCD(0x79d182ad, "walk_speed")), m_control_magnitude, 0.3f, walk_point);
	}
	else
	{
		m_run_toggle = true;
		return Mth::LinearMap(s_get_gunslinger_param(CRCD(0x79d182ad, "walk_speed")), s_get_gunslinger_param(CRCD(0xcc461b87, "run_speed")), m_control_magnitude, walk_point, 1.0f);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector CWalkComponent::calculate_feeler_offset_direction ( int contact )
{
	float angle = contact * (2.0f * Mth::PI / vNUM_FEELERS);
	float cos_angle = cosf(angle);
	float sin_angle = sinf(angle);

	Mth::Vector end_offset_direction;
	end_offset_direction[X] = cos_angle * m_facing[X] - sin_angle * m_facing[Z];
	end_offset_direction[Y] = 0.0f;
	end_offset_direction[Z] = sin_angle * m_facing[X] + cos_angle * m_facing[Z];
	end_offset_direction[W] = 1.0f;
	
	return end_offset_direction;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::determine_stand_pos ( const Mth::Vector& proposed_stand_pos, Mth::Vector& stand_pos, CFeeler& feeler )
{
	// upward offset of standing position at maximum standable slope
	float max_height_adjustment = s_get_gunslinger_param(CRCD(0x6da7f696, "feeler_height")) / s_get_gunslinger_param(CRCD(0xa20c43b7, "push_feeler_length")) * s_get_gunslinger_param(CRCD(0x21dfbe77, "pull_up_offset_forward"));
	
	feeler.m_start = proposed_stand_pos;
	feeler.m_start[Y] += max_height_adjustment;
	feeler.m_end = proposed_stand_pos;
	feeler.m_end[Y] -= max_height_adjustment;
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
	{
		feeler.DebugLine(255, 255, 0, 0);
	}
	#endif
	if (feeler.GetCollision())
	{
		stand_pos = feeler.GetPoint();
		return true;
	}
	
	stand_pos = proposed_stand_pos;
	return false;
}

}

