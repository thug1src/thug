/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:																**
**																			**
**	Module:						 		 									**
**																			**
**	File name:		gfx/ngc/p_nx.cpp										**
**																			**
**	Created:		01/16/2002	-	dc										**
**																			**
**	Description:	ngc platform specific interface to the engine			**
**					This is ngc SPECIFIC!!!!!! If there is anything in		**
**					here that is not ngc specific, then it needs to be		**
**					in nx.cpp												**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sys/profiler.h>
#include	<sys/file/filesys.h>

#include <gfx/nxfontman.h>
#include <gfx/ngc/p_nxfont.h>

#include	"gfx\camera.h"
#include	"gfx\nx.h"
#include	"gfx\nxquickanim.h"
#include	"gfx\nxviewman.h"
#include	"gfx\ngc\p_NxGeom.h"
#include	"gfx\NGc\p_NxMesh.h"
#include	"gfx\NxParticleMgr.h"
#include	"gfx\NxWeather.h"
#include	"gfx\ngc\p_nxWeather.h"
#include	"gfx\NxMiscFX.h"
#include	"gfx\NGc\p_NxModel.h"
#include	"gfx\ngc\p_NxSector.h"
#include	"gfx\ngc\p_NxScene.h"
#include	"gfx\ngc\p_NxSprite.h"
#include	"gfx\ngc\p_NxTexture.h"
#include	"gfx\ngc\p_NxTextured3dPoly.h"
#include	"gfx\ngc\p_NxNewParticleMgr.h"
#include	"core\math.h"
#include 	"sk\engine\SuperSector.h"					
#include 	"gel\scripting\script.h"
#include	"gfx\debuggfx.h"

#include 	"gfx\ngc\nx\nx_init.h"
#include 	"gfx\ngc\nx\import.h"
#include 	"gfx\ngc\nx\material.h"
#include 	"gfx\ngc\nx\mesh.h"
#include 	"gfx\ngc\nx\render.h"
#include 	"gfx\ngc\nx\occlude.h"
#include 	"gfx\ngc\nx\scene.h"
#include 	"gfx\ngc\nx\chars.h"
#include	"gfx\ngc\nx\nx_init.h"

#include	<sys/ngc\p_display.h>
#include	<sys/ngc\p_prim.h>
#include	<sys/ngc\p_render.h>
#include	<gel/movies/ngc\p_movies.h>
#include	<sys/ngc\p_dvd.h>
#include	<sys/ngc\p_buffer.h>

#include	"sys/ngc/p_gx.h"

extern bool gCorrectColor;

static bool s_correctable = true;

int gOffy = 0;

int g_object = 0;
int g_view_object = 0;
NxNgc::sScene * g_view_scene = NULL;
int g_material = -1;
int g_scroll_material = 0;

int g_mip = 0;
int g_passes = -1;

extern unsigned char uv_col_map[][2];

extern u8 g_blur;
u8 sBlur = 0;
extern bool		gLoadingScreenActive;

extern PADStatus padData[PAD_MAX_CONTROLLERS]; // game pad state

extern int g_dl;

extern bool g_in_cutscene;

#define COLOR_MAP_SIZE 64
	
//u16 colorMap[(COLOR_MAP_SIZE*COLOR_MAP_SIZE)*2] __attribute__ (( aligned( 32 )));
//bool color_map_created = false;

//extern NxNgc::sMaterialHeader *	p_u_mat[2048];
//extern int					u_mat_count[2048];
//extern int					num_u_mat;
//bool gPrintMatStats = false;

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

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

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#if 0
static u8 sample_pattern[12][2] =
{
//	{1,1},{3,3},{5,5},
//	{11,1},{9,3},{7,5},
//	{5,7},{3,9},{1,11},
//	{7,7},{9,9},{11,11}
	{3,2},{9,2},{3,10},
	{3,2},{9,2},{9,10},
	{3,2},{3,10},{9,10},
	{9,2},{3,10},{9,10},
};
#endif

extern int meshes_considered;

#ifndef __NOPT_FINAL__
bool gDumpHeap = false;
#endif		// __NOPT_FINAL__

#define SHADOW_TEXTURE_SIZE 256
//#define BLUR_TEXTURE_SIZE 64
//#define BLUR_BORDER 2
//
//#define BLUR_1 ( ( (float)BLUR_TEXTURE_SIZE - (float)BLUR_BORDER ) / (float)BLUR_TEXTURE_SIZE )
//#define BLUR_0 ( (float)BLUR_BORDER / (float)BLUR_TEXTURE_SIZE )

uint8 * shadowTextureData = NULL;
//uint8 * volumeTextureData = NULL;
//uint8 * zTextureDataH = NULL;
//uint8 * zTextureDataL = NULL;
//uint8 * screenTextureData = NULL;
//uint8 * focusTextureData = NULL;
//uint8 * blurTextureData = NULL;

//int gFocus = 1;
//
// Set global palette for shadow & blur.

uint16 shadowPalette[16] __attribute__ (( aligned( 32 ))) = {
	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0x00 ),		// 0
	GXPackedRGB5A3( 0x11, 0x11, 0x11, 0xff ),       // 1
	GXPackedRGB5A3( 0x22, 0x22, 0x22, 0xff ),       // 2
	GXPackedRGB5A3( 0x33, 0x33, 0x33, 0xff ),       // 3
	GXPackedRGB5A3( 0x44, 0x44, 0x44, 0xff ),       // 4
	GXPackedRGB5A3( 0x55, 0x55, 0x55, 0xff ),       // 5
	GXPackedRGB5A3( 0x66, 0x66, 0x66, 0xff ),       // 6
	GXPackedRGB5A3( 0x77, 0x77, 0x77, 0xff ),       // 7
	GXPackedRGB5A3( 0x88, 0x88, 0x88, 0xff ),       // 8
	GXPackedRGB5A3( 0x99, 0x99, 0x99, 0xff ),       // 9
	GXPackedRGB5A3( 0xaa, 0xaa, 0xaa, 0xff ),       // 10
	GXPackedRGB5A3( 0xbb, 0xbb, 0xbb, 0xff ),       // 11
	GXPackedRGB5A3( 0xcc, 0xcc, 0xcc, 0xff ),       // 12
	GXPackedRGB5A3( 0xdd, 0xdd, 0xdd, 0xff ),       // 13
	GXPackedRGB5A3( 0xee, 0xee, 0xee, 0xff ),       // 14
	GXPackedRGB5A3( 0xff, 0xff, 0xff, 0xff ),       // 15
};

//uint16 shadowPalette[16] __attribute__ (( aligned( 32 ))) = {
//	GXPackedRGB5A3( 0xff, 0xff, 0xff, 0xff ),       // 15
//	GXPackedRGB5A3( 0xee, 0xee, 0xee, 0xff ),       // 14
//	GXPackedRGB5A3( 0xdd, 0xdd, 0xdd, 0xff ),       // 13
//	GXPackedRGB5A3( 0xcc, 0xcc, 0xcc, 0xff ),       // 12
//	GXPackedRGB5A3( 0xbb, 0xbb, 0xbb, 0xff ),       // 11
//	GXPackedRGB5A3( 0xaa, 0xaa, 0xaa, 0xff ),       // 10
//	GXPackedRGB5A3( 0x99, 0x99, 0x99, 0xff ),       // 9
//	GXPackedRGB5A3( 0x88, 0x88, 0x88, 0xff ),       // 8
//	GXPackedRGB5A3( 0x77, 0x77, 0x77, 0xff ),       // 7
//	GXPackedRGB5A3( 0x66, 0x66, 0x66, 0xff ),       // 6
//	GXPackedRGB5A3( 0x55, 0x55, 0x55, 0xff ),       // 5
//	GXPackedRGB5A3( 0x44, 0x44, 0x44, 0xff ),       // 4
//	GXPackedRGB5A3( 0x33, 0x33, 0x33, 0xff ),       // 3
//	GXPackedRGB5A3( 0x22, 0x22, 0x22, 0xff ),       // 2
//	GXPackedRGB5A3( 0x11, 0x11, 0x11, 0xff ),       // 1
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0x00 ),		// 0
//};

//uint16 zPalette[256] __attribute__ (( aligned( 32 ))) = {
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),		// 0x00
//	GXPackedRGB5A3( 0x01, 0x01, 0x01, 0xff ),       // 0x01
//	GXPackedRGB5A3( 0x02, 0x02, 0x02, 0xff ),       // 0x02
//	GXPackedRGB5A3( 0x03, 0x03, 0x03, 0xff ),       // 0x03
//	GXPackedRGB5A3( 0x04, 0x04, 0x04, 0xff ),       // 0x04
//	GXPackedRGB5A3( 0x05, 0x05, 0x05, 0xff ),       // 0x05
//	GXPackedRGB5A3( 0x06, 0x06, 0x06, 0xff ),       // 0x06
//	GXPackedRGB5A3( 0x07, 0x07, 0x07, 0xff ),       // 0x07
//	GXPackedRGB5A3( 0x08, 0x08, 0x08, 0xff ),       // 0x08
//	GXPackedRGB5A3( 0x09, 0x09, 0x09, 0xff ),       // 0x09
//	GXPackedRGB5A3( 0x0a, 0x0a, 0x0a, 0xff ),       // 0x0a
//	GXPackedRGB5A3( 0x0b, 0x0b, 0x0b, 0xff ),       // 0x0b
//	GXPackedRGB5A3( 0x0c, 0x0c, 0x0c, 0xff ),       // 0x0c
//	GXPackedRGB5A3( 0x0d, 0x0d, 0x0d, 0xff ),       // 0x0d
//	GXPackedRGB5A3( 0x0e, 0x0e, 0x0e, 0xff ),       // 0x0e
//	GXPackedRGB5A3( 0x0f, 0x0f, 0x0f, 0xff ),       // 0x0f
//	GXPackedRGB5A3( 0x10, 0x10, 0x10, 0xff ),		// 0x10
//	GXPackedRGB5A3( 0x11, 0x11, 0x11, 0xff ),       // 0x11
//	GXPackedRGB5A3( 0x12, 0x12, 0x12, 0xff ),       // 0x12
//	GXPackedRGB5A3( 0x13, 0x13, 0x13, 0xff ),       // 0x13
//	GXPackedRGB5A3( 0x14, 0x14, 0x14, 0xff ),       // 0x14
//	GXPackedRGB5A3( 0x15, 0x15, 0x15, 0xff ),       // 0x15
//	GXPackedRGB5A3( 0x16, 0x16, 0x16, 0xff ),       // 0x16
//	GXPackedRGB5A3( 0x17, 0x17, 0x17, 0xff ),       // 0x17
//	GXPackedRGB5A3( 0x18, 0x18, 0x18, 0xff ),       // 0x18
//	GXPackedRGB5A3( 0x19, 0x19, 0x19, 0xff ),       // 0x19
//	GXPackedRGB5A3( 0x1a, 0x1a, 0x1a, 0xff ),       // 0x1a
//	GXPackedRGB5A3( 0x1b, 0x1b, 0x1b, 0xff ),       // 0x1b
//	GXPackedRGB5A3( 0x1c, 0x1c, 0x1c, 0xff ),       // 0x1c
//	GXPackedRGB5A3( 0x1d, 0x1d, 0x1d, 0xff ),       // 0x1d
//	GXPackedRGB5A3( 0x1e, 0x1e, 0x1e, 0xff ),       // 0x1e
//	GXPackedRGB5A3( 0x1f, 0x1f, 0x1f, 0xff ),       // 0x1f
//	GXPackedRGB5A3( 0x20, 0x20, 0x20, 0xff ),		// 0x20
//	GXPackedRGB5A3( 0x21, 0x21, 0x21, 0xff ),       // 0x21
//	GXPackedRGB5A3( 0x22, 0x22, 0x22, 0xff ),       // 0x22
//	GXPackedRGB5A3( 0x23, 0x23, 0x23, 0xff ),       // 0x23
//	GXPackedRGB5A3( 0x24, 0x24, 0x24, 0xff ),       // 0x24
//	GXPackedRGB5A3( 0x25, 0x25, 0x25, 0xff ),       // 0x25
//	GXPackedRGB5A3( 0x26, 0x26, 0x26, 0xff ),       // 0x26
//	GXPackedRGB5A3( 0x27, 0x27, 0x27, 0xff ),       // 0x27
//	GXPackedRGB5A3( 0x28, 0x28, 0x28, 0xff ),       // 0x28
//	GXPackedRGB5A3( 0x29, 0x29, 0x29, 0xff ),       // 0x29
//	GXPackedRGB5A3( 0x2a, 0x2a, 0x2a, 0xff ),       // 0x2a
//	GXPackedRGB5A3( 0x2b, 0x2b, 0x2b, 0xff ),       // 0x2b
//	GXPackedRGB5A3( 0x2c, 0x2c, 0x2c, 0xff ),       // 0x2c
//	GXPackedRGB5A3( 0x2d, 0x2d, 0x2d, 0xff ),       // 0x2d
//	GXPackedRGB5A3( 0x2e, 0x2e, 0x2e, 0xff ),       // 0x2e
//	GXPackedRGB5A3( 0x2f, 0x2f, 0x2f, 0xff ),       // 0x2f
//	GXPackedRGB5A3( 0x30, 0x30, 0x30, 0xff ),		// 0x30
//	GXPackedRGB5A3( 0x31, 0x31, 0x31, 0xff ),       // 0x31
//	GXPackedRGB5A3( 0x32, 0x32, 0x32, 0xff ),       // 0x32
//	GXPackedRGB5A3( 0x33, 0x33, 0x33, 0xff ),       // 0x33
//	GXPackedRGB5A3( 0x34, 0x34, 0x34, 0xff ),       // 0x34
//	GXPackedRGB5A3( 0x35, 0x35, 0x35, 0xff ),       // 0x35
//	GXPackedRGB5A3( 0x36, 0x36, 0x36, 0xff ),       // 0x36
//	GXPackedRGB5A3( 0x37, 0x37, 0x37, 0xff ),       // 0x37
//	GXPackedRGB5A3( 0x38, 0x38, 0x38, 0xff ),       // 0x38
//	GXPackedRGB5A3( 0x39, 0x39, 0x39, 0xff ),       // 0x39
//	GXPackedRGB5A3( 0x3a, 0x3a, 0x3a, 0xff ),       // 0x3a
//	GXPackedRGB5A3( 0x3b, 0x3b, 0x3b, 0xff ),       // 0x3b
//	GXPackedRGB5A3( 0x3c, 0x3c, 0x3c, 0xff ),       // 0x3c
//	GXPackedRGB5A3( 0x3d, 0x3d, 0x3d, 0xff ),       // 0x3d
//	GXPackedRGB5A3( 0x3e, 0x3e, 0x3e, 0xff ),       // 0x3e
//	GXPackedRGB5A3( 0x3f, 0x3f, 0x3f, 0xff ),       // 0x3f
//	GXPackedRGB5A3( 0x40, 0x40, 0x40, 0xff ),		// 0x40
//	GXPackedRGB5A3( 0x41, 0x41, 0x41, 0xff ),       // 0x41
//	GXPackedRGB5A3( 0x42, 0x42, 0x42, 0xff ),       // 0x42
//	GXPackedRGB5A3( 0x43, 0x43, 0x43, 0xff ),       // 0x43
//	GXPackedRGB5A3( 0x44, 0x44, 0x44, 0xff ),       // 0x44
//	GXPackedRGB5A3( 0x45, 0x45, 0x45, 0xff ),       // 0x45
//	GXPackedRGB5A3( 0x46, 0x46, 0x46, 0xff ),       // 0x46
//	GXPackedRGB5A3( 0x47, 0x47, 0x47, 0xff ),       // 0x47
//	GXPackedRGB5A3( 0x48, 0x48, 0x48, 0xff ),       // 0x48
//	GXPackedRGB5A3( 0x49, 0x49, 0x49, 0xff ),       // 0x49
//	GXPackedRGB5A3( 0x4a, 0x4a, 0x4a, 0xff ),       // 0x4a
//	GXPackedRGB5A3( 0x4b, 0x4b, 0x4b, 0xff ),       // 0x4b
//	GXPackedRGB5A3( 0x4c, 0x4c, 0x4c, 0xff ),       // 0x4c
//	GXPackedRGB5A3( 0x4d, 0x4d, 0x4d, 0xff ),       // 0x4d
//	GXPackedRGB5A3( 0x4e, 0x4e, 0x4e, 0xff ),       // 0x4e
//	GXPackedRGB5A3( 0x4f, 0x4f, 0x4f, 0xff ),       // 0x4f
//	GXPackedRGB5A3( 0x50, 0x50, 0x50, 0xff ),		// 0x50
//	GXPackedRGB5A3( 0x51, 0x51, 0x51, 0xff ),       // 0x51
//	GXPackedRGB5A3( 0x52, 0x52, 0x52, 0xff ),       // 0x52
//	GXPackedRGB5A3( 0x53, 0x53, 0x53, 0xff ),       // 0x53
//	GXPackedRGB5A3( 0x54, 0x54, 0x54, 0xff ),       // 0x54
//	GXPackedRGB5A3( 0x55, 0x55, 0x55, 0xff ),       // 0x55
//	GXPackedRGB5A3( 0x56, 0x56, 0x56, 0xff ),       // 0x56
//	GXPackedRGB5A3( 0x57, 0x57, 0x57, 0xff ),       // 0x57
//	GXPackedRGB5A3( 0x58, 0x58, 0x58, 0xff ),       // 0x58
//	GXPackedRGB5A3( 0x59, 0x59, 0x59, 0xff ),       // 0x59
//	GXPackedRGB5A3( 0x5a, 0x5a, 0x5a, 0xff ),       // 0x5a
//	GXPackedRGB5A3( 0x5b, 0x5b, 0x5b, 0xff ),       // 0x5b
//	GXPackedRGB5A3( 0x5c, 0x5c, 0x5c, 0xff ),       // 0x5c
//	GXPackedRGB5A3( 0x5d, 0x5d, 0x5d, 0xff ),       // 0x5d
//	GXPackedRGB5A3( 0x5e, 0x5e, 0x5e, 0xff ),       // 0x5e
//	GXPackedRGB5A3( 0x5f, 0x5f, 0x5f, 0xff ),       // 0x5f
//	GXPackedRGB5A3( 0x60, 0x60, 0x60, 0xff ),		// 0x60
//	GXPackedRGB5A3( 0x61, 0x61, 0x61, 0xff ),       // 0x61
//	GXPackedRGB5A3( 0x62, 0x62, 0x62, 0xff ),       // 0x62
//	GXPackedRGB5A3( 0x63, 0x63, 0x63, 0xff ),       // 0x63
//	GXPackedRGB5A3( 0x64, 0x64, 0x64, 0xff ),       // 0x64
//	GXPackedRGB5A3( 0x65, 0x65, 0x65, 0xff ),       // 0x65
//	GXPackedRGB5A3( 0x66, 0x66, 0x66, 0xff ),       // 0x66
//	GXPackedRGB5A3( 0x67, 0x67, 0x67, 0xff ),       // 0x67
//	GXPackedRGB5A3( 0x68, 0x68, 0x68, 0xff ),       // 0x68
//	GXPackedRGB5A3( 0x69, 0x69, 0x69, 0xff ),       // 0x69
//	GXPackedRGB5A3( 0x6a, 0x6a, 0x6a, 0xff ),       // 0x6a
//	GXPackedRGB5A3( 0x6b, 0x6b, 0x6b, 0xff ),       // 0x6b
//	GXPackedRGB5A3( 0x6c, 0x6c, 0x6c, 0xff ),       // 0x6c
//	GXPackedRGB5A3( 0x6d, 0x6d, 0x6d, 0xff ),       // 0x6d
//	GXPackedRGB5A3( 0x6e, 0x6e, 0x6e, 0xff ),       // 0x6e
//	GXPackedRGB5A3( 0x6f, 0x6f, 0x6f, 0xff ),       // 0x6f
//	GXPackedRGB5A3( 0x70, 0x70, 0x70, 0xff ),		// 0x70
//	GXPackedRGB5A3( 0x71, 0x71, 0x71, 0xff ),       // 0x71
//	GXPackedRGB5A3( 0x72, 0x72, 0x72, 0xff ),       // 0x72
//	GXPackedRGB5A3( 0x73, 0x73, 0x73, 0xff ),       // 0x73
//	GXPackedRGB5A3( 0x74, 0x74, 0x74, 0xff ),       // 0x74
//	GXPackedRGB5A3( 0x75, 0x75, 0x75, 0xff ),       // 0x75
//	GXPackedRGB5A3( 0x76, 0x76, 0x76, 0xff ),       // 0x76
//	GXPackedRGB5A3( 0x77, 0x77, 0x77, 0xff ),       // 0x77
//	GXPackedRGB5A3( 0x78, 0x78, 0x78, 0xff ),       // 0x78
//	GXPackedRGB5A3( 0x79, 0x79, 0x79, 0xff ),       // 0x79
//	GXPackedRGB5A3( 0x7a, 0x7a, 0x7a, 0xff ),       // 0x7a
//	GXPackedRGB5A3( 0x7b, 0x7b, 0x7b, 0xff ),       // 0x7b
//	GXPackedRGB5A3( 0x7c, 0x7c, 0x7c, 0xff ),       // 0x7c
//	GXPackedRGB5A3( 0x7d, 0x7d, 0x7d, 0xff ),       // 0x7d
//	GXPackedRGB5A3( 0x7e, 0x7e, 0x7e, 0xff ),       // 0x7e
//	GXPackedRGB5A3( 0x7f, 0x7f, 0x7f, 0xff ),       // 0x7f
//	GXPackedRGB5A3( 0x80, 0x80, 0x80, 0xff ),		// 0x80
//	GXPackedRGB5A3( 0x81, 0x81, 0x81, 0xff ),       // 0x81
//	GXPackedRGB5A3( 0x82, 0x82, 0x82, 0xff ),       // 0x82
//	GXPackedRGB5A3( 0x83, 0x83, 0x83, 0xff ),       // 0x83
//	GXPackedRGB5A3( 0x84, 0x84, 0x84, 0xff ),       // 0x84
//	GXPackedRGB5A3( 0x85, 0x85, 0x85, 0xff ),       // 0x85
//	GXPackedRGB5A3( 0x86, 0x86, 0x86, 0xff ),       // 0x86
//	GXPackedRGB5A3( 0x87, 0x87, 0x87, 0xff ),       // 0x87
//	GXPackedRGB5A3( 0x88, 0x88, 0x88, 0xff ),       // 0x88
//	GXPackedRGB5A3( 0x89, 0x89, 0x89, 0xff ),       // 0x89
//	GXPackedRGB5A3( 0x8a, 0x8a, 0x8a, 0xff ),       // 0x8a
//	GXPackedRGB5A3( 0x8b, 0x8b, 0x8b, 0xff ),       // 0x8b
//	GXPackedRGB5A3( 0x8c, 0x8c, 0x8c, 0xff ),       // 0x8c
//	GXPackedRGB5A3( 0x8d, 0x8d, 0x8d, 0xff ),       // 0x8d
//	GXPackedRGB5A3( 0x8e, 0x8e, 0x8e, 0xff ),       // 0x8e
//	GXPackedRGB5A3( 0x8f, 0x8f, 0x8f, 0xff ),       // 0x8f
//	GXPackedRGB5A3( 0x90, 0x90, 0x90, 0xff ),		// 0x90
//	GXPackedRGB5A3( 0x91, 0x91, 0x91, 0xff ),       // 0x91
//	GXPackedRGB5A3( 0x92, 0x92, 0x92, 0xff ),       // 0x92
//	GXPackedRGB5A3( 0x93, 0x93, 0x93, 0xff ),       // 0x93
//	GXPackedRGB5A3( 0x94, 0x94, 0x94, 0xff ),       // 0x94
//	GXPackedRGB5A3( 0x95, 0x95, 0x95, 0xff ),       // 0x95
//	GXPackedRGB5A3( 0x96, 0x96, 0x96, 0xff ),       // 0x96
//	GXPackedRGB5A3( 0x97, 0x97, 0x97, 0xff ),       // 0x97
//	GXPackedRGB5A3( 0x98, 0x98, 0x98, 0xff ),       // 0x98
//	GXPackedRGB5A3( 0x99, 0x99, 0x99, 0xff ),       // 0x99
//	GXPackedRGB5A3( 0x9a, 0x9a, 0x9a, 0xff ),       // 0x9a
//	GXPackedRGB5A3( 0x9b, 0x9b, 0x9b, 0xff ),       // 0x9b
//	GXPackedRGB5A3( 0x9c, 0x9c, 0x9c, 0xff ),       // 0x9c
//	GXPackedRGB5A3( 0x9d, 0x9d, 0x9d, 0xff ),       // 0x9d
//	GXPackedRGB5A3( 0x9e, 0x9e, 0x9e, 0xff ),       // 0x9e
//	GXPackedRGB5A3( 0x9f, 0x9f, 0x9f, 0xff ),       // 0x9f
//	GXPackedRGB5A3( 0xa0, 0xa0, 0xa0, 0xff ),		// 0xa0
//	GXPackedRGB5A3( 0xa1, 0xa1, 0xa1, 0xff ),       // 0xa1
//	GXPackedRGB5A3( 0xa2, 0xa2, 0xa2, 0xff ),       // 0xa2
//	GXPackedRGB5A3( 0xa3, 0xa3, 0xa3, 0xff ),       // 0xa3
//	GXPackedRGB5A3( 0xa4, 0xa4, 0xa4, 0xff ),       // 0xa4
//	GXPackedRGB5A3( 0xa5, 0xa5, 0xa5, 0xff ),       // 0xa5
//	GXPackedRGB5A3( 0xa6, 0xa6, 0xa6, 0xff ),       // 0xa6
//	GXPackedRGB5A3( 0xa7, 0xa7, 0xa7, 0xff ),       // 0xa7
//	GXPackedRGB5A3( 0xa8, 0xa8, 0xa8, 0xff ),       // 0xa8
//	GXPackedRGB5A3( 0xa9, 0xa9, 0xa9, 0xff ),       // 0xa9
//	GXPackedRGB5A3( 0xaa, 0xaa, 0xaa, 0xff ),       // 0xaa
//	GXPackedRGB5A3( 0xab, 0xab, 0xab, 0xff ),       // 0xab
//	GXPackedRGB5A3( 0xac, 0xac, 0xac, 0xff ),       // 0xac
//	GXPackedRGB5A3( 0xad, 0xad, 0xad, 0xff ),       // 0xad
//	GXPackedRGB5A3( 0xae, 0xae, 0xae, 0xff ),       // 0xae
//	GXPackedRGB5A3( 0xaf, 0xaf, 0xaf, 0xff ),       // 0xaf
//	GXPackedRGB5A3( 0xb0, 0xb0, 0xb0, 0xff ),		// 0xb0
//	GXPackedRGB5A3( 0xb1, 0xb1, 0xb1, 0xff ),       // 0xb1
//	GXPackedRGB5A3( 0xb2, 0xb2, 0xb2, 0xff ),       // 0xb2
//	GXPackedRGB5A3( 0xb3, 0xb3, 0xb3, 0xff ),       // 0xb3
//	GXPackedRGB5A3( 0xb4, 0xb4, 0xb4, 0xff ),       // 0xb4
//	GXPackedRGB5A3( 0xb5, 0xb5, 0xb5, 0xff ),       // 0xb5
//	GXPackedRGB5A3( 0xb6, 0xb6, 0xb6, 0xff ),       // 0xb6
//	GXPackedRGB5A3( 0xb7, 0xb7, 0xb7, 0xff ),       // 0xb7
//	GXPackedRGB5A3( 0xb8, 0xb8, 0xb8, 0xff ),       // 0xb8
//	GXPackedRGB5A3( 0xb9, 0xb9, 0xb9, 0xff ),       // 0xb9
//	GXPackedRGB5A3( 0xba, 0xba, 0xba, 0xff ),       // 0xba
//	GXPackedRGB5A3( 0xbb, 0xbb, 0xbb, 0xff ),       // 0xbb
//	GXPackedRGB5A3( 0xbc, 0xbc, 0xbc, 0xff ),       // 0xbc
//	GXPackedRGB5A3( 0xbd, 0xbd, 0xbd, 0xff ),       // 0xbd
//	GXPackedRGB5A3( 0xbe, 0xbe, 0xbe, 0xff ),       // 0xbe
//	GXPackedRGB5A3( 0xbf, 0xbf, 0xbf, 0xff ),       // 0xbf
//	GXPackedRGB5A3( 0xc0, 0xc0, 0xc0, 0xff ),		// 0xc0
//	GXPackedRGB5A3( 0xc1, 0xc1, 0xc1, 0xff ),       // 0xc1
//	GXPackedRGB5A3( 0xc2, 0xc2, 0xc2, 0xff ),       // 0xc2
//	GXPackedRGB5A3( 0xc3, 0xc3, 0xc3, 0xff ),       // 0xc3
//	GXPackedRGB5A3( 0xc4, 0xc4, 0xc4, 0xff ),       // 0xc4
//	GXPackedRGB5A3( 0xc5, 0xc5, 0xc5, 0xff ),       // 0xc5
//	GXPackedRGB5A3( 0xc6, 0xc6, 0xc6, 0xff ),       // 0xc6
//	GXPackedRGB5A3( 0xc7, 0xc7, 0xc7, 0xff ),       // 0xc7
//	GXPackedRGB5A3( 0xc8, 0xc8, 0xc8, 0xff ),       // 0xc8
//	GXPackedRGB5A3( 0xc9, 0xc9, 0xc9, 0xff ),       // 0xc9
//	GXPackedRGB5A3( 0xca, 0xca, 0xca, 0xff ),       // 0xca
//	GXPackedRGB5A3( 0xcb, 0xcb, 0xcb, 0xff ),       // 0xcb
//	GXPackedRGB5A3( 0xcc, 0xcc, 0xcc, 0xff ),       // 0xcc
//	GXPackedRGB5A3( 0xcd, 0xcd, 0xcd, 0xff ),       // 0xcd
//	GXPackedRGB5A3( 0xce, 0xce, 0xce, 0xff ),       // 0xce
//	GXPackedRGB5A3( 0xcf, 0xcf, 0xcf, 0xff ),       // 0xcf
//	GXPackedRGB5A3( 0xd0, 0xd0, 0xd0, 0xff ),		// 0xd0
//	GXPackedRGB5A3( 0xd1, 0xd1, 0xd1, 0xff ),       // 0xd1
//	GXPackedRGB5A3( 0xd2, 0xd2, 0xd2, 0xff ),       // 0xd2
//	GXPackedRGB5A3( 0xd3, 0xd3, 0xd3, 0xff ),       // 0xd3
//	GXPackedRGB5A3( 0xd4, 0xd4, 0xd4, 0xff ),       // 0xd4
//	GXPackedRGB5A3( 0xd5, 0xd5, 0xd5, 0xff ),       // 0xd5
//	GXPackedRGB5A3( 0xd6, 0xd6, 0xd6, 0xff ),       // 0xd6
//	GXPackedRGB5A3( 0xd7, 0xd7, 0xd7, 0xff ),       // 0xd7
//	GXPackedRGB5A3( 0xd8, 0xd8, 0xd8, 0xff ),       // 0xd8
//	GXPackedRGB5A3( 0xd9, 0xd9, 0xd9, 0xff ),       // 0xd9
//	GXPackedRGB5A3( 0xda, 0xda, 0xda, 0xff ),       // 0xda
//	GXPackedRGB5A3( 0xdb, 0xdb, 0xdb, 0xff ),       // 0xdb
//	GXPackedRGB5A3( 0xdc, 0xdc, 0xdc, 0xff ),       // 0xdc
//	GXPackedRGB5A3( 0xdd, 0xdd, 0xdd, 0xff ),       // 0xdd
//	GXPackedRGB5A3( 0xde, 0xde, 0xde, 0xff ),       // 0xde
//	GXPackedRGB5A3( 0xdf, 0xdf, 0xdf, 0xff ),       // 0xdf
//	GXPackedRGB5A3( 0xe0, 0xe0, 0xe0, 0xff ),		// 0xe0
//	GXPackedRGB5A3( 0xe1, 0xe1, 0xe1, 0xff ),       // 0xe1
//	GXPackedRGB5A3( 0xe2, 0xe2, 0xe2, 0xff ),       // 0xe2
//	GXPackedRGB5A3( 0xe3, 0xe3, 0xe3, 0xff ),       // 0xe3
//	GXPackedRGB5A3( 0xe4, 0xe4, 0xe4, 0xff ),       // 0xe4
//	GXPackedRGB5A3( 0xe5, 0xe5, 0xe5, 0xff ),       // 0xe5
//	GXPackedRGB5A3( 0xe6, 0xe6, 0xe6, 0xff ),       // 0xe6
//	GXPackedRGB5A3( 0xe7, 0xe7, 0xe7, 0xff ),       // 0xe7
//	GXPackedRGB5A3( 0xe8, 0xe8, 0xe8, 0xff ),       // 0xe8
//	GXPackedRGB5A3( 0xe9, 0xe9, 0xe9, 0xff ),       // 0xe9
//	GXPackedRGB5A3( 0xea, 0xea, 0xea, 0xff ),       // 0xea
//	GXPackedRGB5A3( 0xeb, 0xeb, 0xeb, 0xff ),       // 0xeb
//	GXPackedRGB5A3( 0xec, 0xec, 0xec, 0xff ),       // 0xec
//	GXPackedRGB5A3( 0xed, 0xed, 0xed, 0xff ),       // 0xed
//	GXPackedRGB5A3( 0xee, 0xee, 0xee, 0xff ),       // 0xee
//	GXPackedRGB5A3( 0xef, 0xef, 0xef, 0xff ),       // 0xef
//	GXPackedRGB5A3( 0xf0, 0xf0, 0xf0, 0xff ),		// 0xf0
//	GXPackedRGB5A3( 0xf1, 0xf1, 0xf1, 0xff ),       // 0xf1
//	GXPackedRGB5A3( 0xf2, 0xf2, 0xf2, 0xff ),       // 0xf2
//	GXPackedRGB5A3( 0xf3, 0xf3, 0xf3, 0xff ),       // 0xf3
//	GXPackedRGB5A3( 0xf4, 0xf4, 0xf4, 0xff ),       // 0xf4
//	GXPackedRGB5A3( 0xf5, 0xf5, 0xf5, 0xff ),       // 0xf5
//	GXPackedRGB5A3( 0xf6, 0xf6, 0xf6, 0xff ),       // 0xf6
//	GXPackedRGB5A3( 0xf7, 0xf7, 0xf7, 0xff ),       // 0xf7
//	GXPackedRGB5A3( 0xf8, 0xf8, 0xf8, 0xff ),       // 0xf8
//	GXPackedRGB5A3( 0xf9, 0xf9, 0xf9, 0xff ),       // 0xf9
//	GXPackedRGB5A3( 0xfa, 0xfa, 0xfa, 0xff ),       // 0xfa
//	GXPackedRGB5A3( 0xfb, 0xfb, 0xfb, 0xff ),       // 0xfb
//	GXPackedRGB5A3( 0xfc, 0xfc, 0xfc, 0xff ),       // 0xfc
//	GXPackedRGB5A3( 0xfd, 0xfd, 0xfd, 0xff ),       // 0xfd
//	GXPackedRGB5A3( 0xfe, 0xfe, 0xfe, 0xff ),       // 0xfe
//	GXPackedRGB5A3( 0xff, 0xff, 0xff, 0xff ),       // 0xff
//};

//uint16 zPaletteH[256] __attribute__ (( aligned( 32 ))) = {
//	GXPackedRGB5A3( 0xf0, 0xf0, 0xf0, 0xff ),		// 0x00
//	GXPackedRGB5A3( 0xf0, 0xf0, 0xf0, 0xff ),       // 0x01
//	GXPackedRGB5A3( 0xf0, 0xf0, 0xf0, 0xff ),       // 0x02
//	GXPackedRGB5A3( 0xf0, 0xf0, 0xf0, 0xff ),       // 0x03
//	GXPackedRGB5A3( 0xf0, 0xf0, 0xf0, 0xff ),       // 0x04
//	GXPackedRGB5A3( 0xf0, 0xf0, 0xf0, 0xff ),       // 0x05
//	GXPackedRGB5A3( 0xf0, 0xf0, 0xf0, 0xff ),       // 0x06
//	GXPackedRGB5A3( 0xf0, 0xf0, 0xf0, 0xff ),       // 0x07
//	GXPackedRGB5A3( 0xf0, 0xf0, 0xf0, 0xff ),       // 0x08
//	GXPackedRGB5A3( 0xf0, 0xf0, 0xf0, 0xff ),       // 0x09
//	GXPackedRGB5A3( 0xf0, 0xf0, 0xf0, 0xff ),       // 0x0a
//	GXPackedRGB5A3( 0xf0, 0xf0, 0xf0, 0xff ),       // 0x0b
//	GXPackedRGB5A3( 0xf0, 0xf0, 0xf0, 0xff ),       // 0x0c
//	GXPackedRGB5A3( 0xf0, 0xf0, 0xf0, 0xff ),       // 0x0d
//	GXPackedRGB5A3( 0xf0, 0xf0, 0xf0, 0xff ),       // 0x0e
//	GXPackedRGB5A3( 0xf0, 0xf0, 0xf0, 0xff ),       // 0x0f
//	GXPackedRGB5A3( 0xe0, 0xe0, 0xe0, 0xff ),		// 0x10
//	GXPackedRGB5A3( 0xe0, 0xe0, 0xe0, 0xff ),       // 0x11
//	GXPackedRGB5A3( 0xe0, 0xe0, 0xe0, 0xff ),       // 0x12
//	GXPackedRGB5A3( 0xe0, 0xe0, 0xe0, 0xff ),       // 0x13
//	GXPackedRGB5A3( 0xe0, 0xe0, 0xe0, 0xff ),       // 0x14
//	GXPackedRGB5A3( 0xe0, 0xe0, 0xe0, 0xff ),       // 0x15
//	GXPackedRGB5A3( 0xe0, 0xe0, 0xe0, 0xff ),       // 0x16
//	GXPackedRGB5A3( 0xe0, 0xe0, 0xe0, 0xff ),       // 0x17
//	GXPackedRGB5A3( 0xe0, 0xe0, 0xe0, 0xff ),       // 0x18
//	GXPackedRGB5A3( 0xe0, 0xe0, 0xe0, 0xff ),       // 0x19
//	GXPackedRGB5A3( 0xe0, 0xe0, 0xe0, 0xff ),       // 0x1a
//	GXPackedRGB5A3( 0xe0, 0xe0, 0xe0, 0xff ),       // 0x1b
//	GXPackedRGB5A3( 0xe0, 0xe0, 0xe0, 0xff ),       // 0x1c
//	GXPackedRGB5A3( 0xe0, 0xe0, 0xe0, 0xff ),       // 0x1d
//	GXPackedRGB5A3( 0xe0, 0xe0, 0xe0, 0xff ),       // 0x1e
//	GXPackedRGB5A3( 0xe0, 0xe0, 0xe0, 0xff ),       // 0x1f
//	GXPackedRGB5A3( 0xd0, 0xd0, 0xd0, 0xff ),		// 0x20
//	GXPackedRGB5A3( 0xd0, 0xd0, 0xd0, 0xff ),       // 0x21
//	GXPackedRGB5A3( 0xd0, 0xd0, 0xd0, 0xff ),       // 0x22
//	GXPackedRGB5A3( 0xd0, 0xd0, 0xd0, 0xff ),       // 0x23
//	GXPackedRGB5A3( 0xd0, 0xd0, 0xd0, 0xff ),       // 0x24
//	GXPackedRGB5A3( 0xd0, 0xd0, 0xd0, 0xff ),       // 0x25
//	GXPackedRGB5A3( 0xd0, 0xd0, 0xd0, 0xff ),       // 0x26
//	GXPackedRGB5A3( 0xd0, 0xd0, 0xd0, 0xff ),       // 0x27
//	GXPackedRGB5A3( 0xd0, 0xd0, 0xd0, 0xff ),       // 0x28
//	GXPackedRGB5A3( 0xd0, 0xd0, 0xd0, 0xff ),       // 0x29
//	GXPackedRGB5A3( 0xd0, 0xd0, 0xd0, 0xff ),       // 0x2a
//	GXPackedRGB5A3( 0xd0, 0xd0, 0xd0, 0xff ),       // 0x2b
//	GXPackedRGB5A3( 0xd0, 0xd0, 0xd0, 0xff ),       // 0x2c
//	GXPackedRGB5A3( 0xd0, 0xd0, 0xd0, 0xff ),       // 0x2d
//	GXPackedRGB5A3( 0xd0, 0xd0, 0xd0, 0xff ),       // 0x2e
//	GXPackedRGB5A3( 0xd0, 0xd0, 0xd0, 0xff ),       // 0x2f
//	GXPackedRGB5A3( 0xc0, 0xc0, 0xc0, 0xff ),		// 0x30
//	GXPackedRGB5A3( 0xc0, 0xc0, 0xc0, 0xff ),       // 0x31
//	GXPackedRGB5A3( 0xc0, 0xc0, 0xc0, 0xff ),       // 0x32
//	GXPackedRGB5A3( 0xc0, 0xc0, 0xc0, 0xff ),       // 0x33
//	GXPackedRGB5A3( 0xc0, 0xc0, 0xc0, 0xff ),       // 0x34
//	GXPackedRGB5A3( 0xc0, 0xc0, 0xc0, 0xff ),       // 0x35
//	GXPackedRGB5A3( 0xc0, 0xc0, 0xc0, 0xff ),       // 0x36
//	GXPackedRGB5A3( 0xc0, 0xc0, 0xc0, 0xff ),       // 0x37
//	GXPackedRGB5A3( 0xc0, 0xc0, 0xc0, 0xff ),       // 0x38
//	GXPackedRGB5A3( 0xc0, 0xc0, 0xc0, 0xff ),       // 0x39
//	GXPackedRGB5A3( 0xc0, 0xc0, 0xc0, 0xff ),       // 0x3a
//	GXPackedRGB5A3( 0xc0, 0xc0, 0xc0, 0xff ),       // 0x3b
//	GXPackedRGB5A3( 0xc0, 0xc0, 0xc0, 0xff ),       // 0x3c
//	GXPackedRGB5A3( 0xc0, 0xc0, 0xc0, 0xff ),       // 0x3d
//	GXPackedRGB5A3( 0xc0, 0xc0, 0xc0, 0xff ),       // 0x3e
//	GXPackedRGB5A3( 0xc0, 0xc0, 0xc0, 0xff ),       // 0x3f
//	GXPackedRGB5A3( 0xb0, 0xb0, 0xb0, 0xff ),		// 0x40
//	GXPackedRGB5A3( 0xb0, 0xb0, 0xb0, 0xff ),       // 0x41
//	GXPackedRGB5A3( 0xb0, 0xb0, 0xb0, 0xff ),       // 0x42
//	GXPackedRGB5A3( 0xb0, 0xb0, 0xb0, 0xff ),       // 0x43
//	GXPackedRGB5A3( 0xb0, 0xb0, 0xb0, 0xff ),       // 0x44
//	GXPackedRGB5A3( 0xb0, 0xb0, 0xb0, 0xff ),       // 0x45
//	GXPackedRGB5A3( 0xb0, 0xb0, 0xb0, 0xff ),       // 0x46
//	GXPackedRGB5A3( 0xb0, 0xb0, 0xb0, 0xff ),       // 0x47
//	GXPackedRGB5A3( 0xb0, 0xb0, 0xb0, 0xff ),       // 0x48
//	GXPackedRGB5A3( 0xb0, 0xb0, 0xb0, 0xff ),       // 0x49
//	GXPackedRGB5A3( 0xb0, 0xb0, 0xb0, 0xff ),       // 0x4a
//	GXPackedRGB5A3( 0xb0, 0xb0, 0xb0, 0xff ),       // 0x4b
//	GXPackedRGB5A3( 0xb0, 0xb0, 0xb0, 0xff ),       // 0x4c
//	GXPackedRGB5A3( 0xb0, 0xb0, 0xb0, 0xff ),       // 0x4d
//	GXPackedRGB5A3( 0xb0, 0xb0, 0xb0, 0xff ),       // 0x4e
//	GXPackedRGB5A3( 0xb0, 0xb0, 0xb0, 0xff ),       // 0x4f
//	GXPackedRGB5A3( 0xa0, 0xa0, 0xa0, 0xff ),		// 0x50
//	GXPackedRGB5A3( 0xa0, 0xa0, 0xa0, 0xff ),       // 0x51
//	GXPackedRGB5A3( 0xa0, 0xa0, 0xa0, 0xff ),       // 0x52
//	GXPackedRGB5A3( 0xa0, 0xa0, 0xa0, 0xff ),       // 0x53
//	GXPackedRGB5A3( 0xa0, 0xa0, 0xa0, 0xff ),       // 0x54
//	GXPackedRGB5A3( 0xa0, 0xa0, 0xa0, 0xff ),       // 0x55
//	GXPackedRGB5A3( 0xa0, 0xa0, 0xa0, 0xff ),       // 0x56
//	GXPackedRGB5A3( 0xa0, 0xa0, 0xa0, 0xff ),       // 0x57
//	GXPackedRGB5A3( 0xa0, 0xa0, 0xa0, 0xff ),       // 0x58
//	GXPackedRGB5A3( 0xa0, 0xa0, 0xa0, 0xff ),       // 0x59
//	GXPackedRGB5A3( 0xa0, 0xa0, 0xa0, 0xff ),       // 0x5a
//	GXPackedRGB5A3( 0xa0, 0xa0, 0xa0, 0xff ),       // 0x5b
//	GXPackedRGB5A3( 0xa0, 0xa0, 0xa0, 0xff ),       // 0x5c
//	GXPackedRGB5A3( 0xa0, 0xa0, 0xa0, 0xff ),       // 0x5d
//	GXPackedRGB5A3( 0xa0, 0xa0, 0xa0, 0xff ),       // 0x5e
//	GXPackedRGB5A3( 0xa0, 0xa0, 0xa0, 0xff ),       // 0x5f
//	GXPackedRGB5A3( 0x90, 0x90, 0x90, 0xff ),		// 0x60
//	GXPackedRGB5A3( 0x90, 0x90, 0x90, 0xff ),       // 0x61
//	GXPackedRGB5A3( 0x90, 0x90, 0x90, 0xff ),       // 0x62
//	GXPackedRGB5A3( 0x90, 0x90, 0x90, 0xff ),       // 0x63
//	GXPackedRGB5A3( 0x90, 0x90, 0x90, 0xff ),       // 0x64
//	GXPackedRGB5A3( 0x90, 0x90, 0x90, 0xff ),       // 0x65
//	GXPackedRGB5A3( 0x90, 0x90, 0x90, 0xff ),       // 0x66
//	GXPackedRGB5A3( 0x90, 0x90, 0x90, 0xff ),       // 0x67
//	GXPackedRGB5A3( 0x90, 0x90, 0x90, 0xff ),       // 0x68
//	GXPackedRGB5A3( 0x90, 0x90, 0x90, 0xff ),       // 0x69
//	GXPackedRGB5A3( 0x90, 0x90, 0x90, 0xff ),       // 0x6a
//	GXPackedRGB5A3( 0x90, 0x90, 0x90, 0xff ),       // 0x6b
//	GXPackedRGB5A3( 0x90, 0x90, 0x90, 0xff ),       // 0x6c
//	GXPackedRGB5A3( 0x90, 0x90, 0x90, 0xff ),       // 0x6d
//	GXPackedRGB5A3( 0x90, 0x90, 0x90, 0xff ),       // 0x6e
//	GXPackedRGB5A3( 0x90, 0x90, 0x90, 0xff ),       // 0x6f
//	GXPackedRGB5A3( 0x80, 0x80, 0x80, 0xff ),		// 0x70
//	GXPackedRGB5A3( 0x80, 0x80, 0x80, 0xff ),       // 0x71
//	GXPackedRGB5A3( 0x80, 0x80, 0x80, 0xff ),       // 0x72
//	GXPackedRGB5A3( 0x80, 0x80, 0x80, 0xff ),       // 0x73
//	GXPackedRGB5A3( 0x80, 0x80, 0x80, 0xff ),       // 0x74
//	GXPackedRGB5A3( 0x80, 0x80, 0x80, 0xff ),       // 0x75
//	GXPackedRGB5A3( 0x80, 0x80, 0x80, 0xff ),       // 0x76
//	GXPackedRGB5A3( 0x80, 0x80, 0x80, 0xff ),       // 0x77
//	GXPackedRGB5A3( 0x80, 0x80, 0x80, 0xff ),       // 0x78
//	GXPackedRGB5A3( 0x80, 0x80, 0x80, 0xff ),       // 0x79
//	GXPackedRGB5A3( 0x80, 0x80, 0x80, 0xff ),       // 0x7a
//	GXPackedRGB5A3( 0x80, 0x80, 0x80, 0xff ),       // 0x7b
//	GXPackedRGB5A3( 0x80, 0x80, 0x80, 0xff ),       // 0x7c
//	GXPackedRGB5A3( 0x80, 0x80, 0x80, 0xff ),       // 0x7d
//	GXPackedRGB5A3( 0x80, 0x80, 0x80, 0xff ),       // 0x7e
//	GXPackedRGB5A3( 0x80, 0x80, 0x80, 0xff ),       // 0x7f
//	GXPackedRGB5A3( 0x70, 0x70, 0x70, 0xff ),		// 0x80
//	GXPackedRGB5A3( 0x70, 0x70, 0x70, 0xff ),       // 0x81
//	GXPackedRGB5A3( 0x70, 0x70, 0x70, 0xff ),       // 0x82
//	GXPackedRGB5A3( 0x70, 0x70, 0x70, 0xff ),       // 0x83
//	GXPackedRGB5A3( 0x70, 0x70, 0x70, 0xff ),       // 0x84
//	GXPackedRGB5A3( 0x70, 0x70, 0x70, 0xff ),       // 0x85
//	GXPackedRGB5A3( 0x70, 0x70, 0x70, 0xff ),       // 0x86
//	GXPackedRGB5A3( 0x70, 0x70, 0x70, 0xff ),       // 0x87
//	GXPackedRGB5A3( 0x70, 0x70, 0x70, 0xff ),       // 0x88
//	GXPackedRGB5A3( 0x70, 0x70, 0x70, 0xff ),       // 0x89
//	GXPackedRGB5A3( 0x70, 0x70, 0x70, 0xff ),       // 0x8a
//	GXPackedRGB5A3( 0x70, 0x70, 0x70, 0xff ),       // 0x8b
//	GXPackedRGB5A3( 0x70, 0x70, 0x70, 0xff ),       // 0x8c
//	GXPackedRGB5A3( 0x70, 0x70, 0x70, 0xff ),       // 0x8d
//	GXPackedRGB5A3( 0x70, 0x70, 0x70, 0xff ),       // 0x8e
//	GXPackedRGB5A3( 0x70, 0x70, 0x70, 0xff ),       // 0x8f
//	GXPackedRGB5A3( 0x60, 0x60, 0x60, 0xff ),		// 0x90
//	GXPackedRGB5A3( 0x60, 0x60, 0x60, 0xff ),       // 0x91
//	GXPackedRGB5A3( 0x60, 0x60, 0x60, 0xff ),       // 0x92
//	GXPackedRGB5A3( 0x60, 0x60, 0x60, 0xff ),       // 0x93
//	GXPackedRGB5A3( 0x60, 0x60, 0x60, 0xff ),       // 0x94
//	GXPackedRGB5A3( 0x60, 0x60, 0x60, 0xff ),       // 0x95
//	GXPackedRGB5A3( 0x60, 0x60, 0x60, 0xff ),       // 0x96
//	GXPackedRGB5A3( 0x60, 0x60, 0x60, 0xff ),       // 0x97
//	GXPackedRGB5A3( 0x60, 0x60, 0x60, 0xff ),       // 0x98
//	GXPackedRGB5A3( 0x60, 0x60, 0x60, 0xff ),       // 0x99
//	GXPackedRGB5A3( 0x60, 0x60, 0x60, 0xff ),       // 0x9a
//	GXPackedRGB5A3( 0x60, 0x60, 0x60, 0xff ),       // 0x9b
//	GXPackedRGB5A3( 0x60, 0x60, 0x60, 0xff ),       // 0x9c
//	GXPackedRGB5A3( 0x60, 0x60, 0x60, 0xff ),       // 0x9d
//	GXPackedRGB5A3( 0x60, 0x60, 0x60, 0xff ),       // 0x9e
//	GXPackedRGB5A3( 0x60, 0x60, 0x60, 0xff ),       // 0x9f
//	GXPackedRGB5A3( 0x50, 0x50, 0x50, 0xff ),		// 0xa0
//	GXPackedRGB5A3( 0x50, 0x50, 0x50, 0xff ),       // 0xa1
//	GXPackedRGB5A3( 0x50, 0x50, 0x50, 0xff ),       // 0xa2
//	GXPackedRGB5A3( 0x50, 0x50, 0x50, 0xff ),       // 0xa3
//	GXPackedRGB5A3( 0x50, 0x50, 0x50, 0xff ),       // 0xa4
//	GXPackedRGB5A3( 0x50, 0x50, 0x50, 0xff ),       // 0xa5
//	GXPackedRGB5A3( 0x50, 0x50, 0x50, 0xff ),       // 0xa6
//	GXPackedRGB5A3( 0x50, 0x50, 0x50, 0xff ),       // 0xa7
//	GXPackedRGB5A3( 0x50, 0x50, 0x50, 0xff ),       // 0xa8
//	GXPackedRGB5A3( 0x50, 0x50, 0x50, 0xff ),       // 0xa9
//	GXPackedRGB5A3( 0x50, 0x50, 0x50, 0xff ),       // 0xaa
//	GXPackedRGB5A3( 0x50, 0x50, 0x50, 0xff ),       // 0xab
//	GXPackedRGB5A3( 0x50, 0x50, 0x50, 0xff ),       // 0xac
//	GXPackedRGB5A3( 0x50, 0x50, 0x50, 0xff ),       // 0xad
//	GXPackedRGB5A3( 0x50, 0x50, 0x50, 0xff ),       // 0xae
//	GXPackedRGB5A3( 0x50, 0x50, 0x50, 0xff ),       // 0xaf
//	GXPackedRGB5A3( 0x40, 0x40, 0x40, 0xff ),		// 0xb0
//	GXPackedRGB5A3( 0x40, 0x40, 0x40, 0xff ),       // 0xb1
//	GXPackedRGB5A3( 0x40, 0x40, 0x40, 0xff ),       // 0xb2
//	GXPackedRGB5A3( 0x40, 0x40, 0x40, 0xff ),       // 0xb3
//	GXPackedRGB5A3( 0x40, 0x40, 0x40, 0xff ),       // 0xb4
//	GXPackedRGB5A3( 0x40, 0x40, 0x40, 0xff ),       // 0xb5
//	GXPackedRGB5A3( 0x40, 0x40, 0x40, 0xff ),       // 0xb6
//	GXPackedRGB5A3( 0x40, 0x40, 0x40, 0xff ),       // 0xb7
//	GXPackedRGB5A3( 0x40, 0x40, 0x40, 0xff ),       // 0xb8
//	GXPackedRGB5A3( 0x40, 0x40, 0x40, 0xff ),       // 0xb9
//	GXPackedRGB5A3( 0x40, 0x40, 0x40, 0xff ),       // 0xba
//	GXPackedRGB5A3( 0x40, 0x40, 0x40, 0xff ),       // 0xbb
//	GXPackedRGB5A3( 0x40, 0x40, 0x40, 0xff ),       // 0xbc
//	GXPackedRGB5A3( 0x40, 0x40, 0x40, 0xff ),       // 0xbd
//	GXPackedRGB5A3( 0x40, 0x40, 0x40, 0xff ),       // 0xbe
//	GXPackedRGB5A3( 0x40, 0x40, 0x40, 0xff ),       // 0xbf
//	GXPackedRGB5A3( 0x30, 0x30, 0x30, 0xff ),		// 0xc0
//	GXPackedRGB5A3( 0x30, 0x30, 0x30, 0xff ),       // 0xc1
//	GXPackedRGB5A3( 0x30, 0x30, 0x30, 0xff ),       // 0xc2
//	GXPackedRGB5A3( 0x30, 0x30, 0x30, 0xff ),       // 0xc3
//	GXPackedRGB5A3( 0x30, 0x30, 0x30, 0xff ),       // 0xc4
//	GXPackedRGB5A3( 0x30, 0x30, 0x30, 0xff ),       // 0xc5
//	GXPackedRGB5A3( 0x30, 0x30, 0x30, 0xff ),       // 0xc6
//	GXPackedRGB5A3( 0x30, 0x30, 0x30, 0xff ),       // 0xc7
//	GXPackedRGB5A3( 0x30, 0x30, 0x30, 0xff ),       // 0xc8
//	GXPackedRGB5A3( 0x30, 0x30, 0x30, 0xff ),       // 0xc9
//	GXPackedRGB5A3( 0x30, 0x30, 0x30, 0xff ),       // 0xca
//	GXPackedRGB5A3( 0x30, 0x30, 0x30, 0xff ),       // 0xcb
//	GXPackedRGB5A3( 0x30, 0x30, 0x30, 0xff ),       // 0xcc
//	GXPackedRGB5A3( 0x30, 0x30, 0x30, 0xff ),       // 0xcd
//	GXPackedRGB5A3( 0x30, 0x30, 0x30, 0xff ),       // 0xce
//	GXPackedRGB5A3( 0x30, 0x30, 0x30, 0xff ),       // 0xcf
//	GXPackedRGB5A3( 0x20, 0x20, 0x20, 0xff ),		// 0xd0
//	GXPackedRGB5A3( 0x20, 0x20, 0x20, 0xff ),       // 0xd1
//	GXPackedRGB5A3( 0x20, 0x20, 0x20, 0xff ),       // 0xd2
//	GXPackedRGB5A3( 0x20, 0x20, 0x20, 0xff ),       // 0xd3
//	GXPackedRGB5A3( 0x20, 0x20, 0x20, 0xff ),       // 0xd4
//	GXPackedRGB5A3( 0x20, 0x20, 0x20, 0xff ),       // 0xd5
//	GXPackedRGB5A3( 0x20, 0x20, 0x20, 0xff ),       // 0xd6
//	GXPackedRGB5A3( 0x20, 0x20, 0x20, 0xff ),       // 0xd7
//	GXPackedRGB5A3( 0x20, 0x20, 0x20, 0xff ),       // 0xd8
//	GXPackedRGB5A3( 0x20, 0x20, 0x20, 0xff ),       // 0xd9
//	GXPackedRGB5A3( 0x20, 0x20, 0x20, 0xff ),       // 0xda
//	GXPackedRGB5A3( 0x20, 0x20, 0x20, 0xff ),       // 0xdb
//	GXPackedRGB5A3( 0x20, 0x20, 0x20, 0xff ),       // 0xdc
//	GXPackedRGB5A3( 0x20, 0x20, 0x20, 0xff ),       // 0xdd
//	GXPackedRGB5A3( 0x20, 0x20, 0x20, 0xff ),       // 0xde
//	GXPackedRGB5A3( 0x20, 0x20, 0x20, 0xff ),       // 0xdf
//	GXPackedRGB5A3( 0x10, 0x10, 0x10, 0xff ),		// 0xe0
//	GXPackedRGB5A3( 0x10, 0x10, 0x10, 0xff ),       // 0xe1
//	GXPackedRGB5A3( 0x10, 0x10, 0x10, 0xff ),       // 0xe2
//	GXPackedRGB5A3( 0x10, 0x10, 0x10, 0xff ),       // 0xe3
//	GXPackedRGB5A3( 0x10, 0x10, 0x10, 0xff ),       // 0xe4
//	GXPackedRGB5A3( 0x10, 0x10, 0x10, 0xff ),       // 0xe5
//	GXPackedRGB5A3( 0x10, 0x10, 0x10, 0xff ),       // 0xe6
//	GXPackedRGB5A3( 0x10, 0x10, 0x10, 0xff ),       // 0xe7
//	GXPackedRGB5A3( 0x10, 0x10, 0x10, 0xff ),       // 0xe8
//	GXPackedRGB5A3( 0x10, 0x10, 0x10, 0xff ),       // 0xe9
//	GXPackedRGB5A3( 0x10, 0x10, 0x10, 0xff ),       // 0xea
//	GXPackedRGB5A3( 0x10, 0x10, 0x10, 0xff ),       // 0xeb
//	GXPackedRGB5A3( 0x10, 0x10, 0x10, 0xff ),       // 0xec
//	GXPackedRGB5A3( 0x10, 0x10, 0x10, 0xff ),       // 0xed
//	GXPackedRGB5A3( 0x10, 0x10, 0x10, 0xff ),       // 0xee
//	GXPackedRGB5A3( 0x10, 0x10, 0x10, 0xff ),       // 0xef
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),		// 0xf0
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf1
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf2
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf3
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf4
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf5
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf6
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf7
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf8
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf9
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xfa
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xfb
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xfc
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xfd
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xfe
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xff
//};
//
//uint16 zPaletteL[256] __attribute__ (( aligned( 32 ))) = {
//	GXPackedRGB5A3( 0x0f, 0x0f, 0x0f, 0xff ),		// 0x00
//	GXPackedRGB5A3( 0x0f, 0x0f, 0x0f, 0xff ),       // 0x01
//	GXPackedRGB5A3( 0x0f, 0x0f, 0x0f, 0xff ),       // 0x02
//	GXPackedRGB5A3( 0x0f, 0x0f, 0x0f, 0xff ),       // 0x03
//	GXPackedRGB5A3( 0x0f, 0x0f, 0x0f, 0xff ),       // 0x04
//	GXPackedRGB5A3( 0x0f, 0x0f, 0x0f, 0xff ),       // 0x05
//	GXPackedRGB5A3( 0x0f, 0x0f, 0x0f, 0xff ),       // 0x06
//	GXPackedRGB5A3( 0x0f, 0x0f, 0x0f, 0xff ),       // 0x07
//	GXPackedRGB5A3( 0x0f, 0x0f, 0x0f, 0xff ),       // 0x08
//	GXPackedRGB5A3( 0x0f, 0x0f, 0x0f, 0xff ),       // 0x09
//	GXPackedRGB5A3( 0x0f, 0x0f, 0x0f, 0xff ),       // 0x0a
//	GXPackedRGB5A3( 0x0f, 0x0f, 0x0f, 0xff ),       // 0x0b
//	GXPackedRGB5A3( 0x0f, 0x0f, 0x0f, 0xff ),       // 0x0c
//	GXPackedRGB5A3( 0x0f, 0x0f, 0x0f, 0xff ),       // 0x0d
//	GXPackedRGB5A3( 0x0f, 0x0f, 0x0f, 0xff ),       // 0x0e
//	GXPackedRGB5A3( 0x0f, 0x0f, 0x0f, 0xff ),       // 0x0f
//	GXPackedRGB5A3( 0x0e, 0x0e, 0x0e, 0xff ),		// 0x10
//	GXPackedRGB5A3( 0x0e, 0x0e, 0x0e, 0xff ),       // 0x11
//	GXPackedRGB5A3( 0x0e, 0x0e, 0x0e, 0xff ),       // 0x12
//	GXPackedRGB5A3( 0x0e, 0x0e, 0x0e, 0xff ),       // 0x13
//	GXPackedRGB5A3( 0x0e, 0x0e, 0x0e, 0xff ),       // 0x14
//	GXPackedRGB5A3( 0x0e, 0x0e, 0x0e, 0xff ),       // 0x15
//	GXPackedRGB5A3( 0x0e, 0x0e, 0x0e, 0xff ),       // 0x16
//	GXPackedRGB5A3( 0x0e, 0x0e, 0x0e, 0xff ),       // 0x17
//	GXPackedRGB5A3( 0x0e, 0x0e, 0x0e, 0xff ),       // 0x18
//	GXPackedRGB5A3( 0x0e, 0x0e, 0x0e, 0xff ),       // 0x19
//	GXPackedRGB5A3( 0x0e, 0x0e, 0x0e, 0xff ),       // 0x1a
//	GXPackedRGB5A3( 0x0e, 0x0e, 0x0e, 0xff ),       // 0x1b
//	GXPackedRGB5A3( 0x0e, 0x0e, 0x0e, 0xff ),       // 0x1c
//	GXPackedRGB5A3( 0x0e, 0x0e, 0x0e, 0xff ),       // 0x1d
//	GXPackedRGB5A3( 0x0e, 0x0e, 0x0e, 0xff ),       // 0x1e
//	GXPackedRGB5A3( 0x0e, 0x0e, 0x0e, 0xff ),       // 0x1f
//	GXPackedRGB5A3( 0x0d, 0x0d, 0x0d, 0xff ),		// 0x20
//	GXPackedRGB5A3( 0x0d, 0x0d, 0x0d, 0xff ),       // 0x21
//	GXPackedRGB5A3( 0x0d, 0x0d, 0x0d, 0xff ),       // 0x22
//	GXPackedRGB5A3( 0x0d, 0x0d, 0x0d, 0xff ),       // 0x23
//	GXPackedRGB5A3( 0x0d, 0x0d, 0x0d, 0xff ),       // 0x24
//	GXPackedRGB5A3( 0x0d, 0x0d, 0x0d, 0xff ),       // 0x25
//	GXPackedRGB5A3( 0x0d, 0x0d, 0x0d, 0xff ),       // 0x26
//	GXPackedRGB5A3( 0x0d, 0x0d, 0x0d, 0xff ),       // 0x27
//	GXPackedRGB5A3( 0x0d, 0x0d, 0x0d, 0xff ),       // 0x28
//	GXPackedRGB5A3( 0x0d, 0x0d, 0x0d, 0xff ),       // 0x29
//	GXPackedRGB5A3( 0x0d, 0x0d, 0x0d, 0xff ),       // 0x2a
//	GXPackedRGB5A3( 0x0d, 0x0d, 0x0d, 0xff ),       // 0x2b
//	GXPackedRGB5A3( 0x0d, 0x0d, 0x0d, 0xff ),       // 0x2c
//	GXPackedRGB5A3( 0x0d, 0x0d, 0x0d, 0xff ),       // 0x2d
//	GXPackedRGB5A3( 0x0d, 0x0d, 0x0d, 0xff ),       // 0x2e
//	GXPackedRGB5A3( 0x0d, 0x0d, 0x0d, 0xff ),       // 0x2f
//	GXPackedRGB5A3( 0x0c, 0x0c, 0x0c, 0xff ),		// 0x30
//	GXPackedRGB5A3( 0x0c, 0x0c, 0x0c, 0xff ),       // 0x31
//	GXPackedRGB5A3( 0x0c, 0x0c, 0x0c, 0xff ),       // 0x32
//	GXPackedRGB5A3( 0x0c, 0x0c, 0x0c, 0xff ),       // 0x33
//	GXPackedRGB5A3( 0x0c, 0x0c, 0x0c, 0xff ),       // 0x34
//	GXPackedRGB5A3( 0x0c, 0x0c, 0x0c, 0xff ),       // 0x35
//	GXPackedRGB5A3( 0x0c, 0x0c, 0x0c, 0xff ),       // 0x36
//	GXPackedRGB5A3( 0x0c, 0x0c, 0x0c, 0xff ),       // 0x37
//	GXPackedRGB5A3( 0x0c, 0x0c, 0x0c, 0xff ),       // 0x38
//	GXPackedRGB5A3( 0x0c, 0x0c, 0x0c, 0xff ),       // 0x39
//	GXPackedRGB5A3( 0x0c, 0x0c, 0x0c, 0xff ),       // 0x3a
//	GXPackedRGB5A3( 0x0c, 0x0c, 0x0c, 0xff ),       // 0x3b
//	GXPackedRGB5A3( 0x0c, 0x0c, 0x0c, 0xff ),       // 0x3c
//	GXPackedRGB5A3( 0x0c, 0x0c, 0x0c, 0xff ),       // 0x3d
//	GXPackedRGB5A3( 0x0c, 0x0c, 0x0c, 0xff ),       // 0x3e
//	GXPackedRGB5A3( 0x0c, 0x0c, 0x0c, 0xff ),       // 0x3f
//	GXPackedRGB5A3( 0x0b, 0x0b, 0x0b, 0xff ),		// 0x40
//	GXPackedRGB5A3( 0x0b, 0x0b, 0x0b, 0xff ),       // 0x41
//	GXPackedRGB5A3( 0x0b, 0x0b, 0x0b, 0xff ),       // 0x42
//	GXPackedRGB5A3( 0x0b, 0x0b, 0x0b, 0xff ),       // 0x43
//	GXPackedRGB5A3( 0x0b, 0x0b, 0x0b, 0xff ),       // 0x44
//	GXPackedRGB5A3( 0x0b, 0x0b, 0x0b, 0xff ),       // 0x45
//	GXPackedRGB5A3( 0x0b, 0x0b, 0x0b, 0xff ),       // 0x46
//	GXPackedRGB5A3( 0x0b, 0x0b, 0x0b, 0xff ),       // 0x47
//	GXPackedRGB5A3( 0x0b, 0x0b, 0x0b, 0xff ),       // 0x48
//	GXPackedRGB5A3( 0x0b, 0x0b, 0x0b, 0xff ),       // 0x49
//	GXPackedRGB5A3( 0x0b, 0x0b, 0x0b, 0xff ),       // 0x4a
//	GXPackedRGB5A3( 0x0b, 0x0b, 0x0b, 0xff ),       // 0x4b
//	GXPackedRGB5A3( 0x0b, 0x0b, 0x0b, 0xff ),       // 0x4c
//	GXPackedRGB5A3( 0x0b, 0x0b, 0x0b, 0xff ),       // 0x4d
//	GXPackedRGB5A3( 0x0b, 0x0b, 0x0b, 0xff ),       // 0x4e
//	GXPackedRGB5A3( 0x0b, 0x0b, 0x0b, 0xff ),       // 0x4f
//	GXPackedRGB5A3( 0x0a, 0x0a, 0x0a, 0xff ),		// 0x50
//	GXPackedRGB5A3( 0x0a, 0x0a, 0x0a, 0xff ),       // 0x51
//	GXPackedRGB5A3( 0x0a, 0x0a, 0x0a, 0xff ),       // 0x52
//	GXPackedRGB5A3( 0x0a, 0x0a, 0x0a, 0xff ),       // 0x53
//	GXPackedRGB5A3( 0x0a, 0x0a, 0x0a, 0xff ),       // 0x54
//	GXPackedRGB5A3( 0x0a, 0x0a, 0x0a, 0xff ),       // 0x55
//	GXPackedRGB5A3( 0x0a, 0x0a, 0x0a, 0xff ),       // 0x56
//	GXPackedRGB5A3( 0x0a, 0x0a, 0x0a, 0xff ),       // 0x57
//	GXPackedRGB5A3( 0x0a, 0x0a, 0x0a, 0xff ),       // 0x58
//	GXPackedRGB5A3( 0x0a, 0x0a, 0x0a, 0xff ),       // 0x59
//	GXPackedRGB5A3( 0x0a, 0x0a, 0x0a, 0xff ),       // 0x5a
//	GXPackedRGB5A3( 0x0a, 0x0a, 0x0a, 0xff ),       // 0x5b
//	GXPackedRGB5A3( 0x0a, 0x0a, 0x0a, 0xff ),       // 0x5c
//	GXPackedRGB5A3( 0x0a, 0x0a, 0x0a, 0xff ),       // 0x5d
//	GXPackedRGB5A3( 0x0a, 0x0a, 0x0a, 0xff ),       // 0x5e
//	GXPackedRGB5A3( 0x0a, 0x0a, 0x0a, 0xff ),       // 0x5f
//	GXPackedRGB5A3( 0x09, 0x09, 0x09, 0xff ),		// 0x60
//	GXPackedRGB5A3( 0x09, 0x09, 0x09, 0xff ),       // 0x61
//	GXPackedRGB5A3( 0x09, 0x09, 0x09, 0xff ),       // 0x62
//	GXPackedRGB5A3( 0x09, 0x09, 0x09, 0xff ),       // 0x63
//	GXPackedRGB5A3( 0x09, 0x09, 0x09, 0xff ),       // 0x64
//	GXPackedRGB5A3( 0x09, 0x09, 0x09, 0xff ),       // 0x65
//	GXPackedRGB5A3( 0x09, 0x09, 0x09, 0xff ),       // 0x66
//	GXPackedRGB5A3( 0x09, 0x09, 0x09, 0xff ),       // 0x67
//	GXPackedRGB5A3( 0x09, 0x09, 0x09, 0xff ),       // 0x68
//	GXPackedRGB5A3( 0x09, 0x09, 0x09, 0xff ),       // 0x69
//	GXPackedRGB5A3( 0x09, 0x09, 0x09, 0xff ),       // 0x6a
//	GXPackedRGB5A3( 0x09, 0x09, 0x09, 0xff ),       // 0x6b
//	GXPackedRGB5A3( 0x09, 0x09, 0x09, 0xff ),       // 0x6c
//	GXPackedRGB5A3( 0x09, 0x09, 0x09, 0xff ),       // 0x6d
//	GXPackedRGB5A3( 0x09, 0x09, 0x09, 0xff ),       // 0x6e
//	GXPackedRGB5A3( 0x09, 0x09, 0x09, 0xff ),       // 0x6f
//	GXPackedRGB5A3( 0x08, 0x08, 0x08, 0xff ),		// 0x70
//	GXPackedRGB5A3( 0x08, 0x08, 0x08, 0xff ),       // 0x71
//	GXPackedRGB5A3( 0x08, 0x08, 0x08, 0xff ),       // 0x72
//	GXPackedRGB5A3( 0x08, 0x08, 0x08, 0xff ),       // 0x73
//	GXPackedRGB5A3( 0x08, 0x08, 0x08, 0xff ),       // 0x74
//	GXPackedRGB5A3( 0x08, 0x08, 0x08, 0xff ),       // 0x75
//	GXPackedRGB5A3( 0x08, 0x08, 0x08, 0xff ),       // 0x76
//	GXPackedRGB5A3( 0x08, 0x08, 0x08, 0xff ),       // 0x77
//	GXPackedRGB5A3( 0x08, 0x08, 0x08, 0xff ),       // 0x78
//	GXPackedRGB5A3( 0x08, 0x08, 0x08, 0xff ),       // 0x79
//	GXPackedRGB5A3( 0x08, 0x08, 0x08, 0xff ),       // 0x7a
//	GXPackedRGB5A3( 0x08, 0x08, 0x08, 0xff ),       // 0x7b
//	GXPackedRGB5A3( 0x08, 0x08, 0x08, 0xff ),       // 0x7c
//	GXPackedRGB5A3( 0x08, 0x08, 0x08, 0xff ),       // 0x7d
//	GXPackedRGB5A3( 0x08, 0x08, 0x08, 0xff ),       // 0x7e
//	GXPackedRGB5A3( 0x08, 0x08, 0x08, 0xff ),       // 0x7f
//	GXPackedRGB5A3( 0x07, 0x07, 0x07, 0xff ),		// 0x80
//	GXPackedRGB5A3( 0x07, 0x07, 0x07, 0xff ),       // 0x81
//	GXPackedRGB5A3( 0x07, 0x07, 0x07, 0xff ),       // 0x82
//	GXPackedRGB5A3( 0x07, 0x07, 0x07, 0xff ),       // 0x83
//	GXPackedRGB5A3( 0x07, 0x07, 0x07, 0xff ),       // 0x84
//	GXPackedRGB5A3( 0x07, 0x07, 0x07, 0xff ),       // 0x85
//	GXPackedRGB5A3( 0x07, 0x07, 0x07, 0xff ),       // 0x86
//	GXPackedRGB5A3( 0x07, 0x07, 0x07, 0xff ),       // 0x87
//	GXPackedRGB5A3( 0x07, 0x07, 0x07, 0xff ),       // 0x88
//	GXPackedRGB5A3( 0x07, 0x07, 0x07, 0xff ),       // 0x89
//	GXPackedRGB5A3( 0x07, 0x07, 0x07, 0xff ),       // 0x8a
//	GXPackedRGB5A3( 0x07, 0x07, 0x07, 0xff ),       // 0x8b
//	GXPackedRGB5A3( 0x07, 0x07, 0x07, 0xff ),       // 0x8c
//	GXPackedRGB5A3( 0x07, 0x07, 0x07, 0xff ),       // 0x8d
//	GXPackedRGB5A3( 0x07, 0x07, 0x07, 0xff ),       // 0x8e
//	GXPackedRGB5A3( 0x07, 0x07, 0x07, 0xff ),       // 0x8f
//	GXPackedRGB5A3( 0x06, 0x06, 0x06, 0xff ),		// 0x90
//	GXPackedRGB5A3( 0x06, 0x06, 0x06, 0xff ),       // 0x91
//	GXPackedRGB5A3( 0x06, 0x06, 0x06, 0xff ),       // 0x92
//	GXPackedRGB5A3( 0x06, 0x06, 0x06, 0xff ),       // 0x93
//	GXPackedRGB5A3( 0x06, 0x06, 0x06, 0xff ),       // 0x94
//	GXPackedRGB5A3( 0x06, 0x06, 0x06, 0xff ),       // 0x95
//	GXPackedRGB5A3( 0x06, 0x06, 0x06, 0xff ),       // 0x96
//	GXPackedRGB5A3( 0x06, 0x06, 0x06, 0xff ),       // 0x97
//	GXPackedRGB5A3( 0x06, 0x06, 0x06, 0xff ),       // 0x98
//	GXPackedRGB5A3( 0x06, 0x06, 0x06, 0xff ),       // 0x99
//	GXPackedRGB5A3( 0x06, 0x06, 0x06, 0xff ),       // 0x9a
//	GXPackedRGB5A3( 0x06, 0x06, 0x06, 0xff ),       // 0x9b
//	GXPackedRGB5A3( 0x06, 0x06, 0x06, 0xff ),       // 0x9c
//	GXPackedRGB5A3( 0x06, 0x06, 0x06, 0xff ),       // 0x9d
//	GXPackedRGB5A3( 0x06, 0x06, 0x06, 0xff ),       // 0x9e
//	GXPackedRGB5A3( 0x06, 0x06, 0x06, 0xff ),       // 0x9f
//	GXPackedRGB5A3( 0x05, 0x05, 0x05, 0xff ),		// 0xa0
//	GXPackedRGB5A3( 0x05, 0x05, 0x05, 0xff ),       // 0xa1
//	GXPackedRGB5A3( 0x05, 0x05, 0x05, 0xff ),       // 0xa2
//	GXPackedRGB5A3( 0x05, 0x05, 0x05, 0xff ),       // 0xa3
//	GXPackedRGB5A3( 0x05, 0x05, 0x05, 0xff ),       // 0xa4
//	GXPackedRGB5A3( 0x05, 0x05, 0x05, 0xff ),       // 0xa5
//	GXPackedRGB5A3( 0x05, 0x05, 0x05, 0xff ),       // 0xa6
//	GXPackedRGB5A3( 0x05, 0x05, 0x05, 0xff ),       // 0xa7
//	GXPackedRGB5A3( 0x05, 0x05, 0x05, 0xff ),       // 0xa8
//	GXPackedRGB5A3( 0x05, 0x05, 0x05, 0xff ),       // 0xa9
//	GXPackedRGB5A3( 0x05, 0x05, 0x05, 0xff ),       // 0xaa
//	GXPackedRGB5A3( 0x05, 0x05, 0x05, 0xff ),       // 0xab
//	GXPackedRGB5A3( 0x05, 0x05, 0x05, 0xff ),       // 0xac
//	GXPackedRGB5A3( 0x05, 0x05, 0x05, 0xff ),       // 0xad
//	GXPackedRGB5A3( 0x05, 0x05, 0x05, 0xff ),       // 0xae
//	GXPackedRGB5A3( 0x05, 0x05, 0x05, 0xff ),       // 0xaf
//	GXPackedRGB5A3( 0x04, 0x04, 0x04, 0xff ),		// 0xb0
//	GXPackedRGB5A3( 0x04, 0x04, 0x04, 0xff ),       // 0xb1
//	GXPackedRGB5A3( 0x04, 0x04, 0x04, 0xff ),       // 0xb2
//	GXPackedRGB5A3( 0x04, 0x04, 0x04, 0xff ),       // 0xb3
//	GXPackedRGB5A3( 0x04, 0x04, 0x04, 0xff ),       // 0xb4
//	GXPackedRGB5A3( 0x04, 0x04, 0x04, 0xff ),       // 0xb5
//	GXPackedRGB5A3( 0x04, 0x04, 0x04, 0xff ),       // 0xb6
//	GXPackedRGB5A3( 0x04, 0x04, 0x04, 0xff ),       // 0xb7
//	GXPackedRGB5A3( 0x04, 0x04, 0x04, 0xff ),       // 0xb8
//	GXPackedRGB5A3( 0x04, 0x04, 0x04, 0xff ),       // 0xb9
//	GXPackedRGB5A3( 0x04, 0x04, 0x04, 0xff ),       // 0xba
//	GXPackedRGB5A3( 0x04, 0x04, 0x04, 0xff ),       // 0xbb
//	GXPackedRGB5A3( 0x04, 0x04, 0x04, 0xff ),       // 0xbc
//	GXPackedRGB5A3( 0x04, 0x04, 0x04, 0xff ),       // 0xbd
//	GXPackedRGB5A3( 0x04, 0x04, 0x04, 0xff ),       // 0xbe
//	GXPackedRGB5A3( 0x04, 0x04, 0x04, 0xff ),       // 0xbf
//	GXPackedRGB5A3( 0x03, 0x03, 0x03, 0xff ),		// 0xc0
//	GXPackedRGB5A3( 0x03, 0x03, 0x03, 0xff ),       // 0xc1
//	GXPackedRGB5A3( 0x03, 0x03, 0x03, 0xff ),       // 0xc2
//	GXPackedRGB5A3( 0x03, 0x03, 0x03, 0xff ),       // 0xc3
//	GXPackedRGB5A3( 0x03, 0x03, 0x03, 0xff ),       // 0xc4
//	GXPackedRGB5A3( 0x03, 0x03, 0x03, 0xff ),       // 0xc5
//	GXPackedRGB5A3( 0x03, 0x03, 0x03, 0xff ),       // 0xc6
//	GXPackedRGB5A3( 0x03, 0x03, 0x03, 0xff ),       // 0xc7
//	GXPackedRGB5A3( 0x03, 0x03, 0x03, 0xff ),       // 0xc8
//	GXPackedRGB5A3( 0x03, 0x03, 0x03, 0xff ),       // 0xc9
//	GXPackedRGB5A3( 0x03, 0x03, 0x03, 0xff ),       // 0xca
//	GXPackedRGB5A3( 0x03, 0x03, 0x03, 0xff ),       // 0xcb
//	GXPackedRGB5A3( 0x03, 0x03, 0x03, 0xff ),       // 0xcc
//	GXPackedRGB5A3( 0x03, 0x03, 0x03, 0xff ),       // 0xcd
//	GXPackedRGB5A3( 0x03, 0x03, 0x03, 0xff ),       // 0xce
//	GXPackedRGB5A3( 0x03, 0x03, 0x03, 0xff ),       // 0xcf
//	GXPackedRGB5A3( 0x02, 0x02, 0x02, 0xff ),		// 0xd0
//	GXPackedRGB5A3( 0x02, 0x02, 0x02, 0xff ),       // 0xd1
//	GXPackedRGB5A3( 0x02, 0x02, 0x02, 0xff ),       // 0xd2
//	GXPackedRGB5A3( 0x02, 0x02, 0x02, 0xff ),       // 0xd3
//	GXPackedRGB5A3( 0x02, 0x02, 0x02, 0xff ),       // 0xd4
//	GXPackedRGB5A3( 0x02, 0x02, 0x02, 0xff ),       // 0xd5
//	GXPackedRGB5A3( 0x02, 0x02, 0x02, 0xff ),       // 0xd6
//	GXPackedRGB5A3( 0x02, 0x02, 0x02, 0xff ),       // 0xd7
//	GXPackedRGB5A3( 0x02, 0x02, 0x02, 0xff ),       // 0xd8
//	GXPackedRGB5A3( 0x02, 0x02, 0x02, 0xff ),       // 0xd9
//	GXPackedRGB5A3( 0x02, 0x02, 0x02, 0xff ),       // 0xda
//	GXPackedRGB5A3( 0x02, 0x02, 0x02, 0xff ),       // 0xdb
//	GXPackedRGB5A3( 0x02, 0x02, 0x02, 0xff ),       // 0xdc
//	GXPackedRGB5A3( 0x02, 0x02, 0x02, 0xff ),       // 0xdd
//	GXPackedRGB5A3( 0x02, 0x02, 0x02, 0xff ),       // 0xde
//	GXPackedRGB5A3( 0x02, 0x02, 0x02, 0xff ),       // 0xdf
//	GXPackedRGB5A3( 0x01, 0x01, 0x01, 0xff ),		// 0xe0
//	GXPackedRGB5A3( 0x01, 0x01, 0x01, 0xff ),       // 0xe1
//	GXPackedRGB5A3( 0x01, 0x01, 0x01, 0xff ),       // 0xe2
//	GXPackedRGB5A3( 0x01, 0x01, 0x01, 0xff ),       // 0xe3
//	GXPackedRGB5A3( 0x01, 0x01, 0x01, 0xff ),       // 0xe4
//	GXPackedRGB5A3( 0x01, 0x01, 0x01, 0xff ),       // 0xe5
//	GXPackedRGB5A3( 0x01, 0x01, 0x01, 0xff ),       // 0xe6
//	GXPackedRGB5A3( 0x01, 0x01, 0x01, 0xff ),       // 0xe7
//	GXPackedRGB5A3( 0x01, 0x01, 0x01, 0xff ),       // 0xe8
//	GXPackedRGB5A3( 0x01, 0x01, 0x01, 0xff ),       // 0xe9
//	GXPackedRGB5A3( 0x01, 0x01, 0x01, 0xff ),       // 0xea
//	GXPackedRGB5A3( 0x01, 0x01, 0x01, 0xff ),       // 0xeb
//	GXPackedRGB5A3( 0x01, 0x01, 0x01, 0xff ),       // 0xec
//	GXPackedRGB5A3( 0x01, 0x01, 0x01, 0xff ),       // 0xed
//	GXPackedRGB5A3( 0x01, 0x01, 0x01, 0xff ),       // 0xee
//	GXPackedRGB5A3( 0x01, 0x01, 0x01, 0xff ),       // 0xef
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),		// 0xf0
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf1
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf2
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf3
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf4
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf5
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf6
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf7
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf8
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xf9
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xfa
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xfb
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xfc
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xfd
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xfe
//	GXPackedRGB5A3( 0x00, 0x00, 0x00, 0xff ),       // 0xff
//};
//
//uint8 zPalette8[256] __attribute__ (( aligned( 32 ))) = {
//	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
//	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
//	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
//	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
//	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
//	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
//	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
//	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
//	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
//	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
//	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
//	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
//	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
//	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
//	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
//	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
//};
//
//#define _H(a)  (((a<<(8+6))&0xc000)|((a<<6)&0x00c0))
//
//uint16 zPalette8H[256] __attribute__ (( aligned( 32 ))) = {
//	_H(0x00),_H(0x01),_H(0x02),_H(0x03),_H(0x04),_H(0x05),_H(0x06),_H(0x07),_H(0x08),_H(0x09),_H(0x0a),_H(0x0b),_H(0x0c),_H(0x0d),_H(0x0e),_H(0x0f),
//	_H(0x10),_H(0x11),_H(0x12),_H(0x13),_H(0x14),_H(0x15),_H(0x16),_H(0x17),_H(0x18),_H(0x19),_H(0x1a),_H(0x1b),_H(0x1c),_H(0x1d),_H(0x1e),_H(0x1f),
//	_H(0x20),_H(0x21),_H(0x22),_H(0x23),_H(0x24),_H(0x25),_H(0x26),_H(0x27),_H(0x28),_H(0x29),_H(0x2a),_H(0x2b),_H(0x2c),_H(0x2d),_H(0x2e),_H(0x2f),
//	_H(0x30),_H(0x31),_H(0x32),_H(0x33),_H(0x34),_H(0x35),_H(0x36),_H(0x37),_H(0x38),_H(0x39),_H(0x3a),_H(0x3b),_H(0x3c),_H(0x3d),_H(0x3e),_H(0x3f),
//	_H(0x40),_H(0x41),_H(0x42),_H(0x43),_H(0x44),_H(0x45),_H(0x46),_H(0x47),_H(0x48),_H(0x49),_H(0x4a),_H(0x4b),_H(0x4c),_H(0x4d),_H(0x4e),_H(0x4f),
//	_H(0x50),_H(0x51),_H(0x52),_H(0x53),_H(0x54),_H(0x55),_H(0x56),_H(0x57),_H(0x58),_H(0x59),_H(0x5a),_H(0x5b),_H(0x5c),_H(0x5d),_H(0x5e),_H(0x5f),
//	_H(0x60),_H(0x61),_H(0x62),_H(0x63),_H(0x64),_H(0x65),_H(0x66),_H(0x67),_H(0x68),_H(0x69),_H(0x6a),_H(0x6b),_H(0x6c),_H(0x6d),_H(0x6e),_H(0x6f),
//	_H(0x70),_H(0x71),_H(0x72),_H(0x73),_H(0x74),_H(0x75),_H(0x76),_H(0x77),_H(0x78),_H(0x79),_H(0x7a),_H(0x7b),_H(0x7c),_H(0x7d),_H(0x7e),_H(0x7f),
//	_H(0x80),_H(0x81),_H(0x82),_H(0x83),_H(0x84),_H(0x85),_H(0x86),_H(0x87),_H(0x88),_H(0x89),_H(0x8a),_H(0x8b),_H(0x8c),_H(0x8d),_H(0x8e),_H(0x8f),
//	_H(0x90),_H(0x91),_H(0x92),_H(0x93),_H(0x94),_H(0x95),_H(0x96),_H(0x97),_H(0x98),_H(0x99),_H(0x9a),_H(0x9b),_H(0x9c),_H(0x9d),_H(0x9e),_H(0x9f),
//	_H(0xa0),_H(0xa1),_H(0xa2),_H(0xa3),_H(0xa4),_H(0xa5),_H(0xa6),_H(0xa7),_H(0xa8),_H(0xa9),_H(0xaa),_H(0xab),_H(0xac),_H(0xad),_H(0xae),_H(0xaf),
//	_H(0xb0),_H(0xb1),_H(0xb2),_H(0xb3),_H(0xb4),_H(0xb5),_H(0xb6),_H(0xb7),_H(0xb8),_H(0xb9),_H(0xba),_H(0xbb),_H(0xbc),_H(0xbd),_H(0xbe),_H(0xbf),
//	_H(0xc0),_H(0xc1),_H(0xc2),_H(0xc3),_H(0xc4),_H(0xc5),_H(0xc6),_H(0xc7),_H(0xc8),_H(0xc9),_H(0xca),_H(0xcb),_H(0xcc),_H(0xcd),_H(0xce),_H(0xcf),
//	_H(0xd0),_H(0xd1),_H(0xd2),_H(0xd3),_H(0xd4),_H(0xd5),_H(0xd6),_H(0xd7),_H(0xd8),_H(0xd9),_H(0xda),_H(0xdb),_H(0xdc),_H(0xdd),_H(0xde),_H(0xdf),
//	_H(0xe0),_H(0xe1),_H(0xe2),_H(0xe3),_H(0xe4),_H(0xe5),_H(0xe6),_H(0xe7),_H(0xe8),_H(0xe9),_H(0xea),_H(0xeb),_H(0xec),_H(0xed),_H(0xee),_H(0xef),
//	_H(0xf0),_H(0xf1),_H(0xf2),_H(0xf3),_H(0xf4),_H(0xf5),_H(0xf6),_H(0xf7),_H(0xf8),_H(0xf9),_H(0xfa),_H(0xfb),_H(0xfc),_H(0xfd),_H(0xfe),_H(0xff)
//};
//
//#define _L(a)  (((a<<(8-2))&0x3f00)|((a>>2)&0x003f))
////#define _L(a)  ((a<<8)|a)
//
//uint16 zPalette8L[256] __attribute__ (( aligned( 32 ))) = {
//	_L(0x00),_L(0x01),_L(0x02),_L(0x03),_L(0x04),_L(0x05),_L(0x06),_L(0x07),_L(0x08),_L(0x09),_L(0x0a),_L(0x0b),_L(0x0c),_L(0x0d),_L(0x0e),_L(0x0f),
//	_L(0x10),_L(0x11),_L(0x12),_L(0x13),_L(0x14),_L(0x15),_L(0x16),_L(0x17),_L(0x18),_L(0x19),_L(0x1a),_L(0x1b),_L(0x1c),_L(0x1d),_L(0x1e),_L(0x1f),
//	_L(0x20),_L(0x21),_L(0x22),_L(0x23),_L(0x24),_L(0x25),_L(0x26),_L(0x27),_L(0x28),_L(0x29),_L(0x2a),_L(0x2b),_L(0x2c),_L(0x2d),_L(0x2e),_L(0x2f),
//	_L(0x30),_L(0x31),_L(0x32),_L(0x33),_L(0x34),_L(0x35),_L(0x36),_L(0x37),_L(0x38),_L(0x39),_L(0x3a),_L(0x3b),_L(0x3c),_L(0x3d),_L(0x3e),_L(0x3f),
//	_L(0x40),_L(0x41),_L(0x42),_L(0x43),_L(0x44),_L(0x45),_L(0x46),_L(0x47),_L(0x48),_L(0x49),_L(0x4a),_L(0x4b),_L(0x4c),_L(0x4d),_L(0x4e),_L(0x4f),
//	_L(0x50),_L(0x51),_L(0x52),_L(0x53),_L(0x54),_L(0x55),_L(0x56),_L(0x57),_L(0x58),_L(0x59),_L(0x5a),_L(0x5b),_L(0x5c),_L(0x5d),_L(0x5e),_L(0x5f),
//	_L(0x60),_L(0x61),_L(0x62),_L(0x63),_L(0x64),_L(0x65),_L(0x66),_L(0x67),_L(0x68),_L(0x69),_L(0x6a),_L(0x6b),_L(0x6c),_L(0x6d),_L(0x6e),_L(0x6f),
//	_L(0x70),_L(0x71),_L(0x72),_L(0x73),_L(0x74),_L(0x75),_L(0x76),_L(0x77),_L(0x78),_L(0x79),_L(0x7a),_L(0x7b),_L(0x7c),_L(0x7d),_L(0x7e),_L(0x7f),
//	_L(0x80),_L(0x81),_L(0x82),_L(0x83),_L(0x84),_L(0x85),_L(0x86),_L(0x87),_L(0x88),_L(0x89),_L(0x8a),_L(0x8b),_L(0x8c),_L(0x8d),_L(0x8e),_L(0x8f),
//	_L(0x90),_L(0x91),_L(0x92),_L(0x93),_L(0x94),_L(0x95),_L(0x96),_L(0x97),_L(0x98),_L(0x99),_L(0x9a),_L(0x9b),_L(0x9c),_L(0x9d),_L(0x9e),_L(0x9f),
//	_L(0xa0),_L(0xa1),_L(0xa2),_L(0xa3),_L(0xa4),_L(0xa5),_L(0xa6),_L(0xa7),_L(0xa8),_L(0xa9),_L(0xaa),_L(0xab),_L(0xac),_L(0xad),_L(0xae),_L(0xaf),
//	_L(0xb0),_L(0xb1),_L(0xb2),_L(0xb3),_L(0xb4),_L(0xb5),_L(0xb6),_L(0xb7),_L(0xb8),_L(0xb9),_L(0xba),_L(0xbb),_L(0xbc),_L(0xbd),_L(0xbe),_L(0xbf),
//	_L(0xc0),_L(0xc1),_L(0xc2),_L(0xc3),_L(0xc4),_L(0xc5),_L(0xc6),_L(0xc7),_L(0xc8),_L(0xc9),_L(0xca),_L(0xcb),_L(0xcc),_L(0xcd),_L(0xce),_L(0xcf),
//	_L(0xd0),_L(0xd1),_L(0xd2),_L(0xd3),_L(0xd4),_L(0xd5),_L(0xd6),_L(0xd7),_L(0xd8),_L(0xd9),_L(0xda),_L(0xdb),_L(0xdc),_L(0xdd),_L(0xde),_L(0xdf),
//	_L(0xe0),_L(0xe1),_L(0xe2),_L(0xe3),_L(0xe4),_L(0xe5),_L(0xe6),_L(0xe7),_L(0xe8),_L(0xe9),_L(0xea),_L(0xeb),_L(0xec),_L(0xed),_L(0xee),_L(0xef),
//	_L(0xf0),_L(0xf1),_L(0xf2),_L(0xf3),_L(0xf4),_L(0xf5),_L(0xf6),_L(0xf7),_L(0xf8),_L(0xf9),_L(0xfa),_L(0xfb),_L(0xfc),_L(0xfd),_L(0xfe),_L(0xff)
//};
//
//#define _64(a)	((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),\
//				((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),\
//				((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),\
//				((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),\
//				((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),\
//				((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),\
//				((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),\
//				((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a),((a<<8)|a)
//
//#define _I(a)	((a<<8)|a)
//
//uint16 zPalette16[65536] __attribute__ (( aligned( 32 ))) = {
//
//	_64(0x00),_64(0x01),_64(0x02),_64(0x03),_64(0x04),_64(0x05),_64(0x06),_64(0x07),_64(0x08),_64(0x09),_64(0x0a),_64(0x0b),_64(0x0c),_64(0x0d),_64(0x0e),_64(0x0f),
//	_64(0x10),_64(0x11),_64(0x12),_64(0x13),_64(0x14),_64(0x15),_64(0x16),_64(0x17),_64(0x18),_64(0x19),_64(0x1a),_64(0x1b),_64(0x1c),_64(0x1d),_64(0x1e),_64(0x1f),
//	_64(0x20),_64(0x21),_64(0x22),_64(0x23),_64(0x24),_64(0x25),_64(0x26),_64(0x27),_64(0x28),_64(0x29),_64(0x2a),_64(0x2b),_64(0x2c),_64(0x2d),_64(0x2e),_64(0x2f),
//	_64(0x30),_64(0x31),_64(0x32),_64(0x33),_64(0x34),_64(0x35),_64(0x36),_64(0x37),_64(0x38),_64(0x39),_64(0x3a),_64(0x3b),_64(0x3c),_64(0x3d),_64(0x3e),_64(0x3f),
//	_64(0x40),_64(0x41),_64(0x42),_64(0x43),_64(0x44),_64(0x45),_64(0x46),_64(0x47),_64(0x48),_64(0x49),_64(0x4a),_64(0x4b),_64(0x4c),_64(0x4d),_64(0x4e),_64(0x4f),
//	_64(0x50),_64(0x51),_64(0x52),_64(0x53),_64(0x54),_64(0x55),_64(0x56),_64(0x57),_64(0x58),_64(0x59),_64(0x5a),_64(0x5b),_64(0x5c),_64(0x5d),_64(0x5e),_64(0x5f),
//	_64(0x60),_64(0x61),_64(0x62),_64(0x63),_64(0x64),_64(0x65),_64(0x66),_64(0x67),_64(0x68),_64(0x69),_64(0x6a),_64(0x6b),_64(0x6c),_64(0x6d),_64(0x6e),_64(0x6f),
//	_64(0x70),_64(0x71),_64(0x72),_64(0x73),_64(0x74),_64(0x75),_64(0x76),_64(0x77),_64(0x78),_64(0x79),_64(0x7a),_64(0x7b),_64(0x7c),_64(0x7d),_64(0x7e),_64(0x7f),
//	_64(0x80),_64(0x81),_64(0x82),_64(0x83),_64(0x84),_64(0x85),_64(0x86),_64(0x87),_64(0x88),_64(0x89),_64(0x8a),_64(0x8b),_64(0x8c),_64(0x8d),_64(0x8e),_64(0x8f),
//	_64(0x90),_64(0x91),_64(0x92),_64(0x93),_64(0x94),_64(0x95),_64(0x96),_64(0x97),_64(0x98),_64(0x99),_64(0x9a),_64(0x9b),_64(0x9c),_64(0x9d),_64(0x9e),_64(0x9f),
//	_64(0xa0),_64(0xa1),_64(0xa2),_64(0xa3),_64(0xa4),_64(0xa5),_64(0xa6),_64(0xa7),_64(0xa8),_64(0xa9),_64(0xaa),_64(0xab),_64(0xac),_64(0xad),_64(0xae),_64(0xaf),
//	_64(0xb0),_64(0xb1),_64(0xb2),_64(0xb3),_64(0xb4),_64(0xb5),_64(0xb6),_64(0xb7),_64(0xb8),_64(0xb9),_64(0xba),_64(0xbb),_64(0xbc),_64(0xbd),_64(0xbe),_64(0xbf),
//	_64(0xc0),_64(0xc1),_64(0xc2),_64(0xc3),_64(0xc4),_64(0xc5),_64(0xc6),_64(0xc7),_64(0xc8),_64(0xc9),_64(0xca),_64(0xcb),_64(0xcc),_64(0xcd),_64(0xce),_64(0xcf),
//	_64(0xd0),_64(0xd1),_64(0xd2),_64(0xd3),_64(0xd4),_64(0xd5),_64(0xd6),_64(0xd7),_64(0xd8),_64(0xd9),_64(0xda),_64(0xdb),_64(0xdc),_64(0xdd),_64(0xde),_64(0xdf),
//	_64(0xe0),_64(0xe1),_64(0xe2),_64(0xe3),_64(0xe4),_64(0xe5),_64(0xe6),_64(0xe7),_64(0xe8),_64(0xe9),_64(0xea),_64(0xeb),_64(0xec),_64(0xed),_64(0xee),_64(0xef),
//	_64(0xf0),_64(0xf1),_64(0xf2),_64(0xf3),_64(0xf4),_64(0xf5),_64(0xf6),_64(0xf7),_64(0xf8),_64(0xf9),_64(0xfa),_64(0xfb),_64(0xfc),_64(0xfd),_64(0xfe),_64(0xff),
//
//	_64(0x00),_64(0x01),_64(0x02),_64(0x03),_64(0x04),_64(0x05),_64(0x06),_64(0x07),_64(0x08),_64(0x09),_64(0x0a),_64(0x0b),_64(0x0c),_64(0x0d),_64(0x0e),_64(0x0f),
//	_64(0x10),_64(0x11),_64(0x12),_64(0x13),_64(0x14),_64(0x15),_64(0x16),_64(0x17),_64(0x18),_64(0x19),_64(0x1a),_64(0x1b),_64(0x1c),_64(0x1d),_64(0x1e),_64(0x1f),
//	_64(0x20),_64(0x21),_64(0x22),_64(0x23),_64(0x24),_64(0x25),_64(0x26),_64(0x27),_64(0x28),_64(0x29),_64(0x2a),_64(0x2b),_64(0x2c),_64(0x2d),_64(0x2e),_64(0x2f),
//	_64(0x30),_64(0x31),_64(0x32),_64(0x33),_64(0x34),_64(0x35),_64(0x36),_64(0x37),_64(0x38),_64(0x39),_64(0x3a),_64(0x3b),_64(0x3c),_64(0x3d),_64(0x3e),_64(0x3f),
//	_64(0x40),_64(0x41),_64(0x42),_64(0x43),_64(0x44),_64(0x45),_64(0x46),_64(0x47),_64(0x48),_64(0x49),_64(0x4a),_64(0x4b),_64(0x4c),_64(0x4d),_64(0x4e),_64(0x4f),
//	_64(0x50),_64(0x51),_64(0x52),_64(0x53),_64(0x54),_64(0x55),_64(0x56),_64(0x57),_64(0x58),_64(0x59),_64(0x5a),_64(0x5b),_64(0x5c),_64(0x5d),_64(0x5e),_64(0x5f),
//	_64(0x60),_64(0x61),_64(0x62),_64(0x63),_64(0x64),_64(0x65),_64(0x66),_64(0x67),_64(0x68),_64(0x69),_64(0x6a),_64(0x6b),_64(0x6c),_64(0x6d),_64(0x6e),_64(0x6f),
//	_64(0x70),_64(0x71),_64(0x72),_64(0x73),_64(0x74),_64(0x75),_64(0x76),_64(0x77),_64(0x78),_64(0x79),_64(0x7a),_64(0x7b),_64(0x7c),_64(0x7d),_64(0x7e),_64(0x7f),
//	_64(0x80),_64(0x81),_64(0x82),_64(0x83),_64(0x84),_64(0x85),_64(0x86),_64(0x87),_64(0x88),_64(0x89),_64(0x8a),_64(0x8b),_64(0x8c),_64(0x8d),_64(0x8e),_64(0x8f),
//	_64(0x90),_64(0x91),_64(0x92),_64(0x93),_64(0x94),_64(0x95),_64(0x96),_64(0x97),_64(0x98),_64(0x99),_64(0x9a),_64(0x9b),_64(0x9c),_64(0x9d),_64(0x9e),_64(0x9f),
//	_64(0xa0),_64(0xa1),_64(0xa2),_64(0xa3),_64(0xa4),_64(0xa5),_64(0xa6),_64(0xa7),_64(0xa8),_64(0xa9),_64(0xaa),_64(0xab),_64(0xac),_64(0xad),_64(0xae),_64(0xaf),
//	_64(0xb0),_64(0xb1),_64(0xb2),_64(0xb3),_64(0xb4),_64(0xb5),_64(0xb6),_64(0xb7),_64(0xb8),_64(0xb9),_64(0xba),_64(0xbb),_64(0xbc),_64(0xbd),_64(0xbe),_64(0xbf),
//	_64(0xc0),_64(0xc1),_64(0xc2),_64(0xc3),_64(0xc4),_64(0xc5),_64(0xc6),_64(0xc7),_64(0xc8),_64(0xc9),_64(0xca),_64(0xcb),_64(0xcc),_64(0xcd),_64(0xce),_64(0xcf),
//	_64(0xd0),_64(0xd1),_64(0xd2),_64(0xd3),_64(0xd4),_64(0xd5),_64(0xd6),_64(0xd7),_64(0xd8),_64(0xd9),_64(0xda),_64(0xdb),_64(0xdc),_64(0xdd),_64(0xde),_64(0xdf),
//	_64(0xe0),_64(0xe1),_64(0xe2),_64(0xe3),_64(0xe4),_64(0xe5),_64(0xe6),_64(0xe7),_64(0xe8),_64(0xe9),_64(0xea),_64(0xeb),_64(0xec),_64(0xed),_64(0xee),_64(0xef),
//	_64(0xf0),_64(0xf1),_64(0xf2),_64(0xf3),_64(0xf4),_64(0xf5),_64(0xf6),_64(0xf7),_64(0xf8),_64(0xf9),_64(0xfa),_64(0xfb),_64(0xfc),_64(0xfd),_64(0xfe),_64(0xff),
//
//	_64(0x00),_64(0x01),_64(0x02),_64(0x03),_64(0x04),_64(0x05),_64(0x06),_64(0x07),_64(0x08),_64(0x09),_64(0x0a),_64(0x0b),_64(0x0c),_64(0x0d),_64(0x0e),_64(0x0f),
//	_64(0x10),_64(0x11),_64(0x12),_64(0x13),_64(0x14),_64(0x15),_64(0x16),_64(0x17),_64(0x18),_64(0x19),_64(0x1a),_64(0x1b),_64(0x1c),_64(0x1d),_64(0x1e),_64(0x1f),
//	_64(0x20),_64(0x21),_64(0x22),_64(0x23),_64(0x24),_64(0x25),_64(0x26),_64(0x27),_64(0x28),_64(0x29),_64(0x2a),_64(0x2b),_64(0x2c),_64(0x2d),_64(0x2e),_64(0x2f),
//	_64(0x30),_64(0x31),_64(0x32),_64(0x33),_64(0x34),_64(0x35),_64(0x36),_64(0x37),_64(0x38),_64(0x39),_64(0x3a),_64(0x3b),_64(0x3c),_64(0x3d),_64(0x3e),_64(0x3f),
//	_64(0x40),_64(0x41),_64(0x42),_64(0x43),_64(0x44),_64(0x45),_64(0x46),_64(0x47),_64(0x48),_64(0x49),_64(0x4a),_64(0x4b),_64(0x4c),_64(0x4d),_64(0x4e),_64(0x4f),
//	_64(0x50),_64(0x51),_64(0x52),_64(0x53),_64(0x54),_64(0x55),_64(0x56),_64(0x57),_64(0x58),_64(0x59),_64(0x5a),_64(0x5b),_64(0x5c),_64(0x5d),_64(0x5e),_64(0x5f),
//	_64(0x60),_64(0x61),_64(0x62),_64(0x63),_64(0x64),_64(0x65),_64(0x66),_64(0x67),_64(0x68),_64(0x69),_64(0x6a),_64(0x6b),_64(0x6c),_64(0x6d),_64(0x6e),_64(0x6f),
//	_64(0x70),_64(0x71),_64(0x72),_64(0x73),_64(0x74),_64(0x75),_64(0x76),_64(0x77),_64(0x78),_64(0x79),_64(0x7a),_64(0x7b),_64(0x7c),_64(0x7d),_64(0x7e),_64(0x7f),
//	_64(0x80),_64(0x81),_64(0x82),_64(0x83),_64(0x84),_64(0x85),_64(0x86),_64(0x87),_64(0x88),_64(0x89),_64(0x8a),_64(0x8b),_64(0x8c),_64(0x8d),_64(0x8e),_64(0x8f),
//	_64(0x90),_64(0x91),_64(0x92),_64(0x93),_64(0x94),_64(0x95),_64(0x96),_64(0x97),_64(0x98),_64(0x99),_64(0x9a),_64(0x9b),_64(0x9c),_64(0x9d),_64(0x9e),_64(0x9f),
//	_64(0xa0),_64(0xa1),_64(0xa2),_64(0xa3),_64(0xa4),_64(0xa5),_64(0xa6),_64(0xa7),_64(0xa8),_64(0xa9),_64(0xaa),_64(0xab),_64(0xac),_64(0xad),_64(0xae),_64(0xaf),
//	_64(0xb0),_64(0xb1),_64(0xb2),_64(0xb3),_64(0xb4),_64(0xb5),_64(0xb6),_64(0xb7),_64(0xb8),_64(0xb9),_64(0xba),_64(0xbb),_64(0xbc),_64(0xbd),_64(0xbe),_64(0xbf),
//	_64(0xc0),_64(0xc1),_64(0xc2),_64(0xc3),_64(0xc4),_64(0xc5),_64(0xc6),_64(0xc7),_64(0xc8),_64(0xc9),_64(0xca),_64(0xcb),_64(0xcc),_64(0xcd),_64(0xce),_64(0xcf),
//	_64(0xd0),_64(0xd1),_64(0xd2),_64(0xd3),_64(0xd4),_64(0xd5),_64(0xd6),_64(0xd7),_64(0xd8),_64(0xd9),_64(0xda),_64(0xdb),_64(0xdc),_64(0xdd),_64(0xde),_64(0xdf),
//	_64(0xe0),_64(0xe1),_64(0xe2),_64(0xe3),_64(0xe4),_64(0xe5),_64(0xe6),_64(0xe7),_64(0xe8),_64(0xe9),_64(0xea),_64(0xeb),_64(0xec),_64(0xed),_64(0xee),_64(0xef),
//	_64(0xf0),_64(0xf1),_64(0xf2),_64(0xf3),_64(0xf4),_64(0xf5),_64(0xf6),_64(0xf7),_64(0xf8),_64(0xf9),_64(0xfa),_64(0xfb),_64(0xfc),_64(0xfd),_64(0xfe),_64(0xff),
//
//	_64(0x00),_64(0x01),_64(0x02),_64(0x03),_64(0x04),_64(0x05),_64(0x06),_64(0x07),_64(0x08),_64(0x09),_64(0x0a),_64(0x0b),_64(0x0c),_64(0x0d),_64(0x0e),_64(0x0f),
//	_64(0x10),_64(0x11),_64(0x12),_64(0x13),_64(0x14),_64(0x15),_64(0x16),_64(0x17),_64(0x18),_64(0x19),_64(0x1a),_64(0x1b),_64(0x1c),_64(0x1d),_64(0x1e),_64(0x1f),
//	_64(0x20),_64(0x21),_64(0x22),_64(0x23),_64(0x24),_64(0x25),_64(0x26),_64(0x27),_64(0x28),_64(0x29),_64(0x2a),_64(0x2b),_64(0x2c),_64(0x2d),_64(0x2e),_64(0x2f),
//	_64(0x30),_64(0x31),_64(0x32),_64(0x33),_64(0x34),_64(0x35),_64(0x36),_64(0x37),_64(0x38),_64(0x39),_64(0x3a),_64(0x3b),_64(0x3c),_64(0x3d),_64(0x3e),_64(0x3f),
//	_64(0x40),_64(0x41),_64(0x42),_64(0x43),_64(0x44),_64(0x45),_64(0x46),_64(0x47),_64(0x48),_64(0x49),_64(0x4a),_64(0x4b),_64(0x4c),_64(0x4d),_64(0x4e),_64(0x4f),
//	_64(0x50),_64(0x51),_64(0x52),_64(0x53),_64(0x54),_64(0x55),_64(0x56),_64(0x57),_64(0x58),_64(0x59),_64(0x5a),_64(0x5b),_64(0x5c),_64(0x5d),_64(0x5e),_64(0x5f),
//	_64(0x60),_64(0x61),_64(0x62),_64(0x63),_64(0x64),_64(0x65),_64(0x66),_64(0x67),_64(0x68),_64(0x69),_64(0x6a),_64(0x6b),_64(0x6c),_64(0x6d),_64(0x6e),_64(0x6f),
//	_64(0x70),_64(0x71),_64(0x72),_64(0x73),_64(0x74),_64(0x75),_64(0x76),_64(0x77),_64(0x78),_64(0x79),_64(0x7a),_64(0x7b),_64(0x7c),_64(0x7d),_64(0x7e),_64(0x7f),
//	_64(0x80),_64(0x81),_64(0x82),_64(0x83),_64(0x84),_64(0x85),_64(0x86),_64(0x87),_64(0x88),_64(0x89),_64(0x8a),_64(0x8b),_64(0x8c),_64(0x8d),_64(0x8e),_64(0x8f),
//	_64(0x90),_64(0x91),_64(0x92),_64(0x93),_64(0x94),_64(0x95),_64(0x96),_64(0x97),_64(0x98),_64(0x99),_64(0x9a),_64(0x9b),_64(0x9c),_64(0x9d),_64(0x9e),_64(0x9f),
//	_64(0xa0),_64(0xa1),_64(0xa2),_64(0xa3),_64(0xa4),_64(0xa5),_64(0xa6),_64(0xa7),_64(0xa8),_64(0xa9),_64(0xaa),_64(0xab),_64(0xac),_64(0xad),_64(0xae),_64(0xaf),
//	_64(0xb0),_64(0xb1),_64(0xb2),_64(0xb3),_64(0xb4),_64(0xb5),_64(0xb6),_64(0xb7),_64(0xb8),_64(0xb9),_64(0xba),_64(0xbb),_64(0xbc),_64(0xbd),_64(0xbe),_64(0xbf),
//	_64(0xc0),_64(0xc1),_64(0xc2),_64(0xc3),_64(0xc4),_64(0xc5),_64(0xc6),_64(0xc7),_64(0xc8),_64(0xc9),_64(0xca),_64(0xcb),_64(0xcc),_64(0xcd),_64(0xce),_64(0xcf),
//	_64(0xd0),_64(0xd1),_64(0xd2),_64(0xd3),_64(0xd4),_64(0xd5),_64(0xd6),_64(0xd7),_64(0xd8),_64(0xd9),_64(0xda),_64(0xdb),_64(0xdc),_64(0xdd),_64(0xde),_64(0xdf),
//	_64(0xe0),_64(0xe1),_64(0xe2),_64(0xe3),_64(0xe4),_64(0xe5),_64(0xe6),_64(0xe7),_64(0xe8),_64(0xe9),_64(0xea),_64(0xeb),_64(0xec),_64(0xed),_64(0xee),_64(0xef),
//	_64(0xf0),_64(0xf1),_64(0xf2),_64(0xf3),_64(0xf4),_64(0xf5),_64(0xf6),_64(0xf7),_64(0xf8),_64(0xf9),_64(0xfa),_64(0xfb),/*_64(0xfc),_64(0xfd),_64(0xfe),_64(0xff)*/
//
//	_I(0x00),
//	_I(0x01),
//	_I(0x02),
//	_I(0x03),
//	_I(0x04),
//	_I(0x05),
//	_I(0x06),
//	_I(0x07),
//	_I(0x08),
//	_I(0x09),
//	_I(0x0a),
//	_I(0x0b),
//	_I(0x0c),
//	_I(0x0d),
//	_I(0x0e),
//	_I(0x0f),
//	_I(0x10),
//	_I(0x11),
//	_I(0x12),
//	_I(0x13),
//	_I(0x14),
//	_I(0x15),
//	_I(0x16),
//	_I(0x17),
//	_I(0x18),
//	_I(0x19),
//	_I(0x1a),
//	_I(0x1b),
//	_I(0x1c),
//	_I(0x1d),
//	_I(0x1e),
//	_I(0x1f),
//	_I(0x20),
//	_I(0x21),
//	_I(0x22),
//	_I(0x23),
//	_I(0x24),
//	_I(0x25),
//	_I(0x26),
//	_I(0x27),
//	_I(0x28),
//	_I(0x29),
//	_I(0x2a),
//	_I(0x2b),
//	_I(0x2c),
//	_I(0x2d),
//	_I(0x2e),
//	_I(0x2f),
//	_I(0x30),
//	_I(0x31),
//	_I(0x32),
//	_I(0x33),
//	_I(0x34),
//	_I(0x35),
//	_I(0x36),
//	_I(0x37),
//	_I(0x38),
//	_I(0x39),
//	_I(0x3a),
//	_I(0x3b),
//	_I(0x3c),
//	_I(0x3d),
//	_I(0x3e),
//	_I(0x3f),
//	_I(0x40),
//	_I(0x41),
//	_I(0x42),
//	_I(0x43),
//	_I(0x44),
//	_I(0x45),
//	_I(0x46),
//	_I(0x47),
//	_I(0x48),
//	_I(0x49),
//	_I(0x4a),
//	_I(0x4b),
//	_I(0x4c),
//	_I(0x4d),
//	_I(0x4e),
//	_I(0x4f),
//	_I(0x50),
//	_I(0x51),
//	_I(0x52),
//	_I(0x53),
//	_I(0x54),
//	_I(0x55),
//	_I(0x56),
//	_I(0x57),
//	_I(0x58),
//	_I(0x59),
//	_I(0x5a),
//	_I(0x5b),
//	_I(0x5c),
//	_I(0x5d),
//	_I(0x5e),
//	_I(0x5f),
//	_I(0x60),
//	_I(0x61),
//	_I(0x62),
//	_I(0x63),
//	_I(0x64),
//	_I(0x65),
//	_I(0x66),
//	_I(0x67),
//	_I(0x68),
//	_I(0x69),
//	_I(0x6a),
//	_I(0x6b),
//	_I(0x6c),
//	_I(0x6d),
//	_I(0x6e),
//	_I(0x6f),
//	_I(0x70),
//	_I(0x71),
//	_I(0x72),
//	_I(0x73),
//	_I(0x74),
//	_I(0x75),
//	_I(0x76),
//	_I(0x77),
//	_I(0x78),
//	_I(0x79),
//	_I(0x7a),
//	_I(0x7b),
//	_I(0x7c),
//	_I(0x7d),
//	_I(0x7e),
//	_I(0x7f),
//	_I(0x80),
//	_I(0x81),
//	_I(0x82),
//	_I(0x83),
//	_I(0x84),
//	_I(0x85),
//	_I(0x86),
//	_I(0x87),
//	_I(0x88),
//	_I(0x89),
//	_I(0x8a),
//	_I(0x8b),
//	_I(0x8c),
//	_I(0x8d),
//	_I(0x8e),
//	_I(0x8f),
//	_I(0x90),
//	_I(0x91),
//	_I(0x92),
//	_I(0x93),
//	_I(0x94),
//	_I(0x95),
//	_I(0x96),
//	_I(0x97),
//	_I(0x98),
//	_I(0x99),
//	_I(0x9a),
//	_I(0x9b),
//	_I(0x9c),
//	_I(0x9d),
//	_I(0x9e),
//	_I(0x9f),
//	_I(0xa0),
//	_I(0xa1),
//	_I(0xa2),
//	_I(0xa3),
//	_I(0xa4),
//	_I(0xa5),
//	_I(0xa6),
//	_I(0xa7),
//	_I(0xa8),
//	_I(0xa9),
//	_I(0xaa),
//	_I(0xab),
//	_I(0xac),
//	_I(0xad),
//	_I(0xae),
//	_I(0xaf),
//	_I(0xb0),
//	_I(0xb1),
//	_I(0xb2),
//	_I(0xb3),
//	_I(0xb4),
//	_I(0xb5),
//	_I(0xb6),
//	_I(0xb7),
//	_I(0xb8),
//	_I(0xb9),
//	_I(0xba),
//	_I(0xbb),
//	_I(0xbc),
//	_I(0xbd),
//	_I(0xbe),
//	_I(0xbf),
//	_I(0xc0),
//	_I(0xc1),
//	_I(0xc2),
//	_I(0xc3),
//	_I(0xc4),
//	_I(0xc5),
//	_I(0xc6),
//	_I(0xc7),
//	_I(0xc8),
//	_I(0xc9),
//	_I(0xca),
//	_I(0xcb),
//	_I(0xcc),
//	_I(0xcd),
//	_I(0xce),
//	_I(0xcf),
//	_I(0xd0),
//	_I(0xd1),
//	_I(0xd2),
//	_I(0xd3),
//	_I(0xd4),
//	_I(0xd5),
//	_I(0xd6),
//	_I(0xd7),
//	_I(0xd8),
//	_I(0xd9),
//	_I(0xda),
//	_I(0xdb),
//	_I(0xdc),
//	_I(0xdd),
//	_I(0xde),
//	_I(0xdf),
//	_I(0xe0),
//	_I(0xe1),
//	_I(0xe2),
//	_I(0xe3),
//	_I(0xe4),
//	_I(0xe5),
//	_I(0xe6),
//	_I(0xe7),
//	_I(0xe8),
//	_I(0xe9),
//	_I(0xea),
//	_I(0xeb),
//	_I(0xec),
//	_I(0xed),
//	_I(0xee),
//	_I(0xef),
//	_I(0xf0),
//	_I(0xf1),
//	_I(0xf2),
//	_I(0xf3),
//	_I(0xf4),
//	_I(0xf5),
//	_I(0xf6),
//	_I(0xf7),
//	_I(0xf8),
//	_I(0xf9),
//	_I(0xfa),
//	_I(0xfb),
//	_I(0xfc),
//	_I(0xfd),
//	_I(0xfe),
//	_I(0xff),
//};

//static uint16 sBlurBuffer[640*448];

namespace NxNgc
{

	// extern void	test_render(Mth::Matrix* camera_orient, Mth::Vector* camera_pos);
	// extern void	test_init();			

} // namespace NxNgc


namespace Nx
{

//	    // Set camera configuration
//	    sc->light.cam.cfg = ( sc->projMode ) ?
//	                        DefaultLightCamera1 : DefaultLightCamera0;
//	    SetCamera(&sc->light.cam);
//	    
//	    // Light camera
//	    sc->projMode           = 0;
//	    sc->light.cam.theta    = 0;
//	    sc->light.cam.phi      = 60;
//	    sc->light.cam.distance = 3000.0F;
//	
//	
//		static void SetCamera( MyCameraObj* cam )
//		{
//			f32  r_theta, r_phi;
//			
//			r_theta = (f32)cam->theta * PI / 180.0F;
//			r_phi   = (f32)cam->phi   * PI / 180.0F;
//		
//			cam->cfg.location.x =
//				cam->distance * cosf(r_theta) * cosf(r_phi);
//			cam->cfg.location.y =
//				cam->distance * sinf(r_theta) * cosf(r_phi);
//			cam->cfg.location.z =
//				cam->distance * sinf(r_phi);
//		
//			MTXLookAt(
//				cam->view,
//				&cam->cfg.location,
//				&cam->cfg.up,
//				&cam->cfg.target );    
//		
//			if ( cam->cfg.type == GX_PERSPECTIVE )
//			{
//				MTXFrustum(
//					cam->proj,
//					cam->cfg.top,
//					- (cam->cfg.top),
//					cam->cfg.left,
//					- (cam->cfg.left),
//					cam->cfg.znear,
//					cam->cfg.zfar );
//			}
//			else // ( cam->cfg.type == GX_ORTHOGRAPHIC )
//			{
//				MTXOrtho(
//					cam->proj,
//					cam->cfg.top,
//					- (cam->cfg.top),
//					cam->cfg.left,
//					- (cam->cfg.left),
//					cam->cfg.znear,
//					cam->cfg.zfar );
//			}
//			
//			GXSetProjection(cam->proj, cam->cfg.type);
//		}
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_start_engine( void )
{
    uint32	size;

	NxNgc::InitialiseEngine();

	
	mp_particle_manager = new CNgcNewParticleManager;

//	Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
//
//	// Allocate memory for shadow map
	size = GX::GetTexBufferSize( SHADOW_TEXTURE_SIZE, SHADOW_TEXTURE_SIZE, GX_TF_I4, GX_FALSE, 0 );
	Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
    shadowTextureData = new uint8[size];
	Mem::Manager::sHandle().BottomUpHeap()->PopAlign();
//
//	size = GXGetTexBufferSize( 320, 224, GX_TF_Z8, GX_FALSE, 0 );
//	zTextureDataH = new uint8[size];
//	zTextureDataL = new uint8[size];
//
//	size = GXGetTexBufferSize( 320, 224, GX_TF_RGBA8, GX_FALSE, 0 );
//	screenTextureData = new uint8[size];
//
//	size = GXGetTexBufferSize( 320, 224, GX_TF_RGBA8, GX_FALSE, 0 );
//	focusTextureData = new uint8[size];
//
//	size = GXGetTexBufferSize( BLUR_TEXTURE_SIZE, BLUR_TEXTURE_SIZE, GX_TF_I4, GX_FALSE, 0 );
//    blurTextureData = new uint8[size];
//
//	size = GXGetTexBufferSize( 640, 448, GX_TF_A8, GX_FALSE, 0 );
//	volumeTextureData = new uint8[size];
//
//
//	Mem::Manager::sHandle().BottomUpHeap()->PopAlign();

	if( NxNgc::EngineGlobals.use_widescreen )
	{
		Script::RunScript( "screen_setup_widescreen" );
	}
	mp_weather = new CNgcWeather;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_pre_render( void )
{
	//NsDisplay::begin();
	//NsRender::begin();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_post_render( void )
{
	//NsRender::end();
	//NsDisplay::end();
//	D3DDevice_Swap( D3DSWAP_DEFAULT );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_render_world( void )
{
	if ( gLoadingScreenActive ) return;

	NsBuffer::begin();

	g_object = 0;
	g_view_scene = NULL;
//	if ( gPrintMatStats )
//	{
//		gPrintMatStats = false;
//
//		char *blend_text[] =
//		{
//			"DIFFUSE        ",
//			"ADD            ",
//			"ADD_FIXED      ",
//			"SUBTRACT       ",
//			"SUB_FIXED      ",
//			"BLEND          ",
//			"BLEND_FIXED    ",
//			"MODULATE       ",
//			"MODULATE_FIXED ",
//			"BRIGHTEN       ",
//			"BRIGHTEN_FIXED ",
//		};
//
//		OSReport( "Unique blend modes: %d\n\n", num_u_mat );
//
//		for ( int um = 0; um < num_u_mat; um++ )
//		{
//			OSReport( "%d: (%4d) ", p_u_mat[um]->m_passes, u_mat_count[um] );
//
//			NxNgc::sMaterialPassHeader * p_u_pass = (NxNgc::sMaterialPassHeader *)&p_u_mat[um][1];
//			for ( int p = 0; p < p_u_mat[um]->m_passes; p++ )
//			{
//				if ( p_u_pass[p].m_blend_mode <= 10 )
//				{
//					OSReport( "%s", blend_text[p_u_pass[p].m_blend_mode] );
//				}
//				else
//				{
//					OSReport( "<unknown>      " );
//				}
//			}
//			OSReport( "\n" );
//		}
//		
//	}
	
	NsCamera cam;
	Mtx light;
	MTXIdentity( light );

	NxNgc::EngineGlobals.gpuBusy = true;

#ifndef __NOPT_FINAL__
	if ( gDumpHeap )
	{
		Mem::Manager& mem_man = Mem::Manager::sHandle();
	
		OSReport ("MEM CONTEXT: %s\n",Mem::Manager::sHandle().GetContextName());
		OSReport("Name            Used  Frag  Free   Blocks\n");
		OSReport("--------------- ----- ----- ------ ------\n");
		Mem::Heap* heap;
		for (heap = mem_man.FirstHeap(); heap != NULL; heap = mem_man.NextHeap(heap))
		{		
				Mem::Region* region = heap->ParentRegion();			
				OSReport( "%12s: %6dK %6dK %6dK   %6d \n",
						heap->GetName(),
						heap->mUsedMem.m_count / 1024,
						heap->mFreeMem.m_count / 1024,
						region->MemAvailable() / 1024,
						heap->mUsedBlocks.m_count
						);										
		}
		gDumpHeap = false;
	}
#endif		// __NOPT_FINAL__

//	NsPrim::end();
//	NsRender::end();
//
////	if ( movies )
////	{
////		NsDisplay::end( false );
////	}
////	else
////	{
//		NsDisplay::end( true );
////	}


	NxNgc::process_instances();

	NsDisplay::begin();
//	NsDisplay::end();
//	NsDisplay::begin();


//	// Create color map if necessary.
//	if ( !color_map_created )
//	{
//		color_map_created = true;
//
//		for ( int y = 0; y < COLOR_MAP_SIZE; y++ )
//		{
//			for ( int x = 0; x < COLOR_MAP_SIZE; x++ )
//			{
//				int r;
//				int g;
//
//				r = 0;
//				r += ( x / 4 ) * 32;
//				r += ( x & 3 );
//				r += ( ( y / 4 ) * ( COLOR_MAP_SIZE * 4 * 2 ) );
//				r += ( ( y & 3 ) * 4 );
//
//				g = 16;
//				g += ( x / 4 ) * 32;
//				g += ( x & 3 );
//				g += ( ( y / 4 ) * ( COLOR_MAP_SIZE * 4 * 2 ) );
//				g += ( ( y & 3 ) * 4 );
//
//				// Red across.
//				colorMap[r] = ( x * 256 ) / COLOR_MAP_SIZE;
//				// Green down.
//				colorMap[g] = ( ( ( y * 256 ) / COLOR_MAP_SIZE ) << 8 );
//			}
//		}
//	}




	// Clear to black with alpha of 124.
	// This is important as the shadow volume stuff uses the alpha channel as a stencil buffer.
	// With destination alpha, the RGBA channels each have 6 bits, so they are only accurate to multiples of 4.
	// To save having to copy the alpha map to a texture, we do a 2-pass full-screen polygon.
	// 1. And the alpha channel with 128. As it was set to 124, anything positive will have bit (128) set.
	// 2. Blend alpha with framebuffer.
//	NsDisplay::setBackgroundColor( (GXColor){0,0,0,124} );
	NsDisplay::setBackgroundColor( (GXColor){0,0,0,255} );
	NsRender::begin();
	NsPrim::begin();

//	cam.orthographic( 0, 0, 640, 448 );
//	cam.begin();
//
//	GX::SetNumTevStages(1);
//	GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
//	GX::SetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetNumTexGens( 0 );
//	GX::SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//	GX::SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//	GX::SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
//	GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_FALSE, GX_TRUE, GX_FALSE, GX_FALSE );
//	GX::SetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA );
//	GX::SetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC );
//	GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//	GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//	GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//
//	NsPrim::quad(   0.0f,   0.0f, -9999.0f,
//				  640.0f,   0.0f, -9999.0f,
//				  640.0f, 448.0f, -9999.0f,
//				    0.0f, 448.0f, -9999.0f,
//				  (GXColor){128,128,128,0} );
//
//	cam.end();

//	bool movies = Flx::Movie_Render();

	//--------------------------------------------------------------
	//--------------------------------------------------------------
	//--------------------------------------------------------------

#	ifdef __USE_PROFILER__
	Sys::VUProfiler->PushContext( 128,128,0 );
#	endif		// __USE_PROFILER__
	if( sp_loaded_scenes[0] != NULL )
	{
		GX::SetZMode ( GX_FALSE, GX_ALWAYS, GX_FALSE );
	



	//	// 8bit mode
	//	shCpFmt = GX_TF_Z8;
	//	shFmt   = GX_TF_I8;
	//
		//-------------------------------------------
		//  1st. pass
		//  Make an image viewed from the light
		//-------------------------------------------
	
		// Color update is disabled. Only Z will be updated.
	//		GXSetColorUpdate(GX_DISABLE);
	
		// To draw "second" surfaces from the light
		//GX::SetCullMode(GX_CULL_FRONT);
	
		// Set viewport for making shadow texture
	//		GXSetViewport(0, 0, SHADOW_TEXTURE_SIZE, SHADOW_TEXTURE_SIZE, 0.0F, 1.0F);
	//		GXSetScissor(0, 0, (u32)SHADOW_TEXTURE_SIZE, (u32)SHADOW_TEXTURE_SIZE);
	
		// Set up the camera..
		//NxNgc::set_camera( &( cur_camera->GetMatrix()), &( cur_camera->GetPos()), cur_camera->GetAdjustedHFOV(), p_cur_viewport->GetAspectRatio());
	
		// Set render mode to use only constant color
		// because we need only depth buffer
	//		GXSetNumChans(1);
		GX::SetChanCtrl(
			GX_COLOR0A0,
			GX_DISABLE,    // disable channel
			GX_SRC_REG,    // amb source
			GX_SRC_REG,    // mat source
			GX_LIGHT_NULL, // light mask
			GX_DF_CLAMP,   // diffuse function
			GX_AF_NONE );
	
		GX::SetTexChanTevIndCull( 0, 1, 1, 0, GX_CULL_NONE );
		GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA,
											   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV,
											   GX_TEV_SWAP0, GX_TEV_SWAP0 );
		GX::SetTevColorInOp( GX_TEVSTAGE0,	   GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC,
											   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV );
		GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR0A0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);
	
	
		// Draw models
	//		MTXCopy(sc->light.cam.view, tmo.view);
	//		DrawModels(&tmo, sc->modelRot);
	//	
//		NxNgc::render_instances( true );
		NxNgc::EngineGlobals.poly_culling = false;
		NxNgc::render_shadow_targets();
	
	






		// Draw line around border.
		cam.orthographic( 0, 0, 640, 448 );
		cam.begin();

		GX::SetPointSize( 6, GX_TO_ONE );
		GX::SetTexChanTevIndCull( 0, 1, 1, 0, GX_CULL_NONE );
		GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
		GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );
		GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR0A0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);

		GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA,
											   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV,
											   GX_TEV_SWAP0, GX_TEV_SWAP0 );

		GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
										   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
		GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
		GX::SetChanCtrl( GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
		GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
		GX::SetChanMatColor( GX_COLOR0A0, (GXColor){0,0,0,255} );

		GX::SetVtxDesc( 1, GX_VA_POS, GX_DIRECT );

		for ( int vvv = 0; vvv < 8; vvv++ )
		{
			float min = -1.0f + ( (float)vvv / 4.0f );
			float max = (float)SHADOW_TEXTURE_SIZE - ( (float)vvv / 4.0f );
			GX::Begin( GX_LINESTRIP, GX_VTXFMT0, 5 ); 
				GX::Position3f32(min, min, -1.0f);
				GX::Position3f32(max, min, -1.0f);
				GX::Position3f32(max, max, -1.0f);
				GX::Position3f32(min, max, -1.0f);
				GX::Position3f32(min, min, -1.0f);
			GX::End();
		}
		cam.end();









	//		// Copy shadow image into texture
	//		GXSetTexCopySrc(0, 0, SHADOW_TEXTURE_SIZE, SHADOW_TEXTURE_SIZE);
	//		GXSetTexCopyDst(SHADOW_TEXTURE_SIZE, SHADOW_TEXTURE_SIZE, GX_TF_Z16, GX_FALSE);
	//		GXCopyTex(zTextureData, GX_FALSE);
	//
	//		// Wait for finishing the copy task in the graphics pipeline
	//		GXPixModeSync();
	
//		// Draw line around border.
//
//		cam.orthographic( 0, 0, 640, 448 );
//		cam.begin();
//
//		GX::SetTexChanTevIndCull( 0, 1, 1, 0, GX_CULL_NONE );
//		GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA,
//											   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV,
//											   GX_TEV_SWAP0, GX_TEV_SWAP0 );
//		GX::SetTevColorInOp( GX_TEVSTAGE0,	   GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC,
//											   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//		GX::SetTevOrder( GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR0A0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL );
//
//		GX::SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
//		GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_FALSE );
//		GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//		GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){128,128,128,255} );
//		GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//	
//		GX::SetVtxDesc( 1, GX_VA_POS, GX_DIRECT );
//	
//		for ( int vvv = 0; vvv < 8; vvv++ )
//		{
//			float min = -1.0f + ( (float)vvv / 4.0f );
//			float max = (float)SHADOW_TEXTURE_SIZE - ( (float)vvv / 4.0f );
//			GX::Begin( GX_LINESTRIP, GX_VTXFMT0, 5 ); 
//				GX::Position3f32(min, min, -1.0f);
//				GX::Position3f32(max, min, -1.0f);
//				GX::Position3f32(max, max, -1.0f);
//				GX::Position3f32(min, max, -1.0f);
//				GX::Position3f32(min, min, -1.0f);
//			GX::End();
//		}


		// Copy shadow image into texture
		GX::SetTexCopySrc(0, 0, SHADOW_TEXTURE_SIZE, SHADOW_TEXTURE_SIZE);
		GX::SetTexCopyDst(SHADOW_TEXTURE_SIZE, SHADOW_TEXTURE_SIZE, GX_CTF_R4, GX_FALSE);
		GX::CopyTex(shadowTextureData, GX_FALSE);
	
		// Wait for finishing the copy task in the graphics pipeline
		GX::PixModeSync();
	
		// Enable color update
	//		GXSetColorUpdate(GX_ENABLE);
	
		// Restore culling mode to normal
		//GX::SetCullMode(GX_CULL_BACK);
	
	
	
	
	
	
	
	
	
//		//--------------------------------------------------------------
//		//--------------------------------------------------------------
//		//--------------------------------------------------------------
//	
//		// Horrible hack - this should be somewhere else ASAP.
//	
//	
//		GX::UploadTexture( shadowTextureData,
//						   SHADOW_TEXTURE_SIZE,
//						   SHADOW_TEXTURE_SIZE,
//						   GX_TF_C4,
//						   GX_CLAMP,
//						   GX_CLAMP,
//						   GX_FALSE,
//						   GX_LINEAR,
//						   GX_LINEAR,
//						   0.0f,
//						   0.0f,
//						   0.0f,
//						   GX_FALSE,
//						   GX_FALSE,
//						   GX_ANISO_1,
//						   GX_TEXMAP0 );
//
//		GX::SetTexCoordScale( GX_TEXCOORD0, GX_TRUE, SHADOW_TEXTURE_SIZE, SHADOW_TEXTURE_SIZE );
//
//		GX::UploadPalette( &shadowPalette,
//						   GX_TL_RGB5A3,
//						   GX_TLUT_16,
//						   GX_TEXMAP0 ); 
//
//
//		GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
//		GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );
//	
//		GX::SetTexChanTevIndCull( 1, 1, 1, 0, GX_CULL_NONE );
//		GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);
//
//		GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO,
//								 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV,
//								 GX_TEV_SWAP0, GX_TEV_SWAP0 );
//		GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO,
//							 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//
//		GX::SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
//		GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_FALSE );
//		GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//		GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){128,128,128,255} );
//		GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//	
//		Mtx mv;
//		MTXIdentity( mv );
//		GX::LoadTexMtxImm(mv, GX_TEXMTX0, GX_MTX3x4);
//	
//		// Set current vertex descriptor to enable position and color0.
//		// Both use 8b index to access their data arrays.
//		GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_TEX0, GX_DIRECT );
//	
//		// Send coordinates.
//	//		GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
//	//			GXPosition3f32(-BLUR_BORDER, -BLUR_BORDER, -1.0f);
//	//			GXTexCoord2f32(0.0f, 0.0f);
//	//			GXPosition3f32(BLUR_TEXTURE_SIZE+BLUR_BORDER, -BLUR_BORDER, -1.0f);
//	//			GXTexCoord2f32(1.0f, 0.0f);
//	//			GXPosition3f32(BLUR_TEXTURE_SIZE+BLUR_BORDER, BLUR_TEXTURE_SIZE+BLUR_BORDER, -1.0f);
//	//			GXTexCoord2f32(1.0f, 1.0f);
//	//			GXPosition3f32(-BLUR_BORDER, BLUR_TEXTURE_SIZE+BLUR_BORDER, -1.0f);
//	//			GXTexCoord2f32(0.0f, 1.0f);
//	//		GXEnd();
//	
//	
//	
//		GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
//			GX::Position3f32(0.0f, 0.0f, -1.0f);
//			GX::TexCoord2f32(BLUR_0, BLUR_0);
//			GX::Position3f32(BLUR_TEXTURE_SIZE, 0.0f, -1.0f);
//			GX::TexCoord2f32(BLUR_1, BLUR_0);
//			GX::Position3f32(BLUR_TEXTURE_SIZE, BLUR_TEXTURE_SIZE, -1.0f);
//			GX::TexCoord2f32(BLUR_1, BLUR_1);
//			GX::Position3f32(0.0f, BLUR_TEXTURE_SIZE, -1.0f);
//			GX::TexCoord2f32(BLUR_0, BLUR_1);
//		GX::End();
//	
//		// Draw line around blur texture.
//		GX::SetTexChanTevIndCull( 0, 1, 1, 0, GX_CULL_NONE );
//		GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR0A0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);
//
//		GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA,
//								 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV,
//								 GX_TEV_SWAP0, GX_TEV_SWAP0 );
//
//		GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC,
//							 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//
//		GX::SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
//		GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_FALSE );
//		GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//		GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){128,128,128,255} );
//		GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//	
//		GX::SetVtxDesc( 1, GX_VA_POS, GX_DIRECT );
//	
//		GX::Begin( GX_LINESTRIP, GX_VTXFMT0, 5 ); 
//			GX::Position3f32(0.0f, 0.0f, -1.0f);
//			GX::Position3f32(BLUR_TEXTURE_SIZE-1, 0.0f, -1.0f);
//			GX::Position3f32(BLUR_TEXTURE_SIZE-1, BLUR_TEXTURE_SIZE-1, -1.0f);
//			GX::Position3f32(0.0f, BLUR_TEXTURE_SIZE-1, -1.0f);
//			GX::Position3f32(0.0f, 0.0f, -1.0f);
//		GX::End();
//
//		// Copy blur image into texture
//		GX::SetTexCopySrc(0, 0, BLUR_TEXTURE_SIZE, BLUR_TEXTURE_SIZE);
//		GX::SetTexCopyDst(BLUR_TEXTURE_SIZE, BLUR_TEXTURE_SIZE, GX_CTF_R4, GX_FALSE);
//		GX::CopyTex(blurTextureData, GX_TRUE);
//	
//		// Wait for finishing the copy task in the graphics pipeline
//		GX::PixModeSync();
//	
//	
//	//		// Clear screen to black.
//	//		GX::SetNumTevStages(1);
//	//		GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
//	//		GX::SetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	//		GX::SetNumTexGens( 0 );
//	//		GX::SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//	//		GX::SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//	//		GX::SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
//	//		GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_FALSE );
//	//		GX::SetTevKColorSel( GX_TEVSTAGE0, GX_TEV_KCSEL_K0 );
//	//		GX::SetTevKAlphaSel( GX_TEVSTAGE0, GX_TEV_KASEL_K0_A );
//	//		GX::SetTevKColor( GX_KCOLOR0, (GXColor){0,0,0,255} );
//	//		GX::SetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST );
//	//		GX::SetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_KONST );
//	//
//	//		GXSetVtxDesc( 1, GX_VA_POS, GX_DIRECT );
//	//
//	//		// Send coordinates.
//	//		GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
//	//			GXPosition3f32(0.0f,   0.0f,   -50000.0f);
//	//			GXPosition3f32(640.0f, 0.0f,   -50000.0f);
//	//			GXPosition3f32(640.0f, 480.0f, -50000.0f);
//	//			GXPosition3f32(0.0f,   480.0f, -50000.0f);
//	//		GXEnd();
//	
//		cam.end();

		// Clear to bg color.
		cam.orthographic( 0, 0, 640, 448 );
		cam.begin();

		GX::SetTexChanTevIndCull( 0, 1, 1, 0, GX_CULL_NONE );
		GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
		GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );
		GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR0A0, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);

		GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA,
											   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV,
											   GX_TEV_SWAP0, GX_TEV_SWAP0 );

		GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
										   GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
		GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );
		GX::SetChanCtrl( GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
		GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){255,255,255,255} );

		GX::SetVtxDesc( 1, GX_VA_POS, GX_DIRECT );

		if ( g_in_cutscene )
		{
			// Bars must be black.
			GX::SetChanMatColor( GX_COLOR0A0, (GXColor){0,0,0,255} );
			GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
				GX::Position3f32( -4.0f, -4.0f, -1.0f );
				GX::Position3f32( -4.0f+648.0f, -4.0f, -1.0f );
				GX::Position3f32( -4.0f+648.0f, -4.0f+488.0f, -1.0f );
				GX::Position3f32( -4.0f, -4.0f+488.0f, -1.0f );
			GX::End();

			GX::SetChanMatColor( GX_COLOR0A0, (GXColor){0x50,0x60,0x70,255} );
			GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
				GX::Position3f32( -4.0f, 56.0f, -1.0f );
				GX::Position3f32( -4.0f+648.0f, 56.0f, -1.0f );
				GX::Position3f32( -4.0f+648.0f, 336.0f, -1.0f );
				GX::Position3f32( -4.0f, 336.0f, -1.0f );
			GX::End();
		}
		else
		{
			// Set to sky color.
			GX::SetChanMatColor( GX_COLOR0A0, (GXColor){0x50,0x60,0x70,255} );
			GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
				GX::Position3f32( -4.0f, -4.0f, -1.0f );
				GX::Position3f32( -4.0f+648.0f, -4.0f, -1.0f );
				GX::Position3f32( -4.0f+648.0f, -4.0f+488.0f, -1.0f );
				GX::Position3f32( -4.0f, -4.0f+488.0f, -1.0f );
			GX::End();
		}
		cam.end();
	}
#	ifdef __USE_PROFILER__
	Sys::VUProfiler->PopContext();
#	endif		// __USE_PROFILER__

	//--------------------------------------------------------------
	//--------------------------------------------------------------
	//--------------------------------------------------------------
	int rendered = 0;
	int considered = 0;
	meshes_considered = 0;

	Nx::CFog::sFogUpdate();

	// Note: this method of setting up the camera must change
	// so that the p_nx module does not reference things higher up the hierarchy.
	int num_viewports = CViewportManager::sGetNumActiveViewports();
	for( int v = 0; v < num_viewports; ++v )
	{
		NxNgc::render_begin();

		NxNgc::EngineGlobals.viewport = ( 1 << v );
		CViewport *p_cur_viewport = CViewportManager::sGetActiveViewport( v );
		Gfx::Camera *cur_camera = NULL;
		if ( p_cur_viewport ) cur_camera = p_cur_viewport->GetCamera();

#	ifdef __USE_PROFILER__
		Sys::VUProfiler->PushContext( 0,128,0 );
#	endif		// __USE_PROFILER__
		if( cur_camera && ( sp_loaded_scenes[0] != NULL ))
		{
//			NxNgc::EngineGlobals.viewport.X		= (DWORD)( p_cur_viewport->GetOriginX() * NxNgc::EngineGlobals.backbuffer_width );
//			NxNgc::EngineGlobals.viewport.Y		= (DWORD)( p_cur_viewport->GetOriginY() * NxNgc::EngineGlobals.backbuffer_height );
//			NxNgc::EngineGlobals.viewport.Width	= (DWORD)( p_cur_viewport->GetWidth() * NxNgc::EngineGlobals.backbuffer_width );
//			NxNgc::EngineGlobals.viewport.Height	= (DWORD)( p_cur_viewport->GetHeight() * NxNgc::EngineGlobals.backbuffer_height );
//			NxNgc::EngineGlobals.viewport.MinZ		= 0.0f;
//			NxNgc::EngineGlobals.viewport.MaxZ		= 1.0f;
//			D3DDevice_SetViewport( &NxNgc::EngineGlobals.viewport );

			float vx = p_cur_viewport->GetOriginX() * 640.0f;
			float vy = p_cur_viewport->GetOriginY() * 448.0f;
			float vw = p_cur_viewport->GetWidth() * 640.0f;
			float vh = p_cur_viewport->GetHeight() * 448.0f;
			
			if( NxNgc::EngineGlobals.letterbox_active )
			{
				vy += 448.0f / 8.0f;
				vh -= 448.0f / 4.0f;
			}

            GX::SetViewport( vx, vy, vw, vh, 0.0f, 1.0f );
            GX::SetScissor( (u32)vx, (u32)vy, (u32)vw, (u32)vh );

			NxNgc::set_camera( &( cur_camera->GetMatrix()), &( cur_camera->GetPos()), cur_camera->GetAdjustedHFOV(), p_cur_viewport->GetAspectRatio());

			// There is no bounding box transform for rendering the world.
	//		NxNgc::set_frustum_bbox_transform( NULL );

			// Render objects of interest for the render target (shadow objects).
	//		NxNgc::render_shadow_targets();

			// Render the world. At this stage, non-sky scenes are rendered using just the opaque sections.

			GX::LoadNrmMtxImm( light, GX_PNMTX0);
			for( int i = 0; i < MAX_LOADED_SCENES; i++ )
			{
				if( sp_loaded_scenes[i] )
				{
					CNgcScene *pNgcScene = static_cast<CNgcScene *>( sp_loaded_scenes[i] );

					// If this is a sky scene, disable z buffer reads and writes, otherwise enable them.
					if( pNgcScene->IsSky())
					{
						NxNgc::EngineGlobals.poly_culling = false;
						// Set up the camera.
						Mth::Vector	centre_pos( 0.0f, 0.0f, 0.0f );
						NxNgc::set_camera( &( cur_camera->GetMatrix()), &centre_pos, cur_camera->GetAdjustedHFOV(), p_cur_viewport->GetAspectRatio(), ( 30000.0f / pNgcScene->GetEngineScene()->m_sphere[W] ) );
	
						GX::SetZMode ( GX_FALSE, GX_ALWAYS, GX_FALSE );
	
						uint32 flags = ( NxNgc::vRENDER_OPAQUE | NxNgc::vRENDER_SEMITRANSPARENT | NxNgc::vRENDER_LIT );

						NxNgc::make_scene_visible( pNgcScene->GetEngineScene() );
						NxNgc::render_scene( pNgcScene->GetEngineScene(), NULL, flags );
					}
					else
					{
						NxNgc::EngineGlobals.poly_culling = true;
						// Set up the camera..
						NxNgc::set_camera( &( cur_camera->GetMatrix()), &( cur_camera->GetPos()), cur_camera->GetAdjustedHFOV(), p_cur_viewport->GetAspectRatio());

						// Build relevant occlusion poly list, now that the camera is set.
						NxNgc::BuildOccluders( &( cur_camera->GetPos()), v);

						// Build relevant occlusion poly list, now that the camera is set.
						GX::SetZMode ( GX_TRUE, GX_LEQUAL, GX_TRUE );

						// Flag this scene as receiving shadows.
						pNgcScene->GetEngineScene()->m_flags |= SCENE_FLAG_RECEIVE_SHADOWS;

						uint32 flags = ( NxNgc::vRENDER_OPAQUE | NxNgc::vRENDER_OCCLUDED | NxNgc::vRENDER_LIT );

						gCorrectColor = true;
						rendered = NxNgc::cull_scene( pNgcScene->GetEngineScene(), NxNgc::vRENDER_OCCLUDED );
						NxNgc::render_scene( pNgcScene->GetEngineScene(), NULL, flags );
						considered = meshes_considered;
						gCorrectColor = false;
					}
				}
			}
		}

#	ifdef __USE_PROFILER__
		Sys::VUProfiler->PopContext();
#	endif		// __USE_PROFILER__

//		// Process data for instances. Specifically, transform the verts.
//		Sys::VUProfiler->PushContext( 0,128,128 );
//		NxNgc::process_instances();
//		Sys::VUProfiler->PopContext();

		// Render all opaque instances.
#	ifdef __USE_PROFILER__
		Sys::VUProfiler->PushContext( 128,0,0 );
#	endif		// __USE_PROFILER__
		GX::SetZMode ( GX_TRUE, GX_LEQUAL, GX_TRUE );
		NxNgc::render_instances( NxNgc::vRENDER_OPAQUE | NxNgc::vRENDER_TRANSFORM );
		NxNgc::render_instances( NxNgc::vRENDER_SEMITRANSPARENT | NxNgc::vRENDER_INSTANCE_PRE_WORLD_SEMITRANSPARENT );
#	ifdef __USE_PROFILER__
		Sys::VUProfiler->PopContext();
#	endif		// __USE_PROFILER__

#	ifdef __USE_PROFILER__
		Sys::VUProfiler->PushContext( 0,128,0 );
#	endif		// __USE_PROFILER__

		// Render all the non-sky semitransparent scene geometry.
		NxNgc::EngineGlobals.poly_culling = true;
		if( cur_camera && ( sp_loaded_scenes[0] != NULL ))
		{
			// There is no bounding box transform for rendering the world.
	//		NxNgc::set_frustum_bbox_transform( NULL );

			GX::LoadNrmMtxImm( light, GX_PNMTX0);
			for( int i = 0; i < MAX_LOADED_SCENES; i++ )
			{
				if( sp_loaded_scenes[i] )
				{
					CNgcScene *pNgcScene = static_cast<CNgcScene *>( sp_loaded_scenes[i] );

					// Only interested in non-sky scenes, this time round.
					if( !pNgcScene->IsSky())
					{
						// Set up the camera..
						NxNgc::set_camera( &( cur_camera->GetMatrix()), &( cur_camera->GetPos()), cur_camera->GetAdjustedHFOV(), p_cur_viewport->GetAspectRatio());

						// Build relevant occlusion poly list, now that the camera is set.
						GX::SetZMode ( GX_TRUE, GX_LEQUAL, GX_TRUE );


						gCorrectColor = true;
						//rendered += NxNgc::cull_scene( pNgcScene->GetEngineScene(), NxNgc::vRENDER_OCCLUDED );
						uint32 flags;
						flags = ( NxNgc::vRENDER_SEMITRANSPARENT | NxNgc::vRENDER_OCCLUDED | NxNgc::vRENDER_LIT );
						NxNgc::render_scene( pNgcScene->GetEngineScene(), NULL, flags );
						flags = ( NxNgc::vRENDER_SHADOW_2ND_PASS );
						NxNgc::render_scene( pNgcScene->GetEngineScene(), NULL, flags );
						//considered += meshes_considered;
						gCorrectColor = false;
						//OSReport( "Meshes rendered: %d of %d\n", rendered, considered );
//					}
//					else
//					{
//						// Draw sky after opaque world polys.
//						// Set up the camera.
//						Mth::Vector	centre_pos( 0.0f, 0.0f, 0.0f );
//						NxNgc::set_camera( &( cur_camera->GetMatrix()), &centre_pos, cur_camera->GetAdjustedHFOV(), p_cur_viewport->GetAspectRatio(), 30.0f);
//
//						GX::SetZMode ( GX_TRUE, GX_LEQUAL, GX_FALSE );
//
//						gCorrectColor = true;
//						NxNgc::render_scene( pNgcScene->GetEngineScene(), NULL );
//						gCorrectColor = false;
					}
				}
			}
		}

		GX::SetZMode ( GX_TRUE, GX_LEQUAL, GX_TRUE );
		NxNgc::render_instances( NxNgc::vRENDER_SEMITRANSPARENT | NxNgc::vRENDER_INSTANCE_POST_WORLD_SEMITRANSPARENT );

#	ifdef __USE_PROFILER__
		Sys::VUProfiler->PopContext();
#	endif		// __USE_PROFILER__

		NxNgc::render_end();

		// Render other stuff.
		if( cur_camera && ( sp_loaded_scenes[0] != NULL ))
		{
			GX::SetFog( GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, (GXColor){0,0,0,0} );		// Turn fog off.
			NxNgc::set_camera( &( cur_camera->GetMatrix()), &( cur_camera->GetPos()), cur_camera->GetAdjustedHFOV(), p_cur_viewport->GetAspectRatio());
		
			GX::SetZMode ( GX_TRUE, GX_LEQUAL, GX_FALSE );
			GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_VTX, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
			GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
			GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
			render_particles();
			mp_weather->Process( Tmr::FrameLength() );
			mp_weather->Render();

			// New style particles. Update should probably be somewhere else.
			Nx::CFog::sFogUpdate();		// Restore fog to its' former glory.
			NxNgc::set_camera( &( cur_camera->GetMatrix()), &( cur_camera->GetPos()), cur_camera->GetAdjustedHFOV(), p_cur_viewport->GetAspectRatio());
			mp_particle_manager->UpdateParticles();
			mp_particle_manager->RenderParticles();

			GX::SetZMode ( GX_TRUE, GX_LEQUAL, GX_TRUE );
			Nx::ShatterRender();
			Nx::TextureSplatRender();
			GX::SetZMode ( GX_TRUE, GX_LEQUAL, GX_FALSE );
		}

//		Gfx::DebugGfx_Draw();

//		// Render all semi-transparent instances.
//		GX::SetZMode ( GX_TRUE, GX_LEQUAL, GX_TRUE );
//#	ifdef __USE_PROFILER__
//		Sys::VUProfiler->PushContext( 128,0,0 );
//#	endif		// __USE_PROFILER__
//		NxNgc::render_instances( NxNgc::vRENDER_SHADOW_1ST_PASS );
//#	ifdef __USE_PROFILER__
//		Sys::VUProfiler->PopContext();
//#	endif		// __USE_PROFILER__
	
	}

	GX::SetViewport( 0.0f, 0.0f, 640.0f, 448.0f, 0.0f, 1.0f );
	GX::SetScissor( 0, 0, 640, 448 );

	// Horrible hack - this should be somewhere else ASAP.
	GX::SetZMode ( GX_FALSE, GX_ALWAYS, GX_FALSE );

	cam.orthographic( 0, 0, 640, 448 );
	cam.begin();










#ifndef __NOPT_FINAL__

	// 2nd pad controls texture display.
	static int tick;
	static unsigned short last;
	unsigned short current = padData[1].button;
	unsigned short press = ( current ^ last ) & current;
	last = current;

	if ( press & ( PAD_BUTTON_UP | PAD_BUTTON_DOWN | PAD_BUTTON_X | PAD_BUTTON_Y ) )
	{
		tick = 0;
	}


	if ( current & ( PAD_BUTTON_UP | PAD_BUTTON_DOWN | PAD_BUTTON_X | PAD_BUTTON_Y ) )
	{
		if ( tick == 20 )
		{
			press |= current;
		}
		if ( ( ( ( tick - 20 ) % 2 ) == 0 ) && ( tick > 20 ) )
		{
			press |= current;
		}

		tick++;
	}

	if ( press & PAD_BUTTON_UP ) g_scroll_material++;
	if ( press & PAD_BUTTON_DOWN ) g_scroll_material--;
	if ( ( press & PAD_BUTTON_UP ) && ( current & PAD_BUTTON_B ) ) g_scroll_material+=99;
	if ( ( press & PAD_BUTTON_DOWN ) && ( current & PAD_BUTTON_B ) ) g_scroll_material-=99;
	if ( g_view_scene )
	{
		if ( g_scroll_material < 0 ) g_scroll_material = 0;
		if ( g_scroll_material > ( g_view_scene->mp_scene_data->m_num_materials - 1 ) ) g_scroll_material = g_view_scene->mp_scene_data->m_num_materials - 1;
	}

	if ( press & PAD_BUTTON_X ) g_view_object++;
	if ( press & PAD_BUTTON_Y ) g_view_object--;
	if ( g_view_object < 0 ) g_view_object = 0;
	if ( g_view_object > g_object ) g_view_object = g_object;

	if ( press & PAD_BUTTON_A )
	{
		if ( g_material == -1 )
		{
			g_material = g_scroll_material;
		}
		else
		{
			g_material = -1;
		}
	}
	if ( g_material != -1 )
	{
		g_material = g_scroll_material;
	}

	if ( press & PAD_TRIGGER_Z )
	{
		g_dl ^= 1;
	}

	if ( press & PAD_TRIGGER_R ) g_mip++;
	if ( g_mip > 5 ) g_mip = 0;

	if ( press & PAD_TRIGGER_L ) g_passes++;
	if ( g_passes > 3 ) g_passes = -1;

	if ( !g_view_scene )
	{
		// Draw Object number & "NO SCENE" info.
		{
			NxNgc::SText message;
			Nx::CFont * p_cfont;
			const char * p_font_name = "testtitle";
			char p_text[128];

			// We can only draw a text string if we have a font & a string.
			p_cfont = Nx::CFontManager::sGetFont( p_font_name );
			if ( !p_cfont )
			{
				Nx::CFontManager::sLoadFont( p_font_name );
				p_cfont = Nx::CFontManager::sGetFont( p_font_name );
			}
			message.mp_string = p_text;
			message.m_rgba = 0x808080ff;

			Nx::CNgcFont * p_nfont = static_cast<Nx::CNgcFont*>( p_cfont );
			NxNgc::SFont * p_font = p_nfont->GetEngineFont();
			message.mp_font = p_font;
			message.m_xscale = 0.75f;
			message.m_yscale = 1.2f;

			message.m_color_override = false;

			if ( p_font )
			{
				sprintf( p_text, "OBJ %d / %d - NOT VISIBLE", g_view_object, g_object + 1 );
				message.m_xpos = 28.0f;
				message.m_ypos = 72.0f;
				message.DrawSingle();
			}
		}
	}
	else
	{
		if ( ( g_material >= 0 ) && ( g_material < g_view_scene->mp_scene_data->m_num_materials ) )
		{
			// Draw material.

			NxNgc::sMaterialHeader * p_mat = &g_view_scene->mp_material_header[g_material];

			uint8 save_passes = p_mat->m_passes;
			p_mat->m_passes = ( g_passes == -1 ) ? p_mat->m_passes : ( g_passes + 1 );

			// Adjust mip to render.
			int mip_off[4] = { 0, 0, 0, 0 };
			{
				NxNgc::sMaterialPassHeader * p_pass = &g_view_scene->mp_material_pass[p_mat->m_pass_item];
				for ( int tex = 0; tex < p_mat->m_passes; tex++, p_pass++ )
				{
					if ( p_pass->m_texture.p_data )
					{
						for ( int mm = 0; mm < g_mip; mm++ )
						{
							int mw = p_pass->m_texture.p_data->ActualWidth >> mm;
							int mh = p_pass->m_texture.p_data->ActualHeight >> mm;
							mip_off[tex] += ( mw * mh ) >> 1;
						}
						p_pass->m_texture.p_data->pTexelData += mip_off[tex];
						p_pass->m_texture.p_data->pAlphaData += mip_off[tex];
						p_pass->m_texture.p_data->ActualWidth >>= g_mip;
						p_pass->m_texture.p_data->ActualHeight >>= g_mip;
					}
				}
			}

			multi_mesh( p_mat,
						&g_view_scene->mp_material_pass[p_mat->m_pass_item],
						true,
						true );
			p_mat->m_passes = save_passes;
			// Adjust mip back.
			{
				NxNgc::sMaterialPassHeader * p_pass = &g_view_scene->mp_material_pass[p_mat->m_pass_item];
				for ( int tex = 0; tex < p_mat->m_passes; tex++, p_pass++ )
				{
					if ( p_pass->m_texture.p_data )
					{
						p_pass->m_texture.p_data->pTexelData -= mip_off[tex];
						p_pass->m_texture.p_data->pAlphaData -= mip_off[tex];
						p_pass->m_texture.p_data->ActualWidth <<= g_mip;
						p_pass->m_texture.p_data->ActualHeight <<= g_mip;
					}
				}
			}

			GX::SetChanCtrl( GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
			GX::SetChanMatColor( GX_COLOR0A0, (GXColor){64,64,64,128} );
			GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){64,64,64,128} );

			GX::SetVtxDesc( 5, GX_VA_POS,  GX_DIRECT,
							   GX_VA_TEX0, GX_DIRECT,
							   GX_VA_TEX1, GX_DIRECT,
							   GX_VA_TEX2, GX_DIRECT,
							   GX_VA_TEX3, GX_DIRECT );

			GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
				GX::Position3f32(32.0f, 96.0f, -1.0f);
				GX::TexCoord2f32(0.0f, 1.0f);
				GX::TexCoord2f32(0.0f, 1.0f);
				GX::TexCoord2f32(0.0f, 1.0f);
				GX::TexCoord2f32(0.0f, 1.0f);
				GX::Position3f32(160.0f, 96.0f, -1.0f);
				GX::TexCoord2f32(1.0f, 1.0f);
				GX::TexCoord2f32(1.0f, 1.0f);
				GX::TexCoord2f32(1.0f, 1.0f);
				GX::TexCoord2f32(1.0f, 1.0f);
				GX::Position3f32(160.0f, 222.0f, -1.0f);
				GX::TexCoord2f32(1.0f, 0.0f);
				GX::TexCoord2f32(1.0f, 0.0f);
				GX::TexCoord2f32(1.0f, 0.0f);
				GX::TexCoord2f32(1.0f, 0.0f);
				GX::Position3f32(32.0f, 222.0f, -1.0f);
				GX::TexCoord2f32(0.0f, 0.0f);
				GX::TexCoord2f32(0.0f, 0.0f);
				GX::TexCoord2f32(0.0f, 0.0f);
				GX::TexCoord2f32(0.0f, 0.0f);
			GX::End();

			float	tx[8] = { 32.0f, 98.0f, 164.0f, 230.0f, 296.0f, 362.0f, 428.0f, 494.0f };
			float	ty[8] = { 230.0f, 230.0f, 230.0f, 230.0f, 230.0f, 230.0f, 230.0f, 230.0f };
			int		tr = 0;

			// Draw Object & material number.
			{
				NxNgc::SText message;
				Nx::CFont * p_cfont;
				const char * p_font_name = "testtitle";
				char p_text[128];

				// We can only draw a text string if we have a font & a string.
				p_cfont = Nx::CFontManager::sGetFont( p_font_name );
				if ( !p_cfont )
				{
					Nx::CFontManager::sLoadFont( p_font_name );
					p_cfont = Nx::CFontManager::sGetFont( p_font_name );
				}
				message.mp_string = p_text;
				message.m_rgba = 0x808080ff;

				Nx::CNgcFont * p_nfont = static_cast<Nx::CNgcFont*>( p_cfont );
				NxNgc::SFont * p_font = p_nfont->GetEngineFont();
				message.mp_font = p_font;
				message.m_xscale = 0.75f;
				message.m_yscale = 1.2f;

				message.m_color_override = false;

				if ( p_font )
				{
					char * p_pass_string = "***";
					if ( g_passes == -1 ) p_pass_string = "ALL";
					if ( g_passes == 0 ) p_pass_string = "P1";
					if ( g_passes == 1 ) p_pass_string = "P2";
					if ( g_passes == 2 ) p_pass_string = "P3";
					if ( g_passes == 3 ) p_pass_string = "P4";
					sprintf( p_text, "OBJ %d / %d - MAT %d / %d DO: %6.3f AC: %3d M: %d %s", g_view_object, g_object + 1, g_material, g_view_scene->mp_scene_data->m_num_materials, p_mat->m_draw_order, p_mat->m_alpha_cutoff, g_mip, p_pass_string );
					message.m_xpos = 28.0f;
					message.m_ypos = 72.0f;
					message.DrawSingle();
				}
			}

			NxNgc::sMaterialPassHeader * p_pass = &g_view_scene->mp_material_pass[p_mat->m_pass_item];
			for ( int tex = 0; tex < p_mat->m_passes; tex++, p_pass++ )
			{
				if ( p_pass->m_texture.p_data )
				{
					// Draw texture.
					int w = p_pass->m_texture.p_data->ActualWidth >> g_mip;
					int h = p_pass->m_texture.p_data->ActualHeight >> g_mip;
					int mip_off = 0;
					for ( int mm = 0; mm < g_mip; mm++ )
					{
						int mw = p_pass->m_texture.p_data->ActualWidth >> mm;
						int mh = p_pass->m_texture.p_data->ActualHeight >> mm;
						mip_off += ( mw * mh ) >> 1;
					}
					GX::UploadTexture(  &p_pass->m_texture.p_data->pTexelData[mip_off],
										w,
										h,
										GX_TF_CMPR,
										GX_CLAMP,
										GX_CLAMP,
										GX_FALSE,
										GX_LINEAR,
										GX_LINEAR,
										0.0f,
										0.0f,
										0.0f,
										GX_FALSE,
										GX_TRUE,
										GX_ANISO_1,
										GX_TEXMAP0 ); 
					GX::SetTexCoordScale( GX_TEXCOORD0, GX_TRUE, w, h );

					GX::SetTexChanTevIndCull( 1, 0, 1, 0, GX_CULL_NONE );
					GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
					GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );

					GX::SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0 );
					GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );		// Replace

					GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);
					GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
											 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV,
											 GX_TEV_SWAP0, GX_TEV_SWAP0 );
					GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
										 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV );

					GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_TEX0, GX_DIRECT );

					GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
						GX::Position3f32(tx[tr], ty[tr], -1.0f);
						GX::TexCoord2f32(0.0f, 1.0f);
						GX::Position3f32(tx[tr] + 62.0f, ty[tr], -1.0f);
						GX::TexCoord2f32(1.0f, 1.0f);
						GX::Position3f32(tx[tr] + 62.0f, ty[tr] + 62.0f, -1.0f);
						GX::TexCoord2f32(1.0f, 0.0f);
						GX::Position3f32(tx[tr], ty[tr] + 62.0f, -1.0f);
						GX::TexCoord2f32(0.0f, 0.0f);
					GX::End();
					if ( p_pass->m_texture.p_data->flags & NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA )
					{
						// Draw alpha.
						int w = p_pass->m_texture.p_data->ActualWidth >> g_mip;
						int h = p_pass->m_texture.p_data->ActualHeight >> g_mip;
						int mip_off = 0;
						for ( int mm = 0; mm < g_mip; mm++ )
						{
							int mw = p_pass->m_texture.p_data->ActualWidth >> mm;
							int mh = p_pass->m_texture.p_data->ActualHeight >> mm;
							mip_off += ( mw * mh ) >> 1;
						}
						GX::UploadTexture(  &p_pass->m_texture.p_data->pAlphaData[mip_off],
											w,
											h,
											GX_TF_CMPR,
											GX_CLAMP,
											GX_CLAMP,
											GX_FALSE,
											GX_LINEAR,
											GX_LINEAR,
											0.0f,
											0.0f,
											0.0f,
											GX_FALSE,
											GX_TRUE,
											GX_ANISO_1,
											GX_TEXMAP0 ); 
						GX::SetTexCoordScale( GX_TEXCOORD0, GX_TRUE, w, h );

						GX::SetTexChanTevIndCull( 1, 0, 1, 0, GX_CULL_NONE );
						GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
						GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );

						GX::SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0 );
						GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );		// Replace

						GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);

						GXTevSwapSel alpha_swap = GX_TEV_SWAP1;
						switch ( ( p_pass->m_texture.p_data->flags & NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_MASK ) )
						{
							case NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_GREEN:
							default:
								alpha_swap = GX_TEV_SWAP1;		// Green
								break;
							case NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_RED:
								alpha_swap = GX_TEV_SWAP2;		// Red
								break;
							case NxNgc::sTexture::TEXTURE_FLAG_CHANNEL_BLUE:
								alpha_swap = GX_TEV_SWAP3;		// Blue
								break;
						}

						GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
												 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV,
												 GX_TEV_SWAP0, alpha_swap );
						GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
											 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV );

						GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_TEX0, GX_DIRECT );

						GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
							GX::Position3f32(tx[tr+1], ty[tr+1], -1.0f);
							GX::TexCoord2f32(0.0f, 1.0f);
							GX::Position3f32(tx[tr+1] + 62.0f, ty[tr+1], -1.0f);
							GX::TexCoord2f32(1.0f, 1.0f);
							GX::Position3f32(tx[tr+1] + 62.0f, ty[tr+1] + 62.0f, -1.0f);
							GX::TexCoord2f32(1.0f, 0.0f);
							GX::Position3f32(tx[tr+1], ty[tr+1] + 62.0f, -1.0f);
							GX::TexCoord2f32(0.0f, 0.0f);
						GX::End();
					}
				}

				// Draw pass info.
				char * blendmode[14] = {
					"DIFF",
					"ADD",
					"ADDF",
					"SUB",
					"SUBF",
					"BLND",
					"BLNDF",
					"MOD",
					"MODF",
					"BRT",
					"BRTF",
					"GLOSS",
					"BLPV",
					"BLPVI"
				};

				NxNgc::SText message;
				Nx::CFont * p_cfont;
				const char * p_font_name = "testtitle";
				char p_text[128];

				// We can only draw a text string if we have a font & a string.
				p_cfont = Nx::CFontManager::sGetFont( p_font_name );
				if ( !p_cfont )
				{
					Nx::CFontManager::sLoadFont( p_font_name );
					p_cfont = Nx::CFontManager::sGetFont( p_font_name );
				}
				message.mp_string = p_text;
				message.m_rgba = 0x808080ff;

				Nx::CNgcFont * p_nfont = static_cast<Nx::CNgcFont*>( p_cfont );
				NxNgc::SFont * p_font = p_nfont->GetEngineFont();
				message.mp_font = p_font;
				message.m_xscale = 0.75f;
				message.m_yscale = 1.2f;

				message.m_color_override = false;

# define toupper(c) ( ( (c) >= 'a' ) && ( (c) <= 'z' ) ) ? (c) += ( 'A' - 'a' ) : (c)

				if ( p_font )
				{
					if ( p_pass->m_texture.p_data )
					{
						sprintf( p_text, "%d,%d", p_pass->m_texture.p_data->ActualWidth, p_pass->m_texture.p_data->ActualHeight );
						message.m_xpos = tx[tr];
						message.m_ypos = ty[tr] + 66.0f + ( 16.0 * 0.0f );
						message.DrawSingle();
					}

					sprintf( p_text, "%s", blendmode[p_pass->m_blend_mode] );
					message.m_xpos = tx[tr];
					message.m_ypos = ty[tr] + 66.0f + ( 16.0 * 1.0f );
					message.DrawSingle();

					sprintf( p_text, "%02x %02x", p_pass->m_color.r, p_pass->m_color.g );
					p_text[0] = toupper( p_text[0] );
					p_text[1] = toupper( p_text[1] );
					p_text[3] = toupper( p_text[3] );
					p_text[4] = toupper( p_text[4] );
					message.m_xpos = tx[tr];
					message.m_ypos = ty[tr] + 66.0f + ( 16.0 * 2.0f );
					message.DrawSingle();

					sprintf( p_text, "%02x %02x", p_pass->m_color.b, p_pass->m_color.a );
					p_text[0] = toupper( p_text[0] );
					p_text[1] = toupper( p_text[1] );
					p_text[3] = toupper( p_text[3] );
					p_text[4] = toupper( p_text[4] );
					message.m_xpos = tx[tr];
					message.m_ypos = ty[tr] + 66.0f + ( 16.0 * 3.0f );
					message.DrawSingle();

					sprintf( p_text, "%02x %02x", p_pass->m_alpha_fix, p_pass->m_flags );
					p_text[0] = toupper( p_text[0] );
					p_text[1] = toupper( p_text[1] );
					p_text[3] = toupper( p_text[3] );
					p_text[4] = toupper( p_text[4] );
					message.m_xpos = tx[tr];
					message.m_ypos = ty[tr] + 66.0f + ( 16.0 * 4.0f );
					message.DrawSingle();

					if ( p_pass->m_texture.p_data )
					{
						sprintf( p_text, "%3.1f %d", (float)(p_pass->m_k) * (1.0f / (float)(1<<8)), p_pass->m_texture.p_data->Levels );
						message.m_xpos = tx[tr];
						message.m_ypos = ty[tr] + 66.0f + ( 16.0 * 5.0f );
						message.DrawSingle();
					}
				}
				tr++;
				if ( p_pass->m_texture.p_data && p_pass->m_texture.p_data->flags & NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA ) tr++;
			}
		}
	}
#endif

//	// Draw texture.
//	GX::UploadTexture(  shadowTextureData,
//						SHADOW_TEXTURE_SIZE,
//						SHADOW_TEXTURE_SIZE,
//						GX_TF_I4,
//						GX_CLAMP,
//						GX_CLAMP,
//						GX_FALSE,
//						GX_LINEAR,
//						GX_LINEAR,
//						0.0f,
//						0.0f,
//						0.0f,
//						GX_FALSE,
//						GX_TRUE,
//						GX_ANISO_1,
//						GX_TEXMAP0 ); 
//	GX::SetTexCoordScale( GX_TEXCOORD0, GX_TRUE, SHADOW_TEXTURE_SIZE, SHADOW_TEXTURE_SIZE );
//
//	GX::UploadPalette( shadowPalette,
//					   GX_TL_RGB5A3,
//					   GX_TLUT_16,
//					   GX_TEXMAP0 );
//
//	GX::SetTexChanTevIndCull( 1, 0, 1, 0, GX_CULL_NONE );
//	GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
//	GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );
//
//	GX::SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0 );
//	GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );		// Replace
//
//	GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);
//	GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
//							 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV,
//							 GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
//						 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV );
//
//	GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_TEX0, GX_DIRECT );
//
//	GX::Begin( GX_QUADS, GX_VTXFMT0, 4 );
//		GX::Position3f32( 32.0f,  32.0f, -1.9f);
//		GX::TexCoord2f32(0.0f, 0.0f);
//		GX::Position3f32(288.0f,  32.0f, -1.9f);
//		GX::TexCoord2f32(1.0f, 0.0f);
//		GX::Position3f32(288.0f, 288.0f, -1.9f);
//		GX::TexCoord2f32(1.0f, 1.0f);
//		GX::Position3f32( 32.0f, 288.0f, -1.9f);
//		GX::TexCoord2f32(0.0f, 1.0f);
//	GX::End();



/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////













//	// Shadow volume stage 1.
//	// And the alpha channel with 128. Pixels in shadow will have alpha of 128, not will be 0.
//	GX::SetTexChanTevIndCull( 0, 1, 1, 0, GX_CULL_NONE );
//	GX::SetBlendMode ( GX_BM_LOGIC, GX_BL_SRCALPHA, GX_BL_DSTALPHA, GX_LO_AND, GX_FALSE, GX_TRUE, GX_FALSE );
//	GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);
//	GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
//							 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV,
//							 GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
//						 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV );
//	GX::SetTevKColor( GX_KCOLOR0, (GXColor){0,0,0,128} );
//	GX::SetTevKSel( GX_TEVSTAGE0, GX_TEV_KCSEL_K0, GX_TEV_KASEL_K0_A, GX_TEV_KCSEL_K0, GX_TEV_KASEL_K0_A );
//
//	GX::SetVtxDesc( 1, GX_VA_POS, GX_DIRECT );
//
//	GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
//		GX::Position3f32(0.0f, 0.0f, -1.0f);
//		GX::Position3f32(640.0f, 0.0f, -1.0f);
//		GX::Position3f32(640.0f, 448.0f, -1.0f);
//		GX::Position3f32(0.0f, 448.0f, -1.0f);
//	GX::End();
//
//	// Shadow volume stage 2.
//	// Blend alpha with shadow color.
//	GX::SetTexChanTevIndCull( 0, 1, 1, 0, GX_CULL_NONE );
//	GX::SetBlendMode( GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_INVDSTALPHA, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_FALSE );
//	GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);
//	GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
//							 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV,
//							 GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_KONST, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
//						 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV );
//	GX::SetTevKColor( GX_KCOLOR0, (GXColor){0,0,0,128} );
//	GX::SetTevKSel( GX_TEVSTAGE0, GX_TEV_KCSEL_K0, GX_TEV_KASEL_K0_A, GX_TEV_KCSEL_K0, GX_TEV_KASEL_K0_A );
//
//	GX::SetVtxDesc( 1, GX_VA_POS, GX_DIRECT );
//
//	GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
//		GX::Position3f32(0.0f, 0.0f, -1.0f);
//		GX::Position3f32(640.0f, 0.0f, -1.0f);
//		GX::Position3f32(640.0f, 448.0f, -1.0f);
//		GX::Position3f32(0.0f, 448.0f, -1.0f);
//	GX::End();








































#if 0
	// Copy framebuffer to 320x224 texture.
	GXSetCopyFilter( GX_FALSE, NULL, GX_FALSE, NULL );
	GXSetTexCopySrc(0, 0, 640, 448);
	GXSetTexCopyDst(320, 224, GX_TF_Z8, GX_TRUE);
	GXCopyTex(zTextureDataH, GX_FALSE);
	GXPixModeSync();

	GXSetCopyFilter( GX_FALSE, NULL, GX_FALSE, NULL );
	GXSetTexCopySrc(0, 0, 640, 448);
	GXSetTexCopyDst(320, 224, GX_CTF_Z8M, GX_TRUE);
	GXCopyTex(zTextureDataL, GX_FALSE);
	GXPixModeSync();

	// Copy top-corner of screen.
	GXSetCopyFilter( GX_FALSE, NULL, GX_FALSE, NULL );
	GXSetTexCopySrc(0, 0, 320, 224);
	GXSetTexCopyDst(320, 224, GX_TF_RGBA8, GX_FALSE);
	GXCopyTex(screenTextureData, GX_FALSE);
	GXPixModeSync();

	// Copy screen and filter down.
	GXSetCopyFilter( GX_TRUE, sample_pattern, GX_TRUE, GXNtsc480IntAa.vfilter );
	GXSetTexCopySrc(0, 0, 640, 448);
	GXSetTexCopyDst(320, 224, GX_TF_RGBA8, GX_TRUE);
	GXCopyTex(focusTextureData, GX_FALSE);
	GXPixModeSync();

	// Shrink down the focus texture.
	GXTexObj	focusTexture;
	GXInitTexObj(
		&focusTexture,
		focusTextureData,
		320,
		224,
		GX_TF_RGBA8,
		GX_CLAMP,
		GX_CLAMP,
		GX_FALSE );
	GXInitTexObjLOD(&focusTexture, GX_NEAR, GX_NEAR, 0, 0, 0, 0, 0, GX_ANISO_1);
	GXLoadTexObj( &focusTexture, GX_TEXMAP0 );

	GX::SetNumChans(0);
	GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
	GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );

	GX::SetNumTevStages( 1 );
	GX::SetNumTexGens( 1 );
	GX::SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0 );
	GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_FALSE );		// Replace

	GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GX::SetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
	GX::SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV );
	GX::SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV );
	GX::SetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
	GX::SetTevColorIn ( GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );

	GXSetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_TEX0, GX_DIRECT );

	GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
		GXPosition3f32(0.0f, 0.0f, -1.0f);
		GXTexCoord2f32(0.0f, 0.0f);
		GXPosition3f32(320.0f, 0.0f, -1.0f);
		GXTexCoord2f32(1.0f, 0.0f);
		GXPosition3f32(320.0f, 224.0f, -1.0f);
		GXTexCoord2f32(1.0f, 1.0f);
		GXPosition3f32(0.0f, 224.0f, -1.0f);
		GXTexCoord2f32(0.0f, 1.0f);
	GXEnd();

	GXSetCopyFilter( GX_TRUE, sample_pattern, GX_TRUE, GXNtsc480IntAa.vfilter );
	GXSetTexCopySrc(0, 0, 320, 224);
	GXSetTexCopyDst(160, 112, GX_TF_RGBA8, GX_TRUE);
	GXCopyTex(focusTextureData, GX_FALSE);
	GXPixModeSync();

	// Draw top-corner screen area we wrote over.
	GXTexObj	screenTexture;
	GXInitTexObj(
		&screenTexture,
		screenTextureData,
		320,
		224,
		GX_TF_RGBA8,
		GX_CLAMP,
		GX_CLAMP,
		GX_FALSE );
	GXInitTexObjLOD(&screenTexture, GX_NEAR, GX_NEAR, 0, 0, 0, 0, 0, GX_ANISO_1);
	GXLoadTexObj( &screenTexture, GX_TEXMAP0 );

	GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
		GXPosition3f32(0.0f, 0.0f, -1.0f);
		GXTexCoord2f32(0.0f, 0.0f);
		GXPosition3f32(320.0f, 0.0f, -1.0f);
		GXTexCoord2f32(1.0f, 0.0f);
		GXPosition3f32(320.0f, 224.0f, -1.0f);
		GXTexCoord2f32(1.0f, 1.0f);
		GXPosition3f32(0.0f, 224.0f, -1.0f);
		GXTexCoord2f32(0.0f, 1.0f);
	GXEnd();

	// Setup z textures
	GXTexObj	zTextureH;
	GXTexObj	zTextureL;
	GXTlutObj	zPalH;
	GXTlutObj	zPalL;

	GXInitTlutObj( &zPalH, &zPalette8H, GX_TL_IA8, 256 );
	GXLoadTlut ( &zPalH, GX_TLUT0 );

	GXInitTlutObj( &zPalL, &zPalette8L, GX_TL_IA8, 256 );
	GXLoadTlut ( &zPalL, GX_TLUT1 );

	GXInitTexObjCI(
		&zTextureH,
		zTextureDataH,
		320,
		224,
		GX_TF_C8,
		GX_CLAMP,
		GX_CLAMP,
		GX_FALSE,
		GX_TLUT0 );
	GXInitTexObjLOD(&zTextureH, GX_NEAR, GX_NEAR, 0, 0, 0, 0, 0, GX_ANISO_1);
	GXLoadTexObj( &zTextureH, GX_TEXMAP0 );

	GXInitTexObjCI(
		&zTextureL,
		zTextureDataL,
		320,
		224,
		GX_TF_C8,
		GX_CLAMP,
		GX_CLAMP,
		GX_FALSE,
		GX_TLUT1 );
	GXInitTexObjLOD(&zTextureL, GX_NEAR, GX_NEAR, 0, 0, 0, 0, 0, GX_ANISO_1);
	GXLoadTexObj( &zTextureL, GX_TEXMAP1 );

	GXInitTexObj(
		&focusTexture,
		focusTextureData,
		160,
		112,
		GX_TF_RGBA8,
		GX_CLAMP,
		GX_CLAMP,
		GX_FALSE );
	GXInitTexObjLOD(&focusTexture, GX_LINEAR, GX_LINEAR, 0, 0, 0, 0, 0, GX_ANISO_1);
	GXLoadTexObj( &focusTexture, GX_TEXMAP2 );









//	GXTexObj	zTexture;
//	GXTlutObj	zPal;
//
//	GXInitTlutObj( &zPal, &zPalette16, GX_TL_IA8, 65535 );
//	GXLoadTlut ( &zPal, GX_TLUT0 );
//
//	GXInitTexObjCI(
//		&zTexture,
//		zTextureData,
//		320,
//		224,
//		GX_TF_C14X2,
//		GX_CLAMP,
//		GX_CLAMP,
//		GX_FALSE,
//		GX_TLUT0 );
//	GXInitTexObjLOD(&zTexture, GX_NEAR, GX_NEAR, 0, 0, 0, 0, 0, GX_ANISO_1);
//	GXLoadTexObj( &zTexture, GX_TEXMAP0 );

	GX::SetTexChanTevIndCull( 3, 0, 3, 0, GX_CULL_NONE ); 
	GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
	GX::SetTexCoordGen( GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
	GX::SetTexCoordGen( GX_TEXCOORD2, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
	GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );

	GX::SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0 );

//	GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ZERO, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_FALSE ); // Replace alpha
//	GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR );		// Replace
	GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_FALSE );		// Blend

//	GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
//	GX::SetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_4, GX_DISABLE, GX_TEVPREV );
//	GX::SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_4, GX_DISABLE, GX_TEVPREV );
//	GX::SetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
//	GX::SetTevColorIn ( GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );
//
//	GX::SetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
//	GX::SetTevSwapMode( GX_TEVSTAGE1, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_4, GX_DISABLE, GX_TEVREG1 );
//	GX::SetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_4, GX_DISABLE, GX_TEVREG1 );
//	GX::SetTevAlphaIn ( GX_TEVSTAGE1, GX_CA_APREV, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
//	GX::SetTevColorIn ( GX_TEVSTAGE1, GX_CC_CPREV, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );
//
//	GX::SetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
//	GX::SetTevSwapMode( GX_TEVSTAGE2, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVREG0 );
//	GX::SetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVREG0 );
//	GX::SetTevAlphaIn ( GX_TEVSTAGE2, GX_CA_A1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
//	GX::SetTevColorIn ( GX_TEVSTAGE2, GX_CC_C1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );
//
//	GX::SetTevKColorSel( GX_TEVSTAGE3, GX_TEV_KCSEL_K0_A );
//	GX::SetTevKColor( GX_KCOLOR0, (GXColor){16,16,16,16} );
//
//	GX::SetTevOrder(GX_TEVSTAGE3, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR_NULL);
//	GX::SetTevSwapMode( GX_TEVSTAGE3, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetTevAlphaOp(GX_TEVSTAGE3, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG1 );
//	GX::SetTevColorOp(GX_TEVSTAGE3, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG1 );
//	GX::SetTevAlphaIn ( GX_TEVSTAGE3, GX_CA_ZERO, GX_CA_KONST, GX_CA_TEXA, GX_CA_ZERO );
//	GX::SetTevColorIn ( GX_TEVSTAGE3, GX_CC_ZERO, GX_CC_KONST, GX_CC_TEXC, GX_CC_ZERO );
//
//	GX::SetTevOrder(GX_TEVSTAGE4, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
//	GX::SetTevSwapMode( GX_TEVSTAGE4, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetTevAlphaOp(GX_TEVSTAGE4, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//	GX::SetTevColorOp(GX_TEVSTAGE4, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//	GX::SetTevAlphaIn ( GX_TEVSTAGE4, GX_CA_A0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A1 );
//	GX::SetTevColorIn ( GX_TEVSTAGE4, GX_CC_C0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C1 );



//	GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
//	GX::SetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG0 );
//	GX::SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVREG0 );
//	GX::SetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA );
//	GX::SetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC );
//
//	GX::SetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR_NULL);
//	GX::SetTevSwapMode( GX_TEVSTAGE1, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//	GX::SetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
////	GX::SetTevAlphaIn ( GX_TEVSTAGE1, GX_CA_A0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA );
////	GX::SetTevColorIn ( GX_TEVSTAGE1, GX_CC_C0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC );
////	GX::SetTevAlphaIn ( GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA );
////	GX::SetTevColorIn ( GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC );
//	GX::SetTevAlphaIn ( GX_TEVSTAGE1, GX_CA_A0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
//	GX::SetTevColorIn ( GX_TEVSTAGE1, GX_CC_C0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );




//	GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
//	GX::SetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_4, GX_DISABLE, GX_TEVPREV );
//	GX::SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_4, GX_DISABLE, GX_TEVPREV );
//	GX::SetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
//	GX::SetTevColorIn ( GX_TEVSTAGE0, GX_CC_ONE, GX_CC_ZERO, GX_CC_TEXC, GX_CC_ZERO );
//
//	GX::SetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
//	GX::SetTevSwapMode( GX_TEVSTAGE1, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_4, GX_DISABLE, GX_TEVREG1 );
//	GX::SetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_4, GX_DISABLE, GX_TEVREG1 );
//	GX::SetTevAlphaIn ( GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
//	GX::SetTevColorIn ( GX_TEVSTAGE1, GX_CC_CPREV, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );
//
//	GX::SetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
//	GX::SetTevSwapMode( GX_TEVSTAGE2, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVREG0 );
//	GX::SetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVREG0 );
//	GX::SetTevAlphaIn ( GX_TEVSTAGE2, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
//	GX::SetTevColorIn ( GX_TEVSTAGE2, GX_CC_C1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );
//
//	GX::SetTevKColorSel( GX_TEVSTAGE3, GX_TEV_KCSEL_K0_A );
//	GX::SetTevKColor( GX_KCOLOR0, (GXColor){16,16,16,16} );
//
//	GX::SetTevOrder(GX_TEVSTAGE3, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR_NULL);
//	GX::SetTevSwapMode( GX_TEVSTAGE3, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetTevAlphaOp(GX_TEVSTAGE3, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV );
//	GX::SetTevColorOp(GX_TEVSTAGE3, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV );
//	GX::SetTevAlphaIn ( GX_TEVSTAGE3, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
//	GX::SetTevColorIn ( GX_TEVSTAGE3, GX_CC_ONE, GX_CC_ZERO, GX_CC_TEXC, GX_CC_C0 );




	GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR_NULL);
	GX::SetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
	GX::SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVREG0 );
	GX::SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVREG0 );
	GX::SetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
	GX::SetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );

	GX::SetTevSwapMode( GX_TEVSTAGE1, GX_TEV_SWAP0, GX_TEV_SWAP0 );
	GX::SetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVREG1 );
	GX::SetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVREG1 );
	GX::SetTevAlphaIn ( GX_TEVSTAGE1, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A0 );
	GX::SetTevColorIn ( GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );

	GX::SetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD2, GX_TEXMAP2, GX_COLOR_NULL, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);
	GX::SetTevSwapMode( GX_TEVSTAGE2, GX_TEV_SWAP0, GX_TEV_SWAP0 );
	GX::SetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV );
	GX::SetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV );
	GX::SetTevAlphaIn ( GX_TEVSTAGE2, GX_CA_A1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
	GX::SetTevColorIn ( GX_TEVSTAGE2, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );
//	GX::SetTevAlphaIn ( GX_TEVSTAGE2, GX_CA_A1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
//	GX::SetTevColorIn ( GX_TEVSTAGE2, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );



//    GXSetTevSwapModeTable( GX_TEV_SWAP1, GX_CH_RED, GX_CH_RED, GX_CH_RED, GX_CH_RED );
//    GXSetTevSwapModeTable( GX_TEV_SWAP2, GX_CH_GREEN, GX_CH_GREEN, GX_CH_GREEN, GX_CH_GREEN );
//    GXSetTevSwapModeTable( GX_TEV_SWAP3, GX_CH_BLUE, GX_CH_BLUE, GX_CH_BLUE, GX_CH_BLUE );

//	GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
//	GX::SetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV );
//	GX::SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV );
//	GX::SetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
//	GX::SetTevColorIn ( GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );



//	if ( gFocus )
//	{
//		// Set current vertex descriptor to enable position and color0.
//		// Both use 8b index to access their data arrays.
//		GXSetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_TEX0, GX_DIRECT );
//
//		GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
//			GXPosition3f32(0.0f, 0.0f, -1.0f);
//			GXTexCoord2f32(0.0f, 0.0f);
//			GXPosition3f32(640.0f, 0.0f, -1.0f);
//			GXTexCoord2f32(1.0f, 0.0f);
//			GXPosition3f32(640.0f, 448.0f, -1.0f);
//			GXTexCoord2f32(1.0f, 1.0f);
//			GXPosition3f32(0.0f, 448.0f, -1.0f);
//			GXTexCoord2f32(0.0f, 1.0f);
//		GXEnd();
//		
//	}
#endif




































/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////



	GX::SetZMode ( GX_FALSE, GX_ALWAYS, GX_FALSE );
	NxNgc::set_blend_mode( NxNgc::vBLEND_MODE_ADD );
	NxNgc::SDraw2D::DrawAll();

//	if ( NsDisplay::shouldReset() && NxNgc::EngineGlobals.disableReset )
//	{
//		GX::SetNumTevStages(1);
//		GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
//		GX::SetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//		GX::SetNumTexGens( 0 );
//		GX::SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//		GX::SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//		GX::SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
//		GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_FALSE );
//		GX::SetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA );
//		GX::SetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC );
//		GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//		GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){0,0,0,255} );
//		GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//
//		GXSetVtxDesc( 1, GX_VA_POS, GX_DIRECT );
//
//		GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
//			GXPosition3f32(128.0f, 224.0f - 64.0f, -1.0f);
//			GXPosition3f32(640.0f - 128.0f, 224.0f - 64.0f, -1.0f);
//			GXPosition3f32(640.0f - 128.0f, 224.0f + 64.0f, -1.0f);
//			GXPosition3f32(128.0f, 224.0f + 64.0f, -1.0f);
//		GXEnd();
//
//		Script::RunScript( "ngc_reset" );
//	}

//	// Set up shadow map texture
//	GXTexObj shadowTexture;
//	GXInitTexObj(
//		&shadowTexture,
//		shadowTextureData,
//		SHADOW_TEXTURE_SIZE,
//		SHADOW_TEXTURE_SIZE,
//		GX_TF_RGBA8,
//		GX_CLAMP,
//		GX_CLAMP,
//		GX_FALSE );
//	GXInitTexObjLOD(&shadowTexture, GX_NEAR, GX_NEAR, 0, 0, 0, 0, 0, GX_ANISO_1);
//	GXLoadTexObj( &shadowTexture, GX_TEXMAP0 );
//
//    GXSetNumChans(1);
//	GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_TEX0, GX_IDENTITY );
//
//    GX::SetNumTevStages(1);
//	GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
//	GX::SetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetNumTexGens( 1 );
//	GX::SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//	GX::SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//	GX::SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
//	GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_FALSE );
//	GX::SetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO );
//	GX::SetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO );
//	GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//	GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){128,128,128,255} );
//	GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//
//    Mtx mv;
//	MTXIdentity( mv );
//    GX::LoadTexMtxImm(mv, GX_TEXMTX0, GX_MTX3x4);
//    GX::LoadTexMtxImm(mv, GX_TEXMTX1, GX_MTX3x4);
//
//	// Set current vertex descriptor to enable position and color0.
//	// Both use 8b index to access their data arrays.
//	GXSetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_TEX0, GX_DIRECT );
//
//	// Send coordinates.
//	GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
//		GXPosition3f32(320, 32, -1.0f);
//		GXTexCoord2f32(0.0f, 0.0f);
//		GXPosition3f32(576, 32, -1.0f);
//		GXTexCoord2f32(1.0f, 0.0f);
//		GXPosition3f32(576, 288, -1.0f);
//		GXTexCoord2f32(1.0f, 1.0f);
//		GXPosition3f32(320, 288, -1.0f);
//		GXTexCoord2f32(0.0f, 1.0f);
//	GXEnd();


	// Blur.
//	if ( sBlur > 0 )
//	{
//		
//	}
//
//sBlurBuffer

	
	
	
	
	
	
	
//	GX::SetTexCoordScale( GX_TEXCOORD0, GX_TRUE, 64, 64 );
//	
//	GX::SetTexChanTevIndCull( 1, 0, 1, 0, GX_CULL_NONE );
//	GX::SetTexCoordGen( GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_FALSE, GX_PTIDENTITY );
//	GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );
//	
//	GX::SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0 );
//	GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );		// Replace
//	
//	GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP7, GX_COLOR_NULL, GX_TEXCOORD0, GX_TEXMAP_NULL, GX_COLOR_NULL);
//	GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
//							 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV,
//							 GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
//						 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_DISABLE, GX_TEVPREV );
//	
//	GX::SetChanCtrl( GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//	GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//	GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//	
//	GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_TEX0, GX_DIRECT );
//	
//	GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
//		GX::Position3f32(64.0f, 64.0f, -1.0f);
//		GX::TexCoord2f32(0.0f, 0.0f);
//		GX::Position3f32(64.0f + 64.0f, 64.0f, -1.0f);
//		GX::TexCoord2f32(1.0f, 0.0f);
//		GX::Position3f32(64.0f + 64.0f, 64.0f + 64.0f, -1.0f);
//		GX::TexCoord2f32(1.0f, 1.0f);
//		GX::Position3f32(64.0f, 64.0f + 64.0f, -1.0f);
//		GX::TexCoord2f32(0.0f, 1.0f);
//	GX::End();
//	
	
	
	
	
	
	
	
	cam.end();
	NsPrim::end();
	NsRender::end();

//	if ( movies )
//	{
//		NsDisplay::end( false );
//	}
//	else
//	{
		//NsDisplay::end( true );
//	}
	NsBuffer::end();

	NxNgc::EngineGlobals.gpuBusy = false;

	NxNgc::EngineGlobals.frameCount = ( NxNgc::EngineGlobals.frameCount + 1 ) & 1;

	NsDisplay::end(true);
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CScene	*	CEngine::s_plat_create_scene(const char *p_name, CTexDict *p_tex_dict, bool add_super_sectors)
{
	// Create scene class instance
	CNgcScene	*p_Ngc_scene	= new CNgcScene;
	CScene		*p_new_scene	= p_Ngc_scene;
	p_new_scene->SetInSuperSectors( add_super_sectors );
	p_new_scene->SetIsSky( false );

	// Create a new sScene so the engine can track assets for this scene.
	NxNgc::sScene *p_engine_scene = new NxNgc::sScene();
	p_Ngc_scene->SetEngineScene( p_engine_scene );

	return p_new_scene;
}

#define MemoryRead( dst, size, num, src )	memcpy(( dst ), ( src ), (( num ) * ( size )));	\
											( src ) += (( num ) * ( size ))

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static int s_plat_scene_size( void *p_mem, CTexDict *p_tex_dict, bool add_super_sectors, bool is_sky, bool is_dictionary )
{
	void * p_start = p_mem;

	// Setup pointer to actual data.
	NxNgc::sSceneHeader * p_scene_header = (NxNgc::sSceneHeader *)p_mem;
	unsigned int * p32 = (unsigned int *)&p_scene_header[1];

	// Setup material DL pointer.
	NxNgc::sMaterialDL * blend_dl = (NxNgc::sMaterialDL *)p32; 
	NxNgc::sTextureDL * tex_dl = (NxNgc::sTextureDL *)&blend_dl[p_scene_header->m_num_blend_dls];
	unsigned char * p8 = (unsigned char *)&tex_dl[p_scene_header->m_num_materials];

	int bytes = ( p_scene_header->m_num_blend_dls * sizeof( NxNgc::sMaterialDL ) ) + ( p_scene_header->m_num_materials * sizeof( NxNgc::sTextureDL ) );
	int rounded_bytes = ( bytes + 31 ) & ~31;
	int pad_bytes = rounded_bytes - bytes;

	p8 = &p8[pad_bytes];

	// Assign blend dl pointers.
	for ( uint bdl = 0; bdl < p_scene_header->m_num_blend_dls; bdl++ )
	{
		p8 += blend_dl[bdl].m_dl_size;
	}

	// Assign texture dl pointers.
	for ( uint tdl = 0; tdl < p_scene_header->m_num_materials; tdl++ )
	{
		p8 += tex_dl[tdl].m_dl_size;
	}
	p32 = (unsigned int *)p8;

	// Point past pool data.
	NxNgc::sObjectHeader* p_data = (NxNgc::sObjectHeader*)&((char*)p32)[p_scene_header->m_num_pool_bytes];

	// Setup VC wibble data.
	NxNgc::sMaterialVCWibbleKeyHeader *	p_key_header	= (NxNgc::sMaterialVCWibbleKeyHeader *)p32; 
	NxNgc::sMaterialVCWibbleKey *		p_key			= (NxNgc::sMaterialVCWibbleKey *)&p_key_header[1]; 
	for ( int vc = 0; vc < p_scene_header->m_num_vc_wibbles; vc++ )
	{
		p_key_header = (NxNgc::sMaterialVCWibbleKeyHeader *)&p_key[p_key_header->m_num_frames]; 
		p_key = (NxNgc::sMaterialVCWibbleKey *)&p_key_header[1];  
	}
	p32 = (uint32 *)p_key_header;

	// Setup material header data.
	NxNgc::sMaterialHeader *material_header = (NxNgc::sMaterialHeader *)p32;
	p32 = (uint32 *)&material_header[p_scene_header->m_num_materials];

	// Setup UV wibble data.
	NxNgc::sMaterialUVWibble *uv_wibble = (NxNgc::sMaterialUVWibble *)p32;
	p32 = (uint32 *)&uv_wibble[p_scene_header->m_num_uv_wibbles];

	// Setup material pass data.
	NxNgc::sMaterialPassHeader *material_pass = (NxNgc::sMaterialPassHeader *)p32;
	p32 = (uint32 *)&material_pass[p_scene_header->m_num_pass_items];

	if ( p_scene_header->m_num_pos )
	{
		p32 = &((unsigned int *)p32)[p_scene_header->m_num_pos*3];
	}

	if ( p_scene_header->m_num_nrm )
	{
		p32 = &((unsigned int *)p32)[p_scene_header->m_num_nrm*3];
	}

	if ( p_scene_header->m_num_col )
	{
		p32 = &((unsigned int *)p32)[p_scene_header->m_num_col];
	}

	if ( p_scene_header->m_num_tex )
	{
		p32 = &((unsigned int *)p32)[p_scene_header->m_num_tex*2];
	}

	if ( p_scene_header->m_num_shadow_faces )
	{
		p32 = (unsigned int *)(&((unsigned short *)p32)[p_scene_header->m_num_shadow_faces*3]);
		p32 = (unsigned int *)(&((NxNgc::sShadowEdge *)p32)[p_scene_header->m_num_shadow_faces]);
	}

	for( uint s = 0; s < p_scene_header->m_num_objects; ++s )
	{
		int num_mesh = p_data->m_num_meshes;

		char * p_skin = (char *)&p_data[1];
		int nbytes = p_data->m_skin.num_bytes;
		NxNgc::sDLHeader* p_dl = (NxNgc::sDLHeader*)&p_skin[nbytes];

		for( int m = 0; m < num_mesh; ++m )
		{
			if ( p_dl->m_size )
			{
				p_dl = (NxNgc::sDLHeader*)&(((char *)&p_dl[1])[p_dl->m_size]);
			}
		}

		p_data = (NxNgc::sObjectHeader*)p_dl;
	}

	// Point up hierarchy.
	uint32 * p_h = (uint32 *)p_data;
	uint32 num_hobj = *p_h++;

	CHierarchyObject * p_end = (CHierarchyObject *)p_h;

	p_end = &p_end[num_hobj];

	return (int)p_end - (int)p_start;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static CScene * s_plat_load_scene_guts( void *p_mem, CTexDict *p_tex_dict, bool add_super_sectors, bool is_sky, bool is_dictionary )
{
	if ( s_correctable ) gCorrectColor = true;

	CSector*		pSector;
	CNgcSector*	pNgcSector;

	// Create scene class instance.
	CScene	*new_scene;
	if( add_super_sectors )
	{
		// Use default size sector table.
		new_scene = new CNgcScene();
	}
	else
	{
		// Use minimum size sector table.
		new_scene = new CNgcScene( 1 );
	}

	new_scene->SetInSuperSectors( add_super_sectors );
	new_scene->SetIsSky( is_sky );

	// Get a scene id from the engine.
	CNgcScene *p_new_ngc_scene = static_cast<CNgcScene *>( new_scene );

	// Create a new sScene so the engine can track assets for this scene.
	NxNgc::sScene *p_engine_scene = new NxNgc::sScene();
	p_new_ngc_scene->SetEngineScene( p_engine_scene );

	// Set the dictionary flag.
	p_engine_scene->m_is_dictionary	= is_dictionary;



//------------------------------------------------------------------------------------

	// Setup pointer to actual data.
	p_engine_scene->mp_scene_data = (NxNgc::sSceneHeader *)p_mem;
	unsigned int * p32 = (unsigned int *)&p_engine_scene->mp_scene_data[1];

	// Setup material DL pointer.
	Dbg_MsgAssert( p_engine_scene->mp_scene_data->m_num_blend_dls < 256, ( "Too many (>255) unique material DLs (%d)\n", p_engine_scene->mp_scene_data->m_num_blend_dls ) );
	p_engine_scene->mp_blend_dl = (NxNgc::sMaterialDL *)p32; 
	p_engine_scene->mp_texture_dl = (NxNgc::sTextureDL *)&p_engine_scene->mp_blend_dl[p_engine_scene->mp_scene_data->m_num_blend_dls];
	unsigned char * p8 = (unsigned char *)&p_engine_scene->mp_texture_dl[p_engine_scene->mp_scene_data->m_num_materials];

	int bytes = ( p_engine_scene->mp_scene_data->m_num_blend_dls * sizeof( NxNgc::sMaterialDL ) ) + ( p_engine_scene->mp_scene_data->m_num_materials * sizeof( NxNgc::sTextureDL ) );
	int rounded_bytes = ( bytes + 31 ) & ~31;
	int pad_bytes = rounded_bytes - bytes;

	p8 = &p8[pad_bytes];

	// Assign blend dl pointers.
	for ( uint bdl = 0; bdl < p_engine_scene->mp_scene_data->m_num_blend_dls; bdl++ )
	{
		p_engine_scene->mp_blend_dl[bdl].mp_dl = p8;
		p8 += p_engine_scene->mp_blend_dl[bdl].m_dl_size;
	}

	// Assign texture dl pointers.
	for ( uint tdl = 0; tdl < p_engine_scene->mp_scene_data->m_num_materials; tdl++ )
	{
		p_engine_scene->mp_texture_dl[tdl].mp_dl = p8;
		p8 += p_engine_scene->mp_texture_dl[tdl].m_dl_size;
	}
	p32 = (unsigned int *)p8;

	// Point past pool data.
	NxNgc::sObjectHeader* p_data = (NxNgc::sObjectHeader*)&((char*)p32)[p_engine_scene->mp_scene_data->m_num_pool_bytes];

	// Setup VC wibble data.
	p_engine_scene->mp_vc_wibble = (NxNgc::sMaterialVCWibbleKeyHeader *)p32;
	NxNgc::sMaterialVCWibbleKeyHeader *	p_key_header	= p_engine_scene->mp_vc_wibble;
	NxNgc::sMaterialVCWibbleKey *		p_key			= (NxNgc::sMaterialVCWibbleKey *)&p_key_header[1]; 
	for ( int vc = 0; vc < p_engine_scene->mp_scene_data->m_num_vc_wibbles; vc++ )
	{
		p_key_header = (NxNgc::sMaterialVCWibbleKeyHeader *)&p_key[p_key_header->m_num_frames]; 
		p_key = (NxNgc::sMaterialVCWibbleKey *)&p_key_header[1];  
	}
	p32 = (uint32 *)p_key_header;

	// Setup material header data.
	p_engine_scene->mp_material_header = (NxNgc::sMaterialHeader *)p32;
	p32 = (uint32 *)&p_engine_scene->mp_material_header[p_engine_scene->mp_scene_data->m_num_materials];

	// Setup UV wibble data.
	p_engine_scene->mp_uv_wibble = (NxNgc::sMaterialUVWibble *)p32;
	p32 = (uint32 *)&p_engine_scene->mp_uv_wibble[p_engine_scene->mp_scene_data->m_num_uv_wibbles];

	// Setup material pass data.
	p_engine_scene->mp_material_pass = (NxNgc::sMaterialPassHeader *)p32;
	p32 = (uint32 *)&p_engine_scene->mp_material_pass[p_engine_scene->mp_scene_data->m_num_pass_items];

	// Setup material data texture pointers.
	NxNgc::sMaterialHeader * p_mat = p_engine_scene->mp_material_header;
	for ( unsigned int lp = 0; lp < p_engine_scene->mp_scene_data->m_num_materials; lp++ )
	{
		p_mat->m_alpha_cutoff = p_mat->m_alpha_cutoff >= 128 ? 255 : p_mat->m_alpha_cutoff * 2;
		// Need to compensate for possible errors in alpha map.
//		int cut = (int)p_mat->m_alpha_cutoff - 16;
//		if ( cut < 0 ) cut = 0;
//		p_mat->m_alpha_cutoff = cut;

		// Setup texture pointer.
		NxNgc::sMaterialPassHeader * p_pass = &p_engine_scene->mp_material_pass[p_mat->m_pass_item];

		// Copy base pass blend mode to material.
		p_mat->m_base_blend = p_pass->m_blend_mode;

		for ( int pass = 0; pass < p_mat->m_passes; pass++ )
		{
			// If textured, resolve texture checksum...
			Nx::CNgcTexture *p_ngc_texture = static_cast<Nx::CNgcTexture*>( p_tex_dict->GetTexLookup()->GetItem( p_pass->m_texture.checksum ) );
			p_pass->m_texture.p_data = ( p_ngc_texture ) ? p_ngc_texture->GetEngineTexture() : NULL;

			// Temp HACK
//			p_pass->m_k = (1<<8);
//			p_pass->m_u_tile = (1<<12);
//			p_pass->m_v_tile = (1<<12);
//			p_pass->m_uv_enabled = 0;
//			p_pass->m_uv_mat[0] = (1<<12);
//			p_pass->m_uv_mat[1] = 0;
//			p_pass->m_uv_mat[2] = 0;
//			p_pass->m_uv_mat[3] = 0;







//			if ( p_pass->m_texture.checksum && !p_ngc_texture )
//			{
//				OSReport( "Failed to hook up texture!!!!!!!!!!! %08x\n", p_pass->m_texture.checksum );
//			}

//			// Adjust K value.
//			p_pass->m_mip_k += 8;

			// Switch color values.
//			u8 r;
//			u8 g;
//			u8 b;
//			u8 a;
//			r = p_pass->m_color.r;
//			g = p_pass->m_color.g;
//			b = p_pass->m_color.b;
//			a = p_pass->m_color.a;
//
//			p_pass->m_color.r = b;
//			p_pass->m_color.g = g;
//			p_pass->m_color.b = r;
//			p_pass->m_color.a = a;

//			p_pass->m_color.a = 255;

//			p_pass->m_alpha_fix = ( p_pass->m_alpha_fix ) >= 128 ? 255 : p_pass->m_alpha_fix * 2;

			// Next pass structure.
			++p_pass;
		}

		// Scan texture DL and patch texture address pointers.
		// Search for SetMode0/1 SetImage0/1/2/3.
		NxNgc::sTextureDL * p_dl = &p_engine_scene->mp_texture_dl[lp]; 
		p_pass = &p_engine_scene->mp_material_pass[p_mat->m_pass_item];

//		int search_size = ( p_dl->m_dl_size > ((5*5)+4) ) ? p_dl->m_dl_size - ((5*5)+4) : 0;

//		for ( int s = 0; s < search_size ; s++ )
		//for ( int s = 0; s < (int)p_dl->m_dl_size ; s++ )

		GX::ResolveDLTexAddr( p_dl, p_pass, p_mat->m_passes );

//		GX::begin( p_dl->mp_dl, p_dl->m_dl_size );
//		multi_mesh( p_mat, p_pass, true, true );
//		p_dl->m_dl_size = GX::end();

		++p_mat;
	}

	// Setup vertex data.
	p_engine_scene->mp_pos_pool	= NULL;
	p_engine_scene->mp_nrm_pool	= NULL;
	p_engine_scene->mp_col_pool	= NULL;
	p_engine_scene->mp_tex_pool	= NULL;

	if ( p_engine_scene->mp_scene_data->m_num_pos )
	{
		p_engine_scene->mp_pos_pool = (float *)p32;
		p32 = &((unsigned int *)p32)[p_engine_scene->mp_scene_data->m_num_pos*3];
	}

	if ( p_engine_scene->mp_scene_data->m_num_col )
	{
		p_engine_scene->mp_col_pool = (unsigned int *)p32;
		p32 = &((unsigned int *)p32)[p_engine_scene->mp_scene_data->m_num_col];
	}

	if ( p_engine_scene->mp_scene_data->m_num_tex )
	{
		p_engine_scene->mp_tex_pool = (s16 *)p32;
		p32 = (unsigned int *)&p_engine_scene->mp_tex_pool[p_engine_scene->mp_scene_data->m_num_tex*2];
	}

	if ( p_engine_scene->mp_scene_data->m_num_nrm )
	{
		p_engine_scene->mp_nrm_pool = (s16 *)p32;
		p32 = (unsigned int*)&p_engine_scene->mp_nrm_pool[p_engine_scene->mp_scene_data->m_num_nrm*3];
	}

	if ( p_engine_scene->mp_scene_data->m_num_shadow_faces )
	{
		p_engine_scene->mp_shadow_volume_mesh = (uint16*)p32;
		p32 = (unsigned int *)(&((unsigned short *)p32)[p_engine_scene->mp_scene_data->m_num_shadow_faces*3]);
		p_engine_scene->mp_shadow_edge = (NxNgc::sShadowEdge *)p32; 
		p32 = (unsigned int *)(&((NxNgc::sShadowEdge *)p32)[p_engine_scene->mp_scene_data->m_num_shadow_faces]);
	}

	// Calculate radius.
	p_engine_scene->m_sphere[X] = 0.0f;
	p_engine_scene->m_sphere[Y] = 0.0f;
	p_engine_scene->m_sphere[Z] = 0.0f;
	p_engine_scene->m_sphere[W] = 0.0f;

	for ( uint32 lp = 0; lp < p_engine_scene->mp_scene_data->m_num_pos; lp++ )
	{
		NsVector v;
		v.x = p_engine_scene->mp_pos_pool[(lp*3)+0];
		v.y = p_engine_scene->mp_pos_pool[(lp*3)+1];
		v.z = p_engine_scene->mp_pos_pool[(lp*3)+2];
		float l = v.length();

		if ( l > p_engine_scene->m_sphere[W] ) p_engine_scene->m_sphere[W] = l;
	}

	// Setup display list pointer.
	p_engine_scene->mp_dl = (NxNgc::sDLHeader *)&(((char*)&p_data[1])[p_data->m_skin.num_bytes]);

	for( uint s = 0; s < p_engine_scene->mp_scene_data->m_num_objects; ++s )
	{
		// Create a new sector to hold the incoming details.
		pSector						= p_new_ngc_scene->CreateSector();
		pNgcSector					= static_cast<CNgcSector*>( pSector );

		// Generate a hanging geom for the sector, used for creating level objects etc.
		CNgcGeom	*p_Ngc_geom	= new CNgcGeom();
		p_Ngc_geom->SetScene( p_new_ngc_scene );
		pNgcSector->SetGeom( p_Ngc_geom );

		// Prepare CNgcGeom for receiving data.
		p_Ngc_geom->InitMeshList();

		// Load sector data.
		p_data = pNgcSector->LoadFromFile( p_data );
		new_scene->AddSector( pSector );

//		if( ( p_data = pNgcSector->LoadFromFile( p_file, p_data ) ) )
//		{
//			new_scene->AddSector( pSector );
//		}
	}

	// At this point we can process any scaling that may need to be applied to the positions.
	if ( p_engine_scene->mp_dl->mp_object_header->m_skin.p_data )
	{
		NxNgc::ApplyMeshScaling( p_engine_scene->mp_dl->mp_object_header );
	}

	// Point up hierarchy.
	uint32 * p_h = (uint32 *)p_data;
	uint32 num_hobj = *p_h++;

	if ( num_hobj )
	{
		p_engine_scene->mp_hierarchyObjects = (CHierarchyObject *)p_h;
		p_engine_scene->m_numHierarchyObjects = num_hobj;
		//p32 = (uint32 *)&p_engine_scene->mp_hierarchyObjects[num_hobj];

//		p_engine_scene->mp_hierarchyObjects = new CHierarchyObject[num_hobj];
//
//		File::Read( p_engine_scene->mp_hierarchyObjects, sizeof( CHierarchyObject ), num_hobj, p_file );
//
//		p_engine_scene->m_numHierarchyObjects = num_hobj;

		// Fix up hierarchical object sphere.
		Lst::HashTable< Nx::CSector > *p_sector_list = new_scene->GetSectorList();
		if( p_sector_list )
		{
			p_sector_list->IterateStart();	
			Nx::CSector *p_sector = p_sector_list->IterateNext();
			while( p_sector )
			{
				pNgcSector = static_cast<CNgcSector*>( p_sector );
				CNgcGeom *p_Ngc_geom = static_cast<CNgcGeom *>( pNgcSector->GetGeom() );

				Lst::Head< NxNgc::sMesh >	*p_mesh_list = p_Ngc_geom->GetMeshList();
				int num_mesh = p_mesh_list->CountItems();
				if (num_mesh)
				{
					Lst::Node< NxNgc::sMesh > *mesh = p_mesh_list->GetNext();
					mesh->GetData()->mp_dl->mp_object_header->m_sphere[W] *= 2.0f;  
				}
				p_sector = p_sector_list->IterateNext();
			}
		}
	}

	if ( s_correctable ) gCorrectColor = false;

	return new_scene;

//------------------------------------------------------------------------------------





























//	// Open the scene file.
//	void* p_file = File::Open( p_name, "rb" );
//	if( !p_file )
//	{
//		Dbg_MsgAssert( p_file, ( "Couldn't open scene file %s\n", p_name ));
//		if ( s_correctable ) gCorrectColor = false;
//		return NULL;
//	}
//	
//	// Version numbers.
//	uint32 mat_version, mesh_version, vert_version;
//	File::Read( &mat_version, sizeof( uint32 ), 1, p_file );
//	File::Read( &mesh_version, sizeof( uint32 ), 1, p_file );
//	File::Read( &vert_version, sizeof( uint32 ), 1, p_file );
//	Dbg_Message( "material version %d\n", mat_version );
//	Dbg_Message( "mesh version %d\n", mesh_version );
//	Dbg_Message( "vertex version %d\n", vert_version );
//
//	// Import materials (they will now be associated at the engine-level with this scene).
//	p_engine_scene->mp_material_array = NxNgc::LoadMaterials( p_file, p_tex_dict->GetTexLookup(), &p_engine_scene->m_num_materials );
//
//	// Read number of sectors.
//	int num_sectors;
//	File::Read( &num_sectors, sizeof( int ), 1, p_file );
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//	NxNgc::sDLHeader* p_data = (NxNgc::sDLHeader*)p_engine_scene->mp_dl;
//
//	for( int s = 0; s < num_sectors; ++s )
//	{
//		// Create a new sector to hold the incoming details.
//		pSector						= p_new_ngc_scene->CreateSector();
//		pNgcSector					= static_cast<CNgcSector*>( pSector );
//		
//		// Generate a hanging geom for the sector, used for creating level objects etc.
//		CNgcGeom	*p_Ngc_geom	= new CNgcGeom();
//		p_Ngc_geom->SetScene( p_new_ngc_scene );
//		pNgcSector->SetGeom( p_Ngc_geom );
//		
//		// Prepare CNgcGeom for receiving data.
//		p_Ngc_geom->InitMeshList();
//		
//		// Load sector data.
//		p_data = pNgcSector->LoadFromFile( p_file, p_data );
//		new_scene->AddSector( pSector );
//
////		if( ( p_data = pNgcSector->LoadFromFile( p_file, p_data ) ) )
////		{
////			new_scene->AddSector( pSector );
////		}
//	}
//
//	// Load hierarchy.
//	uint32 num_hobj;
//	File::Read( &num_hobj, sizeof( uint32 ), 1, p_file );
//
//	if ( num_hobj )
//	{
//		p_engine_scene->mp_hierarchyObjects = new CHierarchyObject[num_hobj];
//
//		File::Read( p_engine_scene->mp_hierarchyObjects, sizeof( CHierarchyObject ), num_hobj, p_file );
//
//		p_engine_scene->m_numHierarchyObjects = num_hobj;
//	}
//
//	// At this point get the engine scene to figure it's bounding volumes.
//	p_engine_scene->FigureBoundingVolumes();
//	
//	File::Close( p_file );
//
//	if ( s_correctable ) gCorrectColor = false;
//
//	return new_scene;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CScene *CEngine::s_plat_load_scene_from_memory( void *p_mem, CTexDict *p_tex_dict, bool add_super_sectors, bool is_sky, bool is_dictionary )
{
	int size = s_plat_scene_size( p_mem, p_tex_dict, add_super_sectors, is_sky, is_dictionary );

	int mem_available;
	bool need_to_pop = false;
	if ( g_in_cutscene )
	{
		Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().FrontEndHeap() );
		mem_available = Mem::Manager::sHandle().Available();
		if ( size < ( mem_available - ( 40 * 1024 ) ) )
		{
			need_to_pop = true;
		}
		else
		{
			Mem::Manager::sHandle().PopContext();
			Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ThemeHeap() );
			mem_available = Mem::Manager::sHandle().Available();
			if ( size < ( mem_available - ( 5 * 1024 ) ) )
			{
				need_to_pop = true;
			}
			else
			{
				Mem::Manager::sHandle().PopContext();
				Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ScriptHeap() );
				mem_available = Mem::Manager::sHandle().Available();
				if ( size < ( mem_available - ( 40 * 1024 ) ) )
				{
					need_to_pop = true;
				}
				else
				{
					Mem::Manager::sHandle().PopContext();
				}
			}
		}
	}

	Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
	char * p_scene_data = new char[size]; 
	Mem::Manager::sHandle().BottomUpHeap()->PopAlign();

	if ( need_to_pop )
	{
		Mem::Manager::sHandle().PopContext();
	}

	memcpy( p_scene_data, p_mem, size );
	DCFlushRange( p_scene_data, size );

	return s_plat_load_scene_guts( p_scene_data, p_tex_dict, add_super_sectors, is_sky, is_dictionary );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CScene*	CEngine::s_plat_load_scene( const char *p_name, CTexDict *p_tex_dict, bool add_super_sectors, bool is_sky, bool is_dictionary )
{
	Dbg_Message( "loading scene from file %s\n", p_name );

	// Open the scene file.
	char gdname[128];
	sprintf( gdname, "%s", p_name );
	void* p_gd_file = File::Open( gdname, "rb" );
	if( !p_gd_file )
	{
		Dbg_MsgAssert( p_gd_file, ( "Couldn't open scene file %s\n", gdname ));
		if ( s_correctable ) gCorrectColor = false;
		return NULL;
	}

	// Read all data.
	int size = File::GetFileSize( p_gd_file );
	Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
	char * p_scene_data = new char[size]; 
	Mem::Manager::sHandle().BottomUpHeap()->PopAlign();
	File::Read( p_scene_data, size, 1, p_gd_file );
	
	File::Close( p_gd_file );

	DCFlushRange( p_scene_data, size );
	return s_plat_load_scene_guts( p_scene_data, p_tex_dict, add_super_sectors, is_sky, is_dictionary );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CEngine::s_plat_add_scene( CScene *p_scene, const char *p_filename )
{
	// Function to incrementally add geometry to a scene - should NOT be getting called on GameCube.
	Dbg_Assert( 0 );
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CEngine::s_plat_unload_scene( CScene *p_scene )
{
	Dbg_MsgAssert( p_scene,( "Trying to delete a NULL scene" ));

	CNgcScene *p_ngc_scene = (CNgcScene*)p_scene;

	// Ask the engine to remove the associated meshes for each sector in the scene.
	p_ngc_scene->DestroySectorMeshes();

	// Get the engine specific scene data and pass it to the engine to delete.
	NxNgc::DeleteScene( p_ngc_scene->GetEngineScene());
	p_ngc_scene->SetEngineScene( NULL );
	
	return true;
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CModel*		CEngine::s_plat_init_model( void )
{
	CNgcModel* pModel = new CNgcModel;

	return pModel;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CEngine::s_plat_uninit_model(CModel* pModel)
{
	Dbg_Assert( pModel );

	delete pModel;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CMesh* CEngine::s_plat_load_mesh( const char* pMeshFileName, Nx::CTexDict* pTexDict, uint32 texDictOffset, bool isSkin, bool doShadowVolume )
{
	s_correctable = false;
	// Load the scene.
	Nx::CScene *p_scene = Nx::CEngine::s_plat_load_scene( pMeshFileName, pTexDict, false, false, false );
	p_scene->SetID(Script::GenerateCRC( pMeshFileName )); 	// store the checksum of the scene name
	p_scene->SetTexDict( pTexDict );
	p_scene->PostLoad( pMeshFileName );
	
	// Disable any scaling.
	NxNgc::DisableMeshScaling();
	
	CNgcMesh *pMesh = new CNgcMesh( pMeshFileName );
	Nx::CNgcScene *p_Ngc_scene = static_cast<Nx::CNgcScene*>( p_scene );
	pMesh->SetScene( p_Ngc_scene );
	pMesh->SetTexDict( pTexDict );
	s_correctable = true;
	return pMesh;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CMesh* CEngine::s_plat_load_mesh( uint32 id, uint32 *p_model_data, int model_data_size, uint8 *p_cas_data, Nx::CTexDict* pTexDict, uint32 texDictOffset, bool isSkin, bool doShadowVolume )
{
	// Convert the id into a usable string.
	Dbg_Assert( id > 0 );
	char id_as_string[16];
	sprintf( id_as_string, "%d\n", id );

	// Load the scene.
	Nx::CScene *p_scene = Nx::CEngine::s_plat_load_scene_from_memory( p_model_data, pTexDict, false, false, false );

	// Store the checksum of the scene name.
	p_scene->SetID( Script::GenerateCRC( id_as_string ));

	p_scene->SetTexDict( pTexDict );
	p_scene->PostLoad( id_as_string );
	
	// Disable any scaling.
	NxNgc::DisableMeshScaling();
	
	CNgcMesh *pMesh = new CNgcMesh();

	// Set CAS data for mesh.
	pMesh->SetCASData( p_cas_data );

	Nx::CNgcScene *p_Ngc_scene = static_cast<Nx::CNgcScene*>( p_scene );
	pMesh->SetScene( p_Ngc_scene );
	pMesh->SetTexDict( pTexDict );
	return pMesh;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CEngine::s_plat_unload_mesh(CMesh* pMesh)
{
	Dbg_Assert( pMesh );

	delete pMesh;

	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_set_mesh_scaling_parameters( SMeshScalingParameters* pParams )
{
	NxNgc::SetMeshScalingParameters( pParams );
}



///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//CTexDict* CEngine::s_plat_load_textures( const char* p_name )
//{
////	NxNgc::LoadTextureFile( p_name );
//	return NULL;
//}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CSprite *	CEngine::s_plat_create_sprite(CWindow2D *p_window)
{
	return new CNgcSprite;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CEngine::s_plat_destroy_sprite( CSprite *p_sprite )
{
	delete p_sprite;
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTextured3dPoly *	CEngine::s_plat_create_textured_3d_poly()
{
	return new NxNgc::CNgcTextured3dPoly;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CEngine::s_plat_destroy_textured_3d_poly(CTextured3dPoly *p_poly)
{
	delete p_poly;
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Nx::CTexture *CEngine::s_plat_create_render_target_texture( int width, int height, int depth, int z_depth )
{
	// Create the CNgcTexture (just a container class for the NxNgc::sTexture).
	CNgcTexture *p_texture = new CNgcTexture();

	// Create the NxNgc::sTexture.
	NxNgc::sTexture *p_engine_texture = new NxNgc::sTexture;
	p_texture->SetEngineTexture( p_engine_texture );
	
	// Set the texture as a render target.
	p_engine_texture->SetRenderTarget( width, height, depth, z_depth );
	
	return p_texture;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CEngine::s_plat_project_texture_into_scene( Nx::CTexture *p_texture, Nx::CModel *p_model, Nx::CScene *p_scene )
{
	Nx::CNgcTexture		*p_ngc_texture	= static_cast<CNgcTexture*>( p_texture );
	Nx::CNgcModel		*p_ngc_model	= static_cast<CNgcModel*>( p_model );
//	Nx::CNgcScene		*p_ngc_scene	= static_cast<CNgcScene*>( p_scene );
//	NxNgc::create_texture_projection_details( p_ngc_texture->GetEngineTexture(), p_ngc_model, p_ngc_scene->GetEngineScene());
	NxNgc::create_texture_projection_details( p_ngc_texture->GetEngineTexture(), p_ngc_model, NULL);
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CEngine::s_plat_set_projection_texture_camera( Nx::CTexture *p_texture, Gfx::Camera *p_camera )
{
	Nx::CNgcTexture		*p_ngc_texture	= static_cast<CNgcTexture*>( p_texture );
	NsVector			pos( p_camera->GetPos()[X], p_camera->GetPos()[Y], p_camera->GetPos()[Z] );
	NsVector			at( pos.x + p_camera->GetMatrix()[Z][X], pos.y + p_camera->GetMatrix()[Z][Y], pos.z + p_camera->GetMatrix()[Z][Z] );
	
	NxNgc::set_texture_projection_camera( p_ngc_texture->GetEngineTexture(), &pos, &at );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CEngine::s_plat_stop_projection_texture( Nx::CTexture *p_texture )
{
	Nx::CNgcTexture *p_ngc_texture = static_cast<CNgcTexture*>( p_texture );
	NxNgc::destroy_texture_projection_details( p_ngc_texture->GetEngineTexture());
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_add_occlusion_poly( uint32 num_verts, Mth::Vector *p_vert_array, uint32 checksum )
{
	if( num_verts == 4 )
	{
		NxNgc::AddOcclusionPoly( p_vert_array[0], p_vert_array[1], p_vert_array[2], p_vert_array[3], checksum );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_enable_occlusion_poly( uint32 checksum, bool enable )
{
	NxNgc::EnableOcclusionPoly( checksum, enable );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_remove_all_occlusion_polys( void )
{
	NxNgc::RemoveAllOcclusionPolys();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// returns true if the sphere at "center", with the "radius"
// is visible to the current camera
// (note, currently this is the last frame's camera on PS2)

bool CEngine::s_plat_is_visible( Mth::Vector&	center, float radius  )
{
	Mth::Vector v;
	v[X] = center[X];
	v[Y] = center[Y];
	v[Z] = center[Z];
	v[W] = radius;

	return NxNgc::IsVisible( v );
}




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const char* CEngine::s_plat_get_platform_extension( void )
{
	// String literals are statically allocated so can be returned safely, (Bjarne, p90)
	return "ngc";
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGeom* CEngine::s_plat_init_geom( void )
{
	CNgcGeom* pGeom = new CNgcGeom;

	return pGeom;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CEngine::s_plat_uninit_geom(CGeom* pGeom)
{
	Dbg_Assert( pGeom );

	delete pGeom;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CQuickAnim* CEngine::s_plat_init_quick_anim()
{
	CQuickAnim* pQuickAnim = new CQuickAnim;

	return pQuickAnim;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CEngine::s_plat_uninit_quick_anim(CQuickAnim* pQuickAnim)
{
	Dbg_Assert( pQuickAnim );

	delete pQuickAnim;

	return;
}

/******************************************************************/
// Wait for any pending asyncronous rendering to finish, so rendering
// data can be unloaded
/******************************************************************/

void CEngine::s_plat_finish_rendering()
{
	// TODO:  Flush pending rendering, so data can be unloaded
	NsDisplay::flush();
} 

/******************************************************************/
// Set the amount that the previous frame is blended with this frame
// 0 = none	  	(just see current frame) 	
// 128 = 50/50
// 255 = 100% 	(so you only see the previous frame)												  
/******************************************************************/

void CEngine::s_plat_set_screen_blur(uint32 amount )
{
//	g_blur = ( amount * 8 ) / 255;
//	sBlur = amount;
} 


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	s_plat_get_num_soundtracks()
{
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const char* s_plat_get_soundtrack_name( int soundtrack_number )
{
	return NULL;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int	CEngine::s_plat_get_num_soundtracks( void )
{
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const char* CEngine::s_plat_get_soundtrack_name( int soundtrack_number )
{
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_set_max_multipass_distance(float dist)
{
//	NxPs2::render::sMultipassMaxDist = dist;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_set_letterbox( bool letterbox )
{
	// Letterbox mode is designed for use on a regular 4:3 screen.
	// It should use the same, wider viewing angle as for widescreen mode, but shrink the resultant image down
	// vertically by 25%.
	if( letterbox )
	{
		if( NxNgc::EngineGlobals.letterbox_active == false )
		{
			// Need to adjust the screen y offset and multiplier to ensure sprites are scaled properly for this mode.
//			NxNgc::EngineGlobals.screen_conv_y_offset		+= ( NxXbox::EngineGlobals.backbuffer_height / 4 ) / 2;
//			NxNgc::EngineGlobals.screen_conv_y_multiplier	= 0.75f;
			NxNgc::EngineGlobals.letterbox_active			= true;
		}
	}
	else
	{
		if( NxNgc::EngineGlobals.letterbox_active == true )
		{
			// Restore the screen y offset and multiplier.
//			NxNgc::EngineGlobals.screen_conv_y_offset		-= ( NxXbox::EngineGlobals.backbuffer_height / 4 ) / 2;
//			NxNgc::EngineGlobals.screen_conv_y_multiplier	= 1.0f;
			NxNgc::EngineGlobals.letterbox_active			= false;
		}
	}
} 
 

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_set_color_buffer_clear( bool clear )
{
}



} // namespace Nx


