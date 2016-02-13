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
**	File name:		pool.cpp												**
**																			**
**	Created by:		03/29/00	-	mjb										**
**																			**
**	Description:	Memory Pool class										**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include "memman.h"
#include "pool.h"
#include <sys\mem\region.h>

/*****************************************************************************
**							  DBG Information								**
*****************************************************************************/


namespace Mem
{



/*****************************************************************************
**								Private Types								**
*****************************************************************************/
	
/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

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

void Pool::dump_free_list( void )
{
	
	
	BlockHeader*	p_header = mp_free_list;

	while ( p_header )
	{
		Dbg_AssertType( p_header, BlockHeader );
		Dbg_Message ( "%p  %d   (%p)", (void*)((uint)p_header + sizeof( BlockHeader )), 
										p_header->mSize, p_header->mpNext );
		p_header = p_header->mpNext;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void*	Pool::allocate( size_t size, bool assert_on_fail )
{
	
		
	Dbg_MsgAssert ( size <= m_size,( "Failed to allocate: Size (%x) > Pool Element (%x)", size, m_size ));

	if ( mp_free_list )
	{
		Dbg_AssertType( mp_free_list, BlockHeader );
		
		BlockHeader* p_header = mp_free_list;
		
		mp_free_list = p_header->mpNext;
		p_header->mpAlloc = this;

		MemDbg_AllocateBlock ( p_header );	

		return (void*)((uint)p_header + BlockHeader::sSize);
	}

	if ( assert_on_fail )
	{
		Dbg_MsgAssert ( false,( "Failed to allocate %d bytes, no free slots", size ));
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Pool::free( BlockHeader* pFreeBlock )
{
	
	
	Dbg_AssertType( pFreeBlock, BlockHeader );

	MemDbg_FreeBlock ( pFreeBlock );	
	   
	pFreeBlock->mpNext = mp_free_list;
	mp_free_list = pFreeBlock;
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

Pool::Pool( Region* region, size_t size, uint count, Direction dir )
: Allocator( region, dir, "Pool" ), m_count( count), m_size( size )
{
	

	if ( !nAlignedBy( m_size, 4 ))
	{
#ifdef __PLAT_NGC__
		Dbg_Warning ( "size %x must be 32 byte aligned ( setting to %x )", m_size, nAlignUpBy( (void*)m_size, 5 ));
		m_size = (size_t)nAlignUpBy( m_size, 5 );
#else
		Dbg_Warning ( "size %x must be 16 byte aligned ( setting to %x )", m_size, nAlignUpBy( (void*)m_size, 4 ));
		m_size = (size_t)nAlignUpBy( m_size, 4 );
#endif
	}

	BlockHeader* p_freeblock = (BlockHeader*)mp_region->Allocate( this, ( m_size + BlockHeader::sSize ) * m_count );
	
	Dbg_MsgAssert ( p_freeblock,( "Failed to allocate Memory Pool from Region" ));

	mp_free_list = p_freeblock;

	for ( uint i = 0; i < ( m_count - 1 ); i++ )
	{
		new ((void*)p_freeblock) BlockHeader( this, m_size );		
		
		BlockHeader* p_nextblock = (BlockHeader*)( (uint)p_freeblock + BlockHeader::sSize + m_size ); 
		p_freeblock->mpNext = p_nextblock;
		p_freeblock = p_nextblock;
	}

	new ((void*)p_freeblock) BlockHeader( this, m_size );		
	p_freeblock->mpNext = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Pool::~Pool( void )
{
	

	Dbg_Code  // warn if there are still allocated slots.
	(
		uint free_list_count = 0;

		for ( BlockHeader* p_free_list = mp_free_list; p_free_list; p_free_list = p_free_list->mpNext )
		{
			free_list_count++;
		}

		Dbg_MsgAssert(( free_list_count == m_count ),( "Pool still contains allocated slots" ));
	)
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mem

