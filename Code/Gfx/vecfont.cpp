#if 0
/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GFX (Graphics Library)									**
**																			**
**	Module:			Graphics Lib (gfx)		 								**
**																			**
**	File name:		vecfont.cpp												**
**																			**
**	Created:		01/31/99	-	mjb										**
**																			**
**	Description:	Vector font class										**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <gfx/vecfont.h>
#include <sys/mem/memman.h>
#include <gfx/image/imagebasic.h>


/*****************************************************************************
**								DBG Information								**
*****************************************************************************/


namespace Gfx
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

uint32				VecFont::s_char_length[256];
VecFont::CharLine*		VecFont::s_char_maps[256];
VecFont::CharLine		VecFont::s_symbol_maps1[15][16] =
{
	 {{64,0,64,16},
      {64,32,64,128}},          /* ! */
     {{32,96,32,128},
      {96,96,96,128}},          /* " */
     {{32,0,32,128},
      {96,0,96,128},
      {0,32,128,32},
      {0,96,128,96}},           /* # */
     {{0,0,128,0},
      {128,0,128,64},
      {128,64,0,64},
      {0,64,0,128},
      {0,128,128,128},
      {64,0,64,128}},           /* $ */
     {{0,0,128,128},
      {0,112,16,128},
      {112,0,128,16}},          /* % */
     {{0,0,0,0}},               /* & - yeah,   right */
     {{64,96,64,128}},          /* ' */
     {{96,128,64,96},
      {64,96,64,32},
      {64,32,96,0}},            /* ( */
     {{32,128,64,96},
      {64,96,64,32},
      {64,32,32,0}},            /* ) */
     {{64,0,64,128},
      {0,64,128,64},
      {32,32,96,96},
      {32,96,96,32}},           /* * */
     {{64,32,64,96},
      {32,64,96,32}},           /* + */
     {{64,0,96,32}},            /* , */
     {{32,64,96,64}},           /* - */
     {{64,0,64,32}},            /* . */     
     {{0,0,128,128}}
};          /* / */

VecFont::CharLine VecFont::s_symbol_maps2[7][16] =
{
	 {{64,0,64,16},
	  {64,128,64,112}},         /* : */
     {{32,0,64,32},
      {64,128,64,112}},         /* ; */
     {{128,128,0,64},
      {0,64,128,0}},            /* < */
     {{0,32,128,32},
      {0,96,128,96}},           /* =
                                 */
     {{0,128,128,64},
      {128,64,0,0}},            /* > */
     {{0,0,0,0}},               /* ? - yeah, right */
     {{0,0,0,0}}                /* @ - yeah, right */
};              

VecFont::CharLine VecFont::s_digit_maps[10][16] =
{
	 {{0,0,128,0},
      {128,0,128,128},
      {128,128,0,128},
      {0,128,0,0}},             /* 0 */
     {{128,0,128,128}},         /* 1 */
     {{0,128,128,128},
      {128,128,128,64},
      {128,64,0,64},
      {0,64,0,0},
      {0,0,128,0}},             /* 2 */
     {{0,128,128,128},
      {128,128,128,0},
      {128,0,0,0},
      {128,64,0,64}},           /* 3 */
     {{0,128,0,64},
      {0,64,128,64},
      {128,128,128,0}},         /* 4 */
     {{128,128,0,128},
      {0,128,0,64},
      {0,64,128,64},
      {128,64,128,0},
      {128,0,0,0}},             /* 5 */
     {{128,128,0,128},
      {0,128,0,0},
      {0,0,128,0},
      {128,0,128,64},
      {128,64,0,64}},           /* 6 */
     {{0,128,128,128},
      {128,128,128,0}},         /* 7 */
     {{0,0,128,0},
      {128,0,128,128},
      {128,128,0,128},
      {0,128,0,0},
      {0,64,128,64}},           /* 8 */
     {{0,0,128,0},
      {128,0,128,128},
      {128,128,0,128},
      {0,128,0,64},      {0,64,128,64}}
}; /* 9 */

VecFont::CharLine VecFont::s_alpha_maps[26][16] =
{
	 {{0,0,64,128},
      {64,128,128,0},
      {32,64,96,64}},           /* a */
     {{0,128,0,0},
      {0,0,128,0},
      {128,0,128,64},
      {128,64,0,64}},           /* b */
     {{128,128,0,128},
      {0,128,0,0},
      {0,0,128,0}},             /* c */
     {{0,0,0,128},
      {0,128,128,64},
      {128,64,128,0},
      {128,0,0,0}},             /* d */
     {{128,128,0,128},
      {0,128,0,0},
      {0,0,128,0},
      {0,64,64,64}},            /* e */
     {{128,128,0,128},
      {0,128,0,0},
      {0,64,64,64}},            /* f */
     {{128,128,0,128},
      {0,128,0,0},
      {0,0,128,0},
      {128,0,128,64}},          /* g */
     {{0,128,0,0},
      {0,64,128,64},
      {128,128,128,0}},         /* h */
     {{64,0,64,128}},           /* i */
     {{0,0,128,0},
      {128,0,128,128}},         /* j */
     {{0,0,0,128},
      {128,0,0,64},
      {0,64,128,128}},          /* k */
     {{0,0,0,128},
      {0,0,128,0}},             /* l */
     {{0,0,0,128},
      {0,128,64,0},
      {64,0,128,128},
      {128,128,128,0}},         /* m */
     {{0,0,0,128},
      {0,128,128,128},
      {128,128,128,0}},         /* n */
     {{0,0,128,0},
      {128,0,128,128},
      {128,128,0,128},
      {0,128,0,0}},             /* o */
     {{0,0,0,128},
      {0,128,128,128},
      {128,128,128,64},
      {128,64,0,64}},           /* p */
     {{128,0,128,128},
      {128,128,0,128},
      {0,128,0,64},
      {0,64,128,64}},           /* q */
     {{0,0,0,128},
      {0,128,128,128},
      {128,128,128,64},
      {128,64,0,64},
      {0,64,128,0}},            /* r */
     {{0,0,128,0},
      {128,0,128,64},
      {128,64,0,64},
      {0,64,0,128},
      {0,128,128,128}},         /* s */
     {{0,128,128,128},
      {64,128,64,0}},           /* t */
     {{0,128,0,0},
      {0,0,128,0},
      {128,0,128,128}},         /* u */
     {{0,128,64,0},
      {64,0,128,128}},          /* v */
     {{0,128,0,0},
      {0,0,64,128},
      {64,128,128,0},
      {128,0,128,128}},         /* w */
     {{0,128,128,0},
      {0,0,128,128}},           /* x */
     {{0,0,128,0},
      {128,0,128,128},
      {0,128,0,64},
      {0,64,128,64}},           /* y */
     {{128,0,0,0},
      {0,0,128,128},      
      {128,128,0,128}}			/* z */
};

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

void VecFont::assign_line_list( uint32 target, CharLine* lineList )
{
    s_char_length[target] = 0;
    s_char_maps[target] = lineList;

    while ( lineList->startX | lineList->startY | lineList->endX | lineList->endY )
    {
        s_char_length[target]++;
        lineList++;
    }
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

VecFont::VecFont( const Image::RGBA color, float w, float h )
{
/*
        
//	RwV2dScale( &m_size, size, (RwReal)(1.0f/192.0f) );
	
	m_color = color;
	m_lineVertBuffer = NULL;
	m_lineVertBufferSize = 0;

    uint32 i;

    for (i = 0; i < 256; i++)
    {
        s_char_length[i] = 0;
    }

    for (i = 0; i < 15; i++)
    {
        assign_line_list('!'+i, s_symbol_maps1[i]);
    }

    for (i = 0; i < 10; i++)
    {
        assign_line_list('0'+i, s_digit_maps[i]);
    }

    for (i = 0; i < 7; i++)
    {
        assign_line_list(':'+i, s_symbol_maps2[i]);
    }

    for (i = 0; i < 26; i++)
    {
        assign_line_list('a'+i, s_alpha_maps[i]);
        assign_line_list('A'+i, s_alpha_maps[i]);
    }
*/
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#define	VECFONTSIZE  50000

void	VecFont::Print( float x, float y, const char* string )
{
	
#if 0

#ifndef __PLAT_NGC__
    uint32 i, numLines;
    RwV2d    localPos = pos;

    /* Make sure that the line vertex buffer is big enough */
    i = numLines = 0;
    while (string[i])
    {
        numLines += s_char_length[(uint32)string[i]];
        i++;
    }



    /* Will it fit? */
    if (numLines > m_lineVertBufferSize)
    {
        uint32		newSize = 2 * sizeof(RWIM2DVERTEX) * numLines;

		if (newSize < VECFONTSIZE)
		{
			numLines = VECFONTSIZE / (2 * sizeof(RWIM2DVERTEX))  + 1;
			newSize = 2 * sizeof(RWIM2DVERTEX) * numLines;
		}
		else
		{
			Dbg_MsgAssert(0,("metric vector font overflow, numLines = %d, m_lineVertBufferSize %d ", numLines, m_lineVertBufferSize));
		}
		
		printf(">>>>>>>>>>>>>>>>>>>>>>>>> newSize = %d\n",newSize);
		
        RWIM2DVERTEX*	newLineVertBuffer;

		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().RwHeap());
        if ( m_lineVertBuffer )
        {
            newLineVertBuffer = (RWIM2DVERTEX *)RwRealloc( m_lineVertBuffer, newSize );
        }
        else
        {
            newLineVertBuffer = (RWIM2DVERTEX *)RwMalloc(newSize);
        }
		Mem::Manager::sHandle().PopContext();

        if (newLineVertBuffer)
        {
            /* Got through and set all uv, recip z, screen z, etc */
            for (i = m_lineVertBufferSize*2; i < numLines*2; i++ )
            {
				RwReal	near_z = RwIm2DGetNearScreenZ();
				RwReal	recip_z = 1.0f/near_z;
                RWIM2DVERTEXSetScreenZ( &newLineVertBuffer[i], near_z );
                RWIM2DVERTEXSetRecipCameraZ( &newLineVertBuffer[i], recip_z );
                RWIM2DVERTEXSetU(&newLineVertBuffer[i], (RwReal)(0.0f), (RwReal)(1.0f));
                RWIM2DVERTEXSetV(&newLineVertBuffer[i], (RwReal)(0.0f), (RwReal)(1.0f));
                RWIM2DVERTEXSetIntRGBA(&newLineVertBuffer[i], m_color.red, m_color.green,
                                      m_color.blue, m_color.alpha);
            }
			m_lineVertBuffer = newLineVertBuffer;
            m_lineVertBufferSize = numLines;
        }
    }

    /* Will it fit now */
    if ( numLines <= m_lineVertBufferSize )
    {
        RWIM2DVERTEX*	curLineVerts = m_lineVertBuffer;

        i = 0;
        while (string[i])
        {
            uint32 	numCharLines = s_char_length[(uint32)string[i]];
            CharLine*	curLine 	 = s_char_maps[(uint32)string[i]];

            while (numCharLines--)
            {
                RwLine line;

                line.start.x = localPos.x + ((RwReal)curLine->startX * m_size.x);
                line.start.y = localPos.y + ((RwReal)(128-curLine->startY) * m_size.y);
                line.end.x = localPos.x + ((RwReal)curLine->endX * m_size.x);
                line.end.y = localPos.y + ((RwReal)(128-curLine->endY) * m_size.y);

                RWIM2DVERTEXSetScreenX(&curLineVerts[0], line.start.x);
                RWIM2DVERTEXSetScreenY(&curLineVerts[0], line.start.y);
                RWIM2DVERTEXSetScreenX(&curLineVerts[1], line.end.x);
                RWIM2DVERTEXSetScreenY(&curLineVerts[1], line.end.y);

                curLineVerts += 2;
                curLine++;
            }

            localPos.x += m_size.x * (RwReal)(192.0f);
            i++;
        }

        /* We don't need to corrupt too much render state here */
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);

        RwIm2DRenderPrimitive(rwPRIMTYPELINELIST, m_lineVertBuffer, numLines*2);
    }
#endif		// __PLAT_NGC__
#endif

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

VecFont::~VecFont( void )
{
	

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Gfx


#endif
