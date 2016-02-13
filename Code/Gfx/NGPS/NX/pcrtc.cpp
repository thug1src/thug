#include <eeregs.h>
#include <libgraph.h>
#include <core/defines.h>
#include "pcrtc.h"
#include "gs.h"
#include "switches.h"
#include "render.h"

namespace NxPs2
{

///////////////////////////////////////////////////////////////////////////////
// void SetupPCRTC(void)
//
// Sets up the read circuit blending that provides anti-aliasing
// also sets the background color (to 0,0,0)
//
// Note, this is dependent on HRES and VRES for horizontal and vertical resolution

void SetupPCRTC(int Enable, int FFMode, int HRes, int VRes)		// HRes and VRes default to 0
{
	// initialize the GS once with sceGsResetGraph().  It is the only way to set NTSC or PAL mode.
	static bool inited = false;
	if (!inited)
	{
		sceGsSyncV(0);
		if (Config::NTSC())
		{
			sceGsResetGraph(0, SCE_GS_INTERLACE, SCE_GS_NTSC, SCE_GS_FIELD);
		} else {
			sceGsResetGraph(0, SCE_GS_INTERLACE, SCE_GS_PAL, SCE_GS_FIELD);
		}

		inited = true;
	}

	// zero resolution => use default res
	if (!HRes)
		HRes = HRES;
	if (!VRes)
		VRes = VRES;

	tGS_SMODE2   smode2;
	tGS_PMODE    pmode;
	tGS_DISPFB1  dispfb1;
	tGS_DISPLAY1 display1;
	tGS_DISPFB2  dispfb2;
	tGS_DISPLAY2 display2;
	tGS_BGCOLOR  bgcolor;

	// sync mode (SMODE2 register)
	smode2.INT  = SCE_GS_INTERLACE;
	smode2.FFMD = FFMode;
	smode2.DPMS = 0;

	pmode.EN1   = 1;				// enable Read Circuit 1
	pmode.EN2   = 1;				// enable Read Circuit 2
	pmode.CRTMD = 1;				// always 1
	pmode.MMOD  = 1;				// use value in ALP for blending
	pmode.AMOD  = 0;				// OUT1 alpha output selection (irrelevant for now)
	pmode.SLBG  = Enable?0:1;		// blend with the output of Read Circuit 2 if Enable=1, or with BGCOLOR if Enable=0
	pmode.ALP 	= Enable?128:0;		// alpha=0.5 if Enable=1, or 0.0 if Enable=0
	pmode.p0	= 0;				// GS manual states that unused bits of PMODE should be set to zero
	pmode.p1	= 0;

	// Read Circuit 2 (DISPFB2 and DISPLAY2 registers)
	dispfb2.FBP = (HRes*VRes/2048);
	dispfb2.FBW = HRes/64;    //   *2 this to also display 1/2 of next frame
 	dispfb2.PSM = PSMCT16S;
	dispfb2.DBX = 240-240/1;		// 1 is the scale
	dispfb2.DBY = 224-224/1;		// 1 is the scale

	if (Config::NTSC())
	{
		display2.DX   = 0x27C;		// horizontal offset }
		display2.DY   = 0x32;		// vertical offset   }
	} else {						//					 } see SCE_GS_SET_DISPLAY_<blah> macros in eestruct.h
		display2.DX   = 0x290;		// horizontal offset }
		display2.DY   = 0x48;		// vertical offset	 }
	}
	display2.MAGH = 2560/HRes-1;	// see GS manual V5.0, p86
	display2.MAGV = 0;				// *1 vertical magnification (reg value plus 1)
	display2.DW   = 2560-1;
	display2.DH	  = VRes-1;
	
	// Read Circuit 1 (DISPFB1 and DISPLAY1 registers)
	//... copy Read Circuit 2 and then shift by 1 row to blend the fields
	dispfb1  = *(tGS_DISPFB1 *)&dispfb2;
	display1 = *(tGS_DISPLAY1 *)&display2;
	display1.DY -= 1;				// offset by 1 scanline, to blend the fields together

	// background colour (Sony says should always be black, unfortunately)
	bgcolor.R = 0;
	bgcolor.G = 0;
	bgcolor.B = 0;

	// write values to GS
	sceGsSyncV(0);
	*GS_SMODE2	 = *(volatile u_long *)&smode2;
	*GS_DISPFB1	 = *(volatile u_long *)&dispfb1;
	*GS_DISPLAY1 = *(volatile u_long *)&display1;
	*GS_DISPFB2	 = *(volatile u_long *)&dispfb2;
	*GS_DISPLAY2 = *(volatile u_long *)&display2;
	*GS_BGCOLOR  = *(volatile u_long *)&bgcolor;
	*GS_PMODE	 = *(volatile u_long *)&pmode;
}

} // namespace NxPs2

