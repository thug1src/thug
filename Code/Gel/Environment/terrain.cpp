/*
	This is just a database for arrays of skater sounds on different terrains.
	
	For each type of sound in sounds.h (like grind, land, slide, wheel roll, etc...)
	there is an array of sounds to play, one for each possible terrain type.
	
	A terrain type of zero indicates that the sound is the default (for surfaces not
	defined in the list).
*/


#include <string.h>
#include <core/defines.h>

#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/utils.h>
#include <gel/net/server/netserv.h>
#include <gel/net/client/netclnt.h>
#include <gel/environment/terrain.h>
#include <gel/soundfx/soundfx.h>

#include <sk/gamenet/gamenet.h>
#include <sys/config/config.h>
#include <sys/replay/replay.h>



namespace Env
{

#ifdef __NOPT_ASSERT__
bool					CTerrainManager::s_terrain_loaded[ vNUM_TERRAIN_TYPES ];
#endif __NOPT_ASSERT__

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

ETerrainType			CTerrainManager::sGetTerrainFromChecksum(uint32 checksum)
{
	switch (checksum)
	{
	case 0x64967688: // TERRAIN_DEFAULT
		return vTERRAIN_DEFAULT;

	case 0x9b37fa90: // TERRAIN_CONCSMOOTH
		return vTERRAIN_CONCSMOOTH;   

	case 0x9383172: // TERRAIN_CONCROUGH
		return vTERRAIN_CONCROUGH;

	case 0xa9ecf4e9: // TERRAIN_METALSMOOTH
		return vTERRAIN_METALSMOOTH;

	case 0xa5aab5e: // TERRAIN_METALROUGH
		return vTERRAIN_METALROUGH;   

	case 0x67af2cb5: // TERRAIN_METALCORRUGATED
		return vTERRAIN_METALCORRUGATED;  

	case 0x1134b31a: // TERRAIN_METALGRATING
		return vTERRAIN_METALGRATING;

	case 0xd814ef41: // TERRAIN_METALTIN
		return vTERRAIN_METALTIN; 

	case 0x39075ea5: // TERRAIN_WOOD
		return vTERRAIN_WOOD;   

	case 0x3afca53f: // TERRAIN_WOODMASONITE
		return vTERRAIN_WOODMASONITE;

	case 0x6651a5d0: // TERRAIN_WOODPLYWOOD
		return vTERRAIN_WOODPLYWOOD;

	case 0xd11a620d: // TERRAIN_WOODFLIMSY
		return vTERRAIN_WOODFLIMSY;   

	case 0xd3e04021: // TERRAIN_WOODSHINGLE
		return vTERRAIN_WOODSHINGLE;

	case 0x866b2b83: // TERRAIN_WOODPIER
		return vTERRAIN_WOODPIER; 

	case 0x3d5a952c: // TERRAIN_BRICK
		return vTERRAIN_BRICK;

	case 0x7315eeac: // TERRAIN_TILE
		return vTERRAIN_TILE;   

	case 0x6aa69ead: // TERRAIN_ASPHALT
		return vTERRAIN_ASPHALT;  

	case 0x32d3fc0a: // TERRAIN_ROCK
		return vTERRAIN_ROCK;   

	case 0x67f6ee4c: // TERRAIN_GRAVEL
		return vTERRAIN_GRAVEL;   

	case 0x103bb3b8: // TERRAIN_SIDEWALK
		return vTERRAIN_SIDEWALK; 

	case 0xa207c1e3: // TERRAIN_GRASS
		return vTERRAIN_GRASS;   

	case 0x802c8f14: // TERRAIN_GRASSDRIED
		return vTERRAIN_GRASSDRIED;   

	case 0x9dfda61e: // TERRAIN_DIRT
		return vTERRAIN_DIRT;   

	case 0xd748f9b7: // TERRAIN_DIRTPACKED
		return vTERRAIN_DIRTPACKED;   

	case 0xf1394aca: // TERRAIN_WATER
		return vTERRAIN_WATER;   

	case 0x5e354fef: // TERRAIN_ICE
		return vTERRAIN_ICE;   

	case 0x3319e21b: // TERRAIN_SNOW
		return vTERRAIN_SNOW;   

	case 0xa5e0d5b9: // TERRAIN_SAND
		return vTERRAIN_SAND;   

	case 0x6362cc4f: // TERRAIN_PLEXIGLASS
		return vTERRAIN_PLEXIGLASS;   

	case 0x66987ee0: // TERRAIN_FIBERGLASS
		return vTERRAIN_FIBERGLASS;   

	case 0x8e6a5d9d: // TERRAIN_CARPET
		return vTERRAIN_CARPET;   

	case 0x2ee8ef42: // TERRAIN_CONVEYOR
		return vTERRAIN_CONVEYOR; 

	case 0xbe7c0e6e: // TERRAIN_CHAINLINK
		return vTERRAIN_CHAINLINK;

	case 0x14259681: // TERRAIN_METALFUTURE
		return vTERRAIN_METALFUTURE;

	case 0xbb1ac0d4: // TERRAIN_GENERIC1
		return vTERRAIN_GENERIC1; 

	case 0x2213916e: // TERRAIN_GENERIC2
		return vTERRAIN_GENERIC2;

	case 0x83b85f36: // TERRAIN_WHEELS
		return vTERRAIN_WHEELS;   

	case 0x22dd3d51: // TERRAIN_WETCONC
		return vTERRAIN_WETCONC;

	case 0xfa298f50: // TERRAIN_METALFENCE
		return vTERRAIN_METALFENCE;

	case 0x69803ba4: // TERRAIN_GRINDTRAIN
		return vTERRAIN_GRINDTRAIN;

	case 0xed035a39: // TERRAIN_GRINDROPE
		return vTERRAIN_GRINDROPE;

	case 0xec66b43b: // TERRAIN_GRINDWIRE
		return vTERRAIN_GRINDWIRE;

	case CRCC(0x3884f029,"TERRAIN_GRINDCONC"):
		return vTERRAIN_GRINDCONC;

	case CRCC(0x92a2112a,"TERRAIN_GRINDROUNDMETALPOLE"):
		return vTERRAIN_GRINDROUNDMETALPOLE;

	case CRCC(0xd7e88122,"TERRAIN_GRINDCHAINLINK"):
		return vTERRAIN_GRINDCHAINLINK;

	case CRCC(0xf5842bce,"TERRAIN_GRINDMETAL"):
		return vTERRAIN_GRINDMETAL;

	case CRCC(0xffc0b5e,"TERRAIN_GRINDWOODRAILING"):
		return vTERRAIN_GRINDWOODRAILING;

	case CRCC(0x8aadc3d0,"TERRAIN_GRINDWOODLOG"):
		return vTERRAIN_GRINDWOODLOG;

	case CRCC(0x60809403,"TERRAIN_GRINDWOOD"):
		return vTERRAIN_GRINDWOOD;

	case CRCC(0xf862cfe0,"TERRAIN_GRINDPLASTIC"):
		return vTERRAIN_GRINDPLASTIC;

	case CRCC(0x2bb33575,"TERRAIN_GRINDELECTRICWIRE"):
		return vTERRAIN_GRINDELECTRICWIRE;

	case CRCC(0x110f1bd3,"TERRAIN_GRINDCABLE"):
		return vTERRAIN_GRINDCABLE;

	case CRCC(0x84e4c7cd,"TERRAIN_GRINDCHAIN"):
		return vTERRAIN_GRINDCHAIN;

	case CRCC(0x3f1c35c5,"TERRAIN_GRINDPLASTICBARRIER"):
		return vTERRAIN_GRINDPLASTICBARRIER;

	case CRCC(0x6c4a1c6a,"TERRAIN_GRINDNEONLIGHT"):
		return vTERRAIN_GRINDNEONLIGHT;

	case CRCC(0xcaf52d56,"TERRAIN_GRINDGLASSMONSTER"):
		return vTERRAIN_GRINDGLASSMONSTER;

	case CRCC(0x8da017a8,"TERRAIN_GRINDBANYONTREE"):
		return vTERRAIN_GRINDBANYONTREE;

	case CRCC(0x7825b83c,"TERRAIN_GRINDBRASSRAIL"):
		return vTERRAIN_GRINDBRASSRAIL;

	case CRCC(0x5ce9c824,"TERRAIN_GRINDCATWALK"):
		return vTERRAIN_GRINDCATWALK;

	case CRCC(0x66ab4005,"TERRAIN_GRINDTANKTURRET"):
		return vTERRAIN_GRINDTANKTURRET;

	case 0x24dca61: // TERRAIN_MEATALSMOOTH		// old typo that still exists
		Dbg_Message("Metal is not spelled 'Meatal'");
	default:
		Dbg_MsgAssert(0, ("Unknown terrain checksum %x\n", checksum));
		return vTERRAIN_DEFAULT;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

ETerrainActionType		CTerrainManager::sGetActionFromChecksum(uint32 checksum)
{
	switch (checksum)
	{
	case 0xcaebd8e: // TA_ROLL
		return vTABLE_WHEELROLL;

	case 0x7e5d9202: // TA_GRIND
		return vTABLE_GRIND;

	case 0xc0669dd5: // TA_OLLIE
		return vTABLE_JUMP;

	case 0x8a1b5a98: // TA_LAND
		return vTABLE_LAND;

	case 0xf0e51d30: // TA_BONK
		return vTABLE_BONK;

	case 0x6ac1023d: // TA_GRINDJUMP
		return vTABLE_GRINDJUMP;

	case 0x6572d1f3: // TA_GRINDLAND
		return vTABLE_GRINDLAND;

	case 0xd6135bf0: // TA_SLIDE
		return vTABLE_SLIDE;

	case 0x6e27388f: // TA_SLIDEJUMP
		return vTABLE_SLIDEJUMP;

	case 0x6194eb41: // TA_SLIDELAND
		return vTABLE_SLIDELAND;

	case 0x20744cde: // TA_REVERT
		return vTABLE_CESS;

	default:
		Dbg_MsgAssert(0, ("Unknown terrain action checksum %x\n", checksum));
		return vTABLE_WHEELROLL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32					CTerrainManager::s_get_action_checksum(ETerrainActionType action)
{
	switch (action)
	{
	case vTABLE_WHEELROLL:
		return 0xcaebd8e; // TA_ROLL

	case vTABLE_GRIND:
		return 0x7e5d9202; // TA_GRIND

	case vTABLE_JUMP:
		return 0xc0669dd5; // TA_OLLIE

	case vTABLE_LAND:
		return 0x8a1b5a98; // TA_LAND

	case vTABLE_BONK:
		return 0xf0e51d30; // TA_BONK

	case vTABLE_GRINDJUMP:
		return 0x6ac1023d; // TA_GRINDJUMP

	case vTABLE_GRINDLAND:
		return 0x6572d1f3; // TA_GRINDLAND

	case vTABLE_SLIDE:
		return 0xd6135bf0; // TA_SLIDE

	case vTABLE_SLIDEJUMP:
		return 0x6e27388f; // TA_SLIDEJUMP

	case vTABLE_SLIDELAND:
		return 0x6194eb41; // TA_SLIDELAND

	case vTABLE_CESS:
		return 0x20744cde; // TA_REVERT

	default:
		Dbg_MsgAssert(0, ("Unknown terrain action %d\n", action));
		return 0xcaebd8e; // TA_ROLL
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool					CTerrainManager::s_get_terrain_actions_struct(ETerrainType terrain, Script::CStruct *p_actions)
{
	Dbg_MsgAssert(p_actions, ("s_get_terrain_actions_struct given NULL CStruct"));

	Script::CStruct *p_default_actions = NULL;
	Script::CStruct *p_terrain_actions = NULL;
	Script::CStruct *p_level_default_actions = NULL;
	Script::CStruct *p_level_terrain_actions = NULL;

	uint32 terrain_checksum;
	ETerrainType terrain_type;

	// Search Default Terrain array first
	Script::CArray *pDefaultTerrainArray = Script::GetArray(CRCD(0x1c2c4218,"standard_terrain"));
	Dbg_MsgAssert(pDefaultTerrainArray, ("standard_terrain not found"));

	for (uint i = 0; (!p_default_actions || !p_terrain_actions) && i < pDefaultTerrainArray->GetSize(); i++)
	{
		// Get next terrain struct
		Script::CStruct *pTerrainStruct = pDefaultTerrainArray->GetStructure(i);
		Dbg_MsgAssert(pTerrainStruct, ("terrain array not made of structures"));

		pTerrainStruct->GetChecksum(CRCD(0x3789ac4e,"Terrain"), &terrain_checksum, Script::ASSERT);
		terrain_type = sGetTerrainFromChecksum(terrain_checksum);

		if (terrain_type == vTERRAIN_DEFAULT)
		{
			pTerrainStruct->GetStructure(CRCD(0x5c969c50,"SoundActions"), &p_default_actions);
			//Dbg_Message("Found default terrain:");
			//Script::PrintContents(p_default_actions);
		}
		else if (terrain_type == terrain)
		{
			pTerrainStruct->GetStructure(CRCD(0x5c969c50,"SoundActions"), &p_terrain_actions);
			//Dbg_Message("Found terrain:");
			//Script::PrintContents(p_terrain_actions);
		}

		if (p_default_actions && p_terrain_actions)
		{
			break;
		}
	}

	// Search Level Terrain array
	Script::CArray *pLevelTerrainArray = Script::GetArray(CRCD(0x96218951,"level_terrain"));
	if (pLevelTerrainArray)
	{
		for (uint i = 0; i < pLevelTerrainArray->GetSize(); i++)
		{
			// Get next terrain struct
			Script::CStruct *pTerrainStruct = pLevelTerrainArray->GetStructure(i);
			Dbg_MsgAssert(pTerrainStruct, ("terrain array not made of structures"));

			pTerrainStruct->GetChecksum(CRCD(0x3789ac4e,"Terrain"), &terrain_checksum, Script::ASSERT);
			terrain_type = sGetTerrainFromChecksum(terrain_checksum);

			if (terrain_type == vTERRAIN_DEFAULT)
			{
				pTerrainStruct->GetStructure(CRCD(0x5c969c50,"SoundActions"), &p_level_default_actions);
			} else if (terrain_type == terrain)
			{
				pTerrainStruct->GetStructure(CRCD(0x5c969c50,"SoundActions"), &p_level_terrain_actions);
			}

			if (p_level_default_actions && p_level_terrain_actions)
			{
				break;
			}
		}
	}

	// Combine them together
	*p_actions = *p_default_actions;

	if (p_level_default_actions)
	{
		*p_actions += *p_level_default_actions;
	}

	if (p_terrain_actions)
	{
		*p_actions += *p_terrain_actions;
	}

	if (p_level_terrain_actions)
	{
		*p_actions += *p_level_terrain_actions;
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct *		CTerrainManager::s_choose_sound_action(Script::CStruct *p_sound_action_struct)
{
	Script::CStruct* p_sound;
	
	// soundAction is a single stucture, so just use it
	if (p_sound_action_struct->GetStructure(CRCD(0x25f654c8, "soundAction"), &p_sound))
	{
		Dbg_MsgAssert(!p_sound->ContainsComponentNamed(CRCD(0xdbc4c4db, "chance")),
			("A sound may only include a chance parameter when it is an element of a random-choice list of sounds"));
		return p_sound;
	}
	
	// soundAction is an array of structures, so choose one randomly based on their given chances
	Script::CArray* p_random_indexed_sound_array;
	p_sound_action_struct->GetArray(CRCD(0x25f654c8, "soundAction"), &p_random_indexed_sound_array, Script::ASSERT);
	
	int total_chance = 0;
	for (int sound_idx = p_random_indexed_sound_array->GetSize(); sound_idx--; )
	{
		p_sound = p_random_indexed_sound_array->GetStructure(sound_idx);
		
		int chance;
		p_sound->GetInteger(CRCD(0xdbc4c4db, "chance"), &chance, Script::ASSERT);
		
		total_chance += chance;
	}
	
	int choice = Mth::Rnd(total_chance);
	
	for (int sound_idx = p_random_indexed_sound_array->GetSize(); sound_idx--; )
	{
		p_sound = p_random_indexed_sound_array->GetStructure(sound_idx);
		
		int chance;
		p_sound->GetInteger(CRCD(0xdbc4c4db, "chance"), &chance, Script::ASSERT);
		
		choice -= chance;
		if (choice < 0)
		{
			return p_sound;
		}
	}
	
	Dbg_Assert(false);
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Script::CStruct *		CTerrainManager::s_get_action_sound_struct(Script::CStruct *p_actions, ETerrainActionType action, float soundChoice)
{
	Dbg_MsgAssert(p_actions, ("s_get_action_sound_struct given NULL action CStruct"));

	Script::CStruct *p_sound;
	
	uint32 checksum = s_get_action_checksum(action);

	// sound is a single sound structure, so just use it
	if (p_actions->GetStructure(checksum, &p_sound))
	{
		return p_sound;
	}
	
	// sound is an array of structures, one of which must be choosen depending upon speed
	Script::CArray* p_float_indexed_sound_array;
	p_actions->GetArray(checksum, &p_float_indexed_sound_array, Script::ASSERT);
	
	// check to insure that the final structure in the array has no useUpTo parameter and thus operates as a default choice
	Dbg_MsgAssert(!p_float_indexed_sound_array->GetStructure(p_float_indexed_sound_array->GetSize() - 1)->ContainsComponentNamed(CRCD(0x9ee9a09a, "useUpTo")),
		("Final sound in a speed-indexed sound list must not include useUpTo"));
	
	for (uint32 sound_action_idx = 0; sound_action_idx < p_float_indexed_sound_array->GetSize(); sound_action_idx++)
	{
		// each structure contains a useUpTo parameter which is checked against speed in turn; if found to be larger
		// than speed, the sound is used; a structure lacking useUpTo is always used
		Script::CStruct* p_sound_action_struct = p_float_indexed_sound_array->GetStructure(sound_action_idx);
		
		float up_to_speed;
		if (!p_sound_action_struct->GetFloat(CRCD(0x9ee9a09a, "useUpTo"), &up_to_speed))
		{
			Dbg_MsgAssert(sound_action_idx == p_float_indexed_sound_array->GetSize() - 1,
				("Only the final sound in a float-indexed sound list may fail to include useUpTo"));
			return s_choose_sound_action(p_sound_action_struct);
		}
		
		if (soundChoice <= up_to_speed)
		{
			return s_choose_sound_action(p_sound_action_struct);
		}
	}
	
	Dbg_Assert(false);
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool					CTerrainManager::s_get_sound_info(Script::CStruct *p_sound, STerrainSoundInfo *p_info)
{
	Dbg_MsgAssert(p_info, ("s_get_sound_info given NULL STerrainSoundInfo"));

	// Set defaults
	p_info->mp_soundName = NULL;
	p_info->m_soundChecksum = 0;
	p_info->m_maxPitch = 100.0f;
	p_info->m_minPitch = 100.0f;
	p_info->m_maxVol = 100.0f;
	p_info->m_minVol = 0.0f;
	p_info->m_loadPitch = 100.0f;
	p_info->m_loadVol = 100.0f;

	p_sound->GetString( CRCD(0x7713c7b,"sound"), &p_info->mp_soundName );
	if ( !p_info->mp_soundName )
	{
		Dbg_MsgAssert(0, ( "Need the name of a sound in s_get_sound_info" ));
		return false;
	}

	// Calculate checksum
	const char	*pNameMinusPath	= p_info->mp_soundName;
	int			stringLength	= strlen( p_info->mp_soundName );
	for( int i = 0; i < stringLength; i++ )
	{
		if(( p_info->mp_soundName[i] == '\\' ) || ( p_info->mp_soundName[i] == '/' ))
			pNameMinusPath = &p_info->mp_soundName[i + 1];
	}
	
	p_info->m_soundChecksum = Script::GenerateCRC( pNameMinusPath );

	p_sound->GetFloat( CRCD(0xfa3e14c5,"maxPitch"), &p_info->m_maxPitch );
	p_sound->GetFloat( CRCD(0x1c5ebb24,"minPitch"), &p_info->m_minPitch );
	
	p_sound->GetFloat( CRCD(0x693daaf,"maxVol"), &p_info->m_maxVol );
	p_sound->GetFloat( CRCD(0x4391992d,"minVol"), &p_info->m_minVol );

	p_sound->GetFloat( CRCD(0xf573682f,"loadPitch"), &p_info->m_loadPitch );
	p_sound->GetFloat( CRCD(0x312ccbcf,"loadVol"), &p_info->m_loadVol );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void					CTerrainManager::s_load_action_sound( Script::CStruct* p_sound )
{
	Dbg_MsgAssert(p_sound, ("s_load_action_sound given NULL sound CStruct"));
	
	STerrainSoundInfo soundInfo;
	#ifdef	__NOPT_ASSERT__
	bool result = 
	#endif
	s_get_sound_info(p_sound, &soundInfo);
	Dbg_Assert(result);

	Sfx::CSfxManager::Instance()->LoadSound( soundInfo.mp_soundName, 0, 0.0f, soundInfo.m_loadPitch, soundInfo.m_loadVol );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void					CTerrainManager::s_load_action_sound_struct( Script::CStruct* p_actions, ETerrainActionType action )
{
	Dbg_MsgAssert(p_actions, ("s_load_action_sound_struct given NULL action CStruct"));

	Script::CStruct *p_sound;

	uint32 checksum = s_get_action_checksum(action);

	// sound is a single sound structure, so just load it
	if (p_actions->GetStructure(checksum, &p_sound))
	{
		s_load_action_sound(p_sound);
		return;
	}
	
	// sound is an array of sound structures, all of which must be loaded
	Script::CArray* p_float_indexed_sound_array;
	p_actions->GetArray(checksum, &p_float_indexed_sound_array, Script::ASSERT);
	
	for (uint32 i = 0; i < p_float_indexed_sound_array->GetSize(); i++)
	{
		Script::CStruct* p_sound_action_struct = p_float_indexed_sound_array->GetStructure(i);
		
		// this entry is a single sound structure, so just load it
		if (p_sound_action_struct->GetStructure(CRCD(0x25f654c8, "soundAction"), &p_sound))
		{
			s_load_action_sound(p_sound);
			continue;
		}
		
		// this entry is an array of sounds, all of which must be loaded
		Script::CArray* p_random_indexed_sound_array;
		p_sound_action_struct->GetArray(CRCD(0x25f654c8, "soundAction"), &p_random_indexed_sound_array, Script::ASSERT);
		
		for (uint32 j = 0; j < p_random_indexed_sound_array->GetSize(); j++)
		{
			p_sound = p_random_indexed_sound_array->GetStructure(j);
			s_load_action_sound(p_sound);
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#ifdef __NOPT_ASSERT__
void					CTerrainManager::s_check_terrain_loaded(ETerrainType terrain)
{
	// Garrett: Comment the assert back in when the new plugin is released (on or after 2/20/03)
	if (!s_terrain_loaded[terrain])
	{
//		Dbg_Message("****** ERROR: Trying to use terrain %d, which isn't loaded ******", terrain);
	}
//	Dbg_MsgAssert(s_terrain_loaded[terrain], ("****** ERROR: Trying to use terrain %d, which isn't loaded ******", terrain));
}
#endif __NOPT_ASSERT__

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void					CTerrainManager::sReset()
{
#ifdef __NOPT_ASSERT__
	// Clear terrain loaded array
	for (int i = 0; i < vNUM_TERRAIN_TYPES; i++)
	{
		s_terrain_loaded[i] = false;
	}
#endif __NOPT_ASSERT__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool					CTerrainManager::sGetTerrainSoundInfo( STerrainSoundInfo *p_info, ETerrainType terrain,
															   ETerrainActionType action, float soundChoice )
{
	bool result = false;

	if (Sfx::NoSoundPlease()) return false;

	Dbg_MsgAssert((terrain >= 0) && (terrain < vNUM_TERRAIN_TYPES), ("Terrain type out of range"));

#ifdef __NOPT_ASSERT__
	// Check that terrain is loaded
	s_check_terrain_loaded(terrain);
#endif __NOPT_ASSERT__

	Dbg_MsgAssert(p_info, ("sGetTerrainSoundInfo given NULL STerrainSoundInfo"));

	// Allocate new script structure for combining data
	Script::CStruct *p_actions = new Script::CStruct;
	Script::CStruct *p_sound;

	if (s_get_terrain_actions_struct(terrain, p_actions))
	{
		p_sound = s_get_action_sound_struct(p_actions, action, soundChoice);
		if (p_sound)
		{
			//Dbg_Message("Extracted sound:");
			//Script::PrintContents(p_sound);

			result = s_get_sound_info(p_sound, p_info);
		}
	}

	// Done with script data
	delete p_actions;

	return result;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool					CTerrainManager::sLoadTerrainSounds( ETerrainType terrain )
{
	if (Sfx::NoSoundPlease()) return false;

	Dbg_MsgAssert((terrain >= 0) && (terrain < vNUM_TERRAIN_TYPES), ("Terrain type out of range"));

	Script::CStruct *p_actions = new Script::CStruct;

//	Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
//	Dbg_Assert(sfx_manager);

	if (s_get_terrain_actions_struct(terrain, p_actions))
	{
		// Go through each action
		for (int action_idx = 0; action_idx < vNUM_ACTION_TYPES; action_idx++)
		{
			s_load_action_sound_struct(p_actions, (ETerrainActionType) action_idx);
		}
	}

	// Done with script data
	delete p_actions;

#ifdef __NOPT_ASSERT__
	// Mark terrain loaded
	s_terrain_loaded[terrain] = true;
#endif __NOPT_ASSERT__

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void					CTerrainManager::sPlaySound( ETerrainActionType action, ETerrainType terrain, const Mth::Vector &pos,
													 float volPercent, float pitchPercent, float soundChoice, bool propogate )
{
	if (Sfx::NoSoundPlease()) return;
	
	Dbg_MsgAssert((terrain >= 0) && (terrain < vNUM_TERRAIN_TYPES), ("Terrain type out of range"));

#ifdef __NOPT_ASSERT__
	// Check that terrain is loaded
	s_check_terrain_loaded(terrain);
#endif __NOPT_ASSERT__

	// need to write pitch as well
	// Replay::WriteSkaterSoundEffect(action,terrain,pos,volPercent);
	
	Sfx::CSfxManager * sfx_manager = Sfx::CSfxManager::Instance();
	GameNet::Manager * gamenet_manager = GameNet::Manager::Instance();
    
	STerrainSoundInfo soundInfo;
	bool found = sGetTerrainSoundInfo( &soundInfo, terrain, action, soundChoice );

	if ( !found )
	{
		// no sounds are supposed to be played for this surface on this transition:
		return;
	}
	
	if( propogate )
	{
		Net::Client* client;
		GameNet::MsgPlaySound msg;
		Net::MsgDesc msg_desc;

		client = gamenet_manager->GetClient( 0 );
		Dbg_Assert( client );

		msg.m_WhichArray = (char) action;
		msg.m_SurfaceFlag = (char) terrain;
		msg.m_Pos[0] = (short) pos[X];
		msg.m_Pos[1] = (short) pos[Y];
		msg.m_Pos[2] = (short) pos[Z];
		msg.m_VolPercent = (char) volPercent;
		msg.m_PitchPercent = (char) pitchPercent;
		msg.m_SoundChoice = (char) soundChoice;

		msg_desc.m_Data = &msg;
		msg_desc.m_Length = sizeof( GameNet::MsgPlaySound );
		msg_desc.m_Id = GameNet::MSG_ID_PLAY_SOUND;
		client->EnqueueMessageToServer( &msg_desc );
	}

	Sfx::sVolume vol;
	sfx_manager->SetVolumeFromPos( &vol, pos, sfx_manager->GetDropoffDist( soundInfo.m_soundChecksum ));

	// Adjust volume according to speed.
	volPercent = sGetVolPercent( &soundInfo, volPercent );
	vol.PercentageAdjustment( volPercent );
	
	if (vol.IsSilent()) return;
	
	pitchPercent = sGetPitchPercent( &soundInfo, pitchPercent );
	
	sfx_manager->PlaySound( soundInfo.m_soundChecksum, &vol, pitchPercent, 0, NULL, soundInfo.mp_soundName );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// set the volume according to the range specified by the designers...
float					CTerrainManager::sGetVolPercent( STerrainSoundInfo *pInfo, float volPercent, bool clipToMaxVol )
{
	
	Dbg_MsgAssert(pInfo, ("pInfo set to NULL."));

	if ( !( ( pInfo->m_minVol == 0.0f ) && ( pInfo->m_maxVol == 100.0f ) ) )
	{
		volPercent = ( pInfo->m_minVol + PERCENT( ( pInfo->m_maxVol - pInfo->m_minVol ), volPercent ) );
	}
	
	if ( clipToMaxVol )
	{
		if ( volPercent > pInfo->m_maxVol )
			volPercent = pInfo->m_maxVol;
	}
	return ( volPercent );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float					CTerrainManager::sGetPitchPercent( STerrainSoundInfo *pInfo, float pitchPercent, bool clipToMaxPitch )
{
	
	Dbg_MsgAssert(pInfo, ("pInfo set to NULL."));

	if ( !( ( pInfo->m_minPitch == 0.0f ) && ( pInfo->m_maxPitch == 100.0f ) ) )
	{
		pitchPercent = ( pInfo->m_minPitch + PERCENT( ( pInfo->m_maxPitch - pInfo->m_minPitch ), pitchPercent ) );
	}
	
	if ( clipToMaxPitch )
	{
		if ( pitchPercent > pInfo->m_maxPitch )
			pitchPercent = pInfo->m_maxPitch;
	}
	return ( pitchPercent );
}

#if 0
// Sound FX for the level...
/*	The surface flag indicates which surface the skater is currently on (grass, cement, wood, metal)
	whichSound is the checksum from the name of the looping sound (should be loaded using LoadSound
		in the script file for each level)
	whichArray indicates whether this sound belongs in the list of wheels rolling sounds, or
		grinding sounds, etc...
*/
void CSk3SfxManager::SetSkaterSoundInfo( int surfaceFlag, uint32 whichSound, int whichArray,
	float maxPitch, float minPitch, float maxVol, float minVol )
{
	
	if (Sfx::NoSoundPlease()) return;
	
	// must initialize PInfo!
	int i;
	Sfx::CSfxManager * sfx_manager = Sfx::CSfxManager::Instance();

	if ( NULL == sfx_manager->GetWaveTableIndex( whichSound ) )
	{
		Dbg_MsgAssert( 0,( "Terrain sound not loaded! surface %d sound %s checksum %d soundType %d",
			surfaceFlag, Script::FindChecksumName( whichSound ), whichSound, whichArray ));
		return;
	}
	int numEntries = mNumEntries[ whichArray ];
	SkaterSoundInfo	*pArray = mSoundArray[ whichArray ];
	SkaterSoundInfo *pInfo = NULL;
	
	for ( i = 0; i < numEntries; i++ )
	{
		if ( pArray[ i ].surfaceFlag == surfaceFlag )
		{
			Dbg_Message( "Re-defining soundtype %d for surfaceFlag %d", whichArray, surfaceFlag );
			pInfo = &pArray[ i ];
			break;
		}
	}
	if ( !pInfo )
	{
		pInfo = &pArray[ mNumEntries[ whichArray ] ];
		mNumEntries[ whichArray ] += 1;
		Dbg_MsgAssert( mNumEntries[ whichArray ] < vMAX_NUM_ENTRIES,( "Array too small type %d.  Increase MAX_NUM_ENTRIES.", whichArray ));
	}
	
	Dbg_MsgAssert( pInfo,( "Please fire Matt immediately after kicking him in the nuts." ));
	
	// surfaceFlag of zero will be used for the default
	pInfo->surfaceFlag = surfaceFlag;
	// if soundChecksum is zero, no sound will play on this surface.
	pInfo->soundChecksum = whichSound;
	pInfo->maxPitch = maxPitch;
	pInfo->minPitch = minPitch;
	pInfo->maxVol = maxVol;
	pInfo->minVol = minVol;

} // end of SetSkaterSoundInfo( )

#endif

} // namespace Env
