/* Vag streaming into SPU2 -- from sony sample -- matt may 2001 */

#include <kernel.h>
#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <string.h>
#include <sif.h>
#include <sifcmd.h>
#include <sifrpc.h>
#include <libsd.h>
#include <sdrcmd.h>
#include <thread.h>
#include "pcm.h"
#include "pcmiop.h"

// ================================================================

#define STREAM_PITCH( x ) ( ( ( x ) * 22050 ) / 48000 )

#define CHECK_TRANSFER_INTEGRITY	0
#define TEST_PLAY_TIMING			0

#define SHOW_STREAM_INFO	0
#define SHOW_STATE	0
#define SHOW_SMALL_STREAM_STATE	0
#define SHOW_SMALL_MUSIC_STATE	0
#define SHOW_ACTION 0
#define SHOW_SMALL_ACTION 0
#define DBUGPRINTF	0
#define SHOW_TIMING	0
#define SHOW_STOP	0
#define SHOW_SMALL_COMMANDS	0
#define PRINT_EXCESSIVE_WAIT 0

#if SHOW_STREAM_INFO
#define ShowStreamInfo( x ) _ShowStreamInfo( x )
#else
#define ShowStreamInfo( x ) 
#endif

#if SHOW_SMALL_COMMANDS
#define ShowSmallCommand printf
#else
#define ShowSmallCommand( x )
#endif

#if SHOW_STOP
#define ShowStop printf
#else
#define ShowStop( x )
#endif

#if SHOW_TIMING
#define ShowTime	printf
#else
#define ShowTime DoNothing
void DoNothing( char* text, ... ) { }
#endif

#if SHOW_ACTION
#define ShowAction printf
#else
#define ShowAction( x )
#endif

#if SHOW_SMALL_STREAM_STATE
#define SmallShowStreamState printf( "  %d", whichStream ), printf
#else
#define SmallShowStreamState( x )
#endif

#if SHOW_SMALL_MUSIC_STATE
#define SmallShowMusicState printf
#else
#define SmallShowMusicState( x )
#endif

#if SHOW_SMALL_ACTION
#define SmallShowAction printf
#else
#define SmallShowAction( x )
#endif

#if SHOW_STATE
#define ShowState printf
#else
#define ShowState( x )
#endif

#if DBUGPRINTF
#define Dbug_Printf( x ) printf( x )
#else
#define Dbug_Printf( x )
#endif

// the program running on the EE side will check to see if any of the buffers need to be filled:
volatile unsigned int gEECommand = 0;

#define TRANS_DMA_CH_MUSIC		1
#define TRANS_DMA_CH_STREAM		0

#define _1st 0
#define _2nd 1

#define TRUE  1
#define FALSE 0

unsigned int VCount=0;
unsigned int gThid = 0;
unsigned int gSem = 0;
unsigned int gpIopBuf;		// IOP SMEM

struct StreamInfo gStreamInfo[ NUM_STREAMS ];
struct StreamInfo gMusicInfo;
volatile int gUsingStreamDMA = 0;
volatile int gSPUUsingStreamDMA = 0;

// Done on the EE now
//volatile unsigned int gStreamVolPercent = 100;

// Timing test code
#if TEST_PLAY_TIMING
static 			int		test_timing_request_id = 2;
static			int		test_timing_send_timing_result = FALSE;
static			int		test_timing_hit_point = FALSE;
#endif // TEST_PLAY_TIMING

u_int GetNAX(int core,int ch);
int OutsideMusicSPULoadBuffer();
int OutsideStreamSPULoadBuffer(int whichStream);
int DownloadMusic(int whichChannel);
int DownloadStream(int whichStream);
void IncMusicBuffer();
void IncStreamBuffer(int whichStream);
void StartMusic();
void StartStream(int whichStream);
int RefreshStreams( int status );
void SendStatus();

static int _AdpcmDmaIntMusic( int, void* );
static int _AdpcmDmaIntStream( int, void* );

#define _L 0
#define _R 1
#define _Lch(x) ((x >> 16) & 0xffff)
#define _Rch(x) (x & 0xffff)

// Timer structure.
typedef struct TimerCtx {
    int thread_id;
    int timer_id;
    int count;
} TimerCtx;

TimerCtx gTimer;

#define ONE_HSCAN 1000*1000/60	// Approx 60 frames per second

/**********************************************************************************
IRQ Interrupt - called, say, once per frame
***********************************************************************************/
int TimerFunc( void *common)
{
    TimerCtx *tc = (TimerCtx *) common;
		VCount++;
		iSignalSema(gSem);	// Signal StreamADPCM to be called
    return (tc->count);
}

/**********************************************************************************
SetTimer - IRQ Interrupt
***********************************************************************************/
int SetTimer( TimerCtx* timer )
{
    struct SysClock clock;
    int timer_id;

    USec2SysClock (ONE_HSCAN, & clock);
    timer->count = clock.low;	/* within 32-bit */

    // Use sysclock timer
    if ((timer_id = AllocHardTimer (TC_SYSCLOCK, 32, 1)) <= 0) {
	printf ("Can NOT allocate hard timer ...\n");
	return (-1);
    }
    timer->timer_id = timer_id;

    if (SetTimerHandler (timer_id, timer->count,
			 (void *)TimerFunc, (void *) timer) != KE_OK) {
	printf ("Can NOT set timeup timer handler ...\n");
	return (-1);
    }

    if (SetupHardTimer (timer_id, TC_SYSCLOCK, TM_NO_GATE, 1) != KE_OK) {
	printf ("Can NOT setup hard timer ...\n");
	return (-1);
    }
    return 0;

}

/**********************************************************************************
startTimer - IRQ Interrupt
***********************************************************************************/
int StartTimer( TimerCtx* timer )
{
    if (StartHardTimer (timer->timer_id) != KE_OK)
    {
		printf ("Can NOT start hard timer ...\n");
		return (-1);
    }
    return 0;
}

// EzADPCM_SDINIT
void AdpcmSdInit( void )
{
    sceSdInit (0);

    //	Disk media: DVD
    //	Output format: PCM
    //	Copy guard: normal (one generation recordable / default)
    sceSdSetCoreAttr (SD_C_SPDIF_MODE, (SD_SPDIF_MEDIA_DVD |
					SD_SPDIF_OUT_PCM   |
					SD_SPDIF_COPY_NORMAL));
    return;
}

void AdpcmSetUpVoice( int core, int vnum, int pSpuBuf )
{
    sceSdSetParam ( core | ( vnum << 1 ) | SD_VP_VOLL, 0 );
    sceSdSetParam ( core | ( vnum << 1 ) | SD_VP_VOLR, 0 );
    sceSdSetParam ( core | ( vnum << 1 ) | SD_VP_PITCH, core == MUSIC_CORE ? DEFAULT_PITCH : STREAM_PITCH( DEFAULT_PITCH ) );
//    sceSdSetParam ( core | ( vnum << 1 ) | SD_VP_ADSR1, 0x000f );
//    sceSdSetParam ( core | ( vnum << 1 ) | SD_VP_ADSR2, 0x1fc0 );
    sceSdSetAddr  ( core | ( vnum << 1 ) | SD_VA_SSA,  pSpuBuf );
    return;
}

// EzADPCM_INIT
int AdpcmInit( int pIopBuf )
{
	int i;
	if ( gEECommand & PCMSTATUS_INITIALIZED )
	{
		return ( 0 );
	}

    if ( gSem == 0 )
	{
		struct SemaParam sem;
		sem.initCount = 0;
		sem.maxCount = 1;
		sem.attr = AT_THFIFO;
		gSem = CreateSema (&sem);
    }
    if (gThid == 0)
	{
		struct ThreadParam param;
		param.attr         = TH_C;
		param.entry        = RefreshStreams;
		param.initPriority = BASE_priority-3;
		param.stackSize    = 0x800; // was 800
		param.option = 0;
		gThid = CreateThread (&param);
		printf( "EzADPCM: create thread ID= %d\n", gThid );
		StartThread (gThid, (u_long) NULL);
    }

	sceSdSetTransIntrHandler( TRANS_DMA_CH_MUSIC, ( sceSdTransIntrHandler ) _AdpcmDmaIntMusic, ( void * ) &gSem );
    sceSdSetTransIntrHandler( TRANS_DMA_CH_STREAM, ( sceSdTransIntrHandler ) _AdpcmDmaIntStream, ( void * ) &gSem );
	SetTimer(&gTimer);		// Setup IRQ interrupt
	StartTimer(&gTimer);		// Start IRQ interrupt (calls TimerFunc)
	
	AdpcmSetUpVoice( MUSIC_CORE, MUSIC_L_VOICE, MUSIC_L_SPU_BUF_LOC );
	AdpcmSetUpVoice( MUSIC_CORE, MUSIC_R_VOICE, MUSIC_R_SPU_BUF_LOC );
	for ( i = 0; i < NUM_STREAMS; i++ )
	{
		AdpcmSetUpVoice( STREAM_CORE, STREAM_VOICE( i ), STREAM_SPU_BUF_LOC( i ) );
	}

	memset( gStreamInfo, 0, sizeof( struct StreamInfo ) * NUM_STREAMS );
	memset( &gMusicInfo, 0, sizeof( struct StreamInfo ) );
	
	gpIopBuf = pIopBuf;
	gEECommand |= PCMSTATUS_INITIALIZED;
	//printf( "PCM Irx iop buf %d\n", gpIopBuf );
	//Dbug_Printf( "PCM irx is initialized\n" );
    return gThid;
}

// EzADPCM_QUIT
void AdpcmQuit( void )
{
    DeleteThread (gThid);
    gThid = 0;
#if 0
    DeleteSema (gSem);
    gSem = 0;
#endif
    return;
}

// EzADPCM_PRELOADMUSIC
// EzADPCM_PLAYMUSIC
int AdpcmPlayMusic( int size, int preload_only )
{
    extern void AdpcmSetMusicVolumeDirect( unsigned int );

    if ( gMusicInfo.status != PCM_STATUS_IDLE )
	{
		ShowAction( "NOTE NOTE NOTE NOTE Can't play music -- music isn't in Idle state.\n" );
		return -1;
    }

    AdpcmSetMusicVolumeDirect( gMusicInfo.volume );

	if (preload_only)
	{
		gMusicInfo.m_preloadMode = TRUE;
	}
	else
	{
		gMusicInfo.m_preloadMode = FALSE;
	}
				
	gMusicInfo.status = PCM_STATUS_PRELOAD;
	gMusicInfo.loadState = MUSIC_LOAD_STATE_IDLE;
	gMusicInfo.size = size;
	gMusicInfo.m_iopBufLoaded[0] = TRUE;
	gMusicInfo.m_iopBufLoaded[1] = FALSE;
	gEECommand &= ~PCMSTATUS_NEED_MUSIC_BUFFER_0;
	if (size > MUSIC_HALF_IOP_BUFFER_SIZE)
	{
		gEECommand |= PCMSTATUS_NEED_MUSIC_BUFFER_1;
	}
	gEECommand |= PCMSTATUS_MUSIC_PLAYING;
	ShowAction( "Starting music\n" );
	SmallShowAction( "-mb" );
    return 0;
}

void AdpcmSetStreamVolume( unsigned int vol, int whichStream );

// EzADPCM_PRELOADMUSIC
// EzADPCM_PLAYSTREAM
int AdpcmPlayStream( int size, int whichStream, unsigned int vol, unsigned int pitch, int preload_only )
{
    if ( gStreamInfo[ whichStream ].status != PCM_STATUS_IDLE )
	{
		printf( "trying to play stream when stream already playing!\n" );
		return -1;
    }

	if (preload_only)
	{
		gStreamInfo[ whichStream ].m_preloadMode = TRUE;
	}
	else
	{
		gStreamInfo[ whichStream ].m_preloadMode = FALSE;
		AdpcmSetStreamVolume( vol, whichStream );
		gStreamInfo[ whichStream ].pitch = STREAM_PITCH( pitch );
	}

	// have to make sure it's okay to use the stream DMA first...
	gStreamInfo[ whichStream ].status = PCM_STATUS_PRELOAD;
	gStreamInfo[ whichStream ].size = size;
	gStreamInfo[ whichStream ].m_iopBufLoaded[0] = TRUE;
	gStreamInfo[ whichStream ].m_iopBufLoaded[1] = FALSE;
	gEECommand &= ~( PCMSTATUS_NEED_STREAM0_BUFFER_0 << whichStream );
	if (size > STREAM_HALF_IOP_BUFFER_SIZE)
	{
		gEECommand |= PCMSTATUS_NEED_STREAM0_BUFFER_1 << whichStream;
	}
	gStreamInfo[ whichStream ].loadState = STREAM_LOAD_STATE_IDLE;
	gEECommand |= ( PCMSTATUS_STREAM_PLAYING( whichStream ) );
	ShowAction( "Starting stream\n" );
	SmallShowAction( "-sb" );
	return 0;
}

// EzADPCM_PLAYPRELOADEDMUSIC
int AdpcmPlayPreloadedMusic()
{
	// Tell update loop it is OK to play
	gMusicInfo.m_preloadMode = FALSE;
	SignalSema(gSem);	// Signal RefreshStreams() to be called

	ShowAction( "Playing preloaded music\n" );
	SmallShowAction( "-ppm" );
	return 0;
}

// EzADPCM_PLAYPRELOADEDSTREAM
int AdpcmPlayPreloadedStream(int whichStream, unsigned int vol, unsigned int pitch)
{
	AdpcmSetStreamVolume( vol, whichStream );
	gStreamInfo[ whichStream ].pitch = STREAM_PITCH( pitch );

	// Tell update loop it is OK to play
	gStreamInfo[ whichStream ].m_preloadMode = FALSE;
	SignalSema(gSem);	// Signal RefreshStreams() to be called

	ShowAction( "Playing preloaded stream\n" );
	SmallShowAction( "-pps" );
	return 0;
}

/* internal */
void _AdpcmSetMusicVoiceMute( void )
{
    sceSdSetParam ( MUSIC_CORE | ( MUSIC_L_VOICE << 1) | SD_VP_VOLL, 0);
    sceSdSetParam ( MUSIC_CORE | ( MUSIC_L_VOICE << 1) | SD_VP_VOLR, 0);
    sceSdSetParam ( MUSIC_CORE | ( MUSIC_R_VOICE << 1) | SD_VP_VOLL, 0);
    sceSdSetParam ( MUSIC_CORE | ( MUSIC_R_VOICE << 1) | SD_VP_VOLR, 0);
    return;
}

/* internal */
void _AdpcmSetStreamVoiceMute( int whichVoice )
{
    sceSdSetParam( STREAM_CORE | ( STREAM_VOICE( whichVoice ) << 1) | SD_VP_VOLL, 0);
    sceSdSetParam( STREAM_CORE | ( STREAM_VOICE( whichVoice ) << 1) | SD_VP_VOLR, 0);
    return;
}

void StopMusic( void )
{
	sceSdSetCoreAttr( MUSIC_CORE | SD_C_IRQ_ENABLE, 0 );
	_AdpcmSetMusicVoiceMute( );
	sceSdSetSwitch( MUSIC_CORE | SD_S_KOFF, ( 1 << MUSIC_R_VOICE ) | ( 1 << MUSIC_L_VOICE ) );
	ShowAction( "Stopping music\n" );
    SmallShowAction( "-!sm!" );
	gMusicInfo.stop = 0;
	gMusicInfo.loadState = MUSIC_LOAD_STATE_IDLE;
	//gMusicInfo.m_iopBufLoaded[0] = FALSE;		// This should not be necessary
	//gMusicInfo.m_iopBufLoaded[1] = FALSE;
	gEECommand &= ~( PCMSTATUS_MUSIC_PLAYING | PCMSTATUS_MUSIC_READY | PCMSTATUS_NEED_MUSIC_BUFFER_0 | PCMSTATUS_NEED_MUSIC_BUFFER_1 );
	gMusicInfo.status = PCM_STATUS_IDLE;
}

void StopStream( int whichStream )
{
	sceSdSetCoreAttr( STREAM_CORE | SD_C_IRQ_ENABLE, 0 );
	_AdpcmSetStreamVoiceMute( whichStream );
	sceSdSetSwitch( STREAM_CORE | SD_S_KOFF, ( 1 << STREAM_VOICE( whichStream ) ) );
	ShowAction( "Stopping stream\n" );
    SmallShowAction( "-!ss!" );
	gStreamInfo[ whichStream ].stop = 0;
	gStreamInfo[ whichStream ].loadState = STREAM_LOAD_STATE_IDLE;
	//gStreamInfo[ whichStream ].m_iopBufLoaded[0] = FALSE;		// This should not be necessary
	//gStreamInfo[ whichStream ].m_iopBufLoaded[1] = FALSE;
	gEECommand &= ~( ( PCMSTATUS_STREAM0_PLAYING | PCMSTATUS_STREAM0_READY | PCMSTATUS_NEED_STREAM0_BUFFER_0 | PCMSTATUS_NEED_STREAM0_BUFFER_1 ) << whichStream );
	gStreamInfo[ whichStream ].status = PCM_STATUS_IDLE;
}

#define NUM_WAIT_CLICKS 10000

// EzADPCM_STOPMUSIC
int AdpcmStopMusic( void )
{
	int i;
	int j = 0;

	if ( gMusicInfo.status != PCM_STATUS_IDLE )
	{
		ShowStop( "stopmusic" );
		gMusicInfo.stop++;
		SignalSema(gSem);	// Signal RefreshStreams() to be called
		// Why the hell are we waiting????  This is VERY lame!
		while ( gMusicInfo.status != PCM_STATUS_IDLE )
		{
			for ( i = 0; i < NUM_WAIT_CLICKS; i++ )
				;
			if ( j++ > NUM_WAIT_CLICKS )
			{
				j = 0;
				printf( "...Waiting for music stop...\n" );
			}
		}
		ShowStop( "\n" );
		return ( 1 );
    }
	return ( 0 );
}

// EzADPCM_STOPSTREAM
int AdpcmStopStream( int whichStream )
{
	int i;
	int j = 0;

	if ( gStreamInfo[ whichStream ].status != PCM_STATUS_IDLE )
	{
		ShowStop( "Stop stream" );
		gStreamInfo[ whichStream ].stop++;
		SignalSema(gSem);	// Signal RefreshStreams() to be called
		// Why the hell are we waiting????  This is VERY lame!
		while ( gStreamInfo[ whichStream ].status != PCM_STATUS_IDLE )
		{
			for ( i = 0; i < NUM_WAIT_CLICKS; i++ )
				;
			if ( j++ > NUM_WAIT_CLICKS )
			{
				printf( "...Waiting for stream %d stop...\n", whichStream );
				j = 0;
			}
		}
		ShowStop( "\n" );
		SmallShowAction( "-ss" );
		ShowAction( "Stopped stream\n" );
		return ( 1 );
    }
	return ( 0 );
}

// EzADPCM_SETMUSICVOL
void AdpcmSetMusicVolume( unsigned int vol )
{
    gMusicInfo.volume = vol;
    gMusicInfo.volumeSet = 1;
    return;
}

// EzADPCM_SETMUSICVOLDIRECT
void AdpcmSetMusicVolumeDirect( unsigned int vol )
{
    gMusicInfo.volume = vol;
    sceSdSetParam ( MUSIC_CORE | ( MUSIC_L_VOICE << 1 ) | SD_VP_VOLL, vol );
    sceSdSetParam ( MUSIC_CORE | ( MUSIC_L_VOICE << 1 ) | SD_VP_VOLR, 0 );
    sceSdSetParam ( MUSIC_CORE | ( MUSIC_R_VOICE << 1 ) | SD_VP_VOLL, 0 );
    sceSdSetParam ( MUSIC_CORE | ( MUSIC_R_VOICE << 1 ) | SD_VP_VOLR, vol );
    return;
}

// Done on the EE now
//void AdpcmSetStreamGlobVol( unsigned int volPercent )
//{
//	gStreamVolPercent = volPercent;
//}

#define PERCENT_MULT ( ( 1 << 16 ) / 100 )
#define PERCENT( x, y ) ( ( ( x ) * ( y ) * PERCENT_MULT ) >> 16 )

// EzADPCM_SETSTREAMVOL
void AdpcmSetStreamVolume( unsigned int vol, int whichStream )
{
    gStreamInfo[ whichStream ].volume = vol;
    gStreamInfo[ whichStream ].volumeSet = 1;
    return;
}

// EzADPCM_SETSTREAMVOLDIRECT
void AdpcmSetStreamVolumeDirect( unsigned int vol, int whichStream )
{
    //sceSdSetParam ( STREAM_CORE | ( STREAM_VOICE( whichStream ) << 1 ) | SD_VP_VOLL, PERCENT( gStreamVolPercent, _Lch( vol ) ) );
    //sceSdSetParam ( STREAM_CORE | ( STREAM_VOICE( whichStream ) << 1 ) | SD_VP_VOLR, PERCENT( gStreamVolPercent, _Rch( vol ) ) );
    sceSdSetParam ( STREAM_CORE | ( STREAM_VOICE( whichStream ) << 1 ) | SD_VP_VOLL, _Lch( vol ) );
    sceSdSetParam ( STREAM_CORE | ( STREAM_VOICE( whichStream ) << 1 ) | SD_VP_VOLR, _Rch( vol ) );
    return;
}

// Shouldn't need these unless debugging or something --
// Instead just get gEECommand each frame and act accordingly.

// EzADPCM_GETMUSICSTATUS
unsigned int AdpcmGetMusicStatus( void )
{
	return gMusicInfo.status;
}

// EzADPCM_GETSTREAMSTATUS
/*unsigned int AdpcmGetStreamStatus( int whichStream )
{
    return gStreamInfo[ whichStream ].status;
} */

static volatile unsigned int sEELastCommand = 0xFFFFFFFF;

// EzADPCM_GETSTATUS
unsigned int AdpcmGetStatus( void )
{
	unsigned int temp;
	temp = gEECommand;
	gEECommand &= ~( PCMSTATUS_NEED_MUSIC_BUFFER_0 | PCMSTATUS_NEED_MUSIC_BUFFER_1 | 
					PCMSTATUS_NEED_STREAM0_BUFFER_0 | PCMSTATUS_NEED_STREAM0_BUFFER_1 |
					PCMSTATUS_NEED_STREAM1_BUFFER_0 | PCMSTATUS_NEED_STREAM1_BUFFER_1 |
					PCMSTATUS_NEED_STREAM2_BUFFER_0 | PCMSTATUS_NEED_STREAM2_BUFFER_1 );
	sEELastCommand = gEECommand;
#if SHOW_SMALL_COMMANDS
	if ( temp & ( PCMSTATUS_NEED_MUSIC_BUFFER_0 | PCMSTATUS_NEED_MUSIC_BUFFER_1 | 
					PCMSTATUS_NEED_STREAM0_BUFFER_0 | PCMSTATUS_NEED_STREAM0_BUFFER_1 |
					PCMSTATUS_NEED_STREAM1_BUFFER_0 | PCMSTATUS_NEED_STREAM1_BUFFER_1 |
					PCMSTATUS_NEED_STREAM2_BUFFER_0 | PCMSTATUS_NEED_STREAM2_BUFFER_1 ) )
	{
		ShowSmallCommand( " <_L_>" );
	}
#endif
	return temp;
}

int AdpcmHasStatusChanged( void )
{
	return sEELastCommand != gEECommand;
}

// ================================================================

#define _ADPCM_MARK_START 0x04
#define _ADPCM_MARK_LOOP  0x02
#define _ADPCM_MARK_END   0x01

#define _AdpcmSetMarkFINAL(a,s) { \
  *( ( unsigned int * ) ( ( a ) + ( s ) - 16 ) ) = 0; \
  *( ( unsigned int * ) ( ( a ) + ( s ) - 12 ) ) = 0; \
  *( ( unsigned int * ) ( ( a ) + ( s ) - 8 ) ) = 0; \
  *( ( unsigned int * ) ( ( a ) + ( s ) - 4 ) ) = 0; \
  *((unsigned char *)((a)+(s)-0x0f)) = (_ADPCM_MARK_LOOP | _ADPCM_MARK_START | _ADPCM_MARK_END); \
  FlushDcache (); }


// Start of loop - note that a (address is either the start or mid address of the IOP buffer. s is half the buffer size)
#define _AdpcmSetMarkSTART(a,s) { \
  *((unsigned char *)((a)+1)) = \
	(_ADPCM_MARK_LOOP | _ADPCM_MARK_START); \
  *((unsigned char *)((a)+0x10+1)) = \
	_ADPCM_MARK_LOOP; \
  *((unsigned char *)((a)+(s)-0x0f)) = \
	_ADPCM_MARK_LOOP; \
	FlushDcache (); }

// End of loop - note that a (address is either the start or mid address of the IOP buffer. s is half the buffer size)
#define _AdpcmSetMarkEND(a,s) { \
  *((unsigned char *)((a)+1)) = \
	 _ADPCM_MARK_LOOP; \
  *((unsigned char *)((a)+0x10+1)) = \
	_ADPCM_MARK_LOOP; \
  *((unsigned char *)((a)+(s)-0x0f)) = \
	(_ADPCM_MARK_LOOP | _ADPCM_MARK_END); \
	FlushDcache (); }

// Preload Start marker - note that a (address is the start address of the IOP buffer. s is buffer size)

#define _AdpcmSetMarkSTARTpre(a,s) { \
  *((unsigned char *)((a)+1)) = \
	(_ADPCM_MARK_LOOP | _ADPCM_MARK_START); \
  *((unsigned char *)((a)+0x10+1)) = \
	_ADPCM_MARK_LOOP; \
	FlushDcache (); }

// Preload End marker - note that a (address is the start address of the IOP buffer. s is buffer size)
#define _AdpcmSetMarkENDpre(a,s) { \
  *((unsigned char *)((a)+(s)-0xf)) = \
	(_ADPCM_MARK_LOOP | _ADPCM_MARK_END); \
	FlushDcache (); }

/* internal */
static int _AdpcmDmaIntMusic(int ch, void *common)	// DMA Interrupt -- when transfering data to SPU2 from IOP
{
	//Kprintf("Got music IRQ for channel %d\n", ch);
	if ( gMusicInfo.status != PCM_STATUS_IDLE )
	{
		gMusicInfo.m_spuTransDone = TRUE;
		return 1;
	}
	return 0;
}

/* internal */
static int _AdpcmDmaIntStream(int ch, void *common)	// DMA Interrupt -- when transfering data to SPU2 from IOP
{
	if ( gUsingStreamDMA )
	{
		int whichStream = gUsingStreamDMA - 1;
		//Kprintf("Received Stream DMA 0 callback\n");
		if ( gStreamInfo[ whichStream ].status != PCM_STATUS_IDLE )
		{
			gStreamInfo[ whichStream ].m_spuTransDone = TRUE;
			gUsingStreamDMA = 0;
			return 1;
		}
	}
	else if (gSPUUsingStreamDMA)
	{
		gSPUUsingStreamDMA = 0;
		//Kprintf("Received SPU DMA 0 callback\n");
	}
	else
	{
		//Kprintf("Received outside DMA 0 callback\n");
	}
	return 0;
}

void UpdateMusic( void )
{
	struct StreamInfo *pInfo;
	pInfo = &gMusicInfo;
	switch ( pInfo->status )
	{
		case PCM_STATUS_IDLE:
			break;

		case PCM_STATUS_PRELOAD:
			switch ( pInfo->loadState )
			{
				case ( MUSIC_LOAD_STATE_IDLE ):
				{
					ShowState( "music preload l\n" );
					SmallShowMusicState( "-MPL" );
					pInfo->m_spuBufSide = _1st;
					pInfo->m_iopBufIndex = _1st;
					pInfo->m_iopOffset = 0;
					pInfo->remaining = pInfo->size;

					// preload left:
					if (DownloadMusic(_L))
					{
						pInfo->loadState++;
					}

					break;
				}
				case ( MUSIC_LOAD_STATE_PRELOADING_L ):
				{
					// Check that transfer is done
					if (!pInfo->m_spuTransDone)
					{
						break;
					}

					if ( pInfo->stop )		// Don't stop until we know the first CD stream and DMA to SPU have finished
					{
						StopMusic( );
						break;
					}

					ShowState( "music preload r\n" );
					SmallShowMusicState( "-MPR" );

					// preload right:
					DownloadMusic(_R);

					pInfo->loadState++;
					break;
				}	
				case ( MUSIC_LOAD_STATE_PRELOADING_R ):
				{
					// Check that transfer is done
					if (!pInfo->m_spuTransDone)
					{
						break;
					}

					if ( pInfo->stop )		// Don't stop until we the DMA to SPU has finished
					{
						StopMusic( );
						break;
					}

					// This will let the EE know that we are ready to play
					gEECommand |= PCMSTATUS_MUSIC_READY;

					// Now check if we are in a preload_only mode
					if (pInfo->m_preloadMode)
					{
						// We need to wait until we get a message from the EE to start
						break;
					}

					ShowState( "Music starting -- waiting for IRQ\n" );
					SmallShowMusicState( " -MSIRQ" );

					pInfo->status = PCM_STATUS_RUNNING;
					IncMusicBuffer();

					// Play music
					StartMusic();
#if TEST_PLAY_TIMING
					if (test_timing_send_timing_result)
					{
						test_timing_hit_point = TRUE;
					}
#endif // TEST_PLAY_TIMING

					pInfo->loadState++;
					break;
				}
			}
			break;

		case PCM_STATUS_RUNNING:
			pInfo->m_spuCurAddr = GetNAX( MUSIC_CORE, MUSIC_L_VOICE );

			switch ( pInfo->loadState )
			{
				case ( MUSIC_LOAD_STATE_WAITING_FOR_REFRESH ):
				{
					if ( pInfo->stop )
					{
						//pInfo->status = PCM_STATUS_READY_TO_STOP;
						StopMusic( );
						return;
					}

					// Wait for SPU buffer cross
					if (!OutsideMusicSPULoadBuffer())
					{
						break;
					}

					ShowState( "Music Running -- Wait For Last Load\n" );
					SmallShowStreamState( "-MRWLL" );

					// Change state and go immediately into it
					pInfo->loadState++;
				}

				case ( MUSIC_LOAD_STATE_WAITING_FOR_LAST_LOAD ):
				{
					if ( pInfo->stop )
					{
						StopMusic( );
						break;
					}

					// After we got this far, we've crossed the SPU buffer and we
					// know we need to download.  But if the new IOP buffer isn't
					// loaded and download before we cross SPU buffers AGAIN, then 
					// we know the stream has failed.  Time to panic!
					if (!OutsideMusicSPULoadBuffer())
					{
						if (!(gEECommand & PCMSTATUS_PANIC))
						{
							// Print only once
							Kprintf("ERROR: Music did not load data in time.  In SPU buffer %d and pos %x\n", pInfo->m_spuBufSide, pInfo->m_spuCurAddr - MUSIC_L_SPU_BUF_LOC);
						}
						gEECommand |= PCMSTATUS_PANIC;
						break;
					}

					ShowState( "Music Running -- Loading L\n" );
					SmallShowMusicState( "-MRLL" );

					// load in left side:
					if (DownloadMusic(_L))
					{
						pInfo->loadState++;
					}

					break;
				}

				case ( MUSIC_LOAD_STATE_LOADING_L ):
				{
					// Check that transfer is done
					if (!pInfo->m_spuTransDone)
					{
						break;
					}

					//Kprintf( "Music Running -- Loading R at offset %x and address %x\n", pInfo->m_iopOffset, gpIopBuf + MUSIC_R_IOP_OFFSET + pInfo->m_iopOffset );
					ShowState( "Music Running -- Loading R\n" );
					SmallShowMusicState( "-MRLR" );
					
					// load in right side:
					DownloadMusic(_R);

					pInfo->loadState++;
					break;
				}
				case ( MUSIC_LOAD_STATE_LOADING_R ):
				{
					// Check that transfer is done
					if (!pInfo->m_spuTransDone)
					{
						break;
					}

					ShowState( "Music Running -- Waiting for IRQ\n" );
					SmallShowMusicState( "-MRIRQ" );

					IncMusicBuffer();

					pInfo->loadState = MUSIC_LOAD_STATE_WAITING_FOR_REFRESH;
					break;
				}
				default:
					printf( "Unknown music loading state %d\n", pInfo->loadState );
					break;
			}
			break;

		case PCM_STATUS_TERMINATE: 
			pInfo->m_spuCurAddr = GetNAX( MUSIC_CORE, MUSIC_L_VOICE );

			if ( !pInfo->stop && ( ( pInfo->m_spuCurAddr > pInfo->m_spuEndAddr ) || ( pInfo->m_spuCurAddr <= pInfo->m_spuEndAddr - 16 ) ) )
			{
				break;
			}

			ShowState( "Music at end\n" );

		case PCM_STATUS_READY_TO_STOP:
			ShowState( "Music terminate\n" );
			SmallShowMusicState( "-MT!!!" );
			StopMusic( );
			break;

		default:
			printf( "unknown music status in pcm irx!!!\n" );
			break;
	}
}

void UpdateStream( int whichStream )
{
	struct StreamInfo *pInfo;
	pInfo = &gStreamInfo[ whichStream ];

	switch ( pInfo->status )
	{
		case PCM_STATUS_IDLE:
			break;

		case PCM_STATUS_PRELOAD:
			switch ( pInfo->loadState )
			{
				case ( STREAM_LOAD_STATE_IDLE ):
				{
					ShowState( "stream preload l\n" );
					SmallShowStreamState( "-SPL" );

					pInfo->m_spuBufSide = _1st;
					pInfo->m_iopBufIndex = _1st;
					pInfo->m_iopOffset = 0;
					pInfo->remaining = pInfo->size;

					if (DownloadStream(whichStream))
					{
						pInfo->loadState++;
					}

					break;
				}
				case ( STREAM_LOAD_STATE_PRELOADING ):
				{
					ShowState( "Stream starting -- waiting for IRQ\n" );
					SmallShowStreamState( "-SSIRQ" );

					// Check that transfer is done
					if (!pInfo->m_spuTransDone)
					{
						break;
					}

					if ( pInfo->stop )		// Don't stop until we know the first CD stream and DMA to SPU have finished
					{
						StopStream( whichStream );
						break;
					}

					// This will let the EE know that we are ready to play
					gEECommand |= PCMSTATUS_STREAM_READY(whichStream);

					// Now check if we are in a preload_only mode
					if (pInfo->m_preloadMode)
					{
						// We need to wait until we get a message from the EE to start
						break;
					}

					pInfo->status = PCM_STATUS_RUNNING;
					IncStreamBuffer(whichStream);

					// Play the stream
					StartStream(whichStream);
					//Kprintf("Started playing stream %d; paused = %d; volume = %x\n", whichStream, gStreamInfo[ whichStream ].paused, pInfo->volume);

					pInfo->loadState++;
					break;
				}
			}
			break;

		case PCM_STATUS_RUNNING:
			pInfo->m_spuCurAddr = GetNAX( STREAM_CORE, STREAM_VOICE( whichStream ) );

			switch ( pInfo->loadState )
			{
				case ( STREAM_LOAD_STATE_WAITING_FOR_REFRESH ):
				{
					if ( pInfo->stop )
					{
						//pInfo->status = PCM_STATUS_READY_TO_STOP;
						StopStream( whichStream );
						return;
					}

					// Wait for SPU buffer cross
					if (!OutsideStreamSPULoadBuffer(whichStream))
					{
						break;
					}

					ShowState( "Stream Running -- Wait For Last Load\n" );
					SmallShowStreamState( "-SRWLL" );

					// Inc state and start it directly
					pInfo->loadState++;
				}

				case ( STREAM_LOAD_STATE_WAITING_FOR_LAST_LOAD ):
				{
					if ( pInfo->stop )
					{
						StopStream( whichStream );
						break;
					}

					// After we got this far, we've crossed the SPU buffer and we
					// know we need to download.  But if the new IOP buffer isn't
					// loaded and download before we cross SPU buffers AGAIN, then 
					// we know the stream has failed.  Time to panic!
					if (!OutsideStreamSPULoadBuffer(whichStream))
					{
						if (!(gEECommand & PCMSTATUS_PANIC))
						{
							// Print only once
							Kprintf("ERROR: Stream %d did not load data in time.  In SPU buffer %d and pos %x\n", whichStream, pInfo->m_spuBufSide, pInfo->m_spuCurAddr - STREAM_SPU_BUF_LOC( whichStream ));
						}
						gEECommand |= PCMSTATUS_PANIC;
						break;
					}

					ShowState( "Stream Running -- Loading\n" );
					SmallShowStreamState( "-SRL" );

					if (DownloadStream(whichStream))
					{
						pInfo->loadState++;
					}

					break;
				}
				case ( STREAM_LOAD_STATE_LOADING ):
				{
					// Check that transfer is done
					if (!pInfo->m_spuTransDone)
					{
						break;
					}

					ShowState( "Stream Running -- Waiting for IRQ\n" );
					SmallShowStreamState( "-SRIRQ" );

					IncStreamBuffer(whichStream);

					pInfo->loadState = STREAM_LOAD_STATE_WAITING_FOR_REFRESH;
					break;
				}
				default:
					printf( "Unknown Stream loading state %d\n", pInfo->loadState );
					break;
			}
			break;

		case PCM_STATUS_TERMINATE: 
			pInfo->m_spuCurAddr = GetNAX( STREAM_CORE, STREAM_VOICE( whichStream ) );

			if ( !pInfo->stop && ( ( pInfo->m_spuCurAddr > pInfo->m_spuEndAddr ) || ( pInfo->m_spuCurAddr <= pInfo->m_spuEndAddr - 16 ) ) )
			{
				break;
			}

		case PCM_STATUS_READY_TO_STOP:
			if ( gUsingStreamDMA != (whichStream + 1) )
			{
				ShowState( "Stream terminate\n" );
				SmallShowStreamState( "-ST!!!" );
				StopStream( whichStream );
			}
			break;

		default:
			printf( "unknown stream status in pcm irx!!!\n" );
			break;
	}
}

// Reserve Stream DMA channel
int LockDMA( int whichStream )
{
	if ( gSPUUsingStreamDMA )
	{
		SmallShowStreamState( " -|spu_no" );		
		//Kprintf("******* Can't lock DMA for Stream %d because of SPU\n", whichStream);
		return FALSE;
	}
	if ( gUsingStreamDMA )
	{
		SmallShowStreamState( " -|no" );		
		//Kprintf("******* Can't lock DMA for Stream %d\n", whichStream);
		return FALSE;
	}
	else
	{
		SmallShowStreamState( " -|ok" );		
		gUsingStreamDMA = 1 + whichStream;
		//Kprintf("Locked DMA for Stream %d\n", whichStream);
		return TRUE;
	}
}

/**********************************************************************************
GetNAX
	Get current NAX (Address where channel is playing in SPU RAM)
	Code must check the NAX 3 times to decide which result is correct.
***********************************************************************************/
u_int GetNAX(int core,int ch)
{
	u_int pos,pos2,pos3;
	ch <<= 1;

	while(1)
	{
		pos=sceSdGetAddr(core|ch|SD_VA_NAX);
		pos2=sceSdGetAddr(core|ch|SD_VA_NAX);
		pos3=sceSdGetAddr(core|ch|SD_VA_NAX);

		if (pos == pos2)
			return(pos);
		else if (pos2 == pos3)
			return(pos2);
	}
}

#if CHECK_TRANSFER_INTEGRITY
void CheckTransfer( int dmaCh )
{
	if ( !sceSdVoiceTransStatus( dmaCh, SD_TRANS_STATUS_CHECK ) )
	{
		int i;
		printf( "waiting for voicetrans" );
		while ( !sceSdVoiceTransStatus( dmaCh, SD_TRANS_STATUS_CHECK ) )
		{
			printf( "." );
			for ( i = 0; i < NUM_WAIT_CLICKS; i++ )
				;
		}
		printf( "\n" );
	}
}
#endif					

// Checks to see if current SPU playing address is outside the loading buffer
int OutsideMusicSPULoadBuffer()
{
	struct StreamInfo *pInfo;
	pInfo = &gMusicInfo;

	if ( pInfo->m_spuBufSide == _1st )
	{
		if ( pInfo->m_spuCurAddr > ( MUSIC_L_SPU_BUF_LOC + SB_BUF_HALF ) )
		{
#if CHECK_TRANSFER_INTEGRITY
			CheckTransfer( TRANS_DMA_CH_MUSIC );
#endif					
			return TRUE;
			//Kprintf("music position past end for SPU buffer %d\n", pInfo->bufSide);
		}
	}
	else
	{
		if ( pInfo->m_spuCurAddr < ( MUSIC_L_SPU_BUF_LOC + SB_BUF_HALF ) )
		{
#if CHECK_TRANSFER_INTEGRITY
			CheckTransfer( TRANS_DMA_CH_MUSIC );
#endif					
			return TRUE;
			//Kprintf("music position past end for SPU buffer %d\n", pInfo->bufSide);
		}
	}

	return FALSE;
}

// Checks to see if current SPU playing address is outside the loading buffer
int OutsideStreamSPULoadBuffer(int whichStream)
{
	struct StreamInfo *pInfo;
	pInfo = &gStreamInfo[ whichStream ];

	if ( pInfo->m_spuBufSide == _1st )
	{
		if ( pInfo->m_spuCurAddr > ( STREAM_SPU_BUF_LOC( whichStream ) + SB_BUF_HALF ) )
		{
#if CHECK_TRANSFER_INTEGRITY
			CheckTransfer( TRANS_DMA_CH_STREAM );
#endif
			return TRUE;
		}
	}
	else
	{
		if ( pInfo->m_spuCurAddr < ( STREAM_SPU_BUF_LOC( whichStream ) + SB_BUF_HALF ) )
		{
#if CHECK_TRANSFER_INTEGRITY
			CheckTransfer( TRANS_DMA_CH_STREAM );
#endif					
			return TRUE;
		}
	}

	return FALSE;
}

// Downloads music data from the IOP to a SPU buffer
int DownloadMusic(int whichChannel)
{
	int music_iop_offset;
	int music_spu_addr;

	struct StreamInfo *pInfo;
	pInfo = &gMusicInfo;

	if (whichChannel == _L)
	{
		//int iopBufferIndex = (pInfo->m_iopOffset < MUSIC_HALF_IOP_BUFFER_SIZE) ? 0 : 1;
		if (!pInfo->m_iopBufLoaded[pInfo->m_iopBufIndex])
		{
			//Kprintf("****** ERROR: Music buffer #%d not loaded.\n", pInfo->m_iopBufIndex);
			return FALSE;
		}

		pInfo->m_spuTransSize = SB_BUF_HALF;
		pInfo->remaining -= SB_BUF_HALF;
		if ( pInfo->remaining < 0 )
		{
			pInfo->m_spuTransSize += pInfo->remaining;
		}

		music_iop_offset = MUSIC_L_IOP_OFFSET;
		music_spu_addr = MUSIC_L_SPU_BUF_LOC;
	}
	else
	{
		music_iop_offset = MUSIC_R_IOP_OFFSET;
		music_spu_addr = MUSIC_R_SPU_BUF_LOC;
	}

	// load in a side:
	if ( pInfo->m_spuBufSide == _1st )
	{
		_AdpcmSetMarkSTART( gpIopBuf + music_iop_offset + pInfo->m_iopOffset, pInfo->m_spuTransSize );
	}
	if ( pInfo->remaining <= 0 )
	{
		_AdpcmSetMarkFINAL( gpIopBuf + music_iop_offset + pInfo->m_iopOffset, pInfo->m_spuTransSize );
		pInfo->m_spuEndAddr = MUSIC_L_SPU_BUF_LOC + pInfo->m_spuTransSize + SB_BUF_HALF * pInfo->m_spuBufSide;
	}
	else if ( pInfo->m_spuBufSide == _2nd )
	{
		_AdpcmSetMarkEND( gpIopBuf + music_iop_offset + pInfo->m_iopOffset, pInfo->m_spuTransSize );
	}

	pInfo->m_spuTransDone = FALSE;
	sceSdVoiceTrans( TRANS_DMA_CH_MUSIC, ( SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA ),
		(unsigned char *) gpIopBuf + music_iop_offset + pInfo->m_iopOffset,
		( music_spu_addr + SB_BUF_HALF * pInfo->m_spuBufSide ), pInfo->m_spuTransSize );
	//Kprintf("Downloading music channel %d from IOP %x to SPU %x of size %d\n", whichChannel, gpIopBuf + music_iop_offset + pInfo->m_iopOffset,
	//		( music_spu_addr + SB_BUF_HALF * pInfo->m_spuBufSide ), pInfo->m_spuTransSize );


	return TRUE;
}

// Downloads audio stream data from the IOP to a SPU buffer
int DownloadStream(int whichStream)
{
	struct StreamInfo *pInfo;
	pInfo = &gStreamInfo[ whichStream ];

	//iopBufferIndex = (pInfo->m_iopOffset < STREAM_HALF_IOP_BUFFER_SIZE) ? 0 : 1;
	if (!pInfo->m_iopBufLoaded[pInfo->m_iopBufIndex])
	{
		//Kprintf("****** ERROR: Stream %d buffer #%d not loaded.\n", whichStream, pInfo->m_iopBufIndex);
		return FALSE;
	}

	if (!LockDMA(whichStream))
	{
		return FALSE;
	}

	pInfo->m_spuTransSize = SB_BUF_HALF;
	pInfo->remaining -= SB_BUF_HALF;
	if ( pInfo->remaining < 0 )
	{
		pInfo->m_spuTransSize += pInfo->remaining;
	}
	//Kprintf( "Stream: IOP offset %d, transfer %d, remaining %d\n", pInfo->m_iopOffset, pInfo->m_spuTransSize, pInfo->remaining);

	// load in left side:
	if ( pInfo->m_spuBufSide == _1st )
	{
		_AdpcmSetMarkSTART( gpIopBuf + STREAM_IOP_OFFSET( whichStream ) + pInfo->m_iopOffset, pInfo->m_spuTransSize );

	}
	if ( pInfo->remaining <= 0 )
	{
		_AdpcmSetMarkFINAL( gpIopBuf + STREAM_IOP_OFFSET( whichStream ) + pInfo->m_iopOffset, pInfo->m_spuTransSize );
		pInfo->m_spuEndAddr = STREAM_SPU_BUF_LOC( whichStream ) + pInfo->m_spuTransSize + SB_BUF_HALF * pInfo->m_spuBufSide;
	}
	else if ( pInfo->m_spuBufSide == _2nd )
	{
		_AdpcmSetMarkEND( gpIopBuf + STREAM_IOP_OFFSET( whichStream ) + pInfo->m_iopOffset, pInfo->m_spuTransSize );
	}

	pInfo->m_spuTransDone = FALSE;
	sceSdVoiceTrans( TRANS_DMA_CH_STREAM, ( SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA ),
		(unsigned char *) gpIopBuf + STREAM_IOP_OFFSET( whichStream ) + pInfo->m_iopOffset,
		( STREAM_SPU_BUF_LOC( whichStream ) + SB_BUF_HALF * pInfo->m_spuBufSide ), pInfo->m_spuTransSize );

	//Kprintf("Downloaded Stream %d buffer #%d\n", whichStream, iopBufferIndex);

	return TRUE;
}

void IncMusicBuffer()
{
	struct StreamInfo *pInfo;
	pInfo = &gMusicInfo;

	pInfo->m_iopOffset += SB_BUF_HALF;
	pInfo->m_spuBufSide = ( pInfo->m_spuBufSide == _1st ) ? _2nd : _1st;

	if ( pInfo->remaining <= 0 )
	{
		// the vag data up to the end should contain all zeros,
		// so it doesn't matter that the interrupt won't happen until the end.
		// stop the music when it gets to the end:
		pInfo->status = PCM_STATUS_TERMINATE;
		ShowState( "Music last buffer\n" );
	}
	else
	{
		if ( (pInfo->m_iopBufIndex == _1st) && (pInfo->m_iopOffset >= MUSIC_HALF_IOP_BUFFER_SIZE) )
		{
			//Dbug_Printf( "music move over\n" );
			pInfo->m_iopOffset += MUSIC_HALF_IOP_BUFFER_SIZE; // move over by half the size since L/R load in together...
			pInfo->m_iopBufIndex = _2nd;
			if ( pInfo->remaining > MUSIC_HALF_IOP_BUFFER_SIZE )
			{
				gEECommand |= PCMSTATUS_NEED_MUSIC_BUFFER_0;
				pInfo->m_iopBufLoaded[0] = FALSE;
			}
		}
		else if ( (pInfo->m_iopBufIndex == _2nd) && ( pInfo->m_iopOffset >= ( MUSIC_IOP_BUFFER_SIZE + MUSIC_HALF_IOP_BUFFER_SIZE ) ) )
		{
			//Dbug_Printf( "music move back\n" );
			pInfo->m_iopOffset = 0;
			pInfo->m_iopBufIndex = _1st;
			if ( pInfo->remaining > MUSIC_HALF_IOP_BUFFER_SIZE )
			{
				gEECommand |= PCMSTATUS_NEED_MUSIC_BUFFER_1;
				pInfo->m_iopBufLoaded[1] = FALSE;
			}
		}
	}
}

void IncStreamBuffer(int whichStream)
{
	struct StreamInfo *pInfo;
	pInfo = &gStreamInfo[ whichStream ];

	pInfo->m_iopOffset += SB_BUF_HALF;
	pInfo->m_spuBufSide = ( pInfo->m_spuBufSide == _1st ) ? _2nd : _1st;
	if ( pInfo->remaining <= 0 )
	{
		// the vag data up to the end should contain all zeros,
		// so it doesn't matter that the interrupt won't happen until the end.
		// stop the stream when it gets to the end:
		pInfo->status = PCM_STATUS_TERMINATE;
	}
	else
	{
		// Must check if it has crossed from one IOP buffer to the other
		if ( (pInfo->m_iopBufIndex == _1st) && (pInfo->m_iopOffset >= STREAM_HALF_IOP_BUFFER_SIZE) )
		{
			SmallShowStreamState( "-SMO" );
			pInfo->m_iopBufIndex = _2nd;
			if ( pInfo->remaining > STREAM_HALF_IOP_BUFFER_SIZE )
			{
				gEECommand |= ( PCMSTATUS_NEED_STREAM0_BUFFER_0 << whichStream );
				pInfo->m_iopBufLoaded[0] = FALSE;
				//Kprintf("Asking for Stream %d buffer #%d\n", whichStream, 0);
			}
		}
		else if ( (pInfo->m_iopBufIndex == _2nd) && pInfo->m_iopOffset >= STREAM_IOP_BUFFER_SIZE )
		{
			SmallShowStreamState( "-SMB" );
			pInfo->m_iopOffset -= STREAM_IOP_BUFFER_SIZE;
			pInfo->m_iopBufIndex = _1st;
			if ( pInfo->remaining > STREAM_HALF_IOP_BUFFER_SIZE )
			{
				gEECommand |= ( PCMSTATUS_NEED_STREAM0_BUFFER_1 << whichStream );
				pInfo->m_iopBufLoaded[1] = FALSE;
				//Kprintf("Asking for Stream %d buffer #%d\n", whichStream, 1);
			}
		}
	}
}

// Start the music that is already preloaded
void StartMusic()
{
	AdpcmSetMusicVolumeDirect( gMusicInfo.volume );
	sceSdSetSwitch( MUSIC_CORE | SD_S_KON, ( ( 1 << MUSIC_R_VOICE ) | ( 1 << MUSIC_L_VOICE ) ) );
}

// Start the stream that is already preloaded
void StartStream(int whichStream)
{
	AdpcmSetStreamVolumeDirect( gStreamInfo[ whichStream ].volume, whichStream );
	// Here's the bug! This was causing a recently paused stream to un-pause...
	if ( !gStreamInfo[ whichStream ].paused )
	{
		sceSdSetParam( STREAM_CORE | ( STREAM_VOICE( whichStream ) << 1 ) | SD_VP_PITCH, gStreamInfo[ whichStream ].pitch );
	}
	sceSdSetSwitch( STREAM_CORE | SD_S_KON, 1 << STREAM_VOICE( whichStream ) );

	//Kprintf("Stream %d using SPU memory (%6x-%6x)\n", whichStream, STREAM_SPU_BUF_LOC( whichStream ), STREAM_SPU_BUF_LOC( whichStream ) + SB_BUF_SIZE - 1);
}

/* internal */
int RefreshStreams( int status )
{
	int i;

    while ( 1 )
	{
		WaitSema(gSem);

		// Update everything
		for ( i = 0; i < NUM_STREAMS; i++ )
		{
			UpdateStream( i );
		}
		UpdateMusic( );

		if ( gMusicInfo.volumeSet )
		{
			gMusicInfo.volumeSet = 0;
			AdpcmSetMusicVolumeDirect( gMusicInfo.volume );
		}
		for ( i = 0; i < NUM_STREAMS; i++ )
		{
			if ( gStreamInfo[ i ].volumeSet )
			{
				gStreamInfo[ i ].volumeSet = 0;
				AdpcmSetStreamVolumeDirect( gStreamInfo[ i ].volume, i );
			}
		}

		SendStatus();
    }
    return 0;
}

void _ShowStreamInfo( struct StreamInfo *pInfo )
{
	printf( "\nStream Info:\n" );
	printf( "paused     %d\n", pInfo->paused );
	printf( "m_spuEndAddr %d\n", pInfo->m_spuEndAddr );
	printf( "m_spuBufSide %d\n", pInfo->m_spuBufSide );
	printf( "m_spuTransSize %d\n", pInfo->m_spuTransSize );
	printf( "loadState  %d\n", pInfo->loadState );
	printf( "volume     %d\n", pInfo->volume );
	printf( "volumeSet  %d\n", pInfo->volumeSet );
	printf( "stop       %d\n", pInfo->stop );
	printf( "status     %d\n", pInfo->status );
	printf( "size       %d\n", pInfo->size );
	printf( "remaining  %d\n", pInfo->remaining );
	printf( "m_iopOffset %d\n", pInfo->m_iopOffset );
	printf( "pitch      %d\n", pInfo->pitch );
	printf( "eecom      %d\n", gEECommand );
}

void PauseStream( int whichStream, int pause )
{
	ShowStreamInfo( &gStreamInfo[ whichStream ] );
	if ( pause )
	{
		if ( !gStreamInfo[ whichStream ].paused )
		{
			sceSdSetParam( STREAM_CORE | ( STREAM_VOICE( whichStream ) << 1 ) | SD_VP_PITCH, 0 );
			gStreamInfo[ whichStream ].paused = 1;
			ShowAction( "Pausing stream\n" );
			SmallShowAction( "-ps" );
		}
	}
	else
	{
		if ( gStreamInfo[ whichStream ].paused )
		{
			sceSdSetParam( STREAM_CORE | ( STREAM_VOICE( whichStream ) << 1 ) | SD_VP_PITCH, gStreamInfo[ whichStream ].pitch );
			gStreamInfo[ whichStream ].paused = 0;
			ShowAction( "Unpausing stream\n" );
			SmallShowAction( "-ups" );
		}
	}
}

volatile int ret = 0;

#define U_DATA( x )	( *( ( unsigned int * ) data_ + x ) ) 
#define DATA( x )	( *( ( int * ) data_ + x ) )

static void *dispatch( unsigned int command, void *data_, int size )
{ 
    int ch;

//    printf( "size %d", size );
//	printf("# dispatch [%04x] %x, %x, %x, %x\n", command, *((int*) data_ + 0), *((int*) data_ + 1), *((int*) data_ + 2), *((int*) data_ + 3));

    ch = command & EzADPCM_CH_MASK;
    switch ( command & EzADPCM_COMMAND_MASK )
	{
		case EzADPCM_INIT:
			ret = AdpcmInit( DATA( 0 ) ); // iop buffer pointer
			break;

		case EzADPCM_QUIT:
			AdpcmQuit( );
			break;

		case EzADPCM_PRELOADMUSIC:
			ret = AdpcmPlayMusic( DATA( 0 ), TRUE );  // size of the entire PCM data in the file
			break;

		case EzADPCM_PRELOADSTREAM:
			ret = AdpcmPlayStream( DATA( 0 ), ch, 0, 0, TRUE); // size of the entire PCM data in the file
			break;

		case EzADPCM_PLAYPRELOADEDMUSIC:
			ret = AdpcmPlayPreloadedMusic();
			break;

		case EzADPCM_PLAYPRELOADEDSTREAM:
			ret = AdpcmPlayPreloadedStream(ch, DATA(0), DATA(1));
			break;

		case EzADPCM_PLAYMUSIC:
			ret = AdpcmPlayMusic( DATA( 0 ), FALSE );  // size of the entire PCM data in the file
			break;

		case EzADPCM_PLAYSTREAM:
			ret = AdpcmPlayStream( DATA( 0 ), ch, DATA( 1 ), DATA( 2 ), FALSE ); // size of the entire PCM data in the file
			break;
		
		case EzADPCM_PAUSEMUSIC:
			if ( DATA( 0 ) )
			{
				if ( !gMusicInfo.paused )
				{
					sceSdSetParam( MUSIC_CORE | ( MUSIC_L_VOICE << 1 ) | SD_VP_PITCH, 0 );
					sceSdSetParam( MUSIC_CORE | ( MUSIC_R_VOICE << 1 ) | SD_VP_PITCH, 0 );
					gMusicInfo.paused = 1;
					ShowAction( "Pausing music\n" );
					SmallShowAction( "-pm" );
				}
			}
			else
			{
				if ( gMusicInfo.paused )
				{
					sceSdSetParam( MUSIC_CORE | ( MUSIC_L_VOICE << 1 ) | SD_VP_PITCH, DEFAULT_PITCH );
					sceSdSetParam( MUSIC_CORE | ( MUSIC_R_VOICE << 1 ) | SD_VP_PITCH, DEFAULT_PITCH );
					gMusicInfo.paused = 0;
					ShowAction( "Unpausing music\n" );
					SmallShowAction( "-upm" );
				}
			}
			break;

		case EzADPCM_PAUSESTREAMS:
		{
			int whichStream;
			
			for ( whichStream = 0; whichStream < NUM_STREAMS; whichStream++ )
			{
				PauseStream( whichStream, DATA( 0 ) );
			}
			break;
		}
		
		case EzADPCM_PAUSESTREAM:
			PauseStream( ch, DATA( 0 ) );
			break;
		
		case EzADPCM_STOPMUSIC:
			ret = AdpcmStopMusic( );
			break;

		case EzADPCM_STOPSTREAMS:
		{
			int i;
			for ( i = 0; i < NUM_STREAMS; i++ )
			{
				ret = AdpcmStopStream( i );
			}
			break;
		}
		
		case EzADPCM_STOPSTREAM:
			if ( NUM_STREAMS > DATA( 0 ) )
				ret = AdpcmStopStream( DATA( 0 )  );
			break;
		
		case EzADPCM_SETMUSICVOL:
			AdpcmSetMusicVolume( U_DATA( 0 ) );
			break;

		case EzADPCM_SETSTREAMVOLANDPITCH:
			sceSdSetParam( STREAM_CORE | ( STREAM_VOICE( ch ) << 1 ) | SD_VP_PITCH, ( U_DATA( 1 ) / 2 ) );
			// Adjust for pitch --> 22,050 hz when standard is 48000
			gStreamInfo[ ch ].pitch = STREAM_PITCH( U_DATA( 1 ) );
			// intentional fall through:
		case EzADPCM_SETSTREAMVOL:
			AdpcmSetStreamVolume( U_DATA( 0 ), ch );
			break;

#if 0
		case EzADPCM_SETSTREAMGLOBVOL:
			AdpcmSetStreamGlobVol( U_DATA( 0 ) );
			break;

		case EzADPCM_GETMUSICSTATUS:
			ret = AdpcmGetMusicStatus( );
			break;
#endif

		case EzADPCM_SDINIT:
			AdpcmSdInit( );
			break;

		default:
			ERROR (("EzADPCM driver error: unknown command %d \n", DATA( 0 ) ) );
			break;
    }
    //printf( "! return value = %x \n", ret );
    return (void*)(&ret);
}

// SifCmd variables

//static sceSifCmdData cmdbuffer[NUM_COMMAND_HANDLERS];
unsigned int gCmdSem;

// Note these can be changed in an interrupt
#define NUM_STREAM_REQUESTS	(10)

static volatile SStreamRequestPacket StreamRequestArray[NUM_STREAM_REQUESTS];
static volatile int FirstStreamRequest;		// Interrupt only reads this value.
static volatile	int FreeStreamRequest;		// This is the main variable that changes in the interrupt

static SSifCmdStreamResultPacket StreamResult;

void request_callback(void *p, void *q);
void load_status_callback(void *p, void *q);

int sce_adpcm_loop( void )
{
	int oldStat;
	int last_cmd_id = -1;

	// Create semaphore to signal command came in
	struct SemaParam sem;
	sem.initCount = 0;
	sem.maxCount = 1;
	sem.attr = AT_THFIFO;
	gCmdSem = CreateSema (&sem);

	// Init the stream queue
	FirstStreamRequest = 0;
	FreeStreamRequest = 0;

	sceSifInitRpc(0);

	// set local buffer & functions
	CpuSuspendIntr(&oldStat);

	// SIFCMD
	// No longer need to call sceSifSetCmdBuffer() since we share it with fileio.irx
	//sceSifSetCmdBuffer( &cmdbuffer[0], NUM_COMMAND_HANDLERS);

	sceSifAddCmdHandler(STREAM_REQUEST_COMMAND, (void *) request_callback, NULL );
	sceSifAddCmdHandler(STREAM_LOAD_STATUS_COMMAND, (void *) load_status_callback, NULL );

	CpuResumeIntr(oldStat);

	// The loop
	while (1) {
		//printf("waiting for pcm command semaphore\n");
		WaitSema(gCmdSem);		// Get signal from callback
		//printf("got pcm command semaphore\n");

		// Note that FreeStreamRequest can change in the interrupt at any time
		// Also, FirstStreamRequest is examined in the interrupt, but just to make sure we don't overflow the buffer
		//Dbg_Assert(FreeStreamRequest != FirstStreamRequest);
		while (FreeStreamRequest != FirstStreamRequest)
		{
			int *p_ret;
			volatile SStreamRequestPacket *p_request = &(StreamRequestArray[FirstStreamRequest]);

#if TEST_PLAY_TIMING
			if (p_request->m_request.m_command == EzADPCM_PLAYPRELOADEDMUSIC)
			{
				test_timing_request_id = p_request->m_request_id;
				test_timing_send_timing_result = TRUE;
				test_timing_hit_point = FALSE;
			}
#endif // TEST_PLAY_TIMING

			//printf("EzPcm: got request id %d with command %x\n", p_request->m_request_id, p_request->m_request.m_command);
			p_ret = dispatch(p_request->m_request.m_command, (void *) p_request->m_request.m_param, 0);

			// Send the result back
			if (last_cmd_id >= 0)	// Wait for previous send to complete (it should already be done, though)
			{
               while(sceSifDmaStat(last_cmd_id) >= 0)
				   printf("Waiting for PCM DMA\n");
			}
			// Gotta copy the id into SStreamRequest
			StreamResult.m_header.opt = p_request->m_request_id;	// Copy id
			StreamResult.m_return_value = *p_ret;
			StreamResult.m_status_flags = AdpcmGetStatus();
            last_cmd_id = sceSifSendCmd(STREAM_RESULT_COMMAND, &StreamResult, sizeof(StreamResult), 0, 0, 0);

			// Increment request index; Note that interrupt can look at this
			FirstStreamRequest = (FirstStreamRequest + 1) % NUM_STREAM_REQUESTS;
		}
	}

    return 0;
}

void request_callback(void *p, void *q)
{
	SSifCmdStreamRequestPacket *h = (SSifCmdStreamRequestPacket *) p;

	// Check to make sure we can add
	int nextFreeReq = (FreeStreamRequest + 1) % NUM_STREAM_REQUESTS;
	if (nextFreeReq == FirstStreamRequest)
	{
		// We can't allow a request to be ignored.  We must abort
		//Dbg_Assert(0);
	}

	// Copy the request into the reuest queue
	StreamRequestArray[FreeStreamRequest].m_request = h->m_request;
	StreamRequestArray[FreeStreamRequest].m_request_id = h->m_header.opt;
	FreeStreamRequest = nextFreeReq;

	// And wake up the dispatch thread
	iSignalSema(gCmdSem);

	return;
}

void load_status_callback(void *p, void *q)
{
	SSifCmdStreamLoadStatusPacket *h = (SSifCmdStreamLoadStatusPacket *) p;

	// Mark buffer as loaded
	if (h->m_stream_num == -1)
	{
		//Kprintf("Music buffer #%d finished loading.\n", h->m_buffer_num);
		gMusicInfo.m_iopBufLoaded[h->m_buffer_num] = TRUE;
	}
	else
	{
		//Kprintf("Stream %d buffer #%d finished loading.\n", h->m_stream_num, h->m_buffer_num);
		gStreamInfo[h->m_stream_num].m_iopBufLoaded[h->m_buffer_num] = TRUE;
	}
}

void SendStatus()
{
	static SSifCmdStreamStatusPacket StreamStatus;

#if TEST_PLAY_TIMING
	if (test_timing_send_timing_result && test_timing_hit_point)
	{
		sEELastCommand = 0xFFFFFFFF;
		StreamStatus.m_header.opt = test_timing_request_id;
		test_timing_send_timing_result = FALSE;
	}
	else
	{
		StreamStatus.m_header.opt = 0;
	}
#endif // TEST_PLAY_TIMING

	// Only send updates when it has changed
	if (AdpcmHasStatusChanged())
	{
		StreamStatus.m_status_flags = AdpcmGetStatus();
		sceSifSendCmd(STREAM_STATUS_COMMAND, &StreamStatus, sizeof(StreamStatus), 0, 0, 0);
	}
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */

/* DON'T ADD STUFF AFTER THIS */
