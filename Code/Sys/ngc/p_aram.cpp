/********************************************************************************
 *																				*
 *	Module:																		*
 *				NsARAM															*
 *	Description:																*
 *				ARAMs matrices.													*
 *	Written by:																	*
 *				Paul Robinson													*
 *	Copyright:																	*
 *				2001 Neversoft Entertainment - All rights reserved.				*
 *																				*
 ********************************************************************************/

/********************************************************************************
 * Includes.																	*
 ********************************************************************************/
#include "p_ARAM.h"

/********************************************************************************
 * Defines.																		*
 ********************************************************************************/
 
/********************************************************************************
 * Structures.																	*
 ********************************************************************************/

/********************************************************************************
 * Local variables.																*
 ********************************************************************************/

#define MAX_BU_BLOCKS 4500
#define MAX_TD_BLOCKS 128
#define MAX_SK0_BLOCKS 2
#define MAX_SK1_BLOCKS 2
#define MAX_SCRIPT_BLOCKS (1024 * 6)

uint32 p_bu[MAX_BU_BLOCKS];
uint32 p_td[MAX_TD_BLOCKS];
uint32 p_sk0[MAX_SK0_BLOCKS];
uint32 p_sk1[MAX_SK1_BLOCKS];
uint32 p_script[MAX_SCRIPT_BLOCKS];
char abu[MAX_BU_BLOCKS];		// 1=allocated, 0=freed
char atd[MAX_TD_BLOCKS];		// 1=allocated, 0=freed
char ask0[MAX_SK0_BLOCKS];	// 1=allocated, 0=freed
char ask1[MAX_SK1_BLOCKS];	// 1=allocated, 0=freed
char ascript[MAX_SCRIPT_BLOCKS];	// 1=allocated, 0=freed
int num_bu;
int num_td;
int num_sk0;
int num_sk1;
int num_script;
uint32 p_cur_td;

uint32 p_bu_base;
uint32 p_td_base;
uint32 p_sk0_base;
uint32 p_sk1_base;
uint32 p_script_base;

uint32 g_min_unused = ( 16 * 1024 * 1024 );
uint32 g_fragmented_bu = 0;
uint32 g_fragmented_td = 0;

#define SKATER0_HEAP_SIZE ( 1 * 1024 )
#define SKATER1_HEAP_SIZE ( 1 * 1024 )

#ifdef __NOPT_FINAL__
#define SCRIPT_HEAP_SIZE ( 1280 * 1024 )
#else
#define SCRIPT_HEAP_SIZE ( 1800 * 1024 )
#endif		// __NOPT_FINAL__

/********************************************************************************
 * Forward references.															*
 ********************************************************************************/

/********************************************************************************
 * Externs.																		*
 ********************************************************************************/

void NsARAM::init( void )
{
	num_bu = 0;
	num_td = 0;
	num_sk0 = 0;
	num_sk1 = 0;

	p_bu_base = ARGetBaseAddress();
	p_sk0_base = ( 1024 * 1024 * 16 ) - SKATER0_HEAP_SIZE; 
	p_sk1_base = p_sk0_base - SKATER1_HEAP_SIZE;
	p_script_base = p_sk1_base - SCRIPT_HEAP_SIZE;
	p_td_base = p_script_base;
	p_bu[0] = p_bu_base;
	p_sk0[0] = p_sk0_base;
	p_sk1[0] = p_sk1_base;
	p_script[0] = p_script_base;
	p_cur_td = p_td_base;
}

uint32 NsARAM::alloc( uint32 size, HEAPTYPE heap )
{
	int lp;
//	int plus_size;
//	bool found;
	uint32 rv = 0;

	switch ( heap )
	{
		case BOTTOMUP:
			if ( unused() < size )
			{
				OSReport( "Not enough ARAM for allocation: %d (unused=%d)\n", size, unused() );
				while (1);
			}

			// Make sure we have enough ARAM left to make the allocation.
			if ( ( p_bu[num_bu] + size ) <= p_cur_td )
			{
				Dbg_MsgAssert( num_bu < MAX_BU_BLOCKS, ( "Too many BU allocations - %d, max is %d", num_bu, MAX_BU_BLOCKS ) );
				rv = p_bu[num_bu];					// The address we're going to return.
				abu[num_bu] = 1;					// Set this entry as allocated.
				num_bu++;							// Added 1 allocation.
				p_bu[num_bu] = p_bu[num_bu-1] + ( ( size + 31 ) & ~31 );	// Move current pointer on.
			}
			break;
		case TOPDOWN:
			if ( unused() < size )
			{
				OSReport( "Not enough ARAM for allocation: %d (unused=%d)\n", size, unused() );
				while (1);
			}

			// Make sure we have enough ARAM left to make the allocation.
			if ( ( p_cur_td - size ) >= p_bu[num_bu] )
			{
				Dbg_MsgAssert( num_td < MAX_TD_BLOCKS, ( "Too many TD allocations - %d, max is %d", num_td, MAX_TD_BLOCKS ) );
				p_cur_td -= ( size + 31 ) & ~31;	// Move current pointer on.
				rv = p_cur_td;						// The address we're going to return.
				p_td[num_td] = p_cur_td;			// Set the pointer in the bu stack.
				atd[num_td] = 1;					// Set this entry as allocated.
				num_td++;							// Added 1 allocation.
			}
			break;
		case SKATER0:
			if ( ( ( p_sk0_base + SKATER0_HEAP_SIZE ) - p_sk0[num_sk0] ) < size )
			{
				OSReport( "Not enough ARAM for allocation: %d (unused=%d)\n", size, ( ( p_sk0_base + SKATER0_HEAP_SIZE ) - p_sk0[num_sk0] ) );
				while (1);
			}

			// Make sure we have enough ARAM left to make the allocation.
			if ( ( p_sk0[num_sk0] + size ) <= ( p_sk0_base + SKATER0_HEAP_SIZE ) )
			{
				Dbg_MsgAssert( num_sk0 < MAX_SK0_BLOCKS, ( "Too many sk0 allocations - %d, max is %d", num_sk0, MAX_SK0_BLOCKS ) );
				rv = p_sk0[num_sk0];				// The address we're going to return.
				ask0[num_sk0] = 1;					// Set this entry as allocated.
				num_sk0++;							// Added 1 allocation.
				p_sk0[num_sk0] = p_sk0[num_sk0-1] + ( ( size + 31 ) & ~31 );	// Move current pointer on.
			}
			break;
		case SKATER1:
			if ( ( ( p_sk1_base + SKATER1_HEAP_SIZE ) - p_sk1[num_sk1] ) < size )
			{
				OSReport( "Not enough ARAM for allocation: %d (unused=%d)\n", size, ( ( p_sk1_base + SKATER1_HEAP_SIZE ) - p_sk1[num_sk1] ) );
				while (1);
			}

			// Make sure we have enough ARAM left to make the allocation.
			if ( ( p_sk1[num_sk1] + size ) <= ( p_sk1_base + SKATER1_HEAP_SIZE ) )
			{
				Dbg_MsgAssert( num_sk1 < MAX_SK1_BLOCKS, ( "Too many sk1 allocations - %d, max is %d", num_sk1, MAX_SK1_BLOCKS ) );
				rv = p_sk1[num_sk1];				// The address we're going to return.
				ask1[num_sk1] = 1;					// Set this entry as allocated.
				num_sk1++;							// Added 1 allocation.
				p_sk1[num_sk1] = p_sk1[num_sk1-1] + ( ( size + 31 ) & ~31 );	// Move current pointer on.
			}
			break;
		case SCRIPT:
//			// Search for an empty block that fits exactly.
//			plus_size = ( ( size + 31 ) & ~31 );
//			found = false;
//			for ( lp = 0; lp < ( num_script - 1 ); lp++ )
//			{
//				if ( !ascript[lp] && ( ( p_script[lp+1] - p_script[lp] ) == size ) )
//				{
//					// Found a slot.
//					ascript[lp] = 1;
//					found = true;
//					rv = p_script[lp];				// The address we're going to return.
//					break;
//				}
//			}
//
//			if ( !found )
//			{
				if ( ( ( p_script_base + SCRIPT_HEAP_SIZE ) - p_script[num_script] ) < size )
				{
					OSReport( "Not enough ARAM for allocation: %d (unused=%d)\n", size, ( ( p_script_base + SCRIPT_HEAP_SIZE ) - p_script[num_script] ) );
					while (1);
				}

				// Make sure we have enough ARAM left to make the allocation.
				if ( ( p_script[num_script] + size ) <= ( p_script_base + SCRIPT_HEAP_SIZE ) )
				{
					Dbg_MsgAssert( num_script < MAX_SCRIPT_BLOCKS, ( "Too many script allocations - %d, max is %d", num_script, MAX_SCRIPT_BLOCKS ) );
					rv = p_script[num_script];				// The address we're going to return.
					ascript[num_script] = 1;					// Set this entry as allocated.
					num_script++;							// Added 1 allocation.
					p_script[num_script] = p_script[num_script-1] + ( ( size + 31 ) & ~31 );	// Move current pointer on.
				}
//			}

			break;
		default:
			break;
	}

	uint32 c_unused = unused();
	if ( c_unused < g_min_unused ) g_min_unused = c_unused;

	g_fragmented_bu = 0;
	for ( lp = 0; lp < ( num_bu - 1 ); lp++ )
	{
		if ( !abu[lp] )
		{
			// Found it! Calculate size.
			g_fragmented_bu += p_bu[lp+1] - p_bu[lp];
		}
	}

	g_fragmented_td = 0;
	for ( lp = 0; lp < ( num_td - 1 ); lp++ )
	{
		if ( !atd[lp] )
		{
			// Found it! Calculate size.
			g_fragmented_td += p_td[lp-1] - p_td[lp];
		}
	}

	return rv;
}

void NsARAM::free( uint32 p )
{
	int lp;

	// BOTTOMUP:: Search for this pointer.
	for ( lp = 0; lp < num_bu; lp++ )
	{
		if ( p_bu[lp] == p )
		{
			// Found it! Flag it as freed.
			abu[lp] = 0;
			break;
		}
	}

	// BOTTOMUP:: Scan the stack pointer list & see if we can reduce it.
	for ( lp = (num_bu-1); lp >= 0; lp-- )
	{
		if ( abu[lp] )
		{
			// Found an allocated entry - this is the top of the stack. Adjust to it.
			num_bu = lp + 1;
			break;
		}
		else
		{
			num_bu = lp;
		}
	}

	// TOPDOWN:: Search for this pointer.
	for ( lp = 0; lp < num_td; lp++ )
	{
		if ( p_td[lp] == p )
		{
			// Found it! Flag it as freed.
			atd[lp] = 0;
			break;
		}
	}

	// TOPDOWN:: Scan the stack pointer list & see if we can reduce it.
	for ( lp = (num_td-1); lp >= 0; lp-- )
	{
		if ( atd[lp] )
		{
			// Found an allocated entry - this is the top of the stack. Adjust to it.
			num_td = lp + 1;
			break;
		}
	}

	// Set current top-down pointer.
	if ( num_td )
	{
		p_cur_td = p_td[num_td-1];
	}
	else
	{
		p_cur_td = p_td_base;
	}

	// SKATER0:: Search for this pointer.
	for ( lp = 0; lp < num_sk0; lp++ )
	{
		if ( p_sk0[lp] == p )
		{
			// Found it! Flag it as freed.
			ask0[lp] = 0;
//			OSReport( "Freed address: %d\n", p );
			break;
		}
	}

	// SKATER0:: Scan the stack pointer list & see if we can reduce it.
	for ( lp = (num_sk0-1); lp >= 0; lp-- )
	{
		if ( ask0[lp] )
		{
			// Found an allocated entry - this is the top of the stack. Adjust to it.
			num_sk0 = lp + 1;
			break;
		}
		else
		{
			num_sk0 = lp;
		}
	}

	// SKATER1:: Search for this pointer.
	for ( lp = 0; lp < num_sk1; lp++ )
	{
		if ( p_sk1[lp] == p )
		{
			// Found it! Flag it as freed.
			ask1[lp] = 0;
			break;
		}
	}

	// SKATER1:: Scan the stack pointer list & see if we can reduce it.
	for ( lp = (num_sk1-1); lp >= 0; lp-- )
	{
		if ( ask1[lp] )
		{
			// Found an allocated entry - this is the top of the stack. Adjust to it.
			num_sk1 = lp + 1;
			break;
		}
		else
		{
			num_sk1 = lp;
		}
	}

	// SCRIPT:: Search for this pointer.
	for ( lp = 0; lp < num_script; lp++ )
	{
		if ( p_script[lp] == p )
		{
			// Found it! Flag it as freed.
			ascript[lp] = 0;
			break;
		}
	}

	// SCRIPT:: Scan the stack pointer list & see if we can reduce it.
	for ( lp = (num_script-1); lp >= 0; lp-- )
	{
		if ( ascript[lp] )
		{
			// Found an allocated entry - this is the top of the stack. Adjust to it.
			num_script = lp + 1;
			break;
		}
		else
		{
			num_script = lp;
		}
	}






	g_fragmented_bu = 0;
	for ( lp = 0; lp < ( num_bu - 1 ); lp++ )
	{
		if ( !abu[lp] )
		{
			// Found it! Calculate size.
			g_fragmented_bu += p_bu[lp+1] - p_bu[lp];
		}
	}

	g_fragmented_td = 0;
	for ( lp = 0; lp < ( num_td - 1 ); lp++ )
	{
		if ( !atd[lp] )
		{
			// Found it! Calculate size.
			g_fragmented_td += p_td[lp-1] - p_td[lp];
		}
	}
}

uint32 NsARAM::unused( void )
{
	return p_cur_td - p_bu[num_bu];
}

uint32 NsARAM::getSize( uint32 p )
{
	int lp;
	uint32 rv = 0;

	// BOTTOMUP:: Search for this pointer.
	for ( lp = 0; lp < num_bu; lp++ )
	{
		if ( p_bu[lp] == p )
		{
			// Found it! Calculate size.
			rv = p_bu[lp+1] - p_bu[lp];
			break;
		}
	}

	// TOPDOWN:: Search for this pointer.
	for ( lp = 0; lp < num_td; lp++ )
	{
		if ( p_td[lp] == p )
		{
			// Found it! Calculate size.
			rv = p_td[lp-1] - p_bu[lp];
			break;
		}
	}

	// SKATER0:: Search for this pointer.
	for ( lp = 0; lp < num_sk0; lp++ )
	{
		if ( p_sk0[lp] == p )
		{
			// Found it! Calculate size.
			rv = p_sk0[lp+1] - p_sk0[lp];
			break;
		}
	}

	// SKATER1:: Search for this pointer.
	for ( lp = 0; lp < num_sk1; lp++ )
	{
		if ( p_sk1[lp] == p )
		{
			// Found it! Calculate size.
			rv = p_sk1[lp+1] - p_sk1[lp];
			break;
		}
	}

	// SCRIPT:: Search for this pointer.
	for ( lp = 0; lp < num_script; lp++ )
	{
		if ( p_script[lp] == p )
		{
			// Found it! Calculate size.
			rv = p_script[lp+1] - p_script[lp];
			break;
		}
	}

	return rv;
}


