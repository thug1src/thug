//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       GunslingerWalkCameraComponent.cpp
//* OWNER:          Dave
//* CREATION DATE:  5/13/03
//****************************************************************************

#include <gel/components/walkcameracomponent.h>
#include <core/math/slerp.h>
#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/components/walkcomponent.h>
#include <gel/components/camerautil.h>
#include <gel/components/cameralookaroundcomponent.h>
#include <gel/components/cameracomponent.h>
#include <gel/components/pedlogiccomponent.h>				// Messy, but for now used to obtain ped positions etc.
#include <gel/components/weaponcomponent.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gfx/debuggfx.h>

// These should be members in the cameralookaroundcomponent.
extern float			spin_modulator;
extern float			tilt_modulator;
extern float			last_heading_change;
extern float			last_tilt_change;
extern bool				gun_fired;
extern bool				allow_strafe_locking;

int						best_ped_timer			= 0;
Obj::CCompositeObject*	p_selected_target		= NULL;
float					target_selection_timer	= 0.0f;		// Times how long the selected target has been targeted for.

namespace Obj
{

	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static float s_get_gunslinger_param( uint32 checksum )
{
	Script::CStruct* p_cam_params = Script::GetStructure( CRCD( 0xa13ff9ae,"GunslingerCameraParameters" ));
	Dbg_Assert( p_cam_params );

	float param;
	p_cam_params->GetFloat( checksum, &param, Script::ASSERT );
	return param;
}

	
	
	
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CWalkCameraComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CWalkCameraComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CWalkCameraComponent::CWalkCameraComponent() : CBaseComponent()
{
	SetType( CRC_WALKCAMERA );

	m_last_actual_matrix.Ident();
	m_last_tripod_pos.Set( 0.0f, 0.0f, 0.0f, 1.0f );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CWalkCameraComponent::~CWalkCameraComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkCameraComponent::InitFromStructure( Script::CStruct* pParams )
{
	uint32 target_id = 0 ;
	pParams->GetChecksum("CameraTarget", &target_id, Script::ASSERT);
	
	CCompositeObject* p_target = static_cast< CCompositeObject* >(CCompositeObjectManager::Instance()->GetObjectByID(target_id));
	Dbg_MsgAssert(p_target, ("Bad CameraTarget given to WalkCameraComponent"));
		
	set_target(p_target);
	
	m_last_dot = 1.0f;
	m_current_zoom = 1.0f;
	
	m_last_actual_matrix = GetObject()->GetMatrix();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkCameraComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkCameraComponent::Finalize()
{
	mp_lookaround_component = GetCameraLookAroundComponentFromObject(GetObject());
	mp_camera_component = GetCameraComponentFromObject(GetObject());
	
	Dbg_Assert(mp_lookaround_component);
	Dbg_Assert(mp_camera_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkCameraComponent::Update()
{
	// optimization KLUDGE
//	if (Script::GetInteger(CRCD(0x1aa88b08, "vehicle_mode")))
//	{
//		GetObject()->Pause(true);
//		return;
//	}
	
	if (mp_target->HasTeleported())
	{
		m_instant_count = 3;
	}

	bool instantly;
	if (m_instant_count > 0)
	{
		--m_instant_count;
		instantly = true;
	}
	else
	{
		instantly = false;
	}
	
	mp_camera_component->StoreOldPosition();

	float frame_length = Tmr::FrameLength();
	
	// get input
//	float horiz_control = GetInputComponentFromObject( GetObject())->GetControlPad().m_scaled_rightX;
	float horiz_control = mp_lookaround_component->mLookaroundHeading;

	// Restore camera position from last frame, previous to refocusing, lookaround and collision detection.
	GetObject()->GetMatrix() = m_last_actual_matrix;

	Mth::Vector	target_facing = -GetObject()->GetMatrix()[Z];
	target_facing[Y] = 0.0f;
	target_facing.Normalize();
	
	Mth::Vector subject_facing = mp_target->GetMatrix()[Z];
	subject_facing[Y] = 0.0f;
	subject_facing.Normalize();
	
	// two options; we either use our old facing as the target facing or use the subject's facing as the target facing
	bool use_subject_facing = true;
	
	// if in a flush request
	if (m_flush_request_active)
	{
		// always use the subject's matrix
	}
	// if controlling camera
	else if (horiz_control != 0.0f)
	{
		use_subject_facing = false;
	}
	// if the subject's facing is towards the camera
	else if (Mth::DotProduct(target_facing, subject_facing) < cosf(Mth::DegToRad(s_get_gunslinger_param(CRCD(0xb8da8a73, "lock_angle")))))
	{
		use_subject_facing = false;
	}
	// if the subject is moving very slowly
	else // if (!mp_target_walk_component->UseDPadCamera())
	{
		float full_slerp_speed;
		float min_slerp_speed;
		if (!mp_target_walk_component->UseDPadCamera())
		{
			full_slerp_speed = s_get_gunslinger_param(CRCD(0xbcef6dda, "full_slerp_speed"));
			min_slerp_speed = s_get_gunslinger_param(CRCD(0x824349e5, "min_slerp_speed"));
		}
		else
		{
			full_slerp_speed = s_get_gunslinger_param(CRCD(0x73da1ec0, "dpad_full_slerp_speed"));
			min_slerp_speed = s_get_gunslinger_param(CRCD(0xda5cad3, "dpad_min_slerp_speed"));
		}
		
		float target_vel = mp_target->GetVel().Length();
		if (target_vel < full_slerp_speed)
		{
			use_subject_facing = false;
		}
	}
	
	// Never want to use the subject facing camera.
	use_subject_facing = false;

	if (use_subject_facing)
	{
		target_facing = subject_facing;
	}
	
	// Build target matrix.
	Mth::Matrix target_matrix;
	target_matrix[W].Set( 0.0f, 0.0f, 0.0f, 1.0f );
	target_matrix[Z] = target_facing;
	target_matrix[Y].Set( 0.0f, 1.0f, 0.0f );
	target_matrix[X].Set( target_facing[Z], 0.0f, -target_facing[X] );

	// Dave note - testing default value for now.
	float slerp = 0.06f;

	target_matrix[X] = -target_matrix[X];
	target_matrix[Z] = -target_matrix[Z];

	// use later for camera position
	Mth::Vector up = mp_target->GetMatrix()[Y];
	
	if (!instantly)
	{
		if( Mth::DotProduct(target_matrix[X], GetObject()->GetMatrix()[X]) > CAMERA_SLERP_STOP &&
			Mth::DotProduct(target_matrix[Y], GetObject()->GetMatrix()[Y]) > CAMERA_SLERP_STOP &&
			Mth::DotProduct(target_matrix[Z], GetObject()->GetMatrix()[Z]) > CAMERA_SLERP_STOP )
		{
			// we're already at our target, so don't do anything
			
			// turn off any flush request
			if (m_flush_request_active)
			{
				m_flush_request_active = false;
				mp_target_walk_component->SetForwardControlLock(false);
			}
		}
		else
		{
			// Slerp to the target matrix.
			Mth::SlerpInterpolator slerper( &GetObject()->GetMatrix(), &target_matrix );
			
			// Apply the slerping.
			slerper.getMatrix( &GetObject()->GetMatrix(), GetTimeAdjustedSlerp( slerp, frame_length ));

			// Calculate for the skater camera.
			m_last_dot = Mth::DotProduct(m_last_actual_matrix[Z], GetObject()->GetMatrix()[Z]);
		}
	}
	else
	{
		GetObject()->GetMatrix() = target_matrix;
	}

	// At this point, GetObject()->GetMatrix() is valid to store.
	m_last_actual_matrix = GetObject()->GetMatrix();

	// Now apply the lookaround adjustments to the matrix.
	// Control over target facing.
	if( horiz_control != 0.0f && !m_flush_request_active )
	{
		// The horiz_control value needs to be damped when there is an item of interest within the reticle area.
		GetObject()->GetMatrix().RotateYLocal( horiz_control );
	}

	float tilt = s_get_gunslinger_param( CRCD( 0xe3c07609, "tilt" ));
	GetObject()->GetMatrix().RotateXLocal( tilt + mp_lookaround_component->mLookaroundTilt );

	Mth::Vector	camera_pos = get_tripod_pos( instantly );

	// Test the weapon component to generate the 'sticky' targetting behavior.
	if( mp_target && p_selected_target )
	{
		CWeaponComponent* p_weapon = GetWeaponComponentFromObject( mp_target );

		p_weapon->SetCurrentTarget( p_selected_target );
		p_weapon->SetSightPos( camera_pos );
		p_weapon->SetSightMatrix( GetObject()->GetMatrix());

		float extra_heading_change, extra_tilt_change;
		p_weapon->ProcessStickyTarget( last_heading_change, last_tilt_change, &extra_heading_change, &extra_tilt_change );

		if(( extra_heading_change != 0.0f ) || ( extra_tilt_change != 0.0f ))
		{
			// Reset the matrix to what it was prior to the heading and tilt adjustments.
			GetObject()->SetMatrix( m_last_actual_matrix );

			mp_lookaround_component->mLookaroundHeading += extra_heading_change;
			horiz_control += extra_heading_change;
			GetObject()->GetMatrix().RotateYLocal( horiz_control );

			mp_lookaround_component->mLookaroundTilt += extra_tilt_change;
			GetObject()->GetMatrix().RotateXLocal( tilt + mp_lookaround_component->mLookaroundTilt );
		}
	}

	// Calculate zoom
	float above, behind;
	calculate_zoom( above, behind );
	
	camera_pos += GetObject()->GetMatrix()[Z] * behind + up * above;
	
	Mth::Vector	focus_pos = mp_target_walk_component->GetEffectivePos() + up * above;
	
	// Focus the camera directly on the target object
	target_matrix[Z] = focus_pos - camera_pos;
	target_matrix[Z].Normalize();

	// Read back the Y from the current matrix.
//	target_matrix[Y] = GetObject()->GetMatrix()[Y];
	target_matrix[Y] = Mth::Vector( 0.0f, 1.0f, 0.0f );

	// Generate new orthonormal X and Y axes.
	target_matrix[X] = Mth::CrossProduct(target_matrix[Y], target_matrix[Z]);
	target_matrix[X].Normalize();

	target_matrix[Y] = Mth::CrossProduct(target_matrix[Z], target_matrix[X]);
	target_matrix[Y].Normalize();

	// Write back into camera matrix.
	// Since camera points in -Z, but player in +Z, we must negate the X and Z axes
	GetObject()->GetMatrix()[X]	= -target_matrix[X];
	GetObject()->GetMatrix()[Y] = target_matrix[Y];
	GetObject()->GetMatrix()[Z] = -target_matrix[Z];
	
	// clean up matrix
	GetObject()->GetMatrix()[X][W] = 0.0f;
	GetObject()->GetMatrix()[Y][W] = 0.0f;
	GetObject()->GetMatrix()[Z][W] = 0.0f;
	GetObject()->GetMatrix()[W].Set(0.0f, 0.0f, 0.0f, 1.0f);

	// Now do collision detection.
	ApplyCameraCollisionDetection( camera_pos, GetObject()->GetMatrix(), camera_pos - GetObject()->GetMatrix()[Z] * behind,	focus_pos );
	
	#ifdef __USER_DAN__
	if (Script::GetInteger(CRCD(0xaf90c5fd, "walking_debug_lines")))
	{
		Gfx::AddDebugStar(focus_pos, 24.0f, MAKE_RGB(255, 200, 0), 1);
	}
	#endif
	
	camera_pos[W] = 1.0f;
	GetObject()->SetPos( camera_pos );

	// Do the target selection.
	if( mp_target )
	{
		Mth::Vector reticle_min	= camera_pos;
		Mth::Vector reticle_max;

		CWeaponComponent* p_weapon = GetWeaponComponentFromObject( mp_target );

		p_weapon->SetSightMatrix( GetObject()->GetMatrix());
		p_selected_target = p_weapon->GetCurrentTarget( reticle_min, &reticle_max );

		if( gun_fired )
		{
			p_weapon->Fire();
		}

		spin_modulator = p_weapon->GetSpinModulator();
		tilt_modulator = p_weapon->GetTiltModulator();

		p_weapon->DrawReticle();
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CWalkCameraComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		// @script | WalkCamera_FlushRequest | Force the camera to lerp quickly to behind the walker
		case CRCC(0x73febd0f, "WalkCamera_FlushRequest"):
			FlushRequest();
			break;
			
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkCameraComponent::GetDebugInfo(Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info sent to CWalkCameraComponent::GetDebugInfo"));

	CBaseComponent::GetDebugInfo(p_info);	  
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkCameraComponent::ReadyForActivation ( const SCameraState& state )
{
	m_last_tripod_pos = state.lastTripodPos;
	m_last_actual_matrix = state.lastActualMatrix;
	m_last_dot = state.lastDot;
	m_current_zoom = state.lastZoom;
	m_flush_request_active = false;
	
	mp_lookaround_component->mLookaroundTilt = 0.0f;
	mp_lookaround_component->mLookaroundHeading = 0.0f;
	mp_lookaround_component->mLookaroundZoom = 1.0f;
	mp_lookaround_component->mLookaroundLock = false;
	
	m_override_active = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkCameraComponent::GetCameraState ( SCameraState& state )
{
	state.lastActualMatrix = m_last_actual_matrix;
	state.lastTripodPos = m_last_tripod_pos;
	state.lastDot = m_last_dot;
	state.lastZoom = m_current_zoom;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkCameraComponent::FlushRequest (   )
{
	m_flush_request_active = true;
	mp_target_walk_component->SetForwardControlLock(true);
	
	// flush requests zero skater cam lookaround
	mp_lookaround_component->mLookaroundHeading = 0.0f;
	mp_lookaround_component->mLookaroundTilt = 0.0f;
	mp_lookaround_component->mLookaroundZoom = 1.0f;
	mp_lookaround_component->mLookaroundLock = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkCameraComponent::set_target ( CCompositeObject* p_target )
{
	mp_target = p_target;
	Dbg_Assert(mp_target);
	
	mp_target_walk_component = GetWalkComponentFromObject(mp_target);
	Dbg_Assert(mp_target_walk_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector CWalkCameraComponent::get_tripod_pos( bool instantly )
{
	if (instantly)
	{
		m_last_tripod_pos = mp_target_walk_component->GetEffectivePos();
	}
	else
	{
		float lerp_xz = GetTimeAdjustedSlerp(s_get_gunslinger_param(CRCD(0xae47899b, "lerp_xz")), Tmr::FrameLength());
		float lerp_y = GetTimeAdjustedSlerp(s_get_gunslinger_param(CRCD(0xe1e4e104, "lerp_y")), Tmr::FrameLength());
		
		const Mth::Vector& target_pos = mp_target_walk_component->GetEffectivePos();

		m_last_tripod_pos.Set(
			Mth::Lerp(m_last_tripod_pos[X], target_pos[X], lerp_xz),
			Mth::Lerp(m_last_tripod_pos[Y], target_pos[Y], lerp_y),
			Mth::Lerp(m_last_tripod_pos[Z], target_pos[Z], lerp_xz)
		);
	}

	return m_last_tripod_pos;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWalkCameraComponent::calculate_zoom ( float& above, float& behind )
{
	float target_zoom = 1.0f;

	// If lookaround override is set, factor in the lookaround override zoom.
	if (mp_lookaround_component->mLookaroundOverride && mp_lookaround_component->mLookaroundZoom != 1.0f)
	{
		target_zoom *= mp_lookaround_component->mLookaroundZoom;
	}
	
//	m_current_zoom += (( target_zoom - m_current_zoom ) * s_get_gunslinger_param( CRCD( 0x748743a7, "zoom_lerp" )));
	m_current_zoom += (( target_zoom - m_current_zoom ) * s_get_gunslinger_param( CRCD( 0x748743a7, "zoom_lerp" )));
	
//	behind = s_get_gunslinger_param(CRCD(0x52e6c41b, "behind")) * m_current_zoom;
	behind = s_get_gunslinger_param(CRCD(0x52e6c41b, "behind")) * m_current_zoom;

	// Behind is also shortened when the lookaround camera is tilting upwards.
	if( mp_lookaround_component->mLookaroundTilt < 0.0f )
	{
		float max_tilt = -0.9f;
//		behind = behind * (0.4f + (0.6f * ((max_tilt + mp_lookaround_component->mLookaroundTilt) / max_tilt)));
		behind = behind * ( 0.0f + ( 1.0f * (( max_tilt - mp_lookaround_component->mLookaroundTilt ) / max_tilt )));
	}
	else if( mp_lookaround_component->mLookaroundTilt > 0.0f )
	{
		float max_tilt = 1.4f;
		behind = behind * ( 0.0f + ( 1.0f * (( max_tilt - mp_lookaround_component->mLookaroundTilt ) / max_tilt )));
	}

	// Use lip_trick_above when doing a lip trick.
//	float above_val = s_get_gunslinger_param(CRCD(0xb96ae2d, "above"));
	float above_val = s_get_gunslinger_param( CRCD( 0xb96ae2d, "above" ));
	
	// Figure above tending towards the perfect above, if zoom is < 1.0.
	if( m_current_zoom < 1.0f )
	{
		above = SKATERCAMERACOMPONENT_PERFECT_ABOVE + ((above_val - SKATERCAMERACOMPONENT_PERFECT_ABOVE) * m_current_zoom);
	}
	else
	{
		above = above_val;
	}
}

}
