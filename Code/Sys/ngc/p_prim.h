#ifndef _PRIM_H_
#define _PRIM_H_

#include <dolphin.h>

namespace NsPrim
{
	void begin ( void );

	void point ( float x, float y, float z, GXColor color );
	void point ( float x, float y, GXColor color );

	void line ( float x0, float y0, float x1, float y1, GXColor color );
	void line ( float x0, float y0, float x1, float y1, GXColor color0, GXColor color1 );
	void line ( float x0, float y0, float z0, float x1, float y1, float z1, GXColor color );
	void line ( float x0, float y0, float z0, float x1, float y1, float z1, GXColor color0, GXColor color1 );

	void tri ( float x0, float y0, float x1, float y1, float x2, float y2, GXColor color );
	void tri ( float x0, float y0, float x1, float y1, float x2, float y2, GXColor color0, GXColor color1, GXColor color2 );
	void tri ( float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, GXColor color );
	void tri ( float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, GXColor color0, GXColor color1, GXColor color2 );

	void quad ( float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, GXColor color );
	void quad ( float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, GXColor color0, GXColor color1, GXColor color2, GXColor color3 );
	void quad ( float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, GXColor color );
	void quad ( float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, GXColor color0, GXColor color1, GXColor color2, GXColor color3 );

	void frame ( float left, float top, float right, float bottom, GXColor color );
	void box ( float left, float top, float right, float bottom, GXColor color );

//	void sprite ( const char * pName, float x, float y );
//	void sprite ( const char * pName, float x0, float y0, float x1, float y1 );
//	void sprite ( const char * pName, float x0, float y0, float z0, float x1, float y1, float z1 );

//	void sprite ( NsTexture * pTexture, float x, float y );
//	void sprite ( NsTexture * pTexture, float x, float y, float z );
//	void sprite ( NsTexture * pTexture, float x0, float y0, float x1, float y1 );
//	void sprite ( NsTexture * pTexture, float x0, float y0, float z0, float x1, float y1, float z1 );
//
//	void subsprite ( NsTexture * pTexture, float x, float y, short sx, short sy, short sw, short sh );
//	void subsprite ( NsTexture * pTexture, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1 );
//	void subsprite ( NsTexture * pTexture, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, float u0, float v0, float u1, float v1, float u2, float v2, float u3, float v3, float z, GXColor color );

	void end ( void );

	int	 begun ( void );
};

#endif		// _PRIM_H_
