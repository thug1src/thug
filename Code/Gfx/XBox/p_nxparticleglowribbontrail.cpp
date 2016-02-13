#include <core/defines.h>
#include "gfx/xbox/nx/render.h"

#include "gfx/xbox/p_nxparticleGlowRibbonTrail.h"

extern DWORD PixelShader1;

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CXboxParticleGlowRibbonTrail::CXboxParticleGlowRibbonTrail()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxParticleGlowRibbonTrail::CXboxParticleGlowRibbonTrail( uint32 checksum, int max_particles, uint32 texture_checksum, uint32 blendmode_checksum, int fix, int num_segments, float split, int history )
{
	m_checksum		= checksum;
	m_max_particles	= max_particles;
	m_num_particles	= 0;
	m_mid_time		= -1.0f;
	m_num_segments	= num_segments;
	m_segment_step	= 360.0f / m_num_segments;
	m_history		= history;
	m_split			= split;

	mp_particle_array = new CParticleEntry[max_particles];

	// Allocate vertex buffer.
	mp_vertices = new float*[( history + 1)];
	for( int lp = 0; lp < ( history + 1 ); lp++ )
	{
		mp_vertices[lp] = new float[max_particles * 3];
	}
	m_num_vertex_buffers = history + 1;

	// Create the engine representation.
	mp_engine_particle = new NxXbox::sParticleSystem( max_particles, NxXbox::PARTICLE_TYPE_GLOWRIBBONTRAIL, texture_checksum, blendmode_checksum, fix, num_segments, history );

	// Default color.
	m_start_color = new Image::RGBA[m_num_vertex_buffers + 3];
	m_mid_color = new Image::RGBA[m_num_vertex_buffers + 3];
	m_end_color = new Image::RGBA[m_num_vertex_buffers + 3];
	for ( int lp = 0; lp < ( m_num_vertex_buffers + 3 ); lp++ )
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
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxParticleGlowRibbonTrail::~CXboxParticleGlowRibbonTrail()
{
	delete [] mp_particle_array;
	for ( int lp = 0; lp < m_num_vertex_buffers; lp++ )
	{
		delete [] mp_vertices[lp];
	}
	delete [] mp_vertices;
	delete [] m_start_color;
	delete [] m_mid_color;
	delete [] m_end_color;
	delete mp_engine_particle;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxParticleGlowRibbonTrail::plat_get_position( int entry, int list, float * x, float * y, float * z )
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
void CXboxParticleGlowRibbonTrail::plat_set_position( int entry, int list, float x, float y, float z )
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
void CXboxParticleGlowRibbonTrail::plat_add_position( int entry, int list, float x, float y, float z )
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
int CXboxParticleGlowRibbonTrail::plat_get_num_particle_colors( void ) { return m_num_vertex_buffers + 3; }
int CXboxParticleGlowRibbonTrail::plat_get_num_vertex_lists( void ) { return m_num_vertex_buffers; }
void CXboxParticleGlowRibbonTrail::plat_set_sr( int entry, uint8 value ) { m_start_color[entry].r = value; }
void CXboxParticleGlowRibbonTrail::plat_set_sg( int entry, uint8 value ) { m_start_color[entry].g = value; }
void CXboxParticleGlowRibbonTrail::plat_set_sb( int entry, uint8 value ) { m_start_color[entry].b = value; }
void CXboxParticleGlowRibbonTrail::plat_set_sa( int entry, uint8 value ) { m_start_color[entry].a = value >> 1; }
void CXboxParticleGlowRibbonTrail::plat_set_mr( int entry, uint8 value ) { m_mid_color[entry].r = value; }
void CXboxParticleGlowRibbonTrail::plat_set_mg( int entry, uint8 value ) { m_mid_color[entry].g = value; }
void CXboxParticleGlowRibbonTrail::plat_set_mb( int entry, uint8 value ) { m_mid_color[entry].b = value; }
void CXboxParticleGlowRibbonTrail::plat_set_ma( int entry, uint8 value ) { m_mid_color[entry].a = value >> 1; }
void CXboxParticleGlowRibbonTrail::plat_set_er( int entry, uint8 value ) { m_end_color[entry].r = value; }
void CXboxParticleGlowRibbonTrail::plat_set_eg( int entry, uint8 value ) { m_end_color[entry].g = value; }
void CXboxParticleGlowRibbonTrail::plat_set_eb( int entry, uint8 value ) { m_end_color[entry].b = value; }
void CXboxParticleGlowRibbonTrail::plat_set_ea( int entry, uint8 value ) { m_end_color[entry].a = value >> 1; }



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxParticleGlowRibbonTrail::plat_render( void )
{
	// Draw the particles.
	if( m_num_particles > 0 )
	{
		int				lp;
		CParticleEntry	*p_particle;
		float			*p_v0, *p_v1;

		// Used to figure the right and up vectors for creating screen-aligned particle quads.
		D3DXMATRIX *p_matrix = (D3DXMATRIX*)&NxXbox::EngineGlobals.view_matrix;

		// Concatenate p_matrix with the emmission angle to create the direction.
		Mth::Vector up( 0.0f, 1.0f, 0.0f, 0.0f );

		// Get the 'right' vector as the cross product of camera 'at and world 'up'.
		Mth::Vector at( p_matrix->m[0][2], p_matrix->m[1][2], p_matrix->m[2][2] );
		Mth::Vector screen_right	= Mth::CrossProduct( at, up );
		Mth::Vector screen_up		= Mth::CrossProduct( screen_right, at );

		Image::RGBA color[3];
		Image::RGBA *p_col0;
		Image::RGBA *p_col1;
		
		// Obtain push buffer lock.
		DWORD *p_push; 
		DWORD dwords_per_particle	= ( 36 * m_num_segments ) + ( 24 * ( m_num_vertex_buffers - 1 ));
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
		p_push[1]	= D3DPT_TRIANGLELIST;
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
				
				// Calculate the interpolator ( 1.0f / particle_life ).
				float terp = p_particle->m_time * ReciprocalEstimateNR_ASM( p_particle->m_life );

				Mth::Vector	pos[2];
				p_v0 = &mp_vertices[0][lp*3];
				pos[0].Set( p_v0[0] + m_pos[X], p_v0[1] + m_pos[Y], p_v0[2] + m_pos[Z] );
				p_v0 = &mp_vertices[1][lp*3];
				pos[1].Set( p_v0[0] + m_pos[X], p_v0[1] + m_pos[Y], p_v0[2] + m_pos[Z] );
			
				Mth::Vector	part_vec = pos[1] - pos[0];
				Mth::Vector perp_vec = Mth::CrossProduct( part_vec, at );
				perp_vec.Normalize();

				float w = p_particle->m_sw + (( p_particle->m_ew - p_particle->m_sw ) * terp );
				float h = p_particle->m_sh + (( p_particle->m_eh - p_particle->m_sh ) * terp );
		
				Mth::Vector tmp[4];

				if( m_mid_time >= 0.0f )
				{
					if( terp < m_mid_time )
					{
						p_col0 = &m_start_color[3];
						p_col1 = &m_mid_color[3];

						// Adjust interpolation for this half of the color blend.
						terp = terp / m_mid_time;
					}
					else
					{
						p_col0 = &m_mid_color[3];
						p_col1 = &m_end_color[3];

						// Adjust interpolation for this half of the color blend.
						terp = ( terp - m_mid_time ) / ( 1.0f - m_mid_time );
					}
				}
				else
				{
					// No mid color specified.
					p_col0 = &m_start_color[3];
					p_col1 = &m_end_color[3];
				}

				Image::RGBA start = *p_col0++;
				Image::RGBA end = *p_col1++;

				// Interpolate and swap red and blue here. Use fixed point math to avoid _ftol2 calls.
				int f_terp	= Ftoi_ASM( terp * 4096.0f );
				color[0].b	= ((((int)start.r ) * 4096 ) + (((int)end.r - (int)start.r ) * f_terp )) / 4096;
				color[0].g	= ((((int)start.g ) * 4096 ) + (((int)end.g - (int)start.g ) * f_terp )) / 4096;
				color[0].r	= ((((int)start.b ) * 4096 ) + (((int)end.b - (int)start.b ) * f_terp )) / 4096;
				color[0].a	= ((((int)start.a ) * 4096 ) + (((int)end.a - (int)start.a ) * f_terp )) / 4096;

				tmp[0]		= pos[0] + ( perp_vec * w * m_split );
				tmp[1]		= pos[0] - ( perp_vec * w * m_split );

				for( int c = 1; c < m_num_vertex_buffers; c++ )
				{
					start = *p_col0++;
					end = *p_col1++;

					// Interpolate and swap red and blue here. Use fixed point math to avoid _ftol2 calls.
					color[1].b	= ((((int)start.r ) * 4096 ) + (((int)end.r - (int)start.r ) * f_terp )) / 4096;
					color[1].g	= ((((int)start.g ) * 4096 ) + (((int)end.g - (int)start.g ) * f_terp )) / 4096;
					color[1].r	= ((((int)start.b ) * 4096 ) + (((int)end.b - (int)start.b ) * f_terp )) / 4096;
					color[1].a	= ((((int)start.a ) * 4096 ) + (((int)end.a - (int)start.a ) * f_terp )) / 4096;

					if ( c > 1 )
					{
						p_v0 = &mp_vertices[c][lp*3];
						pos[1].Set( p_v0[0] + m_pos[X], p_v0[1] + m_pos[Y], p_v0[2] + m_pos[Z] );
						part_vec = pos[1] - pos[0];
						perp_vec = Mth::CrossProduct( part_vec, at );
						perp_vec.Normalize();
					}

					tmp[2]		= pos[1] + ( perp_vec * w * m_split );
					tmp[3]		= pos[1] - ( perp_vec * w * m_split );

					// First tri.
					p_push[0]	= *((DWORD*)&tmp[0][X] );
					p_push[1]	= *((DWORD*)&tmp[0][Y] );
					p_push[2]	= *((DWORD*)&tmp[0][Z] );
					p_push[3]	= *((DWORD*)&color[0] );
					p_push		+= 4;
						
					p_push[0]	= *((DWORD*)&tmp[1][X] );
					p_push[1]	= *((DWORD*)&tmp[1][Y] );
					p_push[2]	= *((DWORD*)&tmp[1][Z] );
					p_push[3]	= *((DWORD*)&color[0] );
					p_push		+= 4;
					
					p_push[0]	= *((DWORD*)&tmp[2][X] );
					p_push[1]	= *((DWORD*)&tmp[2][Y] );
					p_push[2]	= *((DWORD*)&tmp[2][Z] );
					p_push[3]	= *((DWORD*)&color[1] );
					p_push		+= 4;

					// Second tri.
					p_push[0]	= *((DWORD*)&tmp[1][X] );
					p_push[1]	= *((DWORD*)&tmp[1][Y] );
					p_push[2]	= *((DWORD*)&tmp[1][Z] );
					p_push[3]	= *((DWORD*)&color[0] );
					p_push		+= 4;
						
					p_push[0]	= *((DWORD*)&tmp[3][X] );
					p_push[1]	= *((DWORD*)&tmp[3][Y] );
					p_push[2]	= *((DWORD*)&tmp[3][Z] );
					p_push[3]	= *((DWORD*)&color[1] );
					p_push		+= 4;
						
					p_push[0]	= *((DWORD*)&tmp[2][X] );
					p_push[1]	= *((DWORD*)&tmp[2][Y] );
					p_push[2]	= *((DWORD*)&tmp[2][Z] );
					p_push[3]	= *((DWORD*)&color[1] );
					p_push		+= 4;

					color[0] = color[1];
					pos[0] = pos[1];
					tmp[0] = tmp[2];
					tmp[1] = tmp[3];
				}

				// Now draw the glow.
				p_v0 = &mp_vertices[0][lp*3];
				pos[0].Set( p_v0[0] + m_pos[X], p_v0[1] + m_pos[Y], p_v0[2] + m_pos[Z] );
				Mth::Vector	ss_right, ss_up;
	
				ss_right	= screen_right * w;
				ss_up		= screen_up * h;

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
		
				// Swap red and blue here.
				for( int c = 0; c < 3; c++ )
				{
					Image::RGBA start = *p_col0++;
					Image::RGBA end = *p_col1++;

					// Interpolate and swap red and blue here. Use fixed point math to avoid _ftol2 calls.
					color[c].b	= ((((int)start.r ) * 4096 ) + (((int)end.r - (int)start.r ) * f_terp )) / 4096;
					color[c].g	= ((((int)start.g ) * 4096 ) + (((int)end.g - (int)start.g ) * f_terp )) / 4096;
					color[c].r	= ((((int)start.b ) * 4096 ) + (((int)end.b - (int)start.b ) * f_terp )) / 4096;
					color[c].a	= ((((int)start.a ) * 4096 ) + (((int)end.a - (int)start.a ) * f_terp )) / 4096;
				}

				// We know that sin( 0 ) = 0, and cos( 0 ) = 1, so we can optimise the first iteration.
				tmp[0]  = pos[0];
				tmp[0] += ss_right * 0.0f * m_split;
				tmp[0] += ss_up    * 1.0f * m_split;

				tmp[2]  = pos[0];
				tmp[2] += ss_right * 0.0f;
				tmp[2] += ss_up    * 1.0f;

				for ( int lp2 = 0; lp2 < m_num_segments; lp2++ )
				{
					float rt	= sinf( Mth::DegToRad( m_segment_step * ( lp2 + 1 )));
					float up	= cosf( Mth::DegToRad( m_segment_step * ( lp2 + 1 )));

					tmp[1]  = pos[0];
					tmp[1] += ss_right * rt * m_split;
					tmp[1] += ss_up    * up * m_split;

					tmp[3]  = pos[0];
					tmp[3] += ss_right * rt;
					tmp[3] += ss_up    * up;

					// First tri.
					p_push[0]	= *((DWORD*)&pos[0][X] );
					p_push[1]	= *((DWORD*)&pos[0][Y] );
					p_push[2]	= *((DWORD*)&pos[0][Z] );
					p_push[3]	= *((DWORD*)&color[0] );
					p_push		+= 4;
					
					p_push[0]	= *((DWORD*)&tmp[0][X] );
					p_push[1]	= *((DWORD*)&tmp[0][Y] );
					p_push[2]	= *((DWORD*)&tmp[0][Z] );
					p_push[3]	= *((DWORD*)&color[1] );
					p_push		+= 4;
					
					p_push[0]	= *((DWORD*)&tmp[1][X] );
					p_push[1]	= *((DWORD*)&tmp[1][Y] );
					p_push[2]	= *((DWORD*)&tmp[1][Z] );
					p_push[3]	= *((DWORD*)&color[1] );
					p_push		+= 4;

					// Second tri.					
					p_push[0]	= *((DWORD*)&tmp[0][X] );
					p_push[1]	= *((DWORD*)&tmp[0][Y] );
					p_push[2]	= *((DWORD*)&tmp[0][Z] );
					p_push[3]	= *((DWORD*)&color[1] );
					p_push		+= 4;

					p_push[0]	= *((DWORD*)&tmp[1][X] );
					p_push[1]	= *((DWORD*)&tmp[1][Y] );
					p_push[2]	= *((DWORD*)&tmp[1][Z] );
					p_push[3]	= *((DWORD*)&color[1] );
					p_push		+= 4;

					p_push[0]	= *((DWORD*)&tmp[2][X] );
					p_push[1]	= *((DWORD*)&tmp[2][Y] );
					p_push[2]	= *((DWORD*)&tmp[2][Z] );
					p_push[3]	= *((DWORD*)&color[2] );
					p_push		+= 4;

					// Third tri.
					p_push[0]	= *((DWORD*)&tmp[1][X] );
					p_push[1]	= *((DWORD*)&tmp[1][Y] );
					p_push[2]	= *((DWORD*)&tmp[1][Z] );
					p_push[3]	= *((DWORD*)&color[1] );
					p_push		+= 4;

					p_push[0]	= *((DWORD*)&tmp[2][X] );
					p_push[1]	= *((DWORD*)&tmp[2][Y] );
					p_push[2]	= *((DWORD*)&tmp[2][Z] );
					p_push[3]	= *((DWORD*)&color[2] );
					p_push		+= 4;

					p_push[0]	= *((DWORD*)&tmp[3][X] );
					p_push[1]	= *((DWORD*)&tmp[3][Y] );
					p_push[2]	= *((DWORD*)&tmp[3][Z] );
					p_push[3]	= *((DWORD*)&color[2] );
					p_push		+= 4;

					tmp[0] = tmp[1];
					tmp[2] = tmp[3];
				}
				dwords_written	+= dwords_per_particle;
				dword_count		-= dwords_per_particle;
			}
		}

		p_push[0] = D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
		p_push[1] = 0;
		p_push += 2;
		D3DDevice_EndPush( p_push );
	}
}

} // Nx


