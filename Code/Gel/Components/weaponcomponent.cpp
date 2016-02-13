//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       WeaponComponent.cpp
//* OWNER:          Dave
//* CREATION DATE:  06/10/03
//****************************************************************************

// The CWeaponComponent class is an skeletal version of a component
// It is intended that you use this as the basis for creating new
// components.  
// To create a new component called "Watch", (CWatchComponent):
//  - copy Weaponcomponent.cpp/.h to watchcomponent.cpp/.h
//  - in both files, search and replace "Weapon" with "Watch", preserving the case
//  - in WatchComponent.h, update the CRCD value of CRC_WATCH
//  - in CompositeObjectManager.cpp, in the CCompositeObjectManager constructor, add:
//		  	RegisterComponent(CRC_WATCH,			CWatchComponent::s_create); 
//  - and add the include of the header
//			#include <gel/components/watchcomponent.h> 
//  - Add it to build\gel.mkf, like:
//          $(NGEL)/components/WatchComponent.cpp\
//  - Fill in the OWNER (yourself) and the CREATION DATE (today's date) in the .cpp and the .h files
//	- Insert code as needed and remove generic comments
//  - remove these comments
//  - add comments specfic to the component, explaining its usage

#include <gel/components/weaponcomponent.h>
#include <gel/components/pedlogiccomponent.h>

#include <gel/object/compositeobjectmanager.h>
#include <gel/object/compositeobject.h>

#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

#include <sk/engine/feeler.h>


namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CBaseComponent* CWeaponComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CWeaponComponent );	
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CWeaponComponent::CWeaponComponent() : CBaseComponent()
{
	SetType( CRC_WEAPON );

	m_sight_pos.Set( 0.0f, 0.0f, 0.0f, 1.0f );
	m_sight_matrix.Identity();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CWeaponComponent::~CWeaponComponent()
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CWeaponComponent::InitFromStructure( Script::CStruct* pParams )
{
	// ** Add code to parse the structure, and initialize the component

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*
	It's commented out here.  You will also need to comment in the 
	definition in Weaponcomponent.h
void CWeaponComponent::Finalize()
{
	// Virtual function, can be overridden to provided finialization to 
	// a component after all components have been added to an object
	// Usually this consists of getting pointers to other components
	// Note:  It is GUARENTEED (at time of writing) that
	// Finalize() will be called AFTER all components are added
	// and BEFORE the CCompositeObject::Update function is called
	
	// Example:
	// mp_suspend_component =  GetSuspendComponentFromObject( GetObject() );
										
										
}
*/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CWeaponComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	// Default to just calline InitFromStructure()
	// but if that does not handle it, then will need to write a specific 
	// function here. 
	// The user might only want to update a single field in the structure
	// and we don't want to be asserting becasue everything is missing 
	
	InitFromStructure(pParams);
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CWeaponComponent::Update()
{
	// **  You would put in here the stuff that you would want to get run every frame
	// **  for example, a physics type component might update the position and orientation
	// **  and update the internal physics state 
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CBaseComponent::EMemberFunctionResult CWeaponComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
/*
        // @script | DoSomething | does some functionality
		case 0xbb4ad101:		// DoSomething
			DoSomething();
			break;

        // @script | ValueIsTrue | returns a boolean value
		case 0x769260f7:		// ValueIsTrue
		{
			return ValueIsTrue() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}
		break;
*/

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
void CWeaponComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CWeaponComponent::GetDebugInfo"));

	// Add any script components to the p_info structure,
	// and they will be displayed in the script debugger (qdebug.exe)
	// you will need to add the names to debugger_names.q, if they are not existing checksums

	/*	Example:
	p_info->AddInteger(CRCD(0x7cf2a233,"m_never_suspend"),m_never_suspend);
	p_info->AddFloat(CRCD(0x519ab8e0,"m_suspend_distance"),m_suspend_distance);
	*/
	
// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CWeaponComponent::ProcessStickyTarget( float heading_change, float tilt_change, float *p_extra_heading_change, float *p_extra_tilt_change )
{
	// Zero the extra chnage values.
	*p_extra_heading_change = 0.0f;
	*p_extra_tilt_change	= 0.0f;

	// If the player is trying to aim, this section of code tries to 'nudge' the aim in the right direction imperceptibly by
	// moving the cursor just a little more or less based on the requested movement of the cursor.
	if( mp_current_target )
	{
		if( heading_change != 0.0f )
		{
			// Figure default adjustment amount.
			float		heading_cursor_suck		= Script::GetFloat( CRCD( 0xd90b2dfb, "GunslingerLookaroundHeadingCursorSuck" ), Script::ASSERT );
			float		default_adjustment		= heading_change * heading_cursor_suck;

			Mth::Vector	immediate_camera_pos	= m_sight_pos;
			Mth::Vector ped_pos					= mp_current_target->GetPos() + Mth::Vector( 0.0f, 36.0f, 0.0f );
			Mth::Vector	ped_to_cam				= ped_pos - immediate_camera_pos;
			ped_to_cam[Y] = 0.0f;
			ped_to_cam.Normalize();

			// Figure exactly what the angular difference is.
			float		dp						= Mth::DotProduct( ped_to_cam, -m_sight_matrix[Z] );
			float		angular_difference		= acosf( dp );

			// If the angular difference is actually smaller than the default adjustment amount, make the default adjustment amount the angular difference.
			if( Mth::Abs( angular_difference ) < Mth::Abs( default_adjustment ))
			{
				default_adjustment = angular_difference;
			}

			// Now save off this matrix, and increase the rotation by some percentage of the rotation added to the lookaround heading.
			Mth::Matrix	saved_mat				= m_sight_matrix;
			m_sight_matrix.RotateYLocal( default_adjustment );

			// See if this brings us closer.
			float		dp2						= Mth::DotProduct( ped_to_cam, -m_sight_matrix[Z] );
			if( dp2 > dp )
			{
				// Yes it did, so store this change.
				*p_extra_heading_change			= default_adjustment;
			}
			else
			{
				// No it didn't so revert to the old matrix.
				m_sight_matrix = saved_mat;

				// Maybe this change went too far? Try undoing 10% of the last change.
				m_sight_matrix.RotateYLocal( -default_adjustment );

				// See if this brings us closer.
				float	dp3						= Mth::DotProduct( ped_to_cam, -m_sight_matrix[Z] );
				if( dp3 > dp )
				{
					// Yes it did, so store this change.
					*p_extra_heading_change		= -default_adjustment;
				}
				else
				{
					// No it didn't so revert to the old matrix.
					m_sight_matrix				= saved_mat;
				}
			}
		}
	}

	if( mp_current_target )
	{
		if( tilt_change != 0.0f )
		{
			// Figure default adjustment amount.
			float		tilt_cursor_suck		= Script::GetFloat( CRCD( 0x4ece2698, "GunslingerLookaroundTiltCursorSuck" ), Script::ASSERT );
			float		default_adjustment		= tilt_change * tilt_cursor_suck;

			Mth::Vector	immediate_camera_pos	= m_sight_pos;
			Mth::Vector ped_pos					= mp_current_target->GetPos() + Mth::Vector( 0.0f, 36.0f, 0.0f );
			Mth::Vector	ped_to_cam				= ped_pos - immediate_camera_pos;
			ped_to_cam.Normalize();

			// Figure exactly what the angular difference is.
			float		dp						= Mth::DotProduct( ped_to_cam, -m_sight_matrix[Z] );
			float		angular_difference		= acosf( dp );

			// If the angular difference is actually smaller than the default adjustment amount, make the default adjustment amount the angular difference.
			if( Mth::Abs( angular_difference ) < Mth::Abs( default_adjustment ))
			{
				default_adjustment = angular_difference;
			}

			// Now save off this matrix, and increase the rotation by some percentage of the rotation added to the lookaround heading.
			Mth::Matrix	saved_mat				= m_sight_matrix;
			m_sight_matrix.RotateXLocal( default_adjustment );

			// See if this brings us closer.
			float		dp2						= Mth::DotProduct( ped_to_cam, -m_sight_matrix[Z] );
			if( dp2 > dp )
			{
				// Yes it did, so store this change.
				*p_extra_tilt_change			= default_adjustment;
			}
			else
			{
				// No it didn't so revert to the old matrix.
				m_sight_matrix = saved_mat;

				// Maybe this change went too far? Try undoing 10% of the last change.
				m_sight_matrix.RotateXLocal( -default_adjustment );

				// See if this brings us closer.
				float	dp3						= Mth::DotProduct( ped_to_cam, -m_sight_matrix[Z] );

				if( dp3 > dp )
				{
					// Yes it did, so store this change.
					*p_extra_tilt_change		= -default_adjustment;
				}
				else
				{
					// No it didn't so revert to the old matrix.
					m_sight_matrix				= saved_mat;
				}
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CWeaponComponent::DrawReticle( void )
{
	// Draw the reticle, red if enemy is within sights, white otherwise.
	if( m_spin_modulator < 1.0f )
	{
		Gfx::AddDebugStar( m_reticle_max, 24.0f, MAKE_RGB( 255, 0, 0 ), 1 );
	}
	else
	{
		Gfx::AddDebugStar( m_reticle_max, 24.0f, MAKE_RGB( 255, 255, 255 ), 1 );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CWeaponComponent::Fire( void )
{
	if( mp_current_target )
	{
		mp_current_target->SelfEvent( CRCD( 0xfaeec40f, "SkaterInAvoidRadius" ));
	}

	Script::RunScript( CRCD( 0xe03997d0, "PlayGunshot" ));
}



// These values should really be specified per-weapon.
const float SPIN_MODULATION_ANGLE = 0.9848f;	// Cosine of 10 degrees.


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CCompositeObject* CWeaponComponent::GetCurrentTarget( Mth::Vector& start_pos, Mth::Vector* p_reticle_max )
{
	// This is the maximum distance that we will check for targets.
	float target_distance = Script::GetFloat( CRCD( 0x89495011,"GunslingerTargetDistance" ), Script::ASSERT );

	Mth::Vector reticle_min		= start_pos;
	m_reticle_max				= reticle_min - ( m_sight_matrix[Z] * target_distance );

	// Ignore faces based on face flags.
	CFeeler feeler;
	feeler.SetIgnore((uint16)( mFD_NON_COLLIDABLE | mFD_NON_CAMERA_COLLIDABLE ), 0 );
	feeler.SetLine( reticle_min, m_reticle_max );
	bool collision = feeler.GetCollision( true );
	if( collision )
	{
		m_reticle_max = feeler.GetPoint();
		
		// Set the new target distance at the point of contact, to avoid considering targets beyond the collision point.
		target_distance	= target_distance * feeler.GetDist();
	}

	// Only targets within the allowed angle will be considered.
							mp_current_target	= NULL;
	float					best_dp				= SPIN_MODULATION_ANGLE;
	Mth::Vector				best_pos;

	// Rather than cycling through the ped logic components, eventually everything that is targettable should contain a targetting component.
	Obj::CPedLogicComponent *p_ped_logic_component = static_cast<Obj::CPedLogicComponent*>( Obj::CCompositeObjectManager::Instance()->GetFirstComponentByType( CRC_PEDLOGIC ));
	while( p_ped_logic_component )
	{
		Obj::CCompositeObject *p_ped = p_ped_logic_component->GetObject();

		// Have to hack in a height adjustment here - the default position appears to be at ground level.
		Mth::Vector ped_pos		= p_ped->GetPos() + Mth::Vector( 0.0f, 36.0f, 0.0f );

		// Want to get the position of this ped.
		Mth::Vector	ped_to_cam	= ped_pos - reticle_min;

		// Check not too far away.
		if( ped_to_cam.LengthSqr() < ( target_distance * target_distance ))
		{
			ped_to_cam.Normalize();

			// Calculate angle this ped subtends at the camera position.
			float		dp			= Mth::DotProduct( ped_to_cam, -m_sight_matrix[Z] );
			if( dp > best_dp )
			{
				best_dp				= dp;
				best_pos			= ped_pos;
				mp_current_target	= p_ped;
			}
		}
		p_ped_logic_component			= static_cast<Obj::CPedLogicComponent*>( p_ped_logic_component->GetNextSameType());
	}

	// Reset spin and tilt modulators.
	m_spin_modulator	= 1.0f;
	m_tilt_modulator	= 1.0f;

	if( mp_current_target )
	{
		// We have a potential candidate for targetting.
		// However we still want to check that it is close enough to our line of sight.
		float dist_to_ped	= Mth::Distance( best_pos, reticle_min );
		float dist_to_line	= dist_to_ped * sqrtf( 1.0f - ( best_dp * best_dp ));

		// This value should really be per-weapon in some script array.
		if( dist_to_line < Script::GetFloat( CRCD( 0x755235db, "GunslingerLookaroundModulationDistance" ), Script::ASSERT ))
		{
			// Is this the same target as last time?
//			if( mp_best_target == p_selected_target )
//			{
//				// If so, increase the targetting timer.
//				target_selection_timer += frame_length;
//			}
//			else
//			{
//				// Otherwise, reset the timer.
//				p_selected_target		= mp_best_target;
//				target_selection_timer	= frame_length;
//			}

			// Modulator gets smaller as result tends to 1.0.
			float max_damping = Script::GetFloat( CRCD( 0xef314a12, "GunslingerLookaroundDamping" ), Script::ASSERT );
			m_spin_modulator = 1.0f - ( max_damping * (( best_dp - SPIN_MODULATION_ANGLE ) / ( 1.0f - SPIN_MODULATION_ANGLE )));

//			if( gun_fired )
//			{
//				// Make this ped fall down.
//				mp_best_target->SelfEvent( CRCD( 0xfaeec40f, "SkaterInAvoidRadius" ));
//			}
		}
		else
		{
			mp_current_target = NULL;
		}
	}

	// Currently, set tilt modulator to be the same as spin modulator.
	m_tilt_modulator = m_spin_modulator;

	if( p_reticle_max )
	{
		*p_reticle_max = m_reticle_max;
	}

	return mp_current_target;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
}
