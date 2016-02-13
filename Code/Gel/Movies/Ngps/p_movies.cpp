#include <eekernel.h>
#include <stdio.h>
#include <string.h>
#include <libsdr.h>
#include <sdrcmd.h>
#include <libpkt.h>
#include <libdma.h>
#include <libmpeg.h>
#include <libpad.h>
#include <sifrpc.h>
#include <sifdev.h>
#include <libgraph.h>
#include <libdev.h>
#include <core/defines.h>
#include <core/macros.h>
#include <core/singleton.h>

#include "gfx/gfxman.h"

#include "gfx/ngps/nx/nx_init.h"
#include "gfx/ngps/nx/pcrtc.h"
#include "gfx/ngps/nx/resource.h"

#include "gel/music/ngps/p_music.h"

#include "gel/movies/movies.h"
#include "gel/movies/ngps/p_movies.h"
#include "gel/movies/ngps/defs.h"
#include "gel/movies/ngps/videodec.h"
#include "gel/movies/ngps/disp.h"
#include "gel/movies/ngps/readbuf.h"
#include "gel/movies/ngps/vobuf.h"
#include "gel/movies/ngps/audiodec.h"
#include "gel/movies/ngps/strfile.h"
#include "sys/timer.h"

#include <sys/file/filesys.h>
#include <sys/timer.h>
#include <sys/config/config.h>
#include <gel/music/music.h>

// K: To switch this off, make it 0 rather than undefining it, cos it won't compile otherwise
#define FORCE_MOVIES_FROM_CD_PLEASE 0

int videoDecTh;
int defaultTh;
int frd;
u_int controller_val;
int isWithAudio = 1;

bool hitStart = false;
uint64 hitStartFrame;

sceGsDBuff gDB;

namespace Flx
{



static bool initAll( const char *bsfilename );
static void termAll();
static void defMain(void *dummy);
static u_int movie( const char *name );
static int readMpeg(VideoDec *vd, ReadBuf *rb, StrFile *file);
static int isAudioOK();

void dispMain(void);
void videoDecMain( void *dummy);
int videoCallback(sceMpeg *mp, sceMpegCbDataStr *str, void *data);
int pcmCallback(sceMpeg *mp, sceMpegCbDataStr *str, void *data);
int vblankHandler(int);
int handler_endimage(int);
void loadModule(char *moduleName);

#define NUM_VIDEO_BUFFERS	3
#define VRAM_TOTAL_SIZE		( 1024 * 4096 ) // 4 megs of video ram
#define MAX_PIXEL_DATA_SIZE	( 32767 * 16 )

#define ZBUF_ADDR(w, h)		( (((w) + 63) / 64) * (((h) + 31) / 32) * 2)

SMovieMem *gpMovieMem = NULL;

int GetThreadPriority( )
{
	struct ThreadParam info;
	// Fill info with zeros to be sure the following call is doing
	// something to info
	memset(&info,0,sizeof(info));
	// return value not defined in 2.0 PDF EE lib ref
	ReferThreadStatus( GetThreadId(), &info );
	return ( info.currentPriority );
}

// ////////////////////////////////////////////////////////////////
//
// Play Movie
//

#define RESTORE_VRAM 1
#define PLAY_MOVIE_PLEASE 1
#define SAVE_SCRATCHPAD 0
#define SCRATCHPAD_SIZE	8192

void PMovies_PlayMovie( const char *pName )
{
	// Re-enabled for CD even though we don't have a Sony fix.
//	// Garrett: Disabled CD movies until I know the sceCdSt() calls are working again in 2.7.2
//	if (Config::CD() || FORCE_MOVIES_FROM_CD_PLEASE)
//	{
//		return;
//	}
	
	int origThreadPriority;
//	int origArenaSize;
	int width=0;
	int height=0;
//	int origDepth;
//	char *pOrigVram;
//	char *pNonAllignedOrigVram;

	//Dbg_Message("*************** Movie: In render frame %d", Tmr::GetRenderFrame());

	// Check to see if we need to skip movies
	if (hitStart && (hitStartFrame == Tmr::GetRenderFrame()))
	{
		return;
	}
	
	// stop music and streams (we're gonna be using the IOP memory)
	Pcm::StopMusic( );
	Pcm::StopStreams( );

	// Suspend Nx engine and its interrupts
	NxPs2::SuspendEngine();
		

	if (Config::PAL())
	{
		width = 640;
		height = 512;
	} else {
		width = 640;
		height = 480;
	}

	// clear vram onscreen:
	if (Config::PAL())
	{
		clearGsMem(0x00, 0x00, 0x00, 640, ( height * width * 2 ) / 640 );
	}
	else
	{
		clearGsMem(0x00, 0x00, 0x00, width, height * 2);
	}

	// get current thread priority:
	origThreadPriority = GetThreadPriority( );
    ChangeThreadPriority(GetThreadId(), MOVIE_THREAD_PRIORITY );
	

#if PLAY_MOVIE_PLEASE
    isWithAudio = 1; // withAudio;
    
	Dbg_MsgAssert( pName, ( "No movie name specified" ) );

	Dbg_MsgAssert( !gpMovieMem, ( "Movie memory already allocated!" ) );
									  
	// Allocate memory on high heap
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());

	void *pNonAllignedMovieMem = (void *) Mem::Malloc( sizeof( SMovieMem ) + 64 );
	
	if ( !pNonAllignedMovieMem )
	{
		Dbg_MsgAssert( 0, ( "Ran out of memory... can't play movie %s.", pName ) );
		return;
	}
	gpMovieMem = ( SMovieMem * )( ( ( int )pNonAllignedMovieMem ) + ( 64 - ( ( ( int ) pNonAllignedMovieMem ) & 63 ) ) );

	Dbg_MsgAssert( !( ( ( int )& MOVIE_MEM_PTR voBufData & 63 ) ||
		( ( int )& MOVIE_MEM_PTR voBufTag & 63 ) ||
        ( ( int )& MOVIE_MEM_PTR viBufTag & 63 ) ||
		( ( int )& MOVIE_MEM_PTR mpegWork & 63 ) ||
		( ( int )& MOVIE_MEM_PTR defStack & 63 ) ||
		( ( int )& MOVIE_MEM_PTR audioBuff & 63 ) ||
		( ( int )& MOVIE_MEM_PTR viBufData & 63 ) ||
		( ( int )& MOVIE_MEM_PTR videoDecStack & 63 ) ||
		( ( int )& MOVIE_MEM_PTR timeStamp & 63 ) ||
//		( ( int )& MOVIE_MEM_PTR controller_dma_buf & 63 ) ||
		( ( int )& MOVIE_MEM_PTR readBuf & 63 ) ||
        ( ( int )& MOVIE_MEM_PTR infile & 63 ) ||
		( ( int )& MOVIE_MEM_PTR videoDec & 63 ) ||
		( ( int )& MOVIE_MEM_PTR audioDec & 63 ) ||
		( ( int )& MOVIE_MEM_PTR voBuf & 63 ) ), ( "Bad allignment in SMovieMem structure." ) );
/*
	Dbg_Message( "Allignment: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", ( int )& MOVIE_MEM_PTR voBufData & 63, ( int )& MOVIE_MEM_PTR voBufTag & 63,
	 ( int )& MOVIE_MEM_PTR viBufTag & 63, ( int )& MOVIE_MEM_PTR mpegWork & 63, ( int )& MOVIE_MEM_PTR defStack & 63, ( int )& MOVIE_MEM_PTR audioBuff & 63, ( int )& MOVIE_MEM_PTR viBufData & 63,
	 ( int )& MOVIE_MEM_PTR videoDecStack & 63, ( int )& MOVIE_MEM_PTR timeStamp & 63, ( int )& MOVIE_MEM_PTR controller_dma_buf & 63, ( int )& MOVIE_MEM_PTR readBuf & 63,
	 ( int )& MOVIE_MEM_PTR infile & 63, ( int )& MOVIE_MEM_PTR videoDec & 63, ( int )& MOVIE_MEM_PTR audioDec & 63, ( int )& MOVIE_MEM_PTR voBuf & 63 );
*/
	Dbg_Message( "Size of SMovieMem struct: %d k", sizeof( SMovieMem ) / 1024 );


#if SAVE_SCRATCHPAD
	// remember scratchpad:
	unsigned char *pScratchpad = ( unsigned char * )Mem::Malloc( SCRATCHPAD_SIZE );
	Dbg_MsgAssert( pScratchpad, ( "Ran out of memory allocating scratchpad." ) );
	int i;
	for ( i = 0; i < SCRATCHPAD_SIZE; i++ )
	{
		pScratchpad[ i ] = ( ( unsigned char * ) 0x70000000 )[ i ];
	}
#else
	// Make sure scratchpad is available
	bool got_scratch = NxPs2::CSystemResources::sRequestResource(NxPs2::CSystemResources::vSCRATCHPAD_MEMORY);
	Dbg_Assert(got_scratch);
#endif // SAVE_SCRATCHPAD

	// Pop top-down context
	Mem::Manager::sHandle().PopContext();

	sceGsResetPath();
    sceDmaReset(1);

	NxPs2::SetupPCRTC(1, SCE_GS_FRAME);

	// Old way of initializing.  We can't use sceGsResetGraph() anymore.  It interferes with SetupPCRTC().
	// Also, clearGsMem() did nothing before because it used gDB, which you can see was not initialized until
	// after the calls.
#if 0  
		//clearGsMem(0x00, 0x00, 0x00, 640, 480);
		//VSync( );
		
		//sceGsResetGraph(0, SCE_GS_INTERLACE, SCE_GS_NTSC, SCE_GS_FRAME);

		//  Initialize GS memory
		//clearGsMem(0x00, 0x00, 0x00, 640, 480);
#endif
	
	sceGsSetDefDBuff(&gDB, SCE_GS_PSMCT32, width, (height/2),
			 SCE_GS_ZNOUSE, 0, SCE_GS_CLEAR);

	int zbuf_addr = ZBUF_ADDR(width, (height/2));

	// Garrett: Since sceGsResetGraph() saves some register settings in internal library variables (including FRAME),
	// we have to update some settings in gDB with the correct values.
	gDB.disp[0].smode2.FFMD = SCE_GS_FRAME;
	gDB.disp[1].smode2.FFMD = SCE_GS_FRAME;
	gDB.disp[0].dispfb.FBP	= 0x0;
	gDB.disp[1].dispfb.FBP	= zbuf_addr >> 1;
	gDB.disp[0].display.DH	= height-1;
	gDB.disp[1].display.DH	= height-1;
	gDB.draw0.frame1.FBP	= zbuf_addr >> 1;
	gDB.draw1.frame1.FBP	= 0x0;
	gDB.draw0.zbuf1.ZBP		= zbuf_addr;
	gDB.draw1.zbuf1.ZBP		= zbuf_addr;
				 
    FlushCache(0);

	// Wait for all the music and streams to really stop, since we are stealing their IOP memory
	uint32 target_vblank = Tmr::GetVblanks() + (5 * Config::FPS());		// 5 seconds
	while (Tmr::GetVblanks() < target_vblank)
	{
		Pcm::PCMAudio_Update();

		if (Pcm::PCMAudio_GetMusicStatus() != Pcm::PCM_STATUS_FREE)
			continue;
		
		bool done = true;
		for (int streamNum = 0; streamNum < NUM_STREAMS; streamNum++)
		{
			if (Pcm::PCMAudio_GetStreamStatus( streamNum ) != Pcm::PCM_STATUS_FREE)
				done = false;
		}

		if (done)
		{
			break;
		}
	}

	movie( pName );
    //while ( ( movie( pName ) ) != MOVIE_ABORTED)
    //	;

	// Clear double buffers to black:
	clearGsMem(0x00, 0x00, 0x00, width, height);
	
	
	// wait for any sound transfer to complete and shit...
	if ( sceSdRemote( 1, rSdVoiceTransStatus, AUTODMA_CH, SD_TRANS_STATUS_WAIT ) == -1 )
	{
		Dbg_MsgAssert( 0,( "Complete SPU2 Transfer Failed\n"));
	}

	Mem::Free( pNonAllignedMovieMem );
	gpMovieMem = NULL;
	
#if SAVE_SCRATCHPAD
	// restore scratchpad:
	for ( i = 0; i < SCRATCHPAD_SIZE; i++ )
	{
		( ( unsigned char * ) 0x70000000 )[ i ] = pScratchpad[ i ];
	}
	Mem::Free( pScratchpad );
#else
	if (got_scratch)
	{
		NxPs2::CSystemResources::sFreeResource(NxPs2::CSystemResources::vSCRATCHPAD_MEMORY);
	}
#endif // SAVE_SCRATCHPAD

//	VSync( );
//	while ( Tmr::GetVblanks() & 1 );

#endif // playmovieplease


	ChangeThreadPriority(GetThreadId(), origThreadPriority );

	Tmr::VSync( );

	return;
}

// ////////////////////////////////////////////////////////////////
//
// Decode MPEG bitstream
//
// ret:
//   1: ok
//   0: error
//  -1: abort
static u_int movie( const char *name )
{
	CHECK_MOVIE_MEM;

    static int count = 0;

    printf("========================== decode MPEG2 ============= %d ===========\n", count++);
	
    if (initAll(name ))
	{
		printf( "done w/ initall\n" );	
		readMpeg(& MOVIE_MEM_PTR videoDec, & MOVIE_MEM_PTR readBuf, & MOVIE_MEM_PTR infile);
	}

	termAll();

    return controller_val;
}

// ////////////////////////////////////////////////////////////////
//
// Read MPEG data
//
// return value
//     1: normal end
//     -1: aborted
static int readMpeg(VideoDec *vd, ReadBuf *rb, StrFile *file)
{
	
    u_int ctrlmask =  SCE_PADRdown | SCE_PADstart;
    u_char *put_ptr;
    u_char *get_ptr;
    int putsize;
    int getsize;
    int readrest = file->size;
    int writerest = file->size;
    int count;
    int proceed;
    int isStarted = 0;
    u_int button_old = 0;
    u_int pushed = 0;
    u_char cdata[32];
    int isPaused = 0;

    // writerest > 4: to skip the last 4 bytes
    while (isPaused
    	|| (writerest > 4 && videoDecGetState(vd) != VD_STATE_END)) {

    	// /////////////////////////////////////////////////
	//
	// Get controller information
	//
   	if (scePadRead(0, 0, cdata) > 0) {
	    controller_val = 0xffff ^ ((cdata[2] << 8) | cdata[3]);
	} else {
	    controller_val = 0;
	}
	pushed = (button_old ^ controller_val)
			& controller_val & ctrlmask;
	button_old = controller_val;

	CHECK_MOVIE_MEM;

	if (pushed && vd->mpeg.frameCount > 10)
	{

/*	    if (pushed & SCE_PADRleft) {
	    	if (isPaused) {
		    startDisplay(1);
		    if (isWithAudio) {
			audioDecResume(& MOVIE_MEM_PTR audioDec);
		    }
		} else {
		    endDisplay();
		    if (isWithAudio) {
			audioDecPause(& MOVIE_MEM_PTR audioDec);
		    }
		}
		isPaused ^= 1;
	    } else if (!isPaused) {
*/
		// /////////////////////////////////////////////////
		//
		// Abort decoding
		//
		videoDecAbort(& MOVIE_MEM_PTR videoDec);
	//    }

		// Record frame number if we hit START so we skip all movies
		if (pushed & SCE_PADstart)
		{
			hitStartFrame = Tmr::GetRenderFrame();
			hitStart = true;
		} else {
			hitStart = false;
		}
	}
   	
	// /////////////////////////////////////////////////
	//
	// Read data to the read buffer
	//
        putsize = readBufBeginPut(rb, &put_ptr);
	if (readrest > 0 && putsize >= READ_UNIT_SIZE) {
	    count = strFileRead(file, put_ptr, READ_UNIT_SIZE);
	    readBufEndPut(rb, count);
	    readrest -= count;
	}

	switchThread();

    	// /////////////////////////////////////////////////
	//
	// De-multiplex and put data on video/audio input buffer
	//
	getsize = readBufBeginGet(rb, &get_ptr);
	if (getsize > 0) {

	    proceed = sceMpegDemuxPssRing(&vd->mpeg,
	    		get_ptr, getsize, rb->data, rb->size);

	    readBufEndGet(rb, proceed);
	    writerest -= proceed;

	}

    	// /////////////////////////////////////////////////
	//
	// Send audio data to IOP
	//
	proceedAudio();

    	// /////////////////////////////////////////////////
	//
	// Wait until video and audio output buffer become full
	//
	CHECK_MOVIE_MEM;
	
	if (!isStarted && voBufIsFull(& MOVIE_MEM_PTR voBuf) && isAudioOK()) {

	    startDisplay(1);		// start video
	    if (isWithAudio) {
		audioDecStart(& MOVIE_MEM_PTR audioDec);	// start audio
	    }
	    isStarted = 1;
	}
    }

    // try to flush buffers inside decoder
    while (!videoDecFlush(vd)) {
	switchThread();
    }

    // wait till buffers are flushed
    while (!videoDecIsFlushed(vd)
    	&& videoDecGetState(vd) != VD_STATE_END) {

	switchThread();
    }

    endDisplay();
    if (isWithAudio)
	{
		audioDecReset(& MOVIE_MEM_PTR audioDec);
    }

    return 1;
}

// /////////////////////////////////////////////////
//
// Switch to another thread
//
void switchThread()
{
    RotateThreadReadyQueue( MOVIE_THREAD_PRIORITY );
}

// /////////////////////////////////////////////////
//
// Check audio
//
static int isAudioOK()
{
	
    CHECK_MOVIE_MEM;
	return (isWithAudio)? audioDecIsPreset(& MOVIE_MEM_PTR audioDec): 1;
}

// ////////////////////////////////////////////////////////////////
//
// Initialize all modules
//
static bool initAll( const char *bsfilename )
{
	
    struct ThreadParam th_param;

    *D_CTRL = (*D_CTRL | 0x003);
    *D_STAT = 0x4; // clear D_STAT.CIS2

    // /////////////////////////////
    // 
    //  Create read buffer
    // 
    CHECK_MOVIE_MEM;
	
	readBufCreate(& MOVIE_MEM_PTR readBuf);

    // /////////////////////////////
    // 
    //  Initialize video decoder
    // 
    sceMpegInit();
    videoDecCreate(& MOVIE_MEM_PTR videoDec,
    	 MOVIE_MEM_PTR mpegWork, MPEG_WORK_SIZE,
    	 MOVIE_MEM_PTR viBufData,  MOVIE_MEM_PTR viBufTag, VIBUF_SIZE,  MOVIE_MEM_PTR timeStamp, VIBUF_TS_SIZE);

    // /////////////////////////////
    // 
    //  Initialize audio decoder
    // 
#define NEED_TO_INIT_SD_AND_SHIT 0
#if NEED_TO_INIT_SD_AND_SHIT    
	sceSdRemoteInit();
    sceSdRemote(1, rSdInit, SD_INIT_COLD);  // i think having this in would fuck up soundfx module...
#endif
    audioDecCreate(& MOVIE_MEM_PTR audioDec,  MOVIE_MEM_PTR audioBuff, AUDIO_BUFF_SIZE,
			IOP_BUFF_SIZE, Pcm::PCMAudio_GetIopMemory( ) );

    ///////////////////////////////
    // 
    //  Choose stream to be played
    // 
    videoDecSetStream(& MOVIE_MEM_PTR videoDec,
	    sceMpegStrM2V, 0, (sceMpegCallback)videoCallback, & MOVIE_MEM_PTR readBuf);
    if (isWithAudio) {
	videoDecSetStream(& MOVIE_MEM_PTR videoDec,
	    sceMpegStrPCM, 0, (sceMpegCallback)pcmCallback, & MOVIE_MEM_PTR readBuf);
    }

    // /////////////////////////////
    // 
    //  Initialize video output buffer
    // 
    voBufCreate(& MOVIE_MEM_PTR voBuf, ( VoData * )UncAddr( MOVIE_MEM_PTR voBufData),  MOVIE_MEM_PTR voBufTag, N_VOBUF);

    // /////////////////////////////
    // 
    //  Create 'default' thread
    // 
	Dbg_Message( "Starting default thread" );
    th_param.entry = defMain;
    th_param.stack =  MOVIE_MEM_PTR defStack;
    th_param.stackSize = MOVIE_DEF_STACK_SIZE;
    th_param.initPriority = MOVIE_THREAD_PRIORITY;
    th_param.gpReg = &_gp;
    th_param.option = 0;
    defaultTh = CreateThread(&th_param);
    StartThread(defaultTh, NULL);
    
	// /////////////////////////////
    // 
    //  Create docode thread
    // 
	Dbg_Message( "Starting video decoder thread" );
    th_param.entry = videoDecMain;
    th_param.stack =  MOVIE_MEM_PTR videoDecStack;
    th_param.stackSize = MOVIE_STACK_SIZE;
    th_param.initPriority = MOVIE_THREAD_PRIORITY;
    th_param.gpReg = &_gp;
    th_param.option = 0;
    videoDecTh = CreateThread(&th_param);
    StartThread(videoDecTh, NULL );

    // /////////////////////////////
    // 
    //  Initialize controller
    // 

/*	static int isFirst = 1;
	if (isFirst)
	{
	    scePadInit(0);
	    scePadPortOpen(0, 0,  MOVIE_MEM_PTR controller_dma_buf);
	    isFirst = 0;
	}*/

	char filename[ 256 ];
	if (Config::CD() || FORCE_MOVIES_FROM_CD_PLEASE)
	{
		sprintf( filename, "cdrom0:\\%s%s.PSS;1",Config::GetDirectory(), bsfilename );
		unsigned int schraw;
		for ( schraw = 7; schraw < strlen( filename ); schraw++ ) // start after "cdrom0:\"
		{
			if ( filename[ schraw ] >= 'a' && filename[ schraw ] <= 'z' )
			{
				filename[ schraw ] += 'A' - 'a';
			}
			else if ( filename[ schraw ] == '/' )
			{
				filename[ schraw ] = '\\';
			}
		}
	}	
	else
	{
		sprintf( filename, "host0:\\%s.pss", bsfilename );
	}	
	
    // /////////////////////////////
    // 
    //  Open bitstream file
    // 
	File::StopStreaming( );
	if ( Pcm::UsingCD( ) )
	{
		Dbg_MsgAssert( 0,( "Can't load IRX modules when CD is busy." ));
		return false;
	}
    
	bool	loaded = false;
	for (int i = 0;i <10; i++)
	{
		printf ("Attempt %d\n",i);
		if (!strFileOpen(& MOVIE_MEM_PTR infile, filename,
			Pcm::PCMAudio_GetIopMemory( ) + IOP_BUFF_SIZE + ( 2048 * STRFILE_NUM_STREAMING_SECTORS + 64 ) ))			 
		{
			printf ("Can't Open file %s\n", filename);
		}
		else
		{
			loaded = true;
			break;
		}
	}
	
    // /////////////////////////////
    // 
    //  Set Interrupt handlers
    // 
	MOVIE_MEM_PTR videoDec.hid_vblank = AddIntcHandler(INTC_VBLANK_S, vblankHandler, 0);
	EnableIntc(INTC_VBLANK_S);

	MOVIE_MEM_PTR videoDec.hid_endimage = AddDmacHandler(DMAC_GIF, handler_endimage, 0);
	EnableDmac(DMAC_GIF);
	
	return loaded;
}

// ////////////////////////////////////////////////////////////////
//
// Terminate all modules
//
static void termAll()
{
	
	
	CHECK_MOVIE_MEM;

    TerminateThread(videoDecTh);
    DeleteThread(videoDecTh);

    TerminateThread(defaultTh);
    DeleteThread(defaultTh);

    DisableDmac(DMAC_GIF);
    RemoveDmacHandler(DMAC_GIF,  MOVIE_MEM_PTR videoDec.hid_endimage);
	//EnableDmac(DMAC_GIF);

    DisableIntc(INTC_VBLANK_S);
    RemoveIntcHandler(INTC_VBLANK_S,  MOVIE_MEM_PTR videoDec.hid_vblank);
	EnableIntc(INTC_VBLANK_S);
    
	readBufDelete(& MOVIE_MEM_PTR readBuf);
    voBufDelete(& MOVIE_MEM_PTR voBuf);

    videoDecDelete(& MOVIE_MEM_PTR videoDec);
    audioDecDelete(& MOVIE_MEM_PTR audioDec);

    strFileClose(& MOVIE_MEM_PTR infile);

	// Re-init nx engine
	NxPs2::ResetEngine();

	// Re-init quick filesystem (since the CD stream buffer has changed)
	if (Config::CD())
	{
		File::ResetQuickFileSystem();
	}
}

// ////////////////////////////////////////////////////////////////
//
// Main function of default thread
//
static void defMain(void * dummy)
{
    while (1) {
	switchThread();
    }
}

// ////////////////////////////////////////////////////////////////
//
// Print error message
//
void ErrMessage(char *message)
{
    printf("[ Error ] %s\n", message);
}

// ////////////////////////////////////////////////////////////////
//
//  Send audio data to IOP
//
void proceedAudio()
{
	
    
	CHECK_MOVIE_MEM;
	
	audioDecSendToIOP(& MOVIE_MEM_PTR audioDec);
}



} // namespace Flx

