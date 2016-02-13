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
**	Description:	Ngc new parametric particle system						**
*****************************************************************************/

#include <core/defines.h>

#include <gfx/Ngc/p_nxnewparticle.h>

#include <gfx/NxTexMan.h>
#include <gfx/Ngc/p_nxtexture.h>
#include <gfx/Ngc/nx/nx_init.h>
#include <gfx/Ngc/nx/render.h>

#include "gfx/ngc/p_nxparticle.h"

#include "dolphin/base/ppcwgpipe.h"
#include "dolphin/gx/gxvert.h"
#include <charpipeline/GQRSetup.h>


extern "C"
{

extern float ReciprocalEstimate_ASM( float f );

extern void RenderNewParticles( Nx::CParticleStream * p_stream, float lifetime, float midpercent, bool use_mid_color, Image::RGBA * p_color0, Mth::Vector * p0, Mth::Vector * sr, Mth::Vector * su, float * p_params, float nearz );

}

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

//static int rand_seed;
//static int rand_a	= 314159265;
//static int rand_b	= 178453311;


/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//static void seed_particle_rnd( int s, int a, int b )
//{
//	rand_seed		= s;
//	rand_a			= a;
//	rand_b			= b;
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//static int particle_rnd( int n )
//{
//	rand_seed	= rand_seed * rand_a + rand_b;
//	rand_a		= ( rand_a ^ rand_seed ) + ( rand_seed >> 4 );
//	rand_b		+= ( rand_seed >> 3 ) - 0x10101010L;
//	return (int)(( rand_seed & 0xffff ) * n ) >> 16;
//}
//
//

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CParticleStream::AdvanceSeed( int num_places )
{
//	// Seed the random number generator back to the current seed.
//	seed_particle_rnd( m_rand_seed, m_rand_a, m_rand_b );
//
//	// Each particle will call the random function four times.
//	for( int i = 0; i < ( num_places * 4 ); i++ )
//	{
//		particle_rnd( 1 );
//	}
//
//	m_rand_seed = rand_seed;
//	m_rand_a	= rand_a;
//	m_rand_b	= rand_b;

	// Seed the random number generator back to the current seed.
	uint32 rand_current = m_rand_current;

	// Each particle will call the random function four times.
	for( int i = 0; i < ( num_places * 4 ); i++ )
	{
		rand_current = ( rand_current & 1 ) ? ( rand_current >> 1 ) ^ 0x3500 : ( rand_current >> 1 );
	}

	m_rand_current = rand_current;
}
	
	
	
//inline DWORD FtoDW( FLOAT f ) { return *((DWORD*)&f); }

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcNewParticle::plat_render( void )
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
//			mp_newest_stream->m_rand_seed		= rand();
//			mp_newest_stream->m_rand_a			= 314159265;
//			mp_newest_stream->m_rand_b			= 178453311;
			mp_newest_stream->m_rand_current	= rand() & ( 16384 - 1 );
			if ( mp_newest_stream->m_rand_current == 0 ) mp_newest_stream->m_rand_current = 1;
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

//	// Now render the streams. after checking the bounding sphere is visible.
//	D3DXVECTOR3	center( m_bsphere[X], m_bsphere[Y], m_bsphere[Z] );
//	if( !NxNgc::frustum_check_sphere( &center, m_bsphere[W] ))
//	{
//		return;
//	}
	if( !m_params.m_LocalCoord )
	{
		if ( !NxNgc::frustum_check_sphere( m_bsphere ) ) return;
	}
 //   if ( NxNgc::TestSphereAgainstOccluders( &sphere ) )

	GX::SetChanCtrl( GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
	GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_TEX0, GX_DIRECT );

	NxNgc::sMaterialHeader mat;
	NxNgc::sMaterialPassHeader pass;

	mat.m_checksum			= 0;
	mat.m_passes			= 1;
	mat.m_alpha_cutoff		= 0;
	mat.m_flags				= 0;
//	mat.m_material_dl_id	= 0;
	mat.m_draw_order		= 0;
	mat.m_pass_item			= 0;
	mat.m_texture_dl_id		= 0;
//	mat.m_shininess			= 0;
//	mat.m_specular_color	= (GXColor){255,255,255,255};

	pass.m_texture.p_data	= mp_texture;
	pass.m_flags			= (1<<0)|(1<<5)|(1<<6);
	pass.m_filter			= 0;
	pass.m_blend_mode		= m_blend;
	pass.m_alpha_fix		= m_fix;
	pass.m_uv_wibble_index	= 0;
	pass.m_color			= (GXColor){64,64,64,255};
	pass.m_k				= 0;
	pass.m_u_tile			= 0;
	pass.m_v_tile			= 0;
	pass.m_uv_enabled		= false;

	multi_mesh( &mat, &pass, true, true );
	GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );

	// Get the 'right' vector as the cross product of camera 'at and world 'up'.
	NsMatrix*	p_matrix	= &NxNgc::EngineGlobals.camera;

	NsVector	up( 0.0f, 1.0f, 0.0f );
	NsVector screen_right;
	NsVector screen_up;
	screen_right.cross( *p_matrix->getAt(), up );
	screen_up.cross( screen_right, *p_matrix->getAt());

	screen_right.normalize();
	screen_up.normalize();
	
	Mth::Vector sr;
	Mth::Vector su;
	sr[X] = screen_right.x;
	sr[Y] = screen_right.y;
	sr[Z] = screen_right.z;
	sr[W] = 1.0f;

	su[X] = screen_up.x;
	su[Y] = screen_up.y;
	su[Z] = screen_up.z;
	su[W] = 1.0f;

	GQRSetup6( GQR_SCALE_64,		// Pos
			   GQR_TYPE_S16,
			   GQR_SCALE_64,
			   GQR_TYPE_S16 );
	GQRSetup7( 14,		// Normal
			   GQR_TYPE_S16,
			   14,
			   GQR_TYPE_S16 );

	f32				pm[GX_PROJECTION_SZ];
	f32				vp[GX_VIEWPORT_SZ];

	GX::GetProjectionv( pm );
	GX::GetViewportv( vp );
	NsMatrix * p_mtx = &NxNgc::EngineGlobals.local_to_camera;

	float params[6];
	params[0] = p_mtx->getAtX();
	params[1] = p_mtx->getAtY();
	params[2] = p_mtx->getPosZ();
	params[3] = p_mtx->getAtZ();
	params[4] = pm[1];
	params[5] = vp[2];

	// Construct a packet with data for each stream.
	for( i = 0, p_stream = mp_oldest_stream; i < m_num_streams; i++, p_stream++ )
	{
		Dbg_MsgAssert( p_stream->m_num_particles < 65536, ( "particle limit reached" ));

		// Wrap at end of cyclic buffer.
		if( p_stream == mp_stream + m_max_streams )
		{
			p_stream = mp_stream;
		}

#if 1
		RenderNewParticles( p_stream,
							m_params.m_Lifetime,
							m_params.m_ColorMidpointPct,
							m_params.m_UseMidcolor,
							&m_params.m_Color[0],
							&m_s0,
							&sr,
							&su,
							params,
							256.0f );

#else
		float t				= p_stream->m_oldest_age;
		float midpoint_time = m_params.m_Lifetime * ( m_params.m_ColorMidpointPct * 0.01f );

		// Seed the random number generators for this stream.
//		seed_particle_rnd( p_stream->m_rand_seed, p_stream->m_rand_a, p_stream->m_rand_b );
		uint32 rand_current = p_stream->m_rand_current;

		for( int p = 0; p < p_stream->m_num_particles; ++p )
		{
			// Generate random vector. Each component in the range [1.0, 2.0].
//			Mth::Vector r( 1.0f + ((float)particle_rnd( 16384 ) / 16384 ),
//						   1.0f + ((float)particle_rnd( 16384 ) / 16384 ),
//						   1.0f + ((float)particle_rnd( 16384 ) / 16384 ),
//						   1.0f + ((float)particle_rnd( 16384 ) / 16384 ));

			Mth::Vector r;
			r[X] = 1.0f + ((float)( rand_current = ( ( rand_current & 1 ) ? ( rand_current >> 1 ) ^ 0x3500 : ( rand_current >> 1 ) ) ) / 16384.0f );
			r[Y] = 1.0f + ((float)( rand_current = ( ( rand_current & 1 ) ? ( rand_current >> 1 ) ^ 0x3500 : ( rand_current >> 1 ) ) ) / 16384.0f );
			r[Z] = 1.0f + ((float)( rand_current = ( ( rand_current & 1 ) ? ( rand_current >> 1 ) ^ 0x3500 : ( rand_current >> 1 ) ) ) / 16384.0f );
			r[W] = 1.0f + ((float)( rand_current = ( ( rand_current & 1 ) ? ( rand_current >> 1 ) ^ 0x3500 : ( rand_current >> 1 ) ) ) / 16384.0f );

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


//; Calculate the position of the particle, from:
// pos = ( m_p0 + ( t * m_p1 ) + (( t * t ) * m_p2 )) + ( m_s0 + ( t * m_s1 ) + (( t * t ) * m_s2 )).Scale( r );
			Mth::Vector pos = ( m_p0 + ( t * m_p1 ) + (( t * t ) * m_p2 )) + ( m_s0 + ( t * m_s1 ) + (( t * t ) * m_s2 )).Scale( r );

			GXColor matcol;
			matcol.r = col0.r + (uint8)( ( (float)col1.r - (float)col0.r ) * color_interpolator );
			matcol.g = col0.g + (uint8)( ( (float)col1.g - (float)col0.g ) * color_interpolator );
			matcol.b = col0.b + (uint8)( ( (float)col1.b - (float)col0.b ) * color_interpolator );
			matcol.a = col0.a + (uint8)( ( (float)col1.a - (float)col0.a ) * color_interpolator );

			GX::SetChanMatColor( GX_COLOR0A0, matcol );
			GX::SetChanAmbColor( GX_COLOR0A0, matcol );

//			NsVector sr;
//			sr.x = screen_right.x * pos[W];
//			sr.y = screen_right.y * pos[W];
//			sr.z = screen_right.z * pos[W];
//
//			float sx, sy, sz;
//			float tx, ty, tz;
//			float sc;
//			GX::Project( pos[X], pos[Y], pos[Z], (Mtx)p_mtx, pm, vp, &sx, &sy, &sz );
//			GX::Project( pos[X]+sr.x, pos[Y]+sr.y, pos[Z]+sr.z, (Mtx)p_mtx, pm, vp, &tx, &ty, &tz );
//			sc = tx - sx;	//sqrtf( ( ( tx - sx ) * ( tx - sx ) ) + ( ( ty - sy ) * ( ty - sy ) ) + ( ( tz - sz ) * ( tz - sz ) ) );

			// Calculate size.
//			float x = p_mtx->getRightX()*pos[W];
			float z = p_mtx->getAtX()*pos[X] + p_mtx->getAtY()*pos[Y] + p_mtx->getAtZ()*pos[Z] + p_mtx->getPosZ();
			float xc = pos[W] * pm[1];	// + z * pm[2];
			float wc = ( 1.0f / z );
			float sc = xc * vp[2] * wc;	// + vp[0] + vp[2]/2;

			sc = fabsf( sc );

			int size = (int)(sc * 6.0f);
			if ( size <= 255 )
			{
				GX::SetPointSize( size, GX_TO_ONE );
				GX::Begin( GX_POINTS, GX_VTXFMT0, 1 ); 
				GXWGFifo.f32 = pos[X];
				GXWGFifo.f32 = pos[Y];
				GXWGFifo.f32 = pos[Z];
				GXWGFifo.f32 = 0.0f;
				GXWGFifo.f32 = 0.0f;
				GX::End();
			}
			else
			{
				if ( sc > 256.0f )
				{
					pos[W] = ( pos[W] * 256.0f ) / sc;
				}

				NsVector sr;
				sr.x = screen_right.x * pos[W];
				sr.y = screen_right.y * pos[W];
				sr.z = screen_right.z * pos[W];
				NsVector su;
				su.x = screen_up.x * pos[W];
				su.y = screen_up.y * pos[W];
				su.z = screen_up.z * pos[W];
		
				float v0x = pos[X] - su.x;
				float v0y = pos[Y] - su.y;
				float v0z = pos[Z] - su.z;
	
				float v1x = pos[X] + su.x;
				float v1y = pos[Y] + su.y;
				float v1z = pos[Z] + su.z;

				// Send coordinates.
				GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
					GXWGFifo.f32 = v1x - sr.x;
					GXWGFifo.f32 = v1y - sr.y;
					GXWGFifo.f32 = v1z - sr.z;
					GXWGFifo.f32 = 0.0f;
					GXWGFifo.f32 = 0.0f;

					GXWGFifo.f32 = v1x + sr.x;
					GXWGFifo.f32 = v1y + sr.y;
					GXWGFifo.f32 = v1z + sr.z;
					GXWGFifo.f32 = 1.0f;
					GXWGFifo.f32 = 0.0f;

					GXWGFifo.f32 = v0x + sr.x;
					GXWGFifo.f32 = v0y + sr.y;
					GXWGFifo.f32 = v0z + sr.z;
					GXWGFifo.f32 = 1.0f;
					GXWGFifo.f32 = 1.0f;

					GXWGFifo.f32 = v0x - sr.x;
					GXWGFifo.f32 = v0y - sr.y;
					GXWGFifo.f32 = v0z - sr.z;
					GXWGFifo.f32 = 0.0f;
					GXWGFifo.f32 = 1.0f;
				GX::End();
			}



			// Reduce t by particle interval.
			t -= p_stream->m_interval;
		}
#endif
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcNewParticle::update_position( void )
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
void CNgcNewParticle::plat_update( void )
{

	if (m_params.m_LocalCoord)
	{
		update_position();
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcNewParticle::plat_build( void )
{
	// Adjust some of the stream params to improve performance.
	m_params.m_EmitRate	= m_params.m_EmitRate * 0.25f;
	
	// Initialise streams.
	m_max_streams		= 5;
	m_num_streams		= 0;

	mp_stream			= new CParticleStream[m_max_streams]; 
	mp_newest_stream	= mp_stream + m_max_streams - 1;
	mp_oldest_stream	= mp_stream;
	m_emitting			= false;

	// Create a (semi-transparent) material used to render the mesh.
//	mp_material			= new NxNgc::sMaterial;
//	memset( mp_material, 0, sizeof( NxNgc::sMaterial ));

//	mp_material->m_flags[0]		= MATFLAG_TRANSPARENT | MATFLAG_TEXTURED;
//	mp_material->m_passes		= 1;
//	mp_material->m_alpha_cutoff	= 1;
//	mp_material->m_no_bfc		= true;
//	mp_material->m_color[0][0]	= 0.5f;
//	mp_material->m_color[0][1]	= 0.5f;
//	mp_material->m_color[0][2]	= 0.5f;
//	mp_material->m_color[0][3]	= m_params.m_FixedAlpha * ( 1.0f /  128.0f );
//	mp_material->m_reg_alpha[0]	= NxNgc::GetBlendMode( m_params.m_BlendMode );
//
//	// Get texture.
//	Nx::CTexture*		p_texture;
//	Nx::CNgcTexture*	p_Ngc_texture;
//	mp_material->mp_tex[0]	= NULL;
//	p_texture				= Nx::CTexDictManager::sp_particle_tex_dict->GetTexture( m_params.m_Texture );
//	p_Ngc_texture			= static_cast<Nx::CNgcTexture*>( p_texture );
//	if( p_Ngc_texture )
//	{
//		mp_material->mp_tex[0] = p_Ngc_texture->GetEngineTexture();
//	}

	// Have already set m_Lifetime, which is called m_life in newflat.

	// x-component will be overwritten by vu1 code, so can store midtime there
//	*(float *)&m_systemDmaData.m_tagx = m_params.m_UseMidcolor ?
//										m_params.m_Lifetime * m_params.m_ColorMidpointPct * 0.01f :
//										0.0f;


	// Set Texture & blend mode.
	Nx::CTexture *p_texture;
	Nx::CNgcTexture *p_Ngc_texture;

	p_texture = Nx::CTexDictManager::sp_particle_tex_dict->GetTexture( m_params.m_Texture );
	p_Ngc_texture = static_cast<Nx::CNgcTexture*>( p_texture );
	if ( p_Ngc_texture )
	{
		mp_texture = p_Ngc_texture->GetEngineTexture(); 
	}
	m_blend = get_texture_blend( m_params.m_BlendMode ); 
	m_fix = m_params.m_FixedAlpha;


	// and now a load of redundant duplication of data, which should later be removed...

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
		a_.Set(0,0,0,0);
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
		a_.Set(0,0,0,0);
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
//		float q0 = 100.0f / ( m_params.m_Lifetime * m_params.m_ColorMidpointPct );
//		float q1 = 100.0f / ( m_params.m_Lifetime * ( 100.0f - m_params.m_ColorMidpointPct ));

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
//		float q = 1.0f / m_params.m_Lifetime;

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
//	m_rotation = m_params.m_RotMatrix;
	m_rotation.Identity();

	#if 0
	// invert rotation and apply to spatial params
	// leaving this code a bit shoddy and slow until full transition to new-style params
	Mth::Matrix mat;
	mat=m_rotation;
	mat.Transpose();
	Mth::Vector vec;

	vec = m_systemDmaData.m_p0 + 1.5f * m_systemDmaData.m_s0;
	vec *= mat;
	m_systemDmaData.m_p0 = vec - 1.5f * m_systemDmaData.m_s0;

	vec = m_systemDmaData.m_p1 + 1.5f * m_systemDmaData.m_s1;
	vec *= mat;
	m_systemDmaData.m_p1 = vec - 1.5f * m_systemDmaData.m_s1;

	vec = m_systemDmaData.m_p2 + 1.5f * m_systemDmaData.m_s2;
	vec *= mat;
	m_systemDmaData.m_p2 = vec - 1.5f * m_systemDmaData.m_s2;
	#endif
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcNewParticle::plat_destroy( void )
{
	if( mp_stream )
	{
		delete [] mp_stream;
	}

//	if( mp_material )
//	{
//		delete mp_material;
//	}
}



/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Nx





