//****************************************************************************
//* MODULE:         sk/Components
//* FILENAME:       SkaterFinalizePhysicsComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/26/3
//****************************************************************************

#include <sk/components/skaterfinalizephysicscomponent.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/components/skaterstatecomponent.h>
#include <sk/components/skaterloopingsoundcomponent.h>
#include <sk/components/skatersoundcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
				  
namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent* CSkaterFinalizePhysicsComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterFinalizePhysicsComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterFinalizePhysicsComponent::CSkaterFinalizePhysicsComponent() : CBaseComponent()
{
	SetType( CRC_SKATERFINALIZEPHYSICS );

	mp_core_physics_component = NULL;
	mp_state_component = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterFinalizePhysicsComponent::~CSkaterFinalizePhysicsComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterFinalizePhysicsComponent::InitFromStructure( Script::CStruct* pParams )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterFinalizePhysicsComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterFinalizePhysicsComponent::Finalize (   )
{
	mp_core_physics_component = GetSkaterCorePhysicsComponentFromObject(GetObject());
	mp_state_component = GetSkaterStateComponentFromObject(GetObject());
		
	Dbg_Assert(mp_core_physics_component);
	Dbg_Assert(mp_state_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterFinalizePhysicsComponent::Update()
{
	// setup the state component
	
	// Logic for setting/not setting the flag for telling the camera whether to look down on the skater or not.
	if (mp_core_physics_component->m_began_frame_in_lip_state && mp_state_component->GetState() != LIP)
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
	
	mp_state_component->m_camera_display_normal = mp_core_physics_component->m_display_normal;
	mp_state_component->m_camera_current_normal = mp_core_physics_component->m_current_normal;
	
	// make sure the matrices don't get distorted
	GetObject()->m_matrix.OrthoNormalizeAbout(Y);
	mp_core_physics_component->m_lerping_display_matrix.OrthoNormalizeAbout(Y);
	
	// if any part of the matrix has collapsed, then we will neet to patch it up	
	// Note, this is a very rare occurence; probably only occurs when you hit things perfectly at right angles, so
	// you attempt to orthonormalize about an axis that that is now coincident with another axis
	// would not happen if we rotated the matrix, or used quaternions
	if (GetObject()->m_matrix.PatchOrthogonality())
	{
		mp_core_physics_component->ResetLerpingMatrix();
	}
	
	// Extract the informations from the physics object that we need for rendering
	GetObject()->SetDisplayMatrix(mp_core_physics_component->m_lerping_display_matrix);
	
	#ifdef __USER_DAN__
	// Gfx::AddDebugArrow(GetObject()->GetPos(), GetObject()->GetPos() + 60.0f * GetObject()->GetDisplayMatrix()[Z], RED, 0, 1);
	// Gfx::AddDebugArrow(GetObject()->GetPos(), GetObject()->GetPos() + 60.0f * GetObject()->GetDisplayMatrix()[X], BLUE, 0, 1);
	// Gfx::AddDebugArrow(GetObject()->GetPos(), GetObject()->GetPos() + 60.0f * GetObject()->GetDisplayMatrix()[Y], GREEN, 0, 1);
	#endif
	
	// update the sound components' state
	
	Obj::CSkaterSoundComponent *pSoundComponent = GetSkaterSoundComponentFromObject( GetObject() );
	Dbg_Assert( pSoundComponent );
	
	pSoundComponent->SetIsRailSliding( mp_state_component->GetFlag(RAIL_SLIDING) );
	pSoundComponent->SetTerrain( mp_state_component->GetTerrain() );
	
	Obj::CSkaterLoopingSoundComponent *pLoopingSoundComponent = GetSkaterLoopingSoundComponentFromObject( GetObject() );
	Dbg_Assert( pLoopingSoundComponent );
	
	float speed_fraction = sqrtf( GetObject()->GetVel()[X] * GetObject()->GetVel()[X] + GetObject()->GetVel()[Z] * GetObject()->GetVel()[Z] ) / GetSkater()->GetScriptedStat(CRCD(0xcc5f87aa, "Skater_Max_Max_Speed_Stat") );
	pLoopingSoundComponent->SetSpeedFraction( speed_fraction );
	pLoopingSoundComponent->SetIsBailing(mp_state_component->GetFlag(IS_BAILING));
	pLoopingSoundComponent->SetIsRailSliding( mp_state_component->GetFlag(RAIL_SLIDING) );
	pLoopingSoundComponent->SetTerrain( mp_state_component->GetTerrain() );
	pLoopingSoundComponent->SetState( mp_state_component->GetState() );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterFinalizePhysicsComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
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

void CSkaterFinalizePhysicsComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterFinalizePhysicsComponent::GetDebugInfo"));
	
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}
	
}
