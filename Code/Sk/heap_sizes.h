#ifndef __SK_HEAP_SIZES_H
#define __SK_HEAP_SIZES_H

// Sizes of heaps

#define	EXTRA 1000000


#ifdef __PLAT_WN32__

#define	_SCRIPT_HEAP_SIZE				(1024)
#define	SCRIPT_CACHE_HEAP_SIZE			(1024)
#define	FRONTEND_HEAP_SIZE				(1024)
#define	NETWORK_HEAP_qSIZE				(1024)
#define PROFILER_HEAP_SIZE					(1024)
#define	SKATERINFO_HEAP_SIZE			(1024)
#define	SKATER_HEAP_SIZE				(1024)		// default size of skater heap
#define SKATER_GEOM_HEAP_SIZE			(1024)
#define BOOTSTRAP_FRONTEND_HEAP_SIZE	(1024)
#define INTERNET_HEAP_SIZE				(1024)
#define NETMISC_HEAP_SIZE				(1024)
#define	THEME_HEAP_SIZE     			 (1024)		// theme textures heap size

#else

#ifdef	__NOPT_ASSERT__
// K: On debug builds line number info is included in the qb's so add in another 500K for it.
// On release builds the line number info is excluded by passing the nolinenumbers switch to
// cleanass when it is called from easyburn.bat
#define	_SCRIPT_HEAP_SIZE					(3824000+500000+130000)
#else
#define	_SCRIPT_HEAP_SIZE					(3824000+130000)	   
#endif

// Mick: Note the FRONTEND_HEAP_SIZE encompasses both Frontend and Net heaps.
// as they share the same region, with Net being top down.
#define	FRONTEND_HEAP_SIZE					(1100000)  	 	// was 800000

#define NETMISC_HEAP_SIZE					(160000)
#define	BOOTSTRAP_FRONTEND_HEAP_SIZE		(FRONTEND_HEAP_SIZE-100000)		// we have no network play in bootstrap mode  
#define INTERNET_HEAP_SIZE					(450000)
#define PROFILER_HEAP_SIZE					(60000)

// GJ:  I temporarily increased the size
// of the skater info heap until we can figure
// out how the face texture pathway is going to
// work (right now, I need a heap that wouldn't
// get destroyed when changing levels, to store
// 17K worth of temporary face texture data)
//#define	SKATERINFO_HEAP_SIZE			 	(40000)
// Mick: Increased it again for 2P
#define	SKATERINFO_HEAP_SIZE			 	(60000+2000+20000)	// extra 2000 for skater cams +20K for 2p fragmentation concerns

#define	SKATER_HEAP_SIZE				 	(120000-40000+1000-2000)	// default size of skater heap, minus 2000 since skater cams moved to skater info heap
#define	SKATER_GEOM_HEAP_SIZE			 	(680000 - 16000)		// default size of skater heap, plus a little extra cause we were running out for E3
#define	THEME_HEAP_SIZE     			 	(204800)		// theme textures heap size

#ifdef	__PLAT_NGPS__
#ifdef	__NOPT_ASSERT__
// On a regular development build, the memory usage is very different
// to a "final=" build.  Firstly there is the extra debug code, mostly assertions,
// but also memory blocks have a 32 byte rather than 16 byte header
// and many data structures have extra fields for debugging purposes
// this all means that the game will not fit in 32MB, so to fix this, the
// script heap is placed in "debug" memory (>32MB)
// Just that alone would result in there seeming to be MORE memory avaialbe in the normal build
// so to conteeract that, we allocate a block off the bottom_up heap,
// The size of this block is SCRIPT_HEAP_SIZE-DEBUG_ADJUSTMENT
// So you need to adjust DEBUG_ADJUSTMENT so the amount of free memory
// on the main heap is the same in a development build
// and in a "final=" cd build, for example, if you had for CD final build:
// 
// Name            Used  Frag  Free   Min  Blocks                                  
// --------------- ----- ----- ---- ------ ------                                  
//     Top Down:  3942K    0K  894K  884K   3933 
//     BottomUp: 23275K   23K  894K  884K  10432  
//
// and on a regular build:
//
//     Top Down:  4004K    0K  579K  523K   3933   
//     BottomUp: 22115K   23K  579K  523K  10601
//
// Then you see that the regular build is underreporting the actual amount of free memory
// and you should in theory add (894-579) = 315K
// In practice it's a good idea to not push things right to the wire, as memory
// usage can vary in differing situation (specifically when the number of blocks vary,
// like a large number of small allocations vs a small number of large allocations.)
// Since the above was from a worst case situation (NJ), I feel that 200K would be appropiate
	
#define	DEBUG_ADJUSTMENT					(1126400 + 200000 + 300000 + 100000 + 350000 ) // (1126400)		// difference in free memory for "final=" vs debug build
#endif // __NOPT_ASSERT__
#endif // __PLAT_NGPS__

#endif // __PLAT_WN32__

#ifdef __PLAT_XBOX__
// Just need to override some of these values - want to keep as much the same as possible tho.
#undef	SKATER_HEAP_SIZE
#define	SKATER_HEAP_SIZE				( 120000 - 40000 - 2000)

#undef	SKATER_GEOM_HEAP_SIZE
#define	SKATER_GEOM_HEAP_SIZE			( 480000 )

#undef	THEME_HEAP_SIZE
#define	THEME_HEAP_SIZE					( 307200 )

#undef	FRONTEND_HEAP_SIZE
#define	FRONTEND_HEAP_SIZE				( 1050000 )


#endif // __PLAT_XBOX__

#ifdef __PLAT_NGC__
// Just need to override some of these values - want to keep as much the same as possible tho.
#undef	SKATER_HEAP_SIZE
#define	SKATER_HEAP_SIZE				( ( 120000 - 40000 - 2000) + 1000 )
#undef	PROFILER_HEAP_SIZE
#define PROFILER_HEAP_SIZE					  (40000)
#undef	FRONTEND_HEAP_SIZE
#define	FRONTEND_HEAP_SIZE				( 650000 )
#undef	SKATER_GEOM_HEAP_SIZE
#define	SKATER_GEOM_HEAP_SIZE			 (470000)		// default size of skater heap
#undef	THEME_HEAP_SIZE
#define	THEME_HEAP_SIZE     			 	(330000)		// theme textures heap size
#define	SCRIPT_HEAP_SIZE (_SCRIPT_HEAP_SIZE-(300000+(1024*1024)))

#undef	SKATERINFO_HEAP_SIZE
#define	SKATERINFO_HEAP_SIZE			 	(60000+2000+20000+18000)	// extra 2000 for skater cams +20K for 2p fragmentation concerns

#define	AUDIO_HEAP_SIZE			 			(310*1024)
#else
// All platforms except for NGC
#define	SCRIPT_HEAP_SIZE _SCRIPT_HEAP_SIZE
#endif // __PLAT_NGC__


#endif // __SK_HEAP_SIZES_H

