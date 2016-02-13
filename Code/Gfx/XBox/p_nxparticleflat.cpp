#include <core/defines.h>
#include <gfx/xbox/nx/render.h>
#include "gfx/xbox/p_nxparticleflat.h"

extern DWORD PixelShader0;

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxParticleFlat::CXboxParticleFlat()
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxParticleFlat::CXboxParticleFlat( uint32 checksum, int max_particles, uint32 texture_checksum, uint32 blendmode_checksum, int fix, int num_segments, float split )
{
	m_checksum		= checksum;
	m_max_particles = max_particles;
	m_num_particles = 0;
	
	mp_particle_array = new CParticleEntry[max_particles];

	// Allocate vertex buffer.
	mp_vertices = new float[max_particles * 3];

	// Create the engine representation.
	mp_engine_particle = new NxXbox::sParticleSystem( max_particles, NxXbox::PARTICLE_TYPE_FLAT, texture_checksum, blendmode_checksum, fix );

	// Default color.
	m_start_color.r = m_start_color.g = m_start_color.b = 128;
	m_start_color.a = 255;
	m_mid_color.r = m_mid_color.g = m_mid_color.b = 128;
	m_mid_color.a = 255;
	m_end_color.r = m_end_color.g = m_end_color.b = 128;
	m_end_color.a = 255;

	m_mid_time = -1.0f;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxParticleFlat::~CXboxParticleFlat()
{
	delete [] mp_particle_array;
	delete [] mp_vertices;
	delete mp_engine_particle;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxParticleFlat::plat_get_position( int entry, int list, float * x, float * y, float * z )
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
void CXboxParticleFlat::plat_set_position( int entry, int list, float x, float y, float z )
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
void CXboxParticleFlat::plat_add_position( int entry, int list, float x, float y, float z )
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
int CXboxParticleFlat::plat_get_num_particle_colors( void ) { return 1; }
int CXboxParticleFlat::plat_get_num_vertex_lists( void ) { return 1; }

// Note these are r/b reversed for direct uploading to Xbox GPU.
void CXboxParticleFlat::plat_set_sr( int entry, uint8 value ) { m_start_color.b = value; }
void CXboxParticleFlat::plat_set_sg( int entry, uint8 value ) { m_start_color.g = value; }
void CXboxParticleFlat::plat_set_sb( int entry, uint8 value ) { m_start_color.r = value; }
void CXboxParticleFlat::plat_set_sa( int entry, uint8 value ) { m_start_color.a = value; }
void CXboxParticleFlat::plat_set_mr( int entry, uint8 value ) { m_mid_color.b = value; }
void CXboxParticleFlat::plat_set_mg( int entry, uint8 value ) { m_mid_color.g = value; }
void CXboxParticleFlat::plat_set_mb( int entry, uint8 value ) { m_mid_color.r = value; }
void CXboxParticleFlat::plat_set_ma( int entry, uint8 value ) { m_mid_color.a = value; }
void CXboxParticleFlat::plat_set_er( int entry, uint8 value ) { m_end_color.b = value; }
void CXboxParticleFlat::plat_set_eg( int entry, uint8 value ) { m_end_color.g = value; }
void CXboxParticleFlat::plat_set_eb( int entry, uint8 value ) { m_end_color.r = value; }
void CXboxParticleFlat::plat_set_ea( int entry, uint8 value ) { m_end_color.a = value; }


		



#if 1
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxParticleFlat::plat_render( void )
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

		// Calculate space needed.
		DWORD dwords_per_particle	= 32;
		DWORD dword_count			= dwords_per_particle * m_num_particles;

		// Submit particle material.
		mp_engine_particle->mp_material->Submit();
		
		// Set up correct vertex and pixel shader.
		NxXbox::set_vertex_shader( ParticleFlatVS );
		NxXbox::set_pixel_shader( PixelShader0 );
		
		// Load up the combined world->view_projection matrix.
		XGMATRIX	temp_matrix;
		XGMATRIX	dest_matrix;
		XGMATRIX	projMatrix;
		XGMATRIX	viewMatrix;
		XGMATRIX	worldMatrix;
		
		// Projection matrix.
		XGMatrixTranspose( &projMatrix, &NxXbox::EngineGlobals.projection_matrix );
	
		// View matrix.
		XGMatrixTranspose( &viewMatrix, &NxXbox::EngineGlobals.view_matrix );
		viewMatrix.m[3][0] = 0.0f;
		viewMatrix.m[3][1] = 0.0f;
		viewMatrix.m[3][2] = 0.0f;
		viewMatrix.m[3][3] = 1.0f;

		// World space transformation matrix, set to be a translation matrix corresponding to the emitter position.
		XGMatrixTranslation( &worldMatrix, m_pos[0], m_pos[1], m_pos[2] );
		XGMatrixTranspose( &worldMatrix, &worldMatrix );

		// Calculate composite world->view->projection matrix.
		XGMatrixMultiply( &temp_matrix, &viewMatrix, &worldMatrix );
		XGMatrixMultiply( &dest_matrix, &projMatrix, &temp_matrix );

		// Load up the combined world, camera & projection matrix.
		D3DDevice_SetVertexShaderConstantFast( 0, (void*)&dest_matrix, 4 );

		float vector_upload[8];
		vector_upload[0]	= screen_right[X];
		vector_upload[1]	= screen_right[Y];
		vector_upload[2]	= screen_right[Z];
		vector_upload[4]	= screen_up[X];
		vector_upload[5]	= screen_up[Y];
		vector_upload[6]	= screen_up[Z];
		D3DDevice_SetVertexShaderConstantFast( 4, (void*)( &vector_upload[0] ), 2 );

		static float vconstants[32]	= {	0.0f,  0.0f, 1.0f, 1.0f,		// Vert tex coords in C8 through C11
										1.0f,  0.0f, 1.0f, 1.0f,
										1.0f,  1.0f, 1.0f, 1.0f,
										0.0f,  1.0f, 1.0f, 1.0f,
									   -1.0f,  1.0f, 1.0f, 1.0f,		// Vert w/h multipliers in C12 through C15
										1.0f,  1.0f, 1.0f, 1.0f,
										1.0f, -1.0f, 1.0f, 1.0f,
									   -1.0f, -1.0f, 1.0f, 1.0f };
		D3DDevice_SetVertexShaderConstantFast( 8, (void*)( &vconstants[0] ), 8 );

		// Obtain push buffer lock.
		// Note that p_push is returned as a pointer to write-combined memory. Writes to write-combined memory should be
		// consecutive and in increasing order. Reads should be avoided. Additionally, any CPU reads from memory or the
		// L2 cache can force expensive partial flushes of the 32-byte write-combine cache.
		DWORD *p_push; 
		p_push = D3DDevice_BeginPush( dword_count );

		// Set up loop variables here, since we be potentially enetering the loop more than once.
		lp			= 0;
		p_particle	= mp_particle_array;
		p_v			= mp_vertices;

		for( ; lp < m_num_particles; lp++, p_particle++, p_v += 3 )
		{
			// Calculate the interpolator ( 1.0f / particle_life ).
			float terp	= p_particle->m_time * ReciprocalEstimateNR_ASM( p_particle->m_life );

			// Separate interpolator for color.
			float col_terp;

			Mth::Vector	pos( p_v[0], p_v[1], p_v[2] );
			Image::RGBA *p_col0;
			Image::RGBA *p_col1;
		
			if( m_mid_time >= 0.0f )
			{
				if( terp < m_mid_time )
				{
					p_col0		= &m_start_color;
					p_col1		= &m_mid_color;

					// Adjust interpolation for this half of the color blend.
					col_terp	= terp / m_mid_time;
				}
				else
				{
					p_col0		= &m_mid_color;
					p_col1		= &m_end_color;

					// Adjust interpolation for this half of the color blend.
					col_terp	= ( terp - m_mid_time ) / ( 1.0f - m_mid_time );
				}
			}
			else
			{
				// No mid color specified.
				p_col0		= &m_start_color;
				p_col1		= &m_end_color;

				// Color interpoltor value is the same as the regular interpolator.
				col_terp	= terp;
			}

			// We're going to be loading constants.
			p_push[0]	= D3DPUSH_ENCODE( D3DPUSH_SET_TRANSFORM_CONSTANT_LOAD, 1 );

			// Specify the starting register (physical registers are offset by 96 from the D3D logical register).
			p_push[1]	= 96 + 16;

			// Specify the number of DWORDS to load. 12 DWORDS for 3 constants.
			p_push[2]	= D3DPUSH_ENCODE( D3DPUSH_SET_TRANSFORM_CONSTANT, 12 );

			// Load position.
			p_push[3]	= *((DWORD*)&pos[X] );
			p_push[4]	= *((DWORD*)&pos[Y] );
			p_push[5]	= *((DWORD*)&pos[Z] );

			// Load start and end width and height.
			p_push[7]	= *((DWORD*)&p_particle->m_sw );
			p_push[8]	= *((DWORD*)&p_particle->m_sh );
			p_push[9]	= *((DWORD*)&p_particle->m_ew );
			p_push[10]	= *((DWORD*)&p_particle->m_eh );

			// Load size and color interpolators.
			p_push[11]	= *((DWORD*)&terp );
			p_push[12]	= *((DWORD*)&col_terp );

			p_push		+= 15;

			p_push[0]	= D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
			p_push[1]	= D3DPT_QUADLIST;
			p_push		+= 2;

			// NOTE: A maximum of 2047 DWORDs can be specified to D3DPUSH_ENCODE. If there is more than 2047 DWORDs of vertex
			// data, simply split the data into multiple D3DPUSH_ENCODE( D3DPUSH_INLINE_ARRAY ) sections.
			p_push[0]	= D3DPUSH_ENCODE( D3DPUSH_NOINCREMENT_FLAG | D3DPUSH_INLINE_ARRAY, 12 );
			++p_push;

			// Now we can start the actual vertex data.
			p_push[0]	= *((DWORD*)p_col0 );
			p_push[1]	= *((DWORD*)p_col1 );
			p_push[2]	= 0x00000000UL;

			p_push[3]	= *((DWORD*)p_col0 );
			p_push[4]	= *((DWORD*)p_col1 );
			p_push[5]	= 0x00010001UL;

			p_push[6]	= *((DWORD*)p_col0 );
			p_push[7]	= *((DWORD*)p_col1 );
			p_push[8]	= 0x00020002UL;
			
			p_push[9]	= *((DWORD*)p_col0 );
			p_push[10]	= *((DWORD*)p_col1 );
			p_push[11]	= 0x00030003UL;

			p_push		+= 12;

			// End of vertex data for this particle.
			p_push[0] = D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
			p_push[1] = 0;
			p_push += 2;
		}
		D3DDevice_EndPush( p_push );
	}

	// Deal with the Ps2 specific extensions.
	if( m_emit_rate > 0.0f )
	{
		m_emit_rate_fractional += ( m_emit_rate * ( 1.0f / 60.0f ));

		if( m_emit_rate_fractional >= 1.0f )
		{
			// This should actually deal with fractional values by accumulating them.
			emit( Ftoi_ASM( m_emit_rate_fractional ));
			m_emit_rate_fractional -= (float)Ftoi_ASM( m_emit_rate_fractional );
		}
	}
}

#else
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxParticleFlat::plat_render( void )
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

		// Submit particle material.
		mp_engine_particle->mp_material->Submit();
		
		// Set up correct vertex and pixel shader.
		NxXbox::set_vertex_shader( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2( 0 ));
		NxXbox::set_pixel_shader( PixelShader0 );
		
		DWORD dwords_per_particle	= 24;
		DWORD dword_count			= dwords_per_particle * m_num_particles;

		// Obtain push buffer lock.
		// The additional number (+5 is minimum) is to reserve enough overhead for the encoding parameters. It can safely be more, but no less.
		DWORD *p_push; 
		p_push = D3DDevice_BeginPush( dword_count + ( dword_count / 2047 ) + 16 );

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
				
				// Calculate the interpolator ( 1.0f / particle_life ).
				float terp	= p_particle->m_time * ReciprocalEstimateNR_ASM( p_particle->m_life );
				float w		= p_particle->m_sw + (( p_particle->m_ew - p_particle->m_sw ) * terp );
				float h		= p_particle->m_sh + (( p_particle->m_eh - p_particle->m_sh ) * terp );

				// Todo: Move hook to matrix/emitter code to cut down on per particle calculation.
				Mth::Vector	pos( p_v[0] + m_pos[X], p_v[1] + m_pos[Y], p_v[2] + m_pos[Z] );
				Mth::Vector	ss_right, ss_up;
				Mth::Vector tmp;
		
				ss_right	= screen_right * w;
				ss_up		= screen_up * h;

				Image::RGBA color;
				Image::RGBA *p_col0;
				Image::RGBA *p_col1;
		
				if( m_mid_time >= 0.0f )
				{
					if( terp < m_mid_time )
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

				Image::RGBA start	= *p_col0++;
				Image::RGBA end		= *p_col1++;

				// Use fixed point math to avoid _ftol2 calls.
				int f_terp	= Ftoi_ASM( terp * 4096.0f );
				color.r		= ((((int)start.r ) * 4096 ) + (((int)end.r - (int)start.r ) * f_terp )) / 4096;
				color.g		= ((((int)start.g ) * 4096 ) + (((int)end.g - (int)start.g ) * f_terp )) / 4096;
				color.b		= ((((int)start.b ) * 4096 ) + (((int)end.b - (int)start.b ) * f_terp )) / 4096;
				color.a		= ((((int)start.a ) * 4096 ) + (((int)end.a - (int)start.a ) * f_terp )) / 4096;
		
				tmp			= pos - ss_right + ss_up;
				p_push[0]	= *((DWORD*)&tmp[X] );
				p_push[1]	= *((DWORD*)&tmp[Y] );
				p_push[2]	= *((DWORD*)&tmp[Z] );
				p_push[3]	= *((DWORD*)&color );
				p_push[4]	= 0x00000000UL;
				p_push[5]	= 0x00000000UL;
	
				tmp			= pos + ss_right + ss_up;		
				p_push[6]	= *((DWORD*)&tmp[X] );
				p_push[7]	= *((DWORD*)&tmp[Y] );
				p_push[8]	= *((DWORD*)&tmp[Z] );
				p_push[9]	= *((DWORD*)&color );
				p_push[10]	= 0x3F800000UL;
				p_push[11]	= 0x00000000UL;

				tmp			= pos + ss_right - ss_up;		
				p_push[12]	= *((DWORD*)&tmp[X] );
				p_push[13]	= *((DWORD*)&tmp[Y] );
				p_push[14]	= *((DWORD*)&tmp[Z] );
				p_push[15]	= *((DWORD*)&color );
				p_push[16]	= 0x3F800000UL;
				p_push[17]	= 0x3F800000UL;
			
				tmp			= pos - ss_right - ss_up;		
				p_push[18]	= *((DWORD*)&tmp[X] );
				p_push[19]	= *((DWORD*)&tmp[Y] );
				p_push[20]	= *((DWORD*)&tmp[Z] );
				p_push[21]	= *((DWORD*)&color );
				p_push[22]	= 0x00000000UL;
				p_push[23]	= 0x3F800000UL;

				p_push		+= 24;

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
#endif



} // Nx

