/////////////////////////////////////////////////////////////////////////////
// p_NxTexMan.cpp - PS2 platform specific interface to CTexMan
//
// This is PS2 SPECIFIC!!!!!!  So might get a bit messy

#include <core/defines.h>

#include "gfx/NxTexMan.h"
#include "gfx/NGPS/p_NxTexture.h"

#include <libgraph.h>

namespace NxPs2
{
	void WaitForRendering();
}


namespace	Nx
{


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// Functions


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexDict *		CTexDictManager::s_plat_load_texture_dictionary(const char *p_tex_dict_name, bool is_level_data, uint32 texDictOffset, bool isSkin, bool forceTexDictLookup)
{
	return new CPs2TexDict(p_tex_dict_name, is_level_data, texDictOffset, isSkin, forceTexDictLookup);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexDict *		CTexDictManager::s_plat_load_texture_dictionary(uint32 checksum, uint32* pData, int dataSize, bool is_level_data, uint32 texDictOffset, bool isSkin, bool forceTexDictLookup)
{
	CPs2TexDict* pTexDict = new CPs2TexDict( checksum );
	pTexDict->LoadTextureDictionary(NULL, pData, dataSize, is_level_data, texDictOffset, isSkin, forceTexDictLookup);
	return pTexDict;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexDict *		CTexDictManager::s_plat_create_texture_dictionary(uint32 checksum)
{
	return new CPs2TexDict(checksum);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CTexDictManager::s_plat_unload_texture_dictionary(CTexDict *p_tex_dict)
{
	#if 0
	sceGsSyncPath(0, 0);		// To make sure it isn't being used
	#else
	NxPs2::WaitForRendering();
	#endif
	
	delete p_tex_dict;

	return true;
}


} 
 
