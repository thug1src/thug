#include <core/defines.h>
#include <gfx/xbox/nx/render.h>
#include "gfx/xbox/p_NxParticleShaded.h"

extern DWORD PixelShader0;

namespace Nx
{


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxParticleShaded::CXboxParticleShaded()
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxParticleShaded::CXboxParticleShaded( uint32 checksum, int max_particles, uint32 texture_checksum, uint32 blendmode_checksum, int fix )
{
	m_checksum		= checksum;
	m_max_particles = max_particles;
	m_num_particles = 0;
	m_mid_time		= -1.0f;

	mp_particle_array = new CParticleEntry[max_particles];

	// Allocate vertex buffer.
	mp_vertices		= new float[max_particles * 3];

	// Create the engine representation.
	mp_engine_particle = new NxXbox::sParticleSystem( max_particles, NxXbox::PARTICLE_TYPE_SHADED, texture_checksum, blendmode_checksum, fix );

	// Default color.
	for( int c = 0; c < 4; ++c )
	{
		m_start_color[c].r = 128;
		m_start_color[c].g = 128;
		m_start_color[c].b = 128;
		m_start_color[c].a = 255;

		m_mid_color[c].r = 128;
		m_mid_color[c].g = 128;
		m_mid_color[c].b = 128;
		m_mid_color[c].a = 255;

		m_end_color[c].r = 128;
		m_end_color[c].g = 128;
		m_end_color[c].b = 128;
		m_end_color[c].a = 255;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxParticleShaded::~CXboxParticleShaded()
{
	delete [] mp_particle_array;
	delete [] mp_vertices;
	delete mp_engine_particle;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxParticleShaded::plat_get_position( int entry, int list, float * x, float * y, float * z )
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
void CXboxParticleShaded::plat_set_position( int entry, int list, float x, float y, float z )
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
void CXboxParticleShaded::plat_add_position( int entry, int list, float x, float y, float z )
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
int CXboxParticleShaded::plat_get_num_particle_colors( void )
{
	return 4;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxParticleShaded::plat_set_sr( int entry, uint8 value ) { m_start_color[entry].r = value; }
void CXboxParticleShaded::plat_set_sg( int entry, uint8 value ) { m_start_color[entry].g = value; }
void CXboxParticleShaded::plat_set_sb( int entry, uint8 value ) { m_start_color[entry].b = value; }
void CXboxParticleShaded::plat_set_sa( int entry, uint8 value ) { m_start_color[entry].a = value; }
void CXboxParticleShaded::plat_set_mr( int entry, uint8 value ) { m_mid_color[entry].r = value; }
void CXboxParticleShaded::plat_set_mg( int entry, uint8 value ) { m_mid_color[entry].g = value; }
void CXboxParticleShaded::plat_set_mb( int entry, uint8 value ) { m_mid_color[entry].b = value; }
void CXboxParticleShaded::plat_set_ma( int entry, uint8 value ) { m_mid_color[entry].a = value; }
void CXboxParticleShaded::plat_set_er( int entry, uint8 value ) { m_end_color[entry].r = value; }
void CXboxParticleShaded::plat_set_eg( int entry, uint8 value ) { m_end_color[entry].g = value; }
void CXboxParticleShaded::plat_set_eb( int entry, uint8 value ) { m_end_color[entry].b = value; }
void CXboxParticleShaded::plat_set_ea( int entry, uint8 value ) { m_end_color[entry].a = value; }



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxParticleShaded::plat_render( void )
{
	// Draw the particles.
	if( m_num_particles > 0 )
	{
		// Used to figure the right and up vectors for creating screen-aligned particle quads.
		D3DXMATRIX *p_matrix = (D3DXMATRIX*)&NxXbox::EngineGlobals.view_matrix;

		// Concatenate p_matrix with the emmission angle to create the direction.
		Mth::Vector up( 0.0f, 1.0f, 0.0f );

		// Get the 'right' vector as the cross product of camera 'at and world 'up'.
		Mth::Vector at( p_matrix->m[0][2], p_matrix->m[1][2], p_matrix->m[2][2] );
		Mth::Vector screen_right	= Mth::CrossProduct( at, up );
		Mth::Vector screen_up		= Mth::CrossProduct( screen_right, at );

		screen_right.Normalize();
		screen_up.Normalize();

		int				lp;
		CParticleEntry	*p_particle;
		float			*p_v;
//		Mth::Vector		min, max;	// For dynamic bounding box calculation.
	
		// Obtain push buffer lock.
		DWORD *p_push; 
		DWORD dwords_per_particle	= 24;
		DWORD dword_count			= dwords_per_particle * m_num_particles;

		// Submit particle material.
		mp_engine_particle->mp_material->Submit();
		
		// Set up correct vertex and pixel shader.
		NxXbox::set_vertex_shader( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2( 0 ));
		NxXbox::set_pixel_shader( PixelShader0 );
		
		// The additional number (+5 is minimum) is to reserve enough overhead for the encoding parameters. It can safely be more, but no less.
		p_push = D3DDevice_BeginPush( dword_count + 16 );

		// Note that p_push is returned as a pointer to write-combined memory. Writes to write-combined memory should be
		// consecutive and in increasing order. Reads should be avoided. Additionally, any CPU reads from memory or the
		// L2 cache can force expensive partial flushes of the 32-byte write-combine cache.
		p_push[0]	= D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
		p_push[1]	= D3DPT_QUADLIST;
		p_push		+= 2;

		// Set up loop variables here, since we be potentially enetering the loop more than once.
		lp			= 0;
		p_particle	= mp_particle_array;
		p_v			= mp_vertices;

		while( dword_count > 0 )
		{
			int dwords_written = 0;

			// NOTE: A maximum of 2047 DWORDs can be specified to D3DPUSH_ENCODE. If there is more than 2047 DWORDs of vertex
			// data, simply split the data into multiple D3DPUSH_ENCODE( D3DPUSH_INLINE_ARRAY ) sections.
			p_push[0] = D3DPUSH_ENCODE( D3DPUSH_NOINCREMENT_FLAG | D3DPUSH_INLINE_ARRAY, ( dword_count > 2047 ) ? ((int)( 2047 / dwords_per_particle )) * dwords_per_particle: dword_count );
			++p_push;
		
			for( ; lp < m_num_particles; lp++, p_particle++, p_v += 3 )
			{
				// Check to see if writing another particle will take us over the edge.
				if(( dwords_written + dwords_per_particle ) > 2047 )
				{
					break;
				}

				float terp	= p_particle->m_time / p_particle->m_life;
				float w		= p_particle->m_sw + ( ( p_particle->m_ew - p_particle->m_sw ) * terp );
				float h		= p_particle->m_sh + ( ( p_particle->m_eh - p_particle->m_sh ) * terp );

				// Todo: Move hook to matrix/emitter code to cut down on per particle calculation.
				Mth::Vector	pos( p_v[0] + m_pos[X], p_v[1] + m_pos[Y], p_v[2] + m_pos[Z] );
				Mth::Vector	ss_right, ss_up, ss_pos;
				Mth::Vector tmp;
	
				// Dynamic bounding box calculation.
//				if( lp == 0 )
//				{
//					min = pos;
//					max = pos;
//				}
//				else
//				{
//					if( pos[X] < min[X] ) min[X] = pos[X]; else if( pos[X] > max[X] ) max[X] = pos[X];
//					if( pos[Y] < min[Y] ) min[Y] = pos[Y]; else if( pos[Y] > max[Y] ) max[Y] = pos[Y];
//					if( pos[Z] < min[Z] ) min[Z] = pos[Z]; else if( pos[Z] > max[Z] ) max[Z] = pos[Z];
//				}
		
				ss_right	= screen_right * w;
				ss_up		= screen_up * h;

				Image::RGBA color[4];
				Image::RGBA *p_col0;
				Image::RGBA *p_col1;
		
				if( m_mid_time >= 0.0f )
				{
					if( terp < m_mid_time )
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

				for ( int c = 0; c < 4; c++ )
				{
					Image::RGBA start = *p_col0++;
					Image::RGBA end = *p_col1++;

					// Swap red and blue here.
					color[c].b = start.r + (uint8)(( ((float)( end.r - start.r )) * terp ));
					color[c].g = start.g + (uint8)(( ((float)( end.g - start.g )) * terp ));
					color[c].r = start.b + (uint8)(( ((float)( end.b - start.b )) * terp ));
					color[c].a = start.a + (uint8)(( ((float)( end.a - start.a )) * terp ));
				}
		
				tmp			= pos - ss_right + ss_up;
				p_push[0]	= *((DWORD*)&tmp[X] );
				p_push[1]	= *((DWORD*)&tmp[Y] );
				p_push[2]	= *((DWORD*)&tmp[Z] );
				p_push[3]	= *((DWORD*)&color[0] );
				p_push[4]	= 0x00000000UL;
				p_push[5]	= 0x00000000UL;
				p_push		+= 6;
	
				tmp			= pos + ss_right + ss_up;		
				p_push[0]	= *((DWORD*)&tmp[X] );
				p_push[1]	= *((DWORD*)&tmp[Y] );
				p_push[2]	= *((DWORD*)&tmp[Z] );
				p_push[3]	= *((DWORD*)&color[1] );
				p_push[4]	= 0x3F800000UL;
				p_push[5]	= 0x00000000UL;
				p_push		+= 6;

				tmp			= pos + ss_right - ss_up;		
				p_push[0]	= *((DWORD*)&tmp[X] );
				p_push[1]	= *((DWORD*)&tmp[Y] );
				p_push[2]	= *((DWORD*)&tmp[Z] );
				p_push[3]	= *((DWORD*)&color[2] );
				p_push[4]	= 0x3F800000UL;
				p_push[5]	= 0x3F800000UL;
				p_push		+= 6;
		
				tmp			= pos - ss_right - ss_up;		
				p_push[0]	= *((DWORD*)&tmp[X] );
				p_push[1]	= *((DWORD*)&tmp[Y] );
				p_push[2]	= *((DWORD*)&tmp[Z] );
				p_push[3]	= *((DWORD*)&color[3] );
				p_push[4]	= 0x00000000UL;
				p_push[5]	= 0x3F800000UL;
				p_push		+= 6;

				dwords_written	+= dwords_per_particle;
				dword_count		-= dwords_per_particle;
			}
		}

		p_push[0] = D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
		p_push[1] = 0;
		p_push += 2;
		D3DDevice_EndPush( p_push );

		// Set the mesh bounding box and sphere.
//		mp_engine_particle->mp_scene->m_semitransparent_meshes[0]->m_bbox.SetMin( min );
//		mp_engine_particle->mp_scene->m_semitransparent_meshes[0]->m_bbox.SetMax( max );
//		mp_engine_particle->mp_scene->m_semitransparent_meshes[0]->m_sphere_center = D3DXVECTOR3( min[X] + (( max[X] - min[X] ) * 0.5f ), min[Y] + (( max[Y] - min[Y] ) * 0.5f ), min[Z] + (( max[Z] - min[Z] ) * 0.5f ));
//		mp_engine_particle->mp_scene->m_semitransparent_meshes[0]->m_sphere_radius = 360.0f;

		// And the scene bounding sphere.
//		mp_engine_particle->mp_scene->m_sphere_center = mp_engine_particle->mp_scene->m_semitransparent_meshes[0]->m_sphere_center;
//		mp_engine_particle->mp_scene->m_sphere_radius = mp_engine_particle->mp_scene->m_semitransparent_meshes[0]->m_sphere_radius;
	}
}

} // Nx

				


