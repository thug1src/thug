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

#include <xtl.h>
#include <time.h>
#include <sys/timer.h>
#include <sys/profiler.h>
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

static volatile uint64	s_count;

#define	vSMOOTH_N  4	

static volatile uint64	s_vblank = 0;
static volatile uint64	s_total_vblanks = 0;
static float			s_slomo = 1.0f;
static uint64			s_render_frame = 0;	
static uint64			s_stored_vblank = 0;
static volatile bool	s_vsync_handler_installed = false;
static volatile bool	s_swap_handler_installed = false;

static clock_t			high_count;
static clock_t			start_count;
static __int64			high_count_high_precision;
static __int64			start_count_high_precision;

static uint64 GetGameVblanks();

struct STimerInfo
{
	float	render_length;
	double	uncapped_render_length;
	int		render_buffer[vSMOOTH_N];
	uint64	render_vblank;
	uint64	render_last_vblank;	
	int		render_index;
};

static STimerInfo gTimerInfo;
static STimerInfo gStoredTimerInfo;
static float xrate = 60.0f;

static void InitTimerInfo( void )
{
	static bool xrate_set = false;

	if( !xrate_set )
	{
		xrate		= (float)Config::FPS();
		xrate_set	= true;
	}

	gTimerInfo.render_length			= 0.01666666f;		// defualt to 1/60th
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

//#ifdef __NOPT_PROFILE__

//Sys::ProfileData	DMAProfileData;
//Sys::ProfileData	VU1ProfileData;

//#endif // __NOPT_PROFILE__

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
__int64 __declspec( naked ) __stdcall getCycles( void )
{
	_asm
	{
		rdtsc
		ret
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static void __cdecl s_vsync_handler( D3DVBLANKDATA *pData )
{
	// inc vblanks    
	s_vblank++;
	s_total_vblanks++;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static void __cdecl s_swap_callback_handler( D3DSWAPDATA *pData )
{
	// This callback function will be called when a Swap or Present is encountered by the GPU, in order to provide information as to when
	// Swap or Present calls are actually handled by the GPU. Swap and Present are like all other rendering functions in that they are enqueued
	// in the push-buffer and so the handling of the actual command is not synchronous with the actual function call.
	// Even if the Swap or Present is called by the title before a vertical blank, it may not be handled by the GPU until after the vertical blank
	// because the GPU still has to process all of the commands in the push-buffer that are before the Swap or Present.
	++s_render_frame;
	
	int diff;
	gTimerInfo.render_last_vblank	= gTimerInfo.render_vblank;
	gTimerInfo.render_vblank		= pData->SwapVBlank;
	diff = (int)( gTimerInfo.render_vblank - gTimerInfo.render_last_vblank );

	gTimerInfo.render_buffer[gTimerInfo.render_index++]	= diff;

	// Wrap the index around.	
	if( gTimerInfo.render_index == vSMOOTH_N )
	{
		gTimerInfo.render_index = 0;		
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void Init( void )
{
	InitTimerInfo();
	
	s_count						= 0;
	start_count					= clock();
	high_count					= (clock_t)0;
	start_count_high_precision	= getCycles();
	high_count_high_precision	= (__int64)0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void DeInit( void )
{
}



/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void InstallVSyncHandlers( void )
{
	if( !s_vsync_handler_installed )
	{
		// Install the vertical blank callback handler.
		D3DDevice_SetVerticalBlankCallback( s_vsync_handler );
		s_vsync_handler_installed = true;
	}

	if( !s_swap_handler_installed )
	{
		// Install the swap callback handler at this point.
		D3DDevice_SetSwapCallback( s_swap_callback_handler );
		s_swap_handler_installed = true;
	}
}



// when pausing the game, call this to store the current state of OncePerRender( ) (only essential in replay mode)
void StoreTimerInfo( void )
{
}



void RecallTimerInfo( void )
{
}



// Call this function once per rendering loop, to increment the 
// m_render_frame variable
// This function should be synchronized in some way to the vblank, so that it is called 
// directly after the vblank that rendering starts on
void OncePerRender( void )
{
#	ifdef STOPWATCH_STUFF
	Script::IncrementStopwatchFrameIndices();
	Script::DumpFunctionTimes();
	Script::ResetFunctionCounts();	
#	endif

	int total			= 0;
	int uncapped_total	= 0;

	for( int i = 0; i < vSMOOTH_N; ++i )
	{
		int diff = gTimerInfo.render_buffer[i];
		uncapped_total += diff;

		// Handle very bad values.
		if( diff > 10 || diff < 0 )		
		{
			diff = 1;
		}

		// Clamp to 4.
		if( diff > 4 )
		{
			diff = 4;
		}		
		total += diff;
	}

	gTimerInfo.render_length = (float)total / (float)vSMOOTH_N;

	if( gTimerInfo.render_length < 1.0f )
	{
		gTimerInfo.render_length = 1.0f;
	}

	gTimerInfo.uncapped_render_length = (double)uncapped_total/(double)vSMOOTH_N; 
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint64 GetRenderFrame( void )
{
	return s_render_frame;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void Vblank( void )
{
	s_total_vblanks++;
	s_vblank++;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static uint64 GetGameVblanks( void )
{
	return s_vblank;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint64 GetVblanks( void )
{
	return s_total_vblanks;
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
float FrameLength( void )
{
	return gTimerInfo.render_length / xrate * GetSlomo();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
float GetSlomo( void )
{
	return s_slomo;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SetSlomo( float slomo )
{
	s_slomo = slomo;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
double UncappedFrameLength()
{
	return gTimerInfo.uncapped_render_length / xrate;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void VSync( void )
{
	uint64 now = GetVblanks();
	while( now == GetVblanks());	
}
 


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
MicroSeconds GetTimeInUSeconds( void )
{
	high_count_high_precision = ( getCycles() - start_count_high_precision ) / 733;
	return static_cast<Time>( high_count_high_precision );
}



// Added by Ken, to get higher resolution when timing Script::Update, which needs to
// subtract out the time spent in calling C-functions to get an accurate measurement
// of how long the actual script interpretation is taking.
CPUCycles 	GetTimeInCPUCycles ( void )
{
	uint64	high0, low0, high1, low1;

    high0 = s_count;
	low0  = 0;
    high1 = s_count;
    low1  = 0;

    if( high0 == high1 )
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
	InitTimerInfo();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Time GetTime( void )
{
	// E3 fix by Ken: Changed to do it this way because otherwise the multiply by 1000 overflows after a day or so.
	// By using 50/3 instead, and using a float, means it'll overflow after about 50 days instead.
	// The accuracy will slowly degrade over time. Stays accurate to within a few milliseconds after a week though.
	// Note: The casting of s_vblank (which is a uint64) to a uint32 before casting to a float is important. If the
	// uint64 is cast straight to a float it will do tons of floating point stuff which take ages.
	return (uint32) (((float)(uint32)s_vblank)*(1000.0f / (float)xrate));
}


} // namespace Tmr

