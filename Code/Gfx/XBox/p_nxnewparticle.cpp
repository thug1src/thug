/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Skate5													**
**																			**
**	Module:			Gfx			 											**
**																			**
**	File name:		p_NxNewParticle.cpp										**
**																			**
**	Created by:		3/25/03	-	SPG											**
**																			**
**	Description:	Xbox new parametric particle system						**
*****************************************************************************/

#include <core/defines.h>

#include <gfx/xbox/p_nxnewparticle.h>

#include <gfx/NxTexMan.h>
#include <gfx/xbox/p_nxtexture.h>
#include <gfx/xbox/nx/nx_init.h>
#include <gfx/xbox/nx/render.h>


/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Nx
{


/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

static int rand_seed;
static int rand_a	= 314159265;
static int rand_b	= 178453311;


/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static void seed_particle_rnd( int s, int a, int b )
{
	rand_seed		= s;
	rand_a			= a;
	rand_b			= b;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static inline int particle_rnd( int n )
{
	rand_seed	= rand_seed * rand_a + rand_b;
	rand_a		= ( rand_a ^ rand_seed ) + ( rand_seed >> 4 );
	rand_b		+= ( rand_seed >> 3 ) - 0x10101010L;
	return (int)(( rand_seed & 0xffff ) * n ) >> 16;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CParticleStream::AdvanceSeed( int num_places )
{
	// Seed the random number generator back to the current seed.
	seed_particle_rnd( m_rand_seed, m_rand_a, m_rand_b );

	// Each particle will call the random function four times.
	for( int i = 0; i < ( num_places * 4 ); i++ )
	{
		rand_seed	= rand_seed * rand_a + rand_b;
		rand_a		= ( rand_a ^ rand_seed ) + ( rand_seed >> 4 );
		rand_b		+= ( rand_seed >> 3 ) - 0x10101010L;
	}

	m_rand_seed = rand_seed;
	m_rand_a	= rand_a;
	m_rand_b	= rand_b;
}
	
	
	
inline DWORD FtoDW( FLOAT f ) { return *((DWORD*)&f); }

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxNewParticle::plat_render( void )
{
	CParticleStream* p_stream;
	int i;

	// Process the streams.
	if( m_params.m_EmitRate && ( !m_emitting || ( m_params.m_EmitRate != mp_newest_stream->m_rate )))
	{	
		if( m_num_streams < m_max_streams )
		{
			// Add new stream to cyclic buffer
			m_num_streams++;
			mp_newest_stream++;
			if( mp_newest_stream == mp_stream + m_max_streams )
			{
				mp_newest_stream = mp_stream;
			}

			// Initialise new stream.
			mp_newest_stream->m_rate			= m_params.m_EmitRate;
			mp_newest_stream->m_interval		= 1.0f / m_params.m_EmitRate;
			mp_newest_stream->m_oldest_age		= 0.0f;
			mp_newest_stream->m_num_particles	= 0;
			mp_newest_stream->m_rand_seed		= rand();
			mp_newest_stream->m_rand_a			= 314159265;
			mp_newest_stream->m_rand_b			= 178453311;
			m_emitting = true;
		}
		else
		{
			m_emitting = false;
		}
	}
	else
	{
		m_emitting = m_params.m_EmitRate;
	}

	if( !m_num_streams )
		return;

	// Age all streams.
	for( i = 0, p_stream = mp_oldest_stream; i < m_num_streams; ++i )
	{
		// Increase age of oldest particle.
		p_stream->m_oldest_age += 1.0f / 60.0f;

		// Step pointer within cyclic buffer.
		p_stream++;
		if( p_stream == mp_stream + m_max_streams )
		{
			p_stream = mp_stream;
		}
	}

	// Births into newest stream.
	if( m_emitting )
	{
		// How many particles so far emitted?
		mp_newest_stream->m_num_particles = (int)( mp_newest_stream->m_oldest_age * mp_newest_stream->m_rate + 1.0f );
	}

	// Deaths from oldest stream.
	if( mp_oldest_stream->m_oldest_age > m_params.m_Lifetime )
	{
		// Work out number dead.
		int particles_dead = (int)(( mp_oldest_stream->m_oldest_age - m_params.m_Lifetime ) * mp_oldest_stream->m_rate + 1.0f );

		// Remove dead particles.
		mp_oldest_stream->m_num_particles -= particles_dead;

		// Should we keep processing the oldest stream?
		if( mp_oldest_stream->m_num_particles > 0 || ( m_num_streams == 1 && m_emitting ))
		{
			// Adjust age of oldest particle.
			mp_oldest_stream->m_oldest_age -= (float)particles_dead * mp_oldest_stream->m_interval;

			// Advance seed.
			mp_oldest_stream->AdvanceSeed( particles_dead );
		}
		else
		{
			// Remove oldest stream and wrap in cyclic buffer if necessary.
			m_num_streams--;
			mp_oldest_stream++;
			if( mp_oldest_stream == mp_stream + m_max_streams )
			{
				mp_oldest_stream = mp_stream;
			}
			if( !m_num_streams )
				return;
		}
	}

	// Now render the streams. after checking the bounding sphere is visible (for no dynamic systems).
	if( !m_params.m_LocalCoord )
	{
		D3DXVECTOR3	center( m_bsphere[X], m_bsphere[Y], m_bsphere[Z] );
		if( !NxXbox::frustum_check_sphere( &center, m_bsphere[W] ))
		{
			return;
		}
	}

	// Swap the r and b color components in the params.
	for( i = 0; i < vNUM_BOXES; ++i )
	{
		uint8 swap = m_params.m_Color[i].r;
		m_params.m_Color[i].r	= m_params.m_Color[i].b;
		m_params.m_Color[i].b	= swap;
	}

	// Figure the distance from the camera to the system, and use a different technique accordingly.
	// Need to do this because point sprites are limited to 64 pixels in each dimension, and sometimes
	// you can get close enough to a system that a single particle will occupy more than this.
	// A more sophisticated approach would be to figure what the max particle size could be at a given distance,
	// rather than using the current hardcoded distance value of 20 feet.
	float dist_squared = Mth::DistanceSqr( Mth::Vector( m_bsphere[X], m_bsphere[Y], m_bsphere[Z], 0.0f ),
										   Mth::Vector( NxXbox::EngineGlobals.cam_position.x, NxXbox::EngineGlobals.cam_position.y, NxXbox::EngineGlobals.cam_position.z, 0.0f ));
	if( dist_squared < 57600.0f )
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

		// Submit particle material.
		mp_material->Submit();

		// Set up correct vertex and pixel shader.
		NxXbox::set_vertex_shader( ParticleNewFlatVS );
		NxXbox::set_pixel_shader( PixelShader0 );

		// Load up the combined world->view_projection matrix.
		XGMATRIX	dest_matrix;
		XGMATRIX	projMatrix;
		XGMATRIX	viewMatrix;
			
		// Projection matrix.
		XGMatrixTranspose( &projMatrix, &NxXbox::EngineGlobals.projection_matrix );
		
		// View matrix.
		XGMatrixTranspose( &viewMatrix, &NxXbox::EngineGlobals.view_matrix );
		viewMatrix.m[3][0] = 0.0f;
		viewMatrix.m[3][1] = 0.0f;
		viewMatrix.m[3][2] = 0.0f;
		viewMatrix.m[3][3] = 1.0f;

		// Calculate composite world->view->projection matrix (simplified since world transform will be indentity).
		XGMatrixMultiply( &dest_matrix, &projMatrix, &viewMatrix );

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

		float stream_params[24];
		stream_params[0]	= m_s0[X];
		stream_params[1]	= m_s0[Y];
		stream_params[2]	= m_s0[Z];
		stream_params[3]	= m_s0[W];

		stream_params[4]	= m_s1[X];
		stream_params[5]	= m_s1[Y];
		stream_params[6]	= m_s1[Z];
		stream_params[7]	= m_s1[W];

		stream_params[8]	= m_s2[X];
		stream_params[9]	= m_s2[Y];
		stream_params[10]	= m_s2[Z];
		stream_params[11]	= m_s2[W];

		stream_params[12]	= m_p0[X];
		stream_params[13]	= m_p0[Y];
		stream_params[14]	= m_p0[Z];
		stream_params[15]	= m_p0[W];

		stream_params[16]	= m_p1[X];
		stream_params[17]	= m_p1[Y];
		stream_params[18]	= m_p1[Z];
		stream_params[19]	= m_p1[W];

		stream_params[20]	= m_p2[X];
		stream_params[21]	= m_p2[Y];
		stream_params[22]	= m_p2[Z];
		stream_params[23]	= m_p2[W];
		D3DDevice_SetVertexShaderConstantFast( 18, (void*)( &stream_params[0] ), 6 );

		// Construct a packet with data for each stream.
		for( i = 0, p_stream = mp_oldest_stream; i < m_num_streams; i++, p_stream++ )
		{
			Dbg_MsgAssert( p_stream->m_num_particles < 65536, ( "particle limit reached" ));

			// Wrap at end of cyclic buffer.
			if( p_stream == mp_stream + m_max_streams )
			{
				p_stream = mp_stream;
			}

			// Calculate space needed.
			DWORD dwords_per_particle	= 28;
			DWORD dword_count			= dwords_per_particle * p_stream->m_num_particles;

			// Obtain push buffer lock.
			// Note that p_push is returned as a pointer to write-combined memory. Writes to write-combined memory should be
			// consecutive and in increasing order. Reads should be avoided. Additionally, any CPU reads from memory or the
			// L2 cache can force expensive partial flushes of the 32-byte write-combine cache.
			DWORD *p_push; 
			p_push = D3DDevice_BeginPush( dword_count );

			float t				= p_stream->m_oldest_age;
			float midpoint_time = m_params.m_Lifetime * ( m_params.m_ColorMidpointPct * 0.01f );

			// Seed the random number generators for this stream.
			seed_particle_rnd( p_stream->m_rand_seed, p_stream->m_rand_a, p_stream->m_rand_b );

			for( int p = 0; p < p_stream->m_num_particles; ++p )
			{
				// Generate random vector. Each component in the range [1.0, 2.0].
				float r[4];
				r[0] = 1.0f + ((float)particle_rnd( 16384 ) / 16384 );
				r[1] = 1.0f + ((float)particle_rnd( 16384 ) / 16384 );
				r[2] = 1.0f + ((float)particle_rnd( 16384 ) / 16384 );
				r[3] = 1.0f + ((float)particle_rnd( 16384 ) / 16384 );

				float color_interpolator;
				Image::RGBA	col0, col1;

				if( m_params.m_UseMidcolor )
				{
					if( t > midpoint_time )
					{
						color_interpolator	= ( t - midpoint_time ) * ReciprocalEstimate_ASM( m_params.m_Lifetime - midpoint_time );
						col0				= m_params.m_Color[1];
						col1				= m_params.m_Color[2];
					}
					else
					{
						color_interpolator	= t * ReciprocalEstimate_ASM( midpoint_time );
						col0				= m_params.m_Color[0];
						col1				= m_params.m_Color[1];
					}
				}
				else 
				{
					color_interpolator		= t * ReciprocalEstimate_ASM( m_params.m_Lifetime );
					col0					= m_params.m_Color[0];
					col1					= m_params.m_Color[2];
				}

				// We're going to be loading constants.
				p_push[0]	= D3DPUSH_ENCODE( D3DPUSH_SET_TRANSFORM_CONSTANT_LOAD, 1 );

				// Specify the starting register (physical registers are offset by 96 from the D3D logical register).
				p_push[1]	= 96 + 16;

				// Specify the number of DWORDS to load. 8 DWORDS for 2 constants.
				p_push[2]	= D3DPUSH_ENCODE( D3DPUSH_SET_TRANSFORM_CONSTANT, 8 );

				// Load r vector.
				p_push[3]	= *((DWORD*)&r[0] );
				p_push[4]	= *((DWORD*)&r[1] );
				p_push[5]	= *((DWORD*)&r[2] );
				p_push[6]	= *((DWORD*)&r[3] );

				// Load interpolator values.
				p_push[7]	= *((DWORD*)&t );
				p_push[8]	= *((DWORD*)&color_interpolator );
				p_push		+= 11;

				p_push[0]	= D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
				p_push[1]	= D3DPT_QUADLIST;
				p_push		+= 2;

				// NOTE: A maximum of 2047 DWORDs can be specified to D3DPUSH_ENCODE. If there is more than 2047 DWORDs of vertex
				// data, simply split the data into multiple D3DPUSH_ENCODE( D3DPUSH_INLINE_ARRAY ) sections.
				p_push[0]	= D3DPUSH_ENCODE( D3DPUSH_NOINCREMENT_FLAG | D3DPUSH_INLINE_ARRAY, 12 );
				++p_push;

				// Now we can start the actual vertex data.
				p_push[0]	= *((DWORD*)&col0 );
				p_push[1]	= *((DWORD*)&col1 );
				p_push[2]	= 0x00000000UL;

				p_push[3]	= *((DWORD*)&col0 );
				p_push[4]	= *((DWORD*)&col1 );
				p_push[5]	= 0x00010001UL;

				p_push[6]	= *((DWORD*)&col0 );
				p_push[7]	= *((DWORD*)&col1 );
				p_push[8]	= 0x00020002UL;
				
				p_push[9]	= *((DWORD*)&col0 );
				p_push[10]	= *((DWORD*)&col1 );
				p_push[11]	= 0x00030003UL;

				p_push		+= 12;

				// End of vertex data for this particle.
				p_push[0] = D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
				p_push[1] = 0;
				p_push += 2;

				// Reduce t by particle interval.
				t -= p_stream->m_interval;
			}
			D3DDevice_EndPush( p_push );
		}
	}
	else
	{
		// Submit particle material.
		mp_material->Submit();

		// Point sprites actually use texture stage 3...
		NxXbox::set_render_state( RS_UVADDRESSMODE3, 0x00000000UL );
		mp_material->mp_tex[0]->Set( 3 );

		// Set up point sprite rendering.
		D3DDevice_SetRenderState( D3DRS_POINTSPRITEENABLE,	TRUE );
		D3DDevice_SetRenderState( D3DRS_POINTSCALEENABLE,	TRUE );
//		D3DDevice_SetRenderState( D3DRS_POINTSCALE_A,		FtoDW( 0.00f ));
//		D3DDevice_SetRenderState( D3DRS_POINTSCALE_B,		FtoDW( 0.00f ));
//		D3DDevice_SetRenderState( D3DRS_POINTSCALE_C,		FtoDW( 1.00f ));

		// Set up correct vertex and pixel shader.
		NxXbox::set_vertex_shader( ParticleNewFlatPointSpriteVS );
		NxXbox::set_pixel_shader( PixelShaderPointSprite );

		// Load up the combined world->view_projection matrix.
		XGMATRIX	dest_matrix;
		XGMATRIX	projMatrix;
		XGMATRIX	viewMatrix;
			
		// Projection matrix.
		XGMatrixTranspose( &projMatrix, &NxXbox::EngineGlobals.projection_matrix );
		
		// View matrix.
		XGMatrixTranspose( &viewMatrix, &NxXbox::EngineGlobals.view_matrix );
		viewMatrix.m[3][0] = 0.0f;
		viewMatrix.m[3][1] = 0.0f;
		viewMatrix.m[3][2] = 0.0f;
		viewMatrix.m[3][3] = 1.0f;

		// Calculate composite world->view->projection matrix (simplified since world transform will be indentity).
		XGMatrixMultiply( &dest_matrix, &projMatrix, &viewMatrix );

		// Load up the combined world, camera & projection matrix.
		D3DDevice_SetVertexShaderConstantFast( 0, (void*)&dest_matrix, 4 );

		// Load up the stream parameters.
		D3DDevice_SetVertexShaderConstantFast( 18, (void*)( &m_s0[X] ), 1 );
		D3DDevice_SetVertexShaderConstantFast( 19, (void*)( &m_s1[X] ), 1 );
		D3DDevice_SetVertexShaderConstantFast( 20, (void*)( &m_s2[X] ), 1 );

		D3DDevice_SetVertexShaderConstantFast( 21, (void*)( &m_p0[X] ), 1 );
		D3DDevice_SetVertexShaderConstantFast( 22, (void*)( &m_p1[X] ), 1 );
		D3DDevice_SetVertexShaderConstantFast( 23, (void*)( &m_p2[X] ), 1 );

		// Load up the camera position and viewport height.
		static float cam_pos_viewport_height[4];
		cam_pos_viewport_height[0]	= NxXbox::EngineGlobals.cam_position.x;
		cam_pos_viewport_height[1]	= NxXbox::EngineGlobals.cam_position.y;
		cam_pos_viewport_height[2]	= NxXbox::EngineGlobals.cam_position.z;
		cam_pos_viewport_height[3]	= NxXbox::EngineGlobals.viewport.Height * 2.0f;

		D3DDevice_SetVertexShaderConstantFast( 24, (void*)( &cam_pos_viewport_height[0] ), 1 );

		// Construct a packet with data for each stream.
		for( i = 0, p_stream = mp_oldest_stream; i < m_num_streams; i++, p_stream++ )
		{
			Dbg_MsgAssert( p_stream->m_num_particles < 65536, ( "particle limit reached" ));

			// Wrap at end of cyclic buffer.
			if( p_stream == mp_stream + m_max_streams )
			{
				p_stream = mp_stream;
			}

			// Calculate space needed.
			DWORD dwords_per_particle	= 13;
			DWORD dword_count			= dwords_per_particle * p_stream->m_num_particles;

			// Obtain push buffer lock.
			// Note that p_push is returned as a pointer to write-combined memory. Writes to write-combined memory should be
			// consecutive and in increasing order. Reads should be avoided. Additionally, any CPU reads from memory or the
			// L2 cache can force expensive partial flushes of the 32-byte write-combine cache.
			DWORD *p_push; 
			p_push = D3DDevice_BeginPush( dword_count );

			float t				= p_stream->m_oldest_age;
			float midpoint_time = m_params.m_Lifetime * ( m_params.m_ColorMidpointPct * 0.01f );

			// Seed the random number generators for this stream.
			seed_particle_rnd( p_stream->m_rand_seed, p_stream->m_rand_a, p_stream->m_rand_b );

			for( int p = 0; p < p_stream->m_num_particles; ++p )
			{
				// Generate random vector. Each component in the range [1.0, 2.0].
				float r[4];
				r[0] = 1.0f + ((float)particle_rnd( 16384 ) / 16384 );
				r[1] = 1.0f + ((float)particle_rnd( 16384 ) / 16384 );
				r[2] = 1.0f + ((float)particle_rnd( 16384 ) / 16384 );
				r[3] = 1.0f + ((float)particle_rnd( 16384 ) / 16384 );

				float color_interpolator;
				Image::RGBA	col0, col1;

				if( m_params.m_UseMidcolor )
				{
					if( t > midpoint_time )
					{
						color_interpolator	= ( t - midpoint_time ) * ReciprocalEstimate_ASM( m_params.m_Lifetime - midpoint_time );
						col0				= m_params.m_Color[1];
						col1				= m_params.m_Color[2];
					}
					else
					{
						color_interpolator	= t * ReciprocalEstimate_ASM( midpoint_time );
						col0				= m_params.m_Color[0];
						col1				= m_params.m_Color[1];
					}
				}
				else 
				{
					color_interpolator		= t * ReciprocalEstimate_ASM( m_params.m_Lifetime );
					col0					= m_params.m_Color[0];
					col1					= m_params.m_Color[2];
				}

				// Signal the primitive type to follow.
				p_push[0]	= D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
				p_push[1]	= D3DPT_POINTLIST;
				p_push		+= 2;

				// NOTE: A maximum of 2047 DWORDs can be specified to D3DPUSH_ENCODE. If there is more than 2047 DWORDs of vertex
				// data, simply split the data into multiple D3DPUSH_ENCODE( D3DPUSH_INLINE_ARRAY ) sections.
				p_push[0]	= D3DPUSH_ENCODE( D3DPUSH_NOINCREMENT_FLAG | D3DPUSH_INLINE_ARRAY, 8 );
				++p_push;

				// Now we can start the actual vertex data.

				// Load r vector.
				p_push[0]	= *((DWORD*)&r[0] );
				p_push[1]	= *((DWORD*)&r[1] );
				p_push[2]	= *((DWORD*)&r[2] );
				p_push[3]	= *((DWORD*)&r[3] );

				// Load time and color interpolator values.
				p_push[4]	= *((DWORD*)&t );
				p_push[5]	= *((DWORD*)&color_interpolator );

				// Load colors.
				p_push[6]	= *((DWORD*)&col0 );
				p_push[7]	= *((DWORD*)&col1 );

				p_push		+= 8;

				// End of vertex data for this particle.
				p_push[0] = D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
				p_push[1] = 0;
				p_push += 2;

				// Reduce t by particle interval.
				t -= p_stream->m_interval;
			}
			D3DDevice_EndPush( p_push );
		}

		// Restore render states.
		D3DDevice_SetRenderState( D3DRS_POINTSPRITEENABLE, FALSE );
	}

	// Swap the r and b color components back in the params.
	for( i = 0; i < vNUM_BOXES; ++i )
	{
		uint8 swap = m_params.m_Color[i].r;
		m_params.m_Color[i].r	= m_params.m_Color[i].b;
		m_params.m_Color[i].b	= swap;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxNewParticle::update_position( void )
{
	// Convert 3-point -> PVA format
	float t1 = m_params.m_Lifetime * m_params.m_MidpointPct * 0.01f;
	float t2 = m_params.m_Lifetime;
	Mth::Vector u, a_;

	Mth::Vector x0	= m_params.m_BoxPos[0];
	x0[3]			= m_params.m_Radius[0];
	Mth::Vector x1	= m_params.m_BoxPos[1];
	x1[3]			= m_params.m_Radius[1];
	Mth::Vector x2	= m_params.m_BoxPos[2];
	x2[3]			= m_params.m_Radius[2];

	if( m_params.m_UseMidpoint )
	{
		u  = ( t2 * t2 * ( x1 - x0 ) - t1 * t1 * ( x2 - x0 )) / ( t1 * t2 * ( t2 - t1 ));
		a_ = ( t1 * ( x2 - x0 ) - t2 * ( x1 - x0 )) / ( t1 * t2 * ( t2 - t1 ));
	}
	else
	{
		u  = ( x2 - x0 ) / t2;
		a_.Set( 0, 0, 0, 0 );
	}

	m_p0 = x0 - 1.5f * m_s0;
	m_p1 = u  - 1.5f * m_s1;
	m_p2 = a_ - 1.5f * m_s2;
	m_p0[3] = x0[3] - 1.5f * m_s0[3];
	m_p1[3] = u[3]  - 1.5f * m_s1[3];
	m_p2[3] = a_[3] - 1.5f * m_s2[3];
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxNewParticle::plat_update( void )
{
	if( m_params.m_LocalCoord )
	{
		update_position();
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxNewParticle::plat_build( void )
{
	// Reduce emit rate selectively to improve performance.
	m_params.m_EmitRate	= m_params.m_EmitRate * 0.5f;

	// Initialise streams.
	m_max_streams		= 5;
	m_num_streams		= 0;

	mp_stream			= new CParticleStream[m_max_streams]; 
	mp_newest_stream	= mp_stream + m_max_streams - 1;
	mp_oldest_stream	= mp_stream;
	m_emitting			= false;

	// Create a (semi-transparent) material used to render the mesh.
	mp_material			= new NxXbox::sMaterial;
	ZeroMemory( mp_material, sizeof( NxXbox::sMaterial ));

	mp_material->m_flags[0]		= MATFLAG_TRANSPARENT | MATFLAG_TEXTURED;
	mp_material->m_passes		= 1;
	mp_material->m_alpha_cutoff	= 1;
	mp_material->m_no_bfc		= true;
	mp_material->m_color[0][0]	= 0.5f;
	mp_material->m_color[0][1]	= 0.5f;
	mp_material->m_color[0][2]	= 0.5f;
	mp_material->m_color[0][3]	= m_params.m_FixedAlpha * ( 1.0f /  128.0f );
	mp_material->m_reg_alpha[0]	= NxXbox::GetBlendMode( m_params.m_BlendMode );

	// Get texture.
	Nx::CTexture*		p_texture;
	Nx::CXboxTexture*	p_xbox_texture;
	mp_material->mp_tex[0]	= NULL;
	p_texture				= Nx::CTexDictManager::sp_particle_tex_dict->GetTexture( m_params.m_Texture );
	p_xbox_texture			= static_cast<Nx::CXboxTexture*>( p_texture );
	if( p_xbox_texture )
	{
		mp_material->mp_tex[0] = p_xbox_texture->GetEngineTexture();
	}

	// Convert 3-point -> PVA format.
	float t1 = m_params.m_Lifetime * m_params.m_MidpointPct * 0.01f;
	float t2 = m_params.m_Lifetime;
	Mth::Vector x0,x1,x2,u,a_;

	x0    = m_params.m_BoxDims[0];
	x0[3] = m_params.m_RadiusSpread[0];
	x1    = m_params.m_BoxDims[1];
	x1[3] = m_params.m_RadiusSpread[1];
	x2    = m_params.m_BoxDims[2];
	x2[3] = m_params.m_RadiusSpread[2];

	if( m_params.m_UseMidpoint )
	{
		u  = ( t2 * t2 * ( x1 - x0 ) - t1 * t1 * ( x2 - x0 )) / ( t1 * t2 * ( t2 - t1 ));
		a_ = ( t1 * ( x2 - x0 ) - t2 * ( x1 - x0 )) / ( t1 * t2 * ( t2 - t1 ));
	}
	else
	{
		u  = ( x2 - x0 ) / t2;
		a_.Set( 0.0f, 0.0f, 0.0f, 0.0f );
	}

	m_s0 = x0;
	m_s1 = u;
	m_s2 = a_;

	x0    = m_params.m_BoxPos[0];
	x0[3] = m_params.m_Radius[0];
	x1    = m_params.m_BoxPos[1];
	x1[3] = m_params.m_Radius[1];
	x2    = m_params.m_BoxPos[2];
	x2[3] = m_params.m_Radius[2];

	if( m_params.m_UseMidpoint )
	{
		u  =  ( t2 * t2 * ( x1 - x0 ) - t1 * t1 * ( x2 - x0 )) / ( t1 * t2 * ( t2 - t1 ));
		a_ =  ( t1 * ( x2 - x0 ) - t2 * ( x1 - x0 )) / ( t1 * t2 * ( t2 - t1 ));
	}
	else
	{
		u  = ( x2 - x0 ) / t2;
		a_.Set( 0.0f, 0.0f, 0.0f, 0.0f );
	}

	m_p0	= x0 - 1.5f * m_s0;
	m_p1	= u  - 1.5f * m_s1;
	m_p2	= a_ - 1.5f * m_s2;
	m_p0[3]	= x0[3] - 1.5f * m_s0[3];
	m_p1[3]	= u[3]  - 1.5f * m_s1[3];
	m_p2[3]	= a_[3] - 1.5f * m_s2[3];

	update_position();

	// Color.
	if( m_params.m_UseMidcolor )
	{
		float q0 = 100.0f / ( m_params.m_Lifetime * m_params.m_ColorMidpointPct );
		float q1 = 100.0f / ( m_params.m_Lifetime * ( 100.0f - m_params.m_ColorMidpointPct ));

//		m_systemDmaData.m_c0[0] = ((float)m_params.m_Color[1].r - (float)m_params.m_Color[0].r) * q0;
//		m_systemDmaData.m_c0[1] = ((float)m_params.m_Color[1].g - (float)m_params.m_Color[0].g) * q0;
//		m_systemDmaData.m_c0[2] = ((float)m_params.m_Color[1].b - (float)m_params.m_Color[0].b) * q0;
//		m_systemDmaData.m_c0[3] = ((float)m_params.m_Color[1].a - (float)m_params.m_Color[0].a) * q0;

//		m_systemDmaData.m_c1[0] = (float)m_params.m_Color[1].r;
//		m_systemDmaData.m_c1[1] = (float)m_params.m_Color[1].g;
//		m_systemDmaData.m_c1[2] = (float)m_params.m_Color[1].b;
//		m_systemDmaData.m_c1[3] = (float)m_params.m_Color[1].a;

//		m_systemDmaData.m_c2[0] = ((float)m_params.m_Color[2].r - (float)m_params.m_Color[1].r) * q1;
//		m_systemDmaData.m_c2[1] = ((float)m_params.m_Color[2].g - (float)m_params.m_Color[1].g) * q1;
//		m_systemDmaData.m_c2[2] = ((float)m_params.m_Color[2].b - (float)m_params.m_Color[1].b) * q1;
//		m_systemDmaData.m_c2[3] = ((float)m_params.m_Color[2].a - (float)m_params.m_Color[1].a) * q1;
	}
	else // else suppress mid-colour
	{
		float q = 1.0f / m_params.m_Lifetime;

//		m_systemDmaData.m_c1[0] = (float)m_params.m_Color[0].r;
//		m_systemDmaData.m_c1[1] = (float)m_params.m_Color[0].g;
//		m_systemDmaData.m_c1[2] = (float)m_params.m_Color[0].b;
//		m_systemDmaData.m_c1[3] = (float)m_params.m_Color[0].a;

//		m_systemDmaData.m_c2[0] = ((float)m_params.m_Color[2].r - (float)m_params.m_Color[0].r) * q;
//		m_systemDmaData.m_c2[1] = ((float)m_params.m_Color[2].g - (float)m_params.m_Color[0].g) * q;
//		m_systemDmaData.m_c2[2] = ((float)m_params.m_Color[2].b - (float)m_params.m_Color[0].b) * q;
//		m_systemDmaData.m_c2[3] = ((float)m_params.m_Color[2].a - (float)m_params.m_Color[0].a) * q;
	}

	// Rotation matrix.
	m_rotation.Identity();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxNewParticle::plat_destroy( void )
{
	if( mp_stream )
	{
		delete [] mp_stream;
	}

	if( mp_material )
	{
		delete mp_material;
	}
}



/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Nx




