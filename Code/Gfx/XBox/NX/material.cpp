#include <xtl.h>
#include <string.h>
#include <core/defines.h>
#include <core/debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file/filesys.h>
#include <gfx/gfxman.h>
#include "gfx/xbox/p_nxtexture.h"
#include "anim_vertdefs.h"
#include "nx_init.h"
#include "material.h"
#include "scene.h"
#include "render.h"

namespace NxXbox
{


uint32						NumMaterials;

static const float pi_over_180 = (float)Mth::PI / 180.0f;



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sUVWibbleParams::sUVWibbleParams( void )
{
	// Zero out the members.
	ZeroMemory( this, sizeof( sUVWibbleParams ));

	// Set the matrix correctly.
	m_UVMatrix[0] = 1.0f;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sUVWibbleParams::~sUVWibbleParams( void )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sMaterial::sMaterial( void )
{
	m_num_wibble_vc_anims		= 0;
	mp_wibble_vc_params			= NULL;
	mp_wibble_vc_colors			= NULL;
	mp_wibble_texture_params	= NULL;
	m_uv_wibble					= false;
	m_texture_wibble			= false;
	m_grass_layers				= 0;
	m_zbias						= 0;
	for( int p = 0; p < MAX_PASSES; ++p )
	{
		m_flags[p]				= 0;
		mp_UVWibbleParams[p]	= NULL;

		m_envmap_tiling[p][0]	= 3.0f;
		m_envmap_tiling[p][1]	= 3.0f;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sMaterial::~sMaterial( void )
{
	if( mp_wibble_vc_params	)
	{
		for( uint32 i = 0; i < m_num_wibble_vc_anims; ++i )
		{
			delete [] mp_wibble_vc_params[i].mp_keyframes;
		}
		delete [] mp_wibble_vc_params;
	}
	if( mp_wibble_vc_colors	)
	{
		delete [] mp_wibble_vc_colors;
	}

	for( int p = 0; p < MAX_PASSES; ++p )
	{
		if( mp_UVWibbleParams[p] )
			delete mp_UVWibbleParams[p];
	}

	if( mp_wibble_texture_params )
	{
		for( uint32 p = 0; p < MAX_PASSES; ++p )
		{
			delete [] mp_wibble_texture_params->mp_keyframes[p];
		}
		delete mp_wibble_texture_params;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sMaterial::figure_wibble_uv( void )
{
	if( m_uv_wibble )
	{
		// Get the time.
		float t = (float)Tmr::GetTime() * 0.001f;

		for( uint32 p = 0; p < m_passes; ++p )
		{
			// Figure out UV offsets for wibbling if required.
			if( m_flags[p] & MATFLAG_UV_WIBBLE )
			{
				if( !( m_flags[p] & MATFLAG_EXPLICIT_UV_WIBBLE ))
				{
					float uoff, voff;
			
					uoff	= ( t * mp_UVWibbleParams[p]->m_UVel ) + ( mp_UVWibbleParams[p]->m_UAmplitude * sinf( mp_UVWibbleParams[p]->m_UFrequency * t + mp_UVWibbleParams[p]->m_UPhase ));
					voff	= ( t * mp_UVWibbleParams[p]->m_VVel ) + ( mp_UVWibbleParams[p]->m_VAmplitude * sinf( mp_UVWibbleParams[p]->m_VFrequency * t + mp_UVWibbleParams[p]->m_VPhase ));

					// Reduce offset mod 16 and put it in the range -8 to +8.
					uoff	+= 8.0f;
					uoff	-= (float)(( Ftoi_ASM( uoff ) >> 4 ) << 4 );
					voff	+= 8.0f;
					voff	-= (float)(( Ftoi_ASM( voff ) >> 4 ) << 4 );

					mp_UVWibbleParams[p]->m_UVMatrix[2]	= ( uoff < 0.0f ) ? ( uoff + 8.0f ) : ( uoff - 8.0f );
					mp_UVWibbleParams[p]->m_UVMatrix[3]	= ( voff < 0.0f ) ? ( voff + 8.0f ) : ( voff - 8.0f );
				}
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sMaterial::figure_wibble_vc( void )
{
	// The vertex color wibble flag is placed in pass 0.
	if( m_flags[0] & MATFLAG_VC_WIBBLE )
	{
		for( uint32 i = 0; i < m_num_wibble_vc_anims; ++i )
		{
			struct sVCWibbleParams	*p_sequence		= mp_wibble_vc_params + i;
			
			// Get phase-shift.
			int						phase_shift		= p_sequence->m_phase;

			// Time parameters.
			int						num_keys		= p_sequence->m_num_keyframes;
			int						start_time		= p_sequence->mp_keyframes[0].m_time;
			int						end_time		= p_sequence->mp_keyframes[num_keys - 1].m_time;
			int						period			= end_time - start_time;
			int						time			= start_time + ( NxXbox::EngineGlobals.render_start_time + phase_shift ) % period;

			// Locate the keyframe.
			int key;
			for( key = num_keys - 1; key >= 0; --key )
			{
				if( time >= p_sequence->mp_keyframes[key].m_time )
				{
					break;
				}
			}
			
			Dbg_Assert( key < ( num_keys - 1 ));

			// Parameter expressing how far we are between between this keyframe and the next.
			float					t				= (float)( time - p_sequence->mp_keyframes[key].m_time ) * ReciprocalEstimateNR_ASM((float)( p_sequence->mp_keyframes[key + 1].m_time - p_sequence->mp_keyframes[key].m_time ));

			// Interpolate the color.
			uint32 red = (uint32)Ftoi_ASM((( 1.0f - t ) * p_sequence->mp_keyframes[key].m_color.r ) + ( t * p_sequence->mp_keyframes[key + 1].m_color.r ));
			uint32 grn = (uint32)Ftoi_ASM((( 1.0f - t ) * p_sequence->mp_keyframes[key].m_color.g ) + ( t * p_sequence->mp_keyframes[key + 1].m_color.g ));
			uint32 blu = (uint32)Ftoi_ASM((( 1.0f - t ) * p_sequence->mp_keyframes[key].m_color.b ) + ( t * p_sequence->mp_keyframes[key + 1].m_color.b ));
			uint32 alp = (uint32)Ftoi_ASM((( 1.0f - t ) * p_sequence->mp_keyframes[key].m_color.a ) + ( t * p_sequence->mp_keyframes[key + 1].m_color.a ));

			// Switch red and blue, and store.
			mp_wibble_vc_colors[i] = ( alp << 24 ) | ( red << 16 ) | ( grn << 8 ) | blu;
		}
	}
}

	
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sMaterial::figure_wibble_texture( void )
{
	// The vertex color wibble flag is placed in pass 0.
	if( m_texture_wibble )
	{
		int current_time = (int)Tmr::GetTime();

		struct sTextureWibbleParams	*p_sequence		= mp_wibble_texture_params;
			
		for( int pass = 0; pass < MAX_PASSES; ++pass )
		{
			if( m_flags[pass] & MATFLAG_PASS_TEXTURE_ANIMATES )
			{
				// Get phase-shift.
				int						phase_shift	= p_sequence->m_phase[pass];

				// Time parameters.
				int						num_keys	= p_sequence->m_num_keyframes[pass];
				int						start_time	= p_sequence->mp_keyframes[pass][0].m_time;
				int						end_time	= p_sequence->mp_keyframes[pass][num_keys - 1].m_time;
				int						period		= end_time - start_time;
				int						time		= start_time + (( current_time + phase_shift ) % period );

				// Keep track of the iterations, if iterations on this animation are required.
				if( p_sequence->m_num_iterations[pass] > 0 )
				{
					int iteration = ( current_time - start_time ) / period;
					if( iteration >= p_sequence->m_num_iterations[pass] )
					{
						// Set the time such that the animation no longer continues.
						time = start_time + ((( period * p_sequence->m_num_iterations[pass] ) + phase_shift ) % period );
					}
				}

				// Locate the keyframe.
				int key;
				for( key = num_keys - 1; key >= 0; --key )
				{
					if( time >= p_sequence->mp_keyframes[pass][key].m_time )
					{
						break;
					}
				}
			
				// Set the texture.
//				Dbg_Assert( p_sequence->mp_keyframes[pass][key].mp_texture );
				mp_tex[pass] = p_sequence->mp_keyframes[pass][key].mp_texture;
			}
		}
	}
}



inline DWORD F2DW( FLOAT f )
{
	return *( ( DWORD *) & f );
}

					
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sMaterial::Submit( void )
{
	// Dummy 8 element uv 'matrix' for custom pipeline.
	static float custom_uv_mat[8] = {	1.0f, 0.0f, 0.0f, 0.0f,
										0.0f, 1.0f, 0.0f, 0.0f };

	// Matrix for UV wibbling texture transform.
	static D3DMATRIX uv_mat = { 1.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 1.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 0.0f };

	// Matrix for env mapping texture transform. Note that the [0][0] and [1][1] elements of the matrix are set up to scale, based on the
	// material pass properties.
	static D3DMATRIX env_mat = { 0.5f,  0.0f, 0.0f, 0.0f,
								 0.0f, -0.5f, 0.0f, 0.0f,
								 0.0f,  0.0f, 0.0f,	0.0f,
								 0.5f,  0.5f, 0.0f, 0.0f };
	
	// Check for material change lockout.
	if( EngineGlobals.material_override )
	{
		return;
	}

	// Set the alpha blend mode.
	set_blend_mode( m_reg_alpha[0] );
	
	// Set the alpha cutoff value.
	set_render_state( RS_ALPHACUTOFF, (uint32)m_alpha_cutoff );
	
	// Set the backface cull mode.
	set_render_state( RS_CULLMODE, m_no_bfc ? D3DCULL_NONE : D3DCULL_CW );

	// Set the z-bias.
	set_render_state( RS_ZBIAS, m_zbias );
	
	// Figure uv, vc and texture wibble updates if required.
	figure_wibble_uv();
	figure_wibble_vc();
	figure_wibble_texture();
	
	// Set specular properties of this material.
	if( m_flags[0] & MATFLAG_SPECULAR )
	{
		if( EngineGlobals.specular_enabled == 0 )
		{
			set_render_state( RS_SPECULARENABLE, 1 );

			// Set the specular material.
			D3DMATERIAL8 test_mat;
			ZeroMemory( &test_mat, sizeof( D3DMATERIAL8 ));
			test_mat.Specular.r	= m_specular_color[0];
			test_mat.Specular.g	= m_specular_color[1];
			test_mat.Specular.b	= m_specular_color[2];
			test_mat.Specular.a	= 1.0f;
			test_mat.Power		= m_specular_color[3];
			D3DDevice_SetMaterial( &test_mat );

			// If using a custom vertex shader, also need to load the specular color and power to vert shader registers here.
			if( NxXbox::EngineGlobals.custom_pipeline_enabled )
			{
				D3DDevice_SetVertexShaderConstantFast( VSCONST_REG_SPECULAR_COLOR_OFFSET, (void*)&m_specular_color[0], 1 );
			}
		}
	}
	else
	{
		if( EngineGlobals.specular_enabled > 0 )
		{
			set_render_state( RS_SPECULARENABLE, 0 );

			// Set the specular material (basically just to set the power to zero).
			D3DMATERIAL8 test_mat;
			ZeroMemory( &test_mat, sizeof( D3DMATERIAL8 ));
			D3DDevice_SetMaterial( &test_mat );
		}
	}

	// Set up the textures if present. This involves setting the texture and palette pointers and addressing mode.
	// Also, for multipass textures, it may require loading of fixed alpha values into specific constant registers.
	uint32 p;
	for( p = 0; p < m_passes; ++p )
	{
		if( !( EngineGlobals.texture_stage_override & ( 1 << p )))
		{
			// Load pass color and fixed alpha values to the pixel shader, if required.
			if( !EngineGlobals.upload_pixel_shader_constants )
			{
				// Do the comparison as 32bit unsigned ints.
				uint32 *p_test0 = (uint32*)&m_color[p][0];
				uint32 *p_test1 = (uint32*)&EngineGlobals.pixel_shader_constants[p * 4];

				if(( p_test0[0] != p_test1[0] ) || ( p_test0[1] != p_test1[1] ) || ( p_test0[2] != p_test1[2] ) || ( p_test0[3] != p_test1[3] ))
				{
					EngineGlobals.upload_pixel_shader_constants = true;
				}
			}

			if( mp_tex[p] )
			{
				set_texture( p, mp_tex[p]->pD3DTexture, mp_tex[p]->pD3DPalette );

				// Set UV adressing mode.
				set_render_state( RS_UVADDRESSMODE0 + p,	m_uv_addressing[p] );

				// Set filtering mode.
//				set_render_state( RS_MINMAGFILTER0 + p,		m_filtering_mode[p] );
						
				// Set MIP lod bias.
				set_render_state( RS_MIPLODBIASPASS0 + p,	*((uint32*)( &m_k[p] )));

				// Deal with bump mapping setup.
				if( m_flags[p] & MATFLAG_BUMP_SIGNED_TEXTURE )
				{
					// Channel of bump texture value (v,u) MUST be signed.
					if( EngineGlobals.color_sign[p] != ( D3DTSIGN_RSIGNED | D3DTSIGN_GSIGNED | D3DTSIGN_BSIGNED ))
					{
						EngineGlobals.color_sign[p] = ( D3DTSIGN_RSIGNED | D3DTSIGN_GSIGNED | D3DTSIGN_BSIGNED );
						D3DDevice_SetTextureStageState( p, D3DTSS_COLORSIGN, D3DTSIGN_RSIGNED | D3DTSIGN_GSIGNED | D3DTSIGN_BSIGNED );
					}
				}
				else
				{
					if( EngineGlobals.color_sign[p] != ( D3DTSIGN_RUNSIGNED | D3DTSIGN_GUNSIGNED | D3DTSIGN_BUNSIGNED ))
					{
						EngineGlobals.color_sign[p] = ( D3DTSIGN_RUNSIGNED | D3DTSIGN_GUNSIGNED | D3DTSIGN_BUNSIGNED );
						D3DDevice_SetTextureStageState( p, D3DTSS_COLORSIGN, D3DTSIGN_RUNSIGNED | D3DTSIGN_GUNSIGNED | D3DTSIGN_BUNSIGNED );
					}
				}

				if( m_flags[p] & MATFLAG_BUMP_LOAD_MATRIX )
				{
					D3DDevice_SetTextureStageState( p, D3DTSS_BUMPENVMAT00, F2DW( EngineGlobals.bump_env_matrix._11 ));
					D3DDevice_SetTextureStageState( p, D3DTSS_BUMPENVMAT01, F2DW( EngineGlobals.bump_env_matrix._13 ));
					D3DDevice_SetTextureStageState( p, D3DTSS_BUMPENVMAT10, F2DW( EngineGlobals.bump_env_matrix._31 ));
					D3DDevice_SetTextureStageState( p, D3DTSS_BUMPENVMAT11, F2DW( EngineGlobals.bump_env_matrix._33 ));
				}

				if(( m_flags[p] & MATFLAG_ENVIRONMENT ) && ( EngineGlobals.allow_envmapping ))
				{
					// Handle environment mapping.
					env_mat.m[0][0] = 0.5f * m_envmap_tiling[p][0];
					env_mat.m[1][1] = 0.5f * m_envmap_tiling[p][1];

					D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)( D3DTS_TEXTURE0 + p ), &env_mat );
					D3DDevice_SetTextureStageState( p, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2 );
					D3DDevice_SetTextureStageState( p, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR | p );
				}
				else if( m_flags[p] & MATFLAG_UV_WIBBLE )
				{
					D3DDevice_SetTextureStageState( p, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2 );
					D3DDevice_SetTextureStageState( p, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU | p );

					if( NxXbox::EngineGlobals.custom_pipeline_enabled )
					{
						// If using a custom vertex shader, need to load the uv matrix here.
						// If using a custom vertex shader, need to load the custom uv matrix here.
						custom_uv_mat[0] =  mp_UVWibbleParams[p]->m_UVMatrix[0];
						custom_uv_mat[1] = -mp_UVWibbleParams[p]->m_UVMatrix[1];
						custom_uv_mat[3] =  mp_UVWibbleParams[p]->m_UVMatrix[2];

						custom_uv_mat[4] =  mp_UVWibbleParams[p]->m_UVMatrix[1];
						custom_uv_mat[5] =	mp_UVWibbleParams[p]->m_UVMatrix[0];
						custom_uv_mat[7] =  mp_UVWibbleParams[p]->m_UVMatrix[3];

						D3DDevice_SetVertexShaderConstantFast( VSCONST_REG_UV_MAT_OFFSET + ( p * 2 ), (void*)&custom_uv_mat[0], 2 );
					}
					else
					{
						// Handle fixed function UV matrix.
						uv_mat._31 = mp_UVWibbleParams[p]->m_UVMatrix[2];
						uv_mat._32 = mp_UVWibbleParams[p]->m_UVMatrix[3];
						
						D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)( D3DTS_TEXTURE0 + p ), &uv_mat );
					}
				}
				else
				{
					if( NxXbox::EngineGlobals.custom_pipeline_enabled )
					{
						// If using a custom vertex shader, need to load the custom uv matrix here.
						custom_uv_mat[0] = 1.0f; custom_uv_mat[1] = 0.0f; custom_uv_mat[3] = 0.0f;
						custom_uv_mat[4] = 0.0f; custom_uv_mat[5] = 1.0f; custom_uv_mat[7] = 0.0f;
						D3DDevice_SetVertexShaderConstantFast( VSCONST_REG_UV_MAT_OFFSET + ( p * 2 ), (void*)&custom_uv_mat[0], 2 );
					}

					// Regular case. Check states here since it is quicker for this common case than setting blindly.
					uint32 tex_coord_index, tex_trans_flags;

					D3DDevice_GetTextureStageState( p, D3DTSS_TEXTURETRANSFORMFLAGS, &tex_trans_flags );
					D3DDevice_GetTextureStageState( p, D3DTSS_TEXCOORDINDEX, &tex_coord_index );

					if( tex_trans_flags != D3DTTFF_DISABLE )
					{
						D3DDevice_SetTextureStageState( p, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
					}
					if( tex_coord_index != ( D3DTSS_TCI_PASSTHRU | p ))
					{
						D3DDevice_SetTextureStageState( p, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU | p );
					}
				}
			}
			else
			{
				set_texture( p, NULL, NULL );
			}
		}
	}

	if( EngineGlobals.upload_pixel_shader_constants )
	{
		CopyMemory( EngineGlobals.pixel_shader_constants, &m_color[0][0], sizeof( float ) * 4 * m_passes );
	}

	// Make sure to set the textures for unused stages to NULL, to reduce texture overhead.
//	for( ; p < 4; ++p )
//	{
//		if( !( EngineGlobals.texture_stage_override & ( 1 << p )))
//		{
//			set_texture( p, NULL, NULL );
//		}
//	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint32 sMaterial::GetIgnoreVertexAlphaPasses( void )
{
	// Return a bitfield with a bit set for any pass that is flagged to ignore vertex alpha.
	uint32 bf = 0;

	for( uint32 p = 0; p < m_passes; ++p )
	{
		if( m_flags[p] & MATFLAG_PASS_IGNORE_VERTEX_ALPHA )
		{
			bf |= ( 1 << p );
		}
	}
	
	return bf;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sMaterial* GetMaterial( uint32 checksum, sScene *p_scene )
{
	if( p_scene->pMaterialTable )
	{
		p_scene->pMaterialTable->IterateStart();
		sMaterial *p_mat = p_scene->pMaterialTable->IterateNext();
		while( p_mat )
		{
			if( p_mat->m_checksum == checksum )
			{
				return p_mat;
			}
			p_mat = p_scene->pMaterialTable->IterateNext();
		}
	}
	return NULL;
}




#define MemoryRead( dst, size, num, src )	CopyMemory(( dst ), ( src ), (( num ) * ( size )));	\
											( src ) += (( num ) * ( size ))

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Lst::HashTable< sMaterial >	*LoadMaterialsFromMemory( void **pp_mem, Lst::HashTable< Nx::CTexture > *p_texture_table )
{
	uint8	*p_data = (uint8*)( *pp_mem );
	uint32	MMAG, MMIN, K, L, NumSeqs, seq, NumKeys;
	
	// Get number of materials.
	uint32 new_materials;
	MemoryRead( &new_materials, sizeof( uint32 ), 1, p_data );
	
	Lst::HashTable< sMaterial >* pMaterialTable;

	// Create table, dynamically sizing it based on the number of new materials.
	uint32 optimal_table_size	= new_materials * 2;
	uint32 test					= 4;
	uint32 size					= 2;

	for( ;; test <<= 1, ++size )
	{
		// Check if this iteration of table size is sufficient, or if we have hit the maximum size.
		if(( optimal_table_size < test ) || ( size >= 12 ))
		{
			pMaterialTable = new Lst::HashTable< sMaterial >( size );
			break;
		}
	}
	
	// Loop over materials.
	for( uint32 i = 0; i < new_materials; ++i )
	{
		// Create new material.
		sMaterial *pMat = new sMaterial;

		// Get material checksum.
		MemoryRead( &pMat->m_checksum, sizeof( uint32 ), 1, p_data );

		// Get material name checksum.
		MemoryRead( &pMat->m_name_checksum, sizeof( uint32 ), 1, p_data );

		// Get number of passes.
		MemoryRead( &pMat->m_passes, sizeof( uint32 ), 1, p_data );

		// Get alpha cutoff value.
		uint32 AlphaCutoff;
		MemoryRead( &AlphaCutoff, sizeof( uint32 ), 1, p_data );
		Dbg_Assert( AlphaCutoff <= 0xFF );
		pMat->m_alpha_cutoff = (uint8)AlphaCutoff;

		// Get sorted flag.
		MemoryRead( &pMat->m_sorted, sizeof( bool ), 1, p_data );
		
		// Get draw order.
		MemoryRead( &pMat->m_draw_order, sizeof( float ), 1, p_data );
		
		// Get single sided flag.
		bool single_sided;
		MemoryRead( &single_sided, sizeof( bool ), 1, p_data );

		// Get backface cull flag.
		MemoryRead( &pMat->m_no_bfc, sizeof( bool ), 1, p_data );

		// Get z-bias value.
		int zbias;
		MemoryRead( &zbias, sizeof( int ), 1, p_data );
		pMat->m_zbias = (uint8)(( zbias > 16 ) ? 16 : zbias );

		// Get grassify flag and (optionally) grassify data.
		bool grassify;
		MemoryRead( &grassify, sizeof( bool ), 1, p_data );
		if( grassify )
		{
			MemoryRead( &pMat->m_grass_height, sizeof( float ), 1, p_data );
			MemoryRead( &pMat->m_grass_layers, sizeof( int ), 1, p_data );
		}

		// Get specular power and (optionally) specular color.
		MemoryRead( &pMat->m_specular_color[3], sizeof( float ), 1, p_data );
		if( pMat->m_specular_color[3] > 0.0f )
		{
			MemoryRead( &pMat->m_specular_color[0], sizeof( float ) * 3, 1, p_data );
		}

		// Neutral under proven otherwise.
		bool neutral_material_color = true;
		for( uint32 pass = 0; pass < pMat->m_passes; ++pass )
		{
			// Get texture checksum.
			uint32 TextureChecksum;
			MemoryRead( &TextureChecksum, sizeof( uint32 ), 1, p_data );

			// Get material flags.
			MemoryRead( &pMat->m_flags[pass], sizeof( uint32 ), 1, p_data );

			// Get pass color flag.
			bool has_color;
			MemoryRead( &has_color, sizeof( bool ), 1, p_data );

			// Get pass color.
			MemoryRead( &pMat->m_color[pass][0], sizeof( float ) * 3, 1, p_data );

			// Check for color being neutral.
			if( neutral_material_color )
			{
				if(( pMat->m_color[pass][0] != 0.5f ) || ( pMat->m_color[pass][1] != 0.5f ) || ( pMat->m_color[pass][2] != 0.5f ))
				{
					neutral_material_color = false;
				}
			}
			
			// Get ALPHA register value.
			uint64 reg_alpha;
			MemoryRead( &reg_alpha, sizeof( uint64 ), 1, p_data );

			uint32	blend_mode		= (uint32)( reg_alpha & 0xFFFFFFUL );
			uint32	fixed_alpha		= (uint32)( reg_alpha >> 32 );
			pMat->m_reg_alpha[pass]	= blend_mode | ( fixed_alpha << 24 );

			// Also calculate the floating point version of the fixed alpha.
			pMat->m_color[pass][3]	= fixed_alpha / 128.0f;

			// Backface cull test - if this is an alpha blended material, turn off backface culling, except
			// where the material has been explicitly flagged as single sided.
			if(( pass == 0 ) && !single_sided && (( pMat->m_reg_alpha[pass] & sMaterial::BLEND_MODE_MASK ) != 0x00 ))
			{
				pMat->m_no_bfc = true;
			}

			// Get UV addressing types.
			uint32 u_addressing, v_addressing;
			MemoryRead( &u_addressing, sizeof( uint32 ), 1, p_data );
			MemoryRead( &v_addressing, sizeof( uint32 ), 1, p_data );
			pMat->m_uv_addressing[pass] = (( v_addressing << 16 ) | u_addressing );
			
			// Get environment map u and v tiling multiples.
			MemoryRead( &pMat->m_envmap_tiling[pass][0], sizeof( float ) * 2, 1, p_data );

			// Get minification and magnification filtering mode.
			MemoryRead( &pMat->m_filtering_mode[pass], sizeof( uint32 ), 1, p_data );

			// Read uv wibble data if present.
			if( pMat->m_flags[pass] & MATFLAG_UV_WIBBLE )
			{
				// Flag that this material wibbles.
				pMat->m_uv_wibble = true;
				
				// Create uv wibble params structure.
				pMat->mp_UVWibbleParams[pass] = new sUVWibbleParams;
				MemoryRead( pMat->mp_UVWibbleParams[pass], sizeof( float ) * 8, 1, p_data );
			}

			// Read vertex color wibble data.
			if(( pass == 0 ) && ( pMat->m_flags[0] & MATFLAG_VC_WIBBLE ))
			{
				MemoryRead( &NumSeqs, sizeof( uint32 ), 1, p_data );
				pMat->m_num_wibble_vc_anims = NumSeqs;

				// Create sequence data array.
				pMat->mp_wibble_vc_params = new sVCWibbleParams[NumSeqs];
				
				// Create resultant color array.
				pMat->mp_wibble_vc_colors = new D3DCOLOR[NumSeqs];

				for( seq = 0; seq < NumSeqs; ++seq )
				{ 
					MemoryRead( &NumKeys, sizeof( uint32 ), 1, p_data );

					int phase;
					MemoryRead( &phase, sizeof( int ), 1, p_data );

					// Create array for keyframes.
					pMat->mp_wibble_vc_params[seq].m_num_keyframes	= NumKeys;
					pMat->mp_wibble_vc_params[seq].m_phase			= phase;
					pMat->mp_wibble_vc_params[seq].mp_keyframes		= new sVCWibbleKeyframe[NumKeys];

					// Read keyframes into array.
					MemoryRead( pMat->mp_wibble_vc_params[seq].mp_keyframes, sizeof( sVCWibbleKeyframe ), NumKeys, p_data );
				}
			}

			// Read texture wibble data.
			if( pMat->m_flags[pass] & MATFLAG_PASS_TEXTURE_ANIMATES )
			{
				// Create the texture wibble structure if not created yet.
				if( pMat->mp_wibble_texture_params == NULL )
				{
					pMat->mp_wibble_texture_params = new NxXbox::sTextureWibbleParams;
					ZeroMemory( pMat->mp_wibble_texture_params, sizeof( NxXbox::sTextureWibbleParams ));

					// Flag the material as having texture wibble.
					pMat->m_texture_wibble = true;
				}

				int num_keyframes, period, iterations, phase;
				MemoryRead( &num_keyframes, sizeof( int ), 1, p_data );
				MemoryRead( &period, sizeof( int ), 1, p_data );			// This value is currently discarded.
				MemoryRead( &iterations, sizeof( int ), 1, p_data );
				MemoryRead( &phase, sizeof( int ), 1, p_data );

				Dbg_Assert( num_keyframes > 0 );

				pMat->mp_wibble_texture_params->m_num_keyframes[pass]	= num_keyframes;
				pMat->mp_wibble_texture_params->m_phase[pass]			= phase;
				pMat->mp_wibble_texture_params->m_num_iterations[pass]	= iterations;
				pMat->mp_wibble_texture_params->mp_keyframes[pass]		= new NxXbox::sTextureWibbleKeyframe[num_keyframes];

				for( int ati = 0; ati < num_keyframes; ++ati )
				{
					MemoryRead( &pMat->mp_wibble_texture_params->mp_keyframes[pass][ati].m_time,		sizeof( uint32 ), 1, p_data );

					// Read the texture checksum.
					uint32 cs;
					MemoryRead( &cs, sizeof( uint32 ), 1, p_data );

					// Set the TextureChecksum variable so the mp_tex member will get populated.
					if( ati == 0 )
					{
						TextureChecksum = cs;
					}

					// Resolve the checksum to a texture pointer.
					Nx::CXboxTexture *p_xbox_texture = static_cast<Nx::CXboxTexture*>( p_texture_table->GetItem( cs ));
					pMat->mp_wibble_texture_params->mp_keyframes[pass][ati].mp_texture = ( p_xbox_texture ) ? p_xbox_texture->GetEngineTexture() : NULL;
				}
			}

			if( TextureChecksum )
			{
				// If textured, resolve texture checksum...
				Nx::CXboxTexture	*p_xbox_texture	= static_cast<Nx::CXboxTexture*>( p_texture_table->GetItem( TextureChecksum ));
				sTexture			*mp_tex			= ( p_xbox_texture ) ? p_xbox_texture->GetEngineTexture() : NULL;

				// Bail if checksum not found.
				if( mp_tex == NULL )
				{
					Dbg_Message( "error: couldn't find texture checksum %08X\n", TextureChecksum );
					pMat->mp_tex[pass] = NULL;
				}
				else
				{
					// Set texture pointer.
					pMat->mp_tex[pass] = mp_tex;
				}

				// Get mipmap info.
				MemoryRead( &MMAG, sizeof( uint32 ), 1, p_data );
				MemoryRead( &MMIN, sizeof( uint32 ), 1, p_data );
				MemoryRead( &K, sizeof( uint32 ), 1, p_data );
				MemoryRead( &L, sizeof( uint32 ), 1, p_data );
				
				// Default PS2 value for K appears to be -8.0f - we are interested in deviations from this value.
				pMat->m_k[pass]	= ( *(float*)&K ) + 8.0f;
				
				// Dave note 09/03/02 - having MIPs selected earlier than normal seems to cause some problems, since Xbox
				// MIP selection is so different to Ps2. Limit the k value such that Xbox can never select smaller MIPs
				// earlier than it would do by default.
				if( pMat->m_k[pass] > 0.0f )
				{
					pMat->m_k[pass] = 0.0f;
				}
			}
			else
			{
				// ...otherwise just step past mipmap info.
				pMat->mp_tex[pass] = NULL;
				p_data += 16;
			}
		}

		// Set the no material color flag if appropriate.
		if( neutral_material_color )
		{
			pMat->m_flags[0] |= MATFLAG_NO_MAT_COL_MOD;
		}
		
		// Set the specular flag if appropriate.
		if( pMat->m_specular_color[3] > 0.0f )
		{
			pMat->m_flags[0] |= MATFLAG_SPECULAR;
		}

		// There is a problem adding materials with the same checksum into the table.
		// This could happen when materials in different scenes share the same name.
		// It also happens for the dummy material (checksum == 0), so for now just special-case
		// that one.
		if( pMat->m_checksum == 0 )
		{
			if( !pMaterialTable->GetItem( 0 ))
			{
				pMaterialTable->PutItem( pMat->m_checksum, pMat );
			}
		}
		else		
		{
			if( pMaterialTable->GetItem( pMat->m_checksum ))
			{
				Dbg_MsgAssert( 0, ( "NXXBOX ERROR: duplicate material: %x\n", pMat->m_checksum ));
			}
			else
			{
				pMaterialTable->PutItem( pMat->m_checksum, pMat );
			}
		}
	}

	// Set the data pointer to the new position on return.
	*pp_mem = p_data;

	return pMaterialTable;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Lst::HashTable< sMaterial >	*LoadMaterials( void *p_FH, Lst::HashTable< Nx::CTexture > *p_texture_table )
{
	uint32 MMAG, MMIN, K, L, NumSeqs, seq, NumKeys;
	
	// Get number of materials.
	uint32 new_materials;
	File::Read( &new_materials, sizeof( uint32 ), 1, p_FH );
	
	Lst::HashTable< sMaterial >* pMaterialTable;

	// Create table, dynamically sizing it based on the number of new materials.
	uint32 optimal_table_size	= new_materials * 2;
	uint32 test					= 4;
	uint32 size					= 2;

	for( ;; test <<= 1, ++size )
	{
		// Check if this iteration of table size is sufficient, or if we have hit the maximum size.
		if(( optimal_table_size < test ) || ( size >= 12 ))
		{
			pMaterialTable = new Lst::HashTable< sMaterial >( size );
			break;
		}
	}
	
	// Loop over materials.
	for( uint32 i = 0; i < new_materials; ++i )
	{
		// Create new material.
		sMaterial *pMat = new sMaterial;

		// Get material checksum.
		File::Read( &pMat->m_checksum, sizeof( uint32 ), 1, p_FH );

		// Get material name checksum.
		File::Read( &pMat->m_name_checksum, sizeof( uint32 ), 1, p_FH );

		// Get number of passes.
		File::Read( &pMat->m_passes, sizeof( uint32 ), 1, p_FH );

		// Get alpha cutoff value.
		uint32 AlphaCutoff;
		File::Read( &AlphaCutoff, sizeof( uint32 ), 1, p_FH );
		pMat->m_alpha_cutoff = (uint8)AlphaCutoff;

		// Get sorted flag.
		File::Read( &pMat->m_sorted, sizeof( bool ), 1, p_FH );
		
		// Get draw order.
		File::Read( &pMat->m_draw_order, sizeof( float ), 1, p_FH );
		
		// Get single sided flag.
		bool single_sided;
		File::Read( &single_sided, sizeof( bool ), 1, p_FH );

		// Get backface cull flag.
		File::Read( &pMat->m_no_bfc, sizeof( bool ), 1, p_FH );

		// Get z-bias value.
		int zbias;
		File::Read( &zbias, sizeof( int ), 1, p_FH );
		pMat->m_zbias = (uint8)(( zbias > 16 ) ? 16 : zbias );

		// Get grassify flag and (optionally) grassify data.
		bool grassify;
		File::Read( &grassify, sizeof( bool ), 1, p_FH );
		if( grassify )
		{
			File::Read( &pMat->m_grass_height, sizeof( float ), 1, p_FH );
			File::Read( &pMat->m_grass_layers, sizeof( int ), 1, p_FH );
		}

		// Get specular power and (optionally) specular color.
		File::Read( &pMat->m_specular_color[3], sizeof( float ), 1, p_FH );
		if( pMat->m_specular_color[3] > 0.0f )
		{
			File::Read( &pMat->m_specular_color[0], sizeof( float ) * 3, 1, p_FH );
		}

		// Neutral under proven otherwise.
		bool neutral_material_color = true;
		for( uint32 pass = 0; pass < pMat->m_passes; ++pass )
		{
			// Get texture checksum.
			uint32 TextureChecksum;
			File::Read( &TextureChecksum, sizeof( uint32 ), 1, p_FH );

			// Get material flags.
			File::Read( &pMat->m_flags[pass], sizeof( uint32 ), 1, p_FH );

			// Get pass color flag.
			bool has_color;
			File::Read( &has_color, sizeof( bool ), 1, p_FH );

			// Get pass color.
			File::Read( &pMat->m_color[pass][0], sizeof( float ) * 3, 1, p_FH );

			// Check for color being neutral.
			if( neutral_material_color )
			{
				if(( pMat->m_color[pass][0] != 0.5f ) || ( pMat->m_color[pass][1] != 0.5f ) || ( pMat->m_color[pass][2] != 0.5f ))
				{
					neutral_material_color = false;
				}
			}
			
			// Get ALPHA register value.
			uint64 reg_alpha;
			File::Read( &reg_alpha, sizeof( uint64 ), 1, p_FH );

			uint32	blend_mode		= (uint32)( reg_alpha & 0xFFFFFFUL );
			uint32	fixed_alpha		= (uint32)( reg_alpha >> 32 );
			pMat->m_reg_alpha[pass]	= blend_mode | ( fixed_alpha << 24 );

			// Also calculate the floating point version of the fixed alpha.
			pMat->m_color[pass][3]	= fixed_alpha / 128.0f;

			// Backface cull test - if this is an alpha blended material, turn off backface culling, except
			// where the material has been explicitly flagged as single sided.
			if(( pass == 0 ) && !single_sided && (( pMat->m_reg_alpha[pass] & sMaterial::BLEND_MODE_MASK ) != 0x00 ))
			{
				pMat->m_no_bfc = true;
			}

			// Get UV addressing types.
			uint32 u_addressing, v_addressing;
			File::Read( &u_addressing, sizeof( uint32 ), 1, p_FH );
			File::Read( &v_addressing, sizeof( uint32 ), 1, p_FH );
			pMat->m_uv_addressing[pass] = (( v_addressing << 16 ) | u_addressing );
			
			// Get environment map u and v tiling multiples.
			File::Read( &pMat->m_envmap_tiling[pass][0], sizeof( float ) * 2, 1, p_FH );

			// Get minification and magnification filtering mode.
			File::Read( &pMat->m_filtering_mode[pass], sizeof( uint32 ), 1, p_FH );

			// Read uv wibble data if present.
			if( pMat->m_flags[pass] & MATFLAG_UV_WIBBLE )
			{
				// Flag that this material wibbles.
				pMat->m_uv_wibble = true;
				
				// Create uv wibble params structure.
				pMat->mp_UVWibbleParams[pass] = new sUVWibbleParams;
				File::Read( pMat->mp_UVWibbleParams[pass], sizeof( float ) * 8, 1, p_FH );
			}

			// Read vertex color wibble data.
			if(( pass == 0 ) && ( pMat->m_flags[0] & MATFLAG_VC_WIBBLE ))
			{
				File::Read( &NumSeqs, sizeof( uint32 ), 1, p_FH );
				pMat->m_num_wibble_vc_anims = NumSeqs;

				// Create sequence data array.
				pMat->mp_wibble_vc_params = new sVCWibbleParams[NumSeqs];
				
				// Create resultant color array.
				pMat->mp_wibble_vc_colors = new D3DCOLOR[NumSeqs];

				for( seq = 0; seq < NumSeqs; ++seq )
				{ 
					File::Read( &NumKeys, sizeof( uint32 ), 1, p_FH );

					int phase;
					File::Read( &phase, sizeof( int ), 1, p_FH );

					// Create array for keyframes.
					pMat->mp_wibble_vc_params[seq].m_num_keyframes	= NumKeys;
					pMat->mp_wibble_vc_params[seq].m_phase			= phase;
					pMat->mp_wibble_vc_params[seq].mp_keyframes		= new sVCWibbleKeyframe[NumKeys];

					// Read keyframes into array.
					File::Read( pMat->mp_wibble_vc_params[seq].mp_keyframes, sizeof( sVCWibbleKeyframe ), NumKeys, p_FH );
				}
			}

			// Read texture wibble data.
			if( pMat->m_flags[pass] & MATFLAG_PASS_TEXTURE_ANIMATES )
			{
				// Create the texture wibble structure if not created yet.
				if( pMat->mp_wibble_texture_params == NULL )
				{
					pMat->mp_wibble_texture_params = new NxXbox::sTextureWibbleParams;
					ZeroMemory( pMat->mp_wibble_texture_params, sizeof( NxXbox::sTextureWibbleParams ));

					// Flag the material as having texture wibble.
					pMat->m_texture_wibble = true;
				}

				int num_keyframes, period, iterations, phase;
				File::Read( &num_keyframes, sizeof( int ), 1, p_FH );
				File::Read( &period, sizeof( int ), 1, p_FH );			// This value is currently discarded.
				File::Read( &iterations, sizeof( int ), 1, p_FH );
				File::Read( &phase, sizeof( int ), 1, p_FH );

				Dbg_Assert( num_keyframes > 0 );

				pMat->mp_wibble_texture_params->m_num_keyframes[pass]	= num_keyframes;
				pMat->mp_wibble_texture_params->m_phase[pass]			= phase;
				pMat->mp_wibble_texture_params->m_num_iterations[pass]	= iterations;
				pMat->mp_wibble_texture_params->mp_keyframes[pass]		= new NxXbox::sTextureWibbleKeyframe[num_keyframes];

				for( int ati = 0; ati < num_keyframes; ++ati )
				{
					File::Read( &pMat->mp_wibble_texture_params->mp_keyframes[pass][ati].m_time,		sizeof( uint32 ), 1, p_FH );

					// Read the texture checksum.
					uint32 cs;
					File::Read( &cs, sizeof( uint32 ), 1, p_FH );

					// Set the TextureChecksum variable so the mp_tex member will get populated.
					if( ati == 0 )
					{
						TextureChecksum = cs;
					}

					// Resolve the checksum to a texture pointer.
					Nx::CXboxTexture *p_xbox_texture = static_cast<Nx::CXboxTexture*>( p_texture_table->GetItem( cs ));
					pMat->mp_wibble_texture_params->mp_keyframes[pass][ati].mp_texture = ( p_xbox_texture ) ? p_xbox_texture->GetEngineTexture() : NULL;
				}
			}

			if( TextureChecksum )
			{
				// If textured, resolve texture checksum...
				Nx::CXboxTexture	*p_xbox_texture	= static_cast<Nx::CXboxTexture*>( p_texture_table->GetItem( TextureChecksum ));
				sTexture			*mp_tex			= ( p_xbox_texture ) ? p_xbox_texture->GetEngineTexture() : NULL;

				// Bail if checksum not found.
				if( mp_tex == NULL )
				{
					Dbg_Message( "error: couldn't find texture checksum %08X\n", TextureChecksum );
					pMat->mp_tex[pass] = NULL;
				}
				else
				{
					// Set texture pointer.
					pMat->mp_tex[pass] = mp_tex;
				}

				// Get mipmap info.
				File::Read( &MMAG, sizeof( uint32 ), 1, p_FH );
				File::Read( &MMIN, sizeof( uint32 ), 1, p_FH );
				File::Read( &K, sizeof( uint32 ), 1, p_FH );
				File::Read( &L, sizeof( uint32 ), 1, p_FH );
				
				// Default PS2 value for K appears to be -8.0f - we are interested in deviations from this value.
				pMat->m_k[pass]	= ( *(float*)&K ) + 8.0f;
				
				// Dave note 09/03/02 - having MIPs selected earlier than normal seems to cause some problems, since Xbox
				// MIP selection is so different to Ps2. Limit the k value such that Xbox can never select smaller MIPs
				// earlier than it would do by default.
				if( pMat->m_k[pass] > 0.0f )
				{
					pMat->m_k[pass] = 0.0f;
				}
			}
			else
			{
				// ...otherwise just step past mipmap info.
				pMat->mp_tex[pass] = NULL;
				File::Seek( p_FH, 16, SEEK_CUR );
			}
		}

		// Set the no material color flag if appropriate.
		if( neutral_material_color )
		{
			pMat->m_flags[0] |= MATFLAG_NO_MAT_COL_MOD;
		}
		
		// Set the specular flag if appropriate.
		if( pMat->m_specular_color[3] > 0.0f )
		{
			pMat->m_flags[0] |= MATFLAG_SPECULAR;
		}

		// There is a problem adding materials with the same checksum into the table.
		// This could happen when materials in different scenes share the same name.
		// It also happens for the dummy material (checksum == 0), so for now just special-case
		// that one.
		if( pMat->m_checksum == 0 )
		{
			if( !pMaterialTable->GetItem( 0 ))
			{
				pMaterialTable->PutItem( pMat->m_checksum, pMat );
			}
		}
		else		
		{
			if( pMaterialTable->GetItem( pMat->m_checksum ))
			{
				Dbg_MsgAssert( 0, ( "NXXBOX ERROR: duplicate material: %x\n", pMat->m_checksum ));
			}
			else
			{
				pMaterialTable->PutItem( pMat->m_checksum, pMat );
			}
		}
	}
	return pMaterialTable;
}








} // namespace NxXbox

