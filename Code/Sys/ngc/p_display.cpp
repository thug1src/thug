/********************************************************************************
 *																				*
 *	Module:																		*
 *				NsDisplay														*
 *	Description:																*
 *				Sets up and manages a triple-buffered display. This code is		*
 *				derived from the Nintendo "frb-triple" example.					*
 *				Take a look at Sony's documentation for more information at:	*
 *				$/DolphinSDK1.0/man/demos/gxdemos/Framebuffer/frb-triple.html 	*
 *				Also, check out the original source code at:					*
 *				$/DolphinSDK1.0/build/demos/gxdemo/src/Framebuffer/frb-triple.c *
 *	Written by:																	*
 *				Paul Robinson													*
 *	Copyright:																	*
 *				2001 Neversoft Entertainment - All rights reserved.				*
 *																				*
 ********************************************************************************/

/********************************************************************************
 * Includes.																	*
 ********************************************************************************/
#include "p_hw.h"
//#include <stdlib.h>
#include "p_display.h"
//#include <dolphin\dtk.h>
#include <sys\timer.h>
#include <sys\ngc\p_render.h>
//#include <sys\ngc\p_screenshot.h>
#include <sys\ngc\p_prim.h>
#include <sys\ngc\p_camera.h>
#include <sys\ngc\p_dvd.h>
#include <gel/scripting/script.h> 
#include "gfx/ngc/nx/nx_init.h"
#include <gel/mainloop.h>

#include <sys/ngc/p_display.h>
#include <sys/ngc/p_render.h>
#include <sys/ngc/p_prim.h>
#include <gfx/ngc/nx/nx_init.h>
#include <gfx/ngc/nx/chars.h>
#include <gfx/ngc/nx/render.h>
#include <gfx/nxfontman.h>
#include <gfx/ngc/p_nxfont.h>
#include <gel/scripting/symboltable.h> 

#include <core/defines.h>
#include <core/String/stringutils.h>
#include <gfx/NxFontMan.h>
#include <gel/Scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include 	"gfx\ngc\nx\nx_init.h"
#include "gel\music\ngc\p_music.h"
#include <sys\ngc\p_profile.h>
#include "VIDSimpleAudio.h"

extern GXRenderModeObj *rmode;
extern GXRenderModeObj rmodeobj;
extern int inDrawingContext;

extern DVDCallback last_callback[8];
extern s32 last_callback_length[8];
extern DVDFileInfo* last_callback_fileInfo[8];
extern int last_callback_counter[8];

bool g_need_to_flush = true;

//NsProfile profile_gpu( "gpu", 256 );

//#define __INFO_REPORT__

#define P0_COUNT 35
#define P1_COUNT 22

bool g_legal = false;

#ifdef __INFO_REPORT__
int gp0 = GX_PERF0_XF_WAIT_IN;
int gp1 = GX_PERF1_TX_IDLE;
char g_gp_info0[64*P0_COUNT];
char g_gp_info1[64*P1_COUNT];

int frames = 0;

u32 _p0, _p1;
char * p0_name[] =
{
    "GX_PERF0_VERTICES            ",
	"GX_PERF0_CLIP_VTX            ",
	"GX_PERF0_CLIP_CLKS           ",
	"GX_PERF0_XF_WAIT_IN          ",
	"GX_PERF0_XF_WAIT_OUT         ",
	"GX_PERF0_XF_XFRM_CLKS        ",
	"GX_PERF0_XF_LIT_CLKS         ",
	"GX_PERF0_XF_BOT_CLKS         ",
	"GX_PERF0_XF_REGLD_CLKS       ",
	"GX_PERF0_XF_REGRD_CLKS       ",
	"GX_PERF0_CLIP_RATIO          ",

	"GX_PERF0_TRIANGLES           ",
	"GX_PERF0_TRIANGLES_CULLED    ",
	"GX_PERF0_TRIANGLES_PASSED    ",
	"GX_PERF0_TRIANGLES_SCISSORED ",
	"GX_PERF0_TRIANGLES_0TEX      ",
	"GX_PERF0_TRIANGLES_1TEX      ",
	"GX_PERF0_TRIANGLES_2TEX      ",
	"GX_PERF0_TRIANGLES_3TEX      ",
	"GX_PERF0_TRIANGLES_4TEX      ",
	"GX_PERF0_TRIANGLES_5TEX      ",
	"GX_PERF0_TRIANGLES_6TEX      ",
	"GX_PERF0_TRIANGLES_7TEX      ",
	"GX_PERF0_TRIANGLES_8TEX      ",
	"GX_PERF0_TRIANGLES_0CLR      ",
	"GX_PERF0_TRIANGLES_1CLR      ",
	"GX_PERF0_TRIANGLES_2CLR      ",

	"GX_PERF0_QUAD_0CVG           ",
	"GX_PERF0_QUAD_NON0CVG        ",
	"GX_PERF0_QUAD_1CVG           ",
	"GX_PERF0_QUAD_2CVG           ",
	"GX_PERF0_QUAD_3CVG           ",
	"GX_PERF0_QUAD_4CVG           ",
	"GX_PERF0_AVG_QUAD_CNT        ",

	"GX_PERF0_CLOCKS              ",
	"GX_PERF0_NONE                "

};

/********************************/
char * p1_name[] =
{
	"GX_PERF1_TEXELS              ",
	"GX_PERF1_TX_IDLE             ",
	"GX_PERF1_TX_REGS             ",
	"GX_PERF1_TX_MEMSTALL         ",
	"GX_PERF1_TC_CHECK1_2         ",
	"GX_PERF1_TC_CHECK3_4         ",
	"GX_PERF1_TC_CHECK5_6         ",
	"GX_PERF1_TC_CHECK7_8         ",
	"GX_PERF1_TC_MISS             ",

	"GX_PERF1_VC_ELEMQ_FULL       ",
	"GX_PERF1_VC_MISSQ_FULL       ",
	"GX_PERF1_VC_MEMREQ_FULL      ",
	"GX_PERF1_VC_STATUS7          ",
	"GX_PERF1_VC_MISSREP_FULL     ",
	"GX_PERF1_VC_STREAMBUF_LOW    ",
	"GX_PERF1_VC_ALL_STALLS       ",
	"GX_PERF1_VERTICES            ",

	"GX_PERF1_FIFO_REQ            ",
	"GX_PERF1_CALL_REQ            ",
	"GX_PERF1_VC_MISS_REQ         ",
	"GX_PERF1_CP_ALL_REQ          ",

	"GX_PERF1_CLOCKS              ",
	"GX_PERF1_NONE                "

};
#endif		// __INFO_REPORT__

/********************************************************************************
 * Defines.																		*
 ********************************************************************************/
#define QUEUE_MAX 5
#define QUEUE_EMPTY QUEUE_MAX

u8 g_blur = 0;

extern bool		gLoadingBarActive;
extern int			gLoadBarTotalFrames;
extern int			gLoadBarNumFrames;
extern int			gLoadBarX;
extern int			gLoadBarY;
extern int			gLoadBarWidth;
extern int			gLoadBarHeight;
extern int			gLoadBarStartColor[4];
extern int			gLoadBarDeltaColor[4];
extern int			gLoadBarBorderWidth;
extern int			gLoadBarBorderHeight;
extern int			gLoadBarBorderColor[4];

GXColor				messageColor = {0,0,0,255};
static float		lasty = 0;
static float		prevx = 0;
static float		prevy = 0;
static int			selection = 0;
static int			selection_max = 0;
static int			reset_enabled_frames = 0;

/********************************************************************************
 * Structures.																	*
 ********************************************************************************/
typedef struct QItem_ 
{
    void* writePtr;
    void* dataPtr;
    void* copyXFB;
} NsDisplay_QItem;

typedef struct Queue_
{
    NsDisplay_QItem entry[QUEUE_MAX];
    u16 top;
    u16 bot;
} NsDisplay_Queue;

/********************************************************************************
 * Local variables.																*
 ********************************************************************************/

void	(*pIconCallback)( void )	= NULL;

static NsDisplay_Queue	RenderQ;				// Queue for frames in FIFO
static NsDisplay_Queue	DoneQ;					// Queue for frames finished already

static void			  * myXFB1;					// Pointers to the two XFB's
static void			  * myXFB2;
static void			  * copyXFB;				// Which XFB to copy to next
static void			  * dispXFB;				// Which XFB is being displayed now

static GXBool			BPSet;					// Is the FIFO breakpoint set?
static GXBool			BPWait;					// Is breakpt reset waiting on VBlank?
static GXBool			BPGo;					// Indicates breakpt should be released

static u16				lastVCBToken;			// Last sync token the VBlank callback saw
static u16				newToken;				// Value to use for new sync token.

static OSThreadQueue	waitingDoneRender;		// Threads waiting for frames to finish

static OSThread			CUThread;				// OS data for clean-up thread
static u8				CUThreadStack[4096];	// Stack for clean-up thread

static OSAlarm			s_bg_alarm;
static OSThread			s_bg_thread;
static u8				s_bg_thread_stack[4096];

static int				initCount = 0;			// We can only initialize this once.

static int				inDisplayContext = 0;

int						resetDown = 0;			// Whether the reset button has been pressed.

static int				currentBuffer = 0;		// Current buffer to use for triple buffering.
/********************************************************************************
 * Forward references.															*
 ********************************************************************************/
static void				BPCallback			( void );
static void				SetNextBreakPt		( void );
static void				VIPreCallback		( u32 retraceCount );
static void				VIPostCallback		( u32 retraceCount );
static void			  * CleanupThread		( void * param );
static void			  * bg_thread_func		( void * param );
static void				bg_alarm_handler	( OSAlarm* alarm, OSContext* context );

static void				init_queue			( NsDisplay_Queue *q );
static void				enqueue				( NsDisplay_Queue *q, NsDisplay_QItem *qitm );
static NsDisplay_QItem	dequeue				( NsDisplay_Queue *q );
static NsDisplay_QItem	queue_front			( NsDisplay_Queue *q );
static GXBool			queue_empty			( NsDisplay_Queue *q );
static u32				queue_length		( NsDisplay_Queue *q );

static NsDisplay_StartRenderingCallback	startCB = NULL;
static NsDisplay_EndRenderingCallback	endCB = NULL;


/********************************************************************************
 * Externs.																		*
 ********************************************************************************/
extern void			  * hwFrameBuffer1;        // Where to find XFB info
extern void			  * hwFrameBuffer2;
extern PADStatus		padData[PAD_MAX_CONTROLLERS]; // game pad state

//static void* MyAlloc( u32 size )
//{
//	Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
//	void* p = new char[size];
//	Mem::Manager::sHandle().BottomUpHeap()->PopAlign();
//
//	return p;
//}
//
//static void MyFree( void* block )
//{
//	delete (char *)block;
//}

static u32 Update_input( void )
{

  int i;
//  u32 ResetReq = 0;
//  PADStatus Pad[PAD_MAX_CONTROLLERS];
  u32 PadButtonDownVal;
  static u16 buttonLast[4] ={ 0, 0, 0, 0 };

  //
  // Read current PAD status and clamp the analog inputs
  //

//  hwPadRead();

//  PADRead(Pad);
//  PADClamp(Pad);

//  //
//  // Do we have an input device handle yet?
//  //
//
//  for ( i = 0 ; i < PAD_MAX_CONTROLLERS ; i++ )
//  {
//    if ( padData[i].err == PAD_ERR_TRANSFER )
//    {
//      return( 0 );
//    }
//    else if ( padData[i].err == PAD_ERR_NONE )
//    {
//      break;
//    }
//    else if ( padData[i].err == PAD_ERR_NO_CONTROLLER )
//    {
//      ResetReq |= (PAD_CHAN0_BIT >> i);
//    }
//  }
//
//  //
//  // A pad isn't plugged in
//  //
//
//  if ( i == PAD_MAX_CONTROLLERS )
//  {
//    //
//    // Reset pad channels which have been not valid
//    //
//
//    if ( ResetReq )
//    {
//      PADReset( ResetReq );
//    }
//
//    buttonLast[0] = 0;
//    buttonLast[1] = 0;
//    buttonLast[2] = 0;
//    buttonLast[3] = 0;
//    return( 0 );
//  }

  //
  // Get the button downs and save the current state
  //

  PadButtonDownVal = 0;
  for ( i = 0 ; i < PAD_MAX_CONTROLLERS ; i++ )
  {
	  PadButtonDownVal |= PADButtonDown ( buttonLast[i], padData[i].button );
	  buttonLast[i] = padData[i].button;
  }


//  //
//  // Handle the loop control
//  //
//
//  if ( PadButtonDownVal & PAD_BUTTON_START )
//  {
//    Loop_current = ! Loop_current;
//  }
//
//  //
//  // Handle the skip to the next files
//  //
//
//  if ( Pad[i].button & PAD_BUTTON_A )
//  {
//    return( 1 );
//  }
//
//  return( 0 );

  return PadButtonDownVal;
}

void NsDisplay::Check480P( void )
{
	// See if the 480p/widescreen menu should be displayed
	u32 buttons = 0;

	NxNgc::EngineGlobals.screen_brightness = 0.0f;
	for ( int lp = 0; lp < 4; lp++ )
	{
		NsDisplay::begin();
		NsRender::begin();
		NsRender::end();
		NsDisplay::end( true );

		for ( int i = 0 ; i < PAD_MAX_CONTROLLERS ; i++ )
		{
			buttons |= padData[i].button;
		}
	}

	NxNgc::EngineGlobals.screen_brightness = 1.0f;

	// If we have a cable connected, we can continue...
	if ( VIGetDTVStatus() )
	{
		if ( ( buttons & PAD_BUTTON_B ) || OSGetProgressiveMode() )
		{
			while ( !( buttons & PAD_BUTTON_A ) )
			{
				// Render the text.
				NsDisplay::begin();
				NsRender::begin();
	
				NsCamera cam;
				cam.orthographic( 0, 0, 640, 448 );
	
				// Draw the screen.
				NsPrim::begin();
	
				cam.begin();
	
				GX::SetZMode( GX_FALSE, GX_ALWAYS, GX_TRUE );
	
				NxNgc::set_blend_mode( NxNgc::vBLEND_MODE_BLEND );
	
				Script::RunScript( "ngc_widescreen" );
	
				NsDisplay::setBackgroundColor( messageColor );
	
				cam.end();
	
				NsPrim::end();
	
				NsRender::end();
				NsDisplay::end( true );
	
				buttons = Update_input();
	
				if ( buttons & PAD_BUTTON_DOWN ) selection++;
				if ( buttons & PAD_BUTTON_UP ) selection--;
				if ( selection < 0 ) selection = 0;
				if ( selection >= selection_max ) selection = selection_max - 1;
			}
			// Clear to black.
			NsDisplay::setBackgroundColor( (GXColor){0,0,0,0} );
			NsDisplay::begin();
			NsDisplay::end( true );
			NsDisplay::begin();
			NsDisplay::end( true );

			// Run the selected script.
			char buf[32];
			sprintf( buf, "ngc_select%d", selection );
			Script::RunScript( buf );
		}
	}
	else
	{
		// No cable, turn off progressive mode.
		OSSetProgressiveMode(0);
	}

	// Must display licensed by nintendo screen.
	display_legal();
	g_legal = true;
}

void NsDisplay::Check60Hz( void )
{
	// See if the 60Hz menu should be displayed
	u32 buttons = 0;

	NxNgc::EngineGlobals.screen_brightness = 0.0f;
	for ( int lp = 0; lp < 4; lp++ )
	{
		NsDisplay::begin();
		NsRender::begin();
		NsRender::end();
		NsDisplay::end( true );

		for ( int i = 0 ; i < PAD_MAX_CONTROLLERS ; i++ )
		{
			buttons |= padData[i].button;
		}
	}

	NxNgc::EngineGlobals.screen_brightness = 1.0f;

	// If we press the B button, or we're already in 60hz mode...
	if ( ( buttons & PAD_BUTTON_B ) || OSGetEuRgb60Mode() )
	{
		while ( !( buttons & PAD_BUTTON_A ) )
		{
			// Render the text.
			NsDisplay::begin();
			NsRender::begin();

			NsCamera cam;
			cam.orthographic( 0, 0, 640, 448 );

			// Draw the screen.
			NsPrim::begin();

			cam.begin();

			GX::SetZMode( GX_FALSE, GX_ALWAYS, GX_TRUE );

			NxNgc::set_blend_mode( NxNgc::vBLEND_MODE_BLEND );

			Script::RunScript( "ngc_pal60" );

			NsDisplay::setBackgroundColor( messageColor );

			cam.end();

			NsPrim::end();

			NsRender::end();
			NsDisplay::end( true );

			buttons = Update_input();

			if ( buttons & PAD_BUTTON_DOWN ) selection++;
			if ( buttons & PAD_BUTTON_UP ) selection--;
			if ( selection < 0 ) selection = 0;
			if ( selection >= selection_max ) selection = selection_max - 1;
		}
		// Clear to black.
		NsDisplay::setBackgroundColor( (GXColor){0,0,0,0} );
		NsDisplay::begin();
		NsDisplay::end( true );
		NsDisplay::begin();
		NsDisplay::end( true );

		// Run the selected script.
		char buf[32];
		sprintf( buf, "ngc_selectPAL%d", selection );
		Script::RunScript( buf );
	}

	// Must display licensed by nintendo screen.
	display_legal();
	g_legal = true;
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				NsDisplay															*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Initializes the OS, pad, DVD, video functions from the Nintendo	*
 *				library, using hwInit(). The display defaults to				*
 *				GXNtsc480IntDf. Gamma defaults to GX_GM_1_0.					*
 *				Triple buffering is also set up here, with the display turned	*
 *				off as a default. The display will become visible once the		*
 *				first frame has been rendered. This will happen at some point	*
 *				after the end() function is called.								*
 *																				*
 ********************************************************************************/
void NsDisplay::init( void )
{
//    GXFifoObj * fifo;
//    GXFifoObj * fifoCPUCurrent;

#define FIFO_SIZE (256*1024)
#define FIFO_DRAWDONE_TOKEN  0xBEEF
#define FIFO_DRAWING_TOKEN   0xCACE

	Dbg_MsgAssert ( initCount == 0, ( "Display module can only be instanced once.\nThis is the 2nd instance of this class.\n" ) );
	initCount++;

	hwInit(NULL);		// Init the OS, game pad, graphics and  video.






//    fifo = (GXFifoObj *)OSAlloc( sizeof(GXFifoObj) );
//
//    // get the default fifo and free it
//    GXSetDrawDone();
//    fifoCPUCurrent = GXGetCPUFifo();
//
//    // allocate new fifos
//    GXInitFifoBase( fifo, OSAlloc(FIFO_SIZE), FIFO_SIZE );
//
//    // set the CPU and GP fifo
//    GXSetCPUFifo( fifo );
//    GXSetGPFifo( fifo );        
//
//    // set a drawdone token in the fifo
//    GXSetDrawSync( FIFO_DRAWDONE_TOKEN );
//
////    // install a callback so we can capture the GP rendering time
////    GXSetDrawDoneCallback( CheckRenderingTime );
//
//    // free the default fifo when we set GP fifo to a different fifo
//    OSFree( GXGetFifoBase(fifoCPUCurrent) );










	GX::SetDispCopyGamma( GX_GM_1_0 );

	BPSet  = GX_FALSE;
	BPWait = GX_FALSE;
	BPGo   = GX_FALSE;

	lastVCBToken = 0;
	newToken = 1;


    init_queue(&RenderQ);
    init_queue(&DoneQ);

    OSInitThreadQueue( &waitingDoneRender );

    // Creates a new thread. The thread is suspended by default.
    OSCreateThread(
        &CUThread,                          // ptr to the thread to init
        CleanupThread,               		// ptr to the start routine
        0,                                  // param passed to start routine
        CUThreadStack+sizeof(CUThreadStack),// initial stack address
        sizeof CUThreadStack,
        14,                                 // scheduling priority
        OS_THREAD_ATTR_DETACH);             // detached by default

    // Starts the thread
    OSResumeThread(&CUThread);

    myXFB1 = hwFrameBuffer1;
    myXFB2 = hwFrameBuffer2;
    dispXFB = myXFB1;
    copyXFB = myXFB2;

    (void) VISetPreRetraceCallback(VIPreCallback);
    (void) VISetPostRetraceCallback(VIPostCallback);
    (void) GX::SetBreakPtCallback(BPCallback);

    // The screen won't actually unblank until the first frame has
    // been displayed (until VIFlush is called and retrace occurs).
    VISetBlack(FALSE);

	// This should move to a target/platform module or something...
//	OSInitFastCast();

	// Set XF Stall bug to on.
//	GXSetMisc( GX_MT_XF_FLUSH, GX_XF_FLUSH_SAFE );

	// Create & start the background thread.
	OSCreateThread(
        &s_bg_thread,                          // ptr to the thread to init
        bg_thread_func,               		// ptr to the start routine
        0,                                  // param passed to start routine
		s_bg_thread_stack+sizeof(s_bg_thread_stack),// initial stack address
        sizeof s_bg_thread_stack,
        14,                                 // scheduling priority
        OS_THREAD_ATTR_DETACH);             // detached by default
	OSSetAlarm( &s_bg_alarm, OSMillisecondsToTicks( (long long int)( 1000.0f / 60.0f ) ), bg_alarm_handler );
//	OSResumeThread( &s_bg_thread );
	}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				begin															*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Calls hwBeforeRender, which sets up the viewport, and			*
 *				invalidates the vertex and texture cache.						*
 *																				*
 ********************************************************************************/
void NsDisplay::begin ( void )
{
	NsDisplay::doReset();
	if ( inDisplayContext == 1 ) return;

    hwBeforeRender();

    // We must keep latency down while still keeping the FIFO full.
    // We allow only two frames to be in the FIFO at once.

	// This is a critical section that requires no interrupts to
	// happen in between the "if" and the "sleep".  The sleep will
	// reenable interrupts, allowing one to wake up this thread.

	int enabled = OSDisableInterrupts();
	if (queue_length(&RenderQ) > 1) 
	{
		OSSleepThread( &waitingDoneRender );
	}
	OSRestoreInterrupts(enabled);
	
	// Read PAD data.
//	PADRead( padData );
	
	inDisplayContext = 1;
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				end																*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Queues up the current display list for rendering. Also swaps	*
 *				the current draw/display XFB.									*
 *																				*
 ********************************************************************************/
void NsDisplay::end ( bool clear )
{
	if ( !gLoadingBarActive ) g_need_to_flush = true;

    void* tmp_read;
    void* tmp_write;
    NsDisplay_QItem qitm;
    int   enabled;

	if ( inDisplayContext == 0 ) return;


//	//************************************
//
//	GX::SetNumTevStages( 1 );
//	GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
//	GX::SetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//	GX::SetNumTexGens( 0 );
//	GX::SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//	GX::SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
//	GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR );
//	GX::SetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA );
//	GX::SetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC );
//	GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_VTX, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//	GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//
//	int dc = inDrawingContext;
//	inDrawingContext = 1;
//	profile_gpu.histogram( 32, 32, 96, 96, (GXColor){0,128,255,255} );
//	inDrawingContext = dc;
//
//	//************************************

    // End of frame code:
    GX::Flush();
    
    GX::GetFifoPtrs(GX::GetCPUFifo(), &tmp_read, &tmp_write);

    // Create new render queue item
    qitm.writePtr = tmp_write;
    qitm.dataPtr = NULL;        // pointer to frame-related user data
    qitm.copyXFB = copyXFB;
    
    // Technically, you can work this such that you don't
    // need the OSDisabled interrupts.  You need to rework
    // the enqueue/dequeue routines a bit, though, to make
    // them non-interfere with each other.

    enabled = OSDisableInterrupts();
    enqueue(&RenderQ, &qitm);
    OSRestoreInterrupts(enabled);

    if (BPSet == GX_FALSE) {
    
        BPSet = GX_TRUE;
        GX::EnableBreakPt( tmp_write );
		if ( startCB ) startCB();
//		profile_gpu.start();

#ifdef __INFO_REPORT__
		GX::ReadGPMetric( &_p0, &_p1 );
#endif		// __INFO_REPORT__

#ifdef __INFO_REPORT__
//		GXClearPixMetric();
		GX::SetGPMetric( (GXPerf0)gp0, (GXPerf1)gp1 );

		GX::ClearGPMetric();
#endif		// __INFO_REPORT__
    }

    GX::SetDrawSync( newToken );
	
	//BEGIN SCREENSHOT CODE
//	SCREENSHOTService( hwGetCurrentBuffer(), MyAlloc, MyFree );
	//END SCREENSHOT CODE

	u8 newFilter[7];
	
	newFilter[0] = 8 + ( ( g_blur * 24 ) / 8 );
	newFilter[1] = 8 - ( ( g_blur * 8 ) / 8 );
	newFilter[2] = 10 - ( ( g_blur * 10 ) / 8 );
	newFilter[3] = 12 - ( ( g_blur * 12 ) / 8 );
	newFilter[4] = 10 - ( ( g_blur * 10 ) / 8 );
	newFilter[5] = 8 - ( ( g_blur * 8 ) / 8 );
	newFilter[6] = 8 + ( ( g_blur * 24 ) / 8 );

	for ( int i = 0; i < 7; ++i )
	{
		newFilter[i] = u8( (float)newFilter[i] * NxNgc::EngineGlobals.screen_brightness );
	}

	GX::SetCopyFilter( rmode->aa, rmode->sample_pattern, GX_TRUE, newFilter );
	GX::CopyDisp( copyXFB, ( clear ) ? GX_TRUE : GX_FALSE );
	GX::SetCopyFilter( rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter );
	
//	GXCopyDisp( copyXFB, ( clear ) ? GX_TRUE : GX_FALSE );

//	GXCopyDisp( copyXFB, GX_FALSE );
	GX::Flush();

#ifdef __INFO_REPORT__
	frames++;
	sprintf( &g_gp_info0[gp0*64], "%s: %9d", p0_name[gp0], (int)_p0 / frames );
	sprintf( &g_gp_info1[gp1*64], "%s: %9d", p1_name[gp1], (int)_p1 / frames );
//	if ( ( p0 > 10 ) || ( p1 > 10 ) ) OSReport( "p0/p1: %8d - %8d\n", p0, p1 );

//	gp0++;
//	gp1++;
//	if ( gp0 >= P0_COUNT ) gp0 = 0;
//	if ( gp1 >= P1_COUNT ) gp1 = 0;
#endif		// __INFO_REPORT__

    newToken++;
    copyXFB = (copyXFB == myXFB1) ? myXFB2 : myXFB1;

	inDisplayContext = 0;

	currentBuffer++;
	if ( currentBuffer == 2 ) currentBuffer = 0;
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				setBackgroundColor												*
 *	Inputs:																		*
 *				color	The rgba color to set the background to.				*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Sets the background color to the specified color. At the		*
 *				beginning of drawing, the screen is cleared to a specific		*
 *				color - it defaults to 0,0,0,0.									*
 *																				*
 ********************************************************************************/
void NsDisplay::setBackgroundColor ( GXColor color )
{
    GX::SetCopyClear ( color, 0x00ffffff );
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				setRenderStartCallback											*
 *	Inputs:																		*
 *				pCB	The callback to set - NULL means no callback.				*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Sets up the callback to be called when rendering for a frame	*
 *				actually begins - note that when you call NsDisplay::begin or	*
 *				NsDisplay::End bears no relation to when the rendering actually	*
 *				starts or stops. This function is intended to be useful for		*
 *				profiling the performance of the graphics processor.			*
 *																				*
 ********************************************************************************/
void NsDisplay::setRenderStartCallback ( NsDisplay_StartRenderingCallback pCB )
{
	startCB = pCB;
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				setRenderEndCallback											*
 *	Inputs:																		*
 *				pCB	The callback to set - NULL means no callback.				*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Sets up the callback to be called when rendering for a frame	*
 *				actually ends - note that when you call NsDisplay::begin or		*
 *				NsDisplay::End bears no relation to when the rendering actually	*
 *				starts or stops. This function is intended to be useful for		*
 *				profiling the performance of the graphics processor.			*
 *																				*
 ********************************************************************************/
void NsDisplay::setRenderEndCallback ( NsDisplay_EndRenderingCallback pCB )
{
	endCB = pCB;
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				flush															*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Waits for all queued drawing commands to be executed. Use this	*
 *				if you want to make sure that the graphics processor is not		*
 *				reliant upon a piece of memory you want to free up (such as		*
 *				rendering a loading screen texture, then freeing it).			*
 *																				*
 ********************************************************************************/
void NsDisplay::flush ( void )
{
	if ( !g_need_to_flush ) return;
	g_need_to_flush = false;
	for ( int lp = 0; lp < 3; lp++ ) {
//		begin();
		hwBeforeRender();
		
		// End of frame code:
		GX::Flush();

		VIWaitForRetrace();

//		GXGetFifoPtrs(GXGetCPUFifo(), &tmp_read, &tmp_write);
//
//		// Create new render queue item
//		qitm.writePtr = tmp_write;
//		qitm.dataPtr = NULL;        // pointer to frame-related user data
//		qitm.copyXFB = copyXFB;
//
//		// Technically, you can work this such that you don't
//		// need the OSDisabled interrupts.  You need to rework
//		// the enqueue/dequeue routines a bit, though, to make
//		// them non-interfere with each other.
//
//		enabled = OSDisableInterrupts();
//		enqueue(&RenderQ, &qitm);
//		OSRestoreInterrupts(enabled);
//
//		if (BPSet == GX_FALSE) {
//
//			BPSet = GX_TRUE;
//			GXEnableBreakPt( tmp_write );
//		}
//
//		GXSetDrawSync( newToken );
//		GXCopyDisp( copyXFB, ( clear ) ? GX_TRUE : GX_FALSE );
//		GXFlush();
//
//		newToken++;
//		copyXFB = (copyXFB == myXFB1) ? myXFB2 : myXFB1;


	}

    int enabled = OSDisableInterrupts();
	currentBuffer = 0;		// Current buffer to use for triple buffering.
	BPSet  = GX_FALSE;
	BPWait = GX_FALSE;
	BPGo   = GX_FALSE;

	lastVCBToken = 0;
	newToken = 1;
    myXFB1 = hwFrameBuffer1;
    myXFB2 = hwFrameBuffer2;
    dispXFB = myXFB1;
    copyXFB = myXFB2;

    init_queue(&RenderQ);
    init_queue(&DoneQ);
    OSRestoreInterrupts(enabled);

//	GXSetDrawDone();
//	GXWaitDrawDone();
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				getCurrentBufferIndex											*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Gets the current buffer index used for triple buffering.		*
 *																				*
 ********************************************************************************/
int NsDisplay::getCurrentBufferIndex ( void )
{
	return currentBuffer;
}



/********************************************************************************
 *																				*
 *	Method:																		*
 *				shouldReset														*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *																				*
 *																				*
 ********************************************************************************/
bool NsDisplay::shouldReset( void )
{
	return ( ( resetDown == 2 ) || NxNgc::EngineGlobals.resetToIPL );
}



/********************************************************************************
 *																				*
 *	Method:																		*
 *				doReset															*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *																				*
 *																				*
 ********************************************************************************/
//static volatile bool	flushComplete;
//
//static void flushCallback( void )
//{
//	flushComplete = true;
//}
//

//static void _color( GXColor col )
//{
//	NsDisplay::setBackgroundColor( col );
//
//	for ( int lp = 0; lp < 4; lp++ )
//	{
//		NsDisplay::begin();
//		NsRender::begin();
//		NsRender::end();
//		NsDisplay::end(true);
//	}
//}

#define _color(a,b,c,d)

void NsDisplay::doReset( bool hard_reset, bool forceMenu )
{
	// Reset has to be enabled for 4 frames to be recognized.
	if ( reset_enabled_frames < 4 )
	{
		// Count up if reset is not disabled.
		if ( !NxNgc::EngineGlobals.disableReset )
		{
			reset_enabled_frames++;
		}

	}
	// If reset is disabled, the counting starts again at 0.
	if ( NxNgc::EngineGlobals.disableReset )
	{
		reset_enabled_frames = 0;
	}

	if ( NsDisplay::shouldReset() && !NxNgc::EngineGlobals.disableReset && ( reset_enabled_frames == 4 ) )
	{
//		NsDisplay::doReset(false, false);
	}
	else
	{
		return;
	}

	_color( (GXColor){128,128,128,255} );

	bool reset = false;
	bool hard = false;
	bool force = false;
	// Reset if we pressed the button, but ony if it's not disabled.
	if ( NsDisplay::shouldReset() && !NxNgc::EngineGlobals.disableReset )
	{
		switch ( DVDError() )
		{
			case DVD_STATE_COVER_OPEN:
			case DVD_STATE_NO_DISK:
			case DVD_STATE_WRONG_DISK:
				// As per lot-check 4.5.
				reset = true;
				hard = true;
				force = false;
//					NsDisplay::doReset( true, false );
				break;
			default:
//					NsDisplay::doReset( false, false );
				reset = true;
				hard = false;
				force = false;
				break;
		}
	}

	if ( NxNgc::EngineGlobals.resetToIPL )
	{
//		NsDisplay::doReset( true, true );
		reset = true;
		hard = true;
		force = true;
	}

	if ( !reset ) return;
	hard_reset = hard;
	forceMenu = force;

//	DTKFlushTracks( flushCallback );
	//while( !flushComplete );

	_color( (GXColor){128,0,0,255} );

	// Recalibrate pads (also stops motor).
	PADRecalibrate( PAD_CHAN0_BIT | PAD_CHAN1_BIT | PAD_CHAN2_BIT | PAD_CHAN3_BIT );

	_color( (GXColor){0,128,0,255} );
	Pcm::PCMAudio_StopMusic( true );
//#ifndef DVDETH
//	DTKFlushTracks( NULL );
//#endif		// DVDETH

	_color( (GXColor){0,0,128,255} );
	VISetBlack( TRUE );
    VIFlush();
    VIWaitForRetrace();
	NxNgc::EngineGlobals.use_60hz = false;
//	NsDisplay::setBackgroundColor((GXColor){ 0, 0, 0, 0 });
//	for( int i = 0; i < 2; ++i )
//	{
//		NsDisplay::begin();
//		NsRender::begin();
//		NsRender::end();
//		NsDisplay::end();
//	}
//	NsDisplay::flush();

	// Determine whether the correct disc is in the drive - if not we need to do a hot reset.
//	bool disk_okay = DVDCheckDisk();

	// Shut down audio libs - flush pending ARQ transfers and wait for DMA to finish.
	_color( (GXColor){255,0,0,255} );
	ARQFlushQueue();
	_color( (GXColor){0,255,0,255} );
	while( ARGetDMAStatus() != 0 );
	_color( (GXColor){0,0,255,255} );

	AXQuit();
	_color( (GXColor){255,255,0,255} );
	AIReset();
	_color( (GXColor){0,255,255,255} );
	ARQReset();
	_color( (GXColor){255,255,255,255} );
	ARReset();

	_color( (GXColor){255,0,255,255} );
	
	// Perform reset.
	if( hard_reset || forceMenu )
	{
		OSResetSystem( OS_RESET_HOTRESET, 0, forceMenu ? TRUE : FALSE );
		return;
	}

//	if( disk_okay )
//	{
		OSResetSystem( OS_RESET_RESTART, 0, FALSE );
//	}	
//	else
//	{
//		OSResetSystem( OS_RESET_HOTRESET, 0, FALSE );
//	}
}



/*---------------------------------------------------------------------------*
   Breakpoint Interrupt Callback
 *---------------------------------------------------------------------------*/

static void BPCallback ( void )
{
//	profile_gpu.stop();

    NsDisplay_QItem qitm;
    
    qitm = queue_front(&RenderQ);

    // Check whether or not the just-finished frame can be
    // copied already or if it must wait (due to lack of a
    // free XFB).  If it must wait, set a flag for the VBlank
    // interrupt callback to take care of it.

    if (qitm.copyXFB == dispXFB) 
    {
        BPWait = GX_TRUE;
    }
    else
    {
		SetNextBreakPt();
    }

	if ( endCB ) endCB();
}

/*---------------------------------------------------------------------------*
   Routine to move breakpoint ahead, deal with finished frames.
 *---------------------------------------------------------------------------*/

static void SetNextBreakPt ( void )
{
    NsDisplay_QItem qitm;

    // Move entry from RenderQ to DoneQ.

    qitm = dequeue(&RenderQ);

    enqueue(&DoneQ, &qitm);

    OSWakeupThread( &waitingDoneRender );

    // Move breakpoint to next entry, if any.

    if (queue_empty(&RenderQ))
    {
        GX::DisableBreakPt();
        BPSet = GX_FALSE;
//		profile_gpu.start();
	}
    else
    {
        qitm = queue_front(&RenderQ);
        GX::EnableBreakPt( qitm.writePtr );
		if ( startCB ) startCB();
    }
}

/*---------------------------------------------------------------------------*
   VI Pre Callback (VBlank interrupt)

   The VI Pre callback should be kept minimal, since the VI registers
   must be set before too much time passes.  Additional bookkeeping is
   done in the VI Post callback.

 *---------------------------------------------------------------------------*/

static void VIPreCallback ( u32 retraceCount )
{
//    #pragma unused (retraceCount)
    u16 token;

    // We don't need to worry about missed tokens, since 
    // the breakpt holds up the tokens, and the logic only
    // allows one token out the gate at a time.

    token = GX::ReadDrawSync();

    // We actually need to use only 1 bit from the sync token.

    if (token == (u16) (lastVCBToken+1))
    {
        lastVCBToken = token;

        dispXFB = (dispXFB == myXFB1) ? myXFB2 : myXFB1;

        VISetNextFrameBuffer( dispXFB );
        VIFlush();

#ifndef DVDETH
		// We swapped the frame buffers. Start audio (if not yet active & waiting)
		// (we make sure we get at most 2 active channels (even if the file has more channels to offer))
		{
			static const u32 mask[1] = {0x00000003};
			VIDSimpleAudioStartPlayback(mask,1);
		}
#endif		// DVDETH

        BPGo = GX_TRUE;
    }
}

/*---------------------------------------------------------------------------*
   VI Post Callback (VBlank interrupt)
 *---------------------------------------------------------------------------*/

static void VIPostCallback ( u32 retraceCount )
{
//    #pragma unused (retraceCount)

    if (BPWait && BPGo)
    {
        SetNextBreakPt();
        BPWait = GX_FALSE;
        BPGo = GX_FALSE;
    }

	// Read PAD data.
	hwPadRead();

//	// Obtain reset button state.
//	if( resetDown == 0 )
//	{
//		if( OSGetResetButtonState())
//		{
//			// Wait for the button to be released.
//			resetDown = 1;
//		}
//	}
//	else
	if( resetDown == 1 )
	{
		if( !OSGetResetButtonState())
		{
			// The button has been released.
			resetDown = 2;
		}
	}

	if( pIconCallback )
	{
		pIconCallback();
	}

	// Call the timer vblank callback.
	Tmr::IncrementVblankCounters();

	OSResumeThread( &s_bg_thread );

	// Issue DVD callback.
	for ( int lp = 0; lp < 8; lp++ )
	{
		if ( last_callback_counter[lp] == 0 )
		{
			last_callback[lp]( last_callback_length[lp], last_callback_fileInfo[lp] );
		}
		if ( last_callback_counter[lp] >= 0 ) last_callback_counter[lp]--;
	}
}

/*---------------------------------------------------------------------------*
   Cleanup Thread
 *---------------------------------------------------------------------------*/

static void * CleanupThread ( void* param )
{
//    #pragma unused (param)
    NsDisplay_QItem qitm;

    while(1) {

        OSSleepThread( &waitingDoneRender );
        
        qitm = dequeue(&DoneQ);

        // Take qitm.dataPtr and do any necessary cleanup.
        // That is, free up any data that only needed to be
        // around for the GP to read while rendering the frame.
    }
}

/*---------------------------------------------------------------------------*
 * Quick and dirty queue implementation.
 *---------------------------------------------------------------------------*/

static void init_queue(NsDisplay_Queue *q)
{
    q->top = QUEUE_EMPTY;
}

static void enqueue(NsDisplay_Queue *q, NsDisplay_QItem *qitm)
{
    if (q->top == QUEUE_EMPTY) 
    {
        q->top = q->bot = 0;
    }
    else
    {
        q->top = (u16) ((q->top+1) % QUEUE_MAX);
    
		Dbg_MsgAssert ( q->top != q->bot, ( "queue overflow" ) );
    }
    
    q->entry[q->top] = *qitm;
}

static NsDisplay_QItem dequeue(NsDisplay_Queue *q)
{
    u16 bot = q->bot;

	Dbg_MsgAssert ( q->top != QUEUE_EMPTY, ( "queue underflow" ) );
    
    if (q->bot == q->top) 
    {
        q->top = QUEUE_EMPTY;
    }
    else
    {
        q->bot = (u16) ((q->bot+1) % QUEUE_MAX);
    }

    return q->entry[bot];
}

static NsDisplay_QItem queue_front(NsDisplay_Queue *q)
{
	Dbg_MsgAssert ( q->top != QUEUE_EMPTY, ( "queue empty" ) );

    return q->entry[q->bot];
}

static GXBool queue_empty(NsDisplay_Queue *q)
{
    return q->top == QUEUE_EMPTY;
}

static u32 queue_length(NsDisplay_Queue *q)
{
    if (q->top == QUEUE_EMPTY) return 0;

    if (q->top > q->bot)
        return (u32) ((s32) q->top - q->bot + 1);
    else
        return (u32) ((s32) (q->top + QUEUE_MAX) - q->bot + 1);
}

static void * bg_thread_func ( void* param )
{
//	int count = 0;
//	int tick = 0;

//	bool waiting_for_reset = false;

    while( 1 )
	{
////
////		if ( NsDisplay::shouldReset() && !waiting_for_reset ) {
////		if ( ( NsDisplay::shouldReset() && NxNgc::EngineGlobals.disableReset ) || ( DVDError() && Script::GetInteger( "allow_dvd_errors" ) ) && !NxNgc::EngineGlobals.gpuBusy ) {
//		if ( ( DVDError() && Script::GetInteger( "allow_dvd_errors" ) ) && !NxNgc::EngineGlobals.gpuBusy ) {
//			//OSReport( "We have to reset now.\n" );
//			waiting_for_reset = true;
//
//			NxNgc::EngineGlobals.screen_brightness = 1.0f;
//
//			// Render the text.
//			NsDisplay::begin();
//			NsRender::begin();
////			GX::SetCullMode ( GX_CULL_NONE );
//
//			NsCamera cam;
//			cam.orthographic( 0, 0, 640, 448 );
//
//			// Draw the screen.
//			NsPrim::begin();
//
//			cam.begin();
//
//			GX::SetZMode( GX_FALSE, GX_ALWAYS, GX_TRUE );
//
//			NxNgc::set_blend_mode( NxNgc::vBLEND_MODE_ADD );
//
////			if ( NsDisplay::shouldReset() )
////			{
////				// Reset message.
////				Script::RunScript( "ngc_reset" );
////			}
////			else
//			{
//				// DVD Error message.
//				switch ( DVDError() )
//				{
//					case DVD_STATE_FATAL_ERROR:
//					case DVD_STATE_RETRY:
//						Script::RunScript( "ngc_dvd_fatal" );
//						break;
//					case DVD_STATE_COVER_OPEN:
//						Script::RunScript( "ngc_dvd_cover_open" );
//						break;
//					case DVD_STATE_NO_DISK:
//						Script::RunScript( "ngc_dvd_no_disk" );
//						break;
//					case DVD_STATE_WRONG_DISK:
//						Script::RunScript( "ngc_dvd_wrong_disk" );
//						break;
//					default:
//						Script::RunScript( "ngc_dvd_unknown" );
//						break;
//				}
//			}
//
//			NsDisplay::setBackgroundColor( messageColor );
//
//			cam.end();
//
//			NsPrim::end();
//
//			NsRender::end();
//			NsDisplay::end( false );
//
//			// Notes:
//			// Parameters to set in script:
//			//
//			// reset_font = "testtitle"
//			// reset_text = "RESET"
//			// reset_text_col = 255,255,255,255
//			// reset_bg_col = 255,0,32,255
//		}

		if ( !NsDisplay::shouldReset() && !DVDError() ) gLoadBarNumFrames++;

		if ( gLoadingBarActive && !NxNgc::EngineGlobals.gpuBusy && !NsDisplay::shouldReset() )
		{
			NsDisplay::begin();
			NsRender::begin();
			NsPrim::begin();

			NsCamera camera2D;
			camera2D.orthographic( 0, 0, 640, 448 );

			camera2D.begin();

			int cur_width = (gLoadBarWidth - 1);
			if (gLoadBarNumFrames < gLoadBarTotalFrames)
			{
				cur_width = (cur_width * gLoadBarNumFrames) / gLoadBarTotalFrames;
			}

			int x1 = gLoadBarX;
			int y1 = gLoadBarY;
			int x2 = x1 + cur_width;
			int y2 = y1 + (gLoadBarHeight - 1);

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

			int border_x1 = x1 - gLoadBarBorderWidth;
			int border_y1 = y1 - gLoadBarBorderHeight;
			int border_x2 = x1 + (gLoadBarWidth - 1) + gLoadBarBorderWidth;
			int border_y2 = y2 + gLoadBarBorderHeight;

			u32 bc = gLoadBarBorderColor[3]|(gLoadBarBorderColor[2]<<8)|(gLoadBarBorderColor[1]<<16)|(gLoadBarBorderColor[0]<<24);
			u32 sc = gLoadBarStartColor[3]|(gLoadBarStartColor[2]<<8)|(gLoadBarStartColor[1]<<16)|(gLoadBarStartColor[0]<<24); 
			u32 ec = end_color[3]|(end_color[2]<<8)|(end_color[1]<<16)|(end_color[0]<<24);

			GX::SetZMode( GX_FALSE, GX_ALWAYS, GX_FALSE );
			GX::SetCurrMtxPosTex03( GX_PNMTX0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY );

			GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
									 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV,
									 GX_TEV_SWAP0, GX_TEV_SWAP0 );
			GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
								 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );

			GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
			GX::SetTexChanTevIndCull( 0, 1, 1, 0, GX_CULL_NONE );
			GX::SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
			GX::SetBlendMode( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_FALSE );

			// Set current vertex descriptor to enable position and color0.
			// Both use 8b index to access their data arrays.
			GX::SetVtxDesc( 2, GX_VA_POS, GX_DIRECT, GX_VA_CLR0, GX_DIRECT );

			// Set material color.
			GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );

			GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_VTX, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );

			// Border
			GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
				GX::Position3f32( border_x1, border_y1, -1.0f );
				GX::Color1u32( bc );
				GX::Position3f32( border_x1, border_y2, -1.0f );
				GX::Color1u32( bc );
				GX::Position3f32( border_x2, border_y2, -1.0f );
				GX::Color1u32( bc );
				GX::Position3f32( border_x2, border_y1, -1.0f );
				GX::Color1u32( bc );
			GX::End();
			
			// Bar
			GX::Begin( GX_QUADS, GX_VTXFMT0, 4 ); 
				GX::Position3f32( x1, y1, -1.0f );
				GX::Color1u32( sc );
				GX::Position3f32( x1, y2, -1.0f );
				GX::Color1u32( sc );
				GX::Position3f32( x2, y2, -1.0f );
				GX::Color1u32( ec );
				GX::Position3f32( x2, y1, -1.0f );
				GX::Color1u32( ec );
			GX::End();

			camera2D.end();

			NsPrim::end();
			NsRender::end();
			NsDisplay::end( false );

//			current_image = ( current_image + 1 ) % p_data->m_NumFrames ;
		}

		// Vsync callback will resume it.
		OSSuspendThread( &s_bg_thread );
    }
}

static void bg_alarm_handler( OSAlarm* alarm, OSContext* context )
{
	// Check the thread has not been resumed yet...
	if( OSIsThreadSuspended( &s_bg_thread ))
	{
		OSResumeThread( &s_bg_thread );
	}
}

void display_legal( void )
{
	NsDisplay::flush();

	int frames;
	if ( !g_legal )
	{
		frames = 10;
	}
	else
	{
		frames = 2;

	}

	for ( int lp = 0; lp < frames; lp++ )
	{
		if ( g_legal )
		{
			NxNgc::EngineGlobals.screen_brightness = 1.0f;
		}
		else
		{
			NxNgc::EngineGlobals.screen_brightness = ( 1.0f * (float)lp ) / (float)frames;
		}
		// Render the text.
		NsDisplay::begin();
		NsRender::begin();

		NsCamera cam;
		cam.orthographic( 0, 0, 640, 448 );

		// Draw the screen.
		NsPrim::begin();

		cam.begin();

		GX::SetZMode( GX_FALSE, GX_ALWAYS, GX_FALSE );

		NxNgc::set_blend_mode( NxNgc::vBLEND_MODE_BLEND );

		Script::RunScript( "ngc_license" );

		NsDisplay::setBackgroundColor( messageColor );

		cam.end();

		NsPrim::end();

		NsRender::end();
		NsDisplay::end( true );
	}
}

namespace Nx
{

bool ScriptNgc_BGColor(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	int r = messageColor.r;
	int g = messageColor.g;
	int b = messageColor.b;
	int a = messageColor.a;

	pParams->GetInteger("r",&r);
	pParams->GetInteger("g",&g);
	pParams->GetInteger("b",&b);
	pParams->GetInteger("a",&a);

	messageColor.r = r;
	messageColor.g = g;
	messageColor.b = b;
	messageColor.a = a;

	return true;
}

bool ScriptNgc_Message(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	NxNgc::SText message;
	Nx::CFont * p_cfont;
	const char * p_font_name = NULL;
	const char * p_text = NULL;

	if ( !pParams->GetString( "font", &p_font_name ) ) return false;
	if ( !pParams->GetString( "text", &p_text ) ) return false;
	float y = lasty;
	pParams->GetFloat( "y", &y );

	// We can only draw a text string if we have a font & a string.
	if ( p_font_name && p_text )
	{
		p_cfont = Nx::CFontManager::sGetFont( p_font_name );
		if ( !p_cfont )
		{
			Nx::CFontManager::sLoadFont( p_font_name );
			p_cfont = Nx::CFontManager::sGetFont( p_font_name );
			if ( !p_cfont ) return true;
		}
		message.mp_string = (char *)p_text;

		int sr = 128;
		int sg = 128;
		int sb = 128;
		int sa = 255;
		pParams->GetInteger( "r", &sr );
		pParams->GetInteger( "g", &sg );
		pParams->GetInteger( "b", &sb );
		pParams->GetInteger( "a", &sa );
		message.m_rgba = sa | ( sb << 8 ) | ( sg << 16 ) | ( sr << 24 );

		Nx::CNgcFont * p_nfont = static_cast<Nx::CNgcFont*>( p_cfont );
		NxNgc::SFont * p_font = p_nfont->GetEngineFont();
		message.mp_font = p_font;
		float w, h;
		if ( p_font )
		{
			float x = 0.0f;
			message.m_ypos = y;
			float scale = 1.0f;
			pParams->GetFloat( "scale", &scale );
			message.m_xscale = scale;
			message.m_yscale = scale;
			message.m_color_override = false;

			p_font->QueryString( message.mp_string, w, h );
			if ( pParams->GetFloat( "x", &x ) )
			{
				message.m_xpos = x;
			}
			else
			{
				message.m_xpos = ( 640.0f - ( w * scale ) ) / 2.0f;
			}
	
			float appendx = 0.0f;
			float appendy = 0.0f;
			bool append = false;
			if ( pParams->GetFloat( "appendx", &appendx ) || pParams->GetFloat( "appendy", &appendy ) )
			{
				append = true;
				pParams->GetFloat( "appendx", &appendx );
				pParams->GetFloat( "appendy", &appendy );
				message.m_xpos = prevx + appendx;
				message.m_ypos = prevy + appendy;
			}

			message.DrawSingle();
	
			prevx = message.m_xpos + ( w * scale );
			prevy = y;

			if ( !append )
			{
				lasty = y + (float)((int)( h * scale ));
			}
		}
	}

	return true;
}

bool ScriptNgc_Menu(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	NxNgc::SText message;
	Nx::CFont * p_cfont;
	const char * p_font_name = NULL;
	int items = 0;
	const char * p_item;
	char buf[32];
	int sr = 128;
	int sg = 128;
	int sb = 128;
	int sa = 255;
	int ur = 128;
	int ug = 128;
	int ub = 128;
	int ua = 255;

	pParams->GetString( "font", &p_font_name );
	pParams->GetInteger( "items", &items );
	pParams->GetInteger( "sr", &sr );
	pParams->GetInteger( "sg", &sg );
	pParams->GetInteger( "sb", &sb );
	pParams->GetInteger( "sa", &sa );
	pParams->GetInteger( "ur", &ur );
	pParams->GetInteger( "ug", &ug );
	pParams->GetInteger( "ub", &ub );
	pParams->GetInteger( "ua", &ua );

	// We can only draw a text string if we have a font & a string.
	if ( p_font_name && items )
	{
		Nx::CFontManager::sLoadFont( p_font_name );
		p_cfont = Nx::CFontManager::sGetFont( p_font_name );

		for ( int lp = 0; lp < items; lp++ )
		{
			p_item = NULL;
			sprintf( buf, "item%d", lp );
			pParams->GetString( buf, &p_item );

			if ( p_item )
			{
				message.mp_string = (char *)p_item;

				float scale = 1.0f;
				pParams->GetFloat( "scale", &scale );

				if ( lp == selection )
				{
					message.m_rgba = sa | ( sb << 8 ) | ( sg << 16 ) | ( sr << 24 );
				}
				else
				{
					message.m_rgba = ua | ( ub << 8 ) | ( ug << 16 ) | ( ur << 24 );
				}

				Nx::CNgcFont * p_nfont = static_cast<Nx::CNgcFont*>( p_cfont );
				NxNgc::SFont * p_font = p_nfont->GetEngineFont();
				message.mp_font = p_font;
				float w, h;
				p_font->QueryString( message.mp_string, w, h );
				message.m_xpos = ( 640.0f - ( w * scale ) ) / 2.0f;
				message.m_ypos = lasty;
				message.m_xscale = scale;
				message.m_yscale = scale;
				message.m_color_override = false;

				message.DrawSingle();

				lasty += (float)((int)( h * scale ));
			}
		}
	}

	selection_max = items;

	return true;
}

bool ScriptNgc_Set480P(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	OSSetProgressiveMode(1);
	NxNgc::EngineGlobals.use_480p = true;
	hwReInit( NULL );

	// Must display screen saying that progressive mode has been set.
	for ( int lp = 0; lp < 240; lp++ )
	{
		// Render the text.
		NsDisplay::begin();
		NsRender::begin();

		NsCamera cam;
		cam.orthographic( 0, 0, 640, 448 );

		// Draw the screen.
		NsPrim::begin();

		cam.begin();

		GX::SetZMode( GX_FALSE, GX_ALWAYS, GX_TRUE );

		NxNgc::set_blend_mode( NxNgc::vBLEND_MODE_BLEND );

		if ( lp > 60 ) Script::RunScript( "ngc_progressive" );

		NsDisplay::setBackgroundColor( messageColor );

		cam.end();

		NsPrim::end();

		NsRender::end();
		NsDisplay::end( true );
	}
	return true;
}

bool ScriptNgc_Set480I(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	// Already in 480I to display menu. Just set the SRAM bit.
	OSSetProgressiveMode(0);

	return true;
}

bool ScriptNgc_Set60Hz(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	OSSetEuRgb60Mode(1);
	NxNgc::EngineGlobals.use_60hz = true;
	hwReInit( NULL );

	hwGXInit();
    VIConfigure(rmode);

	Config::gFPS = 60;

	// Must display screen saying that 60hz mode has been set.
	for ( int lp = 0; lp < 240; lp++ )
	{
		// Render the text.
		NsDisplay::begin();
		NsRender::begin();

		NsCamera cam;
		cam.orthographic( 0, 0, 640, 448 );

		// Draw the screen.
		NsPrim::begin();

		cam.begin();

		GX::SetZMode( GX_FALSE, GX_ALWAYS, GX_TRUE );

		NxNgc::set_blend_mode( NxNgc::vBLEND_MODE_BLEND );

		if ( lp > 60 ) Script::RunScript( "ngc_60Hz" );

		NsDisplay::setBackgroundColor( messageColor );

		cam.end();

		NsPrim::end();

		NsRender::end();
		NsDisplay::end( true );
	}
	return true;
}

bool ScriptNgc_Set50Hz(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	OSSetEuRgb60Mode(0);
	NxNgc::EngineGlobals.use_60hz = false;

	Config::gFPS = 50;

	// Must display screen saying that 50hz mode has been set.
	for ( int lp = 0; lp < 240; lp++ )
	{
		// Render the text.
		NsDisplay::begin();
		NsRender::begin();

		NsCamera cam;
		cam.orthographic( 0, 0, 640, 448 );

		// Draw the screen.
		NsPrim::begin();

		cam.begin();

		GX::SetZMode( GX_FALSE, GX_ALWAYS, GX_TRUE );

		NxNgc::set_blend_mode( NxNgc::vBLEND_MODE_BLEND );

		if ( lp > 60 ) Script::RunScript( "ngc_50Hz" );

		NsDisplay::setBackgroundColor( messageColor );

		cam.end();

		NsPrim::end();

		NsRender::end();
		NsDisplay::end( true );
	}
	return true;
}

bool ScriptNgc_SetWide(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	NxNgc::EngineGlobals.use_widescreen = true;

	return true;
}

bool ScriptNgc_SetStandard(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	NxNgc::EngineGlobals.use_widescreen = false;

	return true;
}

bool ScriptNgc_ReduceColors(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	bool truth;
	if ( pParams->GetInteger( NONAME, (int*)&truth ) )
	{
		NxNgc::EngineGlobals.reduceColors = truth;
	}

	return true;
}

}




