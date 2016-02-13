/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			GFX														**
**																			**
**	File name:		debuggfx.h												**
**																			**
**	Created: 		11/01/00	-	mjd										**
**																			**
*****************************************************************************/

#ifndef	__DEBUG_GFX_H
#define	__DEBUG_GFX_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/


#include <core/math.h>
#include <core/support.h>
#include <core/singleton.h>
#ifndef __GEL_OBJECT_H
#include <gel/object.h>
#endif

/*****************************************************************************
**								   Defines									**
*****************************************************************************/


namespace Gfx
{

// use this macro to create the last two params to AddDebugLine( )
#define MAKE_RGBA( r, g, b, a )	( ( ( a ) << 24 ) | ( ( b ) << 16 ) | ( ( g ) << 8 ) | ( r ) )
#define MAKE_RGB( r, g, b )		( ( ( 255 ) << 24 ) | ( ( b ) << 16 ) | ( ( g ) << 8 ) | ( r ) )
#define GET_R( rgba ) 	( ( ( rgba ) ) & 255 )
#define GET_G( rgba ) 	( ( ( rgba ) >> 8 ) & 255 )
#define GET_B( rgba ) 	( ( ( rgba ) >> 16 ) & 255 )
#define GET_A( rgba ) 	( ( ( rgba ) >> 24 ) & 255 )
										
/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

// if i put them here, would they be private any more?

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

#define SAME_COLOR_AS_RGB0_PLEASE ( 1 << 31 )

/*	Adds a line to our debug line buffer, to be drawn every frame
	until the buffer wraps around again.
*/						   
void AddDebugStar(Mth::Vector v0, float r = 12.0f, int rgb0 = MAKE_RGB( 127, 0, 127 ), int numDrawFrames = 0);

void AddDebugCircle(Mth::Vector v0, int numPoints, float r, int rgb0, int numDrawFrames);

void AdjustDebugLines( const Mth::Vector &v0);
void AddDebugLine( const Mth::Vector &v0, const Mth::Vector &v1, int rgb0 = MAKE_RGB( 127, 127, 127 ), int rgb1 = SAME_COLOR_AS_RGB0_PLEASE, int numDrawFrames = 0 );
void AddDebugArrow( const Mth::Vector &v0, const Mth::Vector &v1, int rgb0 = MAKE_RGB( 127, 127, 127 ), int rgb1 = SAME_COLOR_AS_RGB0_PLEASE, int numDrawFrames = 0 );

// For drawing skeleton
void AddDebugBone( const Mth::Vector& p1, const Mth::Vector& p2, float red = 1.0f, float green = 0.5f, float blue = 0.5f );

// For drawing bounding box
void AddDebugBox( const Mth::Matrix& root, const Mth::Vector& pos, SBBox *pBox, Mth::Vector *pOffset, int numFrames, Mth::Vector *pRot, int rgb = MAKE_RGB( 200, 0, 0 ) );

// Call every frame from the renderer.
void DebugGfx_Draw( void );

// Cleanup code for this module:
void DebugGfx_CleanUp( void );

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace Gfx

#endif	// __DEBUG_GFX_H

