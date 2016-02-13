//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       skatercam.cpp
//* OWNER:          Dave Cowling
//* CREATION DATE:  02/05/2001
//****************************************************************************

/*

(Mick) 
Problem:

If you stop, and then jump in place while holding "down", you will lean backwards while 
you jump.  This will not really be noticable, but when you land, the camera will seem to 
snap backwards for a single frame

the problem was m_display_matrix is lagging behind
so I have patched it by setting skater_up to mpSkate->m_display_normal when on the ground
this fixes it for the skater glitching, however the world around him is still glitching
as the angle still changes abruptly

one fix might be to somehow smoothly interpolate this angle , which might also help 
with any fix we do for skating off ledges

I need to clean up and comment this code, so it can be modified without fear.....


*/


//#define	DEBUG_CAMERA

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <gel/mainloop.h>

#include <core/math/slerp.h>

#include <objects/skater.h>
#include <objects/skaterflags.h>
#include <objects/skatercam.h>
#include <components/skatercorephysicscomponent.h>
#include <components/skaterbalancetrickcomponent.h>
#include <components/skaterstatecomponent.h>
#include <gfx/nxviewman.h>
#include <gfx/debuggfx.h>

#include <gel/scripting/symboltable.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/script.h>
#include <gel/components/movablecontactcomponent.h>

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								DBG Defines									**
*****************************************************************************/
		
		
static void _xrotlocal ( Mth::Matrix& m, float a )
{
	m.RotateXLocal( a );
}

namespace Obj
{

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

	const float LEAN_TO_SKATERCAM_LEAN	= (( Mth::PI / 6.0f ) / -4096.0f );
	const float	SKATERCAM_INTERPOLATOR_TIMESTEP = ( 1.0f / 60.0f );

	const float VERT_AIR_LANDED_TIME	= 10.0f / 60.0f;
	const float TILT_MAX				= 0.34907f;						// 20 degees.
	const float TILT_MIN				= -0.34907f;					// -20 degees.
	const float TILT_INCREMENT			= 2.0f * ( TILT_MAX / 60.0f );	// 40 degrees/second.
	const float TILT_DECREMENT			= 2.0f * ( TILT_MAX / 60.0f );	// 40 degrees/second.
	const float TILT_RESTORE			= 8.0f * ( TILT_MAX / 60.0f );	// 160 degrees/second.

	// This is the value that mAbove should tend to when the behind value is closer than mBehind.
	const float PERFECT_ABOVE = 3.0f;

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCam::s_input_logic_code( const Inp::Handler < CSkaterCam >& handler )
{    
    Dbg_AssertType( &handler, Inp::Handler< CSkaterCam > );
	
	bool skip_button;
	bool right_button;
	int right_x;
	int right_y;

    CSkaterCam&	obj = handler.GetData();
	
	if ( obj.mpSkater && !obj.mpSkater->IsLocalClient( ) )
	{
		obj.mSkipButton = handler.m_Input->m_Makes & ( Inp::Data::mD_X );
		obj.mRightButton = handler.m_Input->m_Makes & ( Inp::Data::mD_R3 );
		obj.mRightX = handler.m_Input->m_Event[Inp::Data::vA_RIGHT_X] - 128;
		obj.mRightY = handler.m_Input->m_Event[Inp::Data::vA_RIGHT_Y] - 128;
	}
	skip_button = obj.mSkipButton;
	right_button = obj.mRightButton;
	right_x = obj.mRightX;
	right_y = obj.mRightY;

	if (obj.m_input_enabled)
	{
		if( right_button )
		{
			obj.mLookaroundLock = !obj.mLookaroundLock;
		}
	
		if( !obj.mLookaroundLock && !obj.mLookaroundOverride )
		{
			// We want this to map to (PI/2, ~-2PI/10). Switched this 7/10/01 to be opposite up/down to previously.
			float target = ((float)(( right_y < 0 ) ? right_y : ( 0.4f * right_y ))) * ( -1.4f / 128.0f );
			obj.mLookaroundTilt += ( target - obj.mLookaroundTilt ) * 0.0625f;
	
			// We want this to map to (-~PI, ~PI).
			target = ((float)right_x ) * ( 3.0f / 128.0f );
			obj.mLookaroundHeading	+= ( target - obj.mLookaroundHeading ) * 0.0625f;
		}
	
/*		Hey -- Let's not mask these since I need to read them in the player's s_input_logic function
		for replay stuff.  Thanks.  If it breaks something please tell me.  Love Matt
		// Mask off analog right stick input.
		handler.m_Input->MaskDigitalInput( Inp::Data::mD_R3 );
		handler.m_Input->MaskAnalogInput( Inp::Data::mA_RIGHT_X );
		handler.m_Input->MaskAnalogInput( Inp::Data::mA_RIGHT_Y );
*/
	}
}

/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterCam::CSkaterCam( int player )
{
	mMode	= SKATERCAM_MODE_UNDEFINED;

#ifdef	DEBUG_CAMERA
	printf ("+++ SkaterCam Created ..........................\n");				  
#endif
				  
	// Create the SLERP interpolator here.
	mpSlerp = new Mth::SlerpInterpolator;

	// Set current zoom and LERP rates.
	mCurrentZoom		= 1.0f;
	mZoomLerp			= 0.0625f;
	mLastDot			= 1.0f;

	mLerpXZ			= 0.25f;
	mLerpY			= 0.5f;
	mVertAirLerpXZ	= 1.0f;
	mVertAirLerpY	= 1.0f;
	mGrindLerp		= 0.1f;

	mLookaroundTilt		= 0.0f;
	mLookaroundHeading	= 0.0f;
	mLookaroundZoom		= 1.0f;

	Inp::Manager * inp_manager = Inp::Manager::Instance();

	// Register input task.
	m_input_handler			= new Inp::Handler< CSkaterCam >( player, s_input_logic_code, *this, Tsk::BaseTask::Node::vNORMAL_PRIORITY + 1 );
//	m_input_handler_special	= new Inp::Handler< CSkaterCam >( player, s_input_logic_code_special, *this, Tsk::BaseTask::Node::vNORMAL_PRIORITY + 1 );

	m_input_enabled = true;
	inp_manager->AddHandler( *m_input_handler );

    // clear the frame matrix
    // so that future slerps make sense
    Mth::Matrix& frame_matrix = Gfx::Camera::GetMatrix();
    frame_matrix.Ident();

	mLastActualRight 	= frame_matrix[X];
	mLastActualUp 		= frame_matrix[Y];
	mLastActualAt 		= frame_matrix[Z];
	
	Dbg_MsgAssert(false, ("CSkaterCam is not supported."));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterCam::~CSkaterCam( void )
{
#ifdef	DEBUG_CAMERA
	printf ("--- SkaterCam Destroyed ..........................\n");				  
#endif

	if( mpSlerp )
	{
		delete mpSlerp;
	}

	#if 0
	// for cutscene camera

	if( mpCopy )
	{
		Mem::Free( mpCopy );
		mpCopy = NULL;
	}
	#endif

	// Unregister input task.
	if( m_input_handler )
	{
		if( m_input_handler->InList() )
		{
			m_input_handler->Remove();
		}
		delete m_input_handler;
	}
/*	
	if( m_input_handler_special )
	{
		if( m_input_handler_special->InList() )
		{
			m_input_handler_special->Remove();
		}
		delete m_input_handler_special;
	}
*/
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCam::EnableInputHandler( bool enable )
{
	m_input_enabled = enable;

/*
	if( enable )
	{
		// Remove the special handler and add the standard handler.
		if( m_input_handler_special->InList() )
		{
			m_input_handler_special->Remove();
		}
		if( !m_input_handler->InList() )
		{
			Inp::Manager * inp_manager = Inp::Manager::Instance();
			inp_manager->AddHandler( *m_input_handler );
		}
	}
	else
	{
		// Remove the standard handler and add the special handler.
		if( m_input_handler->InList() )
		{
			m_input_handler->Remove();
		}
		if( !m_input_handler_special->InList() )
		{
			Inp::Manager * inp_manager = Inp::Manager::Instance();
			inp_manager->AddHandler( *m_input_handler_special );
		}

	}
*/
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCam::ResetLookAround( void )
{
	mLookaroundTilt		= 0.0f;
	mLookaroundHeading	= 0.0f;

	Update( true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// toggle between replay modes:
void CSkaterCam::SwitchReplayMode( bool increment )  // true to increment, false to decrement
{
	
	int mode = ( int )mMode;
	if ( increment )
	{
		mode++;
	}
	else
	{
		mode--;
	}
	if ( mode < SKATERCAM_FIRST_REPLAY_MODE )
	{
		mode = SKATERCAM_LAST_REPLAY_MODE;
	}
	else if ( mode > SKATERCAM_LAST_REPLAY_MODE )
	{
		mode = SKATERCAM_FIRST_REPLAY_MODE;
	}
	SetMode( ( ESkaterCamMode )mode, 0.0f );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCam::SetMode( ESkaterCamMode mode, float time )
{
	

#if 0	// Garrett: This doesn't work because it doesn't check to see if the screen mode has changed
	if( mode == mMode )
	{
		// Nothing to do here.
		return;
	}
#endif

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
	p_struct->GetFloat( "horiz_fov",				&h_fov );
	p_struct->GetFloat( "behind",					&behind );
	p_struct->GetFloat( "above",					&above );
	p_struct->GetFloat( "tilt",						&tilt );
	p_struct->GetFloat( "origin_offset",			&origin_offset );
	p_struct->GetFloat( "lip_trick_tilt",			&mLipTrickTilt );
	p_struct->GetFloat( "lip_trick_above",			&mLipTrickAbove );

	// Get SLERP values.
	p_struct->GetFloat( "slerp",					&mSlerp );
	p_struct->GetFloat( "vert_air_slerp",			&mVertAirSlerp );
	p_struct->GetFloat( "vert_air_landed_slerp",	&mVertAirLandedSlerp );

	// Get LERP values.
	p_struct->GetFloat( "lerp_xz",					&mLerpXZ );
	p_struct->GetFloat( "lerp_y",					&mLerpY );
	p_struct->GetFloat( "vert_air_lerp_xz",			&mVertAirLerpXZ );
	p_struct->GetFloat( "vert_air_lerp_y",			&mVertAirLerpY );
	p_struct->GetFloat( "grind_lerp",				&mGrindLerp );

	// Get zoom values.
	mGrindZoom			= 1.0f;
	mLipTrickZoom		= 1.0f;
	mBigAirTrickZoom	= 1.0f;

	p_struct->GetFloat( "zoom_lerp",			&zoom_lerp );
	p_struct->GetFloat( "grind_zoom",			&mGrindZoom );
	p_struct->GetFloat( "big_air_trick_zoom",	&mBigAirTrickZoom );
	p_struct->GetFloat( "lip_trick_zoom",		&mLipTrickZoom );
	mZoomLerp			= zoom_lerp;

	// Get camera style name.
	p_struct->GetText( "name", &p_name );

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
		mBehindInterpolatorDelta	= (( behind - mBehind ) * ( SKATERCAM_INTERPOLATOR_TIMESTEP / time ));
		mAboveInterpolatorTime		= time;
		mAboveInterpolatorDelta		= (( above - mAbove ) * ( SKATERCAM_INTERPOLATOR_TIMESTEP / time ));
	}
	else
	{
		mBehind						= FEET( behind );
		mAbove						= FEET( above );
		mBehindInterpolatorTime		= 0.0f;
		mAboveInterpolatorTime		= 0.0f;
	}

	// Set focal length immediately.
	if( this )
	{
		Nx::ScreenMode	mode = Nx::CViewportManager::sGetScreenMode();

		switch( mode )
		{
			case Nx::vSPLIT_V:
			{
#ifdef __PLAT_NGC__
				//Gfx::Camera::SetFocalLength( Script::GetFloat("Skater_Cam_Horiz_FOV"), Nx::vBASE_ASPECT_RATIO );
				Gfx::Camera::SetHFOV(Script::GetFloat("Skater_Cam_Horiz_FOV") / 2.0f);
#else
				//Gfx::Camera::SetFocalLength( mFocalLength, Nx::vBASE_ASPECT_RATIO * 0.5f );
				Gfx::Camera::SetHFOV(mHorizFOV / 2.0f);
#endif // __PLAT_NGC__
				break;
			}

			case Nx::vSPLIT_H:
			{
				//Gfx::Camera::SetFocalLength( mFocalLength * 2.0f, Nx::vBASE_ASPECT_RATIO * 2.0f );
				Gfx::Camera::SetHFOV(mHorizFOV);
				break;
			}

			default:
			{			
				//Gfx::Camera::SetFocalLength( mFocalLength, Nx::vBASE_ASPECT_RATIO );
				Gfx::Camera::SetHFOV(mHorizFOV);
				break;
			}
		}
	}

	switch ( mode )
	{
		case ( SKATERCAM_MODE_REPLAY_FRONT ):
		case ( SKATERCAM_MODE_REPLAY_FRONT_ZOOM ):
			mLookaroundHeadingStartingPoint = Mth::PI;
			break;
		case ( SKATERCAM_MODE_REPLAY_LEFT ):
		case ( SKATERCAM_MODE_REPLAY_LEFT_ZOOM ):
			mLookaroundHeadingStartingPoint = Mth::PI / 2.0f;
			break;
		case ( SKATERCAM_MODE_REPLAY_BEHIND ):
		case ( SKATERCAM_MODE_REPLAY_BEHIND_ZOOM ):
			mLookaroundHeadingStartingPoint = 0;
			break;
		case ( SKATERCAM_MODE_REPLAY_RIGHT ):
		case ( SKATERCAM_MODE_REPLAY_RIGHT_ZOOM ):
			mLookaroundHeadingStartingPoint = -( Mth::PI / 2.0f );
			break;
		default:
			mLookaroundHeadingStartingPoint = 0;
			break;
	}   
}

/*******************************************************************************************************
logic for getTimeAdjustedSlerp()

If the camera is lagging a distance D behind the target, and the target is moving with velocity V, and the lerp is set to L.

Then when the camera is stable, it will be moving at the same rate as the target (V), and since the distance it will move is equal to the lerp multiplied by the distance from the target (which will be the old distance plus the new movement of the target V)

Then:

L*(D+V) = V 

L*D + L*V = V
D = V * (1 - L) / L

Assuming in the above T = 1, then if we have a variable T, the speed of the skater will be T*V, yet we want the distance (Dt) moved to remain unchanged (D), so for a time adjusted lerp Lt

Dt = T * V * (1-Lt)/Lt

Since D = Dt

V * (1 - L) / L = T * V * (1-Lt)/Lt

V cancels out, and we get

Lt = TL / (1 - L + TL)

Sanity check,  

if L is 0.25, and T is 1, then Lt = 1*0.25 / (1 - 0.25 + 1*0.25)  = 0.25
if L is 0.25, and T is 2, then Lt = 2*0.25 / (1 - 0.25 + 2*0.25)  = 0.5 / 1.25 = 0.40
if L is 0.25, and T is 5, then Lt = 5*0.25 / (1 - 0.25 + 5*0.25)  = 1.25 / 2 = 0.625

Sounds about right.

*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static float getTimeAdjustedSlerp( float slerp, float delta )
{
#if	1
	// Mick's method, tries to guarantee that the distance from the target point
	// will never alter at constant velocity, with changing frame rate
	// Lt = TL / (1 - L + TL)
	
	float t = delta * 60.0f;
	float Lt =  t * slerp / (1.0f - slerp + t * slerp);
	return Lt;	
	
#else
	// Dave's method. simulates the effect of lerp over multiple frames
	// curiously seems to be within 2% of the above method.  

	float result	= 0.0f;
	float remainder	= 1.0f;

	while( true )
	{
		if( delta >= ( 1.0f / 60.0f ))
		{
			delta -= ( 1.0f / 60.0f );

			float extra = slerp * remainder;
			remainder -= extra;
			result += extra;
		}
		else
		{
			// Partial fragment, or nothing at all?
			if( delta > 0.0f )
			{
				// The slerp this time will be reduced.
				slerp *= delta / ( 1.0f / 60.0f );

				float extra = slerp * remainder;
				remainder -= extra;
				result += extra;
			}
//			printf ("Result %f, Lt %f, diff = %f\n",result,Lt,result-Lt);
			return result;
		}
	}
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCam::SetLookaround( float heading, float tilt, float time, float zoom )
{
	mLookaroundOverride			= true;

	mLookaroundHeadingTarget	= heading;
	mLookaroundTiltTarget		= tilt;

	// Calculate delta - the amount to move in 1 second.
	if( time > 0.0f )
	{
		mLookaroundHeadingDelta		= ( heading - mLookaroundHeading ) / time;
		mLookaroundTiltDelta		= ( tilt - mLookaroundTilt ) / time;
	}

	mLookaroundDeltaTimer		= time;
	mLookaroundZoom				= zoom;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkaterCam::ClearLookaround( void )
{
	mLookaroundOverride = false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkaterCam::SetShake( float duration, float vert_amp, float horiz_amp, float vert_vel, float horiz_vel )
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

// Added by Ken. 
bool CSkaterCam::UseVertCam( void )
{   
	// Use vert cam when in the lip state.
	if( mpSkaterStateComponent->GetState() == LIP )
	{
#ifdef	DEBUG_CAMERA
		printf ("SkaterCam Using Vert Cam as LIP\n");
#endif
		return true;
	}	
		
	if( mpSkaterStateComponent->GetFlag( VERT_AIR ) && !mpSkaterStateComponent->GetFlag( CAN_BREAK_VERT )  && !mpSkaterStateComponent->GetFlag(SPINE_PHYSICS))
	{
#ifdef	DEBUG_CAMERA
		printf ("SkaterCam Using Vert Cam as VERT_AIR\n");
#endif
		return true;
	}
	
	if( mpSkaterStateComponent->GetJumpedOutOfLipTrick())
	{
#ifdef	DEBUG_CAMERA
		printf ("SkaterCam Using Vert Cam as Jumped out of lip trick\n");
#endif
		return true;
	}

#ifdef	DEBUG_CAMERA
		printf ("Skatecam NOT using vert cam\n");
#endif


	return false;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
					 
void CSkaterCam::addShake( Mth::Vector& cam, Mth::Matrix& frame )
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

// updates the camera position
// returns the dist from the camera to the focus point 					 
float CSkaterCam::Update( bool instantly )
{
	
	if( instantly )
	{
		mInstantCount = 3;
	}

	if( mInstantCount > 0 )
	{
		--mInstantCount;
		instantly = true;
	}

	// Length of last frame in seconds.
	float delta_time = Tmr::FrameLength();

	if (mpSkater && mpSkater->mPaused)	  	// Mick: early out of ithe skater is paused, just allow scripted cameras
	{
		//return 100000.0f;
	}

	// Update any values we are currently interpolating.
	UpdateInterpolators();

		// Would have used a SetPos() function, but the camera update in rwviewer.cpp is spread out all over the place,
		// so it's not easy to do. We'll just store the old position before updating anything...
		Gfx::Camera::StoreOldPos();

		Mth::Matrix& frame_matrix	= Gfx::Camera::GetMatrix();

		frame_matrix[X]   		= mLastActualRight;
		frame_matrix[Y]			= mLastActualUp;
		frame_matrix[Z]			= mLastActualAt;

		Mth::Vector	current_at_vector( frame_matrix[Z][X], frame_matrix[Z][Y], frame_matrix[Z][Z] ); 

		// Obtain skater state.
		EStateType	state			= mpSkaterStateComponent->GetState();
		Mth::Matrix target;
		if( state == WALL )
		{
#ifdef	DEBUG_CAMERA
		printf ("SkaterCam Using mLastPreWallSkaterMatrix, as wallriding\n");
#endif
			target						= mLastPreWallSkaterMatrix;
		}
		else
		{
#ifdef	DEBUG_CAMERA
		printf ("SkaterCam Using GetCameraDisplayMatrix\n");
#endif
			target						= mpSkater->GetDisplayMatrix();
			mLastPreWallSkaterMatrix	= target;
		}

		// Lerp towards the required lean.
		float cam_lean = 0.0f;
		if( state == RAIL )
		{		
			// Need to translate the lean value [-4096, 4096] into a reasonable camera lean angle.
			cam_lean = GetSkaterBalanceTrickComponentFromObject(mpSkater)->mGrind.mManualLean * LEAN_TO_SKATERCAM_LEAN;
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
			skater_up	= mpSkaterPhysicsComponent->m_display_normal;
		}

//		Gfx::AddDebugLine( p_skater->m_pos, ( target[X] * 72.0f ) + p_skater->m_pos, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 4 );
//		Gfx::AddDebugLine( p_skater->m_pos, ( target[Y] * 72.0f ) + p_skater->m_pos, MAKE_RGB( 0, 128, 0 ), MAKE_RGB( 0, 128, 0 ), 4 );
//		Gfx::AddDebugLine( p_skater->m_pos, ( target[Z] * 72.0f ) + p_skater->m_pos, MAKE_RGB( 0, 0, 128 ), MAKE_RGB( 0, 0, 128 ), 4 );
		

		// Use vel for forward facing, if moving in xz plane.
		Mth::Vector vel_xz( mpSkater->GetVel().GetX(), 0.0f, mpSkater->GetVel().GetZ());

		bool use_vert_cam = UseVertCam();

		// use velocity if physics not paused, and if we are moving fast enough, or if we are doing vert cam
		// or if we are doing spine physics
		if( /* !mpSkater->m_physics_paused && */ (vel_xz.Length() > 0.1f || use_vert_cam  || mpSkaterStateComponent->GetFlag( SPINE_PHYSICS )))
		{
			if( use_vert_cam )
			{
				// If it's vert, then set target direction to near straight down.
				target[Z].Set( 0.0f, -1.0f, 0.0f );

				// Zero lookaround when in vert air (and not in lookaround locked or override mode, or doing a lip trick).
				if( !mLookaroundLock && !mLookaroundOverride && ( state != LIP ))
				{
					mLookaroundTilt		= 0.0f;
					mLookaroundHeading	= 0.0f;
				}
			}
			else
			{
				
			
				target[Z]		= mpSkater->GetVel();
				#if 1	// for use with non-straight spine transfer physics
				if (mpSkaterStateComponent->GetFlag( SPINE_PHYSICS ))
				{
					// just use the up velocity, plus the velocity over the spine
					// will be straight down when coing down the other side
					target[Z][X] = 0.0f;
					target[Z][Z] = 0.0f;
					target[Z] += mpSkaterStateComponent->GetSpineVel();				
				}
				#endif
				target[Z].Normalize();

//				printf ("Using Velocity (%f, %f, %f)\n", target[Z][X],target[Z][Y], target[Z][Z]);

//				Gfx::AddDebugLine( mpSkater->m_pos, ( target[Z] * 72.0f ) + mpSkater->m_pos, MAKE_RGB( 128, 128, 128 ), MAKE_RGB( 128, 128, 128 ), 4 );

				// Flatten out the velocity component perpendicular to the ground. However, if the skater is in the air, the
				// m_last_normal will contain the last ground normal, which we don't want, so set it to be 'up'.
				float			dot;
				Mth::Vector		normal;

				if( state == LIP )
				{
					
				
					// Patch for spine physics
					// only reduce component perpendicular to plane by 60%
					// so we still get some up/down camera movmeent 
					// only do this when moving down though, otherwise it looks poo
					if (mpSkaterStateComponent->GetFlag(SPINE_PHYSICS) && target[Z][Y] < 0.0f)
					{
//						normal	= mpSkaterPhysicsComponent->m_current_normal;
						normal.Set( 0.0f, 1.0f, 0.0f );
						//dot		= (( target[Z].GetX() * normal.GetX() ) + ( target[Z].GetY() * normal.GetY() ) + ( target[Z].GetZ() * normal.GetZ() ));
						//dot		= dot * 0.8f;
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
					normal	= mpSkaterPhysicsComponent->m_current_normal;
					//dot		= (( target[Z].GetX() * normal.GetX() ) + ( target[Z].GetY() * normal.GetY() ) + ( target[Z].GetZ() * normal.GetZ() ));
					//dot		= dot * 0.8f;
					dot =  0.8f * Mth::DotProduct(target[Z],normal);
				}

				//target[Z].Set(	target[Z].GetX() - ( dot * normal.GetX() ), target[Z].GetY() - ( dot * normal.GetY() ), target[Z].GetZ() - ( dot * normal.GetZ() ));
				target[Z] -= normal * dot;
				
				
				target[Z].Normalize();

//				Gfx::AddDebugLine( mpSkater->m_pos, ( target[Z] * 72.0f ) + mpSkater->m_pos, MAKE_RGB( 256, 256, 256 ), MAKE_RGB( 256, 256, 256 ), 4 );
			}

			// We need to orthonormalize in a specific order, as when the velocity is going backwards, then 
			// the X orient will be wrong, and so plonk the skater upside down.
			target[X]			= Mth::CrossProduct( target[Y], target[Z] );
			target[X].Normalize();
			target[Y]			= Mth::CrossProduct( target[Z], target[X] );
			target[Y].Normalize();

//			Gfx::AddDebugLine( mpSkater->m_pos, ( target[X] * 72.0f ) + mpSkater->m_pos, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 4 );
//			Gfx::AddDebugLine( mpSkater->m_pos, ( target[Y] * 72.0f ) + mpSkater->m_pos, MAKE_RGB( 0, 128, 0 ), MAKE_RGB( 0, 128, 0 ), 4 );
//			Gfx::AddDebugLine( mpSkater->m_pos, ( target[Z] * 72.0f ) + mpSkater->m_pos, MAKE_RGB( 0, 0, 128 ), MAKE_RGB( 0, 0, 128 ), 4 );
		}    							
							
		// Tilt further if in regular air (going off a big jump, for example), or doing a lip trick.
		if( state == LIP )
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
		target.RotateYLocal( mLookaroundHeading + mLookaroundHeadingStartingPoint );

		// Now tilt the matrix down a bit.
		if( state == LIP )
		{
//			target.RotateYLocal( 0.8f );
//			target.RotateXLocal( mLipTrickTilt + mTiltAddition );
			_xrotlocal ( target, mLipTrickTilt + mTiltAddition );
		}					
		else if( !(mpSkaterStateComponent->GetFlag( VERT_AIR ) && !mpSkaterStateComponent->GetFlag( CAN_BREAK_VERT )  && !mpSkaterStateComponent->GetFlag(SPINE_PHYSICS) ))
		{
//			target.RotateXLocal( mTilt + mTiltAddition );
			_xrotlocal ( target, mTilt + mTiltAddition );
		}					

		// Now adjust for 'lookaround'.
//		target.RotateXLocal( mLookaroundTilt );
		_xrotlocal ( target, mLookaroundTilt );

		static float lip_rotate = 0.0f;

		if( state == LIP )
		{
			lip_rotate = 1.5707963f;

			if( mLipSideDecided == 0 )
			{
				target.RotateY( lip_rotate );

				// Do collision check.
				CFeeler feeler;

				Mth::Vector horiz_z( target[Z][X], 0.0f, target[Z][Z] );
				horiz_z.Normalize();
				Mth::Vector a = mpSkater->m_pos - ( target[Z] * 3.0f );
				Mth::Vector b = a - ( horiz_z * 72.0f );

				feeler.SetIgnore(0, mFD_CAMERA_COLLIDABLE);
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
		
//		Gfx::AddDebugLine( mpSkater->m_pos, ( target[X] * 72.0f ) + mpSkater->m_pos, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 4 );
//		Gfx::AddDebugLine( mpSkater->m_pos, ( target[Y] * 72.0f ) + mpSkater->m_pos, MAKE_RGB( 0, 128, 0 ), MAKE_RGB( 0, 128, 0 ), 4 );
//		Gfx::AddDebugLine( mpSkater->m_pos, ( target[Z] * 72.0f ) + mpSkater->m_pos, MAKE_RGB( 0, 0, 128 ), MAKE_RGB( 0, 0, 128 ), 4 );

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

#define CAMERA_SLERP_STOP 0.9999f
		
		if( dotx > CAMERA_SLERP_STOP && doty > CAMERA_SLERP_STOP && dotz > CAMERA_SLERP_STOP )
		{
			// Do nothing, camera has reached the target so we just leave the frame matrix exactly where it is.
//		    frame_matrix = t_matrix;
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
					mpSlerp->getMatrix( &frame_matrix, getTimeAdjustedSlerp( mVertAirLandedSlerp, delta_time ));
				}
				else
				{
					// Clear the vert air landed timer.
					mVertAirLandedTimer = 0.0f;

					mpSlerp->getMatrix( &frame_matrix, getTimeAdjustedSlerp( mSlerp, delta_time ));

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
						mpSlerp->getMatrix( &frame_matrix, getTimeAdjustedSlerp( new_slerp, delta_time ));

						this_dot = mLastDot * 0.9998f;
					}
                    
					mLastDot = this_dot;
				}
			}
			else
			{
				mpSlerp->getMatrix( &frame_matrix, getTimeAdjustedSlerp( mVertAirSlerp, delta_time ));

				// Set the vert air landed timer to the max, since the skater is in vert air.
				mVertAirLandedTimer = VERT_AIR_LANDED_TIME;
			}						
		}		

		// At this point, frame_matrix is valid to store.
		mLastActualRight	= frame_matrix[X];
		mLastActualUp		= frame_matrix[Y];
		mLastActualAt		= frame_matrix[Z];

		// Set camera position to be the same as the skater.
		Mth::Vector	cam_pos = GetTripodPos( instantly );

		// If in the air doing a trick, we slowly reduce the behind value to 'zoom' in on the skater.
		float above, behind;
		if (instantly)
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



		

//        printf( "above = %f, behind %f\n", above, behind );
		
		//Dbg_Message("Camera before: pos (%5.1f,%5.1f,%5.1f)", cam_pos[X], cam_pos[Y], cam_pos[Z]);

		// Note we use the camera's at vector, as this will keep the skater in the middle of the frame.
		//cam_pos -= frame_matrix[Z] * behind;
		cam_pos += frame_matrix[Z] * behind;
		
		// Move camera along the Skater's up vector.
//		cam_pos += mpSkater->GetMatrix()[Y] * above;
		cam_pos += skater_up * above;

		Mth::Vector	actual_focus_pos = mpSkater->m_pos + ( skater_up * above );


//		Gfx::AddDebugLine( mpSkater->m_pos, actual_focus_pos, MAKE_RGB( 0, 0, 128 ), MAKE_RGB( 0, 0, 128 ), 4 );
													  
//		Dbg_Message("Matrix Z  (%5.4f,%5.4f,%5.4f)", frame_matrix[Z][X], frame_matrix[Z][Y], frame_matrix[Z][Z]);
//		Dbg_Message("Matrix UP (%5.4f,%5.4f,%5.4f)", skater_up[X], skater_up[Y], skater_up[Z]);
//		Dbg_Message("Above %f Behind %f", above, behind);
//		Dbg_Message("focus pos (%5.1f,%5.1f,%5.1f)", actual_focus_pos[X], actual_focus_pos[Y], actual_focus_pos[Z]);
//		Dbg_Message("Camera after: pos (%5.1f,%5.1f,%5.1f)", cam_pos[X], cam_pos[Y], cam_pos[Z]);

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
		Mth::Vector at_pos = cam_pos - ((Mth::Vector)( frame_matrix[Z] )) * behind;

		// camera collision detection
		
		if (mpSkaterPhysicsComponent->mp_movable_contact_component->GetContact() && state == LIP)
		{
			// no camera collision if doing a lipt trick on a moving object
			//printf ("Lip trick on moving object\n");
			
		}
		else
		{
			CFeeler feeler;
			// Ignore faces with mFD_NON_COLLIDABLE set, and ignore faces with mFD_CAMERA_COLLIDABLE not set.
			feeler.SetIgnore(0, mFD_CAMERA_COLLIDABLE);
			bool collision = feeler.GetCollision(at_pos, cam_pos, true);
			if( collision )
			{
				
				//Gfx::AddDebugLine( at_pos, cam_pos, MAKE_RGB( 128, 0, 0 ), MAKE_RGB( 128, 0, 0 ), 2000 );
	
				// Limits the camera to getting nearer than 11.9 inches to the focus point.
				float distance = feeler.GetDist();
				if(( behind * distance * 0.9f ) < 11.9f )
				{
					distance = 11.9f / ( behind * 0.9f );
				}
				
	
				cam_pos = at_pos + (( cam_pos - at_pos ) * ( distance * 0.9f ));
			}
	
#			ifdef __PLAT_NGPS__
			// Now do two additional checks 1 foot to either side of the camera.
			Mth::Vector	left( frame_matrix[X][X], 0.0f, frame_matrix[X][Z], 0.0f );
			left.Normalize( 8.0f );
			left += cam_pos;
	
			collision = feeler.GetCollision(cam_pos, left, true);
			if( collision )
			{
				left		   -= feeler.GetPoint();
				cam_pos		   -= left;
			}
			else
			{
				Mth::Vector	right( -frame_matrix[X][X], 0.0f, -frame_matrix[X][Z], 0.0f );
				right.Normalize( 8.0f );
				right += cam_pos;
	
				collision = feeler.GetCollision(cam_pos, right, true);
				if( collision )
				{
					right		   -= feeler.GetPoint();
					cam_pos		   -= right;
				}
			}
	
			if(collision )
			{
				// Re-orient the camera again.
				target[Z].Set( actual_focus_pos.GetX() - cam_pos.GetX(), actual_focus_pos.GetY() - cam_pos.GetY(), actual_focus_pos.GetZ() - cam_pos.GetZ());
				target[Z].Normalize();
	
				// Read back the Y from the current matrix.
	//			target[Y][0]	= p_frame_matrix->up.x;
	//			target[Y][1]	= p_frame_matrix->up.y;
	//			target[Y][2]	= p_frame_matrix->up.z;
	
				// Generate new orthonormal X and Y axes.
				target[X]		= Mth::CrossProduct( target[Y], target[Z] );
				target[X].Normalize();
				target[Y]		= Mth::CrossProduct( target[Z], target[X] );
				target[Y].Normalize();
	
				// Write back into camera matrix.
				frame_matrix[X][X]	= -target[0][0];
				frame_matrix[X][Y]	= -target[0][1];
				frame_matrix[X][Z]	= -target[0][2];
				frame_matrix[Y][X]	= target[1][0];
				frame_matrix[Y][Y]	= target[1][1];
				frame_matrix[Y][Z]	= target[1][2];
				frame_matrix[Z][X]	= -target[2][0];
				frame_matrix[Z][Y]  = -target[2][1];
				frame_matrix[Z][Z]	= -target[2][2];
			}
#			endif
		}

		cam_pos[W] = 1.0f;
		//Dbg_MsgAssert(cam_pos[W] == 1.0f, ("cam_pos W is %f, not 1.0", cam_pos[W]));
		Gfx::Camera::SetPos(cam_pos);

		// store the position of the camera in CSkaterCam also (used for proximity nodes)
		m_pos = cam_pos;
		Mth::Vector		to_target = at_pos - cam_pos;
				
				
//		Dbg_Message("Camera Final: pos (%5.1f,%5.1f,%5.1f)", cam_pos[X], cam_pos[Y], cam_pos[Z]);

		
		
		return to_target.Length();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector CSkaterCam::GetTripodPos( bool instantly )
{
	float		lerp_xz;
	float		lerp_y;
	float		delta		= Tmr::FrameLength();
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

	if(( mpSkaterStateComponent->GetFlag( VERT_AIR ) && !mpSkaterStateComponent->GetFlag( CAN_BREAK_VERT ) && !mpSkaterStateComponent->GetFlag(SPINE_PHYSICS)))
	{
		#ifdef	DEBUG_CAMERA
		printf ("Skatercam using mVertAirLerp\n");
		#endif

		lerp_xz	= getTimeAdjustedSlerp( mVertAirLerpXZ * mLerpReductionMult, delta );
		lerp_y	= getTimeAdjustedSlerp( mVertAirLerpY * mLerpReductionMult, delta );
	}
	else
	{
		lerp_xz	= getTimeAdjustedSlerp( mLerpXZ * mLerpReductionMult, delta );
		lerp_y	= getTimeAdjustedSlerp( mLerpY * mLerpReductionMult, delta );

		// Added the following check (Dave 7/16/02) to help eliminate the camera rattle when snapping on/off a curb.
		// The flag is set for one frame only.
		if( mpSkaterStateComponent->GetFlag( SNAPPED_OVER_CURB ))
		{
			lerp_y = 1.0f;
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

void CSkaterCam::CalculateZoom( float* p_above, float* p_behind )
{
	mTargetZoom = 1.0f;

	// Deal with zoom for Big Air. Set the flag only when a trick is started.
	if( !mBigAirTrickZoomActive && (mpSkaterStateComponent->GetFlag( VERT_AIR ) && !mpSkaterStateComponent->GetFlag( CAN_BREAK_VERT ) && !mpSkaterStateComponent->GetFlag(SPINE_PHYSICS)) && mpSkaterStateComponent->DoingTrick() )
	{
		mBigAirTrickZoomActive = true;
	}
	else if( !(mpSkaterStateComponent->GetFlag( VERT_AIR ) && !mpSkaterStateComponent->GetFlag( CAN_BREAK_VERT ) && !mpSkaterStateComponent->GetFlag(SPINE_PHYSICS)))
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

	
	// If lookaround override is set, factor in the lookaround override zoom.
	if( mLookaroundOverride && ( mLookaroundZoom != 1.0f ))
	{
		mCurrentZoom = mTargetZoom;
		mCurrentZoom *= mLookaroundZoom;
	}
	else
	{
		mCurrentZoom += (( mTargetZoom - mCurrentZoom ) * mZoomLerp );
	}
	
	*p_behind = mBehind * mCurrentZoom;


	// Behind is also shortened when the lookaround camera is tilting upwards.
	if( mLookaroundTilt < 0.0f )
	{
		float max_tilt = 3.14f * 0.2f;
		*p_behind = *p_behind * ( 0.4f + ( 0.6f * (( max_tilt + mLookaroundTilt ) / max_tilt )));
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
		*p_above = PERFECT_ABOVE + (( above_val - PERFECT_ABOVE ) * mCurrentZoom );
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

void CSkaterCam::UpdateInterpolators( void )
{
	float delta_time = Tmr::FrameLength();

	if( mLookaroundOverride )
	{
		if( mLookaroundDeltaTimer > 0.0f )
		{
			if( mLookaroundDeltaTimer > delta_time )
			{
				mLookaroundHeading		+= mLookaroundHeadingDelta * delta_time;
				mLookaroundTilt			+= mLookaroundTiltDelta * delta_time;
				mLookaroundDeltaTimer	-= delta_time;
			}
			else
			{
				mLookaroundHeading		= mLookaroundHeadingTarget;
				mLookaroundTilt			= mLookaroundTiltTarget;
				mLookaroundDeltaTimer	= 0.0f;
			}
		}
	}

	if( mBehindInterpolatorTime > 0.0f )
	{
		mBehindInterpolatorTime -= SKATERCAM_INTERPOLATOR_TIMESTEP;
		mBehind += mBehindInterpolatorDelta;		
	}
	else
	{
		mBehindInterpolatorTime = 0.0f;
	}

	if( mAboveInterpolatorTime > 0.0f )
	{
		mAboveInterpolatorTime -= SKATERCAM_INTERPOLATOR_TIMESTEP;
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

void CSkaterCam::SetLerpReductionTimer( float time )
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

void CSkaterCam::SetSkater( CSkater* skater )
{
	mpSkater = skater;
	Dbg_Assert(mpSkater);
	
	mpSkaterPhysicsComponent = GetSkaterCorePhysicsComponentFromObject(skater);
	Dbg_Assert(mpSkaterPhysicsComponent);
	
	mpSkaterStateComponent = GetSkaterStateComponentFromObject(skater);
	Dbg_Assert(mpSkaterStateComponent);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkater* CSkaterCam::GetSkater( void )
{
	return mpSkater;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterCam::BindToController( int controller )
{
	// swap
	Spt::SingletonPtr< Inp::Manager > inp_manager;
	inp_manager->ReassignHandler( *m_input_handler, controller );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Obj
