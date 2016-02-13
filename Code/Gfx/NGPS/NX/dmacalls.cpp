#include <core/defines.h>
#include <math.h>
#include <sifdev.h>
#include <stdio.h>
#include "dma.h"
#include "vif.h"
#include "vu1.h"
#include "gif.h"
#include "gs.h"
#include "material.h"
#include "dmacalls.h"
#include "vu1code.h"
#include "switches.h"
#include "render.h"

namespace NxPs2
{


uint8 *pSubroutine;


uint32	*p_patch_PRMODE;
uint32	*p_patch_ALPHA;
uint32	*p_patch_ALPHA2;

//-----------------------------------------------------------
//		H A N D L I N G   D M A   S U B R O U T I N E S
//-----------------------------------------------------------


// begin a subroutine

uint8 *BeginDmaSubroutine(void)
{
	dma::Align(0,16);
	pSubroutine = dma::pLoc;
	vu1::Loc = vu1::Buffer = 0;
	return pSubroutine;
}


// end a subroutine and return its address

void EndDmaSubroutine(void)
{
	((uint16 *)pSubroutine)[1] |= vu1::Loc&0x3FF;
}

#if 0

// call a subroutine, Path1

void Gosub1(uint Num)
{
	uint8 *pSub = Subroutines[Num];
	dma::Tag(dma::call, 0, (uint)pSub);
	vif::BASE(vu1::Loc);
	vif::OFFSET(0);
	vu1::Loc += ((uint16 *)pSub)[1];
}


// call a subroutine, Path2

void Gosub2(uint Num)
{
	dma::Tag(dma::call, 0, (uint)Subroutines[Num]);
	vif::NOP();
	vif::NOP();
}

#endif



//-----------------------------------------
//		D M A   S U B R O U T I N E S
//-----------------------------------------



void BuildDmaSubroutines(void)
{
	//-------------------------------
	//		C L E A R   V R A M
	//-------------------------------

	dma::BeginSub(dma::next);
	dma::BeginTag(dma::end,0);
	vif::BeginDIRECT();
	gif::BeginTag2(gs::A_D, 1, PACKED, SPRITE, 1);
	gs::Reg2(gs::FRAME_1,		PackFRAME(0,16,PSMCT32,0));
	gs::Reg2(gs::ZBUF_1,		PackZBUF(0,0,1));
	gs::Reg2(gs::TEST_1,		PackTEST(0,0,0,0,0,0,1,ZALWAYS));
	gs::Reg2(gs::XYOFFSET_1,	PackXYOFFSET(0,0));
	gs::Reg2(gs::SCISSOR_1,		PackSCISSOR(0,0x7FF,0,0x7FF));
	gs::Reg2(gs::PRMODECONT,	PackPRMODECONT(1));
	gs::Reg2(gs::SCANMSK,		PackSCANMSK(0));
	gs::Reg2(gs::FBA_1,			PackFBA(0));
	gs::Reg2(gs::DTHE,			PackDTHE(0));
	gs::Reg2(gs::TEXA,			PackTEXA(0x00,0,0x80));
	gs::Reg2(gs::RGBAQ,			PackRGBAQ(0,0,0,0,0));
	gs::Reg2(gs::XYZ2,			PackXYZ(0,0,0));
	gs::Reg2(gs::XYZ2,			PackXYZ(0x3FFF,0x3FFF,0));
	gs::Reg2(gs::TEXFLUSH,		0);
	gif::EndTag2(1);
	vif::EndDIRECT();
	vif::FLUSH();
	dma::EndTag();
	dma::Gosubs[CLEAR_VRAM] = dma::EndSub();



	//---------------------------------------
	//		F L I P   A N D   C L E A R
	//---------------------------------------

	#define CLEAR_COLOUR 0x00706050
	
	uint32 screen_w = HRES << 4;
	uint32 screen_h = VRES << 4;

	// begin Path2 subroutine
	dma::BeginSub(dma::ref);

	// wait in case something's still drawing from last frame
	vif::FLUSHA();

	// send everything PATH2
	vif::BeginDIRECT();

	// register setup
	gif::BeginTag2(gs::A_D, 1, PACKED, 0, 0);
	p_patch_ALPHA = (uint32*) (dma::pLoc + 4);
	gs::Reg2(gs::ALPHA_1,		PackALPHA(0,1,2,1,128));		// tweak this alpha for motion blur!
	gs::Reg2(gs::DIMX,			PackDIMX(-4,0,-3,1,2,-2,3,-1,-3,1,-4,0,3,-1,2,-2));
	gs::Reg2(gs::DTHE,			PackDTHE(1));
	gs::Reg2(gs::FBA_1,			PackFBA(0));
	gs::Reg2(gs::FRAME_1,		PackFRAME(DISPLAY_START,HRES/64,PSMCT16S,0x00000000));
	gs::Reg2(gs::PRMODECONT,	PackPRMODECONT(1));
	gs::Reg2(gs::SCANMSK,		PackSCANMSK(0));
	gs::Reg2(gs::SCISSOR_1,		PackSCISSOR(0,HRES - 1,0,VRES - 1));
	gs::Reg2(gs::TEST_1,		PackTEST(0,0,0,0,0,0,1,ZALWAYS));
	gs::Reg2(gs::TEX0_1,		PackTEX0(0,HRES/64,PSMCT32,10,10,1,DECAL,0,0,0,0,0));
	gs::Reg2(gs::TEX1_1,		PackTEX1(1,0,NEAREST,NEAREST,0,0,0));
	gs::Reg2(gs::TEXFLUSH,		0);
	gs::Reg2(gs::XYOFFSET_1,	PackXYOFFSET(0,0));
	gs::Reg2(gs::ZBUF_1,		PackZBUF(ZBUFFER_START,PSMZ24,0));
	gif::EndTag2(0);

	// using a set of screen-high textured sprites, copy draw buffer to disp buffer and clear z-buffer
	gif::BeginTag2(gs::UV|gs::XYZ2<<4, 2, PACKED, SPRITE|TME|FST|ABE, 1);
	for (uint32 x=0x0000; x<screen_w; x+=0x0200)
	{
		vif::StoreV4_32(x+8,      0x0008,     0, 0);		// UV
		vif::StoreV4_32(x,        0x0000,     0, 0);		// XYZ2
		vif::StoreV4_32(x+0x0208, screen_h+8, 0, 0);		// UV
		vif::StoreV4_32(x+0x0200, screen_h,   0, 0);		// XYZ2
	}
	gif::EndTag2(0);

	#if 1	// switch this off if we don't need to clear the screen
	// set up state for screen clear
	gif::BeginTag2(gs::A_D, 1, PACKED, 0, 0);
	gs::Reg2(gs::DTHE,			PackDTHE(0));
	gs::Reg2(gs::FRAME_1,		PackFRAME(0,HRES/64,PSMCT32,0x00000000));
	gs::Reg2(gs::ZBUF_1,		PackZBUF(FRAME_START + (FRAME_SIZE / 2),PSMZ32,0));
	gs::Reg2(gs::RGBAQ,			CLEAR_COLOUR);
	gif::EndTag2(1);

	// an optimised screen clear that sneakily uses the z-buffer to
	// clear the lower half of the screen
	gif::BeginTag2(gs::XYZ2, 1, PACKED, SPRITE, 1);
	for (uint32 x=0x0000; x<screen_w; x+=0x0200)
	{
		vif::StoreV4_32(x,        0x0000    , 0, 			0);		// XYZ2
		vif::StoreV4_32(x+0x0200, screen_h/2, CLEAR_COLOUR, 0);		// XYZ2
	}
	gif::EndTag2(1);
	#endif

	vif::EndDIRECT();

	// done
	dma::Gosubs[FLIP_N_CLEAR] = dma::EndSub();
	



	//---------------------------------------------------------
	//		F L I P   A N D   C L E A R	  L E T T E R B O X E D
	//---------------------------------------------------------


	// begin Path2 subroutine
	dma::BeginSub(dma::ref);

	// wait in case something's still drawing from last frame
	vif::FLUSH();

	// send everything PATH2
	vif::BeginDIRECT();


	// Render in letterboxed mode
	// Need to crop 0.25 of the height, so 0.125 off top and bottom
	
	float crop = screen_h * 0.125;
//	float top_x = 0;
//	float bot_x = 0;
	float top_y = crop;
	float bot_y = screen_h-crop;
	





	// register setup
	gif::BeginTag2(gs::A_D, 1, PACKED, 0, 0);
	p_patch_ALPHA2 = (uint32*) (dma::pLoc + 4);
	gs::Reg2(gs::ALPHA_1,		PackALPHA(0,1,2,1,128));		// tweak this alpha for motion blur!
	gs::Reg2(gs::DIMX,			PackDIMX(-4,0,-3,1,2,-2,3,-1,-3,1,-4,0,3,-1,2,-2));
	gs::Reg2(gs::DTHE,			PackDTHE(1));
	gs::Reg2(gs::FBA_1,			PackFBA(0));
	gs::Reg2(gs::FRAME_1,		PackFRAME(DISPLAY_START,HRES/64,PSMCT16S,0x00000000));
	gs::Reg2(gs::PRMODECONT,	PackPRMODECONT(1));
	gs::Reg2(gs::RGBAQ,			PackRGBAQ(0,0,0,0,0));
	gs::Reg2(gs::SCANMSK,		PackSCANMSK(0));
	gs::Reg2(gs::SCISSOR_1,		PackSCISSOR(0,HRES - 1,0,VRES - 1));
	gs::Reg2(gs::TEST_1,		PackTEST(0,0,0,0,0,0,1,ZALWAYS));
	gs::Reg2(gs::TEX0_1,		PackTEX0(0,HRES/64,PSMCT32,10,10,1,DECAL,0,0,0,0,0));
	gs::Reg2(gs::TEX1_1,		PackTEX1(1,0,NEAREST,NEAREST,0,0,0));
	gs::Reg2(gs::TEXFLUSH,		0);
	gs::Reg2(gs::XYOFFSET_1,	PackXYOFFSET(0,0));
	gs::Reg2(gs::ZBUF_1,		PackZBUF(ZBUFFER_START,PSMZ24,0));
	gif::EndTag2(0);


// Clear the black bars on the screen

	gif::BeginTag2(gs::XYZ2, 1, PACKED, SPRITE, 1);
	for (uint32 x=0x0000; x<screen_w; x+=0x0200)
	{
		vif::StoreV4_32(x,        0x0000    , 0, 0);		// XYZ2
		vif::StoreV4_32(x+0x0200, top_y, 0, 0);				// XYZ2
	}
	for (uint32 x=0x0000; x<screen_w; x+=0x0200)
	{
		vif::StoreV4_32(x,        bot_y   , 0, 0);		// XYZ2
		vif::StoreV4_32(x+0x0200, screen_h, 0, 0);			// XYZ2
	}
	gif::EndTag2(1);


//	float step = (bot_x - top_x)/16;
	
	// screen_w will be either 640*16 = 10240 = 0x2800
	//                      or 512*16 =  8192 = 0x2000
	 

	
	// using a set of screen-high textured sprites, copy draw buffer to disp buffer and clear z-buffer
	gif::BeginTag2(gs::UV|gs::XYZ2<<4, 2, PACKED, SPRITE|TME|FST|ABE, 1);
	for (uint32 x=0x0000; x<screen_w; x+=0x0200)
	{
		vif::StoreV4_32(x+8,      0x0008,     0, 0);		// UV
		vif::StoreV4_32(x,        top_y	,     0, 0);		// XYZ2
		vif::StoreV4_32(x+0x0208, screen_h+8, 0, 0);		// UV
		vif::StoreV4_32(x+0x0200, bot_y,   0, 0);		// XYZ2
	}									  
	gif::EndTag2(0);

	// register setup
	gif::BeginTag2(gs::A_D, 1, PACKED, 0, 0);
	gs::Reg2(gs::DTHE,			PackDTHE(0));
	gs::Reg2(gs::FRAME_1,		PackFRAME(ZBUFFER_START,HRES/64,PSMCT32,0xFF000000));
	gs::Reg2(gs::PRMODECONT,	PackPRMODECONT(1));
	gs::Reg2(gs::RGBAQ,			0);
	gs::Reg2(gs::SCANMSK,		PackSCANMSK(0));
	gs::Reg2(gs::SCISSOR_1,		PackSCISSOR(0,HRES - 1,0,VRES - 1));
	gs::Reg2(gs::TEST_1,		PackTEST(0,0,0,0,0,0,1,ZALWAYS));
	gs::Reg2(gs::XYOFFSET_1,	PackXYOFFSET(0,0));
	gs::Reg2(gs::ZBUF_1,		PackZBUF(ZBUFFER_START + (ZBUFFER_SIZE / 2),PSMZ24,0));
	gif::EndTag2(1);

	// an optimised z-buffer clear that sneakily uses the frame buffer to clear the upper half
	gif::BeginTag2(gs::XYZ2, 1, PACKED, SPRITE, 1);
	for (uint32 x=0x0000; x<screen_w; x+=0x0200)
	{
		vif::StoreV4_32(x,        0x0000    , 0, 0);		// XYZ2
		vif::StoreV4_32(x+0x0200, screen_h/2, 0, 0);		// XYZ2
	}
	gif::EndTag2(1);


	#if 1	// switch this off if we don't need to clear the screen
	// set up state for screen clear
	gif::BeginTag2(gs::A_D, 1, PACKED, 0, 0);
	gs::Reg2(gs::DTHE,			PackDTHE(0));
	gs::Reg2(gs::FRAME_1,		PackFRAME(0,HRES/64,PSMCT32,0x00000000));
	gs::Reg2(gs::ZBUF_1,		PackZBUF(FRAME_START + (FRAME_SIZE / 2),PSMZ32,0));
	gs::Reg2(gs::RGBAQ,			CLEAR_COLOUR);
	gif::EndTag2(1);

	// an optimised screen clear that sneakily uses the z-buffer to
	// clear the lower half of the screen
	gif::BeginTag2(gs::XYZ2, 1, PACKED, SPRITE, 1);
	for (uint32 x=0x0000; x<screen_w; x+=0x0200)
	{
		vif::StoreV4_32(x,        0x0000    , 0, 			0);		// XYZ2
		vif::StoreV4_32(x+0x0200, screen_h/2, CLEAR_COLOUR, 0);		// XYZ2
	}
	gif::EndTag2(1);
	#endif

	vif::EndDIRECT();

	// done
	dma::Gosubs[FLIP_N_CLEAR_LETTERBOX] = dma::EndSub();





	//---------------------------------------
	//		C L E A R   S C R E E N
	//---------------------------------------

	// begin Path2 subroutine
	dma::BeginSub(dma::ref);

	// send everything PATH2
	vif::BeginDIRECT();

	// register setup
	gif::BeginTag2(gs::A_D, 1, PACKED, 0, 0);
	gs::Reg2(gs::DTHE,			PackDTHE(0));
	gs::Reg2(gs::FBA_1,			PackFBA(0));
	gs::Reg2(gs::FRAME_1,		PackFRAME(0,HRES/64,PSMCT32,0x00000000));
	gs::Reg2(gs::PRMODECONT,	PackPRMODECONT(1));
	gs::Reg2(gs::RGBAQ,			CLEAR_COLOUR);
	gs::Reg2(gs::SCANMSK,		PackSCANMSK(0));
	gs::Reg2(gs::SCISSOR_1,		PackSCISSOR(0,HRES - 1,0,VRES - 1));
	gs::Reg2(gs::TEST_1,		PackTEST(0,0,0,0,0,0,1,ZALWAYS));
	gs::Reg2(gs::XYOFFSET_1,	PackXYOFFSET(0,0));
	gs::Reg2(gs::ZBUF_1,		PackZBUF(FRAME_START + (FRAME_SIZE / 2),PSMZ32,0));
	gif::EndTag2(1);

	// an optimised screen clear that sneakily uses the z-buffer to clear the lower half
	gif::BeginTag2(gs::XYZ2, 1, PACKED, SPRITE, 1);
	for (uint32 x=0x0000; x<screen_w; x+=0x0200)
	{
		vif::StoreV4_32(x,        0x0000    , 0, 			0);		// XYZ2
		vif::StoreV4_32(x+0x0200, screen_h/2, CLEAR_COLOUR, 0);		// XYZ2
	}
	gif::EndTag2(1);

	vif::EndDIRECT();

	// done
	dma::Gosubs[CLEAR_SCREEN] = dma::EndSub();



	//---------------------------------------
	//		C L E A R   Z - B U F F E R
	//---------------------------------------

	// begin Path2 subroutine
	dma::BeginSub(dma::ref);

	// send everything PATH2
	vif::BeginDIRECT();

	// register setup
	gif::BeginTag2(gs::A_D, 1, PACKED, 0, 0);
	gs::Reg2(gs::DTHE,			PackDTHE(0));
	gs::Reg2(gs::FRAME_1,		PackFRAME(ZBUFFER_START,HRES/64,PSMCT32,0xFF000000));
	gs::Reg2(gs::PRMODECONT,	PackPRMODECONT(1));
	gs::Reg2(gs::RGBAQ,			0);
	gs::Reg2(gs::SCANMSK,		PackSCANMSK(0));
	gs::Reg2(gs::SCISSOR_1,		PackSCISSOR(0,HRES - 1,0,VRES - 1));
	gs::Reg2(gs::TEST_1,		PackTEST(0,0,0,0,0,0,1,ZALWAYS));
	gs::Reg2(gs::XYOFFSET_1,	PackXYOFFSET(0,0));
	gs::Reg2(gs::ZBUF_1,		PackZBUF(ZBUFFER_START + (ZBUFFER_SIZE / 2),PSMZ24,0));
	gif::EndTag2(1);

	// an optimised z-buffer clear that sneakily uses the frame buffer to clear the upper half
	gif::BeginTag2(gs::XYZ2, 1, PACKED, SPRITE, 1);
	for (uint32 x=0x0000; x<screen_w; x+=0x0200)
	{
		vif::StoreV4_32(x,        0x0000    , 0, 0);		// XYZ2
		vif::StoreV4_32(x+0x0200, screen_h/2, 0, 0);		// XYZ2
	}
	gif::EndTag2(1);

	vif::EndDIRECT();

	// done
	dma::Gosubs[CLEAR_ZBUFFER] = dma::EndSub();




	//-----------------------------------------
	//		S E T   R E N D E R S T A T E
	//-----------------------------------------

	// begin Path2 subroutine
	dma::BeginSub(dma::ref);

	// send everything PATH2
	vif::BeginDIRECT();

	// GS prim sets up all the registers
	gif::BeginTag2(gs::A_D, 1, PACKED, 0, 0);
	gs::Reg2(gs::ALPHA_1,		PackALPHA(0,1,0,1,128));
	gs::Reg2(gs::CLAMP_1,		PackCLAMP(REPEAT,REPEAT,0,0,0,0));
	gs::Reg2(gs::COLCLAMP,		PackCOLCLAMP(1));
	gs::Reg2(gs::DTHE,			PackDTHE(0));
	gs::Reg2(gs::FBA_1,			PackFBA(0));
	gs::Reg2(gs::FRAME_1,		PackFRAME(FRAME_START,HRES/64,PSMCT32,0x00000000));
	p_patch_PRMODE = (uint32*) dma::pLoc;
	gs::Reg2(gs::PRMODECONT,	PackPRMODECONT(1));
	gs::Reg2(gs::PRMODE,		PackPRMODE(1,0,0,1,0,0,0,0));
	gs::Reg2(gs::SCANMSK,		PackSCANMSK(0));
	gs::Reg2(gs::SCISSOR_1,		PackSCISSOR(0,HRES - 1,0,VRES - 1));
	gs::Reg2(gs::TEST_1,		PackTEST(1,AGREATER,0,KEEP,0,0,1,ZGEQUAL));
	gs::Reg2(gs::TEXFLUSH,		0);
	gs::Reg2(gs::XYOFFSET_1,	PackXYOFFSET(XOFFSET, YOFFSET));
	gs::Reg2(gs::ZBUF_1,		PackZBUF(ZBUFFER_START,PSMZ24,0));
	gif::EndTag2(1);

	vif::EndDIRECT();
	vif::FLUSHA();

	// done
	dma::Gosubs[SET_RENDERSTATE] = dma::EndSub();
	


	//-----------------------------------------
	//		S E T   F O G C O L
	//-----------------------------------------

	// begin Path2 subroutine
	dma::BeginSub(dma::ref);

	// send everything PATH2
	vif::BeginDIRECT();

	// GS prim sets up all the registers
	gif::BeginTag2(gs::A_D, 1, PACKED, 0, 0);
	render::p_patch_FOGCOL = (uint32*) dma::pLoc;
	gs::Reg2(gs::FOGCOL, PackFOGCOL(0xFF,0,0));
	gif::EndTag2(1);

	vif::EndDIRECT();

	// done
	dma::Gosubs[SET_FOGCOL] = dma::EndSub();
	


}



} // namespace NxPs2

