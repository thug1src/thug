//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterLoopingSoundComponent.h
//* OWNERD:			Dan
//* CREATION DATE:  2/26/3
//****************************************************************************

#ifndef __COMPONENTS_SKATERLOOPINGSOUNDCOMPONENT_H__
#define __COMPONENTS_SKATERLOOPINGSOUNDCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>
#include <gel/environment/terrain.h>
#include <gel/soundfx/soundfx.h>

#include <sk/objects/skaterflags.h>

#define		CRC_SKATERLOOPINGSOUND CRCD(0xb5f1e9d2, "SkaterLoopingSound")

#define		GetSkaterLoopingSoundComponent() ((Obj::CSkaterLoopingSoundComponent*)GetComponent(CRC_SKATERLOOPINGSOUND))
#define		GetSkaterLoopingSoundComponentFromObject(pObj) ((Obj::CSkaterLoopingSoundComponent*)(pObj)->GetComponent(CRC_SKATERLOOPINGSOUND))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CSkaterPhysicsControlComponent;

	static const float vSS_MAX_PITCH_CHANGE_WITHOUT_UPDATE = 0.5f;
	static const float vSS_MAX_PERCENTAGE_VOLUME_CHANGE_WITHOUT_UPDATE = 0.5f;
	
	static const float vSS_WHEELSPIN_MIN_PITCH = 100.0f;
	static const float vSS_WHEELSPIN_MAX_PITCH = 110.0f;
	static const float vSS_MIN_WHEELSPIN_TIME = 3.0f;
	
	// looping sound used when skater is in the air
	static const Env::STerrainSoundInfo AIR_LOOPING_SOUND_INFO =
	{
		"wheels01",
		CRCD(0x24257432, "wheels01"),
		vSS_WHEELSPIN_MIN_PITCH,
		vSS_WHEELSPIN_MAX_PITCH
	};
	
class CSkaterLoopingSoundComponent : public CBaseComponent
{
public:
    CSkaterLoopingSoundComponent();
    virtual ~CSkaterLoopingSoundComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
	virtual void					Finalize (   );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
	virtual void					Suspend ( bool suspend );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	void							StopLoopingSound (   );

	void							UnPause();
	
	void							SetActive ( bool active );

	void							SetIsBailing( bool is_bailing );
	void							SetIsRailSliding( bool is_sliding );

	void							SetState( EStateType new_state );
	void							SetTerrain( ETerrainType terrain );

	void							SetSpeedFraction( float speed_fraction );
	void							SetVolumeMultiplier( float mult );
private:
	// the ID of the current looping sound
	uint32							m_looping_sound_id;
	
	// checksum of the current looping sound
	uint32							m_looping_sound_checksum;
	
	// holds the last frame's looping sound pitch so that can be used when the skater goes airborne
	float							m_wheelspin_pitch;
	
	// optimization variables
	float							m_last_pitch;
	Sfx::sVolume					m_last_volume;
	
	// constant characteristics
	
	// the drop in pitch of the air looping sound per second
	float m_wheelspin_end_pitch;
	
	// the final pitch of the air looping sound; when reached the sound is turned off
	float m_wheelspin_pitch_step;

	// peer components
	CSkaterPhysicsControlComponent*	mp_physics_control_component;

	ETerrainType					m_terrain;
	EStateType	m_StateType;

	Env::STerrainSoundInfo			m_sound_info;
	bool							m_have_sound_info;
	bool							m_update_sound_info;
	bool							m_unpause;

	bool							m_is_bailing;
	bool							m_is_rail_sliding;

	float							m_speed_fraction;
	float							m_vol_mult;
	
	bool							m_active;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CSkaterLoopingSoundComponent::SetActive ( bool active )
{
	if (m_active != active)
	{
		Suspend(!active);
		m_active = active;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void	CSkaterLoopingSoundComponent::UnPause()
{
	m_unpause = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CSkaterLoopingSoundComponent::SetState( EStateType state )
{
	m_update_sound_info |= (m_StateType != state);
	m_StateType = state;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CSkaterLoopingSoundComponent::SetTerrain( ETerrainType terrain )
{
	m_update_sound_info |= (m_terrain != terrain);
	m_terrain = terrain;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CSkaterLoopingSoundComponent::SetIsBailing( bool is_bailing )
{
	m_is_bailing = is_bailing;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CSkaterLoopingSoundComponent::SetIsRailSliding( bool is_rail_sliding )
{
	m_update_sound_info |= (m_is_rail_sliding != is_rail_sliding);
	m_is_rail_sliding = is_rail_sliding;
}

}

#endif
