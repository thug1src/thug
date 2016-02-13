//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       WalkComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  4/2/3
//****************************************************************************

#ifdef TESTING_GUNSLINGER

// Replace the entire contents of this file with the new file.
#include <gel/components/gunslingerwalkcomponent.cpp>

#else

#include <gel/components/walkcomponent.h>
#include <gel/components/inputcomponent.h>
#include <gel/components/animationcomponent.h>
#include <gel/components/walkcameracomponent.h>
#include <gel/components/modelcomponent.h>
#include <gel/components/triggercomponent.h>
#include <gel/components/movablecontactcomponent.h>
#include <gel/components/railmanagercomponent.h>
#include <gel/components/trickcomponent.h>
#include <gel/components/shadowcomponent.h>
#include <gel/components/modelcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/collision/collcache.h>

#include <sk/objects/rail.h>
#include <sk/objects/skatercareer.h>
#include <sk/engine/feeler.h>
#include <sk/modules/skate/skate.h>
#include <sk/gamenet/gamenet.h>
#include <sk/components/skaterphysicscontrolcomponent.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/components/skaterstatecomponent.h>
#include <sk/components/skaterscorecomponent.h>
	
/*
 * - Camera needs to initialize correctly at restarts.
 * - Faster camera slerp, smaller no-slerp angle during run out.
 * - Retain momentum better when leaving skating.
 * - Accidental manuals when interacting with ledges and ladders.
 * - Catching on curbs right after jump (use snap up code from skater)
 * - Hang
 *    - Jerky rail corners.
 *    - Turn around on wire hangs.
 *    - Drift to next rung effect in hanging too.
 *    - Special pull up to wire animation.
 * - Ladder
 *    - Drift between rungs.
 *    - Match rungs when getting on from the top.
 *    - Match rungs when grabbing from air.
 * - Hang from bottom of ladder.
 * - Ladder to rail across top.  Rail across top to ladder.
 * - Hand-plants, drop-ins, etc.
 * BUGS:
 * - Fall through verts with low frame rate.
 * - Pop in transition from last to first frames of slow cycling (walk/climb) animations.
 */
 
extern bool g_CheatsEnabled;

namespace Obj
{
	const uint32 CWalkComponent::sp_state_names [ CWalkComponent::NUM_WALKING_STATES ] =
	{
		CRCC(0x8cf3cb28, "WALKING_GROUND"),
		CRCC(0x9d1f1a2c, "WALKING_AIR"),
		CRCC(0x74ffc46d, "WALKING_HANG"),
		CRCC(0x1cb1f465, "WALKING_LADDER"),
		CRCC(0x79be4637, "WALKING_ANIMWAIT")
	};
	
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
	mp_model_component = NULL;
	mp_trigger_component = NULL;
	mp_physics_control_component = NULL;
	mp_movable_contact_component = NULL;
	mp_state_component = NULL;
	mp_core_physics_component = NULL;
	
	m_control_direction.Set();
	m_in_air_drift_vel.Set();
	m_rotate_upright_timer = 0.0f;
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
	mp_state_component = GetSkaterStateComponentFromObject(GetObject());
	mp_core_physics_component = GetSkaterCorePhysicsComponentFromObject(GetObject());
	
	Dbg_Assert(mp_input_component);
	Dbg_Assert(mp_animation_component);
	Dbg_Assert(mp_model_component);
	Dbg_Assert(mp_trigger_component);
	Dbg_Assert(mp_physics_control_component);
	Dbg_Assert(mp_movable_contact_component);
	Dbg_Assert(mp_state_component);
	Dbg_Assert(mp_core_physics_component);
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
	
	#ifdef SCRIPT_WALK_DRAG
	m_script_drag_factor = 1.0f;
	#endif
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
	uint32 previous_frame_event = m_frame_event;
	
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
	extract_state_from_object();
	
	m_frame_start_pos = m_pos;
	
	// set the frame length
	m_frame_length = Tmr::FrameLength();
	
	// go to our true Y position
	m_curb_float_height_adjusted = false;
	m_pos[Y] -= m_curb_float_height;
	DUMP_WPOSITION
	
	// lerp down to standard run speed if we're not in a combo
	update_run_speed_factor();
	
	// switch logic based on walking state
	switch (m_state)
	{
		case WALKING_GROUND:
			go_on_ground_state();
			break;

		case WALKING_AIR:
			go_in_air_state();
			break;
																	  
		// case WALKING_HOP:
			// go_hop_state();
			// break;
																	  
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
	if (mp_physics_control_component->HaveBeenReset()) return;
	
	// the there's no curb to adjust due to, lerp down to zero
	if (!m_curb_float_height_adjusted)
	{
		m_curb_float_height = Mth::Lerp(m_curb_float_height, 0.0f, s_get_param(CRCD(0x9b3388fa, "curb_float_lerp_down_rate")) * m_frame_length);
	}
	
	// adjust back to our curb float Y position
	m_pos[Y] += m_curb_float_height;
	DUMP_WPOSITION
	
	// maybe transition into skating via an acid drop from the ground
	if (m_state == WALKING_GROUND)
	{
		maybe_jump_to_acid_drop();
		if (mp_physics_control_component->HaveBeenReset()) return;
	}
	
	// adjust critical point offset
	update_critical_point_offset();
	
	// adjust the display offset
	update_display_offset();
	
	// keep the object from falling through holes in the geometry
	if (m_state == WALKING_GROUND || m_state == WALKING_AIR)
	{
		uber_frig();
	}
	
	// rotate to upright -- non-transition
	lerp_upright();
	
	// setup the object based on this frame's walking
	copy_state_into_object();
	
	// rotate to upright -- transition
	transition_lerp_upright();
	
	// smooth out the anim speed over two frames to minimize poppy behavior
	smooth_anim_speed(previous_frame_event);
	
	Dbg_Assert(m_frame_event);
	GetObject()->SelfEvent(m_frame_event);
	
	// set the animation speeds
	update_anim_speeds();
	
	// camera controls
	set_camera_overrides();
	
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
	{
		Gfx::AddDebugStar(GetObject()->GetPos(), 36.0f, MAKE_RGB(255, 255, 255), 1);
		if (m_critical_point_offset.LengthSqr() != 0.0f)
		{
			Gfx::AddDebugStar(GetObject()->GetPos() + m_critical_point_offset, 36.0f, MAKE_RGB(150, 255, 100), 1);
		}
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
			
		// @script | Walk_AnimWait |
		case CRCC(0x8fe2b013, "Walk_AnimWait"):
			return m_state == WALKING_ANIMWAIT ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
		// @script | Walk_GetStateTime | Loads the time in milliseconds since last state change.
		case CRCC(0xce64576c, "Walk_GetStateTime"):
			pScript->GetParams()->AddInteger(CRCD(0x5ab23cc9, "StateTime"), Tmr::ElapsedTime(m_state_timestamp));
			break;
		
		// @script | Walk_Jump |
		case CRCC(0x83e4bd70, "Walk_Jump"):
			jump();
			break;
		
		#ifdef SCRIPT_WALK_DRAG
		// @script | Walk_SetDragFactor |
		case CRCC(0xc6100a7d, "Walk_SetDragFactor"):
			pParams->GetFloat(NO_NAME, &m_script_drag_factor);
			break;
			
		case CRCC(0x4e4fae43, "Walk_ResetDragFactor"):
			m_script_drag_factor = 1.0f;
			break;
		#endif
			
		case CRCC(0xaf04b983, "Walk_GetSpeedScale"):
		{
			uint32 checksum;
			if (m_anim_effective_speed < s_get_param(CRCD(0xf3649996, "max_slow_walk_speed")))
			{
				checksum = CRCD(0x1150cabb, "WALK_SLOW");
			}
			else if (m_anim_effective_speed < s_get_param(CRCD(0x6a5805d8, "max_fast_walk_speed")))
			{
				checksum = CRCD(0x131f2a2, "WALK_FAST");
			}
			else if (m_anim_effective_speed < s_get_param(CRCD(0x1c94cc9c, "max_slow_run_speed")))
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
				
				// force a server anim speed update
				m_last_ladder_anim_speed = -1.0f;
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
			
		// @script | Walk_GetStateDuration | Returns the duration of the current state in StateDuration
		case CRCC(0x55bf1cd6, "Walk_GetStateDuration"):
			pScript->GetParams()->AddFloat(CRCD(0x4b88e4e4, "StateDuration"), (1.0f / 1000.0f) * Tmr::ElapsedTime(m_state_timestamp));
			break;
			
		// @script | Walk_GetPreviousState | Returns the previous state in PreviousState
		case CRCC(0xe9b1cde8, "Walk_GetPreviousState"):
		{
			pScript->GetParams()->AddChecksum(CRCD(0xf78635da, "PreviousState"), sp_state_names[m_previous_state]);
			break;
		}
			
		// @script | Walk_GetPreviousState | Returns the current state in State
		case CRCC(0xac7b36c8, "Walk_GetState"):
		{
			pScript->GetParams()->AddChecksum(CRCD(0x5c6c2d04, "State"), sp_state_names[m_state]);
			break;
		}
			
		// @script | Walk_GetHangAngle | Returns the current angle of the hang rail
		case CRCC(0x7e01b923, "Walk_GetHangAngle"):
		{
			Dbg_MsgAssert(mp_physics_control_component->IsWalking(), ("Called Walk_GetHangAngle when not in walk mode"));
			Dbg_MsgAssert(m_state == WALKING_HANG, ("Called Walk_GetHangAngle when not hanging"));
			
			Mth::Vector rail_direction = (mp_rail_manager->GetPos(mp_rail_end) - mp_rail_manager->GetPos(mp_rail_start)).Normalize();
			float angle = asinf(rail_direction[Y]);
			if (Mth::CrossProduct(rail_direction, m_facing)[Y] < 0.0f)
			{
				angle = -angle;
			}
			pScript->GetParams()->AddFloat(CRCD(0xead7d286, "HangAngle"), Mth::RadToDeg(angle));
			break;
		}

		/*
		case CRCC(0x14e4985b, "Walk_CancelTransitionalMomentum"):
			m_cancel_transition_momentum = true;
			break;
		*/
		
		/*
		// @script | Walk_SuppressInAirControl | turn off in air velocity control for some number of seconds
		case CRCC(0x30a9de5c, "Walk_SuppressInAirControl"):
			pParams->GetFloat(NO_NAME, &m_in_air_control_suppression_timer, Script::ASSERT);
			break;
		*/

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
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CWalkComponent::GetDebugInfo"));
	
	switch (m_state)
	{
		case WALKING_GROUND:
			p_info->AddChecksum(CRCD(0x109b9260, "m_state"), CRCD(0x58007c97, "GROUND"));
			break;
		case WALKING_AIR:
			p_info->AddChecksum(CRCD(0x109b9260, "m_state"), CRCD(0x439f4704, "AIR"));
			break;
		// case WALKING_HOP:
			// p_info->AddChecksum(CRCD(0x109b9260, "m_state"), CRCD(0xf41aba21, "HOP"));
			// break;										 
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
#endif				 
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
	
    // always reset the state timestamp
    m_state_timestamp = Tmr::GetTime();
	
	if (GetObject()->GetMatrix()[Y][Y] > 0.999f)
	{
		m_rotate_upright_timer = 0.0f;
	}
	else
	{
		// if we're not upright, get ready to rotate to upright
		if (GetObject()->GetMatrix()[Y][Y] > -0.999f)
		{
			m_rotate_upright_axis.Set(-GetObject()->GetMatrix()[Y][Z], 0.0f, GetObject()->GetMatrix()[Y][X]);
			m_rotate_upright_angle = acosf(Mth::Clamp(GetObject()->GetMatrix()[Y][Y], -1.0f, 1.0f));
		}
		else
		{
			m_rotate_upright_axis = GetObject()->GetMatrix()[X];
			m_rotate_upright_angle = Mth::PI;
		}
        
		m_rotate_upright_timer = s_get_param(CRCD(0xb0675803, "rotate_upright_duration"));
		
		if (GetObject()->GetMatrix()[Y][Y] < 0.0f)
		{
			GetObject()->SetPos(GetObject()->GetPos() + 6.0f * GetObject()->GetMatrix()[Y]);
			to_ground_state = false;
		}
	}

	if (to_ground_state)
	{
		set_state(WALKING_GROUND);
		
		// will be incorrect for one frame
		m_ground_normal.Set(0.0f, 1.0f, 0.0);
		
		m_last_ground_feeler_valid = false;
		
		GetObject()->GetVel()[Y] = 0.0f;
		
		m_disallow_acid_drops = false;

		/*
		if (GetObject()->GetVel().LengthSqr() > Mth::Sqr(450.0f))
		{
			m_cancel_transition_momentum = false;
		}
		else
		{
			m_cancel_transition_momentum = true;
		}
		*/
	}
	else
	{
		set_state(WALKING_AIR);
		
		// give a slight velocity boost when transitioning from vert air
		if (mp_core_physics_component->GetFlag(VERT_AIR) && m_rotate_upright_timer != 0.0f)
		{
			Mth::Vector target_facing = GetObject()->GetMatrix()[Z];
			target_facing.Rotate(m_rotate_upright_axis, m_rotate_upright_angle);
			GetObject()->GetVel() += s_get_param(CRCD(0x17b37748, "initial_vert_vel_boost")) * target_facing;
		}
		
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
		
		m_in_air_control_suppression_timer = 0.0f;
		
		m_false_wall.active = false;
		
		// you have to touch the ground at least once before acid dropping out of walking
		m_disallow_acid_drops = mp_core_physics_component->GetFlag(VERT_AIR)
			|| mp_core_physics_component->GetFlag(AIR_ACID_DROP_DISALLOWED);
		
		// m_cancel_transition_momentum = true;
	}

	m_curb_float_height = 0.0f;
	
	m_special_transition_data.type = NO_SPECIAL_TRANSITION;
	
	m_last_frame_event = 0;
	
	// TEMP: debounce R1 after a transition
	m_ignore_grab_button = true;
	
	m_critical_point_offset.Set();
	
	m_display_offset = 0.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::CleanUpWalkState (   )
{
	mp_model_component->SetDisplayOffset(Mth::Vector(0.0f, 0.0f, 0.0f));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::CollideWithOtherSkaterLost ( CCompositeObject* p_other_skater )
{
	set_state(WALKING_AIR);
	m_primary_air_direction = GetObject()->GetVel();
	m_primary_air_direction[Y] = 0.0f;
	m_primary_air_direction.Normalize();
	
	leave_movable_contact_for_air(GetObject()->GetVel(), GetObject()->GetVel()[Y]);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::set_state ( EStateType state )
{
    if (state == m_state) return;
	
	m_previous_state = m_state;
	m_state = state;
	m_state_timestamp = Tmr::GetTime();
	
    if (m_previous_state == WALKING_GROUND || m_previous_state == WALKING_AIR)
    {
        m_wall_push_test.active = false;
		m_run_toggle = false;
    }
	
	if (m_previous_state == WALKING_AIR)
	{
		m_in_air_control_suppression_timer = 0.0f;
	}

	if (m_previous_state == WALKING_AIR)
	{
		m_in_air_drift_vel.Set();
		
		m_disallow_acid_drops = false;
	}
	
	if (m_state == WALKING_GROUND)
	{
		m_control_pegged_duration = 10000.0f;
		m_last_frame_controller_pegged = true;
	}
	
	if (m_state == WALKING_ANIMWAIT)
	{
		m_offset_due_to_movable_contact.Set();
	}
	
	if (m_state != WALKING_AIR)
	{
		m_false_wall.active = false;
	}
	
	if (m_state == WALKING_GROUND || m_previous_state != WALKING_AIR)
	{
		m_drop_rotation_rate = false;
	}
	
	if (m_state == WALKING_HANG)
	{
		m_hang_move_vel = 0.0f;
	}
	
	if (m_state == WALKING_HANG && m_previous_state == WALKING_AIR)
	{
		CControlPad& control_pad = mp_input_component->GetControlPad();
		control_pad.DebounceLeftAnalogUp(s_get_param(CRCD(0xac96d24b, "hang_control_debounce_time")));
		control_pad.DebounceLeftAnalogDown(s_get_param(CRCD(0xac96d24b, "hang_control_debounce_time")));
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::go_on_ground_state (   )
{
	account_for_movable_contact();
	
	setup_collision_cache();
	
	// calculate initial horizontal speed
	float horizontal_speed = m_horizontal_vel.Length();
	
	calculate_horizontal_speed_and_facing(horizontal_speed);
	
	// only skid if you've had the controller pegged for a minimum duration
	if (m_frame_event == CRCD(0x1d537eff, "Skid") && m_control_pegged_duration < s_get_param(CRCD(0x8e1cf27a, "pegged_duration_for_skid")))
	{
		m_frame_event = CRCD(0x9b46e749, "Stand");
	}
	
	// calculate this frame's movement
	m_horizontal_vel = horizontal_speed * m_facing;
	
	// prevent movement into walls
	if (adjust_horizonal_vel_for_environment())
	{
		// turn to face newly adjusted velocity
		adjust_facing_for_adjusted_horizontal_vel();
	}
	else
	{
		m_drop_rotation_rate = false;
	}
	
	// if we are wall pushing, we may have decided to switch states during adjust_horizonal_vel_for_environment based on our environment
	if (m_state != WALKING_GROUND || mp_physics_control_component->HaveBeenReset())
	{
		CFeeler::sClearDefaultCache();
		return;
	}
	
	// apply movement for this frame
	m_pos += m_horizontal_vel * m_frame_length;
	DUMP_WPOSITION
	
	// snap up and down curbs and perhaps switch to air
	respond_to_ground();
	if (m_state != WALKING_GROUND || mp_physics_control_component->HaveBeenReset())
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
			m_facing = m_horizontal_vel / sqrtf(speed_sqr);
		}
	}
	
	CFeeler::sClearDefaultCache();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::calculate_horizontal_speed_and_facing ( float &horizontal_speed )
{
	if (m_control_pegged)
	{
		if (!m_last_frame_controller_pegged)
		{
			m_control_pegged_duration = 0.0f;
		}
		m_control_pegged_duration += m_frame_length;
	}
	m_last_frame_controller_pegged = m_control_pegged;
	
	// calculate user's desired speed
	float desired_speed = calculate_desired_speed();
	
	#ifdef SCRIPT_WALK_DRAG
	// adjust speed by the script set drag factor
	desired_speed *= m_script_drag_factor;
	#endif
			   	
	// setup frame's event
	if (desired_speed <= 0.0f)
	{
		m_frame_event = CRCD(0x9b46e749, "Stand");
	}
	else
	{
		m_frame_event = CRCD(0xaf895b3f, "Run");
	}

	bool special_acceleration = false;
	
	// adjust facing based on input
	
	if (m_control_magnitude > 0.0f)
	{
		float dot = Mth::DotProduct(m_facing, m_control_direction);
		
		if ((horizontal_speed < s_get_param(CRCD(0x52582d5b, "max_rotate_in_place_speed")) && dot < cosf(Mth::DegToRad(s_get_param(CRCD(0x5dff96a4, "max_rotate_in_place_angle")))))
			|| (horizontal_speed < s_get_param(CRCD(0xf1e97e45, "min_skid_speed")) && dot < -cosf(Mth::DegToRad(s_get_param(CRCD(0x2d571c0f, "max_reverse_angle"))))))
		{
			// low speed rotate to desired orientation with no speed change
			
			float delta_angle = Mth::DegToRad(s_get_param(CRCD(0xb557804b, "rotate_in_place_rate"))) * m_control_magnitude * m_frame_length;
			bool left_turn = -m_facing[Z] * m_control_direction[X] + m_facing[X] * m_control_direction[Z] < 0.0f;
			
			if (!m_run_toggle || m_drop_rotation_rate)
			{
				delta_angle *= s_get_param(CRCD(0x7b446c98, "walk_rotate_factor"));
			}
			else
			{
				float cos_delta_angle = cosf(left_turn ? delta_angle : -delta_angle);
				float sin_delta_angle = sinf(left_turn ? delta_angle : -delta_angle);
				Mth::Vector new_facing;
				new_facing[X] = cos_delta_angle * m_facing[X] + sin_delta_angle * m_facing[Z];
				new_facing[Y] = 0.0f;
				new_facing[Z] = -sin_delta_angle * m_facing[X] + cos_delta_angle * m_facing[Z];
				
				// kludge to stop popping when running at a wall using the dpad
				CFeeler feeler;
				feeler.m_start = m_pos;
				feeler.m_start[Y] += s_get_param(CRCD(0x6da7f696, "feeler_height"));
				feeler.m_end = feeler.m_start + s_get_param(CRCD(0x99978d2b, "feeler_length")) * new_facing;
				if (feeler.GetCollision(false))
				{
					delta_angle *= s_get_param(CRCD(0x7b446c98, "walk_rotate_factor"));
				}
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
			if (dot > -cosf(Mth::DegToRad(s_get_param(CRCD(0x2d571c0f, "max_reverse_angle")))))
			{
				// if the turn angle is soft
				
				float worse_factor = Mth::Lerp(
					m_analog_control
						? s_get_param(CRCD(0xa26760f5, "worse_worse_turn_factor"))
						: s_get_param(CRCD(0xab9015bc, "dpad_worse_worse_turn_factor")),
					m_analog_control
						? s_get_param(CRCD(0x6cb2e5de, "worse_turn_factor"))
						: s_get_param(CRCD(0x911f29d7, "dpad_worse_turn_factor")),
					Mth::Clamp(2.0f * (1.0f - Mth::DotProduct(m_control_direction, m_facing)), 0.0f, 1.0f)
				);
				
				// below a speed threshold, scale up the turn rate
				float turn_factor;
				if (horizontal_speed < s_get_param(CRCD(0x27815f69, "max_pop_speed")))
				{
					// quick turn
					turn_factor = Mth::Lerp(
						s_get_param(CRCD(0xb278405d, "best_turn_factor")),
						worse_factor,
						horizontal_speed / s_get_param(CRCD(0x27815f69, "max_pop_speed"))
					);
				}
				else
				{
					// slower turn
					turn_factor = worse_factor;
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
				
				if (horizontal_speed > s_get_param(CRCD(0xf1e97e45, "min_skid_speed")))
				{
					// if moving fast enough to require a skidding stop
					special_acceleration = true;
					horizontal_speed -= s_get_param(CRCD(0x9661ed7, "skid_accel")) * m_frame_length;
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
		float decel_factor = s_get_param(CRCD(0xacfa4e0c, "decel_factor"));
		
		if (desired_speed == 0.0f)
		{
			if (horizontal_speed > s_get_param(CRCD(0x79d182ad, "walk_speed")))
			{
				m_frame_event = CRCD(0x1d537eff, "Skid");
			}
			else
			{
				if (m_last_frame_event == CRCD(0x1d537eff, "Skid") && horizontal_speed > s_get_param(CRCD(0x311d02b2, "stop_skidding_speed")))
				{
					m_frame_event = CRCD(0x1d537eff, "Skid");
				}
				else
				{
					decel_factor = s_get_param(CRCD(0xa2aa811, "low_speed_decel_factor"));
				}
			}
		}
		
		/* needs reworked, at the least
		if (mp_physics_control_component->GetStateSwitchTime() == m_state_timestamp
			&& Tmr::ElapsedTime(mp_physics_control_component->GetStateSwitchTime()) < 400.0f
			&& !m_cancel_transition_momentum
			&& desired_speed < s_get_param(CRCD(0x79d182ad, "walk_speed")))
		{
			decel_factor = 1.0f;
			if (horizontal_speed > Script::GetFloat("start_stand"))
			{
				m_frame_event = CRCD(0x1d537eff, "Skid");
			}
			else
			{
				m_frame_event = CRCD(0x9b46e749, "Stand");
			}
		}
		*/
		
		horizontal_speed = Mth::Lerp(horizontal_speed, desired_speed, decel_factor * m_frame_length);
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

bool  CWalkComponent::adjust_horizonal_vel_for_environment (   )
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
			
			feeler.m_start = m_pos + m_critical_point_offset;
			feeler.m_end = feeler.m_start + m_horizontal_vel * m_frame_length;
			feeler.m_end[Y] += m_vertical_vel * m_frame_length + 0.5f * -get_gravity() * Mth::Sqr(m_frame_length);
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
		
		if (n == vNUM_FEELERS)
		{
			// feet collisions only count for walls
			if (feeler.GetNormal()[Y] > 0.707f)
			{
				mp_contacts[n].in_collision = false;
				continue;
			}
		}
		
		// store the feeler
		mp_contacts[n].feeler = feeler;
		
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
			
			/* no auto-hop-to-hang
			if (Tmr::ElapsedTime(m_wall_push_test.test_start_time) > s_get_param(CRCD(0x928e6775, "hop_delay")))
			{
				if (maybe_climb_up_ladder() || maybe_jump_low_barrier()) return false;
			}
			else
			*/
			if (Tmr::ElapsedTime(m_wall_push_test.test_start_time) > s_get_param(CRCD(0x38d36700, "barrier_jump_delay")))
			{
				if (maybe_climb_up_ladder() || maybe_jump_low_barrier()) return false;
			}
		}
		
		if (mp_input_component->GetControlPad().m_R1.GetPressed() && !m_ignore_grab_button)
		{
			if (maybe_climb_up_ladder(true)/* || maybe_hop_to_hang()*/) return false;
		}
		
		if (mp_physics_control_component->HaveBeenReset()) return false;
	}
	
	// push down steep slopes
	if (m_state == WALKING_GROUND && m_ground_normal[Y] < 0.5f)
	{
		Mth::Vector push_dir = m_ground_normal;
		push_dir[Y] = 0.0f;
		push_dir.Normalize();
		m_pos += s_get_param(CRCD(0x4d16f37d, "push_strength")) * m_frame_length * push_dir;
		DUMP_WPOSITION
	}
	
	if (!contact) return false;
	
	// push away from walls
	for (int n = 0; n < vNUM_FEELERS + 1; n++)
	{
		if (!mp_contacts[n].in_collision) continue;
		                
		if (mp_contacts[n].feeler.GetDist() < s_get_param(CRCD(0xa20c43b7, "push_feeler_length")) / feeler_length)
		{
			m_pos += s_get_param(CRCD(0x4d16f37d, "push_strength")) * m_frame_length * mp_contacts[n].normal;
			DUMP_WPOSITION
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
				m_drop_rotation_rate = true;
				return true;
			}
		}
	}
	
	// direction of proposed movement
	Mth::Vector movement_direction = m_horizontal_vel;
	movement_direction.Normalize();
	
	Mth::Vector adjusted_vel = m_horizontal_vel;
	
	// loop over the contacts (from backward to forward)
	const int p_contact_idxs [ vNUM_FEELERS + 1 ] = { 7, 4, 3, 5, 2, 6, 1, 0 };
	for (int i = 0; i < vNUM_FEELERS + 1; i++)
	{
		int n = p_contact_idxs[i];
		
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
			m_drop_rotation_rate = true;
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
	
	m_drop_rotation_rate = adjusted_vel.LengthSqr() / m_horizontal_vel.LengthSqr() < Mth::Sqr(0.5f);
	
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
	
void CWalkComponent::adjust_facing_for_adjusted_horizontal_vel (   )
{
	// We adjust facing due to adjustment in horizontal velocity due to environment.  Basically, we want to object to turn to face the velocity
	// that the environment has forced upon it.
	
	// IDEA: shift to basing turn amount on angle difference and not speed
	
	float horizontal_speed = m_horizontal_vel.Length();
	
	if (horizontal_speed < s_get_param(CRCD(0x515a933, "wall_turn_speed_threshold"))) return;
	
	// the new facing is in the direction of our adjusted velocity
	Mth::Vector new_facing = m_horizontal_vel;
	new_facing.Normalize();
	
	// smoothly transition between no wall turning to full wall turning
	float turn_ratio;
	if (horizontal_speed > s_get_param(CRCD(0xe6c1cd0d, "max_wall_turn_speed_threshold")))
	{
		turn_ratio = s_get_param(CRCD(0x7a583b9b, "wall_turn_factor")) * m_frame_length;
	}
	else
	{
		turn_ratio = Mth::LinearMap(
			0.0f,
			s_get_param(CRCD(0x7a583b9b, "wall_turn_factor")) * m_frame_length,
			horizontal_speed,
			s_get_param(CRCD(0x0515a933, "wall_turn_speed_threshold")),
			s_get_param(CRCD(0xe6c1cd0d, "max_wall_turn_speed_threshold"))
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
	
void CWalkComponent::respond_to_ground (   )
{
	// Look for the ground below us.  If we find it, snap to it.  If not, go to air state.
	
	CFeeler feeler;
	feeler.m_start = m_pos;
	feeler.m_start[Y] += s_get_param(CRCD(0xcee3a3e1, "snap_up_height"));
	feeler.m_end = m_pos;
	feeler.m_end[Y] -= s_get_param(CRCD(0xaf3e4251, "snap_down_height"));
	
	if (!feeler.GetCollision() || (feeler.GetFlags() & mFD_NOT_SKATABLE))
	{
		if (feeler.GetCollision())
		{
			m_pos = feeler.GetPoint() + 0.1f * feeler.GetNormal();
			DUMP_WPOSITION
		}
		
		// no ground
		
		if (m_last_ground_feeler_valid)
		{
			mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_OFF_EDGE, m_last_ground_feeler);
			if (mp_physics_control_component->HaveBeenReset()) return;
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
		GetObject()->BroadcastEvent(CRCD(0xd96f01f1, "SkaterOffEdge"));
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
	DUMP_WPOSITION
	
	// adjust stair float distance
	if (snap_distance > 0.0f || snap_distance < -3.0f)
	{
		m_curb_float_height = Mth::ClampMin(m_curb_float_height - snap_distance, 0.0f);
	}
	
	// see if we've changed sectors
	if (m_last_ground_feeler.GetSector() != feeler.GetSector())
	{
		if (m_last_ground_feeler_valid)
		{
			mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_OFF, m_last_ground_feeler);
			if (mp_physics_control_component->HaveBeenReset()) return;
		}
		mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_ONTO, feeler);
		if (mp_physics_control_component->HaveBeenReset()) return;
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

void CWalkComponent::account_for_movable_contact (   )
{
	if (!mp_movable_contact_component->UpdateContact(m_pos)) return;
	
	m_pos += mp_movable_contact_component->GetContact()->GetMovement();
	DUMP_WPOSITION
	
	if (mp_movable_contact_component->GetContact()->IsRotated())
	{
		m_facing = mp_movable_contact_component->GetContact()->GetRotation().Rotate(m_facing);
		if (m_facing[Y] != 0.0f)
		{
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
			extract_state_from_object();
	
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
			
			adjust_jump_for_ceiling_obstructions();
			
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
	
void CWalkComponent::adjust_jump_for_ceiling_obstructions (   )
{
	// check for obstructions above and drift backwards to avoid them if we can; this prevents us from banging our head on overhangs when we're up
	// against the wall and trying to jump to hang on them
	CFeeler feeler;
	feeler.m_start = m_pos;
	feeler.m_start[Y] += s_get_param(CRCD(0x9ea1974a, "walker_height"));
	feeler.m_end = feeler.m_start;
	feeler.m_end[Y] += s_get_param(CRCD(0x48c0ac2a, "jump_obstruction_check_height"));
	if (feeler.GetCollision(false))
	{
		float jump_obstruction_check_back = s_get_param(CRCD(0x6ebbab7, "jump_obstruction_check_back"));
		float obstruction_height = feeler.GetPoint()[Y] - feeler.m_start[Y];

		feeler.m_start -= m_facing * jump_obstruction_check_back;
		feeler.m_end -= m_facing * jump_obstruction_check_back;

		if (!feeler.GetCollision(false))
		{

			float acceleration = get_gravity();
			float vel_sqr_at_obstruction_height = Mth::Sqr(m_vertical_vel) - 2.0f * acceleration * obstruction_height;
			if (vel_sqr_at_obstruction_height > 0.0f)
			{
				float time_to_height = (m_vertical_vel - sqrt(vel_sqr_at_obstruction_height)) / acceleration;

				// setup in air drift
				m_in_air_drift_vel = -jump_obstruction_check_back / time_to_height * m_facing;
				m_in_air_drift_stop_height = m_pos[Y] + obstruction_height;

				// suppress control until we return to this height from above
				m_in_air_control_suppression_timer = 2.0f * m_vertical_vel / acceleration - time_to_height;
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CWalkComponent::go_in_air_state (   )
{
	CControlPad& control_pad = mp_input_component->GetControlPad();
	
	setup_collision_cache();
	
	// default air event
	m_frame_event = CRCD(0x439f4704, "Air");
	
	// user control of horizontal velocity
	control_horizontal_vel();
	
	// prevent movement into walls
	if (!adjust_horizonal_vel_for_environment())
	{
		m_drop_rotation_rate = false;
	}
	if (mp_physics_control_component->HaveBeenReset()) return;
	
	// account for the in-air false wall
	adjust_pos_for_false_wall();
	
	// check for head bonking
	adjust_vertical_vel_for_ceiling();
	
	// apply movement and acceleration for this frame
	float acceleration = get_gravity();
	m_pos += m_horizontal_vel * m_frame_length;
	m_pos += m_in_air_drift_vel * m_frame_length;
	m_pos[Y] += m_vertical_vel * m_frame_length + 0.5f * -acceleration * Mth::Sqr(m_frame_length);
	DUMP_WPOSITION
	m_vertical_vel += -acceleration * m_frame_length;
	
	if (m_pos[Y] > m_in_air_drift_stop_height)
	{
		m_in_air_drift_vel.Set();
	}
	
	// see if we've landed yet
	check_for_landing(m_frame_start_pos, m_pos);
	if (m_state != WALKING_AIR || mp_physics_control_component->HaveBeenReset()) return;
	
	// maybe grab a rail; delay regrabbing of hang rails
	if (control_pad.m_R1.GetPressed() && !m_ignore_grab_button
		&& ((m_previous_state != WALKING_HANG && m_previous_state != WALKING_LADDER && m_previous_state != WALKING_ANIMWAIT)
		|| Tmr::ElapsedTime(m_state_timestamp) > s_get_param(CRCD(0xe6e0c0a4, "rehang_delay"))))
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
	
	CFeeler::sClearDefaultCache();
	
	if (control_pad.m_triangle.GetPressed() && maybe_stick_to_rail()) return;
	
	if (maybe_in_air_acid_drop()) return;
	
	maybe_wallplant();
	if (mp_physics_control_component->HaveBeenReset()) return;
	
	// insure that we do not slip through the cracks in the collision geometry which are a side-effect of moving collidable objects
	Mth::Vector previous_pos = m_pos;
	Mth::Vector critical_point = m_pos + m_critical_point_offset;
	Mth::Vector frame_start_critical_point = m_frame_start_pos + m_critical_point_offset;
	if (CCompositeObject* p_inside_object = mp_movable_contact_component->CheckInsideObjects(critical_point, frame_start_critical_point))
	{
		m_pos = critical_point - m_critical_point_offset;
		DUMP_WPOSITION
		
		MESSAGE("WALKING_AIR, within moving object");
		
		m_horizontal_vel.Set();
		m_vertical_vel = 0.0f;
		check_for_landing(m_pos, previous_pos);
	}
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::control_horizontal_vel (   )
{
	// We allow user control over the object's in air velocity.  The algorithm is complicated by the fact that the forward velocity of the jump needs
	// to be accounted for when allowing for velocity adjustment.  It is assumed that the jump direction is the same as the facing.
	
	if (m_in_air_control_suppression_timer != 0.0f)
	{
		m_in_air_control_suppression_timer = Mth::ClampMin(m_in_air_control_suppression_timer - m_frame_length, 0.0f);
		return;
	}
	
	// remove uncontrollable velocity term
	m_horizontal_vel -= m_uncontrollable_air_horizontal_vel;
	
	#ifdef SCRIPT_WALK_DRAG
	// adjust speed by the script set drag factor
	float adjust_magnitude = m_control_magnitude * m_script_drag_factor;
	#else
	float adjust_magnitude = m_control_magnitude;
	#endif
	
	// adjust velocity perpendicular to jump direction
	
	// direction perpendicular to jump direction
	Mth::Vector perp_direction(-m_primary_air_direction[Z], 0.0f, m_primary_air_direction[X]);
	
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
		
		// exponentially approach desired velocity
		para_vel = Mth::Lerp(para_vel, para_desired_vel, s_get_param(CRCD(0xf085443b, "jump_accel_factor")) * m_frame_length);
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
	// if we hit our head, zero vertical velocity
	
	// no ceiling collisions when a false wall is active; otherwise, ceilings would kill our jump when we jump from hanging on a ledge which sticks out.
	if (m_false_wall.active) return;
	
	float walker_height = s_get_param(CRCD(0x9ea1974a, "walker_height"));
	
	// be more forgiving if we're moving down already
	if (m_vertical_vel < 0.0f)
	{
		walker_height -= 18.0f;
	}
	
	// look for a collision up through the body to the head
	CFeeler feeler;
	feeler.m_start = m_pos;
	feeler.m_end = m_pos;
	feeler.m_end[Y] += walker_height;
	if (!feeler.GetCollision()) return;
	
	// project velocity into plane
	m_horizontal_vel[Y] = m_vertical_vel;
	if (Mth::DotProduct(m_horizontal_vel, feeler.GetNormal()) < 0.0f)
	{
		m_horizontal_vel.ProjectToPlane(feeler.GetNormal());
	}
	m_vertical_vel = m_horizontal_vel[Y];
	m_horizontal_vel[Y] = 0.0f;
	
	// determine the displacement vector we must use to get out of the ceiling
	Mth::Vector displacement = feeler.GetPoint() - feeler.m_end;
	displacement.ProjectToNormal(feeler.GetNormal());
	
	// insure there are no collisions along this displacement
	feeler.m_end = m_pos + displacement;
	if (feeler.GetCollision())
	{
		m_pos = feeler.GetPoint() + feeler.GetNormal();
		DUMP_WPOSITION
	}
	else
	{
		m_pos = feeler.m_end;
		DUMP_WPOSITION
	}
	
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
	
	if (feeler.GetFlags() & mFD_NOT_SKATABLE)
	{
        m_pos = feeler.GetPoint() + 0.1f * feeler.GetNormal();
		DUMP_WPOSITION
		
		Mth::Vector remaining_movement = (1.0f - feeler.GetDist()) * (feeler.m_end - feeler.m_start);
		remaining_movement.ProjectToPlane(feeler.GetNormal());
		
		// slide along the surface, but stop at any new collision
		feeler.m_start = m_pos;
		feeler.m_end = m_pos + remaining_movement;
		if (feeler.GetCollision())
		{
			m_pos = feeler.GetPoint() + 0.05f * feeler.GetNormal();
			DUMP_WPOSITION
		}
		else
		{
			m_pos = feeler.m_end;
			DUMP_WPOSITION
		}
		return;
	}
	
	// snap to the collision point
	m_pos = feeler.GetPoint();
	DUMP_WPOSITION
	
	// zero vertical velocity
	m_vertical_vel = 0.0f;
	
	// change to ground state
	set_state(WALKING_GROUND);
	
	// stash the feeler
	m_last_ground_feeler = feeler;
	m_last_ground_feeler_valid = true;
	
	// trip any land trigger
	mp_trigger_component->CheckFeelerForTrigger(TRIGGER_LAND_ON, m_last_ground_feeler);
	if (mp_physics_control_component->HaveBeenReset()) return;
	
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
		
		if (m_control_magnitude == 0.0f && m_horizontal_vel.Length() < s_get_param(CRCD(0x530dcf34, "sticky_land_threshold_speed")))
		{
			m_horizontal_vel.Set();
		}
	}
	else
	{
		m_horizontal_vel.Set();
	}

	m_critical_point_offset.Set();

	// clear any jump requests
	mp_input_component->GetControlPad().m_x.ClearRelease();

	m_frame_event = CRCD(0x57ff2a27, "Land");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CWalkComponent::maybe_wallplant (   )
{
	if (!mp_input_component->GetControlPad().m_x.GetPressed()) return;
	
	// only wallplant on the way down
	if (m_vertical_vel > 0.0f) return;
					   	
	// not when you're too near the ground
	if (mp_state_component->m_height < Script::GetFloat(CRCD(0xd5349cc6, "Physics_Min_Wallplant_Height"))) return;

	// if (Tmr::ElapsedTime(m_state_timestamp) < s_get_param(CRCD(0x2a2d65c, "min_air_before_wallplant"))) return;
	
	// last wallplant must not have been too recently
	if (Tmr::ElapsedTime(mp_core_physics_component->m_last_wallplant_time_stamp) < Script::GetFloat(CRCD(0x82135dd7, "Physics_Disallow_Rewallplant_Duration"))) return;
	
	// no wallplants immediately after you enter air; stops wallplants during the late jump period
	if (Tmr::ElapsedTime(m_state_timestamp) < Script::GetFloat(CRCD(0x4c2b6df3, "Skater_Late_Jump_Slop"))) return;
	
	// identify the primary contact wall; not that we are ignoring the "feet feeler"
	int n;
	int contact_idx;
	const int p_contact_indxs[vNUM_FEELERS] = { 0, 1, 6, 2, 5, 3, 4 };
	for (n = 0; n < vNUM_FEELERS; n++)
	{
		if (mp_contacts[contact_idx = p_contact_indxs[n]].in_collision) break;
	}
	if (n == vNUM_FEELERS) return;
	
	/*
	// here we attempt to stop wallplant when in is more likely that the player is going for a grind or wants to jump up over the wall
	if (GetObject()->m_vel[Y] > 0.0f)
	{
		Mth::Vector wall_point = mp_contacts[contact_idx].feeler.GetPoint();
		Mth::Vector	wall_normal = mp_contacts[contact_idx].feeler.GetNormal();

		Mth::Vector wall_up_vel(0.0f, GetObject()->m_vel[Y] * 0.15f, 0.0f);		// check 0.15 seconds ahead
		wall_up_vel.RotateToPlane(wall_normal);  

		// check at what height will be in two frames
		wall_point += wall_up_vel;		

		CFeeler feeler(wall_point + wall_normal * 6.0f, wall_point - wall_normal * 6.0f);
		if (!feeler.GetCollision())
		{
			#ifdef __USER_DAN__
			if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
			{
				feeler.DebugLine(255, 255, 0, 0);
			}
			#endif
			return;
		}
		else
		{
			#ifdef __USER_DAN__
			if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
			{
				feeler.DebugLine(255, 0, 255, 0);
			}
			#endif
		}
	}
	*/
	
	// reflect the control direction off the wall and use it as our facing and boost direction
	Mth::Vector exit_dir;
	if (m_control_magnitude == 0.0f)
	{
		exit_dir = m_primary_air_direction;
	}
	else
	{
		exit_dir = m_control_direction;
	}
	float dot = Mth::DotProduct(exit_dir, mp_contacts[contact_idx].normal);
	if (dot < 0.0f)
	{
		exit_dir -= 2.0f * dot * mp_contacts[contact_idx].normal;
	}
	
	// set vertical velocity to the walking wallplant jump speed
	m_vertical_vel = s_get_param(CRCD(0x420d18bc, "vert_wall_jump_speed")) * Mth::Lerp(
		s_get_param(CRCD(0x246d0bf3, "min_jump_factor")), 
		1.0f,
		Mth::ClampMax(mp_input_component->GetControlPad().m_x.GetPressedTime() / s_get_param(CRCD(0x12333ebd, "hold_time_for_max_jump")), 1.0f)
	);
	
	// jump out from the wall
	m_horizontal_vel = s_get_param(CRCD(0x99510695, "horiz_wall_jump_speed")) * exit_dir;
	m_primary_air_direction = m_facing;

	// if the wall is moving, add on its velocity
	if (mp_contacts[contact_idx].feeler.IsMovableCollision())
	{
		Mth::Vector movable_contact_vel = mp_contacts[contact_idx].feeler.GetMovingObject()->GetVel();
		m_vertical_vel += movable_contact_vel[Y];
		m_horizontal_vel[X] += movable_contact_vel[X];
		m_horizontal_vel[Z] += movable_contact_vel[Z];
	}
	
	adjust_jump_for_ceiling_obstructions();
	
	// trip any triggers
	mp_trigger_component->CheckFeelerForTrigger(TRIGGER_JUMP_OFF, mp_contacts[contact_idx].feeler);
	if (mp_physics_control_component->HaveBeenReset()) return;
	
	// graffiti the object
	GetTrickComponentFromObject(GetObject())->TrickOffObject(mp_contacts[contact_idx].feeler.GetNodeChecksum());
	
	// time stamp the wallplant
	mp_core_physics_component->m_last_wallplant_time_stamp = Tmr::GetTime();
	
	m_frame_event = CRCD(0xcf74f6b7, "WallPlant");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::uber_frig (   )
{
	// insure that we don't fall to the center of the earth, even if there are holes in the geometry; also, do lighting since we've got the feeler anyway
	
	CFeeler feeler;
	feeler.m_start = m_pos + m_critical_point_offset;
	feeler.m_end = feeler.m_start;
	feeler.m_start[Y] += 1.0f;
	feeler.m_end[Y] -= FEET(400);
	
	if (feeler.GetCollision())
	{
		// set the height for script access
		mp_state_component->m_height = m_pos[Y] - feeler.GetPoint()[Y];
		
		mp_model_component->ApplyLightingFromCollision(feeler);
		
		// Store these values off for the simple shadow calculation.
		CShadowComponent* p_shadow_component = GetShadowComponentFromObject(GetObject());
		p_shadow_component->SetShadowPos(feeler.GetPoint());
		p_shadow_component->SetShadowNormal(feeler.GetNormal()); 
		
		return;
	}
	
	MESSAGE("applying uber frig");
	
	// teleport us back to our position at the frame's start; not pretty, but this isn't supposed to be
	m_pos = m_frame_start_pos;
	DUMP_WPOSITION
	
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
		m_rotate_upright_timer = 0.0f;
		return;
	}
	
	if (m_rotate_upright_timer == 0.0f)
	{
		m_upward = Mth::Lerp(m_upward, Mth::Vector(0.0f, 1.0f, 0.0f), s_get_param(CRCD(0xf22c135, "lerp_upright_rate")) * Tmr::FrameLength());
		m_upward.Normalize();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::transition_lerp_upright (   )
{
	if (m_rotate_upright_timer != 0.0f)
	{
		float lerp_factor = Mth::LinearMap(0.0f, 1.0f, m_frame_length, 0.0f, s_get_param(CRCD(0xb0675803, "rotate_upright_duration")));
		
		GetObject()->GetMatrix().Rotate(m_rotate_upright_axis, m_rotate_upright_angle * lerp_factor);
		GetObject()->SetDisplayMatrix(GetObject()->GetMatrix());
		
		m_rotate_upright_timer -= m_frame_length;
		m_rotate_upright_timer = Mth::ClampMin(m_rotate_upright_timer, 0.0f);
	}
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
	// check to see if we're in a bail
	if (mp_core_physics_component->GetFlag(IS_BAILING)) return false;
	
	// For each forward contact in collision, we check to see if any lacks a collision at the maximum jump height.  For any that do, we find the first height
	// at which they are in contact.  The lowest such height is used at the target jump height.
	
	const int p_forward_contact_idxs[] = { 0, 1, vNUM_FEELERS - 1 };
	
	// we use the lowest hang height as the maximum autojump barrier height
	float top_feeler_height = s_get_param(CRCD(0xda85f5ae, "barrier_jump_highest_barrier"));
	float feeler_length = 3.0f * s_get_param(CRCD(0x99978d2b, "feeler_length"));
	float height_increment = (top_feeler_height - s_get_param(CRCD(0x6da7f696, "feeler_height"))) / vNUM_BARRIER_HEIGHT_FEELERS;
	
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
		
		if (!mp_contacts[n].in_collision || (mp_contacts[n].feeler.GetFlags() & mFD_NOT_SKATABLE)) continue;
		
		// first check to see if the collision normal is not too transverse to the control direction, making a autojump unlikely to succeed
		if (Mth::DotProduct(mp_contacts[n].normal, m_control_direction) > -cosf(Mth::DegToRad(s_get_param(CRCD(0x78e6a5ec, "barrier_jump_max_angle")))))
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
	float jump_height = lowest_height + height_increment + s_get_param(CRCD(0x72660978, "barrier_jump_min_clearance")) - m_pos[Y];
	float required_vertical_velocity = sqrtf(2.0f * get_gravity() * jump_height);
	
	if (m_last_ground_feeler_valid)
	{
		mp_trigger_component->CheckFeelerForTrigger(TRIGGER_JUMP_OFF, m_last_ground_feeler);
		if (mp_physics_control_component->HaveBeenReset()) return true;
	}
	
	// setup the new walking state
	
	m_vertical_vel = required_vertical_velocity;
	
	set_state(WALKING_AIR);
	
	m_primary_air_direction = m_facing;
	leave_movable_contact_for_air(m_horizontal_vel, m_vertical_vel);
	
	m_frame_event = CRCD(0x584cf9e9, "Jump");
	
	GetObject()->BroadcastEvent(CRCD(0x8687163a, "SkaterJump"));
	
	// stop late jumps after an autojump
	GetObject()->RemoveEventHandler(CRCD(0x6b9ca247, "JumpRequested"));
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::maybe_in_air_acid_drop (   )
{
	if (m_disallow_acid_drops) return false;
	if (mp_physics_control_component->IsBoardMissing()) return false;
	
	CControlPad& control_pad = mp_input_component->GetControlPad();
	if (!WALK_SPINE_BUTTONS) return false;
	
	Mth::Vector vel(m_horizontal_vel[X], m_vertical_vel, m_horizontal_vel[Z]);
	if (!mp_core_physics_component->maybe_acid_drop(false, m_pos, m_frame_start_pos, vel, m_special_transition_data.acid_drop_data)) return false;
	
	m_horizontal_vel = vel;
	m_horizontal_vel[Y] = 0.0f;
	m_vertical_vel = vel[Y];
	m_special_transition_data.type = ACID_DROP_TRANSITION;
	m_frame_event = CRCD(0x2eda83ea, "AcidDrop");
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::maybe_jump_to_acid_drop (   )
{
	// we're on the ground; look ahead for acid drops to autojump into
	
	if (m_disallow_acid_drops) return false;
	if (mp_physics_control_component->IsBoardMissing()) return false;
	
	CControlPad& control_pad = mp_input_component->GetControlPad();
	if (!WALK_SPINE_BUTTONS) return false;
	
	Mth::Vector jump_vel = m_horizontal_vel;
	jump_vel[Y] = s_get_param(CRCD(0xc0491d2e, "acid_drop_jump_velocity"));
	
	if (!mp_core_physics_component->maybe_acid_drop(false, m_pos, m_frame_start_pos, jump_vel, m_special_transition_data.acid_drop_data)) return false;
	
	if (m_last_ground_feeler_valid)
	{
		mp_trigger_component->CheckFeelerForTrigger(TRIGGER_JUMP_OFF, m_last_ground_feeler);
		if (mp_physics_control_component->HaveBeenReset()) return true;
	}
	
	set_state(WALKING_AIR);
	
	m_horizontal_vel = jump_vel;
	m_horizontal_vel[Y] = 0.0f;
	m_vertical_vel = jump_vel[Y];
	m_special_transition_data.type = ACID_DROP_TRANSITION;
	m_frame_event = CRCD(0x2eda83ea, "AcidDrop");
	
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
			DUMP_WPOSITION
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

void CWalkComponent::calculate_anim_wait_facing_drift_parameters ( const Mth::Vector& goal_facing, float rotate_factor )
{
	float initial_angle = atan2f(m_facing[X], m_facing[Z]);
	float goal_angle = atan2f(goal_facing[X], goal_facing[Z]);
	
	m_drift_angle = goal_angle - initial_angle;
	if (Mth::Abs(m_drift_angle) > Mth::PI)
	{
		m_drift_angle -= Mth::Sgn(m_drift_angle) * (2.0f * Mth::PI);
	}
	
	m_drift_goal_facing = goal_facing;
	
	m_drift_goal_rotate_factor = rotate_factor;
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
	DUMP_WPOSITION
	
	float angle = Mth::Lerp(-m_drift_angle, 0.0f, Mth::ClampMax(animation_completion_factor / m_drift_goal_rotate_factor, 1.0f));
	m_facing = m_drift_goal_facing;
	m_facing.RotateY(angle);
	
	m_display_offset = Mth::Lerp(m_drift_initial_display_offset, m_drift_goal_display_offset, animation_completion_factor);
	
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
	matrix[W].Set();
	GetObject()->SetMatrix(matrix);
	
	m_display_offset = m_drift_goal_display_offset;
	
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
	// Look for rails.  If we find one, switch to skating and stash the rail data for the skating physics.
	
	if (mp_physics_control_component->IsBoardMissing()) return false;
	
	bool rail_found = false;
	
	if (rail_found = Mdl::Skate::Instance()->GetRailManager()->StickToRail(
		m_frame_start_pos,
		m_pos,
		&m_special_transition_data.rail_data.rail_pos,
		&m_special_transition_data.rail_data.p_node,
		NULL,
		1.0f
	))
	{
		m_special_transition_data.rail_data.p_rail_man = Mdl::Skate::Instance()->GetRailManager();
		m_special_transition_data.rail_data.p_movable_contact = NULL;
	}
	else
	{
		// clean vectors
		m_frame_start_pos[W] = 1.0f;
		m_pos[W] = 1.0f;
		
		for (CRailManagerComponent* p_rail_manager_component = static_cast< CRailManagerComponent* >(CCompositeObjectManager::Instance()->GetFirstComponentByType(CRC_RAILMANAGER));
			p_rail_manager_component && !rail_found;
			p_rail_manager_component = static_cast< CRailManagerComponent* >(p_rail_manager_component->GetNextSameType()))
		{
			Mth::Matrix obj_matrix = p_rail_manager_component->UpdateRailManager();
			Mth::Matrix obj_matrix_inv = obj_matrix;
			obj_matrix_inv.Invert();

			// transform into the frame of the moving object
			Mth::Vector obj_frame_frame_start_pos = obj_matrix_inv.Transform(m_frame_start_pos);
			Mth::Vector obj_frame_pos = obj_matrix_inv.Transform(m_pos);

			if (rail_found = p_rail_manager_component->GetRailManager()->StickToRail(
				obj_frame_frame_start_pos,
				obj_frame_pos,
				&m_special_transition_data.rail_data.rail_pos,
				&m_special_transition_data.rail_data.p_node,
				NULL,
				1.0f
			))
			{
				m_special_transition_data.rail_data.p_rail_man = p_rail_manager_component->GetRailManager();
				m_special_transition_data.rail_data.rail_pos = obj_matrix.Transform(m_special_transition_data.rail_data.rail_pos);
				m_special_transition_data.rail_data.p_movable_contact = p_rail_manager_component->GetObject();
			}
		}
	}
		
	if (!rail_found || !mp_core_physics_component->will_take_rail(
		m_special_transition_data.rail_data.p_node,
		m_special_transition_data.rail_data.p_rail_man,
		true
	))
	{
		return false;
	}
	
	// no single node rails while walking
	if (!m_special_transition_data.rail_data.p_node->GetPrevLink() && !m_special_transition_data.rail_data.p_node->GetNextLink()) return false;
	
	// emulate feeler test in CSkaterCorePhysicsComponent::got_rail so that we don't transition to skating and then reject rail
	CFeeler feeler(m_pos, m_special_transition_data.rail_data.rail_pos);
	if (feeler.GetCollision() && (m_special_transition_data.rail_data.rail_pos - feeler.GetPoint()).LengthSqr() > 6.0f * 6.0f) return false;
	
	if (Script::GetInt(CRCD(0x19fb78fa, "output_tracking_lines")))
	{
		printf ("Tracking%d %.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n", 2, m_frame_start_pos[X], m_frame_start_pos[Y], m_frame_start_pos[Z], m_pos[X], m_pos[Y], m_pos[Z]);
	}
	
	// we have stored the rail data so it can be passed on to the skating physics
	
	m_special_transition_data.type = RAIL_TRANSITION;

	m_frame_event = CRCD(0xa6a3147e, "Rail");
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CWalkComponent::setup_collision_cache (   )
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

void CWalkComponent::update_critical_point_offset (   )
{
	float critical_point_offset_length_sqr = m_critical_point_offset.LengthSqr();
	if (critical_point_offset_length_sqr != 0.0f)
	{
		switch (m_state)
		{
			case WALKING_HANG:
				break;
				
			case WALKING_ANIMWAIT:
				break;
				
			case WALKING_LADDER:
			case WALKING_GROUND:
			case WALKING_AIR:
			{
				Mth::Vector initial_offset = m_critical_point_offset;
				
				// float adjustment_length = 100.0f * m_frame_length;
				float adjustment_length = 1000.0f * m_frame_length;
				float offset_length = sqrtf(critical_point_offset_length_sqr);
				if (adjustment_length < offset_length)
				{
					m_critical_point_offset -= (adjustment_length / offset_length) * m_critical_point_offset;
				}
				else
				{
					m_critical_point_offset.Set();
				}
				
				if (m_state == WALKING_LADDER) break;
				
				CFeeler feeler;
				feeler.m_start = m_frame_start_pos + initial_offset;
				feeler.m_end = m_pos + m_critical_point_offset;
				if (feeler.GetCollision())
				{
					Mth::Vector new_critical_point = feeler.GetPoint() + 0.1f * feeler.GetNormal();
					Mth::Vector required_pos_adjustment = new_critical_point - feeler.m_end;
					m_pos += required_pos_adjustment;
					MESSAGE("adjusting position due to bad critical point");
					DUMPV(required_pos_adjustment);
					DUMPV(m_critical_point_offset);
					DUMP_WPOSITION
				}
				
				if (m_false_wall.active)
				{
					float offset = Mth::DotProduct(m_pos + m_critical_point_offset, m_false_wall.normal) - m_false_wall.distance;
					if (offset > 0.0f)
					{
						m_pos -= offset * m_false_wall.normal;
						DUMP_WPOSITION
					}
				}
				
				break;
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::update_display_offset (   )
{
	switch (m_state)
	{
		case WALKING_ANIMWAIT:
		case WALKING_HANG:
			break;
			
		default:
			m_display_offset = Mth::Lerp(m_display_offset, 0.0f, Tmr::FrameLength() * s_get_param(CRCD(0xb32c972b, "display_offset_restore_rate")));
			break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::adjust_pos_for_false_wall (   )
{
	if (!m_false_wall.active) return;
	
	if (m_pos[Y] > m_false_wall.cancel_height)
	{
		m_false_wall.active = false;
	}
	else
	{
		float offset = Mth::DotProduct(m_false_wall.normal, m_pos) - m_false_wall.distance;
		if (offset > -s_get_param(CRCD(0xa20c43b7, "push_feeler_length")))
		{
			m_pos -= s_get_param(CRCD(0x4d16f37d, "push_strength")) * m_frame_length * m_false_wall.normal;
			DUMP_WPOSITION
			if (Mth::DotProduct(m_horizontal_vel, m_false_wall.normal))
			{
				m_horizontal_vel -= Mth::DotProduct(m_horizontal_vel, m_false_wall.normal) * m_false_wall.normal;
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CWalkComponent::extract_state_from_object (   )
{
	m_pos = GetObject()->m_pos;
	DUMP_WPOSITION
	
	m_horizontal_vel = GetObject()->GetVel();
	m_horizontal_vel[Y] = 0.0f;
	m_vertical_vel = GetObject()->GetVel()[Y];
	
	// note that m_facing and m_upward will often not be orthogonal, but will always span a plan
	
	// generally straight up, but now after a transition from skating
	m_upward = GetObject()->GetMatrix()[Y];
	
	m_facing = GetObject()->GetMatrix()[Z];
	m_frame_initial_facing_Y = m_facing[Y];
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
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CWalkComponent::copy_state_into_object (   )
{
	// build the object's matrix based on our facing
	Mth::Matrix matrix;
	
	m_facing *= sqrtf(1.0f - Mth::Sqr(m_frame_initial_facing_Y));
	m_facing[Y] = m_frame_initial_facing_Y;
	
	// basically, rotate Z upward to perpendicular with m_upward
	matrix[X] = Mth::CrossProduct(m_upward, m_facing);
	matrix[X].Normalize();
	matrix[Y] = m_upward;
	matrix[Z] = Mth::CrossProduct(matrix[X], matrix[Y]);
	matrix[W].Set();
	
	DUMP_WPOSITION
	GetObject()->SetPos(m_pos);
	GetObject()->SetMatrix(matrix);
	GetObject()->SetDisplayMatrix(matrix);
	
	// construct the object's velocity
	switch (m_state)
	{
		case WALKING_GROUND:
		case WALKING_AIR:
			GetObject()->SetVel(m_horizontal_vel);
			GetObject()->GetVel()[Y] = m_vertical_vel;
			break;
			
		case WALKING_HANG:
			GetObject()->GetVel().Set(-m_facing[Z], 0.0f, m_facing[X]);
			GetObject()->GetVel() *= m_hang_move_vel;
			break;
		
		case WALKING_LADDER:
			GetObject()->GetVel().Set(0.0f, m_anim_effective_speed, 0.0f);
			break;
			
		default:
			GetObject()->GetVel().Set();
			break;
	}
	
	mp_model_component->SetDisplayOffset(m_display_offset * matrix[Y]);
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
	if (Mth::Abs(angle) < Mth::DegToRad(s_get_param(CRCD(0x4676a268, "forward_tolerance"))))
	{
		angle = 0.0f;
	}
	
	float sin_angle = sinf(angle);
	float cos_angle = cosf(angle);
	m_control_direction[X] = cos_angle * camera_forward[X] - sin_angle * camera_forward[Z];
	m_control_direction[Z] = sin_angle * camera_forward[X] + cos_angle * camera_forward[Z];
	
	// different control schemes for analog stick and d-pad
	if (control_pad.m_leftX == 0.0f && control_pad.m_leftY == 0.0f)
	{
		// d-pad control
		m_analog_control = false;
	
		if (control_pad.m_leftLength == 0.0f)
		{
			m_control_magnitude = 0.0f;
			m_control_pegged = false;
		}
		else
		{
			if (m_state == WALKING_GROUND && !control_pad.m_x.GetPressed())
			{
				m_control_magnitude = s_get_param(CRCD(0xc1528f7f, "walk_point"));
			}
			else
			{
				m_control_magnitude = 1.0f;
			}
			
            m_control_pegged = true;
		}
	}
	else
	{
		// analog stick control
		m_analog_control = true;
		
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

float CWalkComponent::calculate_desired_speed (   )
{
	float desired_speed;
	
	if (m_control_magnitude == 0.0f)
	{
		desired_speed = 0.0f;
	}
	else
	{
		float walk_point = s_get_param(CRCD(0xc1528f7f, "walk_point"));
		if (m_control_magnitude <= walk_point)
		{
			m_run_toggle = false;
			desired_speed = Mth::LinearMap(0.0f, s_get_param(CRCD(0x79d182ad, "walk_speed")), m_control_magnitude, 0.3f, walk_point);
		}
		else
		{
			m_run_toggle = true;
			desired_speed = Mth::LinearMap(s_get_param(CRCD(0x79d182ad, "walk_speed")), get_run_speed(), m_control_magnitude, walk_point, 1.0f);
		}
	}
	
	return desired_speed;
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
	float max_height_adjustment = 2.0f * s_get_param(CRCD(0x6da7f696, "feeler_height"));
	
	feeler.m_start = proposed_stand_pos;
	feeler.m_start[Y] += max_height_adjustment;
	feeler.m_end = proposed_stand_pos;
	feeler.m_end[Y] -= s_get_param(CRCD(0xaebfa9cd, "stand_pos_search_depth"));
	if (feeler.GetCollision())
	{
		stand_pos = feeler.GetPoint();
		#ifdef __USER_DAN__
		if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
		{
			feeler.DebugLine(100, 100, 0, 0);
		}
		#endif
		return true;
	}
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
	{
		feeler.DebugLine(255, 255, 0, 0);
	}
	#endif
	
	stand_pos = proposed_stand_pos;
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::update_run_speed_factor (   )
{
	if (GetSkaterScoreComponentFromObject(GetObject())->GetScore()->GetScorePotValue() > 0)
	{
		m_run_speed_factor = 1.0f;
		return;
	}
	
	m_run_speed_factor = Mth::ClampMin(m_run_speed_factor - s_get_param(CRCD(0x3fb31dfc, "run_adjust_rate")) * Tmr::FrameLength(), 0.0f);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::smooth_anim_speed ( uint32 previous_frame_event )
{
	if (m_state == WALKING_GROUND)
	{
		if (Tmr::ElapsedTime(m_state_timestamp)	< 100)
		{
			// for the first few frames of the ground state, simply use this frame's anim speed
			m_last_frame_anim_effective_speed = m_anim_effective_speed;
		}
		else
		{
			// otherwise, use the smaller of the two anim speeds from this frame and the last
			float this_frame_anim_effective_speed = m_anim_effective_speed;
			m_anim_effective_speed = Mth::Min(m_anim_effective_speed, m_last_frame_anim_effective_speed);
			m_last_frame_anim_effective_speed = this_frame_anim_effective_speed;
			
			// adjust the frame event as appropriate
			if (m_frame_event == CRCD(0xaf895b3f, "Run") && m_anim_effective_speed == 0.0f)
			{
				switch (previous_frame_event)
				{
					case CRCC(0x1d537eff, "Skid"):
					case CRCC(0xf28adbfc, "RotateLeft"):
					case CRCC(0x912220f8, "RotateRight"):
						m_frame_event = previous_frame_event;
						break;
					default:
						m_frame_event = CRCD(0x9b46e749, "Stand");
						break;
				}
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::set_camera_overrides (   )
{
	switch (m_frame_event)
	{
		// case CRCC(0xf41aba21, "Hop"):
			// mp_camera_component->SetOverrides(m_facing, 0.05f);
			// break;
		
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
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::update_anim_speeds (   )
{
	switch (m_anim_scale_speed)
	{
		case RUNNING:
			if (m_anim_standard_speed > 0.0f)
			{
				mp_animation_component->SetAnimSpeed(m_anim_effective_speed / m_anim_standard_speed, false, false);
				
				// if walking partial anims are active, update the partial anim speed too
				if (mp_physics_control_component->IsBoardMissing())
				{
					Script::CStruct* pParams = new Script::CStruct;
					pParams->AddFloat(CRCD(0xf0d90109, "Speed"), m_anim_effective_speed / m_anim_standard_speed);
					mp_animation_component->CallMemberFunction(CRCD(0xbd4edd44, "SetPartialAnimSpeed"), pParams, NULL);
					delete pParams;
				}
			}
			break;
			
		case HANGMOVE:
		{
			float new_anim_speed = m_anim_effective_speed / s_get_param(CRCD(0x1a47ffab, "hang_move_animation_speed"));
			
			Script::CStruct* pParams = new Script::CStruct;
			pParams->AddFloat(CRCD(0xf0d90109, "Speed"), new_anim_speed);
			mp_animation_component->CallMemberFunction(CRCD(0xbd4edd44, "SetPartialAnimSpeed"), pParams, NULL);
			delete pParams;
			
			mp_animation_component->SetAnimSpeed(new_anim_speed, false, false);
			break;
		}
					
		case LADDERMOVE:
		{
			float new_anim_speed = m_anim_effective_speed / s_get_param(CRCD(0xab2db54, "ladder_move_speed"));
			
			Script::CStruct* pParams = new Script::CStruct;
			pParams->AddFloat(CRCD(0xf0d90109, "Speed"), new_anim_speed);
			mp_animation_component->CallMemberFunction(CRCD(0xbd4edd44, "SetPartialAnimSpeed"), pParams, NULL);
			delete pParams;
			
			// Only ladder animation speed changes are sent to the server in net games.
			if (!GameNet::Manager::Instance()->InNetGame() || Mth::Abs(new_anim_speed - m_last_ladder_anim_speed) < 0.1f)
			{
				mp_animation_component->SetAnimSpeed(new_anim_speed, false, false);
			}
			else
			{
				m_last_ladder_anim_speed = new_anim_speed;
				mp_animation_component->SetAnimSpeed(m_last_ladder_anim_speed, true, false);
			}
			break;
		}
	
		default:
			break;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CWalkComponent::get_gravity (   )
{
	if (!Mdl::Skate::Instance()->GetCareer()->GetCheat(CRCD(0x9c8c6df1, "CHEAT_MOON")))
	{
		return s_get_param(CRCD(0xa5e2da58, "gravity"));
	}
	else
	{
		Mdl::Skate::Instance()->GetCareer()->SetGlobalFlag(Script::GetInteger(CRCD(0x9c8c6df1, "CHEAT_MOON")));
		g_CheatsEnabled = true;
		return 0.5f * s_get_param(CRCD(0xa5e2da58, "gravity"));
	}
}

}

#endif // TESTING_GUNSLINGER
