//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       GfxUtils.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  02/01/2001
//****************************************************************************

#ifndef __GFX_UTILS_H
#define __GFX_UTILS_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

#include <core/math/math.h>
#include <core/string/cstring.h>

#include <gfx/bonedanimtypes.h>
	
/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Mth
{
	class Vector;
};

namespace Script
{
	class CStruct;
	class CScript;
};

namespace Gfx
{

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

void RGBtoHSV( float r, float g, float b, float *h, float *s, float *v );
void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v );

void inlineRGBtoHSV( float r, float g, float b, float *h, float *s, float *v );
void inlineHSVtoRGB( float *r, float *g, float *b, float h, float s, float v );

// some time conversion functions
float FRAMES_TO_TIME(int frames);
int TIME_TO_FRAMES(float time);

void 		GetModelFromFileName( char* filename, char* pModelNameBuf );
bool 		GetScaleFromParams( Mth::Vector* pScaleVector, Script::CStruct* pParams );
bool 		GetLoopingTypeFromParams( Gfx::EAnimLoopingType* pLoopingType, Script::CStruct* pParams );
bool 		GetTimeFromParams( float* pStart, float* pEnd, float Current, float Duration, Script::CStruct* pParams, Script::CScript* pScript );
Str::String	GetModelFileName(const char* pName, const char* pExt);
void    	GetModelFileName(const char* pName, const char* pExt, char* pTargetBuffer);

/*****************************************************************************
**						Inline Functions									**
*****************************************************************************/
					
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void inlineRGBtoHSV( float r, float g, float b, float &h, float &s, float &v )
{
	float min, max, delta;
	min = Mth::Min3( r, g, b );
	max = Mth::Max3( r, g, b );
	v = max;				// v
	delta = max - min;
	if( max != 0.0f )
		s = delta / max;		// s
	else {
		// r = g = b = 0		// s = 0, v is undefined
		s = 0.0f;
		h = -1.0f;
		return;
	}

	// GJ:
	if (delta == 0.0f)
		return;

	if( r == max )
		h = ( g - b ) / delta;		// between yellow & magenta
	else if( g == max )
		h = 2.0f + ( b - r ) / delta;	// between cyan & yellow
	else
		h = 4.0f + ( r - g ) / delta;	// between magenta & cyan
	h *= 60.0f;				// degrees
	if( h < 0.0f )
		h += 360.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void inlineHSVtoRGB( float &r, float &g, float &b, float h, float s, float v )
{
	if( s == 0.0f ) {
		// achromatic (grey)
		r = g = b = v;
		return;
	}

	int i;
	float f, p, q, t;

	h *= (1.0f/60.0f);	// sector 0 to 5
	i = (int)h;			// basically, the floor
	f = h - i;			// factorial part of h
	p = v * ( 1.0f - s );
	q = v * ( 1.0f - s * f );
	t = v * ( 1.0f - s * ( 1.0f - f ) );
	switch( i ) {
		case 0:
			r = v;
			g = t;
			b = p;
			break;
		case 1:
			r = q;
			g = v;
			b = p;
			break;
		case 2:
			r = p;
			g = v;
			b = t;
			break;
		case 3:
			r = p;
			g = q;
			b = v;
			break;
		case 4:
			r = t;
			g = p;
			b = v;
			break;
		default:		// case 5:
			r = v;
			g = p;
			b = q;
			break;
	}
}

} // namespace Gfx

#endif // __GFX_UTILS_H