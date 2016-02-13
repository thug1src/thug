	/*---------------------------------------------------------------------------*
  Project:  Dolphin hw Library
  File:     hwInit.c

  Copyright 1998-2001 Nintendo.  All rights reserved.

  These coded instructions, statements, and computer programs contain
  proprietary information of Nintendo of America Inc. and/or Nintendo
  Company Ltd., and are protected by Federal copyright law.  They may
  not be disclosed to third parties or copied or duplicated in any form,
  in whole or in part, without the prior written consent of Nintendo.

  $Log: /Dolphin/build/libraries/hw/src/hwInit.c $
    
    52    8/24/01 6:30p Carl
    Added more thorough output for GP hang diagnosis.
    
    51    8/22/01 8:26p Carl
    BypassWorkaround is now GPHangWorkaround.
    Added code for GP hang diagnosis.
    
    50    7/03/01 11:03a Hirose
    Deleted obsolete fragment memory management system. Changed viewport
    parameters to support Y-scaling modes rather than full-frame AA mode
    which requires complicated settings in any case.  
    
    49    5/11/01 5:57p Hirose
    changed default gamma correction back to 1.0.
    
    48    5/10/01 7:06p Carl
    Added comments concerning VI-related settings.
    
    47    11/30/00 6:41p Tian
    Fixed memory leak in hwReInit.
    
    46    11/27/00 4:57p Carl
    Added hwSetTevColorIn and hwSetTevOp.
    
    45    10/28/00 5:57p Hirose
    
    44    10/27/00 1:47a Hirose
    Simplification / Clean up. Removed obsolete flags.
    
    43    10/26/00 10:32a Tian
    Added hwReInit and hwEnableBypassWorkaround.  When used, the
    standard hwBeforeRender and hwDoneRender functions will attempt to
    repair the graphics pipe after a timeout.
    
    42    8/27/00 5:45p Alligator
    print warning messages through a callback function
    
    41    8/18/00 2:44p Dante
    Added !defined(WIN32) directive
    
    40    8/08/00 4:21p Hirose
    moved GXDrawDone call from hwSwapBuffers to hwDoneRender
    
    39    8/02/00 3:06p Tian
    Allocate framebuffers from beginning of heap to avoid VI bug (FB's must
    be in lower 16MB).
    
    38    7/21/00 1:48p Carl
    Removed full-frame aa-specific code
    (moved back to hw; see frb-aa-full.c).
    Added hwSwapBuffers().
    
    37    7/18/00 7:08p Hashida
    Fixed a problem that MAC build failed.
    
    36    7/17/00 11:59a Hashida
    Added bad memory support.
    
    35    7/07/00 7:09p Dante
    PC Compatibility
    
    34    6/26/00 5:43p Alligator
    print stats before copy, but still include copy time, lose first
    counter on first loop.
    
    33    6/13/00 12:03p Alligator
    use hwPrintf *after* sampling stat counters
    
    32    6/12/00 1:46p Alligator
    updated hw statistics to support new api
    
    31    6/07/00 10:38a Howardc
    add comment about gamma correction
    
    30    6/05/00 1:55p Carl
    Added hwDoneRenderBottom
    
    29    5/24/00 3:18a Carl
    OddField is gone; use hwDrawField instead.
    
    28    5/24/00 1:02a Ryan
    added default gamma setting of 1.7
    
    27    5/19/00 1:09p Carl
    GXSetVerifyLevel needed to be #ifdef _DEBUG
    
    26    5/18/00 3:02a Alligator
    add hwStat suff, set verify level to 0
    
    25    5/02/00 4:05p Hirose
    added function descriptions / deleted tabs
    defined some global variables as static
    attached prefix to exported global variables
    
    24    4/07/00 5:57p Carl
    Updated GXSetCopyFilter call.
    
    23    3/24/00 6:48p Carl
    Overscan adjust no longer changes predefined modes.
    
    22    3/23/00 5:26p Carl
    Added code to adjust for overscan.
    
    21    3/23/00 1:22a Hirose
    changed to call hwPadInit instead of PADInit
    
    20    3/16/00 6:14p Danm
    Cleaned up code per code review.
    
    19    2/29/00 5:54p Ryan
    update for dvddata folder rename
    
    18    2/29/00 5:05p Danm
    Select render mode based on TV format.
    
    17    2/21/00 7:56p Tian
    Changed order of PAD and VI init, since PAD requires that VI be
    initialized
    
    16    2/18/00 6:31p Carl
    Changed __SINGLEFRAME #ifdefs to CSIM_OUTPUT
    (__SINGLEFRAME is not applicable to library code, only test code).
    Removed even/odd field #ifdefs; use OddField variable instead.
    
    15    2/17/00 6:08p Carl
    Removed clear from GXCopyDisp at end of single-frame test
    
    14    2/17/00 6:03p Hashida
    Changed the way to allocate frame buffer.
    
    13    2/14/00 6:40p Danm
    Enabled GXCopyDisp in hwDoneRender.
    
    12    2/12/00 5:16p Alligator
    Integrate ArtX source tree changes
    
    11    2/11/00 3:23p Danm
    Chaged dst stride on GXCopyDisp from pixel stride to byte stride.
    
    10    2/08/00 1:50p Alligator
    do copy in hwInit to clear the color and Z buffer prior to rendering
    the first frame.
    
    9     1/28/00 10:55p Hashida
    Added VIFlush().
    Changed to wait for two frames after tv mode is changed.
    
    8     1/26/00 3:59p Hashida
    Changed to use VIConfigure.
    
    7     1/25/00 4:12p Danm
    Changed default render mode for all platforms to GXNtsc480IntDf.
    
    6     1/21/00 4:41p Alligator
    added even/odd field select for __SINGLEFRAME field tests
    
    5     1/19/00 3:43p Danm
    Added GXRenderModeObj *hwGetRenderModeObj(void) function.
    
    4     1/13/00 8:53p Danm
    Added GXRenderModeObj * parameter to hwInit()
    
    3     1/13/00 5:55p Alligator
    integrate with ArtX GX library code
    
    2     1/11/00 3:18p Danm
    Reversed init of VI and GX for single field support
    
    16    11/16/99 9:43a Hashida
    Moved DVDInit before using arenaLo.
    
    15    11/01/99 6:09p Tian
    Framebuffers no longer allocated if generating CSIM output.
    
    14    10/13/99 4:32p Alligator
    change GXSetViewport, GXSetScissor to use xorig, yorig, wd, ht
    
    13    10/07/99 12:14p Alligator
    initialize rmode before using it in BeforeRender
    
    12    10/06/99 10:38a Alligator
    changed enums for compressed Z format
    updated Init functions to make GX_ZC_LINEAR default
    
    11    9/30/99 10:32p Yasu
    Renamed and clean up functions and enums.
    
    10    9/28/99 4:14p Hashida
    Moved GXCopyDisp to GXDoneRender
    
    9     9/23/99 3:23p Tian
    New GXFifo/GXInit APIs, cleanup.
    
    8     9/22/99 7:08p Yasu
    Fixed paramter of GXSetViewPort
    
    7     9/22/99 6:12p Yasu
    Changed the parameters of GXSetDispCopySrc().
    Fixed small bugs.
    
    6     9/21/99 2:33p Alligator
    add aa flag, if aa set 16b pix format
    
    5     9/20/99 11:02a Ryan
    
    4     9/17/99 5:24p Ryan
    commented GXCopy temporarily
    
    3     9/17/99 1:33p Ryan
    update to include new VI API's
    
    2     7/28/99 4:21p Ryan
    Added DVDInit() and DVDSetRoot
    
    1     7/20/99 6:06p Alligator
  $NoKeywords: $
 *---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*
    This hw library provides a common application
    framework that is used in all the GX hws distributed
    with the Dolphin SDK.
 *---------------------------------------------------------------------------*/


#include <dolphin.h>
#include "p_hw.h"
#include <charpipeline/GQRSetup.h>
#include "libsn.h"
#include 	"gfx\ngc\nx\nx_init.h"
#include "sys\ngc\p_aram.h"
#include 	"gfx/ngc/nx/mesh.h"
#include "sys\ngc\p_gx.h"
#include "sys\timer.h"

/*---------------------------------------------------------------------------*
    External functions
 *---------------------------------------------------------------------------*/
//extern void hwUpdateStats ( GXBool inc );
//extern void hwPrintStats  ( void );

/*---------------------------------------------------------------------------*
    Global variables
 *---------------------------------------------------------------------------*/
void*   hwFrameBuffer1;
void*   hwFrameBuffer2;
void*   hwCurrentBuffer;

/*---------------------------------------------------------------------------*
    Static variables
 *---------------------------------------------------------------------------*/
static GXBool  hwFirstFrame = GX_TRUE;

//static void*        DefaultFifo;
static char			DefaultFifo[DEFAULT_FIFO_SIZE] ATTRIBUTE_ALIGN(32);
static GXFifoObj*   DefaultFifoObj;

GXRenderModeObj *rmode;
GXRenderModeObj rmodeobj;

/*---------------------------------------------------------------------------*
    Automatic frame skipping to work around GP hang condition.
 *---------------------------------------------------------------------------*/
static BOOL GPHangWorkaround = FALSE;

// used to count missed frames by VI retrace callback handler
static vu32 FrameCount;
static u32  FrameMissThreshold; // number of frames to be considered a timeout

// tokens are used instead of GXDrawDone
#define hw_START_FRAME_TOKEN  0xFEEB
#define hw_END_FRAME_TOKEN    0xB00B

// we need to create a thread to catch hangs at high watermark ("overflow")
static OSThread OVThread;               // OS data for overflow thread
static u8       OVThreadStack[4096];    // Stack for overflow thread

static u32 fbSize;

/*---------------------------------------------------------------------------*
    Fragmented memory systems
 *---------------------------------------------------------------------------*/
typedef struct
{
    void*           start;
    void*           end;
} meminfo;

/*---------------------------------------------------------------------------*
    Foward references
 *---------------------------------------------------------------------------*/
static void __hwInitRenderMode    ( GXRenderModeObj* mode );
static void __hwInitMem           ( void );
static void __hwInitGX            ( void );
static void __hwInitVI            ( void );
//static void __hwInitForEmu        ( void );

static void __NoHangRetraceCallback ( u32 count );
static void __NoHangDoneRender      ( void );

static void  __hwDiagnoseHang     ( void );
static void* __hwOverflowThread   ( void* param );


/*---------------------------------------------------------------------------*
   Functions
 *---------------------------------------------------------------------------*/

/*===========================================================================*


    Initialization


 *===========================================================================*/

extern int resetDown;

void reset_callback( void )
{
	resetDown = 1;
}

/*---------------------------------------------------------------------------*
    Name:           hwInit
    
    Description:    This function initializes the components of
                    the operating system and its device drivers.
                    The mode parameter allows the application to
                    override the default render mode. It then allocates
                    all of main memory except the area for external
                    framebuffer into a heap than can be managed
                    with OSAlloc. This function initializes the video
                    controller to run at 640x480 interlaced display,
                    with 60Hz refresh (actually, 640x448; see below).
    
    Arguments:      mode  : render mode
                            Default render mode will be used when
                            NULL is given as this argument.
    
    Returns:        None
 *---------------------------------------------------------------------------*/
void hwInit( GXRenderModeObj *mode )
{
    /*----------------------------------------------------------------*
     *  Initialize OS                                                 *
     *----------------------------------------------------------------*/
    OSInit();

    /*----------------------------------------------------------------*
     *  Initialize DVD                                                *
     *   (need to initialize DVD BEFORE program uses                  *
     *   arena because DVDInit might change arena lo.)                *
     *----------------------------------------------------------------*/
    DVDInit();

    /*----------------------------------------------------------------*
     *  Initialize VI                                                 *
     *----------------------------------------------------------------*/
    VIInit();

    /*----------------------------------------------------------------*
     *  Initialize game PAD (PADInit is called by hwPadInit)        *
     *----------------------------------------------------------------*/
    hwPadInit();

    /*----------------------------------------------------------------*
     *  Set up rendering mode                                         *
     *  (which reflects the GX/VI configurations and XFB size below)  *
     *----------------------------------------------------------------*/
    __hwInitRenderMode(mode);

    /*----------------------------------------------------------------*
     *  Initialize memory configuration (framebuffers / heap)         *
     *----------------------------------------------------------------*/
    __hwInitMem();

    /*----------------------------------------------------------------*
     *  Configure VI by given render mode                             *
     *----------------------------------------------------------------*/
    VIConfigure(rmode);

    /*----------------------------------------------------------------*
     *  Initialize Graphics                                           *
     *----------------------------------------------------------------*/
    // Alloc default 256K fifo
//    DefaultFifo     = OSAlloc(DEFAULT_FIFO_SIZE);
    DefaultFifoObj  = GX::Init(DefaultFifo, DEFAULT_FIFO_SIZE);
    
    // Configure GX
    __hwInitGX();

    /*----------------------------------------------------------------*
     *  Emulator only initialization portion                          *
     *----------------------------------------------------------------*/
#ifdef EMU
    __hwInitForEmu();
#endif

    /*----------------------------------------------------------------*
     *  Start up VI                                                   *
     *----------------------------------------------------------------*/
    __hwInitVI();

    // Creates a new thread in order to diagnose hangs during high watermark.
    // The thread is suspended by default.
    OSCreateThread(
        &OVThread,                          // ptr to the thread obj to init
        __hwOverflowThread,                 // ptr to the start routine
        0,                                  // param passed to start routine
        OVThreadStack+sizeof(OVThreadStack),// initial stack address
        sizeof(OVThreadStack),
        31,                                 // scheduling priority (low)
        OS_THREAD_ATTR_DETACH);             // detached by default

    OSResumeThread(&OVThread);    // Allow the thread to run

    GQRSetup6( GQR_SCALE_64,		// Pos
               GQR_TYPE_S16,
			   GQR_SCALE_64,
               GQR_TYPE_S16 );
    GQRSetup7( 14,		// Normal
               GQR_TYPE_S16,
               14,
               GQR_TYPE_S16 );

//	hwEnableGPHangWorkaround( 60 );


//	static unsigned long profdata[2048]; // long aligned,
//	// 4K to 64K bytes
//	if(snProfInit(_4KHZ, profdata, sizeof(profdata)) != 0)
//	printf("Profiler init failed\n"); // see SN_PRF… in LIBSN.H
//	// rest of user code follows on from here...

	OSSetResetCallback( reset_callback );
}

/*---------------------------------------------------------------------------*
    Name:           __hwInitRenderMode
    
    Description:    This function sets up rendering mode which configures
                    GX and VI. If mode == NULL, this function use a default
                    rendering mode according to the TV format.
    
    Arguments:      None
    
    Returns:        None
 *---------------------------------------------------------------------------*/
static void __hwInitRenderMode( GXRenderModeObj* mode )
{
    // If an application specific render mode is provided,
    // override the default render mode
    if (mode != NULL) 
    {
        rmode = mode;
    }
    else
    {
		rmode = &GXNtsc480IntDf;

        // Trim off from top & bottom 16 scanlines (which will be overscanned).
        // So almost all hws actually render only 448 lines (in NTSC case.)
        // Since this setting is just for SDK hws, you can specify this
        // in order to match your application requirements.
        GX::AdjustForOverscan(rmode, &rmodeobj, 0, 16);
        
        rmode = &rmodeobj;

		OSReport( "TV format: %d\n", VIGetTvFormat() );

        switch (VIGetTvFormat())
        {
			case VI_NTSC:
				if ( NxNgc::EngineGlobals.use_480p )
				{
					rmode->viTVmode = VI_TVMODE_NTSC_PROG;
					rmode->xFBmode = VI_XFBMODE_SF;
				}
				break;
			case VI_PAL:
			case VI_EURGB60:
			case VI_DEBUG_PAL:
				if ( rmode->viTVmode == VI_TVMODE_NTSC_INT )
				{
					if ( NxNgc::EngineGlobals.use_60hz )
					{
						rmode->viTVmode = VI_TVMODE_EURGB60_INT;
					}
					else
					{
						rmode->viTVmode = VI_TVMODE_PAL_INT;
						rmode->viYOrigin = 23;
					}
				}
				else
				{
					if ( NxNgc::EngineGlobals.use_60hz )
					{
						rmode->viTVmode = VI_TVMODE_EURGB60_DS;
					}
					else
					{
						rmode->viTVmode = VI_TVMODE_PAL_DS;
						rmode->viYOrigin = 23;
					}
				}
				if ( NxNgc::EngineGlobals.use_60hz )
				{
					rmode->xfbHeight = 448;
				}
				else
				{
					rmode->xfbHeight = ( rmode->xfbHeight * 528 ) / 448;
					rmode->viHeight = 528;
				}
				break;
			case VI_MPAL:
				rmode = &GXMpal480IntDf;
				break;
			default:
				//OSHalt("hwInit: invalid TV format\n");
				break;
        }

		// Nintendo's recommended settings.
		rmode->viWidth = 650;
		rmode->viXOrigin = 35;
   }
}

/*---------------------------------------------------------------------------*
    Name:           __hwInitMem
    
    Description:    This function allocates external framebuffers from
                    arena and sets up a heap
   
    Arguments:      None
    
    Returns:        None
 *---------------------------------------------------------------------------*/
static void __hwInitMem( void )
{
    void*    arenaLo;
    void*    arenaHi;

    arenaLo = OSGetArenaLo();
    arenaHi = OSGetArenaHi();


    /*----------------------------------------------------------------*
     *  Allocate external framebuffers                                *
     *----------------------------------------------------------------*/

    // In HW1, we should allocate the buffers at the beginning of arena (<16MB)
    // Need to do this before OSInitAlloc is called.
    fbSize = VIPadFrameBufferWidth(rmode->fbWidth) * rmode->xfbHeight * 
        (u32)VI_DISPLAY_PIX_SZ;
    hwFrameBuffer1 = (void*)OSRoundUp32B((u32)arenaLo);
    hwFrameBuffer2 = (void*)OSRoundUp32B((u32)hwFrameBuffer1 + fbSize);
    hwCurrentBuffer = hwFrameBuffer2;
    
    arenaLo = (void*)OSRoundUp32B((u32)hwFrameBuffer2 + fbSize);
    OSSetArenaLo(arenaLo);

//    /*----------------------------------------------------------------*
//     *  Create a heap                                                 *
//     *----------------------------------------------------------------*/
//
//    // OSInitAlloc should only ever be invoked once.
//    arenaLo = OSGetArenaLo();
//    arenaHi = OSGetArenaHi();
//    arenaLo = OSInitAlloc(arenaLo, arenaHi, 1); // 1 heap
//    OSSetArenaLo(arenaLo);
//
//    // Ensure boundaries are 32B aligned
//    arenaLo = (void*)OSRoundUp32B(arenaLo);
//    arenaHi = (void*)OSRoundDown32B(arenaHi);
//				   	
//    // The boundaries given to OSCreateHeap should be 32B aligned
//    OSSetCurrentHeap(OSCreateHeap(arenaLo, arenaHi));
//    // From here on out, OSAlloc and OSFree behave like malloc and free
//    // respectively
//    OSSetArenaLo(arenaLo=arenaHi);

    /*----------------------------------------------------------------*
     *  Allocate ARAM buffers                                         *
     *----------------------------------------------------------------*/

    ARInit(NULL, 0);
	ARQInit();
	AIInit( NULL );
	AXInit();
	AXSetMode( AX_MODE_DPL2 );
    AXSetCompressor( AX_COMPRESSOR_OFF );
	NsARAM::init();

	// Allocate the 1st 1024 bytes for our own use.
	NxNgc::EngineGlobals.aram_zero = NsARAM::alloc( aram_zero_size );

	// Allocate 2.8mb for the DSP data.
	NxNgc::EngineGlobals.aram_dsp = NsARAM::alloc( aram_dsp_size );

	// Allocate 32k each for the 3 streams.
	NxNgc::EngineGlobals.aram_stream0 = NsARAM::alloc( aram_stream0_size );
	NxNgc::EngineGlobals.aram_stream1 = NsARAM::alloc( aram_stream1_size );
	NxNgc::EngineGlobals.aram_stream2 = NsARAM::alloc( aram_stream2_size );

	// Allocate 32k for the music buffer.
	NxNgc::EngineGlobals.aram_music = NsARAM::alloc( aram_music_size );
}

/*---------------------------------------------------------------------------*
    Name:           __hwInitGX
    
    Description:    This function performs GX configuration by using
                    current rendering mode
   
    Arguments:      None
    
    Returns:        None
 *---------------------------------------------------------------------------*/
static void __hwInitGX( void )
{
    /*----------------------------------------------------------------*
     *  GX configuration by a render mode obj                         *
     *----------------------------------------------------------------*/

    // These are all necessary function calls to take a render mode
    // object and set up the relevant GX configuration.

    GX::SetViewport(0.0F, 0.0F, (f32)rmode->fbWidth, (f32)rmode->efbHeight, 
                  0.0F, 1.0F);
    GX::SetScissor(0, 0, (u32)rmode->fbWidth, (u32)rmode->efbHeight);
    GX::SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX::SetDispCopyDst(rmode->fbWidth, rmode->xfbHeight);
    
	GX::SetDispCopyYScale((f32)(rmode->xfbHeight) / (f32)(rmode->efbHeight)); 

    GX::SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);

    // Note that following is an appropriate setting for full-frame AA mode.
    // You should use "xfbHeight" instead of "efbHeight" to specify actual
    // view size. Since this library doesn't support such special case, please
    // call these in each application to override the normal setting.
#if 0
    GX::SetViewport(0.0F, 0.0F, (f32)rmode->fbWidth, (f32)rmode->xfbHeight, 
                  0.0F, 1.0F);
    GX::SetDispCopyYScale(1.0F);
#endif

	if (rmode->aa)
	{
        GX::SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	}
    else
	{
        GX::SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
//        GX::SetPixelFmt(/*GX_PF_RGB8_Z24*/GX_PF_RGBA6_Z24, GX_ZC_LINEAR);
//		GXSetPixelFmt(/*GX_PF_RGB8_Z24*/GX_PF_RGBA6_Z24, GX_ZC_LINEAR);
	}


    /*----------------------------------------------------------------*
     *  Miscellaneous GX initialization for hws                     *
     *  (Since these are not actually necessary for all applications, *
     *  you may remove them if you feel they are unnecessary.)        *
     *----------------------------------------------------------------*/

    // Clear embedded framebuffer for the first frame
    GX::CopyDisp(hwCurrentBuffer, GX_TRUE);

    // Verify (warning) messages are turned off by default
#ifdef _DEBUG
//    GXSetVerifyLevel(GX_WARN_NONE);
	GXSetVerifyLevel(GX_WARN_NONE);
#endif

    // Gamma correction
    //
    // You may want to use 1.7 or 2.2 if all artwork made on PCs
    // become darker. However, recommended setting is 1.0.
    GX::SetDispCopyGamma(GX_GM_1_0);

	// 2D Primitives.
    GX::SetVtxAttrFmt( GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14 );
    GX::SetVtxAttrFmt( GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT0, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT0, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT0, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT0, GX_VA_TEX3, GX_TEX_ST, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT0, GX_VA_TEX4, GX_TEX_ST, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT0, GX_VA_TEX5, GX_TEX_ST, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT0, GX_VA_TEX6, GX_TEX_ST, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT0, GX_VA_TEX7, GX_TEX_ST, GX_F32, 0 );




//#ifdef SHORT_VERT
//	// Environment geometry. 1=1.
//	GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14 );
//    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_TEX3, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_TEX4, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_TEX5, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_TEX6, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_TEX7, GX_TEX_ST, GX_F32, 0 );
//
//	// Environment geometry. 1=2.
//	GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_POS, GX_POS_XYZ, GX_S16, 1 );
//    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14 );
//    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_TEX3, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_TEX4, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_TEX5, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_TEX6, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_TEX7, GX_TEX_ST, GX_F32, 0 );
//
//	// Environment geometry. 1=4.
//	GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_POS, GX_POS_XYZ, GX_S16, 2 );
//    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14 );
//    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_TEX3, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_TEX4, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_TEX5, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_TEX6, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_TEX7, GX_TEX_ST, GX_F32, 0 );
//
//	// Environment geometry. 1=8.
//	GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_POS, GX_POS_XYZ, GX_S16, 3 );
//    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14 );
//    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_TEX3, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_TEX4, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_TEX5, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_TEX6, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_TEX7, GX_TEX_ST, GX_F32, 0 );
//
//	// Environment geometry. 1=16.
//	GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_POS, GX_POS_XYZ, GX_S16, 4 );
//    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14 );
//    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_TEX3, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_TEX4, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_TEX5, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_TEX6, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_TEX7, GX_TEX_ST, GX_F32, 0 );
//
//	// Environment geometry. 1=32.
//	GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_POS, GX_POS_XYZ, GX_S16, 5 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX3, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX4, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX5, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX6, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX7, GX_TEX_ST, GX_F32, 0 );
//#else
//	// Environment/skinned geometry. 1=64.
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX3, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX4, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX5, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX6, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX7, GX_TEX_ST, GX_F32, 0 );
//#endif		// SHORT_VERT
//	// Environment/skinned geometry. 1=64.
//	GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_POS, GX_POS_XYZ, GX_S16, 6 );
//    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14 );
//    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_TEX3, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_TEX4, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_TEX5, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_TEX6, GX_TEX_ST, GX_F32, 0 );
//    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_TEX7, GX_TEX_ST, GX_F32, 0 );
	GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_S16, 6 );
    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14 );
    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_TEX3, GX_TEX_ST, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_TEX4, GX_TEX_ST, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_TEX5, GX_TEX_ST, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_TEX6, GX_TEX_ST, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT1, GX_VA_TEX7, GX_TEX_ST, GX_F32, 0 );

	// Environment.
    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14 );
    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_TEX0, GX_TEX_ST, GX_S16, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_TEX1, GX_TEX_ST, GX_S16, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_TEX2, GX_TEX_ST, GX_S16, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_TEX3, GX_TEX_ST, GX_S16, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_TEX4, GX_TEX_ST, GX_S16, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_TEX5, GX_TEX_ST, GX_S16, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_TEX6, GX_TEX_ST, GX_S16, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT2, GX_VA_TEX7, GX_TEX_ST, GX_S16, 0 );

    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14 );
    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_TEX0, GX_TEX_ST, GX_S16, 2 );
    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_TEX1, GX_TEX_ST, GX_S16, 2 );
    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_TEX2, GX_TEX_ST, GX_S16, 2 );
    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_TEX3, GX_TEX_ST, GX_S16, 2 );
    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_TEX4, GX_TEX_ST, GX_S16, 2 );
    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_TEX5, GX_TEX_ST, GX_S16, 2 );
    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_TEX6, GX_TEX_ST, GX_S16, 2 );
    GX::SetVtxAttrFmt( GX_VTXFMT3, GX_VA_TEX7, GX_TEX_ST, GX_S16, 2 );

    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14 );
    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_TEX0, GX_TEX_ST, GX_S16, 4 );
    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_TEX1, GX_TEX_ST, GX_S16, 4 );
    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_TEX2, GX_TEX_ST, GX_S16, 4 );
    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_TEX3, GX_TEX_ST, GX_S16, 4 );
    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_TEX4, GX_TEX_ST, GX_S16, 4 );
    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_TEX5, GX_TEX_ST, GX_S16, 4 );
    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_TEX6, GX_TEX_ST, GX_S16, 4 );
    GX::SetVtxAttrFmt( GX_VTXFMT4, GX_VA_TEX7, GX_TEX_ST, GX_S16, 4 );

    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14 );
    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_TEX0, GX_TEX_ST, GX_S16, 6 );
    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_TEX1, GX_TEX_ST, GX_S16, 6 );
    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_TEX2, GX_TEX_ST, GX_S16, 6 );
    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_TEX3, GX_TEX_ST, GX_S16, 6 );
    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_TEX4, GX_TEX_ST, GX_S16, 6 );
    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_TEX5, GX_TEX_ST, GX_S16, 6 );
    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_TEX6, GX_TEX_ST, GX_S16, 6 );
    GX::SetVtxAttrFmt( GX_VTXFMT5, GX_VA_TEX7, GX_TEX_ST, GX_S16, 6 );

    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14 );
    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX0, GX_TEX_ST, GX_S16, 8 );
    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX1, GX_TEX_ST, GX_S16, 8 );
    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX2, GX_TEX_ST, GX_S16, 8 );
    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX3, GX_TEX_ST, GX_S16, 8 );
    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX4, GX_TEX_ST, GX_S16, 8 );
    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX5, GX_TEX_ST, GX_S16, 8 );
    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX6, GX_TEX_ST, GX_S16, 8 );
    GX::SetVtxAttrFmt( GX_VTXFMT6, GX_VA_TEX7, GX_TEX_ST, GX_S16, 8 );

    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14 );
    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0 );
    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_TEX0, GX_TEX_ST, GX_S16, 10 );
    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_TEX1, GX_TEX_ST, GX_S16, 10 );
    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_TEX2, GX_TEX_ST, GX_S16, 10 );
    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_TEX3, GX_TEX_ST, GX_S16, 10 );
    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_TEX4, GX_TEX_ST, GX_S16, 10 );
    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_TEX5, GX_TEX_ST, GX_S16, 10 );
    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_TEX6, GX_TEX_ST, GX_S16, 10 );
    GX::SetVtxAttrFmt( GX_VTXFMT7, GX_VA_TEX7, GX_TEX_ST, GX_S16, 10 );
}

/*---------------------------------------------------------------------------*
    Name:           __hwInitVI
    
    Description:    This function performs VI start up settings that are
                    necessary at the beginning of each hw
    
    Arguments:      None
    
    Returns:        None
 *---------------------------------------------------------------------------*/
static void __hwInitVI( void )
{
    u32 nin;

    // Double-buffering initialization
    VISetNextFrameBuffer(hwFrameBuffer1);
    hwCurrentBuffer = hwFrameBuffer2;

    // Tell VI device driver to write the current VI settings so far
    VIFlush();
    
    // Wait for retrace to start first frame
    VIWaitForRetrace();

    // Because of hardware restriction, we need to wait one more 
    // field to make sure mode is safely changed when we change
    // INT->DS or DS->INT. (VIInit() sets INT mode as a default)
    nin = (u32)rmode->viTVmode & 1;
    if (nin)
        VIWaitForRetrace();
}

/*---------------------------------------------------------------------------*
    Name:           __hwInitForEmu
    
    Description:    This function performs emulator only portion of
                    initialization.
    
    Arguments:      None
    
    Returns:        None
 *---------------------------------------------------------------------------*/
//static void __hwInitForEmu( void )
//{
//    // Set up DVDRoot
//#ifdef MACOS
//    DVDSetRoot("DOLPHIN/dvddata/");
//#endif
//
//#ifdef WIN32
//	unsigned char path[270] = "";
//	strcat(path, installPath);
//	strcat(path, "/dvddata");
//	DVDSetRoot(path);
//#endif
//}



/*===========================================================================*


    Basic hw framework control functions


 *===========================================================================*/

/*---------------------------------------------------------------------------*
    Name:           hwBeforeRender
    
    Description:    This function sets up the viewport to render the
                    appropriate field if field rendering is enabled.
                    Field rendering is a property of the render mode.
    
    Arguments:      None
    
    Returns:        None
 *---------------------------------------------------------------------------*/
void hwBeforeRender( void )
{

#ifndef EMU
    if (GPHangWorkaround)
    {
        // This will overwrite the end frame token from the previous frame.
        GX::SetDrawSync( hw_START_FRAME_TOKEN);

        // Reset the counters for post-hang diagnosis.
        GX::ClearGPMetric();
    }
#endif // !EMU
    

    // Set up viewport (This is inappropriate for full-frame AA.)
    if (rmode->field_rendering)
    {
        GX::SetViewportJitter(
          0.0F, 0.0F, (float)rmode->fbWidth, (float)rmode->efbHeight, 
          0.0F, 1.0F, VIGetNextField());
    }
    else
    {
        GX::SetViewport(
          0.0F, 0.0F, (float)rmode->fbWidth, (float)rmode->efbHeight, 
          0.0F, 1.0F);
    }
    
    // Invalidate vertex cache in GP
    GX::InvalidateVtxCache();

#ifndef EMU
    // Invalidate texture cache in GP
    GX::InvalidateTexAll();
#endif // !EMU

}

/*---------------------------------------------------------------------------*
    Name:           hwDoneRender
    
    Description:    This function copies the embedded frame buffer (EFB)
                    to the external frame buffer (XFB) via GXCopyDisp,
                    and then calls hwSwapBuffers.
    
    Arguments:      None
    
    Returns:        None
 *---------------------------------------------------------------------------*/
void hwDoneRender( bool clear )
{
    // GP hang workaround version (Turned off by default)
    if (GPHangWorkaround)
    {
//        ASSERTMSG(!hwStatEnable,
//                  "DEMOStats and GP hang diagnosis are mutually exclusive");
        __NoHangDoneRender();
        return;
    }

    // Statistics support (Turned off by default)
//    if (hwStatEnable)
//    {
//        GXDrawDone();             // make sure data through pipe
//        hwUpdateStats(GX_TRUE); // sample counters here
//        hwPrintStats();         // don't include time to print stats
//        GXDrawDone();             // make sure data through pipe
//        hwUpdateStats(GX_FALSE); // reset counters here, include copy time
//    }

    // Set Z/Color update to make sure eFB will be cleared at GXCopyDisp.
    // (If you want to control these modes by yourself in your application,
    //  please comment out this part.)
    GX::SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    
    // Issue display copy command
    GX::CopyDisp( hwCurrentBuffer, ( clear ) ? GX_TRUE : GX_FALSE );

    // Wait until everything is drawn and copied into XFB.
    GX::DrawDone();        

    // Set the next frame buffer
    hwSwapBuffers();
}

/*---------------------------------------------------------------------------*
    Name:            RADDEMOLockBackBuffer

    Description:    This function returns the back buffer info, so that you
                    can write right into it.
 *---------------------------------------------------------------------------*/
int hwLockBackBuffer( void ** buf_addr, unsigned int * width, unsigned int * height )
{
  if ( buf_addr )
    *buf_addr = hwCurrentBuffer;
  if ( width )
    *width = rmode->fbWidth;
  if ( height )
    *height = rmode->xfbHeight;
  return( 1 );
}

/*---------------------------------------------------------------------------*
    Name:            RADDEMOUnlockBackBuffer

    Description:    This function returns the back buffer info, so that you
                    can write right into it.
 *---------------------------------------------------------------------------*/
void hwUnlockBackBuffer( int updated )
{
  if ( updated )
  {
    DCStoreRange( hwCurrentBuffer, fbSize );
  }
}


/*---------------------------------------------------------------------------*
    Name:           hwSwapBuffers
    
    Description:    This function finishes copying via GXDrawDone, sets
                    the next video frame buffer, waits for vertical
                    retrace, and swaps internal rendering buffers.
    
    Arguments:      None
    
    Returns:        None
 *---------------------------------------------------------------------------*/
void hwSwapBuffers( void )
{
    // Display the buffer which was just filled by GXCopyDisplay
    VISetNextFrameBuffer(hwCurrentBuffer);

    // If this is the first frame, turn off VIBlack
    if(hwFirstFrame)
    {
        VISetBlack(FALSE);
        hwFirstFrame = GX_FALSE;
    }
    
    // Tell VI device driver to write the current VI settings so far
    VIFlush();
    
    // Wait for vertical retrace.
//    VIWaitForRetrace();
	Tmr::VSync1();

    // Swap buffers
    if(hwCurrentBuffer == hwFrameBuffer1)
        hwCurrentBuffer = hwFrameBuffer2;
    else
        hwCurrentBuffer = hwFrameBuffer1;
}

/*---------------------------------------------------------------------------*
    Name:           hwSetTevColorIn
    
    Description:    This function is a Rev A compatible version of
                    GXSetTevColorIn.  It will set the swap mode if
                    one of TEXC/TEXRRR/TEXGGG/TEXBBB is selected.

                    It doesn't error check for multiple swaps, however.

                    Note that it is available for Rev A or Rev B.
                    For Rev A, it is defined as an inline in hw.h.
    
    Arguments:      Same as GXSetTevColorIn
    
    Returns:        None
 *---------------------------------------------------------------------------*/
#if ( GX_REV > 1 )
void hwSetTevColorIn(GXTevStageID stage,
                       GXTevColorArg a, GXTevColorArg b,
                       GXTevColorArg c, GXTevColorArg d )
{
//    u32 swap = 0;
//
//    if (a == GX_CC_TEXC)
//        { swap = GX_CC_TEXRRR - 1;  }
//    else if (a >= GX_CC_TEXRRR)
//        { swap = a; a = GX_CC_TEXC; }
//
//    if (b == GX_CC_TEXC)
//        { swap = GX_CC_TEXRRR - 1;  }
//    else if (b >= GX_CC_TEXRRR)
//        { swap = b; b = GX_CC_TEXC; }
//
//    if (c == GX_CC_TEXC)
//        { swap = GX_CC_TEXRRR - 1;  }
//    else if (c >= GX_CC_TEXRRR)
//        { swap = c; c = GX_CC_TEXC; }
//
//    if (d == GX_CC_TEXC)
//        { swap = GX_CC_TEXRRR - 1;  }
//    else if (d >= GX_CC_TEXRRR)
//        { swap = d; d = GX_CC_TEXC; }
//
//    GXSetTevColorIn(stage, a, b, c, d);
//
//    if (swap > 0)
//      GXSetTevSwapMode(stage, GX_TEV_SWAP0,
//                       (GXTevSwapSel)(swap - GX_CC_TEXRRR + 1));
}
#endif

/*---------------------------------------------------------------------------*
    Name:           hwSetTevOp
    
    Description:    This function is a Rev-A compatible version of
                    GXSetTevOp.  It will set the swap mode if
                    GX_CC_TEXC is selected for an input.

                    Note that it is available for Rev A or Rev B.
                    For Rev A, it is defined as an inline in hw.h.
    
    Arguments:      Same as GXSetTevOp
    
    Returns:        None
 *---------------------------------------------------------------------------*/
#if ( GX_REV > 1 )
//void hwSetTevOp(GXTevStageID id, GXTevMode mode)
//{
//    GXTevColorArg carg = GX_CC_RASC;
//    GXTevAlphaArg aarg = GX_CA_RASA;
//
//    if (id != GX_TEVSTAGE0) {
//        carg = GX_CC_CPREV;
//        aarg = GX_CA_APREV;
//    }
//
//    switch (mode) {
//      case GX_MODULATE:
//        hwSetTevColorIn(id, GX_CC_ZERO, GX_CC_TEXC, carg, GX_CC_ZERO);
//        GXSetTevAlphaIn(id, GX_CA_ZERO, GX_CA_TEXA, aarg, GX_CA_ZERO);
//        break;
//      case GX_DECAL:
//        hwSetTevColorIn(id, carg, GX_CC_TEXC, GX_CC_TEXA, GX_CC_ZERO);
//        GXSetTevAlphaIn(id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, aarg);
//        break;
//      case GX_BLEND:
//        hwSetTevColorIn(id, carg, GX_CC_ONE, GX_CC_TEXC, GX_CC_ZERO);
//        GXSetTevAlphaIn(id, GX_CA_ZERO, GX_CA_TEXA, aarg, GX_CA_ZERO);
//        break;
//      case GX_REPLACE:
//        hwSetTevColorIn(id, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
//        GXSetTevAlphaIn(id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
//        break;
//      case GX_PASSCLR:
//        hwSetTevColorIn(id, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, carg);
//        GXSetTevAlphaIn(id, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, aarg);
//        break;
//      default:
//        ASSERTMSG(0, "hwSetTevOp: Invalid Tev Mode");
//        break;
//    }
//
//    GXSetTevColorOp(id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
//    GXSetTevAlphaOp(id, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
//}
#endif

/*---------------------------------------------------------------------------*
    Name:           hwGetRenderModeObj
    
    Description:    This function returns the current rendering mode.
                    It is most useful to inquire what the default
                    rendering mode is.
    
    Arguments:      None
    
    Returns:        None
 *---------------------------------------------------------------------------*/
GXRenderModeObj* hwGetRenderModeObj( void )
{
    return rmode;
}

/*---------------------------------------------------------------------------*
    Name:           hwGetCurrentBuffer
    
    Description:    This function returns the pointer to external
                    framebuffer currently active. Since this library
                    swiches double buffer hwFrameBuffer1/hwFrameBuffer2,
                    the returned pointer will be one of them.
    
    Arguments:      None
    
    Returns:        None
 *---------------------------------------------------------------------------*/
void* hwGetCurrentBuffer( void )
{
    return hwCurrentBuffer;
}



/*===========================================================================*


    GP hang work-around auto-recovery system


 *===========================================================================*/
/*---------------------------------------------------------------------------*
    Name:           hwEnableGPHangWorkaround
    
    Description:    Sets up the DEMO library to skip past any GP hangs and
                    attempt to repair the graphics pipe whenever a timeout
                    of /timeoutFrames/ occurs.  This will serve as a
                    temporary work-around for any GP hangs that may occur.

    
    Arguments: timeoutFrames        The number of 60hz frames to wait in
                                    hwDoneRender before aborting the
                                    graphics pipe.  Should be at least
                                    equal to your standard frame rate
                                    (e.g. 60hz games should use a value of 1,
                                    30hz games should use a value of 2)
    
    Returns:        None
 *---------------------------------------------------------------------------*/
void hwEnableGPHangWorkaround ( u32 timeoutFrames )
{
#ifdef EMU
    #pragma unused (timeoutFrames)
#else
    if (timeoutFrames)
    {
//        ASSERTMSG(!hwStatEnable,
//                  "DEMOStats and GP hang diagnosis are mutually exclusive");
        GPHangWorkaround = TRUE;
        FrameMissThreshold = timeoutFrames;
        VISetPreRetraceCallback( __NoHangRetraceCallback );

        // Enable counters for post-hang diagnosis
        hwSetGPHangMetric( GX_TRUE );
    }
    else
    {
        GPHangWorkaround = FALSE;
        FrameMissThreshold = 0;
        hwSetGPHangMetric( GX_FALSE );
        VISetPreRetraceCallback( NULL );
    }
#endif
}

/*---------------------------------------------------------------------------*
    Name:           __NoHangRetraceCallback
    
    Description:    VI callback to count missed frames for GPHangWorkaround
    
    Arguments:      Unused
    
    Returns:        None
 *---------------------------------------------------------------------------*/
static void __NoHangRetraceCallback ( u32 count )
{
//    #pragma unused (count)
    FrameCount++;
}


/*---------------------------------------------------------------------------*
    Name:           __NoHangDoneRender
    
    Description:    Called in lieu of the standard hwDoneRender if 
                    GPHangWorkaround == TRUE.

                    Uses a token to check for end of frame so that we can
                    also count missed frames at the same time.
    
    Arguments:      None
    
    Returns:        None
 *---------------------------------------------------------------------------*/
static void __NoHangDoneRender()
{
    BOOL abort = FALSE;
    GX::CopyDisp(hwCurrentBuffer, GX_TRUE);
    GX::SetDrawSync( hw_END_FRAME_TOKEN );
    
    FrameCount = 0;

    while( (GX::ReadDrawSync() != hw_END_FRAME_TOKEN ) && !abort )
    {
        if (FrameCount >= FrameMissThreshold)
        {
            OSReport("---------WARNING : ABORTING FRAME----------\n");
            abort = TRUE;
            __hwDiagnoseHang();
            hwReInit(rmode); // XXX RMODE?
            // re-enable counters for post-hang diagnosis
            hwSetGPHangMetric( GX_TRUE );
        }
    }
    
    hwSwapBuffers();
}

/*---------------------------------------------------------------------------*
    Name:           hwSetGPHangMetric
    
    Description:    Sets up the GP performance counters in such a way to
                    enable us to detect the cause of a GP hang.  Note that
                    this takes over the performance counters, and you cannot
                    use GXSetGPMetric or GXInitXFRasMetric while you have
                    hwSetGPHangMetric enabled.
    
    Arguments:      enable:  set to GX_TRUE to enable the counters.
                             set to GX_FALSE to disable the counters.
    
    Returns:        None
 *---------------------------------------------------------------------------*/
void hwSetGPHangMetric( GXBool enable )
{
#ifdef EMU
    #pragma unused (enable)
#else
    if (enable)
    {
        // Ensure other counters are off
        GX::SetGPMetric( GX_PERF0_NONE, GX_PERF1_NONE );

		GX::EnableHang();
	}
    else
    {
		GX::DisableHang();
    }
#endif
}

/*---------------------------------------------------------------------------*
    Name:           __hwDiagnoseHang
    
    Description:    Reads performance counters (which should have been set
                    up appropriately already) in order to determine why the
                    GP hung.  The counters must be set as follows:

                    GXSetGPHangMetric( GX_TRUE );
    
                    The above call actually sets up multiple counters, which
                    are read using a non-standard method.

    Arguments:      None
    
    Returns:        None
 *---------------------------------------------------------------------------*/
static void __hwDiagnoseHang()
{
    u32 xfTop0, xfBot0, suRdy0, r0Rdy0;
    u32 xfTop1, xfBot1, suRdy1, r0Rdy1;
    u32 xfTopD, xfBotD, suRdyD, r0RdyD;
    GXBool readIdle, cmdIdle, junk;

    // Read the counters twice in order to see which are changing.
    // This method of reading the counters works in this particular case.
    // You should not use this method to read GPMetric counters.
    GX::ReadXfRasMetric( &xfBot0, &xfTop0, &r0Rdy0, &suRdy0 );
    GX::ReadXfRasMetric( &xfBot1, &xfTop1, &r0Rdy1, &suRdy1 );

    // XF Top & Bot counters indicate busy, others indicate ready.
    // Convert readings into indications of who is ready/idle.
    xfTopD = (xfTop1 - xfTop0) == 0;
    xfBotD = (xfBot1 - xfBot0) == 0;
    suRdyD = (suRdy1 - suRdy0) > 0;
    r0RdyD = (r0Rdy1 - r0Rdy0) > 0;

    // Get CP status
    GX::GetGPStatus(&junk, &junk, &readIdle, &cmdIdle, &junk);

    OSReport("GP status %d%d%d%d%d%d --> ",
             readIdle, cmdIdle, xfTopD, xfBotD, suRdyD, r0RdyD);

    // Depending upon which counters are changing, diagnose the hang.
    // This may not be 100% conclusive, but it's what we've observed so far.
    if (!xfBotD && suRdyD)
    {
        OSReport("GP hang due to XF stall bug.\n");
    }
    else if (!xfTopD && xfBotD && suRdyD)
    {
        OSReport("GP hang due to unterminated primitive.\n");
    }
    else if (!cmdIdle && xfTopD && xfBotD && suRdyD)
    {
        OSReport("GP hang due to illegal instruction.\n");
    }
    else if (readIdle && cmdIdle && xfTopD && xfBotD && suRdyD && r0RdyD)
    {
        OSReport("GP appears to be not hung (waiting for input).\n");
    }
    else
    {
        OSReport("GP is in unknown state.\n");
    }
}

/*---------------------------------------------------------------------------*
    Name:           __hwOverflowThread
    
    Description:    This thread is designed to detect a hang when the main
                    thread is suspended due to the high watermark interrupt.
                    If the GP hangs during this time, it is not typically
                    recoverable.  However, we can still diagnose it.
    
    Arguments:      None
    
    Returns:        None
 *---------------------------------------------------------------------------*/
void* __hwOverflowThread( void* param )
{
//    #pragma unused (param)
    GXBool overhi, junk;
    u32 count;
    OSTick tick1, tick2;

    // There are at least two reasons why this thread may run:
    // 1.  Main thread suspended during high watermark.
    // 2.  Main thread suspended during GXDrawDone.
    // Low watermark and GP finish can suspend this thread.

    while (1)
    {
        // Wait for an overflow (hit to high watermark) to occur.
        do 
        {
            GX::GetGPStatus(&overhi, &junk, &junk, &junk, &junk);
        }
        while (!overhi);
        
        // If GP hang workaround is off, don't do anything.
        if (!FrameMissThreshold)
        {
            continue;
        }

        // Get a reading for this overflow and the current tick.
        count = GX::GetOverflowCount();
        tick1 = OSGetTick();
        
        // Wait until either we time-out or until the next overflow.
        // One frame is approximately 17 milliseconds.
        do {
            tick2 = OSGetTick();
        } while (
            OSTicksToMilliseconds(tick2 - tick1) < FrameMissThreshold * 17 &&
            count == GX::GetOverflowCount() );

        // Exiting the above loop does not imply a hang.
        // We must check the overflow status once more to be certain.
        GX::GetGPStatus(&overhi, &junk, &junk, &junk, &junk);

        // If a new overflow started, then start timing again from the top.
        if (!overhi || count != GX::GetOverflowCount())
        {
            continue;
        }
        
        // We timed out.  Report and diagnose the hang.
        OSReport("---------WARNING : HANG AT HIGH WATERMARK----------\n");

        __hwDiagnoseHang();
        
        // We cannot easily recover from this situation.  Halt program.
        OSHalt("Halting program");
    }
}

/*---------------------------------------------------------------------------*
    Name:           hwReInit
    
    Description:    Re-initializes the graphics pipe.  Makes no assumptions 
                    about the Fifo (allowing you to change it in your program
                    if needed).  
    
    Arguments:      mode   render mode object
    
    Returns:        None
 *---------------------------------------------------------------------------*/
void hwReInit( GXRenderModeObj *mode )
{
    // If an application specific render mode is provided,
    // override the default render mode
    if (mode != NULL) 
    {
        rmode = mode;
    }
    else
    {
		rmode = &GXNtsc480IntDf;

        // Trim off from top & bottom 16 scanlines (which will be overscanned).
        // So almost all hws actually render only 448 lines (in NTSC case.)
        // Since this setting is just for SDK hws, you can specify this
        // in order to match your application requirements.
        GX::AdjustForOverscan(rmode, &rmodeobj, 0, 16);
        
        rmode = &rmodeobj;

		OSReport( "TV format: %d\n", VIGetTvFormat() );

		switch (VIGetTvFormat())
		{
			case VI_NTSC:
				if ( NxNgc::EngineGlobals.use_480p )
				{
					rmode->viTVmode = VI_TVMODE_NTSC_PROG;
					rmode->xFBmode = VI_XFBMODE_SF;
				}
			break;
		  case VI_PAL:
			  if ( rmode->viTVmode == VI_TVMODE_NTSC_INT )
			  {
				  if ( NxNgc::EngineGlobals.use_60hz )
				  {
					  rmode->viTVmode = VI_TVMODE_EURGB60_INT;
				  }
				  else
				  {
					  rmode->viTVmode = VI_TVMODE_PAL_INT;
					  rmode->viYOrigin = 23;
				  }
			  }
			  else
			  {
				  if ( NxNgc::EngineGlobals.use_60hz )
				  {
					  rmode->viTVmode = VI_TVMODE_EURGB60_DS;
				  }
				  else
				  {
					  rmode->viTVmode = VI_TVMODE_PAL_DS;
					  rmode->viYOrigin = 23;
				  }
			  }
			  if ( NxNgc::EngineGlobals.use_60hz )
			  {
				  rmode->xfbHeight = 448;
			  }
			  else
			  {
				  rmode->xfbHeight = ( rmode->xfbHeight * 528 ) / 448;
				  rmode->viHeight = 528;
			  }
			break;
		  case VI_MPAL:
			  rmode = &GXMpal480IntDf;
			break;
		  default:
			OSHalt("hwInit: invalid TV format\n");
			break;
		}
		rmode->viWidth = 650;
		rmode->viXOrigin = 35;
	
		VIConfigure(rmode);
		__hwInitVI();
	}



//    // Create a temporary FIFO while the current one is reset
//    GXFifoObj   tmpobj;
//    char*       tmpFifo = new char[64*1024];
//
//    // Get data on current Fifo.
//    GXFifoObj*  realFifoObj = GXGetCPUFifo();
//    void*   realFifoBase = GXGetFifoBase(realFifoObj);
//    u32     realFifoSize = GXGetFifoSize(realFifoObj);
//    
//    // Abort the GP
//    GXAbortFrame();
//  
//    GXInitFifoBase( &tmpobj, tmpFifo, 64*1024);
//    
//    GXSetCPUFifo(&tmpobj);
//    GXSetGPFifo(&tmpobj);    
//
//    /*----------------------------------------------------------------*
//     *  Initialize Graphics again
//     *----------------------------------------------------------------*/
//    __hwInitRenderMode(mode);
//
//    // This will re-initialize the pointers for the original FIFO.
//    DefaultFifoObj  = GXInit(realFifoBase, realFifoSize);
//
//    __hwInitGX();
//    
//    // NOTE: the VI settings do not necessarily have to be reset, but
//    //       just to be safe, we do so anyway
//    VIConfigure(rmode);
//    __hwInitVI();
//
//    // remove the temporary fifo
//	delete tmpFifo;
}

void hwGXInit( void )
{
	__hwInitGX();
}

