//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterSoundComponent.cpp
//* OWNER:			Dan
//* CREATION DATE:  2/27/3
//****************************************************************************

#ifndef __COMPONENTS_SKATERSOUNDCOMPONENT_H__
#define __COMPONENTS_SKATERSOUNDCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#include <sk/objects/skaterflags.h>

#include <gfx/nxflags.h>

#define		CRC_SKATERSOUND CRCD(0x187574fa, "SkaterSound")

#define		GetSkaterSoundComponent() ((Obj::CSkaterSoundComponent*)GetComponent(CRC_SKATERSOUND))
#define		GetSkaterSoundComponentFromObject(pObj) ((Obj::CSkaterSoundComponent*)(pObj)->GetComponent(CRC_SKATERSOUND))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CSkaterSoundComponent : public CBaseComponent
{
public:
    CSkaterSoundComponent();
    virtual ~CSkaterSoundComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	void							PlayLandSound ( float speed_fraction );
	void							PlayJumpSound ( float speed_fraction );
	void							PlayBonkSound ( float speed_fraction );
	void							PlayCessSound ( float speed_fraction );
	
	void							PlayLandSound ( float speed_fraction, ETerrainType terrain );
	void							PlayJumpSound ( float speed_fraction, ETerrainType terrain );
	void							PlayBonkSound ( float speed_fraction, ETerrainType terrain );
	void							PlayCessSound ( float speed_fraction, ETerrainType terrain );
	
	void							PlayJumpSound ( float speed_fraction, ETerrainType terrain, EStateType stateType );
	
	void							SetTerrain ( ETerrainType terrain ) { m_terrain = terrain; }
	void							SetLastTerrain ( ETerrainType terrain ) { m_lastTerrain = terrain; }
	void							SetState ( EStateType state ) { m_stateType = state; }
	void							SetIsRailSliding ( bool is_sliding ) { m_is_rail_sliding = is_sliding; }
	
	void							SetMaxSpeed ( float max_speed ) { m_max_speed = max_speed; }
	
	void							SetVolumeMultiplier ( float mult );
private:
	EStateType						m_stateType;
	ETerrainType					m_terrain;
	ETerrainType					m_lastTerrain;

	bool							m_is_rail_sliding;
	
	float							m_max_speed;
	float							m_vol_mult;
};

}

#endif
