//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       SubAnimController.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  2/06/2003
//****************************************************************************

#include <gfx/subanimcontroller.h>

#include <core/math.h>

#include <gel/components/animationcomponent.h>
#include <gel/object/compositeobject.h>
										   
#include <gel/scripting/array.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>
						 
#include <gfx/blendchannel.h>
#include <gfx/gfxutils.h>
#include <gfx/nx.h>
#include <gfx/nxanimcache.h>
#include <gfx/nxquickanim.h>
#include <gfx/pose.h>
#include <gfx/skeleton.h>

#include <sk/components/skaterflipandrotatecomponent.h>
						  
namespace Gfx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBonedAnimController::CBonedAnimController( CBlendChannel* pBlendChannel ) : CBaseAnimController( pBlendChannel )
{
	mp_quickAnim = Nx::CEngine::sInitQuickAnim();
	mp_quickAnim->Enable( false );
	
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	m_animScriptName = pAnimationComponent->GetAnimScriptName();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBonedAnimController::~CBonedAnimController()
{
	if ( mp_quickAnim )
	{
		Nx::CEngine::sUninitQuickAnim( mp_quickAnim );
		mp_quickAnim = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CBonedAnimController::InitFromStructure( Script::CStruct* pParams )
{
	CBaseAnimController::InitFromStructure( pParams );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CBonedAnimController::GetPose( Gfx::CPose* pResultPose )
{
	Dbg_MsgAssert( pResultPose, ( "No return data" ) );

//	int skipList[vMAX_BONES];
//	GetSkeleton()->GetSkipList( &skipList[0] );
	uint32*	p_skip_list = GetSkeleton()->GetBoneSkipList();
	uint32	skip_index	= GetSkeleton()->GetBoneSkipIndex();

	Dbg_Assert( mp_quickAnim );
//	mp_quickAnim->GetInterpolatedFrames( pResultPose->m_rotations, pResultPose->m_translations, &skipList[0], mp_blendChannel->GetCurrentAnimTime() );
	mp_quickAnim->GetInterpolatedFrames( pResultPose->m_rotations, pResultPose->m_translations, p_skip_list, skip_index, mp_blendChannel->GetCurrentAnimTime() );
	mp_quickAnim->Enable( true );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CBonedAnimController::Update()
{
	Dbg_MsgAssert( mp_blendChannel->m_loopingType != Gfx::LOOPING_WOBBLE, ( "Not supposed to be wobble type" ) );
			
	float oldTime = mp_blendChannel->m_currentTime;

	mp_blendChannel->m_currentTime = mp_blendChannel->get_new_anim_time();

	mp_blendChannel->ProcessCustomKeys( oldTime, mp_blendChannel->m_currentTime );

	if ( mp_blendChannel->m_currentTime < oldTime )
	{
		mp_quickAnim->Enable( false );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

EAnimFunctionResult CBonedAnimController::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		case 0x5f495ae0:	// invalidateCache
		{
			mp_quickAnim->Enable( false );
			
			// want the other controllers to be invalidated too
			return AF_NOT_EXECUTED;
		}
		break;

		case 0xaf2fae19:	// playsequence
		{
			uint32 anim_name;
			float start_time;
			float end_time;
			int loop_type;
			float blend_period;
			float speed;

			pParams->GetChecksum( CRCD(0x6c2bfb7f,"animname"), &anim_name, Script::ASSERT );
			pParams->GetFloat( CRCD(0xd16b61e6,"starttime"), &start_time, Script::ASSERT );
			pParams->GetFloat( CRCD(0xab81bb3d,"endtime"), &end_time, Script::ASSERT );
			pParams->GetInteger( CRCD(0xcd8d7c1b,"looptype"), &loop_type, Script::ASSERT );
			pParams->GetFloat( CRCD(0x8f0d24ed,"blendperiod"), &blend_period, Script::ASSERT );
			pParams->GetFloat( CRCD(0xf0d90109,"speed"), &speed, Script::ASSERT );
			
			mp_blendChannel->CAnimChannel::PlaySequence( anim_name, start_time, end_time, (Gfx::EAnimLoopingType)loop_type, blend_period, speed);
		
			Dbg_Assert( mp_quickAnim );
			mp_quickAnim->SetAnimAssetName( m_animScriptName + anim_name );
		}
		break;
			
		default:
			return AF_NOT_EXECUTED;
	}

	return AF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CWobbleController::CWobbleController( CBlendChannel* pBlendChannel ) : CBaseAnimController( pBlendChannel )
{
	m_wobbleTargetTime			 = 0.0f;

	m_wobbleDetails.wobbleAmpA    = 0.0f;
	m_wobbleDetails.wobbleAmpB    = 0.0f;
	m_wobbleDetails.wobbleK1      = 0.0f;
	m_wobbleDetails.wobbleK2      = 0.0f;
	m_wobbleDetails.spazFactor    = 0.0f;	
	
	mp_quickAnim = Nx::CEngine::sInitQuickAnim();
	mp_quickAnim->Enable( false );
	
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	m_animScriptName = pAnimationComponent->GetAnimScriptName();
	
	/*
	float startTime;
	float endTime;

	if ( pParams->GetFloat( "startTime", &startTime, Script::NO_ASSERT );
		&& pParams->GetFloat( "endTime", &endTime, Script::NO_ASSERT ) )
	{
		m_wobbleTargetTime = ( startTime + endTime ) / 2;
	}
	*/
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CWobbleController::~CWobbleController()
{
	if ( mp_quickAnim )
	{
		Nx::CEngine::sUninitQuickAnim( mp_quickAnim );
		mp_quickAnim = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWobbleController::InitFromStructure( Script::CStruct* pParams )
{
	CBaseAnimController::InitFromStructure( pParams );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CWobbleController::GetPose( Gfx::CPose* pResultPose )
{
	Dbg_MsgAssert( pResultPose, ( "No return data" ) );

//	int skipList[vMAX_BONES];
//	GetSkeleton()->GetSkipList( &skipList[0] );
	uint32*	p_skip_list = GetSkeleton()->GetBoneSkipList();
	uint32	skip_index	= GetSkeleton()->GetBoneSkipIndex();
	
	Dbg_Assert( mp_quickAnim );
//	mp_quickAnim->GetInterpolatedFrames( pResultPose->m_rotations, pResultPose->m_translations, &skipList[0], mp_blendChannel->GetCurrentAnimTime() );
	mp_quickAnim->GetInterpolatedFrames( pResultPose->m_rotations, pResultPose->m_translations, p_skip_list, skip_index, mp_blendChannel->GetCurrentAnimTime() );
	mp_quickAnim->Enable( true );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float CWobbleController::get_new_wobble_time()
{
	float AmpA = m_wobbleDetails.wobbleAmpA;
	float AmpB = m_wobbleDetails.wobbleAmpB;
	float k1 = m_wobbleDetails.wobbleK1;
	float k2 = m_wobbleDetails.wobbleK2;
	float SpazFactor = m_wobbleDetails.spazFactor;

	float new_time = m_wobbleTargetTime;

	Dbg_MsgAssert( mp_blendChannel->m_startTime < mp_blendChannel->m_endTime, ("start time must be smaller than end time for LOOPING_WOBBLE"));
	float l = ( mp_blendChannel->m_endTime - mp_blendChannel->m_startTime ) / 2;
	float mid = ( mp_blendChannel->m_endTime + mp_blendChannel->m_startTime ) / 2;
	float d = Mth::Abs( new_time - mid );
	SpazFactor = 1 + ( SpazFactor - 1 ) * d / l;
	AmpA *= SpazFactor;
	AmpB *= SpazFactor;

	if ( new_time - AmpA - AmpB < mp_blendChannel->m_startTime )
	{
		new_time = mp_blendChannel->m_startTime + AmpA + AmpB;
	}	
	if ( new_time + AmpA + AmpB > mp_blendChannel->m_endTime )
	{
		new_time = mp_blendChannel->m_endTime - AmpA - AmpB;
	}	

	float t = (int)Tmr::ElapsedTime( 0 ) * k1;
	t -= 16.0f * ( ((int)t) / 16.0f );
	float amp = AmpA + AmpB * sinf( t * 2.0f * Mth::PI );

	t = (int)Tmr::ElapsedTime( 0 ) * k2;
	t -= 16.0f * ( ((int)t) / 16.0f );

	new_time += amp * sinf( t * 2.0f * Mth::PI );

	// These should never be true, but just in case.
	if (new_time < mp_blendChannel->m_startTime)
	{
		new_time = mp_blendChannel->m_startTime;
	}
	if (new_time > mp_blendChannel->m_endTime)
	{
		new_time = mp_blendChannel->m_endTime;
	}

	return new_time;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWobbleController::Update()
{
	// the looping check below is only needed until
	// we stop automatically adding the wobble
	// controller to all channels
	Dbg_MsgAssert ( mp_blendChannel->m_loopingType == Gfx::LOOPING_WOBBLE, ( "Was supposed to be wobble type %d", mp_blendChannel->m_loopingType ) );

	// remember old time, so that we know whether to use the quick anim
	float oldTime = mp_blendChannel->m_currentTime;

	mp_blendChannel->m_currentTime = get_new_wobble_time();

	// remember to use the quick anim
	if ( mp_blendChannel->m_currentTime < oldTime )
	{
		mp_quickAnim->Enable( false );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWobbleController::GetDebugInfo( Script::CStruct* p_info )
{
#ifdef	__DEBUG_CODE__
	p_info->AddFloat( CRCD(0x359ac119,"m_wobbleTargetTime"), m_wobbleTargetTime );

	p_info->AddFloat( CRCD(0xfd266a26,"wobbleAmpA"), m_wobbleDetails.wobbleAmpA );
	p_info->AddFloat( CRCD(0x642f3b9c,"wobbleAmpB"), m_wobbleDetails.wobbleAmpB );
	p_info->AddFloat( CRCD(0x0f43fd49,"wobbleK1"), m_wobbleDetails.wobbleK1 );
	p_info->AddFloat( CRCD(0x964aacf3,"wobbleK2"), m_wobbleDetails.wobbleK2 );
	p_info->AddFloat( CRCD(0xf90b0824,"spazFactor"), m_wobbleDetails.spazFactor );
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

EAnimFunctionResult CWobbleController::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		case 0x5f495ae0:	// invalidateCache
		{
			mp_quickAnim->Enable( false );
			
			// want the other controllers to be invalidated too
			return AF_NOT_EXECUTED;
		}
		break;

		case 0xaf2fae19:	// playsequence
		{
			uint32 anim_name;
			float start_time;
			float end_time;
			int loop_type;
			float blend_period;
			float speed;

			pParams->GetChecksum( CRCD(0x6c2bfb7f,"animname"), &anim_name, Script::ASSERT );
			pParams->GetFloat( CRCD(0xd16b61e6,"starttime"), &start_time, Script::ASSERT );
			pParams->GetFloat( CRCD(0xab81bb3d,"endtime"), &end_time, Script::ASSERT );
			pParams->GetInteger( CRCD(0xcd8d7c1b,"looptype"), &loop_type, Script::ASSERT );
			pParams->GetFloat( CRCD(0x8f0d24ed,"blendperiod"), &blend_period, Script::ASSERT );
			pParams->GetFloat( CRCD(0xf0d90109,"speed"), &speed, Script::ASSERT );
			
			mp_blendChannel->CAnimChannel::PlaySequence( anim_name, start_time, end_time, (Gfx::EAnimLoopingType)loop_type, blend_period, speed);
		
			Dbg_Assert( mp_quickAnim );
			mp_quickAnim->SetAnimAssetName( m_animScriptName + anim_name );
		}
		break;
			
		case 0xd0209498:	// setwobbletarget
		{
			float wobbleTargetAlpha;

			if ( pParams->GetFloat( CRCD(0x4d747fa0,"wobbletargetalpha"), &wobbleTargetAlpha, Script::NO_ASSERT ) )
			{
				// check bounds
				if ( wobbleTargetAlpha < 0.0f )
				{
					wobbleTargetAlpha = 0.0f;
				}

				if ( wobbleTargetAlpha > 1.0f )
				{
					wobbleTargetAlpha = 1.0f;
				}

				m_wobbleTargetTime = mp_blendChannel->m_startTime + ( mp_blendChannel->m_endTime - mp_blendChannel->m_startTime ) * wobbleTargetAlpha;
			}
		}
		break;
			
		case 0xea6d0efd:	// setwobbledetails
		{
			float wobbleAmpA;
			float wobbleAmpB;
			float wobbleK1;
			float wobbleK2;
			float spazFactor;

			if ( pParams->GetFloat( CRCD(0xfd266a26,"wobbleAmpA"), &wobbleAmpA, Script::NO_ASSERT )
				 && pParams->GetFloat( CRCD(0x642f3b9c,"wobbleAmpB"), &wobbleAmpB, Script::NO_ASSERT )
				 && pParams->GetFloat( CRCD(0x0f43fd49,"wobbleK1"), &wobbleK1, Script::NO_ASSERT )
				 && pParams->GetFloat( CRCD(0x964aacf3,"wobbleK2"), &wobbleK2, Script::NO_ASSERT )
				 && pParams->GetFloat( CRCD(0xf90b0824,"spazFactor"), &spazFactor, Script::NO_ASSERT ) )
			{
				m_wobbleDetails.wobbleAmpA = wobbleAmpA;
				m_wobbleDetails.wobbleAmpB = wobbleAmpB;
				m_wobbleDetails.wobbleK1 = wobbleK1;
				m_wobbleDetails.wobbleK2 = wobbleK2;
				m_wobbleDetails.spazFactor = spazFactor;
			}
		}
		break;

		default:
			return AF_NOT_EXECUTED;
	}

	return AF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CFlipRotateController::CFlipRotateController( CBlendChannel* pBlendChannel ) : CBaseAnimController( pBlendChannel )
{
	m_flipped = false;
	m_rotated = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CFlipRotateController::~CFlipRotateController()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CFlipRotateController::InitFromStructure( Script::CStruct* pParams )
{
	CBaseAnimController::InitFromStructure( pParams );

	int flipped;
	if ( pParams->GetInteger( CRCD(0x0c7a712c,"flipped"), &flipped, Script::NO_ASSERT ) )
	{
		m_flipped = flipped;
	}

	int rotated;
	if ( pParams->GetInteger( CRCD(0x6ea3704a,"rotated"), &rotated, Script::NO_ASSERT ) )
	{
		m_rotated = rotated;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CFlipRotateController::Update()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CFlipRotateController::GetPose( Gfx::CPose* pResultPose )
{
	// rather than using the m_flipped and m_rotated flags,
	// we should use the one stored in the CAnimationComponent
	// temporarily...  this is because right now we don't want 
	// to blend across the flip (but eventually we will want to)

	CSkeleton* pSkeleton = GetSkeleton();
	Dbg_MsgAssert( pSkeleton, ( "No skeleton?" ) );

	#ifdef	__NOPT_ASSERT__
	{	// remove this scope if you actually want to use pAnimationComponent
		// in something other than the assertion
		Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
		Dbg_MsgAssert( pAnimationComponent, ( "No animation component?" ) );
	}
	#endif
	
	Obj::CSkaterFlipAndRotateComponent* pSkaterFlipAndRotateComponent = GetSkaterFlipAndRotateComponentFromObject( GetObject() );

	if ( pSkaterFlipAndRotateComponent && pSkaterFlipAndRotateComponent->IsBoardRotated() )
//	if ( m_rotated )
	{
		int idx;
		
		// name of skateboard bone in new skeleton
		idx = pSkeleton->GetBoneIndexById( CRCD(0x98971faf, "bone_board_root") );
		if ( idx != -1 )
		{	   
			Mth::Quat* pQuat = (pResultPose->m_rotations + idx);
			*pQuat = Mth::Quat(0.0f,0.0f,1.0f,0.0f) * *pQuat;
			pQuat->Normalize();
		}		
	}

	// GJ:  this remembers the flip state when the animation was
	// first launched, so that we can blend across the flip...  
	// it produces some weird effects currently in the THPS5, 
	// (such as the pelvis rotating 180 degrees), but this may 
	// be due to the way the scripts were originally written...
	// i'll take a look at it later, but hopefully this will 
	// give Left Field some of the functionality they've been
	// looking for...
	if ( m_flipped )
//	if ( pAnimationComponent->IsFlipped() )
	{
		int i;
		Gfx::CPose oldPose;
		int numBones = pSkeleton->GetNumBones();

//		oldPose = *pResultPose;
		for ( i = 0; i < numBones; i++ )
		{
			oldPose.m_rotations[i] = pResultPose->m_rotations[i];
		}	
		for ( i = 0; i < numBones; i++ )
		{
			oldPose.m_translations[i] = pResultPose->m_translations[i];
		}	

		Mth::Quat* pQuat = (pResultPose->m_rotations);
		Mth::Vector* pTrans = (pResultPose->m_translations);

		for ( i = 0; i < numBones; i++ )
		{
			int flipIndex = pSkeleton->GetFlipIndex(i);	
			if ( flipIndex != -1 )
			{
				*pQuat = *(oldPose.m_rotations+flipIndex);
				*pTrans = *(oldPose.m_translations+flipIndex);				
	   		}
			else
			{
				*pQuat = *(oldPose.m_rotations+i);
				*pTrans = *(oldPose.m_translations+i);
			}
			
			// flip all rotations and translations on Y.
			(*pQuat)[X] = -(*pQuat)[X];
			(*pQuat)[W] = -(*pQuat)[W];
			(*pTrans)[X] = -(*pTrans)[X];

			pQuat++;
			pTrans++;
		}
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CIKController::CIKController( CBlendChannel* pBlendChannel ) : CBaseAnimController( pBlendChannel )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CIKController::~CIKController()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CIKController::InitFromStructure( Script::CStruct* pParams )
{
	CBaseAnimController::InitFromStructure( pParams );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CIKController::Update()
{
	// here we'd do things like grab the position of the bike pedals
	// from the CBikeComponent, and override the leg bones, or whatever

	// if data needs be shared among different
	// blend channels, then we might want to consider
	// storing it in a CIKComponent...  then, the
	// Update() function can grab it like so:
	// Obj::CIKComponent* pIKComponent = GetIKComponentFromObject( GetObject() );
	// if ( pIKComponent )
	// {
	//		Mth::Vector steering_column_pos = pIKComponent->GetBonePosition( "steering_column" );
	//		Mth::Vector left_handle_bar_pos	= pIKComponent->GetBonePosition( "left_handle_bar" );
	// }

	// if each blend channel needs its own set of IK data
	// then we can just do it like so:
	// Mth::Vector steering_column_pos = m_steering_column_pos;
	// Mth::Vector left_handle_bar_pos = m_left_handle_bar_pos;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CIKController::GetDebugInfo( Script::CStruct* p_info )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

EAnimFunctionResult CIKController::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		default:
			return AF_NOT_EXECUTED;
	}

	return AF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CIKController::GetPose( Gfx::CPose* pResultPose )
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CProceduralAnimController::CProceduralAnimController( CBlendChannel* pBlendChannel ) : CBaseAnimController( pBlendChannel )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CProceduralAnimController::~CProceduralAnimController()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CProceduralAnimController::InitFromStructure( Script::CStruct* pParams )
{
	CBaseAnimController::InitFromStructure( pParams );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CProceduralAnimController::Update()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CProceduralAnimController::GetPose( Gfx::CPose* pResultPose )
{
	// To be consistent with THPS4, we should actually do this 
	// AFTER building the object-space transform of each bone
	// (but i'll wait until someone actually asks for this feature)
	
	// I didn't fully re-implement this functionality from THPS4,
	// since we're going to do our procedural cloth animations
	// in a different way for THPS5.  I've left this partial
	// implementation in as an example for other developers
	// to write their own controllers.

	// THPS4 had ping-ponging animations (so that we can animate
	// the shirt rising and falling)...  we can re-implement
	// this later if we end up deciding to use this controller...

	CSkeleton* pSkeleton = GetSkeleton();
	Dbg_MsgAssert( pSkeleton, ( "No skeleton?" ) );

	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	Dbg_MsgAssert( pAnimationComponent, ( "No animation component?" ) );
	
	int numProceduralBones = pAnimationComponent->GetNumProceduralBones(); 
	CProceduralBone* p_current_procedural_bone = pAnimationComponent->GetProceduralBones();

	for ( int i = 0; i < numProceduralBones; i++ )
	{
		int idx = pSkeleton->GetBoneIndexById( p_current_procedural_bone->m_name );
		if ( idx != -1 )
		{
			Mth::Quat* pQuat = pResultPose->m_rotations + idx;

			if ( p_current_procedural_bone->rotEnabled )
			{
				Mth::Vector theRotVector = p_current_procedural_bone->currentRot;
				theRotVector.Scale( Mth::PI * 2.0f / 4096.0f );
				Mth::Quat theRotQuat = Mth::EulerToQuat( theRotVector );
				
				// TODO:  should I be post-rotating this?
				*pQuat = theRotQuat * *pQuat;
				
				pQuat->Normalize();
			}
		}
											  
		p_current_procedural_bone++;
    }

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CProceduralAnimController::SetProceduralBoneTransMin( uint32 boneName, const Mth::Vector& vec )
{
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	Dbg_MsgAssert( pAnimationComponent, ( "No animation component?" ) );
	
	CProceduralBone* pProceduralBone = pAnimationComponent->GetProceduralBoneByName( boneName );

	if ( pProceduralBone )
	{
		pProceduralBone->trans0 = vec;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CProceduralAnimController::SetProceduralBoneTransMax( uint32 boneName, const Mth::Vector& vec )
{
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	Dbg_MsgAssert( pAnimationComponent, ( "No animation component?" ) );
	
	CProceduralBone* pProceduralBone = pAnimationComponent->GetProceduralBoneByName( boneName );

	if ( pProceduralBone )
	{
		pProceduralBone->trans1 = vec;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CProceduralAnimController::SetProceduralBoneTransSpeed( uint32 boneName, const Mth::Vector& vec )
{
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	Dbg_MsgAssert( pAnimationComponent, ( "No animation component?" ) );
	
	CProceduralBone* pProceduralBone = pAnimationComponent->GetProceduralBoneByName( boneName );

	if ( pProceduralBone )
	{
		pProceduralBone->deltaTrans = vec;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CProceduralAnimController::SetProceduralBoneTransActive( uint32 boneName, bool enabled )
{
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	Dbg_MsgAssert( pAnimationComponent, ( "No animation component?" ) );
	
	CProceduralBone* pProceduralBone = pAnimationComponent->GetProceduralBoneByName( boneName );

	if ( pProceduralBone )
	{
		pProceduralBone->transEnabled = enabled;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CProceduralAnimController::SetProceduralBoneTransCurrent( uint32 boneName, const Mth::Vector& vec )
{
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	Dbg_MsgAssert( pAnimationComponent, ( "No animation component?" ) );
	
	CProceduralBone* pProceduralBone = pAnimationComponent->GetProceduralBoneByName( boneName );

	if ( pProceduralBone )
	{
		pProceduralBone->currentTrans = vec;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CProceduralAnimController::SetProceduralBoneRotMin( uint32 boneName, const Mth::Vector& vec )
{
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	Dbg_MsgAssert( pAnimationComponent, ( "No animation component?" ) );
	
	CProceduralBone* pProceduralBone = pAnimationComponent->GetProceduralBoneByName( boneName );

	if ( pProceduralBone )
	{
		pProceduralBone->rot0[X] = vec[X] * 2.0f * Mth::PI / 4096.0f;
		pProceduralBone->rot0[Y] = vec[Y] * 2.0f * Mth::PI / 4096.0f;
		pProceduralBone->rot0[Z] = vec[Z] * 2.0f * Mth::PI / 4096.0f;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CProceduralAnimController::SetProceduralBoneRotMax( uint32 boneName, const Mth::Vector& vec )
{
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	Dbg_MsgAssert( pAnimationComponent, ( "No animation component?" ) );
	
	CProceduralBone* pProceduralBone = pAnimationComponent->GetProceduralBoneByName( boneName );

	if ( pProceduralBone )
	{
		pProceduralBone->rot1[X] = vec[X] * 2.0f * Mth::PI / 4096.0f;
		pProceduralBone->rot1[Y] = vec[Y] * 2.0f * Mth::PI / 4096.0f;
		pProceduralBone->rot1[Z] = vec[Z] * 2.0f * Mth::PI / 4096.0f;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CProceduralAnimController::SetProceduralBoneRotSpeed( uint32 boneName, const Mth::Vector& vec )
{
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	Dbg_MsgAssert( pAnimationComponent, ( "No animation component?" ) );
	
	CProceduralBone* pProceduralBone = pAnimationComponent->GetProceduralBoneByName( boneName );

	if ( pProceduralBone )
	{
		pProceduralBone->deltaRot = vec;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CProceduralAnimController::SetProceduralBoneRotCurrent( uint32 boneName, const Mth::Vector& vec )
{
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	Dbg_MsgAssert( pAnimationComponent, ( "No animation component?" ) );
	
	CProceduralBone* pProceduralBone = pAnimationComponent->GetProceduralBoneByName( boneName );

	if ( pProceduralBone )
	{
		pProceduralBone->currentRot = vec;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CProceduralAnimController::SetProceduralBoneRotActive( uint32 boneName, bool enabled )
{
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	Dbg_MsgAssert( pAnimationComponent, ( "No animation component?" ) );
	
	CProceduralBone* pProceduralBone = pAnimationComponent->GetProceduralBoneByName( boneName );

	if ( pProceduralBone )
	{
		pProceduralBone->rotEnabled = enabled;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CProceduralAnimController::SetProceduralBoneScaleMin( uint32 boneName, const Mth::Vector& vec )
{
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	Dbg_MsgAssert( pAnimationComponent, ( "No animation component?" ) );
	
	CProceduralBone* pProceduralBone = pAnimationComponent->GetProceduralBoneByName( boneName );

	if ( pProceduralBone )
	{
		pProceduralBone->scale0 = vec;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CProceduralAnimController::SetProceduralBoneScaleMax( uint32 boneName, const Mth::Vector& vec )
{
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	Dbg_MsgAssert( pAnimationComponent, ( "No animation component?" ) );
	
	CProceduralBone* pProceduralBone = pAnimationComponent->GetProceduralBoneByName( boneName );

	if ( pProceduralBone )
	{
		pProceduralBone->scale1 = vec;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CProceduralAnimController::SetProceduralBoneScaleSpeed( uint32 boneName, const Mth::Vector& vec )
{
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	Dbg_MsgAssert( pAnimationComponent, ( "No animation component?" ) );
	
	CProceduralBone* pProceduralBone = pAnimationComponent->GetProceduralBoneByName( boneName );

	if ( pProceduralBone )
	{
		pProceduralBone->deltaScale = vec;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CProceduralAnimController::SetProceduralBoneScaleCurrent( uint32 boneName, const Mth::Vector& vec )
{
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	Dbg_MsgAssert( pAnimationComponent, ( "No animation component?" ) );
	
	CProceduralBone* pProceduralBone = pAnimationComponent->GetProceduralBoneByName( boneName );

	if ( pProceduralBone )
	{
		pProceduralBone->currentScale = vec;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CProceduralAnimController::SetProceduralBoneScaleActive( uint32 boneName, bool enabled )
{
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	Dbg_MsgAssert( pAnimationComponent, ( "No animation component?" ) );
	
	CProceduralBone* pProceduralBone = pAnimationComponent->GetProceduralBoneByName( boneName );

	if ( pProceduralBone )
	{
		pProceduralBone->scaleEnabled = enabled;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// TODO:  It's kind of backwards for the CAnimationComponent to be calling
// member functions in the CProceduralAnimController that set values
// back inside the CAnimationComponent...  This is because I haven't
// really decided whether each blend channel should have its own
// proc anim data, or whether the blend channels should share a single
// set of proc anim data.  Right now, the blend channels all share
// a single set of proc anim data (stored in the CAnimationComponent).

EAnimFunctionResult CProceduralAnimController::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
	    // @script | SetBoneTransMin | Sets the minimum translation extent for procedurally animated bone (in world units)
        // @parmopt float | x | 0 | minimum extent x
        // @parmopt float | y | 0 | minimum extent y
        // @parmopt float | z | 0 | minimum extent z
        // @parm name | bone | name of procedural bone
		case 0x02565bd1: //	SetBoneTransMin
			{
				Mth::Vector vec(0.0f,0.0f,0.0f);
				uint32 boneName;
				pParams->GetFloat( CRCD(0x7323e97c,"x"), &vec[X], false );
				pParams->GetFloat( CRCD(0x0424d9ea,"y"), &vec[Y], false );
				pParams->GetFloat( CRCD(0x9d2d8850,"z"), &vec[Z], false );
				pParams->GetChecksum( CRCD(0xcab94088,"bone"), &boneName, true );
				
				this->SetProceduralBoneTransMin( boneName, vec );
			}
			break;
			
		// @script | SetBoneTransMax | Sets the maximum translation extent for procedurally animated bone (in world units)
		// @parmopt float | x | 0 | maximum extent x
		// @parmopt float | y | 0 | maximum extent y
		// @parmopt float | z | 0 | maximum extent z
		// @parm name | bone | name of procedural bone
		case 0x3e5b6488: //	SetBoneTransMax
			{
				Mth::Vector vec(0.0f,0.0f,0.0f);
				uint32 boneName;
				pParams->GetFloat( CRCD(0x7323e97c,"x"), &vec[X], false );
				pParams->GetFloat( CRCD(0x0424d9ea,"y"), &vec[Y], false );
				pParams->GetFloat( CRCD(0x9d2d8850,"z"), &vec[Z], false );
				pParams->GetChecksum( CRCD(0xcab94088,"bone"), &boneName, true );
				
				this->SetProceduralBoneTransMax( boneName, vec );
			}
			break;
		
		// @script | SetBoneTransSpeed | Sets the speed at which the bone's translation will procedurally animate (in 4096 notation)
		// @parmopt int | x | 0 | speed x (2048 immediately goes from T0 to T1)
		// @parmopt int | y | 0 | speed y (2048 immediately goes from T0 to T1)
		// @parmopt int | z | 0 | speed z (2048 immediately goes from T0 to T1)
		// @parm name | bone | name of procedural bone
		case 0xc85b9ac0: //	SetBoneTransSpeed
			{
				Mth::Vector vec(0.0f,0.0f,0.0f);
				uint32 boneName;
				pParams->GetFloat( CRCD(0x7323e97c,"x"), &vec[X], false );
				pParams->GetFloat( CRCD(0x0424d9ea,"y"), &vec[Y], false );
				pParams->GetFloat( CRCD(0x9d2d8850,"z"), &vec[Z], false );
				pParams->GetChecksum( CRCD(0xcab94088,"bone"), &boneName, true );
				
				Dbg_Assert( mp_blendChannel );
				this->SetProceduralBoneTransSpeed( boneName, vec );
			}
			break;
			
		// @script | SetBoneTransCurrent | Immediately sets the procedurally animated bone to a specific translation (in 4096 notation)
		// @parmopt int | x | 0 | current x (0 = T0, 1024 = halfway between T0 and T1, 2048 = T1, 3072 = halfway between T1 and T0, 4096 = T0)
		// @parmopt int | y | 0 | current y (0 = T0, 1024 = halfway between T0 and T1, 2048 = T1, 3072 = halfway between T1 and T0, 4096 = T0)
		// @parmopt int | z | 0 | current z (0 = T0, 1024 = halfway between T0 and T1, 2048 = T1, 3072 = halfway between T1 and T0, 4096 = T0)
		// @parm name | bone | name of procedural bone
		case 0x5ee9d5bd: //	SetBoneTransCurrent
			{
				Mth::Vector vec(0.0f,0.0f,0.0f);
				uint32 boneName;
				pParams->GetFloat( CRCD(0x7323e97c,"x"), &vec[X], false );
				pParams->GetFloat( CRCD(0x0424d9ea,"y"), &vec[Y], false );
				pParams->GetFloat( CRCD(0x9d2d8850,"z"), &vec[Z], false );
				pParams->GetChecksum( CRCD(0xcab94088,"bone"), &boneName, true );
				
				this->SetProceduralBoneTransCurrent( boneName, vec );
			}
			break;
			
		// @script | SetBoneTransActive | Sets whether the bone should procedurally animate its translation
		// @uparm int | active (either 0 or 1)
		// @parm name | bone | name of procedural bone
		case 0x5661fb72: //	SetBoneTransActive
			{
				uint32 boneName;
				int enabled;
				pParams->GetChecksum( CRCD(0xcab94088,"bone"), &boneName, true );
				pParams->GetInteger( NONAME, &enabled, true );
				
                Dbg_Assert( mp_blendChannel );
                this->SetProceduralBoneTransActive( boneName, enabled );
			}
			break;
		
		// @script | SetBoneRotMin | Sets the minimum rotation extent for procedurally animated bone (in 4096 notation)
		// @parmopt int | x | 0 | minimum extent x (in 4096 notation)
		// @parmopt int | y | 0 | minimum extent y (in 4096 notation)
		// @parmopt int | z | 0 | minimum extent z (in 4096 notation)
		// @parm name | bone | name of procedural bone
		case 0xa47767f2: //	SetBoneRotMin
			{
				Mth::Vector vec(0.0f,0.0f,0.0f);
				uint32 boneName;
				pParams->GetFloat( CRCD(0x7323e97c,"x"), &vec[X], false );
				pParams->GetFloat( CRCD(0x0424d9ea,"y"), &vec[Y], false );
				pParams->GetFloat( CRCD(0x9d2d8850,"z"), &vec[Z], false );
				pParams->GetChecksum( CRCD(0xcab94088,"bone"), &boneName, true );
				
				this->SetProceduralBoneRotMin( boneName, vec );
			}
			break;
			
		// @script | SetBoneRotMax | Sets the maximum rotation extent for procedurally animated bone (in 4096 notation)
		// @parmopt int | x | 0 | maximum extent x (in 4096 notation)
		// @parmopt int | y | 0 | maximum extent y (in 4096 notation)
		// @parmopt int | z | 0 | maximum extent z (in 4096 notation)
		case 0x987a58ab: //	SetBoneRotMax
			{
				Mth::Vector vec(0.0f,0.0f,0.0f);
				uint32 boneName;
				pParams->GetFloat( CRCD(0x7323e97c,"x"), &vec[X], false );
				pParams->GetFloat( CRCD(0x0424d9ea,"y"), &vec[Y], false );
				pParams->GetFloat( CRCD(0x9d2d8850,"z"), &vec[Z], false );
				pParams->GetChecksum( CRCD(0xcab94088,"bone"), &boneName, true );
				
				this->SetProceduralBoneRotMax( boneName, vec );
			}
			break;
			
		// @script | SetBoneRotSpeed | Sets the speed at which the bone's rotation will procedurally animate (in 4096 notation)
		// @parmopt int | x | 0 | speed x (2048 immediately goes from T0 to T1)
		// @parmopt int | y | 0 | speed y (2048 immediately goes from T0 to T1)
		// @parmopt int | z | 0 | speed z (2048 immediately goes from T0 to T1)
		// @parm name | bone | name of procedural bone
		case 0x599d3707: //	SetBoneRotSpeed
			{
				Mth::Vector vec(0.0f,0.0f,0.0f);
				uint32 boneName;
				pParams->GetFloat( CRCD(0x7323e97c,"x"), &vec[X], false );
				pParams->GetFloat( CRCD(0x0424d9ea,"y"), &vec[Y], false );
				pParams->GetFloat( CRCD(0x9d2d8850,"z"), &vec[Z], false );
				pParams->GetChecksum( CRCD(0xcab94088,"bone"), &boneName, true );

				this->SetProceduralBoneRotSpeed( boneName, vec );
			}
			break;
			
		// @script | SetBoneRotCurrent | Immediately sets the procedurally animated bone to a specific rotation (in 4096 notation)
		// @parmopt int | x | 0 | current x (0 = T0, 1024 = halfway between T0 and T1, 2048 = T1, 3072 = halfway between T1 and T0, 4096 = T0)
		// @parmopt int | y | 0 | current y (0 = T0, 1024 = halfway between T0 and T1, 2048 = T1, 3072 = halfway between T1 and T0, 4096 = T0)
		// @parmopt int | z | 0 | current z (0 = T0, 1024 = halfway between T0 and T1, 2048 = T1, 3072 = halfway between T1 and T0, 4096 = T0)
		// @parm name | bone | name of procedural bone
		case 0x7235daa7: //	SetBoneRotCurrent
			{
				Mth::Vector vec(0.0f,0.0f,0.0f);
				uint32 boneName;
				pParams->GetFloat( CRCD(0x7323e97c,"x"), &vec[X], false );
				pParams->GetFloat( CRCD(0x0424d9ea,"y"), &vec[Y], false );
				pParams->GetFloat( CRCD(0x9d2d8850,"z"), &vec[Z], false );
				pParams->GetChecksum( CRCD(0xcab94088,"bone"), &boneName, true );
				
				this->SetProceduralBoneRotCurrent( boneName, vec );
			}
			break;
			
		// @script | SetBoneRotActive | Sets whether the bone should procedurally animate its rotation
		// @uparm int | active (either 0 or 1)
		// @parm name | bone | name of procedural bone
		case 0x53f06acc: //	SetBoneRotActive
			{		
				uint32 boneName;
				int enabled;
				pParams->GetChecksum( CRCD(0xcab94088,"bone"), &boneName, true );
				pParams->GetInteger( NONAME, &enabled, true );

                this->SetProceduralBoneRotActive( boneName, enabled );
			}
			break;
			
		// @script | SetBoneScaleMin | Sets the minimum translation extent for procedurally animated bone (in world units)
        // @parmopt float | x | 0 | minimum extent x
        // @parmopt float | y | 0 | minimum extent y
        // @parmopt float | z | 0 | minimum extent z
        // @parm name | bone | name of procedural bone
		case 0xc6889e91: //	SetBoneScaleMin
			{
				Mth::Vector vec(0.0f,0.0f,0.0f);
				uint32 boneName;
				pParams->GetFloat( CRCD(0x7323e97c,"x"), &vec[X], false );
				pParams->GetFloat( CRCD(0x0424d9ea,"y"), &vec[Y], false );
				pParams->GetFloat( CRCD(0x9d2d8850,"z"), &vec[Z], false );
				pParams->GetChecksum( CRCD(0xcab94088,"bone"), &boneName, true );
				
				this->SetProceduralBoneScaleMin( boneName, vec );
			}
			break;
			
		// @script | SetBoneScaleMax | Sets the maximum translation extent for procedurally animated bone (in world units)
		// @parmopt float | x | 0 | maximum extent x
		// @parmopt float | y | 0 | maximum extent y
		// @parmopt float | z | 0 | maximum extent z
		// @parm name | bone | name of procedural bone
		case 0xfa85a1c8: //	SetBoneScaleMax
			{
				Mth::Vector vec(0.0f,0.0f,0.0f);
				uint32 boneName;
				pParams->GetFloat( CRCD(0x7323e97c,"x"), &vec[X], false );
				pParams->GetFloat( CRCD(0x0424d9ea,"y"), &vec[Y], false );
				pParams->GetFloat( CRCD(0x9d2d8850,"z"), &vec[Z], false );
				pParams->GetChecksum( CRCD(0xcab94088,"bone"), &boneName, true );
				
				this->SetProceduralBoneScaleMax( boneName, vec );
			}
			break;
		
		// @script | SetBoneScaleSpeed | Sets the speed at which the bone's translation will procedurally animate (in 4096 notation)
		// @parmopt int | x | 0 | speed x (2048 immediately goes from T0 to T1)
		// @parmopt int | y | 0 | speed y (2048 immediately goes from T0 to T1)
		// @parmopt int | z | 0 | speed z (2048 immediately goes from T0 to T1)
		// @parm name | bone | name of procedural bone
		case 0xd32c2724: //	SetBoneScaleSpeed
			{
				Mth::Vector vec(0.0f,0.0f,0.0f);
				uint32 boneName;
				pParams->GetFloat( CRCD(0x7323e97c,"x"), &vec[X], false );
				pParams->GetFloat( CRCD(0x0424d9ea,"y"), &vec[Y], false );
				pParams->GetFloat( CRCD(0x9d2d8850,"z"), &vec[Z], false );
				pParams->GetChecksum( CRCD(0xcab94088,"bone"), &boneName, true );
				
				this->SetProceduralBoneScaleSpeed( boneName, vec );
			}
			break;
			
		// @script | SetBoneScaleCurrent | Immediately sets the procedurally animated bone to a specific translation (in 4096 notation)
		// @parmopt int | x | 0 | current x (0 = T0, 1024 = halfway between T0 and T1, 2048 = T1, 3072 = halfway between T1 and T0, 4096 = T0)
		// @parmopt int | y | 0 | current y (0 = T0, 1024 = halfway between T0 and T1, 2048 = T1, 3072 = halfway between T1 and T0, 4096 = T0)
		// @parmopt int | z | 0 | current z (0 = T0, 1024 = halfway between T0 and T1, 2048 = T1, 3072 = halfway between T1 and T0, 4096 = T0)
		// @parm name | bone | name of procedural bone
		case 0xd12b3713: //	SetBoneScaleCurrent
			{
				Mth::Vector vec(0.0f,0.0f,0.0f);
				uint32 boneName;
				pParams->GetFloat( CRCD(0x7323e97c,"x"), &vec[X], false );
				pParams->GetFloat( CRCD(0x0424d9ea,"y"), &vec[Y], false );
				pParams->GetFloat( CRCD(0x9d2d8850,"z"), &vec[Z], false );
				pParams->GetChecksum( CRCD(0xcab94088,"bone"), &boneName, true );
				
				this->SetProceduralBoneScaleCurrent( boneName, vec );
			}
			break;
		
		// @script | SetBoneScaleActive | Sets whether the bone should procedurally animate its translation
		// @uparm int | active (either 0 or 1)
		// @parm name | bone | name of procedural bone
		case 0xf11daaae: //	SetBoneScaleActive
			{
				uint32 boneName;
				int enabled;
				pParams->GetChecksum( "bone", &boneName, true );
				pParams->GetInteger( NONAME, &enabled, true );
				
                this->SetProceduralBoneScaleActive( boneName, enabled );
			}
			break;
		
		default:
			return AF_NOT_EXECUTED;
	}

	return AF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPoseController::CPoseController( CBlendChannel* pBlendChannel ) : CBaseAnimController( pBlendChannel )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPoseController::~CPoseController()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPoseController::InitFromStructure( Script::CStruct* pParams )
{
	CBaseAnimController::InitFromStructure( pParams );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPoseController::Update()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPoseController::SetPose( Gfx::CPose* pPose )
{
	memcpy( &m_pose, pPose, sizeof(Gfx::CPose) );

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPoseController::GetPose( Gfx::CPose* pResultPose )
{
	// this nukes the existing pose, meaning that
	// this should be the only controller in the list...
	memcpy( pResultPose, &m_pose, sizeof(Gfx::CPose) );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CLookAtController::CLookAtController( CBlendChannel* pBlendChannel ) : CBaseAnimController( pBlendChannel )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CLookAtController::~CLookAtController()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLookAtController::InitFromStructure( Script::CStruct* pParams )
{
	CBaseAnimController::InitFromStructure( pParams );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLookAtController::Update()
{
	Dbg_MsgAssert( 0, ( "Unimplemented function" ) );

	// GJ:  This is just a sample controller...  I was thinking
	// that this could be used for orienting the head towards
	// an object in the scene, given some constraints
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPartialAnimController::CPartialAnimController( CBlendChannel* pBlendChannel ) : CBaseAnimController( pBlendChannel )
{
	mp_quickAnim = Nx::CEngine::sInitQuickAnim();
	mp_quickAnim->Enable( false );
	
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	m_animScriptName = pAnimationComponent->GetAnimScriptName();

	mp_animChannel = new Gfx::CAnimChannel;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPartialAnimController::~CPartialAnimController()
{
	if ( mp_quickAnim )
	{
		Nx::CEngine::sUninitQuickAnim( mp_quickAnim );
		mp_quickAnim = NULL;
	}

	if ( mp_animChannel )
	{
		delete mp_animChannel;
		mp_animChannel = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPartialAnimController::InitFromStructure( Script::CStruct* pParams )
{   
	CBaseAnimController::InitFromStructure( pParams );
	
	uint32 anim_name;
	pParams->GetChecksum( CRCD(0x6c2bfb7f,"animName"), &anim_name, Script::ASSERT );
	
	float From = 0.0f;
	float To = 0.0f;
	float Current = 0.0f;
	Obj::CAnimationComponent* pAnimationComponent = GetAnimationComponentFromObject( GetObject() );
	float Duration = pAnimationComponent->AnimDuration( anim_name );
	Gfx::GetTimeFromParams( &From, &To, Current, Duration, pParams, NULL );
	  
	Gfx::EAnimLoopingType loopingType;
	Gfx::GetLoopingTypeFromParams( &loopingType, pParams );

	float speed = 1.0f;
	pParams->GetFloat( CRCD(0xf0d90109,"speed"), &speed, Script::NO_ASSERT );
	
	pParams->GetFloat(CRCD(0x230ccbf4, "Current"), &Current);

	float blend_period = 0.0f;

	mp_animChannel->PlaySequence( anim_name, From, To, loopingType, blend_period, speed);
	
	if (Current != 0.0f)
	{
		mp_animChannel->AddTime(Current);
	}

	Dbg_Assert( mp_quickAnim );
	mp_quickAnim->SetAnimAssetName( m_animScriptName + anim_name );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPartialAnimController::GetPose( Gfx::CPose* pResultPose )
{
	
	#ifdef	__NOPT_ASSERT__
	{
		CSkeleton* pSkeleton = GetSkeleton();
		Dbg_MsgAssert( pSkeleton, ( "No skeleton?" ) );	
		Dbg_MsgAssert( pResultPose, ( "No return data" ) );
	}
	#endif
	
//	int skipList[vMAX_BONES];
//	GetSkeleton()->GetSkipList( &skipList[0] );
	uint32*	p_skip_list = GetSkeleton()->GetBoneSkipList();
	uint32	skip_index	= GetSkeleton()->GetBoneSkipIndex();

#if 1
	Dbg_Assert( mp_quickAnim );
	mp_quickAnim->GetInterpolatedFrames( pResultPose->m_rotations, pResultPose->m_translations, p_skip_list, skip_index, mp_animChannel->GetCurrentAnimTime() );
	mp_quickAnim->Enable( true );
#else
	Mth::Quat theRot[128];
	Mth::Vector theTrans[128];

	Dbg_Assert( mp_quickAnim );
	mp_quickAnim->GetInterpolatedFrames( theRot, theTrans, p_skip_list, skip_index, mp_animChannel->GetCurrentAnimTime() );
	mp_quickAnim->Enable( true );
	
	Script::CArray* pArray = Script::GetArray( m_boneListName, Script::ASSERT );
	for ( uint32 i = 0; i < pArray->GetSize(); i++ )
	{
		int idx = pSkeleton->GetBoneIndexById( pArray->GetChecksum(i) );
		if ( idx != -1 )
		{
			*(pResultPose->m_rotations+idx)=theRot[idx];
			*(pResultPose->m_translations+idx)=theTrans[idx];
		}
	}
#endif

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPartialAnimController::Update()
{
	// This class is used for either appending,
	// or overwriting a subset of the bones in the RT array,
	// given a CBonedAnimFrameData...
	Dbg_MsgAssert( mp_animChannel->m_loopingType != Gfx::LOOPING_WOBBLE, ( "Not supposed to be wobble type" ) );
			
	float oldTime = mp_animChannel->m_currentTime;

	mp_animChannel->m_currentTime = mp_animChannel->get_new_anim_time();

//	mp_animChannel->ProcessCustomKeys( oldTime, mp_animChannel->m_currentTime );

	if ( mp_animChannel->m_currentTime < oldTime )
	{
		mp_quickAnim->Enable( false );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

EAnimFunctionResult CPartialAnimController::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		case 0x5f495ae0:	// InvalidateCache
		{
			mp_quickAnim->Enable( false );
			
			// want the other controllers to be invalidated too
			return AF_NOT_EXECUTED;
		}
		break;

		case 0xd63a1b81:	// GetPartialAnimParams
		{
			Dbg_Assert( pScript );
			pScript->GetParams()->AddFloat( CRCD(0xaaecb346,"partialAnimTime"), mp_animChannel->GetCurrentAnimTime() );
			pScript->GetParams()->AddChecksum( CRCD(0x659bf355,"partialAnim"), mp_animChannel->GetCurrentAnim() );
			pScript->GetParams()->AddInteger( CRCD(0x4966cc11,"partialAnimComplete"), mp_animChannel->IsAnimComplete() );

			return AF_TRUE;
		}
		break;

		case 0xbd4edd44: // SetPartialAnimSpeed
		{
			float speed;
			pParams->GetFloat( CRCD(0xf0d90109,"speed"), &speed, Script::ASSERT );			
			mp_animChannel->SetAnimSpeed( speed );

			return AF_TRUE;
		}
		break;

		case 0x6aaeb76f: // IncrementPartialAnimTime
		{
			float incVal;
			pParams->GetFloat( CRCD(0x06c9f278,"incVal"), &incVal, Script::ASSERT );			
			
			// GJ:  a way to fool the animation controller
			// into incrementing its time (frame rate
			// independent) for the viewer object

			float oldAnimSpeed = mp_animChannel->GetAnimSpeed();

			mp_animChannel->SetAnimSpeed( 0.0f );

			mp_animChannel->m_currentTime += incVal;

			mp_animChannel->Update();

			mp_animChannel->SetAnimSpeed( oldAnimSpeed );

			return AF_TRUE;
		}
		break;
			
		case 0xf5e2b871: // ReversePartialAnimDirection
		{
			mp_animChannel->ReverseDirection();

			return AF_TRUE;
		}
		break;

		default:
			return AF_NOT_EXECUTED;
	}

	return AF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}

