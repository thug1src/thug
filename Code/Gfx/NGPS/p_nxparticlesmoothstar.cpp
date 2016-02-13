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

#include "gfx/ngps/p_nxparticleSmoothStar.h"

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CPs2ParticleSmoothStar::CPs2ParticleSmoothStar()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CPs2ParticleSmoothStar::CPs2ParticleSmoothStar( uint32 checksum, int max_particles, uint32 texture_checksum, uint32 blendmode_checksum, int fix, int num_segments, float split, int history )
{
	m_checksum = checksum;
	m_max_particles = max_particles;
	m_num_particles = 0;

	mp_particle_array = new CParticleEntry[max_particles];
	
	// Allocate vertex buffer.
	mp_vertices = new float[max_particles * 3];

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
	for ( int lp = 0; lp < 3; lp++ )
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

	m_num_segments = num_segments;
	m_split = split;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CPs2ParticleSmoothStar::~CPs2ParticleSmoothStar()
{
	delete [] mp_particle_array;
	delete [] mp_vertices;
//	delete mp_engine_particle;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2ParticleSmoothStar::plat_get_position( int entry, int list, float * x, float * y, float * z )
{
	float* p_v = &mp_vertices[entry*3];
	*x = p_v[0];
	*y = p_v[1];
	*z = p_v[2];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2ParticleSmoothStar::plat_set_position( int entry, int list, float x, float y, float z )
{
	float* p_v = &mp_vertices[entry*3];
	p_v[0] = x;
	p_v[1] = y;
	p_v[2] = z;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2ParticleSmoothStar::plat_add_position( int entry, int list, float x, float y, float z )
{
	float* p_v = &mp_vertices[entry*3];
	p_v[0] += x;
	p_v[1] += y;
	p_v[2] += z;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
int CPs2ParticleSmoothStar::plat_get_num_particle_colors( void ) { return 3; }
int CPs2ParticleSmoothStar::plat_get_num_vertex_lists( void ) { return 1; }
void CPs2ParticleSmoothStar::plat_set_sr( int entry, uint8 value ) { m_start_color[entry].r = value; }
void CPs2ParticleSmoothStar::plat_set_sg( int entry, uint8 value ) { m_start_color[entry].g = value; }
void CPs2ParticleSmoothStar::plat_set_sb( int entry, uint8 value ) { m_start_color[entry].b = value; }
void CPs2ParticleSmoothStar::plat_set_sa( int entry, uint8 value ) { m_start_color[entry].a = value >> 1; }
void CPs2ParticleSmoothStar::plat_set_mr( int entry, uint8 value ) { m_mid_color[entry].r = value; }
void CPs2ParticleSmoothStar::plat_set_mg( int entry, uint8 value ) { m_mid_color[entry].g = value; }
void CPs2ParticleSmoothStar::plat_set_mb( int entry, uint8 value ) { m_mid_color[entry].b = value; }
void CPs2ParticleSmoothStar::plat_set_ma( int entry, uint8 value ) { m_mid_color[entry].a = value >> 1; }
void CPs2ParticleSmoothStar::plat_set_er( int entry, uint8 value ) { m_end_color[entry].r = value; }
void CPs2ParticleSmoothStar::plat_set_eg( int entry, uint8 value ) { m_end_color[entry].g = value; }
void CPs2ParticleSmoothStar::plat_set_eb( int entry, uint8 value ) { m_end_color[entry].b = value; }
void CPs2ParticleSmoothStar::plat_set_ea( int entry, uint8 value ) { m_end_color[entry].a = value >> 1; }
		
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2ParticleSmoothStar::plat_render( void )
{
	if (m_num_particles == 0)
		return;

	// Draw the particles.
	
	// Used to figure the right and up vectors for creating screen-aligned particle quads.
	//Mth::Matrix* p_matrix = (Mth::Matrix*)&NxPs2::render::CameraOrientation;
	Mth::Matrix* p_matrix = (Mth::Matrix*)&NxPs2::render::CameraToWorldRotation;

	// Concatenate p_matrix with the emmission angle to create the direction.
	Mth::Vector up( 0.0f, 1.0f, 0.0f, 0.0f );

	// Get the 'right' vector as the cross product of camera 'at and world 'up'.
	Mth::Vector at( p_matrix->GetAt()[X], p_matrix->GetAt()[Y], p_matrix->GetAt()[Z], 0.0f );
	Mth::Vector screen_right	= Mth::CrossProduct( at, up );
	Mth::Vector screen_up		= Mth::CrossProduct( screen_right, at );

	screen_right.Normalize();
	screen_up.Normalize();
	
	int				lp;
	CParticleEntry	*p_particle;
	float			*p_v;

	NxPs2::dma::BeginTag(NxPs2::dma::cnt, 0);
	NxPs2::CImmediateMode::sStartPolyDraw( mp_engine_texture, m_blend, ABS );

	for ( lp = 0, p_particle = mp_particle_array, p_v = mp_vertices; lp < m_num_particles; lp++, p_particle++, p_v += 3 )
	{
		float terp = p_particle->m_time / p_particle->m_life;

		float w = p_particle->m_sw + ( ( p_particle->m_ew - p_particle->m_sw ) * terp );
		float h = p_particle->m_sh + ( ( p_particle->m_eh - p_particle->m_sh ) * terp );

		// Todo: Move hook to matrix/emitter code to cut down on per particle calculation.
		Mth::Vector	pos( p_v[0] + m_pos[X], p_v[1] + m_pos[Y], p_v[2] + m_pos[Z] );
		Mth::Vector	ss_right, ss_up;	//, ss_pos;
		Mth::Vector tmp[4];
	
		ss_right	= screen_right * w;
		ss_up		= screen_up * h;
	
		Image::RGBA color[3];
		Image::RGBA *p_col0;
		Image::RGBA *p_col1;

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

		for ( int c = 0; c < 3; c++ )
		{
			Image::RGBA SmoothStart = *p_col0++;
			Image::RGBA end = *p_col1++;

			color[c].r = SmoothStart.r + (uint8)(( ((float)( end.r - SmoothStart.r )) * terp ));
			color[c].g = SmoothStart.g + (uint8)(( ((float)( end.g - SmoothStart.g )) * terp ));
			color[c].b = SmoothStart.b + (uint8)(( ((float)( end.b - SmoothStart.b )) * terp ));
			color[c].a = SmoothStart.a + (uint8)(( ((float)( end.a - SmoothStart.a )) * terp ));
		}

		tmp[1]  = pos;
		tmp[1] += ss_right * sinf( Mth::DegToRad( 0.0f ) ) * m_split;
		tmp[1] += ss_up    * cosf( Mth::DegToRad( 0.0f ) ) * m_split;

		for ( int lp2 = 0; lp2 < m_num_segments; lp2++ )
		{
			tmp[0]  = pos;
			tmp[0] += ss_right * sinf( Mth::DegToRad( ( ( 360.0f / ((float)m_num_segments*2) ) * ( ( lp2 * 2 ) + 1 ) ) ) ) * m_split;
			tmp[0] += ss_up    * cosf( Mth::DegToRad( ( ( 360.0f / ((float)m_num_segments*2) ) * ( ( lp2 * 2 ) + 1 ) ) ) ) * m_split;
			
			tmp[3]  = pos;
			tmp[3] += ss_right * sinf( Mth::DegToRad( ( ( 360.0f / ((float)m_num_segments*2) ) * ( ( lp2 * 2 ) + 2 ) ) ) ) * m_split;
			tmp[3] += ss_up    * cosf( Mth::DegToRad( ( ( 360.0f / ((float)m_num_segments*2) ) * ( ( lp2 * 2 ) + 2 ) ) ) ) * m_split;
			
			tmp[2]  = pos;
			tmp[2] += ss_right * sinf( Mth::DegToRad( ( ( 360.0f / ((float)m_num_segments*2) ) * ( ( lp2 * 2 ) + 1 ) ) ) );
			tmp[2] += ss_up    * cosf( Mth::DegToRad( ( ( 360.0f / ((float)m_num_segments*2) ) * ( ( lp2 * 2 ) + 1 ) ) ) );

			NxPs2::CImmediateMode::sDrawSmoothStarSegment( tmp[0], pos, tmp[1], tmp[2], tmp[3],
														  *((uint32 *) &color[0]),
														  *((uint32 *) &color[1]),
														  *((uint32 *) &color[2]));
			tmp[1] = tmp[3];
		}
	}
	
	NxPs2::dma::EndTag();
}

} // Nx



