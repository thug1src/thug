//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxParticle.cpp
//* OWNER:          Paul Robinson
//* CREATION DATE:  3/27/2002
//****************************************************************************

#include <core/defines.h>
#include "gfx/ngps/nx/render.h"
#include "gfx/ngps/nx/dma.h"
#include "gfx/ngps/nx/vif.h"
#include "gfx/ngps/nx/vu1.h"
#include "gfx/ngps/nx/gif.h"
#include "gfx/ngps/nx/gs.h"

#include "gfx/ngps/nx/line.h"
#include <gfx/NxTexMan.h>
#include <gfx/ngps/p_nxtexture.h>


#include "gfx/ngps/nx/immediate.h"
#include "gfx/ngps/nx/vu1code.h"

#include "gfx/ngps/nx/mesh.h"

#include "gfx/ngps/p_nxparticleLine.h"

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CPs2ParticleLine::CPs2ParticleLine()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CPs2ParticleLine::CPs2ParticleLine( uint32 checksum, int max_particles, uint32 texture_checksum, uint32 blendmode_checksum, int fix, int num_segments, float split, int history )
{
	m_checksum = checksum;
	m_max_particles = max_particles;
	m_num_particles = 0;

	mp_particle_array = new CParticleEntry[max_particles];
	
	// Allocate vertex buffers.
	mp_vertices = new float*[( history + 1)];
	for ( int lp = 0; lp < ( history + 1 ); lp++ )
	{
		mp_vertices[lp] = new float[max_particles * 3];
	}
	m_num_vertex_buffers = history + 1;

//	// Create the engine representation.
//	mp_engine_particle = new NxPs2::sParticleSystem( max_particles, texture_checksum, blendmode_checksum, fix );
//

	// Get the texture.

	Nx::CTexture *p_texture;
	Nx::CPs2Texture *p_ps2_texture;
	mp_engine_texture = NULL;

	p_texture = Nx::CTexDictManager::sp_particle_tex_dict->GetTexture( texture_checksum );
	p_ps2_texture = static_cast<Nx::CPs2Texture*>( p_texture );
	if ( p_ps2_texture )
	{
		mp_engine_texture = p_ps2_texture->GetSingleTexture(); 
	}

	// Set blendmode.
	m_blend = NxPs2::CImmediateMode::sGetTextureBlend( blendmode_checksum, fix );

	// Default color.
	m_start_color = new Image::RGBA[m_num_vertex_buffers];
	m_mid_color = new Image::RGBA[m_num_vertex_buffers];
	m_end_color = new Image::RGBA[m_num_vertex_buffers];
	for ( int lp = 0; lp < m_num_vertex_buffers; lp++ )
	{
		m_start_color[lp].r = 128;
		m_start_color[lp].g = 128;
		m_start_color[lp].b = 128;
		m_start_color[lp].a = 128;
		m_mid_color[lp].r = 128;
		m_mid_color[lp].g = 128;
		m_mid_color[lp].b = 128;
		m_mid_color[lp].a = 128;
		m_end_color[lp].r = 128;
		m_end_color[lp].g = 128;
		m_end_color[lp].b = 128;
		m_end_color[lp].a = 128;
	}

	m_mid_time = -1.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CPs2ParticleLine::~CPs2ParticleLine()
{
	delete [] mp_particle_array;
	for ( int lp = 0; lp < m_num_vertex_buffers; lp++ )
	{
		delete [] mp_vertices[lp];
	}
	delete [] mp_vertices;
//	delete mp_engine_particle;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2ParticleLine::plat_get_position( int entry, int list, float * x, float * y, float * z )
{
	float* p_v = &mp_vertices[list][entry*3];
	*x = p_v[0];
	*y = p_v[1];
	*z = p_v[2];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2ParticleLine::plat_set_position( int entry, int list, float x, float y, float z )
{
	float* p_v = &mp_vertices[list][entry*3];
	p_v[0] = x;
	p_v[1] = y;
	p_v[2] = z;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2ParticleLine::plat_add_position( int entry, int list, float x, float y, float z )
{
	float* p_v = &mp_vertices[list][entry*3];
	p_v[0] += x;
	p_v[1] += y;
	p_v[2] += z;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
int CPs2ParticleLine::plat_get_num_particle_colors( void ) { return 2; }
int CPs2ParticleLine::plat_get_num_vertex_lists( void ) { return m_num_vertex_buffers; }
void CPs2ParticleLine::plat_set_sr( int entry, uint8 value ) { m_start_color[entry].r = value; }
void CPs2ParticleLine::plat_set_sg( int entry, uint8 value ) { m_start_color[entry].g = value; }
void CPs2ParticleLine::plat_set_sb( int entry, uint8 value ) { m_start_color[entry].b = value; }
void CPs2ParticleLine::plat_set_sa( int entry, uint8 value ) { m_start_color[entry].a = value >> 1; }
void CPs2ParticleLine::plat_set_mr( int entry, uint8 value ) { m_mid_color[entry].r = value; }
void CPs2ParticleLine::plat_set_mg( int entry, uint8 value ) { m_mid_color[entry].g = value; }
void CPs2ParticleLine::plat_set_mb( int entry, uint8 value ) { m_mid_color[entry].b = value; }
void CPs2ParticleLine::plat_set_ma( int entry, uint8 value ) { m_mid_color[entry].a = value >> 1; }
void CPs2ParticleLine::plat_set_er( int entry, uint8 value ) { m_end_color[entry].r = value; }
void CPs2ParticleLine::plat_set_eg( int entry, uint8 value ) { m_end_color[entry].g = value; }
void CPs2ParticleLine::plat_set_eb( int entry, uint8 value ) { m_end_color[entry].b = value; }
void CPs2ParticleLine::plat_set_ea( int entry, uint8 value ) { m_end_color[entry].a = value >> 1; }

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2ParticleLine::plat_render( void )
{
	if (m_num_particles == 0)
		return;

	// Draw the particles.
	
	int				lp;
	CParticleEntry	*p_particle;
	float			*p_v0;
	float			*p_v1;

	NxPs2::dma::BeginTag(NxPs2::dma::cnt, 0);
	NxPs2::CImmediateMode::sStartPolyDraw( mp_engine_texture, m_blend, ABS );

	Image::RGBA color[2];
	Image::RGBA *p_col0;
	Image::RGBA *p_col1;

	for ( lp = 0, p_particle = mp_particle_array, p_v0 = mp_vertices[0], p_v1 = mp_vertices[(m_num_vertex_buffers-1)]; lp < m_num_particles; lp++, p_particle++, p_v0 += 3, p_v1 += 3 )
	{
		float terp = p_particle->m_time / p_particle->m_life;

		if ( m_mid_time >= 0.0f )
		{
			if ( terp < m_mid_time )
			{
				p_col0 = m_start_color;
				p_col1 = m_mid_color;
				// Adjust interpolation for this half of the color blend.
				terp = terp / m_mid_time;
			}
			else
			{
				p_col0 = m_mid_color;
				p_col1 = m_end_color;
				// Adjust interpolation for this half of the color blend.
				terp = ( terp - m_mid_time ) / ( 1.0f - m_mid_time );
			}
		}
		else
		{
			// No mid color specified.
			p_col0 = m_start_color;
			p_col1 = m_end_color;
		}

		for ( int c = 0; c < 2; c++ )
		{
			Image::RGBA start = *p_col0++;
			Image::RGBA end = *p_col1++;

			color[c].r = start.r + (uint8)(( ((float)( end.r - start.r )) * terp ));
			color[c].g = start.g + (uint8)(( ((float)( end.g - start.g )) * terp ));
			color[c].b = start.b + (uint8)(( ((float)( end.b - start.b )) * terp ));
			color[c].a = start.a + (uint8)(( ((float)( end.a - start.a )) * terp ));
		}

		// Todo: Move hook to matrix/emitter code to cut down on per particle calculation.
		Mth::Vector	pos0( p_v0[0] + m_pos[X], p_v0[1] + m_pos[Y], p_v0[2] + m_pos[Z] );
		Mth::Vector	pos1( p_v1[0] + m_pos[X], p_v1[1] + m_pos[Y], p_v1[2] + m_pos[Z] );
	
		NxPs2::CImmediateMode::sDrawLine( pos0, pos1, *((uint32 *) &color[0]), *((uint32 *) &color[1]) );
	}

	NxPs2::dma::EndTag();
}

} // Nx


