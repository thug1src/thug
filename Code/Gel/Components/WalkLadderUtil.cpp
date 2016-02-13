//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       WalkLadderUtil.cpp
//* OWNER:          Dan
//* CREATION DATE:  4/29/3
//****************************************************************************

#include <gel/components/walkcomponent.h>
#include <gel/components/triggercomponent.h>
#include <gel/components/movablecontactcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>

#include <sk/objects/rail.h>
#include <sk/engine/feeler.h>
#include <sk/modules/skate/skate.h>
#include <sk/components/skatercorephysicscomponent.h>

// NOTE: The ladder walking code currently ignores movable rail managers!
                 
namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::maybe_climb_up_ladder ( bool user_requested )
{
	// check to see if we're in a bail
	if (mp_core_physics_component->GetFlag(IS_BAILING)) return false;
	
	SLadderRailData rail_data;

	// Check for appropriate rails.
    if (!Mdl::Skate::Instance()->GetRailManager()->CheckForLadderRail(
        m_pos,
        user_requested ? s_get_param(CRCD(0xeecd255, "button_horiz_snap_distance")) : s_get_param(CRCD(0xa2735dd9, "max_horiz_snap_distance")),
        s_get_param(CRCD(0xc4c164e8, "max_vert_snap_distance")),
		true,
        this,
        rail_data,
        &mp_rail_start
    )) return false;
	
	// store the relevant rail manager
	mp_rail_manager = Mdl::Skate::Instance()->GetRailManager();
	mp_movable_contact_component->LoseAnyContact();
	
	// zero velocities	
	m_vertical_vel = 0.0f;
	m_horizontal_vel.Set();
    
    // setup an anim wait state
	
	m_anim_wait_initial_pos = m_pos;
	m_anim_wait_goal_pos = rail_data.climb_point;
	
    mp_anim_wait_complete_callback = &Obj::CWalkComponent::move_to_ladder_bottom_complete;
	
	m_drift_initial_display_offset = m_display_offset;
	m_drift_goal_display_offset = 0.0f;
	
	calculate_anim_wait_facing_drift_parameters(rail_data.climb_facing);
	
	m_anim_wait_camera_mode = AWC_GOAL;
		  	
	// setup the false wall incase the player jumps out of the animation wait
	m_false_wall.normal = rail_data.climb_facing;
	m_false_wall.cancel_height = rail_data.climb_point[Y];

	set_state(WALKING_ANIMWAIT);

	m_frame_event = CRCD(0xe2524194, "LadderOntoBottom");
    
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::maybe_climb_down_ladder (   )
{
   	// check to see if we're in a bail
	if (mp_core_physics_component->GetFlag(IS_BAILING)) return false;
	
    SLadderRailData rail_data;
    
	// Check for appropriate rails.
    if (!Mdl::Skate::Instance()->GetRailManager()->CheckForLadderRail(
        m_pos,
        s_get_param(CRCD(0xa2735dd9, "max_horiz_snap_distance")),
        s_get_param(CRCD(0xc4c164e8, "max_vert_snap_distance")),
		false,
        this,
        rail_data,
        &mp_rail_start
    )) return false;
	
	// store the relevant rail manager
	mp_rail_manager = Mdl::Skate::Instance()->GetRailManager();
	mp_movable_contact_component->LoseAnyContact();
	
	// zero velocities	
	m_vertical_vel = 0.0f;
	m_horizontal_vel.Set();
	
    // setup an anim wait state
	
	m_anim_wait_initial_pos = m_pos;
	m_anim_wait_goal_pos = rail_data.climb_point;
	
    mp_anim_wait_complete_callback = &Obj::CWalkComponent::move_to_ladder_top_complete;
	
	m_drift_initial_display_offset = m_display_offset;
	m_drift_goal_display_offset = 0.0f;
	
	calculate_anim_wait_facing_drift_parameters(rail_data.climb_facing);
	
	// if (Mth::DotProduct(mp_camera->GetMatrix()[Z], m_facing) > 0.0f)
	// {
		m_anim_wait_camera_mode = AWC_GOAL;
	// }
	// else
	// {
		// m_anim_wait_camera_mode = AWC_CURRENT;
	// }
	   		  	
	// setup the false wall incase the player jumps out of the animation wait
	m_false_wall.normal = rail_data.climb_facing;
	m_false_wall.cancel_height = m_pos[Y];

	set_state(WALKING_ANIMWAIT);

	m_frame_event = CRCD(0xd63adcad, "LadderOntoTop");
    
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::maybe_grab_to_ladder ( const Mth::Vector& start_pos, const Mth::Vector& end_pos )
{
    SLadderRailData rail_data;
	
	// Check for appropriate rails.
    if (!Mdl::Skate::Instance()->GetRailManager()->CheckForAirGrabLadderRail(
		start_pos,
		end_pos,
		this,
		rail_data,
		&mp_rail_start
    )) return false;
	
	// store the relevant rail manager
	mp_rail_manager = Mdl::Skate::Instance()->GetRailManager();
	mp_movable_contact_component->LoseAnyContact();
	
	// we've found a rail to climb
	m_pos = rail_data.climb_point;
	DUMP_WPOSITION
	m_facing = rail_data.climb_facing;
	m_along_rail_factor = rail_data.along_rail_factor;
	
	// zero velocities	
	m_vertical_vel = 0.0f;
	m_horizontal_vel.Set();
	
	set_state(WALKING_LADDER);

	m_frame_event = CRCD(0xc84243da, "Ladder");
    
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::move_to_ladder_bottom_complete (   )
{
	// getting on at the bottom of the ladder
	
	// determine top node
	const CRailNode* p_top_rail_node = mp_rail_start->GetNextLink() ? mp_rail_start->GetNextLink() : mp_rail_start->GetPrevLink();
	Dbg_Assert(p_top_rail_node);
		
	// the offset from the ladder bottom to rail top
	Mth::Vector rail = mp_rail_manager->GetPos(p_top_rail_node) - mp_rail_manager->GetPos(mp_rail_start);
	float rail_length = rail.Length();
	
	// calculate initial positoin along rail
	m_along_rail_factor = s_get_param(CRCD(0x5296e3fd, "ladder_bottom_offset_up")) / rail_length;
	
    set_state(WALKING_LADDER);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::move_to_ladder_top_complete (   )
{
	// getting on at the top of the ladder
	
	// determine top node
	const CRailNode* p_top_rail_node = mp_rail_start->GetNextLink() ? mp_rail_start->GetNextLink() : mp_rail_start->GetPrevLink();
	Dbg_Assert(p_top_rail_node);
		
	// the offset from the ladder bottom to rail top
	Mth::Vector rail = mp_rail_manager->GetPos(p_top_rail_node) - mp_rail_manager->GetPos(mp_rail_start);
	float rail_length = rail.Length();
	
	// calculate initial positoin along rail
	m_along_rail_factor = (rail_length - s_get_param(CRCD(0x76bf49e0, "ladder_top_offset_up"))) / rail_length;
	
    set_state(WALKING_LADDER);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::FilterLadderUpRail ( const Mth::Vector& bottom_pos, const Mth::Vector& top_pos, const Mth::Matrix& orientation, SLadderRailData& rail_data )
{
    // check that we're at the bottom of the ladder
    if (bottom_pos[Y] > top_pos[Y]) return false;
    
    // check that the proposed ladder rail is vertical
    Mth::Vector ladder_direction = top_pos - bottom_pos;
    ladder_direction.Normalize();
    if (ladder_direction[Y] < 0.995f) return false;
	
	// check that we're facing the ladder
	if (Mth::DotProduct(m_facing, orientation[Z]) < cosf(Mth::DegToRad(s_get_param(CRCD(0x456f216d, "max_onto_ladder_angle"))))) return false;
    
    // calculate the climb position
    rail_data.climb_point = bottom_pos + ladder_direction * s_get_param(CRCD(0x5296e3fd, "ladder_bottom_offset_up"));
	rail_data.climb_point -= s_get_param(CRCD(0x35b3bbda, "ladder_climb_offset")) * orientation[Z];
    
    // send feeler to climb position
    CFeeler feeler;
    feeler.m_start = m_pos;
    feeler.m_end = rail_data.climb_point - 6.0f * orientation[Z];
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
	{
		feeler.DebugLine(0, 0, 255, 0);
	}
	#endif
    if (feeler.GetCollision()) return false;
    
    // store the climb facing
    rail_data.climb_facing = orientation[Z];
    
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::FilterLadderDownRail ( const Mth::Vector& bottom_pos, const Mth::Vector& top_pos, const Mth::Matrix& orientation, SLadderRailData& rail_data )
{
    // check that we're at the top of the ladder
    if (bottom_pos[Y] > top_pos[Y]) return false;
    
    // check that the proposed ladder rail is vertical
    Mth::Vector ladder_direction = top_pos - bottom_pos;
    ladder_direction.Normalize();
    if (ladder_direction[Y] < 0.995f) return false;
    
    // calculate the climb position
    rail_data.climb_point = top_pos - ladder_direction * s_get_param(CRCD(0x76bf49e0, "ladder_top_offset_up"));
	rail_data.climb_point -= s_get_param(CRCD(0x35b3bbda, "ladder_climb_offset")) * orientation[Z];
    
	/*
    // send feeler to climb position
    CFeeler feeler;
    feeler.m_start = m_pos;
    feeler.m_end = rail_data.climb_point - 6.0f * orientation[Z];
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
	{
		feeler.DebugLine(0, 0, 255, 0);
	}
	#endif
    if (feeler.GetCollision()) return false;
	*/
    
    // store the climb facing
    rail_data.climb_facing = orientation[Z];
    
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::FilterLadderAirGrabRail ( const Mth::Vector& bottom_pos, const Mth::Vector& top_pos, const Mth::Matrix& orientation, const Mth::Vector& preliminary_climb_point, float along_rail_factor, SLadderRailData& rail_data )
{
    // check that the proposed ladder rail is vertical
    Mth::Vector ladder_direction = top_pos - bottom_pos;
    ladder_direction.Normalize();
    if (ladder_direction[Y] < 0.995f) return false;
	
	// insure that the target point on the ladder is above the climb off bottom point
    float off_bottom_height = bottom_pos[Y] + ladder_direction[Y] * s_get_param(CRCD(0x5296e3fd, "ladder_bottom_offset_up"));
	if (preliminary_climb_point[Y] < off_bottom_height) return false;
	
	// insure that the target point on the ladder is below the climb off top point
    float off_top_height = top_pos[Y] - ladder_direction[Y] * s_get_param(CRCD(0x76bf49e0, "ladder_top_offset_up"));
	if (preliminary_climb_point[Y] > off_top_height) return false;
	
	// move the climb point a touch back off the rail
	rail_data.climb_point = preliminary_climb_point - s_get_param(CRCD(0x35b3bbda, "ladder_climb_offset")) * orientation[Z];
	
	// send a feeler to the climb position
	CFeeler feeler;
	feeler.m_start = m_pos;
	feeler.m_end = rail_data.climb_point;
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
	{
		feeler.DebugLine(0, 0, 255, 0);
	}
	#endif
    if (feeler.GetCollision()) return false;
	
	// store the climb facing
	rail_data.climb_facing = orientation[Z];
	
	rail_data.along_rail_factor = along_rail_factor;
	
	mp_core_physics_component->SetFlagTrue(SNAPPED);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::go_ladder_state (   )
{
	if (Mth::Abs((Mth::PI / 2.0f) - Mth::Abs(mp_input_component->GetControlPad().m_leftAngle)) > cosf(Mth::DegToRad(s_get_param(CRCD(0x7ee1b95b, "ladder_control_tolerance")))))
	{
		ladder_movement();
		return;
	}
	
	m_frame_event = CRCD(0xc84243da, "Ladder");
	m_anim_effective_speed = 0.0f;
	return;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::ladder_movement (   )
{
	// movement up and down the ladder
	
	// the desired speed; this will be used to scale the animation speed
	m_anim_effective_speed = s_get_param(CRCD(0xab2db54, "ladder_move_speed")) * m_control_magnitude * cosf(mp_input_component->GetControlPad().m_leftAngle);
	
	// the movement this frame
	float movement = m_anim_effective_speed * m_frame_length;
	
	// get the top rail node; mp_rail_start is always the bottom node
	const CRailNode* p_top_rail_node = mp_rail_start->GetNextLink() ? mp_rail_start->GetNextLink() : mp_rail_start->GetPrevLink();
	Dbg_Assert(p_top_rail_node);
		
	// the offset from the ladder bottom to rail top
	Mth::Vector rail = mp_rail_manager->GetPos(p_top_rail_node) - mp_rail_manager->GetPos(mp_rail_start);
	Dbg_MsgAssert(rail.Length() > s_get_param(CRCD(0x76bf49e0, "ladder_top_offset_up")), ("Ladder too short for ladder animations"));
	
	// the movement in terms of the fraction of the rail
	float delta_factor = movement / rail.Length();
	
	// movement is up or down
	bool up = delta_factor > 0.0f;
	
	// move along the rail
	m_along_rail_factor += delta_factor;
	
	float rail_length = rail.Length();
	
	// see if we've hit the bottom
	float bottom_along_rail_factor = s_get_param(CRCD(0x5296e3fd, "ladder_bottom_offset_up")) / rail_length;
	if (m_along_rail_factor < bottom_along_rail_factor)
	{
		off_ladder_bottom();
		return;
	}
	
	// see if we've hit the top
	float top_along_rail_factor = (rail_length - s_get_param(CRCD(0x76bf49e0, "ladder_top_offset_up"))) / rail_length;
	if (m_along_rail_factor > top_along_rail_factor)
	{
		// we've hit the top
		off_ladder_top();
		return;
	}
	
	// still clamp for now
	m_along_rail_factor = Mth::Clamp(m_along_rail_factor, 0.0f, 1.0f);
	
	if (m_along_rail_factor == 0.0f || m_along_rail_factor == 1.0f)
	{
		m_frame_event = CRCD(0xc84243da, "Ladder");
		m_anim_effective_speed = 0.0f;
	}
	else
	{
		m_frame_event = up ? CRCD(0xaf5abc82, "LadderMoveUp") : CRCD(0xfec9dded, "LadderMoveDown");
	}
	
	// calculate our new position
	m_pos = mp_rail_manager->GetPos(mp_rail_start) + m_along_rail_factor * rail - s_get_param(CRCD(0x35b3bbda, "ladder_climb_offset")) * m_facing;
	DUMP_WPOSITION
	
	// positive animation speed
	m_anim_effective_speed = Mth::Abs(m_anim_effective_speed);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::off_ladder_bottom (   )
{
	Mth::Vector stand_pos = m_pos - s_get_param(CRCD(0xcaf2831b, "ladder_bottom_offset_forward")) * m_facing;
	stand_pos[Y] -= s_get_param(CRCD(0x5296e3fd, "ladder_bottom_offset_up"));
	m_last_ground_feeler_valid = determine_stand_pos(stand_pos, stand_pos, m_last_ground_feeler);
	
	if (m_last_ground_feeler_valid)
	{
		m_ground_normal = m_last_ground_feeler.GetNormal();
	}
	
    // setup an anim wait state
	
	m_anim_wait_initial_pos = m_pos;
	m_anim_wait_goal_pos = stand_pos;
	
    mp_anim_wait_complete_callback = &Obj::CWalkComponent::off_ladder_bottom_complete;
	
	m_drift_initial_display_offset = m_display_offset;
	m_drift_goal_display_offset = 0.0f;
	
	calculate_anim_wait_facing_drift_parameters(m_facing);
	
	m_anim_wait_camera_mode = AWC_GOAL;
	   		  	
	// setup the false wall incase the player jumps out of the animation wait
	m_false_wall.normal = m_facing;
	m_false_wall.cancel_height = m_pos[Y];

	set_state(WALKING_ANIMWAIT);

	m_frame_event = CRCD(0x4db1bbb1, "LadderOffBottom");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::off_ladder_bottom_complete (   )
{
    set_state(WALKING_GROUND);
	
	if (m_last_ground_feeler_valid)
	{
		mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_ONTO, m_last_ground_feeler);

		// check for a moving contact
		mp_movable_contact_component->CheckForMovableContact(m_last_ground_feeler);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::off_ladder_top (   )
{
	// determine goal position
	
	Mth::Vector stand_pos = m_pos + s_get_param(CRCD(0xb620b580, "ladder_top_offset_forward")) * m_facing; 
	stand_pos[Y] += s_get_param(CRCD(0x76bf49e0, "ladder_top_offset_up"));
	
	CFeeler potential_ground_feeler;
	bool ground = determine_stand_pos(stand_pos, stand_pos, potential_ground_feeler);

	// setup an anim wait state
	
	m_anim_wait_initial_pos = m_pos;
	m_anim_wait_goal_pos = stand_pos;
	
	if (ground)
	{
        mp_anim_wait_complete_callback = &Obj::CWalkComponent::off_ladder_top_to_ground_complete;
		
		m_last_ground_feeler = potential_ground_feeler;
		m_last_ground_feeler_valid = true;
		m_ground_normal = m_last_ground_feeler.GetNormal();
	}
	else
	{
		mp_anim_wait_complete_callback = &Obj::CWalkComponent::off_ladder_top_to_air_complete;
	}
	
	m_drift_initial_display_offset = m_display_offset;
	m_drift_goal_display_offset = 0.0f;
	
	calculate_anim_wait_facing_drift_parameters(m_facing);
	
	m_anim_wait_camera_mode = AWC_GOAL;
	   		  	
	// setup the false wall incase the player jumps out of the animation wait
	m_false_wall.normal = m_facing;
	m_false_wall.cancel_height = stand_pos[Y];
	
	set_state(WALKING_ANIMWAIT);
	
	m_frame_event = CRCD(0x12521bfb, "LadderOffTop");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::off_ladder_top_to_ground_complete (   )
{
    set_state(WALKING_GROUND);
	
	if (m_last_ground_feeler_valid)
	{
		mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_ONTO, m_last_ground_feeler);

		// check for a moving contact
		mp_movable_contact_component->CheckForMovableContact(m_last_ground_feeler);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::off_ladder_top_to_air_complete (   )
{
    set_state(WALKING_AIR);
	
	m_primary_air_direction = m_facing;
	leave_movable_contact_for_air(m_horizontal_vel, m_vertical_vel);
}

}
