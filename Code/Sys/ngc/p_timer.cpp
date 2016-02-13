/*****************************************************************************
**																			**
**			              Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		SYS Library												**
**																			**
**	Module:			Timer (TMR)			 									**
**																			**
**	File name:		ngps/p_timer.cpp										**
**																			**
**	Created by:		01/13/00	-	mjb										**
**																			**
**	Description:	NGPS System Timer										**
**																			**
*****************************************************************************/



/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <dolphin.h>
#include <sys/timer.h>
#include <sys/profiler.h>
#include <gel/scripting/script.h>
#include <sys/config/config.h>

/*****************************************************************************
**							  DBG Information								**
*****************************************************************************/

namespace Tmr 
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define vHIREZ_CMP 15000

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

static 			sint 	s_timer_handler_id = -1;
static volatile uint64	s_count;

#define	vSMOOTH_N  4	

static volatile uint64  s_vblank = 0;
static volatile uint64 s_total_vblanks = 0;
static float s_slomo = 1.0f;
static  uint64	s_render_frame = 0;	
static uint64 s_stored_vblank = 0;

static uint64 GetGameVblanks();

struct STimerInfo
{
	float	render_length;
	float	frame_length;
	double	uncapped_render_length;
	int		render_buffer[vSMOOTH_N];
	uint64	render_vblank;
	uint64	render_last_vblank;	
	int		render_index;
};

static STimerInfo gTimerInfo;
static STimerInfo gStoredTimerInfo;

static void InitTimerInfo( void )
{
	gTimerInfo.render_length			= 0.01666666f;		// defualt to 1/60th
	gTimerInfo.frame_length 			= 1.0f/60.0f;		// default to 1/60th, will be correct after first frame is rendered
	gTimerInfo.uncapped_render_length	= 0.01666666f;		// defualt to 1/60th
	for( int i = 0; i < vSMOOTH_N; i++ )
	{
		gTimerInfo.render_buffer[ i ] = 1;
	}
	gTimerInfo.render_vblank			= 0;	
	gTimerInfo.render_last_vblank		= 0;
	gTimerInfo.render_index				= 0;

	gStoredTimerInfo					= gTimerInfo;

	s_stored_vblank						= 0;
	s_vblank							= 0;
}

Time  							s_milliseconds = 0;			// updated every call to VSync() or VSync1()

#ifdef __NOPT_PROFILE__

//Sys::ProfileData	DMAProfileData;
//Sys::ProfileData	VU1ProfileData;

#endif // __NOPT_PROFILE__

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

//static sint timer_handler ( sint ca )
//{
//
//#ifdef __NOPT_PROFILE__
//    if (( ca == INTC_TIM0 ) && ( *T0_MODE & 0x400 ))  // compare interrupt
//    {
//        *T0_COUNT	= 0;		// reset s_count
//        *T0_MODE	|= 0x400; 	// clear compare flag
//		s_count++;
//#if 0
//		Sys::ProfileData	*prof;
//		 
//		prof = &DMAProfileData;
//		
//		prof->read_state = (( (*D1_CHCR) & D_CHCR_STR_M ) >> 8 ) & 0x000000ff;
//		
//		if( prof->current_state[prof->flip] != prof->read_state )
//		{
//			prof->time[prof->flip][prof->current_offset[prof->flip]] = s_count*(vHIREZ_CMP/150); 
//			prof->current_state[prof->flip] = prof->read_state;
//			prof->state[prof->flip][prof->current_offset[prof->flip]++] = prof->read_state;
//		}
//	
//		prof = &VU1ProfileData;
//		
//		uint32 vpustat;
//		asm volatile (" cfc2 %0, $29 " : "=r" (vpustat));
//		prof->read_state = (vpustat >> 8) & 0x000000ff;
//	
//		if( prof->current_state[prof->flip] != prof->read_state )
//		{
//			prof->time[prof->flip][prof->current_offset[prof->flip]] = s_count*(vHIREZ_CMP/150); 
//			prof->current_state[prof->flip] = prof->read_state;
//			prof->state[prof->flip][prof->current_offset[prof->flip]++] = prof->read_state;
//		}
//#endif
//    }
//#else
//    if (( ca == INTC_TIM0 ) && ( *T0_MODE & 0x800 ))  // overflow interrupt
//    {
//        *T0_MODE 	 |= 0x800; 	// clear overflow
//        s_count += 0x10000;
//    }
//#endif
//
//
//    ExitHandler();			// Mick: Sony requires interrupts to end with an ExitHandler() call
//							// this enables interrupts
//    return 0;
//}
//
static 	sint 	s_vsync_handler_id = -1;

// This is called from the NGC post vblank callback, installed in p_display.cpp.
void IncrementVblankCounters( void )
{
	// inc vblanks    
	s_vblank++;
	s_total_vblanks++;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Init( void )
{
	InitTimerInfo();
	
//	s_timer_handler_id = AddIntcHandler ( INTC_TIM0, timer_handler, 0 );
	s_timer_handler_id = 0;
	Dbg_MsgAssert (( s_timer_handler_id >= 0 ),( "Failed to add timer handler" )); 

	// The vsync callback is actually set up in p_display.cpp.
//	s_vsync_handler_id = AddIntcHandler ( INTC_VBLANK_S, s_vsync_handler, 0 );
	s_vsync_handler_id = 1;
	Dbg_MsgAssert (( s_vsync_handler_id >= 0 ),( "Failed to add vsync handler" )); 
		
	s_count	= 0;
//	*T0_COUNT 	= 0;
//	*T0_COMP 	= vHIREZ_CMP;	// used for hi-res counter (micro-second interrupt)
//	*T0_HOLD 	= 0;
    
#	ifdef __NOPT_PROFILE__
	s_count = 0;
//    *T0_MODE 	= 0xc80; // clear overflow and equal flag
//    *T0_MODE 	= 0x180; // compare int enabled, start clock, speed 150MHz
#	else
//    *T0_MODE 	= 0xc81; // clear overflow and equal flag
//    *T0_MODE 	= 0x281; // overflow int enabled, start clock, speed 150Mhz/16
#	endif

//	EnableIntc ( INTC_TIM0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void DeInit(void)
{
	Dbg_MsgAssert (( s_timer_handler_id >= 0 ),( "Invalid Timer Handler (%d)", s_timer_handler_id )); 
	
//	DisableIntc ( INTC_TIM0 );
//    RemoveIntcHandler ( INTC_TIM0, s_timer_handler_id );
//
//	Dbg_MsgAssert (( s_vsync_handler_id >= 0 ),( "Invalid VSync Handler (%d)", s_vsync_handler_id )); 
//    RemoveIntcHandler ( INTC_VBLANK_S, s_vsync_handler_id );

	s_timer_handler_id = -1;
	s_vsync_handler_id = -1;
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

// when pausing the game, call this to store the current state of OncePerRender( ) (only essential in replay mode)
void StoreTimerInfo( void )
{
	//if ( Replay::GetReplayMode( ) == Replay::REPLAY_MODE_OFF )
	//{
	//	return;
	//}
	//gStoredTimerInfo = gTimerInfo;
	//s_stored_vblank = s_vblank;
}



void RecallTimerInfo( void )
{
	/*if ( Replay::GetReplayMode( ) == Replay::REPLAY_MODE_OFF )
	{
		return;
	}
	gTimerInfo = gStoredTimerInfo;
	s_vblank = s_stored_vblank;
	if ( Replay::GetReplayMode( ) & ( Replay::REPLAY_MODE_PLAY_SAVED | Replay::REPLAY_MODE_PLAY_AFTER_RUN ) )
	{
		s_vblank += Replay::GetLastFrameLength();
	}*/
}

// Call this function once per rendering loop, to increment the 
// m_render_frame variable
// This function should be synchronized in some way to the vblank, so that it is called 
// directly after the vblank that rendering starts on
void OncePerRender()
{
//	Vblank();

	s_render_frame++;
	
	#ifdef STOPWATCH_STUFF
	Script::IncrementStopwatchFrameIndices();
	Script::DumpFunctionTimes();
	Script::ResetFunctionCounts();	
	#endif
	
	int diff;
	gTimerInfo.render_last_vblank = gTimerInfo.render_vblank;
	
	/*
	Replay::ReplayMode replayMode;
	replayMode = Replay::GetReplayMode( );
	if ( replayMode & ( Replay::REPLAY_MODE_PLAY_SAVED | Replay::REPLAY_MODE_PLAY_AFTER_RUN ) )
	{
		diff = Replay::GetFrameLength( );
		gTimerInfo.render_vblank += diff;
	}
	else
	{
		gTimerInfo.render_vblank = GetGameVblanks();
		diff = (int)(gTimerInfo.render_vblank - gTimerInfo.render_last_vblank);
		if ( Replay::REPLAY_MODE_RECORD == replayMode )
		{
			if ( Replay::FixSmoothing( ) ) // the 'smoothing' feature will fuck up our logic if,
											// during replay, there is a framerate difference
											// between the initial frames of play/record before
											// getting our data synced up.
			{
				diff = 1;  // keep it at 60 hz before synchronization.
			}
			Replay::StoreFrameLength( diff );
		}
	}
	*/
	gTimerInfo.render_vblank = GetGameVblanks();
	diff = (int)(gTimerInfo.render_vblank - gTimerInfo.render_last_vblank);

	
	gTimerInfo.render_buffer[ gTimerInfo.render_index++] = diff;
	if ( gTimerInfo.render_index == vSMOOTH_N)
	{
		gTimerInfo.render_index = 0;		
	}
	int total = 0;
	int uncapped_total = 0;
	for (int i=0;i<vSMOOTH_N;i++)
	{
		
		diff = gTimerInfo.render_buffer[i];
		uncapped_total += diff;
		if (diff > 10 || diff <= 1)		// handle very bad values
		{
			diff = 1;
		}
		if (diff >4) 					// limit to 4
		{
			diff = 4;
		}		
		total += diff;
	}
	gTimerInfo.render_length = (float)total/(float)vSMOOTH_N; 
	gTimerInfo.uncapped_render_length = (double)uncapped_total/(double)vSMOOTH_N; 
	gTimerInfo.frame_length = gTimerInfo.render_length/(float)Config::FPS()*GetSlomo();
	
	
//	printf ("%d:  %d, %f\n",(int)GetRenderFrame(),diff, gTimerInfo.render_length);
	
}



uint64 GetRenderFrame()
{
	return s_render_frame;
}


void Vblank()
{
	s_total_vblanks++;
	s_vblank++;
}

static uint64 GetGameVblanks()
{
	return s_vblank;
}

uint64 GetVblanks( void )
{
	return ( s_total_vblanks );
}

// call "StartLogic" when all the logic functions are going to be called  
//void Manager::StartLogic()
//{
//}

// returns the PROJECTED length of the next frame, for use in logic calculations
// done by averaging the frame lengths of the last N frames (default N = 4)
// at 60fps (NTSC), this would return 1/60
// at 50fps (PAL), this would return 1/50		   
// complications arise when the framerate changes
// we want the physics to move smoothly and consistently at all framerates
// however, all we know is the number of ticks the last frames took
// we also need to ensure that the total of all the "FrameLength" calls will
// reflect the actual time that has passed
//				 |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |   |    |   
// say we have   1    1    1    .    2    1    1    1    .    2    1    .    2    1    1    1   .    2 
//    (10x1 + 4x2  = 18 frames
//
//
// now, let's work in 1/60ths of a second for now....
// let's take the average of the last 4 frames, as the logic time we will return for this frame
//
//               1    1    1         1.25 1.25 1.25 1.25     1.25 1.25      1.5  1.5  1.25 1.25     1.25
//
// the reason we do the averaging is to smooth out discontinuities
// if we simply use the length of the previous frame, then if we were to be alternating between 1 and 2 frames
// then the logic would move 2 on frames of length 1, and move 1 on frames of length 2
// this would essentially make the skater visually move alternately .5x and 2x his actual speed
// this looks incredibly jerky, as there is a 4x difference per frame   
// by smoothing it out, you would move 1.5 on every frame, giving relative motion of 0.6666 and 1.3333, 
// this is still jerky, but half as much as the unsmoothed version.
 
float FrameLength()
{

	return gTimerInfo.frame_length;

//	if ( VIGetTvFormat() == VI_PAL ) {
//		return gTimerInfo.render_length / 50.0f * GetSlomo();
//	} else {
//		return gTimerInfo.render_length / 60.0f * GetSlomo();
//	}
}


float GetSlomo()
{
	return s_slomo;
}

void SetSlomo(float slomo)
{
	s_slomo = slomo;
}

double UncappedFrameLength()
{
	return gTimerInfo.uncapped_render_length/(float)Config::FPS();


//	if ( VIGetTvFormat() == VI_PAL ) {
//		return gTimerInfo.uncapped_render_length / 50.0f;
//	} else {
//		return gTimerInfo.uncapped_render_length / 60.0f;
//	}
}



uint64 now;

void VSync()
{
	now = GetVblanks();
	while (now == GetVblanks());	
}
 

// wait until VSync changes from what it was
// (Might return immediately, if it's already changed)
void VSync1()
{
	while (now == GetVblanks());	
	now = GetVblanks();
	s_milliseconds = (Time) (((float)(uint32)s_vblank)*(1000.0f / (float)Config::FPS()));
}

//#ifdef __NOPT_PROFILE__

MicroSeconds 	GetTimeInUSeconds ( void )
{
//	uint64	high0, low0, high1, low1;
//
//    high0 = s_count;
//    low0  = 0;	//*T0_COUNT;
//    high1 = s_count;
//    low1  = 0;	//*T0_COUNT;
//
//
//    if ( high0 == high1 )
//    {
//		return ((high0 * vHIREZ_CMP) + low0)/150;
//    }
//    else
//    {
//		return ((high1 * vHIREZ_CMP) + low1)/150;
//    }
//

	OSTime time = OSGetTime();
	return OSTicksToMicroseconds(time);
}

// Added by Ken, to get higher resolution when timing Script::Update, which needs to
// subtract out the time spent in calling C-functions to get an accurate measurement
// of how long the actual script interpretation is taking.
CPUCycles 	GetTimeInCPUCycles ( void )
{
	uint64	high0, low0, high1, low1;

    high0 = s_count;
    low0  = 0;	//*T0_COUNT;
    high1 = s_count;
    low1  = 0;	//*T0_COUNT;


    if ( high0 == high1 )
    {
		return ((high0 * vHIREZ_CMP) + low0);
    }
    else
    {
		return ((high1 * vHIREZ_CMP) + low1);
    }

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void RestartClock( void )
{
	InitTimerInfo( );
}


Time	GetTime ( void )
{
	//return ( GetTimeInUSeconds() / 1000 );
	//return s_count;
	// A less accurate time, but much faster than doing a 64 bit divide.
	//return ((int)s_vblank)*1000/60;
	
	// E3 fix by Ken: Changed to do it this way because otherwise the multiply by 1000 overflows after a day or so.
	// By using 50/3 instead, and using a float, means it'll overflow after about 50 days instead.
	// The accuracy will slowly degrade over time. Stays accurate to within a few milliseconds after a week though.
	// Note: The casting of s_vblank (which is a uint64) to a uint32 before casting to a float is important. If the
	// uint64 is cast straight to a float it will do tons of floating point stuff which take ages.
	return (int) (((float)(uint32)s_vblank)*(1000.0f / (float)Config::FPS()));
	/*
	if ( Replay::UseNormalClock( ) )
	{
		return (uint32) (((float)(uint32)s_vblank)*50.0f/3.0f);
	}
	else
	{
		return (uint32) (((float)(uint32)gTimerInfo.render_vblank )*50.0f/3.0f);
	}*/
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
//#else // __NOPT_PROFILE__
//
//#ifdef __USER_MATT__
//#error Wrong Motherfuckin Timer
//#endif
//
//Time	Manager::GetTime ( void )
//{
//	uint64	high0, low0, high1, low1;
//
//    high0 = s_count;
//    low0  = *T0_COUNT;
//    high1 = s_count;
//    low1  = *T0_COUNT;
//
//    if ( high0 == high1 )
//    {
//		return static_cast<uint64>(( high0 | ( low0 & 0xffff )) / 9375 );
//    }
//    else
//    {
//		return static_cast<uint64>(( high1 | ( low1 & 0xffff )) / 9375 );
//    }
//}
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
// 
//Time	GetTimeInUSeconds ( void )	   // remove ?
//{
//	uint64	high0, low0, high1, low1;
//
//    high0 = s_count;
//    low0  = *T0_COUNT;
//    high1 = s_count;
//    low1  = *T0_COUNT;
//
//    if ( high0 == high1 )
//    {
//		return static_cast<uint64>(( high0 + ( low0 & 0xffff )) / 9 );
//    }
//    else
//    {
//		return static_cast<uint64>(( high1 + ( low1 & 0xffff )) / 9 );
//    }
//}
//#endif // __NOPT_PROFILE__
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Tmr
