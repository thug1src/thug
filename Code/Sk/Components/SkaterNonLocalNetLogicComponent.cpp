//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterNonLocalNetLogicComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/11/3
//****************************************************************************

#include <core/math/slerp.h>

#include <gfx/baseanimcontroller.h>
#include <gfx/animcontroller.h>
#include <gfx/nxlight.h>
#include <gfx/nxmodel.h>
#include <gfx/nxflags.h>
								
#include <sk/components/skaternonlocalnetlogiccomponent.h>
#include <sk/components/skaterstatehistorycomponent.h>
#include <sk/components/skaterendruncomponent.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/components/skaterflipandrotatecomponent.h>
#include <sk/components/skaterstatecomponent.h>
#include <sk/components/skaterloopingsoundcomponent.h>
#include <sk/gamenet/gamenet.h>
#include <sk/modules/skate/gamemode.h>

#include <gel/object/compositeobject.h>
#include <gel/components/animationcomponent.h>
#include <gel/components/modelcomponent.h>
#include <gel/components/specialitemcomponent.h>
#include <gel/components/lockobjcomponent.h>
#include <gel/components/shadowcomponent.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/net/client/netclnt.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CSkaterNonLocalNetLogicComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterNonLocalNetLogicComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterNonLocalNetLogicComponent::CSkaterNonLocalNetLogicComponent() : CBaseComponent()
{
	SetType( CRC_SKATERNONLOCALNETLOGIC );
	
	mp_state_history_component = NULL;
	mp_state_component = NULL;
	mp_endrun_component = NULL;
	mp_animation_component = NULL;
	mp_model_component = NULL;
	mp_flip_and_rotate_component = NULL;
	mp_shadow_component = NULL;
	m_last_pos_index = 0;
	m_interp_pos = Mth::Vector( 0, 0, 0 );
	m_old_interp_pos = Mth::Vector( 0, 0, 0 );
	m_num_mags = 0;
	m_last_anm_update_time = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterNonLocalNetLogicComponent::~CSkaterNonLocalNetLogicComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterNonLocalNetLogicComponent::InitFromStructure( Script::CStruct* pParams )
{
	Dbg_MsgAssert(GetObject()->GetType() == SKATE_TYPE_SKATER, ("CSkaterNonLocalNetLogicComponent added to non-skater composite object"));
	Dbg_MsgAssert(!GetSkater()->IsLocalClient(), ("CSkaterNonLocalNetLogicComponent added to non-local skater"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterNonLocalNetLogicComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterNonLocalNetLogicComponent::Finalize (   )
{
	mp_state_history_component = GetSkaterStateHistoryComponentFromObject(GetObject());
	mp_state_component = GetSkaterStateComponentFromObject(GetObject());
	mp_endrun_component = GetSkaterEndRunComponentFromObject(GetObject());
	mp_animation_component = GetAnimationComponentFromObject(GetObject());
	mp_flip_and_rotate_component = GetSkaterFlipAndRotateComponentFromObject(GetObject());
	mp_model_component = GetModelComponentFromObject(GetObject());
	mp_shadow_component = GetShadowComponentFromObject(GetObject());
	
	Dbg_Assert(mp_state_history_component);
	Dbg_Assert(mp_state_component);
	Dbg_Assert(mp_endrun_component);
	Dbg_Assert(mp_animation_component);
	Dbg_Assert(mp_flip_and_rotate_component);
	Dbg_Assert(mp_model_component);
	Dbg_Assert(mp_shadow_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterNonLocalNetLogicComponent::Update()
{
	GameNet::PlayerInfo* local_player;
	GameNet::Manager* gamenet_man = GameNet::Manager::Instance();

	m_frame_length = Tmr::UncappedFrameLength();

	bool BeganFrameInLipState = mp_state_component->GetState() == LIP;
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();

	if( mp_state_component->GetState() != AIR )
	{
		mp_state_component->m_camera_display_normal = GetObject()->GetMatrix().GetUp();
		mp_state_component->m_camera_current_normal = GetObject()->GetMatrix().GetUp();
	}

	setup_brightness_and_shadow();             
	
	m_old_extrap_pos = GetObject()->m_pos;
	GetObject()->m_old_pos = GetObject()->m_pos;
	interpolate_client_position();
	// Only extrapolate if we can collide with other players. Otherwise, interpolation looks 
	// much better.
	local_player = gamenet_man->GetLocalPlayer();
	if( !local_player->IsObserving() &&
		!local_player->IsSurveying())
	{
		if( ( GameNet::Manager::Instance()->PlayerCollisionEnabled()) ||
			( skate_mod->GetGameMode()->GetNameChecksum() == CRCD(0xbff33600,"netfirefight")) ||
			( skate_mod->GetGameMode()->GetNameChecksum() == CRCD(0x3d6d444f,"firefight")))
		{
			extrapolate_client_position();
		}
	}
	do_client_animation_update();
	
	// NOTE: Below logic is copied from CSkaterFinalizePhysicsComponent and should instead be merged.
	
	// Logic for setting/not setting the flag for telling the camera whether to look down on the skater or not.
	if (BeganFrameInLipState && mp_state_component->GetState() != LIP)
	{
		// This flag is sort of badly named now, it really means we changed from the lip state to something other than GROUND or LIP.
		mp_state_component->mJumpedOutOfLipTrick = false;
		if (mp_state_component->GetState() == AIR)
		{
			// we only want to set this flag if we jumped straight up; meaning the x and z velocities are close to zero
			if (Mth::Abs(GetObject()->m_vel[X]) < 1.0f && Mth::Abs(GetObject()->m_vel[Z]) < 1.0f)
			{
				mp_state_component->mJumpedOutOfLipTrick = true;
			}				
		}
	}
	
	// this flag needs clearing whenever we get out of the air
	if (mp_state_component->GetState() != AIR)
	{
		mp_state_component->mJumpedOutOfLipTrick = false;
	}
	
	// take the non-local client in and out of the car
	if (mp_state_component->GetDriving() && mp_state_history_component->GetCurrentVehicleControlType() != 0)
	{
		Script::CStruct* pParams = new Script::CStruct;
		pParams->AddChecksum(CRCD(0x5b24faaa, "SkaterId"), GetObject()->GetID());
		pParams->AddChecksum(CRCD(0x81cff663, "control_type"), mp_state_history_component->GetCurrentVehicleControlType());
		RunScript(CRCD(0x7853f8c4, "NonLocalClientInVehicle"), pParams);
		delete pParams;
	}
	else if (!mp_state_component->GetDriving() && m_last_driving)
	{
		Script::CStruct* pParams = new Script::CStruct;
		pParams->AddChecksum(CRCD(0x5b24faaa, "SkaterId"), GetObject()->GetID());
		RunScript(CRCD(0xf02e44f, "NonLocalClientExitVehicle"), pParams);
		delete pParams;
	}
	m_last_driving = mp_state_component->GetDriving();
	
	// update the looping sound component
	CSkaterLoopingSoundComponent* p_looping_sound_component = GetSkaterLoopingSoundComponentFromObject(GetObject());
	Dbg_Assert(p_looping_sound_component);
	if (mp_state_component->m_physics_state == SKATING)
	{
		p_looping_sound_component->SetActive(true);
		float speed_fraction = sqrtf(GetObject()->m_vel[X] * GetObject()->m_vel[X] + GetObject()->m_vel[Z] * GetObject()->m_vel[Z]) / 1000.0f;
		p_looping_sound_component->SetSpeedFraction(speed_fraction);
		p_looping_sound_component->SetState(mp_state_component->m_state);
		p_looping_sound_component->SetTerrain(mp_state_component->m_terrain);
		p_looping_sound_component->SetIsBailing(mp_state_component->GetFlag(IS_BAILING));
		p_looping_sound_component->SetIsRailSliding(mp_state_component->GetFlag(RAIL_SLIDING));
	}
	else
	{
		p_looping_sound_component->SetActive(false);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterNonLocalNetLogicComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterNonLocalNetLogicComponent::GetDebugInfo(Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterNonLocalNetLogicComponent::GetDebugInfo"));

	CBaseComponent::GetDebugInfo(p_info);	  
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterNonLocalNetLogicComponent::Resync (   )
{
	m_client_initial_update_time = 0;
	m_last_anm_update_time = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterNonLocalNetLogicComponent::snap_to_ground( void  )
{
	float up_dot = 0.0f;
	Mth::Vector col_start, col_end;
	CFeeler feeler;
	
	// Since we really don't want to loose contact with the ground while skitching, we use a much bigger snap up dist
	// The problem will come when we get dragged down a slope.  The car will flatten out well ahead of us, so pushing us down through the slop
	// (as we are a few feet behind it) and we will be so far under the ground that our normal snap up will not be able to dig us out of it,
	// so we go in air, uberfrig, and get dragged to a random spot under the level.
	// (This would not happen if we just skitch on flat ground)																								
	// if (mp_state_component->m_skater_flags[SKITCHING].Get())
	// {
		// col_start = GetObject()->m_matrix[Y] * GetPhysicsFloat(CRCD(0x5c0d9610,"Physics_Ground_Snap_Up_SKITCHING"));	// much above feet
	// }
	// else
	// {
		col_start = GetObject()->m_matrix[Y] * GetPhysicsFloat(CRCD(0xe4d79235, "Physics_Ground_Snap_Up"));	  		// bit above the feet
	// }
	
	col_end = GetObject()->m_matrix[Y] * -200.0f;		    // WAY! below the feet, we check distance later

	col_start += GetObject()->m_pos;
	col_end += GetObject()->m_pos;
		 
	bool sticking = false;			
	
	feeler.SetLine( col_start, col_end );
	feeler.SetIgnore( mFD_NON_COLLIDABLE, 0 );
	// get disatnce to ground and snap the skater to it, but only if the ground is skatable, otherwise we just go to "AIR"
	if( feeler.GetCollision())
	{
		Mth::Vector movement = GetObject()->m_pos - feeler.GetPoint(); 
		uint16 flags = feeler.GetFlags();
		float drop_dist = movement.Length();
		float drop_sign = Mth::DotProduct(GetObject()->m_matrix[Y], movement); // might be approx +/- 0.00001f
		if(	(!flags & mFD_SKATABLE ) || ( flags & mFD_NOT_SKATABLE ) || (!flags & mFD_VERT) || 
			(flags & mFD_WALL_RIDABLE) ||
			(feeler.GetNormal()[Y] < sinf(Mth::DegToRad(GetPhysicsFloat(CRCD(0x3eede4d3,"Wall_Non_Skatable_Angle"))))))
		{
			// if below the face (or very close to it), then push us away from it				
			if (drop_sign < 0.001f)
			{
				// at point of contact, and move away from surface
				GetObject()->m_pos = feeler.GetPoint() + feeler.GetNormal();	
			}
		}
		else
		{
			sticking = true;
			
			// Note the two ways of calculating the angle between two faces
			// the more "accurate" method simply takes angle between the two normals
			// however, this fails to account for the direction the skater is travelling
			// when in conjunction with the "snap" up.
			// If the skater approaches a slope from the side, then he can snap up to the slope
			// however, if the angle between the ground and the face to which we are snapping up to 
			// is too great, then we transition to in-air
			// and will drop through the slope
			// the solution is to take the angle between the front vector rotated onto each face.
			
			// Firstly we check the angle between the two faces	
			Mth::Vector normal = feeler.GetNormal();
			
			Mth::Vector	forward = GetObject()->m_matrix[Z];
			float front_dot = Mth::DotProduct(forward,normal);

			Mth::Vector	old_forward = forward;
			forward.RotateToPlane(normal);
			old_forward.RotateToPlane(m_current_normal);

			// angle between front vectors, projected onto faces
			up_dot = Mth::DotProduct(forward, old_forward);
			
			float stick_angle_cosine;
			stick_angle_cosine = cosf(Mth::DegToRad(GetPhysicsFloat(CRCD(0x4138283e, "Ground_stick_angle_forward"))));
			
			if (front_dot > 0.0f  && up_dot > 0.0f && up_dot < stick_angle_cosine)
			{
				sticking = false;
			}
			else
			{
				// only need to test if snap is downwards
				if (drop_sign > 0.0f)
				{
					// angle between the old plane and the new plane is either the same, or withing the set limits
					// (like, < 60 degrees, or so, see physics.q) now calculate the drop distance, based on the angle
					
					//float max_drop = 60.0f;////last_move.Length() * tanf(angle);
#ifdef __PLAT_NGC__
					float angle = acosf(Mth::Clamp(up_dot, -1.0f, 1.0f));
#else
					float angle = acosf(up_dot);
#endif // __PLAT_NGC__

					Mth::Vector	last_move = GetObject()->m_pos - m_old_extrap_pos;
					
					float max_drop = last_move.Length() * tanf(angle);
					
					float min_drop = GetPhysicsFloat(CRCD(0x899ba3d0, "Physics_Ground_Snap_Down"));				
                    // if (mp_state_component->m_skater_flags[SKITCHING].Get())
					// {
						// min_drop = GetPhysicsFloat(CRCD(0x20df7e33, "Physics_Ground_Snap_Down_Skitching"));				
					// }

					if (max_drop < min_drop)
					{
						max_drop = min_drop;
					}
					
					if (drop_dist > max_drop)
					{
						sticking = false;
					}
				}
				
				if (sticking)
				{    
					//new_normal(normal);
					m_current_normal = normal;	  										// remember this, for detecting if it changes
		
					GetObject()->m_matrix[Y] = normal;
					GetObject()->m_matrix.OrthoNormalizeAbout(Y);									// set regular normal immediately
				}		 
			}																		  
		}
					
		if (sticking)
		{	
			// if there is a collision, then snap to it
			GetObject()->m_pos = feeler.GetPoint();
		}
	}
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CRailNode*	CSkaterNonLocalNetLogicComponent::travel_on_rail( CRailNode* start_node, float frame_length )
{
	bool on_rail;

	Dbg_Assert( start_node );

	on_rail = true;

	CRailManager* p_rail_man = Mdl::Skate::Instance()->GetRailManager();

	const CRailNode* pStart = start_node;
	const CRailNode* pEnd = pStart->GetNextLink();
	if( pEnd == NULL )
	{
		Dbg_MsgAssert(pEnd,("NonLocal Skater on Rail node (%d) with no next link\n",start_node->GetNode()));
		return NULL;
	}
	
	//const CRailNode* pFrom = pStart;
	const CRailNode* pOnto = NULL;
	
	Mth::Vector	dir = p_rail_man->GetPos(pEnd) - p_rail_man->GetPos(pStart);
	float segment_length = dir.Length();
	dir *= (1.0f / segment_length);

	// sign is which way we are going along the rail
	//float old_sign = Mth::Sgn(Mth::DotProduct(dir, GetObject()->m_vel));

	// Get gravity force 	
	Mth::Vector gravity(0.0f, GetPhysicsFloat(CRCD(0xd1f46992, "Physics_Rail_Gravity")), 0.0f);

	// Project gravity onto the line we are on
	gravity.ProjectToNormal(dir);
	GetObject()->m_vel += gravity * frame_length;
	
	// sign is which way we are going along the rail
	float sign = Mth::Sgn(Mth::DotProduct(dir, GetObject()->m_vel));
	
	// check to see if we are on the last segment of the rail
	// this logic could be folded into the logic below but it's perhps clearer to have it here
	bool last_segment = false;
	if (sign < 0.0f)
	{
		if (pStart->GetPrevLink() && pStart->GetPrevLink()->IsActive())
		{
			Mth::Vector v1, v2;
			v1 = p_rail_man->GetPos(pEnd) - p_rail_man->GetPos(pStart);
			v2 = p_rail_man->GetPos(pStart) - p_rail_man->GetPos(pStart->GetPrevLink());
			v1[Y] = 0.0f;
			v2[Y] = 0.0f;
			v1.Normalize();
			v2.Normalize();
			float dot = Mth::DotProduct(v1, v2);
			if (dot < cosf(Mth::DegToRad(GetPhysicsFloat(CRCD(0x76c1da15, "Rail_Corner_Leave_Angle")))))
			{
				// there is more, but angle is too sharp
				last_segment = true;
			}
			else
			{
				pOnto = pStart->GetPrevLink();
			}
		}
		else
		{
			last_segment = true;			
		}
	}
	else
	{
		if (pEnd && pEnd->GetNextLink() && pEnd->GetNextLink()->IsActive())
		{
			Mth::Vector  v1,v2;
			v1 = p_rail_man->GetPos(pStart) - p_rail_man->GetPos(pEnd);
			v2 = p_rail_man->GetPos(pEnd) - p_rail_man->GetPos(pEnd->GetNextLink());
			v1[Y] = 0.0f;
			v2[Y] = 0.0f;
			v1.Normalize();
			v2.Normalize();
			float dot = Mth::DotProduct(v1,v2);
			if (dot < cosf(Mth::DegToRad(GetPhysicsFloat(CRCD(0x76c1da15,"Rail_Corner_Leave_Angle")))))
			{
				// there is more, but angle is too sharp
				last_segment = true;
			}
			else
			{
				pOnto = pEnd;
			}
		}
		else
		{
			last_segment = true;
		}
	}

	// Now we need to see if we have gone beyond the end of the rail segment
	
	float extra_dist = 0.0f;
		
	float length_along;
	if (sign < 0.0f)
	{
		// going backwards, so it's the distance from the end of the segment
		length_along = (GetObject()->m_pos - p_rail_man->GetPos(pEnd)).Length();
	}
	else
	{
		// going forwards, so it's the distance from the start
		length_along = (GetObject()->m_pos - p_rail_man->GetPos(pStart)).Length();
	}
	
	if (length_along > segment_length + 0.1f)	// 0.1 inch, so we don't get stuck
	{
		// remember this, so we can move along next segment
		extra_dist = length_along - segment_length;
		
		// gone off the end of the segment
		if (sign < 0.0f)
		{
			if (pStart->GetPrevLink()
				&& pStart->GetPrevLink()->IsActive()
				&& Rail_ValidInEditor(p_rail_man->GetPos(pStart), p_rail_man->GetPos(pStart->GetPrevLink())))
			{
				if (!last_segment)
				{
					// go onto previous segment
					GetObject()->m_pos = p_rail_man->GetPos(pStart);
					start_node = (CRailNode*) pStart->GetPrevLink();
					//set_terrain(start_node->GetTerrain());
					//maybe_trip_rail_trigger(TRIGGER_SKATE_ONTO);
				}		   
				else
				{
					//skate_off_rail(p_rail_man->GetPos(pStart));
					on_rail = false;
				} 		
			}
			else
			{
				//skate_off_rail(p_rail_man->GetPos(pStart));
				on_rail = false;
			}
		}
		else
		{
			if (pEnd->GetNextLink()
				&& pEnd->IsActive()
				&& Rail_ValidInEditor(p_rail_man->GetPos(pEnd), p_rail_man->GetPos(pEnd->GetNextLink())))
			{
				if (!last_segment)
				{
					GetObject()->m_pos = p_rail_man->GetPos(pEnd);
					start_node = (CRailNode*) pEnd;					
					//set_terrain(start_node->GetTerrain());
					//maybe_trip_rail_trigger(TRIGGER_SKATE_ONTO);						
				}		   
				else
				{
					//skate_off_rail(p_rail_man->GetPos(pEnd));
					on_rail = false;
				} 						
			}
			else
			{
				//skate_off_rail(p_rail_man->GetPos(pEnd));
				on_rail = false;
			}
		}		
	}
	
	if( on_rail ) // If still on the rail     GetState() == RAIL)
	{
		// recalculate start, end, dir, as we might be on a new segment
		const CRailNode* pStart = start_node;
		const CRailNode* pEnd = pStart->GetNextLink();
		
		Mth::Vector	dir = p_rail_man->GetPos(pEnd) - p_rail_man->GetPos(pStart);
		dir.Normalize();

		// sign also may have changed, now that we are auto-linking rail segments
		
		// sign is which way we are going along the rail
		float sign = Mth::Sgn(Mth::DotProduct(dir,GetObject()->m_vel));

		//m_rail_time = Tmr::GetTime();
							
		GetObject()->m_vel.RotateToNormal(dir);
		GetObject()->m_vel *= sign;						   						// sign won't be on a new segment

		//float facing_sign = mRail_Backwards ? -sign : sign;
		float facing_sign = sign;
		
		// z is forward
		Mth::Vector target_forward = dir * facing_sign;

		//m_lerping_display_matrix[Z] = Mth::Lerp(m_lerping_display_matrix[Z], target_forward, 0.3f);
		//m_lerping_display_matrix[Z].Normalize(); 
		
		#if 0 // old code
		// m_lerping_display_matrix[Y].Set(0.0f, 1.0f, 0.0f);
		// m_lerping_display_matrix[X] = Mth::CrossProduct(
			// m_lerping_display_matrix[Y],
			// m_lerping_display_matrix[Z]
		// );
		#else
		//m_lerping_display_matrix[X].Set(
			//m_lerping_display_matrix[Z][Z],
			//0.0f,
			//-m_lerping_display_matrix[Z][X],
			//0.0f
		//);
		#endif
		//m_lerping_display_matrix[X].Normalize();
		
		//m_lerping_display_matrix[Y] = Mth::CrossProduct(
			//m_lerping_display_matrix[Z],
			//m_lerping_display_matrix[X]
		//);
		//m_lerping_display_matrix[Y].Normalize();

		// adjust our Z value towards the new value												 
		GetObject()->m_matrix[Z] = target_forward;
		GetObject()->m_matrix[Z].Normalize(); 
		
		#if 0 // old code
		// GetObject()->m_matrix[Y].Set(0.0f, 1.0f, 0.0f);
		// GetObject()->m_matrix[X] = Mth::CrossProduct(GetObject()->m_matrix[Y],GetObject()->m_matrix[Z]);
		#else
		GetObject()->m_matrix[X].Set(
			GetObject()->m_matrix[Z][Z],
			0.0f,
			-GetObject()->m_matrix[Z][X],
			0.0f
		);
		#endif
		GetObject()->m_matrix[X].Normalize();
		
		GetObject()->m_matrix[Y] = Mth::CrossProduct(GetObject()->m_matrix[Z], GetObject()->m_matrix[X]);
		GetObject()->m_matrix[Y].Normalize();

        // This is where we do the actual movement
		// if this makes us bump into the wall or the ground, then we should leave the rail
		GetObject()->m_pos += GetObject()->m_vel * frame_length;			// current movement
		GetObject()->m_pos += extra_dist * target_forward;						// any extra dist from previous segment
	}
	else
	{
		return NULL;
	}

	return start_node;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterNonLocalNetLogicComponent::extrapolate_rail_position( void )
{
	GameNet::Manager* gamenet_man =  GameNet::Manager::Instance();
	CRailManager* p_rail_man = Mdl::Skate::Instance()->GetRailManager();
	CRailNode* start_node;
	int i, prev_index, most_recent_index, mag_index;
	SPosEvent* p_pos_history;
	unsigned int delta_t, net_lag;
	float coeff, ratio, total_mag, total_ratio, total_coeff, vel_mag, frame_length;
	GameNet::PlayerInfo* local_player;
	Net::Client* client;
	Mth::Vector vel;

	client = gamenet_man->GetClient( 0 );

	p_pos_history = mp_state_history_component->GetPosHistory();
	most_recent_index = ( mp_state_history_component->GetNumPosUpdates() - 1 ) % CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS;
	prev_index = ( m_last_pos_index + ( CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS - 1 )) % CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS;
	
	delta_t = (int) ( m_frame_length * Tmr::vRESOLUTION );
	if( gamenet_man->OnServer())
	{
		GameNet::PlayerInfo* player;
		Net::Conn* client_conn;

		player = gamenet_man->GetPlayerByObjectID( GetObject()->GetID());
		Dbg_Assert( player );
		client_conn = player->m_Conn;
		net_lag = client_conn->GetAveLatency();
	}
	else
	{
		Lst::Search< Net::Conn > sh;
		Net::Conn* server_conn;

		server_conn = client->FirstConnection( &sh );
		net_lag = server_conn->GetAveLatency();
	}

	// protect against dbz
	if( delta_t == 0 )
	{
		ratio = 0;
	}
	else
	{
		ratio = (float) ( net_lag - delta_t ) / (float) delta_t;
		if( ratio < 0 )
		{
			ratio = 0;
		}
	}
	
	mag_index = m_num_mags % vMAG_HISTORY_LENGTH;
	vel = m_interp_pos - m_old_interp_pos;

	coeff = 0.0f;
	local_player = gamenet_man->GetLocalPlayer();
	if( local_player )
	{
		Mth::Vector my_vel, their_vel;

		my_vel = local_player->m_Skater->GetVel();
		their_vel = vel;

		my_vel.Normalize();
		their_vel.Normalize();

		// Get the dot product of our movement vectors
		coeff = Mth::DotProduct( my_vel, their_vel );

		// This will result in a value between -1.0f and 1.0f. Adding one will make that
		// between 0 and 2
		coeff += 1.0f;

		// We want a value between 0 and 1, so divide by 2
		coeff /= 2.0f;
	}

	m_mag_history[mag_index] = ( vel.Length() / delta_t ) * 16;
	m_ratio_history[mag_index] = ratio;
	m_extrap_coeff_history[mag_index] = coeff;
	m_num_mags++;

	total_mag = 0;
	total_ratio = 0;
	total_coeff = 0;
	for( i = 0; i < vMAG_HISTORY_LENGTH; i++ )
	{
		if( i >= m_num_mags )
		{
			break;
		}
		total_mag += m_mag_history[i];
		total_ratio += m_ratio_history[i];
		total_coeff += m_extrap_coeff_history[i];
		
	}
	vel_mag = total_mag / i;
	coeff = total_coeff / i;
	ratio = total_ratio / i;
	ratio *= coeff;

	vel.Normalize();
	vel *= vel_mag;
	
	frame_length = m_frame_length * ratio;
	// Sanity check to avoid infinite loops
	if( frame_length > 1.0f )
	{
		frame_length = 1.0f;
	}
	start_node = p_rail_man->GetRailNodeByNodeNumber( p_pos_history[m_last_pos_index].RailNode );
	if( start_node )
	{
		while( frame_length > 0 )
		{
			if( frame_length > m_frame_length )
			{
				start_node = travel_on_rail( start_node, m_frame_length );
			}
			else
			{
				start_node = travel_on_rail( start_node, frame_length );
			}
			
			// If we grinded off a rail, use standard extrapolation instead.
			if( start_node == NULL )
			{
				GetObject()->m_pos = m_interp_pos;
				extrapolate_position();
				break;
			}
			frame_length -= m_frame_length;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CSkaterNonLocalNetLogicComponent::rotate_away_from_wall ( Mth::Vector normal, float &turn_angle )
{
	float lerp;

	lerp = 1.0f;
	// given a wall normal, then calculate the "turn_angle" to rotate the skater by and rotate the matrix and display matrix by this
	// return the angle between the skater and the wall
	// note turn_angle is passed by reference, and is altered !!!!
	
	// given m_right(dot)normal, we should be able to get a nice angle
	float dot_right_normal = Mth::DotProduct(GetObject()->m_matrix[X], normal);

	float angle = acosf(Mth::Clamp(dot_right_normal, -1.0f, 1.0f)); 	

	if (angle > Mth::PI / 2.0f)
	{
		angle -= Mth::PI;
	}
	
	// angle away from the wall
	turn_angle = angle * GetPhysicsFloat(CRCD(0xe07ee1a9, "Wall_Bounce_Angle_Multiplier")) * lerp;
	
	// Rotate the skater so he is at a slight angle to the wall, especially if we are in a right angled corner, where the skater will bounce out

	GetObject()->m_vel.RotateY(turn_angle);
	GetObject()->m_matrix.RotateYLocal(turn_angle);
	
	return angle;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterNonLocalNetLogicComponent::bounce_off_wall ( const Mth::Vector& normal, const Mth::Vector& point )
{
	// Given the normal of the wall, then bounce off it, turning the skater away from the wall
	Mth::Vector col_start, col_end;
	Mth::Vector	forward = GetObject()->m_pos - m_old_extrap_pos;
	Mth::Vector movement = forward;				   		// remember how far we moved
	CFeeler feeler;

    forward.Normalize();
	
	Mth::Vector up_offset = GetObject()->m_matrix[Y] * GetPhysicsFloat(CRCD(0xd4205c9b,"Skater_First_Forward_Collision_Height"));

	float turn_angle;
	float angle = rotate_away_from_wall(normal, turn_angle);

	float min = Mth::DegToRad(GetPhysicsFloat(CRCD(0x1483fd01, "Wall_Bounce_Dont_Slow_Angle"))); 
					 							
	if (Mth::Abs(angle) > min)
	{
		//float old_speed = GetObject()->m_vel.Length();
		
		// The maximum value of Abs(angle) would be PI/2 (90 degrees), so scale the velocity 
		float x = Mth::Abs(angle) - min;
		x /= (Mth::PI / 2.0f) - min;		// get in the range 0 .. 1
		x = 1.0f - x; 						// invert, as we want to stop when angle is 90
		
		GetObject()->m_vel *= x;
	}
	
	// Bit of a patch here to move the skater away from the wall
	// Not needed so much with new sideways collision checks but we keep it in for low ledges.
	// Should perhaps standardize the height so collision checks for side and front but we'd probably still have problems.
	
	GetObject()->m_pos = point - up_offset;
	GetObject()->m_pos += normal * 6.0f;
	
	// Now the majority of cases have been taken care of; we need to see if the skater is going to get stuck in a corner.

	float old_speed = movement.Length();		 					// get how much we moved last time
	Mth::Vector next_movement = GetObject()->m_vel;					// get new direction of velocity
	forward = GetObject()->m_vel;
	forward.Normalize();
	next_movement = forward * old_speed;							// extend by same movment as last time
	
	col_start = GetObject()->m_pos + up_offset;
	col_start += up_offset;
	
	col_end = GetObject()->m_pos + up_offset + next_movement
		+ forward * GetPhysicsFloat(CRCD(0x20102726, "Skater_First_Forward_Collision_Length"));
	
	// Sanity check to make sure we're not using a line that's a whole level long
	if(( col_start - col_end ).Length() > FEET( 20 ))
	{
		return;
	}

	feeler.SetLine( col_start, col_end );
	feeler.SetIgnore( mFD_NON_COLLIDABLE, 0 );
	if( feeler.GetCollision() && GetSkater()->IsLocalClient())
	{
		// Just rotating the skater will lead to another collision, so try just inverting the skater's velocity from it's original and halving it....
		
		// First reverse the rotation, and rotate 180 degrees
		GetObject()->m_vel.RotateY(Mth::DegToRad(180.0f) - turn_angle);
		GetObject()->m_matrix.RotateYLocal(Mth::DegToRad(180.0f) - turn_angle);
		//ResetLerpingMatrix();
		
		GetObject()->m_vel *= 0.5f; 
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterNonLocalNetLogicComponent::extrapolate_position( void )
{
	GameNet::Manager* gamenet_man =  GameNet::Manager::Instance();
	int i, prev_index, most_recent_index, mag_index;
	SPosEvent* p_pos_history;
	unsigned int delta_t, net_lag;
	float coeff, ratio, total_mag, total_ratio, total_coeff, vel_mag;
	Mth::Vector extrap_pos;
	GameNet::PlayerInfo* local_player;
	Mth::Vector vel;
	Net::Client* client;

	client = gamenet_man->GetClient( 0 );
		
	p_pos_history = mp_state_history_component->GetPosHistory();
	most_recent_index = ( mp_state_history_component->GetNumPosUpdates() - 1 ) % CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS;
	prev_index = ( m_last_pos_index + ( CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS - 1 )) % CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS;
	
	delta_t = (int) ( m_frame_length * Tmr::vRESOLUTION );
	if( gamenet_man->OnServer())
	{
		GameNet::PlayerInfo* player;
		Net::Conn* client_conn;

		player = gamenet_man->GetPlayerByObjectID( GetObject()->GetID());
		Dbg_Assert( player );
		client_conn = player->m_Conn;
		net_lag = client_conn->GetAveLatency();
	}
	else
	{
		Lst::Search< Net::Conn > sh;
		Net::Conn* server_conn;

		server_conn = client->FirstConnection( &sh );
		net_lag = server_conn->GetAveLatency();
	}

	// protect against dbz
	if( delta_t == 0 )
	{
		ratio = 0;
	}
	else
	{
		ratio = (float) ( net_lag - delta_t ) / (float) delta_t;
		if( ratio < 0 )
		{
			ratio = 0;
		}
	}
	
	mag_index = m_num_mags % vMAG_HISTORY_LENGTH;
	vel = m_interp_pos - m_old_interp_pos;

	coeff = 0.0f;
	local_player = gamenet_man->GetLocalPlayer();
	if( local_player && local_player->m_Skater )
	{
		Mth::Vector my_vel, their_vel;

		my_vel = local_player->m_Skater->GetVel();
		their_vel = vel;

		my_vel.Normalize();
		their_vel.Normalize();

		// Get the dot product of our movement vectors
		coeff = Mth::DotProduct( my_vel, their_vel );

		// This will result in a value between -1.0f and 1.0f. Adding one will make that
		// between 0 and 2
		coeff += 1.0f;

		// We want a value between 0 and 1, so divide by 2
		coeff /= 2.0f;
	}
	else
	{
		coeff = 0.0f;
	}

	m_mag_history[mag_index] = ( vel.Length() / delta_t ) * 16;
	m_ratio_history[mag_index] = ratio;
	m_extrap_coeff_history[mag_index] = coeff;
	m_num_mags++;

	total_mag = 0;
	total_ratio = 0;
	total_coeff = 0;
	for( i = 0; i < vMAG_HISTORY_LENGTH; i++ )
	{
		if( i >= m_num_mags )
		{
			break;
		}
		total_mag += m_mag_history[i];
		total_ratio += m_ratio_history[i];
		total_coeff += m_extrap_coeff_history[i];
		
	}
	vel_mag = total_mag / i;
	coeff = total_coeff / i;
	ratio = total_ratio / i;
	ratio *= coeff;

	vel.Normalize();
	vel *= vel_mag;
	
	extrap_pos = GetObject()->m_pos + ( vel * ratio );
	extrap_pos[Y] = GetObject()->m_pos[Y];
	GetObject()->m_pos = extrap_pos;
	
	Mth::Vector up_offset = GetObject()->m_matrix[Y] * GetPhysicsFloat(CRCD(0xd4205c9b, "Skater_First_Forward_Collision_Height"));

	Mth::Vector col_start = m_interp_pos + up_offset;
	Mth::Vector col_end = GetObject()->m_pos + up_offset;

	CFeeler feeler;

	// Sanity check to make sure we're not using a line that's a whole level long
	if(( col_start - col_end ).Length() > FEET( 10 ))
	{
		return;
	}

	feeler.SetLine( col_start, col_end );
	feeler.SetIgnore( mFD_NON_COLLIDABLE, 0 );

	if( feeler.GetCollision())
	{
		bounce_off_wall( feeler.GetNormal(), feeler.GetPoint());
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterNonLocalNetLogicComponent::extrapolate_client_position (   )
{
	if( mp_state_history_component->GetNumPosUpdates() >= CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS )
	{
		if( mp_state_component->GetState() == RAIL )
		{
			extrapolate_rail_position();
		}
		else
		{
			extrapolate_position();
			snap_to_ground();
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterNonLocalNetLogicComponent::interpolate_client_position (   )
{
	GameNet::Manager* gamenet_man =  GameNet::Manager::Instance();
	Net::Client* client;
	
	SPosEvent* p_pos_history = mp_state_history_component->GetPosHistory();

	if(( client = gamenet_man->GetClient( 0 )))
	{   
		// Wait for the object update buffer to fill up
		if( mp_state_history_component->GetNumPosUpdates() >= CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS )
		{
			unsigned int cur_time;
			int i, prev_index, start_index, total_t, delta_t;
			float ratio;
			Mth::Vector delta_pos;
            
			// This will be true only if we've never performed an interpolation
			if( m_client_initial_update_time == 0 )
			{
				//Dbg_Printf( "Resync'd Skater %d\n", GetID() );
				// start off at the latest update and offset ourselves backwards in time from there
				start_index = ( mp_state_history_component->GetNumPosUpdates() - 1 ) % CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS;
				
				m_client_initial_update_time = client->m_Timestamp;
				// impose a lag of N frames
				m_server_initial_update_time = p_pos_history[start_index].GetTime() - Tmr::VBlanksF( vNUM_LAG_FRAMES );
				m_last_pos_index = start_index;
                
				//Dbg_Printf( "(%d) server initial time = %d\n", nonlocalnet->m_FrameCounter, (int) m_server_initial_update_time );
				//Dbg_Printf( "(%d) nonlocalnet initial time = %d\n", nonlocalnet->m_FrameCounter, (int) m_client_initial_update_time );
				
                GetObject()->SetMatrix( p_pos_history[start_index].Matrix );
				GetObject()->SetDisplayMatrix( p_pos_history[start_index].Matrix );
				GetObject()->SetPos(p_pos_history[start_index].Position);
				GetObject()->SetTeleported(); 
				GetObject()->m_vel.Set(0.0f, 0.0f, 0.0f);

				// We don't want to actually render the skaters until we have buffered
				// up their position updates and are ready to start interpolating. Now we have
				// that data, so add them to the world
				if( !GetSkater()->IsInWorld())
				{
					GameNet::PlayerInfo* player;
                    
					GetSkater()->AddToCurrentWorld();
					Script::RunScript( "NetIdle", NULL, GetObject() );
					//Hide( false );
					player = gamenet_man->GetPlayerByObjectID( GetObject()->GetID() );
					Dbg_Assert( player );
					
                    // We're just adding them to the world.  If they were standing still,
					// UberFrig would not have been done. So let's do it here
					setup_brightness_and_shadow();
				}
			}
			else
			{   
				Mth::SlerpInterpolator slerp;

				Mth::Matrix interp_matrix;
				int most_recent_index, flag;
				GameNet::PlayerInfo* player;
				
				// Maybe a little bit overkill, but this will definitely set skaters that we can
				// see in the level to be "fully in", which triggers things like their names in
				// the score list
				player = gamenet_man->GetPlayerByObjectID( GetObject()->GetID() );
				if( !player->IsFullyIn())
				{
					player->MarkAsFullyIn();
				}

				start_index = mp_state_history_component->GetNumPosUpdates() % CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS;
				most_recent_index = ( mp_state_history_component->GetNumPosUpdates() - 1 ) % CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS;
				
				cur_time = m_server_initial_update_time + ( client->m_Timestamp - m_client_initial_update_time );

				// If we've gone past the most recent update, slow down a little to allow updates
				// to catch up
				//Dbg_Printf( "(%d) NonLocalNet Timestamp %d\n", nonlocalnet->m_FrameCounter, nonlocalnet->m_Timestamp );
				if( cur_time > p_pos_history[most_recent_index].GetTime())
				{
					// If we have some major catching up to do (usually at the beginning
					// when there are long pauses after loading) catch up in one step
					if(( cur_time - p_pos_history[most_recent_index].GetTime()) > Tmr::Seconds( 1 ))
					{
						//Dbg_Printf( "************************ (%d) Major catchup\n", client->m_FrameCounter );
						m_server_initial_update_time -= ( cur_time - p_pos_history[most_recent_index].GetTime());
					}
					else
					{
						//Dbg_Printf( "************************ (%d) (%d) Slowdown\n", GetObject()->GetID(), client->m_FrameCounter );
						m_server_initial_update_time -= Tmr::VBlanks( 1 );
					}
					
					cur_time = m_server_initial_update_time + ( client->m_Timestamp - m_client_initial_update_time );
					//Dbg_Printf( "- Slowdown\n" );
				}

				i = start_index;
				do							   
				{   
					if( cur_time <= p_pos_history[i].GetTime()) 
					{						 
						static int lag = 0;
						
						m_last_pos_index = i;
						lag = ( start_index + (CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS - i )) % CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS;
						
						// We probably also should adjust if we get too far behind
						if( lag >= 10 )
						{   
							//Dbg_Printf( "************************ (%d) (%d) +7 Catchup. Lag : %d\n", client->m_FrameCounter, GetObject()->GetID(), lag );
							m_server_initial_update_time += Tmr::VBlanks( 10 );
						}
						else if( lag == 0 )
						{
							//Dbg_Printf( "************************ (%d) Resync'ing skater %d\n", client->m_FrameCounter, GetObject()->GetID() );
							GetSkater()->Resync();
						}

						prev_index = ( i + ( CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS - 1 )) % CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS;
						
						delta_t = cur_time - p_pos_history[prev_index].GetTime();
						total_t = p_pos_history[i].GetTime() - p_pos_history[prev_index].GetTime();
						
						ratio = (float) delta_t / (float) total_t;
						
						delta_pos = p_pos_history[i].Position - p_pos_history[prev_index].Position;

						m_old_interp_pos = m_interp_pos;
						
						// Slerp between the two matrices
						Mth::Matrix t1, t2;
						Mth::Vector diff;

						diff = p_pos_history[i].Matrix[Z] - p_pos_history[prev_index].Matrix[Z];
						// Don't slerp if the matrices are way different. Just stick with the matrix
						// we have until the switch is made. 
						if( diff.Length() > 0.75f )
						{
							GetObject()->SetMatrix( p_pos_history[prev_index].Matrix );
							GetObject()->SetDisplayMatrix( p_pos_history[prev_index].Matrix );
						}
						else
						{
							t1 = p_pos_history[prev_index].Matrix;
							t2 = p_pos_history[i].Matrix;
							slerp.setMatrices( &t1, &t2 );
							slerp.getMatrix( &interp_matrix, ratio );
							
							GetObject()->SetMatrix( interp_matrix );
							GetObject()->SetDisplayMatrix( interp_matrix );
						}
						
						if( delta_pos.Length() > FEET( 50 ))
						{
							GetObject()->m_pos = p_pos_history[i].Position;
							GetObject()->m_vel = Mth::Vector( 0, 0, 0 );
							GetObject()->SetTeleported();
						}
						else
						{
							int idx, mag_index;
							Mth::Vector total_vel;

							GetObject()->m_pos = p_pos_history[prev_index].Position + ( delta_pos * ratio );
							GetObject()->m_vel = ( GetObject()->m_pos - m_old_interp_pos ) / m_frame_length; // do_nonlocalnet_position_update, calculating speed
						
							mag_index = m_num_mags % vMAG_HISTORY_LENGTH;

							// Smooth out changes in velocity
							m_vel_history[mag_index] = GetObject()->m_vel;
							for( idx = 0; idx < vMAG_HISTORY_LENGTH; idx++ )
							{
								if( idx >= m_num_mags )
								{
									break;
								}
								
								total_vel += m_vel_history[idx];
							}

							if( idx > 0 )
							{
								GetObject()->m_vel = total_vel / idx;
							}
						}

						m_interp_pos = GetObject()->m_pos;

						// Apply the state/flag data from this PosEvent
						mp_state_component->m_state = static_cast< EStateType >( p_pos_history[i].State );
						mp_state_component->m_doing_trick = p_pos_history[i].DoingTrick;
						mp_state_component->m_terrain = p_pos_history[i].Terrain;
						mp_state_component->m_physics_state = p_pos_history[i].Walking ? WALKING : SKATING;
						mp_state_component->m_driving = p_pos_history[i].Driving;
						mp_endrun_component->SetFlags(  p_pos_history[i].EndRunFlags );
						if( p_pos_history[i].EndRunFlags & CSkaterEndRunComponent::FINISHED_END_OF_RUN )
						{
							//Dbg_Printf( "*** At END OF RUN ***\n" );
						}
						
						for( flag = 0; flag < NUM_ESKATERFLAGS; flag++ )
						{
							mp_state_component->m_skater_flags[flag].Set(p_pos_history[i].SkaterFlags.Test( flag ));
						}
						if( mp_state_component->m_skater_flags[SPINE_PHYSICS].Get())
						{
							mp_state_component->m_spine_vel = GetObject()->m_vel;
						}
						else
						{
							mp_state_component->m_spine_vel.Set(0.0f, 0.0f, 0.0f);
						}

						break;
					}
					i = ( i + 1 ) % CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS;
				} while( i != start_index );
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterNonLocalNetLogicComponent::do_client_animation_update(   )
{
	GameNet::Manager* gamenet_man = GameNet::Manager::Instance();
	Net::Client* client;

	Dbg_Assert( mp_animation_component );

	if(( client = gamenet_man->GetClient( 0 )))
	{   
		// Wait for the object update buffer to fill up
		if( mp_state_history_component->GetNumPosUpdates() >= CSkaterStateHistoryComponent::vNUM_POS_HISTORY_ELEMENTS )
		{
			unsigned int cur_time;
			int i, start_index;
			SAnimEvent* event;
			int most_recent_index;
            
			start_index = mp_state_history_component->GetNumAnimUpdates() % CSkaterStateHistoryComponent::vNUM_ANIM_HISTORY_ELEMENTS;
			most_recent_index = ( mp_state_history_component->GetNumAnimUpdates() - 1 ) % CSkaterStateHistoryComponent::vNUM_ANIM_HISTORY_ELEMENTS;
			
			cur_time = m_server_initial_update_time + ( client->m_Timestamp - m_client_initial_update_time );

			i = start_index;
			bool updated = false;
			do							   
			{   
				event = &mp_state_history_component->GetAnimHistory()[i];
				//Dbg_Printf( "(%d) CurTime: %d EventTime[%d]: %d\n", client->m_FrameCounter, cur_time, event->m_MsgId, event->GetTime());
				if( ( event->GetTime() > m_last_anm_update_time ) &&
					( event->GetTime() <= cur_time ))
				{   
					updated = true;
					switch( event->m_MsgId )
					{
						case GameNet::MSG_ID_PRIM_ANIM_START:
						{
							//Dbg_Printf( "(%d) Playing primary anim: 0x%x Speed: %f Duration: %d Time:%d\n", client->m_Timestamp, event->m_Asset, event->m_Speed, event->m_Duration, event->GetTime());
							mp_animation_component->PlayPrimarySequence( event->m_Asset, false, event->m_StartTime, event->m_EndTime,
																		 (Gfx::EAnimLoopingType) event->m_LoopingType, 
																		 event->m_BlendPeriod, event->m_Speed );
	
							break;
						}
						case GameNet::MSG_ID_SET_WOBBLE_TARGET:
						{
							mp_animation_component->SetWobbleTarget( event->m_Alpha, false );
							break;
						}
						case GameNet::MSG_ID_ROTATE_SKATEBOARD:
						{
							mp_flip_and_rotate_component->RotateSkateboard( event->m_ObjId, event->m_Rotate, 0, false );
							break;
						}
						case GameNet::MSG_ID_SET_WOBBLE_DETAILS:
						{
							Gfx::SWobbleDetails wobbleDetails;
				
							wobbleDetails.wobbleAmpA = event->m_WobbleAmpA;
							wobbleDetails.wobbleAmpB = event->m_WobbleAmpB;
							wobbleDetails.wobbleK1 = event->m_WobbleK1;
							wobbleDetails.wobbleK2 = event->m_WobbleK2;
							wobbleDetails.spazFactor = event->m_SpazFactor;

							mp_animation_component->SetWobbleDetails( wobbleDetails, false );
							break;
						}
						case GameNet::MSG_ID_SET_LOOPING_TYPE:
						{
							mp_animation_component->SetLoopingType( (Gfx::EAnimLoopingType) event->m_LoopingType, false );
							break;
						}
						case GameNet::MSG_ID_SET_HIDE_ATOMIC:
						{
						   	mp_model_component->HideGeom( event->m_Asset, event->m_Hide, false );
							break;
						}
						case GameNet::MSG_ID_SET_ANIM_SPEED:
						{
							mp_animation_component->SetAnimSpeed( event->m_Speed, false );
							break;
						}
						case GameNet::MSG_ID_FLIP_ANIM:
						{
							mp_animation_component->FlipAnimation( event->m_ObjId, event->m_Flipped, 0, false );
							break;
						}
						case GameNet::MSG_ID_ROTATE_DISPLAY:
						{
							mp_model_component->m_display_rotation_offset.Set(0,30,0);
							Tmr::Time start_time=Tmr::ElapsedTime(0);
			
							if( event->m_Flags & 1 )
							{
								mp_model_component->mpDisplayRotationInfo[0].SetUp(event->m_Duration,
															   start_time,
															   event->m_StartAngle,
															   event->m_DeltaAngle,
															   event->m_SinePower,
															   event->m_HoldOnLastAngle);
							}	
							if( event->m_Flags & 2 )
							{
								mp_model_component->mpDisplayRotationInfo[1].SetUp(event->m_Duration,
															   start_time,
															   event->m_StartAngle,
															   event->m_DeltaAngle,
															   event->m_SinePower,
															   event->m_HoldOnLastAngle);
							}	
							if( event->m_Flags & 4 )
							{
								mp_model_component->mpDisplayRotationInfo[2].SetUp(event->m_Duration,
															   start_time,
															   event->m_StartAngle,
															   event->m_DeltaAngle,
															   event->m_SinePower,
															   event->m_HoldOnLastAngle);
							}	
							break;
						}
						case (char) GameNet::MSG_ID_CLEAR_ROTATE_DISPLAY:
						{
							mp_model_component->mpDisplayRotationInfo[0].Clear();
							mp_model_component->mpDisplayRotationInfo[1].Clear();
							mp_model_component->mpDisplayRotationInfo[2].Clear();
							break;
						}
						case GameNet::MSG_ID_CREATE_SPECIAL_ITEM:
						{
							CSpecialItemComponent* pSpecialItemComponent = GetSpecialItemComponentFromObject(GetObject());
							Dbg_MsgAssert( pSpecialItemComponent, ( "No special item component?" ) );

							Script::CStruct* pSpecialItemParams = Script::GetStructure( event->m_Asset, Script::ASSERT );
							CCompositeObject* pSpecialItemObject = pSpecialItemComponent->CreateSpecialItem( event->m_Index, pSpecialItemParams );
								
							Script::CStruct* pLockParams = new Script::CStruct;
							// pass along any bone or offset parameters...
							//pLockParams->AppendStructure( pParams );
							pLockParams->AddChecksum( CRCD(0xcab94088,"bone"), event->m_Bone );
							pLockParams->AddChecksum( CRCD(0x40c698af,"id"), GetObject()->GetID() );
								
							// component-based
							CLockObjComponent* pLockObjComponent = GetLockObjComponentFromObject( pSpecialItemObject );
							Dbg_MsgAssert( pLockObjComponent, ( "No lockobj component" ) );
							pLockObjComponent->InitFromStructure( pLockParams );				
												
							delete pLockParams;
						}
						break;

						case GameNet::MSG_ID_DESTROY_SPECIAL_ITEM:
						{
							CSpecialItemComponent* pSpecialItemComponent = GetSpecialItemComponentFromObject(GetObject());
                            pSpecialItemComponent->DestroySpecialItem( event->m_Index );
							break;
						}
					}
				}
				i = ( i + 1 ) % CSkaterStateHistoryComponent::vNUM_ANIM_HISTORY_ELEMENTS;
			} while( i != start_index );
			
			/*static Tmr::Time s_time = 0;

			if( !updated )
			{
				if(( Tmr::GetTime() - s_time ) > 1000 )
				{
					i = start_index;
					do							   
					{   
						event = &mp_state_history_component->GetAnimHistory()[i];
						Dbg_Printf( "(%d) CurTime: %d EventTime[%d]: %d\n", client->m_FrameCounter, cur_time, event->m_MsgId, event->GetTime());
						i = ( i + 1 ) % CSkaterStateHistoryComponent::vNUM_ANIM_HISTORY_ELEMENTS;
					} while( i != start_index );
					s_time = Tmr::GetTime();
				}
			}*/
            
			m_last_anm_update_time = cur_time;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
					
void CSkaterNonLocalNetLogicComponent::setup_brightness_and_shadow (   )
{
	// logic extracted from CAdjustComponent::uber_frig which is judged to be required by nonlocal clients
	
	CFeeler feeler;
	
	feeler.m_start = GetObject()->m_pos;
	feeler.m_end = GetObject()->m_pos;

	// Very minor adjustment to move origin away from vert walls
	feeler.m_start += GetObject()->m_matrix[Y] * 0.001f;
	
	feeler.m_start[Y] += 8.0f;
	feeler.m_end[Y] -= FEET(400);
		   
	if (!feeler.GetCollision()) return;
	
	if (mp_state_component->GetState() != RAIL && feeler.IsBrightnessAvailable())
	{
		Nx::CModelLights *p_model_lights = mp_model_component->GetModel()->GetModelLights();
		if (p_model_lights)
		{
			p_model_lights->SetBrightness(feeler.GetBrightness());
		} 
		else
		{
			// Garrett: This should move to CModel eventually
			Nx::CLightManager::sSetBrightness(feeler.GetBrightness());
		}
	}
	
	// Store these values off for the simple shadow calculation.
	mp_shadow_component->SetShadowPos(feeler.GetPoint());
	mp_shadow_component->SetShadowNormal(feeler.GetNormal()); 
}

}
