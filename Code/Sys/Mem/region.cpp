/*****************************************************************************
**																			**
**			              Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		System Library											**
**																			**
**	Module:			Memory (Mem) 											**
**																			**
**	File name:		region.cpp												**
**																			**
**	Created by:		03/20/00	-	mjb										**
**																			**
**	Description:	Region Class											**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <sys\mem\region.h>
#include <sys/config/config.h>
#include "alloc.h"

/*****************************************************************************
**							  DBG Information								**
*****************************************************************************/

namespace Mem
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/
	
/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

Region::~Region( void )
{
	

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Region::init( void* pStart, void* pEnd )
{
	

	Dbg_AssertPtr( pStart );

	mp_start	= pStart;
	mp_end		= pEnd;

	m_min_free  = (int)pEnd - (int)pStart;

	m_alloc[vBOTTOM_UP] = 
	m_alloc[vTOP_DOWN] 	= NULL;

	Dbg_Notify( "Region(%p) created %p -> %p ", this, mp_start, mp_end );

	if (!Config::CD())
	{
#ifdef __CLEAR_MEMORY__
		uint64* ptr = (uint64*)mp_start; 
		uint 	size = (uint)mp_end - (uint)mp_start;
	
		for ( uint i = 0; i < size; i += 8 )
		{
			*ptr++ = vFREE_BLOCK;
		}
#endif
	}
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

void		Region::RegisterAllocator( Allocator* pAlloc )
{
	

	int index = ( pAlloc->m_dir & 2 ) >> 1;

	Dbg_MsgAssert ( m_alloc[index] == NULL,( "Attempt to register multiple Allocators" ));

	m_alloc[index] = pAlloc;

	pAlloc->mp_top = ( pAlloc->m_dir == Allocator::vBOTTOM_UP ) ? mp_start : mp_end;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Region::UnregisterAllocator( Allocator* pAlloc )
{
	
	
	int index = ( pAlloc->m_dir & 2 ) >> 1;
	
	Dbg_MsgAssert ( m_alloc[index] == pAlloc,( "Allocator not currently registered" ));

	m_alloc[index] = NULL;
}
	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Givne an allocator pAlloc, then allocate some space from this region
// for this allocator
// account for the m_dir flag in the allocator, which will be -1 or +1

void*		Region::Allocate( Allocator* pAlloc, size_t size, bool assert_on_fail )
{
	
	
	Dbg_AssertType ( pAlloc, Allocator );

	// Get index = 0 if BottomUp
	//     index = 1 if TopDown
	int index = ( pAlloc->m_dir & 2 ) >> 1;
	Dbg_MsgAssert ( m_alloc[index] == pAlloc,( "Allocator not currently registered" ));

	// mp_top of an allocator is the "Top" of the heap relative to what direction it is going in
	// hence for a BottomUp heap, the mp_top will be the byte after the highest byte
	// and for a TopDown heap, the mp_top will be the lowest byte	
	
	// here we get a new p_top for the allocator in pAlloc
	// then we just need to test it
	void* p_top = (void*) (( (uint)pAlloc->mp_top ) + ( size * pAlloc->m_dir ) );
	
	if( Allocator* p_alloc = m_alloc[index^1] )	  // If other allocator exists
	{
		if ( index == vBOTTOM_UP )				  // we are bottom up, other allocater is top down
		{
			if ( p_top > p_alloc->mp_top )		 // our new top is above the other allocator's base
			{
				if( assert_on_fail )			 // so fail
				{
					Dbg_MsgAssert( false,( "Failed to Allocate %d bytes", size ));
				}
				Dbg_Warning( "Failed to Allocate %d bytes", size ); 
				return NULL;
			}
		}
		else	// we are top down, other allocator is bottom up
		{
			if ( p_top < p_alloc->mp_top )			// new top is below the other allocator's top
			{
				if( assert_on_fail )				// so fail
				{
					Dbg_MsgAssert( false,( "Failed to Allocate %d bytes", size ));
				}
				Dbg_Warning( "Failed to Allocate %d bytes", size ); 
				return NULL;
			}
		}
	}
	else
	{
		// here there is no other allocator
		if ( index == vBOTTOM_UP )				// for a single bottom up heap
		{
			if ( p_top > mp_end )  				// if new p_top goes past end of region
			{
				if( assert_on_fail )			// then fail
				{
					Dbg_MsgAssert( false,( "Failed to Allocate %d bytes", size ));
				}
				Dbg_Warning( "Failed to Allocate %d bytes", size ); 
				return NULL;
			}
		}
		else	// single top down	heap
		{
			if ( p_top < mp_start )		 // p_top goes below start of region
			{
				if( assert_on_fail )
				{
					Dbg_MsgAssert( false,( "Failed to Allocate %d bytes", size ));
				}
				Dbg_Warning( "Failed to Allocate %d bytes", size ); 
				return NULL;
			}
		}
	}

	// If it's vBOTTOM_UP then
	//    return pAlloc->mp_top, which is the old end of the heap, and the start of the new block
	// else
	// 	  return p_top, which is the base of the new block (when heap growing downwards)

	void* p_ret = ( index == vBOTTOM_UP ) ? pAlloc->mp_top : p_top;

	// store new "top" of heap
	pAlloc->mp_top = p_top;

	int free = MemAvailable();
	if (free < m_min_free)
	{
		m_min_free = free;
	}


	// and return the start of the new memory
	return p_ret;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Region::Region( void* pStart, void* pEnd )
{
	
		
	init( pStart, pEnd );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

AllocRegion::AllocRegion( size_t size )
{
	

	size = (size_t) nAlignDown( size );
	
	mp_start = new char[size];
	
	init( mp_start, (void*)( (uint)mp_start + size ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

AllocRegion::~AllocRegion( void )
{
	

	delete[] (char *)mp_start;
}
					
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int Region::MemAvailable( void )
{
	
		
	int bot = (int)((m_alloc[0]) ? m_alloc[0]->mp_top : mp_start);
	int top = (int)((m_alloc[1]) ? m_alloc[1]->mp_top : mp_end);

	return ((int)top - (int)bot);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mem

