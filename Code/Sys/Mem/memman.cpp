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
**	File name:		memman.cpp												**
**																			**
**	Created by:		03/20/00	-	mjb										**
**																			**
**	Description:	Memory manager											**
**																			**
*****************************************************************************/



/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <malloc.h>
#include <string.h>
#include <core/defines.h>
#include <core/thread.h>
#include <core/string/stringutils.h>
#include "memman.h"
#include <sys\mem\region.h>
#include <sys\config\config.h>	// for memory profiling, to see if we ahve the extra memory
#include "heap.h"
#include "alloc.h"
#include <sys/profiler.h>
#ifdef __PLAT_XBOX__
#include <xtl.h>
#endif
#ifdef __PLAT_NGC__
#include <dolphin.h>
#endif

#include <sk/heap_sizes.h>


#ifdef	__PLAT_NGPS__

static	bool	s_use_semaphore = false;

void WaitSemaMaybe(int sema)
{
	if (s_use_semaphore)
	{
		WaitSema(sema);
	}
}


void SignalSemaMaybe(int sema) 
{
	if (s_use_semaphore)
	{
		SignalSema(sema);
	}
}

#endif


/*****************************************************************************
**							  DBG Information								**
*****************************************************************************/


/*****************************************************************************
**								  Externals									**
*****************************************************************************/

#if defined ( __PLAT_NGPS__ )
extern char _code_start[]					__attribute__((section(".text")));
extern char _code_end[]						__attribute__((section(".text")));
extern char _data_end[]						__attribute__((section(".text")));
extern char _mem_start[]					__attribute__((section(".mem_block")));
extern char _mem_end[]						__attribute__((section(".mem_block")));
extern char _std_mem_end[]					__attribute__((section(".mem_block")));
extern char _debug_heap_start[]				__attribute__((section(".mem_block")));
extern char _script_heap_start[]			__attribute__((section(".mem_block")));
#else
char* 	_code_start;
char* 	_code_end;
char* 	_data_end;
char* 	_mem_start;
char* 	_mem_end;
char*	_std_mem_end;
char *	_debug_heap_start;
#endif

/*****************************************************************************
**								   Defines									**
*****************************************************************************/


namespace Mem
{



/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

char 			Manager::s_manager_buffer[sizeof(Manager)];
char 			Manager::s_region_buffer[sizeof(Region)];
char 			Manager::s_script_region_buffer[sizeof(Region)];
char 			Manager::s_top_heap_buffer[sizeof(Heap)];
char 			Manager::s_bot_heap_buffer[sizeof(Heap)];
char 			Manager::s_debug_heap_buffer[sizeof(Heap)];
Manager*		Manager::sp_instance = NULL;
#ifdef __PLAT_NGPS__
static	int		s_context_semaphore;
#endif
char			Manager::s_debug_region_buffer[sizeof(Region)];

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

Manager::Manager( void )
{
	
	
	m_current_id = 0;
	mp_process_man = NULL;
	
#	if defined ( __PLAT_XBOX__ )
	// Just grab 33mb of main memory.
	_mem_start		= (char*)XPhysicalAlloc(	33 * 1024 * 1024,				// size of region
												MAXULONG_PTR,					// base physical address
												0,								// memory alignment
												PAGE_READWRITE );				// memory protection and type

	_mem_end		= _mem_start + ( 33 * 1024 * 1024 );
	_std_mem_end	= _mem_end;
#	elif defined ( __PLAT_WN32__ )
	// Nasty hack for WN32 for now - just grab 38mb of main memory via a malloc.
	_mem_start	= (char *)malloc( 38 * 1024 * 1024 );
	_mem_end	= _mem_start + ( 38 * 1024 * 1024 );
	_std_mem_end = _mem_end;

#	endif 
#	if defined ( __PLAT_NGC__ )
	// Fill in what we know.
	_code_start		= (char *)0xfadebabe;		// Junk addresses.
	_code_end		= (char *)0xfacced00;
	_data_end		= (char *)0xcaccfacc;
	_mem_start		= &((char *)OSGetArenaLo())[8192];  // Real, actual useful addresses.
	_mem_end		= (char *)OSGetArenaHi();
	_std_mem_end	= (char *)OSGetArenaHi();
#	endif

	// create root memory region
	mp_region 	= new ((void*)s_region_buffer) Region( nAlignUp( _mem_start ), nAlignDown( _std_mem_end ) );

	// create default heaps
	mp_top_heap = new ((void*)s_top_heap_buffer) Heap( mp_region, Heap::vTOP_DOWN, "Top Down" );
	mp_bot_heap = new ((void*)s_bot_heap_buffer) Heap( mp_region, Heap::vBOTTOM_UP, "BottomUp" );

	m_heap_list[0] = mp_top_heap;
	m_heap_list[1] = mp_bot_heap;
	
	m_num_heaps = 2;

#	if !defined( __PLAT_NGC__ ) || ( defined( __PLAT_NGC__ ) && !defined( __NOPT_FINAL__ ) )
	uint codesize = ((uint)(_code_end) - (uint)(_code_start))/1024;
	uint datasize = ((uint)(_data_end) - (uint)(_code_end))/1024;
	printf ( "code [%p - %p] (%dK) + data [%p - %p] (%dK) = %dK \n",
			 _code_start, _code_end, codesize, 
			 _code_end, _data_end, datasize,
			 codesize + datasize );
#endif
			 
	m_pushed_context_count = 0;
	mp_internet_region = NULL;
	mp_net_misc_region = NULL;

	mp_cutscene_region = NULL;
	mp_cutscene_bottom_heap = NULL;
	mp_cutscene_top_heap = NULL;

#ifdef __PLAT_NGPS__
	// Create a semaphore to prevent threads from re-entering push/pop memory contexts
	struct SemaParam params;

	params.initCount = 1;
	params.maxCount = 10;

	s_context_semaphore = CreateSema( &params );			 
#endif

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Manager::~Manager( void )
{
	

	mp_top_heap->~Heap();
	mp_bot_heap->~Heap();
	mp_region->~Region();

#ifdef __PLAT_NGPS__
	DeleteSema( s_context_semaphore );
#endif
}


/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

void*	Manager::New( size_t size, bool assert_on_fail, Allocator* pAlloc )
{   
#ifdef __PLAT_NGPS__
	WaitSemaMaybe( s_context_semaphore );
#endif // __PLAT_NGPS__

	if ( !pAlloc ) 					// set to 'default' allocator
	{
		pAlloc = mp_context->mp_alloc;
	}

	void* p_ret = pAlloc->allocate( size, assert_on_fail );
	
	if ( p_ret )	// if allocation was successful
	{
		Allocator::s_set_id( p_ret );		// stamp ID; used by Mem::Handle
	}

#if 0 
	if ( mp_process_man )
	{
		mp_process_man->AllocEv( p_ret, size, pAlloc );
	}
#endif

#ifdef __PLAT_NGPS__
	SignalSemaMaybe( s_context_semaphore );
#endif // __PLAT_NGPS__

	return p_ret;
}


// Returns the amount of memory avaialbe in the current context
// currently this is only valid for Heaps
int		Manager::Available()
{
	return mp_context->mp_alloc->available();
}

// Ken addition, for use by pip.cpp when it needs to reallocate the block of memory used
// by a pre file that has just been loaded in, so that the pre can be decompressed.
void*	Manager::ReallocateDown( size_t newSize, void *pOld, Allocator* pAlloc )
{   
#ifdef __PLAT_NGPS__
	WaitSemaMaybe( s_context_semaphore );
#endif // __PLAT_NGPS__

	if ( !pAlloc ) 					// set to 'default' allocator
	{
		pAlloc = mp_context->mp_alloc;
	}

	void* p_ret = pAlloc->reallocate_down( newSize, pOld );
	
	if ( p_ret )	// if allocation was successful
	{
		Allocator::s_set_id( p_ret );		// stamp ID; used by Mem::Handle
	}

#ifdef __PLAT_NGPS__
	SignalSemaMaybe( s_context_semaphore );
#endif // __PLAT_NGPS__

	return p_ret;
}

// Ken addition, for use by pip.cpp when it needs to reallocate the block of memory used
// by a pre file that has just been loaded in, so that the pre can be decompressed.
// This is for when the bottoms-up heap is being used.
void *Manager::ReallocateUp( size_t newSize, void *pOld, Allocator* pAlloc )
{   
#ifdef __PLAT_NGPS__
	WaitSemaMaybe( s_context_semaphore );
#endif // __PLAT_NGPS__

	if ( !pAlloc ) 					// set to 'default' allocator
	{
		pAlloc = mp_context->mp_alloc;
	}

	void* p_ret = pAlloc->reallocate_up( newSize, pOld );
	
	if ( p_ret )	// if allocation was successful
	{
		Allocator::s_set_id( p_ret );		// stamp ID; used by Mem::Handle
	}

#ifdef __PLAT_NGPS__
	SignalSemaMaybe( s_context_semaphore );
#endif // __PLAT_NGPS__

	return p_ret;
}

void *Manager::ReallocateShrink( size_t newSize, void *pOld, Allocator* pAlloc )
{   
#ifdef __PLAT_NGPS__
	WaitSemaMaybe( s_context_semaphore );
#endif // __PLAT_NGPS__

	if ( !pAlloc ) 					// set to 'default' allocator
	{
		pAlloc = mp_context->mp_alloc;
	}

	void* p_ret = pAlloc->reallocate_shrink( newSize, pOld );
	
	if ( p_ret )	// if allocation was successful
	{
		Allocator::s_set_id( p_ret );		// stamp ID; used by Mem::Handle
	}

#ifdef __PLAT_NGPS__
	SignalSemaMaybe( s_context_semaphore );
#endif // __PLAT_NGPS__

	return p_ret;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::PushMemoryMarker( uint32 uiID )
{
}
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::PopMemoryMarker( uint32 uiID )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


void	Manager::Delete( void* pAddr )
{
#ifdef __PLAT_NGPS__
	WaitSemaMaybe( s_context_semaphore );
#endif // __PLAT_NGPS__

	if( pAddr != NULL )
	{
		Allocator::s_free( pAddr );

#if 0
		// 000810 JAB: Modified s_free to return the allocator.
		if (mp_process_man)
		{
			mp_process_man->FreeEv( pAddr, p_alloc );
		}
#endif

	}

#ifdef __PLAT_NGPS__
	SignalSemaMaybe( s_context_semaphore );
#endif // __PLAT_NGPS__
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


bool	Manager::Valid( void* pAddr )
{
	if( pAddr != NULL )
	{
		return Allocator::s_valid( pAddr );
	}
	else
	{
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


size_t	Manager::GetAllocSize( void* pAddr )
{
	if( pAddr != NULL )
	{
		return Allocator::s_get_alloc_size( pAddr );
	}
	else
	{
		Dbg_MsgAssert( 0, ( "Trying to get the size of an invalid block" ) );
		return 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::PushContext( Allocator* alloc )
{
	
	Dbg_AssertType ( alloc, Allocator );

#ifdef __PLAT_NGPS__
	WaitSemaMaybe( s_context_semaphore );
#endif // __PLAT_NGPS__

//	printf ("Pushed context %d to %s\n",m_pushed_context_count, alloc->GetName());
//	DumpUnwindStack(20,0);

	Dbg_MsgAssert(m_pushed_context_count < vMAX_CONTEXT-1,("Pushed too many contexts"));
	mp_context = &m_contexts[m_pushed_context_count];	
	mp_context->mp_alloc = alloc;	
	m_pushed_context_count++;

#ifdef __PLAT_NGPS__
	SignalSemaMaybe( s_context_semaphore );
#endif // __PLAT_NGPS__

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::PopContext( void )
{
	

#ifdef __PLAT_NGPS__
	WaitSemaMaybe( s_context_semaphore );
#endif // __PLAT_NGPS__

	Dbg_MsgAssert( m_pushed_context_count,( "Heap stack underflow" ));
	
	m_pushed_context_count--;
	m_contexts[m_pushed_context_count].mp_alloc = (Mem::Allocator*)-1;	
	if (m_pushed_context_count)
	{
		mp_context = &m_contexts[m_pushed_context_count-1];		
	}
	else
	{
		mp_context = NULL;	 // stack has now been emptied
	}

#ifdef __PLAT_NGPS__
	SignalSemaMaybe( s_context_semaphore );
#endif // __PLAT_NGPS__
}

//GJ:  Context number doesn't seem to be used...
//Originally, it was used so that you could access the heaps from script,
//but now you can access the heaps using names, so the numbers aren't needed
//any more
//int Manager::GetContextNumber()
//{   
//	return mp_context->mp_alloc->GetNumber();		
//}

char * Manager::GetContextName()
{
	
	return mp_context->mp_alloc->GetName();		
}

Allocator* Manager::GetContextAllocator()
{
	
	return mp_context->mp_alloc;		
}

// Added by Ken, so that pip.cpp knows whether to try and expand a memory block up or down.
Allocator::Direction Manager::GetContextDirection()
{
	
	return mp_context->mp_alloc->GetDirection();		
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const char* Manager::GetHeapName( uint32 whichHeap )
{
	// first look through the named heaps...
	CNamedHeapInfo* pInfo = find_named_heap_info( whichHeap );
	if ( pInfo )
	{
		return pInfo->mp_heap_name;
	}
	
	switch ( whichHeap )
	{
		case 0xc7800b0:		// bottomupheap
			return "BottomUpHeap";
			break;
		case 0x8fdb68af:		// topdownheap
			return "TopDownHeap";
			break;
		case 0x62f3a0f3:		// frontendheap
			return "FrontEndHeap";
			break;
		case 0xe3551d2e:		// networkheap
			return "NetworkHeap";
			break;
		case 0x96d29d93:		// netmischeap
			return "NetMiscHeap";
			break;
		case 0xfcd5166b:		// internettopdownheap
			return "InternetTopDownHeap";
			break;
		case 0x90020867:		// internetbottomupheap
			return "InternetBottomUpHeap";
			break;
		case 0xfa33d9b:		// scriptheap
			return "ScriptHeap";
			break;
		case 0x70cb0238:		// debugheap
			return "DebugHeap";
			break;
		case 0xc3909393:		// skaterheap0
			return "Skater0";
			break;
		case 0xb497a305:		// skaterheap1
			return "Skater1";
			break;
		case 0x2d9ef2bf:		// skaterheap2
			return "Skater2";
			break;
		case 0x5a99c229:		// skaterheap3
			return "Skater3";
			break;
		case 0x572a9f4c:		// skatergeomheap0
			return "SkaterGeom0";
			break;
		case 0x202dafda:		// skatergeomheap1
			return "SkaterGeom1";
			break;
		case 0xb924fe60:		// skatergeomheap2
			return "SkaterGeom2";
			break;
		case 0xce23cef6:		// skatergeomheap3
			return "SkaterGeom3";
			break;
		case 0x8682d24:		// skaterinfoheap
			return "SkaterInfo";
			break;
        case 0x17ecb880:		// themeheap
			return "ThemeHeap";
            break;
        case 0x7ed56b49:		// CutsceneBottomUpHeap
			return "CutsceneBottomUp";
            break;
        case 0x25d71469:		// CutsceneTopDownHeap
			return "CutsceneTopDown";
            break;
		default:	// unrecognized heap
			Dbg_Assert ( 0 );
	}

	return "Unrecognized Heap";
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Heap* Manager::GetHeap( uint32 whichHeap )
{
	Heap* pHeap = NULL;

	// first look through the named heaps...
	CNamedHeapInfo* pInfo = find_named_heap_info( whichHeap );
	if ( pInfo )
	{
		return pInfo->mp_heap;
	}

	switch ( whichHeap )
	{
		case 0xc7800b0:		// bottomupheap
			pHeap = BottomUpHeap();
			break;
		case 0x8fdb68af:		// topdownheap
			pHeap = TopDownHeap();
			break;
		case 0x62f3a0f3:		// frontendheap
			pHeap = FrontEndHeap();
			break;
		case 0xe3551d2e:		// networkheap
			pHeap = NetworkHeap();
			break;
		case 0x96d29d93:		// netmischeap
			pHeap = NetMiscHeap();
			break;
		case 0xfcd5166b:		// internettopdownheap
			pHeap = InternetTopDownHeap();
			break;
		case 0x90020867:		// internetbottomupheap
			pHeap = InternetBottomUpHeap();
			break;
		case 0xfa33d9b:		// scriptheap
			pHeap = ScriptHeap();
			break;
		case 0x70cb0238:		// debugheap
			pHeap = DebugHeap();
			break;
		case 0xc3909393:		// skaterheap0
			pHeap = SkaterHeap(0);
			break;
		case 0xb497a305:		// skaterheap1
			pHeap = SkaterHeap(1);
			break;
		case 0x2d9ef2bf:		// skaterheap2
			pHeap = SkaterHeap(2);
			break;
		case 0x5a99c229:		// skaterheap3
			pHeap = SkaterHeap(3);
			break;
		case 0x572a9f4c:		// skatergeomheap0
			pHeap = SkaterGeomHeap(0);
			break;
		case 0x202dafda:		// skatergeomheap1
			pHeap = SkaterGeomHeap(1);
		break;
		case 0xb924fe60:		// skatergeomheap2
			pHeap = SkaterGeomHeap(2);
		break;
		case 0xce23cef6:		// skatergeomheap3
			pHeap = SkaterGeomHeap(3);
		break;
		case 0x8682d24:		// skaterinfoheap
			pHeap = SkaterInfoHeap();
		break;
        case 0x17ecb880:		// themeheap
			pHeap = ThemeHeap();
		break;
        case 0x7ed56b49:		// CutsceneBottomUpHeap
			pHeap = CutsceneBottomUpHeap();
		break;
        case 0x25d71469:		// CutsceneTopDownHeap
			pHeap = CutsceneTopDownHeap();
		break;
		default:	// unrecognized heap
			Dbg_MsgAssert ( 0, ( "Unrecognized heap %08x", whichHeap ) );
	}

	return pHeap;
}


Mem::Heap *Manager::CreateHeap( Region* region, Mem::Allocator::Direction dir, char* p_name)
{
	Mem::Heap * pHeap = new Mem::Heap(region, dir, p_name);
	// At this point we can maintain the heap list 
	m_heap_list[m_num_heaps++] = pHeap;
	return pHeap;
}

void Manager::RemoveHeap(Mem::Heap *pHeap)
{

#ifndef __PLAT_WN32__
	#ifdef	__NOPT_ASSERT__			 
	// Check first to see if there is something on the heap before it is deleted
	// Deleting a heap with stuff on it might indicate an error
	if (pHeap->mUsedBlocks.m_count)
	{
		printf ("Deleting a heap %s with %d used blocks still on it\n",pHeap->mp_name,pHeap->mUsedBlocks.m_count);
		#ifndef __PLAT_NGC__
		printf ("\n\nDumping Heap\n");
		MemView_DumpHeap(pHeap);
		printf ("\n\nAnalyzing Heap\n");
		MemView_AnalyzeHeap(pHeap);
		Dbg_MsgAssert(0, ("Deleting heap <%s> with %d used blocks still on it\n",pHeap->mp_name,pHeap->mUsedBlocks.m_count));
		
		#endif		// __PLAT_NGC__
	}
	#endif
#endif

	// remove from list of heaps
	for (int i=0;i<m_num_heaps;i++)
	{
		if (m_heap_list[i] == pHeap)
		{
			m_num_heaps--;	 							// chop off the last heaps
			m_heap_list[i] = m_heap_list[m_num_heaps]; 	// and insert if in the hole
			break;
		}
	}	
	
	delete pHeap;	
	
}

Mem::Heap *		Manager::FirstHeap()
{
	if (m_num_heaps)
	{
		return m_heap_list[0];
	}
	else
	{
		return NULL;
	}
}

Mem::Heap *		Manager::NextHeap(Mem::Heap * pHeap)
{
	for (int i = 0; i<m_num_heaps-1;i++)
	{
		if (m_heap_list[i] == pHeap)
		{
			return m_heap_list[i+1];
		}
	}
	return NULL;	
}

#ifdef	DEBUG_ADJUSTMENT
static void *p_adjustment;
#endif

void Manager::InitOtherHeaps()
{
	if (Config::Bootstrap())
	{
		mp_frontend_region = new Mem::AllocRegion( (BOOTSTRAP_FRONTEND_HEAP_SIZE) );	
	}
	else
	{
		mp_frontend_region = new Mem::AllocRegion( (FRONTEND_HEAP_SIZE) );	
	}
	
	printf ("allocated mp_frontend_region at %p\n",mp_frontend_region);
	
	// Network heap is now a top down heap on the the FE region
	// as they are mutually exclusive in their peak usages
	// since network heap is only really used in game, when FE usage is small
	mp_network_heap = CreateHeap( mp_frontend_region, Mem::Allocator::vTOP_DOWN, "Network" );
	printf ("Setup TOP_DOWN mp_network_heap at %p\n",mp_network_heap);
	mp_frontend_heap = CreateHeap( mp_frontend_region, Mem::Allocator::vBOTTOM_UP, "FrontEnd" );
	printf ("Setup mp_frontend_heap at %p\n",mp_frontend_heap);
	
#ifdef __PLAT_NGC__
	mp_audio_region = new Mem::AllocRegion( (AUDIO_HEAP_SIZE) );	
	printf ("allocated mp_audio_region at %p\n",mp_audio_region);
	mp_audio_heap = CreateHeap( mp_audio_region, Mem::Allocator::vBOTTOM_UP, "Audio" );
	printf ("Setup mp_audio_heap at %p\n",mp_audio_heap);
#endif		// __PLAT_NGC__ 

#ifdef	DEBUG_ADJUSTMENT
// allocate the script region off the debug heap
// and then allocate a block of size (SCRIPT_HEAP_SIZE-DEBUG_ADJUSTMENT)
// to bring regular memory usage back into line with non-debug builds
	mp_script_region 	= new ((void*)s_script_region_buffer) Region( nAlignUp( _script_heap_start ), nAlignDown( _script_heap_start+SCRIPT_HEAP_SIZE ) );
	printf ("allocated mp_script_region at %p\n",mp_script_region);
	mp_script_heap = CreateHeap( mp_script_region, Mem::Allocator::vBOTTOM_UP, "Script" );
	printf ("Setup mp_script_heap at %p\n",mp_script_heap);
	// and allocate the extra memory of the regular heap and forget about it
	p_adjustment = Mem::Malloc(SCRIPT_HEAP_SIZE-DEBUG_ADJUSTMENT);
#else	
	mp_script_region = new Mem::AllocRegion( (SCRIPT_HEAP_SIZE) );	
	printf ("allocated mp_script_region at %p\n",mp_script_region);
	mp_script_heap = CreateHeap( mp_script_region, Mem::Allocator::vBOTTOM_UP, "Script" );
	printf ("Setup mp_script_heap at %p\n",mp_script_heap);
#endif

#ifdef		__USE_PROFILER__
	mp_profiler_region = new Mem::AllocRegion( (PROFILER_HEAP_SIZE) );	
	printf ("allocated mp_profiler_region at %p\n",mp_profiler_region);
	mp_profiler_heap = CreateHeap( mp_profiler_region, Mem::Allocator::vBOTTOM_UP, "profiler" );
	printf ("Setup mp_profiler_heap at %p\n",mp_profiler_heap);
#endif

	mp_skater_info_region = new Mem::AllocRegion( (SKATERINFO_HEAP_SIZE) );	
	printf ("allocated mp_skater_info_region at %p\n",mp_skater_info_region);
	mp_skater_info_heap = CreateHeap( mp_skater_info_region, Mem::Allocator::vBOTTOM_UP, "skt_info" );
	printf ("Setup mp_skater_info_heap at %p\n",mp_skater_info_heap);

    mp_theme_region = new Mem::AllocRegion( (THEME_HEAP_SIZE) );	
	printf ("allocated mp_theme_region at %p\n",mp_theme_region);
	mp_theme_heap = CreateHeap( mp_theme_region, Mem::Allocator::vBOTTOM_UP, "theme" );
	printf ("Setup mp_theme_heap at %p\n",mp_theme_heap);

	// Allocate the permanent skater heap(s)
	
	for (int i = 0; i< NUM_PERM_SKATER_HEAPS; i++)
	{
		mp_skater_region[i] = new Mem::AllocRegion( (SKATER_HEAP_SIZE) );	
		printf ("allocated mp_skater_region at %p\n",mp_skater_region[i]);
		mp_skater_heap[i] = CreateHeap( mp_skater_region[i], Mem::Allocator::vBOTTOM_UP, "skater" );
		printf ("Setup mp_skater_heap at %p\n",mp_skater_heap[i]);

		mp_skater_geom_region[i] = new Mem::AllocRegion( (SKATER_GEOM_HEAP_SIZE) );
		printf ("allocated mp_skater_geom_region at %p\n",mp_skater_geom_region[i]);
		mp_skater_geom_heap[i] = CreateHeap( mp_skater_geom_region[i], Mem::Allocator::vBOTTOM_UP, "skater_geom" );
		printf ("Setup mp_skater_geom_heap at %p\n",mp_skater_geom_heap[i]);
	}
	// clear other temp heaps to null
	for (int heap = NUM_PERM_SKATER_HEAPS; heap < NUM_SKATER_HEAPS; heap++)
	{
		mp_skater_region[heap] = NULL;			
		mp_skater_heap[heap] = NULL;	  

		mp_skater_geom_region[heap] = NULL;
		mp_skater_geom_heap[heap] = NULL;
	}
}

void Manager::InitNetMiscHeap()
{
	if( mp_net_misc_region == NULL )
	{

		//#ifndef	__NOPT_ASSERT__
		#if	1		// always allocate internet heap normally
		// normally the internet heap just goes on the top down heap
		sp_instance->PushContext( sp_instance->mp_bot_heap );
		#else
		// but with assertions there is not enough room, so put it on the debug heap
		if (Config::GotExtraMemory())
		{
			sp_instance->PushContext( sp_instance->mp_debug_heap );			
		}
		else
		{
			// running on a regular PS2, allow them to try, but will probably fail later with out of memory
			sp_instance->PushContext( sp_instance->mp_bot_heap );
		}
		#endif		
		
		mp_net_misc_region = new Mem::AllocRegion( (NETMISC_HEAP_SIZE) );	
		mp_net_misc_heap = CreateHeap( mp_net_misc_region, Mem::Allocator::vBOTTOM_UP, "NetMisc" );

		sp_instance->PopContext();
	}
}

void Manager::DeleteNetMiscHeap()
{
	if( mp_net_misc_region )
	{
		RemoveHeap( mp_net_misc_heap );
		delete mp_net_misc_region;
		mp_net_misc_region = NULL;
	}
}

void Manager::InitInternetHeap()
{
	if( mp_internet_region == NULL )
	{

		//#ifndef	__NOPT_ASSERT__
		#if	1		// always allocate internet heap normally
		// normally the internet heap just goes on the top down heap
		sp_instance->PushContext( sp_instance->mp_bot_heap );
		#else
		// but with assertions there is not enough room, so put it on the debug heap
		if (Config::GotExtraMemory())
		{
			sp_instance->PushContext( sp_instance->mp_debug_heap );			
		}
		else
		{
			// running on a regular PS2, allow them to try, but will probably fail later with out of memory
			sp_instance->PushContext( sp_instance->mp_bot_heap );
		}
		#endif		
		
		mp_internet_region = new Mem::AllocRegion( (INTERNET_HEAP_SIZE) );	
		mp_internet_top_heap = CreateHeap( mp_internet_region, Mem::Allocator::vTOP_DOWN, "InternetTopDown" );
		mp_internet_bottom_heap = CreateHeap( mp_internet_region, Mem::Allocator::vBOTTOM_UP, "InternetBottomUp" );

		sp_instance->PopContext();
	}
}

void Manager::DeleteInternetHeap()
{
	if( mp_internet_region )
	{
		RemoveHeap( mp_internet_top_heap );
		RemoveHeap( mp_internet_bottom_heap );
		delete mp_internet_region;
		mp_internet_region = NULL;
	}
}

void Manager::InitCutsceneHeap( int heap_size )
{
	// Note that it will create the cutscene heap on the current mem context...

	if ( mp_cutscene_region == NULL )
	{
		// put it on the top-down heap, because it's used only temporarily
//		sp_instance->PushContext( sp_instance->mp_top_heap );
		
		mp_cutscene_region = new Mem::AllocRegion( heap_size );

		Dbg_MsgAssert( mp_cutscene_bottom_heap == NULL, ( "CutsceneBottomUpHeap already exists" ) );
		mp_cutscene_bottom_heap = CreateHeap( mp_cutscene_region, Mem::Allocator::vBOTTOM_UP, "CutsceneBottomUp" );
		
		Dbg_MsgAssert( mp_cutscene_top_heap == NULL, ( "CutsceneTopDownHeap already exists" ) );
		mp_cutscene_top_heap = CreateHeap( mp_cutscene_region, Mem::Allocator::vTOP_DOWN, "CutsceneTopDown" );

//		sp_instance->PopContext();
	}
}

void Manager::DeleteCutsceneHeap()
{
	if ( mp_cutscene_region )
	{
		RemoveHeap( mp_cutscene_bottom_heap );
		mp_cutscene_bottom_heap = NULL;
		
		RemoveHeap( mp_cutscene_top_heap );
		mp_cutscene_top_heap = NULL;

		delete mp_cutscene_region;
		mp_cutscene_region = NULL;
	}
}


void Manager::InitDebugHeap()
{
	#ifdef	__PLAT_NGPS__
	// The Debug heap is allocated directly from debug memory (>32MB on PS2)
	// as such, it should only ever be used on the TOOL (T10K) debug stations, or equivalents on other platforms 
	mp_debug_region 	= new ((void*)s_debug_region_buffer) Region( nAlignUp( _debug_heap_start ), nAlignDown( _debug_heap_start+DEBUG_HEAP_SIZE ) );
	mp_debug_heap = CreateHeap( mp_debug_region, Mem::Allocator::vBOTTOM_UP, "debug" );
	#endif
}

void Manager::InitSkaterHeaps(int players)
{
	printf ("Init Skater Heaps\n");
	   
	// some game modes specify 8 players, however 4 of those
	// are observers ??? 
	// anyway, for now, just don't allow them to create more heaps than the max
	// it will assert later if you try to access a heap that has not been created							   
	if (players > NUM_SKATER_HEAPS)
	{
		players = NUM_SKATER_HEAPS;
	}
	
	// Initialize the other skater heaps
	for (int heap = NUM_PERM_SKATER_HEAPS; heap < players; heap++)
	{
		Dbg_MsgAssert(!mp_skater_region[heap],( "Skater region %d is still active!!!\n",heap));
		Dbg_MsgAssert(!mp_skater_heap[heap],( "Skater heap %d is still active!!!\n",heap));
		mp_skater_region[heap] = new Mem::AllocRegion( (SKATER_HEAP_SIZE) );	
		printf ("EXTRA: allocated mp_skater_region at %p\n",mp_skater_region[heap]);
		mp_skater_heap[heap] = CreateHeap( mp_skater_region[heap], Mem::Allocator::vBOTTOM_UP, "skaterX" );
		printf ("EXTRA: Setup mp_skater_heap at %p\n",mp_skater_heap[heap]);
	
		Dbg_MsgAssert(!mp_skater_geom_region[heap],( "Skater geom region %d is still active!!!\n",heap));
		Dbg_MsgAssert(!mp_skater_geom_heap[heap],( "Skater geom heap %d is still active!!!\n",heap));
		mp_skater_geom_region[heap] = new Mem::AllocRegion( (SKATER_GEOM_HEAP_SIZE) );	
		printf ("EXTRA: allocated mp_skater_region at %p\n",mp_skater_geom_region[heap]);
		mp_skater_geom_heap[heap] = CreateHeap( mp_skater_geom_region[heap], Mem::Allocator::vBOTTOM_UP, "skaterGeomX" );
		printf ("EXTRA: Setup mp_skater_geom_heap at %p\n",mp_skater_geom_heap[heap]);
	}
	
	printf ("END Init Skater Heaps\n");
}

// Delete the temporary skater heaps
void Manager::DeleteSkaterHeaps()
{
	for (int i=NUM_PERM_SKATER_HEAPS;i<NUM_SKATER_HEAPS;i++)
	{
		if (mp_skater_region[i])
		{		
			printf ("DELETING SKATER HEAP %d\n",i);
			RemoveHeap(mp_skater_heap[i]);
			delete mp_skater_region[i];
			mp_skater_heap[i] = NULL;
			mp_skater_region[i] = NULL;
		}

		if (mp_skater_geom_region[i])
		{
			printf ("DELETING SKATER GEOM HEAP %d\n",i);
			RemoveHeap(mp_skater_geom_heap[i]);
			delete mp_skater_geom_region[i];
			mp_skater_geom_heap[i] = NULL;
			mp_skater_geom_region[i] = NULL;
		}
	}
}

void Manager::DeleteOtherHeaps()
{
	RemoveHeap(mp_frontend_heap);
	delete mp_frontend_region;
	RemoveHeap(mp_script_heap);
	delete mp_script_region;
	RemoveHeap(mp_network_heap);
	delete mp_network_region;
#ifdef		__USE_PROFILER__
	RemoveHeap(mp_profiler_heap);
	delete mp_debug_region;
#endif	
	RemoveHeap(mp_skater_info_heap);
	delete mp_skater_info_region;
	// Deallocate the permanent skater heap(s)
	for (int i = 0; i< NUM_PERM_SKATER_HEAPS; i++)
	{
		RemoveHeap(mp_skater_heap[i]);
        delete mp_skater_region[i];
		
		RemoveHeap(mp_skater_geom_heap[i]);
		delete mp_skater_geom_region[i];
	}
	DeleteSkaterHeaps();		// just in case we did some preemptive exit
}
														 
														 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::sSetUp( void )
{
	

	if ( !sp_instance )
	{
		sp_instance = new ((void*)s_manager_buffer) Manager;

		sp_instance->PushContext( sp_instance->mp_bot_heap );		// make bottom-up heap default
    
//		sp_instance->InitOtherHeaps();							
							

	}
	else
	{
		Dbg_Warning( "Already Initialized!" );
	}
}

// K: Separated this out from sSetUp because this needs to be called from main(), after
// the config manager has initialised, because it must only be called if extra memory is
// available, and we only know that after Config::Init has been called.
// sSetUp is called from pre_main, and the config manager cannot be called from there 
// because it needs the command line args.
void Manager::sSetUpDebugHeap( void )
{
	if ( sp_instance )
	{
		sp_instance->InitDebugHeap();
	}
	else
	{
		Dbg_MsgAssert(0,("Called sSetUpDebugHeap when mem manager not initialized!"));
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::sCloseDown( void )
{
	

	if ( sp_instance )
	{
		
#ifndef __PLAT_WN32__
		sp_instance->DeleteOtherHeaps();							
#endif		
		sp_instance->PopContext();

		sp_instance->~Manager();
		sp_instance = NULL;
	}
	else
	{
		Dbg_Warning( "Not Initialized!" );
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


void Manager::RegisterPcsMemMan( Pcs::Manager* pReg ) 
{
	

	Dbg_Assert( mp_process_man == NULL );	// should not initialize twice.

	mp_process_man = pReg;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CNamedHeapInfo* Manager::find_named_heap_info( uint32 name )
{
	for ( int i = 0; i < NUM_NAMED_HEAPS; i++ )
	{
		if ( m_named_heap_info[i].m_used && m_named_heap_info[i].m_name == name )
		{
			return &m_named_heap_info[i];
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Heap* Manager::NamedHeap( uint32 name, bool assertOnFail )
{
	CNamedHeapInfo* pInfo = find_named_heap_info( name );

	if ( pInfo )
	{
		return pInfo->mp_heap;
	}
		
	Dbg_MsgAssert( !assertOnFail, ( "Couldn't find named heap %08x", name ) );
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::InitNamedHeap( uint32 name, uint32 size, const char* pHeapName )
{
	CNamedHeapInfo* pNamedHeapInfo = NULL;

	for ( int i = 0; i < NUM_NAMED_HEAPS; i++ )
	{
		if ( !m_named_heap_info[i].m_used )
		{
			pNamedHeapInfo = &m_named_heap_info[i];
			break;
		}
	}

	Dbg_MsgAssert( pNamedHeapInfo, ( "No more free named heaps (Increase NUM_NAMED_HEAPS from %d)!", NUM_NAMED_HEAPS ) );

	// set the correct name
	pNamedHeapInfo->m_name = name;
	Dbg_MsgAssert( strlen(pHeapName) < CNamedHeapInfo::vMAX_HEAP_NAME_LEN, ( "Heap name %s is too long", pHeapName ) );
	strcpy( pNamedHeapInfo->mp_heap_name, pHeapName );
	
	Dbg_MsgAssert(!pNamedHeapInfo->mp_region, ( "Named region is still active" ) );
	pNamedHeapInfo->mp_region =	new Mem::AllocRegion( size );	
	Dbg_Message( "EXTRA: Allocated pNamedHeapInfo->mp_region at %p", pNamedHeapInfo->mp_region );
	
	Dbg_MsgAssert(!pNamedHeapInfo->mp_heap, ("Named heap is still active"));
	pNamedHeapInfo->mp_heap = CreateHeap( pNamedHeapInfo->mp_region, 
										  Mem::Allocator::vBOTTOM_UP, 
										  pNamedHeapInfo->mp_heap_name );
	Dbg_Message( "EXTRA: Setup pNamedHeapInfo->mp_region at %p", pNamedHeapInfo->mp_heap );

	pNamedHeapInfo->m_used = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Manager::DeleteNamedHeap( uint32 name, bool assertOnFail )
{
	CNamedHeapInfo* pInfo = find_named_heap_info( name );

	if ( pInfo )
	{
		Dbg_Message( "Deleting named heap %s", pInfo->mp_heap_name );
		Dbg_Assert( pInfo->mp_region );
		Dbg_Assert( pInfo->mp_heap );
		RemoveHeap( pInfo->mp_heap );
		delete pInfo->mp_region;
		pInfo->mp_region = NULL;
		pInfo->mp_heap = NULL;
		pInfo->m_name = 0;
		pInfo->m_used = false;
		return true;
	}
	else
	{
		Dbg_MsgAssert( !assertOnFail, ( "Couldn't find named heap %08x to delete", name ) );
	}
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void*	Malloc( size_t size )
{
	
	
	void *v =  Mem::Manager::sHandle().New( size, true );
	
	return v;
}

int		Available()
{
	return Mem::Manager::sHandle().Available(); 
}

void* ReallocateDown( size_t newSize, void *pOld )
{
	return Mem::Manager::sHandle().ReallocateDown(newSize,pOld);
}

void* ReallocateUp( size_t newSize, void *pOld )
{
	return Mem::Manager::sHandle().ReallocateUp(newSize,pOld);
}

void* ReallocateShrink( size_t newSize, void *pOld )
{
	return Mem::Manager::sHandle().ReallocateShrink(newSize,pOld);
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Free( void* pAddr )
{
	

	Mem::Manager::sHandle().Delete( pAddr );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Valid( void* pAddr )
{	
	return Mem::Manager::sHandle().Valid( pAddr );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

size_t	GetAllocSize( void* pAddr )
{
	return Mem::Manager::sHandle().GetAllocSize( pAddr );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void*	Realloc( void* mem, size_t newSize )
{
	/* should really add resize functions to do this more efficiently */

	void* ptr = NULL;

	if ( newSize )
	{   
//		Mem::Manager::sHandle().PushContext(Manager::sHandle().TopDownHeap());
		ptr = Mem::Manager::sHandle().New( newSize, true );
//		Mem::Manager::sHandle().PopContext();	
	}

	if ( mem )
	{
		if ( ptr )
		{
			memmove ( ptr, mem, newSize ); 
		}

		Mem::Manager::sHandle().Delete( mem );
	}
	
	return ptr;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void*	Calloc( size_t numObj, size_t sizeObj )
{
	
	
	return Mem::Manager::sHandle().New(( numObj * sizeObj ), true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// enable multiple threads to access the memory manager at once
void	SetThreadSafe(bool safe)
{
	#ifdef	__PLAT_NGPS__
		s_use_semaphore = safe;	
	#endif
}

bool	IsThreadSafe( void )
{
#ifdef	__PLAT_NGPS__
	return s_use_semaphore;
#else
	return false;
#endif
}

#ifdef __NOPT_ASSERT__

#define	MAX_PROFILE_NAME 64
#define	MAX_NUM_PROFILES  5000
#define	MAX_LEVELS	  32

class	CMemProfile
{		  
public:
	char	m_type[MAX_PROFILE_NAME];
	int		m_blocks;
	int		m_size;
	int		m_depth;
	int		m_count;		// number of pushes of same nam, same depth context
	int		m_link;			// link forward to profile that has merged with this profile
};


//static		CMemProfile				s_profile_list[MAX_NUM_PROFILES];		// a flat array of all profiles
static		CMemProfile			*	s_profile_list;		// a dynamically allocated flat array of all profiles
	
static		CMemProfile	*			s_profile_stack[MAX_LEVELS];		// stack of pushed profiles 
	
static		int						s_profile_count = 0;
	
static		CMemProfile*			sp_current_profile = NULL;							// current entry in profile list (will need to pop back....
static		int						s_profile_stack_depth = 0;						// depth in stack (index into m_profile_stack[])
static		int						s_next_profile = 0;								// index into array

//static 		int						s_launched = false;		// true after we've laucnhed the level once

static		bool					s_active = true;

void	PushMemProfile(char *p_type)
{
	if( s_active )
	{
	
		if (!s_profile_list)
		{
			if (Config::GotExtraMemory())
			{
				Mem::Manager::sHandle().PushContext(Manager::sHandle().DebugHeap());
				s_profile_list = (CMemProfile *)Mem::Malloc(MAX_NUM_PROFILES *sizeof(CMemProfile));
				Mem::Manager::sHandle().PopContext();
				
			}
			else
			{
				s_active = false;			// no extra memory, or maybe it's an X-Box....
				return;
			}
		}
		
		if (s_next_profile >= MAX_NUM_PROFILES)
		{
			s_active = false;
			return;
		}

/*		
		if (!strcmp("LaunchLevel",p_type))
		{
			if (s_launched)
			{
				s_active = false;
				return;
			}
			s_launched = true;
		}
*/
							  
		// if we have a current profile, then push it
		if (sp_current_profile)
		{
			Dbg_MsgAssert(s_profile_stack_depth < MAX_LEVELS,("mem profile stack overflow, unmatched push?"));
			s_profile_stack[s_profile_stack_depth++] = sp_current_profile;
		}
		
		// get a new profile from the list
		Dbg_MsgAssert(s_next_profile < MAX_NUM_PROFILES,("mem profile heap overflow, too many pushes"));
		sp_current_profile = &s_profile_list[s_next_profile++];




		// just copy over the memory containing the name												  
		char *p = &(sp_current_profile->m_type[0]);
		char *q = (char*) p_type;
		for (int i=0;i<MAX_PROFILE_NAME;i++)
		{
			*p++ = *q++;			
		}
		// and set the counters to zero
		sp_current_profile->m_blocks = 0;
		sp_current_profile->m_size = 0;
		sp_current_profile->m_depth = s_profile_stack_depth;
		sp_current_profile->m_count = 1;
		sp_current_profile->m_link = 0;

		// Then, search back through the list to see if there 
		// are any entries att he same level that have this same string
		// if so, then zero the old instance, and add the size and count to this one
		// there should only be one, as any other one would already be zeroed
		int	check = s_next_profile-2;
		while (check>0
				&& s_profile_list[check].m_depth == s_profile_stack_depth)
		{
			if (s_profile_list[check].m_depth == s_profile_stack_depth)
			{
				// same depth
				if (strcmp(s_profile_list[check].m_type,p_type) == 0)
				{
					// same string, so bring this one forward to add to this
					sp_current_profile->m_blocks += s_profile_list[check].m_blocks;
					sp_current_profile->m_size += s_profile_list[check].m_size;
					sp_current_profile->m_count += s_profile_list[check].m_count;
					// and zero out the old one					
					s_profile_list[check].m_blocks = 0;
					s_profile_list[check].m_size = 0;
					// Patch in the index of the new profile,
					// so late deletions can be accounted for
					s_profile_list[check].m_link = s_profile_stack_depth - 1;
					break;
				}
			}
			check--;
		}


	}
}


/******************************************************************/

void	PopMemProfile()
{
	if( s_active)
	{
		Dbg_MsgAssert(sp_current_profile,("Popped one more memory profile than we pushed"));

		// set time on the current profile		
//		sp_current_profile->m_end_time = Tmr::GetTimeInUSeconds();
		
		if ( ! s_profile_stack_depth)
		{
			// nothing left on stack, so set current profile to NULL
			sp_current_profile = NULL;
			// update the count of valid profiles
			s_profile_count = s_next_profile;
		}
		else
		{
			// get the last profile pushed on the stack
			sp_current_profile = s_profile_stack[--s_profile_stack_depth]; 
		}
	}
}


// given an index in the the mem profile list
// then get the total size of all allocations
// at this level, or below
// up the next entry at the same depth, or the end of the list
int		total_size(int n)
{
	
	int size = s_profile_list[n].m_size;
	int depth = s_profile_list[n].m_depth;
	n++;
	while (n < s_next_profile && s_profile_list[n].m_depth > depth)
	{
		size += s_profile_list[n].m_size;
		n++;
	}
	return size;
					
}

#ifndef __PLAT_WN32__
// dump the memory profile in a tree format, like
//
//  level stuff     100000
//     peds           5000
//     cars          23000
//    other          72000
//  skater stuff    ...... 
void	DumpMemProfile(int level, char *p_type)
{
	char buf[512];
	if( s_active )
	{
	


		printf ("\nDumping Memory Profile\n");
		printf ("There are %d mem profile contexts\n",s_next_profile);
		for (int i=0;i<s_next_profile;i++)
		{
			if (s_profile_list[i].m_depth <= level)
			{
				if (total_size(i))
				{
					sprintf (buf,"%2d: ",s_profile_list[i].m_depth);
					dump_printf(buf);
					for (int tab = 0;tab < s_profile_list[i].m_depth;tab++)
					{
						sprintf(buf,"   ");
						dump_printf(buf);
					}
					sprintf (buf,">%10s____",Str::PrintThousands(total_size(i)));
					dump_printf(buf);
					if (s_profile_list[i].m_count == 1)
					{
						sprintf (buf,"%s\n",s_profile_list[i].m_type);
					}
					else
					{
						sprintf (buf,"%s (%d) (avg: %s)\n",s_profile_list[i].m_type,s_profile_list[i].m_count,Str::PrintThousands(total_size(i)/s_profile_list[i].m_count));
					}
					dump_printf(buf);
					if (s_profile_list[i].m_depth < level && s_profile_list[i].m_size && total_size(i) != s_profile_list[i].m_size)
					{
						sprintf (buf,"%2d: ",s_profile_list[i].m_depth+1);
						dump_printf(buf);
						for (int tab2 = 0;tab2 < s_profile_list[i].m_depth+1;tab2++)
						{
							sprintf(buf,"   ");
							dump_printf(buf);
						}
						sprintf (buf,">%10s____",Str::PrintThousands(s_profile_list[i].m_size));
						dump_printf(buf);
						sprintf (buf,"Untracked %s\n",s_profile_list[i].m_type);			
						dump_printf(buf);
						
					}
				}
			}
		}
	}
	else
	{
		printf ("Mem Profiler not active, probably overflowed, try restarting...\n");
	}
}
#endif// __PLAT_WN32__
void	AllocMemProfile(Allocator::BlockHeader* p_block)
{
	if( s_active )
	{
		if (sp_current_profile)
		{
			sp_current_profile->m_blocks++;
			
			// If it's on the debug heap, then set size to zero to avoid confusion
			if (Mem::Manager::sHandle().GetContextAllocator() != Mem::Manager::sHandle().DebugHeap())
			{
				sp_current_profile->m_size += p_block->mSize;
			}
			
			p_block->mp_profile = sp_current_profile;
		}
	}
}

void	FreeMemProfile(Allocator::BlockHeader* p_block)
{
	if( s_active )
	{
		
		CMemProfile	*	p_profile  = p_block->mp_profile;						  
		if (p_profile)
		{
//			Dbg_MsgAssert(p_profile->m_blocks,("mutli-block freed out of context"));
			// Skip over any that have been combined, until we find the final combined block
			while (p_profile->m_link != 0)
			{
				p_profile = &s_profile_list[p_profile->m_link];
			}
			p_profile->m_blocks--;
			p_profile->m_size -= p_block->mSize;
			p_profile = NULL;
		}
	}
}

#else

void	PushMemProfile(char *p_type)
{
}

void	PopMemProfile()
{
}

#ifndef __PLAT_WN32__
void	DumpMemProfile(int level, char *p_type)
{
}
#endif// __PLAT_WN32__

void	AllocMemProfile(Allocator::BlockHeader* p_block)
{
}

void	FreeMemProfile(Allocator::BlockHeader* p_block)
{
}

#endif __NOPT_ASSERT__


} // namespace Mem

