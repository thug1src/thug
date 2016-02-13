#include <stdio.h>
#include <stdlib.h>
#include <libgraph.h>
#include <libdma.h>
#include <devvu0.h>
#include <devvu1.h>
#include <core/defines.h>
#include <core/debug.h>
#include <libsn.h>
#include <sys/file/pip.h>
#include <sys/config/config.h>
#include "nx_init.h"
#include "dma.h"
#include "vif.h"
#include "vu1.h"
#include "vu1code.h"
#include "vu0code.h"
#include "gif.h"
#include "gs.h"
#include "dmacalls.h"
#include "pcrtc.h"
#include "render.h"
#include "chars.h"
#include "scene.h"
#include "group.h"
#include "interrupts.h"
#include "switches.h"
#include "instance.h"
#include "geomnode.h"
#include "texture.h"
#include "resource.h"
#include "fx.h"

namespace NxPs2
{

extern	void	WaitForRendering();

// Interrupts
static	void	StartInterrupts();
static	void	StopInterrupts();

Mth::Matrix GmyMat[4];

// Interrupt Handler IDs
#if USE_INTERRUPTS
	
#if GIF_INTERRUPT
static int sGifHandlerId = -1;
#endif

#if GS_INTERRUPT
static int sGsHandlerId = -1;
#endif

#if VIF1_INTERRUPT
static int sVif1HandlerId = -1;
#endif

#endif

//////////////////////////////////////////////////////////////////
// void InitialiseEngine(void)
//
// Sets up the PS2 engine, down to the hardware level
//  - Resets the hardware, using sce library calls, setting video mode
//  - sets up the PCRTC registers (display buffer conficuration, and read circuit anti-aliasing)
//  - Initilizes memory for DMA buffers
//  - Builds the standard DMA subroutines (clear VRAM, page flip, etc)
//  - adds GS, VIF1 and GIF interrupts
//  - creates an empty "scene" and an empty group as the default things to render


void InitialiseEngine(void)
{
    // Mick:  Make it safe to call more than once
    // Needed for a last minute loading screen hack TT#12480
	static bool done = false;
	if (done)
	{
		return;
	}
	done = true;
	
	// Initialize devices
	sceDmaReset(1);
	sceGsResetPath();
    sceDevVu1PutDBit(1);	// sceGsResetPath will clear the D-bit, so need to switch it on
    //sceDevVu0PutTBit(1);

	// setup PCRTC registers - switch off display until vram has been cleared
	SetupPCRTC(0, SCE_GS_FIELD);

	// Clear resoruces
	CSystemResources::sInitResources();

	// allocate dma list memory (prebuilt & run-time) and subroutine table
	dma::pPrebuiltBuffer = (uint8 *)Mem::Calloc(PREBUILT_DMA_BUFFER_SIZE/16,16);
	Dbg_MsgAssert(dma::pPrebuiltBuffer, ("couldn't malloc memory for prebuilt dma buffer"));
	
	if (Config::GotExtraMemory())
	{
		// Allocate a dummy buffer so that the current heap's usage is the same
		// as for when there is not extra memory.
		dma::pDummyBuffer = (uint8 *)Mem::Calloc(NON_DEBUG_DMA_BUFFER_SIZE/16,16);
		
		// But for the actual runtime buffer allocate a massive buffer off the debug heap.
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());
		dma::pRuntimeBuffer = (uint8 *)Mem::Calloc(DEBUG_DMA_BUFFER_SIZE/16,16);
		Mem::Manager::sHandle().PopContext();
		
		Dbg_MsgAssert(dma::pRuntimeBuffer, ("couldn't malloc memory for runtime dma buffer"));
		// set runtime dma list pointers (they are double buffered, using half the buffer each)
		dma::pList[0] = dma::pRuntimeBuffer;
		dma::pList[1] = dma::pRuntimeBuffer + DEBUG_DMA_BUFFER_SIZE/2;
	}
	else
	{
		dma::pRuntimeBuffer = (uint8 *)Mem::Calloc(NON_DEBUG_DMA_BUFFER_SIZE/16,16);
		Dbg_MsgAssert(dma::pRuntimeBuffer, ("couldn't malloc memory for runtime dma buffer"));
		// set runtime dma list pointers (they are double buffered, using half the buffer each)
		dma::pList[0] = dma::pRuntimeBuffer;
		dma::pList[1] = dma::pRuntimeBuffer + NON_DEBUG_DMA_BUFFER_SIZE/2;
	}
	dma::size = NON_DEBUG_DMA_BUFFER_SIZE/2;  // normal sized (half the buffer)
	
		
	dma::Gosubs = (uint64 *)Mem::Calloc(NUM_SUBROUTINES, sizeof(uint64));
	Dbg_MsgAssert(dma::Gosubs, ("couldn't malloc memory for dma gosub table"));
	//Subroutines = (uint8 **)Mem::Calloc(256, sizeof(uint8 *));
	//Dbg_MsgAssert(Subroutines, ("couldn't malloc memory for dma gosub table"));

	// initialise dma pointer for use by BuildDmaSubroutines
	dma::pLoc = dma::pPrebuiltBuffer;

	// prebuild dma subroutines (into dma::pPrebuiltBuffer)
	BuildDmaSubroutines();

	// Enable interrupts
	StartInterrupts();

	// wait for v-sync
	sceGsSyncV(0);

	// clear vram
	FlushCache(WRITEBACK_DCACHE);
	*D1_QWC  = 0;						// must zero QWC because the first action will be to use current MADR & QWC
	*D1_TADR = dma::Gosubs[CLEAR_VRAM] >> 32;
	*D1_CHCR = 0x145;					// start transfer, tte=1, chain mode, from memory
	sceGsSyncPath(0, 0);

	// setup PCRTC registers - vram is cleared so switch on display
	SetupPCRTC(1, SCE_GS_FIELD);

	// upload vu0 code
	*D0_QWC  = ((uint8 *)MPGEnd0-(uint8 *)MPGStart0+15)/16;
	*D0_MADR = (uint)MPGStart0;
	*D0_CHCR = 0x100;					// start transfer, normal mode
	sceGsSyncPath(0, 0);

	// empty scene list
	sScene::pHead = NULL;

	// create the shadow group
	sGroup::pShadow = (sGroup *)Mem::Malloc(sizeof(sGroup));
	memset(sGroup::pShadow,0,sizeof(sGroup));
	Dbg_MsgAssert(sGroup::pShadow, ("couldn't allocate shadow group\n"));
	sGroup::pShadow->Priority = 1500.001f;
	sGroup::pShadow->pMeshes = NULL;
	sGroup::pShadow->NumMeshes = 0;
	#if STENCIL_SHADOW
	sGroup::pShadow->VramStart = 0x3740;
	#else
	sGroup::pShadow->VramStart = 0x3E00;
	#endif
	sGroup::pShadow->VramEnd   = 0x4000;
	sGroup::pShadow->flags = 0;
	sGroup::pShadow->profile_color = 0xFFFFFF;		// bright white = shadow group

	// create the fog group
	sGroup::pFog = (sGroup *)Mem::Malloc(sizeof(sGroup));
	memset(sGroup::pFog,0,sizeof(sGroup));
	Dbg_MsgAssert(sGroup::pFog, ("couldn't allocate fog group\n"));
	sGroup::pFog->Priority = 9999.0f;
	sGroup::pFog->pMeshes = NULL;
	sGroup::pFog->NumMeshes = 0;
	sGroup::pFog->VramStart = 0x3000;
	sGroup::pFog->VramEnd   = 0x4000;
	sGroup::pFog->flags = 0;
	sGroup::pFog->pScene = NULL;
	sGroup::pFog->profile_color = 0x303030;			// dark grey = fog group
	
	// create the particle group
	sGroup::pParticles = (sGroup *)Mem::Malloc(sizeof(sGroup));
	memset(sGroup::pParticles,0,sizeof(sGroup));
	Dbg_MsgAssert(sGroup::pParticles, ("couldn't allocate particle group\n"));
	sGroup::pParticles->Priority = 10000.0f;
	sGroup::pParticles->pMeshes = NULL;
	sGroup::pParticles->NumMeshes = 0;
	sGroup::pParticles->VramStart = 0x3000;
	sGroup::pParticles->VramEnd   = 0x4000;
	sGroup::pParticles->flags = 0;
	sGroup::pParticles->pScene = NULL;
	sGroup::pParticles->profile_color = 0x0;		// black = particle group

	// create epilogue group
	sGroup::pEpilogue = (sGroup *)Mem::Malloc(sizeof(sGroup));
	memset(sGroup::pEpilogue,0,sizeof(sGroup));
	Dbg_MsgAssert(sGroup::pEpilogue, ("couldn't allocate epilogue group\n"));
	sGroup::pEpilogue->Priority = 1000000;
	sGroup::pEpilogue->pMeshes = NULL;
	sGroup::pEpilogue->NumMeshes = 0;
	sGroup::pEpilogue->VramStart = 0x0000;	// pretend it uses all of vram
	sGroup::pEpilogue->VramEnd   = 0x4000;	// so there's no buffer address worries
	sGroup::pEpilogue->profile_color = 0x800000;		// blue = epilogue group

	// link predefined groups together
	sGroup::pShadow->pNext = sGroup::pFog;
	sGroup::pFog->pNext = sGroup::pParticles;
	sGroup::pParticles->pNext = sGroup::pEpilogue;
	sGroup::pEpilogue->pNext = NULL;
	sGroup::pShadow->pPrev = NULL;
	sGroup::pFog->pPrev = sGroup::pShadow;
	sGroup::pParticles->pPrev = sGroup::pFog;
	sGroup::pEpilogue->pPrev = sGroup::pParticles;
	sGroup::pHead = sGroup::pShadow;
	sGroup::pTail = sGroup::pEpilogue;

	// initialise vram group buffering scheme
	sGroup::VramBufferBase = 0x2BC0;

	// initialise font vram base pointer
	FontVramStart = FontVramBase = 0x2BC0;	// a cheat

	// initialise empty font list
	pFontList = NULL;



	// activate the world!
	CGeomNode::sWorld.SetActive(true);

	// set up fog palette
//	Fx::SetupFogPalette(0x40806060, 10.0f);
}

void 	AllocateExtraDMA(bool allocate)
{
	static void * s_extra = NULL;
	static void * old_pList1;
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());

	if (allocate)
	{
		Dbg_MsgAssert(!s_extra,("Tried to allocate extra DMA when already allocated"));
		old_pList1 = dma::pList[1];
		s_extra = (uint8 *)Mem::Calloc(NON_DEBUG_DMA_BUFFER_SIZE/16,16);	  // same size as the entire double buffer
		dma::pList[1] = (uint8 *)s_extra;
		dma::size = NON_DEBUG_DMA_BUFFER_SIZE;  // double sized
	}
	else
	{
		if (s_extra)
		{
			dma::pList[1] = (uint8 *)old_pList1;
			Mem::Free(s_extra);
			s_extra = NULL;
			dma::size = NON_DEBUG_DMA_BUFFER_SIZE/2;  // normal sized
		}
	}
	
	dma::pLoc = 0 ;  	// reset pLoc as it might not be valid.  This will catch this function being called in the wrong place
	
	Mem::Manager::sHandle().PopContext();	
}


//////////////////////////////////////////////////////////////////
// void ResetEngine(void)
//
// Sets up the PS2 engine, down to the hardware level
//  - Resets the hardware, using sce library calls, setting video mode
//  - sets up the PCRTC registers (display buffer conficuration, and read circuit anti-aliasing)
//  - clears VRAM, which also sets the TEXA register
//  - resets GS, VIF1 and GIF interrupts

void ResetEngine(void)
{
	// initialize devices
	sceDmaReset(1);
	sceGsResetPath();
    sceDevVu1PutDBit(1);	// sceGsResetPath will clear the D-bit, so need to switch it on
    //sceDevVu0PutTBit(1);

	// Don't call sceGsResetGraph() anymore.  It interferes with SetupPCRTC().
	//// re-initialize the GS
	//if (Config::NTSC())
	//{
	//	sceGsResetGraph(0, SCE_GS_INTERLACE, SCE_GS_NTSC, SCE_GS_FIELD);
	//} else {
	//	sceGsResetGraph(0, SCE_GS_INTERLACE, SCE_GS_PAL, SCE_GS_FIELD);
	//}

	// setup PCRTC registers (don't need this call anymore; the last call is all that is necessary)
	//SetupPCRTC(0, SCE_GS_FIELD);

	// restart interrupts
	StartInterrupts();

	// wait for v-sync
	sceGsSyncV(0);

	// clear vram (Garrett: This is the ONLY place that TEXA is ever set!  Caused many weird bugs if not called.)
	FlushCache(WRITEBACK_DCACHE);
	*D1_QWC  = 0;						// must zero QWC because the first action will be to use current MADR & QWC
	*D1_TADR = dma::Gosubs[CLEAR_VRAM] >> 32;
	*D1_CHCR = 0x145;					// start transfer, tte=1, chain mode, from memory
	sceGsSyncPath(0, 0);

	// setup PCRTC registers - vram is cleared so switch on display
	SetupPCRTC(1, SCE_GS_FIELD);

}

//////////////////////////////////////////////////////////////////
// void SuspendEngine(void)
//
// Suspends the PS2 engine by waiting for any rendering to finish
// and then shuts down the interrupts.

void SuspendEngine(void)
{
	// Let the engine finish what it was doing first
	WaitForRendering();

	// stop interrupts
	StopInterrupts();
}


//////////////////////////////////////////////////////////////////
// void StartInterrupts(void)
//
// Starts all the interrupts for the PS2 engine.

static void StartInterrupts(void)
{
	#if USE_INTERRUPTS
	
	#if GIF_INTERRUPT
    // install interrupt handler for texture upload completion
	if ((sGifHandlerId = AddDmacHandler(DMAC_GIF, GifHandler, 0)) == -1)
	{
		printf("Couldn't register dmac handler\n");
		exit(1);
	}
	*D_STAT = 1 << DMAC_GIF;
	EnableDmac(DMAC_GIF);
	#endif

	#if GS_INTERRUPT
    // install interrupt handler for render completion
	if ((sGsHandlerId = AddIntcHandler(INTC_GS, GsHandler, 0)) == -1)
	{
		printf("Couldn't register GS handler\n");
		exit(1);
	}
	*GS_IMR = PackIMR(0,1,1,1);		// SIGNAL, FINISH, HSync, VSync
	EnableIntc(INTC_GS);
	#endif
	
	#if VIF1_INTERRUPT
    // install interrupt handler for render completion
	if ((sVif1HandlerId = AddIntcHandler(INTC_VIF1, Vif1Handler, 0)) == -1)
	{
		printf("Couldn't register vif1 handler\n");
		exit(1);
	}
	*VIF1_ERR = 0;
	EnableIntc(INTC_VIF1);
	#endif

	// set intermittent mode transfer
	*GIF_MODE = 0x0;
	#endif

}



//////////////////////////////////////////////////////////////////
// void StopInterrupts(void)
//
// Stops all the interrupts for the PS2 engine.

static void StopInterrupts(void)
{
	#if USE_INTERRUPTS
	
	#if GIF_INTERRUPT
    // remove interrupt handler for texture upload completion
	DisableDmac(DMAC_GIF);
	if (sGifHandlerId >= 0)
	{
		RemoveDmacHandler(DMAC_GIF, sGifHandlerId);
		sGifHandlerId = -1;
	}
	#endif

	#if GS_INTERRUPT
    // remove interrupt handler for render completion
	DisableIntc(INTC_GS);
	if (sGsHandlerId >= 0)
	{
		RemoveIntcHandler(INTC_GS, sGsHandlerId);
		sGsHandlerId = -1;
	}
	#endif
	
	#if VIF1_INTERRUPT
    // remove interrupt handler for render completion
	DisableIntc(INTC_VIF1);
	if (sVif1HandlerId >= 0)
	{
		RemoveIntcHandler(INTC_VIF1, sVif1HandlerId);
		sVif1HandlerId = -1;
	}
	#endif

	#endif
}

} // namespace NxPs2

