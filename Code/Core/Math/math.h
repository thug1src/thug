/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Core Library											**
**																			**
**	Module:			Math (MTH)			   									**
**																			**
**	File name:		core/math/math.h										**
**																			**
**	Created:		11/23/99	-	mjb										**
**																			**
**	Description:	Math Library											**
**																			**
*****************************************************************************/

#ifndef __CORE_MATH_MATH_H
#define __CORE_MATH_MATH_H

/*****************************************************************************
**								   Includes									**
*****************************************************************************/

#ifdef __PLAT_XBOX__
#include <math.h>		// Required for fabsf().
#include <core\math\xbox\sse.h>
#endif

#ifdef __PLAT_WN32__
#include <math.h>		// Required for fabsf().
#endif

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#ifdef __USER_MATT__
#define DEBUGGING_REPLAY_RND 0
#else
#define DEBUGGING_REPLAY_RND 0
#endif

#if DEBUGGING_REPLAY_RND

#define Rnd( x ) Rnd_DbgVersion( ( x ), __LINE__, __FILE__ )

#else
#define Rnd( x ) Rnd( x )
#endif

namespace Mth
{
						   


/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

/*****************************************************************************
**							   Type Definitions								**
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

void InitialRand(int a);

#if DEBUGGING_REPLAY_RND

int Rnd_DbgVersion(int n, int line, char *file);
void SetRndTestMode( int mode );
bool RndFuckedUp( void );

#else

int Rnd(int n);

#endif

// use for non-deterministic things
// especially for CD-timing reliant stuff that would throw off our random number dealy whopper.
int Rnd2(int n);

float PlusOrMinus(float n);
void InitSinLookupTable();
float Kensinf(float x);
float Kencosf(float x);
float Kenacosf(float x);
float Atan2 (float y, float x);			// replaces atan2, much faster



/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

inline	int Abs ( int x )
{
	return ( x>=0?x:-x);
}


inline	float	Abs ( float x )
{
	return float ( fabsf ( x ));
}

// round down to nearest whole number
inline	float	Whole ( float x )
{
	return (float) ( (int) ( x ));
}

// round to nearest whole number
inline	float	Round ( float x )
{
		return (float) ( (int) ( x + 0.499999f));
}


//////////////////////////////////////////////////////////////////////////////////////////
// Calculate the "Parabolic Lerp" value for a set of parameters
// (Note:  made-up name, by Mick)
//
// A lerp (Linear intERPalotion) value is always in the range 0.0 to 1.0
// and is used to move one value (A) towards another (B)
// usually the lerp value is fixed.
//
// A new value C is calculated as C = A + lerp * (B - A)
//
// a "ParaLerp" is a lerp value that varies based on the distance apart of the two values
//
// The purpose of this function	is to be able to smoothly change a value
// that really only needs lerping when the values are within a small range
// The result of this equation is a parabolic curve that quickly tends 
// towards lerp = 1.0, as x increases
// The valeu "Rate" is the value of x for which lerp half way between Min and 1.0
// Min is the value of lerp when x is 0 
inline float ParaLerp(float x, float Rate = 1.0f, float Minimum = 0.0f)
{
	float lerp = Minimum + (1 - 1 / (1 + x / Rate) ) * (1 - Minimum);
	return lerp; 

}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	ClampMin ( float v, float min )
{
	if ( v < min )
	{
		return min;
	}
	else
	{
		return v;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	ClampMax ( float v, float max )
{
	if ( v > max )
	{
		return max;
	}
	else
	{
		return v;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	Clamp ( float v, float min, float max )
{
	return ClampMin ( ClampMax ( v, max ), min );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	Lerp ( float a, float b, float value )
{
	return a + ( b - a ) * value;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	LinearMap ( float a, float b, float value, float value_min, float value_max )
{
	return Lerp(a, b, (value - value_min) / (value_max - value_min));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	SmoothStep ( float value )
{
	// interpolates from zero to one with a zero derivative at the end-points
	return -2.0f * value * value * ( value - (3.0f / 2.0f) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	SmoothInterp ( float a, float b, float value )
{
	return Lerp(a, b, SmoothStep(value));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	SmoothMap ( float a, float b, float value, float value_min, float value_max )
{
	return Lerp(a, b, SmoothStep((value - value_min) / (value_max - value_min)));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	Max ( float a, float b )
{
	return ( a > b ) ? a : b;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	Min ( float a, float b )
{
	return ( a < b ) ? a : b;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	Sgn ( float x )
{
	if ( x < 0.f )
	{
		return -1.0f;
	}
	else
	{
		return 1.0f;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	Sqr ( float x )
{
	return ( x * x );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void	Swap ( float& a, float& b )
{
	float	t = a;
	a = b; 
	b = t;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	bool	Equal ( const float a, const float b, const float perc )
{
	return ( Abs ( a - b ) <= (( Abs ( a ) + Abs ( b )) * perc ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	sint	Max ( sint a, sint b )
{
	return ( a > b ) ? a : b;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	sint	Min ( sint a, sint b )
{
	return ( a < b ) ? a : b;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	Max3 ( float a, float b, float c )
{
	return Max(a, Max(b,c));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	Min3 ( float a, float b, float c )
{
	return Min(a, Min(b,c));
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	Determinant	( float a, float b, float c, float d )
{
	return ( a * d ) - ( b * c );

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	Determinant3  ( float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3 )
{
	return 	a1 * (b2 * c3 - b3 * c2) - 
			b1 * (a2 * c3 - a3 * c2) + 
			c1 * (a2 * b3 - a3 * b2);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	int		WeightedRnd  ( float min_val, float max_val )
{
	// This random function uses the West distribution
	// to return values that favor the midpoint.

	// For example, WeightedRnd( 10, 20 ) returns:
	// Bucket[10] = 2.12 percent
	// Bucket[11] = 5.72 percent
	// Bucket[12] = 9.44 percent
	// Bucket[13] = 13.87 percent
	// Bucket[14] = 18.16 percent
	// Bucket[15] = 15.70 percent
	// Bucket[16] = 14.52 percent
	// Bucket[17] = 10.16 percent
	// Bucket[18] = 6.05 percent
	// Bucket[19] = 2.26 percent
	// Bucket[20] = 2.00 percent
	
	float range = ( max_val - min_val ) / 2;
	float midpoint = min_val + range;

	float random_val = range - (float)sqrtf((float)Rnd( (int)(range * range) ) );

	// negate it half of the time
	if ( Rnd(2) )
	{
		random_val = -random_val;
	}

	return (int)( random_val + midpoint );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


// Trig

const	float	EPSILON 	= 0.000001f;
const	float	PI			= 3.141592654f;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	DegToRad ( float degrees )
{
	return degrees * ( PI / 180.0f);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	RadToDeg ( float radians )
{
	return radians * ( 180.0f / PI );
}

							
int		RunFilter( int target, int current, int delta );
float	FRunFilter( float target, float current, float delta );

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mth

#endif // __CORE_MATH_MATH_H
