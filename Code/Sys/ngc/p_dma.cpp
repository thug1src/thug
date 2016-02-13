/********************************************************************************
 *																				*
 *	Module:																		*
 *				NsDMA															*
 *	Description:																*
 *				DMAs matrices.													*
 *	Written by:																	*
 *				Paul Robinson													*
 *	Copyright:																	*
 *				2001 Neversoft Entertainment - All rights reserved.				*
 *																				*
 ********************************************************************************/

/********************************************************************************
 * Includes.																	*
 ********************************************************************************/
#include "p_DMA.h"

/********************************************************************************
 * Defines.																		*
 ********************************************************************************/

#define BUFFER_SIZE 16384
 
/********************************************************************************
 * Structures.																	*
 ********************************************************************************/

/********************************************************************************
 * Local variables.																*
 ********************************************************************************/

static volatile bool	dmaComplete;

/********************************************************************************
 * Forward references.															*
 ********************************************************************************/

/********************************************************************************
 * Externs.																		*
 ********************************************************************************/

static void arqCallback( u32 pointerToARQRequest )
{
	dmaComplete = true;
}

/********************************************************************************
 *																				*
 *	Method:																		*
 *				toARAM															*
 *	Inputs:																		*
 *				<none>															*
 *	Output:																		*
 *				<none>															*
 *	Description:																*
 *				Declares a scene object.										*
 *																				*
 ********************************************************************************/

void NsDMA::toARAM( u32 aram, void * p_mram, int size )
{
	ARQRequest	request;

	if ( ( ( aram & 31 ) == 0 ) && ( ( (u32)p_mram & 31 ) == 0 ) && ( ( size & 31 ) == 0 ) )
	{
		// Fast version.
		dmaComplete = false;
		DCFlushRange( p_mram, size );
		ARQPostRequest	(	&request,
							0,						// Owner.
							ARQ_TYPE_MRAM_TO_ARAM,	// Type.
							ARQ_PRIORITY_HIGH,		// Priority.
							(u32)p_mram,			// Source.
							aram,					// Dest.
							size,					// Length.
							arqCallback );			// Callback

		// Wait for it to complete.
		while( !dmaComplete );

		DCFlushRange ( p_mram, size );
	}
	else
	{
		char s_buf[BUFFER_SIZE+32+32] __attribute__ (( aligned( 32 )));
		char * p_buffer = (char *)OSRoundUp32B(s_buf);

		// DMA.
		int address = ( aram & ~31 );
		int remaining = ( ( size + 31 ) & ~31 );
		char * pLoad = (char *)p_mram;
		while ( remaining )
		{
			int thistime = BUFFER_SIZE;
			if ( remaining < BUFFER_SIZE ) thistime = remaining;

			memcpy( p_buffer, pLoad, thistime );

			dmaComplete = false;
			DCFlushRange( p_buffer, thistime );
			ARQPostRequest	(	&request,
								0,						// Owner.
								ARQ_TYPE_MRAM_TO_ARAM,	// Type.
								ARQ_PRIORITY_HIGH,		// Priority.
								(u32)p_buffer,			// Source.
								address,				// Dest.
								thistime,				// Length.
								arqCallback );			// Callback

			// Wait for it to complete.
			while( !dmaComplete );

			DCFlushRange ( p_buffer, thistime );

			address += thistime;
			remaining -= thistime;
			pLoad += thistime;
		}
	}
}

void NsDMA::toMRAM( void * p_mram, u32 aram, int size )
{
	ARQRequest	request;
	char s_buf[BUFFER_SIZE+32+32] __attribute__ (( aligned( 32 )));
	char * p_buffer = (char *)OSRoundUp32B(s_buf);

	int address = ( aram & ~31 );
	int adjust = ( aram - address );
	int remaining = size;
	char * pStore = (char *)p_mram;
	while ( remaining )
	{
		int thistime = BUFFER_SIZE - adjust;
		if ( remaining < BUFFER_SIZE ) thistime = remaining;

		dmaComplete = false;
		DCFlushRange ( p_buffer, (thistime + adjust + 31 ) & ~31 );
//		DCInvalidateRange( p_buffer, thistime ); 
		ARQPostRequest	(	&request,
							0,						// Owner.
							ARQ_TYPE_ARAM_TO_MRAM,	// Type.
							ARQ_PRIORITY_HIGH,		// Priority.
							address,				// Source.
							(u32)p_buffer,			// Dest.
							(thistime + adjust + 31 ) & ~31,// Length.
							arqCallback );			// Callback

		// Wait for it to complete.
		while( !dmaComplete );

		DCFlushRange ( p_buffer, (thistime + adjust + 31 ) & ~31 );
//		DCStoreRange( p_buffer, thistime + adjust ); 

		// Copy to actual mram address.
		memcpy( pStore, &p_buffer[adjust], thistime );

		// Next chunk.
		pStore += thistime;
		address += thistime + adjust;
		remaining -= thistime;
		adjust = 0;
	}
}


