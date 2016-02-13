//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxParticle.cpp
//* OWNER:          Paul Robinson
//* CREATION DATE:  3/27/2002
//****************************************************************************

#include <core/defines.h>
#include "gfx/Ngc/nx/render.h"
#include <gfx/NxTexMan.h>
#include <gfx/Ngc/p_nxtexture.h>
#include "gfx/Ngc/nx/mesh.h"
#include "gfx/ngc/nx/nx_init.h"
#include "sys/ngc/p_gx.h"

#include "gfx/Ngc/p_nxparticleStar.h"

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CNgcParticleStar::CNgcParticleStar()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CNgcParticleStar::CNgcParticleStar( uint32 checksum, int max_particles, uint32 texture_checksum, uint32 blendmode_checksum, int fix, int num_segments, float split, int history )
{
	m_checksum = checksum;
	m_max_particles = max_particles;
	m_num_particles = 0;

	mp_particle_array = new CParticleEntry[max_particles];
	
	// Allocate vertex buffer.
	mp_vertices = new float[max_particles * 3];

//	// Create the engine representation.
//	mp_engine_particle = new NxNgc::sParticleSystem( max_particles, texture_checksum, blendmode_checksum, fix );
//

	// Get the texture.

	Nx::CTexture *p_texture;
	Nx::CNgcTexture *p_Ngc_texture;
	mp_engine_texture = NULL;

	p_texture = Nx::CTexDictManager::sp_particle_tex_dict->GetTexture( texture_checksum );
	p_Ngc_texture = static_cast<Nx::CNgcTexture*>( p_texture );
	if ( p_Ngc_texture )
	{
		mp_engine_texture = p_Ngc_texture->GetEngineTexture(); 
	}

	// Set blendmode.
	m_blend = (uint8)get_texture_blend( blendmode_checksum );
	m_fix = fix;

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
	
CNgcParticleStar::~CNgcParticleStar()
{
	delete [] mp_particle_array;
	delete [] mp_vertices;
//	delete mp_engine_particle;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CNgcParticleStar::plat_get_position( int entry, int list, float * x, float * y, float * z )
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
	
void CNgcParticleStar::plat_set_position( int entry, int list, float x, float y, float z )
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
	
void CNgcParticleStar::plat_add_position( int entry, int list, float x, float y, float z )
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
	
int CNgcParticleStar::plat_get_num_particle_colors( void ) { return 3; }
int CNgcParticleStar::plat_get_num_vertex_lists( void ) { return 1; }
void CNgcParticleStar::plat_set_sr( int entry, uint8 value ) { m_start_color[entry].r = value; }
void CNgcParticleStar::plat_set_sg( int entry, uint8 value ) { m_start_color[entry].g = value; }
void CNgcParticleStar::plat_set_sb( int entry, uint8 value ) { m_start_color[entry].b = value; }
void CNgcParticleStar::plat_set_sa( int entry, uint8 value ) { m_start_color[entry].a = value; }
void CNgcParticleStar::plat_set_mr( int entry, uint8 value ) { m_mid_color[entry].r = value; }
void CNgcParticleStar::plat_set_mg( int entry, uint8 value ) { m_mid_color[entry].g = value; }
void CNgcParticleStar::plat_set_mb( int entry, uint8 value ) { m_mid_color[entry].b = value; }
void CNgcParticleStar::plat_set_ma( int entry, uint8 value ) { m_mid_color[entry].a = value; }
void CNgcParticleStar::plat_set_er( int entry, uint8 value ) { m_end_color[entry].r = value; }
void CNgcParticleStar::plat_set_eg( int entry, uint8 value ) { m_end_color[entry].g = value; }
void CNgcParticleStar::plat_set_eb( int entry, uint8 value ) { m_end_color[entry].b = value; }
void CNgcParticleStar::plat_set_ea( int entry, uint8 value ) { m_end_color[entry].a = value; }
		
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CNgcParticleStar::plat_render( void )
{
	NxNgc::sMaterialHeader		mat;
	NxNgc::sMaterialPassHeader	pass;

	// Header.
	mat.m_checksum			= 0xa9db601e;   // particle 
	mat.m_passes			= 1;
	mat.m_alpha_cutoff		= 1;
	mat.m_flags				= (1<<1);		// 2 sided.
//	mat.m_shininess			= 0.0f;

	// Pass 0.
	pass.m_texture.p_data	= mp_engine_texture;
	pass.m_flags			= ( mp_engine_texture ? (1<<0) : 0 ) | (1<<5) | (1<<6);		// textured, clamped.
	pass.m_filter			= 0;
	pass.m_blend_mode		= (unsigned char)m_blend;
	pass.m_alpha_fix		= (unsigned char)m_fix; 
	pass.m_k				= 0;
	pass.m_color.r			= 128;
	pass.m_color.g			= 128;
	pass.m_color.b			= 128;
	pass.m_color.a			= 255;

	NxNgc::multi_mesh( &mat, &pass, true, true );

	GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
	GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );

	// Draw the particles.
	
	// Used to figure the right and up vectors for creating screen-aligned particle quads.
	NsMatrix*	p_matrix	= &NxNgc::EngineGlobals.camera;

	// Concatenate p_matrix with the emmission angle to create the direction.

	NsVector	up( 0.0f, 1.0f, 0.0f );

	// Get the 'right' vector as the cross product of camera 'at and world 'up'.
	NsVector screen_right;
	NsVector screen_up;
	screen_right.cross( *p_matrix->getAt(), up );
	screen_up.cross( screen_right, *p_matrix->getAt());

	screen_right.normalize();
	screen_up.normalize();
	
	int				lp;
	CParticleEntry	*p_particle;
	float			*p_v;

	GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_CLR0, GX_DIRECT );

	for ( lp = 0, p_particle = mp_particle_array, p_v = mp_vertices; lp < m_num_particles; lp++, p_particle++, p_v += 3 )
	{
		float terp = p_particle->m_time / p_particle->m_life;

		float w = p_particle->m_sw + ( ( p_particle->m_ew - p_particle->m_sw ) * terp );
		float h = p_particle->m_sh + ( ( p_particle->m_eh - p_particle->m_sh ) * terp );

		// Todo: Move hook to matrix/emitter code to cut down on per particle calculation.
		Mth::Vector	pos( p_v[0] + m_pos[X], p_v[1] + m_pos[Y], p_v[2] + m_pos[Z] );
		Mth::Vector	ss_right, ss_up;	//, ss_pos;
		Mth::Vector tmp[3];
	
		VECScale((Vec*)&screen_right, (Vec*)&ss_right, w );
		VECScale((Vec*)&screen_up, (Vec*)&ss_up, h );
	
		GXColor color[3];
		GXColor *p_col0;
		GXColor *p_col1;

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
			GXColor start = *p_col0++;
			GXColor end = *p_col1++;

			color[c].r = start.r + (uint8)(( ((float)( end.r - start.r )) * terp ));
			color[c].g = start.g + (uint8)(( ((float)( end.g - start.g )) * terp ));
			color[c].b = start.b + (uint8)(( ((float)( end.b - start.b )) * terp ));
			color[c].a = start.a + (uint8)(( ((float)( end.a - start.a )) * terp ));
		}

		tmp[0]  = pos;
		tmp[0] += ss_right * sinf( Mth::DegToRad( 0.0f ) ) * m_split;
		tmp[0] += ss_up    * cosf( Mth::DegToRad( 0.0f ) ) * m_split;

		for ( int lp2 = 0; lp2 < m_num_segments; lp2++ )
		{
			tmp[1]  = pos;
			tmp[1] += ss_right * sinf( Mth::DegToRad( ( ( 360.0f / ((float)m_num_segments) ) * ( lp2 + 1 ) ) ) ) * m_split;
			tmp[1] += ss_up    * cosf( Mth::DegToRad( ( ( 360.0f / ((float)m_num_segments) ) * ( lp2 + 1 ) ) ) ) * m_split;

			tmp[2]  = pos;
			tmp[2] += ss_right * sinf( Mth::DegToRad( ( ( 360.0f / ((float)m_num_segments*2) ) * ( ( lp2 * 2 ) + 1 ) ) ) );
			tmp[2] += ss_up    * cosf( Mth::DegToRad( ( ( 360.0f / ((float)m_num_segments*2) ) * ( ( lp2 * 2 ) + 1 ) ) ) );

			GX::Begin( GX_TRIANGLESTRIP, GX_VTXFMT0, 4 );

			GX::Position3f32( pos[X], pos[Y], pos[Z] );
			GX::Color1u32( *((uint32*)&color[0]) );

			GX::Position3f32( tmp[0][X], tmp[0][Y], tmp[0][Z] );
			GX::Color1u32( *((uint32*)&color[1]) );

			GX::Position3f32( tmp[1][X], tmp[1][Y], tmp[1][Z] );
			GX::Color1u32( *((uint32*)&color[1]) );

			GX::Position3f32( tmp[2][X], tmp[2][Y], tmp[2][Z] );
			GX::Color1u32( *((uint32*)&color[2]) );

			GX::End();

			tmp[0] = tmp[1];
		}
	}
}

} // Nx



