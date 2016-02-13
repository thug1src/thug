#if 0

/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GFX (Graphics Library)									**
**																			**
**	Module:			Graphics  (GFX)											**
**																			**
**	File name:		gfx/vecfont.h											**
**																			**
**	Created: 		11/11/99	-	mjb										**
**																			**
*****************************************************************************/

#ifndef __GFX_VECFONT_H
#define __GFX_VECFONT_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <gfx/image/imagebasic.h>


/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Gfx
{



/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class  VecFont  : public Spt::Class
{
	

public:
						VecFont( const Image::RGBA color, float w, float h);
						~VecFont( void );		 

	void				Print( float x, float y, const char* string );

private:


	struct CharLine
	{
		uint8			startX;
		uint8			startY;
		uint8			endX;
		uint8			endY;
	};

	void				assign_line_list( uint32 target, CharLine* lineList );

	float				m_size_x,m_size_y;
	Image::RGBA			m_color;
	
//	RWIM2DVERTEX*		m_lineVertBuffer;
//	RwUInt32			m_lineVertBufferSize;

	static uint32		s_char_length[256];
	static CharLine*	s_char_maps[256];
	static CharLine		s_symbol_maps1[15][16];
	static CharLine		s_symbol_maps2[7][16];
	static CharLine		s_digit_maps[10][16];
	static CharLine		s_alpha_maps[26][16];
};


/*****************************s************************************************
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

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace Gfx

#endif // __GFX_VECFONT_H

#endif

