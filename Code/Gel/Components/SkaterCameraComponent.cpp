//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       SkaterCameraComponent.cpp
//* OWNER:          Dave Cowling
//* CREATION DATE:  02/21/03
//****************************************************************************

#include <gel/components/skatercameracomponent.h>

#include <gel/object/compositeobjectmanager.h>
#include <gel/object/compositeobject.h>
#include <gel/components/cameracomponent.h>
#include <gel/components/inputcomponent.h>
#include <gel/components/camerautil.h>
#include <gel/components/cameralookaroundcomponent.h>
#include <gel/components/movablecontactcomponent.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

#include <gfx/nxviewman.h>

#include <sk/objects/proxim.h>
#include <sk/components/skaterphysicscontrolcomponent.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/components/skaterbalancetrickcomponent.h>
#include <sk/components/skaterstatecomponent.h>


namespace Obj
{

const float SKATERCAMERACOMPONENT_LEAN_TO_SKATERCAM_LEAN	= (( Mth::PI / 6.0f ) / -4096.0f );
	
const float	SKATERCAMERACOMPONENT_INTERPOLATOR_TIMESTEP		= ( 1.0f / 60.0f );

// This is the value that mAbove should tend to when the behind value is closer than mBehind.

const float VERT_AIR_LANDED_TIME	= 10.0f / 60.0f;
const float TILT_MAX				= 0.34907f;						// 20 degees.
const float TILT_MIN				= -0.34907f;					// -20 degees.
const float TILT_INCREMENT			= 2.0f * ( TILT_MAX / 60.0f );	// 40 degrees/second.
const float TILT_DECREMENT			= 2.0f * ( TILT_MAX / 60.0f );	// 40 degrees/second.
const float TILT_RESTORE			= 8.0f * ( TILT_MAX / 60.0f );	// 160 degrees/second.
	
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static void _xrotlocal ( Mth::Matrix& m, float a )
{
	m.RotateXLocal( a );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CBaseComponent* CSkaterCameraComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterCameraComponent );	
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CSkaterCameraComponent::CSkaterCameraComponent() : CBaseComponent()
{
	SetType( CRC_SKATERCAMERA );
	
	mMode = SKATERCAM_MODE_UNDEFINED;
				  
	// Create the SLERP interpolator here.
	mpSlerp				= new Mth::SlerpInterpolator;

	// Set current zoom and LERP rates.
	mCurrentZoom		= 1.0f;
	mZoomLerp			= 0.0625f;
	mLastDot			= 1.0f;

	mLerpXZ				= 0.25f;
	mLerpY				= 0.5f;
	mVertAirLerpXZ		= 1.0f;
	mVertAirLerpY		= 1.0f;
	mGrindLerp			= 0.1f;
	
	mpSkater= NULL;
	mpSkaterStateComponent = NULL;
	mpSkaterPhysicsControlComponent = NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CSkaterCameraComponent::~CSkaterCameraComponent()
{
	delete mpSlerp;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkaterCameraComponent::Finalize (   )
{
	mp_lookaround_component = GetCameraLookAroundComponentFromObject(GetObject());
	
	Dbg_Assert(mp_lookaround_component);
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkaterCameraComponent::Enable( bool enable )
{
	CCameraComponent *p_cam_comp = GetCameraComponentFromObject( GetObject());
	Dbg_MsgAssert( p_cam_comp, ( "SkaterCameraComponent requires CameraComponent attached to parent" ));
	p_cam_comp->Enable( enable );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Gfx::Camera* CSkaterCameraComponent::GetCamera( void )
{
	CCameraComponent *p_cam_comp = GetCameraComponentFromObject( GetObject());
	Dbg_MsgAssert( p_cam_comp, ( "SkaterCameraComponent requires CameraComponent attached to parent" ));
	return p_cam_comp->GetCamera();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const Mth::Vector& CSkaterCameraComponent::GetPosition( void ) const 
{
	CCameraComponent *p_cam_comp = GetCameraComponentFromObject( GetObject());
	Dbg_MsgAssert( p_cam_comp, ( "SkaterCameraComponent requires CameraComponent attached to parent" ));
	return p_cam_comp->GetPosition();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkaterCameraComponent::SetPosition( Mth::Vector& pos )
{
	CCameraComponent *p_cam_comp = GetCameraComponentFromObject( GetObject());
	Dbg_MsgAssert( p_cam_comp, ( "SkaterCameraComponent requires CameraComponent attached to parent" ));
	p_cam_comp->SetPosition( pos );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const Mth::Matrix& CSkaterCameraComponent::GetMatrix( void ) const
{
	CCameraComponent *p_cam_comp = GetCameraComponentFromObject( GetObject());
	Dbg_MsgAssert( p_cam_comp, ( "SkaterCameraComponent requires CameraComponent attached to parent" ));
	return p_cam_comp->GetMatrix();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkaterCameraComponent::InitFromStructure( Script::CStruct* pParams )
{

// NOT We need a CCameraComponent attached in order to get camera details.
//	CCameraComponent *p_cam_comp	= GetCameraComponentFromObject( GetObject());
//	Dbg_MsgAssert( p_cam_comp, ( "CSkaterCameraComponent needs CCameraComponent attached to parent" ));

	// Clear the frame matrix, so that future slerps make sense.
	//Mth::Matrix& frame_matrix		= p_cam_comp->GetMatrix();
	Mth::Matrix& frame_matrix		= GetObject()->GetMatrix();
	frame_matrix.Ident();

	mLastActualRight 				= frame_matrix[X];
	mLastActualUp 					= frame_matrix[Y];
	mLastActualAt 					= frame_matrix[Z];


	// The camera requires a "CameraTarget" checksum parameter
	uint32	target_id = 0 ;
	pParams->GetChecksum("CameraTarget", &target_id, true);

	Dbg_MsgAssert(target_id >=0 && target_id <= 15,("Bad CameraTarget 0x%x in SkaterCameraComponent",target_id));

	CSkater* p_skater = static_cast<CSkater*>(CCompositeObjectManager::Instance()->GetObjectByID(target_id)); 
	Dbg_MsgAssert(p_skater,("Can't find skater %d in SkaterCameraComponent",target_id));

		
	SetSkater(p_skater);
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkaterCameraComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	// Reinitialise.
	InitFromStructure( pParams );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkaterCameraComponent::Commit( void )
{
	// Under the normal operation of components, the CSkaterCameraComponent updates the values
	// in CCameraComponent during its Update() function. The CCameraComponent in turn will set
	// the Gfx::CCamera values during it's Update() function.
	// In some cases however, we need the camera changes to be committed to the Gfx::Camera
	// immediately, usually when logic is paused, and the component Update()'s are not taking place.
	CCameraComponent *p_cam_component = GetCameraComponentFromObject( GetObject());
	if( p_cam_component )
	{
		p_cam_component->Update();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCameraComponent::ReadyForActivation ( const SCameraState& state )
{
	Dbg_MsgAssert(mpSkater, ("Skater camera (%s) has NULL target", Script::FindChecksumName(GetObject()->GetID())));
	
	mLastActualRight = state.lastActualMatrix[X];
	mLastActualUp = state.lastActualMatrix[Y];
	mLastActualAt = state.lastActualMatrix[Z];
	mLastTripodPos = state.lastTripodPos;
	mLastDot = state.lastDot;
	mCurrentZoom = state.lastZoom;
	mVertAirLandedTimer = 0.0f;
	mInstantCount = 0;
	
	mp_lookaround_component->mLookaroundHeading = 0.0f;
	mp_lookaround_component->mLookaroundLock = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCameraComponent::Update( void )
{
	if( mpSkater == NULL )
	{
		return;
	}
	
	// optimization KLUDGE
	if (mpSkaterPhysicsControlComponent && mpSkaterPhysicsControlComponent->IsDriving())
	{
		GetObject()->Pause(true);
		return;
	}
	
	// We need a CCameraComponent attached in order to get camera details.
	CCameraComponent *p_cam_comp = GetCameraComponentFromObject( GetObject());
	Dbg_MsgAssert( p_cam_comp, ( "CSkaterCameraComponent needs CCameraComponent attached to parent" ));

	// This used to be passed in as a param for the old CSkaterCam::Update().
	// Now we get it from a flag in the skater
	bool instantly = false;
	
	Dbg_Assert( mpSkater );
	if (mpSkater->HasTeleported())
	{
		instantly = true;
	}

	if( instantly )
	{
		mInstantCount = 3;
	}

	if( mInstantCount > 0 )
	{
		--mInstantCount;
		instantly = true;
	}

		
//	if (instantly)
//	{
//		printf ("%d: Instantly!\n",(int)Tmr::GetRenderFrame());
//	}
		
	// Length of last frame in seconds.
	float delta_time = Tmr::FrameLength();

	// Update any values we are currently interpolating.
	UpdateInterpolators();

	// Would have used a SetPos() function, but the camera update in rwviewer.cpp is spread out all over the place,
	// so it's not easy to do. We'll just store the old position before updating anything...
//	Gfx::Camera::StoreOldPos();
	p_cam_comp->StoreOldPosition();

//	Mth::Matrix& frame_matrix	= Gfx::Camera::GetMatrix();
	Mth::Matrix& frame_matrix	= p_cam_comp->GetMatrix();

	frame_matrix[X]   		= mLastActualRight;
	frame_matrix[Y]			= mLastActualUp;
	frame_matrix[Z]			= mLastActualAt;

	Mth::Vector	current_at_vector( frame_matrix[Z][X], frame_matrix[Z][Y], frame_matrix[Z][Z] ); 

	// Obtain skater state.
	EStateType	state			= mpSkaterStateComponent->GetState();
	Mth::Matrix target;

	if( state == WALL )
	{
#		ifdef	DEBUG_CAMERA
		printf ("SkaterCam Using mLastPreWallSkaterMatrix, as wallriding\n");
#		endif
		target						= mLastPreWallSkaterMatrix;
	}
	else
	{
#		ifdef	DEBUG_CAMERA
		printf ("SkaterCam Using GetCameraDisplayMatrix\n");
#		endif
		// target						= static_cast< CCompositeObject* >(mpSkater)->GetDisplayMatrix();
		if (mpSkater->IsLocalClient())
		{
			target						= mpSkater->GetDisplayMatrix();
		}
		else
		{
			target						= static_cast< CCompositeObject* >(mpSkater)->GetDisplayMatrix();
		}
		mLastPreWallSkaterMatrix	= target;
	}

	// Lerp towards the required lean.
	float cam_lean = 0.0f;
	if( state == RAIL && mpSkater->IsLocalClient())
	{		
		// Need to translate the lean value [-4096, 4096] into a reasonable camera lean angle.
		cam_lean = GetSkaterBalanceTrickComponentFromObject(mpSkater)->mGrind.mManualLean * SKATERCAMERACOMPONENT_LEAN_TO_SKATERCAM_LEAN;
	}

	if( cam_lean != mLean )
	{
		if( instantly )
		{
			mLean = cam_lean;
		}
		else
		{
			mLean += (( cam_lean - mLean ) * mGrindLerp );
		}
	}
		
	Mth::Vector skater_up;
					
	if (state != GROUND)
	{
		skater_up	= target[Y];
	}
	else
	{
		skater_up	= mpSkaterStateComponent->GetCameraDisplayNormal();
	}

//	Gfx::AddDebugLine( p_skater->m_pos, ( target[X] * 72.0f ) + p_skater->m_pos, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 4 );
//	Gfx::AddDebugLine( p_skater->m_pos, ( target[Y] * 72.0f ) + p_skater->m_pos, MAKE_RGB( 0, 128, 0 ), MAKE_RGB( 0, 128, 0 ), 4 );
//	Gfx::AddDebugLine( p_skater->m_pos, ( target[Z] * 72.0f ) + p_skater->m_pos, MAKE_RGB( 0, 0, 128 ), MAKE_RGB( 0, 0, 128 ), 4 );
		
	// Use vel for forward facing, if moving in xz plane.
	Mth::Vector vel_xz( mpSkater->GetVel().GetX(), 0.0f, mpSkater->GetVel().GetZ());

	bool use_vert_cam = UseVertCam();
	
	// use velocity if physics not paused, and if we are moving fast enough, or if we are doing vert cam
	// or if we are doing spine physics
	if(
		!(mpSkaterPhysicsControlComponent && mpSkaterPhysicsControlComponent->IsPhysicsSuspended()) && (vel_xz.Length() > 0.1f || use_vert_cam  || mpSkaterStateComponent->GetFlag( SPINE_PHYSICS ))
	   )
	{
		if( use_vert_cam )
		{
			// If it's vert, then set target direction to near straight down.
			target[Z].Set( 0.0f, -1.0f, 0.0f );

			// Zero lookaround when in vert air (and not in lookaround locked or override mode, or doing a lip trick).
			if( !mp_lookaround_component->mLookaroundLock && !mp_lookaround_component->mLookaroundOverride && ( state != LIP ))
			{
				mp_lookaround_component->mLookaroundTilt		= 0.0f;
				mp_lookaround_component->mLookaroundHeading	= 0.0f;
			}
		}
		else
		{
			target[Z]		= mpSkater->GetVel();
			
#			if 1	// for use with non-straight spine transfer physics
			if( mpSkaterStateComponent->GetFlag( SPINE_PHYSICS ))
			{
				// Just use the up velocity, plus the velocity over the spine will be straight down when coming down the other side.
				target[Z][X] = 0.0f;
				target[Z][Z] = 0.0f;
				target[Z] += mpSkaterStateComponent->GetSpineVel();
			}
#			endif

			target[Z].Normalize();

//			printf ("Using Velocity (%f, %f, %f)\n", target[Z][X],target[Z][Y], target[Z][Z]);

//			Gfx::AddDebugLine( mpSkater->m_pos, ( target[Z] * 72.0f ) + mpSkater->m_pos, MAKE_RGB( 128, 128, 128 ), MAKE_RGB( 128, 128, 128 ), 4 );

			// Flatten out the velocity component perpendicular to the ground. However, if the skater is in the air, the
			// m_last_normal will contain the last ground normal, which we don't want, so set it to be 'up'.
			float			dot;
			Mth::Vector		normal;

			if( state == AIR )
			{
				// Patch for spine physics
				// only reduce component perpendicular to plane by 60%
				// so we still get some up/down camera movmeent 
				// only do this when moving down though, otherwise it looks poo
				if (mpSkaterStateComponent->GetFlag(SPINE_PHYSICS) && target[Z][Y] < 0.0f)
				{
					normal.Set( 0.0f, 1.0f, 0.0f );
					dot =  0.7f * Mth::DotProduct(target[Z],normal);	// change this to 0.8 to look down when coming down other side of a spine
				}
				else
				{
					normal.Set( 0.0f, 1.0f, 0.0f );
					dot		= target[Z].GetY();
				}
				// Set world up as the target vector.
				target[Y].Set( 0.0f, 1.0f, 0.0f );
			}
			else if( state == WALL )
			{
				normal.Set( 0.0f, 1.0f, 0.0f );
				dot		= target[Z].GetY();
			}
			else
			{
				normal	= mpSkaterStateComponent->GetCameraCurrentNormal();
				dot =  0.8f * Mth::DotProduct(target[Z],normal);
			}
			
			target[Z] -= normal * dot;
			target[Z].Normalize();
			
//			Gfx::AddDebugLine( mpSkater->m_pos, ( target[Z] * 72.0f ) + mpSkater->m_pos, MAKE_RGB( 256, 256, 256 ), MAKE_RGB( 256, 256, 256 ), 4 );
		}

		// We need to orthonormalize in a specific order, as when the velocity is going backwards, then 
		// the X orient will be wrong, and so plonk the skater upside down.
		target[X]			= Mth::CrossProduct( target[Y], target[Z] );
		target[X].Normalize();
		
		target[Y]			= Mth::CrossProduct( target[Z], target[X] );
		target[Y].Normalize();

//		Gfx::AddDebugLine( mpSkater->m_pos, ( target[X] * 72.0f ) + mpSkater->m_pos, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 4 );
//		Gfx::AddDebugLine( mpSkater->m_pos, ( target[Y] * 72.0f ) + mpSkater->m_pos, MAKE_RGB( 0, 128, 0 ), MAKE_RGB( 0, 128, 0 ), 4 );
//		Gfx::AddDebugLine( mpSkater->m_pos, ( target[Z] * 72.0f ) + mpSkater->m_pos, MAKE_RGB( 0, 0, 128 ), MAKE_RGB( 0, 0, 128 ), 4 );
	}
	
	if (mpSkater->IsLocalClient() && mpSkaterStateComponent->GetFlag(IN_ACID_DROP))
	{
		CSkaterCorePhysicsComponent* pSkaterPhysicsComponent = GetSkaterCorePhysicsComponentFromObject(mpSkater);
		Dbg_Assert(pSkaterPhysicsComponent);
		
		target = pSkaterPhysicsComponent->m_acid_drop_camera_matrix;
	}
							
	// Tilt further if in regular air (going off a big jump, for example), or doing a lip trick.
	if( state == AIR )
	{
		mTiltAddition += TILT_INCREMENT * Tmr::FrameRatio();
		if( mTiltAddition > TILT_MAX )
		{
			mTiltAddition = TILT_MAX;
		}				
	}
	else if( mTiltAddition > 0.0f )
	{
		mTiltAddition -= TILT_RESTORE * Tmr::FrameRatio();
		if( mTiltAddition < 0.0f )
		{
			mTiltAddition = 0.0f;
		}				
	}
	else if( mTiltAddition < 0.0f )
	{
		mTiltAddition += TILT_RESTORE * Tmr::FrameRatio();
		if( mTiltAddition > 0.0f )
		{
			mTiltAddition = 0.0f;
		}				
	}

	// PJR: Needed to do this to fix a compiler crash on NGC. Hopefully SN will
	// fix this & I can revert this hack...
	target.RotateYLocal( mp_lookaround_component->mLookaroundHeading + mp_lookaround_component->mLookaroundHeadingStartingPoint );

	// Now tilt the matrix down a bit.
	if( state == LIP )
	{
//		target.RotateYLocal( 0.8f );
//		target.RotateXLocal( mLipTrickTilt + mTiltAddition );
		_xrotlocal ( target, mLipTrickTilt + mTiltAddition );
	}					
	else if( !(mpSkaterStateComponent->GetFlag( VERT_AIR ) && !mpSkaterStateComponent->GetFlag( CAN_BREAK_VERT )  && !mpSkaterStateComponent->GetFlag(SPINE_PHYSICS) ) /*|| !mpSkaterStateComponent->GetFlag(IN_ACID_DROP)*/)
	{
//		target.RotateXLocal( mTilt + mTiltAddition );
		_xrotlocal ( target, mTilt + mTiltAddition );
	}
	
	// Now adjust for 'lookaround'.
//	target.RotateXLocal( mLookaroundTilt );
	_xrotlocal( target, mp_lookaround_component->mLookaroundTilt );
	
	static float lip_rotate = 0.0f;

	if( state == LIP )
	{
		lip_rotate = Mth::PI / 2.0f;

		if( mLipSideDecided == 0 )
		{
			target.RotateY( lip_rotate );

			// Do collision check.
			CFeeler feeler;

			Mth::Vector horiz_z( target[Z][X], 0.0f, target[Z][Z] );
			horiz_z.Normalize();
			Mth::Vector a = mpSkater->m_pos - ( target[Z] * 3.0f );
			Mth::Vector b = a - ( horiz_z * 72.0f );

			feeler.SetIgnore(IGNORE_FACE_FLAGS_1, IGNORE_FACE_FLAGS_0);
			bool collision = feeler.GetCollision(a, b, true);
			if( !collision )
			{
				// Side 1 is fine.
				mLipSideDecided = 1;
			}
			else
			{
				float dist = feeler.GetDist();
				target.RotateY( -lip_rotate * 2.0f );
				horiz_z.Set( target[Z][X], 0.0f, target[Z][Z] );
				horiz_z.Normalize();
				a = mpSkater->m_pos - ( target[Z] * 3.0f );
				b = a - ( horiz_z * 72.0f );

				collision = feeler.GetCollision(a, b, true);
				if( !collision )
				{
					// Side 2 is fine.
					mLipSideDecided = 2;
				}
				else
				{
					if( feeler.GetDist() < dist )
					{
						// Side 2 is better than side 1.
						mLipSideDecided = 2;
					}
					else
					{
						// Side 1 is better than side 2.
						target.RotateY( lip_rotate * 2.0f );
						mLipSideDecided = 1;
					}
				}
			}
		}
		else
		{
			if( mLipSideDecided == 1 )
			{
				target.RotateY( lip_rotate );
			}
			else
			{
				target.RotateY( -lip_rotate );
			}
		}
	}
	else
	{
		mLipSideDecided = 0;
	}					

	// Set skater flag to indicate when the liptrick camera has spun to the opposite side.
	if( mpSkater && ( mLipSideDecided == 2 ))
	{
		mpSkater->mScriptFlags |= ( 1 << Script::GetInteger( 0x16b8e4cb /* "FLAG_SKATER_LIPTRICK_CAM_REVERSED" */ ));
	}
	else
	{
		mpSkater->mScriptFlags &= ~( 1 << Script::GetInteger( 0x16b8e4cb /* "FLAG_SKATER_LIPTRICK_CAM_REVERSED" */ ));
	}
	
//	Gfx::AddDebugLine( mpSkater->m_pos, ( target[X] * 72.0f ) + mpSkater->m_pos, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 4 );
//	Gfx::AddDebugLine( mpSkater->m_pos, ( target[Y] * 72.0f ) + mpSkater->m_pos, MAKE_RGB( 0, 128, 0 ), MAKE_RGB( 0, 128, 0 ), 4 );
//	Gfx::AddDebugLine( mpSkater->m_pos, ( target[Z] * 72.0f ) + mpSkater->m_pos, MAKE_RGB( 0, 0, 128 ), MAKE_RGB( 0, 0, 128 ), 4 );
	
	// Why doing this, when setting below?
	Mth::Matrix t_matrix(0.0f, 0.0f, 0.0f);
	
	// Since camera points in -Z, but player in +Z, we must negate the X and Z axes
	t_matrix[X][X]		= -target[0][0];
	t_matrix[X][Y]		= -target[0][1];
	t_matrix[X][Z]		= -target[0][2];
	t_matrix[X][W]		= 0.0f;
	
	t_matrix[Y][X]		= target[1][0];
	t_matrix[Y][Y]		= target[1][1];
	t_matrix[Y][Z]		= target[1][2];
	t_matrix[Y][W]		= 0.0f;
	
	t_matrix[Z][X]		= -target[2][0];
	t_matrix[Z][Y]		= -target[2][1];
	t_matrix[Z][Z]		= -target[2][2];
	t_matrix[Z][W]		= 0.0f;
			
	t_matrix[W][X]		= 0.0f;
	t_matrix[W][Y]		= 0.0f;
	t_matrix[W][Z]		= 0.0f;
	t_matrix[W][W]		= 1.0f;
        
	frame_matrix[W][X]	= 0.0f;
	frame_matrix[W][Y]	= 0.0f;
	frame_matrix[W][Z]	= 0.0f;
	frame_matrix[W][W]	= 1.0f;

	// if we want an instant update, then just move it
	if (instantly)
	{
		frame_matrix = t_matrix;
	}

	float dotx = Mth::DotProduct( t_matrix[X], frame_matrix[X] );
	float doty = Mth::DotProduct( t_matrix[Y], frame_matrix[Y] );
	float dotz = Mth::DotProduct( t_matrix[Z], frame_matrix[Z] );

	if( dotx > CAMERA_SLERP_STOP && doty > CAMERA_SLERP_STOP && dotz > CAMERA_SLERP_STOP )
	{
		// Do nothing, camera has reached the target so we just leave the frame matrix exactly where it is.
//	    frame_matrix = t_matrix;
	}
	else
	{
		// Initialise the SLERP interpolator, with the current matrix as the start, and the ideal
		// matrix as the end (target).

		// GARY:  is this right?  i assume p_matrix = &t_matrix
		mpSlerp->setMatrices( &frame_matrix, &t_matrix );

		// Copy flags and shit (why copying all this when most of the values will be overwritten?)
		Mth::Matrix stored_frame;
		stored_frame = frame_matrix;
		frame_matrix = t_matrix;			

		// Continue to slerp towards the target, updating frame_matrix with the slerped values.
		if( !(mpSkaterStateComponent->GetFlag( VERT_AIR ) && !mpSkaterStateComponent->GetFlag( CAN_BREAK_VERT ) && !mpSkaterStateComponent->GetFlag(SPINE_PHYSICS)))
		{
			// If the skater has just landed from big air, use the big air land slerp value.
			if(( mpSkaterStateComponent->GetState() == GROUND ) && ( mVertAirLandedTimer >= delta_time ))
			{
				mVertAirLandedTimer -= delta_time;
				mpSlerp->getMatrix( &frame_matrix, GetTimeAdjustedSlerp( mVertAirLandedSlerp, delta_time ));
			}
			else
			{
				// Clear the vert air landed timer.
				mVertAirLandedTimer = 0.0f;
				
				mpSlerp->getMatrix( &frame_matrix, GetTimeAdjustedSlerp( mSlerp, delta_time ));

				// At this point we can check to see the angle that the camera's 'at' vector moved through this frame.
				// If this angle is too large, we need to limit it, regardless of the slerp. (This is primarily to avoid
				// the nasty jerk when transitioning rails etc.
				float this_dot = ( current_at_vector.GetX() * frame_matrix[Mth::AT][X] ) 
                       + ( current_at_vector.GetY() * frame_matrix[Mth::AT][Y] ) 
                       + ( current_at_vector.GetZ() * frame_matrix[Mth::AT][Z] ); 

				// Dot values will be in the range [0,1], with 1 being no change.
				if( this_dot < ( mLastDot * 0.9998f ))
				{
					// The angular change this frame is too big. Need to recalculate with an adjusted slerp value.
					float new_slerp = mSlerp * ( this_dot / ( mLastDot * 0.9998f ));
					mpSlerp->setMatrices( &stored_frame, &t_matrix );
					mpSlerp->getMatrix( &frame_matrix, GetTimeAdjustedSlerp( new_slerp, delta_time ));

					this_dot = mLastDot * 0.9998f;
				}
				mLastDot = this_dot;
			}
		}
		else
		{
			mpSlerp->getMatrix( &frame_matrix, GetTimeAdjustedSlerp( mVertAirSlerp, delta_time ));

			// Set the vert air landed timer to the max, since the skater is in vert air.
			mVertAirLandedTimer = VERT_AIR_LANDED_TIME;
		}						
	}		

	// Mick:  This is a patch to allow the walk camera to override the updating of the camera frame	
	// At this point, frame_matrix is valid to store.
	mLastActualRight	= frame_matrix[X];
	mLastActualUp		= frame_matrix[Y];
	mLastActualAt		= frame_matrix[Z];
	
	// Set camera position to be the same as the skater.
	Mth::Vector	cam_pos = GetTripodPos( instantly );
	
	// If in the air doing a trick, we slowly reduce the behind value to 'zoom' in on the skater.
	float above, behind;
	if( instantly )
	{
		float temp = mZoomLerp;
		mZoomLerp = 1.0f;					// fully lerp instantly
		CalculateZoom( &above, &behind );
		mZoomLerp = temp;
	}
	else
	{
		CalculateZoom( &above, &behind );
	}
	
	// Note we use the camera's at vector, as this will keep the skater in the middle of the frame.
	//cam_pos -= frame_matrix[Z] * behind;
	cam_pos += frame_matrix[Z] * behind;
		
	// Move camera along the Skater's up vector.
	cam_pos += skater_up * above;
	
	Mth::Vector	actual_focus_pos = mpSkater->m_pos + ( skater_up * above );
	
//	Gfx::AddDebugLine( mpSkater->m_pos, actual_focus_pos, MAKE_RGB( 0, 0, 128 ), MAKE_RGB( 0, 0, 128 ), 4 );
													  
//	Dbg_Message("Matrix Z  (%5.4f,%5.4f,%5.4f)", frame_matrix[Z][X], frame_matrix[Z][Y], frame_matrix[Z][Z]);
//	Dbg_Message("Matrix UP (%5.4f,%5.4f,%5.4f)", skater_up[X], skater_up[Y], skater_up[Z]);
//	Dbg_Message("Above %f Behind %f", above, behind);
//	Dbg_Message("focus pos (%5.1f,%5.1f,%5.1f)", actual_focus_pos[X], actual_focus_pos[Y], actual_focus_pos[Z]);
//	Dbg_Message("Camera after: pos (%5.1f,%5.1f,%5.1f)", cam_pos[X], cam_pos[Y], cam_pos[Z]);

	// Reorient the camera to look directly at the skater. We don't want to look at the interpolated position,
	// since this can lead to nasty jerkiness, especially on rails etc.
	target[Z].Set( actual_focus_pos.GetX() - cam_pos.GetX(), actual_focus_pos.GetY() - cam_pos.GetY(), actual_focus_pos.GetZ() - cam_pos.GetZ());
	target[Z].Normalize();

	// Read back the Y from the current matrix.
	target[Y][0]	= frame_matrix[Y][X];
	target[Y][1]	= frame_matrix[Y][Y];
	target[Y][2]	= frame_matrix[Y][Z];

	// Generate new orthonormal X and Y axes.
	target[X]		= Mth::CrossProduct( target[Y], target[Z] );
	target[X].Normalize();
	target[Y]		= Mth::CrossProduct( target[Z], target[X] );
	target[Y].Normalize();

	// Here is where lean may safely be applied without moving the focus position on screen.
	if( mLean != 0.0f )
	{
		target[Y].Rotate( target[Z], mLean );
		target[X].Rotate( target[Z], mLean );
	}
	
	// Write back into camera matrix.
	// Since camera points in -Z, but player in +Z, we must negate the X and Z axes
	frame_matrix[X][X]		= -target[0][0];
	frame_matrix[X][Y]		= -target[0][1];
	frame_matrix[X][Z]		= -target[0][2];
	frame_matrix[X][W]		= 0.0f;

	frame_matrix[Y][X]		= target[1][0];
	frame_matrix[Y][Y]		= target[1][1];
	frame_matrix[Y][Z]		= target[1][2];
	frame_matrix[Y][W]		= 0.0f;

	frame_matrix[Z][X]		= -target[2][0];
	frame_matrix[Z][Y]		= -target[2][1];
	frame_matrix[Z][Z]		= -target[2][2];
	frame_matrix[Z][W]		= 0.0f;

	// Update the position if there is a shake active.
	if(	mShakeDuration > 0.0f )
	{
		addShake( cam_pos, frame_matrix );
	}
		
	// Now do collision detection.
	
	bool no_camera_collision;
	if (mpSkater->IsLocalClient())
	{
		CSkaterCorePhysicsComponent* pSkaterPhysicsComponent = GetSkaterCorePhysicsComponentFromObject(mpSkater);
		Dbg_Assert(pSkaterPhysicsComponent);
		
		no_camera_collision = pSkaterPhysicsComponent->mp_movable_contact_component->GetContact() && state == LIP;
	}
	else
	{
		no_camera_collision = false;
	}
	
	if (no_camera_collision)
	{
		// no camera collision if doing a lip trick on a moving object
		//printf ("Lip trick on moving object\n");
	}
	else
	{
		ApplyCameraCollisionDetection(cam_pos, frame_matrix, cam_pos - ((Mth::Vector)( frame_matrix[Z] )) * behind, actual_focus_pos);
	}
	
	cam_pos[W] = 1.0f;
//	Gfx::Camera::SetPos( cam_pos );
	p_cam_comp->SetPosition( cam_pos );
	
	// reset old position if in instant update
	if (instantly)
	{
		p_cam_comp->StoreOldPosition();
	}
	
//	Dbg_Message("Camera Final: pos (%5.1f,%5.1f,%5.1f)", cam_pos[X], cam_pos[Y], cam_pos[Z]);

	// Store the position of the camera in CSkaterCam also (used for proximity nodes).

	// Dave - there is no m_pos anymore...
//	m_pos = cam_pos;

	// Dave - this used to return the value below when it was CSkaterCam::Update()
//	Mth::Vector	to_target = at_pos - cam_pos;
//	return to_target.Length();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterCameraComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		// @script | SC_ShakeCamera | shake the skater camera
        // @parm float | duration | duration of the shake (0 clears the shake)
        // @parm float | vert_amp | maximum vertical amplidtude of the shake
        // @parm float | horiz_amp | maximum horizontal amplidtude of the shake
        // @parm float | vert_vel | vertical velocity factor (how many full cycles of movement in 1 second) 
        // @parm float | horiz_vel | horizontal velocity factor (how many full cycles of movement in 1 second) 
		case CRCC(0x75a516d3,"SC_ShakeCamera"):
		{
			float duration	= 0.0f;
			float vert_amp	= 0.0f;
			float horiz_amp	= 0.0f;
			float vert_vel	= 0.0f;
			float horiz_vel	= 0.0f;

			pParams->GetFloat( "duration",	&duration );
			pParams->GetFloat( "vert_amp",	&vert_amp );
			pParams->GetFloat( "horiz_amp",	&horiz_amp );
			pParams->GetFloat( "vert_vel",	&vert_vel );
			pParams->GetFloat( "horiz_vel",	&horiz_vel );

			SetShake( duration, vert_amp, horiz_amp, vert_vel, horiz_vel );
			
			break;
		}
		
				 
		// @script | SC_SetMode | Set the mode of a skater camera
        // @parm integer | mode | mode number 1-4, start out at 2 (normal_medium)
		case CRCC(0xd7867ffe,"SC_SetMode"):
		{
		
			int mode = 0;
			pParams->GetInteger(CRCD(0x6835b854,"mode"),&mode, true);
			SetMode((ESkaterCamMode)mode);	   
			break; 	
		}

	
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}

	// the "default" case of the switch statement handles
	// unrecognized functions;  if we make it down here,
	// that means that the component already handled it
	// somehow
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCameraComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterCameraComponent::GetDebugInfo"));

	// Add any script components to the p_info structure,
	// and they will be displayed in the script debugger (qdebug.exe)
	// you will need to add the names to debugger_names.q, if they are not existing checksums

	/*	Example:
	p_info->AddInteger("m_never_suspend",m_never_suspend);
	p_info->AddFloat("m_suspend_distance",m_suspend_distance);
	*/
	
	// We call the base component's GetDebugInfo, so we can add info from the common base component.
	CBaseComponent::GetDebugInfo( p_info );
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCameraComponent::ResetLookAround( void )
{
	mp_lookaround_component->mLookaroundTilt		= 0.0f;
	mp_lookaround_component->mLookaroundHeading	= 0.0f;
	SetNoLerpUpdate( true );
	Update();
	Commit();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCameraComponent::ResetMode( )
{
	// Just call SetMode() again with the current mode
	SetMode(mMode);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCameraComponent::SetMode( ESkaterCamMode mode, float time )
{
	// We need a CCameraComponent attached in order to set camera details.
	CCameraComponent *p_cam_comp = GetCameraComponentFromObject(GetObject());
	Dbg_MsgAssert( p_cam_comp, ( "CSkaterCameraComponent needs CCameraComponent attached to parent" ));

	Dbg_MsgAssert( mode < SKATERCAM_NUM_MODES,( "Bad mode" ));

	// Obtain details for this mode.
	float h_fov, behind, above, tilt, origin_offset, zoom_lerp;
	const char* p_name;

	// Obtain a pointer to the array entry for the desired mode. The array is now dependent
	// on which game mode is currently being played, 1 player, 2 player vert, or 2 player horiz.
	Nx::ScreenMode screen_mode = Nx::CViewportManager::sGetScreenMode();

	Script::CArray* p_array;

	switch( screen_mode )
	{
		case Nx::vSPLIT_V:
		{
			p_array = Script::GetArray( "Skater_Camera_2P_Vert_Array" );
			break;
		}

		case Nx::vSPLIT_H:
		{
			p_array = Script::GetArray( "Skater_Camera_2P_Horiz_Array" );
			break;
		}

		default:
		{			
			p_array = Script::GetArray( "Skater_Camera_Array" );
			break;
		}
	}


	// Extra check here, since the array in physics.q may change.
	Dbg_MsgAssert( mode < (int)p_array->GetSize(), ( "Bad mode" ));

	uint32 cs = p_array->GetNameChecksum( mode );
	Script::CScriptStructure* p_struct = Script::GetStructure( cs );

	mMode = mode;

	mLipTrickTilt = 0.0f;

	// Read the values from the array.
	p_struct->GetFloat( CRCD(0x94df1603,"horiz_fov"),				&h_fov );
	p_struct->GetFloat( CRCD(0x52e6c41b,"behind"),					&behind );
	p_struct->GetFloat( CRCD(0xb96ae2d,"above"),					&above );
	p_struct->GetFloat( CRCD(0xe3c07609,"tilt"),						&tilt );
	p_struct->GetFloat( CRCD(0x12f6992,"origin_offset"),			&origin_offset );
	p_struct->GetFloat( CRCD(0x7f0032ac,"lip_trick_tilt"),			&mLipTrickTilt );
	p_struct->GetFloat( CRCD(0xadb6390e,"lip_trick_above"),			&mLipTrickAbove );

	// Get SLERP values.
	p_struct->GetFloat( CRCD(0xf54fb9c5,"slerp"),					&mSlerp );
	p_struct->GetFloat( CRCD(0x2fa11029,"vert_air_slerp"),			&mVertAirSlerp );
	p_struct->GetFloat( CRCD(0x42fd4a0,"vert_air_landed_slerp"),	&mVertAirLandedSlerp );

	// Get LERP values.
	p_struct->GetFloat( CRCD(0xae47899b,"lerp_xz"),					&mLerpXZ );
	p_struct->GetFloat( CRCD(0xe1e4e104,"lerp_y"),					&mLerpY );
	p_struct->GetFloat( CRCD(0xf386f4d9,"vert_air_lerp_xz"),			&mVertAirLerpXZ );
	p_struct->GetFloat( CRCD(0x4882a1fe,"vert_air_lerp_y"),			&mVertAirLerpY );
	p_struct->GetFloat( CRCD(0xbef0d195,"grind_lerp"),				&mGrindLerp );

	// Get zoom values.
	mGrindZoom			= 1.0f;
	mLipTrickZoom		= 1.0f;
	mBigAirTrickZoom	= 1.0f;

	p_struct->GetFloat( CRCD(0x748743a7,"zoom_lerp"),			&zoom_lerp );
	p_struct->GetFloat( CRCD(0x5a7f5cc5,"grind_zoom"),			&mGrindZoom );
	p_struct->GetFloat( CRCD(0xd790b83d,"big_air_trick_zoom"),	&mBigAirTrickZoom );
	p_struct->GetFloat( CRCD(0xd414c22e,"lip_trick_zoom"),		&mLipTrickZoom );
	mZoomLerp			= zoom_lerp;

	// Get camera style name.
	p_struct->GetText( CRCD(0xa1dc81f9,"name"), &p_name );

	// Send the new camera mode to console line 1.
	//HUD::PanelMgr* panelMgr = HUD::PanelMgr::Instance();
	//panelMgr->SendConsoleMessage( p_name, 1 );

	// Need to calculate interpolate timers here.
	mHorizFOV		= h_fov;
	mTilt			= tilt;
	mOriginOffset	= FEET( origin_offset );		// Convert to inches.

	// Set up interpolators for those values that require it.
	if( time > 0.0f )
	{
		behind						= FEET( behind );				// Convert to inches.
		above						= FEET( above );				// Convert to inches.
		mBehindInterpolatorTime		= time;
		mBehindInterpolatorDelta	= (( behind - mBehind ) * ( SKATERCAMERACOMPONENT_INTERPOLATOR_TIMESTEP	/ time ));
		mAboveInterpolatorTime		= time;
		mAboveInterpolatorDelta		= (( above - mAbove ) * ( SKATERCAMERACOMPONENT_INTERPOLATOR_TIMESTEP / time ));
	}
	else
	{
		mBehind						= FEET( behind );
		mAbove						= FEET( above );
		mBehindInterpolatorTime		= 0.0f;
		mAboveInterpolatorTime		= 0.0f;
	}
	
	// Set focal length immediately.
	switch( screen_mode )
	{
		case Nx::vSPLIT_V:
		{
#			ifdef __PLAT_NGC__
			p_cam_comp->SetHFOV( Script::GetFloat( "Skater_Cam_Horiz_FOV" ) / 2.0f );
#			else
			p_cam_comp->SetHFOV( mHorizFOV / 2.0f );
#			endif
			break;
		}

		case Nx::vSPLIT_H:
		{
			p_cam_comp->SetHFOV( mHorizFOV );
			break;
		}

		default:
		{			
			p_cam_comp->SetHFOV( mHorizFOV );
			break;
		}
	}

	switch( mode )
	{
		case ( SKATERCAM_MODE_REPLAY_FRONT ):
		case ( SKATERCAM_MODE_REPLAY_FRONT_ZOOM ):
			mp_lookaround_component->mLookaroundHeadingStartingPoint = Mth::PI;
			break;
		case ( SKATERCAM_MODE_REPLAY_LEFT ):
		case ( SKATERCAM_MODE_REPLAY_LEFT_ZOOM ):
			mp_lookaround_component->mLookaroundHeadingStartingPoint = Mth::PI / 2.0f;
			break;
		case ( SKATERCAM_MODE_REPLAY_BEHIND ):
		case ( SKATERCAM_MODE_REPLAY_BEHIND_ZOOM ):
			mp_lookaround_component->mLookaroundHeadingStartingPoint = 0;
			break;
		case ( SKATERCAM_MODE_REPLAY_RIGHT ):
		case ( SKATERCAM_MODE_REPLAY_RIGHT_ZOOM ):
			mp_lookaround_component->mLookaroundHeadingStartingPoint = -( Mth::PI / 2.0f );
			break;
		default:
			mp_lookaround_component->mLookaroundHeadingStartingPoint = 0;
			break;
	}   
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCameraComponent::ToggleMode()
{
	if ( mMode >= SKATERCAM_NUM_NORMAL_MODES )
	{
		return;
	}
	
	mMode = (ESkaterCamMode)( mMode + 1);
	if( mMode >= SKATERCAM_NUM_NORMAL_MODES )
	{
		mMode = SKATERCAM_MODE_FIRST_VALID;
	}

	ResetMode();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkaterCameraComponent::SetShake( float duration, float vert_amp, float horiz_amp, float vert_vel, float horiz_vel )
{
	mShakeDuration			= duration;
	mShakeInitialDuration	= duration;
	mShakeMaxVertAmp		= vert_amp;
	mShakeMaxHorizAmp		= horiz_amp;
	mShakeVertVel			= vert_vel;
	mShakeHorizVel			= horiz_vel;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CSkaterCameraComponent::UseVertCam( void )
{   
	if( mpSkater == NULL )
	{
		return false;
	}
	
	// this logic is used only by cameras observing non-local clients; in all other cases, skating camera logic in suspended when viewing a walking
	// skater; when observing, skater camera logic is always used; this prevents the "birds-eye-view" issue when observing a skater who transitioned to
	// walking during vert air
	if( mpSkaterStateComponent->GetPhysicsState() == WALKING )
	{
		return false;
	}
	
	// Use vert cam when in the lip state.
	if( mpSkaterStateComponent->GetState() == LIP )
	{
#		ifdef DEBUG_CAMERA
		printf ("SkaterCam Using Vert Cam as LIP\n");
#		endif
		return true;
	}
	
	if( mpSkaterStateComponent->GetFlag( VERT_AIR ) && !mpSkaterStateComponent->GetFlag( CAN_BREAK_VERT )  && !mpSkaterStateComponent->GetFlag(SPINE_PHYSICS))
	{
#		ifdef DEBUG_CAMERA
		printf ("SkaterCam Using Vert Cam as VERT_AIR\n");
#		endif
		return true;
	}
	
	if( mpSkaterStateComponent->GetJumpedOutOfLipTrick())
	{
#		ifdef DEBUG_CAMERA
		printf ("SkaterCam Using Vert Cam as Jumped out of lip trick\n");
#		endif
		return true;
	}

#	ifdef DEBUG_CAMERA
	printf ("Skatecam NOT using vert cam\n");
#	endif

	return false;	
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkaterCameraComponent::addShake( Mth::Vector& cam, Mth::Matrix& frame )
{
	float damping		= mShakeDuration / mShakeInitialDuration;
	float vert_angle	= mShakeDuration * mShakeVertVel * Mth::PI * 2.0f;
	float horiz_angle	= mShakeDuration * mShakeHorizVel * Mth::PI * 2.0f;
	float vert_offset	= mShakeMaxVertAmp * sinf( vert_angle ) * damping;
	float horiz_offset	= mShakeMaxHorizAmp * sinf( horiz_angle ) * damping;

	cam += frame[X] * horiz_offset;
	cam += frame[Y] * vert_offset;

	mShakeDuration		-= Tmr::FrameLength();
	if( mShakeDuration < 0.0f )
		mShakeDuration = 0.0f;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector CSkaterCameraComponent::GetTripodPos( bool instantly )
{
	float		lerp_xz;
	float		lerp_y;
	float		delta		= Tmr::FrameLength();

	if( mpSkater == NULL )
	{
		return Mth::Vector( 0.0f, 0.0f, 0.0f );
	}

	Mth::Vector now			= mpSkater->m_pos;

	// The tripod pos is always *tending* towards the skater position, but the rate at which it does so is
	// definable, to allow for some lag, which improves the feeling of speed.
	if( mLerpReductionTimer > 0.0f )
	{
		mLerpReductionTimer -= delta;
		if( mLerpReductionTimer > 0.0f )
		{
			mLerpReductionMult += ( mLerpReductionDelta * delta );
		}
		else
		{
			mLerpReductionMult	= 1.0f;
			mLerpReductionTimer	= 0.0f;
		}
	}
	else
	{
		mLerpReductionMult	= 1.0f;
	}

	Dbg_Assert( mpSkaterStateComponent );
	if(( mpSkaterStateComponent->GetFlag( VERT_AIR ) && !mpSkaterStateComponent->GetFlag( CAN_BREAK_VERT ) && !mpSkaterStateComponent->GetFlag( SPINE_PHYSICS )))
	{
		#ifdef	DEBUG_CAMERA
		printf ("Skatercam using mVertAirLerp\n");
		#endif

		lerp_xz	= GetTimeAdjustedSlerp( mVertAirLerpXZ * mLerpReductionMult, delta );
		lerp_y	= GetTimeAdjustedSlerp( mVertAirLerpY * mLerpReductionMult, delta );
	}
	else
	{
		lerp_xz	= GetTimeAdjustedSlerp( mLerpXZ * mLerpReductionMult, delta );
		lerp_y	= GetTimeAdjustedSlerp( mLerpY * mLerpReductionMult, delta );

		// Added the following check (Dave 7/16/02) to help eliminate the camera rattle when snapping on/off a curb.
		// The flag is set for one frame only.
		if( mpSkaterStateComponent->GetFlag( SNAPPED_OVER_CURB ))
		{
			lerp_y = 1.0f;
		}
		
		if( mpSkaterStateComponent->GetFlag( SNAPPED ))
		{
			instantly = true;
		}
	}

	if( instantly )
	{
		mLastTripodPos = now;
	}
	else
	{
		mLastTripodPos.Set( mLastTripodPos.GetX() + (( now.GetX() - mLastTripodPos.GetX() ) * lerp_xz ),
							mLastTripodPos.GetY() + (( now.GetY() - mLastTripodPos.GetY() ) * lerp_y ),
							mLastTripodPos.GetZ() + (( now.GetZ() - mLastTripodPos.GetZ() ) * lerp_xz ));
	}

	return mLastTripodPos;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkaterCameraComponent::CalculateZoom( float* p_above, float* p_behind )
{
	if( mpSkater == NULL )
	{
		return;
	}

	mTargetZoom = 1.0f;

	// Deal with zoom for Big Air. Set the flag only when a trick is started.
	if( !mBigAirTrickZoomActive && ( mpSkaterStateComponent->GetFlag( VERT_AIR ) && !mpSkaterStateComponent->GetFlag( CAN_BREAK_VERT ) && !mpSkaterStateComponent->GetFlag(SPINE_PHYSICS)) && mpSkaterStateComponent->DoingTrick() )
	{
		mBigAirTrickZoomActive = true;
	}
	else if( !( mpSkaterStateComponent->GetFlag( VERT_AIR ) && !mpSkaterStateComponent->GetFlag( CAN_BREAK_VERT ) && !mpSkaterStateComponent->GetFlag( SPINE_PHYSICS )))
	{
		// Must have landed.
		mBigAirTrickZoomActive = false;
	}

	if( mBigAirTrickZoomActive )
	{
		mTargetZoom = mBigAirTrickZoom;
	}
	else
	{
		EStateType state = mpSkaterStateComponent->GetState();

		// If the skater is grinding, zoom also.
		if( state == RAIL )
		{
			mTargetZoom = mGrindZoom;
		}
		else if( state == LIP )
		{
			mTargetZoom = mLipTrickZoom;
		}
		else if( state == LIP )
		{
			mTargetZoom = mLipTrickZoom;
		}
	}

	
	#if 0
	// If lookaround override is set, factor in the lookaround override zoom.
	if( mp_lookaround_component->mLookaroundOverride && ( mp_lookaround_component->mLookaroundZoom != 1.0f ))
	{
		mCurrentZoom = mTargetZoom;
		mCurrentZoom *= mp_lookaround_component->mLookaroundZoom;
	}
	else
	{
		mCurrentZoom += (( mTargetZoom - mCurrentZoom ) * mZoomLerp );
	}
	#else
	// If lookaround override is set, factor in the lookaround override zoom.
	if( mp_lookaround_component->mLookaroundOverride && ( mp_lookaround_component->mLookaroundZoom != 1.0f ))
	{
		mTargetZoom *= mp_lookaround_component->mLookaroundZoom;
	}
	
	mCurrentZoom += (( mTargetZoom - mCurrentZoom ) * mZoomLerp );
	#endif
	
	*p_behind = mBehind * mCurrentZoom;

	// Behind is also shortened when the lookaround camera is tilting upwards.
	if( mp_lookaround_component->mLookaroundTilt < 0.0f )
	{
		float max_tilt = 3.14f * 0.2f;
		*p_behind = *p_behind * ( 0.4f + ( 0.6f * (( max_tilt + mp_lookaround_component->mLookaroundTilt ) / max_tilt )));
	}

	// Use lip_trick_above when doing a lip trick.
	float above_val = mAbove;
	if( mpSkaterStateComponent->GetState() == LIP )
	{
		above_val = mLipTrickAbove;
	}
	
	// Figure above tending towards the perfect above, if zoom is < 1.0.
	if( mCurrentZoom < 1.0f )
	{
		*p_above = SKATERCAMERACOMPONENT_PERFECT_ABOVE + (( above_val - SKATERCAMERACOMPONENT_PERFECT_ABOVE ) * mCurrentZoom );
	}
	else
	{
		*p_above = above_val;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkaterCameraComponent::UpdateInterpolators(   )
{
	if( mBehindInterpolatorTime > 0.0f )
	{
		mBehindInterpolatorTime -= SKATERCAMERACOMPONENT_INTERPOLATOR_TIMESTEP;
		mBehind += mBehindInterpolatorDelta;		
	}
	else
	{
		mBehindInterpolatorTime = 0.0f;
	}

	if( mAboveInterpolatorTime > 0.0f )
	{
		mAboveInterpolatorTime -= SKATERCAMERACOMPONENT_INTERPOLATOR_TIMESTEP;
		mAbove += mAboveInterpolatorDelta;		
	}
	else
	{
		mAboveInterpolatorTime = 0.0f;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkaterCameraComponent::SetLerpReductionTimer( float time )
{
	if( time > 0.0f )
	{
		mLerpReductionTimer	= time;
		mLerpReductionDelta	= 1.0f / time;
		mLerpReductionMult	= 0.0f;
	}
	else
	{
		mLerpReductionTimer	= 0.0f;
		mLerpReductionDelta	= 0.0f;
		mLerpReductionMult	= 1.0f;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkaterCameraComponent::SetSkater( CSkater* p_skater )
{
	mpSkater = p_skater;
	
	if( p_skater == NULL )
	{
		mpSkaterStateComponent = NULL;
		mpSkaterPhysicsControlComponent = NULL;
	}
	else
	{
		mpSkaterStateComponent = GetSkaterStateComponentFromObject(mpSkater);
		Dbg_Assert(mpSkaterStateComponent);
		
		// non-local clients will not have a CSkaterPhysicsControlComponent
		mpSkaterPhysicsControlComponent = GetSkaterPhysicsControlComponentFromObject(mpSkater);
		
		SetNoLerpUpdate(true);
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CSkater* CSkaterCameraComponent::GetSkater( void )
{
	return mpSkater;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkaterCameraComponent::GetCameraState ( SCameraState& state )
{
	state.lastActualMatrix[X] = mLastActualRight;
	state.lastActualMatrix[Y] = mLastActualUp;
	state.lastActualMatrix[Z] = mLastActualAt;
	state.lastActualMatrix[W].Set( 0.0f, 0.0f, 0.0f, 1.0f );
	state.lastTripodPos = mLastTripodPos;
	state.lastDot = mLastDot;
	state.lastZoom = mCurrentZoom;
}
	
}
