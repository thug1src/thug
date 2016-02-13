/*****************************************************************************
**																			**
**			              Neversoft Entertainment	                        **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Sys Library												**
**																			**
**	Module:			Memory Manager (Mem)									**
**																			**
**	Created:		03/20/00	-	mjb										**
**																			**
**	File name:		core/sys/mem/memman.h									**
**																			**
*****************************************************************************/

#ifndef	__SYS_MEM_MEMMAN_H
#define	__SYS_MEM_MEMMAN_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/singleton.h>
#include <core/list.h>
#include <sys\mem\region.h>
#include "heap.h"
#include "alloc.h"
//#ifndef __PLAT_XBOX__

#ifndef	__SYS_MEM_MEMPTR_H
#	include "memptr.h"
#endif
//#endif
#include "handle.h"

#if 0
#ifdef __PLAT_WN32__
#include "mem/wn32/p_memman.h"
#else
#ifdef __PLAT_NGPS__
#include "mem/ngps/p_memman.h"
#else
#ifdef __PLAT_NGC__
#include "mem/ngc/p_memman.h"
#else
#error Unsupported Platform
#endif
#endif
#endif
#endif

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

// redefine the standard memory library functions, so they give us errors


#define	NUM_PERM_SKATER_HEAPS	1
#define	NUM_SKATER_HEAPS		8
#define NUM_NAMED_HEAPS			4

#define		DEBUG_HEAP_SIZE				32767*1024		// 1K short of 32MB

		
namespace Pcs
{
	class Manager;
}

namespace Mem
{

/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/

class CNamedHeapInfo
{
public:
	enum
	{
		vMAX_HEAP_NAME_LEN = 128
	};

	CNamedHeapInfo()
	{
		mp_region = NULL;
		mp_heap = NULL;
		m_name = 0;
		m_used = false;
	}

public:
	Region*		mp_region;
	Heap*		mp_heap;
	uint32		m_name;
	char		mp_heap_name[vMAX_HEAP_NAME_LEN];
	bool		m_used;
};

class Manager : public Spt::Class			
{
	
public:

	enum
	{
		vMAX_CONTEXT = 16,
		vMAX_HEAPS = 32
	};

	void						PushContext( Allocator* alloc );
	void						PopContext( void );

	void*						New( size_t size, bool assert_on_fail = true, Allocator* pAlloc = NULL );
	int							Available();
	void*						ReallocateDown( size_t newSize, void *pOld, Allocator* pAlloc = NULL );
	void*						ReallocateUp( size_t newSize, void *pOld, Allocator* pAlloc = NULL );
	void*						ReallocateShrink( size_t newSize, void *pOld, Allocator* pAlloc = NULL );
	void						Delete( void* pAddr );
	bool						Valid( void* pAddr );
	size_t						GetAllocSize( void* pAddr );
								
	Heap*						TopDownHeap( void ) 		{ return mp_top_heap; }
	Heap*						BottomUpHeap( void )  		{ return mp_bot_heap; }
	Heap*						FrontEndHeap( void ) 		{ return mp_frontend_heap; }
	Heap*						ScriptHeap( void ) 			{ return mp_script_heap; }
	Heap*						NetworkHeap( void ) 		{ return mp_network_heap; }
	Heap*						NetMiscHeap( void ) 		{ return mp_net_misc_heap; }
	Heap*						ProfilerHeap( void ) 		{ return mp_profiler_heap; }
	Heap*						DebugHeap( void ) 			{ return mp_debug_heap; }
	Heap*						SkaterHeap( int n ) 		{ return mp_skater_heap[n]; }
	Heap*						SkaterInfoHeap( ) 			{ return mp_skater_info_heap; }
	Heap*						SkaterGeomHeap( int n ) 	{ return mp_skater_geom_heap[n]; }
	Heap*						InternetTopDownHeap( void )	{ return mp_internet_top_heap; }
	Heap*						InternetBottomUpHeap( void ){ return mp_internet_bottom_heap; }
    Heap*						ThemeHeap( void )           { return mp_theme_heap; }
	Heap*						CutsceneTopDownHeap( void )	{ return mp_cutscene_top_heap; }
	Heap*						CutsceneBottomUpHeap( void ){ return mp_cutscene_bottom_heap; }
#ifdef __PLAT_NGC__
	Heap*						AudioHeap( void )			{ return mp_audio_heap; }
#endif	
	Heap*						NamedHeap( uint32 name, bool assertOnFail = true );

	void						RegisterPcsMemMan( Pcs::Manager* );

	/* Global versions for all memory allocations */
	void						PushMemoryMarker( uint32 uiID );
	void						PopMemoryMarker( uint32 uiID );

	static void					sSetUp( void );
	static void					sSetUpDebugHeap( void );
	
	static void					sCloseDown( void );
	static Manager&				sHandle( void );

	
	void 						InitOtherHeaps();
	void 						DeleteOtherHeaps();

	void						InitInternetHeap();
	void						DeleteInternetHeap();

	void						InitNetMiscHeap();
	void						DeleteNetMiscHeap();

	void						InitCutsceneHeap(int heap_size);
	void						DeleteCutsceneHeap();

	void						InitDebugHeap();

	void 						InitSkaterHeaps(int players);
	void 						DeleteSkaterHeaps();

	void						InitNamedHeap(uint32 name, uint32 size, const char* pHeapName);
	bool						DeleteNamedHeap(uint32 name, bool assertOnFail = true);
	
	const char*					GetHeapName( uint32 whichHeap );
	Heap *						GetHeap( uint32 whichHeap );

	Heap *						CreateHeap( Region* region, Mem::Allocator::Direction dir /*= Mem::Allocator::vBOTTOM_UP*/, char* p_name );//= "unknown heap" );
	void						RemoveHeap( Heap * pHeap); 
	Heap *						FirstHeap();
	Heap *						NextHeap(Heap * pHeap);
	

//	int 						GetContextNumber();
	char *						GetContextName();
	Allocator::Direction		GetContextDirection();
	Allocator*					GetContextAllocator();


								~Manager( void );
private :
								Manager( void );								   
																 
	Mem::Heap *					m_heap_list[vMAX_HEAPS]; 
	int							m_num_heaps;   							

	static char 				s_manager_buffer[];
	static char 				s_region_buffer[];
	static char 				s_debug_region_buffer[];
	static char 				s_script_region_buffer[];
	static char 				s_profiler_region_buffer[];
	static char 				s_debug_heap_buffer[];
	static char 				s_top_heap_buffer[];
	static char 				s_bot_heap_buffer[];
	static bool					s_initialized;
	static Manager*				sp_instance;
		
	class MemManContext
	{
	
		friend class Manager;
	
	private:
		Allocator*						mp_alloc;
	};


	Ptr< MemManContext > 		mp_context;

	// Mick: Contexts are now statically allocated off this 
	// array, rather than off the heap, as that was causing fragmentation
	// in rare but crash-worthy circumstances			
	MemManContext				m_contexts[vMAX_CONTEXT];
	int							m_pushed_context_count;


	Region*						mp_region;
	Heap*						mp_top_heap;
	Heap*						mp_bot_heap;
	
	Region*						mp_frontend_region;
	Heap*						mp_frontend_heap;

	Region*						mp_script_region;
	Heap*						mp_script_heap;

	Region*						mp_network_region;
	Heap*						mp_network_heap;

	Region*						mp_net_misc_region;
	Heap*						mp_net_misc_heap;

	Region*						mp_internet_region;
	Heap*						mp_internet_top_heap;
	Heap*						mp_internet_bottom_heap;

	Region*						mp_cutscene_region;
	Heap*						mp_cutscene_top_heap;
	Heap*						mp_cutscene_bottom_heap;

#ifdef __PLAT_NGC__
	Region*						mp_audio_region;
	Heap*						mp_audio_heap;
#endif		// __PLAT_NGC__

	Region*						mp_debug_region;
	Heap*						mp_debug_heap;

	Region*						mp_profiler_region;
	Heap*						mp_profiler_heap;

	Region*						mp_skater_region[NUM_SKATER_HEAPS];
	Heap*						mp_skater_heap[NUM_SKATER_HEAPS];
	
	Region*						mp_skater_geom_region[NUM_SKATER_HEAPS];
	Heap*						mp_skater_geom_heap[NUM_SKATER_HEAPS];

	CNamedHeapInfo				m_named_heap_info[NUM_NAMED_HEAPS];

	Region*						mp_skater_info_region;
	Heap*						mp_skater_info_heap;

    Region*						mp_theme_region;
	Heap*						mp_theme_heap;

	Pcs::Manager*				mp_process_man;
	uint						m_current_id;

protected:
	CNamedHeapInfo*				find_named_heap_info( uint32 name );
};

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

extern "C"
{

int		Available();
void*	Malloc( size_t size );
void	Free( void* mem );
bool	Valid( void* pAddr );
size_t	GetAllocSize( void* pAddr );
void*	ReallocateDown( size_t newSize, void *pOld );
void*	ReallocateUp( size_t newSize, void *pOld );
void*	ReallocateShrink( size_t newSize, void *pOld );
void*	Realloc( void* mem, size_t newSize );
void*	Calloc( size_t numObj, size_t sizeObj );
}

void	PushMemProfile(char *p_type);
void	PopMemProfile();
void	DumpMemProfile(int level, char *p_type=NULL);
void	AllocMemProfile(Allocator::BlockHeader* p_block);
void	FreeMemProfile(Allocator::BlockHeader* p_block);

void	SetThreadSafe(bool safe);
bool	IsThreadSafe( void );




/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

inline Manager&	Manager::sHandle( void )
{
	

	Dbg_AssertType( sp_instance, Manager );

	return *sp_instance;
}


} // namespace Mem


// Some debug functions
bool dump_open(const char *name);
void dump_printf(char *p);
void dump_close();
extern int dumping_printfs;



#endif  // __SYS_MEM_MEMMAN_H
