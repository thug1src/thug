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
**	Module:			Graphics (GFX)		 									**
**																			**
**	File name:		p_gfxman.cpp											**
**																			**
**	Created:		07/26/99	-	mjb										**
**																			**
**	Description:	Graphics device manager									**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/
		
#include <xtl.h>
#include <xgraphics.h>
#include <sys/file/filesys.h>
#include <gfx/gfxman.h>
#include <gfx/xbox/nx/nx_init.h>
#include <gfx/xbox/nx/gamma.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/


/*****************************************************************************
**								  Externals									**
*****************************************************************************/

namespace Gfx
{


/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void Manager::SetGammaNormalized( float fr, float fg, float fb )
{
	NxXbox::SetGammaNormalized( fr, fg, fb );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void Manager::GetGammaNormalized( float *fr, float *fg, float *fb )
{
	NxXbox::GetGammaNormalized( fr, fg, fb );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::DumpVRAMUsage( void )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::ScreenShot( const char *fileroot )
{
	// Called twice per frame - once to request the screenshot, and once (post Swap()), to actually perform it.
	if( NxXbox::EngineGlobals.screenshot_name[0] == 0 )
	{
		strcpy( NxXbox::EngineGlobals.screenshot_name, fileroot );
		return;
	}
	
	char fileName[32];
	char fullFileName[64];
	
	// Try to find a good filename of the format filebasexxx.bmp. "Good" is defined here as one that isn't already used.
	for( int i = 0;; ++i )
	{
		sprintf( fileName, "screens\\%s%03d.bmp", fileroot, i );

		// Found an unused one! Yay!
		if( !File::Exist( fileName ))
		{
			sprintf( fullFileName, "d:\\data\\screens\\%s%03d.bmp", fileroot, i );
			break;
		}
	}
	
	// Obtain the render surface.
	IDirect3DSurface8 *p_render_target = NxXbox::EngineGlobals.p_RenderSurface;

	// Get the surface description, just for s and g.
	D3DSURFACE_DESC surface_desc;
	p_render_target->GetDesc( &surface_desc );

	// This is great - this function spits surfaces straight out into a file.
	HRESULT hr = XGWriteSurfaceToFile( p_render_target, fullFileName );
	Dbg_Assert( hr == S_OK );
}






/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


} // namespace Gfx
