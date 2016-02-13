///********************************************************************************
// *																				*
// *	Module:																		*
// *				Render															*
// *	Description:																*
// *				Allows a rendering context to be opened and modified as desired	*
// *				by the user. A rendering context must be open before Prim or	*
// *				Model commands can be issued.									*
// *	Written by:																	*
// *				Paul Robinson													*
// *	Copyright:																	*
// *				2001 Neversoft Entertainment - All rights reserved.				*
// *																				*
// ********************************************************************************/
//
///********************************************************************************
// * Includes.																	*
// ********************************************************************************/
//#include <math.h>
//#include <stdarg.h>
//#include "p_debugfont.h"
//#include "p_render.h"
//
///********************************************************************************
// * Defines.																		*
// ********************************************************************************/
//
///********************************************************************************
// * Structures.																	*
// ********************************************************************************/
//
///********************************************************************************
// * Local variables.																*
// ********************************************************************************/
//float ySpacing = 0.0f;
//
///********************************************************************************
// * Forward references.															*
// ********************************************************************************/
//
///********************************************************************************
// * Externs.																		*
// ********************************************************************************/
//extern unsigned char debugFont[(512/8)*128];
//
//namespace NsDebugFont
//{
///********************************************************************************
// *																				*
// *	Method:																		*
// *				print															*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Opens the drawing context. This must be called before any		*
// *				primitives can be built. Sets up an orthogonal viewport.		*
// *				The viewport is set to 0.0f,0.0f at the top-left corner, and	*
// *				640.0f,480.0f at the bottom-right corner.						*
// *																				*
// ********************************************************************************/
//void printCharacterScaleUp ( float x, float y, int size, GXColor color, char c )
//{
//	unsigned char * p8;
//	int				xx;
//	int				yy;
//	unsigned short	line;
//	float			scale;
//	float			sx;
//	float			sy;
//	float			ex;
//	float			ey;
//
//	p8 = &debugFont[((c&0x1f)*2)+((c&0xe0)*32)];
//	scale = (float)size / 16.0f;
//
//	for ( yy = 0; yy < 16; yy++ ) {
//		// Read this line.
//		line = ( p8[0] << 8 ) | p8[1];
//		for ( xx = 0; xx < 16; xx++ ) {
//			if ( line & ( 0x8000 >> xx ) ) {
//				sx = x+(((float)xx)*scale);
//				sy = y+(((float)yy)*scale);
//				ex = x+(((float)xx+1)*scale);
//				ey = y+(((float)yy+1)*scale);
//				NsPrim::box ( sx, sy, ex, ey, color );
//			}
//		}
//		// Move down a line.
//		p8 += (512/8);
//	}
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				print															*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Opens the drawing context. This must be called before any		*
// *				primitives can be built. Sets up an orthogonal viewport.		*
// *				The viewport is set to 0.0f,0.0f at the top-left corner, and	*
// *				640.0f,480.0f at the bottom-right corner.						*
// *																				*
// ********************************************************************************/
//void printCharacter ( float x, float y, int size, GXColor color, char c )
//{
//	unsigned char * p8;
//	int				xx;
//	int				yy;
//	unsigned short	line;
//
//	if ( size < 16 ) {
//		printCharacterScaleDown ( x, y, size, color, c );
//	} else if ( size == 16 ) {
//
//		p8 = &debugFont[((c&0x1f)*2)+((c&0xe0)*32)];
//
//		for ( yy = 0; yy < 16; yy++ ) {
//			// Read this line.
//			line = ( p8[0] << 8 ) | p8[1];
//			for ( xx = 0; xx < 16; xx++ ) {
//				if ( line & ( 0x8000 >> xx ) ) {
//					NsPrim::point ( x+(float)xx, y+(float)yy, color );
//				}
//			}
//			// Move down a line.
//			p8 += (512/8);
//		}
//	} else {
//		printCharacterScaleUp ( x, y, size, color, c );
//	}
//
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				print															*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Opens the drawing context. This must be called before any		*
// *				primitives can be built. Sets up an orthogonal viewport.		*
// *				The viewport is set to 0.0f,0.0f at the top-left corner, and	*
// *				640.0f,480.0f at the bottom-right corner.						*
// *																				*
// ********************************************************************************/
//void printCharacterScaleDown ( float x, float y, int size, GXColor color, char c )
//{
//	unsigned char * p8;
//	int				xx;
//	int				yy;
//	unsigned short	line;
//	float			scale;
//	unsigned char	intensity[16][16];
//	unsigned char	area[16][16];
//
//	scale = (float)size / 16.0f;
//	p8 = &debugFont[((c&0x1f)*2)+((c&0xe0)*32)];
//
//	// Reset Intensity level.
//	for ( yy = 0; yy < size; yy++ ) {
//		for ( xx = 0; xx < size; xx++ ) {
//			intensity[yy][xx] = 0;
//			area[yy][xx] = 0;
//		}
//	}
//
//	// Set Intensity level.
//	for ( yy = 0; yy < 16; yy++ ) {
//		line = ( p8[0] << 8 ) | p8[1];
//		for ( xx = 0; xx < 16; xx++ ) {
//			if ( line & ( 0x8000 >> xx ) ) intensity[(int)((float)yy*scale)][(int)((float)xx*scale)]++;
//			area[(int)((float)yy*scale)][(int)((float)xx*scale)]+=2;
//		}
//		// Move down a line.
//		p8 += 512/8;
//	}
//
//	// Draw pixels.
//	for ( yy = 0; yy < size; yy++ ) {
//		for ( xx = 0; xx < size; xx++ ) {
//			if ( intensity[yy][xx] ) {
//				NsPrim::point ( x+(float)xx, y+(float)yy, (GXColor) { (color.r>>1)+((color.r*intensity[yy][xx])/area[yy][xx]), (color.g>>1)+((color.g*intensity[yy][xx])/area[yy][xx]), (color.b>>1)+((color.b*intensity[yy][xx])/area[yy][xx]), color.a } );
//			}
//		}
//	}
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				print															*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Opens the drawing context. This must be called before any		*
// *				primitives can be built. Sets up an orthogonal viewport.		*
// *				The viewport is set to 0.0f,0.0f at the top-left corner, and	*
// *				640.0f,480.0f at the bottom-right corner.						*
// *																				*
// ********************************************************************************/
//void printf ( float x, float y, NsFontEffect fe, int size, GXColor color, char * format, ... )
//{
//	va_list	marker;
////	int		lp;
//	char	buffer[1024*8];
//	char  * p8;
//	float	left;
//	float	cx;
//	float	cy;
//
//	// Call our sprintf which correctly handles float, double and long double.
//	va_start ( marker, format );
//	vsprintf ( buffer, format, marker );
//
//	left = x;
//	cx = x;
//	cy = y;
//
//	NsRender::setZMode ( NsZMode_Always, 0, 0, 0 );
//
//	p8 = buffer;
//	while ( *p8 ) {
//		switch ( *p8 ) {
//			case '\n':
//				cx = left;
//				cy += (float)size + ySpacing;
//				break;
//			default:
//				switch ( fe ) {
//					case NsFontEffect_Shadow:
//						printCharacter ( cx+1.0f, cy+1.0f, size, (GXColor){0,0,0,128}, *p8 );
//						printCharacter ( cx, cy, size, color, *p8 );
//						break;
//					case NsFontEffect_Outline:
//						printCharacter ( cx-1.0f, cy, size, (GXColor){0,0,0,128}, *p8 );
//						printCharacter ( cx+1.0f, cy, size, (GXColor){0,0,0,128}, *p8 );
//						printCharacter ( cx, cy-1.0f, size, (GXColor){0,0,0,128}, *p8 );
//						printCharacter ( cx, cy+1.0f, size, (GXColor){0,0,0,128}, *p8 );
//						printCharacter ( cx, cy, size, color, *p8 );
//						break;
//					case NsFontEffect_Bold:
//						printCharacter ( cx-1.0f, cy, size, color, *p8 );
//						printCharacter ( cx+1.0f, cy, size, color, *p8 );
//						printCharacter ( cx, cy-1.0f, size, color, *p8 );
//						printCharacter ( cx, cy+1.0f, size, color, *p8 );
//						printCharacter ( cx, cy, size, color, *p8 );
//						break;
//					default:
//						printCharacter ( cx, cy, size, color, *p8 );
//						break;
//				}
//
//				cx += (float)size;
//				break;
//		}
//		p8++;
//	}
//
//	va_end ( marker );
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				setYSpacing														*
// *	Inputs:																		*
// *				spacing	The amount of space between y lines.					*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Set sthe amount of space that is added between character lines.	*
// *				Default is set to 0.0f;
// *																				*
// ********************************************************************************/
//void setYSpacing ( float spacing )
//{
//	ySpacing = spacing;
//}
//
//}		// namespace Render
