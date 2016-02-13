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
**	File name:		alloc.cpp												**
**																			**
**	Created by:		03/20/00	-	mjb										**
**																			**
**	Description:	Abstract Allocator class								**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/support.h>
#include "memman.h"
#include "alloc.h"

#ifdef __PLAT_NGC__
#ifdef __EFFICIENT__
#include <dolphin.h>
#endif		// __EFFICIENT__
#endif		// __PLAT_NGC__

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

uint		Allocator::s_current_id = 1;
#ifdef __EFFICIENT__
uint		Allocator::s_align_stack[16] = { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
uint		Allocator::s_align_stack_index = 0;
const uint	Allocator::BlockHeader::sSize = sizeof(BlockHeader); 
#else
#ifdef __PLAT_NGC__
const uint	Allocator::BlockHeader::sSize = (uint)nAlignUpBy( sizeof(BlockHeader), 5 ); 
#else
const uint	Allocator::BlockHeader::sSize = (uint)nAlignUpBy( sizeof(BlockHeader), 4 ); 
#endif		// __PLAT_NGC__
#endif

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

Allocator::Context::Context( void ) 
: m_node( this ), mp_free_list( NULL ) 
{
	

#ifdef __LINKED_LIST_HEAP__
	mp_used_list = NULL;
#endif

	Dbg_AssertType( &m_node, Lst::Node< Context > );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Allocator::Context::~Context( void ) 
{
	

#ifdef __NOPT_DEBUG__
	Dbg_MsgAssert( mp_used_list == NULL,( "Memory Leak" ));
	Dbg_MsgAssert( mp_free_list == NULL,( "Corrupt Free List" ));
	Dbg_AssertType( &m_node, Lst::Node< Context > );
#endif
}


bool Allocator::s_valid( void* pAddr )
{
	BlockHeader* p_bheader = BlockHeader::sRead( pAddr ); 
	return (p_bheader->mId == vALLOC_MAGIC);
}

size_t Allocator::s_get_alloc_size( void* pAddr )
{
	BlockHeader* p_bheader = BlockHeader::sRead( pAddr ); 
	return (p_bheader->mSize);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Allocator* Allocator::s_free( void* pAddr )
{   
	BlockHeader* p_bheader = BlockHeader::sRead( pAddr ); 
	Allocator* p_ret = p_bheader->mpAlloc;

#if 0
	

	Mem::Manager& mem_man = Mem::Manager::sHandle();	
	bool found = false;
	for (Mem::Heap* heap = mem_man.FirstHeap(); heap != NULL; heap = mem_man.NextHeap(heap))
	{		
		if (heap == p_ret) found = true;
	}
	Dbg_MsgAssert(found,("Double dealloc of %p, allocator = %p?", pAddr, p_ret));
#endif

	Dbg_MsgAssert(p_bheader->mId != 0xDEADDEAD,("Freeing Block Twice!\n"));
	Dbg_MsgAssert(p_bheader->mId == vALLOC_MAGIC,("Freeing Corrupt Block\n"));
	
	p_bheader->mpAlloc->free( p_bheader );
	p_bheader->mId = 0xDEADDEAD;						// clear Handle ID to DEAD value

	return p_ret;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Allocator::s_set_id( void* pAddr )
{
	

	BlockHeader* p_bheader = (BlockHeader*)( ((uint)pAddr) - BlockHeader::sSize );
#ifdef __EFFICIENT__
	p_bheader = (BlockHeader*)(((uint)p_bheader) - p_bheader->mPadBytes);
#endif		// __EFFICIENT__
	Dbg_AssertType( p_bheader, BlockHeader );
	Dbg_AssertType( p_bheader->mpAlloc, Allocator );
	
	p_bheader->mId = vALLOC_MAGIC;		//	s_current_id++;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Allocator::BlockHeader* Allocator::BlockHeader::sRead( void* pAddr )
{
	BlockHeader* p_ret = (BlockHeader*)(((uint)pAddr) - BlockHeader::sSize);
#ifdef __EFFICIENT__
	p_ret = (BlockHeader*)(((uint)p_ret) - p_ret->mPadBytes);
#endif		// __EFFICIENT__
	
	Dbg_AssertType( p_ret, BlockHeader );
	Dbg_AssertType( p_ret->mpAlloc, Allocator );
	
	return p_ret;
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

Allocator::Allocator( Region* region, Direction dir, char *p_name )
: mp_region( region ), m_dir( dir ), mp_name(p_name)
{
	
	
	mp_region->RegisterAllocator( this );
	
	m_context_stack.AddToHead( &m_initial_context.m_node );
	mp_context = &m_initial_context;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Allocator::~Allocator( void )
{
	

	Dbg_MsgAssert( !m_context_stack.IsEmpty(),( "Heap stack underflow" ));

	mp_region->UnregisterAllocator( this ); 

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint	Allocator::sGetId( void* pAddr )
{
	

#ifdef __EFFICIENT__
	if ( pAddr == NULL )
#else
	if (( pAddr == NULL ) || !nAligned( pAddr ))
#endif		// __EFFICIENT__
	{
		return 0;
	}

	BlockHeader* p_bheader = (BlockHeader*)( ((uint)pAddr) - BlockHeader::sSize );
#ifdef __EFFICIENT__
	p_bheader = (BlockHeader*)(((uint)p_bheader) - p_bheader->mPadBytes);
#endif		// __EFFICIENT__

	return p_bheader->mId;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Allocator::PushContext( void )
{
	

	Context* new_context = new Context;
	m_context_stack.AddToHead( &new_context->m_node );
	mp_context = new_context;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Allocator::PopContext( void )
{
	

	Dbg_MsgAssert( !m_context_stack.IsEmpty(),( "Heap stack underflow" ));

#ifdef __NOPT_DEBUG__
	dump_free_list();
	dump_used_list();
#endif

	delete mp_context;
		
	Lst::Search< Context > sh;
	mp_context = sh.FirstItem( m_context_stack );

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


#ifdef __NOPT_DEBUG__

void 	Allocator::dump_used_list( void )
{
	
#ifndef __PLAT_NGC__
	BlockHeader*	p_header = mp_context->mp_used_list;
	
	Dbg_Message( "USED LIST for Heap (%p)", this );

	while ( p_header )
	{
		Dbg_AssertType( p_header, BlockHeader );
		Dbg_Message ( "%p  %x   (%p)", (void*)((uint)p_header + BlockHeader::sSize ), 
										p_header->mSize, p_header->mp_next_used );
		p_header = p_header->mp_next_used;
	}

	Dbg_Message( "----------------------" );
#endif		// __PLAT_NGC__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void 	Allocator::dump_free_list( void )
{
	
	
	BlockHeader*	p_header = mp_context->mp_free_list;
	
	Dbg_Message( "FREE LIST for Heap (%p)", this );
	
	while ( p_header )
	{
		Dbg_AssertType( p_header, BlockHeader );
		Dbg_Message ( "%p  %x   (%p)", (void*)((uint)p_header + BlockHeader::sSize ), 
										p_header->mSize, p_header->mpNext );
		p_header = p_header->mpNext;
	}

	Dbg_Message( "----------------------" );
}

#endif


#ifdef	__LINKED_LIST_HEAP__    

// find the block that contains this pointer
// return NULL if not found   
// also includes the header area of the block  							   
void * Allocator::find_block(void *p)
{

	BlockHeader*	p_header = mp_context->mp_used_list;
	
	while ( p_header )
	{
		
		void * p_start = (void*)((uint)p_header + BlockHeader::sSize);
		int size = p_header->mSize;
		void * p_end = (void*)((int)p_start + size);

		if (p >= p_start && p <p_end)
		{
			return (void*)p_header; 
		}		
		p_header = p_header->mp_next_used;
	}
	return NULL;	
}

Allocator::BlockHeader* Allocator::first_block()
{

	BlockHeader*	p_header = mp_context->mp_used_list;
	
	return p_header;	
}




#endif

bool  SameContext(void *p_mem,  Mem::Allocator *p_alloc)
{
	Allocator::BlockHeader* p_bheader = Allocator::BlockHeader::sRead( p_mem ); 
	return (p_bheader->mpAlloc == p_alloc);
}


#ifdef __EFFICIENT__
void Allocator::PushAlign( int align )
{
	s_align_stack_index++;

	switch ( align )
	{
		case 4:
			s_align_stack[s_align_stack_index] = 2;
			break;
		case 8:
			s_align_stack[s_align_stack_index] = 3;
			break;
		case 16:
			s_align_stack[s_align_stack_index] = 4;
			break;
		case 32:
			s_align_stack[s_align_stack_index] = 5;
			break;
		case 64:
			s_align_stack[s_align_stack_index] = 6;
			break;
		case 128:
			s_align_stack[s_align_stack_index] = 7;
			break;
		default:
			Dbg_MsgAssert( false, ( "Illegal alignment (%d) specified. Must be 4, 8, 16, 32, 64 or 128.\n", align ) );
			break;
	}
}

void Allocator::PopAlign( void )
{
	s_align_stack_index--;
}

uint Allocator::GetAlign( void )
{
	return s_align_stack[s_align_stack_index];
}
#endif		// __EFFICIENT__

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mem

