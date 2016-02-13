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

#include "gfx/Ngc/p_nxparticleflat.h"

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CNgcParticleFlat::CNgcParticleFlat()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CNgcParticleFlat::CNgcParticleFlat( uint32 checksum, int max_particles, uint32 texture_checksum, uint32 blendmode_checksum, int fix, int num_segments, float split, int history )
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
	m_start_color.r = 128;
	m_start_color.g = 128;
	m_start_color.b = 128;
	m_start_color.a = 128;
	m_mid_color.r = 128;
	m_mid_color.g = 128;
	m_mid_color.b = 128;
	m_mid_color.a = 128;
	m_end_color.r = 128;
	m_end_color.g = 128;
	m_end_color.b = 128;
	m_end_color.a = 128;

	m_mid_time = -1.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CNgcParticleFlat::~CNgcParticleFlat()
{
	delete [] mp_particle_array;
	delete [] mp_vertices;
//	delete mp_engine_particle;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CNgcParticleFlat::plat_get_position( int entry, int list, float * x, float * y, float * z )
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
	
void CNgcParticleFlat::plat_set_position( int entry, int list, float x, float y, float z )
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
	
void CNgcParticleFlat::plat_add_position( int entry, int list, float x, float y, float z )
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
	
int CNgcParticleFlat::plat_get_num_particle_colors( void ) { return 1; }
int CNgcParticleFlat::plat_get_num_vertex_lists( void ) { return 1; }
void CNgcParticleFlat::plat_set_sr( int entry, uint8 value ) { m_start_color.r = value; }
void CNgcParticleFlat::plat_set_sg( int entry, uint8 value ) { m_start_color.g = value; }
void CNgcParticleFlat::plat_set_sb( int entry, uint8 value ) { m_start_color.b = value; }
void CNgcParticleFlat::plat_set_sa( int entry, uint8 value ) { m_start_color.a = value; }
void CNgcParticleFlat::plat_set_mr( int entry, uint8 value ) { m_mid_color.r = value; }
void CNgcParticleFlat::plat_set_mg( int entry, uint8 value ) { m_mid_color.g = value; }
void CNgcParticleFlat::plat_set_mb( int entry, uint8 value ) { m_mid_color.b = value; }
void CNgcParticleFlat::plat_set_ma( int entry, uint8 value ) { m_mid_color.a = value; }
void CNgcParticleFlat::plat_set_er( int entry, uint8 value ) { m_end_color.r = value; }
void CNgcParticleFlat::plat_set_eg( int entry, uint8 value ) { m_end_color.g = value; }
void CNgcParticleFlat::plat_set_eb( int entry, uint8 value ) { m_end_color.b = value; }
void CNgcParticleFlat::plat_set_ea( int entry, uint8 value ) { m_end_color.a = value; }
		
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CNgcParticleFlat::plat_render( void )
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

	if ( mp_engine_texture )
	{
		GX::SetVtxDesc( 3, GX_VA_POS, GX_DIRECT, GX_VA_CLR0, GX_DIRECT, GX_VA_TEX0, GX_DIRECT );
	}
	else
	{
		GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_CLR0, GX_DIRECT );
	}
	if ( m_num_particles > 0 ) GX::Begin( GX_QUADS, GX_VTXFMT0, m_num_particles * 4 );

	for ( lp = 0, p_particle = mp_particle_array, p_v = mp_vertices; lp < m_num_particles; lp++, p_particle++, p_v += 3 )
	{
		float terp = p_particle->m_time / p_particle->m_life;

		float w = p_particle->m_sw + ( ( p_particle->m_ew - p_particle->m_sw ) * terp );
		float h = p_particle->m_sh + ( ( p_particle->m_eh - p_particle->m_sh ) * terp );
		
		// Todo: Move hook to matrix/emitter code to cut down on per particle calculation.
		Mth::Vector	pos( p_v[0] + m_pos[X], p_v[1] + m_pos[Y], p_v[2] + m_pos[Z] );
		Mth::Vector	ss_right, ss_up;	//, ss_pos;
		Mth::Vector tmp[4];
	
		VECScale((Vec*)&screen_right, (Vec*)&ss_right, w );
		VECScale((Vec*)&screen_up, (Vec*)&ss_up, h );
	
		tmp[0]		= pos - ss_right + ss_up;
		tmp[1]		= pos + ss_right + ss_up;		
		tmp[2]		= pos + ss_right - ss_up;		
		tmp[3]		= pos - ss_right - ss_up;		
	
		GXColor color;
		GXColor *p_col0;
		GXColor *p_col1;
		
		if ( m_mid_time >= 0.0f )
		{
			if ( terp < m_mid_time )
			{
				p_col0 = &m_start_color;
				p_col1 = &m_mid_color;
				// Adjust interpolation for this half of the color blend.
				terp = terp / m_mid_time;
			}
			else
			{
				p_col0 = &m_mid_color;
				p_col1 = &m_end_color;
				// Adjust interpolation for this half of the color blend.
				terp = ( terp - m_mid_time ) / ( 1.0f - m_mid_time );
			}
		}
		else
		{
			// No mid color specified.
			p_col0 = &m_start_color;
			p_col1 = &m_end_color;
		}

		GXColor start = *p_col0;
		GXColor end = *p_col1;

		color.r = start.r + (uint8)(( ((float)( end.r - start.r )) * terp ));
		color.g = start.g + (uint8)(( ((float)( end.g - start.g )) * terp ));
		color.b = start.b + (uint8)(( ((float)( end.b - start.b )) * terp ));
		color.a = start.a + (uint8)(( ((float)( end.a - start.a )) * terp ));
	
		GX::Position3f32( tmp[0][X], tmp[0][Y], tmp[0][Z] );
		GX::Color1u32( *((uint32*)&color) );
		if ( mp_engine_texture ) GX::TexCoord2f32( 0.0f, 0.0f );

		GX::Position3f32( tmp[1][X], tmp[1][Y], tmp[1][Z] );
		GX::Color1u32( *((uint32*)&color) );
		if ( mp_engine_texture ) GX::TexCoord2f32( 1.0f, 0.0f );

		GX::Position3f32( tmp[2][X], tmp[2][Y], tmp[2][Z] );
		GX::Color1u32( *((uint32*)&color) );
		if ( mp_engine_texture ) GX::TexCoord2f32( 1.0f, 1.0f );

		GX::Position3f32( tmp[3][X], tmp[3][Y], tmp[3][Z] );
		GX::Color1u32( *((uint32*)&color) );
		if ( mp_engine_texture ) GX::TexCoord2f32( 0.0f, 1.0f );
	}

	if ( m_num_particles > 0 ) GX::End();
}

} // Nx

