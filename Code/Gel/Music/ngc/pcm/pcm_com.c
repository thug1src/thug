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
#include <thread.h>
#include "pcm.h"
#include "pcmiop.h"

// ================================================================
#define DATA_INTEGRITY_CHECK 1

#define SHOW_STATE	0
#define SHOW_SMALL_STATE 1
#define SHOW_ACTION 0
#define SHOW_SMALL_ACTION 1
#define DBUGPRINTF	0
#define SHOW_TIMING	1
#define SHOW_STOP	1
#define SHOW_SMALL_COMMANDS	1

#if SHOW_SMALL_COMMANDS
#define ShowSmallCommand printf
#else
#define ShowSmallCommand( x )
#endif

#if SHOW_STOP
#define ShowStop printf   //printf( " %d}{", GetTime( ) ), printf
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
#define ShowAction printf( "Time %d", GetTime( ) ), printf
#else
#define ShowAction( x )
#endif

#if SHOW_SMALL_STATE
#define SmallShowState printf
#else
#define SmallShowState( x )
#endif

#if SHOW_SMALL_ACTION
#define SmallShowAction printf( " %d*", GetTime( ) ), printf
#else
#define SmallShowAction( x )
#endif

#if SHOW_STATE
#define ShowState printf( "Time %d                  ", GetTime( ) ), printf
#else
#define ShowState( x )
#endif

#if DBUGPRINTF
#define Dbug_Printf( x ) printf( x )
#else
#define Dbug_Printf( x )
#endif

//volatile int gTestPause = 0;

#define SHARE_DMA_CH	0

// the program running on the EE side will check to see if any of the buffers need to be filled:
volatile unsigned int gEECommand = 0;

#if SHARE_DMA_CH
#define TRANS_DMA_CH_COMMON		0
#define TRANS_DMA_CH_MUSIC		TRANS_DMA_CH_COMMON
#define TRANS_DMA_CH_STREAM		TRANS_DMA_CH_COMMON

enum{
	LOAD_STATUS_IDLE = 				0,
	LOAD_STATUS_LOADING_MUSIC =		( 1 << 0 ),
	LOAD_STATUS_LOADING_STREAM = 	( 1 << 1 ),
	LOAD_STATUS_NEED_MUSIC =		( 1 << 2 ),
	LOAD_STATUS_NEED_STREAM =		( 1 << 3 ),
};

volatile unsigned int gLoadStatus = LOAD_STATUS_IDLE;

#else
#define TRANS_DMA_CH_MUSIC		1
#define TRANS_DMA_CH_STREAM		0
#endif


#define _1st 0
#define _2nd 1

#define DEFAULT_PITCH	0x1000

unsigned int gThid = 0;
unsigned int gSem = 0;
unsigned int gpIopBuf;		// IOP SMEM
unsigned int gMusicIopOffset;
unsigned int gStreamIopOffset;

enum{
	MUSIC_LOAD_STATE_IDLE,
	MUSIC_LOAD_STATE_PRELOADING_L,
	MUSIC_LOAD_STATE_PRELOADING_R,
	MUSIC_LOAD_STATE_WAITING_FOR_IRQ,
	MUSIC_LOAD_STATE_LOADING_L,
	MUSIC_LOAD_STATE_LOADING_R,
};

volatile unsigned int gMusicPaused = 0;
volatile unsigned int gStreamPaused = 0;
volatile unsigned int gStreamTimeOffset = 0;  // when paused, keep track of time passed since last IRQ
volatile unsigned int gMusicTimeOffset = 0;   // when paused, keep track of time passed since last IRQ

#define RF_START_MUSIC			( 1 << 0 )
#define RF_START_STREAM			( 1 << 1 )
#define RF_UNPAUSE_MUSIC		( 1 << 2 )
#define RF_UNPAUSE_STREAM		( 1 << 3 )

volatile unsigned int gRequestFlags = 0;

volatile unsigned int gTimeOfLastMusicUpdate = 0;
volatile unsigned int gTimeOfLastStreamUpdate = 0;

volatile unsigned int gMusicBufSide = _1st;
volatile unsigned int gMusicTransSize = 0;
volatile unsigned int gMusicLoadState = MUSIC_LOAD_STATE_IDLE;
volatile unsigned int gMusicVolume;
volatile unsigned int gMusicVolumeSet = 0;
volatile unsigned int gMusicStop = 0;
volatile unsigned int gMusicStatus = EzADPCM_STATUS_IDLE;
volatile unsigned int gUpdateMusic = 0;
volatile unsigned int gMusicSize;
volatile unsigned int gMusicRemaining;

enum{
	STREAM_LOAD_STATE_IDLE,
	STREAM_LOAD_STATE_PRELOADING,
	STREAM_LOAD_STATE_WAITING_FOR_IRQ,
	STREAM_LOAD_STATE_LOADING,
};


volatile unsigned int gStreamBufSide = _1st;
volatile unsigned int gStreamTransSize = 0;
volatile unsigned int gStreamLoadState = STREAM_LOAD_STATE_IDLE;
volatile unsigned int gStreamVolume;
volatile unsigned int gStreamVolumeSet = 0;
volatile unsigned int gStreamStop = 0;
volatile unsigned int gStreamStatus = EzADPCM_STATUS_IDLE;
volatile unsigned int gUpdateStream = 0;
volatile unsigned int gStreamSize;
volatile unsigned int gStreamRemaining;

int _AdpcmPlay( int status );

#if SHARE_DMA_CH
static int _AdpcmDmaIntCommon( int, void* );
#else
static int _AdpcmDmaIntMusic( int, void* );
static int _AdpcmDmaIntStream( int, void* );
#endif
static int _AdpcmSpu2Int( int, void * );

#define _L 0
#define _R 1
#define _Lch(x) ((x >> 16) & 0xffff)
#define _Rch(x) (x & 0xffff)

unsigned int GetTime( void )
{
	unsigned int sex, usex, msex;
	struct SysClock sysClock;
	GetSystemTime( &sysClock );
	SysClock2USec( &sysClock, &sex, &usex );
	msex = ( sex * 1000 ) + ( usex / 1000 );
	return ( msex );
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
    sceSdSetParam ( core | ( vnum << 1 ) | SD_VP_PITCH, DEFAULT_PITCH );
    sceSdSetParam ( core | ( vnum << 1 ) | SD_VP_ADSR1, 0x000f );
    sceSdSetParam ( core | ( vnum << 1 ) | SD_VP_ADSR2, 0x1fc0 );
    sceSdSetAddr  ( core | ( vnum << 1 ) | SD_VA_SSA,  pSpuBuf );
    return;
}

/*const char StrayVoiceData[ STRAY_VOICE_BLOCKER_SIZE ] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0C, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0C, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};*/

// EzADPCM_INIT
int AdpcmInit( int pIopBuf )
{
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
		param.entry        = _AdpcmPlay;
		param.initPriority = 5;// BASE_priority-3;
		param.stackSize    = 0x800; // was 800
		param.option = 0;
		gThid = CreateThread (&param);
		printf( "EzADPCM: create thread ID= %d\n", gThid );
		StartThread (gThid, (u_long) NULL);
    }
	
/*
	// This data represents an empty looping sound...
	// The voices wandering across SPU ram will get
	// stuck here, so as not to trigger the interrupts
	// reserved for the stream and music voices and shit.
	strcpy( ( char * )pIopBuf, StrayVoiceData );
	// send that to the SPU ram:
	sceSdVoiceTrans( 0, ( SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA ),
		(unsigned char *) pIopBuf, STRAY_VOICE_BLOCKER_SPU_ADDR, STRAY_VOICE_BLOCKER_SIZE );
	sceSdVoiceTransStatus( 0, SD_TRANS_STATUS_WAIT );*/
		
#if SHARE_DMA_CH
	sceSdSetTransIntrHandler( TRANS_DMA_CH_COMMON, ( sceSdTransIntrHandler ) _AdpcmDmaIntCommon, ( void * ) &gSem );
#else
	sceSdSetTransIntrHandler( TRANS_DMA_CH_MUSIC, ( sceSdTransIntrHandler ) _AdpcmDmaIntMusic, ( void * ) &gSem );
    sceSdSetTransIntrHandler( TRANS_DMA_CH_STREAM, ( sceSdTransIntrHandler ) _AdpcmDmaIntStream, ( void * ) &gSem );
#endif
    sceSdSetSpu2IntrHandler( ( sceSdSpu2IntrHandler ) _AdpcmSpu2Int, ( void * ) &gSem );

	AdpcmSetUpVoice( MUSIC_CORE, MUSIC_L_VOICE, MUSIC_L_SPU_BUF_LOC );
	AdpcmSetUpVoice( MUSIC_CORE, MUSIC_R_VOICE, MUSIC_R_SPU_BUF_LOC );
	AdpcmSetUpVoice( STREAM_CORE, STREAM_VOICE, STREAM_SPU_BUF_LOC );

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

// EzADPCM_PLAYMUSIC
int AdpcmPlayMusic( int size )
{
    extern void AdpcmSetMusicVolumeDirect( unsigned int );

    if ( gMusicStatus != EzADPCM_STATUS_IDLE )
	{
		ShowAction( "NOTE NOTE NOTE NOTE Can't play music -- music isn't in Idle state.\n" );
		return -1;
    }

    AdpcmSetMusicVolumeDirect( gMusicVolume );
				
	gMusicStatus = EzADPCM_STATUS_PRELOAD;
	gMusicLoadState = MUSIC_LOAD_STATE_IDLE;
	gMusicSize = size;
	gMusicTimeOffset = 0;
	if ( gMusicSize < 64 )
	{
		gMusicSize = 64;
	}
	gEECommand &= ~( PCMSTATUS_NEED_MUSIC_BUFFER_0 | PCMSTATUS_NEED_MUSIC_BUFFER_1 );
	gEECommand |= PCMSTATUS_MUSIC_PLAYING;

	// stagger the two streams so there are no IRQ or DMA interrupt conflicts:
	gRequestFlags |= RF_START_MUSIC;
    return 0;
}

// EzADPCM_PLAYSTREAM
int AdpcmPlayStream( int size )
{
    extern void AdpcmSetStreamVolumeDirect( unsigned int );

    if ( gStreamStatus != EzADPCM_STATUS_IDLE )
	{
		printf( "trying to play stream when stream already playing!\n" );
		return -1;
    }

    AdpcmSetStreamVolumeDirect( gStreamVolume );

	gStreamStatus = EzADPCM_STATUS_PRELOAD;
	gStreamSize = size;
	if ( gStreamSize < 64 )
	{
		gStreamSize = 64;
	}
	gStreamLoadState = STREAM_LOAD_STATE_IDLE;
	gEECommand &= ~( PCMSTATUS_NEED_STREAM_BUFFER_0 | PCMSTATUS_NEED_STREAM_BUFFER_1 );
	gEECommand |= PCMSTATUS_STREAM_PLAYING;
	gStreamTimeOffset = 0;
	gRequestFlags = RF_START_STREAM;
	ShowAction( "Requesting stream start\n" );
    SmallShowAction( "-rsb" );
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
void _AdpcmSetStreamVoiceMute( void )
{
    sceSdSetParam( STREAM_CORE | ( STREAM_VOICE << 1) | SD_VP_VOLL, 0);
    sceSdSetParam( STREAM_CORE | ( STREAM_VOICE << 1) | SD_VP_VOLR, 0);
    return;
}

void StopMusic( void )
{
	sceSdSetCoreAttr( MUSIC_CORE | SD_C_IRQ_ENABLE, 0 );
	gMusicStatus = EzADPCM_STATUS_IDLE;
	gEECommand &= ~( PCMSTATUS_MUSIC_PLAYING | PCMSTATUS_NEED_MUSIC_BUFFER_0 | PCMSTATUS_NEED_MUSIC_BUFFER_1 );
	_AdpcmSetMusicVoiceMute( );
	sceSdSetSwitch( MUSIC_CORE | SD_S_KOFF, ( 1 << MUSIC_L_VOICE ) );
	sceSdSetSwitch( MUSIC_CORE | SD_S_KOFF, ( 1 << MUSIC_R_VOICE ) );
	ShowAction( "Stopping music\n" );
    SmallShowAction( "-!sm!" );
}

void StopStream( void )
{
	sceSdSetCoreAttr( STREAM_CORE | SD_C_IRQ_ENABLE, 0 );
	gStreamStatus = EzADPCM_STATUS_IDLE;
	gEECommand &= ~( PCMSTATUS_STREAM_PLAYING | PCMSTATUS_NEED_STREAM_BUFFER_0 | PCMSTATUS_NEED_STREAM_BUFFER_1 );
	_AdpcmSetStreamVoiceMute( );
	sceSdSetSwitch( STREAM_CORE | SD_S_KOFF, ( 1 << STREAM_VOICE ) );
	ShowAction( "Stopping stream\n" );
    SmallShowAction( "-!ss!" );
}

#define NUM_WAIT_CLICKS 25000

// EzADPCM_STOPMUSIC
int AdpcmStopMusic( void )
{
	int i;
	int j = 0;

	if ( gMusicStatus != EzADPCM_STATUS_IDLE )
	{
		if ( gRequestFlags & RF_START_MUSIC )
		{
			gRequestFlags &= ~RF_START_MUSIC;
			gMusicStatus = EzADPCM_STATUS_IDLE;
			ShowAction( "Supressing music request\n" );
			SmallShowAction( "-smr" );
		}
		else if ( gMusicPaused )
		{
			// if loading is happening, wait then just stop:
			ShowStop( "stop music %d", GetTime( ) );
			while ( gMusicLoadState != MUSIC_LOAD_STATE_WAITING_FOR_IRQ )
			{
				for ( i = 0; i < NUM_WAIT_CLICKS; i++ )
					;
				j++;
				if ( j > NUM_WAIT_CLICKS )
				{
					j = 0;
					ShowStop( "." );
				}
			}
//			ShowStop( "\n" );
			ShowAction( "Stopped paused music\n" );
			SmallShowAction( "-spm" );
			StopMusic( );
		}
		else
		{
			ShowStop( " %d-stopmusic", GetTime( ) );
			gMusicStop++;
			while ( gMusicStatus != EzADPCM_STATUS_IDLE )
			{
				for ( i = 0; i < NUM_WAIT_CLICKS; i++ )
					;
				if ( j++ > NUM_WAIT_CLICKS )
				{
					j = 0;
					ShowStop( "." );
					//printf( " ms %d mls %d rf %d eecom %d\n", gMusicStatus, gMusicLoadState, gRequestFlags, gEECommand );
				}
			}
			//ShowStop( "\n" );
		}
		gEECommand &= ~ ( PCMSTATUS_MUSIC_PLAYING | PCMSTATUS_NEED_MUSIC_BUFFER_0 | PCMSTATUS_NEED_MUSIC_BUFFER_1 );
		return ( 1 );
    }
	return ( 0 );
}

// EzADPCM_STOPSTREAM
int AdpcmStopStream( void )
{
	int i;
	int j = 0;

	if ( gStreamStatus != EzADPCM_STATUS_IDLE )
	{
		if ( gRequestFlags & RF_START_STREAM )
		{
			gRequestFlags &= ~RF_START_STREAM;
			gStreamStatus = EzADPCM_STATUS_IDLE;
			ShowAction( "Supressing stream request\n" );
			SmallShowAction( "-ssr" );
		}
		else if ( gStreamPaused )
		{
			// if loading is happening, wait then just stop:
			ShowStop( " %d-Stop p stream", GetTime( ) );
			while ( gStreamLoadState != STREAM_LOAD_STATE_WAITING_FOR_IRQ )
			{
				for ( i = 0; i < NUM_WAIT_CLICKS; i++ )
					;
				ShowStop( "." );
			}
			//ShowStop( "\n" );
			StopStream( );
		}
		else
		{
			ShowStop( " %d-Stop stream", GetTime( ) );
			gStreamStop++;
			while ( gStreamStatus != EzADPCM_STATUS_IDLE )
			{
				for ( i = 0; i < NUM_WAIT_CLICKS; i++ )
					;
				if ( j++ > NUM_WAIT_CLICKS )
				{
					ShowStop( "." );
//					printf( " ss %x sls %d rf %d eecom %d\n", gStreamStatus, gStreamLoadState, gRequestFlags, gEECommand );
					j = 0;
				}
			}
			//ShowStop( "\n" );
			SmallShowAction( "-ss" );
			ShowAction( "Stopped stream\n" );
		}
		gEECommand &= ~ ( PCMSTATUS_STREAM_PLAYING | PCMSTATUS_NEED_STREAM_BUFFER_0 | PCMSTATUS_NEED_STREAM_BUFFER_1 );
		return ( 1 );
    }
	return ( 0 );
}

// EzADPCM_SETMUSICVOL
void AdpcmSetMusicVolume( unsigned int vol )
{
    gMusicVolumeSet = 1;
    gMusicVolume = vol;
    return;
}

// EzADPCM_SETMUSICVOLDIRECT
void AdpcmSetMusicVolumeDirect( unsigned int vol )
{
    gMusicVolume = vol;
    sceSdSetParam ( MUSIC_CORE | ( MUSIC_L_VOICE << 1 ) | SD_VP_VOLL, vol );
    sceSdSetParam ( MUSIC_CORE | ( MUSIC_L_VOICE << 1 ) | SD_VP_VOLR, 0 );
    sceSdSetParam ( MUSIC_CORE | ( MUSIC_R_VOICE << 1 ) | SD_VP_VOLL, 0 );
    sceSdSetParam ( MUSIC_CORE | ( MUSIC_R_VOICE << 1 ) | SD_VP_VOLR, vol );
    return;
}

// EzADPCM_SETSTREAMVOL
void AdpcmSetStreamVolume( unsigned int vol )
{
    gStreamVolumeSet = 1;
    gStreamVolume = vol;
    return;
}

// EzADPCM_SETSTREAMVOLDIRECT
void AdpcmSetStreamVolumeDirect( unsigned int vol )
{
    gStreamVolume = vol;
    sceSdSetParam ( STREAM_CORE | ( STREAM_VOICE << 1 ) | SD_VP_VOLL, _Lch( vol ) );
    sceSdSetParam ( STREAM_CORE | ( STREAM_VOICE << 1 ) | SD_VP_VOLR, _Rch( vol ) );
    return;
}

// Shouldn't need these unless debugging or something --
// Instead just get gEECommand each frame and act accordingly.

// EzADPCM_GETMUSICSTATUS
unsigned int AdpcmGetMusicStatus( void )
{
	return gMusicStatus;
}

// EzADPCM_GETSTREAMSTATUS
unsigned int AdpcmGetStreamStatus( void )
{
    return gStreamStatus;
}

// actual time 300 ms across one buffer (600ms across double buffer)
#define SINGLE_BUFFER_MS	300
#define MIN_INTERRUPT_SEPARATION_MS		90
#define MAX_MS_BETWEEN_STREAMS			( SINGLE_BUFFER_MS - MIN_INTERRUPT_SEPARATION_MS )
#define MIN_MS_BETWEEN_STREAMS			( MIN_INTERRUPT_SEPARATION_MS )

int SafeForStreamToGo( unsigned int time )
{
	if ( gMusicStatus == EzADPCM_STATUS_IDLE )
	{
		SmallShowState( "[ss]" );
		return ( 1 );
	}
	else
	{
		time -= gStreamTimeOffset;
		if ( gMusicPaused )
		{
			if ( time > gMusicPaused + MIN_MS_BETWEEN_STREAMS )
			{
				SmallShowState( "[ss0]" );
				return ( 1 );
			}
		}
		else
		{
			int musicMod;
			int streamMod;
			musicMod = gTimeOfLastMusicUpdate % SINGLE_BUFFER_MS;
			streamMod = time % SINGLE_BUFFER_MS;
			if ( musicMod > streamMod )
			{
				if ( ( musicMod - streamMod > MIN_MS_BETWEEN_STREAMS ) &&
					( ( ( streamMod + SINGLE_BUFFER_MS ) - musicMod ) > MIN_MS_BETWEEN_STREAMS ) )
				{
					SmallShowState( "[ss1]" );
					return ( 1 );
				}
			}
			else if ( ( streamMod - musicMod > MIN_MS_BETWEEN_STREAMS ) &&
					( ( ( musicMod + SINGLE_BUFFER_MS ) - streamMod ) > MIN_MS_BETWEEN_STREAMS ) )
			{
				SmallShowState( "[ss2]" );
				return ( 1 );
			}
		}
/*		
		else if ( ( time > gTimeOfLastMusicUpdate + MIN_MS_BETWEEN_STREAMS ) &&
			 ( time < gTimeOfLastMusicUpdate + MAX_MS_BETWEEN_STREAMS ) )
		{
			SmallShowState( "[ss1]" );
			return ( 1 );
		}
		else
		{
			unsigned int predictedTimeOfInterrupt;
			predictedTimeOfInterrupt = ( time + SINGLE_BUFFER_MS );
			if ( ( predictedTimeOfInterrupt > ( gTimeOfLastMusicUpdate + MIN_MS_BETWEEN_STREAMS ) )
				&& ( predictedTimeOfInterrupt < ( gTimeOfLastMusicUpdate + MAX_MS_BETWEEN_STREAMS ) ) )
			{
				SmallShowState( "[ss2]" );
				return ( 1 );
			}
			else
			{
				//printf( " <>gsto %d<>", gStreamTimeOffset );
				SmallShowState( "[nss]" ); //fe ss %x sls %d eecom %d\n", gStreamStatus, gStreamLoadState, gEECommand );
			}
		}*/
	}
	SmallShowState( "[nss]" ); //fe ss %x sls %d eecom %d\n", gStreamStatus, gStreamLoadState, gEECommand );
	return ( 0 );
}

int SafeForMusicToGo( unsigned int time )
{
	if ( gStreamStatus == EzADPCM_STATUS_IDLE )
	{
		SmallShowState( "[ms]" ); //fe ss %x sls %d eecom %d\n", gStreamStatus, gStreamLoadState, gEECommand );
		return ( 1 );
	}
	else
	{
		time -= gMusicTimeOffset;
		if ( gStreamPaused )
		{
			if ( time > gStreamPaused + MIN_MS_BETWEEN_STREAMS )
			{
				SmallShowState( "[ms0]" ); //fe ss %x sls %d eecom %d\n", gStreamStatus, gStreamLoadState, gEECommand );
				return ( 1 );
			}
		}
		else
		{
			int streamMod;
			int musicMod;
			streamMod = gTimeOfLastStreamUpdate % SINGLE_BUFFER_MS;
			musicMod = time % SINGLE_BUFFER_MS;
			if ( musicMod > streamMod )
			{
				if ( ( musicMod - streamMod > MIN_MS_BETWEEN_STREAMS ) &&
					( ( ( streamMod + SINGLE_BUFFER_MS ) - musicMod ) > MIN_MS_BETWEEN_STREAMS ) )
				{
					SmallShowState( "[ms1]" );
					return ( 1 );
				}
			}
			else if ( ( streamMod - musicMod > MIN_MS_BETWEEN_STREAMS ) &&
					( ( ( musicMod + SINGLE_BUFFER_MS ) - streamMod ) > MIN_MS_BETWEEN_STREAMS ) )
			{
				SmallShowState( "[ms2]" );
				return ( 1 );
			}
		}
/*		
        else if ( ( time > gTimeOfLastStreamUpdate + MIN_MS_BETWEEN_STREAMS ) &&
			 ( time < gTimeOfLastStreamUpdate + MAX_MS_BETWEEN_STREAMS ) )
		{
			SmallShowState( "[ms1]" ); //fe ss %x sls %d eecom %d\n", gStreamStatus, gStreamLoadState, gEECommand );
			return ( 1 );
		}
		else
		{
			unsigned int predictedTimeOfInterrupt;
			predictedTimeOfInterrupt = ( time + SINGLE_BUFFER_MS );
			if ( ( predictedTimeOfInterrupt > ( gTimeOfLastStreamUpdate + MIN_MS_BETWEEN_STREAMS ) )
				&& ( predictedTimeOfInterrupt < ( gTimeOfLastStreamUpdate + MAX_MS_BETWEEN_STREAMS ) ) )
			{
				SmallShowState( "[ms2]" ); //fe ss %x sls %d eecom %d\n", gStreamStatus, gStreamLoadState, gEECommand );
				return ( 1 );
			}
			else
			{
				SmallShowState( "[nsm]" ); //fe ss %x sls %d eecom %d\n", gStreamStatus, gStreamLoadState, gEECommand );
			}
		}*/
		
	}
	SmallShowState( "[nsm]" ); //fe ss %x sls %d eecom %d\n", gStreamStatus, gStreamLoadState, gEECommand );
	return ( 0 );
}

// EzADPCM_GETSTATUS
unsigned int AdpcmGetStatus( void )
{
	unsigned int temp;
	unsigned int time;
	
	if ( gRequestFlags & ( RF_START_MUSIC | RF_START_STREAM | RF_UNPAUSE_MUSIC | RF_UNPAUSE_STREAM ) )
	{
		time = GetTime( );
		if ( ( gRequestFlags & RF_START_STREAM ) && ( SafeForStreamToGo( time ) ) )
		{
			gUpdateStream = 1;
			gTimeOfLastStreamUpdate = time;
			ShowAction( "Starting stream\n" );
			SmallShowAction( "-sb" );
			SignalSema( gSem );
			gRequestFlags &= ~RF_START_STREAM;
		}
		else if ( ( gRequestFlags & RF_START_MUSIC ) && ( SafeForMusicToGo( time ) ) )
		{
			gUpdateMusic = 1;
			gTimeOfLastStreamUpdate = time;
			ShowAction( "Starting music\n" );
			SmallShowAction( "-mb" );
			SignalSema( gSem );
			gRequestFlags &= ~RF_START_MUSIC;
		}
		else  // unpause one or both of these motherfuckers:
		{
			if ( ( gRequestFlags & RF_UNPAUSE_STREAM ) && ( SafeForStreamToGo( time ) )	)
			{
				sceSdSetParam( STREAM_CORE | ( STREAM_VOICE << 1 ) | SD_VP_PITCH, DEFAULT_PITCH );
				gStreamPaused = 0;
				gTimeOfLastStreamUpdate = time - gStreamTimeOffset;
				gStreamTimeOffset = 0;
				gRequestFlags &= ~RF_UNPAUSE_STREAM;
				ShowAction( "Unpausing stream\n" );
				SmallShowAction( "-ups" );
			}
			if ( ( gRequestFlags & RF_UNPAUSE_MUSIC ) && ( SafeForMusicToGo( time ) ) )
			{
				sceSdSetParam( MUSIC_CORE | ( MUSIC_L_VOICE << 1 ) | SD_VP_PITCH, DEFAULT_PITCH );
				sceSdSetParam( MUSIC_CORE | ( MUSIC_R_VOICE << 1 ) | SD_VP_PITCH, DEFAULT_PITCH );
				gMusicPaused = 0;
				gTimeOfLastMusicUpdate = time - gMusicTimeOffset;
				gMusicTimeOffset = 0;
				gRequestFlags &= ~RF_UNPAUSE_MUSIC;
				ShowAction( "Unpausing music\n" );
				SmallShowAction( "-upm" );
			}
		}
	}
	
	temp = gEECommand;
	gEECommand &= ~( PCMSTATUS_NEED_MUSIC_BUFFER_0 | PCMSTATUS_NEED_MUSIC_BUFFER_1 | 
					PCMSTATUS_NEED_STREAM_BUFFER_0 | PCMSTATUS_NEED_STREAM_BUFFER_1 );
#if SHOW_SMALL_COMMANDS
	if ( temp & ( PCMSTATUS_NEED_MUSIC_BUFFER_0 | PCMSTATUS_NEED_MUSIC_BUFFER_1 | 
					PCMSTATUS_NEED_STREAM_BUFFER_0 | PCMSTATUS_NEED_STREAM_BUFFER_1 ) )
	{
		ShowSmallCommand( " <_L_>" );
	}
#endif
	return temp;
}

// ================================================================

#define _ADPCM_MARK_START 0x04
#define _ADPCM_MARK_LOOP  0x02
#define _ADPCM_MARK_END   0x01

#define _AdpcmSetMarkSTART(a,s) { \
  *((unsigned char *)((a)+1)) = \
	(_ADPCM_MARK_LOOP | _ADPCM_MARK_START); \
  *((unsigned char *)((a)+0x10+1)) = \
	_ADPCM_MARK_LOOP; \
  *((unsigned char *)((a)+(s)-0x0f)) = \
	_ADPCM_MARK_LOOP; \
	FlushDcache (); }
#define _AdpcmSetMarkEND(a,s) { \
  *((unsigned char *)((a)+1)) = \
	 _ADPCM_MARK_LOOP; \
  *((unsigned char *)((a)+0x10+1)) = \
	_ADPCM_MARK_LOOP; \
  *((unsigned char *)((a)+(s)-0x0f)) = \
	(_ADPCM_MARK_LOOP | _ADPCM_MARK_END); \
	FlushDcache (); }

#define _AdpcmSetMarkFINAL(a,s) { \
  *((unsigned char *)((a)+(s)-0x0f)) = \
	(_ADPCM_MARK_LOOP | _ADPCM_MARK_START | _ADPCM_MARK_END); \
	FlushDcache (); }

#define _AdpcmSetMarkSTARTpre(a,s) { \
  *((unsigned char *)((a)+1)) = \
	(_ADPCM_MARK_LOOP | _ADPCM_MARK_START); \
  *((unsigned char *)((a)+0x10+1)) = \
	_ADPCM_MARK_LOOP; \
	FlushDcache (); }
#define _AdpcmSetMarkENDpre(a,s) { \
  *((unsigned char *)((a)+(s)-0x0f)) = \
	(_ADPCM_MARK_LOOP | _ADPCM_MARK_END); \
	FlushDcache (); }

#if SHARE_DMA_CH

/* internal */
static int _AdpcmDmaIntCommon(int ch, void *common) // DMA Interrupt -- when transfering data to SPU2 from IOP
{
	if ( gLoadStatus & LOAD_STATUS_LOADING_MUSIC )
	{
		if ( gMusicStatus != EzADPCM_STATUS_IDLE )
		{
			gUpdateMusic++;
			iSignalSema (* (int *) common);
			return 1;
		}
	}
	else if ( gLoadStatus & LOAD_STATUS_LOADING_STREAM )
	{
		if ( gStreamStatus != EzADPCM_STATUS_IDLE )
		{
			gUpdateStream++;
			iSignalSema (* (int *) common);
			return 1;
		}
	}
	return 0;
}

#else

/* internal */
static int _AdpcmDmaIntMusic(int ch, void *common)	// DMA Interrupt -- when transfering data to SPU2 from IOP
{
	if ( gMusicStatus != EzADPCM_STATUS_IDLE )
	{
		gUpdateMusic++;
		iSignalSema (* (int *) common);
		return 1;
	}
	return 0;
}

/* internal */
static int _AdpcmDmaIntStream(int ch, void *common)	// DMA Interrupt -- when transfering data to SPU2 from IOP
{
	if ( gStreamStatus != EzADPCM_STATUS_IDLE )
	{
		gUpdateStream++;
		iSignalSema (* (int *) common);
		return 1;
	}
	return 0;
}

#endif

/* internal */
static int _AdpcmSpu2Int( int core, void *common ) // SPU2 Voice Interrupt
{
	if ( core == ( 1 << MUSIC_CORE ) )
	{
		if ( gMusicLoadState != MUSIC_LOAD_STATE_WAITING_FOR_IRQ )
		{
			int shit;
			for ( shit = 0; shit < 666; shit++ )
			{
				printf( "shit\n" );
				return 0;
			}
		}
		gUpdateMusic++;
	}
	else
	{
		if ( gStreamLoadState != STREAM_LOAD_STATE_WAITING_FOR_IRQ )
		{
			int shit;
			for ( shit = 0; shit < 666; shit++ )
			{
				printf( "shit\n" );
				return 0;
			}
		}
		gUpdateStream++;
	}
	iSignalSema (* (int *) common);
    return 1;
}

#define LAST_VAG_POS 15

//prototype:
void UpdateStream( void );

void UpdateMusic( void )
{
	switch ( gMusicStatus )
	{
		case EzADPCM_STATUS_IDLE:
			break;

		case EzADPCM_STATUS_PRELOAD:
			switch ( gMusicLoadState )
			{
				case ( MUSIC_LOAD_STATE_IDLE ):
				{
#if SHARE_DMA_CH					
					if ( gLoadStatus & LOAD_STATUS_LOADING_STREAM )
					{
						gLoadStatus |= LOAD_STATUS_NEED_MUSIC;
						return;
					}
					gLoadStatus |= LOAD_STATUS_LOADING_MUSIC;
#endif
					ShowState( "music preload l\n" );
					SmallShowState( "-MPL" );
					gMusicBufSide = _1st;
					gMusicIopOffset = 0;
					gMusicRemaining = gMusicSize;
					gMusicRemaining -= SB_BUF_SIZE;
					gMusicTransSize = SB_BUF_SIZE;
					if ( gMusicRemaining < 0 )
					{
						gMusicTransSize += gMusicRemaining;
					}

#if DATA_INTEGRITY_CHECK
					{
						int gronk;
						for ( gronk = 0; gronk < gMusicTransSize >> 4; gronk++ )
						{
							if ( *( ( char * )( 1 + gpIopBuf + MUSIC_L_IOP_OFFSET + ( gronk * 16 ) ) ) != 0 )
							{
								printf( "Fucked up data!!!!" );
							}
						}
					}
#endif
					
					// preload left:
					_AdpcmSetMarkSTARTpre( gpIopBuf + MUSIC_L_IOP_OFFSET, gMusicTransSize );
					if ( gMusicRemaining > 0 )
					{
						_AdpcmSetMarkENDpre( gpIopBuf + MUSIC_L_IOP_OFFSET, gMusicTransSize );
					}
					else
					{
						_AdpcmSetMarkFINAL( gpIopBuf + MUSIC_L_IOP_OFFSET, gMusicTransSize );
					}
					sceSdVoiceTrans( TRANS_DMA_CH_MUSIC, ( SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA ),
						(unsigned char *) ( gpIopBuf + MUSIC_L_IOP_OFFSET ),
						MUSIC_L_SPU_BUF_LOC, gMusicTransSize );
					gMusicLoadState++;
					break;
				}
				case ( MUSIC_LOAD_STATE_PRELOADING_L ):
				{
					ShowState( "music preload r\n" );
					SmallShowState( "-MPR" );
					// preload right:
#if DATA_INTEGRITY_CHECK
					{
						int gronk;
						for ( gronk = 0; gronk < gMusicTransSize >> 4; gronk++ )
						{
							if ( *( ( char * )( 1 + gpIopBuf + MUSIC_R_IOP_OFFSET + ( gronk * 16 ) ) ) != 0 )
							{
								printf( "Fucked up data!!!!" );
							}
						}
					}
#endif
					_AdpcmSetMarkSTARTpre( gpIopBuf + MUSIC_R_IOP_OFFSET, gMusicTransSize );
					if ( gMusicRemaining > 0 )
					{
						_AdpcmSetMarkENDpre( gpIopBuf + MUSIC_R_IOP_OFFSET, gMusicTransSize );
					}
					else
					{
						_AdpcmSetMarkFINAL( gpIopBuf + MUSIC_R_IOP_OFFSET, gMusicTransSize );
					}
					sceSdVoiceTrans( TRANS_DMA_CH_MUSIC, ( SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA ),
						(unsigned char *) ( gpIopBuf + MUSIC_R_IOP_OFFSET ),
						MUSIC_R_SPU_BUF_LOC, gMusicTransSize );
					gMusicLoadState++;
					break;
				}	
				case ( MUSIC_LOAD_STATE_PRELOADING_R ):
				{
					ShowState( "Music starting -- waiting for IRQ\n" );
					SmallShowState( " %d-MSIRQ", GetTime( ) );
					gMusicIopOffset = SB_BUF_SIZE;
					if ( gMusicRemaining <= 0 )
					{
						sceSdSetAddr( MUSIC_CORE | SD_A_IRQA, MUSIC_L_SPU_BUF_LOC + gMusicTransSize - LAST_VAG_POS );
						gMusicStatus = EzADPCM_STATUS_TERMINATE;
					}
					else
					{
						sceSdSetAddr( MUSIC_CORE | SD_A_IRQA, MUSIC_L_SPU_BUF_LOC + SB_BUF_HALF );
						gMusicStatus = EzADPCM_STATUS_RUNNING;
					}
					AdpcmSetMusicVolumeDirect( gMusicVolume );
					sceSdSetCoreAttr( MUSIC_CORE | SD_C_IRQ_ENABLE, 1 );
					sceSdSetSwitch( MUSIC_CORE | SD_S_KON, ( ( 1 << MUSIC_R_VOICE ) | ( 1 << MUSIC_L_VOICE ) ) );
					gMusicLoadState++;
					gTimeOfLastMusicUpdate = GetTime( );
//					printf( "music wait IRQ...\n" );
#if SHARE_DMA_CH					
					gLoadStatus &= ~LOAD_STATUS_LOADING_MUSIC;
					if ( gLoadStatus & LOAD_STATUS_NEED_STREAM )
					{
						gLoadStatus &= ~LOAD_STATUS_NEED_STREAM;
						UpdateStream( );
					}
#endif
					break;
				}
			}
			break;

		case EzADPCM_STATUS_RUNNING:
			switch ( gMusicLoadState )
			{
				case ( MUSIC_LOAD_STATE_WAITING_FOR_IRQ ):
				{
#if SHARE_DMA_CH					
					if ( gLoadStatus & LOAD_STATUS_LOADING_STREAM )
					{
						gLoadStatus |= LOAD_STATUS_NEED_MUSIC;
						return;
					}
					gLoadStatus |= LOAD_STATUS_LOADING_MUSIC;
#endif					
					ShowState( "Music Running -- Loading L\n" );
					SmallShowState( "-MRLL" );
					sceSdSetCoreAttr ( MUSIC_CORE | SD_C_IRQ_ENABLE, 0);
					gMusicTransSize = SB_BUF_HALF;
					gMusicRemaining -= SB_BUF_HALF;
					if ( gMusicRemaining < 0 )
					{
						gMusicTransSize += gMusicRemaining;
					}
#if DATA_INTEGRITY_CHECK
					{
						int gronk;
						for ( gronk = 0; gronk < gMusicTransSize >> 4; gronk++ )
						{
							if ( *( ( char * )( 1 + gpIopBuf + MUSIC_L_IOP_OFFSET + gMusicIopOffset + ( gronk * 16 ) ) ) != 0 )
							{
								printf( "Fucked up data!!!!" );
							}
						}
					}
#endif
					
					// load in left side:
					if ( gMusicBufSide == _1st )
					{
						_AdpcmSetMarkSTART( gpIopBuf + MUSIC_L_IOP_OFFSET + gMusicIopOffset, gMusicTransSize );
					}
					if ( gMusicRemaining <= 0 )
					{
						_AdpcmSetMarkFINAL( gpIopBuf + MUSIC_L_IOP_OFFSET + gMusicIopOffset, gMusicTransSize );
					}
					else if ( gMusicBufSide == _2nd )
					{
						_AdpcmSetMarkEND( gpIopBuf + MUSIC_L_IOP_OFFSET + gMusicIopOffset, gMusicTransSize );
					}
					
					sceSdVoiceTrans( TRANS_DMA_CH_MUSIC, ( SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA ),
						(unsigned char *) gpIopBuf + MUSIC_L_IOP_OFFSET + gMusicIopOffset,
						( MUSIC_L_SPU_BUF_LOC + SB_BUF_HALF * gMusicBufSide ), gMusicTransSize );
					gMusicLoadState++;
					break;
				}
				case ( MUSIC_LOAD_STATE_LOADING_L ):
				{
					ShowState( "Music Running -- Loading R\n" );
					SmallShowState( "-MRLR" );
#if DATA_INTEGRITY_CHECK
					{
						int gronk;
						for ( gronk = 0; gronk < gMusicTransSize >> 4; gronk++ )
						{
							if ( *( ( char * )( 1 + gpIopBuf + MUSIC_R_IOP_OFFSET + gMusicIopOffset + ( gronk * 16 ) ) ) != 0 )
							{
								printf( "Fucked up data!!!!" );
							}
						}
					}
#endif
					
					// load in right side:
					if ( gMusicBufSide == _1st )
					{
						_AdpcmSetMarkSTART( gpIopBuf + MUSIC_R_IOP_OFFSET + gMusicIopOffset, gMusicTransSize );
					}
					if ( gMusicRemaining <= 0 )
					{
						_AdpcmSetMarkFINAL( gpIopBuf + MUSIC_R_IOP_OFFSET + gMusicIopOffset, gMusicTransSize );
					}
					else if ( gMusicBufSide == _2nd )
					{
						_AdpcmSetMarkEND( gpIopBuf + MUSIC_R_IOP_OFFSET + gMusicIopOffset, gMusicTransSize );
					}
					sceSdVoiceTrans( TRANS_DMA_CH_MUSIC, ( SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA ),
						(unsigned char *) gpIopBuf + MUSIC_R_IOP_OFFSET + gMusicIopOffset,
						( MUSIC_R_SPU_BUF_LOC + SB_BUF_HALF * gMusicBufSide ), gMusicTransSize );
					gMusicLoadState++;
					break;
				}
				case ( MUSIC_LOAD_STATE_LOADING_R ):
				{
					ShowState( "Music Running -- Waiting for IRQ\n" );
					SmallShowState( " %d-MRIRQ", GetTime( ) );
					// reset the interrupt:
					if ( gMusicRemaining <= 0 )
					{
						sceSdSetAddr( MUSIC_CORE | SD_A_IRQA, gMusicTransSize - LAST_VAG_POS + MUSIC_L_SPU_BUF_LOC + SB_BUF_HALF * gMusicBufSide );
					}
					else
					{
						sceSdSetAddr( MUSIC_CORE | SD_A_IRQA, MUSIC_L_SPU_BUF_LOC + SB_BUF_HALF * gMusicBufSide );
					}
					gTimeOfLastMusicUpdate = GetTime( );
					gMusicIopOffset += SB_BUF_HALF;
					gMusicBufSide = ( gMusicBufSide == _1st ) ? _2nd : _1st;
					
					if ( gMusicRemaining <= 0 )
					{
						// the vag data up to the end should contain all zeros,
						// so it doesn't matter that the interrupt won't happen until the end.
						// stop the music when it gets to the end:
						gMusicStop++;
					}
					else
					{
						if ( gMusicIopOffset == HALF_BUFFER_SIZE )
						{
							//Dbug_Printf( "music move over\n" );
							gMusicIopOffset += HALF_BUFFER_SIZE; // move over by half the size since L/R load in together...
							if ( gMusicRemaining > HALF_BUFFER_SIZE )
							{
								gEECommand |= PCMSTATUS_NEED_MUSIC_BUFFER_0;
							}
						}
						else if ( gMusicIopOffset == ( SINGLE_STREAM_BUFFER_SIZE + HALF_BUFFER_SIZE ) )
						{
							//Dbug_Printf( "music move back\n" );
							gMusicIopOffset = 0;
							if ( gMusicRemaining > HALF_BUFFER_SIZE )
							{
								gEECommand |= PCMSTATUS_NEED_MUSIC_BUFFER_1;
							}
						}
					}
					if ( gMusicStop )
					{
						gMusicStop = 0;
						gMusicStatus = EzADPCM_STATUS_TERMINATE;
					}
					sceSdSetCoreAttr( MUSIC_CORE | SD_C_IRQ_ENABLE, 1);
					gMusicLoadState = MUSIC_LOAD_STATE_WAITING_FOR_IRQ;
//					printf( "music wait IRQ\n" );
#if SHARE_DMA_CH
					gLoadStatus &= ~LOAD_STATUS_LOADING_MUSIC;
					if ( gLoadStatus & LOAD_STATUS_NEED_STREAM )
					{
						gLoadStatus &= ~LOAD_STATUS_NEED_STREAM;
						UpdateStream( );
					}
#endif
					break;
				}
				default:
					printf( "Unknown music loading state %d\n", gMusicLoadState );
					break;
			}
			break;

		case EzADPCM_STATUS_TERMINATE: 
			ShowState( "Music terminate\n" );
			SmallShowState( "-MT!!!" );
			StopMusic( );
			break;

		default:
			printf( "unknown music status in pcm irx!!!\n" );
			break;
	}
}

void UpdateStream( void )
{
//	printf( "hmmm %d %d bufside %d left %d\n", gStreamStatus, gStreamLoadState, gStreamBufSide, gStreamRemaining );
	switch ( gStreamStatus )
	{
		case EzADPCM_STATUS_IDLE:
			break;

		case EzADPCM_STATUS_PRELOAD:
			switch ( gStreamLoadState )
			{
				case ( STREAM_LOAD_STATE_IDLE ):
				{
#if SHARE_DMA_CH					
					if ( gLoadStatus & LOAD_STATUS_LOADING_MUSIC )
					{
						gLoadStatus |= LOAD_STATUS_NEED_STREAM;
						return;
					}
					gLoadStatus |= LOAD_STATUS_LOADING_STREAM;
#endif					
					ShowState( "stream preload l\n" );
					SmallShowState( "-SPL" );
					gStreamBufSide = _1st;
					gStreamIopOffset = 0;
					gStreamRemaining = gStreamSize;
					gStreamRemaining -= SB_BUF_SIZE;
					gStreamTransSize = SB_BUF_SIZE;
					if ( gStreamRemaining < 0 )
					{
						gStreamTransSize += gStreamRemaining;
					}
#if DATA_INTEGRITY_CHECK
					{
						int gronk;
						for ( gronk = 0; gronk < gStreamTransSize >> 4; gronk++ )
						{
							if ( *( ( char * )( 1 + gpIopBuf + STREAM_IOP_OFFSET + ( gronk * 16 ) ) ) != 0 )
							{
								printf( "Fucked up data!!!!" );
							}
						}
					}
#endif
					// preload left:
					_AdpcmSetMarkSTARTpre( gpIopBuf + STREAM_IOP_OFFSET, gStreamTransSize );
					if ( gStreamRemaining > 0 )
					{
						_AdpcmSetMarkENDpre( gpIopBuf + STREAM_IOP_OFFSET, gStreamTransSize );
					}
					else
					{
						_AdpcmSetMarkFINAL( gpIopBuf + STREAM_IOP_OFFSET, gStreamTransSize );
					}
					sceSdVoiceTrans( TRANS_DMA_CH_STREAM, ( SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA ),
						(unsigned char *) ( gpIopBuf + STREAM_IOP_OFFSET ),
						STREAM_SPU_BUF_LOC, gStreamTransSize );
					gStreamLoadState++;
					break;
				}
				case ( STREAM_LOAD_STATE_PRELOADING ):
				{
					ShowState( "Stream starting -- waiting for IRQ\n" );
					SmallShowState( " %d-SSIRQ", GetTime( ) );
					gStreamIopOffset = SB_BUF_SIZE;
					if ( gStreamRemaining <= 0 )
					{
						sceSdSetAddr( STREAM_CORE | SD_A_IRQA, STREAM_SPU_BUF_LOC + gStreamTransSize - LAST_VAG_POS );
						//printf( "tiny stream!!! addr %x transsize %x remaining %x", STREAM_SPU_BUF_LOC + gStreamTransSize - LAST_VAG_POS, gStreamTransSize, gStreamRemaining );
						gStreamStatus = EzADPCM_STATUS_TERMINATE;
					}
					else
					{
						sceSdSetAddr( STREAM_CORE | SD_A_IRQA, STREAM_SPU_BUF_LOC + SB_BUF_HALF );
						gStreamStatus = EzADPCM_STATUS_RUNNING;
					}
					AdpcmSetStreamVolumeDirect( gStreamVolume );
					sceSdSetCoreAttr( STREAM_CORE | SD_C_IRQ_ENABLE, 1 );
					sceSdSetSwitch( STREAM_CORE | SD_S_KON, 1 << STREAM_VOICE );
					gStreamLoadState++;
					gTimeOfLastStreamUpdate = GetTime( );
#if SHARE_DMA_CH
					gLoadStatus &= ~LOAD_STATUS_LOADING_STREAM;
					if ( gLoadStatus & LOAD_STATUS_NEED_MUSIC )
					{
						gLoadStatus &= ~LOAD_STATUS_NEED_MUSIC;
						UpdateMusic( );
					}
#endif
					break;
				}
			}
			break;

		case EzADPCM_STATUS_RUNNING:
			switch ( gStreamLoadState )
			{
				case ( STREAM_LOAD_STATE_WAITING_FOR_IRQ ):
				{
					ShowState( "Stream Running -- Loading\n" );
					SmallShowState( "-SRL" );
#if SHARE_DMA_CH					
					if ( gLoadStatus & LOAD_STATUS_LOADING_MUSIC )
					{
						gLoadStatus |= LOAD_STATUS_NEED_STREAM;
						return;
					}
					gLoadStatus |= LOAD_STATUS_LOADING_STREAM;
#endif					
					sceSdSetCoreAttr ( STREAM_CORE | SD_C_IRQ_ENABLE, 0);
					gStreamTransSize = SB_BUF_HALF;
					gStreamRemaining -= SB_BUF_HALF;
					if ( gStreamRemaining < 0 )
					{
						gStreamTransSize += gStreamRemaining;
					}

#if DATA_INTEGRITY_CHECK
					{
						int gronk;
						for ( gronk = 0; gronk < gStreamTransSize >> 4; gronk++ )
						{
							if ( *( ( char * )( 1 + gpIopBuf + gStreamIopOffset + STREAM_IOP_OFFSET + ( gronk * 16 ) ) ) != 0 )
							{
								printf( "Fucked up data!!!!" );
							}
						}
					}
#endif
					
					// load in left side:
					if ( gStreamBufSide == _1st )
					{
						_AdpcmSetMarkSTART( gpIopBuf + STREAM_IOP_OFFSET + gStreamIopOffset, gStreamTransSize );
						
					}
					if ( gStreamRemaining <= 0 )
					{
						_AdpcmSetMarkFINAL( gpIopBuf + STREAM_IOP_OFFSET + gStreamIopOffset, gStreamTransSize );
					}
					else if ( gStreamBufSide == _2nd )
					{
						_AdpcmSetMarkEND( gpIopBuf + STREAM_IOP_OFFSET + gStreamIopOffset, gStreamTransSize );
					}
					sceSdVoiceTrans( TRANS_DMA_CH_STREAM, ( SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA ),
						(unsigned char *) gpIopBuf + STREAM_IOP_OFFSET + gStreamIopOffset,
						( STREAM_SPU_BUF_LOC + SB_BUF_HALF * gStreamBufSide ), gStreamTransSize );
					gStreamLoadState++;
					break;
				}
				case ( STREAM_LOAD_STATE_LOADING ):
				{
					ShowState( "Stream Running -- Waiting for IRQ\n" );
					SmallShowState( " %d-SRIRQ", GetTime( ) );
					// reset the interrupt:
					if ( gStreamRemaining <= 0 )
					{
						sceSdSetAddr( STREAM_CORE | SD_A_IRQA, gStreamTransSize - LAST_VAG_POS + STREAM_SPU_BUF_LOC + SB_BUF_HALF * gStreamBufSide );
					}
					else
					{
						sceSdSetAddr( STREAM_CORE | SD_A_IRQA, STREAM_SPU_BUF_LOC + SB_BUF_HALF * gStreamBufSide );
					}
					gTimeOfLastStreamUpdate = GetTime( );
		
					gStreamIopOffset += SB_BUF_HALF;
					gStreamBufSide = ( gStreamBufSide == _1st ) ? _2nd : _1st;
					
					if ( gStreamRemaining <= 0 )
					{
						// the vag data up to the end should contain all zeros,
						// so it doesn't matter that the interrupt won't happen until the end.
						// stop the stream when it gets to the end:
						gStreamStop++;
					}
					else
					{
						if ( gStreamIopOffset == HALF_BUFFER_SIZE )
						{
							//Dbug_Printf( "Stream move over\n" );
							if ( gStreamRemaining > HALF_BUFFER_SIZE )
							{
								gEECommand |= PCMSTATUS_NEED_STREAM_BUFFER_0;
							}
						}
						else if ( gStreamIopOffset == SINGLE_STREAM_BUFFER_SIZE )
						{
							//Dbug_Printf( "Stream move back\n" );
							gStreamIopOffset = 0;
							if ( gStreamRemaining > HALF_BUFFER_SIZE )
							{
								gEECommand |= PCMSTATUS_NEED_STREAM_BUFFER_1;
							}
						}
					}
					if ( gStreamStop )
					{
						gStreamStop = 0;
						gStreamStatus = EzADPCM_STATUS_TERMINATE;
					}
					sceSdSetCoreAttr( STREAM_CORE | SD_C_IRQ_ENABLE, 1);
					gStreamLoadState = STREAM_LOAD_STATE_WAITING_FOR_IRQ;
//					printf( "stream wait IRQ\n" );
#if SHARE_DMA_CH
					gLoadStatus &= ~LOAD_STATUS_LOADING_STREAM;
					if ( gLoadStatus & LOAD_STATUS_NEED_MUSIC )
					{
						gLoadStatus &= ~LOAD_STATUS_NEED_MUSIC;
						UpdateMusic( );
					}
#endif
					break;
				}
				default:
					printf( "Unknown Stream loading state %d\n", gStreamLoadState );
					break;
			}
			break;

		case EzADPCM_STATUS_TERMINATE: 
			ShowState( "Stream terminate\n" );
			SmallShowState( "-ST!!!" );
			StopStream( );
			break;

		default:
			printf( "unknown stream status in pcm irx!!!\n" );
			break;
	}
}

/* internal */
int _AdpcmPlay( int status )
{
    while ( 1 )
	{
//        int st, mu;
		WaitSema(gSem);
		
//		ShowTime( "ssig us %d um %d ss %d ms %d ee %d time %d\n", gUpdateStream, gUpdateMusic, gStreamStatus, gMusicStatus, gEECommand, GetTime( ) );
//		SmallShowTime( " <%d>", GetTime( ) );
//		st = gUpdateStream;
//		mu = gUpdateMusic;
		while ( gUpdateStream )
		{
//printf( "s u\n" );		
			gUpdateStream--;
			UpdateStream( );
		}
		while ( gUpdateMusic )
		{
//printf( "m u\n" );		
			gUpdateMusic--;
			UpdateMusic( );
		}
		
/*		
		if ( gTestPause == 666 )
		{
			sceSdSetParam( MUSIC_CORE | ( MUSIC_L_VOICE << 1 ) | SD_VP_PITCH, 0 );
			sceSdSetParam( MUSIC_CORE | ( MUSIC_R_VOICE << 1 ) | SD_VP_PITCH, 0 );
			sceSdSetParam( STREAM_CORE | ( STREAM_VOICE << 1 ) | SD_VP_PITCH, 0 );
			gPaused = 1;
		}
		if ( ( !gTestPause ) && ( !st && !mu ) )
		{
			printf( "MOTHERFUCKER MOTHERFUCKER MOTHERFUCKER MOTHERFUCKER\n" );
			gTestPause = 30;
		}*/

//printf( "hmm NAX %x\n", sceSdGetAddr( MUSIC_CORE | ( MUSIC_R_VOICE << 1 ) | SD_VA_NAX ) );
		if ( gMusicVolumeSet )
		{
			gMusicVolumeSet = 0;
			AdpcmSetMusicVolumeDirect( gMusicVolume );
		}
		if ( gStreamVolumeSet )
		{
			gStreamVolumeSet = 0;
			AdpcmSetStreamVolumeDirect( gStreamVolume );
		}
    }
    return 0;
}

static int rpc_arg [16];
volatile int ret = 0;

static void *dispatch( unsigned int command, void *data_, int size )
{ 
    int ch;
    int          data  = *((         int *) data_);
    unsigned int dataU = *((unsigned int *) data_);

/*    PRINTF (("# dispatch [%04x] %x, %x, %x, %x\n",
	      command,
	      *((int*) data_ + 0), 
	      *((int*) data_ + 1),
	      *((int*) data_ + 2),
          *((int*) data_ + 3)));*/

    ch = command & EzADPCM_CH_MASK;
    switch ( command & EzADPCM_COMMAND_MASK )
	{
		case EzADPCM_INIT:
			ret = AdpcmInit( data ); // iop buffer pointer
			break;

		case EzADPCM_QUIT:
			AdpcmQuit( );
			break;

		case EzADPCM_PLAYMUSIC:
			ret = AdpcmPlayMusic( data );  // size of the entire PCM data in the file
			break;

		case EzADPCM_PLAYSTREAM:
			ret = AdpcmPlayStream( data ); // size of the entire PCM data in the file
			break;

		case EzADPCM_PAUSEMUSIC:
			if ( data )
			{
				if ( !gMusicPaused )
				{
					sceSdSetParam( MUSIC_CORE | ( MUSIC_L_VOICE << 1 ) | SD_VP_PITCH, 0 );
					sceSdSetParam( MUSIC_CORE | ( MUSIC_R_VOICE << 1 ) | SD_VP_PITCH, 0 );
					gMusicPaused = GetTime( );
					gMusicTimeOffset = gMusicPaused - gTimeOfLastMusicUpdate;
					if ( gMusicTimeOffset > SINGLE_BUFFER_MS )
					{
/*						if ( gMusicStatus != EzADPCM_STATUS_TERMINATE )
						{
							printf( "\n\nfuck your mom %x \n\n", gMusicStatus );
						}*/
						gMusicTimeOffset -= SINGLE_BUFFER_MS;
					}
					ShowAction( "Pausing music\n" );
					SmallShowAction( "-pm" );
				}
				gRequestFlags &= ~RF_UNPAUSE_MUSIC;
			}
			else
			{
				if ( gMusicPaused )
				{
					gRequestFlags |= RF_UNPAUSE_MUSIC;
					ShowAction( "Requesting unpause music\n" );
					SmallShowAction( "-rupm" );
				}
			}
			break;

		case EzADPCM_PAUSESTREAM:
			if ( data )
			{
				if ( !gStreamPaused )
				{
					sceSdSetParam( STREAM_CORE | ( STREAM_VOICE << 1 ) | SD_VP_PITCH, 0 );
					gStreamPaused = GetTime( );
					gStreamTimeOffset = gStreamPaused - gTimeOfLastStreamUpdate;
					if ( gStreamTimeOffset > SINGLE_BUFFER_MS )
					{
/*						if ( gStreamStatus != EzADPCM_STATUS_TERMINATE )
						{
							printf( "\n\nfuck your mom %x \n\n", gStreamStatus );
						}*/
						gStreamTimeOffset -= SINGLE_BUFFER_MS;
					}
					ShowAction( "Pausing stream\n" );
					SmallShowAction( "-ps" );
				}
				gRequestFlags &= ~( RF_UNPAUSE_STREAM );
			}
			else
			{
				if ( gStreamPaused )
				{
					gRequestFlags |= RF_UNPAUSE_STREAM;
					ShowAction( "Requesting unpause stream\n" );
					SmallShowAction( "-rups" );
				}
			}
			break;
		
		case EzADPCM_STOPMUSIC:
			ret = AdpcmStopMusic( );
			break;

		case EzADPCM_STOPSTREAM:
			ret = AdpcmStopStream( );
			break;

		case EzADPCM_SETMUSICVOL:
			AdpcmSetMusicVolume( dataU );
			break;

		case EzADPCM_SETMUSICVOLDIRECT:
			AdpcmSetMusicVolumeDirect( dataU );
			break;

		case EzADPCM_SETSTREAMVOL:
			AdpcmSetStreamVolume( dataU );
			break;

		case EzADPCM_SETSTREAMVOLDIRECT:
			AdpcmSetStreamVolumeDirect( dataU );
			break;

		case EzADPCM_GETSTATUS:
			ret = AdpcmGetStatus( );
			break;

		case EzADPCM_GETMUSICSTATUS:
			ret = AdpcmGetMusicStatus( );
			break;

		case EzADPCM_GETSTREAMSTATUS:
			ret = AdpcmGetStreamStatus( );
			break;

		case EzADPCM_SDINIT:
			AdpcmSdInit( );
			break;

		default:
			ERROR (("EzADPCM driver error: unknown command %d \n", data));
			break;
    }
    //printf( "! return value = %x \n", ret );
    return (void*)(&ret);
}

int sce_adpcm_loop( void )
{
    sceSifQueueData qd;
    sceSifServeData sd;

    sceSifInitRpc (0);
    sceSifSetRpcQueue (&qd, GetThreadId ());
    sceSifRegisterRpc (&sd, EzADPCM_DEV, dispatch, (void*)rpc_arg, NULL, NULL, &qd);
    //Dbug_Printf(("goto adpcm cmd loop\n"));
    
    sceSifRpcLoop (&qd);

    return 0;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */
