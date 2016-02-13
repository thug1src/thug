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
**	File name:		gamma.cpp												**
**																			**
**	Created:		02/27/02	-	dc										**
**																			**
**	Description:	Gamma setting code										**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/
		
#include <xtl.h>
#include "gamma.h"

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/


/*****************************************************************************
**								  Externals									**
*****************************************************************************/

namespace NxXbox
{


/*****************************************************************************
**								   Defines									**
*****************************************************************************/

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

static D3DGAMMARAMP	gammaTable;
static float		gammaValueR = gammaDefault;	// Current red gamma value.
static float		gammaValueG = gammaDefault;	// Current green gamma value.
static float		gammaValueB = gammaDefault;	// Current blue gamma value.



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
static void gammaInitTable( void )
{
	// Create the gamma intensity lookup table.
	// It works like this: given a colour value r [0,255] do r' = gammaTable.red[r] etc.
	for( int i = 0; i < gammaDomain; ++i )
	{
		gammaTable.red[i]	= (BYTE)((float)gammaRange * pow(((float)i / (float)gammaDomain ), 1.0f / gammaValueR ));
		gammaTable.green[i]	= (BYTE)((float)gammaRange * pow(((float)i / (float)gammaDomain ), 1.0f / gammaValueG ));
		gammaTable.blue[i]	= (BYTE)((float)gammaRange * pow(((float)i / (float)gammaDomain ), 1.0f / gammaValueB ));
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void gammaSetValue( float gr, float gg, float gb )
{
	gammaValueR = gr;
	gammaValueG = gg;
	gammaValueB = gb;

	// Create the table.
	gammaInitTable();

	// Pass the table along to the hardware.
	D3DDevice_SetGammaRamp( D3DSGR_NO_CALIBRATION, &gammaTable );
}



/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SetGammaNormalized( float fr, float fg, float fb )
{
	// Cap it....
	fr = ( fr < 0.0f ) ? 0.0f : (( fr > 1.0f ) ? 1.0f : fr );
	fg = ( fg < 0.0f ) ? 0.0f : (( fg > 1.0f ) ? 1.0f : fg );
	fb = ( fb < 0.0f ) ? 0.0f : (( fb > 1.0f ) ? 1.0f : fb );

	// Scale it....
	fr *= MAX_GAMMA;
	fg *= MAX_GAMMA;
	fb *= MAX_GAMMA;

	// Offset it.
	fr += 0.5f;
	fg += 0.5f;
	fb += 0.5f;

	// f<col> now goes from 0.5 to 2.5.
	gammaSetValue( fr, fg, fb );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void GetGammaNormalized( float *fr, float *fg, float *fb )
{
	// Peform the opposite operation to that done in SetGammaNormalized().
	*fr = ( gammaValueR - 0.5f ) / MAX_GAMMA;
	*fg = ( gammaValueG - 0.5f ) / MAX_GAMMA;
	*fb = ( gammaValueB - 0.5f ) / MAX_GAMMA;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


} // namespace Gfx
