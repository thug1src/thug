/********************************************************************************
 *																				*
 *	Module:																		*
 *				NsBuffer														*
 *	Description:																*
 *				Provides a cyclic buffer for rendering purposes.				*
 *	Written by:																	*
 *				Paul Robinson													*
 *	Copyright:																	*
 *				2002 Neversoft Entertainment - All rights reserved.				*
 *																				*
 ********************************************************************************/

/********************************************************************************
 * Includes.																	*
 ********************************************************************************/
#include "p_buffer.h"

/********************************************************************************
 * Defines.																		*
 ********************************************************************************/
 
/********************************************************************************
 * Structures.																	*
 ********************************************************************************/

/********************************************************************************
 * Local variables.																*
 ********************************************************************************/

char * g_p_buffer = NULL;
int g_buffer_offset = 0;
int g_buffer_size = 0;

int g_current_bytes = 0;
int g_last_bytes = 0;

int g_max_bytes = 0;

/********************************************************************************
 * Forward references.															*
 ********************************************************************************/

/********************************************************************************
 * Externs.																		*
 ********************************************************************************/

namespace NsBuffer
{

/********************************************************************************
 *																				*
 *	Method:																		*
 *				init															*
 *	Inputs:																		*
 *				Size of buffer to initialize.									*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Creates a buffer for rendering allocations.						*
 *																				*
 ********************************************************************************/

void init ( int size )
{
	Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
	g_p_buffer = new char[size];
	Mem::Manager::sHandle().BottomUpHeap()->PopAlign();
	g_buffer_offset = 0;
	g_buffer_size = size;
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				reset															*
 *	Inputs:																		*
 *				Reset buffer to beginning location.								*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Resets to beginning of buffer.									*
 *																				*
 ********************************************************************************/

void reset( bool clear )
{
	g_buffer_offset = 0;
	g_current_bytes = 0;

	if ( clear )
	{
		memset( g_p_buffer, 0, g_buffer_size );
	}
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
 *				Latches stat information for the current frame.					*
 *																				*
 ********************************************************************************/

void begin ( void )
{
	g_current_bytes = 0;
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
 *				Calculates stat information for the current frame.				*
 *																				*
 ********************************************************************************/

void end ( void )
{
	if ( g_max_bytes < g_current_bytes )
	{
		g_max_bytes = g_current_bytes;
	}

	g_last_bytes = g_current_bytes;
	g_current_bytes = 0;
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				alloc															*
 *	Inputs:																		*
 *				Number of bytes to allocate.									*
 *	Output:																		*
 *				Pointer to buffer space.										*
 *	Description:																*
 *				Allocates the requested amount of buffer space.					*
 *																				*
 ********************************************************************************/

void * alloc ( int size )
{
	if ( !g_p_buffer ) return NULL;

	void * rv = NULL;

	// See if the remaining buffer space is large enough.
	if ( ( g_buffer_size - g_buffer_offset ) < size )
	{
		// Not large enough, go back to the beginning.
		g_buffer_offset = 0;
	}
	rv = &g_p_buffer[g_buffer_offset];
	g_buffer_offset += size;
	g_current_bytes += size;

	return rv;
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				bytes_used														*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Calculates stat information for the current frame.				*
 *																				*
 ********************************************************************************/

int bytes_used ( void )
{
	return g_last_bytes;
}

int get_size ( void )
{
	return g_buffer_size;
}

}		// NsBuffer
