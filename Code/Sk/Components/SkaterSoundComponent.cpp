//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterSoundComponent.cpp
//* OWNER:			Dan
//* CREATION DATE:  2/27/3
//****************************************************************************

#include <sk/components/skatersoundcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/environment/terrain.h>

#include <sk/modules/skate/skate.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent* CSkaterSoundComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterSoundComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterSoundComponent::CSkaterSoundComponent() : CBaseComponent()
{
	SetType( CRC_SKATERSOUND );
	
	m_is_rail_sliding = false;
	m_max_speed = 1.0f;
	m_vol_mult = 1.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterSoundComponent::~CSkaterSoundComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterSoundComponent::InitFromStructure( Script::CStruct* pParams )
{
	float vol_mult;
	if (pParams->GetFloat( CRCD(0xf1a99b27,"volume_mult"), &vol_mult))
	{
		SetVolumeMultiplier(vol_mult);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterSoundComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterSoundComponent::Update()
{
	// As a minor optimization, CSkaterSoundComponent is always suspended.
	Suspend(true);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterSoundComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
        // @script | PlayJumpSound |
		case CRCC(0xe338369, "PlayJumpSound"):
		{
			switch ( m_stateType )
			{
				case GROUND:
				case AIR:
					PlayJumpSound(GetObject()->GetVel().Length() / m_max_speed);
					break;
					 
				case RAIL:
					PlayJumpSound(GetObject()->GetVel().Length() / m_max_speed);
				
				default:
					break;
			}
			break;
		}
		
        // @script | PlayLandSound | 
		case CRCC(0x2c779f22, "PlayLandSound"):
		{
			switch ( m_stateType )
			{
				case GROUND:
				case AIR:
					PlayLandSound(Mth::Abs(GetObject()->GetVel()[Y]) / m_max_speed);
					break;
					
				case RAIL:
					PlayLandSound(GetObject()->GetVel().Length() / m_max_speed);
					break;
					
				default:
					break;
			}
			break;
		}
		
		// @script | PlayBonkSound |
		case CRCC(0x0069e457, "PlayBonkSound"):
		{
			PlayBonkSound(GetObject()->GetVel().Length() / m_max_speed);
			break;
		}
		
        // @script | PlayCessSound | 
		case CRCC(0x6f5e9124, "PlayCessSound"):
		{
			PlayCessSound(GetObject()->GetVel().Length() / m_max_speed);
			break;
		}

		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}

    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterSoundComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterSoundComponent::GetDebugInfo"));

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterSoundComponent::PlayLandSound ( float speed_fraction )
{
	ETerrainType terrain;
	if ( m_stateType == GROUND || m_stateType == AIR )
	{
		terrain = m_lastTerrain;
	}
	else
	{
		terrain = m_terrain;
	}
	
	PlayLandSound(speed_fraction, terrain);
}
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterSoundComponent::PlayLandSound ( float speed_fraction, ETerrainType terrain )
{
	Env::ETerrainActionType table;
	if ( m_stateType == RAIL )
	{
		table = m_is_rail_sliding ? Env::vTABLE_SLIDELAND : Env::vTABLE_GRINDLAND;
	}
	else
	{
		table = Env::vTABLE_LAND;
	}
	
	speed_fraction *= 100.0f;
	
	#ifdef __NOPT_ASSERT__
	if (Script::GetInteger(CRCD(0xb9ba2d27, "debug_skater_triggered_sounds")))
	{
		Dbg_Message("Playing sound [land]:   %.1f", speed_fraction);
	}
	#endif
	
	Env::CTerrainManager::sPlaySound(table, terrain, GetObject()->GetPos(), speed_fraction * m_vol_mult, speed_fraction, speed_fraction);
}
   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterSoundComponent::PlayJumpSound ( float speed_fraction )
{
	ETerrainType terrain;
	if ( m_stateType == GROUND || m_stateType == AIR )
	{
		terrain = m_lastTerrain;
	}
	else
	{
		terrain = m_terrain;
	}
	
	PlayJumpSound(speed_fraction, terrain);
}
   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterSoundComponent::PlayJumpSound ( float speed_fraction, ETerrainType terrain )
{
	PlayJumpSound(speed_fraction, terrain, m_stateType);
}
   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterSoundComponent::PlayJumpSound ( float speed_fraction, ETerrainType terrain, EStateType stateType )
{
	Env::ETerrainActionType table;
	if ( stateType == RAIL )
	{
		table = m_is_rail_sliding ? Env::vTABLE_SLIDEJUMP : Env::vTABLE_GRINDJUMP; 
	}
	else
	{
		table = Env::vTABLE_JUMP;
	}
	
	speed_fraction *= 100.0f;
	
	#ifdef __NOPT_ASSERT__
	if (Script::GetInteger(CRCD(0xb9ba2d27, "debug_skater_triggered_sounds")))
	{
		Dbg_Message("Playing sound [ollie]:  %.1f", speed_fraction);
	}
	#endif

	Env::CTerrainManager::sPlaySound(table, terrain, GetObject()->GetPos(), speed_fraction * m_vol_mult, speed_fraction, speed_fraction);
}
   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterSoundComponent::PlayBonkSound ( float speed_fraction )
{
	PlayBonkSound(speed_fraction, m_terrain);
}
   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterSoundComponent::PlayBonkSound ( float speed_fraction, ETerrainType terrain )
{
	speed_fraction *= 100.0f;
	
	#ifdef __NOPT_ASSERT__
	if (Script::GetInteger(CRCD(0xb9ba2d27, "debug_skater_triggered_sounds")))
	{
		Dbg_Message("Playing sound [bonk]:   %.1f", speed_fraction);
	}
	#endif

	Env::CTerrainManager::sPlaySound(Env::vTABLE_BONK, terrain, GetObject()->GetPos(), speed_fraction * m_vol_mult, speed_fraction, speed_fraction);
}
   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterSoundComponent::PlayCessSound ( float speed_fraction )
{
	PlayCessSound(speed_fraction, m_terrain);
}
   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterSoundComponent::PlayCessSound ( float speed_fraction, ETerrainType terrain )
{
	speed_fraction *= 100.0f;
	
	#ifdef __NOPT_ASSERT__
	if (Script::GetInteger(CRCD(0xb9ba2d27, "debug_skater_triggered_sounds")))
	{
		Dbg_Message("Playing sound [revert]: %.1f", speed_fraction);
	}
	#endif

	Env::CTerrainManager::sPlaySound(Env::vTABLE_CESS, terrain, GetObject()->GetPos(), speed_fraction * m_vol_mult, speed_fraction, speed_fraction);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterSoundComponent::SetVolumeMultiplier( float mult )
{
	Dbg_MsgAssert( mult >= 0.0f && mult <= 1.0f, ( "SetVolumeMultiplier called with bad mult value: %f", mult ) );
	m_vol_mult = mult;
}

}
