/********************************************************************************
 *																				*
 *	Module:																		*
 *				NsProfile															*
 *	Description:																*
 *				Code used to profile how much time something takes.				*
 *	Written by:																	*
 *				Paul Robinson													*
 *	Copyright:																	*
 *				2001 Neversoft Entertainment - All rights reserved.				*
 *																				*
 ********************************************************************************/

/********************************************************************************
 * Includes.																	*
 ********************************************************************************/
#include <math.h>
#include "p_profile.h"
#include "p_assert.h"

/********************************************************************************
 * Defines.																		*
 ********************************************************************************/

/********************************************************************************
 * Structures.																	*
 ********************************************************************************/

/********************************************************************************
 * Local variables.																*
 ********************************************************************************/
static float	last_x0, last_y0, last_x1, last_y1;
static float	last_bar[256];


/********************************************************************************
 * Forward references.															*
 ********************************************************************************/

/********************************************************************************
 * Externs.																		*
 ********************************************************************************/

/********************************************************************************
 *																				*
 *	Method:																		*
 *				NsProfile															*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Initializes a profile object.									*
 *																				*
 ********************************************************************************/
NsProfile::NsProfile ( char * pName )
{
	OSInitStopwatch ( &m_watch, pName );
	m_accumulated = 0;
	m_pHistoryBuffer = NULL;
}

NsProfile::NsProfile()
{
	NsProfile ( "<no name given>" );
}

NsProfile::NsProfile ( char * pName, unsigned int historySize )
{
	unsigned int lp;

	OSInitStopwatch ( &m_watch, pName );
	m_accumulated = 0;

	m_pHistoryBuffer = new float[historySize];
	for ( lp = 0; lp < historySize; lp++ ) {
		m_pHistoryBuffer[lp] = 0.0f;
	}
	m_historyBufferSize = historySize;
	m_historyEntry = 0;
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				start															*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Start recording how much time something takes.					*
 *																				*
 ********************************************************************************/
void NsProfile::start ( void )
{
	OSStartStopwatch ( &m_watch );
	m_latch = m_watch.total;
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				stop															*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Stop recording how much time something takes.					*
 *																				*
 ********************************************************************************/
void NsProfile::stop ( void )
{
	OSStopStopwatch ( &m_watch );
	m_accumulated += m_watch.total - m_latch;
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				draw															*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Draw a bar representing how much time something takes.			*
 *																				*
 ********************************************************************************/
void NsProfile::draw ( float x0, float y0, float x1, float y1, GXColor color )
{
	float	ms;
	float	bar;

	ms = (float)OSTicksToMilliseconds( ((float)m_accumulated) );   

	bar = ms * ( x1 - x0 ) / ( 1000.0f / 60.0f );

	NsPrim::box ( x0-1.0f, y0-1.0f, x1 + 1.0f, y1 + 1.0f, (GXColor){255,255,255,128} );
	NsPrim::box ( x0, y0, x1, y1, (GXColor){0,0,0,128} );
	NsPrim::box ( x0, y0, x0 + bar, y1, color );

	// Reset the stopwatch.
	m_accumulated = 0;
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				histogram														*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Draw a histogram chart representing how much time something		*
 *				takes.															*
 *																				*
 ********************************************************************************/
void NsProfile::histogram ( float x0, float y0, float x1, float y1, GXColor color )
{
	float			ms;
	float			bar;
	float			xpos;
	unsigned int	lp;
	int				he;

	// Quick reject test.
	assertf ( m_pHistoryBuffer, ( "No history buffer set up." ) );

	// Update history buffer.
	ms = (float)OSTicksToMilliseconds( ((float)m_accumulated) );   
//	if ( ms > ( 1000.0f / 60.0f ) ) ms = ( 1000.0f / 60.0f );
	m_pHistoryBuffer[m_historyEntry] = ms;
	m_historyEntry++;
	m_historyEntry %= m_historyBufferSize;

	// Draw background.
	NsPrim::box ( x0-1.0f, y0-1.0f, x1 + 1.0f, y1 + 1.0f, (GXColor){255,255,255,128} );
	NsPrim::box ( x0, y0, x1, y1, (GXColor){0,0,0,128} );

	// Draw bars.
	he = m_historyEntry;
	for ( lp = 0; lp < m_historyBufferSize; lp++ ) {
		ms = m_pHistoryBuffer[he];
		bar = ms * ( y1 - y0 ) / ( 1000.0f / 60.0f );
		last_bar[lp] = bar;
		xpos = x0 + ( ( (float)lp * ( x1 - x0 ) ) / (float)m_historyBufferSize );
		NsPrim::line ( xpos, y1, xpos, y1 - bar, color );
		he++;
		he %= m_historyBufferSize;
	}

	// Reset the stopwatch.
	m_accumulated = 0;

	// Save for appends.
	last_x0 = x0;
	last_x1 = x1;
	last_y0 = y0;
	last_y1 = y1;
}

void NsProfile::append ( GXColor color, bool update )
{
	float			ms;
	float			bar;
	float			xpos;
	unsigned int	lp;
	int				he;
	float			x0 = last_x0;
	float			x1 = last_x1;
	float			y0 = last_y0;
	float			y1 = last_y1;

	// Quick reject test.
	assertf ( m_pHistoryBuffer, ( "No history buffer set up." ) );

	// Update history buffer.
	ms = (float)OSTicksToMilliseconds( ((float)m_accumulated) );   
//	if ( ms > ( 1000.0f / 60.0f ) ) ms = ( 1000.0f / 60.0f );
	m_pHistoryBuffer[m_historyEntry] = ms;
	m_historyEntry++;
	m_historyEntry %= m_historyBufferSize;

	// Draw bars.
	he = m_historyEntry;
	for ( lp = 0; lp < m_historyBufferSize; lp++ ) {
		ms = m_pHistoryBuffer[he];
		bar = ms * ( y1 - y0 ) / ( 1000.0f / 60.0f );
		xpos = x0 + ( ( (float)lp * ( x1 - x0 ) ) / (float)m_historyBufferSize );
		NsPrim::line ( xpos, y1 - ( bar + last_bar[lp] ), xpos, y1 - last_bar[lp], color );
		he++;
		he %= m_historyBufferSize;
		if ( update  ) last_bar[lp] += bar;
	}

	// Reset the stopwatch.
	m_accumulated = 0;
}




//`	float	time;
//static OSStopwatch  SwMsec;         // stopwatch for single frame render time
//static volatile f32 MSecGX  = 0.0f; // volatile since it is during drawdone callback
//    OSInitStopwatch( &SwMsec, "render" );    // time to render single frame
//        OSStartStopwatch( &SwMsec ); // SwMsec measure single frame render time for CPU and GP;
//	    swTime  = OSCheckStopwatch( &SwMsec );
//	    MSecCPU2 = (f32)OSTicksToMilliseconds( ((f32)swTime) );
//        OSStopStopwatch(  &SwMsec ); // reset SwMsec for next frame
//        OSResetStopwatch( &SwMsec );
//    OSTime        swTime;             // current stopwatch time in 'OSTime' format. 
//	    swTime  = OSCheckStopwatch( &SwMsec );
//	    MSecCPU = (f32)OSTicksToMilliseconds( ((f32)swTime) );   
//typedef struct OSStopwatch
//{
//    char*       name;       // name of this stopwatch
//    OSTime      total;      // total time running
//    u32         hits;       // number of times turned on and off
//    OSTime      min;        // smallest time measured
//    OSTime      max;        // longest time measured
//
//    OSTime      last;       // time at which this sw was last started
//    BOOL        running;    // TRUE if sw is running
//} OSStopwatch;
//
//
//void    OSInitStopwatch     ( OSStopwatch* sw, char* name );
//void    OSStartStopwatch    ( OSStopwatch* sw );
//void    OSStopStopwatch     ( OSStopwatch* sw );
//OSTime  OSCheckStopwatch    ( OSStopwatch* sw );
//void    OSResetStopwatch    ( OSStopwatch* sw );
//void    OSDumpStopwatch     ( OSStopwatch* sw );











