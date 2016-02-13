/********************************************************************************
 *																				*
 *	Module:																		*
 *				Render															*
 *	Description:																*
 *				Allows a rendering context to be opened and modified as desired	*
 *				by the user. A rendering context must be open before Prim or	*
 *				Model commands can be issued.									*
 *	Written by:																	*
 *				Paul Robinson													*
 *	Copyright:																	*
 *				2001 Neversoft Entertainment - All rights reserved.				*
 *																				*
 ********************************************************************************/

/********************************************************************************
 * Includes.																	*
 ********************************************************************************/
#include <math.h>
#include "p_render.h"
#include "p_assert.h"
#include "p_gx.h"

/********************************************************************************
 * Defines.																		*
 ********************************************************************************/

/********************************************************************************
 * Structures.																	*
 ********************************************************************************/

/********************************************************************************
 * Local variables.																*
 ********************************************************************************/
//static int tx = -1;
//static int bm = -1;
//static int km = -1;
//static int ai = -1;
//static int ci = -1;

static int		inRenderingContext	= 0;
//static bool		fogEnabled			= false;
//static GXColor	fogColor			= (GXColor){ 0, 0, 0, 0 };
//static float	fogNear				= 0.0f;
//static float	fogFar				= 0.0f;
//static NsZMode	zMode;
//static int		zCompare;
//static int		zUpdate;
//static int		zRejectBeforeTexture;

//u16 colorMap[(4*4)*2] __attribute__ (( aligned( 32 ))) =
//{
//	0x0000, 0x0055, 0x00aa, 0x00ff, 
//	0x0000, 0x0055, 0x00aa, 0x00ff, 
//	0x0000, 0x0055, 0x00aa, 0x00ff, 
//	0x0000, 0x0055, 0x00aa, 0x00ff, 
//	0x0000, 0x0000, 0x0000, 0x0000, 
//	0x5500, 0x5500, 0x5500, 0x5500, 
//	0xaa00, 0xaa00, 0xaa00, 0xaa00, 
//	0xff00, 0xff00, 0xff00, 0xff00  
//};

//u16 colorMap[(256*256)*2] __attribute__ (( aligned( 32 )));
//
//static bool colorMapMade = false;
//
//static void makeColorMap( void )
//{
//	for ( int y = 0; y < 256; y++ )
//	{
//		for ( int x = 0; x < 256; x++ )
//		{
//			int r;
//			int g;
//
//			r = 0;
//			r += ( x / 4 ) * 32;
//			r += ( x & 3 );
//			r += ( ( y / 4 ) * ( 256 * 4 * 2 ) );
//			r += ( ( y & 3 ) * 4 );
//
//			g = 16;
//			g += ( x / 4 ) * 32;
//			g += ( x & 3 );
//			g += ( ( y / 4 ) * ( 256 * 4 * 2 ) );
//			g += ( ( y & 3 ) * 4 );
//
//			// Red across.
//			colorMap[r] = x;
//			// Green down.
//			colorMap[g] = ( y << 8 );
//		}
//	}
//}


/********************************************************************************
 * Forward references.															*
 ********************************************************************************/

/********************************************************************************
 * Externs.																		*
 ********************************************************************************/

namespace NsRender
{
/********************************************************************************
 *																				*
 *	Method:																		*
 *				Render															*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Generates a rendering context.									*
 *																				*
 ********************************************************************************/
//Render::Render()
//{
//	inRenderingContext = 0;
//
//	tx = -1;
//	bm = -1;
//	km = -1;
//	ai = -1;
//	ci = -1;
//}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				begin															*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Opens the drawing context. This must be called before any		*
 *				primitives can be built. Sets up an orthogonal viewport.		*
 *				The viewport is set to 0.0f,0.0f at the top-left corner, and	*
 *				640.0f,480.0f at the bottom-right corner.						*
 *																				*
 ********************************************************************************/
void begin ( void )
{
//	if ( !colorMapMade )
//	{
//		makeColorMap();
//		colorMapMade = true;
//	}

	GX::begin();

//	Mtx44	mProj;								// projection matrix:
//    Mtx		mID;								// transformation matrix (set to identity)

//	assert ( !inRenderingContext );
	if ( inRenderingContext ) return;

    // disable lighting - use register color only
	GX::SetTexChanTevIndCull( 0, 2, 1, 0, GX_CULL_NONE );
	GX::SetClipMode(GX_CLIP_ENABLE);
	GX::SetChanCtrl( GX_COLOR0A0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
	GX::SetChanCtrl( GX_COLOR1A1, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );

    GX::SetTevSwapModeTable( GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA );
    GX::SetTevSwapModeTable( GX_TEV_SWAP1, GX_CH_BLUE, GX_CH_BLUE, GX_CH_RED, GX_CH_GREEN );
    GX::SetTevSwapModeTable( GX_TEV_SWAP2, GX_CH_BLUE, GX_CH_BLUE, GX_CH_BLUE, GX_CH_BLUE );
    GX::SetTevSwapModeTable( GX_TEV_SWAP3, GX_CH_RED, GX_CH_RED, GX_CH_RED, GX_CH_RED );
    
//	GX::SetTevSwapModeTable( GX_TEV_SWAP2, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_BLUE );
//	GX::SetTevSwapModeTable( GX_TEV_SWAP3, GX_CH_BLUE, GX_CH_BLUE, GX_CH_RED, GX_CH_GREEN );

//	// Make sure color texture is uploaded.
//	GXTexObj	texObj;
////	GXInitTexObj( &texObj, colorMap, 4, 4, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE );
//	GXInitTexObj( &texObj, colorMap, 256, 256, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE );
//	GXLoadTexObj( &texObj, GX_TEXMAP7 );
}

///********************************************************************************
// *																				*
// *	Method:																		*
// *				setBlendMode													*
// *	Inputs:																		*
// *				blendMode	The blend mode to set.								*
// *				pTex		The texture to set up.								*
// *				fixAlpha	The fixed alpha value to use for fixed alpha modes.	*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Provides a mechanism to set a variety of blend modes for flat	*
// *				or textured primitives. State is maintained between blend		*
// *				modes, so the fewest amount of state changes are made.			*
// *																				*
// ********************************************************************************/
//void setBlendMode ( NsBlendMode blendMode, /*NsTexture*/void * pTex, unsigned char fixAlpha )
//{
//	int				texval;
//
////	tx = -1;
//// 	bm = -1;
//// 	km = -1;
//// 	ai = -1;
//// 	ci = -1;
//
//	// PJR - Note: This should probably be in Tex::upload().
//	if ( pTex ) {
//		if ( tx != ( 1 | ( pTex->m_texID << 8 ) ) ) {
//			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, pTex->m_texID, GX_COLOR0A0);
//			GXSetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//		    if ( (tx & 255) != 1 ) {
//		    	GXSetNumTexGens( 1 );
//				GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//				GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//				GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
//			}
//			tx = ( 1 | ( pTex->m_texID << 8 ) );
//		}
//		texval = ( 1 << 8 );
//	} else {
//		if ( tx != 0 ) {
//		    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
//			GXSetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//		    GXSetNumTexGens( 0 );
//			GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//			GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//			GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
//			tx = 0;
//		}
//		texval = ( 0 << 8 );
//	}
//	// Set Blend Mode.
//	switch ( blendMode ) {
//		case NsBlendMode_Sub:		// Subtractive, using texture alpha
//		case NsBlendMode_SubAlpha:		// Subtractive, using constant alpha
//			if ( bm != 0 ) {
//				GXSetBlendMode ( GX_BM_SUBTRACT, GX_BL_ZERO, GX_BL_ZERO, GX_LO_CLEAR );
//				bm = 0;
//			}
//		    break;
//		case NsBlendMode_Blend:		// Blend, using texture alpha
//		case NsBlendMode_BlendAlpha:		// Blend, using constant alpha
//			if ( bm != 1 ) {
//				GXSetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR );
//				bm = 1;
//			}
//			break;
//		case NsBlendMode_AddAlpha:		// Additive, using constant alpha
//			if ( bm != 2 ) {
//				GXSetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR );
//				bm = 2;
//			}
//			break;
//		case NsBlendMode_AlphaTex:		// Blend framebuffer color and texture alpha.
//			if ( bm != 3 ) {
//				GXSetBlendMode ( GX_BM_BLEND, GX_BL_ZERO, GX_BL_SRCALPHA, GX_LO_CLEAR );
//				bm = 3;
//			}
//		    break;
//		case NsBlendMode_AlphaFB:		// Blend framebuffer color and texture alpha, plus framebuffer color.
//			if ( bm != 4 ) {		
//				GXSetBlendMode ( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_SRCALPHA, GX_LO_CLEAR );
//				bm = 4;
//			}
//		    break;
//		case NsBlendMode_Add:		// Additive, using texture alpha
//			if ( bm != 5 ) {
//				GXSetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR );
//				bm = 5;
//			}
//			break;
//		case NsBlendMode_None:		// Do nothing. No cut-out, no blend.
//		default:
//			if ( bm != 6 ) {
//				GXSetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR );
//				bm = 6;
//			}
//			break;
//	}
//	// Set Konstant.
//	switch ( blendMode ) {
//		case NsBlendMode_SubAlpha:		// Subtractive, using constant alpha
//			if ( km != ( 0 | ( fixAlpha << 8 ) ) ) {
//				GXSetTevKColorSel( GX_TEVSTAGE0, GX_TEV_KCSEL_K0_A );
//				GXSetTevKColor( GX_KCOLOR0, (GXColor){128,128,128,fixAlpha} );
//				km = 0 | ( fixAlpha << 8 );
//			}
//		case NsBlendMode_BlendAlpha:		// Blend, using constant alpha
//		case NsBlendMode_AddAlpha:		// Additive, using constant alpha
//			if ( km != ( 1 | ( fixAlpha << 8 ) ) ) {
//				GXSetTevKAlphaSel( GX_TEVSTAGE0, GX_TEV_KASEL_K0_A );
//				GXSetTevKColor( GX_KCOLOR0, (GXColor){128,128,128,fixAlpha} );
//				km = 1 | ( fixAlpha << 8 );
//			}
//		case NsBlendMode_None:		// Do nothing. No cut-out, no blend.
//		case NsBlendMode_Sub:		// Subtractive, using texture alpha
//		case NsBlendMode_Blend:		// Blend, using texture alpha
//		case NsBlendMode_Add:		// Additive, using texture alpha
//		case NsBlendMode_AlphaTex:		// Blend framebuffer color and texture alpha.
//		case NsBlendMode_AlphaFB:		// Blend framebuffer color and texture alpha, plus framebuffer color.
//		default:
//			break;
//	}
//	// Set Alpha input.
//	switch ( blendMode ) {
//		case NsBlendMode_BlendAlpha:		// Blend, using constant alpha
//		case NsBlendMode_AddAlpha:		// Additive, using constant alpha
//			if ( ai != 1 ) {
//				GXSetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_KONST, GX_CA_RASA, GX_CA_ZERO );
//				ai = 1;
//			}
//			break;
//		case NsBlendMode_Sub:		// Subtractive, using texture alpha
//		case NsBlendMode_SubAlpha:		// Subtractive, using constant alpha
//			break;
//		case NsBlendMode_None:		// Do nothing. No cut-out, no blend.
//		case NsBlendMode_Blend:		// Blend, using texture alpha
//		case NsBlendMode_Add:		// Additive, using texture alpha
//		case NsBlendMode_AlphaTex:		// Blend framebuffer color and texture alpha.
//		case NsBlendMode_AlphaFB:		// Blend framebuffer color and texture alpha, plus framebuffer color.
//		default:
//			if ( ai != ( 2 | texval ) ) {
//				if ( tx == 0 ) {
////					GXSetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ONE, GX_CA_RASA, GX_CA_ZERO );
//					GXSetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA );
//				} else {
//					GXSetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO );
//				}
//				ai = 2 | texval;
//			}
//			break;
//	}
//	// Set Color input.
//	switch ( blendMode ) {
//		case NsBlendMode_Sub:		// Subtractive, using texture alpha
//			if ( ci != ( 0 | texval ) ) {
//				if ( tx == 0 ) {
//					GXSetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_RASA, GX_CC_RASC, GX_CC_ZERO );
//				} else {
//
////out_reg = (d (op) ((1.0 - c)*a + c*b) + bias) * scale;
////
////dst_pix_clr = dst_pix_clr - src_pix_clr [clamped to zero]
////
////We want: dst-(texc*texa)
//
//
////out_reg = (0 + ((1.0 - TEXC)*0 + TEXA*RASC) + 0) * 2;
//
////out_reg = (d (op) ((1.0 - c)*a + c*b) + bias) * scale;
////out_reg = (d (op) ((1.0 - TEXA)*1 + TEXA*0) + bias) * scale;
//
////					GXSetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXA, GX_CC_RASC, GX_CC_ZERO );
//					GXSetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXA, GX_CC_TEXC, GX_CC_ZERO );
//				}
//				ci = 0 | texval;
//			}
//		    break;
//		case NsBlendMode_SubAlpha:		// Subtractive, using constant alpha
//			if ( ci != ( 1 | texval ) ) {
//				if ( tx == 0 ) {
//					GXSetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_KONST, GX_CC_RASC, GX_CC_ZERO );
//				} else {
//					GXSetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_KONST, GX_CC_TEXC, GX_CC_ZERO );
//				}
//				ci = 1 | texval;
//			}
//			break;
//		case NsBlendMode_AlphaTex:		// Blend framebuffer color and texture alpha.
//		    break;
//		case NsBlendMode_AlphaFB:		// Blend framebuffer color and texture alpha, plus framebuffer color.
//			if ( ci != 2 ) {
//				GXSetTevColorIn ( GX_TEVSTAGE0, GX_CC_ONE, GX_CC_ONE, GX_CC_ONE, GX_CC_ONE );
//				ci = 2;
//			}
//		    break;
//		case NsBlendMode_None:		// Do nothing. No cut-out, no blend.
//		case NsBlendMode_Blend:		// Blend, using texture alpha
//		case NsBlendMode_Add:		// Additive, using texture alpha
//		case NsBlendMode_BlendAlpha:		// Blend, using constant alpha
//		case NsBlendMode_AddAlpha:		// Additive, using constant alpha
//		default:
//			if ( ci != ( 3 | texval ) ) {
//				if ( tx == 0 ) {
////					GXSetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_RASA, GX_CC_RASC, GX_CC_ZERO );
//					GXSetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC );
//				} else {
//					GXSetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_RASC, GX_CC_TEXC, GX_CC_ZERO );
//				}
//
//				ci = 3 | texval;
//			}
//			break;
//	}
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				setBlendMode													*
// *	Inputs:																		*
// *				blendMode	The blend mode to set.								*
// *				pTex		The texture to set up.								*
// *				fixAlpha	The fixed alpha value to use for fixed alpha modes.	*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Provides a mechanism to set a variety of blend modes for flat	*
// *				or textured primitives. State is maintained between blend		*
// *				modes, so the fewest amount of state changes are made.			*
// *																				*
// ********************************************************************************/
//void setBlendMode ( NsBlendMode blendMode, /*NsTexture*/void * pTex, float fixAlpha )
//{
//	setBlendMode ( blendMode, pTex, (u8)(fixAlpha * 128.0f ) );
//}

///********************************************************************************
// *																				*
// *	Method:																		*
// *				setZMode														*
// *	Inputs:																		*
// *				mode				The z compare mode to use.					*
// *				zcompare			Sets whether the z compare happens.			*
// *				zupdate				Sets whether z buffer updating happens.		*
// *				rejectBeforeTexture	Sets where the z compare happens - if		*
// *									before texturing, all pixels will be		*
// *									tested. If after, only pixels that pass		*
// *									through the TEV stages will be tested.		*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Sets the hardware Z compare hardware to the specified values.	*
// *																				*
// ********************************************************************************/
//void setZMode ( NsZMode mode, int zcompare, int zupdate, int rejectBeforeTexture )
//{
//	GXSetZMode ( zcompare ? GX_TRUE : GX_FALSE, (GXCompare)mode, zupdate ? GX_TRUE : GX_FALSE );
//	GXSetZCompLoc( rejectBeforeTexture ? GX_TRUE : GX_FALSE );
//
//	zMode					= mode;
//	zCompare				= zcompare;
//	zUpdate					= zupdate;
//	zRejectBeforeTexture	= rejectBeforeTexture;
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				getZMode														*
// *	Inputs:																		*
// *				mode				The z compare mode to use.					*
// *				zcompare			Sets whether the z compare happens.			*
// *				zupdate				Sets whether z buffer updating happens.		*
// *				rejectBeforeTexture	Sets where the z compare happens - if		*
// *									before texturing, all pixels will be		*
// *									tested. If after, only pixels that pass		*
// *									through the TEV stages will be tested.		*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Sets the hardware Z compare hardware to the specified values.	*
// *																				*
// ********************************************************************************/
//void getZMode ( NsZMode* mode, int* zcompare, int* zupdate, int* rejectBeforeTexture )
//{
//	*mode					= zMode;
//	*zcompare				= zCompare;
//	*zupdate				= zUpdate;
//	*rejectBeforeTexture	= zRejectBeforeTexture;
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				setCullMode														*
// *	Inputs:																		*
// *				mode	The polygon cull mode to use.							*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Sets the hardware polygon culling mode.							*
// *																				*
// ********************************************************************************/
//void setCullMode ( NsCullMode cullMode )
//{
//	GXSetCullMode ( (GXCullMode)cullMode );
//}
//
//
//
//bool enableFog( bool enable )
//{
//	// Return what state fog was in prior to this call.
//	bool rv = fogEnabled;
//	if( enable != fogEnabled )
//	{
//		fogEnabled = enable;
//		if( fogEnabled )
//		{
//			GXSetFog( GX_FOG_LIN, fogNear, fogFar, 0.1f, 100000.0f, fogColor );
//		}
//		else
//		{
//			GXSetFog( GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, fogColor );
//		}
//	}
//	return rv;
//}
//
//
//
//void setFogColor( int r, int g, int b )
//{
//	fogColor.r = r;
//	fogColor.g = g;
//	fogColor.b = b;
//	fogColor.a = 0;
//	if( fogEnabled )
//	{
//		GXSetFog( GX_FOG_LIN, fogNear, fogFar, 0.1f, 100000.0f, fogColor );
//	}
//}
//
//
//
//void setFogNear( float near )
//{
//	fogNear = near;
//	if( fogEnabled )
//	{
//		GXSetFog( GX_FOG_LIN, fogNear, fogFar, 0.1f, 100000.0f, fogColor );
//	}
//}
//
//
//
//void setFogFar( float far )
//{
//	fogFar = far;
//	if( fogEnabled )
//	{
//		GXSetFog( GX_FOG_LIN, fogNear, fogFar, 0.1f, 100000.0f, fogColor );
//	}
//}
//
//
//
//NsBlendMode	getBlendMode( unsigned int blend0 )
//{
//	// Same functionality as in CRW, convert PS2 style blend mode to Gamecube.
//	// Useful in cases such as the terrain particles.
//
//	NsBlendMode	result;
//
//	switch( blend0 )
//	{
//		case 0x00000000:
//		case 0x0000000a:
//		case 0x0000001a:
//		case 0x0000002a:
//			// Direct write.
//			result = NsBlendMode_None;
//			break;
//		case 0x00000042:
//			// Subtractive fb/texel.
//			result = NsBlendMode_Sub;
//			break;
//		case 0x00000044:
//			// Blend fb/texel.
//			result = NsBlendMode_Blend;
//			break;
//		case 0x00000048:
//			// Additive fb/texel.
//			result = NsBlendMode_Add;
//			break;
//		case 0x00000062:
//			// Constant subtractive.
//			result		= NsBlendMode_SubAlpha;
//			break;
//		case 0x00000064:
//			// Constant blend.
//			result = NsBlendMode_BlendAlpha;
//			break;
//		case 0x00000068:
//			// Constant additive.
//			result = NsBlendMode_AddAlpha;
//			break;
//		case 0x00000089:
//			// Blend fb * texture alpha.
//			result = NsBlendMode_AlphaTex;
//			break;
//		case 0x00000046:
//			// Blend fb * inv(texture alpha).
//			result = NsBlendMode_InvAlphaTex;
//			break;
//		case 0x00000049:
//			// Blend ( fbRGB * texture alpha ) + fbRGB.
//			result = NsBlendMode_AlphaFB;
//			break;
//		case 0x00000080:
//		case 0x000000A0:
//			// Output RGB 0,0,0.
//			result = NsBlendMode_Black;
//			break;
//		default:
//			// Default: Direct write.
//			result = NsBlendMode_None;
////			printf ( "\n+++++ Unknown blend mode %08x,%08x", blend0, blend1 );
//			break;
//	}
//	return result;
//}
//
//
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				end																*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Closes the rendering context. Prim and Model commands can no	*
// *				longer be issued.												*
// *																				*
// ********************************************************************************/
void end ( void )
{
//	assert ( inRenderingContext );

   //=========================================================

    // restore previous state
    GX::SetZMode( GX_TRUE, GX_LEQUAL, GX_TRUE );

	inRenderingContext = 0;

	GX::end();
}

}		// namespace Render
