//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterLoopingSoundComponent.cpp
//* OWNERD:			Dan
//* CREATION DATE:  2/26/3
//****************************************************************************

#include <sk/components/skaterloopingsoundcomponent.h>
#include <sk/components/skaterphysicscontrolcomponent.h>
#include <sk/modules/skate/skate.h>
#include <sk/objects/skater.h>
#include <sk/objects/skaterflags.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/utils.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent* CSkaterLoopingSoundComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterLoopingSoundComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterLoopingSoundComponent::CSkaterLoopingSoundComponent() : CBaseComponent()
{
	SetType( CRC_SKATERLOOPINGSOUND );
	
	m_is_bailing = m_is_rail_sliding = false;
	m_have_sound_info = false;
	m_update_sound_info = true;
	m_StateType = AIR;
	m_vol_mult = 1.0f;
	m_active = true;
	m_unpause = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterLoopingSoundComponent::~CSkaterLoopingSoundComponent()
{
	StopLoopingSound();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterLoopingSoundComponent::InitFromStructure( Script::CStruct* pParams )
{
	m_wheelspin_pitch_step = (vSS_WHEELSPIN_MIN_PITCH - (vSS_WHEELSPIN_MIN_PITCH - vSS_WHEELSPIN_MAX_PITCH / 2.0f)) / vSS_MIN_WHEELSPIN_TIME;
	m_wheelspin_end_pitch = vSS_WHEELSPIN_MIN_PITCH - vSS_WHEELSPIN_MAX_PITCH / 2.0f;
	pParams->GetFloat( CRCD(0xf1a99b27,"volume_mult"), &m_vol_mult, Script::NO_ASSERT );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterLoopingSoundComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterLoopingSoundComponent::Finalize (   )
{
	mp_physics_control_component = GetSkaterPhysicsControlComponentFromObject(GetObject());
	
	// Dbg_Assert(mp_physics_control_component);
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterLoopingSoundComponent::Update()
{
	if (!m_active)
	{
		Suspend(true);
		return;
	}
	
	// update looping sounds
	
	Sfx::sVolume volume;
	float pitch;

	switch ( m_StateType )
	{
		case RAIL:
		{
			if ( m_is_bailing )
			{
				m_have_sound_info = false;
				break;
			}
			
			Env::ETerrainActionType table;
			if ( m_is_rail_sliding )
			{
				table = Env::vTABLE_SLIDE;
			}
			else								
			{
				table = Env::vTABLE_GRIND;
			}
			
			if (m_update_sound_info)
			{
				m_have_sound_info = Env::CTerrainManager::sGetTerrainSoundInfo(&m_sound_info, m_terrain, table);
				m_update_sound_info = false;
			}
			
			break;
		}
		
		case GROUND:
			if (m_update_sound_info)
			{
				m_have_sound_info = Env::CTerrainManager::sGetTerrainSoundInfo(&m_sound_info, m_terrain, Env::vTABLE_WHEELROLL);			
				m_update_sound_info = false;
			}

			break;
			
		case AIR:
			if (m_update_sound_info)
			{
				m_have_sound_info = true;
				m_sound_info = AIR_LOOPING_SOUND_INFO;
				m_update_sound_info = false;
			}
			
			break;
			
		default:
			break;
	} // END switch on skater state

	// if the sound has changed, turn off the old one
	if (m_looping_sound_id && (!m_have_sound_info || m_looping_sound_checksum != m_sound_info.m_soundChecksum))
	{
		Sfx::CSfxManager::Instance()->StopSound(m_looping_sound_id);
		m_looping_sound_id = 0;
	}
	
	// we have no sound to play
	if (!m_have_sound_info) return;
	
	// setup the sound's pitch and volume based on the skater's state
	
	Sfx::CSfxManager* p_sfx_manager = Sfx::CSfxManager::Instance();
	
	// adjust volume based of skater's offset from the nearest camera
	p_sfx_manager->SetVolumeFromPos(&volume, GetObject()->GetPos(), p_sfx_manager->GetDropoffDist(m_sound_info.m_soundChecksum));
	
	// if the skater is in the air and this isn't the first frame we've been playing the air looping sound
	if ( m_StateType == AIR && m_looping_sound_id )
	{
		// drop the pitch over time
		if (m_wheelspin_pitch > m_wheelspin_end_pitch)
		{
			m_wheelspin_pitch -= m_wheelspin_pitch_step * Tmr::FrameLength();
		}
		// then kill the volume
		else
		{
			volume.SetSilent();
		}
		
		pitch = m_wheelspin_pitch;
	}
	else
	{
		// adjust the volume and pitch based on the speed
		 
		if ( m_speed_fraction > 0.0f)
		{
			volume.PercentageAdjustment(Env::CTerrainManager::sGetVolPercent(&m_sound_info, 100.0f * m_speed_fraction * m_vol_mult));
			
			pitch = m_speed_fraction * (m_sound_info.m_maxPitch - m_sound_info.m_minPitch) + m_sound_info.m_minPitch;
			
			// save the current pitch incase we are in the air next frame
			m_wheelspin_pitch = pitch;
		}
		else
		{
			volume.SetSilent();
			m_wheelspin_pitch = pitch = 0.0f;
		}
	}
	
	// if the volume is zero
	if (volume.IsSilent())
	{
		// stop playing the sound
		if (m_looping_sound_id)
		{
			Sfx::CSfxManager::Instance()->StopSound(m_looping_sound_id);
			m_looping_sound_id = 0;
		}
		return;
	}
	
	// NOTE: removed until I can figure out what to do with this
	// adjust the sound based on doppler effects
	// if ( Nx::CViewportManager::sGetScreenMode( ) == 0 ) // that zero should be an enum or something...
	// {
		// sfx_manager->AdjustPitchForDoppler( &pitch, mp_physics->m_pos, mp_physics->m_old_pos, mp_physics->m_time, Nx::CViewportManager::sGetActiveCamera( 0 ) );
	// }
	
	// NOTE: removing all replay code for now
	// save pitch information for the replay code
	// m_pitch_min = sound_info.m_minPitch;
	// m_pitch_max = sound_info.m_maxPitch;
	
	// if we're not already playing a sound
	if (!m_looping_sound_id)
	{
		m_last_volume = volume;
		m_last_pitch = pitch;
		
		// start the sound
		m_looping_sound_id = p_sfx_manager->PlaySound(m_sound_info.m_soundChecksum, &volume, pitch, 0, NULL, m_sound_info.mp_soundName);
		if (!m_looping_sound_id)
		{
			return;
		}
		
		// save the checksum of the currently playing sound
		m_looping_sound_checksum = m_sound_info.m_soundChecksum;
	}
	
	// if we need to update the sound already playing; since we scale all channels equally, we can get away with only checking the first volume
	else if (m_unpause || Mth::Abs(volume.m_channels[0] - m_last_volume.m_channels[0]) > vSS_MAX_PERCENTAGE_VOLUME_CHANGE_WITHOUT_UPDATE
		|| Mth::Abs(pitch - m_last_pitch) > vSS_MAX_PITCH_CHANGE_WITHOUT_UPDATE)
	{
		m_last_volume = volume;
		m_last_pitch = pitch;
		m_unpause = false;
		
		p_sfx_manager->UpdateLoopingSound(m_looping_sound_id, &volume, pitch);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterLoopingSoundComponent::CallMemberFunction ( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		case CRCC(0xede3935f, "SkaterLoopingSound_TurnOn"):
			SetActive(true);
			break;
			
		case CRCC(0x9731e193, "SkaterLoopingSound_TurnOff"):
			SetActive(false);
			break;
			
		case CRCC(0xb1e7291, "PlayAnim"):
			// if a transition anim is interrupted, we must turn the looping sounds on here
			Dbg_MsgAssert( mp_physics_control_component, ( "Don't call PlayAnim on a non-skater" ) );
			
			Script::CStruct* p_anim_tags_struct;
			if (GetObject()->GetTags()
				&& mp_physics_control_component->IsSkating()
				&& GetObject()->GetTags()->GetStructure(CRCD(0x5db4115f, "AnimTags"), &p_anim_tags_struct)
				&& p_anim_tags_struct->ContainsFlag(CRCD(0x910d77c1, "WalkToSkateTransition")))
			{
				Suspend(false);
				m_active = true;
			}
			return CBaseComponent::MF_NOT_EXECUTED;
			
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}

	return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterLoopingSoundComponent::GetDebugInfo ( Script::CStruct *p_info )
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterLoopingSoundComponent::GetDebugInfo"));
	
	p_info->AddInteger("m_looping_sound_id", m_looping_sound_id);
	p_info->AddChecksum("m_looping_sound_checksum", m_looping_sound_checksum);

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterLoopingSoundComponent::Suspend ( bool suspend )
{
	CBaseComponent::Suspend(suspend);
	
	if (suspend)
	{
		StopLoopingSound();
	}
	else
	{
		m_update_sound_info = true;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterLoopingSoundComponent::StopLoopingSound (   )
{
	if (m_looping_sound_id)
	{
		Sfx::CSfxManager::Instance()->StopSound(m_looping_sound_id);
		m_looping_sound_id = 0;
	}
}
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterLoopingSoundComponent::SetSpeedFraction( float speed_fraction )
{
	m_speed_fraction = speed_fraction;
	if ( m_speed_fraction > 1.0f )
	{
		m_speed_fraction = 1.0f;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterLoopingSoundComponent::SetVolumeMultiplier( float mult )
{
	Dbg_MsgAssert( mult >= 0.0f && mult <= 1.0f, ( "SetVolumeMultiplier called with bad mult value: %f", mult ) );
	m_vol_mult = mult;
}

}
