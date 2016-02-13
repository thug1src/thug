//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       GunslingerCameraLookAroundComponent.cpp
//* OWNER:          Dave
//* CREATION DATE:  5/13/03
//****************************************************************************

#include <gel/components/cameralookaroundcomponent.h>
#include <gel/components/inputcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

#include <gfx/nxviewman.h>

#include <gfx/debuggfx.h>
						   
float	spin_modulator			= 1.0f;
float	tilt_modulator			= 1.0f;
float	last_heading_change		= 0.0f;
float	last_tilt_change		= 0.0f;
bool	gun_fired				= false;
bool	allow_strafe_locking	= false;

float	screen_angle			= 72.0f;		// Test value for setting field of view.

float	lookaround_tilt_angular_speed		= 0.0f;
float	lookaround_heading_angular_speed	= 0.0f;



namespace Obj
{
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CCameraLookAroundComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CCameraLookAroundComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCameraLookAroundComponent::CCameraLookAroundComponent() : CBaseComponent()
{
	SetType( CRC_CAMERALOOKAROUND );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CCameraLookAroundComponent::~CCameraLookAroundComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCameraLookAroundComponent::InitFromStructure( Script::CStruct* pParams )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCameraLookAroundComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCameraLookAroundComponent::Finalize (   )
{
	mp_input_component = GetInputComponentFromObject(GetObject());
	
	Dbg_Assert(mp_input_component);

	mLookaroundTilt					= 0.0f;
	mLookaroundHeading				= 0.0f;
	mLookaroundZoom					= 1.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CCameraLookAroundComponent::Update()
{
	CControlPad * p_control_pad;
	
	if (Script::GetInt(CRCD(0x702247a5,"Viewer_controller"), false))
	{
		p_control_pad = &mp_input_component->GetControlPad2();
	}
	else
	{
		p_control_pad = &mp_input_component->GetControlPad();
	}

	if( p_control_pad->m_up.GetTriggered())
	{
		p_control_pad->m_up.ClearTrigger();

		// Unfortunately, pressing up on the left control stick will also cause m_up to be triggered, so check this
		// is really the d-pad press.
		if( p_control_pad->GetScaledLeftAnalogStickMagnitude() == 0.0f )
		{
			// This would be a good place to put the field of view setting code for the sniper rifle.
			if( screen_angle == 72.0f )
			{
				screen_angle = 25.0f;
			}
			else if( screen_angle == 25.0f )
			{
				screen_angle = 7.5f;
			}
			else if( screen_angle == 7.5f )
			{
				screen_angle = 72.0f;
			}
			Nx::CViewportManager::sSetScreenAngle( screen_angle );
		}
	}
	
	float frame_length = Tmr::FrameLength();
	
	// Strafe locking is allowed by default.
	allow_strafe_locking = true;

	// Shouldn't really do this here. Disable strafe locking if not strafing, or if moving forward or backwards.
	float fb = p_control_pad->m_scaled_leftY;
	float lr = p_control_pad->m_scaled_leftX;

	if( Mth::Abs( fb ) > 0.0f )
	{
		allow_strafe_locking = false;
	}

	if( Mth::Abs( lr ) < 0.2f )
	{
		allow_strafe_locking = false;
	}

	if (!mLookaroundLock && !mLookaroundOverride)
	{
		float target = p_control_pad->m_scaled_rightY;

		// Modulate with the variable used to damp cursor movement when aiming at a target.
		target = target * tilt_modulator;

#		if 1
//		if( target != 0.0f )
		{
			allow_strafe_locking = false;

			if( Script::GetInteger( CRCD( 0x9edfc7af, "GunslingerInvertAiming" )) == 0 )
			{
				// Negate value if vertical aiming invert is not enabled.
				target = -target;
			}

			// Get script values.
			float tilt_ka = Script::GetFloat( CRCD( 0xcac0c1d4, "GunslingerLookaroundTiltKa" ), Script::ASSERT );
			float tilt_ea = Script::GetFloat( CRCD( 0x5443ec5a, "GunslingerLookaroundTiltEa" ), Script::ASSERT );

			float tilt_ks = Script::GetFloat( CRCD( 0x3979b09c, "GunslingerLookaroundTiltKs" ), Script::ASSERT );
			float tilt_es = Script::GetFloat( CRCD( 0xa7fa9d12, "GunslingerLookaroundTiltEs" ), Script::ASSERT );

			// Calculate acceleration.
			float a = tilt_ka * powf( Mth::Abs( target ), tilt_ea ) * (( target > 0.0f ) ? 1.0f : ( target < 0.0f ) ? -1.0f : 0.0f );

			// Calculate max speed.
			float s = tilt_ks * powf( Mth::Abs( target ), tilt_es ) * (( target > 0.0f ) ? 1.0f : ( target < 0.0f ) ? -1.0f : 0.0f );

			lookaround_tilt_angular_speed += a;

			if( s == 0.0f )
			{
				lookaround_tilt_angular_speed = 0.0f;
			}
			else if((( s > 0.0f ) && ( lookaround_tilt_angular_speed > s )) || (( s < 0.0f ) && ( lookaround_tilt_angular_speed < s )))
			{
					lookaround_tilt_angular_speed = s;
			}

			float lookaround_tilt_speed = Script::GetFloat( CRCD( 0x67796648, "GunslingerLookaroundTiltSpeed" ), Script::ASSERT );
			mLookaroundTilt += lookaround_tilt_angular_speed * lookaround_tilt_speed * frame_length;

			last_tilt_change = lookaround_tilt_angular_speed * lookaround_tilt_speed * frame_length;

			if( mLookaroundTilt > 1.1f )
				mLookaroundTilt = 1.1f;
			else if( mLookaroundTilt < -0.85f )
				mLookaroundTilt = -0.85f;
		}
#		else
		// This calculation is different for Gunslinger, since we want to be able to look up higher.
//		target = -1.4f * (target < 0.0f ? (0.7f * target) : (0.4f * target));
		target = -1.4f * ( target < 0.0f ? ( 0.7f * target ) : ( 0.7f * target ));

		if( Mth::Abs( target - mLookaroundTilt ) > 0.001f )
		{
			// Ditto for this calc.
//			mLookaroundTilt += (target - mLookaroundTilt) * 3.75f * frame_length;
			mLookaroundTilt += (target - mLookaroundTilt) * 6.0f * frame_length;
		}
		else
		{
			mLookaroundTilt = target;
		}
#		endif
		
#		if 1
		target = p_control_pad->m_scaled_rightX;

		// Modulate with the variable used to damp cursor movement when aiming at a target.
		target = target * spin_modulator;
		
//		if( target != 0.0f )
		{
			allow_strafe_locking = false;

			// Get script values.
			float heading_ka = Script::GetFloat( CRCD( 0x6fd803d9, "GunslingerLookaroundHeadingKa" ), Script::ASSERT );
			float heading_ea = Script::GetFloat( CRCD( 0xf15b2e57, "GunslingerLookaroundHeadingEa" ), Script::ASSERT );

			float heading_ks = Script::GetFloat( CRCD( 0x9c617291, "GunslingerLookaroundHeadingKs" ), Script::ASSERT );
			float heading_es = Script::GetFloat( CRCD( 0x2e25f1f, "GunslingerLookaroundHeadingEs" ), Script::ASSERT );

			// Calculate acceleration.
			float a = heading_ka * powf( Mth::Abs( target ), heading_ea ) * (( target > 0.0f ) ? 1.0f : ( target < 0.0f ) ? -1.0f : 0.0f );

			// Calculate max speed.
			float s = heading_ks * powf( Mth::Abs( target ), heading_es ) * (( target > 0.0f ) ? 1.0f : ( target < 0.0f ) ? -1.0f : 0.0f );

			lookaround_heading_angular_speed += a;

			if( s == 0.0f )
			{
				lookaround_heading_angular_speed = 0.0f;
			}
			else if((( s > 0.0f ) && ( lookaround_heading_angular_speed > s )) || (( s < 0.0f ) && ( lookaround_heading_angular_speed < s )))
			{
					lookaround_heading_angular_speed = s;
			}

			float lookaround_heading_speed = Script::GetFloat( CRCD( 0x8501d824, "GunslingerLookaroundHeadingSpeed" ), Script::ASSERT );

			// Control stick left - reticle should move left.
			mLookaroundHeading -= lookaround_heading_angular_speed * lookaround_heading_speed * frame_length;

			last_heading_change	= lookaround_heading_angular_speed * lookaround_heading_speed * frame_length;

			if( mLookaroundHeading > ( 2 * Mth::PI ))
				mLookaroundHeading -= ( 2 * Mth::PI );
			else if( mLookaroundHeading < 0.0f )
				mLookaroundHeading += 2 * Mth::PI;
		}
#		else
		target = 3.0f * p_control_pad->m_scaled_rightX;
		if (Mth::Abs(target - mLookaroundHeading) > 0.001f)
		{
			mLookaroundHeading += (target - mLookaroundHeading) * 3.75f * frame_length;
		}
		else
		{
			mLookaroundHeading = target;
		}
#		endif
	}
	
	float delta_time = Tmr::FrameLength();

	if (mLookaroundOverride)
	{
		if (mLookaroundDeltaTimer > 0.0f)
		{
			if (mLookaroundDeltaTimer > delta_time)
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

	// Total hack to simulate shooting for now.
	gun_fired = false;
	if( p_control_pad->m_R1.GetTriggered())
	{
		gun_fired = true;
		p_control_pad->m_R1.ClearTrigger();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CCameraLookAroundComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
        // @script | SC_SetSkaterCamOverride | 
        // @parm float | heading |
        // @parm float | tilt | 
        // @parm float | time | 
        // @parm float | zoom | 
		case 0xb8f23899:	// SC_SetSkaterCamOverride
		{
			float heading, tilt, time;
			float zoom = 1.0f;
			pParams->GetFloat( CRCD(0xfd4bc03e,"heading"), &heading, Script::ASSERT );
			pParams->GetFloat( CRCD(0xe3c07609,"tilt"), &tilt, Script::ASSERT );
			pParams->GetFloat( CRCD(0x906b67ba,"time"), &time, Script::ASSERT );
			pParams->GetFloat( CRCD(0x48d4868b,"zoom"), &zoom );

			SetLookaround( heading, tilt, time, zoom );
			
			break;
		}

        // @script | SC_ClearSkaterCamOverride |
		case 0xe3b327c0:	// SC_ClearSkaterCamOverride
			ClearLookaround();
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

void CCameraLookAroundComponent::GetDebugInfo(Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info sent to CCameraLookAroundComponent::GetDebugInfo"));

	CBaseComponent::GetDebugInfo(p_info);	  
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCameraLookAroundComponent::SetLookaround( float heading, float tilt, float time, float zoom )
{
	mLookaroundOverride				= true;

	mLookaroundHeadingTarget		= heading;
	mLookaroundTiltTarget			= tilt;

	// Calculate delta - the amount to move in 1 second.
	if (time > 0.0f)
	{
		mLookaroundHeadingDelta		= (heading - mLookaroundHeading) / time;
		mLookaroundTiltDelta		= (tilt - mLookaroundTilt) / time;
	}

	mLookaroundDeltaTimer			= time;
	mLookaroundZoom					= zoom;
}

#if 0 // old control input checking code

char touched[256 * 256 / 8];
bool initialized = false;

void MapInput (   )
{
	if (!initialized)
	{
		for (int n = 255 * 255; n--; )
		{
			touched[n / 8] = 0;
		}
		initialized = true;
	}
	
	CControlPad& pad = mp_input_component->GetControlPad();
	
	int x = static_cast< int >(pad.m_rightX) + 128;
	int y = static_cast< int >(pad.m_rightY) + 128;
	
	// printf("x = %i; y = %i\n", x, y);
	
	int index = (x * 256) + y;
	
	// printf("index / 8 = %i\n", index / 8);
	// printf("MAX = %i\n", 256 * 256 / 8);
	
	touched[index / 8] |= (1 << (index % 8));
}


void DrawMap (   )
{
	Mth::Vector pos = GetObject()->GetPos();
	Mth::Matrix matrix = GetObject()->GetMatrix();
	
	Mth::Vector center = pos - 300.0f * matrix[Z];
	Mth::Vector up = matrix[Y];
	Mth::Vector left = -matrix[X];
	
	for (int n = 255 * 255; n--; )
	{
		if (!(touched[n / 8] & (1 << (n % 8)))) continue;
		
		int x = n / 256;
		int y = n % 256;
		
		Mth::Vector point = center + up * (y - 128) + left * (x - 128);
		
		Gfx::AddDebugLine(point + up + left, point - up - left, MAKE_RGB(255, 255, 255), 0, 1);
		Gfx::AddDebugLine(point + up - left, point - up + left, MAKE_RGB(255, 255, 255), 0, 1);
	}
}

#endif

}
