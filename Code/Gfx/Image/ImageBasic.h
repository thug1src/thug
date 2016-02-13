/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		            											**
**																			**
**	Module:			              			 								**
**																			**
**	File name:		                    									**
**																			**
**	Created by:     rjm        				    			                **
**																			**
**	Description:					                                        **
**																			**
*****************************************************************************/

#ifndef __GFX_IMAGE_IMAGEBASIC_H
#define __GFX_IMAGE_IMAGEBASIC_H


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
/*****************************************************************************
**								   Defines									**
*****************************************************************************/


namespace Image
{

enum JustX
{
	vJUST_LEFT 		= 0,
	vJUST_CENTER_X,
	vJUST_RIGHT,
};

enum JustY
{
	vJUST_TOP		= 0,
	vJUST_CENTER_Y,	
	vJUST_BOTTOM,
};


struct RGBA
{
	RGBA() { }
	RGBA(uint8 red, uint green, uint blue, uint alpha) :
		r(red),
		g(green),
		b(blue),
		a(alpha) { }

	void	Blend128(const RGBA & rgba);
	void	Blend255(const RGBA & rgba);

	uint8 r;
	uint8 g;
	uint8 b;
	uint8 a;
};

////////////////////////
// Inlines

inline void RGBA::Blend128(const RGBA & rgba)
{
	int blend_val;

	blend_val = r * rgba.r;
	r = (uint8) (blend_val >> 7);
	blend_val = g * rgba.g;
	g = (uint8) (blend_val >> 7);
	blend_val = b * rgba.b;
	b = (uint8) (blend_val >> 7);
	blend_val = a * rgba.a;
	a = (uint8) (blend_val >> 7);
}

inline void RGBA::Blend255(const RGBA & rgba)
{
	int blend_val;

	blend_val = r * rgba.r;
	r = (uint8) (blend_val / 255);
	blend_val = g * rgba.g;
	g = (uint8) (blend_val / 255);
	blend_val = b * rgba.b;
	b = (uint8) (blend_val / 255);
	blend_val = a * rgba.a;
	a = (uint8) (blend_val >> 7);		// Alpha is always 128 == 1.0
}

}

#endif //__GFX_IMAGE_IMAGEBASIC_H

