/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			GFX														**
**																			**
**	File name:		debuggfx.cpp											**
**																			**
**	Created by:		11/01/00	-	mjd										**
**																			**
**	Description:	Draws debug lines, other debug graphics					**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <string.h>


#include <core/defines.h>
#include <gfx/debuggfx.h>
#include <sys/timer.h>				// Including for debugging
#include <sys/config/config.h>
#include <sys/profiler.h>
#include <gel/scripting/symboltable.h>

#include <gfx/nxviewman.h>			// for camera

#ifdef	__PLAT_NGPS__	
#include <gfx/ngps/nx/line.h>
#include <gfx/ngps/nx/render.h>
namespace Sys
{
	extern void			box(int x0,int y0, int x1, int y1, uint32 color);
}
#endif

#ifdef	__PLAT_XBOX__
#include <gfx/xbox/nx/nx_init.h>
#include <gfx/xbox/nx/render.h>
#endif

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

#ifdef	__PLAT_NGPS__
#define DEBUGGERY_LINE_ARRAY_SIZE		10000			// * 48 bytes = 4.8MB off the debug heap
#else
#define DEBUGGERY_LINE_ARRAY_SIZE		2048
#endif
/*****************************************************************************
**								Private Types								**
*****************************************************************************/

struct SDebuggeryLine{
	Mth::Vector			v0, v1;
	unsigned int	rgb0, rgb1;
	int				in_use;
	int				num_draw_frames;
};

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

#ifndef __PLAT_NGC__
SDebuggeryLine *DebuggeryLineArray = NULL;		//[ DEBUGGERY_LINE_ARRAY_SIZE ];
#endif		// __PLAT_NGC__

#ifdef	__DEBUG_CODE__
static bool	sActive = false;
#endif
/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

void DebuggeryLines_Draw( void );
void DebuggeryLines_CleanUp( void );

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/


void DebuggeryLines_CleanUp( void )
{
#ifdef	__DEBUG_CODE__
#ifndef __PLAT_NGC__
	if (!DebuggeryLineArray)
	{
		// DOn't bother if not allocated yet
		return;
	}
	
	int i;
	
	for ( i = 0; i < DEBUGGERY_LINE_ARRAY_SIZE; i++ )
	{
		DebuggeryLineArray[ i ].in_use = 0;
	}
#endif		// __PLAT_NGC__
#endif
}

#ifndef __PLAT_NGC__
#ifdef	__DEBUG_CODE__
static bool DebuggeryLinesInitialized = 0;
#endif
#endif		// __PLAT_NGC__


void DebuggeryLines_Draw( void )
{
#ifdef	__DEBUG_CODE__


	if (!sActive)
	{
		return;
	}
	
	sActive = false;
#	if defined( __PLAT_NGPS__ )

	Mth::Vector	cam_fwd;
	
	if (Nx::CViewportManager::sGetActiveCamera())
	{
		cam_fwd = Nx::CViewportManager::sGetActiveCamera()->GetMatrix()[Z];
	}
	else
	{
		printf("WARNING: called Gfx::DebuggeryLines_Draw without a camera\n");
		return;
	}

	// can remove this when this module is properly initialized:
	int i;
	uint32	last_color = 0x33333333;	 	// the dodginess of my code astounds me!

	if ( !DebuggeryLinesInitialized )
	{
		DebuggeryLines_CleanUp( );
		DebuggeryLinesInitialized = 1;
	}

	SDebuggeryLine* p_debugline = DebuggeryLineArray;	
	for ( i = DEBUGGERY_LINE_ARRAY_SIZE; i ; i-- )
	{
		if ( p_debugline->in_use )
		{
		
			NxPs2::DMAOverflowOK = 2;
		
			if ( last_color != p_debugline->rgb0)
			{
				if (last_color != 0x33333333)
				{
					NxPs2::ChangeLineColor(0x80000000 + (0x00ffffff & p_debugline->rgb0));
				}			
				else
				{
					NxPs2::BeginLines3D(0x80000000 + (0x00ffffff & p_debugline->rgb0));
				}
				last_color = p_debugline->rgb0;
			}
		
			sActive = true;
			NxPs2::DrawLine3D(p_debugline->v0[X],
					   p_debugline->v0[Y],
					   p_debugline->v0[Z],
					   p_debugline->v1[X],
					   p_debugline->v1[Y],
					   p_debugline->v1[Z]);
			NxPs2::DrawLine3D(p_debugline->v0[X],
					   p_debugline->v0[Y],
					   p_debugline->v0[Z],
					   p_debugline->v1[X],
					   p_debugline->v1[Y],
					   p_debugline->v1[Z]);


			if ( p_debugline->in_use == 2 )
			{
					
				// Calculate and draw an arrowhead 
				// 1/4th the length, at ~30 degrees					   
				Mth::Vector	a = p_debugline->v0;
				Mth::Vector b = p_debugline->v1;
				Mth::Vector	ab = (b-a);
				ab /= 4.0f;
				Mth::Vector out;
				out = ab;
				out.Normalize();
				out = Mth::CrossProduct(out, cam_fwd);
				out *= ab.Length()/3.0f;			
				
				Mth::Vector left =  b - ab + out;
				Mth::Vector right = b - ab - out;
	
				NxPs2::DrawLine3D(left[X],left[Y],left[Z],b[X],b[Y],b[Z]);
				NxPs2::DrawLine3D(right[X],right[Y],right[Z],b[X],b[Y],b[Z]);
				NxPs2::DrawLine3D(right[X],right[Y],right[Z],left[X],left[Y],left[Z]);	 // crossbar
				// have to draw it twice for some stupid reason	(final segment of a particular color is not rendered)
				NxPs2::DrawLine3D(left[X],left[Y],left[Z],b[X],b[Y],b[Z]);
				NxPs2::DrawLine3D(right[X],right[Y],right[Z],b[X],b[Y],b[Z]);
				NxPs2::DrawLine3D(right[X],right[Y],right[Z],left[X],left[Y],left[Z]);
				
			}			   
					   
					   
			if ( p_debugline->num_draw_frames )
			{
				p_debugline->num_draw_frames--;
				if ( !p_debugline->num_draw_frames )
					p_debugline->in_use = false;
			}
		}
		p_debugline++;
	}

	if (last_color != 0x33333333)
	{
		NxPs2::EndLines3D();
	}			
#	elif defined( __PLAT_XBOX__ )
	struct sLineVert
	{
		D3DVECTOR	pos;
		D3DCOLOR	col;
	};

	static sLineVert	line_verts[DEBUGGERY_LINE_ARRAY_SIZE * 2];
	uint32				last_color = 0x33333333;
	uint32				index = 0;
	if( !DebuggeryLinesInitialized )
	{
		DebuggeryLines_CleanUp();
		DebuggeryLinesInitialized = 1;
	}
	
	for( int i = 0; i < DEBUGGERY_LINE_ARRAY_SIZE; ++i )
	{
		Dbg_Assert( index < DEBUGGERY_LINE_ARRAY_SIZE * 2 );
		if( DebuggeryLineArray[i].in_use )
		{
			if( DebuggeryLineArray[ i ].num_draw_frames )
			{
				DebuggeryLineArray[ i ].num_draw_frames--;
				if( !DebuggeryLineArray[i].num_draw_frames )
					DebuggeryLineArray[i].in_use = false;
			}
			sActive = true;
			
			// Need to switch rgba to bgra.
			uint32 rgb0 = ( DebuggeryLineArray[i].rgb0 & 0xFF00FF00UL ) | (( DebuggeryLineArray[i].rgb0 & 0x00FF0000UL ) >> 16 ) |(( DebuggeryLineArray[i].rgb0 & 0x000000FFUL ) << 16 );
			uint32 rgb1 = ( DebuggeryLineArray[i].rgb1 & 0xFF00FF00UL ) | (( DebuggeryLineArray[i].rgb1 & 0x00FF0000UL ) >> 16 ) |(( DebuggeryLineArray[i].rgb1 & 0x000000FFUL ) << 16 );
			
			line_verts[index].col	= rgb0;
			line_verts[index].pos.x	= DebuggeryLineArray[i].v0[X];
			line_verts[index].pos.y	= DebuggeryLineArray[i].v0[Y];
			line_verts[index].pos.z	= DebuggeryLineArray[i].v0[Z];
			++index;
			line_verts[index].col	= rgb1;
			line_verts[index].pos.x	= DebuggeryLineArray[i].v1[X];
			line_verts[index].pos.y	= DebuggeryLineArray[i].v1[Y];
			line_verts[index].pos.z	= DebuggeryLineArray[i].v1[Z];
			++index;
		}
	}
	
	if( index > 0 )
	{
		NxXbox::set_texture( 0, NULL );
		NxXbox::set_blend_mode( NxXbox::vBLEND_MODE_DIFFUSE );
		NxXbox::set_vertex_shader( D3DFVF_XYZ | D3DFVF_DIFFUSE );
		NxXbox::set_pixel_shader( 0 );
		HRESULT hr = NxXbox::EngineGlobals.p_Device->DrawPrimitiveUP( D3DPT_LINELIST, index / 2, line_verts, sizeof( sLineVert ));
	}
#	endif
#endif
}  // end of DebuggeryLines_Draw( )

 
// Draw a bunch of 2D rectangles that we added to the list this frame. 
// currently used for the framerate indicator.


void DebuggeryRects_Draw( float time)
{

#ifdef	__DEBUG_CODE__

	#ifdef	__PLAT_NGPS__
	#ifdef		__USE_PROFILER__
	
						  
		if (Script::GetInteger(CRCD(0xd9859988,"Display_framerate_box")) == 0)
		{
			return;
		}
						  
		if (time < 0.016667f)
		{
			return;
		}
		
		if (Config::CD())
		{
			return;
		}

		int size = (int) (time * 400); 

		
		int x = 70-size;
		int y = 200-size;
		int w = size*2;
		int h = size*2;

		
		Sys::box( x,y,x+w,y+h, 0x0000ff);
		Sys::box( x+w/4,y+h/4,x+w*3/4,y+h*3/4, 0xffffff);
	
	#endif
	#endif
#endif

}


/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

// given a movement vector v0, then move all the debug lines by this amount
// (this allows us to see the lines relative to a moving object)
void AdjustDebugLines( const Mth::Vector &v0)
{
#ifdef	__DEBUG_CODE__

#ifndef __PLAT_NGC__
	if (!DebuggeryLineArray)
	{
		// DOn't bother if not allocated yet
		return;
	}

	for (int i= 0; i< DEBUGGERY_LINE_ARRAY_SIZE; i++ )
	{
		if ( DebuggeryLineArray[ i ].in_use )
		{
			DebuggeryLineArray[ i ].v0 += v0;
			DebuggeryLineArray[ i ].v1 += v0;
		}
	}

#endif		// __PLAT_NGC__
#endif
}


/*	Adds a line to our debug line buffer, to be drawn every frame
	until the buffer wraps around again...
*/						   
void AddDebugLine( const Mth::Vector &v0, const Mth::Vector &v1, int rgb0, int rgb1, int numDrawFrames )
{
#ifdef	__DEBUG_CODE__
#ifndef __PLAT_NGC__
	
    static int DebuggeryLineIndex = 0;
	
	if (!DebuggeryLineArray)
	{
		if (!Config::GotExtraMemory())
		{
			return;
		}
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());
		DebuggeryLineArray  = (SDebuggeryLine *) Mem::Malloc(DEBUGGERY_LINE_ARRAY_SIZE * sizeof (SDebuggeryLine));
		Mem::Manager::sHandle().PopContext();
		DebuggeryLines_CleanUp( );	  //initialize it
		
	}
	
//	Dbg_MsgAssert((v0-v1).Length() < 1000,("Suspiciously long line...."));

	if ( !DebuggeryLinesInitialized )
	{
		DebuggeryLines_CleanUp( );
		DebuggeryLinesInitialized = 1;
	}

	if ( rgb1 == SAME_COLOR_AS_RGB0_PLEASE )
	{
		rgb1 = rgb0;
	}
	
	DebuggeryLineArray[ DebuggeryLineIndex ].v0 = v0;
	
	DebuggeryLineArray[ DebuggeryLineIndex ].v1 = v1;
													 
	DebuggeryLineArray[ DebuggeryLineIndex ].rgb0 = rgb0;
	DebuggeryLineArray[ DebuggeryLineIndex ].rgb1 = rgb1;

	DebuggeryLineArray[ DebuggeryLineIndex ].in_use = 1;
	
	DebuggeryLineArray[ DebuggeryLineIndex ].num_draw_frames = numDrawFrames;
															  
	if ( ++DebuggeryLineIndex >= DEBUGGERY_LINE_ARRAY_SIZE )
	{
		DebuggeryLineIndex = 0;
	}
	
	sActive = true;
	
#endif		// __PLAT_NGC__
#endif
} // end of AddDebugLine( )


void AddDebugArrow( const Mth::Vector &v0, const Mth::Vector &v1, int rgb0, int rgb1, int numDrawFrames )
{
#ifdef	__DEBUG_CODE__
#ifndef __PLAT_NGC__
	
    static int DebuggeryLineIndex = 0;
	
	if (!DebuggeryLineArray)
	{
		if (!Config::GotExtraMemory())
		{
			return;
		}
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());
		DebuggeryLineArray  = (SDebuggeryLine *) Mem::Malloc(DEBUGGERY_LINE_ARRAY_SIZE * sizeof (SDebuggeryLine));
		Mem::Manager::sHandle().PopContext();
		DebuggeryLines_CleanUp( );	  //initialize it
		
	}
	
//	Dbg_MsgAssert((v0-v1).Length() < 1000,("Suspiciously long line...."));

	if ( !DebuggeryLinesInitialized )
	{
		DebuggeryLines_CleanUp( );
		DebuggeryLinesInitialized = 1;
	}

	if ( rgb1 == SAME_COLOR_AS_RGB0_PLEASE )
	{
		rgb1 = rgb0;
	}
	
	DebuggeryLineArray[ DebuggeryLineIndex ].v0 = v0;
	
	DebuggeryLineArray[ DebuggeryLineIndex ].v1 = v1;
													 
	DebuggeryLineArray[ DebuggeryLineIndex ].rgb0 = rgb0;
	DebuggeryLineArray[ DebuggeryLineIndex ].rgb1 = rgb1;

	DebuggeryLineArray[ DebuggeryLineIndex ].in_use = 2;
	
	DebuggeryLineArray[ DebuggeryLineIndex ].num_draw_frames = numDrawFrames;
															  
	if ( ++DebuggeryLineIndex >= DEBUGGERY_LINE_ARRAY_SIZE )
	{
		DebuggeryLineIndex = 0;
	}
	
	sActive = true;
	
#endif		// __PLAT_NGC__
#endif
} // end of AddDebugLine( )


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void AddDebugStar(Mth::Vector v0, float r, int rgb0, int numDrawFrames)
{
#ifdef	__DEBUG_CODE__
	Mth::Vector v1,v2;
	
	v1 = v0; v2 = v0; v1[X]+=r; v2[X]-=r; AddDebugLine(v1,v2,rgb0,rgb0,numDrawFrames);
	v1 = v0; v2 = v0; v1[Y]+=r; v2[Y]-=r; AddDebugLine(v1,v2,rgb0,rgb0,numDrawFrames);
	v1 = v0; v2 = v0; v1[Z]+=r; v2[Z]-=r; AddDebugLine(v1,v2,rgb0,rgb0,numDrawFrames);
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void AddDebugCircle(Mth::Vector v0, int numPoints, float r, int rgb0, int numDrawFrames)
{
#ifdef	__DEBUG_CODE__
	Mth::Vector v1, v2;

	Mth::Vector fwd( 0.0f, 0.0f, r );

	v1 = v0 + fwd;

	for ( int i = 0; i < numPoints; i++ )
	{
		fwd.RotateY( 2.0f * Mth::PI / (float)numPoints );
		v2 = v0 + fwd;

		AddDebugLine(v1,v2,rgb0,rgb0,numDrawFrames);
		
		v1 = v2;
	}
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void AddDebugBone( const Mth::Vector& p1, const Mth::Vector& p2, float red, float green, float blue )
{
#ifdef	__DEBUG_CODE__
	// GJ:  Pretty much the same as AddDebugLine, but with 
	// a more convenient interface for the skeleton code

	Gfx::AddDebugLine( p1, p2,
					   MAKE_RGB( (int)(red * 255), (int)(green * 255), (int)(blue * 255) ),
					   MAKE_RGB( (int)(red * 255), (int)(green * 255), (int)(blue * 255) ),
					   1 );
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void AddDebugBox( const Mth::Matrix& root, const Mth::Vector& pos, SBBox *pBox, Mth::Vector *pOffset, int numFrames, Mth::Vector *pRot, int rgb )
{
#ifdef	__DEBUG_CODE__
	
	if (Config::CD())
	{
		return;
	}
		
	Mth::Vector box[ 8 ];
	Mth::Matrix m;
	int i;

	m = root;
	if ( pRot )
	{
		m.RotateLocal( *pRot );
	}

	box[ 0 ] = box[ 1 ] = box[ 2 ] = box[ 3 ] = pBox->m_max;

	box[ 1 ][ X ] = pBox->m_min[ X ];
	box[ 2 ][ Y ] = pBox->m_min[ Y ];
	box[ 3 ][ Z ] = pBox->m_min[ Z ];
	
	box[ 5 ] = box[ 6 ] = box[ 7 ] = box[ 4 ] = pBox->m_min;
	
	box[ 5 ][ X ] = pBox->m_max[ X ];
	box[ 6 ][ Y ] = pBox->m_max[ Y ];
	box[ 7 ][ Z ] = pBox->m_max[ Z ];
	
	
	for ( i = 0; i < 8; i++ )
	{
//		float tempY = box[ i ][ Y ];
//		box[ i ][ Y ] = box[ i ][ Z ];
//		box[ i ][ Z ] = -tempY;
		if ( pOffset )
		{
			box[ i ] += *pOffset;
		}
		box[ i ] = m.Transform( box[ i ] );
		box[ i ] += pos;
	}

	for ( i = 1; i < 4; i++ )
	{
		Gfx::AddDebugLine( box[ 0 ], box[ i ], rgb, rgb, numFrames );
	}
	for ( i = 5; i < 8; i++ )
	{
		Gfx::AddDebugLine( box[ 4 ], box[ i ], rgb, rgb, numFrames );
	}
	// fill in the cracks:
	Gfx::AddDebugLine( box[ 1 ], box[ 6 ], rgb, rgb, numFrames );
	Gfx::AddDebugLine( box[ 1 ], box[ 7 ], rgb, rgb, numFrames );

	Gfx::AddDebugLine( box[ 2 ], box[ 5 ], rgb, rgb, numFrames );
	Gfx::AddDebugLine( box[ 2 ], box[ 7 ], rgb, rgb, numFrames );

	Gfx::AddDebugLine( box[ 3 ], box[ 5 ], rgb, rgb, numFrames );
	Gfx::AddDebugLine( box[ 3 ], box[ 6 ], rgb, rgb, numFrames );
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void DebugGfx_Draw( void )
{	
#ifdef	__DEBUG_CODE__
	DebuggeryLines_Draw( );
   
//	Mdl::FrontEnd* p_frontend = Mdl::FrontEnd::Instance();
//	if( !p_frontend->GamePaused())
	{
		DebuggeryRects_Draw(Tmr::FrameLength());	 
	}	
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void DebugGfx_CleanUp( void )
{
#ifdef	__DEBUG_CODE__
	
	
	DebuggeryLines_CleanUp( );
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Gfx




