//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterAdjustPhysicsComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/26/3
//****************************************************************************

#include <sk/components/skateradjustphysicscomponent.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/engine/contact.h>
#include <sk/engine/feeler.h>
#include <sk/parkeditor2/parked.h>

#include <gel/object/compositeobject.h>
#include <gel/objtrack.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

#include <gel/components/modelcomponent.h>
#include <gel/components/shadowcomponent.h>
#include <gel/components/movablecontactcomponent.h>

void	TrackingLine2(int type, Mth::Vector &start, Mth::Vector &end)
{
	if	(Script::GetInt(CRCD(0x19fb78fa,"output_tracking_lines")))
	{
//		Gfx::AddDebugLine(start, end, 0xffffff);
		printf ("Tracking%d %.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",type, start[X], start[Y], start[Z], end[X], end[Y], end[Z]);
	}
}

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CSkaterAdjustPhysicsComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterAdjustPhysicsComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterAdjustPhysicsComponent::CSkaterAdjustPhysicsComponent() : CBaseComponent()
{
	SetType( CRC_SKATERADJUSTPHYSICS );
	
	mp_core_physics_component = NULL;
	mp_model_component = NULL;
	mp_shadow_component = NULL;
	mp_movable_contact_component = NULL;
	
	m_uber_frigged_this_frame = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterAdjustPhysicsComponent::~CSkaterAdjustPhysicsComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterAdjustPhysicsComponent::InitFromStructure( Script::CStruct* pParams )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterAdjustPhysicsComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterAdjustPhysicsComponent::Finalize (   )
{
	mp_core_physics_component = GetSkaterCorePhysicsComponentFromObject(GetObject());
	mp_model_component = GetModelComponentFromObject(GetObject());
	mp_shadow_component = GetShadowComponentFromObject(GetObject());
	mp_movable_contact_component = GetMovableContactComponentFromObject(GetObject());
	mp_state_component = GetSkaterStateComponentFromObject(GetObject());

	Dbg_Assert(mp_core_physics_component);
	Dbg_Assert(mp_model_component);
	Dbg_Assert(mp_shadow_component);
	Dbg_Assert(mp_movable_contact_component);
	Dbg_Assert(mp_state_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterAdjustPhysicsComponent::Update()
{
	// If on the ground, or moving downwards, then allow us to hit a car again		
	if (mp_core_physics_component->GetState() == GROUND || GetObject()->m_vel[Y] < 0.0f)
	{
		mp_core_physics_component->SetFlagTrue(CAN_HIT_CAR);
	}

	if (!mp_core_physics_component->GetFlag(SKITCHING))
	{
		check_inside_objects();
	}

	uber_frig();

	GetObject()->m_old_pos = GetObject()->m_pos;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterAdjustPhysicsComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
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

void CSkaterAdjustPhysicsComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterAdjustPhysicsComponent::GetDebugInfo"));
	
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
					
void CSkaterAdjustPhysicsComponent::uber_frig (   )
{
	// Uberfrig() will attempt to see if we are in a region with valid collision
	// if were are not, then it will attempt to move us back to a position that is valid; generally this is the old position
	
	m_uber_frigged_this_frame = false;

	if (GetObject()->m_pos != mp_core_physics_component->m_safe_pos)
	{		
		#ifdef	__NOPT_ASSERT__
		if (! (Tmr::GetRenderFrame() & 15) )
		{
			TrackingLine2(4, GetObject()->m_old_pos, GetObject()->m_pos);	  // 4 = line
		}
		#endif
		
		mp_core_physics_component->m_col_start = GetObject()->m_pos;
		mp_core_physics_component->m_col_end = GetObject()->m_pos;

		// Very minor adjustment to move origin away from vert walls
		mp_core_physics_component->m_col_start += GetObject()->m_matrix[Y] * 0.001f;
		
		mp_core_physics_component->m_col_start[Y] += 8.0f;
		mp_core_physics_component->m_col_end[Y] -= FEET(400);
			   
		if (mp_core_physics_component->get_member_feeler_collision())
		{
			// if we are in "contact" with something, and on the ground or in air, then that should be what we collide with
			// so if not, we need to leave contact, and impart the final velocity
			// ("on ground" just means skating on something, which could be the moving object
			if (!mp_core_physics_component->GetFlag(SKITCHING) && !mp_core_physics_component->GetFlag(SPINE_PHYSICS))
			{
				// but we leave it if skitching, or attempting to spin between them
				if (mp_movable_contact_component->HaveContact()
					&& (mp_core_physics_component->GetState() == GROUND || mp_core_physics_component->GetState() == AIR))
				{
					if (!mp_core_physics_component->m_feeler.IsMovableCollision() 
						|| mp_core_physics_component->m_feeler.GetMovingObject() != mp_movable_contact_component->GetContact()->GetObject())
					{
						GetObject()->m_vel += mp_movable_contact_component->GetContact()->GetObject()->GetVel();
						DUMP_VELOCITY;
						mp_movable_contact_component->LoseAnyContact();
					}
				}
			}
			
			static Mth::Vector xa, xb, xc;

			float height = GetObject()->m_pos[Y] - mp_core_physics_component->m_feeler.GetPoint()[Y];

			mp_state_component->m_height = height;
			
			// if we are below the ground, then move him up
			if (height < 0.001f)
			{
				GetObject()->m_pos[Y] = mp_core_physics_component->m_feeler.GetPoint()[Y] + 0.001f; 	// above ground by a fraction of an inch
				DUMP_POSITION;
				height = 0.0f;
			}

			if (mp_core_physics_component->GetState() != RAIL)
			{
				mp_model_component->ApplyLightingFromCollision(mp_core_physics_component->m_feeler);
			}
			
			// Store these values off for the simple shadow calculation.
			mp_shadow_component->SetShadowPos(mp_core_physics_component->m_feeler.GetPoint());
			mp_shadow_component->SetShadowNormal(mp_core_physics_component->m_feeler.GetNormal()); 
		
			if (Ed::CParkEditor::Instance()->UsingCustomPark())
			{
				// when we are in the park editor, there are other checks we might want to do
				CFeeler feeler;
				
				// ignore non-collidable, under-ok
				feeler.SetIgnore(mFD_NON_COLLIDABLE | mFD_UNDER_OK, 0);
				feeler.SetStart(GetObject()->m_pos + Mth::Vector(0.0f, 3000.0f, 0.0f, 0.0f));
				feeler.SetEnd(GetObject()->m_pos + GetObject()->m_matrix[Y]);
				if (feeler.GetCollision())
				{
					// Something above me that I'm not supposed to be under; just move the skater back to the old pos, and flip him
					
					// if only just under it, then pop me up
					if ((GetObject()->m_pos - feeler.GetPoint()).LengthSqr() < 6.0f * 6.0f)
					{
						GetObject()->m_pos = feeler.GetPoint();
						DUMP_POSITION;
					}
					else
					{
						// if we are not moving, then pop us up above the face we detected a collision with
						if ((GetObject()->m_pos - mp_core_physics_component->m_safe_pos).LengthSqr() < 1.0f * 1.0f)
						{
							GetObject()->m_pos = feeler.GetPoint();
							DUMP_POSITION;
						}
						else
						{
							GetObject()->m_pos = mp_core_physics_component->m_safe_pos;
							DUMP_POSITION;
						}
						GetObject()->m_vel = GetObject()->m_vel * (-1.0f / 2.0f);
						DUMP_VELOCITY;
						if (mp_core_physics_component->GetState() == RAIL)
						{
							mp_core_physics_component->SetState(AIR);			// Stop grinding
							GetObject()->SelfEvent(CRCD(0xafaa46ba, "OffRail"));					// regular exception
							GetObject()->m_pos[Y] += 2.0f;								// make sure we are out of the rail...
							DUMP_POSITION;
						}
					}
				}
			}
			
			m_nudge = 0;	
			
			 // Mick:  Set a safe position.  Here there was a collision, so safe
			mp_core_physics_component->m_safe_pos = GetObject()->m_pos;
			
		}
		else
		{
			// I'm going to assume that if we are on a rail, then the uberfrig is just a problem with the collision getting parallel to the wall		
			// so I can safely ignore it, as no rail should be laid out into nothingness
			// except in the park editor, dammit...
			if ((mp_core_physics_component->GetState() != RAIL && mp_core_physics_component->GetState() != LIP)
				|| Ed::CParkEditor::Instance()->UsingCustomPark())
			{
				m_uber_frigged_this_frame = true;
				
				// first nudge is zero
				static Mth::Vector uberfrig_nudges[] =
				{
					Mth::Vector(0.0f, 0.0f, 0.0f),
					Mth::Vector(0.1f, 0.1f, 0.1f),
					Mth::Vector(-0.2f, 0.0f, 0.0f),
					Mth::Vector(0.0f, 0.0f, -0.2f),
					Mth::Vector(0.2f, 0.0f, 0.0f)
				};
				
				if (m_nudge < 5)
				{
					GetObject()->m_pos = mp_core_physics_component->m_safe_pos + uberfrig_nudges[m_nudge++];
					DUMP_POSITION;
				}
				else
				{
					// just reset it, and hope for the best.  Probably we are just over a seam, and will get over it.
					m_nudge = 0;
				}
				
				GetObject()->m_vel = -GetObject()->m_vel + Mth::Vector(0.101f, 0.101f, 0.1001f);
				DUMP_VELOCITY;
				
				// special case for wallrides, lip tricks and suchlike
				// just change them to air state; bit of a patch, but prevent you getting stuck in a state													
				if (mp_core_physics_component->GetState() != GROUND && mp_core_physics_component->GetState() != AIR)
				{
					mp_core_physics_component->SetState(AIR);
				}
			}
			
			mp_state_component->m_height = 0.0f;
		}
	} 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterAdjustPhysicsComponent::check_inside_objects (   )
{
	if (mp_movable_contact_component->CheckInsideObjects(GetObject()->m_pos, mp_core_physics_component->m_safe_pos))
	{
		MESSAGE("found to be inside a moving object");
		
		// Whichever way we rcover from being inside a moving object, this abrupt change in velocity and position means any rail grinding might
		// be messed up, either with zero velocity, or being moved away from the rail; so, the best thing to do is skate off the rail

		if (mp_core_physics_component->GetState() == RAIL)
		{
			mp_core_physics_component->SetState(AIR);
			GetObject()->SelfEvent(CRCD(0xafaa46ba, "OffRail"));
		}
		else if (GetObject()->m_pos == mp_core_physics_component->m_safe_pos)
		{
			// Set velocity to zero, otherwise we can get stuck inside things as the velocity from this frame will keep pushing you back inside things
			// especially if m_safe_pos is just inside the object, when there will be no collision detected
			// If we are on a rail, let them keep the velocity as maybe we can handle it next frame
			GetObject()->GetVel().Set();
		}
	}
}

}
