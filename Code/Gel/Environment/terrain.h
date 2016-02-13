/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Gel Module			 									**
**																			**
**	File name:		gel/environment/terrain.h								**
**																			**
**	Created by:		Garrett Feb. 2003										**
**																			**
**	Description:	Terrains and its properties (soundFX, etc.)				**
**																			**
*****************************************************************************/

#ifndef __GEL_TERRAIN_H
#define __GEL_TERRAIN_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/macros.h>
#include <core/math.h>

#include <gfx/nxflags.h>

namespace Script
{
	class CScriptStructure;
	class CScript;
	class CStruct;
}

namespace Env
{

// these sound trigger values are also used to
// communicate with the sounds.cpp module and scripted
// soundfx for jumping/landing...
enum ETerrainActionType
{
	vTABLE_WHEELROLL = 0,
	vTABLE_GRIND, // on a rail...
	vTABLE_JUMP,
	vTABLE_LAND,
	vTABLE_BONK,
	vTABLE_GRINDJUMP,
	vTABLE_GRINDLAND,
	vTABLE_SLIDE, // on a rail
	vTABLE_SLIDEJUMP,
	vTABLE_SLIDELAND,
	vTABLE_CESS,
	vNUM_ACTION_TYPES,
};
	
struct STerrainSoundInfo
{
	const char *mp_soundName;
	uint32		m_soundChecksum;		// Checksum of sound filename
	float		m_maxPitch;
	float		m_minPitch;
	float		m_maxVol;
	float		m_minVol;
	float		m_loadPitch;			// Values to use when the sound is loaded
	float		m_loadVol;
};

struct STerrainInfo
{
	ETerrainType		m_type;
	bool				m_loaded;		// tells if being used by current level
	STerrainSoundInfo	m_sounds[vNUM_ACTION_TYPES];

	float				m_friction;
	// etc, etc, etc
};

/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/

class  CTerrainManager
{
	
public:
	static void					sReset();

	static bool					sGetTerrainSoundInfo( STerrainSoundInfo *p_info, ETerrainType terrain,
													  ETerrainActionType action, float soundChoice = 0.0f );

	static bool					sLoadTerrainSounds( ETerrainType terrain );
	static float				sGetVolPercent( STerrainSoundInfo *pInfo, float volPercent, bool clipToMaxVol = false );
	static float				sGetPitchPercent( STerrainSoundInfo *pInfo, float pitchPercent, bool clipToMaxVol = false );

	static void 				sPlaySound( ETerrainActionType action, ETerrainType terrain, const Mth::Vector &pos,
											float volPercent, float pitchPercent, float soundChoice, bool propogate = true );

	static ETerrainType			sGetTerrainFromChecksum(uint32 checksum);
	static ETerrainActionType	sGetActionFromChecksum(uint32 checksum);


private:

	static uint32				s_get_action_checksum(ETerrainActionType action);

	static bool					s_get_terrain_actions_struct(ETerrainType terrain, Script::CStruct *p_actions);
	static Script::CStruct *	s_get_action_sound_struct(Script::CStruct *p_actions, ETerrainActionType action, float soundChoice);
	static bool					s_get_sound_info(Script::CStruct *p_sound, STerrainSoundInfo *p_info);
	static Script::CStruct *	s_choose_sound_action(Script::CStruct *p_sound_action_struct);
	
	static void					s_load_action_sound( Script::CStruct* p_sound );
	static void					s_load_action_sound_struct( Script::CStruct* p_actions, ETerrainActionType action );
	
	
	//STerrainInfo 				sDefaultTerrainArray[ vNUM_TERRAIN_TYPES ];	// This one is inited at the beginning and never changes
	//STerrainInfo 				sCurrentTerrainArray[ vNUM_TERRAIN_TYPES ];	// This one is derived from the default and the level array

#ifdef __NOPT_ASSERT__
								// We check this array to make sure non-loaded terrains
	static void					s_check_terrain_loaded(ETerrainType terrain);

	static bool					s_terrain_loaded[ vNUM_TERRAIN_TYPES ];
#endif __NOPT_ASSERT__
};

}  // namespace Env

#endif
