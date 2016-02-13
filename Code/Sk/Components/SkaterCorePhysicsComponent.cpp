//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterCorePhysicsComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/21/3
//****************************************************************************

#include <sk/objects/rail.h>
#include <sk/objects/skatercareer.h>
#include <sk/objects/gap.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/components/skatersoundcomponent.h>
#include <sk/components/skaterrotatecomponent.h>
#include <sk/components/skaterscorecomponent.h>
#include <sk/components/skatermatrixqueriescomponent.h>
#include <sk/components/skaterendruncomponent.h>
#include <sk/components/skaterbalancetrickcomponent.h>
#include <sk/components/skaterloopingsoundcomponent.h>
#include <sk/components/skaterstatecomponent.h>
#include <sk/components/skaterflipandrotatecomponent.h>
#include <sk/components/skaterphysicscontrolcomponent.h>
#include <sk/components/skatergapcomponent.h>

#include <gel/objtrack.h>
#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/components/inputcomponent.h>
#include <gel/components/skitchcomponent.h>
#include <gel/components/triggercomponent.h>
#include <gel/components/trickcomponent.h>
#include <gel/components/railmanagercomponent.h>
#include <gel/components/motioncomponent.h>
#include <gel/components/shadowcomponent.h>
#include <gel/components/movablecontactcomponent.h>
#include <gel/components/walkcomponent.h>
#include <gel/collision/collcache.h>

#include <sk/engine/contact.h>
#include <sk/modules/skate/skate.h>
#include <sk/parkeditor2/parked.h>
#include <sk/gamenet/gamenet.h>

#include <core/math/slerp.h>

#define	FLAGEXCEPTION(X) GetObject()->SelfEvent(X)
							
void	TrackingLine(int type, Mth::Vector &start, Mth::Vector &end)
{
	if	(Script::GetInt(CRCD(0x19fb78fa,"output_tracking_lines")))
	{
//		Gfx::AddDebugLine(start, end, 0xffffff);
		printf ("Tracking%d %.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",type, start[X], start[Y], start[Z], end[X], end[Y], end[Z]);
	}
}
 
extern bool g_CheatsEnabled;

namespace Obj
{
	Mth::Vector acid_hold;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CSkaterCorePhysicsComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterCorePhysicsComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterCorePhysicsComponent::CSkaterCorePhysicsComponent() : CBaseComponent()
{
	SetType( CRC_SKATERCOREPHYSICS );
	
	mp_input_component = NULL;
	mp_trigger_component = NULL;
	mp_sound_component = NULL;
	mp_trick_component = NULL;
	mp_rotate_component = NULL;
	mp_score_component = NULL;
	mp_balance_trick_component = NULL;
	mp_state_component = NULL;
	mp_movable_contact_component = NULL;
	mp_physics_control_component = NULL;
	mp_walk_component = NULL;
	
	mp_coll_cache = Nx::CCollCacheManager::sCreateCollCache();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterCorePhysicsComponent::~CSkaterCorePhysicsComponent()
{
	Nx::CCollCacheManager::sDestroyCollCache(mp_coll_cache);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::InitFromStructure( Script::CStruct* pParams )
{
	Dbg_MsgAssert(GetObject()->GetType() == SKATE_TYPE_SKATER, ("CSkaterCorePhysicsComponent added to non-skater composite object"));
	
	m_rolling_friction = GetPhysicsFloat(CRCD(0x78f80ec4, "Physics_Rolling_Friction"));
	
    GetPos().Set(0.0f, 0.0f, 0.0f);
	DUMP_POSITION;

	GetMatrix().Identity();
    ResetLerpingMatrix();

	m_display_normal.Set(0.0f, 1.0f, 0.0f);
	m_current_normal = m_display_normal;
	m_last_display_normal = m_display_normal;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::Finalize (   )
{
	mp_input_component = GetInputComponentFromObject(GetObject());
	mp_sound_component = GetSkaterSoundComponentFromObject(GetObject());
	mp_trigger_component = GetTriggerComponentFromObject(GetObject());
	mp_trick_component = GetTrickComponentFromObject(GetObject());
	mp_rotate_component = GetSkaterRotateComponentFromObject(GetObject());
	mp_score_component = GetSkaterScoreComponentFromObject(GetObject());
	mp_balance_trick_component = GetSkaterBalanceTrickComponentFromObject(GetObject());
	mp_state_component = GetSkaterStateComponentFromObject(GetObject());
	mp_movable_contact_component = GetMovableContactComponentFromObject(GetObject());
	mp_physics_control_component = GetSkaterPhysicsControlComponentFromObject(GetObject());
	mp_walk_component = GetWalkComponentFromObject(GetObject());
	
	Dbg_Assert(mp_input_component);
	Dbg_Assert(mp_sound_component);
	Dbg_Assert(mp_trigger_component);
	Dbg_Assert(mp_trick_component);
	Dbg_Assert(mp_rotate_component);
	Dbg_Assert(mp_score_component);
	Dbg_Assert(mp_balance_trick_component);
	Dbg_Assert(mp_movable_contact_component);
	Dbg_Assert(mp_physics_control_component);
	Dbg_Assert(mp_walk_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::Update()
{
	DUMP_POSITION;
	
	m_frame_length = Tmr::FrameLength();
	
	// bool up = GetVel()[Y] > 0.0f;

	m_landed_this_frame = false;

	m_began_frame_in_lip_state = GetState() == LIP;
	m_began_frame_in_transfer = GetFlag(SPINE_PHYSICS);
	
	handle_tensing();

	limit_speed();

	SetFlagFalse(SNAPPED_OVER_CURB);  // this flag only gets set for one frame, to fix camera snaps
	SetFlagFalse(SNAPPED);  // this flag only gets set for one frame, to fix camera snaps
	
	setup_default_collision_cache();
	
	switch (GetState())
	{
		case GROUND:
			// Mick: Remember the last ground position for calculating which side of the rail we're on later.
			// Note, we do this BEFORE we move as the movement might take us off the ground
			m_last_ground_pos = GetPos();
			do_on_ground_physics();
			maybe_skitch();
			break;
			
		case AIR:
			do_in_air_physics();
			if (GetState() == GROUND)
			{
				m_landed_this_frame = true;
			}
			break;
			
		case WALL:
			do_wallride_physics();
			break;
			
		case LIP:
			do_lip_physics();
			break;
			
		case RAIL:
			do_rail_physics();
			break;
			
		case WALLPLANT:
			do_wallplant_physics();
			break;
	}
	
	handle_post_transfer_limit_overrides();
	
	// NOTE: moved this call from after CSkaterRotateComponent::Update to before it
	maybe_stick_to_rail();
	
	update_special_friction_index();
	
	// if (up && GetVel()[Y] < 0.0f)
	// {
		// DUMPF(GetPos()[Y]);
	// }
	
	if (m_vert_air_last_frame != (GetState() == AIR && GetFlag(VERT_AIR) && !GetFlag(SPINE_PHYSICS)))
	{
		m_vert_air_last_frame = !m_vert_air_last_frame;
        GetObject()->BroadcastEvent(m_vert_air_last_frame ? CRCD(0xf225fe69, "SkaterEnterVertAir") : CRCD(0x5e27200a, "SkaterExitVertAir"));
	}
	
	#ifdef __USER_DAN__
	// Gfx::AddDebugArrow(GetPos(), GetPos() + 60.0f * GetMatrix()[Z], RED, 0, 1);
	// Gfx::AddDebugArrow(GetPos(), GetPos() + 60.0f * GetMatrix()[X], BLUE, 0, 1);
	// Gfx::AddDebugArrow(GetPos(), GetPos() + 60.0f * GetMatrix()[Y], GREEN, 0, 1);
	#endif
	
	CFeeler::sClearDefaultCache();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterCorePhysicsComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		// @script | Jump | 
        // @flag BonelessHeight | 
		case CRCC(0x584cf9e9, "Jump"):
			do_jump(pParams);
			break;
			
        // @script | Flipped | true if flipped
		case CRCC(0xc7a712c, "Flipped"):
			return GetFlag(FLIPPED) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
        // @script | Switched | true if switched
		case CRCC(0x8f66b80b, "Switched"):
			return IsSwitched() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
        // @script | Crouched | true if skater crouched
		case CRCC(0x4adc6c2a, "Crouched"):
			return GetFlag(TENSE) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
        // @script | OnGround | true if current state is on ground
		case CRCC(0x5ea287f2, "OnGround"):
			return GetState() == GROUND ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
        // @script | InAir | true if current state is in air
		case CRCC(0x3527fc07, "InAir"):
			return GetState() == AIR ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
        // @script | OnWall | true if current state is on wall
		case CRCC(0xa32c1a15, "OnWall"):
			return GetState() == WALL ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
        // @script | OnLip | true if current state is on lip
		case CRCC(0x5cb1fbd8, "OnLip"):
			return GetState() == LIP ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
        // @script | OnRail | true if current state is on rail
		case CRCC(0xe9851e62, "OnRail"):
			return GetState() == RAIL ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
        // @script | InWallplant | true if current state is in wallplant
		case CRCC(0xa64dcf8b, "InWallplant"):
			return GetState() == WALLPLANT ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;

		// @script | FirstTimeOnThisRail | true if this is the first time we grinded this rail wihout doing something else
		// like skating or wallriding
		case CRCC(0x262d42d5,"FirstTimeOnThisRail"):
			return GetFlag(NEW_RAIL) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;

		// @script | StartSkitch | Start skitch physics
		// This will send a SkitchOn exception to the object
		case CRCC(0xca6b3809, "StartSkitch"):
			start_skitch();
			break;

        // @script | Skitching | Returns True if we are skitching
		case CRCC(0x9c6a7e41, "Skitching"):
			return GetFlag(SKITCHING) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;

        // @script | StopSkitch | Stop Skitch physics
		// this wills send a SkitchOff exeception to the object
		case CRCC(0x32dcc9cf, "StopSkitch"):
			StopSkitch();
			break;
			
		// @script | CancelWallpush | Cancels the current wallpush event
		case CRCC(0x5e8f9e77, "CancelWallpush"):
			SetFlagTrue(CANCEL_WALL_PUSH);
			break;
			 
        // @script | AirTimeLessThan | true if the air time is
        // less than the specified time
        // @uparm 1.0 | time (default is milliseconds)
        // @flag seconds | time in seconds
        // @flag frames | time in frames
        case CRCC(0xc890a84, "AirTimeLessThan"):
        // @script | AirTimeGreaterThan | true if the air time is 
        // greater than the specified time
        // @uparm 1.0 | time (default is milliseconds)
        // @flag seconds | time in seconds
        // @flag frames | time in frames
		case CRCC(0xbbf2b570, "AirTimeGreaterThan"):
		{
			float t = 0;
			pParams->GetFloat(NO_NAME, &t);
	
			Tmr::Time TestTime;
			if (pParams->ContainsFlag(CRCD(0xd029f619, "seconds")) || pParams->ContainsFlag(CRCD(0x49e0ee96, "second")))
			{
				TestTime = static_cast< Tmr::Time >(t * 1000);
			}	
			else if (pParams->ContainsFlag(CRCD(0x19176c5, "frames")) || pParams->ContainsFlag(CRCD(0x4a07c332, "frame")))
			{
				TestTime = static_cast< Tmr::Time >(t * (1000 / 60));
			}
			else
			{
				TestTime = static_cast< Tmr::Time >(t);
			}

			Tmr::Time AirTime;
			if (GetState() == AIR || GetState() == WALL)
			{
				AirTime = Tmr::ElapsedTime(m_went_airborne_time);
			}
			else
			{
				AirTime = m_landed_time - m_went_airborne_time;
			}	
			
			if (Checksum == CRCD(0xc890a84, "AirTimeLessThan"))
			{
				return AirTime < TestTime ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			}
			else
			{
				return AirTime > TestTime ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			}	
		}

		// @script | GetAirTime | Puts the air time, in seconds, into a param called airtime
		case CRCC(0xde583e34, "GetAirTime"):
		{
			Tmr::Time AirTime;
			if (GetState() == AIR || GetState() == WALL)
			{
				AirTime = Tmr::ElapsedTime(m_went_airborne_time);
			}
			else
			{
				AirTime = m_landed_time - m_went_airborne_time;
			}	
			pScript->GetParams()->AddFloat(CRCD(0xad6bcdb4, "AirTime"), AirTime * (1.0f / 1000.0f));	
			break;
		}
		
		// @script | GetAirTimeLeft | Puts the amount of air time left before landing, in seconds, into a param called AirTimeLeft
		case CRCC(0x1996b797, "GetAirTimeLeft"):
		{
			pScript->GetParams()->AddFloat(CRCD(0x7e2a8993, "AirTimeLeft"),
				Mth::ClampMin(calculate_time_to_reach_height(GetPos()[Y] - mp_state_component->m_height, GetPos()[Y], GetVel()[Y]), 0.0f));
			break;
		}
						
        // @script | Braking | true if skater is braking
		case CRCC(0x1f8bbd05, "Braking"):
			return is_braking() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;

		case CRCC(0xebbcf455, "CanBrakeOff"):
			m_pressing_down_brakes = false;
			break;

		case CRCC(0xbc19e291, "CanBrakeOn"):
			m_pressing_down_brakes = true;
			break;
			
        // @script | CanKick | true if skater can kick
		case CRCC(0x2f66333, "CanKick"):
			return can_kick() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;

        // @script | CanKickOn | sets can kick to true
		case CRCC(0x68bf6c13, "CanKickOn"):
			m_force_cankick_off = false;
			break;
			
        // @script | CanKickOff | sets can kick to false
		case CRCC(0xe8deb0d7, "CanKickOff"):
			m_force_cankick_off = true;
			break;
		
        // @script | ForceAutokickOn | turns on auto kick
		case CRCC(0x34dcfc97, "ForceAutokickOn"):
			m_auto_kick = true;
			break;

        // @script | ForceAutoKickOff | turns off auto kick
		case CRCC(0x257947e, "ForceAutokickOff"):
			m_auto_kick = false;
			break;

	    // @script | AutoKickIsOff | true if auto kick is off
		case CRCC(0x1baa1d9, "AutoKickIsOff"):
			return m_auto_kick == false ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
        // @script | RestoreAutokick | restores auto kick to 
        // original preferences
		case CRCC(0x9fcfdfeb, "RestoreAutokick"):
			m_auto_kick = Mdl::Skate::Instance()->mp_controller_preferences[GetSkater()->m_skater_number].AutoKickOn;
			break;
			
        // @script | DoCarPlantBoost | boost after doing car plant
		case CRCC(0x17846595, "DoCarPlantBoost"):
		{
			GetVel()[Y] = Mth::ClampMin(GetVel()[Y], 0.0f);
			GetVel()[Y] += GetPhysicsFloat(CRCD(0xcb49b3f2, "Carplant_upward_boost"));	
			
			DUMP_VELOCITY;
			
			Mth::Vector front = GetVel();
			front[Y] = 0.0f;
			if (front.LengthSqr() < 10.0f * 10.0f)
			{
				front = GetMatrix()[Z];				
				front[Y] = 0.0f;
				if (front.LengthSqr() < 0.01f * 0.01f)
				{
					front.Set(1.0f, 0.0f, 1.0f);
				}
			}
			front.Normalize();
			GetVel() += front * GetPhysicsFloat(CRCD(0xb7422173, "Carplant_forward_boost"));
			
			DUMP_VELOCITY;
			break;
		}
		
		// @script | HandleLipOllieDirection |
		case CRCC(0x259cb90d, "HandleLipOllieDirection"):
		{
			CControlPad& control_pad = mp_input_component->GetControlPad();
			
			Mth::Vector out = GetMatrix()[Z];
			out[Y] = 0.0f;
			out.Normalize();	 
			Mth::Vector along(out[Z], 0.0f, -out[X], 0.0f);
											  
			// We've jumped off a lip, so we want to give us some velocity to the left
			if (control_pad.m_right.GetPressed())
			{
				// don't allow right jumping
				return CBaseComponent::MF_FALSE;
			}
			else if (BREAK_SPINE_BUTTONS)
			{
				GetVel() += out * GetPhysicsFloat(CRCD(0x1f96c224, "Lip_side_hop_speed"));
				DUMP_VELOCITY;
				return CBaseComponent::MF_TRUE;
			} 
			else if (control_pad.m_left.GetPressed()
				&& static_cast< int >(control_pad.m_left.GetPressedTime()) > GetPhysicsInt(CRCD(0x3080e9e7, "Lip_held_jump_out_time")))
			{
				GetVel() += out * GetPhysicsFloat(CRCD(0xdc818b7e, "Lip_side_jump_speed")); 
				DUMP_VELOCITY;
				return CBaseComponent::MF_TRUE;
			} 
			if (control_pad.m_up.GetPressed()
				&& static_cast< int >(control_pad.m_up.GetPressedTime()) > GetPhysicsInt(CRCD(0x9b25142a, "Lip_held_jump_along_time")))
			{
				GetVel() -= along * GetPhysicsFloat(CRCD(0x1ab3809f, "Lip_along_jump_speed")); 
				DUMP_VELOCITY;
				return CBaseComponent::MF_TRUE;
			}
			else if (control_pad.m_down.GetPressed()
				&& static_cast< int >(control_pad.m_down.GetPressedTime()) > GetPhysicsInt(CRCD(0x9b25142a,"Lip_held_jump_along_time")))
			{
				GetVel() += along * GetPhysicsFloat(CRCD(0x1ab3809f, "Lip_along_jump_speed"));
				DUMP_VELOCITY;
				return CBaseComponent::MF_TRUE;
			} 
			else
			{
				return CBaseComponent::MF_FALSE;
			}
		}  	
			
        // @script | OrientToNormal | 
		case CRCC(0x1d1fd4f0, "OrientToNormal"):
			m_display_normal = GetMatrix()[Y];
			m_current_normal = GetMatrix()[Y];
			new_normal(m_feeler.GetNormal());
			break;
			
        // @script | SetSpeed | 
        // @uparm 0.0 | speed in inches per second, so 3000 is very very fast
		// The skater's max speed is about 1100 inches per second (depends on stats)
		// this is usually used in conjunction with Overridelimits, to provide a temporary speed boost
		case CRCC(0x383b939b, "SetSpeed"):
		{
			float Speed = 0.0f;
			pParams->GetFloat(NO_NAME, &Speed);
			float length_sqr = GetVel().LengthSqr();
			if (length_sqr < 0.001f)
			{
				GetVel() = GetMatrix()[Z];
			}
			else
			{
				GetVel() *= 1.0f / sqrtf(length_sqr);
			}
			GetVel() *= Speed;
			DUMP_VELOCITY;
			break;
		}
		
        // @script | NoSpin | sets flag, disabling spin
		case CRCC(0x54ef2b79, "NoSpin"):
			mNoSpin = true;
			SetFlagFalse(AUTOTURN);
			break;

        // @script | CanSpin | sets flag enabling spin
		case CRCC(0xe2998b9, "CanSpin"):
			mNoSpin = false;
			break;
		
        // @script | InBail | sets is_bailing flag 
		case CRCC(0x6fc5aae0, "InBail"):
		{
			SetFlagTrue(IS_BAILING);
			break;
		}
		
        // @script | NotInBail | clears is_bailing flag
		case CRCC(0xbd4303f4, "NotInBail"):
		{
			SetFlagFalse(IS_BAILING);
			break;
		}
			
        // @script | IsInBail | true if bailing (as defined by is_bailing flag)
		case CRCC(0xa901a50c, "IsInBail"):
			return GetFlag(IS_BAILING) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;

	    // @script | BailOn | turn bails on
		case CRCC(0xe0df1f0a, "BailOn"):
			m_bail = true;
			break;
			
        // @script | BailOff | turn bails off
		case CRCC(0x8c3d7864, "BailOff"):
			m_bail = false;
			break;

        // @script | BailIsOn | true if bails on
		case CRCC(0xe1d5168, "BailIsOn"):
			return m_bail ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			

        // @script | FrontTruckSparks | true if sparks should be coming from the front trucks, false if from rear
        case CRCC(0xe0760055, "FrontTruckSparks"):
			return m_front_truck_sparks ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			break;

        // @script | SetFrontTruckSparks | sets sparks to come from front trucks
		case CRCC(0x19d0e5fb, "SetFrontTruckSparks"):
			m_front_truck_sparks = true;
			break;

        // @script | SetRearTruckSparks | sets sparks to come from rear trucks
		case CRCC(0xb48ec756, "SetRearTruckSparks"):
			m_front_truck_sparks = false;
			break;
			
        // @script | SetState | sets the state to the specified state
        // @uparm name | state name (lip, air, ground)
		case CRCC(0x948ebf96, "SetState"):
		{
			uint32 StateChecksum = 0;
			pParams->GetChecksum(NO_NAME, &StateChecksum);
			
			if (GetState() == LIP && StateChecksum != CRCD(0xa549b57b, "Lip"))
			{
				// Trigger the lip off event.
				maybe_trip_rail_trigger(TRIGGER_LIP_OFF);
			}

			// If we are going to "LIP" via script then clear the rail node, as it's a patch to get the skater to freeze
			if (GetState() != LIP && StateChecksum == CRCD(0xa549b57b, "Lip"))
			{
				mp_rail_node = NULL;
			}
			
			// if script alters state from wallride (like for a bail), then snap upright   
			// note, this command does not support setting TO wall, if it does, we should check here
			if (GetState() == WALL)
			{
				new_normal(Mth::Vector(0.0f, 1.0f, 0.0f));
				ResetLerpingMatrix();
			}
			
			switch (StateChecksum)
			{
				case 0:
					break;
				case CRCC(0x439f4704, "Air"):
					SetState(AIR);
					break;	
				case CRCC(0x58007c97, "Ground"):
					m_last_ground_feeler.SetTrigger(0); // prevent spurious gaps
					SetState(GROUND);
					break;	
				case CRCC(0xa549b57b, "Lip"):
					SetState(LIP);
					break;
				default:
					Dbg_Assert(false);
					break;
			}		
			break;
		}

        // @script | SetRailSound | sets the rail sound to either grind or slide
        // @uparm Grind | pass either Grind or Slide
		case CRCC(0x947fccf4, "SetRailSound"):
		{
			uint32 rail_sound_checksum = 0;
			pParams->GetChecksum(NO_NAME, &rail_sound_checksum, Script::ASSERT);
			Dbg_MsgAssert(rail_sound_checksum == CRCD(0x255ed86f,"Grind") || rail_sound_checksum == CRCD(0x8d10119d, "Slide"),
				("\n%s\nBad rail sound type '%s' sent to SetRailSound", pScript->GetScriptInfo(), Script::FindChecksumName(rail_sound_checksum)));
			if (rail_sound_checksum == CRCD(0x8d10119d, "Slide"))
			{
				SetFlagTrue(RAIL_SLIDING);
			}
			else
			{
				SetFlagFalse(RAIL_SLIDING);
			}
			break;
		}

		// @script | LockVelocityDirection | When passed the flag On this will cause the skater's
		// velocity to be locked in its current direction and be unaffected by the skater's rotation.
		// Only works when on the ground however.
		// @flag On | Enable
		// @flag Off | Disable (Ie, back to normal)
		case CRCC(0xacb82c02, "LockVelocityDirection"):
			m_lock_velocity_direction = pParams->ContainsFlag(CRCD(0xf649d637, "On"));
			break;
			
        // @script | SetRollingFriction | change the rolling friction value
        // @flag Default | use the default value
        // @uparm 0.0 | friction coeff
		case CRCC(0x510f983b, "SetRollingFriction"):
			pParams->GetFloat(NO_NAME, &m_rolling_friction);
			if (pParams->ContainsFlag(CRCD(0x1ca1ff20, "Default")))
			{
				if (m_special_friction_duration == 0.0f)
				{
					m_rolling_friction = GetPhysicsFloat(CRCD(0x78f80ec4, "Physics_Rolling_Friction"));
				}
			}	
			break;
        
        // @script | SetGrindTweak | 
        // @uparm int | grind tweak
		case CRCC(0x71b993b7, "SetGrindTweak"):
			pParams->GetInteger(NO_NAME, &mGrindTweak);
			break;
   
        // @script | Ledge | true if ledge
		case CRCC(0x315d9ed4, "Ledge"):
			return mLedge ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
        // @script | BadLedge | true if bad ledge
		case CRCC(0xbe371b7b, "BadLedge"):
			return mBadLedge ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
        // @script | SkateInAble | Do a collision check using a horizontal line a bit below the skater, and return true if it hits something skatable.
        // @flag Left | go left to right
		// Parameters in physics.q: SkateInAble_HorizOffset and SkateInAble_DownOffset
        // @flag Lip | This will do a check to see if the other side of the spine that the
		// skater might be doing a lip trick on is skateinable.
		// The collision check is down relative to the skater, but horizontal relative to 
		// the world because of the skater's wacky orientation when doing a lip trick.
		// The direction of the collision check is towards the skater, so that it detects the
		// other side of the spine.
		// Parameters in physics.q: SkateInAble_LipHorizOffset and SkateInAble_LipDownOffset
		case CRCC(0x55934543, "SkateInAble"):
			if (pParams->ContainsFlag(CRCD(0xa549b57b, "Lip")))
			{
				float HorizOff = GetPhysicsFloat(CRCD(0xb92cbe3b, "SkateInAble_LipHorizOffset"));
				float DownOff = GetPhysicsFloat(CRCD(0xefdfe781, "SkateInAble_LipDownOffset"));
				m_col_end = GetPos();
				m_col_end[Y] -= DownOff;
				m_col_start = m_col_end - HorizOff * GetMatrix()[Y];
				if (get_member_feeler_collision())
				{
					return m_col_flag_vert ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
				}
				else
				{
					// If the first check fails, do another check straight down. This is for when the skater is doing a lip trick on a high up rail that
					// still has vert beneath it.
					HorizOff = GetPhysicsFloat(CRCD(0x108613cb, "SkateInAble_LipExtraCheckHorizOffset"));
					DownOff = GetPhysicsFloat(CRCD(0xf9dc446d, "SkateInAble_LipExtraCheckDownOffset"));
					m_col_start = GetPos() - HorizOff * GetMatrix()[Y];
					m_col_end = m_col_start;
					m_col_end[Y] -= DownOff;
					if (get_member_feeler_collision())
					{
						return m_col_flag_vert ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
					}
				}
			}
			else
			{
				float HorizOff=GetPhysicsFloat(CRCD(0xc78ebe56,"SkateInAble_HorizOffset"));
				Mth::Vector a = GetPos() + HorizOff * GetMatrix()[X];
				a[Y] -= GetPhysicsFloat(CRCD(0xfca3378c,"SkateInAble_DownOffset"));
				Mth::Vector b = a - 2.0f * HorizOff * GetMatrix()[X];
				
				if (pParams->ContainsFlag(CRCD(0x85981897,"Left")))

				{
					m_col_start = a;
					m_col_end = b;
				}
				else
				{
					m_col_start = b;
					m_col_end = a;
				}
			
				if (get_member_feeler_collision())
				{
					return m_col_flag_vert ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
				}	
			}	
			return CBaseComponent::MF_FALSE;
			
        // @script | LastSpinWas | check if last spin was frontside or backside (requires one flag)
        // @flag Backside |
        // @flag Frontside |
		case CRCC(0x6c8a1316, "LastSpinWas"):
			if (pParams->ContainsFlag(CRCD(0x47953f00, "Backside")))
			{
				if (GetFlag(FLIPPED))
				{
					return !mYAngleIncreased ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
				}
				else
				{
					return mYAngleIncreased ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
				}				
			}	
			else if (pParams->ContainsFlag(CRCD(0x996d5512, "Frontside")))
			{
				if (GetFlag(FLIPPED))
				{
					return mYAngleIncreased ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
				}
				else
				{
					return !mYAngleIncreased ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
				}	
			}	
			else
			{
				Dbg_MsgAssert(false, ("LastSpinWas requires a FrontSide or BackSide flag"));
			}
			break;
			
        // @script | LandedFromSpine | 
		case CRCC(0x448e3630, "LandedFromSpine"):
			return mLandedFromSpine ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
        // @script | LandedFromVert | 
		case CRCC(0xf37ce040, "LandedFromVert"):
			return mLandedFromVert ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
   
        // @script | SetLandedFromVert | sets the mlandedfromvert flag 
        // (check with LandedFromVert)
		case CRCC(0x133e1293, "SetLandedFromVert"):
			mLandedFromVert = true;
			break;
		
        // @script | ResetLandedFromVert | clears mlandedfromvert flag
        // (check with LandedFromVert)
		case CRCC(0xf922431, "ResetLandedFromVert"):
			mLandedFromVert = false;
			break;
			
		// @script | IsInVertAir | check if VERT_AIR flag is set
		case CRCC(0xdfb8f052, "InVertAir"):
			return GetFlag(VERT_AIR) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			
        // @script | AllowLipNoGrind | 
		case CRCC(0xc5b4b390, "AllowLipNoGrind"):
			mAllowLipNoGrind = true;
			break;

        // @script | ClearAllLipNoGrind |
		case CRCC(0xb9eebff6, "ClearAllowLipNoGrind"):
			mAllowLipNoGrind = false;
			break;
			
        // @script | NoRailTricks | don't allow rail tricks
        // (turn on with AllowRailTricks)
		case CRCC(0xec681a59, "NoRailTricks"):
			mNoRailTricks = true;
			break;

        // @script | AllowRailTricks | turn off with NoRailTricks
		case CRCC(0x65a559a, "AllowRailTricks"):
			mNoRailTricks = false;
			break;
			
        // @script | TurnToFaceVelocity | turn to face the proper direction
		case CRCC(0x461e5b92, "TurnToFaceVelocity"):
			// Mick: Don't do it if vel is too short as skater will collapse
			if (GetVel().LengthSqr() < 0.01f * 0.01f) break;
			
			GetMatrix()[Z] = GetVel();
			GetMatrix()[Z].Normalize();						
			GetMatrix()[X] = Mth::CrossProduct(GetMatrix()[Y], GetMatrix()[Z]);
			GetMatrix()[X].Normalize();
			GetMatrix()[Y] = Mth::CrossProduct(GetMatrix()[Z], GetMatrix()[X]);
			GetMatrix()[Y].Normalize();
			
			ResetLerpingMatrix();	
			break;
		
        // @script | OverrideLimits | Overrides the limits to the maximum speed that the skater
		// can achieve.  This allows us to do temporary speed boosts whihc are much faster 
		// than regular gameplay.  
		// You can specify a time that this speed limit will last for 
        // @parm float | max | Maximum speed in inches per second (normal gameplay is around 1000, so 5000 is nice)
		// @parmopt float | max_max | max | usualy the same as above.  It's a hard cap, making it bigger that max will give
		// you a smoother limit to the speed.
		// @parmopt float | friction | 0.0000020 | air friction.  Slows you down at a rate proportional to speed. So reduce this to stay fast longer
		// @parmopt float | gravity | physics default | down_gravity. gravity that slows you down when going up a slope
		// @parmopt float | time | 10000000000000,0f | time in seconds that this effect should last for
		// @flag end | end the effect (other parameters are unneeded and ignored if end is flagged)
		case CRCC(0x90f1d8d5, "OverrideLimits"):
		{
			if (pParams->ContainsFlag(CRCD(0xff03cc4e, "end")))
			{
				m_override_limits_time = 0.0f;
			}
			else
			{
				pParams->GetFloat(CRCD(0x6289dd76, "max"), &m_override_max, Script::ASSERT);
				m_override_max_max = m_override_max;
				pParams->GetFloat(CRCD(0x73a19e3a, "max_max"), &m_override_max_max);
				m_override_limits_time = 10000000000000.0f;
				pParams->GetFloat(CRCD(0x906b67ba, "time"), &m_override_limits_time);
				m_override_air_friction = 0.0000020f;
				pParams->GetFloat(CRCD(0xedf3e7f4, "friction"), &m_override_air_friction);
				m_override_down_gravity = GetPhysicsFloat(CRCD(0x6618a713, "Physics_Ground_Gravity"));
				pParams->GetFloat(CRCD(0xa5e2da58, "gravity"), &m_override_down_gravity);
				
				if (pParams->ContainsFlag(CRCD(0xdf312928, "NoTimeLimit")))
				{
					// magic number meaning no time limit
					m_override_limits_time = -1.0f;
				}
			}
			break;
		}

        // @script | SetSpecialFriction | This command is used in the Revert script to specify a set of friction values that should be used for
		// each Revert in the combo. m_special_friction_index gets reset as soon as the combo ends, ie by the ClearPanel_Landed & ClearPanel_Bailed
	    // functions.
        // @uparm [] | array of friction values
		case CRCC(0x5e8d9b80,"SetSpecialFriction"):
		{
			Script::CArray* pArray = NULL;
			pParams->GetArray(NO_NAME, &pArray);
			Dbg_MsgAssert(pArray, ("\n%s\nSetSpecialFriction requires an array of friction values", pScript->GetScriptInfo()));
			if (m_special_friction_index >= static_cast< int >(pArray->GetSize()))
			{
				m_special_friction_index = static_cast< int >(pArray->GetSize()) - 1;
			}	
			m_rolling_friction = pArray->GetFloat(m_special_friction_index);
			if (m_special_friction_index == 0)
			{
				m_special_friction_decrement_time_stamp = Tmr::GetTime();
			}
			++m_special_friction_index;
			
			pParams->GetFloat(CRCD(0x79a07f3f, "Duration"), &m_special_friction_duration);
			break;
		}
			
		// @script | OverrideCancelGround | Overrided the Cancel_Ground flag in gaps
		// for use with reverts
        // @flag Off | turn this flag off
		case CRCC(0xb94bc0c6, "OverrideCancelGround"):
			SetFlag(OVERRIDE_CANCEL_GROUND, !pParams->ContainsFlag(CRCD(0xd443a2bc, "Off")));
			break;	
		     			
        // @script | SetExtraPush | 
        // @parmopt float | radius | 48.0 | wall push radius
        // @parmopt float | speed | 100.0 | wall push speed
        // @parmopt float | turn | 6.0 | wall rotate speed
		case CRCC(0xe47ff4b5, "SetExtraPush"):
		{
			m_wall_push_radius = 48.0f;
			m_wall_push_speed = 100.0f;
			m_wall_rotate_speed = 6.0f;
			pParams->GetFloat(CRCD(0xc48391a5, "radius"), &m_wall_push_radius);
			pParams->GetFloat(CRCD(0xf0d90109, "speed"), &m_wall_push_speed);
			pParams->GetFloat(CRCD(0xdfdfeab8, "turn"), &m_wall_rotate_speed);
			break;
		}
		
		// @script | GetMatrixNormal | places the skater's matrix normal in x, y, and z
		case CRCC(0x5144783, "GetMatrixNormal"):
		{
			pScript->GetParams()->AddFloat(CRCD(0x7323e97c, "x"), GetMatrix()[Y][X]);
			pScript->GetParams()->AddFloat(CRCD(0x0424d9ea, "y"), GetMatrix()[Y][Y]);
			pScript->GetParams()->AddFloat(CRCD(0x9d2d8850, "z"), GetMatrix()[Y][Z]);
			break;
		}
		
		// Overloading the CMotionComponent member function for skater specific logic
		case CRCC(0x8819dd8b, "Obj_MoveToNode"):
		{
			CMotionComponent* p_motion_component = GetMotionComponentFromObject(GetObject());
			Dbg_Assert(p_motion_component);
			
			// call the member function, which will set m_matrix and m_pos
			bool result = p_motion_component->CallMemberFunction( Checksum, pParams, pScript );
			
			// and copy this into m_display_matrix
			ResetLerpingMatrix();

			// set the shadow to stick to his feet, assuming we are on the ground
			// Note: These are used by the simple shadow, not the detailed one.
			CShadowComponent* p_shadow_component = GetShadowComponentFromObject(GetObject());
			Dbg_Assert(p_shadow_component);
			
			p_shadow_component->SetShadowPos( GetSkater()->m_pos );
			p_shadow_component->SetShadowNormal( GetSkater()->m_matrix[Y] );

			m_safe_pos = GetPos();			// needed for uberfrig
			GetOldPos() = GetPos();			// needed for camera, so it thinks we've stopped
			
			GetObject()->SetTeleported();

			// Force an update of the skater's camera if this is a local skater			
			if( GetSkater()->IsLocalClient())
			{
				Obj::CCompositeObject *p_obj = GetSkater()->GetCamera();
				if (p_obj)
				{
					p_obj->Update();	 // Not the best way of doing it...
				}
			}

			if (!pParams->ContainsFlag( CRCD(0xd607e2e6,"NoReset") ))
			{
				// assume we want the skater to stop...						
				m_last_ground_feeler.SetTrigger(0);  // patch to stop spurious gaps
				GetVel().Set();
				DUMP_VELOCITY;
				SetState(GROUND);
				
				// Make sure the flippedness of the skater is in a stable state
				// that reflects if he is goofy or not
                SetFlag(FLIPPED, !GetSkater()->m_isGoofy);
				
				// reset any flippedness of animation
				CSkaterFlipAndRotateComponent* p_flip_and_rotate_component = GetSkaterFlipAndRotateComponentFromObject(GetObject());
				Dbg_Assert(p_flip_and_rotate_component);
				p_flip_and_rotate_component->ApplyFlipState();
			}
			else
			{
				// the "NoReset" flag is set
				// so velocity is maintained
				// but we need to check if the orient flag is set
				// as that indicates that we want the velocity
				// to be oriented as well.
				// (if NoReset was set, then velocity will be zero)
				if ( pParams->ContainsFlag( CRCD(0x90a91232, "orient") ))
				{
					float y_vel = GetVel()[Y];	   			// y velocity is preserved.....
					GetVel()[Y] = 0.0f;
					float speed = GetVel().Length();  		// how fast are we going?
					GetVel() = GetMatrix()[Z] * speed;		// face forward, at that speed
					GetVel()[Y] = y_vel;
					DUMP_VELOCITY;
				}
			}
			return result ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}

		// Used by the create-a-goal cursor to initialise the cursor position.
		// Cannot use the current skater's position because he might have just bailed by
		// jumping into water, which would then cause the cursor to get stuck.
		case 0xf7e21884: // GetLastGroundPos
		{
			pScript->GetParams()->AddVector(CRCD(0x7f261953,"Pos"),m_last_ground_pos[X],m_last_ground_pos[Y],m_last_ground_pos[Z]);
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

void CSkaterCorePhysicsComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterCorePhysicsComponent::GetDebugInfo"));

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::Reset (   )
{
	// In Network games, don't set the skater to 0,0,0 because he is about to be teleported to a starting position and 0,0,0 means nothing.
	if (!GameNet::Manager::Instance()->InNetGame())
	{
		/*
		GetPos().Set(0.0f, 0.0f, 0.0f);
		GetOldPos().Set(0.0f, 0.0f, 0.0f);
		m_safe_pos.Set(0.0f, 0.0f, 0.0f);
		*/
		GetOldPos() = GetPos();
		m_safe_pos = GetPos();
		DUMP_POSITION;
	}
	
	m_pre_lip_pos.Set();
	
	/*
	GetMatrix().Identity();
	ResetLerpingMatrix();
	*/
	
	m_state_change_timestamp = Tmr::GetTime();
	
	GetVel().Set(0.0f, 0.0f, 0.0f);
	DUMP_VELOCITY;
	
	SetState(GROUND);
	m_previous_state = GROUND;
	
	for (int flag = NUM_ESKATERFLAGS; flag--; )
	{
		SetFlagTrue(static_cast< ESkaterFlag >(flag));
		SetFlagFalse(static_cast< ESkaterFlag >(flag));
	}
	SetFlag(FLIPPED, !GetSkater()->m_isGoofy);
	
	mp_movable_contact_component->LoseAnyContact();
	
	// bit nasty, prevents us getting "SKATE_OFF_EDGE" triggers
	m_last_ground_feeler.SetTrigger(0);

	m_wall_push_radius = 0.0f;
	
	m_tense_time = 0;

	mWallrideTime = 0;
	
	mNoSpin = false;
	
	m_bail = false;
	
	m_force_cankick_off = false;
	m_pressing_down_brakes = true;
	
	mp_state_component->mJumpedOutOfLipTrick = false;
		
	// mp_rail_node points to previous rail grinded, which might no longer be valid
	// best to set it to NULL  (fixed crash in park editor with multiple test-plays TT#678)	
	mp_rail_node = NULL;
	
	m_override_limits_time = 0.0f;
	m_transfer_overrides_factor = 1.0f;
	
	m_last_wallplant_time_stamp = 0;
	m_last_wallpush_time_stamp = 0;
	
	m_last_jump_time_stamp = 0;
	
	m_special_friction_duration = 0.0f;
}
          
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::ReadySkateState ( bool to_ground_state, const SRailData* p_rail_data, const SAcidDropData* p_acid_drop_data )
{
	// setup the state in preparation for being in skating mode next object update
	
	// reset all flags except FLIPPED
	ResetFlags();
	
	if (to_ground_state)
	{
		SetState(GROUND);
		m_previous_state = GROUND;
	}
	else
	{
		SetState(AIR);
		m_previous_state = AIR;
		
		// no in-air orientation control after walking
		SetFlagTrue(NO_ORIENTATION_CONTROL);
		
		if (mp_walk_component->m_disallow_acid_drops)
		{
			SetFlagTrue(AIR_ACID_DROP_DISALLOWED);
		}
	}
	
	m_state_change_timestamp = Tmr::GetTime();
	
	ResetLerpingMatrix();
	
	m_display_normal = m_last_display_normal = m_current_normal = GetMatrix()[Y];
	
	GetOldPos() = m_safe_pos = GetPos();
	
	// bit nasty, prevents us getting "SKATE_OFF_EDGE" triggers
	m_last_ground_feeler.SetTrigger(0);

	m_wall_push_radius = 0.0f;
	
	m_tense_time = 0;

	mWallrideTime = 0;
	
	mNoSpin = false;
	
	// m_bail = false;
	
	m_force_cankick_off = false;
	m_pressing_down_brakes = true;
	
	mp_state_component->mJumpedOutOfLipTrick = false;
		
	m_override_limits_time = 0.0f;
	m_transfer_overrides_factor = 1.0f;
	
	m_last_wallplant_time_stamp = 0;
	m_last_wallpush_time_stamp = 0;
	
	m_last_jump_time_stamp = 0;
	
	// handle special transitions
	if (p_rail_data)
	{
		// TT8717.  If walker had a movable contact, but that contact was not the same as the object containing the relavent rail manager component,
		// the skater was asserting in do_rail_physics.  We need to lose any old contact and acquire the appropriate contact.
		mp_movable_contact_component->LoseAnyContact();
		if (p_rail_data->p_movable_contact)
		{
			mp_movable_contact_component->ObtainContact(p_rail_data->p_movable_contact);
		}
		got_rail(p_rail_data->rail_pos, p_rail_data->p_node, p_rail_data->p_rail_man, true, true);
		GetOldPos() = m_safe_pos = GetPos();
	}
	else if (p_acid_drop_data)
	{
		enter_acid_drop(*p_acid_drop_data);
	}
	
	#if 0
	const char* p_state_names [   ] =
	{
		"GROUND",
		"AIR",
		"WALL",
		"LIP",
		"RAIL",
		"WALLPLANT"
	};
	DUMPS(p_state_names[GetState()]);
	#endif
}
          
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::CleanUpSkateState (   )
{
	if (m_vert_air_last_frame)
	{
		m_vert_air_last_frame = false;
        GetObject()->BroadcastEvent(CRCD(0x5e27200a, "SkaterExitVertAir"));
	}
	
	// longer rerail delay when walking
	m_rerail_time = static_cast< Tmr::Time >(GetPhysicsFloat(CRCD(0xe4412eb7, "Rail_walk_rerail_time")));
	
	// lip time counts towards rerail delay when walking
	if (m_lip_time > m_rail_time)
	{
		m_rail_time = m_lip_time;
	}
	
	// no grinding from walking after jumping out of a lip
	if (GetState() == AIR && m_previous_state == LIP)
	{
		SetFlagFalse(CAN_RERAIL);
	}
	
	// Extra clean up so that an observing camera will act OK while walking.  Note that observing cameras always use skater camera logic, even when the
	// observed player is walking.
	mp_state_component->m_state = GROUND;
	mp_state_component->m_camera_display_normal.Set(0.0f, 1.0f, 0.0f);
	mp_state_component->m_camera_current_normal.Set(0.0f, 1.0f, 0.0f);
	mp_state_component->m_spine_vel.Set();
	mp_state_component->mJumpedOutOfLipTrick = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CSkaterCorePhysicsComponent::ResetFlags (   )
{
	// reset all flags except FLIPPED
	for (int flag = NUM_ESKATERFLAGS; flag--; )
	{
		ESkaterFlag skater_flag = static_cast< ESkaterFlag >(flag);
		if (skater_flag == FLIPPED) continue;
		SetFlagTrue(skater_flag);
		SetFlagFalse(skater_flag);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::SetState ( EStateType state )
{
	if (mp_state_component->m_state != state)
	{
		m_state_change_timestamp = Tmr::GetTime();
		m_previous_state = mp_state_component->m_state;
	}
	
	// if going out of lip state, and previous lip pos is valid (not zero), then move back to the previous lip position (so we drop down nicely)
	if (mp_state_component->m_state == LIP
		&& state != LIP 
		&& (m_pre_lip_pos[X] != 0.0f || m_pre_lip_pos[Y] != 0.0f || m_pre_lip_pos[Z] != 0.0f))
	{
		// *** (SetState(LIP))  kind of nasty, but we want to allow them to move through the lip
		// we leave m_old_trigger_pos alone
		GetPos() = m_pre_lip_pos;
		GetOldPos() = GetPos();
		DUMP_POSITION;
	}

	// If setting the state to anything other than lip then kill the pre_lip_pos	
	if (state != LIP)
	{
		m_pre_lip_pos.Set(0.0f, 0.0f, 0.0f);
	}

	if (state != GROUND)
	{
		// start recording graffiti tricks
		mp_trick_component->SetGraffitiTrickStarted( true );
	}

	if (mp_state_component->m_state == RAIL && state != RAIL)
	{
		SetFlagFalse(RAIL_SLIDING);
	}

	if (mp_state_component->m_state == AIR && state != AIR)
	{
		// If going from AIR to any other state, record the landing time. Required for the AirTimeLessThan and AirTimeGreaterThan functions.
		m_landed_time = Tmr::GetTime();
		
		SetFlagFalse(NO_ORIENTATION_CONTROL);
		SetFlagFalse(OLLIED_FROM_RAIL);
	}	
	else if (mp_state_component->m_state != AIR && state == AIR)
	{
		// If going to AIR, record the takeoff time, unless going from WALL to AIR.
		// (The 'unless' bit fixes TT493, where hitting the ground from doing a wall-ride was not setting a big enough airtime)
		if (mp_state_component->m_state != WALL)
		{
			m_went_airborne_time = Tmr::GetTime();
		}	

		// Reset the spin tracking stuff.
		mp_trick_component->mTallyAngles = 0.0f;
		
		// Clear the L1 and R1 triggers, since they're used for spin taps.
		mp_input_component->GetControlPad().m_L1.ClearTrigger();
		mp_input_component->GetControlPad().m_R1.ClearTrigger();
		// Just to be sure
		m_tap_turns = 0.0f;
	}   
	
	mp_state_component->m_state = state;
																			   
	mp_sound_component->SetState(state);
}
          
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::ReverseFacing (   )
{
	GetMatrix()[Z] = -GetMatrix()[Z];
	GetMatrix()[X] = -GetMatrix()[X];
	ResetLerpingMatrix();
	mRail_Backwards = !mRail_Backwards;
	
	#ifdef __NOPT_ASSERT__
	if (DebugSkaterScripts && GetObject()->GetType() == SKATE_TYPE_SKATER)
	{
		printf("%d: Rotating skater\n", (int) Tmr::GetRenderFrame());
	}
	#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::CollideWithOtherSkaterLost ( CCompositeObject* p_other_skater )
{
	// reset all flags except FLIPPED
	ResetFlags();

	SetState(AIR);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::limit_speed (   )
{
	float save_y = GetVel()[Y];
	GetVel()[Y] = 0.0f;
	
	float speed = GetVel().Length();
	float max_max_speed = GetSkater()->GetScriptedStat(CRCD(0xcc5f87aa, "Skater_Max_Max_Speed_Stat")); 
	float max_speed = GetSkater()->GetScriptedStat(CRCD(0x2eacddb3, "Skater_Max_Speed_Stat"));

	if (m_override_limits_time != 0.0f)
	{
		// -1.0f is a magic number causing no time limit
		if (m_override_limits_time != -1.0f)
		{
			m_override_limits_time -= m_frame_length;
			if (m_override_limits_time < 0.0f)
			{
				m_override_limits_time = 0.0f;
			}
		}
		max_max_speed = m_override_max_max;
		max_speed = m_override_max;
	}
	
	if (speed > max_max_speed)
	{
		speed = max_max_speed;
		GetVel().Normalize(speed);
		DUMP_VELOCITY;
	}
	
	if (speed > max_speed)
	{
		apply_wind_resistance(GetPhysicsFloat(CRCD(0x850eb87a, "Physics_Heavy_Air_Friction")));
	}
	
	GetVel()[Y] = save_y;
	
	// decrement the special friction counter
	if (m_special_friction_duration != 0.0f)
	{
		m_special_friction_duration -= m_frame_length;
		if (m_special_friction_duration <= 0.0f)
		{
			// reset the special friction to default
			m_special_friction_duration = 0.0f;
			m_rolling_friction = GetPhysicsFloat(CRCD(0x78f80ec4, "Physics_Rolling_Friction"));
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::apply_wind_resistance ( float friction )
{
	// air friction is proportional to the square of the velocity; this is a limiting friction, and has much more effect at high speeds
	float speed_squared = GetVel().LengthSqr();
	if (speed_squared < 0.00001f) return;
	
	Mth::Vector air_friction = GetVel();
	air_friction.Normalize(friction * 60.0f * m_frame_length * speed_squared);
	
	if (air_friction.LengthSqr() > speed_squared)
	{
		GetVel().Set();
		DUMP_VELOCITY;
	}
	else
	{
		GetVel() -= air_friction;
		DUMP_VELOCITY;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::do_on_ground_physics (  )
{		
	// clear any flags that have no meaning when on the ground
	SetFlagFalse(CAN_BREAK_VERT);
	SetFlagFalse(VERT_AIR);
	SetFlagFalse(TRACKING_VERT);
	SetFlagFalse(SPINE_PHYSICS);
	SetFlagFalse(IN_ACID_DROP);
	SetFlagFalse(IN_RECOVERY);
	SetFlagFalse(AIR_ACID_DROP_DISALLOWED);

	// ground skateing will cancel any memory of what rail we were on, so the next one seems fresh	
	mp_rail_node = NULL;
	m_last_rail_trigger_node_name = 0;
	
	// rotate velocity, so it lays in the plane we are on: (as skaters orientation can lag behind)	
	// Dave note: calling RotateToPlane() with too small |vector| causes Nan problems - fix at some point.
	if( GetVel().Length() > 0.001f )
	{
		GetVel().RotateToPlane(m_current_normal);
	}
		
	// get gravitational force 	
	Mth::Vector gravity(0.0f, GetPhysicsFloat(CRCD(0x6618a713, "Physics_Ground_Gravity")), 0.0f);

	// allow for override if we are going up the slope (we might want to set gravity to zero when rolling up a slope)
	if (m_override_limits_time != 0.0f && GetVel()[Y] > 0.0f)
	{
		gravity.Set(0.0f, m_override_down_gravity, 0.0f);
	}

	// project gravity onto the plane we are on
	gravity.ProjectToPlane(m_current_normal);
	
	// add gravity into our velocity, adjusting for time
	GetVel() += gravity * m_frame_length;
	DUMP_VELOCITY;

	// based on if we are doing a balance trick, then either handle braking/kicking, or do the balance physics
	switch (mp_balance_trick_component->mBalanceTrickType)
	{
		case 0:
			// Only do braking & speeding up when not doing a balance trick like a manual			
			if (is_trying_to_brake())
			{
				do_brake();
			}
			else
			{
				m_braking = false;
				if (can_kick())
				{
					do_kick();
				}
			}
			break;
			
		case CRCC(0xac90769, "NoseManual"):
		case CRCC(0xef24413b, "Manual"):
			m_braking = false;
			
			mp_balance_trick_component->mManual.DoManualPhysics();

			// start recording graffiti tricks
			mp_trick_component->SetGraffitiTrickStarted(true);
			
			if (Mdl::Skate::Instance()->GetCareer()->GetCheat(CRCD(0xb38341c9, "CHEAT_PERFECT_MANUAL")))
			{
				// Here, set the flag. It may seem redundant, but the above line is very likely
				// to be hacked by gameshark. They probably won't notice this one, which will
				// set the flags as if they had actually enabled the cheat -- which enables us
				// to detect that it has been turned on more easily.
				Mdl::Skate::Instance()->GetCareer()->SetGlobalFlag( Script::GetInteger(CRCD(0xb38341c9, "CHEAT_PERFECT_MANUAL")));
				mp_balance_trick_component->mManual.mManualLean = 0.0f;
				mp_balance_trick_component->mManual.mManualLeanDir = 0.0f;
				g_CheatsEnabled = true;
			}
			break;
			
		case CRCC(0x3506ce64, "Skitch"):
			mp_balance_trick_component->mSkitch.DoManualPhysics();
            if (Mdl::Skate::Instance()->GetCareer()->GetCheat(CRCD(0x69a1ce96, "CHEAT_PERFECT_SKITCH")))
			{
				// Here, set the flag. It may seem redundant, but the above line is very likely
				// to be hacked by gameshark. They probably won't notice this one, which will
				// set the flags as if they had actually enabled the cheat -- which enables us
				// to detect that it has been turned on more easily.
				Mdl::Skate::Instance()->GetCareer()->SetGlobalFlag( Script::GetInteger(CRCD(0x69a1ce96, "CHEAT_PERFECT_SKITCH")));
				mp_balance_trick_component->mSkitch.mManualLean = 0.0f;
				mp_balance_trick_component->mSkitch.mManualLeanDir = 0.0f;
				g_CheatsEnabled = true;
			}
			break;			
			
		case CRCC(0x255ed86f, "Grind"):
		case CRCC(0x8d10119d, "Slide"):
		{
			#ifdef __NOPT_ASSERT__
			CSkaterEndRunComponent* p_endrun_component = GetSkaterEndRunComponentFromObject(GetObject());
			Dbg_Assert(p_endrun_component);
            Dbg_MsgAssert(!p_endrun_component->IsEndingRun() || !p_endrun_component->RunHasEnded(), ("Grind balance trick done on ground?"));
			#endif
			break;			
		}
			
		case CRCC(0xa549b57b, "Lip"):	
		{
			#ifdef __NOPT_ASSERT__
			CSkaterEndRunComponent* p_endrun_component = GetSkaterEndRunComponentFromObject(GetObject());
			Dbg_Assert(p_endrun_component);
			Dbg_MsgAssert(!p_endrun_component->IsEndingRun() || !p_endrun_component->RunHasEnded(), ("Lip balance trick done on ground?"));
			#endif
			break;
		}
		
		default:	
		{
			Dbg_Assert(false);
			break;
		}
	}		

	// Ground friction is rolling friction + wind resistance
	// Update velocity for friction before using velocity
	handle_ground_friction();

	// if too close to a wall, then adjust velocity to move the skater away from it
	// the distance that is checked can be changed by script, and probably gets big in bails, to stop the skater clipping through walls
	push_away_from_walls();

	// Calculate how much we want to move, based on the velocity
	Mth::Vector m_movement = GetVel() * m_frame_length;

	// when skitching, we get sucked into the skitch point
	if (GetFlag(SKITCHING))
	{
		if (mp_movable_contact_component->GetContact() && mp_movable_contact_component->GetContact()->GetObject())
		{
			GetVel() = mp_movable_contact_component->GetContact()->GetObject()->GetVel() * GetPhysicsFloat(CRCD(0xdef25b34, "skitch_speed_match"));
			DUMP_VELOCITY;
		}
		m_movement.Set();
		move_to_skitch_point();
	}

	// if we are in contact with something, then factor in that "movement," but not if (we are skitching, and in not in initial movement) 
	if (mp_movable_contact_component->UpdateContact(GetPos()))  // still need to update it every frame, otherwise it gets confused
	{
		if (!GetFlag(SKITCHING))
		{
			// handle movement due to contact seperately from normal physic movement 
			
			GetPos() += mp_movable_contact_component->GetContact()->GetMovement();		 
			GetOldPos() = GetPos();	// (WITH CONTACT) frig, as movement is due to something carying us, no need to incorporate that into collision   
			m_safe_pos = GetPos();	// (WITH CONTACT) frig, as movement is due to something carying us, no need to incorporate that into collision
			DUMP_POSITION;
			
			if (mp_movable_contact_component->GetContact()->IsRotated())
			{
				GetMatrix() *= mp_movable_contact_component->GetContact()->GetRotation();
				m_lerping_display_matrix *= mp_movable_contact_component->GetContact()->GetRotation();
			}
		}
			
	}
	
	// set "move_again" flag, for while loop
	// generally this will be cleared after the first time through the loop
	// but it can be set again, indicating that out movmeent was brought short
	// by hitting a curver surface that we can track around (like a Qp)			  
	bool move_again = true;
	
	// we only let them move again once
	// so we have a flag that says if we have already done it this frame
	bool already_moved_again = false;	
	
	while (move_again)
	{
		move_again = false;
		
		Mth::Vector	start_pos = GetPos();	  		// remember where we started on this iteration
		GetPos() += m_movement;	  				// Move him
		DUMP_POSITION;
			
		// Handle collisions //////////////////////////////////////////////						
		if (!GetFlag(SKITCHING))   	// if skitching, then dont bother with forward collision
		{
 			handle_forward_collision();							
		}
		

		// Mick - removed the test for m_moving_to_skitch for now
		// as we want the cars to be able to drag the player over gaps and jumps
		// and it does not seem to look bad this way
		// the only problem might be during very abrupt cahnges in ground angle, 
		// or maybe very uneven ground		
		if (GetFlag(SKITCHING) /* && m_moving_to_skitch*/)
		{
			// moving to skitch point, so don't snap to ground, as
			// we might snap onto the vehicle
			// instead, just project the skater onto the plane of the car
			
			// Dan: We still need to update terrain, however
			m_feeler.m_start = GetPos() + 36.0f * GetMatrix()[Y];
			m_feeler.m_end = GetPos() - 200.0f * GetMatrix()[Y];
			if (get_member_feeler_collision())
			{
				set_terrain(m_feeler.GetTerrain());
			}
		}
		else
		{
			snap_to_ground();
		}
													 
		// now see how far we have moved
		// interestingly we often move more than we are supposed to
		// probably due to the "snap to ground" stuff.		
		Mth::Vector actual_movement_vector;
		actual_movement_vector = GetPos() - start_pos;
		float actual_distance_moved = actual_movement_vector.Length();
		float attempted_distance = m_movement.Length();

		if (!already_moved_again && GetState() == GROUND)
		{
			if (actual_distance_moved < (attempted_distance - 0.1f))
			{
				m_movement = GetVel();					 	
				m_movement.Normalize();			  								// get new direction of travel
				m_movement *= attempted_distance;									// at old speed
				m_movement *= 1.0f - actual_distance_moved / attempted_distance;	// scale to account for movement							
				move_again = true;
				already_moved_again = true;				
			}
		}
	}

	// The remaining "ground physics" should only be done if we are still on the ground (might have skated off it) 
	if (GetState() == GROUND)
	{
		handle_ground_rotation();			
		
		// don't let the board slide sideways	
		// K: Avoid anything that might change the velocity direction if this flag is set.
		if (!m_lock_velocity_direction)
		{
			remove_sideways_velocity(GetVel());
		}	
	
		if (!GetFlag(SKITCHING))
		{
			// check if we are too close to a wall, and pop us out and away		
			check_side_collisions();		
			
			// check if we are moving slowly and leaning, and mush us more away from a wall if so
			check_leaning_into_wall();
		}
		
		// K: This flag is to allow the skater to be rotated on the ground without affecting his
		// velocity direction, so also disable the flipping otherwise 360 rotations won't be possible.
		if (!m_lock_velocity_direction)
		{
			// might have gone upa  slope, and gravity pulled us down, so we are not skating backwards
			flip_if_skating_backwards();	
		}	
	
		// Check for jumping
		if (maybe_flag_ollie_exception())	
		{
			maybe_straight_up();
			SetFlagTrue(CAN_BREAK_VERT);
		}
	
		mp_trick_component->TrickOffObject(m_last_ground_feeler.GetNodeChecksum());
			
		m_tap_turns = 0.0f;	
	}
		
	#ifdef __NOPT_ASSERT__
	if (Script::GetInteger(CRCD(0x3ae85eef, "skater_trails")))
	{
		Gfx::AddDebugLine(GetPos() + m_current_normal, GetOldPos() + m_current_normal, GREEN, 0, 0);
	}
	#endif
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCorePhysicsComponent::is_trying_to_brake (   )
{								   
	CControlPad& control_pad = mp_input_component->GetControlPad();
	
	// if not autokick, and not accelerating (square pressed), and going slow, then brake	
	if (!m_auto_kick && !control_pad.m_square.GetPressed() && GetVel().Length() < 50.0f)
	{
		return true;
	}
	
	return control_pad.m_down.GetPressed() 				// Down must be pressed
		&& m_pressing_down_brakes						// and must be enabled
		&&
		(
			GetVel().Length() < 50.0f		   	// and either going really slow
			||
			(
				!control_pad.m_right.GetPressed()		// or not pressed right or left
				 &&
				!control_pad.m_left.GetPressed()
			)
			||
			(                                           // or going backwards
				Mth::DotProduct(GetVel(), GetMatrix()[Z]) < 0.0f
			)
		);				
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::do_brake (   )
{		
	// Handle braking
	
	m_braking = true;
	
	if (on_steep_slow_slope())
	{
		m_braking = false;
		return;
	}

	float speed = GetVel().Length();

	// if already really slow (or stopped), then just stop
	if (speed < GetPhysicsFloat(CRCD(0xe5d73f6f, "Physics_Brake_Acceleration")) * 2.0f * m_frame_length)
	{
			GetVel().Set();
			DUMP_VELOCITY;
	}
	else
	{
		#if 0 // old code, this check doesn't seem necessary given the above if branch; Dan
		// Mth::Vector forward = GetVel();
		// forward.Normalize();
		// forward *= -PHYSICS_BRAKE_ACCELERATION;
		// Mth::Vector old_vel = GetVel();					// remember old velocity		
		// m_vel += forward * m_frame_length;						 	// apply braking force
		// if (Mth::DotProduct(GetVel(), old_vel) < 0.0f)	// if the velocty now in other direction
		// {
			// GetVel().Set();								// then clear it to zero velocity
		// }
		#else
		Mth::Vector brake = GetVel();
		brake.Normalize(-GetPhysicsFloat(CRCD(0xe5d73f6f, "Physics_Brake_Acceleration")) * m_frame_length);
		GetVel() += brake;
		DUMP_VELOCITY;
		#endif
	}
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCorePhysicsComponent::on_steep_slow_slope (   )
{
	float speed = GetVel().Length();
	if (speed < GetPhysicsFloat(CRCD(0x62a1fa03,"Skater_max_sloped_turn_speed"))
		&& m_current_normal[Y] < GetPhysicsFloat(CRCD(0xc3527ef2, "Skater_max_sloped_turn_cosine")))
	{
		return true;
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCorePhysicsComponent::can_kick (   )
{
	CControlPad& control_pad = mp_input_component->GetControlPad();					 
	
	if (m_force_cankick_off)
	{
		return false;
	}

	// don't allow kicking if autokick is off and square not pressed	
	if (!m_auto_kick && !control_pad.m_square.GetPressed())
	{
		return false;
	}
	
	// don't allow kicking (accelerating) if "down" is pressed
	// as we would either be braking or sharp turning  
	if (control_pad.m_down.GetPressed())
	{
		return false;
	}

	float speed = GetVel().Length();
	
	if (!GetFlag(TENSE))
	{
		if (speed > GetSkater()->GetScriptedStat(CRCD(0x4610c2e3, "Skater_Max_Standing_Kick_Speed_Stat")))
		{
			return false;
		}
	}
	else
	{
		if (speed > GetSkater()->GetScriptedStat(CRCD(0x92e0247c, "Skater_Max_Crouched_Kick_Speed_Stat")))
		{
			return false;
		}
	}
	
	Dbg_Assert(m_auto_kick || control_pad.m_square.GetPressed());
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::do_kick (   )
{
	// Get skater's forward direction
	Mth::Vector forward;
	
	// K: Avoid anything that might change the velocity direction if this flag is set.
	if (m_lock_velocity_direction)
	{
		forward = GetVel();
		
		// Make sure the skater doesn't get stuck unable to move.
		if (forward.Length() < 0.5f)
		{
			forward = GetMatrix()[Z];
		}	
		else
		{
			forward.Normalize();
		}
	}
	else
	{
		forward = GetMatrix()[Z];
	}
		
	if (GetFlag(TENSE))
	{
		forward *= GetSkater()->GetScriptedStat(CRCD(0x3d24128e, "Physics_crouching_Acceleration_stat"));
	}
	else
	{
		forward *= GetSkater()->GetScriptedStat(CRCD(0x5f9b864d, "Physics_Standing_Acceleration_stat"));
	}
		
	// apply to velocity
	GetVel() += forward * m_frame_length;
	DUMP_VELOCITY;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::handle_ground_friction (   )
{
	// Apply the various type of friction that are acting when on the ground
	
	// if no autokick, and we are not bailing, and we have default friction then don't apply any friction
	// as it sucks to slow down when you do not have autokick
	if (!m_auto_kick && !GetFlag(IS_BAILING) && m_rolling_friction == GetPhysicsFloat(CRCD(0x78f80ec4, "Physics_Rolling_Friction")))
	{
		// autokick friction
		// currently none
	}
	else
	{	
		// non-autokick friction
		handle_wind_resistance();
		handle_rolling_resistance();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::handle_wind_resistance (   )
{	
	// Wind resistance differs, based on if we are crouched or not (as when we are crouched, we have a lower profile, so less wind resistance)
	
	float crouched_friction = GetPhysicsFloat(CRCD(0xbed96eda, "Physics_Crouched_Air_Friction"));
	float standing_friction = GetPhysicsFloat(CRCD(0x1a78b6fc, "Physics_Standing_Air_Friction"));
	
	if (m_override_limits_time != 0.0f)
	{
		crouched_friction = m_override_air_friction;
		standing_friction = m_override_air_friction;
	}
		  
	if (GetFlag(TENSE))
	{
		apply_wind_resistance(crouched_friction);
	}
	else
	{
		apply_wind_resistance(standing_friction);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
		
void CSkaterCorePhysicsComponent::handle_rolling_resistance (   )
{	
	// Only have rolling resistance if we are not going slow on a steep slope.	
	if (!slide_off_slow_steep_slope())
	{
		apply_rolling_friction();
	}
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCorePhysicsComponent::slide_off_slow_steep_slope (   )
{
	// If we are on very steep ground and moving slowly then do not allow the skater to brake
	if (on_steep_slow_slope())
	{
		float speed = GetVel().Length();
		
		Mth::Vector forward;
		if (speed < 0.001f)
		{
			forward = GetMatrix()[Z];
		}
		else
		{
			forward = GetVel();
			
		}
		
		Mth::Vector fall(0.0f, -1.0f, 0.0f);
		fall.ProjectToPlane(m_current_normal);	  
		
		float angle = Mth::GetAngleAbout(forward, fall, GetMatrix()[Y]);
		float rot = GetPhysicsFloat(CRCD(0x7dd5678b, "Skater_Slow_Turn_on_slopes")) * Mth::Sgn(angle) * m_frame_length;
		if (Mth::Abs(rot) > Mth::Abs(angle))
		{
			// just about done, so just turn the last bit of angle left
			rot = angle;
		}
		GetMatrix().RotateYLocal(rot);				
		m_lerping_display_matrix.RotateYLocal(rot);				
		
		return true;
	}
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::apply_rolling_friction (   )
{
	// apply a constant braking friction; if the velocity reaches 0, then we will stop (and not change direction)  		
	// This funtion will typically be used for rolling resistance
	
	// rolling friction is a constant, and is mostly noticable at slow speeds
	// if your speed is less than that produced by the force of rolling friction, then you will abruptly stop
	Mth::Vector rolling_friction = GetVel();
	float length = rolling_friction.Length();
	
	if (length < 0.0001f)
	{
		GetVel().Set();
		DUMP_VELOCITY;
		return;
	}
	
	rolling_friction *= 60.0f * m_rolling_friction * m_frame_length / length;
	
	if (rolling_friction.LengthSqr() > length * length)
	{
		GetVel().Set();
		DUMP_VELOCITY;
	}
	else
	{
		GetVel() -= rolling_friction;
		DUMP_VELOCITY;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
		
void CSkaterCorePhysicsComponent::push_away_from_walls (   )
{
	if (!m_wall_push_radius || !m_wall_push_speed) return;
	
	m_wall_push.Set();
	m_wall_dist = 1.0f;
	
	Mth::Vector	start, end;
	
	start = GetPos() + GetMatrix()[Y] * GetPhysicsFloat(CRCD(0xb45b39e2, "Skater_side_collide_height"));
	
	end = start + GetMatrix()[X] * m_wall_push_radius;
	check_for_wall_push(start, end, 0);
	
	end = start - GetMatrix()[X] * m_wall_push_radius;
	check_for_wall_push(start, end, 1);
	
	end = start + GetMatrix()[Z] * m_wall_push_radius;
	check_for_wall_push(start, end, 2);
	
	end = start - GetMatrix()[Z] * m_wall_push_radius;
	check_for_wall_push(start, end, 3);

	if (m_wall_dist == 1.0f) return;
	
	float push_speed = m_wall_push_speed * (1.0f - m_wall_dist);
	m_wall_push.Normalize(push_speed);
	GetVel() += m_wall_push;
	GetVel().RotateToPlane(m_current_normal);
	DUMP_VELOCITY;
	
	// if facing into the wall, then rotate away from it
	if (Mth::DotProduct(GetMatrix()[Z], m_feeler.GetNormal()) < 0.0f)
	{
		Mth::Vector target = GetMatrix()[Z];
		target.RotateToPlane(m_feeler.GetNormal());
		float angle = Mth::GetAngleAbout(GetMatrix()[Z], target, GetMatrix()[Y]);
		
		float rot = m_wall_rotate_speed * Mth::Sgn(angle) * m_frame_length;
		if (Mth::Abs(rot) > Mth::Abs(angle))
		{
			// just about done, so just turn the last bit of angle left
			rot = angle;
		}
		GetMatrix().RotateYLocal(rot);				
		m_lerping_display_matrix.RotateYLocal(rot);	   
	} 		
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::check_for_wall_push ( const Mth::Vector& start, const Mth::Vector& end, int index )
{
	// do not allow a push in this direction until a push in any other direction has NOT happenend for half a second
	// this should prevent the nasty flickering
	for (int i = 0; i < 4; i++)
	{
		if (i == index) continue;
		
		// get time for opposite direction
		Tmr::Time t = Tmr::ElapsedTime(m_push_time[index ^ 1]);
		
		if (t > 500)
		{
			return;
		}
	}
		
	m_col_start = start;
	m_col_end = end;
	
	if (get_member_feeler_collision())
	{
		m_wall_push += m_feeler.GetNormal();
		if (m_feeler.GetDist() < m_wall_dist)
		{
			m_wall_dist = m_feeler.GetDist();
		}
		m_push_time[index] = Tmr::GetTime();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::move_to_skitch_point (   )						
{
	
	CControlPad& control_pad = mp_input_component->GetControlPad();
	
	if (!mp_movable_contact_component->GetContact() || !mp_movable_contact_component->GetContact()->GetObject())
	{
		FLAGEXCEPTION(CRCD(0x47d44b84, "OffMeterBottom"));
		return;
	}

	CCompositeObject* p_skitch_object = mp_movable_contact_component->GetContact()->GetObject();
	
	CSkitchComponent* p_skitch_comp = GetSkitchComponentFromObject(p_skitch_object);
	Dbg_Assert(p_skitch_comp);
	
	////////////////////////////////////////////////////////
	// Do moving between the skitch nodes
	// if L1 or R1 pressed
	// Basically we just look for nodes to the left or right of us at a
	// one foot interval
	// up a a maximum of 10 feet
	// if one is found, then switch the m_node_index to that node 

	float dir = 0.0f;	   
	   
	if (control_pad.m_L1.GetTriggered())
	{		
		control_pad.m_L1.ClearTrigger();
		dir = 1.0f;
	}	

	if (control_pad.m_R1.GetTriggered())
	{		
		control_pad.m_R1.ClearTrigger();
		dir = -1.0f;
	}	

	if (control_pad.m_L2.GetTriggered())
	{		
		control_pad.m_L2.ClearTrigger();
		dir = 1.0f;
	}	

	if (control_pad.m_R2.GetTriggered())
	{		
		control_pad.m_R2.ClearTrigger();
		dir = -1.0f;
	}	

	// if moving, and either was not moving or moving in opposite direction, then we can actually try looking for a node
	if (dir != 0.0f && (!m_moving_to_skitch || dir != m_skitch_dir))
	{
		float len = 6.0f;
		float step = 6.0f;
		float max = 12.0f * 10.0f;
		while (len < max)
		{
			// get an offset to the right or left
			Mth::Vector offset = GetMatrix()[X];  // X points left, from the camera's POV			
			offset *= len * dir;
			Mth::Vector	test_pos = GetPos() + offset;
	
			// see if there is a skitch point there
			Mth::Vector dummy(0.0f, 0.0f, 0.0f);
			int index = p_skitch_comp->GetNearestSkitchPoint(&dummy, test_pos);
	
			// if there is, then switch to that point
			if (index != m_skitch_index)
			{
				m_skitch_index = index;
				m_moving_to_skitch = true;
				m_skitch_dir = dir;
				
				if (dir == 1.0f)
				{
					FLAGEXCEPTION(CRCD(0x74bf80cf, "SkitchLeft"));
				}
				else
				{
					FLAGEXCEPTION(CRCD(0x2e7474b5, "SkitchRight"));
				}				
				break;
			}	
		
			len += step;
		}
	}
	
	// 
	// end of moving between skitch nodes
	///////////////////////////////////////////////////
	
	if (!mp_movable_contact_component->GetContact() || !GetFlag(SKITCHING)) return;

	///////////////////////////////////////////////////////
	// Do the actual movement

	Mth::Vector skitch_point;
	if (!p_skitch_comp->GetIndexedSkitchPoint( &skitch_point, m_skitch_index)) return;

	// zero out W component to prevent overflows (in theory this should not be necessary)
	GetPos()[W] = 0.0f;

	Mth::Vector target = skitch_point + GetPhysicsFloat(CRCD(0x21fb182c, "skitch_offset")) * -p_skitch_object->GetMatrix()[Z];

	if (m_moving_to_skitch)
	{
		Mth::Vector con_move = mp_movable_contact_component->GetContact()->GetMovement();

		GetPos() += con_move;
		DUMP_POSITION;

		Mth::Vector dir = target - GetPos();
		float dir_length_sqr = dir.LengthSqr();
		float suck_speed = GetPhysicsFloat(CRCD(0x97496256, "skitch_suck_speed")) * m_frame_length;

		// if skater is stuck in a wall, then end the skitch when car is fifteen feet away 														 
		if (dir_length_sqr > FEET(15.0f) * FEET(15.0f))
		{
			FLAGEXCEPTION(CRCD(0x47d44b84, "OffMeterBottom"));
			return;
		}

		if (dir_length_sqr <= suck_speed * suck_speed)
		{
			// we have arrived, so no need for sucking later
			GetPos() = target;
			m_moving_to_skitch = false;
			DUMP_POSITION;
		}							   
		else
		{
			dir *= suck_speed / sqrtf(dir_length_sqr);
			GetPos() += dir;
			DUMP_POSITION;
		}
	}
	else
	{
			GetPos() = target;
			DUMP_POSITION;
	}
	
	// Copy the objects Display matrix over the skater's
	// will orient the skater the same way as the thing that is dragging it, so if the car looks solid, then so should the skater...
	GetMatrix() = p_skitch_object->GetDisplayMatrix();
	
	// we also set the display matrix, to avoid little glitches
	ResetLerpingMatrix();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
     
void CSkaterCorePhysicsComponent::handle_forward_collision (   )
{
	if (GetPos() == GetOldPos()) return;
	
	Mth::Vector	forward = GetPos() - GetOldPos();
	forward.Normalize();	
	
	Mth::Vector up_offset = GetMatrix()[Y] * GetPhysicsFloat(CRCD(0xd4205c9b, "Skater_First_Forward_Collision_Height"));
	
	m_col_start = GetOldPos() + up_offset;
	
	m_col_end = GetPos() + up_offset + forward * GetPhysicsFloat(CRCD(0x20102726, "Skater_First_Forward_Collision_Length"));

	if (!get_member_feeler_collision()) return;
	
	// we have hit something going forward
	// either it is a wall, and we need to bounce off it or it is a steep QP, and we need to stick to it.
	// (note, with the slow normal changing, then it is possible that we might even collide with the poly that we are on
	
	Mth::Vector	normal = m_feeler.GetNormal();								
							
	float dot = Mth::DotProduct(normal, m_current_normal);			

	// For fairly shallow curves, the dot between two normals will be pretty large
	// it's very important here to distinguish between a tight curve (like in a narrow QP) and a direct hit with a wall.
			   
	if (!m_col_flag_skatable || Mth::Abs(dot) < 0.01f)
	{
		mp_trigger_component->CheckFeelerForTrigger(TRIGGER_BONK, m_feeler);
		if (mp_physics_control_component->HaveBeenReset()) return;
		
		bounce_off_wall(normal);
	}
	else
	{
		// it's a qp, stick to it
		
		// Just move our contact point to the point of collision.  This is not right, as it could kill a lot of the movement for this frame
		// but should do for now (and stops you falling through)											
		GetPos() = m_feeler.GetPoint();
		
		// move it off the surface a little, so we are not IN it (which would be indeterminates as to which side)
		GetPos() += normal * 0.1f;
		DUMP_POSITION;
	}
}			

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::bounce_off_wall ( const Mth::Vector& normal )
{
	if (check_for_wallpush()) return;
	
	// Given the normal of the wall, then bounce off it, turning the skater away from the wall
	
	Mth::Vector	forward = GetPos() - GetOldPos();
	Mth::Vector movement = forward;				   		// remember how far we moved
	forward.Normalize();
	
	Mth::Vector up_offset = GetMatrix()[Y] * GetPhysicsFloat(CRCD(0xd4205c9b,"Skater_First_Forward_Collision_Height"));

	float turn_angle;
	float angle = rotate_away_from_wall(normal, turn_angle);

	float min = Mth::DegToRad(GetPhysicsFloat(CRCD(0x1483fd01, "Wall_Bounce_Dont_Slow_Angle"))); 
					 							
	if (Mth::Abs(angle) > min)
	{
		float old_speed = GetVel().Length();
		
		// The maximum value of Abs(angle) would be PI/2 (90 degrees), so scale the velocity 
		float x = Mth::Abs(angle) - min;
		x /= (Mth::PI / 2.0f) - min;		// get in the range 0 .. 1
		x = 1.0f - x; 						// invert, as we want to stop when angle is 90
		
		GetVel() *= x;
		DUMP_VELOCITY;

		// if (negative ^ flipped) then backwards flail, otherwise forward flail	

		if (old_speed > GetPhysicsFloat(CRCD(0xbe0a58a0, "Wall_Bounce_Dont_Flail_Speed")))
		{
			#ifdef	__NOPT_ASSERT__
			{
				Mth::Vector	up_offset = GetMatrix()[Y] * GetPhysicsFloat(CRCD(0xd4205c9b,"Skater_First_Forward_Collision_Height"));
				Mth::Vector start = up_offset+GetOldPos();
				Mth::Vector end = up_offset+GetPos();
				TrackingLine(1, start, end);	  // 1 = wall bounce flail
			}
			#endif
			
			if ((angle < 0.0f) ^ (GetFlag(FLIPPED)))														   
			{
				FLAGEXCEPTION(CRCD(0xb4101d70,"FlailLeft"));
			}
			else
			{
				FLAGEXCEPTION(CRCD(0x756a7535,"FlailRight"));
			}
			
			// Player's terrain isn't set to the terrain in m_feeler, as this is a wall we're bouncing off of (or chain link fence or something)...
			// Steve: we gots ta figure out how to do this...
			// Perhaps have another terrain value sent to client skaters that tell them to play a bonk sound?
			mp_sound_component->PlayBonkSound( old_speed / GetSkater()->GetScriptedStat(CRCD(0xcc5f87aa, "Skater_Max_Max_Speed_Stat")), m_feeler.GetTerrain());
		}
	}
	
	// Bit of a patch here to move the skater away from the wall
	// Not needed so much with new sideways collision checks but we keep it in for low ledges.
	// Should perhaps standardize the height so collision checks for side and front but we'd probably still have problems.
	
	GetPos() = m_feeler.GetPoint() - up_offset;
	GetPos() += normal * 6.0f;
	DUMP_POSITION;
	
	// Now the majority of cases have been taken care of; we need to see if the skater is going to get stuck in a corner.

	float old_speed = movement.Length();		 					// get how much we moved last time
	Mth::Vector next_movement = GetVel();					// get new direction of velocity
	forward = GetVel();
	forward.Normalize();
	next_movement = forward * old_speed;							// extend by same movment as last time
	
	m_col_start = GetPos() + up_offset;
	m_col_start += up_offset;
	
	m_col_end = GetPos() + up_offset + next_movement
		+ forward * GetPhysicsFloat(CRCD(0x20102726, "Skater_First_Forward_Collision_Length"));
	
	if (get_member_feeler_collision() && GetSkater()->IsLocalClient())
	{
		// Just rotating the skater will lead to another collision, so try just inverting the skater's velocity from it's original and halving it....
		
		// First reverse the rotation, and rotate 180 degrees
		GetVel().RotateY(Mth::DegToRad(180.0f) - turn_angle);
		GetMatrix().RotateYLocal(Mth::DegToRad(180.0f) - turn_angle);
		ResetLerpingMatrix();
		
		GetVel() *= 0.5f; 
		DUMP_VELOCITY;
		
		if (old_speed > GetPhysicsFloat(CRCD(0xbe0a58a0, "Wall_Bounce_Dont_Flail_Speed")))
		{
			FLAGEXCEPTION(CRCD(0xb4101d70, "FlailLeft"));
		}
	}
}			

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::snap_to_ground (   )
{
	CControlPad& control_pad = mp_input_component->GetControlPad();
	
	float up_dot = 0.0f;
	
	// Since we really don't want to loose contact with the ground while skitching, we use a much bigger snap up dist
	// The problem will come when we get dragged down a slope.  The car will flatten out well ahead of us, so pushing us down through the slop
	// (as we are a few feet behind it) and we will be so far under the ground that our normal snap up will not be able to dig us out of it,
	// so we go in air, uberfrig, and get dragged to a random spot under the level.
	// (This would not happen if we just skitch on flat ground)																								
	
	// Dan: snap_to_ground is never called while skitching
	// if (GetFlag(SKITCHING))
	// {
		// m_col_start = GetMatrix()[Y] * GetPhysicsFloat(CRCD(0x5c0d9610,"Physics_Ground_Snap_Up_SKITCHING"));	// much above feet
	// }
	// else
	// {
		m_col_start = GetMatrix()[Y] * GetPhysicsFloat(CRCD(0xe4d79235, "Physics_Ground_Snap_Up"));	  		// bit above the feet
	// }
	
	m_col_end = GetMatrix()[Y] * -200.0f;		    // WAY! below the feet, we check distance later

	m_col_start += GetPos();
	m_col_end += GetPos();
		 
	bool sticking = false;			
	// get disatnce to ground and snap the skater to it, but only if the ground is skatable, otherwise we just go to "AIR"
	if (get_member_feeler_collision())
	{
		Mth::Vector movement = GetPos() - m_feeler.GetPoint(); 
		float drop_dist = movement.Length();
		float drop_sign = Mth::DotProduct(GetMatrix()[Y], movement); // might be approx +/- 0.00001f
		if (!m_col_flag_skatable)	
		{
			// if below the face (or very close to it), then push us away from it				
			if (drop_sign < 0.001f)
			{
				// at point of contact, and move away from surface
				GetPos() = m_feeler.GetPoint() + m_feeler.GetNormal();	
				DUMP_POSITION;
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
			Mth::Vector normal = m_feeler.GetNormal();
			
			Mth::Vector	forward = GetMatrix()[Z];
			float front_dot = Mth::DotProduct(forward,normal);

			Mth::Vector	old_forward = forward;
			forward.RotateToPlane(normal);
			old_forward.RotateToPlane(m_current_normal);

			// angle between front vectors, projected onto faces
			up_dot = Mth::DotProduct(forward, old_forward);
			
			float stick_angle_cosine;
			if (!control_pad.m_up.GetPressed())
			{
				stick_angle_cosine = cosf(Mth::DegToRad(GetPhysicsFloat(CRCD(0xef161c2a, "Ground_stick_angle"))));
			}
			else
			{
				stick_angle_cosine = cosf(Mth::DegToRad(GetPhysicsFloat(CRCD(0x4138283e, "Ground_stick_angle_forward"))));
			}
			
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
					
#ifdef __PLAT_NGC__
					float angle = acosf(Mth::Clamp(up_dot, -1.0f, 1.0f));
#else
					float angle = acosf(up_dot);
#endif // __PLAT_NGC__

					Mth::Vector	last_move = GetPos() - GetOldPos();
					
					float max_drop = last_move.Length() * tanf(angle);
					
					float min_drop = GetPhysicsFloat(CRCD(0x899ba3d0, "Physics_Ground_Snap_Down"));				
                    // if (GetFlag(SKITCHING))
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
					SetFlag(LAST_POLY_WAS_VERT, m_col_flag_vert);
					new_normal(normal);
				}		 
			}																		  
		}
					
		if (sticking)
		{	
			// if there is a collision, then snap to it
			GetPos() = m_feeler.GetPoint();
			DUMP_POSITION;
			
			if (GetState() == GROUND && movement.Length() > 2.0f)
			{
				// curbs are assumed to be between parallel surfaces, so check this...
				if (up_dot > 0.99f)
				{
					SetFlag(SNAPPED_OVER_CURB, true);
				}				
			}

			// will return trivially if terrain is already set to this type...
			set_terrain(m_feeler.GetTerrain());
			
			adjust_normal();

			// check to see if we have skated onto a movable object
			check_movable_contact();

			// still on ground, so store the latest ground collision data
			// check first to see if we are about to change
			if (m_last_ground_feeler.GetSector() != m_feeler.GetSector())
			{
				// changin sectors, so check the sector we came from and the one we are going to
				mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_OFF, m_last_ground_feeler);
				if (mp_physics_control_component->HaveBeenReset()) return;
				
				mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_ONTO, m_feeler);
				if (mp_physics_control_component->HaveBeenReset()) return;
			}

			if (!mp_trick_component->GraffitiTrickStarted())
			{
				// clear the graffiti trick buffer if we're moving to a new world sector, but haven't started our trick yet...
				mp_trick_component->SetGraffitiTrickStarted(false);
			}
			
			set_last_ground_feeler(m_feeler);
		}
	}
	
	// skated off surface into the air
	if (!sticking)
	{
		SetState(AIR);
		GetObject()->BroadcastEvent(CRCD(0xd96f01f1, "SkaterOffEdge"));
		FLAGEXCEPTION(CRCD(0x3b1001b6, "GroundGone"));
		
		maybe_straight_up();
		
		if (GetFlag(VERT_AIR))
		{
			SetFlagTrue(CAN_BREAK_VERT);

			// we want to break vert, but don't want to go into spine physics for a frame
			// so don't do this check if we are trying to break the spine
			// (we can still break vert or spine on the next frame, when in the air)
 			if (!BREAK_SPINE_BUTTONS)
			{
				maybe_break_vert();	
			}
			
			// if we did not break vert now
			// then only allow us to break vert later if we've been tapping the "up" button
			// this is indicated by us having RELEASED or Pressed in teh last few ticks
			if (static_cast< int >(control_pad.m_up.GetReleasedTime()) > GetPhysicsInt(CRCD(0x6bb5b751, "Skater_vert_active_up_time"))
				&& static_cast< int >(control_pad.m_up.GetPressedTime()) > GetPhysicsInt(CRCD(0x6bb5b751, "Skater_vert_active_up_time")))			
			{
				// "UP" was not pressed any time recently, so don't let us break late
				SetFlagFalse(CAN_BREAK_VERT);	
			}
		}
		else if (BREAK_SPINE_BUTTONS)
		{
			SAcidDropData acid_drop_data;
			if (maybe_acid_drop(true, GetPos(), GetOldPos(), GetVel(), acid_drop_data))
			{
				enter_acid_drop(acid_drop_data);
			}
		}
		
		mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_OFF_EDGE, m_last_ground_feeler);
		if (mp_physics_control_component->HaveBeenReset()) return;
	}
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::new_normal ( Mth::Vector normal )
{
	if (Ed::CParkEditor::Instance()->UsingCustomPark())
	{
		// check if this new normal will make us lean into a wall
		if (normal[Y] > 0.0f)
		{
			CFeeler	feeler(GetPos(), GetPos() + 72.0f * normal);
			if (feeler.GetCollision())
			{
				normal.Set(0.0f,1.0f,0.0f);
			}
		}
	}
	
	if (normal != m_current_normal)
	{
		m_current_normal = normal;	  										// remember this, for detecting if it changes
		m_last_display_normal = m_display_normal;				// remember start position for lerping	
		m_normal_lerp = 1.0f;												// set lerp counter
		
		GetMatrix()[Y] = normal;
		GetMatrix().OrthoNormalizeAbout(Y);									// set regular normal immediately
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::adjust_normal (   )
{
	///////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////
	//
	// As the skater moves over the ground, he will go from being in contact
	// with one polygon, to another.
	// The "normal" of a polygon is the vector perpendicular to the surface of
	// the polygon, and the skater is normally aligned so that the "up" vector
	// of the skater (the Y component of the orientation matrix) is the same
	// as the normal or the polgon he is in contact with.
	//
	// However, as we go from one polygon to another (in a quarterpipe, for example)
	// the normal will change abruptly, and this looks very jerky
	// So, we try to smooth this out by remembering the old normal, and interpolating 
	// towards the current display normal
	// this is done at a fixed speed, controled by the script value "Normal_Lerp_Speed" 
	// whihc is defined in phsyics.q
	//
	// m_normal_lerp represents how far off the current face normal we are, it
	// will vary from 1.0 (still at the last display normal) to 0.0 (at current normal)
	// the intermediate normal is stored in m_display_normal
	// and is also copied directly into m_lerping_display_matrix, to rotate the skater 

	// if m_normal_lerp is 0.0, then we don't need to do anything, as we should already be there
	if (m_normal_lerp != 0.0f)	   		// if lerping
	{			
		// If the last display normal is the same as the current normal, then we can't interpolate between them
		if (m_last_display_normal == m_current_normal)
		{
			m_normal_lerp = 0.0f;
		}
		else
		{
			// adjust lerp at constant speed from 1.0 to 0.0, accounting for framerate
			m_normal_lerp -= GetPhysicsFloat(CRCD(0xd8120182, "Normal_Lerp_Speed")) * m_frame_length * 60.0f;
	
			// if gone all the way, then clear lerping values and set m_display_normal to be the current face normal
			if (m_normal_lerp <= 0.0f)
			{
				m_normal_lerp = 0.0f;
				m_display_normal = m_current_normal;
			}
			else
			{
				// Still between old and current normal, so calculate intermediate normal
				m_display_normal = Mth::Lerp(m_current_normal, m_last_display_normal, m_normal_lerp);											
				m_display_normal.Normalize();
			}
		}
	}
	
	// Now update the orientation matrix.
	// We need our up (Y) vector to be this vector
	// if it changes, rotate the X and Z vectors to match
	if (m_lerping_display_matrix[Y] != m_display_normal)
	{
		// lerp the y axis
		m_lerping_display_matrix[Y] = m_display_normal;
		m_lerping_display_matrix.OrthoNormalizeAbout(Y);	
		m_lerping_display_matrix[X] = GetMatrix()[X];
		m_lerping_display_matrix[Z] = GetMatrix()[Z];		
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::check_movable_contact (   )
{
	// given m_feeler is our current contact point with the ground, then check if our mp_movable_contact_component->GetContact() information needs updating				
	
	if (GetFlag(SKITCHING)) return;
	
	if (mp_movable_contact_component->CheckForMovableContact(m_feeler))
	{
		GetVel() -= mp_movable_contact_component->GetContact()->GetObject()->GetVel();
		DUMP_VELOCITY;
	}
}				

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::maybe_straight_up (   )
{			
	// Skater has just left gound, either jumped, or skated off it
	// so we need to check if the ground we left was flagged as VERT and if it is,
	// then we need to set our skater's velocity and orientation so they go straight up
	
	if (GetFlag(LAST_POLY_WAS_VERT))	
	{
		TrackingLine(3, GetOldPos(), GetPos());	  // 3 = going vert
		
		// Get the normal to the plane we jumped from
		Mth::Vector	up_plane_normal = m_current_normal;

		m_vert_normal = m_current_normal;
		m_vert_pos = GetOldPos();
		
		// move vert pos down one inch to better track subtle changes in edge height
		m_vert_pos[Y] -= 1.0f;
											
		// clear any Y component to this plane, makes it vertical
		up_plane_normal[Y] = 0.0f;
		
		if (up_plane_normal.Length() > 0.001f)
		{
			// and re-normalize to get a unit normal to the vertical plane.
			up_plane_normal.Normalize();
			
			GetVel().RotateToPlane(up_plane_normal);
			DUMP_VELOCITY;
	
			new_normal(up_plane_normal);
			
			// Fall line is used for auto turn
			m_fall_line = GetMatrix()[Z];
			m_fall_line[Y] = -m_fall_line[Y];
				
			// offset the jumper away from the plane by an inch
			GetPos() += up_plane_normal * GetPhysicsFloat(CRCD(0x78384871, "Physics_Vert_Push_Out"));
			DUMP_POSITION;
			
			SetFlagTrue(VERT_AIR);
			SetFlagTrue(TRACKING_VERT);
			SetFlagTrue(AUTOTURN);
			m_vert_upstep = 6.0f;			
		}
		else
		{
			SetFlagFalse(VERT_AIR);
		}
	}
	else
	{
		SetFlagFalse(VERT_AIR);
	}
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::maybe_break_vert (   )
{
	// We "Break Vert" when we are pressing forward at the start of transitioning from the ground into a "vert" state
	// however, we need to defer the test until there is no ground in that "forward" direction (which will be directly beneath the skater's feet)
	
	CControlPad& control_pad = mp_input_component->GetControlPad();
	
	if (
		(	
			control_pad.m_up.GetPressed() 
			&& static_cast< int >(control_pad.m_up.GetPressedTime()) > GetPhysicsInt(CRCD(0x9d2f8cc8, "Skater_vert_push_time"))
			&& !control_pad.m_left.GetPressed()		// must be JUST up, to avoid accidents when turning
			&& !control_pad.m_right.GetPressed()
			&& !control_pad.m_square.GetPressed()	// and don't break if trying to do a trick involving up
			&& !control_pad.m_circle.GetPressed()
		)
		||
		(
			BREAK_SPINE_BUTTONS
		)
	)
	{
		// aha... up is pressed, so we want to break the air poly

		if (!BREAK_SPINE_BUTTONS)
		{
			// normal breaking the air polygon, we just fly forward
			
			float speed = GetVel().Length() * GetPhysicsFloat(CRCD(0x13f33b41, "physics_break_air_speed_scale"));
			
			GetVel()[X] += -m_display_normal[X] * speed;
			GetVel()[Z] += -m_display_normal[Z] * speed;
			GetVel()[Y] *= GetPhysicsFloat(CRCD(0x848c5cd6, "physics_break_air_up_scale"));
			DUMP_VELOCITY;
			
			// Now, since we broke the vert poly, the way it was set up, the skater will have alrready been snapped to vertical position
			// so we need to rotate him forwads 45 degrees to componsate
			GetMatrix().RotateXLocal(Mth::DegToRad(GetPhysicsFloat(CRCD(0xd757f4bb, "Skater_Break_Vert_forward_tilt"))));				

			SetFlagFalse(VERT_AIR);	   	// just regular air, if we broke the air poly
			SetFlagFalse(TRACKING_VERT);	// and certainly not tracking the vert
			SetFlagFalse(CAN_BREAK_VERT);	// and as we broke vert, we don't want to do it again
			SetFlagFalse(AIR_ACID_DROP_DISALLOWED);	// allow acid drops once once again

			// and we want to be going in the direction of our velocity, so set front x and Z, but leave Y
			Mth::Vector vel_normal = GetVel();
			vel_normal.Normalize();
			
			GetMatrix()[Z][X] = vel_normal[X];
			GetMatrix()[Z][Z] = vel_normal[Z];
			GetMatrix()[Z].Normalize();		
			GetMatrix().OrthoNormalizeAbout(Z); 
		}
		else
		{
			if (!maybe_spine_transfer())
			{
				// cannot find a transfer target, so just break the air polygon
				GetVel()[X] += -m_display_normal[X] * 24.0f;
				GetVel()[Z] += -m_display_normal[Z] * 24.0f;
				DUMP_VELOCITY;
				GetMatrix().RotateXLocal(Mth::DegToRad(GetPhysicsFloat(CRCD(0xd757f4bb, "Skater_Break_Vert_forward_tilt"))));				
	
				SetFlagFalse(VERT_AIR);	   	// just regular air, if we broke the air poly
				SetFlagFalse(TRACKING_VERT);	// and certainly not tracking the vert
				SetFlagFalse(CAN_BREAK_VERT);	// and as we broke vert, we don't want to do it again
				SetFlagTrue(IN_RECOVERY);		// tell him to just upright himself
				
				// and we want to be going in the direction of our velocity, so set front X and Z, but leave Y
				Mth::Vector vel_normal = GetVel();
				vel_normal.Normalize();
				
				GetMatrix()[Z][X] = vel_normal[X];
				GetMatrix()[Z][Z] = vel_normal[Z];
				GetMatrix()[Z].Normalize();		
				GetMatrix().OrthoNormalizeAbout(Z); 
			
				return;
			}
		}
		
		ResetLerpingMatrix();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
                    
bool CSkaterCorePhysicsComponent::maybe_spine_transfer (   )
{
	// Break spin button is pressed, so try to break the spine
			
	// The line to check along is the skater's forward directinal vector, rotated onto the XZ plane
	// if you go straight up the wall, then this will be the same as the normal of the wall (in XY) as we previously calculated
	// however we also want to handle the cases where you approach the QP at an angle
	
	// Need to take the forward vector (Z) and rotate it "forward" 90 degrees
	// Rotate about an axis perpendicular to both the horizontal part of m_matrix[Y] and also the world up (0,1,0)
	
	Mth::Vector skater_up = GetMatrix()[Y];	// skater_up is expected to be horizontal here, as we are "vert"
	skater_up[Y] = 0.0f;
	skater_up.Normalize();
	
	// get a vector perpendicular to the plane containing m_matrix[Z] and the world up 	
	#if 0 // old code - crossing by axis alined	vector bugs me
	// Mth::Vector world_up(0.0f, 1.0f, 0.0f);
	// Mth::Vector side = Mth::CrossProduct(skater_up, world_up);
	#else
	Mth::Vector side(-skater_up[Z], 0.0f, skater_up[X], 0.0f);
	#endif
	
	// assuming we have not slowed down much, then our velocity should roughly be in the direction we took off from 
	Mth::Vector forward = -GetVel();
	forward.Normalize();
				
	Mth::Vector wall_out = forward; 							// forward facing vector
	wall_out.Rotate(side, Mth::PI / 2.0f);					// rotate fowrad 90 degrees
	
	float speed;
	float dist = 12.0f;
	float time = 1.0f;
	bool hip_transfer = false;							  
			  
	CFeeler feeler;
	
	// Here the "wall" is what we are currently skating on, anything with "wall" in the name refers to that

	Mth::Vector target;
	Mth::Vector target_normal;
	bool target_found = false;			

	// First find a point beneath our current position
	// Nice long line, higher than we can posibly jump
	feeler.m_start = GetPos() + wall_out * 0.5f;
	feeler.m_end = GetPos() + wall_out * 0.5f;
	feeler.m_end[Y] -= 4000.0f;
	
	// ignore everything that is NOT vert
	// feeler.SetIgnore(0, mFD_VERT);
	
	Mth::Vector	wall_pos;
	if (feeler.GetCollision())
	{
		wall_pos = feeler.GetPoint();

		Mth::Vector start_normal = feeler.GetNormal();
		start_normal[Y] = 0.0f;
		start_normal.Normalize();
		
		target_found = look_for_transfer_target(-wall_out, start_normal, hip_transfer, target, target_normal);
			
		if (!target_found)
		{
			Mth::Vector left_along_vert(-start_normal[Z], 0.0f, start_normal[X]);
			
			// no target was found in the forward direction, perhaps we should look slightly left or right; look in the horizontal direction which is
			// halfway between the previous search direction and the plane of the vert
			if (mp_input_component->GetControlPad().m_left.GetPressed() && !mp_input_component->GetControlPad().m_right.GetPressed())
			{
				Mth::Vector search_dir = left_along_vert + -wall_out;
				search_dir.Normalize();
				target_found = look_for_transfer_target(search_dir, start_normal, hip_transfer, target, target_normal);
			}
			else if (mp_input_component->GetControlPad().m_right.GetPressed() && !mp_input_component->GetControlPad().m_left.GetPressed())
			{
				Mth::Vector search_dir = -left_along_vert + -wall_out;
				search_dir.Normalize();
				target_found = look_for_transfer_target(search_dir, start_normal, hip_transfer, target, target_normal);
			}
		}
	}
	
	if (!target_found) return false;
	
	Mth::Vector XZ_to_target = target - wall_pos;
	XZ_to_target[Y] = 0.0f;
	dist = XZ_to_target.Length();
	
	// We are only going to allow this later if the target point is the same level
	// as the takeoff point, and we have a clear line
	// so set it to this now, so we calculate the time correctly
	target[Y] = GetPos()[Y];

	// if the two faces are not really perpendicular or if the spine is wider than
	// then we determine that we are on a "hip" and we just want to go across it without drifting left or right
	// so we want to project all our velocity straight up

	Mth::Vector	horizontal_target_normal = target_normal;
	horizontal_target_normal[Y] = 0.0f;
	horizontal_target_normal.Normalize();
	
	Mth::Vector cache_vel = GetVel();
	
	float face_dot = Mth::Abs(Mth::DotProduct(skater_up, horizontal_target_normal));															
	if (face_dot < 0.9f)
	{
		GetVel()[Y] = GetVel().Length();
		GetVel()[X] = 0.0f;
		GetVel()[Z] = 0.0f;
		DUMP_VELOCITY;
	}
	else
	{
		// if spine more than two feet wide, then also don't allow drift
		if (dist > FEET(2.0f))
		{
			GetVel()[Y] = GetVel().Length();
			GetVel()[X] = 0.0f;
			GetVel()[Z] = 0.0f;
			DUMP_VELOCITY;
		}
	}
	
	// one inch out, to ensure miss the lip
	dist += 1.0f;

	#if 0 // old transfer code
	// get angle to rotate about, being the vector perpendicular to the world up vector and the difference between the two face normals
	// (generally for a spine these normals will be opposite, however they might be up to 90 degrees or more off when doing a hip)
	
	Mth::Vector normal_diff = target_normal - skater_up;
	normal_diff[Y] = 0.0f;
	normal_diff.Normalize();
	
	m_spine_rotate_axis[X] = -normal_diff[Z];
	m_spine_rotate_axis[Y] = 0.0f;
	m_spine_rotate_axis[Z] = normal_diff[X];
	m_spine_rotate_axis[W] = 0.0f;;
	#endif
	
	// for gravity calculations, temporarily pretend we are doing spine physics, so g is constant
	SetFlagTrue(SPINE_PHYSICS);
	time = calculate_time_to_reach_height(target[Y], GetPos()[Y], GetVel()[Y]);
	SetFlagFalse(SPINE_PHYSICS);

	// subtract some frames of time, to ensure we make it
	// time -= m_frame_length * 2.0f;
	
	if (time < 0.1f)
	{
		time = 0.1f;
	}
	
	speed = dist / time;
	
	// if spine more than two foot wide, then make sure that we have enough speed to get over it
	// otherwise, just do a little pop over, and allow them to recover						  
	if (dist > 24.0f && speed * speed > GetVel().LengthSqr())
	{
		return false;
	}

	// we have found a target point, either by looking directly in front or by doing the drop-down method
	// but we don't want to go for it until there is a clear line from our current position to the target
	
	Mth::Vector	target_XZ = target;
	target_XZ[Y] = GetPos()[Y];
	
	feeler.m_start = GetPos();
	feeler.m_end = target_XZ;
	if (feeler.GetCollision())
	{
		// don't do anything.  We have a valid transfer but we can wait until we get high enough before we try for it
		return true;
	}
		
	// setup the transfer's matrix slerp
	
	Mth::Vector land_facing;
	if (!hip_transfer)
	{
		land_facing = target - GetPos();
		land_facing[Y] = -(land_facing[X] * target_normal[X] + land_facing[Z] * target_normal[Z]) / target_normal[Y];
		land_facing.Normalize();
	}
	else
	{
		Mth::Vector offset = target - GetPos();
		offset.Normalize();
		float dot = Mth::DotProduct(offset, horizontal_target_normal);
		if (dot < 0.0f)
		{
			land_facing.Set(0.0f, 1.0f, 0.0f);
		}
		else
		{
			land_facing.Set(0.0f, -1.0f, 0.0f);
		}
	}

	Mth::Matrix transfer_slerp_start = GetMatrix();

	// calculate the facing we want when we land; retain our horizontal direction and choose a vertical component which puts us parallel so the target
	// poly's plane

	// calculate goal matrix
	Mth::Matrix transfer_slerp_goal;
	transfer_slerp_goal[Z] = land_facing;
	transfer_slerp_goal[Z].ProjectToPlane(target_normal);
	transfer_slerp_goal[Z].Normalize();
	transfer_slerp_goal[Y] = target_normal;
	transfer_slerp_goal[X] = Mth::CrossProduct(transfer_slerp_goal[Y], transfer_slerp_goal[Z]);
	transfer_slerp_goal[W].Set();
	
	// store the goal facing for use in adjusting the velocity at land time
	m_transfer_goal_facing = transfer_slerp_goal[Z];

	// if the skater is entering the spine transfer with an odd facing due to rotation, we want to preserve that angle in the slerp's goal matrix

	// calculate the deviation between the skater's velocity and facing
	float angle = Mth::GetAngleAbout(GetMatrix()[Z], cache_vel, GetMatrix()[Y]);
	
	// be a bit forgiving for hip transfers, as you often have to hit left/right to trigger them, which causes rotation
	if (Mth::Abs(angle) < Mth::DegToRad(30.0f))
	{
		angle = 0.0f;
	}

	// rotate goal facing to reflect the deviation in the initial facing
	transfer_slerp_goal.RotateYLocal(-angle);

	// setup the slerp state
	m_transfer_slerper.setMatrices(&transfer_slerp_start, &transfer_slerp_goal);
	m_transfer_slerp_timer = 0.0f;
	m_transfer_slerp_duration = Mth::ClampMin(time, 0.9f); // clamp the time to stop super fast rotations
	m_transfer_slerp_previous_matrix = transfer_slerp_start;
	
	// insure that the slerp takes us over the top, and doesn't invert us
	Mth::Matrix slerp_test;
	m_transfer_slerper.getMatrix(&slerp_test, 0.5f);
	if (slerp_test[Y][Y] < 0.0f)
	{
		m_transfer_slerper.invertDirection();
	}
	
	// remember the height we are aiming for, so when we come down through this height
	// then we remove the non vert velocity (or make it very small....)
	m_transfer_target_height = target[Y];
	
	// set velocity over the wall fast enough to land on the target point																	 
	mp_state_component->m_spine_vel = (target - GetPos()) / time;		// velocity from start to target
	mp_state_component->m_spine_vel[Y] = 0.0f;															// but ignore Y, as gravity handles that...
	
	// tell the code we are doing spine physics, so we lean quicker 
	if (!hip_transfer)
	{
		GetObject()->SpawnAndRunScript(CRCD(0xa5179e9e, "SkaterAwardTransfer"));	// award a trick (might want to do it as an exception later)
	}
	else
	{
		GetObject()->SpawnAndRunScript(CRCD(0x283bb5d6, "SkaterAwardHipTransfer"));	// award a trick (might want to do it as an exception later)
	}
	
	// no late jumps during a transfer
	GetObject()->RemoveEventHandler(CRCD(0x8ffefb28, "Ollied"));
	
	SetFlagTrue(SPINE_PHYSICS);	// flag in spin physics, to do the lean forward, and also allow downcoming lip tricks
	SetFlagFalse(IN_ACID_DROP);
	SetFlagFalse(TRACKING_VERT);	// we are still vert, but not tracking the vert
	SetFlagFalse(CAN_BREAK_VERT);	// and as we "broke" vert, we don't want to do it again
	
	return true;
 }

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
                    
bool CSkaterCorePhysicsComponent::look_for_transfer_target ( const Mth::Vector& search_dir, const Mth::Vector& start_normal, bool& hip_transfer, Mth::Vector& target, Mth::Vector& target_normal )
{
	// take a bunch of steps forward until we find one		
	// This is not very good, as we have to do 80 collision checks....
	// we really need to optimize our collision detection to be able to select a set of "nearby" object
	// or to select a set that intersects a sphere, or a plane
	// (here, we could just get the set that intersects the plane)
	// this could be statically cached by the colision code, and have one set
	// or perhaps more flexibly, each "feeler" could have a set of objects
	// that it deals with (defaulting to the set of all objects)
	
	CFeeler feeler;
	
	// setup collision cache
	Nx::CCollCache* p_coll_cache = Nx::CCollCacheManager::sCreateCollCache();
	Mth::CBBox bbox;
	Mth::Vector p;
	p = GetPos() + search_dir * 10.0f;
	bbox.AddPoint(p);
	p[Y] -= 4000.0f;
	bbox.AddPoint(p);
	p = GetPos() + search_dir * 500.0f;
	bbox.AddPoint(p);
	p[Y] -= 4000.0f;
	bbox.AddPoint(p);
	p_coll_cache->Update(bbox);
	feeler.SetCache(p_coll_cache);
	
	for (float step = 10.0f; step < 500.0f; step += 6.0f)		
	{
		// First find a VERT point a bit in front of us
		// can be some distance below us 
		// allowing us to transfer from high to low pools
		// (and low to high, proving you can jump up from the low point to the high point first)
		feeler.m_start = GetPos() + search_dir * step;		// start at current height
		feeler.m_end = feeler.m_start;
		feeler.m_end[Y] -= 4000.0f;									// long way below
		
		if (feeler.GetCollision() && (feeler.GetFlags() & mFD_VERT) && is_vert_for_transfers(feeler.GetNormal()))
		{
			Mth::Vector horizontal_normal = feeler.GetNormal();
			horizontal_normal[Y] = 0.0f;
			horizontal_normal.Normalize();
			float dot = Mth::DotProduct(start_normal, horizontal_normal);
			if (dot <= 0.95f)
			{
				target = feeler.GetPoint();
				target_normal = feeler.GetNormal();
				
				hip_transfer = dot > -0.866f;
				
				// feeler.m_end[Y] += 3960.0f;
				// feeler.DebugLine(255, 100, 100, 0);
				
				Nx::CCollCacheManager::sDestroyCollCache(p_coll_cache);
				
				return true;
			}
			else
			{
				// feeler.m_end[Y] += 3960.0f;
				// feeler.DebugLine(100, 255, 100, 0);
			}
		}
		else
		{
			// feeler.m_end[Y] += 3960.0f;
			// feeler.DebugLine(100, 100, 255, 0);
		}
	}
	
	Nx::CCollCacheManager::sDestroyCollCache(p_coll_cache);
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
                    
bool CSkaterCorePhysicsComponent::maybe_acid_drop ( bool skated_off_edge, const Mth::Vector &pos, const Mth::Vector& old_pos, Mth::Vector& vel, SAcidDropData& acid_drop_data )
{
	// horizontal direction in which a drop would occur
	Mth::Vector drop_direction;
	if (mp_physics_control_component->IsSkating())
	{
		drop_direction = vel;
		drop_direction[Y] = 0.0f;
		float length = drop_direction.Length();
		if (length < 0.01f) return false;
		drop_direction *= 1.0f / length;
	}
	else
	{
		drop_direction = mp_walk_component->m_facing;
	}
	
	bool target_found = false;
	Mth::Vector target;
	
	// in order not to miss vert polys with a thin horizontal projection, we check for them starting at this frame's initial position
	Mth::Vector search_pos = old_pos;
	search_pos[Y] = Mth::Max(pos[Y], old_pos[Y]);
	
	float scan_distance = 500.0f;
	float scan_height = 0.0f;
	if (mp_physics_control_component->IsWalking())
	{
		if (mp_walk_component->m_state == CWalkComponent::WALKING_GROUND)
		{
			// and use a reduced scan distance
			scan_distance = Script::GetFloat(CRCD(0xe50a9d56, "Physics_Acid_Drop_Walking_On_Ground_Search_Distance"));

			// and look for vert polys above us
			scan_height = 200.0f;
		}
		else
		{
			// look slightly behind us for acid drops (we may be facing down a vert we're standing on)
			search_pos -= 12.0f * drop_direction;
		}
	}
	
	CFeeler feeler;
	
	// setup collision cache
	Nx::CCollCache* p_coll_cache = Nx::CCollCacheManager::sCreateCollCache();
	Mth::CBBox bbox;
	Mth::Vector p;
	p = search_pos;
	p[Y] += scan_height;
	bbox.AddPoint(p);
	p[Y] -= 4200.0f;
	bbox.AddPoint(p);
	p = search_pos + drop_direction * scan_distance;
	p[Y] += scan_height;
	bbox.AddPoint(p);
	p[Y] -= 4200.0f;
	bbox.AddPoint(p);
	p_coll_cache->Update(bbox);
	feeler.SetCache(p_coll_cache);
	
	Mth::Vector target_normal;
	Mth::Vector horizontal_target_normal;
	float distance;
	for (distance = 0.01f; distance < scan_distance; distance += 3.0f)
	{
		// look for a vert poly below us
		feeler.m_start = search_pos + distance * drop_direction;
		feeler.m_end = feeler.m_start;
		feeler.m_start[Y] += scan_height;
		feeler.m_end[Y] -= 4000.0f;
		if (feeler.GetCollision() && (feeler.GetFlags() & mFD_VERT) && is_vert_for_transfers(feeler.GetNormal()))
		{
			// the horizontal projection of the vert's normal just correspond somewhat to our direction			 
			target_normal = horizontal_target_normal = feeler.GetNormal();
			horizontal_target_normal[Y] = 0.0f;
			horizontal_target_normal.Normalize();
			
			if (mp_physics_control_component->IsWalking() && mp_walk_component->m_state == CWalkComponent::WALKING_AIR)
			{
				// special acceptance rules for walking in-air acid drops
				target_found = Mth::DotProduct(drop_direction, horizontal_target_normal) <= -0.25f
					|| Mth::DotProduct(drop_direction, horizontal_target_normal) >= 0.05f;
			}
			else
			{
				target_found = Mth::DotProduct(drop_direction, horizontal_target_normal) >= 0.05f;
			}
			
			if (target_found)
			{
				target = feeler.GetPoint();
				// feeler.m_end[Y] += 3960.0f;
				// feeler.DebugLine(255, 100, 100, 0);
				break;
			}
			else
			{
				// feeler.m_end[Y] += 3960.0f;
				// feeler.DebugLine(100, 255, 100, 0);
			}
		}
		else
		{
			// feeler.m_end[Y] += 3960.0f;
			// feeler.DebugLine(100, 100, 255, 0);
		}
		
		// use a larger incrememt at larger distances, as we have several frames yet to find these polys
		if (distance > 100.0f)
		{
			distance += 24.0f;
		}
	}
	
	if (!target_found)
	{
		// no valid acid drop target found
		Nx::CCollCacheManager::sDestroyCollCache(p_coll_cache);
		return false;
	}
	
	float original_target_height = target[Y];
	
	// because our search began behind us, the horizontal offset to the target may not be forward
	Mth::Vector horizontal_offset = target - pos;
	horizontal_offset[Y] = 0.0f;
	distance = horizontal_offset.Length();
	if (Mth::DotProduct(horizontal_offset, drop_direction) < 0.0f)
	{
		distance = -distance;
	}
	drop_direction = horizontal_offset / distance;
	
	// stash a copy of velocity so we can pretend it has an adjusted value
	Mth::Vector hold_vel = vel;
	
	if (mp_physics_control_component->IsWalking())
	{
		// because when walking they are necessarily in the same direction, project our horizontal velocity in the drop direction
		vel.ProjectToNormal(drop_direction);
		vel[Y] = hold_vel[Y];
	}
	
	// calculate our effective horizontal velocity
	float initial_horiz_speed = sqrtf(vel[X] * vel[X] + vel[Z] * vel[Z]);
	
	if (mp_physics_control_component->IsWalking())
	{
		// boost our effective horizontal speed up to maximum run speed
		float horizontal_boost = mp_walk_component->get_run_speed();
		if (initial_horiz_speed < horizontal_boost)
		{
			vel[X] = horizontal_boost * GetWalkComponentFromObject(GetObject())->m_facing[X];
			vel[Z] = horizontal_boost * GetWalkComponentFromObject(GetObject())->m_facing[Z];
			initial_horiz_speed = horizontal_boost;
		}
	}
	
	// give a slight upward pop
	if (skated_off_edge)
	{
		vel[Y] = Mth::Max(vel[Y], GetPhysicsFloat(CRCD(0x95a79c32, "Physics_Acid_Drop_Pop_Speed")));
	}
	
	// but limit upward speed to something reasonable
	if (!mp_physics_control_component->IsWalking() || mp_walk_component->m_state != CWalkComponent::WALKING_GROUND)
	{
		vel[Y] = Mth::Min(vel[Y], 2.0f * GetPhysicsFloat(CRCD(0x95a79c32, "Physics_Acid_Drop_Pop_Speed")));
	}
	
	// grab the acceleration we will have during our acid drop
	SetFlagTrue(SPINE_PHYSICS);
	float acceleration = get_air_gravity();
	SetFlagFalse(SPINE_PHYSICS);
	
	// calculate what height we would have if we used our current horizontal velocity to reach the target position
	float final_height;
	if (distance > 0.0f && initial_horiz_speed > 0.0001f)
	{
		float time_to_target = distance / initial_horiz_speed;
		final_height = pos[Y] + vel[Y] * time_to_target + 0.5f * acceleration * time_to_target * time_to_target;
	}
	else
	{
		// for backwards acid drops, just act as through we are directly over the target point
		final_height = pos[Y];
	}
	
	// if we need to jump up to the target
	if (mp_physics_control_component->IsWalking() && vel[Y] > 0.0f && pos[Y] < target[Y])
	{
		// check to see if we'll ever reach that height with our effective upward velocity
		float max_height = pos[Y] + vel[Y] * vel[Y] / (-2.0f * acceleration);
		float time_to_target = Mth::Abs(distance) / initial_horiz_speed;
		float time_to_max_height = vel[Y] / -acceleration;
		if (time_to_target < time_to_max_height)
		{
			// effectively, this means that we're willing to reduce our horizontal boost in order allow more time to reach the required height
			final_height = max_height;
		}
	}
	
	// if we can't reach the target with our current velocity, ditch the acid drop
	if (final_height < target[Y])
	{
		Nx::CCollCacheManager::sDestroyCollCache(p_coll_cache);
		vel = hold_vel;
		return false;
	}
	
	// calculate the air time before the acid drop would hit its true target; prevent acid drops from occuring moments before landing
	SetFlagTrue(SPINE_PHYSICS);
	float time_to_reach_target_height = calculate_time_to_reach_height(original_target_height, pos[Y], vel[Y]);
	SetFlagFalse(SPINE_PHYSICS);
	if (time_to_reach_target_height < Script::GetFloat(CRCD(0x32c20f7e, "Physics_Acid_Drop_Min_Air_Time")))
	{
		Nx::CCollCacheManager::sDestroyCollCache(p_coll_cache);
		vel = hold_vel;
		return false;
	}
	
	// ensure that we have a clear shot to the target
	
	bool clear_path = false;
    
	// keep shifting our target point up until we can get a clear shot to it, or we get to an unreachable height
	while (target[Y] < final_height)
	{
		feeler.m_start = pos;
		
		// check a path constructed from two concatenated lines, with the midpoint halfway along the acid drop trajectory; this is an attempt
		// to allow most acid drop which might be disallowed by a ledge which would block a straight line
		
		// calculate the time span required to fall to the target height
		SetFlagTrue(SPINE_PHYSICS);
		float half_time_to_reach_target_height = 0.5f * calculate_time_to_reach_height(target[Y], pos[Y], vel[Y]);
		SetFlagFalse(SPINE_PHYSICS);
		
		// calculate the spine velocity which would be used for this target
		float required_speed = 0.5f * distance / half_time_to_reach_target_height;
		
		// calculate the height we will be at halfway through the acid drop
		float height_halfway = pos[Y] + vel[Y] * half_time_to_reach_target_height
			+ 0.5f * acceleration * Mth::Sqr(half_time_to_reach_target_height);
		
		// calculate the point halfway through the acid drop
		Mth::Vector halfway_point = pos;
		halfway_point[Y] = height_halfway;
		halfway_point += required_speed * half_time_to_reach_target_height * drop_direction;
		
		// check for collisions alone the two-line path
		feeler.m_end = halfway_point;
		if (!feeler.GetCollision())
		{
			// feeler.DebugLine(255, 255, 0);
			feeler.m_start = feeler.m_end;
			feeler.m_end = target;
			feeler.m_end[Y] += 1.0f;
			if (!feeler.GetCollision())
			{
				// feeler.DebugLine(255, 255, 0);
				clear_path = true;
				break;
			}
			else
			{
				// feeler.DebugLine(0, 0, 0, 0);
			}
		}
		
		// feeler.DebugLine(0, 0, 0, 0);
		
		// try a higher target point
		target[Y] += 24.0f;
	}
	
	// no clear path was found along the acid drop
	if (!clear_path)
	{
		Nx::CCollCacheManager::sDestroyCollCache(p_coll_cache);
		vel = hold_vel;
		return false;
	}
	DUMP_VELOCITY;
	
	Nx::CCollCacheManager::sDestroyCollCache(p_coll_cache);
	
	acid_drop_data.target_pos = target;
	acid_drop_data.target_normal = target_normal;
	acid_drop_data.true_target_height = original_target_height;
	return true;
}

void CSkaterCorePhysicsComponent::enter_acid_drop ( const SAcidDropData& acid_drop_data )
{
	const Mth::Vector& target_pos = acid_drop_data.target_pos;
	const Mth::Vector& target_normal = acid_drop_data.target_normal;
	const float& true_target_height = acid_drop_data.true_target_height;
	
	// setup the skater state for the acid drop
	
	Mth::Vector horizontal_offset = target_pos - GetPos();
	horizontal_offset[Y] = 0.0f;
	float distance = horizontal_offset.Length();
	if (Mth::DotProduct(horizontal_offset, GetVel()) < 0.0f)
	{
		distance = -distance;
	}
	Mth::Vector drop_direction = horizontal_offset / distance;
	
	// calculate the spine speed required to reach the target given our current vertical velocity
	SetFlagTrue(SPINE_PHYSICS);
	float time_to_reach_target_height = calculate_time_to_reach_height(target_pos[Y], GetPos()[Y], GetVel()[Y]);
	float required_speed = distance / time_to_reach_target_height;
	SetFlagFalse(SPINE_PHYSICS);
	mp_state_component->m_spine_vel.Set(required_speed * drop_direction[X], 0.0f, required_speed * drop_direction[Z]);
	acid_hold = mp_state_component->m_spine_vel;
	
	// once we reach this height, the skater's horizontal velocity will be zeroed out
	m_transfer_target_height = target_pos[Y];
	
	// Gfx::AddDebugStar(target, 24.0f, RED, 0);
	
	// enter the acid drop state
	SetFlagTrue(SPINE_PHYSICS);
	SetFlagTrue(VERT_AIR);
	SetFlagTrue(IN_ACID_DROP);
	SetFlagFalse(TRACKING_VERT);
	SetFlagFalse(AUTOTURN);
	
	// zero our horizontal velocity
	GetVel()[X] = 0.0f;
	GetVel()[Z] = 0.0f;
	DUMP_VELOCITY;
	
	// setup the acid drop's matrix slerp
	
	Mth::Matrix acid_drop_slerp_start = GetMatrix();
	
	// calculate the facing we want when we land; retain our horizontal direction and choose a vertical component which puts us parallel so the target
	// poly's plane
	Mth::Vector land_facing = drop_direction;
	land_facing[Y] = -(land_facing[X] * target_normal[X] + land_facing[Z] * target_normal[Z]) / target_normal[Y];
	land_facing.Normalize();
	
	// calculate goal matrix
	Mth::Matrix acid_drop_slerp_goal;
	acid_drop_slerp_goal[Z] = land_facing;
	acid_drop_slerp_goal[Z].ProjectToPlane(target_normal);
	acid_drop_slerp_goal[Z].Normalize();
	acid_drop_slerp_goal[Y] = target_normal;
	acid_drop_slerp_goal[X] = Mth::CrossProduct(acid_drop_slerp_goal[Y], acid_drop_slerp_goal[Z]);
	acid_drop_slerp_goal[W].Set();
	
	// store the goal facing for use in adjusting the velocity at land time
	m_transfer_goal_facing = acid_drop_slerp_goal[Z];
	
	// setup a good camera matrix for the acid drop (before applying any deviation preserving adjustments)
	m_acid_drop_camera_matrix = acid_drop_slerp_goal;
	if (m_acid_drop_camera_matrix[Z][Y] > 0.0f)
	{
		m_acid_drop_camera_matrix[Z] *= -1.0f;
		m_acid_drop_camera_matrix[X] *= -1.0f;
	}
	
	// if the skater is entering the acid drop with an odd facing due to rotation, we want to preserve that angle in the slerp's goal matrix
	
	// calculate the deviation between the skater's velocity and facing
	Mth::Vector horizontal_facing = GetMatrix()[Z];
	horizontal_facing[Y] = 0.0f;
	float angle = Mth::GetAngleAbout(horizontal_facing, drop_direction, GetMatrix()[Y]);
	
	// rotate goal facing to reflect the deviation in the initial facing
	acid_drop_slerp_goal.RotateYLocal(-angle);

	// setup the slerp state
	m_transfer_slerper.setMatrices(&acid_drop_slerp_start, &acid_drop_slerp_goal);
	m_transfer_slerp_timer = 0.0f;
	m_transfer_slerp_duration = time_to_reach_target_height;
	m_transfer_slerp_previous_matrix = acid_drop_slerp_start;
	
	// trigger the appropriate script
	Script::CStruct* p_params = new Script::CStruct;
	p_params->AddFloat(CRCD(0xbb00fe40, "DropHeight"), GetPos()[Y] - true_target_height);
	GetObject()->SpawnAndRunScript(CRCD(0xc7ed5fef, "SkaterAcidDropTriggered"), -1, false, false, p_params);
	
	// no late jumps during an acid drop
	GetObject()->RemoveEventHandler(CRCD(0x8ffefb28, "Ollied"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
                    
void CSkaterCorePhysicsComponent::handle_post_transfer_limit_overrides (   )
{
	// After a transfer, if we are above our standard speed limits, we ignore those limits for a short duration.  Over the duration the speed limits
	// lerp from our starting speed down to the standard speed limits.  Only non-air time is counted towards the duration.
	
	// detect leaving an acid drop
	if (m_began_frame_in_transfer && !GetFlag(SPINE_PHYSICS) && !GetFlag(IS_BAILING))
	{
		// only override limits if we start above the limits
		float standard_max = GetSkater()->GetScriptedStat(CRCD(0x2eacddb3, "Skater_Max_Speed_Stat"));
		float speed = GetVel().Length();
		if (speed < 1.25f * standard_max) return;
		
		// setup state to ignore speed limits
		m_transfer_overrides_factor = Mth::Min(speed / standard_max, GetPhysicsFloat(CRCD(0xc6b38be0, "Physics_Transfer_Speed_Limit_Override_Max")));
		
		// large values
		m_override_max = 1e20f;
		m_override_max_max = 1e20f;
	}
	else if (m_transfer_overrides_factor == 1.0f) return;
	
	// turn on speed limit overrides
	m_override_limits_time = -1.0f;
	
	// count our timer down
	m_transfer_overrides_factor -= m_frame_length * GetPhysicsFloat(CRCD(0xf9b006aa, "Physics_Transfer_Speed_Limit_Override_Drop_Rate"));
	
	// end the ignoring of speed limits if the duration is up
	if (m_transfer_overrides_factor < 1.0f)
	{
		m_transfer_overrides_factor = 1.0f;
		m_override_limits_time = 0.0f;
		return;
	}
	
	// grab the standard speed limit
	float standard_max = GetSkater()->GetScriptedStat(CRCD(0x2eacddb3, "Skater_Max_Speed_Stat")); 
	
	// calculate the appropriate speed limit based on the time since the acid drop and the speed at the end of the acid drop
	float time_based_appropriate_max = m_transfer_overrides_factor * standard_max;
	
	// calculate a speed limit based on the current speed; thus, if we break during the ignoring of speed limits, our speed limits will turn back on
	float speed_based_appropriate_max = 1.1f * GetVel().Length();
	
	// take the lowest speed limit; never increase the speed limit
	float appropriate_max;
	if (GetState() != AIR)
	{
		appropriate_max = Mth::Min3(time_based_appropriate_max, speed_based_appropriate_max, m_override_max);
	}
	else
	{
		// in air, don't drop the limits when your current speed drops; otherwise you lose your overrides as the top of vert air
		appropriate_max = Mth::Min(time_based_appropriate_max, m_override_max);
	}
	
	// end the ignoring of speed limits if the duration is up
	if (appropriate_max < standard_max)
	{
		m_transfer_overrides_factor = 1.0f;
		m_override_limits_time = 0.0f;
		return;
	}
	
	// set the artificially high speed limit override
	m_override_max = appropriate_max;
	
	// the max max speed limit will never fall below the standard max max speed limit
	m_override_max_max = m_override_max / standard_max * GetSkater()->GetScriptedStat(CRCD(0x2eacddb3, "Skater_Max_Speed_Stat"));
	
	PERIODIC(10)
	{
		printf("Post-Transfer Speed Limit Overrides:  current / standard = %.2f\n", m_override_max / standard_max);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
                    
float CSkaterCorePhysicsComponent::get_air_gravity (   )
{
	// given that we are in the air, figure out what gravity to use	
	// based on if we are VERT or regular air and the cheat modes
	// make sure that if you use this in calculations, then your flags do not change while you expect it to be the same
	float gravity;

	if (GetFlag(VERT_AIR) || GetFlag(SPINE_PHYSICS))   // Note, spine is treated same as vert
	{							  
		// gravity = GetPhysicsFloat(CRCD(0xfaa40754, "Physics_Air_Gravity")) / GetSkater()->GetScriptedStat(CRCD(0x441c38a0, "Physics_vert_hang_Stat"));
		gravity = GetPhysicsFloat(CRCD(0xfaa40754, "Physics_Air_Gravity")) / GetPhysicsFloat(CRCD(0x441c38a0, "Physics_vert_hang_Stat"));
	}
	else
	{
		// gravity = GetPhysicsFloat(CRCD(0xfaa40754, "Physics_Air_Gravity")) / GetSkater()->GetScriptedStat(CRCD(0xc31ca696, "Physics_Air_hang_Stat"));
		gravity = GetPhysicsFloat(CRCD(0xfaa40754, "Physics_Air_Gravity")) / GetPhysicsFloat(CRCD(0xc31ca696, "Physics_Air_hang_Stat"));
	}
	
	if (Mdl::Skate::Instance()->GetCareer()->GetCheat(CRCD(0x9c8c6df1, "CHEAT_MOON")))
	{
		// Here, set the flag. It may seem redundant, but the above line is very likely
		// to be hacked by gameshark. They probably won't notice this one, which will
		// set the flags as if they had actually enabled the cheat -- which enables us
		// to detect that it has been turned on more easily.
		Mdl::Skate::Instance()->GetCareer()->SetGlobalFlag( Script::GetInteger(CRCD(0x9c8c6df1, "CHEAT_MOON")));
		gravity *= GetPhysicsFloat(CRCD(0xec128f0, "moon_gravity"));
		g_CheatsEnabled = true;
	}
	 
	return gravity;
} 

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				
float CSkaterCorePhysicsComponent::calculate_time_to_reach_height ( float target_height, float pos_Y, float vel_Y )
{
	// s = ut - 1/2 * g * t^2			 (note, -g = a in the more traditional formula)
	// solve this using the quadratic equation, gives us the formula below
	// Note the sign of s is important.....
	float distance = pos_Y - target_height; 
	float velocity = vel_Y;
	float acceleration = -get_air_gravity();
	return (velocity + sqrtf(velocity * velocity + 2.0f * acceleration * distance)) / acceleration; 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::handle_ground_rotation (   )
{
	CControlPad& control_pad = mp_input_component->GetControlPad();	

	float rot = 0.0f; 
	float speed = GetVel().Length();						
	
	if (control_pad.m_left.GetPressed())
	{
		if (!control_pad.m_down.GetPressed())
		{
			rot = GetPhysicsFloat(CRCD(0x374e056b, "Physics_Ground_Rotation"));	
		}
		else
		{
			rot = GetPhysicsFloat(CRCD(0x7933a8ef, "Physics_Ground_Sharp_Rotation")); 
			if (speed < 10.0f)
			{
				// If not moving, then we want more control over turning, so ramp up the turing speed over a second
				int pressed_time = control_pad.m_left.GetPressedTime();
				if (pressed_time < STOPPED_TURN_RAMP_TIME)
				{
					rot = rot * pressed_time / STOPPED_TURN_RAMP_TIME;
				}
			}
		}
	}
	else if (control_pad.m_right.GetPressed())
	{
		if (!control_pad.m_down.GetPressed())
		{
			rot = -GetPhysicsFloat(CRCD(0x374e056b, "Physics_Ground_Rotation"));
		}
		else
		{
			rot = -GetPhysicsFloat(CRCD(0x7933a8ef, "Physics_Ground_Sharp_Rotation")); 
			if (speed < 10.0f)
			{
				// If not moving, then we want more control over turning, so ramp up the turing speed over a second
				int pressed_time = control_pad.m_right.GetPressedTime();
				if (pressed_time < STOPPED_TURN_RAMP_TIME)
				{
					rot = rot * pressed_time / STOPPED_TURN_RAMP_TIME;
				}
			}
		}
	}

	/*
	bool do_cess = false;							
	if (CESS_SLIDE_BUTTONS)
	{
		float cess_turn_min_speed = GetPhysicsFloat(CRCD(0xae84e34a, "cess_turn_min_speed"));
		if (speed > cess_turn_min_speed)
		{
			do_cess = true;
			
			float cess_turn_cap_speed = GetPhysicsFloat(CRCD(0x8242c4fe, "cess_turn_cap_speed"));
			
			float scale = speed;
			if (scale > cess_turn_cap_speed)
			{
				scale = cess_turn_cap_speed;
			}
			scale -= cess_turn_min_speed;
			scale /= cess_turn_cap_speed - cess_turn_min_speed;
		
			rot = rot * scale * GetPhysicsFloat(CRCD(0x22834151, "cess_turn_multiplier"));
		}
	}
	*/

	if (rot == 0.0f) return;
	
	rot *= m_frame_length;
	
	mYAngleIncreased = rot > 0.0f;
	
	// K: Avoid anything that might change the velocity direction if this flag is set.
	if (!m_lock_velocity_direction /* && !do_cess*/)
	{
		GetVel().RotateY(rot);					// Note:  Need to rotate this about UP vector		
		DUMP_VELOCITY;
	}	
	
	GetMatrix().RotateYLocal(rot);				
	m_lerping_display_matrix.RotateYLocal(rot);				
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::remove_sideways_velocity ( Mth::Vector& vel )
{				
	// Remove any non-forward component of the velocity
	
	float speed = vel.Length(); 		// get size of velocity
	if (speed > 0.00001f)
	{
		// if (!USE_BIKE_PHYSICS)
		// {		
			vel *= 1.0f / speed;																// get unit vector in direction of velocity
			float direction = Mth::Sgn(Mth::DotProduct(vel, GetMatrix()[Z]));	// get fwds or backwards
			
			vel = GetMatrix()[Z];												// get forward direction
			vel *= speed * direction;													// apply all speed in this direction
			DUMP_VELOCITY;
		// }
		// else
		// {
		  // Mth::Vector old_vel = vel;
		  
		  // vel.ProjectToNormal(GetMatrix()[Z]); 								// leave forward velocity alone
		  
		  // old_vel -= GetVel();		  														// find remaining sideways velocity
		  
		  // old_vel *= 1.0f - GetPhysicsFloat(CRCD(0x53385759, "cess_Friction"));
		  // vel += old_vel;
		  // DUMP_VELOCITY;
		// }
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::check_side_collisions (   )
{
	// check for collisins left and right of the skater; if we get collisions on both sides then restore him to the original position
	
	Mth::Vector debounce = GetPos();

	float side_col = GetPhysicsFloat(CRCD(0x406b425f, "Skater_side_collide_length"));

	if (check_side(-1.0f, side_col))
	{
		if (check_side(1.0f, side_col))
		{
			GetPos() = debounce;			// two collisions, back to safety
			DUMP_POSITION;
		}
	}
	else
	{
		if (check_side(1.0f, side_col))
		{
			if (check_side(-1.0f, side_col))
			{
				GetPos() = debounce;		// two collisions, back to safety
				DUMP_POSITION;
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCorePhysicsComponent::check_side ( float side, float side_col )
{
	#ifdef STICKY_WALLRIDES
	// Prevous checks might have put us into a wall ride, so just ignore future checks					 
	if (GetState() == WALL)
	{
		return false;
	}
	#endif
			  
	// - - - side collision detection  - - - - - - - - - - - - - - - - -

	m_col_start = GetPos() + GetMatrix()[Y] * GetPhysicsFloat(CRCD(0xb45b39e2, "Skater_side_collide_height"));
	
	float col_len = side_col;
	#ifdef STICKY_WALLRIDES
	if (GetState() == AIR)
	{
		col_len += GetPhysicsFloat(CRCD(0x1c58c8b4, "Skater_air_extra_side_col"));
	}
	#endif
	m_col_end = m_col_start + side * GetMatrix()[X] * col_len;  

	if (!get_member_feeler_collision()) return false;
	
	Mth::Vector WallFloorNormal = m_feeler.GetNormal();

	#ifdef STICKY_WALLRIDES
	if (GetState() == AIR)
	{
		if (check_for_wallride())
		{
			return true;
		}
		else
		{
			// push a bit away from the wall if in the air
			Mth::Vector to_wall = m_feeler.GetPoint();
			to_wall -= m_col_start;				// vector towards the wall
			float push_dist = to_wall.Length();					// distance to wall
			push_dist -= col_len - side_col;	   				// adjust by the extra push we gave
			if (push_dist > 0.0f)								// if closer to wall than side_col
			{
				to_wall.Normalize(push_dist);				    // then get direction to wall, scaled by dist we want to move				
				GetPos() -= to_wall / 10.0f;		    // move 1/10th of the way, for nice lerp
				DUMP_POSITION;
				
				float turn_angle;					
				// Rotate away from wall only if not rotating myself
				// otherwise velocity can be continually rotated one way while 
				// the orientation is rated the other way via the d-pad 
				
				CControlPad& control_pad = mp_input_component->GetControlPad();

				// don't rotate at all if in the air, as it changes our direction, usually not what we want
				if (GetState() != AIR
					&& !control_pad.m_R1.GetPressed()
					&& !control_pad.m_L1.GetPressed() 
					&& !control_pad.m_left.GetPressed() 
					&& !control_pad.m_right.GetPressed()) 
				{
					rotate_away_from_wall(WallFloorNormal, turn_angle, 0.2f); 	// and rotate away from the wall
				}
			}
		}
	}
	else
	#endif
	{
		float angle = Mth::DotProduct(WallFloorNormal, GetMatrix()[Y]);
		// Consider 90+-30 degrees as wall (_0_5)
		// Consider 90+-15 degrees as wall (_0_25)
		if (angle < 0.25f && angle > -0.25f)
		{
			// Undo movement
			GetPos() = m_safe_pos;
			DUMP_POSITION;

			float turn_angle;
			rotate_away_from_wall(WallFloorNormal, turn_angle, 0.2f);

			// Try moving him off the wall:
			CFeeler	feeler(m_feeler.GetPoint() + WallFloorNormal, m_feeler.GetPoint() + WallFloorNormal * side_col);	
			if (!feeler.GetCollision())
			{
				mp_trigger_component->CheckFeelerForTrigger(TRIGGER_BONK, m_feeler);
				if (mp_physics_control_component->HaveBeenReset()) return false;
				
				// Lower skater back down to the ground
				GetPos() = feeler.m_end - GetMatrix()[Y] * GetPhysicsFloat(CRCD(0xb45b39e2,"Skater_side_collide_height"));
				DUMP_POSITION;
				return true;
			}
		}
	}
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCorePhysicsComponent::check_for_wallpush (   )
{
	// requires that m_feeler is a wall
	
	if (!mp_input_component->GetControlPad().m_triangle.GetPressed()) return false;
	
	// last wallpush must not have been too recently
	if (Tmr::ElapsedTime(m_last_wallpush_time_stamp) < Script::GetFloat(CRCD(0x17d543, "Physics_Disallow_Rewallpush_Duration"))) return false;
	
	// wall normal must be opposite our forward direction; just under the maximum flail angle
	if (Mth::DotProduct(GetMatrix()[Z], m_feeler.GetNormal()) >= -sinf(Mth::DegToRad(Script::GetFloat(CRCD(0x1483fd01, "Wall_Bounce_Dont_Slow_Angle")) - 1.0f))) return false;
	
	// last wallplant must not have been too recently
	if (Tmr::ElapsedTime(m_last_wallplant_time_stamp) < Script::GetFloat(CRCD(0x17d543, "Physics_Disallow_Rewallpush_Duration"))) return false;
	
	// throw a wallpush event for the scripts
	GetObject()->SelfEvent(CRCD(0x4c03635b, "WallPush"));
	
	// check to see if the wallpush has been canceled during the event
	if (GetFlag(CANCEL_WALL_PUSH))
	{
		SetFlagFalse(CANCEL_WALL_PUSH);
		return false;
	}
	
	// reverse direction of velocity perpendicular to the wall
	Mth::Vector perp_vel = Mth::DotProduct(GetVel(), m_feeler.GetNormal()) * m_feeler.GetNormal();
	GetVel() -= 2.0f * perp_vel;
	
	// damp horizontal velocity
	float speed = GetVel().Length();
	if (speed > 0.001f)
	{
		GetVel() *= Mth::Max(
			Script::GetFloat(CRCD(0xb78542c2, "Physics_Wallpush_Min_Exit_Speed")),
			speed - Script::GetFloat(CRCD(0x1112fb1c, "Physics_Wallpush_Speed_Loss"))
		) / speed;
	}
	else
	{
		GetVel() = -Script::GetFloat(CRCD(0xb78542c2, "Physics_Wallpush_Min_Exit_Speed")) * GetMatrix()[Z];
	}
	
	// project the resulting velocity into the ground's plane
	GetVel().RotateToPlane(m_current_normal);
    
	DUMP_VELOCITY;
	
	// set orientation along new velocity
	GetMatrix()[Z] = GetVel();
	GetMatrix()[Z].Normalize();
	GetMatrix()[Y] = m_current_normal;
	GetMatrix()[X] = Mth::CrossProduct(GetMatrix()[Y], GetMatrix()[Z]);
	ResetLerpingMatrix();
	
	// time stamp the wallplant
	m_last_wallpush_time_stamp = Tmr::GetTime();
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCorePhysicsComponent::check_for_wallplant (   )
{
	// requires that m_feeler is a wall
	
	// we must be somewhat vertical
	if (GetMatrix()[Y][Y] < 0.1f) return false;
	
	// last wallplant must not have been too recently
	if (Tmr::ElapsedTime(m_last_wallplant_time_stamp) < Script::GetFloat(CRCD(0x82135dd7, "Physics_Disallow_Rewallplant_Duration"))) return false;
	
	// not when you're too near the ground
	if (mp_state_component->m_height < Script::GetFloat(CRCD(0xd5349cc6, "Physics_Min_Wallplant_Height"))) return false;
	
	// wall must be substantially vertical
	if (!(m_feeler.GetFlags() & mFD_VERT) && Mth::Abs(m_feeler.GetNormal()[Y]) > 0.1f) return false;
	
	float speed = GetVel().Length();
	if (speed < 0.01f) return false;
	Mth::Vector forward = GetVel() / speed;
		
	// horizontal wall normal must be opposite our horizontal velocity
	Mth::Vector horizontal_forward = forward;
	horizontal_forward[Y] = 0.0f;
	horizontal_forward.Normalize();
	Mth::Vector horizontal_normal = m_feeler.GetNormal();
	horizontal_normal[Y] = 0.0f;
	horizontal_normal.Normalize();
	if (Mth::DotProduct(horizontal_forward, horizontal_normal) > -sinf(Mth::DegToRad(Script::GetFloat(CRCD(0x8f79cc1c, "Physics_Wallplant_Min_Approach_Angle"))))) return false;
	
	// here we attempt to stop wallplant when in is more likely that the player is going for a grind
	if (GetVel()[Y] > 0.0f && mp_input_component->GetControlPad().m_triangle.GetPressed())
	{
		Mth::Vector wall_point = m_feeler.GetPoint();
		Mth::Vector	wall_normal = m_feeler.GetNormal();

		Mth::Vector wall_up_vel(0.0f, GetVel()[Y] * 0.15f, 0.0f);		// check 0.15 seconds ahead
		wall_up_vel.RotateToPlane(wall_normal);  

		// check at what height will be in two frames
		wall_point += wall_up_vel;		

		CFeeler feeler(wall_point + wall_normal * 6.0f, wall_point - wall_normal * 6.0f);
		if (!feeler.GetCollision())
		{
			#ifdef __USER_DAN__
			if (Script::GetInteger(CRCD(0x3ae85eef, "skater_trails")))
			{
				feeler.DebugLine(255, 255, 0, 0);
			}
			#endif
			return false;
		}
		else
		{
			#ifdef __USER_DAN__
			if (Script::GetInteger(CRCD(0x3ae85eef, "skater_trails")))
			{
				feeler.DebugLine(255, 0, 255, 0);
			}
			#endif
		}
	}
		
	// check for wallplant trick
	// K: Modified this to take an array of triggers, so that Kurt could check out using
	// Down,X DownLeft,X or DownRight,X as a trigger.
	bool triggered=false;
	Script::CArray *p_trick_query_array=Script::GetArray(CRCD(0x5d1f84a7, "Wallplant_Trick"));
	for (uint32 i=0; i<p_trick_query_array->GetSize(); ++i)
	{
		Script::CStruct *p_trick_query_struct=p_trick_query_array->GetStructure(i);
		if (mp_trick_component->QueryEvents(p_trick_query_struct))
		{
			triggered=true;
			break;
		}
	}
	if (!triggered)
	{
		return false;
	}
	
	// trip wallplant triggers
	mp_trigger_component->CheckFeelerForTrigger(TRIGGER_JUMP_OFF, m_feeler);
	if (mp_physics_control_component->HaveBeenReset()) return true;
	
	// zero vertical velocity
	GetVel()[Y] = 0.0f;
	
	// reverse velocity in the direction of the wall's normal
	Mth::Vector perp_vel = Mth::DotProduct(GetVel(), horizontal_normal) * horizontal_normal;
	GetVel() -= 2.0f * perp_vel;
	
	// damp horizontal velocity
	float horizontal_speed = sqrtf(GetVel()[X] * GetVel()[X] + GetVel()[Z] * GetVel()[Z]);
	if (horizontal_speed > 0.0001f)
	{
		GetVel()[Y] = 0.0f;
		GetVel().Normalize(Mth::Max(
			Script::GetFloat(CRCD(0x7cee396c, "Physics_Wallplant_Min_Exit_Speed")),
			horizontal_speed - Script::GetFloat(CRCD(0x9b2d4a3, "Physics_Wallplant_Speed_Loss"))
		));
	}
	else
	{
		GetVel() = -Script::GetFloat(CRCD(0x7cee396c, "Physics_Wallplant_Min_Exit_Speed")) * horizontal_forward;
	}
	
	// replace vertical velocity with a wallplant boost
	GetVel()[Y] = Script::GetFloat(CRCD(0x74957fa, "Physics_Wallplant_Vertical_Exit_Speed"));
	
	if (m_feeler.IsMovableCollision())
	{
		// if the wall is moving, we are now in contact with it
		if (!mp_movable_contact_component->HaveContact() || m_feeler.GetMovingObject() != mp_movable_contact_component->GetContact()->GetObject())
		{
			mp_movable_contact_component->LoseAnyContact();
			mp_movable_contact_component->ObtainContact(m_feeler.GetMovingObject());
		}
	}
	
	DUMP_VELOCITY;
	
	// move to just outside the wall, insuring that there is no additional collision along the line to that point
	m_feeler.m_start = m_feeler.GetPoint();
	m_feeler.m_end = m_feeler.GetPoint() + Script::GetFloat(CRCD(0x24be8f0, "Physics_Wallplant_Distance_From_Wall")) * m_feeler.GetNormal();
	if (m_feeler.GetCollision())
	{
		GetPos() = m_feeler.GetPoint() + 0.1f * m_feeler.GetNormal();
	}
	else
	{
		GetPos() = m_feeler.m_end;
	}
	DUMP_POSITION;
	
	// set orientation along new velocity
	GetMatrix()[Z] = GetVel();
	GetMatrix()[Z][Y] = 0.0f;
	GetMatrix()[Z].Normalize();
	GetMatrix()[Y].Set(0.0f, 1.0f, 0.0f);
	GetMatrix()[X] = Mth::CrossProduct(GetMatrix()[Y], GetMatrix()[Z]);
	ResetLerpingMatrix();
	
	// time stamp the wallplant
	m_last_wallplant_time_stamp = Tmr::GetTime();
	
	// throw a wallplant event for the scripts
	GetObject()->SelfEvent(CRCD(0xcf74f6b7, "WallPlant"));
	
	// trick off the object
	mp_trick_component->TrickOffObject(m_feeler.GetNodeChecksum());
	
	// turn back on orientation control in case we just came out of walking
	SetFlagFalse(NO_ORIENTATION_CONTROL);
	
	// acid drops are always allowed after a wallplant
	SetFlagFalse(AIR_ACID_DROP_DISALLOWED);
	
	// let the camera know we're snapping our position slightly
	SetFlagTrue(SNAPPED);
	
	// enter wallplant state
	SetState(WALLPLANT);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCorePhysicsComponent::check_for_wallride (   )
{
	// Requires that m_feeler is a valid wall
	
	if (!m_col_flag_wallable) return false;
		
	// Allow a wall-ride attempt if triangle being pressed & long enough after the last wall-ride.	
	if ((mp_trick_component->GetButtonState(CRCD(0x20689278, "Triangle")) || mp_trick_component->TriggeredInLastNMilliseconds(
			CRCD(0x20689278, "Triangle"),
			1000 * GetPhysicsFloat(CRCD(0x5d241b00, "Wall_Ride_Triangle_Window"))
		)) && static_cast< int >(Tmr::ElapsedTime(mWallrideTime)) > 1000 * GetPhysicsFloat(CRCD(0xf0c74ec2, "Wall_Ride_Delay")))
	{
		////////////////////////////////////////////////
		// check to see if we are going upwards, and within 0.15 seconds from the top of the wall (based on current up speed)
		// and we are going upwards, then don't wallride, as we will probably snap into a rail soon after
		//

		if (GetVel()[Y] > 0.0f)
		{
			Mth::Vector wall_point = m_feeler.GetPoint();
			Mth::Vector	wall_normal = m_feeler.GetNormal();
	
			Mth::Vector wall_up_vel(0.0f, GetVel()[Y] * 0.15f, 0.0f);		// check 0.15 seconds ahead
			wall_up_vel.RotateToPlane(wall_normal);  
			
			// check at what height will be in two frames
			wall_point += wall_up_vel;		
			
			CFeeler check_feeler(wall_point + wall_normal * 6.0f, wall_point - wall_normal * 6.0f);
			if (!check_feeler.GetCollision()) return false;
		}	
		
		////////////////////////////////////////////////
		mWallNormal = m_feeler.GetNormal();
		
		Mth::Vector SquashedWallNormal = mWallNormal;			 // wall normal in the XZ plane
		SquashedWallNormal[Y] = 0;
		SquashedWallNormal.Normalize();
		
		Mth::Vector SquashedVel = GetVel();			 			// velocity in the XZ plane
		SquashedVel[Y] = 0;
		SquashedVel.Normalize();
		
		// Calculate the speed "along" the wall (i.e., in the XZ plane) needed to have enough speed along the wall to make the wallride viable 
		// (otherwise we just go up and down, and end up at odd angles)
		Mth::Vector vel_along_wall = GetVel();
		vel_along_wall[Y] = 0;
		vel_along_wall.ProjectToPlane(SquashedWallNormal);
		float speed_along_wall = vel_along_wall.Length();
		
		if (speed_along_wall < GetPhysicsFloat(CRCD(0xf0636a67, "Wall_Ride_Min_Speed"))) return false;
		
		float dot = fabsf(Mth::DotProduct(SquashedVel, SquashedWallNormal));
		
		// If all angles OK then trigger a wall-ride.
		if (mWallNormal[Y] > -sinf(Mth::DegToRad(GetPhysicsFloat(CRCD(0xae23e850, "Wall_Ride_Upside_Down_Angle")))) && 
			dot < sinf(Mth::DegToRad(GetPhysicsFloat(CRCD(0x6ac9f64d, "Wall_Ride_Max_Incident_Angle")))) && 
			GetMatrix()[Y][Y] > cosf(Mth::DegToRad(GetPhysicsFloat(CRCD(0xbfcbb7e8, "Wall_Ride_Max_Tilt")))))
		{
			if (!mp_trick_component->GraffitiTrickStarted())
			{
				// clear the graffiti trick buffer if we're moving to a new
				// world sector, but haven't started our trick yet...
				mp_trick_component->SetGraffitiTrickStarted(false);
			}

			// landed in a wall ride, check for triggers associated with this object
			// note we pass "TRIGGER_LAND_ON", perhaps not semanticaly correct but we use it for landing on rails,
			// so the meaning is the same across ground-rail-wallride
			set_last_ground_feeler(m_feeler);
			mp_trigger_component->CheckFeelerForTrigger(TRIGGER_LAND_ON, m_last_ground_feeler);
			if (mp_physics_control_component->HaveBeenReset()) return false;

			Mth::Vector	point = m_feeler.GetPoint();				   

			// check to see if we are going into a corner
			// by checking the vector from out current pos along a line parallel to our movment rotated onto the wall.
			Mth::Vector	forward_vel = GetPos() - GetOldPos();
			forward_vel.RotateToPlane(mWallNormal);
			m_col_start = GetPos();
			m_col_end = GetPos() + forward_vel * 3.0f;
			if (get_member_feeler_collision()) return false;
			
			// Record start time.
			mWallrideTime = static_cast< int >(Tmr::GetTime());
			
			// Snap position to wall.
			// we don't want to snap too close
			// previously we moved him an inch away from the wall after snapping him to the collision point.
			// However, this seemed to cause problems in corner so now I just move him back one inch along the collision line
			// which is hence guarenteed not to push him through walls.
			
			GetPos() = point;	   					// move skater
			GetPos() += mWallNormal;	// move away from surface
			DUMP_POSITION;
			
			// Rotate velocity to plane.
			GetVel().RotateToPlane(mWallNormal);
			DUMP_VELOCITY;
			
			// Mick: if we set the velocity as direction, skater will keep going up more
			GetMatrix()[Z] = GetVel();
			
			GetMatrix()[Z].Normalize();
			GetMatrix()[Y] = mWallNormal;
			GetMatrix()[X] = Mth::CrossProduct(GetMatrix()[Y], GetMatrix()[Z]);
			GetMatrix()[X].Normalize();
			ResetLerpingMatrix();

			// Decide whether left or right wall-ride.
			#if 0 // old code
			// Mth::Vector Up(0.0f, 1.0f, 0.0f);
			// Mth::Vector Cross = Mth::CrossProduct(mWallNormal, Up);
			#else
			Mth::Vector Cross(-mWallNormal[Z], 0.0f, mWallNormal[X], 0.0f);
			#endif
			
			if (Mth::DotProduct(GetVel(), Cross) < 0.0f)
			{
				// Let the script do any extra logic, like playing anims & stuff.
				FLAGEXCEPTION(CRCD(0x5de19c83, "WallRideLeft"));
			}
			else
			{
				FLAGEXCEPTION(CRCD(0x51372712, "WallRideRight"));
			}
			
			SetState(WALL);

			// Handle contact			
			check_movable_contact();
			
			return true;
		}	
		else
		{
			// Otherwise trigger the bail script.
			// FLAGEXCEPTION(CRCD(0x2ec3c7f5, "WallRideBail"));
		}
	}
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CSkaterCorePhysicsComponent::rotate_away_from_wall ( const Mth::Vector& normal, float &turn_angle, float lerp )
{
	// given a wall normal, then calculate the "turn_angle" to rotate the skater by and rotate the matrix and display matrix by this
	// return the angle between the skater and the wall
	// note turn_angle is passed by reference, and is altered !!!!
	
	// given m_right(dot)normal, we should be able to get a nice angle
	float dot_right_normal = Mth::DotProduct(GetMatrix()[X], normal);

	float angle = acosf(Mth::Clamp(dot_right_normal, -1.0f, 1.0f)); 	

	if (angle > Mth::PI / 2.0f)
	{
		angle -= Mth::PI;
	}
	
	// angle away from the wall
	turn_angle = angle * GetPhysicsFloat(CRCD(0xe07ee1a9, "Wall_Bounce_Angle_Multiplier")) * lerp;
	
	// Rotate the skater so he is at a slight angle to the wall, especially if we are in a right angled corner, where the skater will bounce out

	GetVel().RotateY(turn_angle);
	DUMP_VELOCITY;
	GetMatrix().RotateYLocal(turn_angle);
	m_lerping_display_matrix.RotateYLocal(turn_angle);	   
	
	return angle;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::check_leaning_into_wall (   )
{
	// If we are leaning left or right, then do a collision check in that direction, and proportional to the amount of the lean.
	// If there is a collision, then push the skater away from the wall.
	
	// First determine if we actually need to be doing this
	
	// Need to be moving at a reasonable rate
	if (GetVel().Length() > GetPhysicsFloat(CRCD(0xbc33d268, "Skate_min_wall_lean_push_speed"))) return;
	
	CControlPad& control_pad = mp_input_component->GetControlPad();

	float time;
	if (control_pad.m_left.GetPressed())
	{
		time = control_pad.m_left.GetPressedTime();
	}
	else if (control_pad.m_right.GetPressed())
	{
		time = control_pad.m_right.GetPressedTime();
	}
	else
	{
		return;
	}
	time *= 1.0f / 1000.0f;

	// Calculate the length of the vector based on how long you have held down the left or right button.  
	// (Ideally it would be tied to the animation, but this is simple, and it works)	
	float min_time = GetPhysicsFloat(CRCD(0x435f0653, "Skate_wall_lean_push_time"));
	if (time > min_time)
	{
		time = min_time;
	}
	
	if (control_pad.m_right.GetPressed())
	{
		time = -time;
	}
	float length = time / min_time * GetPhysicsFloat(CRCD(0x4b8bab12, "Skate_wall_lean_push_length"));	

	
	// Now we've got the length, and in the right direction, check for a collision
	
	m_col_start = GetPos() + GetMatrix()[Y] * GetPhysicsFloat(CRCD(0xbfbbd0af, "Skate_wall_lean_push_height"));
	m_col_end = m_col_start + GetMatrix()[X] * length;
	if (get_member_feeler_collision())
	{
	
		// see how much we are into the wall
		Mth::Vector push = m_feeler.GetPoint() - m_col_end;
		
		// Only push directly out from the wall (like the upwards collision, otherwise we get pushed back)
		Mth::Vector normal = m_feeler.GetNormal();
		push.ProjectToNormal(normal);
		
		m_col_start = GetPos() + GetMatrix()[Y];
		m_col_end = m_col_start + push;
		if (!get_member_feeler_collision())
		{
			// just move him, might put him 1 inch above the ground, but regular physics should snap him down
			GetPos() = m_col_end - GetMatrix()[Y];
			DUMP_POSITION;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::flip_if_skating_backwards (   )
{	
	// if sufficient speed, and facing backwards
	if (!GetFlag(IS_BAILING)
		&& !GetFlag(SKITCHING)		// if skitching, probably just car turning a slow sharp corner
		&& !is_braking()
		&& Mth::DotProduct(GetVel(), GetMatrix()[Z]) < 0.0f
		&& GetVel().LengthSqr() > Mth::Sqr(GetPhysicsFloat(CRCD(0x2bf71eeb, "Skater_Flip_Speed")))
		)
	{
		// flip Z and X, to rotate 180 degrees about y
		GetMatrix()[Z] = -GetMatrix()[Z];
		GetMatrix()[X] = -GetMatrix()[X];
		m_lerping_display_matrix[Z] = -m_lerping_display_matrix[Z];
		m_lerping_display_matrix[X] = -m_lerping_display_matrix[X];

		// Dan: we can no longer flip mid animation
		/*
		CSkaterFlipAndRotateComponent* p_flip_and_rotate_component = GetSkaterFlipAndRotateComponentFromObject(GetObject());
		Dbg_Assert(p_flip_and_rotate_component);
		p_flip_and_rotate_component->ToggleFlipState();
		*/
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCorePhysicsComponent::maybe_flag_ollie_exception (   )
{
	#ifdef __NOPT_ASSERT__
	if (GetFlag(TENSE) && Script::GetInteger("TurboOllie"))
	{
		m_tense_time = GetFlagElapsedTime(TENSE);
		SetFlagFalse(TENSE);
		FLAGEXCEPTION(CRCD(0x8ffefb28, "Ollied"));
		return true;
	}
	#endif
	
	CControlPad& control_pad = mp_input_component->GetControlPad();
	
	if (GetFlag(TENSE) && !control_pad.m_x.GetPressed())
	{
		// Remember the tense time, cause it will be needed when the Jump script command executes.
		m_tense_time = GetFlagElapsedTime(TENSE);
		
		SetFlagFalse(TENSE);
		
		control_pad.m_x.ClearRelease();
		
		FLAGEXCEPTION(CRCD(0x8ffefb28, "Ollied"));
		
		return true;
	}	

	return false;
}  

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
 
void CSkaterCorePhysicsComponent::maybe_skitch (   )
{
	CControlPad& control_pad = mp_input_component->GetControlPad();

	if (GetFlag(SKITCHING) || !SKITCH_BUTTON || GetState() != GROUND) return;
	
	// Find the nearest skitch point.
	Mth::Vector closest_pos;
	float closest_dist = GetPhysicsFloat(CRCD(0x2928f080, "Skitch_Max_Distance"));
	CCompositeObject* p_closest = NULL;
	
	for (CSkitchComponent *p_skitch_comp = static_cast< CSkitchComponent* >(CCompositeObjectManager::Instance()->GetFirstComponentByType(CRC_SKITCH));
		  p_skitch_comp;
		  p_skitch_comp = static_cast< CSkitchComponent* >(p_skitch_comp->GetNextSameType()))
	{
		if (!p_skitch_comp->CanSkitch()) continue;
		
		CCompositeObject* p_composite_object = p_skitch_comp->GetObject();
		if (p_composite_object == GetObject()) continue;
		
		Mth::Vector	skitch_point(0.0f, 0.0f, 0.0f);
		int skitch_index = p_skitch_comp->GetNearestSkitchPoint(&skitch_point, GetPos());
		if (skitch_index == -1) continue;
		
		Mth::Vector line_to = skitch_point - GetPos();
		float dist_to = line_to.Length();
		if ( dist_to < closest_dist)
		{
			closest_dist = dist_to;
			p_closest = p_composite_object;
			closest_pos	= skitch_point;
			m_skitch_index = skitch_index;
		}
	}

	if (p_closest)
	{
		// Clear any triggers, so old L1/R1 presses don't affect us.
		control_pad.m_L1.ClearTrigger();
		control_pad.m_R1.ClearTrigger();
		mp_skitch_object = p_closest;

		FLAGEXCEPTION(CRCD(0x2f184eb1, "Skitched"));
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::do_in_air_physics (   )
{
	CControlPad& control_pad = mp_input_component->GetControlPad();

	// Acceleration is re-calculated every frame
	// here it is just set to the air gravity 
	// there are generally no other forces that we would use in the air

	Mth::Vector acc(0.0f, get_air_gravity(), 0.0f);

	SetFlagFalse(LAST_POLY_WAS_VERT);
	
	// disallow acid drops before this frame's breaking of vert air
	if (GetFlag(VERT_AIR) || GetFlag(SPINE_PHYSICS) || GetFlag(IN_ACID_DROP) || GetFlag(IS_BAILING))
	{
		SetFlagTrue(AIR_ACID_DROP_DISALLOWED);
	}
	
	///////////////////////////////////////////////////////////////////////////////
	// Adjust orientation before we do any movement
	// in the air the orientation is independent of the velocity but it will affect the collision checks we do (left/right/forwards/up)

	handle_air_rotation(); 		// rotate left/right (spinning by holding left/right or L1/R1)
	
	handle_air_lean();			// lean forwards backwards (by hold up/down) also handles spine leans
	
	handle_transfer_slerping();	// during acid drops, we slerp to our required orientation
	
	if (GetFlag(IN_RECOVERY) || VERT_RECOVERY_BUTTONS)
	{
		// no control here, except for the VERT_RECOVERY_BUTTONS, above 
		handle_air_vert_recovery();	// recovering from going off the end of a vert polygon
	}
	
	// Upright if not vert, and not doing a spine transfer
	if (!GetFlag(VERT_AIR) && !GetFlag(SPINE_PHYSICS) && !mp_rotate_component->IsApplyingRotation(X) && !mp_rotate_component->IsApplyingRotation(Z))
	{
		rotate_upright();
	}		

	// end of adjusting orientation in air
	////////////////////////////////////////////////////////////////////
	
	//////////////////////////////////////////////////////////////////////////////
	// Apply special friction even in the air
	
	if (m_special_friction_duration != 0.0f)
	{
		apply_rolling_friction();
	}
	
	// end of friction
	////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	// Calculate what velocity to use
	// generally this is just the stored m_vel, but if we are doing
	// a spine transfer	we a	dd in m_spine_vel
	// (note, we also update the state of the spine transfer here, 
	// perhaps that would be better done elsewhere)
	
	Mth::Vector vel = GetVel();
	
	///////////////////////////////////////////////////////////////////////////

	if (GetFlag(SPINE_PHYSICS))
	{
		// this check is only valid if we are not in contact with a moving object		
		if (vel[Y] < 0.0f && GetPos()[Y] < m_transfer_target_height)	// just check if we have dropped below the target height		
		{
			// if (Mth::Abs(mp_state_component->m_spine_vel.Length() - 0.1f) > 1.0f)
			// {
				// Gfx::AddDebugStar(GetPos(), 24, GREEN, 0);
			// }
			mp_state_component->m_spine_vel.Normalize(0.1f); // make spine velocity very small, but still there, so camera works
		}		
		else
		{
			vel += mp_state_component->m_spine_vel;
		}
	}

	///////////////////////////////////////////////////
	// calculate contact movement
	// If the skater is in contact with an object, then his velocity (m_vel) is considered to be local to that object, so here we have to add in the
	// movement of the object from the last frame; we also account for rotation here, in the call to mp_movable_contact_component->UpdateContact

	Mth::Vector	contact_movement;
	contact_movement.Set(0.0f, 0.0f, 0.0f);

	// if we are in contact with something, then factor in that "movement"
	if (mp_movable_contact_component->UpdateContact(GetPos()))
	{
		contact_movement = mp_movable_contact_component->GetContact()->GetMovement();
	}

	// end of calculating contact movment
	/////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////
	// Update position
	
	// Update position using correct equations of motion (S = ut + 0.5at^2)
	GetPos() += vel * m_frame_length + acc * (0.5f * m_frame_length * m_frame_length);
	DUMP_POSITION;
	
	// add in movement due to contact
	GetPos() += contact_movement;
	DUMP_POSITION;
	
	#ifdef __NOPT_ASSERT__
	if (Script::GetInteger(CRCD(0x3ae85eef, "skater_trails")))
	{
		if (GetFlag(SPINE_PHYSICS))
		{
			Gfx::AddDebugLine(GetPos(), GetOldPos(), RED, 0, 0);
		}
		else if (GetFlag(VERT_AIR))
		{
			Gfx::AddDebugLine(GetPos(), GetOldPos(), YELLOW, 0, 0);
		}
		else
		{
			Gfx::AddDebugLine(GetPos(), GetOldPos(), BLUE, 0, 0);
		}
	}
	#endif
	
	//
	// end updating position
	//////////////////////////////////////////////////////////////
	
	
	/////////////////////////////////////////////////////////////////
	// update velocity
	//
	
	// Update Velocity in air	
	GetVel() += acc * m_frame_length;
	DUMP_VELOCITY;
	
	//
	// end updating velocity
	/////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////
	// Handle trying to break VERT	
	//	
	
	if (GetFlag(CAN_BREAK_VERT))
	{
		if (GetFlag(TRACKING_VERT) && GetFlag(VERT_AIR))
		{
			// might want to check how long we've been in the air before allowing this
			// also don't want to do it if we've been doing tricks and stuff 
			
			// first of all, check if there is a collision beneath my feet
			m_col_start = GetPos() + GetMatrix()[Y] * 1.0f;		// bit above the feet
			m_col_end = GetPos() + GetMatrix()[Y] * -23.0f;
			if (!get_member_feeler_collision())
			{
				maybe_break_vert();
				
				// if we did not break vert now, then only allow us to break vert later if we've been tapping the "up" button
				// this is indicated by us having RELEASED or Pressed in the last few ticks
				if (static_cast< int >(control_pad.m_up.GetReleasedTime()) > GetPhysicsInt(CRCD(0x6bb5b751, "Skater_vert_active_up_time"))
					&& static_cast< int >(control_pad.m_up.GetPressedTime()) > GetPhysicsInt(CRCD(0x6bb5b751, "Skater_vert_active_up_time")))
				{
					// "UP" was not pressed any time recently, so don't let us break late
					SetFlagFalse(CAN_BREAK_VERT);	
				}
			}
		}	 
		   
		// Allow us to break vert polys for a while after we ge onto them
		// to make it more robust
		// Essentially lets CAN_BREAK_VERT flag expire....
		if (static_cast< int >(Tmr::ElapsedTime(GetFlagTime(CAN_BREAK_VERT))) > GetPhysicsInt(CRCD(0x26495947, "Skater_Vert_Allow_break_Time")))
		{
			SetFlagFalse(CAN_BREAK_VERT);
		}
	}
	else
	{
		// We are flagged as not being able to break vert
		// however, we still want to be able to do this at any time on the up journey
		// if we try break the spine
		if (GetFlag(VERT_AIR) && BREAK_SPINE_BUTTONS && !GetFlag(SPINE_PHYSICS) && GetVel()[Y] > 0.0f)
		{
			maybe_break_vert();
		}
	
	}
	
	//
	// End of Handling breaking Vert
	////////////////////////////////////////
	
	if (!mp_movable_contact_component->HaveContact())
	{
		
		//////////////////////////////////////////////////////////////////////
		// Now update the tracking position before we do a collision check	   
		// "Tracking" is used when we are "vert" (moving in a vertical plane through the air above a quarterpipe), to track the ground directly
		// benath us, and if the QP bends, to move the skater appropiately so he can catch vert air along bent QPs, and round corners. 		 
		
		if (GetFlag(TRACKING_VERT) && GetFlag(VERT_AIR))
		{
			m_col_start[X] = GetPos()[X];
			m_col_start[Y] = m_vert_pos[Y];
			m_col_start[Z] = GetPos()[Z];
	
			m_col_end = m_col_start;
			
			m_col_start += m_vert_normal * 30;			// Away from face
			m_col_end -= m_vert_normal * 30;			// into face
	
			// Now see if there is a collision, and track it if so
	
			// First we try an above this position
			// raising us up by m_vert_upstep
			// which starts at 6 inches, and is halved down until
			// less than half an inch, at which point we stop trying to move up
	
			bool collision;
	
			if (m_vert_upstep > 0.5f)
			{
				m_col_start[Y] += m_vert_upstep;
				m_col_end[Y] += m_vert_upstep;
				// Only check for vert polys, ignore collision info
				collision = get_member_feeler_collision(0, mFD_VERT);  
		
				// If we did not find a collision six inches above, then check back at the old height
				if (!collision)
				{
					m_col_start[Y] -= m_vert_upstep;
					m_col_end[Y] -= m_vert_upstep;	
					m_vert_upstep *= 1.0f / 2.0f;			// binary search will zoom in on the edge		
					collision = get_member_feeler_collision(0, mFD_VERT);  
				}
			}
			else
			{
				collision = get_member_feeler_collision(0, mFD_VERT);  	
			}
	
	
			if (!collision)
			{
				// there is no collision, which usually mean we have gone off the end of a qp but might mean the tracking point has drifted off the top
				// of the very polygon (it might not be exactly horizontal), so try tracking down up to 30 inches to see if we can find it again
				for (int i = 0; i < 10; i++)
				{
					m_col_start[Y] -= 3.0f;
					m_col_end[Y] -= 3.0f;
					collision = get_member_feeler_collision(0, mFD_VERT);  
					if (collision)
					{
						// need to store new m_vert_pos
						// as code below does not generally allow us to go below it
						m_vert_pos[Y] = m_feeler.GetPoint()[Y];
						break;
					}
				}
				
			}
	
	
			// Dot product in the XZ plane is the angle between them			
			float change_dot = sqrtf(
				m_vert_normal[X] * m_feeler.GetNormal()[X] + m_vert_normal[Z] * m_feeler.GetNormal()[Z]
			);
			
			// the dot check is a fix for sharp corners, nearly are right angles, TT#438
			if (collision && change_dot > 0.02f)
			{
				// let's just track it simple for now
				Mth::Vector track_point = m_feeler.GetPoint();
	
				// This is a bit of a patch
				// basically clamp the tracking point at the hihgest level it has reached, that way we can never "slip" down a slope
				// which the rest of the physics makes you do, for reasons best explained with a diagram
				if (m_vert_pos[Y] > track_point[Y])  
				{
					track_point[Y] = m_vert_pos[Y];
				}
				
				// keep vert pos updated (only used for height)
				m_vert_pos = track_point;
				
				GetPos()[X] = track_point[X];	
				GetPos()[Z] = track_point[Z];	
				m_vert_normal = m_feeler.GetNormal();
	
				// offset the jumper away from the plane
				GetPos() += m_vert_normal * (GetPhysicsFloat(CRCD(0x78384871, "Physics_Vert_Push_Out")));
				DUMP_POSITION;
				
				// The normal we might be tracking might not be vert, so need to adjust it so it is fully horizontal
				// it should never be fully horizontal (which would cause it to implode									
				Mth::Vector	flat_normal = m_vert_normal;
				flat_normal[Y] = 0.0f;
				flat_normal.Normalize();
				
				// adjust the orientation to match the plane
				new_normal(flat_normal);
				
				// now velocity
				GetVel().RotateToPlane(flat_normal);
			} 
			else
			{
				SetFlagFalse(TRACKING_VERT);
			}
		}
	}
	
	//
	// End of updating tracking
	////////////////////////////////////////////////////////////////////////////////////////	
    
	///////////////////////////////////////////////////////
	// Adjust the normal while in the air	
	//
	
	// When on an air poly, we have the usual drifting of the UP vector to smooth out changes in the normal
	// (note, we can be in VERT_AIR, but also SPINE_PHYSICS, in whcih case the orientation is controled by the "lean" routine
	// which is leaning the skater forward, so he comes down on the opposing face)
	if (GetFlag(VERT_AIR) && !GetFlag(SPINE_PHYSICS)
		&& !mp_rotate_component->IsApplyingRotation(X) && !mp_rotate_component->IsApplyingRotation(Z))
	{
		adjust_normal();
	}
	else
	{
		// otherwise, the matrix is same for display and physics
		ResetLerpingMatrix();
	}
	
	//
	// End of adjusting normal
	//////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////
	// Handle any collisions resultant from our movement
	// remember at this point m_pos is the new position
	// and we might have gone through a wall
	
	// here again, we are factoring in the contact_movement to the start position											 
	// We check if there is anything in font of us
	bool snapped_up = handle_forward_collision_in_air(GetOldPos() + contact_movement);					

	// If we are now in the WALL (Wallride) state, then just return	
	if (GetState() == WALL || GetState() == WALLPLANT)
	{
		// Maybe call DoWallPhysics() to avoid a glitch? Maybe not, cos GetOldPos() will be wrong ?
		return;
	}	
	
	// check for actual movement collision (along the full line of travel)
	// note we factor in the contact_movement here which effectivly ignores it
	// so contact movement could possibly drag us through a solid object
	// if this is a problem, then we need to add another collision check, just for the contact movement
	m_col_start = GetOldPos() + contact_movement;
	m_col_end = GetPos();
								
	if (!snapped_up && get_member_feeler_collision())
	{
		bool hit_vertical = m_feeler.GetNormal()[Y] < 0.1f;
		
		if (check_for_air_snap_up(GetOldPos() + contact_movement) && (GetVel()[Y] > 10.0f || hit_vertical) && mp_movable_contact_component->GetTimeSinceLastLostContact() > 500)
		{
		}
		else
		{
			Mth::Vector normal = m_feeler.GetNormal();
				
			// set our position (the point of contact) to be that point we just found
			GetPos() = m_feeler.GetPoint();
			
			// Move point up one inch to avoid dropping through geometry with something below it (like canada blades, and park editor stuff)
			GetPos() += normal;
			DUMP_POSITION;
			
			if (m_col_flag_skatable)
			{
				//////////////////////////////////////////////////////////////////////////
				// handle landing on the ground
				// Collided with a skatable face!! yay, let's stick to it!
				
				SetState(GROUND);
				check_movable_contact();
				
				// Used by the LandedFromVert script command.
				// landing from a spine transfer is considered to be the same as landing from vert
				if (GetFlag(VERT_AIR) || GetFlag(SPINE_PHYSICS))
				{
					// Note: Not setting it to false if the VERT_AIR flag is not set.
					// This is because sometimes Scott wants to force the mLandedFromVert to be on from in script. Don't want landing to override that.
					// mLandedFromVert never gets reset by the C-code, only by the ResetLandedFromVert script command.
					mLandedFromVert = true;
					
	 				// This flag however cannot be cleared by script
					// Added this flag for use by ClearPanel_Landed, because mLandedFromVert is false at that point even if landing from vert,
					// due to being cleared by script.
					m_true_landed_from_vert = true;
				}
				else
				{
					m_true_landed_from_vert = false;
				}
					
				mLandedFromSpine = GetFlag(SPINE_PHYSICS);

				set_terrain(m_feeler.GetTerrain());
				
				mp_sound_component->PlayLandSound(GetObject()->GetVel().Length() / GetSkater()->GetScriptedStat(CRCD(0xcc5f87aa, "Skater_Max_Max_Speed_Stat")), mp_state_component->m_terrain);

				if (!mp_trick_component->GraffitiTrickStarted())
				{
					// clear the graffiti trick buffer if we're moving to a new world sector, but haven't started our trick yet...
					mp_trick_component->SetGraffitiTrickStarted(false);
				}

				set_last_ground_feeler(m_feeler);
				mp_trigger_component->CheckFeelerForTrigger(TRIGGER_LAND_ON, m_last_ground_feeler);

				// Now, we landed, and triggered an event, which might have reset us, so we should possibly abort here if we were restarted
				if (mp_physics_control_component->HaveBeenReset())
				{
					// K: I added this here to be consistent with the above change on 7 Mar (see above comment)
					// Need it though?
					FLAGEXCEPTION(CRCD(0x532b16ef, "Landed"));
					return;
				}
				
				
				if (!m_true_landed_from_vert 			// not if coming down off vert
					&& !mLandedFromVert				// and not pretending to (like jumping out of a lip trick)
					&& GetVel()[X] == 0.0f
					&& GetVel()[Z] == 0.0f 
					&& control_pad.m_down.GetPressed())
				{
					// we had just jumped straight up, and are braking, and not on vert
					GetVel()[Y] = 0.0f;	
					DUMP_VELOCITY;
				}
				else
				{
					if (!GetFlag(SPINE_PHYSICS))
					{
						GetVel().ProjectToPlane(m_feeler.GetNormal());	   	// kill vel that is perpendicular to normal
						DUMP_VELOCITY;
					}
					else
					{
						// special landing from transfers to prevent speed loss
						
						Mth::Vector landing_vel = GetVel();
						
						// rotate all velocity to the facing direction
						GetVel().RotateToNormal(m_transfer_goal_facing);
						DUMP_VELOCITY;
						
						// now, rotate into the plane instead of projecting (actually, m_transfer_goal_facing should already be in the plane)
						GetVel().RotateToPlane(m_feeler.GetNormal());
						DUMP_VELOCITY;
						
						// test what velocity we will have once the ground physics removes sideways velocity
						Mth::Vector test_vel = GetVel();
						remove_sideways_velocity(test_vel);
						
						// if we'd be moving upwards (this can occur because sometimes an acid drop's goal facing is along a vert, not down it; and thus,
						// a slight upwards rotation means that the skater will land pointing up and immediately hop off the vert; this looks very
						// bizzare, so we prevent it here)
						if (test_vel[Y] > 0.0f)
						{
							// use a standard landing instead
							GetVel() = landing_vel;
							GetVel().ProjectToPlane(m_feeler.GetNormal());
							DUMP_VELOCITY;
						}
						
						if (GetVel().LengthSqr() < Mth::Sqr(Script::GetFloat(CRCD(0x59484878, "Physics_Acid_Drop_Min_Land_Speed"))))
						{
							GetVel().Normalize(Script::GetFloat(CRCD(0x59484878, "Physics_Acid_Drop_Min_Land_Speed")));
						}
					}
					GetVel().ZeroIfShorterThan(10.0f);
				}
				
				SetFlagFalse(VERT_AIR);
				
				m_display_normal = m_feeler.GetNormal();
				m_current_normal = m_display_normal;
				m_last_display_normal = m_display_normal;
				
				GetMatrix()[Y] = m_display_normal;
				GetMatrix().OrthoNormalizeAbout(Y);   
				ResetLerpingMatrix();

				// Flagging the exceptions needs to be the last thing done, as the script for the
				// excpetion is executed immediately, and might need some of the above values
				// Like, PitchGreaterThan requires m_last_display_matrix to be correct (as calculated above)				
				
				FLAGEXCEPTION(CRCD(0x532b16ef, "Landed"));
				
				return;
				 	
				//
				// end of handling landing
				//////////////////////////////////////////////////////////////
			}
			else
			{
				// it's a wall, bounce off it
				GetVel().RotateToPlane(normal);			
				DUMP_VELOCITY;
				GetMatrix()[Z].RotateToPlane(normal);
				GetMatrix().OrthoNormalizeAbout(Z);
				ResetLerpingMatrix();
							
				// Bit of a patach here to move the skater away from the wall
				GetPos() += normal * GetPhysicsFloat(CRCD(0x23410c14, "Skater_Min_Distance_To_Wall"));
				DUMP_POSITION;
				
				// no acid drops after an in-air collision
				SetFlagTrue(AIR_ACID_DROP_DISALLOWED);
				
			}
		}
	}
	else
	{
		// No forward collision was found, so nothing to do   
		
		// so do another check to push me away from walls, will also need a wallride check here 
		#ifdef		STICKY_WALLRIDES
		if (!GetFlag(VERT_AIR))
		{
			// check if we are too close to a wall, and pop us out and away		
			check_side_collisions();
		}
		#endif
	}
	
	// Push skater down from any roof he might now be under			
	if (handle_upward_collision_in_air())
	{
		// no acid drops after an in-air collision
		SetFlagTrue(AIR_ACID_DROP_DISALLOWED);
	}
	
	//
	// End of handling collisions
	/////////////////////////////////////////////////////////////////////////////////////////////////
	
	// If we've triggered a jump, then do the jump (note, same as jump from ground for now, might want to make it different) 	
	maybe_flag_ollie_exception();
	
	// if in standard vanilla air, check for acid drop
    if (!GetFlag(AIR_ACID_DROP_DISALLOWED) && BREAK_SPINE_BUTTONS)
	{
		bool count_as_skate_off_edge = m_went_airborne_time > m_last_jump_time_stamp
			&& Tmr::ElapsedTime(m_went_airborne_time) < 250;
		
		SAcidDropData acid_drop_data;
		if (maybe_acid_drop(count_as_skate_off_edge, GetPos(), GetOldPos(), GetVel(), acid_drop_data))
		{
			enter_acid_drop(acid_drop_data);
		}
	}
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::handle_air_rotation (   )
{
	CControlPad& control_pad = mp_input_component->GetControlPad();		 

	//////////////////////////////////////////////////////////////////////////////////
	// Part 1 - Decide on what spin to do, based on the controls

	if (mNoSpin) return;
	
	// IF doing a spine transfer, then no auto turning....	
	if (GetFlag(SPINE_PHYSICS))
	{
		SetFlagFalse(AUTOTURN);
	}
	
	int time = 0; 			
	float rot = 0.0f;
	float no_time = 0.0f;
	float ramp_time = 0.0f;

	// if (!USE_BIKE_PHYSICS)
	// {
		if (m_spin_taps)
		{
			if (control_pad.m_L1.GetTriggered())
			{
				control_pad.m_L1.ClearTrigger();
				if (!control_pad.m_R1.GetPressed())	 	// ignore it if R1 is pressed
				{
					m_tap_turns += Mth::PI;
				}
			}
			if (control_pad.m_R1.GetTriggered())
			{
				control_pad.m_R1.ClearTrigger();
				if (!control_pad.m_L1.GetPressed())	 	// ignore it if L1 is pressed
				{
					m_tap_turns -= Mth::PI;
				}
			}
		
			if (m_tap_turns != 0.0f)
			{
				time = 1;
				float turn_amount = Mth::Sgn(m_tap_turns) * GetSkater()->GetScriptedStat(CRCD(0xf7a2acc1, "Physics_air_tap_turn_speed_stat"));
				if (Mth::Abs(turn_amount * m_frame_length) > Mth::Abs(m_tap_turns))
				{
					m_tap_turns = 0.0f;
				}
				else
				{
					m_tap_turns -= turn_amount * m_frame_length;
				}	
				rot = turn_amount;
			}		
		}
		else
		{
			if (control_pad.m_L1.GetPressed() && !control_pad.m_R1.GetPressed())
			{
				rot = GetSkater()->GetScriptedStat(CRCD(0x4957db43, "Physics_air_rotation_stat"));	
				time = 1;
				if (control_pad.m_L1.GetPressedTime() > GetPhysicsFloat(CRCD(0xabdd3395, "skater_autoturn_cancel_time")))
				{
					SetFlagFalse(AUTOTURN);
				}
			}								
			
			if (control_pad.m_R1.GetPressed() && !control_pad.m_L1.GetPressed())
			{
				rot = -GetSkater()->GetScriptedStat(CRCD(0x4957db43, "Physics_air_rotation_stat"));
				time = 1;
				if (control_pad.m_R1.GetPressedTime() > GetPhysicsFloat(CRCD(0xabdd3395, "skater_autoturn_cancel_time")))
				{
					SetFlagFalse(AUTOTURN);
				}
			}								
		}
	// }
	
	// if just transitioned from walking, ignore Left/Right for rotation purposes
	if (!GetFlag(NO_ORIENTATION_CONTROL))
	{
		// if time is set, then we just ignore the Left/Right button and take no account of the spinning ramp
		if (!time)
		{
			if (control_pad.m_left.GetPressed())
			{
				rot = GetSkater()->GetScriptedStat(CRCD(0x4957db43, "Physics_air_rotation_stat"));	
				time = control_pad.m_left.GetPressedTime();
				if (time > GetPhysicsFloat(CRCD(0xabdd3395, "skater_autoturn_cancel_time")))
				{
					SetFlagFalse(AUTOTURN);
				}
			}
		
			if (control_pad.m_right.GetPressed())
			{
				rot = -GetSkater()->GetScriptedStat(CRCD(0x4957db43, "Physics_air_rotation_stat"));	
				time = control_pad.m_right.GetPressedTime();
				if (time > GetPhysicsFloat(CRCD(0xabdd3395, "skater_autoturn_cancel_time")))
				{
					SetFlagFalse(AUTOTURN);
				}
			}
			
			no_time = GetPhysicsFloat(CRCD(0xa092b2be, "Physics_Air_No_Rotate_Time"));
			ramp_time = GetPhysicsFloat(CRCD(0x5d756349, "Physics_Air_Ramp_Rotate_Time"));
		
			// if just tapped, then no leaning for a while
			if (time <= no_time)
			{
				rot = 0;
			}
		
			// if tapped enough, then ramp up to full speed over a small amount of time		
			if (time < ramp_time)
			{
				rot = rot * (time - no_time) / (ramp_time - no_time);
			}
		}
	}
	
	// if we are not rotating by hand, then might want to auto-turn	
	if (!rot && GetFlag(VERT_AIR) && GetFlag(AUTOTURN))
	{
		float angle_from_vert = acosf(Mth::Clamp(GetMatrix()[Z][Y], -1.0f, 1.0f));
		
		if (angle_from_vert < Mth::DegToRad(GetPhysicsFloat(CRCD(0x61224f2e, "skater_autoturn_vert_angle"))))
		{
			// If pointing more or less straight up, then don't auto-turn
			SetFlagFalse(AUTOTURN);				
		}
		else
		{
			float angle = Mth::GetAngleAbout(GetMatrix()[Z], m_fall_line, GetMatrix()[Y]);
							
			rot = Mth::Sgn(angle) * GetPhysicsFloat(CRCD(0x9db33213, "Skater_autoturn_speed"));
			if (Mth::Abs(rot * m_frame_length) > Mth::Abs(angle))
			{
				// just about done, so just turn the last bit of angle left
				rot = angle / m_frame_length;
				SetFlagFalse(AUTOTURN);
			}
			#if 0 // Dan: removed as this code has no effect
			time = 1;	  		// frig, we really don't want to start auto turn immediately
			ramp_time = 0;
			#endif
		}
	}
	
	// End of Part 1
	//////////////////////////////////////////////////////////////////////////////////
	
	if (rot == 0.0f) return;
	
	//////////////////////////////////////////////////////////////////////////////////
	// Part 2 - Do the actual rotation

	mYAngleIncreased = rot > 0.0f;
			
	rot *= m_frame_length;
	GetMatrix().RotateYLocal(rot);				
	m_lerping_display_matrix.RotateYLocal(rot);				
	
	// End of Part 2
	//////////////////////////////////////////////////////////////////////////////////
		
							   
	//////////////////////////////////////////////////////////////////////////////////
	// Part 3 - tracking rotation for the purposes of score (Spin multipliers)								   
	// Keep track of the total angle spun through.
	
	mp_trick_component->mTallyAngles += Mth::RadToDeg(rot);
	
	// Set the spin.
	if (GetFlag(VERT_AIR) && !GetFlag(SPINE_PHYSICS))
	{
		// If in vert air, only count the spin if it is at least 360, because getting 180 is too easy.
		if (Mth::Abs(mp_trick_component->mTallyAngles) >= 360.0f - GetPhysicsFloat(CRCD(0x50c5cc2f, "spin_count_slop")) + 0.1f)
		{
			mp_score_component->GetScore()->UpdateSpin(mp_trick_component->mTallyAngles);
		}	
	}
	else
	{
		mp_score_component->GetScore()->UpdateSpin(mp_trick_component->mTallyAngles);
	}	
	
	// End of Part 3
	//////////////////////////////////////////////////////////////////////////////////
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::handle_air_lean (   )
{						
	// Handle leaning formward and backwards in the air
												 
	//////////////////////////////////////////////////////////////////////////////////
	// Spine specific lean, should be seperated out for spine physics
	// we automatically rotate forward but don't let skaters's up vector to go past horizontal 
	
	// no leaning during transfers; we slerp instead
	if (GetFlag(SPINE_PHYSICS) || GetFlag(NO_ORIENTATION_CONTROL)) return;
	
	#if 0 // old transfer code
	if (GetFlag(SPINE_PHYSICS))
	{
		if (GetMatrix()[Y][Y] > -0.05f)	   // if y component of up vector (the Y vector) is +ve, then we are heads up
		{
			float rot = GetSkater()->GetScriptedStat(CRCD(0x110a1742, "Physics_spine_lean_stat"));
				
			GetMatrix().Rotate(m_spine_rotate_axis, -rot*m_frame_length);
		}
		return;
	}
	#endif
	
	//
	//////////////////////////////////////////////////////////////////////////////////
	
	///////////////////////////////////////////////////////////////////////////////////
	// 1 - Control Logic	
	//
	
	// don't even try to lean if in VERT_AIR		  
	if (GetFlag(VERT_AIR)) return;
	
	CControlPad& control_pad = mp_input_component->GetControlPad();
	
	// Don't want to lean if "grab" or "kick" pressed, as we could be doing some kind of grab trick
	if (control_pad.m_circle.GetPressed() || control_pad.m_square.GetPressed()) return;
	
	float rot = 0.0f;
	int time = 0;
	
	if (control_pad.m_up.GetPressed())
	{
		rot = GetSkater()->GetScriptedStat(CRCD(0xcbfdd841, "Physics_air_lean_stat"));
		time = control_pad.m_up.GetPressedTime();
	}

	if (control_pad.m_down.GetPressed())
	{
		rot = -GetSkater()->GetScriptedStat(CRCD(0xcbfdd841, "Physics_air_lean_stat"));	
		time = control_pad.m_down.GetPressedTime();
	}

	float no_time = GetPhysicsFloat(CRCD(0x5f70ef46, "Physics_Air_No_Lean_Time"));
	float ramp_time = GetPhysicsFloat(CRCD(0x12cd2d14, "Physics_Air_Ramp_Lean_Time"));

	// if just tapped, then no rotation for a while
	if (time <= no_time) return;
	
	// if tapped enough, then ramp up to full speed over a small amount of time		
	if (time < ramp_time)
	{
		rot = rot * (time - no_time) / (ramp_time - no_time);
	}

	// end of control logic
	//////////////////////////////////////////////////////////////////////////////////
	

	//////////////////////////////////////////////////////////////////////////////////
	// air lean physics
	
	if (rot == 0.0f) return;
	
	GetMatrix().RotateXLocal(rot*m_frame_length);				
	m_lerping_display_matrix.RotateXLocal(rot*m_frame_length);				
	
	// end of air lean physics
	//////////////////////////////////////////////////////////////////////////////////
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::handle_transfer_slerping (   )
{
	// during transfers, we slerp to our required orientation
	if (!GetFlag(SPINE_PHYSICS)) return;
	
	// we can't simply use the slerp's result vector at the skater's vector, because we need to allow the skater to rotate; instead, we apply
	// the change in the result vector each frame
	
	// increment the timer
	m_transfer_slerp_timer += m_frame_length;
	
	// calculate this frame's slerp result
	Mth::Matrix transfer_slerp_current_matrix;
	m_transfer_slerper.getMatrix(
		&transfer_slerp_current_matrix,
		Mth::SmoothStep(Mth::ClampMax(m_transfer_slerp_timer / m_transfer_slerp_duration, 1.0f))
	);
	
	// invert the slerp result vector from last frame
	Mth::Matrix inverse_transfer_slerp_previous_matrix = m_transfer_slerp_previous_matrix;
	inverse_transfer_slerp_previous_matrix.InvertUniform();
	
	// calculate the change between the frames
	Mth::Matrix transfer_slerp_delta_matrix = inverse_transfer_slerp_previous_matrix * transfer_slerp_current_matrix;
	
	// apply the change to the skater's matrix
	GetMatrix() *= transfer_slerp_delta_matrix;
	ResetLerpingMatrix();
	
	// store this frame's matrix
	m_transfer_slerp_previous_matrix = transfer_slerp_current_matrix;
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::handle_air_vert_recovery (   )
{						   
	// recovering from going off the end of a vert polygon	

	// don't want to recover during a transfer
	if (GetFlag(SPINE_PHYSICS)) return;

	if (GetVel()[Y] > 0.0f && mp_movable_contact_component->GetContact()) return;
	
	// right, we are in vert, but no longer tracking, so perhaps we are over flat ground?
	CFeeler	feeler;
	feeler.m_start = GetPos();
	feeler.m_end = GetPos();
	feeler.m_end[Y] -= 500.0f;
	
	// can't find ground
	if (!feeler.GetCollision()) return;

	// still over vert poly
	if (feeler.GetFlags() & mFD_VERT) return;
	
	// still over steep ground
	if (feeler.GetNormal()[Y] < 0.2f) return;

	if (GetFlag(VERT_AIR))
	{
		SetFlagFalse(VERT_AIR);
	}

	SetFlagTrue(IN_RECOVERY);

	// we want to smoothly rotate the skater in the plane formed by the up vector, and the skater's up vector
	
	// got sufficently far up 
	if (GetMatrix()[Y][Y] > 0.9f) return;
	
	float rot = GetSkater()->GetScriptedStat(CRCD(0xe7116a96,"Physics_recover_rate_stat"));	
	
	// need to rotate the Y vector in the plane formed by the XZ velocity and the (0,1,0) up vector
	// (or rather, rotate it about the vector prependicular to this
	
	// get a vector perpendicular to the plane containing m_matrix[Z] and the world up 	
	Mth::Vector forward = GetMatrix()[Y];
	forward[Y] = 0.0f;
	forward.Normalize();
	#if 0 // old code
	// Mth::Vector world_up(0.0f,1.0f,0.0f);
	// Mth::Vector side = Mth::CrossProduct(forward,world_up);
	#else
	Mth::Vector side(-forward[Z], 0.0f, forward[X], 0.0f);
	#endif
	
	GetMatrix().Rotate(side, rot * m_frame_length);
	
	ResetLerpingMatrix();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::rotate_upright (   )
{
	// Attempt to upright player (sideways) while going through air
	//
	// Cannot just check angle between mUp and world_up as that would include
	// tilting forward/backward when character is already uprighted sideways
	// (and tilting forward/backward should solely be under player control).
	// Thus, should rotate about mFront only.
	//
	// Then, basically what we want to do is:
	// 1) Rotate about mFront so that mUp is in the plane of world_up and mUp.
	// 2) Pick one of CW/CCW to rotate by, such that angle between mUp and
	//    world_up is <= 90 degrees.
	//
	// As we can be off by at most 180 degrees, rotating by 5 degrees would
	// upright us in 180/5=36 frames. For 45 degrees off (more likely), we
	// would be uprighted in 45/5=9 frames.
	//
	// Your average jump is probably around 30 frames long, so try 1 degree?
	
	// If the skater's Z vector is nearly straight up or down, then 
	// uprighting sideways will not look good 
	// (On XBox, some innacuracy in Z causes the skater to rotate oddly during "drop-in" situations)
	// so, here we test for this case, and just return if so
	if (Mth::Abs(GetMatrix()[Z][Y]) > 0.95f) return;
	
	// get a vector perpendicular to the plane containing m_matrix[Z] and the world up 	
	#if 0 // old code
	// Mth::Vector v;
	// Mth::Vector world_up;
	// world_up.Set(0.0f,1.0f,0.0f);
	// v = Mth::CrossProduct(GetMatrix()[Z],world_up);
	#else
	Mth::Vector v(-GetMatrix()[Z][Z], 0.0f, GetMatrix()[Z][X], 0.0f);
	#endif

	// Then get the dot product of the up vector with this 	
	// if up vector is in the same plane, then this will be zero
	float dot = Mth::DotProduct(GetMatrix()[Y], v);
	if (dot > 0.02f * m_frame_length * 60.0f) // prevent wobbling
	{
		float rot = Mth::DegToRad(Script::GetFloat(CRCD(0xabd57877, "skater_upright_sideways_speed")));
		GetMatrix().RotateZLocal(rot * m_frame_length);				
		ResetLerpingMatrix();
	}
	else if (dot < -0.02f * m_frame_length * 60.0f)
	{
		float rot = Mth::DegToRad(Script::GetFloat(CRCD(0xabd57877, "skater_upright_sideways_speed")));
		GetMatrix().RotateZLocal(-rot * m_frame_length);				
		ResetLerpingMatrix();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCorePhysicsComponent::handle_forward_collision_in_air ( const Mth::Vector &start_pos )
{
	// return true if there was a collision, but we snapped up over it
	
	Mth::Vector	forward = GetPos() - start_pos;
	forward.Normalize();	
	
	Mth::Vector up_offset = GetMatrix()[Y] * GetPhysicsFloat(CRCD(0xd4205c9b, "Skater_First_Forward_Collision_Height"));
	
	m_col_start = start_pos + up_offset;
	m_col_end = GetPos() + up_offset + forward * GetPhysicsFloat(CRCD(0x20102726, "Skater_First_Forward_Collision_Length"));
	
	if (get_member_feeler_collision())
	{
		// we have hit something going forward
		// either it is a wall, and we need to bounce off it
		// or it is a floor, and we need to stick to it.
							 
		Mth::Vector normal = m_feeler.GetNormal();

		float dot = Mth::DotProduct(normal, m_current_normal);
		if (!m_col_flag_skatable || (dot < 0.8f && normal[Y] < 0.5f))
		{

		   // at this point we have hit something that is not skatable and is fairly vertical
		   // so we are considering if we want to bounce off it
		   // one possibility is that it is a low ledge
		   //
		   // so, we check for snapping up, and if we do snap up, and hit_vertical is true, then we don't bounce off the wall
			
			/*
			// View from side
									   /
									  /	 <---------- Movement of skater
									 /
									/
				---------------+   /
							   |  /
							   | /
							   |/
							   /  <-------------  point of collision 
							  /|
							 / |
							/  |
						   /   |
						  /    +---------------	 <-------------- The ground
						 /
						/
					   /
					  /
					 /	<------------ where we end up
			*/
		   
			// Need to store m_feeler, as it is corrupted by CheckForAirSnapUp
			CFeeler tmp_feeler = m_feeler;
			
			bool hit_vertical = m_feeler.GetNormal()[Y] < 0.1f;
			
			// Since this collision had the forward vector added into it, we need to temporarily add in the forward vector again
			// so we can check for snapping up; note this will result in the skater snapping forward by the length of the forward vector (currently 10")
			// whenever we snap up in this particular check		    			
			Mth::Vector	tmp = GetPos();
			GetPos() = m_col_end - up_offset;					
			if (check_for_air_snap_up(start_pos))
			{
				// we snapped up, so if we are either moving upward or the first collision was near vertical, then we return true, allowing us to continue
				// if we were moving upwards, then we would continue into the air
				// if we were moving down, but the collision was near vert, then we do not want to bonk off the surface, and instead we let
				// the landing happen in the next frame
				if (GetVel()[Y] > 10.0f || hit_vertical) return true;
			}
			
			GetPos() = tmp;
			DUMP_POSITION;
			m_feeler = tmp_feeler;

			#ifdef		SNAP_OVER_THIN_WALLS
			// we have hit a wall, and we can't snap up over i;  see if we can just move up and go over it....

			// only do this if we are roughly vertical and going up and the wall is very verticle
			if (GetMatrix()[Y][Y] > 0.5f		   // skater is upright
			    && GetVel()[Y] > 0.0f			   // skater travelling upwards
				&& normal[Y] < 0.01f)					   // thing we hit is not far off vert
			{
				 
				float snap_Y = GetPhysicsFloat(CRCD(0x786b3272, "Physics_Air_Snap_Up"));
				tmp_feeler.m_start[Y] += snap_Y;
				tmp_feeler.m_end[Y] += snap_Y;
						   
				if (!tmp_feeler.GetCollision())
				{
					while (snap_Y > 4.0f)
					{
						// move down until we get a collision
						// so as to minimize the amount we snap up
						tmp_feeler.m_start[Y] -= 2.0f;
						tmp_feeler.m_end[Y] -= 2.0f;
						if (tmp_feeler.GetCollision()) break;
						snap_Y -= 2.0f;
					}
					GetPos()[Y] += snap_Y;
					DUMP_POSITION;
					return true;
				}
			}
			#endif
			
			// If we are on a vert poly, then just slide off the wall
			// just kill velocity perpendicular to the wall
			if (GetFlag(VERT_AIR))
			{
				GetPos() = m_feeler.GetPoint() - up_offset;
				GetPos() += normal * GetPhysicsFloat(CRCD(0x23410c14, "Skater_Min_Distance_To_Wall"));				
				DUMP_POSITION;
				
				if (!GetFlag(SPINE_PHYSICS))
				{
					GetVel().ProjectToPlane(normal);
				}
				else
				{
					GetVel().RotateToPlane(m_feeler.GetNormal());
				}
				DUMP_VELOCITY;
				return false;			
			}
			
			// maybe wall plant
			if (check_for_wallplant()) return false;
			
			// maybe wall ride
			if (check_for_wallride()) return false;
			
			// no acid drops after a in-air collision
			SetFlagTrue(AIR_ACID_DROP_DISALLOWED);
				
			mp_trigger_component->CheckFeelerForTrigger(TRIGGER_BONK, m_feeler);
			if (mp_physics_control_component->HaveBeenReset()) return false;

			// only play bonk sound for things that are near vert 												  
			if (m_feeler.GetNormal().GetY() < 0.05f)
			{
				mp_sound_component->PlayBonkSound(GetObject()->GetVel().Length() / GetSkater()->GetScriptedStat(CRCD(0xcc5f87aa, "Skater_Max_Max_Speed_Stat")), m_feeler.GetTerrain());
			}

			// it's a wall, bounce off it
			
			float vel_y = 0.0f;

			// ignore y vel if the polygon is not too much upside down 
			if (normal[Y] > -0.1f)
			{
				vel_y = GetVel()[Y];					// remember old Y vel
				GetVel()[Y] = 0.0f;					// kill Y vel
				DUMP_VELOCITY;
			}
			
			GetVel().ProjectToPlane(normal);											// project X and Z to plane of collision poly	
			GetVel() += normal * (GetVel().Length() * (1.0f / 10.0f));		// 10% tiny push away.....
			DUMP_VELOCITY;
			
			if (normal[Y] > -0.1f)
			{
				GetVel()[Y] = vel_y;					// restore old Y vel, so it's impossible to jump higher by jumping against a wall
				DUMP_VELOCITY;
			}			
			
			float Z_Y = GetMatrix()[Z][Y];
			GetMatrix()[Z][Y] = 0.0f;
			GetMatrix()[Z].RotateToPlane(normal);
			GetMatrix()[Z][Y] = Z_Y;
			GetMatrix()[Z] += (1.0f / 20.0f) * normal;
			GetMatrix()[Z].Normalize();			
			GetMatrix()[X] = Mth::CrossProduct(GetMatrix()[Y], GetMatrix()[Z]);
			GetMatrix()[X].Normalize();
			GetMatrix()[Y] = Mth::CrossProduct(GetMatrix()[Z], GetMatrix()[X]);
			GetMatrix()[Y].Normalize();
			
			// Bit of a patach here to move the skater away from the wall
			GetPos() = m_feeler.GetPoint() - up_offset;
			GetPos() += normal * GetPhysicsFloat(CRCD(0x23410c14, "Skater_Min_Distance_To_Wall"));
			DUMP_POSITION;
			
			// if the thing we have collided with is a movable object, then add in the last movement to move us away 
			if (m_feeler.IsMovableCollision())
			{
				Mth::Vector obj_vel = m_feeler.GetMovingObject()->GetVel();

				// if object is moving in same direction as face normal
				if (Mth::DotProduct(obj_vel, m_feeler.GetNormal()) > 0.0f)
				{
					// then add in the velocity, just to be on the safe side
					GetPos() += obj_vel * m_frame_length * 2.0f;		// 2.0f is a safety factor
					DUMP_POSITION;
					// and move with the thing we just hit
					GetVel() += obj_vel;					
					DUMP_VELOCITY;
				}
			}
		}
	}
	return false;
}	 

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCorePhysicsComponent::check_for_air_snap_up ( const Mth::Vector& start_pos )
{
	////////////////////////////////////////////////////////////////////////////////////////
	// We moved from GetOldPos() to m_pos
	// and some collision was detected
	// either along this line, or along the "forward" line, which is 8 inches above
	// regardless, we want to see if the new m_pos (considered bad)
	// can be snapped upwards to a surface, where the normal is in the same direction as
	// the current movement
	// and check to see if there is a clear line to this position
	// currently in physics.q:  Physics_Air_Snap_Up = 15       
	//
	// Note, we ignore if the skater is going up or down, the calling code has to 
	// handle that (for the spcial case of hitting a wall coming down, when we can snap over it)

	// don't do it if bailing, to avoid snapping through loops
	if (GetFlag(IS_BAILING)) return false;
	
	CFeeler feeler;
	
	float up_len = GetPhysicsFloat(CRCD(0x786b3272,"Physics_Air_Snap_Up"));
	
	feeler.m_start = GetPos();
	feeler.m_start[Y] += up_len;
	feeler.m_end = GetPos();
	
	// Since the new pos might actually be ABOVE the ledge (i.e., we might have just clipped though the corner, and not be inside it)
	// then we need to check all the way down to the previous height, but only if moving upwards
	if (start_pos[Y] < GetPos()[Y])
	{
		feeler.m_end[Y] = start_pos[Y];		
	}

	// if the start pos, plus the snap up distance was above the top of the collision line, then extend the collision line up to that	
	if (start_pos[Y] + up_len > feeler.m_start[Y])
	{
		feeler.m_start[Y] = start_pos[Y] + up_len;
	}
	
	// set up the feeler, now check for collision
	if (feeler.GetCollision())
	{
		// Collision, possibly something we can snap up to
		
		// movement vector is in same direction as face normal, so we are moving away from it
		// usually this implies we are moving up, away from the horizontal face at the top of a wall, or a curb
		if (feeler.GetNormal().GetY() > 0.5f)		// not if too vertical
		{			
			// Okay, we can snap up, so all we have to do is see if the line is clear
			Mth::Vector	snap_point = feeler.GetPoint() + feeler.GetNormal();
			
			feeler.m_start = start_pos;
			feeler.m_start[Y] += up_len;
			feeler.m_end = snap_point;
			feeler.m_end[Y] += up_len;
			
			if (!feeler.GetCollision())
			{
				// No collision along upper line,so we consider it safe to move
				// note, this will generally result in passing through the corner of some geometry (generally like the top of a wall, or a curb)
				GetPos() = snap_point;	 		// set to snap point
				GetPos()[Y] += 0.1f;				// offset up a little, so we are outside the plane
				DUMP_POSITION;
				return true;								// return true, indicating we snapped up, so carry on in air
			}
			else
			{
				// was a collision when trying to move to new position, so don't move
			}
		}
		else
		{
			// too vert, so don't allow snap (probably on a QP)
		}

		return false;
	}
	else
	{
		// No collision, so don't do anything, probably deep within a wall, or off the level.
	}

	return false;	 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCorePhysicsComponent::handle_upward_collision_in_air (   )
{
	// return true if there was a collision, but we snapped up over it
	
	// if we are upside down, then return false
	if (GetMatrix()[Y][Y] < 0.0f) return false;
	
	float head_height = GetPhysicsFloat(CRCD(0x542cf0c7, "Skater_default_head_height"));
	
	// ignore head collisions for a duration after wallplants
    if (Tmr::ElapsedTime(m_last_wallplant_time_stamp) <= static_cast< Tmr::Time >(Script::GetInteger(CRCD(0x2757ed2c, "Physics_Ignore_Ceilings_After_Wallplant_Duration"))))
	{
		head_height = 6.0f;
	}
	
	Mth::Vector up_offset = GetMatrix()[Y] * head_height;
	
	m_col_start = GetPos();
	
	m_col_end = GetPos();
	m_col_end += up_offset;
	
	if (get_member_feeler_collision())
	{
		Mth::Vector ceiling_normal = m_feeler.GetNormal();
		
		// if it's not at least tilted a bit downwards, then ignore it and return false	   
		if (ceiling_normal[Y] > -0.1f) return false;
					  
		// get the vector we need to push down
		Mth::Vector push_down = m_feeler.GetPoint() - m_col_end;
		
		// only push down away from the ceiling	(otherwise we would push back along the line of travel, jerky)
		push_down.ProjectToNormal(ceiling_normal);

		// push down as far as we can go, so check for collisions		
		m_col_start = GetPos();
		m_col_end = GetPos() + push_down;
		if (get_member_feeler_collision())
		{
			GetPos() = m_feeler.GetPoint() + 0.001f * m_feeler.GetNormal();
			DUMP_POSITION;
		}
		else
		{
			GetPos() = m_col_end;
			DUMP_POSITION;
		}
		
		GetVel().ProjectToPlane(ceiling_normal);
		DUMP_VELOCITY;
		
		return true; 	// we have had a collision, so return true
	}
	
	return false;  // no collision found, return false
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCorePhysicsComponent::handle_upward_collision_in_wallride (   )
{
	m_col_start = GetPos() + 18.0f * mWallNormal;
	
	m_col_end = m_col_start;
	m_col_end[Y] += GetPhysicsFloat(CRCD(0x542cf0c7, "Skater_default_head_height"));
	
	if (get_member_feeler_collision())
	{
		Mth::Vector ceiling_normal = m_feeler.GetNormal();
		
		// if it's not at least tilted a bit downwards, then ignore it and return false	   
		if (ceiling_normal[Y] > -0.1f) return false;
					  
		// get the vector we need to push down
		Mth::Vector push_down = m_feeler.GetPoint() - m_col_end;
		
		// only push down away from the ceiling	(otherwise we would push back along the line of travel, jerky)
		push_down.ProjectToNormal(ceiling_normal);

		// push down as far as we can go, so check for collisions		
		m_col_start = GetPos();
		m_col_end = GetPos() + push_down;
		if (get_member_feeler_collision())
		{
			GetPos() = m_feeler.GetPoint() + m_feeler.GetNormal();
			DUMP_POSITION;
		}
		else
		{
			GetPos() = m_col_end;
			DUMP_POSITION;
		}
		
		GetVel().ProjectToPlane(ceiling_normal);
		DUMP_VELOCITY;
		
		return true; 	// we have had a collision, so return true
	}
	
	return false;  // no collision found, return false
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::do_wallride_physics (   )
{
	SetFlagFalse(SPINE_PHYSICS);
	SetFlagFalse(IN_ACID_DROP);
	SetFlagFalse(IN_RECOVERY);
	SetFlagFalse(AIR_ACID_DROP_DISALLOWED);

	// Wallriding will cancel any memory of what rail we were on, so the next one seems fresh	
	mp_rail_node = NULL;
	m_last_rail_trigger_node_name = 0;
	


	// Keep updating the time, cos it needs to be the time that the last wall ride finished.
	mWallrideTime = Tmr::GetTime();

	// set wallride movement to nothing
	Mth::Vector m_movement(0.0f, 0.0f, 0.0f);
	
	// if we are in contact with something, then factor in that "movement"
	if (mp_movable_contact_component->UpdateContact(GetPos()))
	{
		m_movement = mp_movable_contact_component->GetContact()->GetMovement();
		if (mp_movable_contact_component->GetContact()->IsRotated())
		{
			GetMatrix() *= mp_movable_contact_component->GetContact()->GetRotation();
			m_lerping_display_matrix *= mp_movable_contact_component->GetContact()->GetRotation();
		}
	}

	// Mick: changed to include gravity
	// note this is the way THPS2 worked
	// the physics might not be intuitive, but it works	
	// also note we are intentionally not removing sideways components of velocity, so we get to drift up a bit
	// after we hit the wall, allowing us to to the "claw" trick (Jump-WallRide-Jump-Grind)
	
	Mth::Vector acc(0.0f, GetPhysicsFloat(CRCD(0xc617caf, "Wall_Ride_Gravity")), 0.0f);
	GetPos() += GetVel() * m_frame_length + acc * (0.5f * m_frame_length * m_frame_length);
	GetPos() += m_movement;
	DUMP_POSITION;
	GetVel() += acc * m_frame_length;
	DUMP_VELOCITY;
			  
	#if 0 // old code
	// Mth::Vector Down(0.0f,-1.0f,0.0f);
	// Mth::Vector Horiz=Mth::CrossProduct(mWallNormal,Down);
	#else
	Mth::Vector Horiz(mWallNormal[Z], 0.0f, -mWallNormal[X], 0.0f);
	#endif
	Horiz.Normalize();
	
	// Down is a unit vector pointing down along the plane of the wall.
	Mth::Vector Down = Mth::CrossProduct(Horiz, mWallNormal);
	
	float Theta = GetPhysicsFloat(CRCD(0x64a48a64, "Wall_Ride_Turn_Speed"));
	
	// Check if Theta is bigger than the angle the skater's z is already making with the Down vector.
	// If Theta is bigger, don't rotate by it, otherwise the skater will turn beyond the vertical.
	float zdot = Mth::DotProduct(GetMatrix()[Z], Down);
	if (zdot > 0.68f || (zdot > 0.0f && zdot > cosf(Theta)))
	{
		// Pointing pretty much straight down, so don't turn.
	}
	else
	{
		// Choose which way to turn.
		// Need to turn one way or the other depending on whether the skater's z axis is on the left or right side of the down vector.
		// This is the same as whether the skater's x axis is pointing up or down, so just check the sign of the dot product of x with down.
		float xdot = Mth::DotProduct(GetMatrix()[X], Down);
		if (xdot <= 0.0f)
		{
			Theta = -Theta;
		}	

		// Not strictly required for NGPS, fixes a problem on NGC. However, given m_vel is used only
		// as a three-element vector, not a bad idea to set this for all platforms.
		GetVel()[W] = 1.0f;
	
		// Rotate both the skater and the skater's velocity by Theta about the wall normal axis.
		GetVel().Rotate(mWallNormal, Theta);
		DUMP_VELOCITY;

		// set skater to face in the same direction as velocity
		GetMatrix()[Z] = GetVel();
		
		GetMatrix()[Z].Normalize();
		GetMatrix()[Y] = mWallNormal;
		GetMatrix()[X] = Mth::CrossProduct(GetMatrix()[Y], GetMatrix()[Z]);
		GetMatrix()[X].Normalize();
		ResetLerpingMatrix();
	}	
	
	// Forward collision check	
	Mth::Vector forward = GetPos() - GetOldPos();
	forward.Normalize();	
	
	m_col_start = GetOldPos();
	
	m_col_end = GetPos();
	m_col_end += forward * GetPhysicsFloat(CRCD(0x20102726, "Skater_First_Forward_Collision_Length"));

	if (get_member_feeler_collision())
	{
		// we have hit something going forward
		// either it is a wall, and we need to bounce off it or it is a steep QP, and we need to stick to it.
		// (note, with the slow normal changing, then it is possible that we might even collide with the poly that we are on)
		
		Mth::Vector	normal = m_feeler.GetNormal();								
								
		float dot = Mth::DotProduct(normal, mWallNormal);			

		GetPos() = m_feeler.GetPoint() + normal;
		DUMP_POSITION;

		// For fairly shallow curves, the dot between two normals will be pretty large
		// it's very important here to distinguish between a tight curve (like in a narrow QP) and a direct hit with a wall.
		
		if (!m_col_flag_wallable || Mth::Abs(dot) < 0.01f)
		{
			if (normal[Y] > 0.9f)
			{
				// If the new poly is the ground, pop upright and set the landed exception.
				GetPos() = m_feeler.GetPoint();
				DUMP_POSITION;

				// Set every orientation type thing I can think of so as to
				// pop his orientation to that of the ground.				
				GetMatrix()[Z] = GetVel();
				GetMatrix()[Z].ProjectToPlane(normal);	 // Mick: project forward vel to ground
				GetMatrix()[Z].Normalize();
				GetMatrix()[Y] = normal;				  		
				GetMatrix()[X] = Mth::CrossProduct(GetMatrix()[Y],GetMatrix()[Z]);
				GetMatrix()[X].Normalize();
				
				ResetLerpingMatrix();
				
				GetSkaterMatrixQueriesComponentFromObject(GetObject())->ResetLatestMatrix();
				
				m_last_display_normal = GetMatrix()[Y];
				m_display_normal = GetMatrix()[Y];
				m_current_normal = GetMatrix()[Y];
				
				// check for sticking to rail first; use the "override_air" to stick to rail)
				maybe_stick_to_rail(true);
				if (GetState() != RAIL)
				{
					SetState(GROUND);
					FLAGEXCEPTION(CRCD(0x532b16ef, "Landed"));
				}				
				return;
			}
			else
			{
				new_normal(Mth::Vector(0.0f, 1.0f, 0.0f));
				
				GetPos() += 18.0f * mWallNormal;
				DUMP_POSITION;
			
				SetState(AIR);
				GetObject()->BroadcastEvent(CRCD(0xd96f01f1, "SkaterOffEdge"));
				FLAGEXCEPTION(CRCD(0x3b1001b6, "GroundGone"));


			}	
		}
		else
		{
			mWallNormal = normal;			// update the normal to the new one...			
			new_normal(mWallNormal);
            ResetLerpingMatrix();
			GetVel().RotateToPlane(mWallNormal);		// make velocity go along it
			DUMP_VELOCITY;
		}
	}
	
	if (handle_upward_collision_in_wallride())
	{
		new_normal(Mth::Vector(0.0f, 1.0f, 0.0f));
		
		GetPos() += 18.0f * mWallNormal;
		DUMP_POSITION;

		SetState(AIR);
		GetObject()->BroadcastEvent(CRCD(0xd96f01f1, "SkaterOffEdge"));
		FLAGEXCEPTION(CRCD(0x3b1001b6, "GroundGone"));
	}

	// Downwards collision check (down in direction of skater's y)
	
	m_col_start = GetMatrix()[Y] * GetPhysicsFloat(CRCD(0xe4d79235, "Physics_Ground_Snap_Up"));
	m_col_start += GetPos();
	
	m_col_end = GetMatrix()[Y] * GetPhysicsFloat(CRCD(0x438ba168, "Wall_Ride_Down_Collision_Check_Length"));
	m_col_end += GetPos();
	
	if (get_member_feeler_collision())
	{
		mWallNormal=m_feeler.GetNormal();

 		// Snap position to wall.
		GetPos() = m_feeler.GetPoint();
		// but one inch out from it
		GetPos() += mWallNormal;
		DUMP_POSITION;
			
		// Rotate velocity to plane.
		GetVel().RotateToPlane(mWallNormal);
		DUMP_VELOCITY;
			
		// Snap skater's orientation to the plane.
		new_normal(mWallNormal);
		ResetLerpingMatrix();
		
		if (!m_col_flag_wallable)
		{
			GetVel() += mWallNormal * 100.0f;		// boost away from the wall
			DUMP_VELOCITY;
			
			new_normal(Mth::Vector(0.0f, 1.0f, 0.0f));
            ResetLerpingMatrix();
			SetState(AIR);
			
			GetObject()->BroadcastEvent(CRCD(0xd96f01f1, "SkaterOffEdge"));
			FLAGEXCEPTION(CRCD(0x3b1001b6, "GroundGone"));
			mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_OFF_EDGE, m_last_ground_feeler);
			
			if (mp_physics_control_component->HaveBeenReset()) return;
		}
		else
		{
			// handle wallride transition from one object to the next
			if (m_feeler.GetSector() != m_last_ground_feeler.GetSector())
			{
				mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_OFF_EDGE, m_last_ground_feeler);
				if (mp_physics_control_component->HaveBeenReset()) return;
				mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_ONTO, m_last_ground_feeler);
				if (mp_physics_control_component->HaveBeenReset()) return;
			}
			if (!mp_trick_component->GraffitiTrickStarted())
			{
				// clear the graffiti trick buffer if we're moving to a new world sector, but haven't started our trick yet...
				mp_trick_component->SetGraffitiTrickStarted(false);
			}

			set_last_ground_feeler(m_feeler);
		}
	}
	else
	{
		new_normal(Mth::Vector(0.0f, 1.0f, 0.0f));
		ResetLerpingMatrix();
		GetPos() += 18.0f * mWallNormal;
		DUMP_POSITION;
		SetState(AIR);
		GetObject()->BroadcastEvent(CRCD(0xd96f01f1, "SkaterOffEdge"));
		FLAGEXCEPTION(CRCD(0x3b1001b6, "GroundGone"));
		mp_trigger_component->CheckFeelerForTrigger(TRIGGER_SKATE_OFF_EDGE, m_last_ground_feeler);
		if (mp_physics_control_component->HaveBeenReset()) return;
	}
	
	// look for head collisions
	
	
	// This was cut-and-past'd from DoOnGroundPhysics
	handle_tensing();
	
	if (maybe_flag_ollie_exception())
	{
		mp_trigger_component->CheckFeelerForTrigger(TRIGGER_JUMP_OFF, m_last_ground_feeler);
		if (mp_physics_control_component->HaveBeenReset()) return;
		
		GetVel() += GetPhysicsFloat(CRCD(0x349d02d4, "Wall_Ride_Jump_Out_Speed")) * mWallNormal;
		GetVel()[Y] += GetPhysicsFloat(CRCD(0x5a9de35a, "Wall_Ride_Jump_Up_Speed"));
		DUMP_VELOCITY;
		
		// Pop the skater upright again immediately.
		GetMatrix()[Y].Set(0.0f, 1.0f, 0.0);
		GetMatrix().OrthoNormalizeAbout(Y);
		ResetLerpingMatrix();

		// and immediately adjust the normal, to prevent single frame snaps when walliing and the 90 degree swoop when wallride to grind
		m_normal_lerp = 0.0f;
		m_display_normal = GetMatrix()[Y];
		m_current_normal  = m_display_normal;
		m_last_display_normal  = m_display_normal;
	}
	
	// if we've already started doing a trick, then start remembering our trick chain
	// GJ:  It's possible to do a wallride while mNumTricksInCombo == 0, for some reason
	mp_trick_component->TrickOffObject(m_last_ground_feeler.GetNodeChecksum());
			
	#ifdef __NOPT_ASSERT__
	if (Script::GetInteger(CRCD(0x3ae85eef, "skater_trails")))
	{
		Gfx::AddDebugLine(GetPos() + m_current_normal, GetOldPos() + m_current_normal, PURPLE, 0, 0);
	}
	#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::do_wallplant_physics (   )
{
	// check if the wallplant duration is up
	if (Tmr::ElapsedTime(m_state_change_timestamp) > Script::GetFloat(CRCD(0xa06e446b, "Physics_Wallplant_Duration")))
	{
		SetState(AIR);
		return;
	}

	// Wallplanting will cancel any memory of what rail we were on, so the next one seems fresh	
	mp_rail_node = NULL;
	m_last_rail_trigger_node_name = 0;

	
	// during a wallplant, our velocity is set to the exit velocity, but we simply ignore it
	
	// the only movement comes from our moving contact; we assume that this movement does not lead to collisions
	if (mp_movable_contact_component->UpdateContact(GetPos()))
	{
		GetPos() += mp_movable_contact_component->GetContact()->GetMovement();
		DUMP_POSITION;
		if (mp_movable_contact_component->GetContact()->IsRotated())
		{
			GetMatrix() *= mp_movable_contact_component->GetContact()->GetRotation();
			m_lerping_display_matrix *= mp_movable_contact_component->GetContact()->GetRotation();
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::maybe_stick_to_rail ( bool override_air )
{
	// Only alow them to grind if we are in the air, that way we will avoid nasty problem with snapping through geometry onto rails 
	if (GetState() != AIR && !override_air) return;

	// K: Don't allow a grind trick if bailing, otherwise you can late-trick into a liptrick, bail, get snapped into a grind, & hence bail again.
	if (GetFlag(IS_BAILING)) return;
		
	// K: No grinds or lip tricks are allowed if this flag is set. This is switched on and off by the NoGrind and AllowGrind script commands.
	// These are used when jumping out of a lip trick to disallow going straight into a grind.
	if (mNoRailTricks) return;

	if (!mp_input_component->GetControlPad().m_triangle.GetPressed())
	{
		// Mick: not sure why this was removed
		// SetFlagTrue(CAN_RERAIL);
		return;
	}
	
	// don't grind for a short duration after a wallplant
	if (Tmr::ElapsedTime(m_last_wallplant_time_stamp) < static_cast< Tmr::Time >(Script::GetInteger(CRCD(0x96dca7dc,"Physics_Wallplant_Disallow_Grind_Duration")))) return;

	Mth::Vector a = GetOldPos();
	Mth::Vector b = GetPos();
	
	// if we were on a rail recently, and in the park editor, then don't let us snap to rails that are very perpendicular to us for a while
	float min_dot;
	if (Ed::CParkEditor::Instance()->UsingCustomPark() && Tmr::ElapsedTime(m_rail_time) < m_rerail_time)
	{
		// Should only do this if we've recently been on a rail
		min_dot = cosf(Mth::DegToRad(GetPhysicsFloat(CRCD(0x76c1da15, "Rail_Corner_Leave_Angle"))));
	}
	else
	{
		min_dot = 1.0f;
	}

	// eventually we should get the rail manager base on whatever rail we are on
	CRailManager* p_rail_man = Mdl::Skate::Instance()->GetRailManager();
		
	// first check the world rail manager (rails that do not move)
	CRailNode* pNode;		
	Mth::Vector rail_pos;
	if (p_rail_man->StickToRail(a, b, &rail_pos, &pNode, NULL, min_dot))
	{
		TrackingLine(2,GetOldPos(), GetPos());	  // 2 = stick to rail
		
		mp_movable_contact_component->LoseAnyContact();
		if (will_take_rail(pNode, p_rail_man))
		{
			got_rail(rail_pos, pNode, p_rail_man);
		}
		return;
	}
	
	// iterate through all rail manager components, starting with the first one
	for (CRailManagerComponent* p_railmanager_component = static_cast< CRailManagerComponent* >(CCompositeObjectManager::Instance()->GetFirstComponentByType(CRC_RAILMANAGER));
		p_railmanager_component;
		p_railmanager_component = static_cast< CRailManagerComponent* >(p_railmanager_component->GetNextSameType()))
	{
		CRailManager* p_rail_man = p_railmanager_component->GetRailManager();
		
		Mth::Matrix	total_mat = p_railmanager_component->UpdateRailManager();
		Mth::Matrix	inv = total_mat;
		inv.Invert();

		a[W] = 1.0f;
		b[W] = 1.0f;

		// Transform a,b into the space of the object			
		Mth::Vector local_a = inv.Transform(a);
		Mth::Vector local_b = inv.Transform(b);

		if (p_rail_man->StickToRail(local_a, local_b, &rail_pos, &pNode, NULL, min_dot))
		{
			// transform from object space to world space
			rail_pos[W] = 1.0f;
			rail_pos = total_mat.Transform(rail_pos);
			if (will_take_rail(pNode, p_rail_man))
			{
				got_rail(rail_pos, pNode, p_rail_man); 
				mp_movable_contact_component->ObtainContact(p_railmanager_component->GetObject());
				
				GetVel() -= mp_movable_contact_component->GetContact()->GetObject()->GetVel();
			}
			return;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCorePhysicsComponent::will_take_rail ( const CRailNode* pNode, CRailManager* p_rail_man, bool from_walk )
{
	// no grinding if we're in an acid drop which was generated from an ollie out of a grind; prevents acid-grind-acid-grind repetition
	if (GetState() == AIR && GetFlag(IN_ACID_DROP) && GetFlag(OLLIED_FROM_RAIL)
		&& (mp_rail_node == pNode || mp_rail_node == pNode->GetNextLink() || mp_rail_node == pNode->GetPrevLink()))
	{
		return false;
	}
	
	// if it's a different rail, then allow rerailing
	if (!mp_rail_node																			// if there was no last rail
		
		// Dan: removed this line to prevent occasional rerailing when you grind off the end of a car; why was it here in the first place?
		// || (mp_rail_man && mp_rail_man->IsMoving() && !mp_movable_contact_component->HaveContact())	// or last rail was movable, and we've lost contact
		
		|| mp_rail_node != pNode																// or not same segment
		&& mp_rail_node != pNode->GetNextLink()  												// or the one before
		&& mp_rail_node != pNode->GetPrevLink()) 												// of the one after
	{
		if (mp_rail_node && Ed::CParkEditor::Instance()->UsingCustomPark())
		{
			Dbg_Assert(mp_rail_man);
			
			// in park editor rail must also not share end points to be considered different
			Mth::Vector a = mp_rail_man->GetPos(mp_rail_node) - mp_rail_man->GetPos(pNode);
			Mth::Vector b = mp_rail_man->GetPos(mp_rail_node) - mp_rail_man->GetPos(pNode->GetNextLink());
			Mth::Vector c = mp_rail_man->GetPos(mp_rail_node->GetNextLink()) - mp_rail_man->GetPos(pNode);
			Mth::Vector d = mp_rail_man->GetPos(mp_rail_node->GetNextLink()) - mp_rail_man->GetPos(pNode->GetNextLink());

			if (Mth::Abs(a.Length()) > 6.0f			
				&& Mth::Abs(b.Length()) > 6.0f			
				&& Mth::Abs(c.Length()) > 6.0f			
				&& Mth::Abs(d.Length()) > 6.0f)
			{
				SetFlagTrue(CAN_RERAIL);
			}
		}
		else
		{
			SetFlagTrue(CAN_RERAIL);
		}
	}
	
    return (GetFlag(CAN_RERAIL) || Tmr::ElapsedTime(m_rail_time) > m_rerail_time)
		&& (from_walk
			|| (GetState() != RAIL 									// not already on a rail
			&& (!GetFlag(TRACKING_VERT) || GetVel()[Y] > 0.0f)));		// must be not vert, or going up 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::got_rail ( const Mth::Vector& rail_pos, const CRailNode* pNode, CRailManager* p_rail_man, bool no_lip_tricks, bool from_walk )
{
	// got a point on rail, with start node number from a particular rail manager
	// use this info to start grinding or lipping on the rail
	
	CControlPad& control_pad = mp_input_component->GetControlPad();
	
	CSkaterFlipAndRotateComponent* p_flip_and_rotate_component = GetSkaterFlipAndRotateComponentFromObject(GetObject());
	Dbg_Assert(p_flip_and_rotate_component);
	p_flip_and_rotate_component->DoAnyFlipRotateOrBoardRotateAfters();


	// Mick - landed on a rail
	// if it's a "new" rail, then tell the robot detector about it
	if (mp_rail_node != pNode)
	{
		int rail_index = p_rail_man->GetNodeIndex(pNode);		
		mp_score_component->GetScore()->UpdateRobotDetection(rail_index);
	}

	mp_rail_node = pNode;					  
	mp_rail_man = p_rail_man;
	
	// handle single node rails
	if (!pNode->GetPrevLink() && !pNode->GetNextLink())
	{
		// for a single node rail, we apply in a single frame all the effects of entering and exiting the rail state;

		/////////////////////////////////////////////////////
		// Emulate entering the rail state (with horizontal dir direction)

		// check for collision in moving from m_pos to rail_pos
		
		m_col_start = GetPos();
		m_col_end = rail_pos;
		if (get_member_feeler_collision())
		{
			// check distance from the rail to the collision point
			if ((rail_pos - m_feeler.GetPoint()).LengthSqr() > 6.0f * 6.0f) 
			{
				return;
			}
		}
		
		// check first if we are not moving much in the XY and if not, then se the XZ velocity to the matrix[Z], so we always go forward
		if (Mth::Abs(GetVel()[X]) < 0.01f && Mth::Abs(GetVel()[Z]) < 0.01f)
		{
			GetVel()[X] = GetMatrix()[Z][X];
			GetVel()[Z] = GetMatrix()[Z][Z];
			DUMP_VELOCITY;
		}
		
		if (!from_walk && GetVel()[X] * GetVel()[X] + GetVel()[Z] * GetVel()[Z] > 10.0f * 10.0f)
		{
			GetVel().RotateToPlane(Mth::Vector(0.0f, 1.0f, 0.0f));
			DUMP_VELOCITY;
		}

		// rail direction is taken to always simply be along our horizontal velocity, rotated up
		Mth::Vector dir = GetVel();
		dir[Y] = 0.0f;
		dir.Normalize();
		float angle = Mth::DegToRad(Script::GetFloat(CRCD(0xbb357ecb,"Physics_Point_Rail_Kick_Upward_Angle")));
		float c = cosf(angle);
		float s = sinf(angle);
		Mth::Vector boost_dir(c * dir[X], s, c * dir[Z]);
		
		// get the rail node name
		Script::CArray* pNodeArray = mp_rail_man->GetNodeArray();
		Script::CStruct* pNode = pNodeArray->GetStructure(mp_rail_node->GetNode());
		pNode->GetChecksum(CRCD(0xa1dc81f9, "Name"), &m_last_rail_node_name, Script::ASSERT);
		
		mp_trick_component->TrickOffObject(m_last_rail_node_name);

		// Now we want to see if the rail has a trigger, and if it does, trigger it....
		
		uint32 trigger_script = 0;
		
		// no need to call maybe_trip_rail_trigger for a single node rail
		if (pNode->GetChecksum(CRCD(0x2ca8a299, "TriggerScript"), &trigger_script))
		{
			mp_trigger_component->TripTrigger(
				TRIGGER_LAND_ON,
				m_last_rail_node_name,
				pNodeArray == Script::GetArray(CRCD(0xc472ecc5, "NodeArray")) ? NULL : pNodeArray,
				mp_movable_contact_component->HaveContact() ? mp_movable_contact_component->GetContact()->GetObject() : NULL
			);
		}

		GetPos() = rail_pos;
		DUMP_POSITION;

		// Now we'v got onto the rail, we need to:
		// 1) kill velocity perpendicular to the rail
		// 2) add a speed boost in the direction we are going.

		SetFlagFalse(VERT_AIR);
		SetFlagFalse(TRACKING_VERT);

		// if we are transitioning from wall to rail, then snap him upright		
		if (GetState() == WALL)
		{
			new_normal(Mth::Vector(0.0f, 1.0f, 0.0f));
			ResetLerpingMatrix();
		}

		SetState(RAIL);

		set_terrain(mp_rail_node->GetTerrain());
		mp_sound_component->PlayLandSound(GetObject()->GetVel().Length() / GetSkater()->GetScriptedStat(CRCD(0xcc5f87aa, "Skater_Max_Max_Speed_Stat")), mp_state_component->m_terrain);
		
		float old_y = GetVel()[Y];
		GetVel().ProjectToNormal(dir);	   							// kill perp velocity
		if (from_walk && Mth::DotProduct(GetVel(), GetMatrix()[Z]) < 0.0f)
		{
			if (GetVel().LengthSqr() < Mth::Sqr(1.1f * mp_walk_component->s_get_param(CRCD(0x896c8888, "jump_adjust_speed"))))
			{
				GetVel().Set();
				dir = GetMatrix()[Z];
				dir[Y] = 0.0f;
				dir.Normalize();
			}

		}
		GetVel()[Y] = old_y;											// except for Y

		GetVel() += dir * GetPhysicsFloat(CRCD(0xa3ef4833, "Point_Rail_Speed_Boost"));	// add speed boost			
		DUMP_VELOCITY;


		// (Mick) Set m_rail_time, otherwise there is a single frame where it is invalid
		// and this allows us to immediately re-rail and hence do the "insta-bail", since the triangle button will be held down   
		m_rail_time  = Tmr::GetTime();

		/////////////////////////////////////////////////////
		// Emulate effects of rail state (with boost_dir direction)

		SetFlagFalse(SPINE_PHYSICS);
		SetFlagFalse(IN_ACID_DROP);
		SetFlagFalse(IN_RECOVERY);
		SetFlagFalse(AIR_ACID_DROP_DISALLOWED);

		// set default rerail time	
		m_rerail_time = static_cast< Tmr::Time >(GetPhysicsFloat(CRCD(0x2b4645ad, "Rail_Minimum_Rerail_Time")));
		// and dissalow any overriding of this
		SetFlagFalse(CAN_RERAIL);		// dont allow rerailing after coming off a segment
		
		GetVel().RotateToNormal(boost_dir);
		DUMP_VELOCITY;

		/////////////////////////////////////////////////////
		// Emulate exiting the rail state

		// no need to call maybe_trip_rail_trigger for a single node rail
		if (trigger_script)
		{
			mp_trigger_component->TripTrigger(
				TRIGGER_SKATE_OFF,
				m_last_rail_node_name,
				pNodeArray == Script::GetArray(CRCD(0xc472ecc5, "NodeArray")) ? NULL : pNodeArray,
				mp_movable_contact_component->HaveContact() ? mp_movable_contact_component->GetContact()->GetObject() : NULL
			);
		}
			
		SetState(AIR);
		GetPos()[Y] += 1.0f;
		DUMP_POSITION;

		/////////////////////////////////////////////////////
		// Do extra point rail logic

		// trigger the appropriate script
		GetObject()->SelfEvent(CRCD(0xb8048f1d, "PointRail"));
		
		return;
	}
	
	///////////////////////////////////////////////////////////////////////////
	// 						Check for lip trick
	///////////////////////////////////////////////////////////////////////////
	
	float LipAllowSine = sinf(Mth::DegToRad(GetPhysicsFloat(CRCD(0x8157c5d9, "LipAllowAngle"))));
	if (pNode->GetFlag(LIP_OVERRIDE))
	{
		LipAllowSine = sinf(Mth::DegToRad(GetPhysicsFloat(CRCD(0xf3b6f95e, "LipAllowAngle_Override"))));
	}
	float HorizSine = sinf(Mth::DegToRad(GetPhysicsFloat(CRCD(0xe2a85fbb, "LipPlayerHorizontalAngle"))));
	float VertCos = cosf(Mth::DegToRad(GetPhysicsFloat(CRCD(0x61079462, "LipRampVertAngle"))));

	if ((mAllowLipNoGrind											// This flag makes lip tricks always happen, hooray!
		|| (
			GetVel()[Y] > 0.0f											// going upwards
			&& Mth::Abs(GetMatrix()[Y][Y]) < HorizSine		   			// fairly horizontal skater
			&& GetMatrix()[Z][Y] > 0.0f									// Facing fairly up in the air
			&& Mth::Abs(m_current_normal[Y]) < VertCos					// last poly we were on was near vert
			&& Mth::Abs(GetMatrix()[X][Y]) < LipAllowSine		 			// skater pointing fairly straight up
		)) && !no_lip_tricks)
	{
		// Reset a bunch of stuff.
		GetVel().Set(0.0f, 0.0f, 0.0f);
		DUMP_VELOCITY;
		SetFlagFalse(VERT_AIR);
		SetFlagFalse(TRACKING_VERT);
		SetFlagFalse(LAST_POLY_WAS_VERT);
		SetFlagFalse(CAN_BREAK_VERT);
		// SetFlagFalse(LEAN);
		
		// Make sure any other balance trick is stopped.
		mp_balance_trick_component->stop_balance_trick();
		
		m_pre_lip_pos = GetPos();
		
		// Into the lip state
		SetState(LIP);
		
		// Snap the position
		GetPos() = rail_pos;
		DUMP_POSITION;
	
		// Pop the skater horizontal
		Mth::Vector u = m_current_normal;
		u[Y] = 0.0f;
		u.Normalize();
		GetMatrix()[Y] = u;		
		
		GetMatrix()[Z].Set(0.0f, 1.0f, 0.0f);
		
		#if 0 // old code
		// GetMatrix()[X] = Mth::CrossProduct(m_matrix[Y],m_matrix[Z]);
		// GetMatrix()[X].Normalize();
		#else
		GetMatrix()[X].Set(
			-GetMatrix()[Y][Z],
			0.0f,
			GetMatrix()[Y][X],
			0.0f
		);
		#endif

		ResetLerpingMatrix();
		
		// Update the "Require_Lip" flag so lip only gaps don't need to wait on frame
		GetSkaterGapComponentFromObject(GetObject())->UpdateCancelRequire(0,REQUIRE_LIP);
		
		
		// Trigger the lip on event.
		maybe_trip_rail_trigger(TRIGGER_LIP_ON);

		mp_trick_component->mUseSpecialTrickText = false;
		
		if (!GetObject()->GetScript())
		{
			GetObject()->SetScript(new Script::CScript);
		}

		// Run the lip script.
		GetObject()->GetScript()->SetScript(CRCD(0x1647cf96, "LipTrick"), NULL, GetObject());
		GetObject()->GetScript()->Update();
	
		// Set the mDoingTrick flag so that the camera can detect that a trick is being done.
		mp_state_component->SetDoingTrick(true);

		set_terrain(mp_rail_node->GetTerrain());
	
		return;
	}
	
	// no lip trick, so we rail
	
	set_terrain(mp_rail_node->GetTerrain());
			  
	const CRailNode* pStart = mp_rail_node;
	const CRailNode* pEnd = pStart->GetNextLink();

	Mth::Vector	dir = mp_rail_man->GetPos(pEnd) - mp_rail_man->GetPos(pStart);
	dir.Normalize();
	
	// Now, we've get a rail
	
	// check for collision in moving from m_pos to rail_pos
	
	m_col_start = GetPos();
	m_col_end = rail_pos;
	if (get_member_feeler_collision())
	{
		// check distance from the rail to the collision point
		if ((rail_pos - m_feeler.GetPoint()).LengthSqr() > 6.0f * 6.0f) 
		{
			return;
		}
	}
	
	if (GetFlag(IN_ACID_DROP))
	{
		MESSAGE("GRINDING FROM ACID DROP");
		DUMPV(acid_hold);
		DUMP_VELOCITY;
		GetVel() += acid_hold;
		DUMP_VELOCITY;
	}

	// we need to check if this is the end of the rail and we are going to come off it next frame, then we don't want to get on it....

	// check first if we are not moving much in the XY and if not, then se the XZ velocity to the matrix[Z], so we always go forward
	if (GetVel()[X] == 0.0f && GetVel()[Z] == 0.0f)
	{
		GetVel()[X] = GetMatrix()[Z][X];
		GetVel()[Z] = GetMatrix()[Z][Z];
	}
	
	//////////////////////////////////////////////////////////////////			
	// Mick: Start of patch
	// (For Oil Rig type problem with steep rails)
	// if we are moving forward, then rotate velocity onto a horizontal plane
	// so we don't seem to be going backwards up steep rails when we jump onto them
	
	#if 0 // old code
	// Mth::Vector flat_vel = GetVel();
	// flat_vel[Y] = 0;
	// if (flat_vel.Length() > 10.0f)
	// {
		// GetVel().RotateToPlane(Mth::Vector(0.0f,1.0f,0.0f));
	//}
	#else
	if (!from_walk && GetVel()[X] * GetVel()[X] + GetVel()[Z] * GetVel()[Z] > 10.0f * 10.0f)
	{
		GetVel().RotateToPlane(Mth::Vector(0.0f, 1.0f, 0.0f));
		DUMP_VELOCITY;
	}
	#endif
	
	//
	// Mick: end of patch
	/////////////////////////////////////////////////////////////////////

	float dot = Mth::DotProduct(dir, GetVel());
	
	// sign is which way we are going along the rail
	float sign;
	if (dot == 0.0f)
	{
		// if the dot product can not determine the direction (pull up from hangs), choose randomly
		sign = Mth::Rnd(2) ? 1.0f : -1.0f;
	}
	else
	{
	   sign = Mth::Sgn(dot);
	}
	
	if (sign < 0.0f)
	{
		// will be going backwards along the rail
		
		// if the rail stick point is this last point, then we don't want to snap to it as we will immediatly fly off,
		// and the point might be coincident with a wall or the ground and that will cause problems
		if (!pStart->GetPrevLink() && mp_rail_man->GetPos(pStart) == rail_pos) return;
	}
	else
	{
		// same for going forward along the rail
		if (!pEnd->GetNextLink() && mp_rail_man->GetPos(pEnd) == rail_pos) return;
	}
	
	// Now we want to see if the rail has a trigger, and if it does, trigger it....
	maybe_trip_rail_trigger(TRIGGER_LAND_ON);
	
	GetPos() = rail_pos;
	DUMP_POSITION;
	
	// Now we'v got onto the rail, we need to:
	// 1) kill velocity perpendicular to the rail
	// 2) add a speed boost in the direction we are going.
	
	SetFlagFalse(VERT_AIR);
	SetFlagFalse(TRACKING_VERT);

	// if we are transitioning from wall to rail, then snap him upright		
	if (GetState() == WALL)
	{
		new_normal(Mth::Vector(0.0f, 1.0f, 0.0f));
		ResetLerpingMatrix();
	}
	
	SetState(RAIL);
	
	// don't play the rail sound when coming onto a rail from walking
	if (Tmr::ElapsedTime(mp_physics_control_component->GetStateSwitchTime()) != 0)
	{
		// play sound based on pre-rail velocity
		mp_sound_component->PlayLandSound(GetObject()->GetVel().Length() / GetSkater()->GetScriptedStat(CRCD(0xcc5f87aa, "Skater_Max_Max_Speed_Stat")), mp_state_component->m_terrain);
	}

	float old_y = GetVel()[Y];
	GetVel().ProjectToNormal(dir);	   							// kill perp velocity
	if (from_walk && Mth::DotProduct(GetVel(), GetMatrix()[Z]) < 0.0f)
	{
		if (GetVel().LengthSqr() < Mth::Sqr(1.1f * mp_walk_component->s_get_param(CRCD(0x896c8888, "jump_adjust_speed"))))
		{
			GetVel().Set();
			sign = Mth::Sgn(Mth::DotProduct(dir, GetMatrix()[Z]));
		}
		
	}
	GetVel()[Y] = old_y;											// except for Y
	
	GetVel() += dir * sign * GetPhysicsFloat(CRCD(0x457bb395, "Rail_Speed_Boost"));	// add speed boost			
	DUMP_VELOCITY;
	
	// Ken stuff ...
	bool Rail_RightOfRail = false;
	bool Rail_Parallel = false;
	mRail_Backwards = false;

	if (sign < 0.0f)
	{
		dir = -dir;
	}
	// dir is now the unit vector pointing along the rail in the direction we will be moving on the rail
		
	// Calculate which side of the rail we're on
	#if 0 // old code
	// Mth::Vector d = rail_pos - m_last_ground_pos;
	// Mth::Vector side_vel(d.GetZ(), 0.0f, -d.GetX());
	// if (Mth::DotProduct(dir, side_vel) < 0.0f)
	// {
		// Rail_RightOfRail = true;
	// }	
	#else
	float side_vel_X = rail_pos[Z] - m_last_ground_pos[Z];
	float side_vel_Z = -(rail_pos[X] - m_last_ground_pos[X]);
	if (dir[X] * side_vel_X + dir[Z] * side_vel_Z < 0.0f)
	{
		Rail_RightOfRail = true;
	}
	#endif
	
	///////////////////////////////////////////////////////////////////////////////////
	// 							   Bad ledge detection
	///////////////////////////////////////////////////////////////////////////////////
	
	// Calculate the up and down offsets for collision test.
	m_col_start.Set(0.0f, GetPhysicsFloat(CRCD(0xe4d79235, "Physics_Ground_Snap_Up")), 0.0f, 0.0f);
	m_col_end.Set(0.0f, -GetPhysicsFloat(CRCD(0x9cd7ed5c, "Rail_Bad_Ledge_Drop_Down_Dist")), 0.0f, 0.0f);
	
	// Add the rail pos, so start and end are now above and below the rail.
	m_col_start += rail_pos;
	m_col_end += rail_pos;

	// Calculate a side offset, using the rail direction rotated 90 degrees.
	Mth::Vector Off(dir[Z], 0.0f, -dir[X], 0.0f);
	Off *= GetPhysicsFloat(CRCD(0x31669752, "Rail_Bad_Ledge_Side_Dist"));

	// Add the offset and do the left collision.
	m_col_start += Off;
	m_col_end += Off;
	bool LeftCollision = get_member_feeler_collision();
	
	// Move across to the other side and do the right collision.
	m_col_start -= 2.0f * Off;
	m_col_end -= 2.0f * Off;
	bool RightCollision = get_member_feeler_collision();
	
	// Bit of logic to get whether it's a bad ledge or not.
	mBadLedge = (LeftCollision && !Rail_RightOfRail) || (RightCollision && Rail_RightOfRail);
	mLedge = LeftCollision || RightCollision;
	
	///////////////////////////////////////////////////////////////////////////////////
	
	float RightDot = Mth::DotProduct(dir, GetMatrix()[X]);
	float FrontDot = Mth::DotProduct(dir, GetMatrix()[Z]);
	
	if (Mth::Abs(RightDot) < GetPhysicsFloat(CRCD(0xd0688d4e, "Rail_Tolerance")))
	{
		// The skater's right vector is close to perpendicular to the rail, so the skater must be pointing fairly parallel to it.
		Rail_Parallel = true;
		
		// Use the front vector to determine forwards/backwards, since the front vector is fairly parallel to the rail.
		if (FrontDot < 0.0f)
		{
			mRail_Backwards = true;
		}
	}
	else
	{
		if (GetFlag(FLIPPED))
		{
			RightDot = -RightDot;
		}
			
		// The skater's right vector is fairly parallel to the rail, so use it to determine forwards/backwards.
		if (RightDot < 0.0f)
		{
			mRail_Backwards = true;
		}
	}

	mp_trick_component->mUseSpecialTrickText = false;

	if (!mp_trick_component->TriggerAnyExtraGrindTrick(
			Rail_RightOfRail,
			Rail_Parallel,
			mRail_Backwards,
			GetFlag(FLIPPED))
		)
	{
		do_grind_trick(CSkaterPad::sGetDirection(
			control_pad.m_up.GetPressed(),
			control_pad.m_down.GetPressed(),
			control_pad.m_left.GetPressed(),
			control_pad.m_right.GetPressed()
		), Rail_RightOfRail, Rail_Parallel, mRail_Backwards, GetFlag(FLIPPED));
	}	
	
	// (Mick) Set m_rail_time, otherwise there is a single frame where it is invalid
	// and this allows us to immediately re-rail and hence do the "insta-bail", since the triangle button will be held down   
	m_rail_time  = Tmr::GetTime();
}		

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::do_lip_physics (   )
{
	SetFlagFalse(SPINE_PHYSICS);
	SetFlagFalse(IN_ACID_DROP);
	SetFlagFalse(IN_RECOVERY);
	
	if (mp_movable_contact_component->UpdateContact(GetPos()))
	{
		GetPos() += mp_movable_contact_component->GetContact()->GetMovement();
		DUMP_POSITION;
		
		// this is close to correct, 'cept that but because of rotation, it's not quite right
		m_pre_lip_pos += mp_movable_contact_component->GetContact()->GetMovement();
		
		if (mp_movable_contact_component->GetContact()->IsRotated())
		{

			GetMatrix() *= mp_movable_contact_component->GetContact()->GetRotation();
			m_lerping_display_matrix *= mp_movable_contact_component->GetContact()->GetRotation();
		}
	}
	
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0x1a5eab7, "rail_highlights")))
	{
		Gfx::AddDebugLine(mp_rail_man->GetPos(mp_rail_node), mp_rail_man->GetPos(mp_rail_node->GetNextLink()), MAKE_RGB(Mth::Rnd(256), Mth::Rnd(256), Mth::Rnd(256)), 0, 1);
		Gfx::AddDebugLine(mp_rail_man->GetPos(mp_rail_node) + Mth::Vector(1.0f, 0.0f, 0.0f), mp_rail_man->GetPos(mp_rail_node->GetNextLink()) + Mth::Vector(1.0f, 0.0f, 0.0f), MAKE_RGB(Mth::Rnd(256), Mth::Rnd(256), Mth::Rnd(256)), 0, 1);
		Gfx::AddDebugLine(mp_rail_man->GetPos(mp_rail_node) + Mth::Vector(0.0f, 1.0f, 0.0f), mp_rail_man->GetPos(mp_rail_node->GetNextLink()) + Mth::Vector(0.0f, 1.0f, 0.0f), MAKE_RGB(Mth::Rnd(256), Mth::Rnd(256), Mth::Rnd(256)), 0, 1);
		Gfx::AddDebugLine(mp_rail_man->GetPos(mp_rail_node) + Mth::Vector(0.0f, 0.0f, 1.0f), mp_rail_man->GetPos(mp_rail_node->GetNextLink()) + Mth::Vector(0.0f, 0.0f, 1.0f), MAKE_RGB(Mth::Rnd(256), Mth::Rnd(256), Mth::Rnd(256)), 0, 1);
	}
	#endif
	
	// Update any balance control required.
	if (mp_balance_trick_component->mBalanceTrickType == CRCD(0xa549b57b, "Lip"))
	{
		if (Mdl::Skate::Instance()->GetCareer()->GetCheat(CRCD(0xcd09e062, "CHEAT_PERFECT_RAIL")))
		{
			// Here, set the flag. It may seem redundant, but the above line is very likely
			// to be hacked by gameshark. They probably won't notice this one, which will
			// set the flags as if they had actually enabled the cheat -- which enables us
			// to detect that it has been turned on more easily.
			Mdl::Skate::Instance()->GetCareer()->SetGlobalFlag( Script::GetInteger(CRCD(0xcd09e062, "CHEAT_PERFECT_RAIL")));
			mp_balance_trick_component->mLip.mManualLean = 0.0f;
			mp_balance_trick_component->mLip.mManualLeanDir = 0.0f;
			g_CheatsEnabled = true;
		}
		mp_balance_trick_component->mLip.DoManualPhysics();
	}
	
	handle_tensing();
	if (maybe_flag_ollie_exception())	
	{
		// Trigger the lip jump event.
		maybe_trip_rail_trigger(TRIGGER_LIP_JUMP);
	}

	// lip tricks are very rail-like when determining which world sector you've just tricked off of
	if (mp_rail_node)
	{
		if (mp_rail_man->GetNodeArray())
		{
			const CRailNode* pRail = mp_rail_node;
			Script::CArray* pNodeArray = Script::GetArray(CRCD(0xc472ecc5, "NodeArray"));
			Script::CStruct* pNode = pNodeArray->GetStructure(pRail->GetNode());
			pNode->GetChecksum(CRCD(0xa1dc81f9, "name"), &m_last_rail_node_name, Script::ASSERT);
			mp_trick_component->TrickOffObject(m_last_rail_node_name);
		}
	}
	
	m_lip_time = Tmr::GetTime();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::do_rail_physics (   )
{
	SetFlagFalse(SPINE_PHYSICS);
	SetFlagFalse(IN_ACID_DROP);
	SetFlagFalse(IN_RECOVERY);
	SetFlagFalse(AIR_ACID_DROP_DISALLOWED);

	// First of all we apply the movement due to contact; best to do it first, as then we will be on the rail we are moving along

	if (mp_movable_contact_component->HaveContact())
	{
		// need to update the transform of the rail manager
		// no need to do this														
		CRailManagerComponent* p_rail_manager_component = GetRailManagerComponentFromObject(mp_movable_contact_component->GetContact()->GetObject());
		Dbg_Assert(p_rail_manager_component);
		
		p_rail_manager_component->UpdateRailManager();
	
		if (mp_movable_contact_component->UpdateContact(GetPos()))
		{
			
			GetPos() += mp_movable_contact_component->GetContact()->GetMovement();
			DUMP_POSITION;
			if (mp_movable_contact_component->GetContact()->IsRotated())
			{
				GetMatrix() *= mp_movable_contact_component->GetContact()->GetRotation();
				m_lerping_display_matrix *= mp_movable_contact_component->GetContact()->GetRotation();
			}
				
		}
	}

	// set default rerail time	
	m_rerail_time = static_cast< Tmr::Time >(GetPhysicsFloat(CRCD(0x2b4645ad, "Rail_minimum_rerail_time")));
	// and dissalow any overriding of this
	SetFlagFalse(CAN_RERAIL);		// dont allow rerailing after coming off a segment

	
	if (maybe_flag_ollie_exception())
	{
		maybe_trip_rail_trigger(TRIGGER_JUMP_OFF);
		
		m_rerail_time = static_cast< Tmr::Time >(GetPhysicsFloat(CRCD(0xbf35053, "Rail_jump_rerail_time")));
		
		SetFlagTrue(OLLIED_FROM_RAIL);
		
		// when we jump off a rail, then raise him up one inch, so we don't collide with the top of a fence or something
		GetPos()[Y] += 1.0f;			// up one inch......
		DUMP_POSITION;
	}
	else
	{
		switch (mp_balance_trick_component->mBalanceTrickType)
		{
			case 0:
			case 0x0ac90769: // NoseManual	
			case 0xef24413b: // Manual
			case 0xa549b57b: // Lip	
				break;
				
			case 0x255ed86f: // Grind
			case 0x8d10119d: // Slide
				mp_balance_trick_component->mGrind.DoManualPhysics();
				if (Mdl::Skate::Instance()->GetCareer()->GetCheat(CRCD(0xcd09e062, "CHEAT_PERFECT_RAIL")))
				{
					// Here, set the flag. It may seem redundant, but the above line is very likely
					// to be hacked by gameshark. They probably won't notice this one, which will
					// set the flags as if they had actually enabled the cheat -- which enables us
					// to detect that it has been turned on more easily.
					Mdl::Skate::Instance()->GetCareer()->SetGlobalFlag( Script::GetInteger(CRCD(0xcd09e062, "CHEAT_PERFECT_RAIL")));
					mp_balance_trick_component->mGrind.mManualLean = 0.0f;
					mp_balance_trick_component->mGrind.mManualLeanDir = 0.0f;
					g_CheatsEnabled = true;
				}
				break;	
						
			default:	
				Dbg_Assert(false);
				break;
		}		
	
		// Get the rail segments

		const CRailNode* pStart = mp_rail_node;
		const CRailNode* pEnd = pStart->GetNextLink();
		
		const CRailNode* pFrom = pStart;
		const CRailNode* pOnto = NULL;
		
		#ifdef __USER_DAN__
		if (Script::GetInteger(CRCD(0x1a5eab7, "rail_highlights")))
		{
			Gfx::AddDebugLine(mp_rail_man->GetPos(pStart), mp_rail_man->GetPos(pEnd), MAKE_RGB(Mth::Rnd(256), Mth::Rnd(256), Mth::Rnd(256)), 0, 1);
			Gfx::AddDebugLine(mp_rail_man->GetPos(pStart) + Mth::Vector(1.0f, 0.0f, 0.0f), mp_rail_man->GetPos(pEnd) + Mth::Vector(1.0f, 0.0f, 0.0f), MAKE_RGB(Mth::Rnd(256), Mth::Rnd(256), Mth::Rnd(256)), 0, 1);
			Gfx::AddDebugLine(mp_rail_man->GetPos(pStart) + Mth::Vector(0.0f, 1.0f, 0.0f), mp_rail_man->GetPos(pEnd) + Mth::Vector(0.0f, 1.0f, 0.0f), MAKE_RGB(Mth::Rnd(256), Mth::Rnd(256), Mth::Rnd(256)), 0, 1);
			Gfx::AddDebugLine(mp_rail_man->GetPos(pStart) + Mth::Vector(0.0f, 0.0f, 1.0f), mp_rail_man->GetPos(pEnd) + Mth::Vector(0.0f, 0.0f, 1.0f), MAKE_RGB(Mth::Rnd(256), Mth::Rnd(256), Mth::Rnd(256)), 0, 1);
		}
		#endif
		
		Mth::Vector	dir = mp_rail_man->GetPos(pEnd) - mp_rail_man->GetPos(pStart);
		float segment_length = dir.Length();
		dir *= 1.0f / segment_length;

		// sign is which way we are going along the rail
		float old_sign = Mth::Sgn(Mth::DotProduct(dir, GetVel()));

		// Get gravity force 	
		Mth::Vector gravity(0.0f, GetPhysicsFloat(CRCD(0xd1f46992, "Physics_Rail_Gravity")), 0.0f);

		// Project gravity onto the line we are on
		gravity.ProjectToNormal(dir);
		GetVel() += gravity * m_frame_length;
		DUMP_VELOCITY;
		
		// sign is which way we are going along the rail
		float sign = Mth::Sgn(Mth::DotProduct(dir, GetVel()));
		
		// sign might have changed here
		// so could do the "flipping on a rail" thing
		// .................
		
		if (sign != old_sign)
		{
			// Note, we JUST flip the "Backwards" flag, as we want to stay in essentailly the same
			// pose as before, and as the sign of our velocity dotted with the rail has
			// changed, then all we need to to is tell the code we are
			// going in the opposite direction to what we were going before, and the
			// target vector will be calculated correctly.
			mRail_Backwards = !mRail_Backwards;			 		
		}
		
		// check to see if we are on the last segment of the rail
		// this logic could be folded into the logic below but it's perhps clearer to have it here
		bool last_segment = false;
		if (sign < 0.0f)
		{
			if (pStart->GetPrevLink() && pStart->GetPrevLink()->IsActive())
			{
				Mth::Vector v1, v2;
				v1 = mp_rail_man->GetPos(pEnd) - mp_rail_man->GetPos(pStart);
				v2 = mp_rail_man->GetPos(pStart) - mp_rail_man->GetPos(pStart->GetPrevLink());
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
				v1 = mp_rail_man->GetPos(pStart) - mp_rail_man->GetPos(pEnd);
				v2 = mp_rail_man->GetPos(pEnd) - mp_rail_man->GetPos(pEnd->GetNextLink());
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
			length_along = (GetPos() - mp_rail_man->GetPos(pEnd)).Length();
		}
		else
		{
			// going forwards, so it's the distance from the start
			length_along = (GetPos() - mp_rail_man->GetPos(pStart)).Length();
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
					&& Rail_ValidInEditor(mp_rail_man->GetPos(pStart), mp_rail_man->GetPos(pStart->GetPrevLink())))
				{
					if (!last_segment)
					{
						// go onto previous segment
						GetPos() = mp_rail_man->GetPos(pStart);
						DUMP_POSITION;
						mp_rail_node = pStart->GetPrevLink();
						set_terrain(mp_rail_node->GetTerrain());
						maybe_trip_rail_trigger(TRIGGER_SKATE_ONTO);
					}		   
					else
					{
						skate_off_rail(mp_rail_man->GetPos(pStart));
					} 		
				}
				else
				{
					skate_off_rail(mp_rail_man->GetPos(pStart));
				}
			}
			else
			{
				if (pEnd->GetNextLink()
					&& pEnd->IsActive()
					&& Rail_ValidInEditor(mp_rail_man->GetPos(pEnd), mp_rail_man->GetPos(pEnd->GetNextLink())))
				{
					if (!last_segment)
					{
						GetPos() = mp_rail_man->GetPos(pEnd);
						DUMP_POSITION;
						mp_rail_node = pEnd;					
						set_terrain(mp_rail_node->GetTerrain());
						maybe_trip_rail_trigger(TRIGGER_SKATE_ONTO);						
					}		   
					else
					{
						skate_off_rail(mp_rail_man->GetPos(pEnd));
					} 						
				}
				else
				{
					skate_off_rail(mp_rail_man->GetPos(pEnd));
				}
			}		
		}
		
		if (GetState() == RAIL)
		{
			// recalculate start, end, dir, as we might be on a new segment
			const CRailNode* pStart = mp_rail_node;
			const CRailNode* pEnd = pStart->GetNextLink();
			
			Mth::Vector	dir = mp_rail_man->GetPos(pEnd) - mp_rail_man->GetPos(pStart);
			dir.Normalize();

		    // sign also may have changed, now that we are auto-linking rail segments
			
			// sign is which way we are going along the rail
			float sign = Mth::Sgn(Mth::DotProduct(dir,GetVel()));

			m_rail_time = Tmr::GetTime();
								
			GetVel().RotateToNormal(dir);
			GetVel() *= sign;						   						// sign won't be on a new segment
			DUMP_VELOCITY;

			float facing_sign = mRail_Backwards ? -sign : sign;
			
			// z is forward
			Mth::Vector target_forward = dir * facing_sign;

			m_lerping_display_matrix[Z] = Mth::Lerp(m_lerping_display_matrix[Z], target_forward, 0.3f);
			m_lerping_display_matrix[Z].Normalize(); 
			
			#if 0 // old code
			// m_lerping_display_matrix[Y].Set(0.0f, 1.0f, 0.0f);
			// m_lerping_display_matrix[X] = Mth::CrossProduct(
				// m_lerping_display_matrix[Y],
				// m_lerping_display_matrix[Z]
			// );
			#else
			m_lerping_display_matrix[X].Set(
				m_lerping_display_matrix[Z][Z],
				0.0f,
				-m_lerping_display_matrix[Z][X],
				0.0f
			);
			#endif
			m_lerping_display_matrix[X].Normalize();
			
			m_lerping_display_matrix[Y] = Mth::CrossProduct(
				m_lerping_display_matrix[Z],
				m_lerping_display_matrix[X]
			);
			m_lerping_display_matrix[Y].Normalize();

			// adjust our Z value towards the new value												 
			GetMatrix()[Z] = target_forward;
			GetMatrix()[Z].Normalize(); 
			
			#if 0 // old code
			// GetMatrix()[Y].Set(0.0f, 1.0f, 0.0f);
			// GetMatrix()[X] = Mth::CrossProduct(GetMatrix()[Y],GetMatrix()[Z]);
			#else
			GetMatrix()[X].Set(
				GetMatrix()[Z][Z],
				0.0f,
				-GetMatrix()[Z][X],
				0.0f
			);
			#endif
			GetMatrix()[X].Normalize();
			
			GetMatrix()[Y] = Mth::CrossProduct(GetMatrix()[Z], GetMatrix()[X]);
			GetMatrix()[Y].Normalize();

			Mth::Vector m_movement(0.0f, 0.0f, 0.0f);
			
			// This is where we do the actual movement
			
			// if this makes us bump into the wall or the ground, then we should leave the rail
			
			Mth::Vector old_pos = GetPos();  	
			GetPos() += GetVel() * m_frame_length;			// current movement
			GetPos() += extra_dist * target_forward;						// any extra dist from previous segment
			GetPos() += m_movement;							// movement due to contact with moving object
			DUMP_POSITION;
			
			// Mick:  use "old_pos" to generate the direction
			// so it is guarenteed to be parallel to the rail
			// and m_pos might have been adjusted if we continued from
			// one rail to another (in-line) rail, like in the park editor
			Mth::Vector movement = GetPos() - old_pos;
			Mth::Vector direction = movement;
			direction.Normalize();
			
			bool always_check = false;
			if (!last_segment && Ed::CParkEditor::Instance()->UsingCustomPark())
			{
				// in the park editor we can have tight curves that end in walls, so we want to always do the forward check
				// if we are on a segment that is horizontal, and leads to another segment that is also horizontal
				Mth::Vector from = mp_rail_man->GetPos(pFrom->GetNextLink()) - mp_rail_man->GetPos(pFrom);				
				Mth::Vector onto = mp_rail_man->GetPos(pOnto->GetNextLink()) - mp_rail_man->GetPos(pOnto);		
				from.Normalize();
				onto.Normalize();
				float delta = Mth::Abs(from[Y] - onto[Y]);		
				if (delta < 0.01f)
				{
					// lines have a sufficently close Y angle
					always_check = true;
				}
			}
			
			// Only check for hitting a wall if we are on a segment of rail that has no more rail
			if (last_segment || always_check)
			{
				m_col_start = old_pos;
				m_col_end = GetPos();
				m_col_end += movement + (direction * 6.0f);
				
				// raise them up one inch, so we don't collide with the rail
				m_col_start += GetMatrix()[Y];			 
				m_col_end += GetMatrix()[Y];  
				if (get_member_feeler_collision())
				{
					// if in the park editor, then ignore collision with invisible surfaces 
					if (!Ed::CParkEditor::Instance()->UsingCustomPark() || !(m_feeler.GetFlags() & mFD_INVISIBLE))
					{
						maybe_trip_rail_trigger(TRIGGER_SKATE_OFF);
						// don't let him make this movement!!
						GetPos() = GetOldPos();
						GetPos()[Y] += 1.0f;
						DUMP_POSITION;
						
						// project velocity along the plane if we run into a wall
						GetVel().ProjectToPlane(m_feeler.GetNormal());
						DUMP_VELOCITY;
						
						SetState(AIR);			// knocked off rail, as something in front
						FLAGEXCEPTION(CRCD(0xafaa46ba, "OffRail"));
					}
				}
			}
		}	
	}			
	
	// set the normal value, so the camera is not too confused
	m_current_normal = GetMatrix()[Y];
	  
	// Add mGrindTweak to the score.
	// adjusted by the robot rail mult (1.0 to 0.1, depending on how much you've ground the rail)
	mp_score_component->GetScore()->TweakTrick(mGrindTweak * mp_score_component->GetScore()->GetRobotRailMult());

	// if we've already started doing a trick, then start remembering our trick chain
	{
		const CRailNode* pRail = mp_rail_node;
		Dbg_Assert(pRail);

		if (mp_rail_man->GetNodeArray())
		{
			Script::CArray* pNodeArray=Script::GetArray(CRCD(0xc472ecc5, "NodeArray"));
			Script::CStruct* pNode=pNodeArray->GetStructure(pRail->GetNode());
			pNode->GetChecksum(CRCD(0xa1dc81f9, "name"), &m_last_rail_node_name, true);
			mp_trick_component->TrickOffObject(m_last_rail_node_name);
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::ollie_off_rail_rotate (   )
{
	float rot;
	if (mp_input_component->GetControlPad().m_left.GetPressed())
	{
		rot = Mth::DegToRad(GetPhysicsFloat(CRCD(0x38505ef3, "Rail_Jump_Angle")));
	}
	else if (mp_input_component->GetControlPad().m_right.GetPressed())
	{
		rot = -Mth::DegToRad(GetPhysicsFloat(CRCD(0x38505ef3, "Rail_Jump_Angle")));
	}
	else return;

	GetVel().RotateY(rot);		 
	GetMatrix().RotateYLocal(rot);				
	m_lerping_display_matrix.RotateYLocal(rot);				
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::skate_off_rail ( const Mth::Vector& off_point )
{
	// we have skated off a rail; either it was the end of a rail or we hit a sharp corner
	// we need to see if there is another rail in front of us that we can continue skating on

	bool really_off = true;
	
	Mth::Vector	a;
	if (Ed::CParkEditor::Instance()->UsingCustomPark())
	{
		// in the park editor, we use the last point on the rail we just took off from as the start of the line to use for looking for new rails
		// so we don't get rail segments earlier in the list
		a = off_point;
	}
	else
	{
		// go back a step
		a = GetPos() - GetVel() * m_frame_length;
		DUMP_POSITION;
	}
	
	// use current pos, as we know that is off the end
	Mth::Vector b = GetPos();
	
	Mth::Vector rail_pos;	
	CRailNode* pNode;	
	

	// we need to tilt the line down, as the rail code currently fails to find a rail if the line we check is exactly parallel with it	
	a[Y] += 6.0f;							   
	b[Y] += 6.1f;							   

	float min_dot = cosf(Mth::DegToRad(GetPhysicsFloat(CRCD(0x76c1da15, "Rail_Corner_Leave_Angle"))));

	// Note: we are now passing in Rail_Side() of the current rail and velocity
	// so we can see if we switch from a rail on a left facing ledge to a rail on a right facing ledge, and try to inhibit that type of transition
	// in favour of one that retains the same side
	if (mp_rail_man->StickToRail(
			a,
			b, 
			&rail_pos,
			&pNode,
			mp_rail_node,
			min_dot,
			mp_rail_node->Side(GetVel())
		))
	{		
		// Mick, in park editor, we also disallow this if the rail is the next or the prev rail node from our current node
		if (mp_rail_node != pNode && (
				!Ed::CParkEditor::Instance()->UsingCustomPark()
				|| mp_rail_node->GetNextLink() != pNode
				&& mp_rail_node->GetPrevLink() != pNode
			))
		{
			const CRailNode* pNewStart = pNode;
			const CRailNode* pNewEnd = pNewStart->GetNextLink();	
			
			// check to see if our new position is within the two points
			Mth::Vector	to_start = mp_rail_man->GetPos(pNewStart) - GetPos();
			Mth::Vector	to_end = mp_rail_man->GetPos(pNewEnd) - GetPos();
			
			float mid_dot = Mth::DotProduct(to_start, to_end);

			// In game, the point must actualy be in the line, so mid dot will be negative
			bool ok = mid_dot < 0.0f;					
			
			// Park Editor specific rail joining			
			if (!ok && Ed::CParkEditor::Instance()->UsingCustomPark())
			{
				// in the park editor, we let the user overshoot, so he sticks to tight curves that are between two pieces
				ok = true;

				// we need to ensure that the rail segment is a good continuation of the segment that we were just on
				// Namely that one of the new start/end points is close to the end of the rail that we just came off
				// and that the direction we will be going along is within 45 degrees of the direction we were going along before. 
				// (NOT DONE HERE, as the following test is sufficent)
											 
				// first a simple elimination, if the rail is longer than our velocity projected onto it then we can not posibly have overshot it!!
				Mth::Vector	new_rail_segment = mp_rail_man->GetPos(pNewEnd) - mp_rail_man->GetPos(pNewStart);
				Mth::Vector skater_movement = b - a;
				float new_rail_length = new_rail_segment.Length();
				if (new_rail_length > skater_movement.Length())
				{
					ok = false;
				}
				
				// now we could find the shortest distance between two line segments; should be within 2 inches
				// (ALSO NOT DONE)
				
				// bit of a patch for the park editor, it's gone past the end of the rail, so set him in the middle of the rail, so we can continue nicely
				// possible glitch here, but better than falling of the rail
				// It's going to be a small rail anyway, so won't look too bad.
				if (ok && mid_dot >= 0.0f)
				{
					rail_pos = (mp_rail_man->GetPos(pNewEnd) + mp_rail_man->GetPos(pNewStart)) / 2.0f;
				}

			}

			if (ok)
			{
				Mth::Vector	newdir = mp_rail_man->GetPos(pNewEnd) - mp_rail_man->GetPos(pNewStart);
				newdir.Normalize();
				if (GetVel()[X] == 0.0f && GetVel()[Z] == 0.0f)
				{
					GetVel()[X] = GetMatrix()[Z][X];
					GetVel()[Z] = GetMatrix()[Z][Z];
					DUMP_VELOCITY;
				}

				// sign is which way we are going along the rail
				float sign = Mth::Sgn(Mth::DotProduct(newdir, GetVel()));
				GetVel().RotateToNormal(newdir);
				GetVel() *= sign;
				
				mp_rail_node = pNode;		// oh yes, this is the node
				GetPos() = rail_pos;			// move to closest position on the line
				DUMP_POSITION;
				maybe_trip_rail_trigger(TRIGGER_SKATE_ONTO);
				set_terrain(mp_rail_node->GetTerrain());
		
				really_off = false;			
			}				
			else
			{
				// new position is outside this rail
			}	
		}
		else
		{
			// its the same as this one
		}	
	}
	else
	{
		// did not find any
	}
	
	if (!really_off) return;

	maybe_trip_rail_trigger(TRIGGER_SKATE_OFF);
	SetState(AIR);
	GetPos()[Y] += 1.0f;
	DUMP_POSITION;
	FLAGEXCEPTION(CRCD(0xafaa46ba,"OffRail"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::maybe_trip_rail_trigger ( uint32 type )
{
	// given that m_rail_node is valid, then trigger any script associated with this rail node
	// will search backwards for the first rail that has a trigger script, and then execute that.
	
	if (!mp_rail_node) return;
	
	Dbg_Assert(mp_rail_man);
		 
	
	const CRailNode* pStartOfRail = mp_rail_node;																	 
	const CRailNode* pRail = mp_rail_node;																	 
	
	// no node array in rail manager indicates auto generated rails, so just return
	Script::CArray* pNodeArray = mp_rail_man->GetNodeArray();
	if (!pNodeArray) return;
	
	Script::CStruct* pNode = pNodeArray->GetStructure(mp_rail_node->GetNode());

	// find a rail node that has a "TriggerScript" in it
	uint32 value = 0;
	if (pNode->GetChecksum(CRCD(0x2ca8a299, "Triggerscript"), &value))
	{
		// we got it 
	}
	else
	{
		// did not get it, so scoot backwards until we detect a loop or we find one
		const CRailNode* p_loop_detect = pRail;	 	// start loop detect at the start
		pRail = pRail->GetPrevLink(); 				// and the first node we check is the next one
		int loop_advance = 0;
		while (pRail && pRail != pStartOfRail && pRail != p_loop_detect)
		{
			pNode = pNodeArray->GetStructure(pRail->GetNode());
			if (pNode->GetChecksum(CRCD(0x2ca8a299, "Triggerscript"), &value)) break;
			
			pRail = pRail->GetPrevLink(); 
			// The p_loop_detect pointer goes backwards at half speed, so if there is a loop
			// then pRail is guarenteed to eventually catch up with p_loop_detect
			if (loop_advance)
			{
				p_loop_detect = p_loop_detect->GetPrevLink();				
			}
			loop_advance ^= 1;
		}
	}

	if (value)
	{
		// If this is different to last time, then set flag accordingly
		uint32 new_last_rail_node_name;
		pNode->GetChecksum(CRCD(0xa1dc81f9, "name"), &new_last_rail_node_name, Script::ASSERT);
//		printf ("%s,%s\n",Script::FindChecksumName(new_last_rail_node_name), Script::FindChecksumName(m_last_rail_node_name));
		if (new_last_rail_node_name != m_last_rail_trigger_node_name)
		{
			SetFlagTrue(NEW_RAIL);
		}
		else
		{
			SetFlagFalse(NEW_RAIL);
		}
		m_last_rail_node_name = new_last_rail_node_name;
		m_last_rail_trigger_node_name = new_last_rail_node_name;
		
		
		// if we are using the default node array, then set it to NULL, so TriggerEventFromNode can use this default, which is a lot faster
		if (pNodeArray == Script::GetArray(CRCD(0xc472ecc5, "NodeArray")))
		{
			pNodeArray = NULL;
		}
		
		mp_trigger_component->TripTrigger(
			type,
			m_last_rail_node_name,
			pNodeArray,
			mp_movable_contact_component->HaveContact() ? mp_movable_contact_component->GetContact()->GetObject() : NULL
		);
	}
}		

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::handle_tensing (   )
{
	if (!GetFlag(TENSE) && mp_input_component->GetControlPad().m_x.GetPressed())
	{
		// just starting to tense, so set the flag 
		SetFlagTrue(TENSE);		
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterCorePhysicsComponent::get_member_feeler_collision ( uint16 ignore_1, uint16 ignore_0 )
{
	// use m_col_start and m_col_end to set up m_feeler, then check for collision and if there is a collision, then get the various skater collision flags

	m_feeler.SetLine(m_col_start, m_col_end);
	m_feeler.SetIgnore(ignore_1, ignore_0);

	bool collision = m_feeler.GetCollision();
	
	if (!collision) return false;
	
	if (m_feeler.GetNodeChecksum() && m_feeler.GetTrigger())
	{
		// Given a node name, then find the checksum of the triggerscript in that node
		// note that this is VERY SLOW, as it has to scan the whole node array
		// it's usually only called once per frame, per skater, but might still be a good candidate for optimization
		
		// Now just clear the script, indicating it needed doing later
		m_feeler.SetScript(0);
	}

	// get any skating specific flags here
	uint16 flags = m_feeler.GetFlags();

	// get normal, as it's the kind of things we use in face flag determination				   
	Mth::Vector	normal = m_feeler.GetNormal();								
					
	// Make wallable default to off.				
	m_col_flag_wallable = false;
	
	///////////////////////////////////////////////////////////////////
	// Vert
	
	m_col_flag_vert = flags & mFD_VERT;
	
	////////////////////////////////////////////////////////////////
	// Skatable					   
	   
	if (flags & mFD_SKATABLE)
	{
		// if flaged as skatable, than that overrides all other flags
		m_col_flag_skatable = true;
	}
	else if (flags & mFD_NOT_SKATABLE)
	{
		// if flagged as non_skatable, then that overrides all other flags
		m_col_flag_skatable = false;
	}
	else if (flags & mFD_VERT)
	{			
		// if flagged as VERT, then it's the top of a QP, so it's skatable and this overrides the angle flag
		m_col_flag_skatable = true;
	}
	else if (flags & mFD_WALL_RIDABLE)
	{
		m_col_flag_skatable = false;
		m_col_flag_wallable = true;
	}
	else
	{
		// determine the skatablitlity based on the angle
		// if angle is > 5 degrees from vert, then it is skatable
		// here the y component of the normal is the cosine of the angle from vertical
								  
		if (normal[Y] < sinf(Mth::DegToRad(GetPhysicsFloat(CRCD(0x3eede4d3,"Wall_Non_Skatable_Angle")))))
		{
			// get could possibly do another test here to see if there is a polygon beneath this meaning this is a QP
			// however, we want to be flagging the tops of all QPs
			m_col_flag_skatable = false;
		}
		else
		{
			m_col_flag_skatable = true;
		}
		
	}

	return collision;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::do_jump ( Script::CStruct *pParams )
{
	// Called by the Jump script command
	
	// play sounds before modifying the skater speed, as the sound volume and pitch are based on the pre-jump velocity
	bool play_sound = !pParams->ContainsFlag(CRCD(0xe13620a8, "no_sound"));
	
	switch (GetState())
	{
		case GROUND:
			mp_trigger_component->CheckFeelerForTrigger(TRIGGER_JUMP_OFF, m_last_ground_feeler);
			if (mp_physics_control_component->HaveBeenReset()) return;
			
			// K: Remember the last ground position for calculating which side of the rail we're on later.
			m_last_ground_pos = GetPos();
			
			if (play_sound)
			{
				mp_sound_component->PlayJumpSound(GetObject()->GetVel().Length() / GetSkater()->GetScriptedStat(CRCD(0xcc5f87aa, "Skater_Max_Max_Speed_Stat")), m_last_ground_feeler.GetTerrain());
			}	
			
			maybe_straight_up();
			SetFlagTrue(CAN_BREAK_VERT);
			break;

		case AIR:
			if (play_sound)
			{
				mp_sound_component->PlayJumpSound(GetObject()->GetVel().Length() / GetSkater()->GetScriptedStat(CRCD(0xcc5f87aa, "Skater_Max_Max_Speed_Stat")), m_last_ground_feeler.GetTerrain());
			}	
			break;

		case RAIL:
			if (play_sound)
			{
				Dbg_Assert(mp_rail_node);
				mp_sound_component->PlayJumpSound(GetObject()->GetVel().Length() / GetSkater()->GetScriptedStat(CRCD(0xcc5f87aa, "Skater_Max_Max_Speed_Stat")), mp_state_component->m_terrain);
			}	
			break;
			
		default:
			break;
	}
			  
	Dbg_Assert(pParams);
	
	int max_tense_time = GetPhysicsInt(CRCD(0xf733c097, "skater_max_tense_time"));
	m_tense_time = Mth::Min(m_tense_time, max_tense_time);
	
	float max_jump_speed;
	float min_jump_speed;
	
	bool from_vert = GetFlag(VERT_AIR) || (GetState() == GROUND && GetFlag(LAST_POLY_WAS_VERT));

	if (pParams->ContainsFlag(CRCD(0xec6b7fc7, "BonelessHeight")))
	{
		if (from_vert)
		{
			max_jump_speed = GetSkater()->GetScriptedStat(CRCD(0x39766891, "Physics_Boneless_air_Jump_Speed_stat"));
			min_jump_speed = GetSkater()->GetScriptedStat(CRCD(0x4cdb18cb, "Physics_Boneless_air_Jump_Speed_min_stat"));
		}
		else
		{
			max_jump_speed = GetSkater()->GetScriptedStat(CRCD(0x8851a76e, "Physics_Boneless_Jump_Speed_stat"));
			min_jump_speed = GetSkater()->GetScriptedStat(CRCD(0x6e8efe38, "Physics_Boneless_Jump_Speed_min_stat"));
		}
	}	
	else
	{
		if (from_vert)
		{
			max_jump_speed = GetSkater()->GetScriptedStat(CRCD(0x9f79c8ea, "Physics_air_Jump_Speed_stat"));
			min_jump_speed = GetSkater()->GetScriptedStat(CRCD(0x91b07824, "Physics_air_Jump_Speed_min_stat"));
		}
		else
		{
			max_jump_speed = GetSkater()->GetScriptedStat(CRCD(0x2ade3ad, "Physics_Jump_Speed_stat"));
			min_jump_speed = GetSkater()->GetScriptedStat(CRCD(0xc8815e43, "Physics_Jump_Speed_min_stat"));
		}
	}	
	
	float jump_speed = Mth::LinearMap(min_jump_speed, max_jump_speed, m_tense_time, 0.0f, max_tense_time);
	
	// If any speed param is specified, then use that instead. Need by Zac for when small skater is doing scripted jumps in the front end.
	pParams->GetFloat(CRCD(0xf0d90109, "speed"), &jump_speed);

	SetFlagFalse(TENSE);

	// if we have a very high downward velocity and we are landing on vert, then do not jump

	// when jumping, don't add in the current velocity if it is less than zero
	// this will give us nicer feeling jumps when 
	//  - comping down wall rides
	//  - skating down slopes
	//  - skating down the back of bumps, liks the speed bumps in CA
	//  - landing on a QP

	
	if (GetFlag(VERT_AIR) && GetVel()[Y] < 0.0f)
	{
		 	// push outward	(and upward) away from vert poly
			GetVel() += jump_speed * m_display_normal;
			DUMP_VELOCITY;
			jump_speed = 0;
			SetFlagFalse(VERT_AIR);
	}
	else
	{
		if (GetVel()[Y] < 0.0f)
		{
			GetVel()[Y] = 0.0f;
			DUMP_VELOCITY;
		}
	}
	
	GetVel()[Y] += jump_speed;
	DUMP_VELOCITY;

	// if pointing down, then jump down, as moving up will make us pass through whatever we are on	
	if (GetMatrix()[Y][Y] < -0.1f)
	{
		GetVel()[Y] -= jump_speed * 1.5f;
		DUMP_VELOCITY;
		GetPos() += GetMatrix()[Y] * 12.0f;
		DUMP_POSITION;
	}
	
	// allow side jumps when jumping off rails or when late jumping in air after skating off a rail
	if (GetState() == RAIL || (GetState() == AIR && m_previous_state == RAIL))
	{
		ollie_off_rail_rotate();
	}
	
	// don't change the state until after sound is played, as the above switch relyies on the GROUND/RAIL state...
	SetState(AIR);
	
	m_last_jump_time_stamp = Tmr::GetTime();
	
	GetObject()->BroadcastEvent(CRCD(0x8687163a, "SkaterJump"));
}			

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::start_skitch (   )
{
	// Start Skitching, given that we've queue one up
	// Note, mp_skitch_object is a smart pointer so there is a slight possibility that it might have died but we check that
	
	// Mick:  I'm using .Convert here.   I'm not sure if makes any difference,
	// but it can't hurt and should compile the same.
	if (!mp_skitch_object.Convert()) return;
	
	mp_movable_contact_component->ObtainContact(mp_skitch_object);
	SetFlagTrue(SKITCHING);
	m_moving_to_skitch = true;
	move_to_skitch_point();
	
	// Mick:  For some reason we sometimes get a NULL smart pointer here
	// so I'm checking again here, in addition to changing the code
	// to use .Convert 	
	if (!mp_skitch_object.Convert()) return;
	
	mp_skitch_object->SelfEvent(CRCD(0x35224f25, "SkitchOn"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::do_grind_trick ( uint Direction, bool Right, bool Parallel, bool Backwards, bool Regular )
{
	int SubArrayIndex = 0;
	
	// Choose which array index to use.
	switch (Direction)
	{
		case 0: // Nowt
			SubArrayIndex = 0;
			break;
		case PAD_U: // Up
			SubArrayIndex = 1;
			break;
		case PAD_D: // Down
			SubArrayIndex = 2;
			break;
		case PAD_L: // Left
			SubArrayIndex = 3;
			break;
		case PAD_R: // Right
			SubArrayIndex = 4;
			break;
		case PAD_UL: // UpLeft
			SubArrayIndex = 5;
			break;
		case PAD_UR: // UpRight
			SubArrayIndex = 6;
			break;
		case PAD_DL: // DownLeft
			SubArrayIndex = 7;
			break;
		case PAD_DR: // DownRight
			SubArrayIndex = 8;
			break;
		default:
			Dbg_MsgAssert(false, ("Bad button checksum"));
			break;
	}	
		
	// GetArray will assert if GrindTrickList not found.
	Script::CArray* pMainArray = Script::GetArray(CRCD(0x2ab3341d, "GrindTrickList"));
	Script::CArray* pSubArray = pMainArray->GetArray(SubArrayIndex);

	uint Index = 0;
	if (Right) Index |= 1;
	if (Parallel) Index |= 2;
	if (Backwards) Index |= 4;
	if (Regular) Index |= 8;

	// Initialise mGrindTweak, which should get set by a SetGrindTweak command in the script that is about to be run.
	ResetGrindTweak();
		
	CSkaterFlipAndRotateComponent* p_flip_and_rotate_component = GetSkaterFlipAndRotateComponentFromObject(GetObject());
	Dbg_Assert(p_flip_and_rotate_component);
	p_flip_and_rotate_component->DoAnyFlipRotateOrBoardRotateAfters();
	
	if (!GetObject()->GetScript())
	{
		GetObject()->SetScript(new Script::CScript);
	}
	
	GetObject()->GetScript()->SetScript(pSubArray->GetNameChecksum(Index), NULL, GetObject());
	GetObject()->GetScript()->Update();
	
	// Set the mDoingTrick flag so that the camera can detect that a trick is being done.
	mp_state_component->SetDoingTrick(true);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::StopSkitch (   )
{
	// If we were skitching, then stop it
	
	if (!GetFlag(SKITCHING)) return;
	
	SetFlagFalse(SKITCHING);
	if (mp_skitch_object)
	{
		mp_skitch_object->SelfEvent(CRCD(0x2739b86d, "SkitchOff"));
		mp_skitch_object = NULL;
	}
	mp_movable_contact_component->LoseAnyContact();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::setup_default_collision_cache (   )
{
	// reduced Y extent as this no longer subsumes the uberfrig
	Mth::CBBox bbox(
		GetPos() - Mth::Vector(FEET(15.0f), FEET(17.0f), FEET(15.0f), 0.0f),
		GetPos() + Mth::Vector(FEET(15.0f), FEET(8.0f), FEET(15.0f), 0.0f)
	);
	
	mp_coll_cache->Update(bbox);
	CFeeler::sSetDefaultCache(mp_coll_cache);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCorePhysicsComponent::update_special_friction_index (   )
{
	if (m_special_friction_index == 0) return;
	
	if (Tmr::ElapsedTime(m_special_friction_decrement_time_stamp) > static_cast< Tmr::Time >(1000.0f * Script::GetFloat(CRCD(0xf4813ad5, "Physics_Time_Before_Free_Revert"))))
	{
		m_special_friction_index--;
		MESSAGE("You earned a free revert!!!!");
		if (m_special_friction_index != 0)
		{
			m_special_friction_decrement_time_stamp = Tmr::GetTime();
		}
	}
}

}
