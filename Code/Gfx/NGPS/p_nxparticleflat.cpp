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

#include "gfx/ngps/p_nxparticleflat.h"

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CPs2ParticleFlat::CPs2ParticleFlat()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CPs2ParticleFlat::CPs2ParticleFlat( uint32 checksum, int max_particles, uint32 texture_checksum, uint32 blendmode_checksum, int fix, int num_segments, float split, int history )
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
	
CPs2ParticleFlat::~CPs2ParticleFlat()
{
	delete [] mp_particle_array;
	delete [] mp_vertices;
//	delete mp_engine_particle;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CPs2ParticleFlat::plat_get_position( int entry, int list, float * x, float * y, float * z )
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
	
void CPs2ParticleFlat::plat_set_position( int entry, int list, float x, float y, float z )
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
	
void CPs2ParticleFlat::plat_add_position( int entry, int list, float x, float y, float z )
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
	
int CPs2ParticleFlat::plat_get_num_particle_colors( void ) { return 1; }
int CPs2ParticleFlat::plat_get_num_vertex_lists( void ) { return 1; }
void CPs2ParticleFlat::plat_set_sr( int entry, uint8 value ) { m_start_color.r = value; }
void CPs2ParticleFlat::plat_set_sg( int entry, uint8 value ) { m_start_color.g = value; }
void CPs2ParticleFlat::plat_set_sb( int entry, uint8 value ) { m_start_color.b = value; }
void CPs2ParticleFlat::plat_set_sa( int entry, uint8 value ) { m_start_color.a = value >> 1; }
void CPs2ParticleFlat::plat_set_mr( int entry, uint8 value ) { m_mid_color.r = value; }
void CPs2ParticleFlat::plat_set_mg( int entry, uint8 value ) { m_mid_color.g = value; }
void CPs2ParticleFlat::plat_set_mb( int entry, uint8 value ) { m_mid_color.b = value; }
void CPs2ParticleFlat::plat_set_ma( int entry, uint8 value ) { m_mid_color.a = value >> 1; }
void CPs2ParticleFlat::plat_set_er( int entry, uint8 value ) { m_end_color.r = value; }
void CPs2ParticleFlat::plat_set_eg( int entry, uint8 value ) { m_end_color.g = value; }
void CPs2ParticleFlat::plat_set_eb( int entry, uint8 value ) { m_end_color.b = value; }
void CPs2ParticleFlat::plat_set_ea( int entry, uint8 value ) { m_end_color.a = value >> 1; }
		
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
#if 0

void CPs2ParticleFlat::plat_render( void )
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
	NxPs2::CImmediateMode::sStartPolyDraw( mp_engine_texture, m_blend );

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
	
		tmp[0]		= pos - ss_right + ss_up;
		tmp[1]		= pos + ss_right + ss_up;		
		tmp[2]		= pos + ss_right - ss_up;		
		tmp[3]		= pos - ss_right - ss_up;		
	
		Image::RGBA color;
		Image::RGBA *p_col0;
		Image::RGBA *p_col1;
		
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

		Image::RGBA start = *p_col0;
		Image::RGBA end = *p_col1;

		color.r = start.r + (uint8)(( ((float)( end.r - start.r )) * terp ));
		color.g = start.g + (uint8)(( ((float)( end.g - start.g )) * terp ));
		color.b = start.b + (uint8)(( ((float)( end.b - start.b )) * terp ));
		color.a = start.a + (uint8)(( ((float)( end.a - start.a )) * terp ));
	
		NxPs2::CImmediateMode::sDrawQuadTexture( mp_engine_texture, tmp[0], tmp[1], tmp[2], tmp[3],
												 *((uint32 *) &color),
												 *((uint32 *) &color),
												 *((uint32 *) &color),
												 *((uint32 *) &color));
	}

	NxPs2::dma::EndTag();
}

#else

void CPs2ParticleFlat::plat_render( void )
{
	if (m_num_particles <= 0)
		return;

	Dbg_MsgAssert(mp_engine_texture, ("no support for non-textured sprites yet..."));

	int i,j=0;
	CParticleEntry *p_particle;
	float *p_v;
	float *p_xyzr=NULL;
	uint32 *p_rgba=NULL;

	// add a dma packet
	NxPs2::dma::BeginTag(NxPs2::dma::cnt, 0);

	// VU context
	NxPs2::vu1::BeginPrim(ABS, VU1_ADDR(L_VF10));
	NxPs2::vu1::StoreVec(*(NxPs2::Vec *)&NxPs2::render::InverseIntViewportScale);
	NxPs2::vu1::StoreVec(*(NxPs2::Vec *)&NxPs2::render::InverseIntViewportOffset);
	NxPs2::vu1::StoreMat(*(NxPs2::Mat *)&NxPs2::render::WorldToIntViewport);	// VF12-15
	NxPs2::vu1::EndPrim(0);
	NxPs2::vu1::BeginPrim(ABS, VU1_ADDR(L_VF30));
	NxPs2::vif::StoreV4_32F(640.0f, 480.0f, 0.0f, 0.0f);						// VF30
	NxPs2::vif::StoreV4_32F(NxPs2::render::IntViewportScale[0]/NxPs2::render::Tx,	// VF31
							NxPs2::render::IntViewportScale[1]/NxPs2::render::Ty,
							0.0f, 0.0f);
	NxPs2::vu1::EndPrim(0);

	// GS context
	NxPs2::gs::BeginPrim(ABS, 0, 0);
	NxPs2::gs::Reg1(NxPs2::gs::ALPHA_1,	m_blend);
	NxPs2::gs::Reg1(NxPs2::gs::TEX0_1,	mp_engine_texture->m_RegTEX0);
	NxPs2::gs::Reg1(NxPs2::gs::TEX1_1,	mp_engine_texture->m_RegTEX1);
	NxPs2::gs::Reg1(NxPs2::gs::ST,		PackST(0x3F800000,0x3F800000));
	NxPs2::gs::Reg1(NxPs2::gs::RGBAQ,	PackRGBAQ(0,0,0,0,0x3F800000));
	NxPs2::gs::EndPrim(0);

	for (i=0, p_particle=mp_particle_array, p_v=mp_vertices; i<m_num_particles; i++, j--, p_particle++, p_v+=3)
	{
		if (i==0 || j==0)
		{
			j = m_num_particles - i;
			if (j > 80)
				j = 80;
				
			NxPs2::BeginModelPrimImmediate(NxPs2::gs::XYZ2		|
										   NxPs2::gs::ST<<4		|
										   NxPs2::gs::RGBAQ<<8	|
										   NxPs2::gs::XYZ2<<12,
										   4, SPRITE|ABE|TME, 1, VU1_ADDR(SpriteCull));

			// create an unpack for the colours
			NxPs2::vif::BeginUNPACK(0, V4_8, ABS, UNSIGNED, 3);
			p_rgba = (uint32 *)NxPs2::dma::pLoc;
			NxPs2::dma::pLoc += j * 4;
			NxPs2::vif::EndUNPACK();

			// and one for the positions (& sizes)
			NxPs2::vif::BeginUNPACK(0, V4_32, ABS, SIGNED, 4);
			p_xyzr = (float *)NxPs2::dma::pLoc;
			NxPs2::dma::pLoc += j * 16;
			NxPs2::vif::EndUNPACK();

			NxPs2::EndModelPrimImmediate(1);

			NxPs2::vif::MSCAL(VU1_ADDR(Parser));
		}

		float t = p_particle->m_time / p_particle->m_life;

		// just use the width for now...
		float width = p_particle->m_sw + ( ( p_particle->m_ew - p_particle->m_sw ) * t );
		
		// store position
		*p_xyzr++ = p_v[0]+m_pos[0];
		*p_xyzr++ = p_v[1]+m_pos[1];
		*p_xyzr++ = p_v[2]+m_pos[2];
		*p_xyzr++ = width;

		Image::RGBA start, end, colour;

		if ( m_mid_time >= 0.0f )
		{
			if ( t < m_mid_time )
			{
				start = m_start_color;
				end   = m_mid_color;
				// Adjust interpolation for this half of the color blend.
				t /= m_mid_time;
			}
			else
			{
				start = m_mid_color;
				end   = m_end_color;
				// Adjust interpolation for this half of the color blend.
				t = ( t - m_mid_time ) / ( 1.0f - m_mid_time );
			}
		}
		else
		{
			// No mid color specified.
			start = m_start_color;
			end   = m_end_color;
		}

		// compute interpolated colour
		colour.r = start.r + (uint8) (int) ( ((float)( end.r - start.r )) * t );
		colour.g = start.g + (uint8) (int) ( ((float)( end.g - start.g )) * t );
		colour.b = start.b + (uint8) (int) ( ((float)( end.b - start.b )) * t );
		colour.a = start.a + (uint8) (int) ( ((float)( end.a - start.a )) * t );

		// store colour
		*p_rgba++ = *(uint32 *)&colour;
	}

	// restore transform
	NxPs2::vu1::BeginPrim(ABS, VU1_ADDR(L_VF12));		// was L_VF16
	NxPs2::vu1::StoreMat(*(NxPs2::Mat *)&NxPs2::render::AdjustedWorldToViewport);		// VF16-19
	NxPs2::vu1::EndPrim(1);
	NxPs2::vif::MSCAL(VU1_ADDR(Parser));

	// end the dma tag
	NxPs2::dma::EndTag();

}

#endif

} // Nx

