/////////////////////////////////////////////////////////////////////////////
// p_NxTexMan.cpp - Xbox platform specific interface to CTexMan
//

#include <core/defines.h>

#include "gfx/NxTexMan.h"
#include "gfx/xbox/p_NxTexture.h"
#include "gfx/xbox/nx/texture.h"

namespace	Nx
{


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// Functions


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CTexDict* CTexDictManager::s_plat_load_texture_dictionary( const char *p_tex_dict_name, bool is_level_data, uint32 texDictOffset, bool is_skin, bool forceTexDictLookup )
{
	return new CXboxTexDict( p_tex_dict_name );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CTexDict* CTexDictManager::s_plat_load_texture_dictionary( uint32 checksum, uint32 *p_data, int data_size, bool is_level_data, uint32 texDictOffset, bool is_skin, bool forceTexDictLookup )
{
	CXboxTexDict *p_dict = new CXboxTexDict( checksum );
	p_dict->LoadTextureDictionaryFromMemory( p_data );
	return p_dict;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CTexDict* CTexDictManager::s_plat_create_texture_dictionary( uint32 checksum )
{
	return new CXboxTexDict( checksum );
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
 
