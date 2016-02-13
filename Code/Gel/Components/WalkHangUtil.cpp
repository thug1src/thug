//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       WalkHangUtil.cpp
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
#include <sk/components/skatercorephysicscomponent.h>

#include <gfx/nxmodel.h>

/*
 * Contains those CWalkComponent member functions which are concerned with hanging.
 */

namespace Obj
{
     
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*
bool CWalkComponent::maybe_hop_to_hang (   )
{
	SHangRailData rail_data;
	
	Mth::Vector search_start_pos = m_pos;
	search_start_pos[Y] += s_get_param(CRCD(0x2c942693, "lowest_hang_height"));
	
	// clean vectors; facing is a directional vector
	search_start_pos[W] = 1.0f;
	m_facing[W] = 0.0f;
	
	bool rail_found = false;
	
	float hop_to_hang_reach = s_get_param(CRCD(0x8b785f26, "max_hop_to_hang_height")) - s_get_param(CRCD(0x2c942693, "lowest_hang_height"));
	float max_hop_to_hang_forward_reach = s_get_param(CRCD(0x3b4ca389, "max_hop_to_hang_forward_reach"));
	float max_hop_to_hang_backward_reach = s_get_param(CRCD(0xae806bee, "max_hop_to_hang_backward_reach"));
	
	// check level for appropriate rails
	if (rail_found = Mdl::Skate::Instance()->GetRailManager()->CheckForHopToHangRail(
		search_start_pos,
		hop_to_hang_reach,
		m_facing,
		max_hop_to_hang_forward_reach,
		max_hop_to_hang_backward_reach,
		this,
		rail_data,
		&mp_rail_node
	))
	{
		mp_rail_manager = Mdl::Skate::Instance()->GetRailManager();
		mp_movable_contact_component->LoseAnyContact();
	}
	else
	{
		float max_hop_to_movable_hang_vel_sqr = s_get_param(CRCD(0x511cb4, "max_hop_to_movable_hang_vel"));
		max_hop_to_movable_hang_vel_sqr *= max_hop_to_movable_hang_vel_sqr;
		
		// check moving objects for appropriate rails
		for (CRailManagerComponent* p_rail_manager_component = static_cast< CRailManagerComponent* >(CCompositeObjectManager::Instance()->GetFirstComponentByType(CRC_RAILMANAGER));
			p_rail_manager_component && !rail_found;
			p_rail_manager_component = static_cast< CRailManagerComponent* >(p_rail_manager_component->GetNextSameType()))
		{
			// only hop to moving hangs below a threshold velocity
			if (p_rail_manager_component->GetObject()->GetVel().LengthSqr() > max_hop_to_movable_hang_vel_sqr) continue;
			
			Mth::Matrix obj_matrix_inv = p_rail_manager_component->UpdateRailManager();
			obj_matrix_inv.Invert();

			// transform into the frame of the moving object
			Mth::Vector obj_frame_search_start_pos = obj_matrix_inv.Transform(search_start_pos);
			Mth::Vector obj_frame_facing = obj_matrix_inv.Transform(m_facing);

			if (rail_found = p_rail_manager_component->GetRailManager()->CheckForHopToHangRail(
				obj_frame_search_start_pos,
				hop_to_hang_reach,
				obj_frame_facing,
				max_hop_to_hang_forward_reach,
				max_hop_to_hang_backward_reach,
				this,
				rail_data,
				&mp_rail_node
			))
			{
				mp_rail_manager = p_rail_manager_component->GetRailManager();
				mp_movable_contact_component->ObtainContact(p_rail_manager_component->GetObject());
			}
		}
	}
	
	if (!rail_found) return false;
	
	// we've found a rail to hop up to
	m_goal_hop_pos = rail_data.hang_point;
	m_goal_hop_facing = rail_data.hang_facing;
	m_along_rail_factor = rail_data.along_rail_factor;
	m_vertical_hang_offset = rail_data.vertical_hang_offset;
	m_horizontal_hang_offset = rail_data.horizontal_hang_offset;
	m_initial_hang_animation = rail_data.initial_animation;
	
	// calculate the vertical velocity required to jump to the hang point
	#ifdef __NOPT_ASSERT__
	if (m_pos[Y] >= m_goal_hop_pos[Y])
	{
		DUMPV(m_pos);
		DUMPV(m_goal_hop_pos);
	}
	Dbg_Assert(m_pos[Y] < m_goal_hop_pos[Y]);
	#endif
	
	float hop_target_vert_vel = s_get_param(CRCD(0x7aa618cc, "hop_target_vert_vel"));
	float gravity = s_get_param(CRCD(0xa5e2da58, "gravity"));
	
	// ready state for the hop to hang
	
	// calculate the required hop velocity
	m_vertical_vel = sqrtf(Mth::Sqr(hop_target_vert_vel) + 2.0f * gravity * (m_goal_hop_pos[Y] - m_pos[Y]));
	
	// calculate the required hop duration
	float duration = (m_vertical_vel + hop_target_vert_vel) / gravity;
	
	// not neccesarly exactly in the check_direction of the facing
	m_horizontal_vel = (1.0f / duration) * (m_goal_hop_pos - m_pos);
	
	// trip the ground's jump triggers
	if (m_last_ground_feeler_valid)
	{
		mp_trigger_component->CheckFeelerForTrigger(TRIGGER_JUMP_OFF, m_last_ground_feeler);
		if (should_bail_from_frame()) return true;
	}
	
	set_state(WALKING_HOP);
    
	m_frame_event = CRCD(0xf41aba21, "Hop");
	
	return true;
}
*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::maybe_drop_to_hang (   )
{
	// check to see if we're in a bail
	if (mp_core_physics_component->GetFlag(IS_BAILING)) return false;
	
	// only drop to hang when moving slowly
	if (m_horizontal_vel.LengthSqr() > Mth::Sqr(s_get_param(CRCD(0x1d1e6af, "drop_to_hang_speed_factor")) * get_run_speed())) return false;
	
	// clean vector
	m_frame_start_pos[W] = 1.0f;
	m_pos[W] = 1.0f;
	
	SHangRailData rail_data;
	
	bool rail_found = false;
	
	// check level for appropriate rails
	mp_rail_manager = Mdl::Skate::Instance()->GetRailManager();
	if (rail_found = mp_rail_manager->CheckForHangRail(
		m_frame_start_pos,
		m_pos,
		-m_facing,
		this,
		rail_data,
		Script::GetFloat(CRCD(0xa8276303, "Drop_To_Climb_Max_Snap"))
	))
	{
		mp_movable_contact_component->LoseAnyContact();
	}
	else
	{
		// check moving objects for appropriate rails
		for (CRailManagerComponent* p_rail_manager_component = static_cast< CRailManagerComponent* >(CCompositeObjectManager::Instance()->GetFirstComponentByType(CRC_RAILMANAGER));
			p_rail_manager_component && !rail_found;
			p_rail_manager_component = static_cast< CRailManagerComponent* >(p_rail_manager_component->GetNextSameType()))
		{
			Mth::Matrix obj_matrix_inv = p_rail_manager_component->UpdateRailManager();
			obj_matrix_inv.Invert();

			// transform into the frame of the moving object
			Mth::Vector obj_frame_start_pos = obj_matrix_inv.Transform(m_frame_start_pos);
			Mth::Vector obj_pos = obj_matrix_inv.Transform(m_pos);

			mp_rail_manager = p_rail_manager_component->GetRailManager();
			if (rail_found = mp_rail_manager->CheckForHangRail(
				obj_frame_start_pos,
				obj_pos,
				-m_facing,
				this,
				rail_data,
				Script::GetFloat(CRCD(0xa8276303, "Drop_To_Climb_Max_Snap"))
			))
			{
				mp_movable_contact_component->ObtainContact(p_rail_manager_component->GetObject());
			}
		}
	}

	if (!rail_found) return false;
	
	// we've found a rail to drop down to
	
	mp_rail_start = rail_data.p_rail_node;
	mp_rail_end = mp_rail_start->GetNextLink();
	Dbg_Assert(mp_rail_end);
	
	// zero velocities	
	m_vertical_vel = 0.0f;
	m_horizontal_vel.Set();
	
	m_along_rail_factor = rail_data.along_rail_factor;
	m_vertical_hang_offset = rail_data.vertical_hang_offset;
	m_horizontal_hang_offset = rail_data.horizontal_hang_offset;
    
    // setup an anim wait state
	
	m_anim_wait_initial_pos = m_pos;
	m_anim_wait_goal_pos = rail_data.hang_point;
	
    mp_anim_wait_complete_callback = &Obj::CWalkComponent::drop_to_hang_complete;
	
	calculate_anim_wait_facing_drift_parameters(rail_data.hang_facing, s_get_param(CRCD(0xfdc8aec0, "drop_to_hang_rotate_factor")));
	
	m_anim_wait_camera_mode = AWC_GOAL;
	
	m_critical_point_offset = s_get_param(CRCD(0xc3263e6e, "hang_critical_point_horiz_offset")) * rail_data.hang_facing;
	
	// setup the false wall incase the player jumps out of the animation wait
	m_false_wall.normal = rail_data.hang_facing;
	m_false_wall.cancel_height = m_pos[Y];
	
	m_drift_initial_display_offset = m_display_offset;
	m_drift_goal_display_offset = get_hang_display_offset();

	set_state(WALKING_ANIMWAIT);

	m_frame_event = CRCD(0xeb71d98e, "DropToHang");
    
    return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::drop_to_hang_complete (   )
{
	m_critical_point_offset = s_get_param(CRCD(0xc3263e6e, "hang_critical_point_horiz_offset")) * m_facing;
	m_critical_point_offset[Y] = get_hang_critical_point_vert_offset();
	
	set_state(WALKING_HANG);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::maybe_grab_to_hang ( Mth::Vector start_pos, Mth::Vector end_pos )
{
	// only grab to hang when moving down
	if (m_vertical_vel > 50.0f) return false;
	
	start_pos[Y] += s_get_param(CRCD(0xa8c13e74, "hang_vert_origin_offset"));
	end_pos[Y] += s_get_param(CRCD(0xa8c13e74, "hang_vert_origin_offset"));
	
	// clean vectors
	start_pos[W] = 1.0f;
	end_pos[W] = 1.0f;
	
	SHangRailData rail_data;
	
	bool rail_found = false;
	
	// check level for appropriate rails
	mp_rail_manager = Mdl::Skate::Instance()->GetRailManager();
	if (rail_found = mp_rail_manager->CheckForHangRail(
		start_pos,
		end_pos,
		m_facing,
		this,
		rail_data,
		Script::GetFloat(CRCD(0x30ce7f2c, "Climb_Max_Snap"))
	))
	{
		mp_movable_contact_component->LoseAnyContact();
	}
	else
	{
		// check moving objects for appropriate rails
		for (CRailManagerComponent* p_rail_manager_component = static_cast< CRailManagerComponent* >(CCompositeObjectManager::Instance()->GetFirstComponentByType(CRC_RAILMANAGER));
			p_rail_manager_component && !rail_found;
			p_rail_manager_component = static_cast< CRailManagerComponent* >(p_rail_manager_component->GetNextSameType()))
		{
			Mth::Matrix obj_matrix_inv = p_rail_manager_component->UpdateRailManager();
			obj_matrix_inv.Invert();
			
			// transform into the frame of the moving object
			Mth::Vector obj_frame_start_pos = obj_matrix_inv.Transform(start_pos);
			Mth::Vector obj_frame_end_pos = obj_matrix_inv.Transform(end_pos);
			
			mp_rail_manager = p_rail_manager_component->GetRailManager();
			if (rail_found = mp_rail_manager->CheckForHangRail(
				obj_frame_start_pos,
				obj_frame_end_pos,
				m_facing,
				this,
				rail_data,
				Script::GetFloat(CRCD(0x30ce7f2c, "Climb_Max_Snap"))
			))
			{
				mp_movable_contact_component->ObtainContact(p_rail_manager_component->GetObject());
			}
		}
	}
	
	if (!rail_found) return false;
		
	// we've found a rail to hang onto
	
	mp_rail_start = rail_data.p_rail_node;
	mp_rail_end = mp_rail_start->GetNextLink();
	Dbg_Assert(mp_rail_end);
	
	m_pos = rail_data.hang_point;
	DUMP_WPOSITION
	m_facing = rail_data.hang_facing;
	m_along_rail_factor = rail_data.along_rail_factor;
	m_vertical_hang_offset = rail_data.vertical_hang_offset;
	m_horizontal_hang_offset = rail_data.horizontal_hang_offset;
	m_initial_hang_animation = rail_data.initial_animation;
	
	m_critical_point_offset = s_get_param(CRCD(0xc3263e6e, "hang_critical_point_horiz_offset")) * m_facing;
	m_critical_point_offset[Y] = get_hang_critical_point_vert_offset();
	
	m_horizontal_vel.Set();
	m_vertical_vel = 0.0f;
	
	m_display_offset = get_hang_display_offset();

	set_state(WALKING_HANG);

	m_frame_event = CRCD(0x4194ecca, "Hang");
	
	mp_core_physics_component->SetFlagTrue(SNAPPED);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::FilterHangRail ( const Mth::Vector& pos_a, const Mth::Vector& pos_b, const Mth::Vector& preliminary_hang_point, const Mth::Vector& grab_from_point, float along_rail_factor, SHangRailData& rail_data, bool drop_to_hang )
{
	// check this rail for potential hang use
	
	float hang_vert_origin_offset = s_get_param(CRCD(0xa8c13e74, "hang_vert_origin_offset")); 
	
	// never snap upward to a hang rail
	if (m_state == WALKING_AIR && preliminary_hang_point[Y] > grab_from_point[Y]) return false;
	
	// never snap too far vertically to a hang rail
	if (m_state == WALKING_AIR && Mth::Abs(preliminary_hang_point[Y] - grab_from_point[Y]) > 12.0f) return false;
	
	// is the rail too steep
	Mth::Vector rail = pos_b - pos_a;
	Mth::Vector rail_direction = rail;
	rail_direction.Normalize();
	if (Mth::Abs(rail_direction[Y]) > sinf(Mth::DegToRad(s_get_param(CRCD(0x98724ebc, "hang_max_rail_ascent"))))) return false;
	
	// calculate the facing we'd have while hanging
	
	rail_data.hang_facing.Set(-(rail[Z]), 0.0f, rail[X]);
	rail_data.hang_facing.Normalize();
	if (!drop_to_hang)
	{
		// normally, we attempt retain our current facing
		if (Mth::DotProduct(rail_data.hang_facing, m_facing) < 0.0f)
		{
			rail_data.hang_facing.Negate();
		}
	}
	else
	{
		// unless dropping to a hang
		if (Mth::DotProduct(rail_data.hang_facing, m_facing) > 0.0f)
		{
			rail_data.hang_facing.Negate();
		}
	}

	bool rail_points_left = rail_data.hang_facing[Z] * rail_direction[X] - rail_data.hang_facing[X] * rail_direction[Z] > 0.0f;
	if ((rail_data.p_rail_node->GetFlag(NO_HANG_WITH_LEFT_ALONG_RAIL) && rail_points_left)
		|| (rail_data.p_rail_node->GetFlag(NO_HANG_WITH_RIGHT_ALONG_RAIL) && !rail_points_left))
	{
		rail_data.hang_facing.Negate();
		rail_points_left = !rail_points_left;
	}
	
	// investigate the hang point to see if its a good hang
	
	CFeeler feeler;
	Nx::CCollCache* p_coll_cache = Nx::CCollCacheManager::sCreateCollCache();
	Mth::CBBox bbox(
		preliminary_hang_point - Mth::Vector(24.0f, 24.0f, 24.0f),
		preliminary_hang_point + Mth::Vector(24.0f, 12.0f + hang_vert_origin_offset, 24.0f)
	);
	p_coll_cache->Update(bbox);
	feeler.SetCache(p_coll_cache);
	
	rail_data.along_rail_factor = along_rail_factor;
	
	if (!investigate_hang_rail_vicinity(feeler, preliminary_hang_point, rail_data))
	{
		// if we're attempting to grab a rail from the air, try both possible facings
		if (m_state == WALKING_AIR || !drop_to_hang)
		{
			// see if we're allowed to try the other facing
			if ((rail_data.p_rail_node->GetFlag(NO_HANG_WITH_LEFT_ALONG_RAIL) && !rail_points_left)
				|| (rail_data.p_rail_node->GetFlag(NO_HANG_WITH_RIGHT_ALONG_RAIL) && rail_points_left))
			{
				Nx::CCollCacheManager::sDestroyCollCache(p_coll_cache);
				return false;
			}
			
			rail_data.hang_facing.Negate();
			if (!investigate_hang_rail_vicinity(feeler, preliminary_hang_point, rail_data))
			{
				Nx::CCollCacheManager::sDestroyCollCache(p_coll_cache);
				return false;
			}
		}
		else
		{
			Nx::CCollCacheManager::sDestroyCollCache(p_coll_cache);
			return false;
		}
	}
	
	rail_data.vertical_hang_offset = rail_data.hang_point[Y] - preliminary_hang_point[Y];
	rail_data.horizontal_hang_offset = Mth::DotProduct(rail_data.hang_facing, rail_data.hang_point - preliminary_hang_point);
	
	// determine which sort of initial hang animation would be used
	feeler.m_start = rail_data.hang_point + s_get_param(CRCD(0xc3263e6e, "hang_critical_point_horiz_offset")) * rail_data.hang_facing;
	feeler.m_start[Y] += s_get_param(CRCD(0xdbb3d53e, "hang_init_anim_feeler_height"));
	feeler.m_end = feeler.m_start;
	feeler.m_end += s_get_param(CRCD(0x2f83ae83, "hang_init_anim_feeler_length")) * rail_data.hang_facing;
	if (feeler.GetCollision())
	{
		#ifdef __USER_DAN__
		if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
		{
			feeler.DebugLine(255, 0, 0, 0);
		}
		#endif
		rail_data.initial_animation = CRCD(0xec0a1009, "WALL");
	}
	else
	{
		#ifdef __USER_DAN__
		if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
		{
			feeler.DebugLine(0, 0, 255, 0);
		}
		#endif
		rail_data.initial_animation = CRCD(0x1ee948a5, "SWING");
	}
	
	Nx::CCollCacheManager::sDestroyCollCache(p_coll_cache);
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::investigate_hang_rail_vicinity ( CFeeler& feeler, const Mth::Vector& preliminary_hang_point, SHangRailData& rail_data )
{
	/* turning off geo investigation for hang point adjustment
	// run a feeler up/down at the preliminary hang point to find the true top of the ledge
	feeler.SetLine(preliminary_hang_point, preliminary_hang_point);
	feeler.m_start[Y] += s_get_param(CRCD(0x1bc501ab, "ledge_top_feeler_up"));
	feeler.m_end[Y] -= s_get_param(CRCD(0xf5f39a8, "ledge_top_feeler_down"));
	if (feeler.GetCollision())
	{
		// found a ledge top
		rail_data.hang_point = feeler.GetPoint();
	}
	else
	{
		// use the prelimiary hang point
		rail_data.hang_point = preliminary_hang_point;
	}
	
	// run a feeler in an inch below the hang point along the hang facing to find the front of the ledge
	feeler.m_start = rail_data.hang_point - s_get_param(CRCD(0x95fd5dc5, "ledge_front_feeler_back")) * rail_data.hang_facing;
	feeler.m_end = rail_data.hang_point + s_get_param(CRCD(0xccb4f5d2, "ledge_front_feeler_forward")) * rail_data.hang_facing;
	feeler.m_start[Y] -= 1.0f;
	feeler.m_end[Y] -= 1.0f;
	if (feeler.GetCollision())
	{
		// found a ledge front
		rail_data.hang_point = feeler.GetPoint();
		rail_data.hang_point[Y] += 1.0f;
	}
	// hang a touch outside of it
	rail_data.hang_point -= 2.0f * rail_data.hang_facing;
	*/
	
	rail_data.hang_point = preliminary_hang_point;
	
	Mth::Vector left(-rail_data.hang_facing[Z], 0.0f, rail_data.hang_facing[X]);
	
	Mth::Vector rail = mp_rail_manager->GetPos(rail_data.p_rail_node) - mp_rail_manager->GetPos(rail_data.p_rail_node->GetNextLink());
	
	// now that we have the hang position, run two feelers up and down the hang location to insure there is room
	if (!vertical_hang_obstruction_check(feeler, rail_data, left) || !vertical_hang_obstruction_check(feeler, rail_data, -left)) return false;

	// adjust the hang point for the origin of the hanging skater
	rail_data.hang_point[Y] -= s_get_param(CRCD(0xa8c13e74, "hang_vert_origin_offset"));
	rail_data.hang_point += s_get_param(CRCD(0x262a9467, "hang_horiz_origin_offset")) * rail_data.hang_facing;
	
	// use our move feelers for a look around
	if (check_environment_for_hang_movement_collisions(rail_data.hang_facing, rail_data.along_rail_factor, rail_data.hang_point, true, left, rail, rail_data.p_rail_node, rail_data.p_rail_node->GetNextLink())) return false;
	if (check_environment_for_hang_movement_collisions(rail_data.hang_facing, rail_data.along_rail_factor, rail_data.hang_point, false, left, rail, rail_data.p_rail_node, rail_data.p_rail_node->GetNextLink())) return false;
	
	// adjust the feeler target check point based on hanging's critical point offset
	Mth::Vector critical_point = rail_data.hang_point + s_get_param(CRCD(0xc3263e6e, "hang_critical_point_horiz_offset")) * rail_data.hang_facing;
	critical_point[Y] += get_hang_critical_point_vert_offset();
	
	// check for obstructions to our hang point; it is critical that our critical point remains on the happy side of collision geometry
	feeler.m_start = m_pos;
	feeler.m_end = critical_point;
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
	{
		feeler.DebugLine(255, 0, 255, 0);
	}
	#endif
	if (feeler.GetCollision())
	{
		if (m_state == WALKING_GROUND)
		{
			// we found an obstruction taking a direct route; if we'll be hopping to the rail, try an up-and-over route
			
			feeler.m_end = m_pos;
			feeler.m_end[Y] += s_get_param(CRCD(0xa85ec070, "hop_obstruction_feeler_up"));
			#ifdef __USER_DAN__
			if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
			{
				feeler.DebugLine(255, 0, 255, 0);
			}
			#endif
			if (feeler.GetCollision())
			{
				#ifdef __USER_DAN__
				if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
				{
					Gfx::AddDebugStar(feeler.GetPoint(), 12.0f, MAKE_RGB(0, 255, 255), 0);
				}
				#endif
				return false;
			}
			
			feeler.m_start = feeler.m_end;
			feeler.m_end = critical_point;
			#ifdef __USER_DAN__
			if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
			{
				feeler.DebugLine(255, 0, 255, 0);
			}
			#endif
			if (feeler.GetCollision())
			{
				#ifdef __USER_DAN__
				if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
				{
					Gfx::AddDebugStar(feeler.GetPoint(), 12.0f, MAKE_RGB(0, 255, 255), 0);
				}
				#endif
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	
	return true;
}
		
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::vertical_hang_obstruction_check ( CFeeler& feeler, SHangRailData& rail_data, const Mth::Vector& check_offset_direction )
{
	feeler.m_start = rail_data.hang_point + s_get_param(CRCD(0xc3263e6e, "hang_critical_point_horiz_offset")) * rail_data.hang_facing
		+ s_get_param(CRCD(0xafbf7687, "hang_obstruction_feeler_side")) * check_offset_direction;
	feeler.m_end = feeler.m_start;
	feeler.m_start[Y] += -s_get_param(CRCD(0xa8c13e74, "hang_vert_origin_offset")) + get_hang_critical_point_vert_offset();
	feeler.m_end[Y] += s_get_param(CRCD(0x8f35f29f, "hang_obstruction_feeler_up"));
	if (feeler.GetCollision())
	{
		#ifdef __USER_DAN__
		if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
		{
			feeler.DebugLine(255, 0, 50, 0);
		}
		#endif
		return false;
	}
	Mth::Swap(feeler.m_start, feeler.m_end);
	if (feeler.GetCollision())
	{
		#ifdef __USER_DAN__
		if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
		{
			feeler.DebugLine(0, 255, 50, 0);
		}
		#endif
		return false;
	}
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
	{
		feeler.DebugLine(200, 200, 50, 0);
	}
	#endif
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*
void CWalkComponent::go_hop_state (   )
{
	// During the hop to hang state, the player has no control.  We simply move the goal position.
	
	if (mp_movable_contact_component->UpdateContact(m_pos))
	{
		m_pos += mp_movable_contact_component->GetContact()->GetMovement();
		
		if (mp_movable_contact_component->GetContact()->IsRotated())
		{
			m_facing = mp_movable_contact_component->GetContact()->GetRotation().Rotate(m_facing);
			m_goal_hop_facing = mp_movable_contact_component->GetContact()->GetRotation().Rotate(m_goal_hop_facing);
			if (m_facing[Y] != 0.0f)
			{
				m_facing[Y] = 0.0f;
				m_facing.Normalize();
			}
			if (m_goal_hop_facing[Y] != 0.0f)
			{
				m_goal_hop_facing[Y] = 0.0f;
				m_goal_hop_facing.Normalize();
			}
		}
	}
	
	if (m_vertical_vel > -s_get_param(CRCD(0x7aa618cc, "hop_target_vert_vel")))
	{
		m_pos += m_horizontal_vel * m_frame_length;
		m_pos[Y] += m_vertical_vel * m_frame_length + 0.5f * -s_get_param(CRCD(0xa5e2da58, "gravity")) * Mth::Sqr(m_frame_length);
		
		m_vertical_vel += -s_get_param(CRCD(0xa5e2da58, "gravity")) * m_frame_length;
		
		// BUG: we really want more control over this; this can cause pops; perhaps hold the hop duration and use the hop fraction complete
		m_facing = Mth::Lerp(m_facing, m_goal_hop_facing, 30.0f * Tmr::FrameLength()); 

		m_frame_event = CRCD(0xf41aba21, "Hop");
	}
	else
	{
		m_horizontal_vel.Set();
		m_vertical_vel = 0.0f;
		
		m_pos = m_goal_hop_pos;
		
		m_facing = m_goal_hop_facing;
		
		set_state(WALKING_HANG);
		
		m_frame_event = CRCD(0x4194ecca, "Hang");
	}
}
*/
		
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::go_hang_state (   )
{
	// Mth::Vector rail = (mp_rail_manager->GetPos(mp_rail_start) - mp_rail_manager->GetPos(mp_rail_end)).Normalize();
	// DUMPF(Mth::RadToDeg(asin(Mth::Abs(rail[Y]))));
	
	if (mp_movable_contact_component->UpdateContact(m_pos))
	{
		CRailManagerComponent* p_rail_manager_component = GetRailManagerComponentFromObject(mp_movable_contact_component->GetContact()->GetObject());
		Dbg_Assert(p_rail_manager_component);

		p_rail_manager_component->UpdateRailManager();
		
		// calculate our new position
		set_pos_from_hang_rail_state();
	}
	
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0x1a5eab7, "rail_highlights")))
	{
		Gfx::AddDebugLine(mp_rail_manager->GetPos(mp_rail_start), mp_rail_manager->GetPos(mp_rail_end), MAKE_RGB(Mth::Rnd(256), Mth::Rnd(256), Mth::Rnd(256)), 0, 1);
		Gfx::AddDebugLine(mp_rail_manager->GetPos(mp_rail_start) + Mth::Vector(1.0f, 0.0f, 0.0f), mp_rail_manager->GetPos(mp_rail_end) + Mth::Vector(1.0f, 0.0f, 0.0f), MAKE_RGB(Mth::Rnd(256), Mth::Rnd(256), Mth::Rnd(256)), 0, 1);
		Gfx::AddDebugLine(mp_rail_manager->GetPos(mp_rail_start) + Mth::Vector(0.0f, 1.0f, 0.0f), mp_rail_manager->GetPos(mp_rail_end) + Mth::Vector(0.0f, 1.0f, 0.0f), MAKE_RGB(Mth::Rnd(256), Mth::Rnd(256), Mth::Rnd(256)), 0, 1);
		Gfx::AddDebugLine(mp_rail_manager->GetPos(mp_rail_start) + Mth::Vector(0.0f, 0.0f, 1.0f), mp_rail_manager->GetPos(mp_rail_end) + Mth::Vector(0.0f, 0.0f, 1.0f), MAKE_RGB(Mth::Rnd(256), Mth::Rnd(256), Mth::Rnd(256)), 0, 1);
	}
	#endif
	
	// only respond to full up/down controls and only after a frame delay (to allow the debounce a frame to kick in)
	if (m_control_pegged)
	{
		// up; IsLeftAnalogUpPressed allows for analog debouncing
		if (mp_input_component->GetControlPad().IsLeftAnalogUpPressed())
		{
			// potential standing position after pull up
			Mth::Vector stand_pos = m_pos + s_get_param(CRCD(0x21dfbe77, "pull_up_offset_forward")) * m_facing;
			stand_pos[Y] += s_get_param(CRCD(0x52fa7ca6, "pull_up_offset_up"));
			if (maybe_pull_up_from_hang(stand_pos)) return;
			
			// try standing a little closer
			stand_pos = m_pos + 0.5f * s_get_param(CRCD(0x21dfbe77, "pull_up_offset_forward")) * m_facing;
			stand_pos[Y] += s_get_param(CRCD(0x52fa7ca6, "pull_up_offset_up"));
			if (maybe_pull_up_from_hang(stand_pos)) return;
		}
		// down; IsLeftAnalogDownPressed allows for analog debouncing
		else if (mp_input_component->GetControlPad().IsLeftAnalogDownPressed())
		{
			// drop
			set_state(WALKING_AIR);
			m_primary_air_direction = m_facing;
			leave_movable_contact_for_air(m_horizontal_vel, m_vertical_vel);
			m_frame_event = CRCD(0x439f4704, "Air");
			
			return;
		}
	}
	
	// left or right
	float control_vel;
	if (Mth::Abs((Mth::PI / 2.0f) - Mth::Abs(mp_input_component->GetControlPad().m_leftAngle)) < cosf(Mth::DegToRad(s_get_param(CRCD(0x1b877928, "hang_vert_control_tolerance")))))
	{
		control_vel = s_get_param(CRCD(0xd77ee881, "hang_move_speed")) * m_control_magnitude
			* (mp_input_component->GetControlPad().m_leftAngle > 0.0f ? 1.0f : -1.0f);
	}
	else
	{
		control_vel = 0.0f;
	}
	m_hang_move_vel = Mth::Lerp(m_hang_move_vel, control_vel, s_get_param(CRCD(0xc506a97c, "hang_move_lerp_rate")) * m_frame_length);
	
	if (Mth::Abs(m_hang_move_vel) > s_get_param(CRCD(0x228b5376, "hang_move_cutoff")))
	{
		m_anim_effective_speed = Mth::Abs(m_hang_move_vel);
	
		hang_movement(m_hang_move_vel * m_frame_length);

		// calculate our new position
		set_pos_from_hang_rail_state();
	}
	else
	{
		m_frame_event = CRCD(0x4194ecca, "Hang");
	}
	
	/* this doesn't make sense with the new higher critical point
	// run a feeler between our old and new position to insure that our position stays safe
	CFeeler feeler;
	feeler.m_start = m_frame_start_pos + s_get_param(CRCD(0xc3263e6e, "hang_critical_point_horiz_offset")) * m_facing;
	feeler.m_end = m_pos + s_get_param(CRCD(0xc3263e6e, "hang_critical_point_horiz_offset")) * m_facing;
	if (feeler.GetCollision())
	{
		// our feet have hit something; often this means the rail has dropped to ground level; drop from our old position
		m_pos = m_frame_start_pos;
		set_state(WALKING_AIR);
		m_primary_air_direction = m_facing;
		leave_movable_contact_for_air(m_horizontal_vel, m_vertical_vel);
		m_frame_event = CRCD(0x439f4704, "Air");
		return;
	}
	*/
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
		
void CWalkComponent::hang_movement ( float movement )
{
	// cache current critical point location and entire state
	Mth::Vector initial_critical_point = m_pos + m_critical_point_offset;
	Mth::Vector initial_pos = m_pos;
	float initial_along_rail_factor = m_along_rail_factor;
	const CRailNode* p_initial_rail_start = mp_rail_start;
	const CRailNode* p_initial_rail_end = mp_rail_end;
	Mth::Vector initial_critical_point_offset = m_critical_point_offset; 
	Mth::Vector initial_facing = m_facing; 
	
	// allow left/right movement along rail
	
	Mth::Vector perp_facing(-m_facing[Z], 0.0f, m_facing[X]);
	
	// the offset from rail start to rail end
	Mth::Vector rail = mp_rail_manager->GetPos(mp_rail_start) - mp_rail_manager->GetPos(mp_rail_end);
	
	// move left or right
	bool left = movement > 0.0f;
	
	// is rail fractional position parameter ascends or descends with the movement
	bool with_rail;
	if (Mth::DotProduct(perp_facing, rail) < 0.0f)
	{
		with_rail = left;
	}
	else
	{
		with_rail = !left;
	}

	// check for collisions
	if (check_environment_for_hang_movement_collisions(m_facing, m_along_rail_factor, m_pos, left, perp_facing, rail, mp_rail_start, mp_rail_end))
	{
		m_hang_move_vel = 0.0f;
		m_frame_event = CRCD(0x4194ecca, "Hang");
		return;
	}
	
	// the movement in terms of the fraction of the rail
	float delta_factor = Mth::Abs(movement) / rail.Length();
	
	// move along the rail
	m_along_rail_factor += with_rail ? delta_factor : -delta_factor;
	
	// if we're moving off the rail
	if (m_along_rail_factor < 0.0f || m_along_rail_factor > 1.0f)
	{
		bool found_rail = find_next_rail(m_along_rail_factor, mp_rail_start, mp_rail_end, mp_rail_start, mp_rail_end);
			
		// there's no connecting rail
		if (!found_rail)
		{
			// clamp the movement
			m_hang_move_vel = 0.0f;
			m_along_rail_factor = Mth::Clamp(m_along_rail_factor, 0.0f, 1.0f);
			m_frame_event = CRCD(0x4194ecca, "Hang");
		}
		else
		{
			// move onto the neighboring rail
			float remaining_movement = Mth::Abs(m_along_rail_factor - Mth::Clamp(m_along_rail_factor, 0.0f, 1.0f)) * rail.Length();
			move_onto_neighboring_hang_rail(remaining_movement, m_along_rail_factor < 0.0f, with_rail ? rail : -rail);
			m_frame_event = left ? CRCD(0x2d9815c3, "HangMoveLeft") : CRCD(0x279b1f0b, "HangMoveRight");
		}
	}
	else
	{
		m_frame_event = left ? CRCD(0x2d9815c3, "HangMoveLeft") : CRCD(0x279b1f0b, "HangMoveRight");
	}
	
	// check to see if the critical point has gone through geo
	
	Mth::Vector critical_point = m_pos + m_critical_point_offset;
	CFeeler feeler;
	feeler.m_start = initial_critical_point;
	feeler.m_end = critical_point;
	if (feeler.GetCollision())
	{
		// restore entire to state to initial state
		m_pos = initial_pos;
		DUMP_WPOSITION
		m_along_rail_factor = initial_along_rail_factor;
		mp_rail_start = p_initial_rail_start;
		mp_rail_end = p_initial_rail_end;
		m_critical_point_offset = initial_critical_point_offset;
		m_facing = initial_facing;
		
		m_hang_move_vel = 0.0f;
		m_frame_event = CRCD(0x4194ecca, "Hang");
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
		
bool CWalkComponent::check_environment_for_hang_movement_collisions ( const Mth::Vector& facing, float along_rail_factor, const Mth::Vector& pos, bool left, const Mth::Vector& perp_facing, Mth::Vector rail, const CRailNode* p_rail_start, const CRailNode* p_rail_end )
{
	// check environment in the appropriate check_direction for collisions which would prevent hanging movement
	// OPTIMIZATION: use a CCollCache here
	
	bool with_rail;
	if (Mth::DotProduct(perp_facing, rail) < 0.0f)
	{
		with_rail = left;
	}
	else
	{
		with_rail = !left;
	}
	
	// check_direction to the side to check for geo
	Mth::Vector check_direction = left ? perp_facing : -perp_facing;
	
	// adjusted facing due to adjusted check_direction
	Mth::Vector check_direction_facing = facing;

	// see if we're near the end of the rail; if so, we may want to angle our feelers out (so we can make inside turns while hanging)
	float distance_to_end_of_rail = rail.Length() * (with_rail ? 1.0f - along_rail_factor : along_rail_factor);
	if (distance_to_end_of_rail < s_get_param(CRCD(0x3344a6d0, "hang_move_collision_side_length")) + 1.0f)
	{
		// pretend we're past the rail when calling find_next_rail
		along_rail_factor += with_rail ? 2.0f : -2.0f;
		
		const CRailNode* p_next_start;
		const CRailNode* p_next_end;
		if (find_next_rail(along_rail_factor, p_rail_start, p_rail_end, p_next_start, p_next_end))
		{
			Mth::Vector new_check_direction = (along_rail_factor > 0.5f ? -1.0f : 1.0f)
				* (mp_rail_manager->GetPos(p_next_start) - mp_rail_manager->GetPos(p_next_end));
			new_check_direction[Y] = 0.0f;
			new_check_direction.Normalize();
			
			// only use adjusted feelers for inside turns
			float dot = Mth::DotProduct(facing, new_check_direction);
			if (dot < 0.0f && dot > -0.87f)
			{
				check_direction = new_check_direction;
				if (left)
				{
					check_direction_facing[X] = check_direction[Z];
					check_direction_facing[Z] = -check_direction[X];
				}
				else
				{
					check_direction_facing[X] = -check_direction[Z];
					check_direction_facing[Z] = check_direction[X];
				}
			}
		}
	}
	
	CFeeler feeler;
	feeler.m_start = pos;
	feeler.m_start[Y] += s_get_param(CRCD(0x84ff931c, "hang_move_collision_up"));
	feeler.m_start -= s_get_param(CRCD(0xcbeb3d89, "hang_move_collision_back")) * facing;
	
	// feel to the side
	feeler.m_end = feeler.m_start;
	feeler.m_end += s_get_param(CRCD(0x3344a6d0, "hang_move_collision_side_length")) * check_direction;
	if (feeler.GetCollision())
	{
		#ifdef __USER_DAN__
		if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
		{
			feeler.DebugLine(255, 0, 0, 1);
		}
		#endif
		return true;
	}
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
	{
		feeler.DebugLine(0, 0, 255, 1);
	}
	#endif
	
	// feel up and to the side
	feeler.m_end = feeler.m_start;
	feeler.m_end[Y] += s_get_param(CRCD(0xc774dd6d, "hang_move_collision_side_height"));
	feeler.m_end += s_get_param(CRCD(0x3344a6d0, "hang_move_collision_side_length")) * check_direction;
	if (feeler.GetCollision())
	{
		#ifdef __USER_DAN__
		if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
		{
			feeler.DebugLine(255, 0, 0, 1);
		}
		#endif
		return true;
	}
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
	{
		feeler.DebugLine(0, 0, 255, 1);
	}
	#endif
	
	// feel half up and to the side
	feeler.m_end = feeler.m_start;
	feeler.m_end[Y] += 0.5f * s_get_param(CRCD(0xc774dd6d, "hang_move_collision_side_height"));
	feeler.m_end += s_get_param(CRCD(0x3344a6d0, "hang_move_collision_side_length")) * check_direction;
	if (feeler.GetCollision())
	{
		#ifdef __USER_DAN__
		if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
		{
			feeler.DebugLine(255, 0, 0, 1);
		}
		#endif
		return true;
	}
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
	{
		feeler.DebugLine(0, 0, 255, 1);
	}
	#endif
	
	// feel down (to critical point) and to the side and back to the critical point
	feeler.m_end = feeler.m_start;
	feeler.m_end[Y] += -s_get_param(CRCD(0x84ff931c, "hang_move_collision_up")) + get_hang_critical_point_vert_offset() - 1.0f;
	feeler.m_end += s_get_param(CRCD(0xc3263e6e, "hang_critical_point_horiz_offset")) * check_direction_facing;
	feeler.m_end += s_get_param(CRCD(0x3344a6d0, "hang_move_collision_side_length")) * check_direction;
	 // add a touch along the rail so we don't get stuck with both directions feelers in the ground
	feeler.m_end += with_rail ? -rail.Normalize() : rail.Normalize();
	if (feeler.GetCollision())
	{
		#ifdef __USER_DAN__
		if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
		{
			feeler.DebugLine(255, 0, 0, 1);
		}
		#endif
		return true;
	}
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
	{
		feeler.DebugLine(0, 0, 255, 1);
	}
	#endif
	
	// feel half down and to the side and back to the critical point
	feeler.m_end = feeler.m_start;
	feeler.m_end[Y] -= 0.5f * s_get_param(CRCD(0xc774dd6d, "hang_move_collision_side_height"));
	feeler.m_end += s_get_param(CRCD(0xc3263e6e, "hang_critical_point_horiz_offset")) * check_direction_facing;
	feeler.m_end += s_get_param(CRCD(0x3344a6d0, "hang_move_collision_side_length")) * check_direction;
	if (feeler.GetCollision())
	{
		#ifdef __USER_DAN__
		if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
		{
			feeler.DebugLine(255, 0, 0, 1);
		}
		#endif
		return true;
	}
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
	{
		feeler.DebugLine(0, 0, 255, 1);
	}
	#endif
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::move_onto_neighboring_hang_rail ( float movement, bool descending, const Mth::Vector& old_rail )
{
	Mth::Vector next_rail = mp_rail_manager->GetPos(mp_rail_start) - mp_rail_manager->GetPos(mp_rail_end);
	float next_rail_length = next_rail.Length();
	
	// if the next rail is shorter than our movement, just ditch the extra movement
	movement = Mth::ClampMax(movement, next_rail_length);
	
	// we've found the appropriate neighbor rail to move on to, so set up our state
	
	// calculate our position along the rail
	m_along_rail_factor = movement / next_rail_length;
	if (descending)
	{
		m_along_rail_factor = 1.0f - m_along_rail_factor;
	}
	
	// calculate our next facing
	Mth::Vector next_facing(-next_rail[Z], 0.0f, next_rail[X]);
	next_facing.Normalize();
	
	// check its check_direction
	if (Mth::CrossProduct(old_rail, m_facing)[Y] * Mth::CrossProduct(descending ? -next_rail : next_rail, next_facing)[Y] < 0.0f)
	{
		next_facing.Negate();
	}
	
	if (Mth::DotProduct(next_facing, m_facing) < cosf(Mth::DegToRad(60.0f)))
	{
		// move the camera behind us
		mp_camera_component->FlushRequest();
	}
	m_facing = next_facing;
	
	m_critical_point_offset = s_get_param(CRCD(0xc3263e6e, "hang_critical_point_horiz_offset")) * m_facing;
	m_critical_point_offset[Y] = get_hang_critical_point_vert_offset();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::set_pos_from_hang_rail_state (   )
{
	m_pos = Mth::Lerp(mp_rail_manager->GetPos(mp_rail_start), mp_rail_manager->GetPos(mp_rail_end), m_along_rail_factor);
	m_pos[Y] += m_vertical_hang_offset;
	m_pos += m_horizontal_hang_offset * m_facing;
	DUMP_WPOSITION
	
	if (mp_movable_contact_component->HaveContact() && mp_movable_contact_component->GetContact()->IsRotated())
	{
		// recalculate facing
		Mth::Vector new_facing = mp_rail_manager->GetPos(mp_rail_start) - mp_rail_manager->GetPos(mp_rail_end);
		new_facing[Y] = 0.0f;
		new_facing.Normalize();
		new_facing.Set(-new_facing[Z], 0.0f, new_facing[X]);
		if (Mth::DotProduct(new_facing, m_facing) > 0.0f)
		{
			m_facing = new_facing;
		}
		else
		{
			m_facing = -new_facing;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::find_next_rail ( float& along_rail_factor, const CRailNode* p_current_start, const CRailNode* p_current_end, const CRailNode*& p_new_start, const CRailNode*& p_new_end )
{
	// find the neighboring rail, if there is one
	
	Dbg_Assert(along_rail_factor < 0.0f || along_rail_factor > 1.0f);
	
	const CRailNode* next_rail_node = NULL;
	if (along_rail_factor < 0.0f)
	{
		// look for a connection
		if (p_current_start->GetPrevLink() && p_current_start->GetPrevLink()->IsActive() && !p_current_start->GetPrevLink()->GetFlag(NO_CLIMBING)
			&& Rail_ValidInEditor(mp_rail_manager->GetPos(p_current_start->GetPrevLink()), mp_rail_manager->GetPos(p_current_start)))
		{
			p_new_end = p_current_start;
			p_new_start = p_current_start->GetPrevLink();
			// MESSAGE("FOUND CONNECTING RAIL");
			return true;
		}
		
		// look for another node who's position corresponds to the current start node
		if (!mp_rail_manager->CheckForCoincidentRailNode(p_current_start, nBit(NO_CLIMBING) | nBit(LADDER), &next_rail_node))
		{
			// MESSAGE("NO COINCIDENT RAIL");
			return false;
		}
	}
	else
	{
		// look for a connection
		if (p_current_end->GetNextLink() && p_current_end->IsActive() && !p_current_end->GetFlag(NO_CLIMBING)
			&& Rail_ValidInEditor(mp_rail_manager->GetPos(p_current_end), mp_rail_manager->GetPos(p_current_end->GetNextLink())))
		{
			p_new_start = p_current_end;
			p_new_end = p_current_end->GetNextLink();
			// MESSAGE("FOUND CONNECTING RAIL");
			return true;
		}
		
		// look for another node who's position corresponds a given node's position
		if (!mp_rail_manager->CheckForCoincidentRailNode(p_current_end, nBit(NO_CLIMBING) | nBit(LADDER), &next_rail_node))
		{
			// MESSAGE("NO COINCIDENT RAIL");
			return false;
		}
	}
	
	Dbg_Assert(next_rail_node && next_rail_node->GetNextLink());
	
	bool rails_are_flush = mp_rail_manager->RailNodesAreCoincident(next_rail_node, p_current_end)
		|| mp_rail_manager->RailNodesAreCoincident(next_rail_node->GetNextLink(), p_current_start);
    
	Mth::Vector old_rail_dir = (mp_rail_manager->GetPos(p_current_end) - mp_rail_manager->GetPos(p_current_start)).Normalize();
	Mth::Vector new_rail_dir = (mp_rail_manager->GetPos(next_rail_node->GetNextLink()) - mp_rail_manager->GetPos(next_rail_node)).Normalize();
	if (!rails_are_flush)
	{
		new_rail_dir *= -1.0f;
	}
	if (Mth::DotProduct(old_rail_dir, new_rail_dir) < -0.866f)
	{
		MESSAGE("BAD ANGLE ON COINCIDENT RAIL");
		return false;
	}
	
	if (!rails_are_flush)
	{
		along_rail_factor = 1.0f - along_rail_factor;
	}
	p_new_start = next_rail_node;
	p_new_end = next_rail_node->GetNextLink();
	MESSAGE("FOUND COINCIDENT RAIL");
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWalkComponent::maybe_pull_up_from_hang ( Mth::Vector& proposed_stand_pos )
{
	// check to see if there's a good place for us to stand if we pull up; if so, do so
	
	Mth::Vector stand_pos;
	
	CFeeler potential_ground_feeler;
	bool ground = determine_stand_pos(proposed_stand_pos, stand_pos, potential_ground_feeler);
	
	if (!ground)
	{
		Mth::Vector initial_stand_pos = stand_pos;
		Mth::Vector left(-m_facing[Z], 0.0f, m_facing[X]);
		
		// if we couldn't find the ground, we may be at the end of a rail which hangs over the geo; look for stand positions to the left and right
		const float SIDEWAYS_SEARCH_DISTANCE = 12.0f;
		proposed_stand_pos += SIDEWAYS_SEARCH_DISTANCE * left;
		ground = determine_stand_pos(proposed_stand_pos, stand_pos, potential_ground_feeler);
		if (!ground)
		{
			proposed_stand_pos -= (2.0f * SIDEWAYS_SEARCH_DISTANCE) * left;
			ground = determine_stand_pos(proposed_stand_pos, stand_pos, potential_ground_feeler);
			if (!ground)
			{
				stand_pos = initial_stand_pos;
			}
		}
	}
	
	// check for walls around the standing position
	CFeeler feeler;
	float push_feeler_length = 0.5f * s_get_param(CRCD(0xa20c43b7, "push_feeler_length"));
	float feeler_height = s_get_param(CRCD(0x6da7f696, "feeler_height"));
	for (int n = 0; n < vNUM_FEELERS; n++)
	{
		float angle = n * (2.0f * Mth::PI / vNUM_FEELERS);
		
		float cos_angle = cosf(angle);
		float sin_angle = sinf(angle);
		
		Mth::Vector end_offset;
		end_offset[X] = cos_angle * m_facing[X] - sin_angle * m_facing[Z];
		end_offset[Y] = 0.0f;
		end_offset[Z] = sin_angle * m_facing[X] + cos_angle * m_facing[Z];
		end_offset[W] = 1.0f;
		end_offset *= push_feeler_length;
		
		feeler.m_start = stand_pos;
		feeler.m_start[Y] += feeler_height;
		feeler.m_end = stand_pos + end_offset;
		feeler.m_end[Y] += feeler_height;
		
		#ifdef __USER_DAN__
		if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
		{
			feeler.DebugLine(0, 255, 255, 0);
		}
		#endif
		if (feeler.GetCollision()) return false;
	}
	
	// check line from hang point to standing position for obstructions
	feeler.m_start = m_pos;
	feeler.m_start += -6.0f * m_facing;
	feeler.m_start[Y] += s_get_param(CRCD(0xa8c13e74, "hang_vert_origin_offset")) + s_get_param(CRCD(0x9b2818cf, "pull_up_obstruction_height"));
	feeler.m_end = stand_pos;
	feeler.m_end[Y] += 1.0f;
	if (feeler.GetCollision())
	{
		#ifdef __USER_DAN__
		if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
		{
			feeler.DebugLine(0, 100, 100, 0);
		}
		#endif
		return false;
	}
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
	{
		feeler.DebugLine(100, 0, 100, 0);
	}
	#endif
	
	// pull up looks ok, so let's do it
	
	if (ground)
	{
		mp_anim_wait_complete_callback = &Obj::CWalkComponent::pull_up_from_hang_to_ground_complete;
		
		m_last_ground_feeler = potential_ground_feeler;
		m_last_ground_feeler_valid = true;
		m_ground_normal = potential_ground_feeler.GetNormal();
	}
	else
	{
		mp_anim_wait_complete_callback = &Obj::CWalkComponent::pull_up_from_hang_to_air_complete;
	}
	
	m_anim_wait_initial_pos = m_pos;
	m_anim_wait_goal_pos = stand_pos;

	calculate_anim_wait_facing_drift_parameters(m_facing);
	
	m_anim_wait_camera_mode = AWC_GOAL;
	
	m_drift_initial_display_offset = m_display_offset;
	m_drift_goal_display_offset = 0.0f;
						 	
	m_critical_point_offset = -(6.0f + s_get_param(CRCD(0x21dfbe77, "pull_up_offset_forward"))) * m_facing;
	m_critical_point_offset[Y] = 1.0f;
	
	// setup the false wall incase the player jumps out of the animation wait
	m_false_wall.normal = m_facing;
	m_false_wall.cancel_height = m_pos[Y] - m_vertical_hang_offset;

	set_state(WALKING_ANIMWAIT);

	m_frame_event = CRCD(0x9ef9f8f1, "PullUpFromHang");
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::pull_up_from_hang_to_ground_complete (   )
{
    set_state(WALKING_GROUND);
	
	if (m_last_ground_feeler_valid)
	{
		mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_ONTO, m_last_ground_feeler);

		// check for a moving contact
		mp_movable_contact_component->CheckForMovableContact(m_last_ground_feeler);
	}
	
	m_critical_point_offset.Set();
	
	m_display_offset = m_drift_goal_display_offset;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkComponent::pull_up_from_hang_to_air_complete (   )
{
    set_state(WALKING_AIR);
	
	m_primary_air_direction = m_facing;
	leave_movable_contact_for_air(m_horizontal_vel, m_vertical_vel);
	
	m_display_offset = m_drift_goal_display_offset;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CWalkComponent::get_hang_display_offset (   )
{
    if (mp_model_component->GetModel()->IsScalingEnabled())
	{
		return (1.0f - mp_model_component->GetModel()->GetScale()[Y]) * s_get_param(CRCD(0xa8c13e74, "hang_vert_origin_offset"));
	}
	else
	{
		return 0.0f;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CWalkComponent::get_hang_critical_point_vert_offset (   )
{
	return s_get_param(CRCD(0xde86132e, "hang_critical_point_vert_offset"))
		- s_get_param(CRCD(0x7201ad48, "max_cas_scaling")) * s_get_param(CRCD(0xa8c13e74, "hang_vert_origin_offset"));
}

}
