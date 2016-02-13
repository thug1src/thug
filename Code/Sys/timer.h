/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		SYS Library		   		 								**
**																			**
**	Module:			Timer (TMR)												**
**																			**
**	Created:		01/13/00	-	mjb										**
**																			**
**	File name:		timer.h													**
**																			**
*****************************************************************************/

#ifndef __SYS_TIMER_H
#define __SYS_TIMER_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifdef __PLAT_NGPS__
#include <eeregs.h> // Needed for T0_COUNT
#endif

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif


/*****************************************************************************
**								   Defines									**
*****************************************************************************/

// Ken: This declaration is to avoid having to include config.h, to reduce dependencies.
namespace Config
{
extern int gFPS;
}

namespace Tmr
{

enum 
{
	vRESOLUTION		= 1000,		// timer resolution in milliseconds
	vMAX_TIME		= vUINT_MAX,
};

/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/

#define vHIREZ_CMP 15000			// 15000

typedef uint32	Time;					// Milliseconds (so wraps after 49 days)

#ifdef __PLAT_NGPS__
typedef uint32	MicroSeconds;			// MicroSeconds
typedef uint32	CPUCycles;
#else
typedef uint64	CPUCycles;				// CPUCycles, also needs higher resolution
typedef uint64	MicroSeconds;			// MicroSeconds
#endif

Time					GetTime( void );  // GetTimeInMSeconds

#ifdef __PLAT_NGPS__
extern volatile uint32 gCPUTimeCount;
inline CPUCycles GetTimeInCPUCycles ( void ) {return gCPUTimeCount* vHIREZ_CMP + *T0_COUNT;}
inline MicroSeconds GetTimeInUSeconds( void ) {return GetTimeInCPUCycles() / 150; }
#else
CPUCycles				GetTimeInCPUCycles( void );
MicroSeconds			GetTimeInUSeconds( void );
#endif

void	OncePerRender();
uint64	GetRenderFrame();
void 	Vblank();

// total vblanks from bootup... be careful not to use this for anything that would affect replay!
uint64 	GetVblanks();

float 	FrameLength();
double 	UncappedFrameLength();
void 	VSync();
void 	VSync1();
float 	GetSlomo();
void 	SetSlomo(float slomo);
void	RestartClock( void );

void	StoreTimerInfo( void );
void	RecallTimerInfo( void );

void	Init(void);
void	DeInit(void);

#ifdef __PLAT_NGC__
void	IncrementVblankCounters( void );
#endif

#ifdef __PLAT_XBOX__
void	InstallVSyncHandlers( void );
#endif

#ifdef	__PLAT_NGPS__
extern Time  s_milliseconds;
inline Time	GetQuickTime ( void ) {return s_milliseconds;}
#endif


/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/


/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/


/*************************************************************************
**								Macros									**
**************************************************************************/

inline 	Time	ElapsedTime( Time time )
{
	return ( GetTime() - ( time ));
}

inline 	float	FrameRatio()
{								
	return ( FrameLength() * 60.0f);	  	// note this is 60 regardless of NTSC/PAL
													// as we want it to return 1.2 for pal
}

inline 	Time	InVBlanks( Time time )
{
	return ( time / ( vRESOLUTION / Config::gFPS ));
}

inline 	Time	InMSeconds( Time time )
{
	return ( time / ( vRESOLUTION / 1000 ));
}

inline 	Time	InHSeconds( Time time )
{
	return ( time / ( vRESOLUTION / 100 ));
}

inline 	Time	InTSeconds( Time time )
{
	return ( time / ( vRESOLUTION / 10 ));
}

inline 	Time	InSeconds( Time time )
{
	return ( time /   vRESOLUTION );
}

inline 	Time	InMinutes( Time time )
{
	return ( time / ( vRESOLUTION * 60 ));
}

inline 	Time	InHours( Time time )
{
	return ( time / ( vRESOLUTION * 60 * 60 ));
}

inline	Time	VBlanks( int time )
{
	return ( time * ( vRESOLUTION / Config::gFPS ));
}

inline	Time	VBlanksF( float time )
{
	return (Time) ( time * ((float) vRESOLUTION / (float) Config::gFPS ));
}

inline	Time	MSeconds( Time time )
{
	return ( time * ( vRESOLUTION / 1000 ));
}

inline	Time	HSeconds( Time time )
{
	return ( time * ( vRESOLUTION / 100 ));
}

inline	Time	TSeconds( Time time )
{
	return ( time * ( vRESOLUTION / 10 ));
}

inline	Time	Seconds( Time time )
{
	return ( time * vRESOLUTION );
}

inline	Time	Minutes( Time time )
{
	return ( time * ( vRESOLUTION * 60 ));
}

inline	Time	Hours( Time time )
{
	return ( time * ( vRESOLUTION * 60 * 60 ));
}


} // namespace Tmr

#endif	// __SYS_TIMER_H
