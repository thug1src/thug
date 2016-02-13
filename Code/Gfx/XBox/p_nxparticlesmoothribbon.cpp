#include <core/defines.h>
#include "gfx/xbox/nx/render.h"

#include "gfx/xbox/p_nxparticleSmoothRibbon.h"

extern DWORD PixelShader1;

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxParticleSmoothRibbon::CXboxParticleSmoothRibbon()
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxParticleSmoothRibbon::CXboxParticleSmoothRibbon( uint32 checksum, int max_particles, uint32 texture_checksum, uint32 blendmode_checksum, int fix, int num_segments, float split, int history )
{
	m_checksum		= checksum;
	m_max_particles = max_particles;
	m_num_particles = 0;
	m_mid_time		= -1.0f;

	mp_particle_array = new CParticleEntry[max_particles];

	// Allocate vertex buffer.
	mp_vertices = new float*[( history + 1)];
	for ( int lp = 0; lp < ( history + 1 ); lp++ )
	{
		mp_vertices[lp] = new float[max_particles * 3];
	}
	m_num_vertex_buffers = history + 1;

	// Create the engine representation.
	mp_engine_particle = new NxXbox::sParticleSystem( max_particles, NxXbox::PARTICLE_TYPE_SMOOTHRIBBON, texture_checksum, blendmode_checksum, fix );

	// Default color.
	for ( int lp = 0; lp < 4; lp++ )
	{
		m_start_color[lp].r = 128;
		m_start_color[lp].g = 128;
		m_start_color[lp].b = 128;
		m_start_color[lp].a = 255;
		m_mid_color[lp].r = 128;
		m_mid_color[lp].g = 128;
		m_mid_color[lp].b = 128;
		m_mid_color[lp].a = 255;
		m_end_color[lp].r = 128;
		m_end_color[lp].g = 128;
		m_end_color[lp].b = 128;
		m_end_color[lp].a = 255;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxParticleSmoothRibbon::~CXboxParticleSmoothRibbon()
{
	delete [] mp_particle_array;
	for( int lp = 0; lp < m_num_vertex_buffers; lp++ )
	{
		delete [] mp_vertices[lp];
	}
	delete [] mp_vertices;
	delete mp_engine_particle;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxParticleSmoothRibbon::plat_get_position( int entry, int list, float * x, float * y, float * z )
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
void CXboxParticleSmoothRibbon::plat_set_position( int entry, int list, float x, float y, float z )
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
void CXboxParticleSmoothRibbon::plat_add_position( int entry, int list, float x, float y, float z )
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
int CXboxParticleSmoothRibbon::plat_get_num_particle_colors( void ) { return 4; }
int CXboxParticleSmoothRibbon::plat_get_num_vertex_lists( void ) { return m_num_vertex_buffers; }
void CXboxParticleSmoothRibbon::plat_set_sr( int entry, uint8 value ) { m_start_color[entry].r = value; }
void CXboxParticleSmoothRibbon::plat_set_sg( int entry, uint8 value ) { m_start_color[entry].g = value; }
void CXboxParticleSmoothRibbon::plat_set_sb( int entry, uint8 value ) { m_start_color[entry].b = value; }
void CXboxParticleSmoothRibbon::plat_set_sa( int entry, uint8 value ) { m_start_color[entry].a = value; }
void CXboxParticleSmoothRibbon::plat_set_mr( int entry, uint8 value ) { m_mid_color[entry].r = value; }
void CXboxParticleSmoothRibbon::plat_set_mg( int entry, uint8 value ) { m_mid_color[entry].g = value; }
void CXboxParticleSmoothRibbon::plat_set_mb( int entry, uint8 value ) { m_mid_color[entry].b = value; }
void CXboxParticleSmoothRibbon::plat_set_ma( int entry, uint8 value ) { m_mid_color[entry].a = value; }
void CXboxParticleSmoothRibbon::plat_set_er( int entry, uint8 value ) { m_end_color[entry].r = value; }
void CXboxParticleSmoothRibbon::plat_set_eg( int entry, uint8 value ) { m_end_color[entry].g = value; }
void CXboxParticleSmoothRibbon::plat_set_eb( int entry, uint8 value ) { m_end_color[entry].b = value; }
void CXboxParticleSmoothRibbon::plat_set_ea( int entry, uint8 value ) { m_end_color[entry].a = value; }



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxParticleSmoothRibbon::plat_render( void )
{
	// Draw the particles.
	if( m_num_particles > 0 )
	{
		int				lp;
		CParticleEntry	*p_particle;
		float			*p_v0;
		float			*p_v1;
		Mth::Vector		min, max;	// For dynamic bounding box calculation.

		// Used to figure the right and up vectors for creating screen-aligned particle quads.
		D3DXMATRIX *p_matrix = (D3DXMATRIX*)&NxXbox::EngineGlobals.view_matrix;

		// Get the 'right' vector as the cross product of camera 'at and world 'up'.
		Mth::Vector at( p_matrix->m[0][2], p_matrix->m[1][2], p_matrix->m[2][2] );

		Image::RGBA color[4];
		Image::RGBA *p_col0;
		Image::RGBA *p_col1;
		
		// Obtain push buffer lock.
		DWORD *p_push; 
		DWORD dwords_per_particle	= 32;
		DWORD dword_count			= dwords_per_particle * m_num_particles;

		// Submit particle material.
		mp_engine_particle->mp_material->Submit();
		
		// Set up correct vertex and pixel shader.
		NxXbox::set_vertex_shader( D3DFVF_XYZ | D3DFVF_DIFFUSE );
		NxXbox::set_pixel_shader( PixelShader1 );
		
		// The additional number (+5 is minimum) is to reserve enough overhead for the encoding parameters. It can safely be more, but no less.
		p_push = D3DDevice_BeginPush( dword_count + 32 );

		// Note that p_push is returned as a pointer to write-combined memory. Writes to write-combined memory should be
		// consecutive and in increasing order. Reads should be avoided. Additionally, any CPU reads from memory or the
		// L2 cache can force expensive partial flushes of the 32-byte write-combine cache.
		p_push[0]	= D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
		p_push[1]	= D3DPT_QUADLIST;
		p_push		+= 2;

		// Set up loop variables here, since we be potentially enetering the loop more than once.
		lp			= 0;
		p_particle	= mp_particle_array;
		p_v0		= mp_vertices[0];
		p_v1		= mp_vertices[(m_num_vertex_buffers - 1)];

		while( dword_count > 0 )
		{
			int dwords_written = 0;

			// NOTE: A maximum of 2047 DWORDs can be specified to D3DPUSH_ENCODE. If there is more than 2047 DWORDs of vertex
			// data, simply split the data into multiple D3DPUSH_ENCODE( D3DPUSH_INLINE_ARRAY ) sections.
			p_push[0] = D3DPUSH_ENCODE( D3DPUSH_NOINCREMENT_FLAG | D3DPUSH_INLINE_ARRAY, ( dword_count > 2047 ) ? ((int)( 2047 / dwords_per_particle )) * dwords_per_particle: dword_count );
			++p_push;
		
			for( ; lp < m_num_particles; lp++, p_particle++, p_v0 += 3, p_v1 += 3 )
			{
				// Check to see if writing another particle will take us over the edge.
				if(( dwords_written + dwords_per_particle ) > 2047 )
				{
					break;
				}

				float terp = p_particle->m_time / p_particle->m_life;

				Mth::Vector	pos0( p_v0[0] + m_pos[X], p_v0[1] + m_pos[Y], p_v0[2] + m_pos[Z] );
				Mth::Vector	pos1( p_v1[0] + m_pos[X], p_v1[1] + m_pos[Y], p_v1[2] + m_pos[Z] );

				// Dynamic bounding box calculation.
//				if( lp == 0 )
//				{
//					min = pos0;
//					max = pos0;
//				}
//				else
//				{
//					if( pos0[X] < min[X] ) min[X] = pos0[X]; else if( pos0[X] > max[X] ) max[X] = pos0[X];
//					if( pos0[Y] < min[Y] ) min[Y] = pos0[Y]; else if( pos0[Y] > max[Y] ) max[Y] = pos0[Y];
//					if( pos0[Z] < min[Z] ) min[Z] = pos0[Z]; else if( pos0[Z] > max[Z] ) max[Z] = pos0[Z];
//				}

				Mth::Vector	part_vec = pos1 - pos0;
				Mth::Vector perp_vec = Mth::CrossProduct( part_vec, at );
				perp_vec.Normalize();

				float w = p_particle->m_sw + ( ( p_particle->m_ew - p_particle->m_sw ) * terp );
		
				Mth::Vector tmp[4];
				tmp[0]		= pos0 + ( perp_vec * w );
				tmp[1]		= pos0 - ( perp_vec * w );
				tmp[2]		= pos1 - ( perp_vec * w );
				tmp[3]		= pos1 + ( perp_vec * w );
	
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

				for( int c = 0; c < 4; c++ )
				{
					Image::RGBA start = *p_col0++;
					Image::RGBA end = *p_col1++;

					// Swap red and blue here.
					color[c].b = start.r + (uint8)(( ((float)( end.r - start.r )) * terp ));
					color[c].g = start.g + (uint8)(( ((float)( end.g - start.g )) * terp ));
					color[c].r = start.b + (uint8)(( ((float)( end.b - start.b )) * terp ));
					color[c].a = start.a + (uint8)(( ((float)( end.a - start.a )) * terp ));
				}

				// First quad.
				p_push[0]	= *((DWORD*)&pos0[X] );
				p_push[1]	= *((DWORD*)&pos0[Y] );
				p_push[2]	= *((DWORD*)&pos0[Z] );
				p_push[3]	= *((DWORD*)&color[0] );
				p_push		+= 4;

				p_push[0]	= *((DWORD*)&pos1[X] );
				p_push[1]	= *((DWORD*)&pos1[Y] );
				p_push[2]	= *((DWORD*)&pos1[Z] );
				p_push[3]	= *((DWORD*)&color[3] );
				p_push		+= 4;
				
				p_push[0]	= *((DWORD*)&tmp[3][X] );
				p_push[1]	= *((DWORD*)&tmp[3][Y] );
				p_push[2]	= *((DWORD*)&tmp[3][Z] );
				p_push[3]	= *((DWORD*)&color[2] );
				p_push		+= 4;
				
				p_push[0]	= *((DWORD*)&tmp[0][X] );
				p_push[1]	= *((DWORD*)&tmp[0][Y] );
				p_push[2]	= *((DWORD*)&tmp[0][Z] );
				p_push[3]	= *((DWORD*)&color[1] );
				p_push		+= 4;
				
				// Second quad.
				p_push[0]	= *((DWORD*)&pos0[X] );
				p_push[1]	= *((DWORD*)&pos0[Y] );
				p_push[2]	= *((DWORD*)&pos0[Z] );
				p_push[3]	= *((DWORD*)&color[0] );
				p_push		+= 4;

				p_push[0]	= *((DWORD*)&pos1[X] );
				p_push[1]	= *((DWORD*)&pos1[Y] );
				p_push[2]	= *((DWORD*)&pos1[Z] );
				p_push[3]	= *((DWORD*)&color[3] );
				p_push		+= 4;
				
				p_push[0]	= *((DWORD*)&tmp[2][X] );
				p_push[1]	= *((DWORD*)&tmp[2][Y] );
				p_push[2]	= *((DWORD*)&tmp[2][Z] );
				p_push[3]	= *((DWORD*)&color[2] );
				p_push		+= 4;
				
				p_push[0]	= *((DWORD*)&tmp[1][X] );
				p_push[1]	= *((DWORD*)&tmp[1][Y] );
				p_push[2]	= *((DWORD*)&tmp[1][Z] );
				p_push[3]	= *((DWORD*)&color[1] );
				p_push		+= 4;

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


