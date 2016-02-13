//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterStateComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/31/3
//****************************************************************************

#include <sk/components/skaterstatecomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>

/*
 * Holds all skater state which is needed by both local and nonlocal clients.  This way, code external to the skater can access this information in a
 * consistent manner, without having to know which components within the skater are controling the state.
 *
 * Currently, state within the core physics component has not been moved into theis component.
 */
 
namespace Obj
{
//											Fireball										
static uint32 s_powerups[vNUM_POWERUPS] = { 0xd039432c };

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CSkaterStateComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterStateComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterStateComponent::CSkaterStateComponent() : CBaseComponent()
{
	int i;

	SetType( CRC_SKATERSTATE );
	for( i = 0; i < vNUM_POWERUPS; i++ )
	{
		m_powerups[i] = false;
	}
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterStateComponent::~CSkaterStateComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterStateComponent::InitFromStructure( Script::CStruct* pParams )
{
	m_state = AIR;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterStateComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterStateComponent::Update()
{
	Suspend(true);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterStateComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
        // @script | DoingTrick | true if we're doing a trick
		case 0x58ad903f: // DoingTrick
			return DoingTrick() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			break;
			
		case 0xb07ac662: // HasPowerup
		{
			uint32 type;

			pParams->GetChecksum( NONAME, &type, true );
			return HasPowerup( type ) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			break;
		}

		case 0xe11b7ca:	// PickedUpPowerup
		{
			uint32 type;

			pParams->GetChecksum( NONAME, &type, true );
			PickedUpPowerup( type );
			break;
		}
		
		// @script | GetTerrain | returns the number of the terrain in 'terrain'
		case CRCC(0x44ba5fce, "GetTerrain"):
			pScript->GetParams()->AddInteger(CRCD(0x3789ac4e, "terrain"), m_terrain);
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

void CSkaterStateComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterStateComponent::GetDebugInfo"));
	
	const uint32 p_state_checksums [   ] =
	{
		CRCD(0x58007c97, "GROUND"),
		CRCD(0x439f4704, "AIR"),
		CRCD(0xec0a1009, "WALL"),
		CRCD(0xa549b57b, "LIP"),
		CRCD(0xa6a3147e, "RAIL"),
		CRCD(0xcf74f6b7, "WALLPLANT")
	};
	
	const uint32 p_flag_checksums [ NUM_ESKATERFLAGS ] =
	{
		CRCD(0x42f41014, "TENSE"),
		CRCD(0x0c7a712c, "FLIPPED"),
		CRCD(0xb39b4f1b, "VERT_AIR"),
		CRCD(0xc6bdeafc, "TRACKING_VERT"),
		CRCD(0x7747d16a, "LAST_POLY_WAS_VERT"),
		CRCD(0x0b6c902c, "CAN_BREAK_VERT"),
		CRCD(0x1261f6a0, "CAN_RERAIL"),
		CRCD(0x2bdce1e1, "RAIL_SLIDING"),
		CRCD(0xfb2e505c, "CAN_HIT_CAR"),
		CRCD(0xb2791a2f, "AUTOTURN"),
		CRCD(0x21523880, "IS_BAILING"),
		CRCD(0xe8e7a9a1, "SPINE_PHYSICS"),
		CRCD(0x4b45106a, "IN_RECOVERY"),
		CRCD(0x9c6a7e41, "SKITCHING"),
		CRCD(0x468c28b6, "OVERRIDE_CANCEL_GROUND"),
		CRCD(0xa29e3a92, "SNAPPED_OVER_CURB"),
		CRCD(0x4424288f, "SNAPPED"),
		CRCD(0x0849fb13, "IN_ACID_DROP"),
		CRCD(0x35f996fb, "AIR_ACID_DROP_DISALLOWED"),
		CRCD(0xd1c9cb24, "CANCEL_WALL_PUSH"),
		CRCD(0x260e0844, "NO_ORIENTATION_CONTROL"),
		CRCD(0x524ea0a3, "NEW_RAIL")
	};
	
	p_info->AddChecksum(CRCD(0x109b9260, "m_state"), p_state_checksums[m_state]);
	for (int flag = 0; flag < NUM_ESKATERFLAGS; flag++)
	{
		p_info->AddChecksum(p_flag_checksums[flag], GetFlag(static_cast< ESkaterFlag >(flag)) ? CRCD(0x203b372, "true") : CRCD(0xd43297cf, "false"));
	}
	
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterStateComponent::Reset (   )
{
	SetDoingTrick(false);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static int	s_get_powerup_index( uint32 type )
{
	int i;

	for( i = 0; i < vNUM_POWERUPS; i++ )
	{
		if( s_powerups[i] == type )
		{
			return i;
		}
	}

	return -1;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CSkaterStateComponent::HasPowerup( uint32 type )
{
	int index;

	index = s_get_powerup_index( type );
	if( index == -1 )
	{
		return false;
	}
	
	return m_powerups[index];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterStateComponent::ClearPowerups( void )
{
	int i;

	for( i = 0; i < vNUM_POWERUPS; i++ )
	{
		m_powerups[i] = false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterStateComponent::PickedUpPowerup( uint32 type )
{
	int index;

	index = s_get_powerup_index( type );
	if( index == -1 )
	{
		return;
	}
	
	m_powerups[index] = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}
