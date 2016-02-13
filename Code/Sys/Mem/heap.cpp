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
**	File name:		heap.cpp												**
**																			**
**	Maintained by:	Mick													**
**																			**
**	Description:	Heap class												**
**																			**
*****************************************************************************/


// (Mick) undefine this line if you want the full PS2 callstack reporting on allocations
// and the trashing of memory
// Also you might want to set gHeapPools in Poolable.cpp
// which makes the pools use the debug heap, so this debuggin applies to them.
#define __PURE_HEAP__			// No debugging

//////////////////////////////////////////////////////
// Memory Debugging Trick:
// dump the call stack for allocations and de-allocation of 
// a particular block 
// (maybe extend this to any block that encompasses this address????)  							   
#define		REPORT_ON		0			// would be somethng like 0x3a2b50, if you want it to work 

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include "memman.h"
#include "heap.h"
#include <sys\mem\region.h>

#define DUMP_HEAP 0			// Change to 1 to have a handy text dump in memory of the heap status.
							// Use the debugger to see it - look at gMemStatus, 64 bytes per line.

#define NUM_ADDRESS 0		// Change this to 8 to have a handy list of addresses to check.
							// If set to 0, no extra code/data will be generated.
							// Remember to change to 8, compile, then get the addresses in question
							// as the act of adding this code/data will change your addresses.
#if NUM_ADDRESS > 0
uint32 gAddress[NUM_ADDRESS] =
{
	0x81828561,
	0x818284c3,
	0x81827621,
	0x818275e3,
	0x81827541,
	0x81826523,
	0x818264e1,
	0x81826443
};

int g128 = 0;
int g32 = 0;

static void check_address( void * p, int size )
{
	for ( int lp = 0; lp < 8; lp++ )
	{
		if ( gAddress[lp] == ((uint32)p) ) {
			Dbg_Message( "We found the address we're looking for: 0x%08x (%d)\n", (uint32)p, size );
		}
	}

	if ( size == 32 )
	{
		g32++;
	}
	if ( size == 128 )
	{
		g128++;
	}
}

#else
#define check_address(a,b)
#endif

#if DUMP_HEAP == 1
#ifdef __PLAT_XBOX__
static SIZE_T	peakUsage = 64 * 1024 * 1024;	// 64mb.
char gMemStatus[64*28];
#else
char gMemStatus[64*16];
#endif

void dump_heap( void )
{

	Mem::Manager& mem_man = Mem::Manager::sHandle();

	sprintf(&gMemStatus[64*0], "Name            Used   Frag   Free   Blocks                    \n");
	sprintf(&gMemStatus[64*1], "--------------- ------ ------ ------ ------                    \n");
	Mem::Heap* heap;
	int line = 2;
	for (heap = mem_man.FirstHeap(); heap != NULL; heap = mem_man.NextHeap(heap))
	{		
			Mem::Region* region = heap->ParentRegion();			
			sprintf( &gMemStatus[64*line], "%12s: %5dK %5dK %5dK   %5d \n",
					heap->GetName(),
					heap->mUsedMem.m_count / 1024,
					heap->mFreeMem.m_count / 1024,
					region->MemAvailable() / 1024,
					heap->mUsedBlocks.m_count
					);
			line++;
	}

#	ifdef __PLAT_XBOX__
	MEMORYSTATUS stat;
    GlobalMemoryStatus( &stat );
    sprintf( &gMemStatus[64 * line++], "%4d  free kb of physical memory.\n", stat.dwAvailPhys / 1024 );

	if( stat.dwAvailPhys < peakUsage )
		peakUsage = stat.dwAvailPhys;

    sprintf( &gMemStatus[64 * line++], "%4d  free kb at peak physical memory usage.\n", peakUsage / 1024 );
#	endif



}
#endif		// DUMP_HEAP == 1
	
namespace Script
{
	class	CStruct;
	class	CScript;
}

namespace CFuncs
{
bool ScriptDumpHeaps( Script::CStruct *pParams, Script::CScript *pScript );
}

#ifdef	__PLAT_NGPS__
#include <gfx/ngps/p_memview.h>
#elif defined( __PLAT_NGC__ )
#include <gfx/ngc/p_memview.h>
#endif




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

#ifndef	__NOPT_ASSERT__
#define	__PURE_HEAP__
#endif


#ifndef	__PURE_HEAP__		
#define	__TRASH_BLOCKS__				// define this to trash contents of alloc and free, regardless of build				
#endif


						
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

inline Mem::Allocator::BlockHeader*	Heap::next_addr( Allocator::BlockHeader* pHeader )
{
	
	
	Dbg_AssertType( pHeader, BlockHeader );

	return (BlockHeader*)( (uint)pHeader + BlockHeader::sSize + pHeader->mSize );
}


#ifdef	__TRASH_BLOCKS__

//static const uint64 vTRASH_ALLOCATED_BLOCK = 0x0101010101010101;
static const uint64 vTRASH_ALLOCATED_BLOCK = 0x5555555555555555LL; 	// grey
static const uint64 vTRASH_FREE_BLOCK 	 =   0xbbbbbbbbbbbbbbbbLL;	// white
//static const uint64 vTRASH_FREE_BLOCK 	 =       0x3f3f003f3f003f3f;	// ??? ??? (with 0 so string are terminated)
								

inline void	Trash_AllocateBlock( Mem::Allocator::BlockHeader* pBlock )
{	
#ifdef __EFFICIENT__
	memset( (void*)((uint)pBlock + Mem::Allocator::BlockHeader::sSize ), vTRASH_ALLOCATED_BLOCK & 0xff, pBlock->mSize );
#else
	uint64* ptr = (uint64*)((uint)pBlock + Mem::Allocator::BlockHeader::sSize );

	for ( uint i = 0; i < pBlock->mSize; i += 8 )
	{
		*ptr++ = vTRASH_ALLOCATED_BLOCK;
	}
#endif		// __EFFICIENT__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void	Trash_FreeBlock( Mem::Allocator::BlockHeader* pBlock )
{
#ifdef __EFFICIENT__
	memset( (void*)((uint)pBlock + Mem::Allocator::BlockHeader::sSize ), vTRASH_FREE_BLOCK & 0xff, pBlock->mSize );
#else
	uint64* ptr = (uint64*)((uint)pBlock + Mem::Allocator::BlockHeader::sSize ); 	

	for ( uint i = 0; i < pBlock->mSize; i += 8 )
	{
		*ptr++ = vTRASH_FREE_BLOCK;
	}
#endif		// __EFFICIENT__
}

inline void	Trash_FreeBlockHeader( Mem::Allocator::BlockHeader* pBlock )
{
#ifdef __EFFICIENT__
	memset( (void*)((uint)pBlock), vTRASH_FREE_BLOCK & 0xff, Mem::Allocator::BlockHeader::sSize );
#else
	uint64* ptr = (uint64*)((uint)pBlock); 	

	for ( uint i = 0; i < Mem::Allocator::BlockHeader::sSize; i += 8 )
	{
		*ptr++ = vTRASH_FREE_BLOCK;
	}
#endif		// __EFFICIENT__
}


#endif

				
				
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Heap::free ( BlockHeader* pFreeBlock )
{
#if DUMP_HEAP == 1
	dump_heap();
#endif		// DUMP_HEAP == 1
	
#ifdef __EFFICIENT__
	pFreeBlock->mSize += pFreeBlock->mPadBytes;
	pFreeBlock->mPadBytes = 0;
#endif		// __EFFICIENT__
	
	Dbg_AssertType ( pFreeBlock, BlockHeader );

	#ifdef	__NOPT_ASSERT__
	#ifdef	__PLAT_NGPS__
	// find what feeded a particular block
	if ( ((uint)pFreeBlock + BlockHeader::sSize) == REPORT_ON)
	{
		printf ("Freeing 0x%x",REPORT_ON);
		DumpUnwindStack(20,NULL);
	}
	#endif
	#endif
			  
			  
	BlockHeader*	p_before = NULL;
	BlockHeader*	p_2before = NULL;
	BlockHeader*	p_after = mp_context->mp_free_list;


#ifdef	__LINKED_LIST_HEAP__    

#ifdef	__PLAT_NGPS__
#ifndef	__PURE_HEAP__		
	MemView_Free((void*)pFreeBlock);
#endif
#endif
	FreeMemProfile(pFreeBlock);

	// mp_next_used and mp_prev_used are a doble linked null ternimated list

	// unlink next and prev
	if ( pFreeBlock->mp_next_used )
	{
		pFreeBlock->mp_next_used->mp_prev_used = pFreeBlock->mp_prev_used;
	}
	if ( pFreeBlock->mp_prev_used )
	{
		pFreeBlock->mp_prev_used->mp_next_used = pFreeBlock->mp_next_used;
	}
	// unlink if it's the head of the list
	if ( mp_context->mp_used_list == pFreeBlock )
	{
		mp_context->mp_used_list = pFreeBlock->mp_next_used;
	}

	pFreeBlock->mp_prev_used = NULL;
	pFreeBlock->mp_next_used = NULL;

#endif

// we trash the free block before we merge it with anything
// otherwise we end us trashing multi-megabytes
// when memory is fragmented (which it often is, in the middle of things)
	#ifdef	__TRASH_BLOCKS__
	Trash_FreeBlock ( pFreeBlock ); 
	#endif

	mFreeBlocks++;
	mFreeMem += pFreeBlock->mSize;

	mUsedBlocks--;
	mUsedMem -= pFreeBlock->mSize;

	// p_after starts at the head of the free list
	// and traverses it until p_after is the block after this 
	// block
	while ( p_after && ( (uint)p_after < (uint)pFreeBlock ))
	{		
		p_2before = p_before;
		p_before = p_after;
		p_after = p_after->mpNext;
	}

	pFreeBlock->mpNext = p_after; 						// insert pFreeBlock before p_after
	

	if ( p_after )
	{
		if ( next_addr( pFreeBlock ) == p_after ) 		// p_after joins pFreeBlock
		{
			pFreeBlock->mpNext = p_after->mpNext;
			pFreeBlock->mSize += p_after->mSize + BlockHeader::sSize;
			
									  
			p_after->~BlockHeader();
			#ifdef	__TRASH_BLOCKS__
			Trash_FreeBlockHeader ( p_after ); 
			#endif
			mFreeBlocks--;
			mFreeMem += BlockHeader::sSize;
			mUsedMem -= BlockHeader::sSize;
		}
	}
	
	
	if ( p_before )
	{
		if ( next_addr( p_before ) == pFreeBlock )		// pFreeBlock joins p_before
		{	
			p_before->mSize += pFreeBlock->mSize + BlockHeader::sSize;
			p_before->mpNext = pFreeBlock->mpNext;



			pFreeBlock->~BlockHeader();
			
			#ifdef	__TRASH_BLOCKS__
			Trash_FreeBlockHeader ( pFreeBlock ); 
			#endif

			
			mFreeBlocks--;
			mFreeMem += BlockHeader::sSize;
			mUsedMem -= BlockHeader::sSize;
			
			pFreeBlock = p_before;
			p_before = p_2before;
		}
		else											// insert pFreeBlock after p_before
		{							   				
			p_before->mpNext = pFreeBlock;
		}
	}
	else
	{
		mp_context->mp_free_list = pFreeBlock;
	}

	MemDbg_FreeBlock ( pFreeBlock ); 
	

														// reclaim free space in region

	if ( ( m_dir == vBOTTOM_UP ) && ( !p_after ) && 
		 ( mp_top == (void*)((uint)pFreeBlock + pFreeBlock->mSize + BlockHeader::sSize )))
	{
		if( p_before )
		{
			p_before->mpNext = NULL;
		}
		else
		{
			mp_context->mp_free_list = NULL;
		}



		mp_top = (void*)((uint)mp_top - ( pFreeBlock->mSize + BlockHeader::sSize) * m_dir );
		pFreeBlock->~BlockHeader();
		mFreeBlocks--;
		mFreeMem -= pFreeBlock->mSize;
		mUsedMem -= BlockHeader::sSize;
		#ifdef	__TRASH_BLOCKS__
		Trash_FreeBlockHeader ( pFreeBlock ); 
		#endif
	}
	else if (( m_dir == vTOP_DOWN ) && ( !p_before ) &&
			 ( mp_top == (void*)((uint)pFreeBlock )))
	{		 
	
	
		mp_top = (void*)((uint)mp_top - ( pFreeBlock->mSize + BlockHeader::sSize) * m_dir );
		mp_context->mp_free_list = pFreeBlock->mpNext;
		pFreeBlock->~BlockHeader();
		
		
		mFreeBlocks--;
		mFreeMem -= pFreeBlock->mSize;
		mUsedMem -= BlockHeader::sSize;
		#ifdef	__TRASH_BLOCKS__
		Trash_FreeBlockHeader ( pFreeBlock ); 
		#endif
	}
	
#if 0
	Mem::Manager& mem_man = Mem::Manager::sHandle();
	Mem::Heap* heap = mem_man.BottomUpHeap();
	Mem::Region* region = heap->ParentRegion();
	if (heap->mFreeMem.m_count > 3200*1000)
	{
		printf ("\nBottomUp Fragmentation %7dK, in %5d Blocks\n",heap->mFreeMem.m_count / 1024, heap->mFreeBlocks.m_count);
		DumpUnwindStack(20,NULL);
		MemView_DumpFragments(heap);		
		Dbg_MsgAssert(0,("Frag"));
	}
#endif
	
	
	
}


// Returns the size in bytes of either the largest free fragmented block,
// or the free space in the region whichever is larger
int Heap::LargestFreeBlock()
{
	
	uint32 size = 0;
	BlockHeader* p_header = mp_context->mp_free_list;
	while ( p_header ) 
	{
		if ( p_header->mSize >= size )
		{
			size = p_header->mSize;
		}
		p_header = p_header->mpNext;
	}
	uint32 region_size = mp_region->MemAvailable();
	if (region_size > size)
	{	
		size = region_size;
	}
	return size;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Note the block returned might be bigger than asked for
// by 16 or 32 bytes.
// memory system calls MUST account for this 
void*	Heap::allocate( size_t size, bool assert_on_fail )
{
#ifdef	__PLAT_NGPS__
//	if (size > 10*1024)
//	{
//		printf ("\nAllocating %d\n",size);
//		printf ("%s\n",Mem::Manager::sHandle().GetContextName());
//	}
	#endif
	
	Dbg_MsgAssert(size >0, ("Trying to allocate Zero bytes\n"));
	/*
	if (size == 0)
	{
		static int zeros=0;
		printf("(%d) Trying to allocate Zero bytes\n", zeros++);
		DumpUnwindStack(20,NULL);
	}
	*/	
	
	
#if DUMP_HEAP == 1
	dump_heap();
#endif		// DUMP_HEAP == 1

	BlockHeader* p_header = mp_context->mp_free_list;
	BlockHeader* p_freeblock = NULL;
	BlockHeader* p_last = NULL;
	BlockHeader* p_prev = NULL;

//	printf ("Trying to allocate %d bytes, current free list = %p\n",size,p_header);							 
							 
//	if (size == 8688) while (1);		

#ifdef __EFFICIENT__
	int align = 1 << GetAlign();
	int offset_bytes;
	size = (uint)nAlignUpBy( size, 2 );	// all allocations aligned by 4 bytes
#else
#ifdef __PLAT_NGC__
	size = (uint)nAlignUpBy( size, 5 );	// all allocations aligned by 16 bytes
#else
	size = (uint)nAlignUpBy( size, 4 );	// all allocations aligned by 16 bytes
#endif
#endif
	while ( p_header ) 	// find smallest free block large enough to fulfill request
	{
		Dbg_AssertType ( p_header, BlockHeader );

#ifdef __EFFICIENT__
		int off = ( ( ( (int)p_header + BlockHeader::sSize ) + ( align - 1 ) ) & -align ) - ( (int)p_header + BlockHeader::sSize );
		uint adjusted_size = size + off;
		if ( p_header->mSize >= adjusted_size )
#else
		if ( p_header->mSize >= size )
#endif
		{
			if (( p_freeblock == NULL ) || ( p_freeblock->mSize > p_header->mSize ))
			{
				p_prev = p_last;
				p_freeblock = p_header;
			}
		}

		p_last = p_header;
		p_header = p_header->mpNext;
	}

	if ( p_freeblock )	// found a free block large enough
	{	
#ifdef __EFFICIENT__
		offset_bytes = ( ( ( (int)p_freeblock + BlockHeader::sSize ) + ( align - 1 ) ) & -align ) - ( (int)p_freeblock + BlockHeader::sSize );
#endif
		if ( p_prev )
		{
			p_prev->mpNext = p_freeblock->mpNext;
		}
		else
		{
			mp_context->mp_free_list = p_freeblock->mpNext;
		}

		p_freeblock->mpAlloc = this;

		mFreeBlocks--;
		mUsedBlocks++;
		mFreeMem -= p_freeblock->mSize;
		mUsedMem += p_freeblock->mSize;

#ifdef __EFFICIENT__
		BlockHeader*	p_leftover 	= (BlockHeader*)((uint)p_freeblock + BlockHeader::sSize + size + offset_bytes );
		int				new_size 	= p_freeblock->mSize - size - BlockHeader::sSize - offset_bytes;
#else
		BlockHeader*	p_leftover 	= (BlockHeader*)((uint)p_freeblock + BlockHeader::sSize + size );
		int				new_size 	= p_freeblock->mSize - size - BlockHeader::sSize;
#endif		// __EFFICIENT__
		
		if ( new_size > 0 )	// create new free node for left-over memory
		{
			p_freeblock->mSize = size;
			new ((void*)p_leftover) BlockHeader( this, new_size );			
			mUsedBlocks++;
			free( p_leftover );
		}
		else
		{
			// new_size is 0, which implies that this block is (size) ... (size + BlockHeader::Size)
			// which means we'll be allocating a bit more than we need
			// but as (size) is not reference again, then that's not a problem
#ifdef __EFFICIENT__
			p_freeblock->mSize -= offset_bytes;
#endif		// __EFFICIENT__
		}

	}
	else				// request extra space from region
	{
		p_freeblock = (BlockHeader*)mp_region->Allocate( this, size + BlockHeader::sSize, assert_on_fail );

#ifdef __EFFICIENT__
		if ( m_dir == vBOTTOM_UP )
		{
			offset_bytes = ( ( ( (int)p_freeblock + BlockHeader::sSize ) + ( align - 1 ) ) & -align ) - ( (int)p_freeblock + BlockHeader::sSize );
			// Allocate, but keep the original pointer.
			mp_region->Allocate( this, offset_bytes, assert_on_fail );
		}
		else
		{
			offset_bytes = ( (int)p_freeblock + BlockHeader::sSize ) - ( ( (int)p_freeblock + BlockHeader::sSize ) & -align );
			// Allocate, increase size, not offset bytes.
			if ( offset_bytes )
			{
				p_freeblock = (BlockHeader*)mp_region->Allocate( this, offset_bytes, assert_on_fail );
				size += offset_bytes;
				offset_bytes = 0;
			}
		}
#endif
		if ( p_freeblock )
		{
			new ((void*)p_freeblock) BlockHeader( this, size );

			mUsedBlocks++;
#ifdef __EFFICIENT__
			mUsedMem += size + BlockHeader::sSize + offset_bytes;
#else			
			mUsedMem += size + BlockHeader::sSize;
#endif		// __EFFICIENT__
		}
	}

	if ( p_freeblock )
	{		
		MemDbg_AllocateBlock ( p_freeblock );

		#ifdef	__TRASH_BLOCKS__
		Trash_AllocateBlock ( p_freeblock ); 
		#endif


#ifdef	__LINKED_LIST_HEAP__    

		if( mp_context->mp_used_list )
		{
			p_freeblock->mp_next_used = mp_context->mp_used_list;
			mp_context->mp_used_list->mp_prev_used = p_freeblock;
		}
		else
		{
			p_freeblock->mp_next_used = NULL;	 	
		}	
		p_freeblock->mp_prev_used = NULL;	

#ifdef	__PLAT_NGPS__
#ifndef	__PURE_HEAP__		
		MemView_Alloc((void*)p_freeblock);
#endif
#endif
		mp_context->mp_used_list = p_freeblock; 
#endif
		AllocMemProfile(p_freeblock);

		#ifdef	__NOPT_ASSERT__
		#ifdef	__PLAT_NGPS__
		// find what allocated a particular block
		if ( ((uint)p_freeblock + BlockHeader::sSize) == REPORT_ON)
		{
			printf ("allocating 0x%x",REPORT_ON);
			DumpUnwindStack(20,NULL);
		}
		#endif
		#endif

#ifdef __EFFICIENT__
		check_address( (void*)((uint)p_freeblock + BlockHeader::sSize + offset_bytes), size );

		// Fill padding bytes with bytes offset value.
		p_freeblock->mPadBytes = offset_bytes;
		uint8 * p8 = (uint8*)((uint)p_freeblock + BlockHeader::sSize);
		for ( int lp = 0; lp < offset_bytes; lp++ )
		{
			*p8++ = offset_bytes;
		}

		return (void*)((uint)p_freeblock + BlockHeader::sSize + offset_bytes);
#else
		check_address( (void*)((uint)p_freeblock + BlockHeader::sSize), size );

		return (void*)((uint)p_freeblock + BlockHeader::sSize);
#endif		// __EFFICIENT__
	}

// Mick: heap allocations currently coded to ALWAYS assert on fail
//	if ( assert_on_fail )
	{
		Dbg_MsgAssert ( false,( "Failed to allocate %d bytes", size ));


	}

//	Mem::Heap* heap = Mem::Manager::sHandle().BottomUpHeap();
//	printf ("Dumping Fragments.....\n");
//	MemView_DumpFragments(heap);
	

	printf ("----------------------------------\n");
	printf ("failed to allocate %d bytes\n", size); 	

#ifdef __PLAT_NGC__
#ifndef __NOPT_FINAL__
	{
		Mem::Manager& mem_man = Mem::Manager::sHandle();

		printf ("MEM CONTEXT: %s\n",Mem::Manager::sHandle().GetContextName());
		printf("Name            Used  Frag  Free   Blocks\n");
		printf("--------------- ----- ----- ------ ------\n");
		Mem::Heap* heap;
		for (heap = mem_man.FirstHeap(); heap != NULL; heap = mem_man.NextHeap(heap))
		{		
				Mem::Region* region = heap->ParentRegion();			
				printf( "%12s: %6dK %6dK %6dK   %6d \n",
						heap->GetName(),
						heap->mUsedMem.m_count / 1024,
						heap->mFreeMem.m_count / 1024,
						region->MemAvailable() / 1024,
						heap->mUsedBlocks.m_count
						);										
		}
	}
#endif
#endif		// __PLAT_NGC__ 

#ifndef __PLAT_WN32__
	CFuncs::ScriptDumpHeaps(NULL,NULL);
#endif

	return NULL;

}

int	Heap::available()
{
	return LargestFreeBlock();
}

// Ken addition, for use by pip.cpp when it needs to reallocate the block of memory used
// by a pre file that has just been loaded in, so that the pre can be decompressed.
//
// This function will return a pointer to a new block of memory which includes the original
// block, but such that the returned pointer is lower down in memory than the original.
// The original contents of the memory block will be unchanged.
// This function is most likely to work if used on the top-down heap.
// It will assert if it does not work.
void* Heap::reallocate_down( size_t newSize, void *pOld )
{
	// What this function is going to do is allocate a new block with size equal to 
	// the difference between newSize and the size of pOld, then see if the new block 
	// is directly below the passed pOld.
	// If it is, then it will merge the two blocks together.
	// If it is not, it will assert.
	
	Dbg_MsgAssert(pOld,("NULL pOld sent to reallocate_down !"));
	BlockHeader* p_old_block = BlockHeader::sRead( pOld ); 
	
#ifdef __EFFICIENT__
	newSize = (uint)nAlignUpBy( newSize, 2 );	// all allocations aligned by 4 bytes
#else
#ifdef __PLAT_NGC__
	newSize = (uint)nAlignUpBy( newSize, 5 );	// all allocations aligned by 32 bytes
#else
	newSize = (uint)nAlignUpBy( newSize, 4 );	// all allocations aligned by 16 bytes
#endif
#endif		// __EFFICIENT__
	
	Dbg_MsgAssert(p_old_block->mSize < newSize,("Tried to reallocate a block that was already big enough, old size=%d, requested size=%d",p_old_block->mSize,newSize));
	
	// When bobbling together the new and old blocks, we will gain the space used by the old
	// block header, so we don't really need to include that space in the new allocation request.
	// But, if newSize is exactly a blockheader size bigger than the old, then that would cause
	// allocate to be called on a size of zero.
	// So allocate a blockheader size more than necessary to avoid this.
	void *p_new=allocate(newSize-p_old_block->mSize,true);
	Dbg_MsgAssert(p_new,("allocate failed!"));
	
	// Got the new block, so now check that it is directly below the old.	
	BlockHeader* p_new_block=BlockHeader::sRead( p_new );
	Dbg_MsgAssert( (BlockHeader*)(((uint)p_new)+p_new_block->mSize) == p_old_block,("reallocate_down failed! New block is not directly below the old."));
	
	// Got this far, so the new block is right below the old.
	// Now we have to remove the old block header so as to bobble the two blocks together.


#ifdef	__LINKED_LIST_HEAP__    

#ifdef	__PLAT_NGPS__
#ifndef	__PURE_HEAP__	  
	// Both blocks need to be freed from the memory tracker
	// as together they take up all the space of the new block, whcih will be added below  
	MemView_Free((void*)p_old_block); 
	MemView_Free((void*)p_new_block); 
#endif
#endif

	// mp_next_used and mp_prev_used are a doble linked null ternimated list

	// unlink next and prev
	if ( p_old_block->mp_next_used )
	{
		p_old_block->mp_next_used->mp_prev_used = p_old_block->mp_prev_used;
	}
	if ( p_old_block->mp_prev_used )
	{
		p_old_block->mp_prev_used->mp_next_used = p_old_block->mp_next_used;
	}
	// unlink if it's the head of the list
	if ( mp_context->mp_used_list == p_old_block )
	{
		mp_context->mp_used_list = p_old_block->mp_next_used;
	}

	p_old_block->mp_prev_used = NULL;
	p_old_block->mp_next_used = NULL;
#endif
// Free both blocks, so we can just add one later
	FreeMemProfile(p_old_block);
	FreeMemProfile(p_new_block);

#ifdef __EFFICIENT__
	newSize += p_old_block->mPadBytes;
#endif		// __EFFICIENT__

	p_old_block->~BlockHeader();	
	#ifdef	__TRASH_BLOCKS__
	Trash_FreeBlockHeader ( p_old_block ); 
	#endif
	
	// The memory occupied by the old block header has gone from being used as a BlockHeader, 
	// to being used as part of a used memory block, so mUsedMem does not need to change.
	// The number of used blocks has changed though, since two are being merged into one.
	--mUsedBlocks;
	// Nothing has changed regarding free blocks, so mFreeBlocks and mFreeMem stay the same.
	
	
	// That's p_old_block cleaned up, so now modify p_new_block's size.
	p_new_block->mSize=newSize+BlockHeader::sSize;

#ifdef	__PLAT_NGPS__
#ifndef	__PURE_HEAP__		
	MemView_Alloc((void*)p_new_block);
#endif
#endif
	AllocMemProfile(p_new_block);

	
#ifdef __EFFICIENT__
	return (void*)((uint)p_new_block + BlockHeader::sSize + p_new_block->mPadBytes);
#else
	return (void*)((uint)p_new_block + BlockHeader::sSize);
#endif		// __EFFICIENT__
}

// This will make the passed memory block bigger.
// The original contents of the memory block will be unchanged.
// This function is most likely to work if used on the bottom-up heap.
// It will assert if it does not work.
void *Heap::reallocate_up( size_t newSize, void *pOld )
{
	// What this function is going to do is allocate a new block with size equal to 
	// the difference between newSize and the size of pOld, then see if the new block 
	// is directly above the passed pOld.
	// If it is, then it will merge the two blocks together.
	// If it is not, it will assert.
	
	Dbg_MsgAssert(pOld,("NULL pOld sent to reallocate_up !"));
	BlockHeader* p_old_block = BlockHeader::sRead( pOld ); 
	
#ifdef __EFFICIENT__
	newSize = (uint)nAlignUpBy( newSize, 2 );	// all allocations aligned by 4 bytes
#else
#ifdef __PLAT_NGC__
	newSize = (uint)nAlignUpBy( newSize, 5 );	// all allocations aligned by 32 bytes
#else
	newSize = (uint)nAlignUpBy( newSize, 4 );	// all allocations aligned by 16 bytes
#endif
#endif		// __EFFICIENT__
	
	if (p_old_block->mSize >= newSize)
	{
		// The current block is already big enough, so nothing to do.
		return pOld;
	}
		
	// When bobbling together the new and old blocks, we will gain the space used by the old
	// block header, so we don't really need to include that space in the new allocation request.
	// But, if newSize is exactly a blockheader size bigger than the old, then that would cause
	// allocate to be called on a size of zero.
	// So allocate a blockheader size more than necessary to avoid this.
	void *p_new=allocate(newSize-p_old_block->mSize,true);
	Dbg_MsgAssert(p_new,("allocate failed!"));
	
	// Got the new block, so now check that it is directly above the old.	
	BlockHeader* p_new_block=BlockHeader::sRead( p_new );
	if ( (BlockHeader*)(((uint)pOld)+p_old_block->mSize) != p_new_block)
	{
		// It isn't!
		free(p_new_block);
		return NULL;
	}	
	
	// Got this far, so the new block is right above the old.
	// Now we have to remove the new block header so as to bobble the two blocks together.


#ifdef	__LINKED_LIST_HEAP__    

// remove both blocks from memory tracking, ready to add the new block
// that encompasses both of them
#ifdef	__PLAT_NGPS__
#ifndef	__PURE_HEAP__		
	MemView_Free((void*)p_old_block); 
	MemView_Free((void*)p_new_block); 
#endif
#endif
	// mp_next_used and mp_prev_used are a doble linked null ternimated list

	// unlink next and prev
	if ( p_new_block->mp_next_used )
	{
		p_new_block->mp_next_used->mp_prev_used = p_new_block->mp_prev_used;
	}
	if ( p_new_block->mp_prev_used )
	{
		p_new_block->mp_prev_used->mp_next_used = p_new_block->mp_next_used;
	}
	// unlink if it's the head of the list
	if ( mp_context->mp_used_list == p_new_block )
	{
		mp_context->mp_used_list = p_new_block->mp_next_used;
	}

	p_new_block->mp_prev_used = NULL;
	p_new_block->mp_next_used = NULL;
#endif
	FreeMemProfile(p_old_block);
	FreeMemProfile(p_new_block);

	// (Mick) Since the alloc function might return a bigger block than was asked for
	// we need to account for this in calculating the size of the new block
	// we can get the size of the new block before we trash the header
	int new_block_size = p_new_block->mSize;

	p_new_block->~BlockHeader();	
	#ifdef	__TRASH_BLOCKS__
	Trash_FreeBlockHeader ( p_new_block ); 
	#endif
	
	// The memory occupied by the new block header has gone from being used as a BlockHeader, 
	// to being used as part of a used memory block, so mUsedMem does not need to change.
	// The number of used blocks has changed though, since two are being merged into one.
	--mUsedBlocks;
	// Nothing has changed regarding free blocks, so mFreeBlocks and mFreeMem stay the same.

#ifdef	__NOPT_ASSERT__
	if (pOld == (void*)REPORT_ON)
	{
		printf ("%p: realloced up from %d to %d (actually %d) bytes\n", p_old_block, p_old_block->mSize, newSize, new_block_size + BlockHeader::sSize);  	
	}
#endif
	
	
	// That's p_new_block cleaned up, so now modify p_old_block's size.
//	p_old_block->mSize=newSize+BlockHeader::sSize;		 // Old method, did not account for alloc returning oversized blocks
	p_old_block->mSize += new_block_size + BlockHeader::sSize;	  // (Mick) New method, uses the actual size of the new block (plus blockheader)

#ifdef	__PLAT_NGPS__
#ifndef	__PURE_HEAP__		
	MemView_Alloc((void*)p_old_block); 
#endif
#endif
	AllocMemProfile(p_old_block);


	
	return pOld;
}

void *Heap::reallocate_shrink( size_t newSize, void *pOld )
{
	// This will shrink the passed block by creating a new free block out of the remaining
	// space, if there is enough room to make a new free block. (at least 32 bytes needed)
	// It creates the new free block by first creating a new used block, then freeing that.
	// That way, all the merging of free blocks will be taken care of.
		
	Dbg_MsgAssert(pOld,("NULL pOld sent to reallocate_up !"));
	BlockHeader* p_old_block = BlockHeader::sRead( pOld ); 


	Dbg_MsgAssert(p_old_block->mpAlloc == this,("Shrinking block in wrong context"));


	
	//printf("Block size before shrinking = %d\n",p_old_block->mSize);
	
	Dbg_MsgAssert(newSize<=p_old_block->mSize,("Larger size sent to reallocate_shrink:\nold size=%d  requested new size=%d",p_old_block->mSize,newSize));
	uint32 size_diff=p_old_block->mSize-newSize;
	
	// We can only shrink down by a number of bytes that is divisible be 16 (4 on ngc), so
	// that the resulting block size for the old block remains a multiple of 16 (4)
#ifdef __EFFICIENT__
	size_diff&=~((1<<2)-1);
#else
	#ifdef __PLAT_NGC__
	size_diff&=~((1<<5)-1);
	#else
	size_diff&=~((1<<4)-1);
	#endif
#endif		// __EFFICIENT__
	
	// If the amount the block is being shrunk by is not enough to hold a block header
	// with a bit left over, then we can't make a new free block with it, so return without
	// doing anything.
	if (size_diff<=BlockHeader::sSize)
	{
		return pOld;
	}	
	
	// Calculate a pointer to the new block.
	BlockHeader *p_new_block=(BlockHeader*)((uint32)pOld+p_old_block->mSize-size_diff);

	#ifdef	__PLAT_NGPS__
	Dbg_MsgAssert(((int)p_new_block&0xf) == 0,("p_new_block odd (%p), pOld = %p, p_old_block = %p, p_old_block->mSize = %d, size_diff = %d",
										p_new_block, pOld, p_old_block->mSize, size_diff));
	#endif
	
	// Fill in new block header
	p_new_block->mSize=size_diff-BlockHeader::sSize;
#ifdef __EFFICIENT__
	p_new_block->mPadBytes = 0;
#endif		// __EFFICIENT__ 
	/////////////////////////////////////////////////////////////////////
	// Do all the stuff one needs to do when creating a new used block.
	// I just happily cut-and-pasted this lot from ::allocate
	p_new_block->mpAlloc = this;
	MemDbg_AllocateBlock ( p_new_block );
	#ifdef	__TRASH_BLOCKS__
	Trash_AllocateBlock ( p_new_block ); 
	#endif

#ifdef	__LINKED_LIST_HEAP__    

	if( mp_context->mp_used_list )
	{
		p_new_block->mp_next_used = mp_context->mp_used_list;
		mp_context->mp_used_list->mp_prev_used = p_new_block;
	}
	else
	{
		p_new_block->mp_next_used = NULL;	 	
	}	
	p_new_block->mp_prev_used = NULL;	

#ifdef	__PLAT_NGPS__
#ifndef	__PURE_HEAP__		
	MemView_Alloc((void*)p_new_block);
#endif
#endif
	mp_context->mp_used_list = p_new_block; 
#endif
	AllocMemProfile(p_new_block);
	/////////////////////////////////////////////////////////////////////
	
	// mUsedMem does not change, since we've made a new used block out of memory
	// that was already being used.
	// The number of used blocks has increased by one though.
	++mUsedBlocks;
	// Nothing has changed regarding mFreeBlocks or mFreeMem yet. That will all be handled
	// when the block gets freed.
	
	// Free the block just created.
	free(p_new_block);
		

#ifdef	__PLAT_NGPS__
#ifndef	__PURE_HEAP__		
	MemView_Free((void*)p_old_block);	   // free at old size
#endif
#endif
	FreeMemProfile(p_old_block);

	// Update the size of the old block.
	p_old_block->mSize-=size_diff;
	
#ifdef	__PLAT_NGPS__
#ifndef	__PURE_HEAP__		
	MemView_Alloc((void*)p_old_block);	   // re-register at new size
#endif
#endif
	AllocMemProfile(p_old_block);

	
	//printf("New block size after shrinking = %d\n",p_old_block->mSize);
	return pOld;
}

/****************************************************************************
**							   Public Functions								**
*****************************************************************************/

Heap::Heap( Region* region, Direction dir, char *p_name )
: Allocator( region, dir, p_name )
{
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mem

