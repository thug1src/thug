
/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GFX (Graphics Library)									**
**																			**
**	Module:			Graphics (GFX)		 									**
**																			**
**	File name:		particle.cpp											**
**																			**
**	Created:		03/27/02	-	dc										**
**																			**
**	Description:	Low level particle rendering code						**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/
		
#include <xtl.h>
#include <core/math.h>
#include <gfx/NxTexMan.h>
#include <gfx/xbox/p_nxtexture.h>
#include "render.h"
#include "particles.h"

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/


/*****************************************************************************
**								  Externals									**
*****************************************************************************/

namespace NxXbox
{


/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sParticleSystem::sParticleSystem( uint32 max_particles, eParticleType type, uint32 texture_checksum, uint32 blendmode_checksum, int fix, int num_segments, int history )
{
	// Obtain the texture.
	Nx::CTexture		* p_texture			= Nx::CTexDictManager::sp_particle_tex_dict->GetTexture( texture_checksum );
	Nx::CXboxTexture	*p_xbox_texture		= static_cast<Nx::CXboxTexture*>( p_texture );
	NxXbox::sTexture	*p_engine_texture	= NULL;
	if( p_xbox_texture )
	{
		p_engine_texture = p_xbox_texture->GetEngineTexture(); 
	}
	
	// Create a (semi-transparent) material used to render the mesh.
	sMaterial *p_material			= new sMaterial();
	p_material->m_flags[0]			= 0x40 | (( p_engine_texture ) ? MATFLAG_TEXTURED : 0 );
	p_material->m_checksum			= (uint32)rand() * (uint32)rand();
	p_material->m_passes			= 1;
	p_material->mp_tex[0]			= p_engine_texture;
	p_material->m_reg_alpha[0]		= GetBlendMode( blendmode_checksum ) | ((uint32)fix << 24 );
	p_material->m_color[0][0]		= 0.5f;
	p_material->m_color[0][1]		= 0.5f;
	p_material->m_color[0][2]		= 0.5f;
	p_material->m_color[0][3]		= fix * ( 1.0f /  128.0f );

	p_material->m_uv_addressing[0]	= 0;
	p_material->m_k[0]				= 0.0f;
	p_material->m_alpha_cutoff		= 1;
	p_material->m_no_bfc			= true;
	p_material->m_uv_wibble			= false;

	mp_material						= p_material;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sParticleSystem::~sParticleSystem( void )
{
	delete mp_material;
}

} // namespace NxXbox
