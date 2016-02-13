#include <string.h>
#include <sys/timer.h>
#include <core/defines.h>
#include <core/debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file/filesys.h>
//#include "import.h"
#include "material.h"
#include "scene.h"
#include "render.h"
#include "gfx/ngc/p_nxtexture.h"
#include "nx_init.h"

#include <sys/ngc/p_display.h>
#include <sys/ngc/p_gx.h>

//int gMatBytes = 0;

extern u16 colorMap[];

bool gCorrectColor = false;

extern bool gMeshUseCorrection;
extern bool gMeshUseAniso;

int g_mat = 0;

namespace NxNgc
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
//sMaterial::sMaterial( void )
//{
//	m_num_wibble_vc_anims	= 0;
//	mp_wibble_vc_params		= NULL;
//	mp_wibble_vc_colors		= NULL;
//	m_uv_wibble				= false;
//	for( int p = 0; p < MAX_PASSES; ++p )
//	{
//		Flags[p]			 = 0;
//		mp_UVWibbleParams[p] = NULL;
//	}
//}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
//sMaterial::~sMaterial( void )
//{
////	if( mp_wibble_vc_params	)
////	{
////		for( uint32 i = 0; i < m_num_wibble_vc_anims; ++i )
////		{
////			delete [] mp_wibble_vc_params[i].mp_keyframes;
////		}
////		delete [] mp_wibble_vc_params;
////	}
////	if( mp_wibble_vc_colors	)
////	{
////		delete [] mp_wibble_vc_colors;
////	}
//}



///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void sMaterial::figure_wibble_uv( void )
//{
//	float uoff, voff, t;
//
//	m_wibble_uv_time	= (float)Tmr::GetTime();
//	t					= m_wibble_uv_time * 0.001f;
//	m_wibble_uv_update	= 0;
//
//	for( uint32 p = 0; p < Passes; ++p )
//	{
//		// Figure out UV offsets for wibbling if required.
//		if( Flags[p] & MATFLAG_UV_WIBBLE )
//		{
//			m_wibble_uv_update |= ( 1 << p );
//		
//			uoff	= ( t * m_pUVControl->UVWibbleParams[p].m_UVel ) + ( m_pUVControl->UVWibbleParams[p].m_UAmplitude * sinf( m_pUVControl->UVWibbleParams[p].m_UFrequency * t + m_pUVControl->UVWibbleParams[p].m_UPhase ));
////			uoff	-= (float)(int)uoff;
//	
//			voff	= ( t * m_pUVControl->UVWibbleParams[p].m_VVel ) + ( m_pUVControl->UVWibbleParams[p].m_VAmplitude * sinf( m_pUVControl->UVWibbleParams[p].m_VFrequency * t + m_pUVControl->UVWibbleParams[p].m_VPhase ));
////			voff	-= (float)(int)voff;
//
//			m_pUVControl->UVWibbleOffset[p][0]	= uoff;
//			m_pUVControl->UVWibbleOffset[p][1]	= voff;
//		}
//	}
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void sMaterial::figure_wibble_vc( void )
//{
//	// The vertex color wibble flag is placed in pass 0.
//	if( Flags[0] & MATFLAG_VC_WIBBLE )
//	{
//		int current_time = (int)Tmr::GetTime();
//
//		for( uint32 i = 0; i < m_num_wibble_vc_anims; ++i )
//		{
//			struct sVCWibbleParams	*p_sequence		= mp_wibble_vc_params + i;
//			
//			// Get phase-shift.
//			int						phase_shift		= p_sequence->m_phase;
//
//			// Time parameters.
//			int						num_keys		= p_sequence->m_num_keyframes;
//			int						start_time		= p_sequence->mp_keyframes[0].m_time;
//			int						end_time		= p_sequence->mp_keyframes[num_keys - 1].m_time;
//			int						period			= end_time - start_time;
//			int						time			= start_time + (( current_time + phase_shift ) % period );
//
//			// Locate the keyframe.
//			int key;
//			for( key = num_keys - 1; key >= 0; --key )
//			{
//				if( time >= p_sequence->mp_keyframes[key].m_time )
//				{
//					break;
//				}
//			}
//			
//			// Parameter expressing how far we are between between this keyframe and the next.
//			float					t				= (float)( time - start_time ) / (float)period;
//
//			// Interpolate the color.
//			GXColor	rgba;
//			rgba.r = (uint8)((( 1.0f - t ) * p_sequence->mp_keyframes[key].m_color.r ) + ( t * p_sequence->mp_keyframes[key + 1].m_color.r ));
//			rgba.g = (uint8)((( 1.0f - t ) * p_sequence->mp_keyframes[key].m_color.g ) + ( t * p_sequence->mp_keyframes[key + 1].m_color.g ));
//			rgba.b = (uint8)((( 1.0f - t ) * p_sequence->mp_keyframes[key].m_color.b ) + ( t * p_sequence->mp_keyframes[key + 1].m_color.b ));
//			rgba.a = (uint8)((( 1.0f - t ) * p_sequence->mp_keyframes[key].m_color.a ) + ( t * p_sequence->mp_keyframes[key + 1].m_color.a ));
//
//			mp_wibble_vc_colors[i] = *((uint32*)&rgba);
//		}
//	}
//}
//	
//static int	stage_id = 0;	// 0-15
//static int	tev_id = 0;					// 0-3
//static int	uv_id = 0;					// 0-7
//static int	map_id = 0;					// 0-3
//static int	layer_id = 1;					// 1-3
//static bool correct_color = false;
//
//static void multi_start ( int passes )
//{
//	// Set everything to 0.
//	stage_id	= 0;
//	tev_id		= 0;
//	uv_id		= 0;
//	map_id		= 0;
//	layer_id	= 1;
//
//	// Don't forget these!
////	GXSetNumChans(2);
////	GX::SetNumTexGens(2);
////	GX::SetNumTevStages(2);
//
//	if ( gCorrectColor && gMeshUseCorrection )
//	{
//		GXTexObj	texObj;
//		GXInitTexObj( &texObj, colorMap, 256, 256, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE );
//		GXLoadTexObj( &texObj, GX_TEXMAP7 );
//
//		// Tev 0: Use rasterization lookup table (RG)
//		// Note: Blue = 0 and Alpha = 0;
//		GX::SetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//		GX::SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
//		GX::SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
//		GX::SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG1);
//		GX::SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG1);
//		stage_id++;
//
//		// Tev 1: Use rasterization lookup table (BA),
//		// then add previous tev (RG + BA)
//		GX::SetTevSwapMode( GX_TEVSTAGE1, GX_TEV_SWAP0, GX_TEV_SWAP3 );
//		GX::SetTevColorIn(GX_TEVSTAGE1, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C1);
//		GX::SetTevAlphaIn(GX_TEVSTAGE1, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A1);
//		GX::SetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG2);
//		GX::SetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG2);
//		stage_id++;
//		correct_color = true;
//	}
//	else
//	{
//		correct_color = false;
//	}
//
////	// Create 2 dummy stages.
////	GXTevStageID s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
////
////	GX::SetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR_NULL );
////	GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
////	GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
////	GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
////	GX::SetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
////	GX::SetTevColorIn ( s_id, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );
////	stage_id++;
////
////	s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
////	
////	GX::SetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR0A0 );
////	GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
////	GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG2 );
////	GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG2 );
////	GX::SetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA );
////	GX::SetTevColorIn ( s_id, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC );
////	stage_id++;
//
////	if ( !gCorrectColor || !gMeshUseCorrection )
////	{
//////		// Create 2 dummy stages.
//////		GXTevStageID s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
//////
//////		GX::SetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR0A0 );
//////		GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//////		GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG2 );
//////		GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG2 );
//////		GX::SetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA );
//////		GX::SetTevColorIn ( s_id, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC );
//////		stage_id++;
////		correct_color = false;
////	}
//}
//
//static void multi_add_layer ( BlendModes blend, int raw_fix )
//{
//	// Set inputs.
//	GXTevStageID s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
//	
//	int reg_id = layer_id;	//( layer_id > 1 ) ? 1 : layer_id;	//( tev_id == 3 ) ? 2 : tev_id;		// CPREV, C0, C1, C0.
//  //  int reg_id = layer_id;	//( layer_id == 3 ) ? 2 : layer_id;		// CPREV, C0, C1, C0.
//
////	GXTevAlphaArg newa = (GXTevAlphaArg)(((int)GX_CA_APREV)+layer_id);
//	GXTevColorArg newc = (GXTevColorArg)(((int)GX_CC_CPREV)+(reg_id*2));
//	GXTevColorArg newca= (GXTevColorArg)(((int)GX_CC_APREV)+(reg_id*2));
//
//
//    // out_reg = (d (op) ((1.0 - c)*a + c*b) + bias) * scale;
//
//	int fix = ( raw_fix >= 128 ) ? 255 : raw_fix * 2;
//
//	switch ( blend ) {
//		case vBLEND_MODE_ADD:
//		case vBLEND_MODE_ADD_FIXED:
//			// Add using texture or fixed alpha.
//			GX::SetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR_NULL );
//            GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//			GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV );
//			GX::SetTevColorIn ( s_id, GX_CC_ZERO, newc, newca, GX_CC_CPREV );
//			stage_id++;
//			break;
//		case vBLEND_MODE_SUBTRACT:
//		case vBLEND_MODE_SUB_FIXED:
//			// Subtract using texture or fixed alpha.
//			GX::SetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR_NULL );
//            GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//			GX::SetTevAlphaOp(s_id, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevColorOp(s_id, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV );
//			GX::SetTevColorIn ( s_id, GX_CC_ZERO, newc, newca, GX_CC_CPREV );
//			stage_id++;
//			break;
//		case vBLEND_MODE_BLEND:
//		case vBLEND_MODE_BLEND_FIXED:
//			// Blend using texture or fixed alpha.
//			GX::SetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR_NULL );
//            GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//			GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV );
//			GX::SetTevColorIn ( s_id, GX_CC_CPREV, newc, newca, GX_CC_ZERO );
//			stage_id++;
//			break;
//		case vBLEND_MODE_MODULATE:
//			// Modulate current layer with previous layer.
//			GX::SetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR_NULL );
//            GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//			GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV );
//			GX::SetTevColorIn ( s_id, GX_CC_ZERO, newca, GX_CC_CPREV, GX_CC_ZERO );
//			stage_id++;
//			break;
//		case vBLEND_MODE_MODULATE_FIXED:
//			// Modulate current layer with fixed alpha.
//			GX::SetTevKAlphaSel( s_id, (GXTevKAlphaSel)(((int)GX_TEV_KASEL_K0_A)+layer_id) );
//			GX::SetTevKColorSel( s_id, (GXTevKColorSel)(((int)GX_TEV_KCSEL_K0_A)+layer_id) );
//			GX::SetTevKColor( (GXTevKColorID)(((int)GX_KCOLOR0)+layer_id), (GXColor){fix,fix,fix,fix} );
//			GX::SetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR_NULL );
//            GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//			GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV );
//			GX::SetTevColorIn ( s_id, GX_CC_ZERO, GX_CC_CPREV, GX_CC_KONST, GX_CC_ZERO );
//			stage_id++;
//			break;
//		case vBLEND_MODE_BRIGHTEN:
//			// Modulate current layer with previous layer, & add current layer.
//			GX::SetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR_NULL );
//            GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//			GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV );
//			GX::SetTevColorIn ( s_id, GX_CC_ZERO, newca, GX_CC_CPREV, GX_CC_CPREV );
//			stage_id++;
//			break;
//		case vBLEND_MODE_BRIGHTEN_FIXED:
//			// Modulate current layer with fixed alpha, & add current layer.
//			GX::SetTevKAlphaSel( s_id, (GXTevKAlphaSel)(((int)GX_TEV_KASEL_K0_A)+layer_id) );
//			GX::SetTevKColorSel( s_id, (GXTevKColorSel)(((int)GX_TEV_KCSEL_K0_A)+layer_id) );
//			GX::SetTevKColor( (GXTevKColorID)(((int)GX_KCOLOR0)+layer_id), (GXColor){fix,fix,fix,fix} );
//			GX::SetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR_NULL );
//            GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//			GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV );
//			GX::SetTevColorIn ( s_id, GX_CC_ZERO, GX_CC_CPREV, GX_CC_KONST, GX_CC_CPREV );
//			stage_id++;
//			break;
//		case vBLEND_MODE_DIFFUSE:
//		default:
//			// Replace with this texture. Shouldn't ever be used, but here for compatibility.
//			GX::SetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR_NULL );
//            GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//			GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV );
//			GX::SetTevColorIn ( s_id, newc, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );
//			stage_id++;
//			break;
//	}
////	layer_id++;
//}
//
//static void multi_add_texture ( sTexture * p_texture, GXColor matcol, GXTexWrapMode umode, GXTexWrapMode vmode, BlendModes blend, int fix, uint32 flags, char * p_uv_slot, float k )
//{
//	if ( map_id >= 8 ) return;
//	
//	void * p_tex_data = p_texture->pTexelData;
//	void * p_alpha_data = p_texture->has_alpha ? p_texture->pAlphaData : NULL;
//	int width = p_texture->ActualWidth;
//	int height = p_texture->ActualHeight;
//	int levels = p_texture->Levels;
//	int reg_id = ( tev_id > 1 ) ? 1 : tev_id;	//( tev_id == 3 ) ? 2 : tev_id;		// CPREV, C0, C1, C0.
//	
//	GXTevAlphaArg rasa;
//	GXTevColorArg rasc;
//	if ( correct_color )
//	{
//		rasa = GX_CA_A2;
//		rasc = GX_CC_C2;
//	}
//	else
//	{
//		rasa = GX_CA_RASA;
//		rasc = GX_CC_RASC;
//	}
//
//	int cur_map = map_id + 1;
//	int alpha_map = -1;
//	bool uniqueMaterialColor = false;
//
//	if ( ( matcol.r == 128 ) && ( matcol.g == 128 ) && ( matcol.b == 128 ) && ( matcol.a == 255 ) )
//	{
//		uniqueMaterialColor = false;
//	}
//	else
//	{
//		uniqueMaterialColor = true;
//	}
//
//	if ( p_alpha_data )
//	{
//		switch ( blend )
//		{
//			case vBLEND_MODE_ADD:
//			case vBLEND_MODE_SUBTRACT:
//			case vBLEND_MODE_BLEND:
//			case vBLEND_MODE_BRIGHTEN:
//			case vBLEND_MODE_MODULATE:
//			case vBLEND_MODE_DIFFUSE:
//				if ( cur_map < 8 )
//				{
//					alpha_map = cur_map;
//					cur_map++;
//				}
//				break;
//			default:
//				break;
//		}
//	}
//
//	GXTexObj	texObj;
//	GXInitTexObj( &texObj, p_tex_data, width, height, ((GXTexFmt)p_texture->format), umode, vmode, levels > 1 ? GX_TRUE : GX_FALSE );
//	if ( levels > 1 )
//	{
//		if ( gMeshUseAniso && ( tev_id == 0 ) )
//		{
//			// If we're correcting the color, we also want ANISO_4.
//			GXInitTexObjLOD( &texObj, GX_LIN_MIP_LIN, GX_LINEAR, 0.0f, levels - 1, k, GX_FALSE, GX_TRUE, GX_ANISO_4 );
//		}
//		else
//		{
//			GXInitTexObjLOD( &texObj, GX_LIN_MIP_LIN, GX_LINEAR, 0.0f, levels - 1, k, GX_FALSE, GX_TRUE, GX_ANISO_1 );
//		}
//	}
//	else
//	{
//		GXInitTexObjLOD( &texObj, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, k, GX_FALSE, GX_TRUE, GX_ANISO_1 );
//	}
//	GXLoadTexObj( &texObj, (GXTexMapID)(((int)GX_TEXMAP0)+map_id) );
//
//	if ( alpha_map >= 0 )
//	{
//		switch ( blend )
//		{
//			case vBLEND_MODE_ADD:
//			case vBLEND_MODE_SUBTRACT:
//			case vBLEND_MODE_BLEND:
//			case vBLEND_MODE_BRIGHTEN:
//			case vBLEND_MODE_MODULATE:
//			case vBLEND_MODE_DIFFUSE:
//				GXTexObj	alphaObj;
//				GXInitTexObj( &alphaObj, p_alpha_data, width, height, ((GXTexFmt)p_texture->format), umode, vmode, levels > 1 ? GX_TRUE : GX_FALSE );
//				if ( levels > 1 )
//				{
//					if ( gMeshUseAniso && ( tev_id == 0 ) )
//					{
//						// If we're correcting the color, we also want ANISO_4.
//						GXInitTexObjLOD( &alphaObj, GX_LIN_MIP_LIN, GX_LINEAR, 0.0f, levels - 1, k, GX_FALSE, GX_TRUE, GX_ANISO_4 );
//					}
//					else
//					{
//						GXInitTexObjLOD( &alphaObj, GX_LIN_MIP_LIN, GX_LINEAR, 0.0f, levels - 1, k, GX_FALSE, GX_TRUE, GX_ANISO_1 );
//					}
//				}
//				else
//				{
//					GXInitTexObjLOD( &alphaObj, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, k, GX_FALSE, GX_TRUE, GX_ANISO_1 );
//				}
//				GXLoadTexObj( &alphaObj, (GXTexMapID)(((int)GX_TEXMAP0)+alpha_map) );
//				break;
//			default:
//				p_alpha_data = NULL;
//				break;
//		}
//	}
//
//	GXTexMapID t_id = (GXTexMapID)(((int)GX_TEXMAP0)+map_id);
//	GXTexCoordID u_id = (GXTexCoordID)(((int)GX_TEXCOORD0)+uv_id);
//	GXTevStageID s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
//
//	// Modulate material color if required.
//	if ( uniqueMaterialColor )
//	{
//		GX::SetTevKColor( (GXTevKColorID)(((int)GX_KCOLOR0)+tev_id), (GXColor){matcol.r,matcol.g,matcol.b,fix} );
//		GX::SetTevKColorSel( s_id, (GXTevKColorSel)(((int)GX_TEV_KCSEL_K0)+tev_id) );
//
//		GX::SetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR0A0 );
//		GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//		GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
//		GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
//		GX::SetTevAlphaIn ( s_id, rasa, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
//		GX::SetTevColorIn ( s_id, GX_CC_ZERO, rasc, GX_CC_KONST, GX_CC_ZERO );
//		stage_id++;
//		s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
//	}
//	
//	// Set texture coordinates. Note: If p_tex is NULL, it means this layer is environment mapped.
////	if ( !(flags & MATFLAG_ENVIRONMENT) )
////	{
//		*p_uv_slot = uv_id;
//		uv_id++;
////	} else {
////		*p_uv_slot = -1;
////	}
//
//	// Load this texture up into a temporary register for use later.
//	// Note: conveniently, the 1st texture ends up in TEVPREV which means it will be fine
//	// if no blends are performed.
//	switch ( blend )
//	{
//		case vBLEND_MODE_ADD_FIXED:
//		case vBLEND_MODE_SUB_FIXED:
//		case vBLEND_MODE_BLEND_FIXED:
//		case vBLEND_MODE_BRIGHTEN_FIXED:
//		case vBLEND_MODE_MODULATE_FIXED:
//			GX::SetTevKAlphaSel( s_id, (GXTevKAlphaSel)(((int)GX_TEV_KASEL_K0_A)+tev_id) );
//			if ( !uniqueMaterialColor ) GX::SetTevKColor( (GXTevKColorID)(((int)GX_KCOLOR0)+tev_id), (GXColor){fix,fix,fix,fix} );
//			GX::SetTevOrder( s_id, u_id, t_id, GX_COLOR0A0 );
//            GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//			GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
//			GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
//			if ( uniqueMaterialColor )
//			{
//				GX::SetTevAlphaIn ( s_id, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
//				GX::SetTevColorIn ( s_id, GX_CC_ZERO, GX_CC_TEXC, (GXTevColorArg)(((int)GX_CC_CPREV)+(reg_id*2)), GX_CC_ZERO );
//			}
//			else
//			{
//				GX::SetTevAlphaIn ( s_id, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
//				GX::SetTevColorIn ( s_id, GX_CC_ZERO, GX_CC_TEXC, rasc, GX_CC_ZERO );
//			}
//			stage_id++;
//			break;
//		case vBLEND_MODE_ADD:
//		case vBLEND_MODE_SUBTRACT:
//		case vBLEND_MODE_BLEND:
//		case vBLEND_MODE_BRIGHTEN:
//		case vBLEND_MODE_MODULATE:
//		case vBLEND_MODE_DIFFUSE:
//		default:
//			// We need to add a stage, if we have an alpha map.
//			if ( alpha_map >= 0 )
//			{
//				// Set inputs.
//				GXTexMapID a_id = (GXTexMapID)(((int)GX_TEXMAP0)+alpha_map);
//
//				GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP1 );       // Alpha map is in Green channel, so use this to swap it to the alpha channel.
//				GX::SetTevOrder( s_id, u_id, a_id, GX_COLOR0A0 );
//				GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
//				GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
//				if ( uniqueMaterialColor )
//				{
//					GX::SetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_TEXA, (GXTevAlphaArg)(((int)GX_CA_APREV)+reg_id), GX_CA_ZERO );
//					GX::SetTevColorIn ( s_id, (GXTevColorArg)(((int)GX_CC_CPREV)+(reg_id*2)), GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );
//				}
//				else
//				{
//					GX::SetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_TEXA, rasa, GX_CA_ZERO );
//					GX::SetTevColorIn ( s_id, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );
//				}
//				stage_id++;
//				s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
//				GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//				GX::SetTevOrder( s_id, u_id, t_id, GX_COLOR0A0 );
//				GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
//				GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
//				GX::SetTevAlphaIn ( s_id, GX_CA_ZERO, (GXTevAlphaArg)(((int)GX_CA_APREV)+reg_id), GX_CA_TEXA, GX_CA_ZERO );
//				if ( uniqueMaterialColor )
//				{
//					GX::SetTevColorIn ( s_id, GX_CC_ZERO, GX_CC_TEXC, (GXTevColorArg)(((int)GX_CC_CPREV)+(reg_id*2)), GX_CC_ZERO );
//				}
//				else
//				{
//					GX::SetTevColorIn ( s_id, GX_CC_ZERO, GX_CC_TEXC, rasc, GX_CC_ZERO );
//				}
//				stage_id++;
//			} else {
//				GX::SetTevOrder( s_id, u_id, t_id, GX_COLOR0A0 );
//				GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//				GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
//				GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+reg_id) );
//				if ( uniqueMaterialColor )
//				{
//					GX::SetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_TEXA, (GXTevAlphaArg)(((int)GX_CA_APREV)+reg_id), GX_CA_ZERO );
//					GX::SetTevColorIn ( s_id, GX_CC_ZERO, GX_CC_TEXC, (GXTevColorArg)(((int)GX_CC_CPREV)+(reg_id*2)), GX_CC_ZERO );
//				}
//				else
//				{
//					GX::SetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_TEXA, rasa, GX_CA_ZERO );
//					GX::SetTevColorIn ( s_id, GX_CC_ZERO, GX_CC_TEXC, rasc, GX_CC_ZERO );
//				}
//				stage_id++;
//			}
//
//			break;
//	}
//
//	if ( tev_id > 0 ) {
//		multi_add_layer ( blend, fix );
//	} else {
//		// Set blend mode for base layer.
//		switch ( blend )
//		{
//			case vBLEND_MODE_ADD:
//			case vBLEND_MODE_ADD_FIXED:
//				GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR );
//				break;
//			case vBLEND_MODE_SUBTRACT:
//			case vBLEND_MODE_SUB_FIXED:
//				GX::SetBlendMode ( GX_BM_SUBTRACT, GX_BL_ZERO, GX_BL_ZERO, GX_LO_CLEAR );
//				break;
//			case vBLEND_MODE_BLEND:
//			case vBLEND_MODE_BLEND_FIXED:
//				GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR );
//				break;
//			case vBLEND_MODE_BRIGHTEN:
//			case vBLEND_MODE_BRIGHTEN_FIXED:
////				GX::SetBlendMode ( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ONE, GX_LO_CLEAR );
//				GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR );
//				break;
//			case vBLEND_MODE_MODULATE_FIXED:
//			case vBLEND_MODE_MODULATE:
////				GX::SetBlendMode ( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ZERO, GX_LO_CLEAR );
//				GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR );
//				break;
//			case vBLEND_MODE_DIFFUSE:
//			default:
//				GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR );
//				break;
//		}
//	}
//
//	tev_id++;
//	map_id = cur_map;
//}
//
//static void multi_end ( BlendModes blend, uint32 alphacutoff )
//{
//	// Special case. If the base layer uses subtractive mode, we need to add a stage to
//	// modulate the color & alpha.
//	switch ( blend )
//	{
////		case vBLEND_MODE_DIFFUSE:
////			GX::SetAlphaCompare(GX_GEQUAL, 0, GX_AOP_AND, GX_GEQUAL, 0 );
////			if ( alphacutoff == 0 )
////			{
////				GXSetZCompLoc( GX_TRUE );
////			}
////			else
////			{
////				GXSetZCompLoc( GX_FALSE );		// Textures with holes will not write to the z buffer when a<=cutoff.
////			}
////			break;
//		case vBLEND_MODE_SUB_FIXED:
//		case vBLEND_MODE_SUBTRACT:
//			{
//				GXTevStageID s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
//
//				GX::SetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR_NULL );
//				GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//				GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//				GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//				GX::SetTevAlphaIn ( s_id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV );
//				GX::SetTevColorIn ( s_id, GX_CC_ZERO, GX_CC_CPREV, GX_CC_APREV, GX_CC_ZERO );
//				stage_id++;
//				//GX::SetAlphaCompare(GX_GEQUAL, 0, GX_AOP_AND, GX_GEQUAL, 0 );
////				GXSetZCompLoc( GX_FALSE );
//			}
//			break;
//		default:
//			//GX::SetAlphaCompare(GX_GEQUAL, alphacutoff, GX_AOP_AND, GX_GEQUAL, alphacutoff );
////			if ( alphacutoff == 0 )
////			{
////				GXSetZCompLoc( GX_TRUE );
////			}
////			else
////			{
////				GXSetZCompLoc( GX_FALSE );		// Textures with holes will not write to the z buffer when a<=cutoff.
////			}
//			break;
//	}
//
////	if ( blend == vBLEND_MODE_DIFFUSE )
////	{
////		//Always pass pixels with multi-texturing as the base must be opaque.
////		GX::SetAlphaCompare(GX_GEQUAL, 0, GX_AOP_AND, GX_GEQUAL, 0 );
////		GXSetZCompLoc( GX_TRUE );
////	}
////	else
////	{
////	}
//
//	if ( correct_color )
//	{
//		GXTexCoordID u_id;
//		// Convert color0 to s,t for 2-D texture lookup (RG)
//		u_id = (GXTexCoordID)(((int)GX_TEXCOORD0)+uv_id);
//		GX::SetTexCoordGen(u_id, GX_TG_SRTG, GX_TG_COLOR0, GX_IDENTITY);
//		GX::SetTevOrder(GX_TEVSTAGE0, u_id, GX_TEXMAP7, GX_COLOR_NULL);
//		uv_id++;
//	
//		// Convert color1 to s,t for 2-D texture lookup (BA)
//		u_id = (GXTexCoordID)(((int)GX_TEXCOORD0)+uv_id);
//		GX::SetTexCoordGen(u_id, GX_TG_SRTG, GX_TG_COLOR1, GX_IDENTITY);
//		GX::SetTevOrder(GX_TEVSTAGE1, u_id, GX_TEXMAP7, GX_COLOR_NULL);
//		uv_id++;
//		GX::SetNumChans( 2 );
//	}
//	else
//	{
//		GX::SetNumChans( 1 );
//	}
//
//	// Set final number of textures/stages.
//	GX::SetNumTexGens( uv_id );
//    GX::SetNumTevStages( stage_id );
//}
//
////sMaterial					*Materials;
//uint32						NumMaterials;
////Lst::HashTable< sMaterial > *pMaterialTable	= NULL;
//
//void sMaterial::Submit( uint32 mesh_flags, GXColor mesh_color )
//{
//	// Figure uv and vc wibble updates if required.
//	figure_wibble_uv();
//	figure_wibble_vc();
//
//	
//
//	for ( uint layer = 0; layer < Passes; layer++ )
//	{
//		if( Flags[layer] & MATFLAG_UV_WIBBLE )
//		{
//			// Set up the texture generation matrix here.
//			Mtx	m;
//			MTXTrans( m, m_pUVControl->UVWibbleOffset[layer][0], m_pUVControl->UVWibbleOffset[layer][1], 0.0f );
//			switch ( layer )
//			{
//				case 0:
//					GX::LoadTexMtxImm( m, GX_TEXMTX0, GX_MTX2x4 );
//					break;
//				case 1:
//					GX::LoadTexMtxImm( m, GX_TEXMTX1, GX_MTX2x4 );
//					break;
//				case 2:
//					GX::LoadTexMtxImm( m, GX_TEXMTX2, GX_MTX2x4 );
//					break;
//				case 3:
//					GX::LoadTexMtxImm( m, GX_TEXMTX3, GX_MTX2x4 );
//					break;
//				default:
//					break;
//				
//			}
//
////			// Handle UV wibbling.
////			D3DXMATRIX mat;
////			mat._11 = 1.0f;					mat._12 = 0.0f; 
////			mat._21 = 0.0f;					mat._22 = 1.0f;
////			mat._31 = UVWibbleOffset[p][0];	mat._32 = UVWibbleOffset[p][1];
////			mat._41 = 0.0f;					mat._42 = 0.0f;
////			D3DDevice_SetTransform((D3DTRANSFORMSTATETYPE)( D3DTS_TEXTURE0 + p ), &mat );
////			D3DDevice_SetTextureStageState( p, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2 );
////			D3DDevice_SetTextureStageState( p, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU | p );
//		}
//	}
//
////	// Generate display list.
////
////	multi_start ( Passes );
////	for ( uint layer = 0; layer < Passes ; layer++ )
////	{
////		GXTexWrapMode u_mode;
////		GXTexWrapMode v_mode;
////		if ( Flags[layer] & MATFLAG_ENVIRONMENT )
////		{
////			u_mode = GX_REPEAT;
////			v_mode = GX_REPEAT;
////		}
////		else
////		{
////			u_mode = UVAddressing[layer] & (1<<0)  ? GX_CLAMP : GX_REPEAT;
////			v_mode = UVAddressing[layer] & (1<<1) ? GX_CLAMP : GX_REPEAT;
////		}
////
////		if ( pTex[layer] )
////		{
////			multi_add_texture ( pTex[layer],
////								matcol[layer],
////								u_mode,
////								v_mode,
////								(BlendModes)blendMode[layer],
////								fixAlpha[layer],
////								Flags[layer],
////								&uv_slot[layer],
////								K[layer] );
////		}
////		else
////		{
////			// Untextured.
////			GXTevStageID s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
////			BlendModes blend = (BlendModes)blendMode[layer];
////			uint32 fix = fixAlpha[layer];
////
////			uv_slot[layer] = -1;
////
////			switch ( blend )
////			{
////				case vBLEND_MODE_ADD_FIXED:
////				case vBLEND_MODE_SUB_FIXED:
////				case vBLEND_MODE_BLEND_FIXED:
////				case vBLEND_MODE_BRIGHTEN_FIXED:
////				case vBLEND_MODE_MODULATE_FIXED:
////					GX::SetTevKAlphaSel( s_id, (GXTevKAlphaSel)(((int)GX_TEV_KASEL_K0_A)+tev_id) );
////					GX::SetTevKColor( (GXTevKColorID)(((int)GX_KCOLOR0)+tev_id), (GXColor){fix,fix,fix,fix} );
////					GX::SetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR0A0 );
////					GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
////					GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id) );
////					GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id) );
////					GX::SetTevAlphaIn ( s_id, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
////					GX::SetTevColorIn ( s_id, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );
////					stage_id++;
////					break;
////				case vBLEND_MODE_ADD:
////				case vBLEND_MODE_SUBTRACT:
////				case vBLEND_MODE_BLEND:
////				case vBLEND_MODE_BRIGHTEN:
////				case vBLEND_MODE_MODULATE:
////				case vBLEND_MODE_DIFFUSE:
////				default:
////					GX::SetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR0A0 );
////					GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
////					GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id) );
////					GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id) );
////					GX::SetTevAlphaIn ( s_id, GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
////					GX::SetTevColorIn ( s_id, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );
////					stage_id++;
////					break;
////			}
////
////			if ( tev_id > 0 ) {
////				multi_add_layer ( blend, fix );
////			} else {
////				// Set blend mode for base layer.
////				switch ( blend )
////				{
////					case vBLEND_MODE_ADD:
////					case vBLEND_MODE_ADD_FIXED:
////						GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR );
////						break;
////					case vBLEND_MODE_SUBTRACT:
////					case vBLEND_MODE_SUB_FIXED:
////						GX::SetBlendMode ( GX_BM_SUBTRACT, GX_BL_ZERO, GX_BL_ZERO, GX_LO_CLEAR );
////						break;
////					case vBLEND_MODE_BLEND:
////					case vBLEND_MODE_BLEND_FIXED:
////						GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR );
////						break;
////					case vBLEND_MODE_BRIGHTEN:
////					case vBLEND_MODE_BRIGHTEN_FIXED:
////						GX::SetBlendMode ( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ONE, GX_LO_CLEAR );
////						break;
////					case vBLEND_MODE_MODULATE_FIXED:
////					case vBLEND_MODE_MODULATE:
////						GX::SetBlendMode ( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ZERO, GX_LO_CLEAR );
////						break;
////					case vBLEND_MODE_DIFFUSE:
////					default:
////						GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR );
////						break;
////				}
////			}
////			tev_id++;
////		}
////	}
////
////	multi_end( (BlendModes)blendMode[0], AlphaCutoff );
//
//
//	// Set the cull mode, but only if the calling code didn't override this by setting CULL_NONE.
//	if ( EngineGlobals.poly_culling )
//	{
//		GX::SetCullMode ( m_no_bfc ? GX_CULL_NONE : GX_CULL_FRONT );
//	}
//	else
//	{
//		GX::SetCullMode ( GX_CULL_NONE );
//	}
//
//#ifdef __USE_MAT_DL__
//	if ( mp_display_list )
//	{
//		GXCallDisplayList ( mp_display_list, m_display_list_size ); 
//	}
//#else
//	Initialize( mesh_flags, mesh_color );
//#endif		// __USE_MAT_DL__
//
//	// See if we need to set the comp location (just check the base layer).
//	if ( pTex[0] )
//	{
//		if ( pTex[0]->HasHoles )
//		{
//			GX::SetZCompLoc( GX_FALSE );
//		}
//		else
//		{
////			GX::SetZCompLoc( GX_FALSE );
//			GX::SetZCompLoc( GX_TRUE );
////			GX::SetAlphaCompare(GX_GEQUAL, 0, GX_AOP_AND, GX_GEQUAL, 0 );
//		}
//	}
//	GX::SetAlphaCompare(GX_GEQUAL, AlphaCutoff, GX_AOP_AND, GX_GEQUAL, AlphaCutoff );
//
////	if ( mp_display_list )
////	{
////		GXCallDisplayList ( mp_display_list, m_display_list_size ); 
////	}
////	else
////	{
////		// If no material, default to purple, untextured.
////		GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
////		GX::SetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
////		GX::SetNumTexGens( 0 );
////		GX::SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
////		GX::SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
////		GX::SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
////		GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR );
////		GX::SetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA );
////		GX::SetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC );
////		GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,0,255,255} );
////		GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){128,128,128,255} );
////		GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
////	}
//}
//
//void sMaterial::Initialize( uint32 mesh_flags, GXColor mesh_color )
//{
//#ifdef __USE_MAT_DL__
//	// Generate display list.
//	Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
//#define DL_BUILD_SIZE (32*1024)
//	uint8 * p_build_dl = new (Mem::Manager::sHandle().TopDownHeap()) uint8[DL_BUILD_SIZE];
//	DCFlushRange ( p_build_dl, DL_BUILD_SIZE );
//
//	GXBeginDisplayList ( p_build_dl, DL_BUILD_SIZE );
//	GXResetWriteGatherPipe();
//#endif		// __USE_MAT_DL__
//
//	multi_start ( Passes );
//	for ( uint layer = 0; layer < Passes ; layer++ )
//	{
//		GXTexWrapMode u_mode;
//		GXTexWrapMode v_mode;
//		if ( Flags[layer] & MATFLAG_ENVIRONMENT )
//		{
//			u_mode = GX_REPEAT;
//			v_mode = GX_REPEAT;
//		}
//		else
//		{
//			u_mode = UVAddressing[layer] & (1<<0)  ? GX_CLAMP : GX_REPEAT;
//			v_mode = UVAddressing[layer] & (1<<1) ? GX_CLAMP : GX_REPEAT;
//		}
//
//		GXColor col;
//
//		if ( mesh_flags & sMesh::MESH_FLAG_MATERIAL_COLOR_OVERRIDE && !( Flags[layer] & 0x80 ) )
//		{
//			col = mesh_color;
//		}
//		else
//		{
//			col = matcol[layer];
//		}
//
//		if ( pTex[layer] )
//		{
//			multi_add_texture ( pTex[layer],
//								col,
//								u_mode,
//								v_mode,
//								(BlendModes)blendMode[layer],
//								fixAlpha[layer],
//								Flags[layer],
//								&uv_slot[layer],
//								K[layer] );
//		}
//		else
//		{
//			// Untextured.
//			GXTevStageID s_id = (GXTevStageID)(((int)GX_TEVSTAGE0)+stage_id);
//			BlendModes blend = (BlendModes)blendMode[layer];
//			uint32 fix = fixAlpha[layer];
//
//			uv_slot[layer] = -1;
//
//			switch ( blend )
//			{
//				case vBLEND_MODE_ADD_FIXED:
//				case vBLEND_MODE_SUB_FIXED:
//				case vBLEND_MODE_BLEND_FIXED:
//				case vBLEND_MODE_BRIGHTEN_FIXED:
//				case vBLEND_MODE_MODULATE_FIXED:
//					GX::SetTevKAlphaSel( s_id, (GXTevKAlphaSel)(((int)GX_TEV_KASEL_K0_A)+tev_id) );
//					GX::SetTevKColor( (GXTevKColorID)(((int)GX_KCOLOR0)+tev_id), (GXColor){fix,fix,fix,fix} );
//					GX::SetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR0A0 );
//					GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//					GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id) );
//					GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id) );
//					GX::SetTevAlphaIn ( s_id, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
//					GX::SetTevColorIn ( s_id, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );
//					stage_id++;
//					break;
//				case vBLEND_MODE_ADD:
//				case vBLEND_MODE_SUBTRACT:
//				case vBLEND_MODE_BLEND:
//				case vBLEND_MODE_BRIGHTEN:
//				case vBLEND_MODE_MODULATE:
//				case vBLEND_MODE_DIFFUSE:
//				default:
//					GX::SetTevOrder( s_id, GX_TEXCOORD_NULL, GX_TEX_DISABLE, GX_COLOR0A0 );
//					GX::SetTevSwapMode( s_id, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//					GX::SetTevAlphaOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id) );
//					GX::SetTevColorOp(s_id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, (GXTevRegID)(((int)GX_TEVPREV)+tev_id) );
//					GX::SetTevAlphaIn ( s_id, GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
//					GX::SetTevColorIn ( s_id, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );
//					stage_id++;
//					break;
//			}
//
//			if ( tev_id > 0 ) {
//				multi_add_layer ( blend, fix );
//			} else {
//				// Set blend mode for base layer.
//				switch ( blend )
//				{
//					case vBLEND_MODE_ADD:
//					case vBLEND_MODE_ADD_FIXED:
//						GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR );
//						break;
//					case vBLEND_MODE_SUBTRACT:
//					case vBLEND_MODE_SUB_FIXED:
//						GX::SetBlendMode ( GX_BM_SUBTRACT, GX_BL_ZERO, GX_BL_ZERO, GX_LO_CLEAR );
//						break;
//					case vBLEND_MODE_BLEND:
//					case vBLEND_MODE_BLEND_FIXED:
//						GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR );
//						break;
//					case vBLEND_MODE_BRIGHTEN:
//					case vBLEND_MODE_BRIGHTEN_FIXED:
//						GX::SetBlendMode ( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ONE, GX_LO_CLEAR );
//						break;
//					case vBLEND_MODE_MODULATE_FIXED:
//					case vBLEND_MODE_MODULATE:
//						GX::SetBlendMode ( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ZERO, GX_LO_CLEAR );
//						break;
//					case vBLEND_MODE_DIFFUSE:
//					default:
//						GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR );
//						break;
//				}
//			}
//			tev_id++;
//		}
//	}
//
//	multi_end( (BlendModes)blendMode[0], AlphaCutoff );
//
//#ifdef __USE_MAT_DL__
//	int size = GXEndDisplayList();
//
//	// If we already have a display list & this one will fit, just use the existing piece of memory.
//	uint8 * p_dl;
//	if ( mp_display_list && ( size <= m_display_list_size ) )
//	{
//		p_dl = mp_display_list;
//	}
//	else
//	{
//		if ( mp_display_list )
//		{
//			//gMatBytes -= m_display_list_size;
//			delete mp_display_list;
//		}
//		p_dl = new uint8[size];
//		//gMatBytes += size;
//	}
//
//	memcpy ( p_dl, p_build_dl, size );
//	DCFlushRange ( p_dl, size );
//
//	delete p_build_dl;
//
//	mp_display_list = p_dl;
//	m_display_list_size = size;
//	Mem::Manager::sHandle().BottomUpHeap()->PopAlign();
//#endif		// __USE_MAT_DL__
//}




sMaterial* GetMaterial( uint32 checksum, sScene *p_scene )
{
//	for ( int lp = 0; lp < p_scene->m_num_materials; lp++ )
//	{
//		if( p_scene->mp_material_array[lp].Checksum == checksum )
//		{
//			return &p_scene->mp_material_array[lp];
//		}
//	}
	return NULL;
}

void InitializeMaterial( sMaterial* p_material )
{
//	p_material->Initialize();
	p_material->uv_slot[0] = 0;
	p_material->uv_slot[1] = 1;
	p_material->uv_slot[2] = 2;
	p_material->uv_slot[3] = 3;
}

#define MemoryRead( dst, size, num, src )	memcpy(( dst ), ( src ), (( num ) * ( size )));	\
											( src ) += (( num ) * ( size ))

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sMaterial * LoadMaterialsFromMemory( void **pp_mem, Lst::HashTable< Nx::CTexture > *p_texture_table, int * p_num_materials )
{
	uint8	*p_data = (uint8*)( *pp_mem );
	uint32 MMAG, MMIN, K, L, NumSeqs, seq, NumKeys;
	uint32 i;

	// Get number of materials.
	uint32 new_materials;
	MemoryRead( &new_materials, sizeof( uint32 ), 1, p_data );
	
	if ( p_num_materials ) *p_num_materials = new_materials;

	// Create array of materials.
	sMaterial * p_mat_array = new sMaterial[new_materials];
	
	// Loop over materials.
	for( i = 0; i < new_materials; ++i )
	{
		// Create new material.
		sMaterial *pMat = &p_mat_array[i];
		
		pMat->m_num_wibble_vc_anims	= 0;
		pMat->mp_wibble_vc_params	= NULL;
		pMat->mp_wibble_vc_colors	= NULL;

		// Get material checksum.
		MemoryRead( &pMat->Checksum, sizeof( uint32 ), 1, p_data );

		// Get number of passes.
		uint32 pass;
		MemoryRead( &pass, sizeof( uint32 ), 1, p_data );
		pMat->Passes = pass;

		// Get alpha cutoff value.
		uint32 ac;
		MemoryRead( &ac, sizeof( uint32 ), 1, p_data );
		pMat->AlphaCutoff = ac;		//( ac >= 128 ) ? 255 : ( ac * 2 );

		// Get sorted flag.
		bool sorted;
		MemoryRead( &sorted, sizeof( bool ), 1, p_data );
		pMat->m_sorted = sorted;
		
		// Get draw order.
		MemoryRead( &pMat->m_draw_order, sizeof( float ), 1, p_data );
		
		// Get backface cull flag.
		bool bfc;
		MemoryRead( &bfc, sizeof( bool ), 1, p_data );
		pMat->m_no_bfc = bfc;

//		// Get grassify flag and (optionally) grassify data.
//		bool grassify;
//		MemoryRead( &grassify, sizeof( bool ), 1, p_data );
//		pMat->m_grassify = (uint8)grassify;
//		if( grassify )
//		{
//			MemoryRead( &pMat->m_grass_height, sizeof( float ), 1, p_data );
//			int layers;
//			MemoryRead( &layers, sizeof( int ), 1, p_data );
//			pMat->m_grass_layers = (uint8)layers;
//		}

		GXColor mat_col[MAX_PASSES];

		pMat->m_pUVControl = NULL;
		for( uint32 pass = 0; pass < pMat->Passes; ++pass )
		{
			// Get texture checksum.
			uint32 TextureChecksum;
			MemoryRead( &TextureChecksum, sizeof( uint32 ), 1, p_data );

			// Get material flags.
			MemoryRead( &pMat->Flags[pass], sizeof( uint32 ), 1, p_data );

			// Get ALPHA register value.
			uint64 ra;
			MemoryRead( &ra, sizeof( uint64 ), 1, p_data );
			pMat->blendMode[pass] = ra & 0xff;
			pMat->fixAlpha[pass] = ( ra >> 32 ) & 0xff;

			// Backface cull test - if this is an alpha blended material, turn off backface culling.
			if(( pass == 0 ) && (( ra & 0xFF ) != 0x00 ))
			{
				pMat->m_no_bfc = true;
			}

			// Get UV addressing types.
			uint32 u_addressing, v_addressing;
			MemoryRead( &u_addressing, sizeof( uint32 ), 1, p_data );
			MemoryRead( &v_addressing, sizeof( uint32 ), 1, p_data );
			pMat->UVAddressing[pass] = (( v_addressing << 1 ) | u_addressing );
			
			// Step past material colors, currently unsupported.
			float col[9];
			MemoryRead( col, sizeof( float ) * 9, 1, p_data );

			mat_col[pass].r = (u8)((col[0] * 255.0f) + 0.5f);
			mat_col[pass].g = (u8)((col[1] * 255.0f) + 0.5f);
			mat_col[pass].b = (u8)((col[2] * 255.0f) + 0.5f);
			mat_col[pass].a = 255;

			// Read uv wibble data if present.
			if( pMat->Flags[pass] & MATFLAG_UV_WIBBLE )
			{
				if ( !pMat->m_pUVControl ) pMat->m_pUVControl = new sUVControl;
				MemoryRead( &( pMat->m_pUVControl->UVWibbleParams[pass] ), sizeof( sUVWibbleParams ), 1, p_data );
			}

			// Step past vc wibble data, currently unsupported.
			if(( pass == 0 ) && ( pMat->Flags[0] & MATFLAG_VC_WIBBLE ))
			{
				MemoryRead( &NumSeqs, sizeof( uint32 ), 1, p_data );
				pMat->m_num_wibble_vc_anims = NumSeqs;

				// Create sequence data array.
				pMat->mp_wibble_vc_params = new sVCWibbleParams[NumSeqs];
				
				// Create resultant color array.
				pMat->mp_wibble_vc_colors = new uint32[NumSeqs];

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

					for ( uint lp = 0; lp < NumKeys; lp++ )
					{
						uint8 r = pMat->mp_wibble_vc_params[seq].mp_keyframes[lp].m_color.r;
						uint8 g = pMat->mp_wibble_vc_params[seq].mp_keyframes[lp].m_color.g;
						uint8 b = pMat->mp_wibble_vc_params[seq].mp_keyframes[lp].m_color.b;
						uint8 a = pMat->mp_wibble_vc_params[seq].mp_keyframes[lp].m_color.a;
						pMat->mp_wibble_vc_params[seq].mp_keyframes[lp].m_color.r = b;
						pMat->mp_wibble_vc_params[seq].mp_keyframes[lp].m_color.g = r;
						pMat->mp_wibble_vc_params[seq].mp_keyframes[lp].m_color.b = g;
						pMat->mp_wibble_vc_params[seq].mp_keyframes[lp].m_color.a = a;
					}
				}
			}

			if( TextureChecksum )
			{
				// If textured, resolve texture checksum...
				Nx::CNgcTexture *p_ngc_texture = static_cast<Nx::CNgcTexture*>( p_texture_table->GetItem( TextureChecksum ) );
				sTexture *pTex = ( p_ngc_texture ) ? p_ngc_texture->GetEngineTexture() : NULL;

				// Bail if checksum not found.
				if( pTex == NULL )
				{
					Dbg_Message( "error: couldn't find texture checksum %08X\n", TextureChecksum );
//					exit( 1 );
					pMat->pTex[pass] = NULL;
				}
				else
				{
					// Set texture pointer.
					pMat->pTex[pass] = pTex;
				}

				// Get mipmap info.
				MemoryRead( &MMAG, sizeof( uint32 ), 1, p_data );
				MemoryRead( &MMIN, sizeof( uint32 ), 1, p_data );

				MemoryRead( &K, sizeof( uint32 ), 1, p_data );
				MemoryRead( &L, sizeof( uint32 ), 1, p_data );
				
				// Default PS2 value for K appears to be -8.0f - we are interested in deviations from this value.
				pMat->K[pass]	= ( *(float*)&K ) + 8.0f;
			
				// Dave note 08/03/02 - having MIPs selected earlier than normal seems to cause some problems, since Xbox
				// MIP selection is so different to Ps2. Limit the k value such that Xbox can never select smaller MIPs
				// earlier than it would do by default.
				if( pMat->K[pass] > 0.0f )
				{
					pMat->K[pass] = 0.0f;
				}
			}
			else
			{
				// ...otherwise just step past mipmap info.
				pMat->pTex[pass] = NULL;
				uint32 mip[4];
				MemoryRead( mip, sizeof( uint32 ) * 4, 1, p_data );
			}
		}

		// Generate material colors.
		for( uint32 pass = 0; pass < pMat->Passes; ++pass )
		{
			if ( ( mat_col[0].r == mat_col[pass].r ) &&
				 ( mat_col[0].g == mat_col[pass].g ) && 
				 ( mat_col[0].b == mat_col[pass].b ) && 
				 ( mat_col[0].a == mat_col[pass].a ) )
			{
				// Do nothing.
				pMat->matcol[pass] = (GXColor){128,128,128,255};
			}
			else
			{
				// Work out the texture color.
				GXColor tc;
				float base_r = (float)(mat_col[0].r > 0 ? mat_col[0].r : 1 );
				float base_g = (float)(mat_col[0].g > 0 ? mat_col[0].g : 1 );
				float base_b = (float)(mat_col[0].b > 0 ? mat_col[0].b : 1 );
				float base_a = (float)(mat_col[0].a > 0 ? mat_col[0].a : 1 );
				float r = ( ((float)mat_col[pass].r) / base_r ) * 128.0f;
				float g = ( ((float)mat_col[pass].g) / base_g ) * 128.0f;
				float b = ( ((float)mat_col[pass].b) / base_b ) * 128.0f;
				float a = ( ((float)mat_col[pass].a) / base_a ) * 255.0f;
				int _r = (int)r;
				int _g = (int)g;
				int _b = (int)b;
				int _a = (int)a;
				if ( _r > 255 ) _r = 255;
				if ( _g > 255 ) _g = 255;
				if ( _b > 255 ) _b = 255;
				if ( _a > 255 ) _a = 255;
				tc.r = (u8)( _r );
				tc.g = (u8)( _g );
				tc.b = (u8)( _b );
				tc.a = (u8)( _a );
				pMat->matcol[pass] = tc;
			}
		}

//		pMat->mp_display_list = NULL;
//		pMat->Initialize();
		pMat->uv_slot[0] = 0;
		pMat->uv_slot[1] = 1;
		pMat->uv_slot[2] = 2;
		pMat->uv_slot[3] = 3;
	}
	*pp_mem = p_data;

	return p_mat_array;
}

sMaterial *LoadMaterials( void *p_FH, Lst::HashTable< Nx::CTexture > *p_texture_table, int * p_num_materials )
{
	uint32 MMAG, MMIN, K, L, NumSeqs, seq, NumKeys;
	uint32 i;

	// Get number of materials.
	uint32 new_materials;
	File::Read( &new_materials, sizeof( uint32 ), 1, p_FH );
	
	if ( p_num_materials ) *p_num_materials = new_materials;

	// Create array of materials.
	sMaterial * p_mat_array = new sMaterial[new_materials];
	
	// Loop over materials.
	for( i = 0; i < new_materials; ++i )
	{
		// Create new material.
		sMaterial *pMat = &p_mat_array[i];
		
		pMat->m_num_wibble_vc_anims	= 0;
		pMat->mp_wibble_vc_params	= NULL;
		pMat->mp_wibble_vc_colors	= NULL;

		// Get material checksum.
		File::Read( &pMat->Checksum, sizeof( uint32 ), 1, p_FH );

		// Get number of passes.
		uint32 pass;
		File::Read( &pass, sizeof( uint32 ), 1, p_FH );
		pMat->Passes = pass;

		// Get alpha cutoff value.
		uint32 ac;
		File::Read( &ac, sizeof( uint32 ), 1, p_FH );
		pMat->AlphaCutoff = ac;		//( ac >= 128 ) ? 255 : ( ac * 2 );

		// Get sorted flag.
		bool sorted;
		File::Read( &sorted, sizeof( bool ), 1, p_FH );
		pMat->m_sorted = sorted;
		
		// Get draw order.
		File::Read( &pMat->m_draw_order, sizeof( float ), 1, p_FH );
		
		// Get backface cull flag.
		bool bfc;
		File::Read( &bfc, sizeof( bool ), 1, p_FH );
		pMat->m_no_bfc = bfc;

//		// Get grassify flag and (optionally) grassify data.
//		bool grassify;
//		File::Read( &grassify, sizeof( bool ), 1, p_FH );
//		pMat->m_grassify = (uint8)grassify;
//		if( grassify )
//		{
//			File::Read( &pMat->m_grass_height, sizeof( float ), 1, p_FH );
//			int layers;
//			File::Read( &layers, sizeof( int ), 1, p_FH );
//			pMat->m_grass_layers = (uint8)layers;
//		}

		GXColor mat_col[MAX_PASSES];

		pMat->m_pUVControl = NULL;
		for( uint32 pass = 0; pass < pMat->Passes; ++pass )
		{
			// Get texture checksum.
			uint32 TextureChecksum;
			File::Read( &TextureChecksum, sizeof( uint32 ), 1, p_FH );

			// Get material flags.
			File::Read( &pMat->Flags[pass], sizeof( uint32 ), 1, p_FH );

			// Get ALPHA register value.
			uint64 ra;
			File::Read( &ra, sizeof( uint64 ), 1, p_FH );
			pMat->blendMode[pass] = ra & 0xff;
			pMat->fixAlpha[pass] = ( ra >> 32 ) & 0xff;

			// Backface cull test - if this is an alpha blended material, turn off backface culling.
			if(( pass == 0 ) && (( ra & 0xFF ) != 0x00 ))
			{
				pMat->m_no_bfc = true;
			}

			// Get UV addressing types.
			uint32 u_addressing, v_addressing;
			File::Read( &u_addressing, sizeof( uint32 ), 1, p_FH );
			File::Read( &v_addressing, sizeof( uint32 ), 1, p_FH );
			pMat->UVAddressing[pass] = (( v_addressing << 1 ) | u_addressing );
			
			// Step past material colors, currently unsupported.
			float col[9];
			File::Read( col, sizeof( float ) * 9, 1, p_FH );

			mat_col[pass].r = (u8)((col[0] * 255.0f) + 0.5f);
			mat_col[pass].g = (u8)((col[1] * 255.0f) + 0.5f);
			mat_col[pass].b = (u8)((col[2] * 255.0f) + 0.5f);
			mat_col[pass].a = 255;

			// Read uv wibble data if present.
			if( pMat->Flags[pass] & MATFLAG_UV_WIBBLE )
			{
				if ( !pMat->m_pUVControl ) pMat->m_pUVControl = new sUVControl;
				File::Read( &( pMat->m_pUVControl->UVWibbleParams[pass] ), sizeof( sUVWibbleParams ), 1, p_FH );
			}

			// Step past vc wibble data, currently unsupported.
			if(( pass == 0 ) && ( pMat->Flags[0] & MATFLAG_VC_WIBBLE ))
			{
				File::Read( &NumSeqs, sizeof( uint32 ), 1, p_FH );
				pMat->m_num_wibble_vc_anims = NumSeqs;

				// Create sequence data array.
				pMat->mp_wibble_vc_params = new sVCWibbleParams[NumSeqs];
				
				// Create resultant color array.
				pMat->mp_wibble_vc_colors = new uint32[NumSeqs];

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

					for ( uint lp = 0; lp < NumKeys; lp++ )
					{
						uint8 r = pMat->mp_wibble_vc_params[seq].mp_keyframes[lp].m_color.r;
						uint8 g = pMat->mp_wibble_vc_params[seq].mp_keyframes[lp].m_color.g;
						uint8 b = pMat->mp_wibble_vc_params[seq].mp_keyframes[lp].m_color.b;
						uint8 a = pMat->mp_wibble_vc_params[seq].mp_keyframes[lp].m_color.a;
						pMat->mp_wibble_vc_params[seq].mp_keyframes[lp].m_color.r = b;
						pMat->mp_wibble_vc_params[seq].mp_keyframes[lp].m_color.g = r;
						pMat->mp_wibble_vc_params[seq].mp_keyframes[lp].m_color.b = g;
						pMat->mp_wibble_vc_params[seq].mp_keyframes[lp].m_color.a = a;
					}
				}
			}

			if( TextureChecksum )
			{
				// If textured, resolve texture checksum...
				Nx::CNgcTexture *p_ngc_texture = static_cast<Nx::CNgcTexture*>( p_texture_table->GetItem( TextureChecksum ) );
				sTexture *pTex = ( p_ngc_texture ) ? p_ngc_texture->GetEngineTexture() : NULL;

				// Bail if checksum not found.
				if( pTex == NULL )
				{
					Dbg_Message( "error: couldn't find texture checksum %08X\n", TextureChecksum );
//					exit( 1 );
					pMat->pTex[pass] = NULL;
				}
				else
				{
					// Set texture pointer.
					pMat->pTex[pass] = pTex;
				}

				// Get mipmap info.
				File::Read( &MMAG, sizeof( uint32 ), 1, p_FH );
				File::Read( &MMIN, sizeof( uint32 ), 1, p_FH );

				File::Read( &K, sizeof( uint32 ), 1, p_FH );
				File::Read( &L, sizeof( uint32 ), 1, p_FH );
				
				// Default PS2 value for K appears to be -8.0f - we are interested in deviations from this value.
				pMat->K[pass]	= ( *(float*)&K ) + 8.0f;
			
				// Dave note 08/03/02 - having MIPs selected earlier than normal seems to cause some problems, since Xbox
				// MIP selection is so different to Ps2. Limit the k value such that Xbox can never select smaller MIPs
				// earlier than it would do by default.
				if( pMat->K[pass] > 0.0f )
				{
					pMat->K[pass] = 0.0f;
				}
			}
			else
			{
				// ...otherwise just step past mipmap info.
				pMat->pTex[pass] = NULL;
				uint32 mip[4];
				File::Read( mip, sizeof( uint32 ) * 4, 1, p_FH );
			}
		}

		// Generate material colors.
		for( uint32 pass = 0; pass < pMat->Passes; ++pass )
		{
			if ( ( mat_col[0].r == mat_col[pass].r ) &&
				 ( mat_col[0].g == mat_col[pass].g ) && 
				 ( mat_col[0].b == mat_col[pass].b ) && 
				 ( mat_col[0].a == mat_col[pass].a ) )
			{
				// Do nothing.
				pMat->matcol[pass] = (GXColor){128,128,128,255};
			}
			else
			{
				// Work out the texture color.
				GXColor tc;
				float base_r = (float)(mat_col[0].r > 0 ? mat_col[0].r : 1 );
				float base_g = (float)(mat_col[0].g > 0 ? mat_col[0].g : 1 );
				float base_b = (float)(mat_col[0].b > 0 ? mat_col[0].b : 1 );
				float base_a = (float)(mat_col[0].a > 0 ? mat_col[0].a : 1 );
				float r = ( ((float)mat_col[pass].r) / base_r ) * 128.0f;
				float g = ( ((float)mat_col[pass].g) / base_g ) * 128.0f;
				float b = ( ((float)mat_col[pass].b) / base_b ) * 128.0f;
				float a = ( ((float)mat_col[pass].a) / base_a ) * 255.0f;
				int _r = (int)r;
				int _g = (int)g;
				int _b = (int)b;
				int _a = (int)a;
				if ( _r > 255 ) _r = 255;
				if ( _g > 255 ) _g = 255;
				if ( _b > 255 ) _b = 255;
				if ( _a > 255 ) _a = 255;
				tc.r = (u8)( _r );
				tc.g = (u8)( _g );
				tc.b = (u8)( _b );
				tc.a = (u8)( _a );
				pMat->matcol[pass] = tc;
			}
		}

//		pMat->mp_display_list = NULL;
//		pMat->Initialize();
		pMat->uv_slot[0] = 0;
		pMat->uv_slot[1] = 1;
		pMat->uv_slot[2] = 2;
		pMat->uv_slot[3] = 3;
	}

	return p_mat_array;
}

} // namespace NxNgc

