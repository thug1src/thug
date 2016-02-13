///////////////////////////////////////////////////////////////////////////////
// NxTexMan.cpp

// start autoduck documentation
// @DOC nxtexman
// @module nxtexman | None
// @subindex Scripting Database
// @index script | nxtexman

#include <core/defines.h>
#include <core/crc.h>
#include <gel/Scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/checksum.h>

#include "gfx/NxTexMan.h"
#include "gfx/NxSprite.h"
#include "gfx/Nx.h"

#include <sys/config/config.h>

// for downloading faces
#include <sk/modules/skate/skate.h>
#include <sk/objects/skaterprofile.h>
#include <gfx/facetexture.h>
#include <gfx/modelappearance.h>

namespace	Nx
{

/////////////////////////////////////////////////////////////////////////////
// CTexMan

Lst::HashTable<CTexDict>	CTexDictManager::s_tex_dict_lookup(4);

// temporary sprite texture dictionary
CTexDict *					CTexDictManager::sp_sprite_tex_dict = sCreateTextureDictionary("sprite");
CTexDict *					CTexDictManager::sp_particle_tex_dict = sCreateTextureDictionary("particle");

/////////////////////////////////////////////////////////////////////////////
// These functions are the platform independent part of the interface.
					 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexDict *			CTexDictManager::sCreateTextureDictionary(const char *p_tex_dict_name)
{
	CTexDict *p_dict;
	uint32 checksum = Crc::GenerateCRCFromString(p_tex_dict_name);

	p_dict = sGetTextureDictionary(checksum);

	// Assert for now unless we can think of a reason to use with ref counts
	Dbg_MsgAssert((p_dict == NULL), ("Texture dictionary %s already exists", p_tex_dict_name));

	if (p_dict)	
	{
		// If already loaded, then just link up with it
		p_dict->IncRefCount();
	} else {
		p_dict = s_plat_create_texture_dictionary(checksum);
		s_tex_dict_lookup.PutItem(checksum, p_dict);
	}
	return p_dict;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexDict *			CTexDictManager::sLoadTextureDictionary(uint32 fileNameChecksum, uint32* pData, int dataSize, bool isLevelData, uint32 texDictOffset, bool isSkin, bool forceTexDictLookup )
{
	Mem::PushMemProfile((char*)"TEX_data_buffer");

	uint32 checksum = fileNameChecksum + texDictOffset;

	CTexDict *p_dict;
	p_dict = sGetTextureDictionary(checksum);

	if (p_dict)	
	{
		// Mick: temporary assertion to track down annoyingly persisten phantom texture dcitionaries
		// you can probably remove this
		//Dbg_MsgAssert(0,("Not supposed to be multiply using tex dicts yet! (%s/%x)",texture_dict_name,texDictOffset));

		// If already loaded, then just link up with it
		p_dict->IncRefCount();
	}
	else
	{
		p_dict = s_plat_load_texture_dictionary(checksum, pData, dataSize, isLevelData, texDictOffset, isSkin, forceTexDictLookup);
		p_dict->set_checksum(checksum);			// Since it just uses the base name
		s_tex_dict_lookup.PutItem(checksum, p_dict);
	}

	Mem::PopMemProfile(/*(char*)"TEX_data_buffer"*/);

	return p_dict;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexDict *			CTexDictManager::sLoadTextureDictionary(const char *p_tex_dict_name, bool isLevelData, uint32 texDictOffset, bool isSkin, bool forceTexDictLookup )
{
	// append the extension
	char	texture_dict_name[128];
	sprintf(texture_dict_name,"%s.%s",p_tex_dict_name,CEngine::sGetPlatformExtension());

	Mem::PushMemProfile((char*)p_tex_dict_name);
	
	// the texDictOffset prevents clashes when we need multiple
	// separate versions of the same texture dictionary, such as
	// for multiplayer create-a-skater parts (note:  peds should not
	// use this system because they should share actual texture
	// dictionaries)
	uint32 checksum = Crc::GenerateCRCFromString(p_tex_dict_name) + texDictOffset;

	CTexDict *p_dict;
	p_dict = sGetTextureDictionary(checksum);

	if (p_dict)	
	{
		// Mick: temporary assertion to track down annoyingly persisten phantom texture dcitionaries
		// you can probably remove this
		//Dbg_MsgAssert(0,("Not supposed to be multiply using tex dicts yet! (%s/%x)",texture_dict_name,texDictOffset));

		// If already loaded, then just link up with it
		p_dict->IncRefCount();
	}
	else
	{
		p_dict = s_plat_load_texture_dictionary(texture_dict_name, isLevelData, texDictOffset, isSkin, forceTexDictLookup);
		p_dict->set_checksum(checksum);			// Since it just uses the base name
		s_tex_dict_lookup.PutItem(checksum, p_dict);
	}

	Mem::PopMemProfile(/*(char*)p_tex_dict_name*/);

	return p_dict;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CTexDictManager::sUnloadTextureDictionary(CTexDict *p_tex_dict)
{
	if (p_tex_dict->DecRefCount() == 0)
	{
		s_tex_dict_lookup.FlushItem(p_tex_dict->GetChecksum());  // Remove the reference to it before we delete it
		s_plat_unload_texture_dictionary(p_tex_dict);			 // as deleting it can kill the checksum with memory trashing
		return TRUE;
	}

	return FALSE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexDict *		  	CTexDictManager::sGetTextureDictionary(uint32 checksum)
{
	return s_tex_dict_lookup.GetItem(checksum);
}


// @script | LoadTexture | Loads a 2D sprite texture
// @parm string |  | path and name of texture
// @flag no_vram_alloc | won't allocate in vram
bool ScriptLoadTexture(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	const char *p_name;
	if (!pParams->GetText(NONAME, &p_name))
		Dbg_MsgAssert(0, ("no texture specified"));

	bool alloc_vram = true;
	if (pParams->ContainsFlag(CRCD(0x3955ff2e,"no_vram_alloc")))
	{
		alloc_vram = false;
	}
	
	CTexture *p_texture = CTexDictManager::sp_sprite_tex_dict->LoadTexture(p_name, true, alloc_vram);

	return p_texture != NULL;
}



// @script | UnloadTexture | Unloads a 2D sprite texture
// @parm string |  | name of texture
bool ScriptUnloadTexture(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	CTexture *p_texture = NULL;
	const char *p_name;
	uint32 checksum;

	if (pParams->GetText(NONAME, &p_name))
	{
		// get texture based on string
		p_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(p_name);
	}
	else if (pParams->GetChecksum(NONAME, &checksum))
	{							 
		// get texture based on checksum
		p_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(checksum);
	}
	else
	{
		Dbg_MsgAssert(0, ("no texture specified in %s", pScript->GetScriptInfo()));
	}

	if (p_texture)
	{
		CTexDictManager::sp_sprite_tex_dict->UnloadTexture(p_texture);
	}
	else
	{
		if ( !pParams->ContainsFlag( CRCD(0x3d92465e,"dont_assert") ) )
		{
			Dbg_MsgAssert(0, ("Can't find texture %s to unload", p_name));
		}
	}
	
	return true;
}

// @script | AddTextureToVram | Puts 2D sprite texture into VRAM (so it is drawable)
// @parm string |  | name of texture
bool ScriptAddTextureToVram(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
#if 0
	const char *p_name;
	if (!pParams->GetText(NONAME, &p_name))
		Dbg_MsgAssert(0, ("no texture specified"));
	
	CTexture *p_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(p_name);
	if (p_texture)
	{
		p_texture->AddToVram();
	} else {
		Dbg_MsgAssert(0, ("Can't find texture %s to add to vram", p_name));
	}
#endif

	return true;
}

// @script | RemoveTextureFromVram | Removes 2D sprite texture from VRAM
// @parm string |  | name of texture
// @flag no_assert | won't assert on failure
bool ScriptRemoveTextureFromVram(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
#if 0
	const char *p_name;
	if (!pParams->GetText(NONAME, &p_name))
		Dbg_MsgAssert(0, ("no texture specified"));
	
	CTexture *p_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(p_name);
	if (p_texture)
	{
		p_texture->RemoveFromVram();
	} else if (!pParams->ContainsFlag(CRCD(0x512c7426,"no_assert"))) {
		Dbg_MsgAssert(0, ("Can't find texture %s to remove from vram", p_name));
	}
#endif

	return true;
}

// @script | LoadFaceTextureFromProfile | Loads a 2D sprite texture
bool ScriptLoadFaceTextureFromProfile(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
    bool alloc_vram = true;
	uint32 checksum;
    pParams->GetChecksum( "checksum", &checksum, Script::ASSERT );
	
    // Copy the downloaded face for use as a sprite
    Mdl::Skate* skate_mod = Mdl::Skate::Instance();
    Obj::CSkaterProfile* pSkaterProfile = skate_mod->GetCurrentProfile();
    Dbg_Assert( pSkaterProfile );
    Dbg_MsgAssert( !pSkaterProfile->IsPro(), ( "Can only map face onto a custom skater.  UI must make the custom skater active before this point" ) );
    Gfx::CModelAppearance* pAppearance = pSkaterProfile->GetAppearance();
    Dbg_Assert( pAppearance );
    Gfx::CFaceTexture* pFaceTexture = pAppearance->GetFaceTexture();

	Dbg_MsgAssert(pFaceTexture,("NULL pFaceTexture"));
	Dbg_MsgAssert(pFaceTexture->IsValid(),("Invalid pFaceTexture"));

    Nx::CTexture* p_texture = Nx::CTexDictManager::sp_sprite_tex_dict->LoadTextureFromBuffer(pFaceTexture->GetTextureData(), pFaceTexture->GetTextureSize(), checksum, true, alloc_vram);
    Dbg_MsgAssert( p_texture, ( "Appearance has no face texture" ) );


	return p_texture != NULL;
}

// @script | Generate32BitImage | Generates 32bit image data for texture
// @parm name |  | name of texture
// @parmopt int | renderable | 0 | If set to 1, make 32-bit image renderable in a sprite
// @parmopt int | store_original | 0 | If set to 1, keeps original 32-bit image around that can be used with some functions
bool ScriptGenerate32BitImage(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	uint32 checksum;
	if (!pParams->GetChecksum(NONAME, &checksum))
		Dbg_MsgAssert(0, ("no texture specified"));
	
	CTexture *p_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(checksum);
	if (p_texture)
	{
		int value;
		bool renderable = false;
		bool store_original = false;

		if (pParams->GetInteger(CRCD(0xa5d7cfaa,"renderable"), &value) && (value == 1))
		{
			renderable = true;
		}
		if (pParams->GetInteger(CRCD(0xeefaf080,"store_original"), &value) && (value == 1))
		{
			store_original = true;
		}

		p_texture->Generate32BitImage(renderable, store_original);
	}
	else
	{
		Dbg_MsgAssert(0, ("Can't find texture %s to generate 32-bit image", Script::FindChecksumName(checksum)));
	}

	return true;
}

// @script | OffsetTexture | Move texture (cropping the part that moves off and filling in the new parts with
// either the old edge or a supplied fill color)
// @parm name |  | name of texture
// @parm int | x_offset | x offset in pixels
// @parm int | y_offset | y offset in pixels
// @parmopt int | fill_r | 128 | fill color red component
// @parmopt int | fill_g | 128 | fill color green component
// @parmopt int | fill_b | 128 | fill color blue component
// @parmopt int | fill_a | 128 | fill color alpha component
bool ScriptOffsetTexture(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	uint32 checksum;
	if (!pParams->GetChecksum(NONAME, &checksum))
		Dbg_MsgAssert(0, ("no texture specified"));
	
	CTexture *p_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(checksum);
	if (p_texture)
	{
		int x_offset = 0;
		int y_offset = 0;
		bool use_fill_color = false;
		int r = 128, g = 128, b = 128, a = 128;

		pParams->GetInteger(CRCD(0xd83d589e,"x_offset"), &x_offset);
		pParams->GetInteger(CRCD(0x14975800,"y_offset"), &y_offset);

		use_fill_color |= pParams->GetInteger(CRCD(0xb7a78c53,"fill_r"), &r);
		use_fill_color |= pParams->GetInteger(CRCD(0xda7a68b8,"fill_g"), &g);
		use_fill_color |= pParams->GetInteger(CRCD(0xaa109c37,"fill_b"), &b);
		use_fill_color |= pParams->GetInteger(CRCD(0x3319cd8d,"fill_a"), &a);

		Image::RGBA fill_color(r, g, b, a);

		p_texture->Offset(x_offset, y_offset, use_fill_color, fill_color);
	}
	else
	{
		Dbg_MsgAssert(0, ("Can't find texture %s to offset", Script::FindChecksumName(checksum)));
	}

	return true;
}

// @script | AdjustTextureRegion | Stretches and shrinks a texture region from the start point to the end point
// @parm name |  | name of texture
// @parm int | xpos | X position of regiom
// @parm int | ypos | Y position of regiom
// @parm int | width | width of regiom
// @parm int | height | height of regiom
// @parm int | split_axis | axis where the split line crosses (0 = X, 1 = Y)
// @parm int | start_point | split line start point on axis
// @parm int | end_point | split line end point on axis
bool ScriptAdjustTextureRegion(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	uint32 checksum;
	if (!pParams->GetChecksum(NONAME, &checksum))
		Dbg_MsgAssert(0, ("no texture specified"));
	
	CTexture *p_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(checksum);
	if (p_texture)
	{
		int x_pos = 0;
		int y_pos = 0;
		int width = p_texture->GetWidth();
		int height = p_texture->GetHeight();
		int split_axis = X;
		int start_point = 0;
		int end_point = 0;

		pParams->GetInteger(CRCD(0xfa8972e,"xpos"), &x_pos);
		pParams->GetInteger(CRCD(0xb714f04b,"ypos"), &y_pos);
		pParams->GetInteger(CRCD(0x73e5bad0,"width"), &width);
		pParams->GetInteger(CRCD(0xab21af0,"height"), &height);
		pParams->GetInteger(CRCD(0xe679982e,"split_axis"), &split_axis);
		pParams->GetInteger(CRCD(0x3d0f162a,"start_point"), &start_point);
		pParams->GetInteger(CRCD(0x39c54bde,"end_point"), &end_point);

		p_texture->AdjustRegion(x_pos, y_pos, width, height, split_axis, start_point, end_point);
	} else {
		Dbg_MsgAssert(0, ("Can't find texture %s to adjust", Script::FindChecksumName(checksum)));
	}

	return true;
}

// @script | PullTextureToEdge | Pull texture from point along the axis by num_pixels (+ or - determines the direction)
// and crop anything going outside the texture border.
// @parm name |  | name of texture
// @parm int | point | pull point on axis
// @parm int | axis | axis to pull along (0 = X, 1 = Y)
// @parm int | num_pixels | number of pixels (+ or - determines the direction)
bool ScriptPullTextureToEdge(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	uint32 checksum;
	if (!pParams->GetChecksum(NONAME, &checksum))
		Dbg_MsgAssert(0, ("no texture specified"));
	
	CTexture *p_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(checksum);
	if (p_texture)
	{
		int point = 0;
		int axis = X;
		int num_pixels = 0;

		pParams->GetInteger(CRCD(0x485a0cdb,"point"), &point);
		pParams->GetInteger(CRCD(0x7af07905,"axis"), &axis);
		pParams->GetInteger(CRCD(0x88cf948c,"num_pixels"), &num_pixels);

		p_texture->PullToEdge(point, axis, num_pixels);
	}
	else
	{
		Dbg_MsgAssert(0, ("Can't find texture %s to pull", Script::FindChecksumName(checksum)));
	}

	return true;
}
// @script | PushTextureToPoint | Push texture to point along the axis by num_pixels (+ or - determines the direction)
// and either stretch the edge line or fill it in with a fill color.
// @parm name |  | name of texture
// @parm int | point | pull point on axis
// @parm int | axis | axis to pull along (0 = X, 1 = Y)
// @parm int | num_pixels | number of pixels (+ or - determines the direction)
// @parmopt int | fill_r | 128 | fill color red component
// @parmopt int | fill_g | 128 | fill color green component
// @parmopt int | fill_b | 128 | fill color blue component
// @parmopt int | fill_a | 128 | fill color alpha component
bool ScriptPushTextureToPoint(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	uint32 checksum;
	if (!pParams->GetChecksum(NONAME, &checksum))
		Dbg_MsgAssert(0, ("no texture specified"));
	
	CTexture *p_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(checksum);
	if (p_texture)
	{
		int point = 0;
		int axis = X;
		int num_pixels = 0;
		bool use_fill_color = false;
		int r = 128, g = 128, b = 128, a = 128;

		pParams->GetInteger(CRCD(0x485a0cdb,"point"), &point);
		pParams->GetInteger(CRCD(0x7af07905,"axis"), &axis);
		pParams->GetInteger(CRCD(0x88cf948c,"num_pixels"), &num_pixels);

		use_fill_color |= pParams->GetInteger(CRCD(0xb7a78c53,"fill_r"), &r);
		use_fill_color |= pParams->GetInteger(CRCD(0xda7a68b8,"fill_g"), &g);
		use_fill_color |= pParams->GetInteger(CRCD(0xaa109c37,"fill_b"), &b);
		use_fill_color |= pParams->GetInteger(CRCD(0x3319cd8d,"fill_a"), &a);

		Image::RGBA fill_color(r, g, b, a);

		p_texture->PushToPoint(point, axis, num_pixels, use_fill_color, fill_color);
	}
	else
	{
		Dbg_MsgAssert(0, ("Can't find texture %s to push", Script::FindChecksumName(checksum)));
	}

	return true;
}

// @script | AdjustTextureBrightness | Scales the texture brightness by the supplied value
// @parm name |  | name of texture
// @parm float | brightness | Brightness value (1.0 = no change)
// @parmopt int | adjust_current | 0 | If set to 1, adjusts the current image, even if the original image exists
bool ScriptAdjustTextureBrightness(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	uint32 checksum;
	if (!pParams->GetChecksum(NONAME, &checksum))
		Dbg_MsgAssert(0, ("no texture specified"));
	
	CTexture *p_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(checksum);
	if (p_texture)
	{
		float brightness = 1.0f;
		int adjust_current = 0;
		bool force_adjust_current = false;

		pParams->GetFloat(CRCD(0x2689291c,"brightness"), &brightness, true);
		pParams->GetInteger(CRCD(0xc164842a,"adjust_current"), &adjust_current);
		if (adjust_current == 1)
		{
			force_adjust_current = true;
		}

		p_texture->AdjustBrightness(brightness, force_adjust_current);
	}
	else
	{
		Dbg_MsgAssert(0, ("Can't find texture %s", Script::FindChecksumName(checksum)));
	}

	return true;
}

// @script | AdjustTextureHSV | Scales the texture HSV by the supplied values
// @parm name |  | name of texture
// @parm float | h | Hue offset (0 - 360)
// @parm float | s | Saturation scale (1.0 = no change)
// @parm float | v | Brightness scale (1.0 = no change)
// @parmopt int | adjust_current | 0 | If set to 1, adjusts the current image, even if the original image exists
bool ScriptAdjustTextureHSV(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	uint32 checksum;
	if (!pParams->GetChecksum(NONAME, &checksum))
		Dbg_MsgAssert(0, ("no texture specified"));
	
	CTexture *p_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(checksum);
	if (p_texture)
	{
		float h = 0.0f;
		float s = 1.0f;
		float v = 1.0f;
		int adjust_current = 0;
		bool force_adjust_current = false;

		bool got_params;
		got_params = pParams->GetFloat(CRCD(0x6e94f918,"h"), &h);
		got_params = pParams->GetFloat(CRCD(0xe4f130f4,"s"), &s) && got_params;
		got_params = pParams->GetFloat(CRCD(0x949bc47b,"v"), &v) && got_params;

		Dbg_MsgAssert(got_params, ("Need to supply values for h, s, and v"));
		Dbg_MsgAssert((h >= 0.0f) && (h <= 360.0f), ("h must be in the range of 0-360"));
		Dbg_MsgAssert(s >= 0.0f, ("s cannot be negative"));

		pParams->GetInteger(CRCD(0xc164842a,"adjust_current"), &adjust_current);
		if (adjust_current == 1)
		{
			force_adjust_current = true;
		}

		p_texture->AdjustHSV(h, s, v, force_adjust_current);
	}
	else
	{
		Dbg_MsgAssert(0, ("Can't find texture %s", Script::FindChecksumName(checksum)));
	}

	return true;
}
// @script | CopyTexture | Copy 2D sprite texture into a new texture
// @parm name | src | name of texture
// @parm name | new | name of new texture
bool ScriptCopyTexture(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	uint32 checksum;
	if (!pParams->GetChecksum(CRCD(0x9fbbdb72,"src"), &checksum))
		Dbg_MsgAssert(0, ("no texture specified"));
	
	CTexture *p_texture = CTexDictManager::sp_sprite_tex_dict->GetTexture(checksum);
	if (p_texture)
	{
		uint32 new_checksum;
		if (pParams->GetChecksum(CRCD(0x941cbbba,"new"), &new_checksum))
		{
			CTexDictManager::sp_sprite_tex_dict->CopyTexture(new_checksum, p_texture);
		}
		else
		{
			Dbg_MsgAssert(0, ("no new texture specified"));
		}
	} else {
		Dbg_MsgAssert(0, ("Can't find texture %s to copy", Script::FindChecksumName(checksum)));
	}

	return true;
}

// @script | CombineTextures | Combine two textures
// @parm name | src | name of orig texture (and final texture if no new texture is supplied)
// @parm name | top | name of top additional texture
// @parmopt name | new | | name of new destination texture
// @parmopt flag | no_palette_gen | | use original palette (faster)
bool ScriptCombineTextures(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	uint32 new_checksum = 0;
	uint32 tex1_checksum;
	uint32 tex2_checksum;
	bool gen_palette = true;

	if (!pParams->GetChecksum(CRCD(0x9fbbdb72,"src"), &tex1_checksum))
	{
		Dbg_MsgAssert(0, ("no original source texture specified"));
	}
	if (!pParams->GetChecksum(CRCD(0xe126e035,"top"), &tex2_checksum))
	{
		Dbg_MsgAssert(0, ("no top source texture specified"));
	}
	pParams->GetChecksum(CRCD(0x941cbbba,"new"), &new_checksum);
	if (pParams->ContainsFlag(CRCD(0x5905256b,"no_palette_gen")))
	{
		gen_palette = false;
	}

	CTexture *p_texture1 = CTexDictManager::sp_sprite_tex_dict->GetTexture(tex1_checksum);
	if (p_texture1)
	{
		CTexture *p_texture2 = CTexDictManager::sp_sprite_tex_dict->GetTexture(tex2_checksum);
		if (p_texture2)
		{
			if (new_checksum)
			{
				// Make new texture
				CTexDictManager::sp_sprite_tex_dict->CombineTextures(new_checksum, p_texture1, p_texture2, gen_palette);
			}
			else
			{
				// Just overwrite original texture
				p_texture1->CombineTextures(p_texture2, gen_palette);
			}
		}
		else
		{
			Dbg_MsgAssert(0, ("Can't find texture %s to combine", Script::FindChecksumName(tex2_checksum)));
		}
	}
	else
	{
		Dbg_MsgAssert(0, ("Can't find texture %s to combine", Script::FindChecksumName(tex1_checksum)));
	}

	return true;
}

bool ScriptLoadParticleTexture(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	const char *p_name;
	if (!pParams->GetText(NONAME, &p_name))
		Dbg_MsgAssert(0, ("no texture specified"));

//	Dbg_MsgAssert(*p_name,("%s\n Empty name string for Particle Texture\n",pScript->GetScriptInfo()));

	if (! *p_name)
	{
		Dbg_Message("WARNING:  empty name in LoadAllParticleTextures - proabably an old level\n");
		return false;
	}

	
	CTexture *p_texture = CTexDictManager::sp_particle_tex_dict->GetTexture(p_name);
	
	bool	perm = pParams->ContainsFlag(CRCD(0xd5928f25,"perm"));
	
	if (!p_texture)
	{
		p_texture = CTexDictManager::sp_particle_tex_dict->LoadTexture(p_name, true, true, perm);
	}
	else
	{
		// The testure is already there
		// if we are on CD, we assume that's the one we want to load
		// but if not, then we assume we want to re-load the texture
		// for quick previewing by artists
		// so we unload it, and load it up agian
		#ifdef	__PLAT_NGPS__		
		if (!Config::CD())
		{	
			if (Config::gGotExtraMemory)
			{
				// If we have the extra memory, then use that
				// to avoid fragmentation when testing variants of perm textures
				Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());
			}
			perm = p_texture->IsPerm();
			CTexDictManager::sp_particle_tex_dict->UnloadTexture(p_texture);
			p_texture = CTexDictManager::sp_particle_tex_dict->LoadTexture(p_name, true, true, perm);
			if (Config::gGotExtraMemory)
			{
				Mem::Manager::sHandle().PopContext();
			}
		}
		#endif
	}
	
	return p_texture != NULL;
}




bool ScriptUnloadParticleTexture(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	const char *p_name;
	if (!pParams->GetText(NONAME, &p_name))
		Dbg_MsgAssert(0, ("no texture specified"));
	
	CTexture *p_texture = CTexDictManager::sp_particle_tex_dict->GetTexture(p_name);
	if (p_texture)
	{
		p_texture->RemoveFromVram();
		CTexDictManager::sp_particle_tex_dict->UnloadTexture(p_texture);
	} else {

		printf ("Can't find particle texture %s to unload", p_name);
	}
	return true;
}

bool ScriptLoadSFPTexture(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	const char *p_name;
	if (!pParams->GetText(NONAME, &p_name))
		Dbg_MsgAssert(0, ("no texture specified"));
	
	CTexture *p_texture = CTexDictManager::sp_particle_tex_dict->LoadTexture(p_name, false);

	return p_texture != NULL;
}




bool ScriptUnloadSFPTexture(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	const char *p_name;
	if (!pParams->GetText(NONAME, &p_name))
		Dbg_MsgAssert(0, ("no texture specified"));
	
	CTexture *p_texture = CTexDictManager::sp_particle_tex_dict->GetTexture(p_name);
	if (p_texture)
	{
		CTexDictManager::sp_particle_tex_dict->UnloadTexture(p_texture);
	} else {
		Dbg_MsgAssert(0, ("Can't find texture %s to unload", p_name));
	}
	return true;
}

bool ScriptDumpTextures(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	#ifdef	__NOPT_ASSERT__
	// Get the hashable that has the textures in it
	Lst::HashTable<CTexture> * p_textures = CTexDictManager::sp_particle_tex_dict->GetTexLookup();
	p_textures->PrintContents();

	p_textures->IterateStart();
	Nx::CTexture *p_tex = p_textures->IterateNext();		
	while(p_tex)
	{
	
		printf ("0x%8x: (%d x %d) %s\n",
					p_tex->GetChecksum(),
					 p_tex->GetWidth(),
					 p_tex->GetHeight(),
					 p_tex->GetName()
					);  
	
		p_tex = p_textures->IterateNext();		
	}
	#endif
	
	return true;
}


void 	FlushParticleTextures(bool all)
{
   
   CTexDictManager::sp_particle_tex_dict->FlushTextures(all); 
	
}


} //end namespace Nx


