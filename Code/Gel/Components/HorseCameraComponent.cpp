//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       HorseCameraComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  2/10/3
//****************************************************************************

#include <core/defines.h>
#include <core/math.h>
									 
#include <gel/components/horsecameracomponent.h>
#include <gel/components/horsecomponent.h>
#include <gel/components/camerautil.h>
#include <gel/components/pedlogiccomponent.h>
#include <gel/components/weaponcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>

#include <gfx/NxViewMan.h>


#define MESSAGE(a) { printf("M:%s:%i: %s\n", __FILE__ + 15, __LINE__, a); }
#define DUMPI(a) { printf("D:%s:%i: " #a " = %i\n", __FILE__ + 15, __LINE__, a); }
#define DUMPB(a) { printf("D:%s:%i: " #a " = %s\n", __FILE__ + 15, __LINE__, a ? "true" : "false"); }
#define DUMPF(a) { printf("D:%s:%i: " #a " = %g\n", __FILE__ + 15, __LINE__, a); }
#define DUMPE(a) { printf("D:%s:%i: " #a " = %e\n", __FILE__ + 15, __LINE__, a); }
#define DUMPS(a) { printf("D:%s:%i: " #a " = %s\n", __FILE__ + 15, __LINE__, a); }
#define DUMPP(a) { printf("D:%s:%i: " #a " = %p\n", __FILE__ + 15, __LINE__, a); }
#define DUMPV(a) { printf("D:%s:%i: " #a " = %g, %g, %g\n", __FILE__ + 15, __LINE__, (a)[X], (a)[Y], (a)[Z]); }
#define DUMP4(a) { printf("D:%s:%i: " #a " = %g, %g, %g, %g\n", __FILE__ + 15, __LINE__, (a)[X], (a)[Y], (a)[Z], (a)[W]); }
#define DUMPM(a) { DUMP4(a[X]); DUMP4(a[Y]); DUMP4(a[Z]); DUMP4(a[W]); }
#define MARK { printf("K:%s:%i: %s\n", __FILE__ + 15, __LINE__, __PRETTY_FUNCTION__); }
#define PERIODIC(n) for (static int p__ = 0; (p__ = ++p__ % (n)) == 0; )

#define vVELOCITY_WEIGHT_DROP_THRESHOLD				MPH_TO_IPS(15.0f)
// #define vLOCK_ATTRACTOR_VELOCITY_THRESHOLD			MPH_TO_IPS(5.0f)
#define vLOCK_ATTRACTOR_VELOCITY_THRESHOLD			MPH_TO_IPS(5000000.0f)
#define vSTATE_CHANGE_DELAY (1.0f)

extern float	screen_angle;
extern bool		gun_fired;

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CHorseCameraComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CHorseCameraComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CHorseCameraComponent::CHorseCameraComponent() : CBaseComponent()
{
	SetType( CRC_HORSECAMERA );
	
	m_state				= STATE_NORMAL;
	m_offset_height		= FEET(5.0f);
	m_offset_distance	= FEET(12.0f);
	m_offset_tilt		= 0.0f;
	m_alignment_rate	= 3.0f;

	m_orientation_matrix.Ident();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CHorseCameraComponent::~CHorseCameraComponent()
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseCameraComponent::InitFromStructure( Script::CStruct* pParams )
{
	uint32 subject_id;
	
	if (pParams->ContainsComponentNamed(CRCD(0x431c185, "subject")))
	{
		pParams->GetChecksum(CRCD(0x431c185, "subject"), &subject_id, Script::ASSERT);
		mp_subject = static_cast< CCompositeObject* >(CCompositeObjectManager::Instance()->GetObjectByID(subject_id));
		Dbg_MsgAssert(mp_subject, ("Vehicle camera given subject which is not a composite object"));
		mp_subject_vehicle_component = static_cast< CHorseComponent* >(mp_subject->GetComponent(CRC_HORSE));
		Dbg_MsgAssert(mp_subject_vehicle_component, ("HorseCameraComponent given subject which contains no HorseComponent"));
	}
		
	pParams->GetFloat( CRCD( 0x9213625f, "alignment_rate" ), &m_alignment_rate);
	pParams->GetFloat( CRCD( 0x14849b6d, "offset_height" ), &m_offset_height);
	pParams->GetFloat( CRCD( 0xbd3d3ca9, "offset_distance" ), &m_offset_distance);

	if( pParams->GetFloat( CRCD( 0x480e14e2, "offset_tilt"), &m_offset_tilt ))
	{
		// Convert tilt from degrees to radians.
		m_offset_tilt = Mth::DegToRad( m_offset_tilt );
	}
	
	reset_camera();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseCameraComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure( pParams );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseCameraComponent::Finalize( void )
{
	mp_input_component = GetInputComponentFromObject( GetObject());
	Dbg_Assert( mp_input_component );
}

	
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseCameraComponent::Update()
{
	// Decide whether we want to be active right now.
	if( mp_subject_vehicle_component )
	{
		if( !mp_subject_vehicle_component->ShouldUpdateCamera())
		{
			return;
		}
	}

	m_frame_length = Tmr::FrameLength();

	get_controller_input();

	calculate_attractor_direction();
	
	// Due to rounding errors this can sometimes be > |1|, which hoses acosf(), so limit here.
	float angular_distance = acosf( Mth::Clamp( Mth::DotProduct( m_direction, m_attractor_direction ), -1.0f, 1.0f ));
	if( angular_distance > Mth::PI / 2.0f )
	{
		angular_distance = Mth::PI - angular_distance;
	}
	
	bool	sign = Mth::CrossProduct( m_direction, m_attractor_direction )[Y] > 0.0f;
	float	step = m_alignment_rate * angular_distance * Tmr::FrameLength();
	if( step > angular_distance )
	{
		step = angular_distance;
	}
	
	m_direction.RotateY(( sign ? 1.0f : -1.0f ) * step );
	
	m_direction.Normalize();
	calculate_orientation_matrix();
	
	// Save off the orientation matrix at this point.
	Mth::Matrix saved_mat = m_orientation_matrix;

	m_orientation_matrix.RotateYLocal( m_lookaround_heading );
	m_orientation_matrix.RotateXLocal( m_lookaround_tilt - m_offset_tilt );

	// Test the weapon component to generate the 'sticky' targetting behavior.
	if( mp_best_target )
	{
		// Need to get the rider so we can get the base object and then the weapon component (messy).
		CCompositeObject*	p_obj		= mp_subject_vehicle_component->GetRider();
		if( p_obj )
		{
			CWeaponComponent* p_weapon	= GetWeaponComponentFromObject( p_obj );
			p_weapon->SetCurrentTarget( mp_best_target );
			Mth::Vector sight_pos = mp_subject->GetPos() + Mth::Vector( 0.0f, m_offset_height, 0.0f, 0.0f );
			p_weapon->SetSightPos( sight_pos );
			p_weapon->SetSightMatrix( m_orientation_matrix );

			float extra_heading_change, extra_tilt_change;
			p_weapon->ProcessStickyTarget( m_lookaround_heading_delta, m_lookaround_tilt_delta, &extra_heading_change, &extra_tilt_change );

			if(( extra_heading_change != 0.0f ) || ( extra_tilt_change != 0.0f ))
			{
				// Reset the matrix to what it was prior to the heading and tilt adjustments.
				m_orientation_matrix = saved_mat;

				m_lookaround_heading += extra_heading_change;
				m_orientation_matrix.RotateYLocal( m_lookaround_heading );

				m_lookaround_tilt += extra_tilt_change;
				m_orientation_matrix.RotateXLocal( m_lookaround_tilt - m_offset_tilt );
			}
		}
	}

	calculate_position();

	ApplyCameraCollisionDetection(
		m_pos, 
		m_orientation_matrix, 
		m_pos - m_offset_distance * m_orientation_matrix[Z], 
		m_pos - m_offset_distance * m_orientation_matrix[Z], 
		false, 
		false
	);
	
	m_pos[W] = 1.0f;
	m_orientation_matrix[X][W] = 0.0f;
	m_orientation_matrix[Y][W] = 0.0f;
	m_orientation_matrix[Z][W] = 0.0f;

	do_reticle();

	GetObject()->SetPos( m_pos );
	GetObject()->SetMatrix( m_orientation_matrix );
	GetObject()->SetDisplayMatrix( m_orientation_matrix );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CHorseCameraComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		case CRCC(0x469fd, "VehicleCamera_Reset"):
			RefreshFromStructure(pParams);
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
void CHorseCameraComponent::GetDebugInfo(Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info sent to C......Component::GetDebugInfo"));
	
	p_info->AddChecksum("mp_subject", mp_subject->GetID());

	CBaseComponent::GetDebugInfo(p_info);	  
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseCameraComponent::get_controller_input( void )
{
	CControlPad& control_pad = mp_input_component->GetControlPad();

	// Switch the aiming state if triggered.
	if( control_pad.m_R3.GetTriggered())
	{
		control_pad.m_R3.ClearTrigger();

		// Restore screen angle.
		screen_angle = 72.0f;
		Nx::CViewportManager::sSetScreenAngle( screen_angle );

		if( m_state == STATE_NORMAL )
			m_state = STATE_AIMING;
		else if( m_state == STATE_AIMING )
			m_state = STATE_NORMAL;
	}

	// Fire if triggered.
	gun_fired = false;
	if( control_pad.m_R1.GetTriggered())
	{
		gun_fired = true;
		control_pad.m_R1.ClearTrigger();
	}

	// Switch sniper mode if triggered and in aiming mode.
	if( control_pad.m_up.GetTriggered())
	{
		control_pad.m_up.ClearTrigger();

		// Unfortunately, pressing up on the left control stick will also cause m_up to be triggered, so check this
		// is really the d-pad press.
		if( control_pad.GetScaledLeftAnalogStickMagnitude() == 0.0f )
		{
			if( m_state == STATE_AIMING )
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
	}

	// Deal with adjusting aim direction.
	float tilt_target		= control_pad.m_scaled_rightY;
	float heading_target	= control_pad.m_scaled_rightX;

	if( m_state == STATE_NORMAL )
	{
		// Aiming input should be ignored.
		tilt_target		= 0.0f;
		heading_target	= 0.0f;
	}

	// Modulate with the variable used to damp cursor movement when aiming at a heading_target.
	heading_target	= heading_target * m_spin_modulator;
	tilt_target		= tilt_target * m_tilt_modulator;
		
	// Get script values.
	float heading_ka = Script::GetFloat( CRCD( 0xe31bc279, "GunslingerHorseLookaroundHeadingKa" ), Script::ASSERT );
	float heading_ea = Script::GetFloat( CRCD( 0x7d98eff7, "GunslingerHorseLookaroundHeadingEa" ), Script::ASSERT );
	float heading_ks = Script::GetFloat( CRCD( 0x10a2b331, "GunslingerHorseLookaroundHeadingKs" ), Script::ASSERT );
	float heading_es = Script::GetFloat( CRCD( 0x8e219ebf, "GunslingerHorseLookaroundHeadingEs" ), Script::ASSERT );

	// Calculate acceleration.
	float a = heading_ka * powf( Mth::Abs( heading_target ), heading_ea ) * (( heading_target > 0.0f ) ? 1.0f : ( heading_target < 0.0f ) ? -1.0f : 0.0f );

	// Calculate max speed.
	float s = heading_ks * powf( Mth::Abs( heading_target ), heading_es ) * (( heading_target > 0.0f ) ? 1.0f : ( heading_target < 0.0f ) ? -1.0f : 0.0f );

	m_lookaround_heading_angular_speed += a;

	if( s == 0.0f )
	{
		m_lookaround_heading_angular_speed = 0.0f;
	}
	else if((( s > 0.0f ) && ( m_lookaround_heading_angular_speed > s )) || (( s < 0.0f ) && ( m_lookaround_heading_angular_speed < s )))
	{
		m_lookaround_heading_angular_speed = s;
	}

	float lookaround_heading_speed = Script::GetFloat( CRCD( 0x8cf5f0cd, "GunslingerHorseLookaroundHeadingSpeed" ), Script::ASSERT );

	// Control stick left - reticle should move left.
	m_lookaround_heading_delta	= -m_lookaround_heading_angular_speed * lookaround_heading_speed * m_frame_length;
	m_lookaround_heading	   += m_lookaround_heading_delta;

	if( m_lookaround_heading > Mth::PI )
		m_lookaround_heading = m_lookaround_heading - ( 2 * Mth::PI );
	else if( m_lookaround_heading <= -Mth::PI )
		m_lookaround_heading = ( 2 * Mth::PI ) + m_lookaround_heading;

	if( Script::GetInteger( CRCD( 0x9edfc7af, "GunslingerInvertAiming" )) == 0 )
	{
		// Negate value if vertical aiming invert is not enabled.
		tilt_target = -tilt_target;
	}

	// Get script values.
	float tilt_ka = Script::GetFloat( CRCD( 0x3e8e974f, "GunslingerHorseLookaroundTiltKa" ), Script::ASSERT );
	float tilt_ea = Script::GetFloat( CRCD( 0xa00dbac1, "GunslingerHorseLookaroundTiltEa" ), Script::ASSERT );
	float tilt_ks = Script::GetFloat( CRCD( 0xcd37e607, "GunslingerHorseLookaroundTiltKs" ), Script::ASSERT );
	float tilt_es = Script::GetFloat( CRCD( 0x53b4cb89, "GunslingerHorseLookaroundTiltEs" ), Script::ASSERT );

	// Calculate acceleration.
	a = tilt_ka * powf( Mth::Abs( tilt_target ), tilt_ea ) * (( tilt_target > 0.0f ) ? 1.0f : ( tilt_target < 0.0f ) ? -1.0f : 0.0f );

	// Calculate max speed.
	s = tilt_ks * powf( Mth::Abs( tilt_target ), tilt_es ) * (( tilt_target > 0.0f ) ? 1.0f : ( tilt_target < 0.0f ) ? -1.0f : 0.0f );

	m_lookaround_tilt_angular_speed += a;

	if( s == 0.0f )
	{
		m_lookaround_tilt_angular_speed = 0.0f;
	}
	else if((( s > 0.0f ) && ( m_lookaround_tilt_angular_speed > s )) || (( s < 0.0f ) && ( m_lookaround_tilt_angular_speed < s )))
	{
		m_lookaround_tilt_angular_speed = s;
	}

	float lookaround_tilt_speed = Script::GetFloat( CRCD( 0xebbaa7e8, "GunslingerHorseLookaroundTiltSpeed" ), Script::ASSERT );
	m_lookaround_tilt_delta	= m_lookaround_tilt_angular_speed * lookaround_tilt_speed * m_frame_length;
	m_lookaround_tilt	   += m_lookaround_tilt_delta;

	if( m_lookaround_tilt > 1.1f )
		m_lookaround_tilt = 1.1f;
	else if( m_lookaround_tilt < -0.85f )
		m_lookaround_tilt = -0.85f;

	if( m_state == STATE_NORMAL )
	{
		// Smoothly move the lookaround values back to zero.

		m_lookaround_tilt		-= ( m_lookaround_tilt * 0.05f );
		m_lookaround_heading	-= ( m_lookaround_heading * 0.05f );
	}
}

	
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseCameraComponent::reset_camera( void )
{
	m_state	= STATE_NORMAL;

	m_attractor_direction = -mp_subject->GetMatrix()[Z];
	m_attractor_direction[Y] = 0.0f;
	m_attractor_direction.Normalize();
	
	calculate_attractor_direction();
	
	m_direction = m_attractor_direction;
	
	calculate_orientation_matrix();
	calculate_position();
	
	GetObject()->SetPos( m_pos );
	GetObject()->SetMatrix( m_orientation_matrix );
	GetObject()->SetDisplayMatrix(m_orientation_matrix);
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseCameraComponent::calculate_attractor_direction( void )
{
	if( m_state == STATE_NORMAL )
	{
		Mth::Vector vel_direction = -mp_subject_vehicle_component->GetVel();
		vel_direction[Y] = 0.0f;
		float vel = vel_direction.Length();
		vel_direction.Normalize();
	
		float vel_weight = Mth::ClampMax(vel / vVELOCITY_WEIGHT_DROP_THRESHOLD, 1.0f) * Mth::DotProduct(vel_direction, -mp_subject->GetMatrix()[Z]);
		
		m_attractor_direction = -mp_subject->GetMatrix()[Z];
		m_attractor_direction[Y] = 0.0f;
		m_attractor_direction.Normalize();
		
		if (vel_weight > 0.0f)
		{
			m_attractor_direction += vel_weight * vel_direction;
			m_attractor_direction.Normalize();
		}
	}
	else if( m_state == STATE_AIMING )
	{
		m_attractor_direction = m_direction;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseCameraComponent::calculate_orientation_matrix( void )
{
	m_orientation_matrix[Z] = m_direction;
	m_orientation_matrix[X] = Mth::CrossProduct( Mth::Vector( 0.0f, 1.0f, 0.0f, 0.0f ), m_orientation_matrix[Z] ).Normalize();
	m_orientation_matrix[Y] = Mth::CrossProduct( m_orientation_matrix[Z], m_orientation_matrix[X] );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseCameraComponent::calculate_position( void )
{
	Mth::Vector direction = m_orientation_matrix[Z];
	m_pos = mp_subject->GetPos() + Mth::Vector( m_offset_distance * direction[X], m_offset_height + ( m_offset_distance * direction[Y] ), m_offset_distance * direction[Z], 0.0f );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CHorseCameraComponent::do_reticle( void )
{
	// Need to get the rider so we can get the base object and then the weapon component (messy).
	CCompositeObject*	p_obj		= mp_subject_vehicle_component->GetRider();
	if( p_obj )
	{
		Mth::Vector reticle_min	= m_pos;
		Mth::Vector reticle_max;

		CWeaponComponent* p_weapon	= GetWeaponComponentFromObject( p_obj );

		p_weapon->SetSightMatrix( m_orientation_matrix );
		mp_best_target = p_weapon->GetCurrentTarget( reticle_min, &reticle_max );

		if( gun_fired )
		{
			p_weapon->Fire();
		}

		m_spin_modulator = p_weapon->GetSpinModulator();
		m_tilt_modulator = p_weapon->GetTiltModulator();

		p_weapon->DrawReticle();
	}
}












}
