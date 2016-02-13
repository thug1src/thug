//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       SkaterCameraComponent.h
//* OWNER:          Dave Cowling
//* CREATION DATE:  02/21/03
//****************************************************************************

#ifndef __COMPONENTS_SKATERCAMERACOMPONENT_H__
#define __COMPONENTS_SKATERCAMERACOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>
#include <core/math/slerp.h>

#include <gel/inpman.h>
#include <gel/object/basecomponent.h>
#include <gel/components/camerautil.h>

#include <sk/objects/skater.h>

// Replace this with the CRCD of the component you are adding.
#define		CRC_SKATERCAMERA							CRCD( 0x5e43a604, "SkaterCamera" )

//  Standard accessor macros for getting the component either from within an object, or given an object.
#define		GetSkaterCameraComponent()					((Obj::CSkaterCameraComponent*)GetComponent( CRC_SKATERCAMERA ))
#define		GetSkaterCameraComponentFromObject( pObj )	((Obj::CSkaterCameraComponent*)(pObj)->GetComponent( CRC_SKATERCAMERA ))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CInputComponent;
	class CSkaterStateComponent;
	class CSkaterPhysicsControlComponent;
	class CCameraLookAroundComponent;

class CSkaterCameraComponent : public CBaseComponent
{
	friend class CWalkCameraComponent;
	
public:

	enum ESkaterCamMode
	{
		SKATERCAM_MODE_UNDEFINED		= 0,
		SKATERCAM_MODE_FIRST_VALID		= 1,
		SKATERCAM_MODE_NORMAL_NEAR		= 1,
		SKATERCAM_MODE_NORMAL_MEDIUM,
		SKATERCAM_MODE_NORMAL_FAR,
		SKATERCAM_MODE_NORMAL_MEDIUM_LTG,
        SKATERCAM_NUM_NORMAL_MODES,
		SKATERCAM_FIRST_REPLAY_MODE = SKATERCAM_NUM_NORMAL_MODES,
		SKATERCAM_MODE_REPLAY_FRONT = SKATERCAM_NUM_NORMAL_MODES,
		SKATERCAM_MODE_REPLAY_FRONT_ZOOM,
        SKATERCAM_MODE_REPLAY_LEFT,
		SKATERCAM_MODE_REPLAY_LEFT_ZOOM,
		SKATERCAM_MODE_REPLAY_BEHIND,
		SKATERCAM_MODE_REPLAY_BEHIND_ZOOM,
		SKATERCAM_MODE_REPLAY_RIGHT,
		SKATERCAM_MODE_REPLAY_RIGHT_ZOOM,
		SKATERCAM_LAST_REPLAY_MODE = SKATERCAM_MODE_REPLAY_RIGHT_ZOOM,
		SKATERCAM_NUM_MODES,
		SKATERCAM_MODE_ANIM_PLAYBACK,
	};
	
											CSkaterCameraComponent();
    virtual									~CSkaterCameraComponent();

public:
    virtual void            				Update();
    virtual void            				InitFromStructure( Script::CStruct* pParams );
    virtual void            				RefreshFromStructure( Script::CStruct* pParams );
	virtual void							Finalize();
    
    virtual EMemberFunctionResult   		CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 							GetDebugInfo( Script::CStruct* p_info );

	Mth::Vector								GetTripodPos( bool instantly = false );
	void									UpdateInterpolators(   );
	void									CalculateZoom( float* p_above, float* p_behind );
	void									addShake( Mth::Vector& cam, Mth::Matrix& frame );

	void									SetNoLerpUpdate( bool instant )	{ if( instant ) mInstantCount = 3; }
	void									Commit( void );

	void									SetShake( float duration, float vert_amp, float horiz_amp, float vert_vel, float horiz_vel );
	bool									UseVertCam( void );
	void									ActivateLerpIntoBehavior ( float duration );

	void									ResetLookAround( void );
	void									ResetMode( );
	void									SetMode( ESkaterCamMode mode, float time = 1.0f );
	void									ToggleMode( );
	ESkaterCamMode							GetMode( void )								{ return mMode; }
	void									SetLerpReductionTimer( float time );
	void									SetSkater( CSkater* p_skater );
	CSkater*								GetSkater( void );
	
	void									ReadyForActivation ( const SCameraState& state );
	void									GetCameraState ( SCameraState& state );

	static CBaseComponent*					s_create();

	// These functions access the CCameraComponent which is required to be attached.
	void									Enable( bool enable );
	Gfx::Camera*							GetCamera( void );
	const Mth::Vector &						GetPosition( void ) const;
	void									SetPosition( Mth::Vector& pos );
	const Mth::Matrix&						GetMatrix( void ) const;

	bool									mSkipButton;
	bool									mRightButton;
	int										mRightX;
	int										mRightY;

private:
	ESkaterCamMode							mMode;

	int										mInstantCount;				// Used to maintain the 'instantly' flag during update for several frames.

	float									mLastDot;
	Mth::Vector								mLastTripodPos;
	Mth::Vector								mLastActualRight;			// Used because the frame is adjusted after focus LERPING.
	Mth::Vector								mLastActualUp;
	Mth::Vector								mLastActualAt;

	Mth::Matrix								mLastPreWallSkaterMatrix;	// Used because we don't want to use the skater's matrix during wall rides.

	float									mBehind;
	float									mBehindInterpolatorTime;
	float									mBehindInterpolatorDelta;
	float									mAbove;
	float									mAboveInterpolatorTime;
	float									mAboveInterpolatorDelta;

	float									mVertAirLandedTimer;		// Defines for how long the vert air landed slerp gets applied.

	float									mHorizFOV;

	float									mLerpXZ;
	float									mLerpY;
	float									mVertAirLerpXZ;
	float									mVertAirLerpY;
	float									mGrindLerp;

	float									mLerpReductionTimer;
	float									mLerpReductionDelta;
	float									mLerpReductionMult;

	bool									mBigAirTrickZoomActive;
	float									mCurrentZoom;
	float									mTargetZoom;
	float									mZoomLerp;				// LERP rate for zoom.
	float									mBigAirTrickZoom;		// Target zoom for big air trick.
	float									mGrindZoom;				// Target zoom for grind.
	float									mLipTrickZoom;			// Target zoom for lip trick.
	int										mLipSideDecided;

	float									mLean;					// For lean during grind imbalance.

	float									mTilt;
	float									mTiltAddition;
	float									mLipTrickTilt;
	float									mLipTrickAbove;
	float									mWalkSlerp;
	float									mSlerp;
	float									mVertAirSlerp;
	float									mVertAirLandedSlerp;
	float									mOriginOffset;

	float									mShakeDuration;			// Shake duration in seconds (0.0 means no shake).
	float									mShakeInitialDuration;
	float									mShakeMaxVertAmp;
	float									mShakeMaxHorizAmp;
	float									mShakeVertVel;
	float									mShakeHorizVel;
	
	CCameraLookAroundComponent*				mp_lookaround_component;

	Mth::SlerpInterpolator*					mpSlerp;
	CSkater*								mpSkater;
	CSkaterStateComponent*					mpSkaterStateComponent;
	CSkaterPhysicsControlComponent*			mpSkaterPhysicsControlComponent;
};

}

#endif
