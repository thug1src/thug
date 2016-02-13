///////////////////////////////////////////////////////////////////////////////////
// NxTexMan.H - Neversoft Engine, Rendering portion, Platform independent interface

#ifndef	__GFX_NX_TEX_MAN_H__
#define	__GFX_NX_TEX_MAN_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/hashtable.h>

#include <gfx/nxtexture.h>


namespace Script
{
	class CScriptStructure;
	class CScript;
}

namespace Nx
{

///////////////////////////////////////////////////////////////////////////////////
// Nx::CTexDictManager
class	CTexDictManager
{
public:

	// Create and destroy CTexDicts
	static CTexDict *			sCreateTextureDictionary(const char *p_tex_dict_name);	// Creates an empty dictionary
	static CTexDict *			sLoadTextureDictionary(uint32 checksum, uint32* pData, int dataSize, bool isLevelData, uint32 texDictOffset=0, bool isSkin=0, bool forceTexDictLookup=false );    // Loads dictionary from file
	static CTexDict *			sLoadTextureDictionary(const char *p_tex_dict_name, bool isLevelData, uint32 texDictOffset=0, bool isSkin=0, bool forceTexDictLookup=false );    // Loads dictionary from file
	static bool					sUnloadTextureDictionary(CTexDict *p_tex_dict);

	static CTexDict *  			sGetTextureDictionary(uint32 checksum);		// Will not affect ref_count

	// temporary sprite texture dictionary
	static CTexDict *			sp_sprite_tex_dict;
	static CTexDict *			sp_particle_tex_dict;

private:	
	static Lst::HashTable<CTexDict>	s_tex_dict_lookup;

	// Platform-specific calls
	// The following two will only be called if a physical load or unload is done
	static CTexDict *			s_plat_load_texture_dictionary(const char *p_tex_dict_name, bool is_level_data, uint32 texDictOffset, bool isSkin, bool forceTexDictLookup = false );
	static CTexDict *			s_plat_load_texture_dictionary(uint32 checksum, uint32* pData, int dataSize, bool is_level_data, uint32 texDictOffset, bool isSkin, bool forceTexDictLookup = false );
	static CTexDict *			s_plat_create_texture_dictionary(uint32 checksum);
	static bool					s_plat_unload_texture_dictionary(CTexDict *p_tex_dict);
};

bool ScriptLoadTexture(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptUnloadTexture(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptAddTextureToVram(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptRemoveTextureFromVram(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptLoadFaceTextureFromProfile(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptGenerate32BitImage(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptOffsetTexture(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptAdjustTextureRegion(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptPullTextureToEdge(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptPushTextureToPoint(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptAdjustTextureBrightness(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptAdjustTextureHSV(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptCopyTexture(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptCombineTextures(Script::CScriptStructure *pParams, Script::CScript *pScript);

bool ScriptLoadParticleTexture(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptUnloadParticleTexture(Script::CScriptStructure *pParams, Script::CScript *pScript);

bool ScriptLoadSFPTexture(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptUnloadSFPTexture(Script::CScriptStructure *pParams, Script::CScript *pScript);

bool ScriptDumpTextures(Script::CScriptStructure *pParams, Script::CScript *pScript);


void 	FlushParticleTextures(bool all);


}


#endif

