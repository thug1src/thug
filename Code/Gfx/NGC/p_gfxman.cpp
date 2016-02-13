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
		
#include <gfx/gfxman.h>

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

//#define SELECT_VIDEO_MODE
// Some defines for gamma settings.
#define gammaDefault	1.0f
#define gammaDomain		256
#define gammaRange		256.0f
#define MIN_GAMMA		0.0f
#define MAX_GAMMA		2.0f



/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

//static D3DGAMMARAMP	gammaTable;
//static float		gammaValueR = gammaDefault;	// Current red gamma value.
//static float		gammaValueG = gammaDefault;	// Current green gamma value.
//static float		gammaValueB = gammaDefault;	// Current blue gamma value.



/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
//static void gammaInitTable( void )
//{
//	// Create the gamma intensity lookup table.
//	// It works like this: given a colour value r [0,255] do r' = gammaTable.red[r] etc.
//	for( int i = 0; i < gammaDomain; ++i )
//	{
//		gammaTable.red[i]	= (BYTE)((float)gammaRange * pow(((float)i / (float)gammaDomain ), 1.0f / gammaValueR ));
//		gammaTable.green[i]	= (BYTE)((float)gammaRange * pow(((float)i / (float)gammaDomain ), 1.0f / gammaValueG ));
//		gammaTable.blue[i]	= (BYTE)((float)gammaRange * pow(((float)i / (float)gammaDomain ), 1.0f / gammaValueB ));
//	}
//}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void gammaSetValue( float gr, float gg, float gb )
{
//	gammaValueR = gr;
//	gammaValueG = gg;
//	gammaValueB = gb;
//
//	// Create the table.
//	gammaInitTable();
//
//	// Pass the table along to the hardware.
//	D3DDevice_SetGammaRamp( D3DSGR_NO_CALIBRATION, &gammaTable );
}



/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
//void Manager::SetGammaNormalized( float fr, float fg, float fb )
//{
//	// Cap it....
//	fr = ( fr < 0.0f ) ? 0.0f : (( fr > 1.0f ) ? 1.0f : fr );
//	fg = ( fg < 0.0f ) ? 0.0f : (( fg > 1.0f ) ? 1.0f : fg );
//	fb = ( fb < 0.0f ) ? 0.0f : (( fb > 1.0f ) ? 1.0f : fb );
//
//	// Scale it....
//	fr *= MAX_GAMMA;
//	fg *= MAX_GAMMA;
//	fb *= MAX_GAMMA;
//
//	// Offset it.
//	fr += 0.5f;
//	fg += 0.5f;
//	fb += 0.5f;
//
//	// f<col> now goes from 0.5 to 2.5.
//	gammaSetValue( fr, fg, fb );
//}




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
//	char fileName[32];
//	char fullFileName[64];
//
////	Skate3 code, disabled for now.
//#	if 0
//	
//	// Try to find a good filename of the format filebasexxx.bmp. "Good" is defined here as one that isn't already used.
//	RwFileFunctions* fileFunctions = RwOsGetFileInterface();
//	for( int i = 0;; ++i )
//	{
//		sprintf( fileName, "screens\\%s%03d.bmp", fileroot, i );
//
//		// Found an unused one! Yay!
//		if( !fileFunctions->rwfexist( fileName ))
//		{
//			sprintf( fullFileName, "d:\\data\\screens\\%s%03d.bmp", fileroot, i );
//			break;
//		}
//	}
//
//	// Obtain the render surface.
//	IDirect3DSurface8* p_render_target;
//	D3DDevice_GetRenderTarget( &p_render_target );
//
//	// Get the surface description, just for s and g.
//	D3DSURFACE_DESC surface_desc;
//	p_render_target->GetDesc( &surface_desc );
//
//	// This is great - this function spits surfaces straight out into a file.
//	HRESULT hr = XGWriteSurfaceToFile( p_render_target, fullFileName );
//	Dbg_Assert( hr == S_OK );
//
//#	endif
}






/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


} // namespace Gfx
