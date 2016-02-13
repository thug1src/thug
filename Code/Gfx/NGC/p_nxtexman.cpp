/////////////////////////////////////////////////////////////////////////////
// p_NxTexMan.cpp - Ngc platform specific interface to CTexMan
//

#include <core/defines.h>

#include "gfx/NxTexMan.h"
#include "gfx/Ngc/p_NxTexture.h"
#include "gfx/Ngc/nx/import.h"

namespace	Nx
{


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// Functions


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CTexDict* CTexDictManager::s_plat_load_texture_dictionary( const char *p_tex_dict_name, bool is_level_data, uint32 texDictOffset, bool isSkin, bool forceTexDictLookup )
{
//	NxNgc::LoadTextureFile( p_tex_dict_name );
	return new CNgcTexDict( p_tex_dict_name, forceTexDictLookup );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CTexDict* CTexDictManager::s_plat_load_texture_dictionary( uint32 checksum, uint32 *p_data, int data_size, bool is_level_data, uint32 texDictOffset, bool is_skin, bool forceTexDictLookup )
{
	CNgcTexDict *p_dict = new CNgcTexDict( checksum );
	p_dict->LoadTextureDictionaryFromMemory( p_data );
	return p_dict;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CTexDict* CTexDictManager::s_plat_create_texture_dictionary( uint32 checksum )
{
	return new CNgcTexDict( checksum );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CTexDictManager::s_plat_unload_texture_dictionary( CTexDict* p_tex_dict )
{
	delete p_tex_dict;
	return true;
}


} 
 

