#include <libgraph.h>
#include <libdma.h>
#include <core\math.h>
#include <core\defines.h>
#include "dma.h"
#include "vif.h"
#include "gif.h"
#include "gs.h"
#include "dmacalls.h"
#include "group.h"
#include "nx_init.h"
#include "render.h"
#include "interrupts.h"
#include "switches.h"
#include "sprite.h"


namespace NxPs2
{


// Callback handler for loading bar
static int s_loading_bar_handler_id = -1;

/////////////////////////////////////////////////////////////////////////////
// Loading bar

static uint128 p_VifData[64];

int			gLoadBarTotalFrames;		// Number of frames it takes for loading bar to go to 100%
int			gLoadBarNumFrames;			// Number of frames so far
float		gLoadBarX = 150.0f;			// Bar position
float		gLoadBarY = 400.0f;
float		gLoadBarWidth = 340.0f;		// Bar size
float		gLoadBarHeight = 20.0f;
int			gLoadBarStartColor[4] = {  255,    0,   0,  128 };
int			gLoadBarDeltaColor[4] = { -255,  255,   0,  128 };
float		gLoadBarBorderWidth = 5.0f;	// Border width
float		gLoadBarBorderHeight = 5.0f;	// Border height
int			gLoadBarBorderColor[4] = {  40, 40, 40, 128 };


static int s_loading_bar_handler(int ca)
{
	// If writing to a variable, make sure it is volatile (or game will crash)
	
	// inc frame count    
	gLoadBarNumFrames++;

	//-----------------------------------------
	//		DRAW BAR
	//-----------------------------------------
	sceGsSyncPath(0, 0);
	if (sGroup::pRenderGroup)	// Can't handle this condition
	{
		ExitHandler();
		return 0;
	}

	// Save floats
    unsigned int floatBuffer[32];
    saveFloatRegs(floatBuffer);

	// Save all these because we could already be in the middle of building packets
	uint8*	old_dma_loc = dma::pLoc;		  
	uint8*	old_dma_tag = dma::pTag;
	uint8*	old_gif_tag = gif::pTag;
	uint8*	old_vif_code = vif::pVifCode;

	dma::pLoc = (uint8 *) p_VifData;

	// begin Path3 subroutine
	dma::BeginTag(dma::cnt, 0);

	// send everything PATH3
	//vif::BeginDIRECT();
	vif::NOP();
	vif::NOP();

	// GS prim sets up all the registers
	gif::BeginTag2(gs::A_D, 1, PACKED, 0, 0);
	gs::Reg2(gs::ALPHA_1,		PackALPHA(0,1,0,1,128));
	gs::Reg2(gs::CLAMP_1,		PackCLAMP(REPEAT,REPEAT,0,0,0,0));
	gs::Reg2(gs::COLCLAMP,		PackCOLCLAMP(1));
	//gs::Reg2(gs::DTHE,			PackDTHE(0));
	gs::Reg2(gs::DIMX,			PackDIMX(-4,0,-3,1,2,-2,3,-1,-3,1,-4,0,3,-1,2,-2));
	gs::Reg2(gs::DTHE,			PackDTHE(1));
	//gs::Reg2(gs::FBA_1,			PackFBA(0));
	gs::Reg2(gs::FRAME_1,		PackFRAME(DISPLAY_START,HRES/64,PSMCT16S,0xFF000000));	// draw directly to display buffer
	gs::Reg2(gs::PRMODECONT,	PackPRMODECONT(1));
	gs::Reg2(gs::SCANMSK,		PackSCANMSK(0));
	gs::Reg2(gs::SCISSOR_1,	 	PackSCISSOR(0,HRES - 1,0,VRES - 1));
	gs::Reg2(gs::TEST_1,		PackTEST(0,0,0,0,0,0,1,ZALWAYS));
	gs::Reg2(gs::XYOFFSET_1,	PackXYOFFSET(0x0, 0x0));
	gs::Reg2(gs::ZBUF_1,		PackZBUF(ZBUFFER_START,PSMZ24,1));
	gif::EndTag2(1);

	// Find scaled values (mainly for PAL)
	NxPs2::SDraw2D::CalcScreenScale();		// May not have been calculated yet
	float scaledBarX = gLoadBarX * NxPs2::SDraw2D::GetScreenScaleX();
	float scaledBarY = gLoadBarY * NxPs2::SDraw2D::GetScreenScaleY();
	float scaledBarWidth = gLoadBarWidth * NxPs2::SDraw2D::GetScreenScaleX();
	float scaledBarHeight = gLoadBarHeight * NxPs2::SDraw2D::GetScreenScaleY();
	float scaledBarBorderWidth = gLoadBarBorderWidth * NxPs2::SDraw2D::GetScreenScaleX();
	float scaledBarBorderHeight = gLoadBarBorderHeight * NxPs2::SDraw2D::GetScreenScaleY();

	int cur_width = (int) ((scaledBarWidth - 1.0f) * 16.0f);
	if (gLoadBarNumFrames < gLoadBarTotalFrames)
	{
		cur_width = (cur_width * gLoadBarNumFrames) / gLoadBarTotalFrames;
	}

	int x1 = (int) (scaledBarX * 16.0f);
	int y1 = (int) (scaledBarY * 16.0f);
	int x2 = x1 + cur_width;
	int y2 = y1 + (int) ((scaledBarHeight - 1.0f) * 16.0f);

	int end_color[4];
	if (gLoadBarNumFrames < gLoadBarTotalFrames)
	{
		end_color[0] = gLoadBarStartColor[0] + ((gLoadBarDeltaColor[0] * gLoadBarNumFrames) / gLoadBarTotalFrames);
		end_color[1] = gLoadBarStartColor[1] + ((gLoadBarDeltaColor[1] * gLoadBarNumFrames) / gLoadBarTotalFrames);
		end_color[2] = gLoadBarStartColor[2] + ((gLoadBarDeltaColor[2] * gLoadBarNumFrames) / gLoadBarTotalFrames);
		end_color[3] = gLoadBarStartColor[3] + ((gLoadBarDeltaColor[3] * gLoadBarNumFrames) / gLoadBarTotalFrames);
	} else {
		end_color[0] = gLoadBarStartColor[0] + gLoadBarDeltaColor[0];
		end_color[1] = gLoadBarStartColor[1] + gLoadBarDeltaColor[1];
		end_color[2] = gLoadBarStartColor[2] + gLoadBarDeltaColor[2];
		end_color[3] = gLoadBarStartColor[3] + gLoadBarDeltaColor[3];
	}

	int border_x1 = x1 - (int) (scaledBarBorderWidth * 16.0f);
	int border_y1 = y1 - (int) (scaledBarBorderHeight * 16.0f);
	int border_x2 = x1 + (int) (((scaledBarWidth - 1) * 16.0f) + (scaledBarBorderWidth * 16.0f));
	int border_y2 = y2 + (int) (scaledBarBorderHeight * 16.0f);

	gif::BeginTag2(gs::XYZ2<<4|gs::RGBAQ, 2, PACKED, TRISTRIP|IIP|FST|AA1/*|ABE*/, 1);

	// Border
	vif::StoreV4_32(gLoadBarBorderColor[0], gLoadBarBorderColor[1],
					gLoadBarBorderColor[2], gLoadBarBorderColor[3]);	// RGBAQ
	vif::StoreV4_32(border_x1, border_y1, 0, 0x8000);					// XYZ2
	vif::StoreV4_32(gLoadBarBorderColor[0], gLoadBarBorderColor[1],
					gLoadBarBorderColor[2], gLoadBarBorderColor[3]);	// RGBAQ
	vif::StoreV4_32(border_x1, border_y2, 0, 0x8000);					// XYZ2
	vif::StoreV4_32(gLoadBarBorderColor[0], gLoadBarBorderColor[1],
					gLoadBarBorderColor[2], gLoadBarBorderColor[3]);	// RGBAQ
	vif::StoreV4_32(border_x2, border_y1, 0, 0);						// XYZ2
	vif::StoreV4_32(gLoadBarBorderColor[0], gLoadBarBorderColor[1],
					gLoadBarBorderColor[2], gLoadBarBorderColor[3]);	// RGBAQ
	vif::StoreV4_32(border_x2, border_y2, 0, 0);						// XYZ2

	// Bar
	vif::StoreV4_32(gLoadBarStartColor[0], gLoadBarStartColor[1],
					gLoadBarStartColor[2], gLoadBarStartColor[3]);			// RGBAQ
	vif::StoreV4_32( x1,  y1, 0, 0x8000);									// XYZ2
	vif::StoreV4_32(gLoadBarStartColor[0], gLoadBarStartColor[1],
					gLoadBarStartColor[2], gLoadBarStartColor[3]);			// RGBAQ
	vif::StoreV4_32( x1,  y2, 0, 0x8000);									// XYZ2
	vif::StoreV4_32(end_color[0], end_color[1], end_color[2], end_color[3]);// RGBAQ
	vif::StoreV4_32( x2,  y1, 0, 0);										// XYZ2
	vif::StoreV4_32(end_color[0], end_color[1], end_color[2], end_color[3]);// RGBAQ
	vif::StoreV4_32( x2,  y2, 0, 0);										// XYZ2

	gif::EndTag2(1);

	//vif::EndDIRECT();

	dma::EndTag();

	// done
	iFlushCache(WRITEBACK_DCACHE);

	// kick dma to upload textures from this group
	*D2_QWC = 0;						// must zero QWC because the first action will be to use current MADR & QWC
	*D2_TADR = (uint)p_VifData;			// address of 1st tag
	*D2_CHCR = 0x145;					// start transfer, tte=1, chain mode, from memory
	sceGsSyncPath(0, 0);

	// Restore used variables
	dma::pLoc = old_dma_loc;		  
	dma::pTag = old_dma_tag;
	gif::pTag = old_gif_tag;
	vif::pVifCode = old_vif_code;

    // restore floats
    restoreFloatRegs(floatBuffer);

	ExitHandler();			// Mick: Sony requires interrupts to end with an ExitHandler() call
							// this enables interrupts	
	return 0;
}

void StartLoadingBar(int frames)
{
	//Dbg_Assert(s_loading_bar_handler_id < 0);

	if (s_loading_bar_handler_id < 0)
	{
		gLoadBarTotalFrames = frames;
		gLoadBarNumFrames = 0;

		s_loading_bar_handler_id = AddIntcHandler(INTC_VBLANK_S, s_loading_bar_handler, -1);
		Dbg_MsgAssert (( s_loading_bar_handler_id >= 0 ),( "Failed to add loading bar handler" )); 
	}
}

void RemoveLoadingBar()
{
	if (s_loading_bar_handler_id >= 0)
	{
		RemoveIntcHandler(INTC_VBLANK_S, s_loading_bar_handler_id);
		s_loading_bar_handler_id = -1;
	}
}


} // namespace NxPs2

