//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       skatercam.cpp
//* OWNER:          Dave Cowling
//* CREATION DATE:  02/05/2001
//****************************************************************************

#ifndef __OBJECTS_SKATERCAM_H
#define __OBJECTS_SKATERCAM_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

#include <core/math/math.h>
#include <core/math/matrix.h>
#include <core/math/vector.h>

#include <gfx/camera.h>

/*****************************************************************************
**							Forward Declarations							**
*****************************************************************************/

namespace Mth
{
    class SlerpInterpolator;
};

#if 0
namespace NxPlugin
{
	class CameraHeader;
};
#endif
                   
namespace Obj
{
    class CSkater;
    class CSkaterCorePhysicsComponent;
    class CSkaterStateComponent;

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

class CSkaterCam : public Gfx::Camera
{
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
	};

public:

					CSkaterCam( int player );
					~CSkaterCam( void );
	bool			UseVertCam();
	float			Update( bool instantly = false );
	bool			IsAnimFinished( void );
	bool			IsAnimHeld( void );
	void			SetAnimSkippable( bool skippable );
	void			SwitchReplayMode( bool increment );
	void			SetMode( ESkaterCamMode mode, float time = 1.0f );
	int				GetMode( void )								{ return (int)mMode; }
	void			SetLookaround( float heading, float tilt, float time, float zoom = 1.0f );
	void			ClearLookaround( void );
	void			ClearLookaroundLock( void )							{ mLookaroundLock = false; }
	void			SetShake( float duration, float vert_amp, float horiz_amp, float vert_vel, float horiz_vel );
	void			EnableInputHandler( bool enable );
	void			SetSkater( CSkater* skater );
	void			SetLerpReductionTimer( float time );
	CSkater*		GetSkater( void );
	void 			ResetLookAround( void );

	void			BindToController( int controller );

	// needed for proxim.cpp (each camera stores its position 
	// after writing that position to render camera)
//	Mth::Vector		m_pos;

	bool			mSkipButton;
	bool			mRightButton;
	int				mRightX;
	int				mRightY;
	ESkaterCamMode	mMode;
	
private:

	Mth::Vector		GetTripodPos( bool instantly = false );
	void			UpdateInterpolators( void );
	void			CalculateZoom( float* p_above, float* p_behind );
	void			addShake( Mth::Vector& cam, Mth::Matrix& frame );

	Mth::SlerpInterpolator*	mpSlerp;

	CSkater*		mpSkater;
	CSkaterCorePhysicsComponent*	mpSkaterPhysicsComponent;
	CSkaterStateComponent*	mpSkaterStateComponent;

	float			mLastDot;
	Mth::Vector		mLastTripodPos;

	static						Inp::Handler< CSkaterCam >::Code s_input_logic_code;
	Inp::Handler< CSkaterCam >*	m_input_handler;
//	static						Inp::Handler< CSkaterCam >::Code s_input_logic_code_special;
//	Inp::Handler< CSkaterCam >*	m_input_handler_special;

	bool			m_input_enabled;
	
	int				mInstantCount;				// Used to maintain the 'instantly' flag during update for several frames.

	float			mHorizFOV;

	Mth::Vector		mLastActualRight;			// Used because the frame is adjusted after focus LERPING.
	Mth::Vector		mLastActualUp;
	Mth::Vector		mLastActualAt;

	Mth::Matrix		mLastPreWallSkaterMatrix;	// Used because we don't want to use the skater's matrix during wall rides.

	float			mBehind;
	float			mBehindInterpolatorTime;
	float			mBehindInterpolatorDelta;

	float			mAbove;
	float			mAboveInterpolatorTime;
	float			mAboveInterpolatorDelta;

	bool			mBigAirTrickZoomActive;
	float			mCurrentZoom;
	float			mTargetZoom;
	float			mZoomLerp;				// LERP rate for zoom.
	float			mBigAirTrickZoom;		// Target zoom for big air trick.
	float			mGrindZoom;				// Target zoom for grind.
	float			mLipTrickZoom;			// Target zoom for lip trick.
	int				mLipSideDecided;

	float			mVertAirLandedTimer;	// Defines how long the vert air landed slerp gets applied.

	float			mLerpReductionTimer;
	float			mLerpReductionDelta;
	float			mLerpReductionMult;

	bool			mLookaroundLock;
	bool			mLookaroundOverride;	// For when the designer is scripting the lookaround values.
	float			mLookaroundZoom;		// Allows designers to adjust the zoom when overrideing the lookaround values.
	float			mLookaroundTilt;
	float			mLookaroundHeading;
	float			mLookaroundHeadingStartingPoint;
	float			mLookaroundTiltTarget;
	float			mLookaroundHeadingTarget;
	float			mLookaroundTiltDelta;
	float			mLookaroundHeadingDelta;
	float			mLookaroundDeltaTimer;

	float			mTilt;
	float			mTiltAddition;
	float			mLipTrickTilt;
	float			mLipTrickAbove;
	float			mSlerp;
	float			mVertAirSlerp;
	float			mVertAirLandedSlerp;
	float			mOriginOffset;

	float			mShakeDuration;			// Shake duration in seconds (0.0 means no shake).
	float			mShakeInitialDuration;
	float			mShakeMaxVertAmp;
	float			mShakeMaxHorizAmp;
	float			mShakeVertVel;
	float			mShakeHorizVel;
	
	float			mLean;					// For lean during grind imbalance.

	float			mLerpXZ;
	float			mLerpY;
	float			mVertAirLerpXZ;
	float			mVertAirLerpY;
	float			mGrindLerp;
};
	
/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace Obj

#endif	// __OBJECTS_SKATERCAM_H
