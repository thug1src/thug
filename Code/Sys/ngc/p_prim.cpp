/********************************************************************************
 *																				*
 *	Module:																		*
 *				NsPrim															*
 *	Description:																*
 *				Provides functionality for drawing various types of single		*
 *				primitives to the display.										*
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
#include "p_prim.h"
#include "p_assert.h"
#include "p_render.h"
//#include "p_tex.h"
#include 	"gfx/ngc/nx/mesh.h"
#include <sys/ngc/p_gx.h>

/********************************************************************************
 * Defines.																		*
 ********************************************************************************/

/********************************************************************************
 * Structures.																	*
 ********************************************************************************/

/********************************************************************************
 * Local variables.																*
 ********************************************************************************/
int inDrawingContext = 0;

/********************************************************************************
 * Forward references.															*
 ********************************************************************************/

/********************************************************************************
 * Externs.																		*
 ********************************************************************************/

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
void NsPrim::begin ( void )
{
	inDrawingContext = 1;
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				point															*
 *	Inputs:																		*
 *				x, y, z		coordinate of point.								*
 *				color		color of the point.									*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Builds a point primitive.										*
 *																				*
 ********************************************************************************/
void NsPrim::point ( float x, float y, float z, GXColor color )
{
//	assert ( inDrawingContext );

    // Set current vertex descriptor to enable position and color0.
    // Both use 8b index to access their data arrays.
    GX::SetVtxDesc( 1, GX_VA_POS, GX_DIRECT );

    // Set material color.
    GX::SetChanMatColor( GX_COLOR0A0, color );
    GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){128,128,128,255} );

    GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );

	// Send coordinates.
    GX::Begin( GX_POINTS, GX_VTXFMT0, 1 ); 
        GX::Position3f32(x, y, z);
    GX::End();
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				point															*
 *	Inputs:																		*
 *				x, y		coordinate of point.								*
 *				color		color of the point.									*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Builds a point primitive.										*
 *																				*
 ********************************************************************************/
void NsPrim::point ( float x, float y, GXColor color )
{
	point ( x, y, -1.0f, color );
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				line															*
 *	Inputs:																		*
 *				x0, y0, z0	coordinates of end 0.								*
 *				x1, y1, z0	coordinates of end 1.								*
 *				color		color of the primitive.								*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Builds a flat color line primitive.								*
 *																				*
 ********************************************************************************/
void NsPrim::line ( float x0, float y0, float z0, float x1, float y1, float z1, GXColor color )
{
//	assert ( inDrawingContext );

    // Set current vertex descriptor to enable position and color0.
    // Both use 8b index to access their data arrays.
    GX::SetVtxDesc( 1, GX_VA_POS, GX_DIRECT );

    // Set material color.
    GX::SetChanMatColor( GX_COLOR0A0, color );
    GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){128,128,128,255} );

    GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );

	// Send coordinates.
    GX::Begin( GX_LINES, GX_VTXFMT0, 2 ); 
        GX::Position3f32(x0, y0, z0);
        GX::Position3f32(x1, y1, z1);
    GX::End();
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				line															*
 *	Inputs:																		*
 *				x0, y0		coordinates of end 0.								*
 *				x1, y1		coordinates of end 1.								*
 *				color		color of the primitive.								*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Builds a flat color line primitive.								*
 *																				*
 ********************************************************************************/
void NsPrim::line ( float x0, float y0, float x1, float y1, GXColor color )
{
	line ( x0, y0, -1.0f, x1, y1, -1.0f, color );
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				line															*
 *	Inputs:																		*
 *				x0, y0, z0	coordinates of end 0.								*
 *				x1, y1, z0	coordinates of end 1.								*
 *				color0		color of the primitive at corner 0.					*
 *				color1		color of the primitive at corner 1.					*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Builds a flat color line primitive.								*
 *																				*
 ********************************************************************************/
void NsPrim::line ( float x0, float y0, float z0, float x1, float y1, float z1, GXColor color0, GXColor color1 )
{
//	assert ( inDrawingContext );

    // Set current vertex descriptor to enable position and color0.
    // Both use 8b index to access their data arrays.
    GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_CLR0, GX_DIRECT );

    GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );

	// Send coordinates.
    GX::Begin( GX_LINES, GX_VTXFMT0, 2 ); 
        GX::Position3f32(x0, y0, z0);
		GX::Color1u32(*((u32*)&color0));
        GX::Position3f32(x1, y1, z1);
		GX::Color1u32(*((u32*)&color1));
    GX::End();
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				line															*
 *	Inputs:																		*
 *				x0, y0		coordinates of end 0.								*
 *				x1, y1		coordinates of end 1.								*
 *				color0		color of the primitive at corner 0.					*
 *				color1		color of the primitive at corner 1.					*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Builds a flat color line primitive.								*
 *																				*
 ********************************************************************************/
void NsPrim::line ( float x0, float y0, float x1, float y1, GXColor color0, GXColor color1 )
{
	line ( x0, y0, -1.0f, x1, y1, -1.0f, color0, color1 );
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				tri																*
 *	Inputs:																		*
 *				x0, y0, z0	coordinates of corner 0.							*
 *				x1, y1, z1	coordinates of corner 1.							*
 *				x2, y2, z2	coordinates of corner 2.							*
 *				color		color of the primitive.								*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Builds a flat color triangle primitive.							*
 *																				*
 ********************************************************************************/
void NsPrim::tri ( float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, GXColor color )
{
//	assert ( inDrawingContext );

    // Set current vertex descriptor to enable position and color0.
    // Both use 8b index to access their data arrays.
    GX::SetVtxDesc( 1, GX_VA_POS, GX_DIRECT );

    // Set material color.
    GX::SetChanMatColor( GX_COLOR0A0, color );
    GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){128,128,128,255} );

    GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );

	// Send coordinates.
    GX::Begin( GX_TRIANGLES, GX_VTXFMT0, 3 ); 
        GX::Position3f32(x0, y0, z0);
        GX::Position3f32(x1, y1, z1);
        GX::Position3f32(x2, y2, z2);
    GX::End();
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				tri																*
 *	Inputs:																		*
 *				x0, y0		coordinates of corner 0.							*
 *				x1, y1		coordinates of corner 1.							*
 *				x2, y2		coordinates of corner 2.							*
 *				color		color of the primitive.								*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Builds a flat color triangle primitive.							*
 *																				*
 ********************************************************************************/
void NsPrim::tri ( float x0, float y0, float x1, float y1, float x2, float y2, GXColor color )
{
	tri ( x0, y0, -1.0f, x1, y1, -1.0f, x2, y2, -1.0f, color );
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				tri																*
 *	Inputs:																		*
 *				x0, y0, z0	coordinates of corner 0.							*
 *				x1, y1, z1	coordinates of corner 1.							*
 *				x2, y2, z2	coordinates of corner 2.							*
 *				color0		color of the primitive at corner 0.					*
 *				color1		color of the primitive at corner 1.					*
 *				color2		color of the primitive at corner 2.					*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Builds a gouraud-shaded triangle primitive.						*
 *																				*
 ********************************************************************************/
void NsPrim::tri ( float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, GXColor color0, GXColor color1, GXColor color2 )
{
//	assert ( inDrawingContext );

    // Set current vertex descriptor to enable position and color0.
    // Both use 8b index to access their data arrays.
    GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_CLR0, GX_DIRECT );

    GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE );

	// Send coordinates.
    GX::Begin( GX_TRIANGLES, GX_VTXFMT0, 3 ); 
        GX::Position3f32(x0, y0, z0);
		GX::Color1u32(*((u32*)&color0));
        GX::Position3f32(x1, y1, z1);
		GX::Color1u32(*((u32*)&color1));
        GX::Position3f32(x2, y2, z2);
		GX::Color1u32(*((u32*)&color2));
    GX::End();
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				tri																*
 *	Inputs:																		*
 *				x0, y0		coordinates of corner 0.							*
 *				x1, y1		coordinates of corner 1.							*
 *				x2, y2		coordinates of corner 2.							*
 *				color0		color of the primitive at corner 0.					*
 *				color1		color of the primitive at corner 1.					*
 *				color2		color of the primitive at corner 2.					*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Builds a gouraud-shaded triangle primitive.						*
 *																				*
 ********************************************************************************/
void NsPrim::tri ( float x0, float y0, float x1, float y1, float x2, float y2, GXColor color0, GXColor color1, GXColor color2 )
{
	tri ( x0, y0, -1.0f, x1, y1, -1.0f, x2, y2, -1.0f, color0, color1, color2 );
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				quad															*
 *	Inputs:																		*
 *				x0, y0, z0	coordinates of corner 0.							*
 *				x1, y1, z1	coordinates of corner 1.							*
 *				x2, y2, z2	coordinates of corner 2.							*
 *				x3, y3, z3	coordinates of corner 3.							*
 *				color		color of the primitive.								*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Builds a flat color quad primitive.								*
 *																				*
 ********************************************************************************/
void NsPrim::quad ( float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, GXColor color )
{
//	assert ( inDrawingContext );

    // Set current vertex descriptor to enable position and color0.
    // Both use 8b index to access their data arrays.
    GX::SetVtxDesc( 1, GX_VA_POS, GX_DIRECT );

    // Set material color.
    GX::SetChanMatColor( GX_COLOR0A0, color );
    GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){128,128,128,255} );

    GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );

	// Send coordinates.
    GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
        GX::Position3f32(x0, y0, z0);
        GX::Position3f32(x1, y1, z1);
        GX::Position3f32(x2, y2, z2);
        GX::Position3f32(x3, y3, z3);
    GX::End();
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				quad															*
 *	Inputs:																		*
 *				x0, y0		coordinates of corner 0.							*
 *				x1, y1		coordinates of corner 1.							*
 *				x2, y2		coordinates of corner 2.							*
 *				x3, y3		coordinates of corner 3.							*
 *				color		color of the primitive.								*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Builds a flat color quad primitive.								*
 *																				*
 ********************************************************************************/
void NsPrim::quad ( float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, GXColor color )
{
	quad ( x0, y0, -1.0f, x1, y1, -1.0f, x2, y2, -1.0f, x3, y3, -1.0f, color );
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				quad															*
 *	Inputs:																		*
 *				x0, y0, z0	coordinates of corner 0.							*
 *				x1, y1, z1	coordinates of corner 1.							*
 *				x2, y2, z2	coordinates of corner 2.							*
 *				x3, y3, z3	coordinates of corner 3.							*
 *				color0		color of the primitive at corner 0.					*
 *				color1		color of the primitive at corner 1.					*
 *				color2		color of the primitive at corner 2.					*
 *				color3		color of the primitive at corner 3.					*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Builds a gouraud-shaded quad primitive.							*
 *																				*
 ********************************************************************************/
void NsPrim::quad ( float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, GXColor color0, GXColor color1, GXColor color2, GXColor color3 )
{
//	assert ( inDrawingContext );

    // Set current vertex descriptor to enable position and color0.
    // Both use 8b index to access their data arrays.
    GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_CLR0, GX_DIRECT );

    GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );

	// Send coordinates.
    GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
        GX::Position3f32(x0, y0, z0);
		GX::Color1u32(*((u32*)&color0));
        GX::Position3f32(x1, y1, z1);
		GX::Color1u32(*((u32*)&color1));
        GX::Position3f32(x2, y2, z2);
		GX::Color1u32(*((u32*)&color2));
        GX::Position3f32(x3, y3, z3);
		GX::Color1u32(*((u32*)&color3));
    GX::End();
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				quad															*
 *	Inputs:																		*
 *				x0, y0		coordinates of corner 0.							*
 *				x1, y1		coordinates of corner 1.							*
 *				x2, y2		coordinates of corner 2.							*
 *				x3, y3		coordinates of corner 3.							*
 *				color0		color of the primitive at corner 0.					*
 *				color1		color of the primitive at corner 1.					*
 *				color2		color of the primitive at corner 2.					*
 *				color3		color of the primitive at corner 3.					*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Builds a gouraud-shaded quad primitive.							*
 *																				*
 ********************************************************************************/
void NsPrim::quad ( float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, GXColor color0, GXColor color1, GXColor color2, GXColor color3 )
{
	quad ( x0, y0, -1.0f, x1, y1, -1.0f, x2, y2, -1.0f, x3, y3, -1.0f, color0, color1, color2, color3 );
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				frame															*
 *	Inputs:																		*
 *				left	left side position of the orthogonal region.			*
 *				right	right side position of the orthogonal region.			*
 *				top		top side position of the orthogonal region.				*
 *				bottom	bottom side position of the orthogonal region.			*
 *				color	color of the lines.										*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Draws the perimiter lines to an orthogonal region.				*
 *																				*
 ********************************************************************************/
void NsPrim::frame ( float left, float top, float right, float bottom, GXColor color )
{
	line ( left,  top,    right, top,    color );
	line ( left,  bottom, right, bottom, color );
	line ( left,  top,    left,  bottom, color );
	line ( right, top,    right, bottom, color );
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				box																*
 *	Inputs:																		*
 *				left	left side position of the orthogonal region.			*
 *				right	right side position of the orthogonal region.			*
 *				top		top side position of the orthogonal region.				*
 *				bottom	bottom side position of the orthogonal region.			*
 *				color	color of the box.										*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Draws a solid box representing an orthogonal region.			*
 *																				*
 ********************************************************************************/
void NsPrim::box ( float left, float top, float right, float bottom, GXColor color )
{
	quad ( left, top, right, top, right, bottom, left, bottom, color );
}

///********************************************************************************
// *																				*
// *	Method:																		*
// *				quad															*
// *	Inputs:																		*
// *				x0, y0, z0	coordinates of corner 0.							*
// *				x1, y1, z1	coordinates of corner 1.							*
// *				x2, y2, z2	coordinates of corner 2.							*
// *				x3, y3, z3	coordinates of corner 3.							*
// *				color		color of the primitive.								*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Builds a flat color quad primitive.								*
// *																				*
// ********************************************************************************/
//void NsPrim::sprite ( NsTexture * pTexture, float x0, float y0, float z0, float x1, float y1, float z1 )
//{
//	int		upheight;
//
//	assert ( inDrawingContext );
//
//    // Set current vertex descriptor to enable position and color0.
//    // Both use 8b index to access their data arrays.
//    GXClearVtxDesc();
//    GXSetVtxDesc( GX_VA_POS, GX_DIRECT );
//    GXSetVtxDesc( GX_VA_TEX0, GX_DIRECT );
//
//    // Set material color.
//    GX::SetChanMatColor( GX_COLOR0A0, (GXColor){128,128,128,128} );
//    GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//
//    GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//
////	if ( (pTexture->width > 512) || (pTexture->height > 512) ) {
//		int		lp, lp2;
//		int		w, h;
//		char  * p8;
//		float	fx0, fy0;
//		float	fx1, fy1;
//		float	bv;
//
//		p8 = (char *)pTexture->m_pImage;
//		for ( lp2 = 0; lp2 < pTexture->m_height; lp2 += 512 ) {
//			h = (pTexture->m_height - lp2) < 512 ? (pTexture->m_height - lp2) : 512;
//			upheight = (h&(h-1)) ? ((~((h>>1)|(h>>2)|(h>>3)|(h>>4)|(h>>5)|(h>>6)|(h>>7)|(h>>8)|(h>>9)))&h)<<1 : h;
//			for ( lp = 0; lp < pTexture->m_width; lp += 512 ) {
//				w = (pTexture->m_width - lp) < 512 ? (pTexture->m_width - lp) : 512;
//				// Upload this segment.
//				pTexture->upload( NsTexture_Wrap_Clamp, (void *)p8, w, upheight );
//				p8 += ( w * h * pTexture->m_depth ) / 8;
//
//				// Calculate coordinates.
//				fx0 = ( lp        * ( x1 - x0 ) ) / pTexture->m_width;
//				fx1 = ( (lp + w)  * ( x1 - x0 ) ) / pTexture->m_width;
//				fy0 = ( lp2       * ( y1 - y0 ) ) / pTexture->m_height;
//				fy1 = ( (lp2 + h) * ( y1 - y0 ) ) / pTexture->m_height;
////				OSReport ( "Upload: %4d,%4d %4dx%4d   Draw: %6.1f,%6.1f %6.1f,%6.1f\n", lp, lp2, w, h, fx0, fy0, fx1, fy1 );
//
//				bv = (float)h / (float)upheight;
//				// Send coordinates.
//			    GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
//			        GXPosition3f32(fx0, fy0, -1.0f);
//					GXTexCoord2f32(0.0f, 0.0f);
//			        GXPosition3f32(fx1, fy0, -1.0f);
//					GXTexCoord2f32(1.0f, 0.0f);
//			        GXPosition3f32(fx1, fy1, -1.0f);
//					GXTexCoord2f32(1.0f, bv);
//			        GXPosition3f32(fx0, fy1, -1.0f);
//					GXTexCoord2f32(0.0f, bv);
//			    GXEnd();
//			}
//		}
////	} else {
////		int h;
////		float	bu;
////		float	bv;
////		h = pTexture->m_height;
////		upheight = (h&(h-1)) ? ((~((h>>1)|(h>>2)|(h>>3)|(h>>4)|(h>>5)|(h>>6)|(h>>7)|(h>>8)|(h>>9)))&h)<<1 : h;
////
////		pTexture->upload( NsTexture_Wrap_Clamp, pTexture->m_pImage, pTexture->m_width, upheight );
//////		pTexture->upload( NsTexture_Wrap_Clamp );
////
////		NsRender::setBlendMode ( NsBlendMode_Blend, pTexture, (unsigned char)128 );
////
////
////
////		bu = 1.0f;
////		bv = (float)h / (float)upheight;
////		// Send coordinates.
////	    GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
////	        GXPosition3f32(x0, y0, -1.0f);
////			GXTexCoord2f32(0.0f, 0.0f);
////	        GXPosition3f32(x1, y0, -1.0f);
////			GXTexCoord2f32(bu, 0.0f);
////	        GXPosition3f32(x1, y1, -1.0f);
////			GXTexCoord2f32(bu, bv);
////	        GXPosition3f32(x0, y1, -1.0f);
////			GXTexCoord2f32(0.0f, bv);
////	    GXEnd();
////	}
//}
//
//void NsPrim::sprite ( NsTexture * pTexture, float x, float y, float z )
//{
//	sprite ( pTexture, x, y, z, x + (float)pTexture->m_width, y + (float)pTexture->m_height, z );
//}
//
////void sprite ( Tex * pTexture, float x, float y )
////{
////	sprite ( pTexture, x, y, -1.0f, x + (float)pTexture->m_width, y + (float)pTexture->m_height, -1.0f );
////}
////
////void sprite ( Tex * pTexture, float x0, float y0, float x1, float y1 )
////{
////	sprite ( pTexture, x0, y0, -1.0f, x1, y1, -1.0f );
////}
//
////void sprite ( const char * pName, float x0, float y0, float z0, float x1, float y1, float z1 )
////{
////	Tex
////}
//
//void NsPrim::subsprite ( NsTexture * pTexture, float x, float y, short sx, short sy, short sw, short sh )
//{
//	float x0, y0, x1, y1;
//
//	assert ( inDrawingContext );
//
//	x0 = x;
//	y0 = y;
//	x1 = x + (float)sw;
//	y1 = y + (float)sh;
//
//	pTexture->upload( NsTexture_Wrap_Clamp );
//
//    // Set current vertex descriptor to enable position and color0.
//    // Both use 8b index to access their data arrays.
//    GXClearVtxDesc();
//    GXSetVtxDesc( GX_VA_POS, GX_DIRECT );
//    GXSetVtxDesc( GX_VA_TEX0, GX_DIRECT );
//
//    // Set material color.
//    GX::SetChanMatColor( GX_COLOR0A0, (GXColor){128,128,128,128} );
//    GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//
//    GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//
//	// Send coordinates.
//    GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
//        GXPosition3f32(x0, y0, -1.0f);
//		GXTexCoord2f32((float)sx/(float)pTexture->m_width, (float)sy/(float)pTexture->m_height);
//        GXPosition3f32(x1, y0, -1.0f);
//		GXTexCoord2f32(((float)sx+(float)sw)/(float)pTexture->m_width, (float)sy/(float)pTexture->m_height);
//        GXPosition3f32(x1, y1, -1.0f);
//		GXTexCoord2f32(((float)sx+(float)sw)/(float)pTexture->m_width, ((float)sy+(float)sh)/(float)pTexture->m_height);
//        GXPosition3f32(x0, y1, -1.0f);
//		GXTexCoord2f32((float)sx/(float)pTexture->m_width, ((float)sy+(float)sh)/(float)pTexture->m_height);
//    GXEnd();
//}
//
//void NsPrim::subsprite ( NsTexture * pTexture, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1 )
//{
//	assert ( inDrawingContext );
//
//	pTexture->upload( NsTexture_Wrap_Clamp );
//
//    // Set current vertex descriptor to enable position and color0.
//    // Both use 8b index to access their data arrays.
//    GXClearVtxDesc();
//    GXSetVtxDesc( GX_VA_POS, GX_DIRECT );
//    GXSetVtxDesc( GX_VA_TEX0, GX_DIRECT );
//
//    // Set material color.
//    GX::SetChanMatColor( GX_COLOR0A0, (GXColor){128,128,128,128} );
//    GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//
//    GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//
//	// Send coordinates.
//    GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
//        GXPosition3f32(x0, y0, -1.0f);
//		GXTexCoord2f32(u0, v0);
//        GXPosition3f32(x1, y0, -1.0f);
//		GXTexCoord2f32(u1, v0);
//        GXPosition3f32(x1, y1, -1.0f);
//		GXTexCoord2f32(u1, v1);
//        GXPosition3f32(x0, y1, -1.0f);
//		GXTexCoord2f32(u0, v1);
//    GXEnd();
//}
//
//void NsPrim::subsprite ( NsTexture * pTexture, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, float u0, float v0, float u1, float v1, float u2, float v2, float u3, float v3, float z, GXColor color )
//{
//	assert ( inDrawingContext );
//
//	pTexture->upload( NsTexture_Wrap_Clamp );
//
//    // Set current vertex descriptor to enable position and color0.
//    // Both use 8b index to access their data arrays.
//    GXClearVtxDesc();
//    GXSetVtxDesc( GX_VA_POS, GX_DIRECT );
//    GXSetVtxDesc( GX_VA_TEX0, GX_DIRECT );
//
//    // Set material color.
//    GX::SetChanMatColor( GX_COLOR0A0, color );
//    GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//
//    GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//
//	// Send coordinates.
//    GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
//        GXPosition3f32(x0, y0, z);
//		GXTexCoord2f32(u0, v0);
//        GXPosition3f32(x1, y1, z);
//		GXTexCoord2f32(u1, v1);
//        GXPosition3f32(x2, y2, z);
//		GXTexCoord2f32(u2, v2);
//        GXPosition3f32(x3, y3, z);
//		GXTexCoord2f32(u3, v3);
//    GXEnd();
//}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				end																*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Closes the drawing context. This must be called after you are	*
 *				finished building primitives. Restores the viewport which was	*
 *				current at the time of the call to begin().						*
 *																				*
 ********************************************************************************/
void NsPrim::end ( void )
{
//	assert ( inDrawingContext );

	inDrawingContext = 0;
}

int NsPrim::begun ( void )
{
	return inDrawingContext;
}
