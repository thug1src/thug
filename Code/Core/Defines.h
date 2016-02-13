/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Core Library											**
**																			**
**	Module:			Standard Header											**
**																			**
**	File name:		core/defines.h											**
**																			**
**	Created:		05/27/99	-	mjb										**
**																			**
**	All code depends on the definitions in this files						**
**	It should be included in every file										**
**																			**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#define __CORE_DEFINES_H

/*****************************************************************************
**								   Includes									**
*****************************************************************************/


#ifdef __PLAT_WN32__
//#include <strstream>
//#include <fstream.h>
#ifdef __USE_OLD_STREAMS__
#include <iostream.h>
#else
#include <iostream>
using namespace std;
#endif
#include <stdio.h>
#include <string.h>

#pragma warning( disable : 4800 ) 
#pragma warning( disable : 4355 ) // 'this' : used in base member initializer list
#pragma warning( disable : 4291 ) // no matching operator delete found



#else
#ifdef __PLAT_NGPS__
#include <stdio.h>
#include <eetypes.h>
//#include <iostream.h>
#include <string.h>
#include <stdlib.h>
#else
#ifdef __PLAT_NGC__
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <typeinfo>  
#else
#ifdef __PLAT_XBOX__
#include <stdio.h>
//#include <iostream.h>
#include <string.h>
#include <stdlib.h>

#pragma warning( disable : 4800 ) 
#pragma warning( disable : 4291 ) // no matching operator delete found
#pragma warning( disable : 4355 ) // 'this' : used in base member initializer list
#pragma warning( disable : 4995 ) // name was marked as #pragma deprecated

#endif
#endif
#endif
#endif

#ifndef __PLAT_WN32__
//#ifdef __NOPT_ASSERT__
#ifdef __PLAT_NGC__
#ifdef __NOPT_FINAL__
	#define printf(A...)
#else
	int OurPrintf(const char *fmt, ...);
	#define printf OurPrintf
#endif
#else
	int OurPrintf(const char *fmt, ...);
	#define printf OurPrintf
#endif	

//#else
//	inline void NullPrintf(const char *fmt, ...){}
//	#define printf NullPrintf	
//#endif
#endif


/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define vINT8_MAX   		0x7F
#define vINT8_MIN   		0x81
#define vINT16_MAX   		0x7FFF
#define vINT16_MIN   		0x8001
#define vINT32_MAX   		0x7FFFFFFF
#define vINT32_MIN   		0x80000001
#define vINT64_MAX   		0x7FFFFFFFFFFFFFFF
#define vINT64_MIN   		0x8000000000000001

#define vUINT8_MAX   		0xFF
#define vUINT16_MAX   		0xFFFF
#define vUINT32_MAX   		0xFFFFFFFF
#define vUINT64_MAX   		0xFFFFFFFFFFFFFFFF

#ifndef TRUE
#define FALSE				0
#define TRUE				(!FALSE)
#endif

#ifndef NULL
#define NULL				0
#endif

/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/

typedef char				int8;
typedef short				int16;

typedef unsigned int		uint;
typedef unsigned char		uint8;
typedef unsigned short		uint16;

typedef signed int			sint;
typedef signed char			sint8;
typedef signed short		sint16;

#define vINT_MAX			vINT32_MAX
#define vINT_MIN			vINT32_MIN
#define vUINT_MAX			vUINT32_MAX

#ifdef __PLAT_WN32__
typedef long				int32;
typedef unsigned long		uint32;
typedef signed long			sint32;
typedef __int64				int64;
typedef unsigned __int64	uint64;
typedef signed __int64		sint64;

#endif

#ifdef __PLAT_NGPS__
typedef int					int32;
typedef unsigned int		uint32;
typedef signed int			sint32;
typedef long				int64;
typedef unsigned long		uint64;
typedef signed long			sint64;
typedef long128				int128;
typedef u_long128			uint128;
typedef long128				sint128;

#endif

#ifdef __PLAT_XBOX__
typedef long				int32;
typedef unsigned long		uint32;
typedef signed long			sint32;
typedef __int64				int64;
typedef unsigned __int64	uint64;
typedef signed __int64		sint64;

#endif

#ifdef __PLAT_NGC__
typedef int					int32;
typedef unsigned int		uint32;
typedef signed int			sint32;
typedef long long			int64;
typedef unsigned long long	uint64;
typedef signed long long	sint64;
// Paul: No GameCube 128-bit types.
//typedef long128				int128;
//typedef u_long128			uint128;
//typedef long128				sint128;
typedef long long			int128;
typedef unsigned long long	uint128;
typedef signed long long	sint128;

#endif

#if defined(__PLAT_NGPS__) || defined(__PLAT_XBOX__) || defined(__PLAT_NGC__)

class ostream
{
public:
	ostream& operator<< ( char* str ) 		{ printf ( str ); return *this; }
	ostream& operator<< ( const char* str ) { printf ( str ); return *this; }
	ostream& operator<< ( sint i ) 			{ printf ( "%d", i ); return *this; }
	ostream& operator<< ( uint i ) 			{ printf ( "%u", i ); return *this; }
	ostream& operator<< ( float f ) 		{ printf ( "%f", f ); return *this; }
	ostream& operator<< ( void* p ) 		{ printf ( "%p", p ); return *this; }
	ostream& operator<< ( const void* p )	{ printf ( "%p", p ); return *this; }
};

#endif 

#define	vINT_BITS			32
#define	vPTR_BITS			32

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Alignment macros


#ifdef __PLAT_NGPS__
#define nAlign(bits)	__attribute__((aligned((bits>>3))))
#else
#define nAlign(bits)
#endif

#define	nPad64(X)	uint64		_pad##X;
#define	nPad32(X)	uint32		_pad##X;
#define	nPad16(X)	uint16		_pad##X;
#define	nPad8(X)	uint8		_pad##X;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// version stamping

#define __nTIME__		__TIME__
#define __nDATE__		__DATE__

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#define nBit(b)				( 1 << (b) )

typedef	sint				nID;
typedef	sint8				nID8;
typedef	sint16				nID16;
typedef	sint32				nID32;
typedef	sint64				nID64;

#define	nMakeID(a,b,c,d)		( (nID) ( ( (nID) (a) ) << 24 | ( (nID) (b) ) << 16 |	\
								( (nID) (c) ) << 8 | ( (nID) (d) ) ) )


//	nMakeStructID() differs from nMakeID in that struct IDs are always
//	readable from a memory dump, where as IDs would be reversed on little
//	endian machines

#if		__CPU_BIG_ENDIAN__
#define	nMakeStructID(a,b,c,d) ( (nID) ( ((nID)(a))<<24 | ((nID)(b))<<16 | \
										  ((nID)(c))<<8  | ((nID)(d)) ))
#else
#define	nMakeStructID(a,b,c,d) ( (ID) ( ((nID)(d))<<24 | ((nID)(c))<<16 | \
										  ((nID)(b))<<8  | ((nID)(a)) ))
#endif

/*****************************************************************************
**									Macros									**
*****************************************************************************/

#define	nReverse32(L)	( (( (L)>>24 ) & 0xff) | (((L)>>8) &0xff00) | (((L)<<8)&0xff0000) | (((L)<<24)&0xff000000))
#define	nReverse16(L)	( (( (L)>>8 ) & 0xff) | (((L)<<8)&0xff00) )

#if	__CPU_BIG_ENDIAN__

#define	nBgEn32(L) 	(L)
#define	nBgEn16(L) 	(L)

#define	nLtEn32(L)	nReverse32(L)
#define	nLtEn16(L)	nReverse16(L)

#else

#define	nBgEn32(L) 	nReverse32(L)
#define	nBgEn16(L) 	nReverse16(L)

#define	nLtEn32(L)	(L)
#define	nLtEn16(L)	(L)

#endif

#ifdef __PLAT_XBOX__
#define __PRETTY_FUNCTION__ "?"

#define isnanf	_isnan
#define isinff	_isnan

#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#define __CPU_WORD_BALIGN__    4		// Memory word byte alignment

#define PTR_ALIGNMASK	 ( vUINT_MAX << __CPU_WORD_BALIGN__)

// The alignment macros align elements for fastest access

#define nAligned(P) 		( !( (uint) (P) & (~PTR_ALIGNMASK) ) )
#define nAlignDown(P) 		(void*)( (uint) (P) & PTR_ALIGNMASK )
#define nAlignUp(P)			(void*)( ( (uint) (P) + ( 1 << __CPU_WORD_BALIGN__ ) - 1 ) & PTR_ALIGNMASK )
#define nAlignedBy(P,A) 	( !( (uint) (P) & ( ~(vUINT_MAX << (A) ) ) ) )
#define nAlignDownBy(P,A) 	(void*)( (uint) (P) & (vUINT_MAX << (A) ) )
#define nAlignUpBy(P,A)		(void*)( ( (uint) (P) + ( 1 << (A) ) - 1 ) & ( vUINT_MAX <<( A ) ) )
#define nStorage(X)			nAlignUp ( (X) + 1 )

/****************************************************************************/

#define	nAddPointer(P,X)		(void*) ( (uint) (P) + (uint) (X) )
#define	nSubPointer(P,X)		(void*) ( (uint) (P) - (uint) (X) )

/****************************************************************************/

// Converts a string into a checksum. This macro can be used for readability.
// Later, for speed, some application can scan all .cpp and .h files, and
// replace all instances of CRCX(...) with corresponding CRCD(...) instances
//
// Example: CRCX("object_id")
#define CRCX(_s)			Script::GenerateCRC(_s)
// This macro exists simply for readability. Whenever you see a CRCD(...) instance,
// you will know what string the checksum maps to. CRCD(...) instances in the code
// can be generated from CRCX(...), and can be reverse mapped if desired.
//
// Example: CRCD(0xdcd2a9d4, "object_id")

#include <core/debug.h>




/*****************************************************************************
**								   Includes									**
*****************************************************************************/




#include <core/support.h>
#include <sys/mem/memman.h>

#include <core/crc.h>

#ifdef	__PLAT_NGPS__
#include <gfx/ngps/p_memview.h>
#include "libsn.h"
#elif defined( __PLAT_NGC__ )
#include <gfx/ngc/p_memview.h>
//#include "libsn.h"
#elif defined( __PLAT_XBOX__ )
#include <gfx/xbox/p_memview.h>
#endif


// Mick:  This check slows the game down quite a bit
#if  1 && 	defined( __NOPT_ASSERT__ )

extern  uint32	check_checksum(uint32 _i, const char *_s, const char *f, int line);

#define CRCD(_i, _s)		check_checksum(_i, _s, __FILE__, __LINE__)
#else
#define CRCD(_i, _s)		_i
#endif

// CRC-C, for use only in switch statements, where you want to use the same syntax as CRCD
#define CRCC(_i, _s)		_i



//#ifdef __PLAT_NGC__
//class TCPPInit
//{
//public:
//  static bool IsHeapInitialized ;
//} ;
//
//// these definitions override the new and delete operators.
//
#ifdef __PLAT_NGC__
static inline void* operator new       ( size_t blocksize ) ;

static inline void* operator new[]     ( size_t blocksize ) ;

static inline void operator delete     ( void* block ) ;

static inline void operator delete[]   ( void* block ) ;
#endif		// __PLAT_NGC__
//
//#else

#ifdef __PLAT_WN32__
inline void* 	operator new( size_t size, bool assert_on_fail )
{
	return new char[size];
}

#else
#ifndef __PLAT_NGC__
/******************************************************************/
/* Global new/delete operators                                    */
/*                                                                */
/******************************************************************/
inline void* 	operator new( size_t size )
{
	return Mem::Manager::sHandle().New( size, true );
}
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void* 	operator new[] ( size_t size )
{
	return Mem::Manager::sHandle().New( size, true );
}
#endif		// __PLAT_NGC__

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void* 	operator new( size_t size, bool assert_on_fail )
{
	return Mem::Manager::sHandle().New( size, assert_on_fail );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void* 	operator new[] ( size_t size, bool assert_on_fail )
{
	return Mem::Manager::sHandle().New( size, assert_on_fail );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void*	operator new( size_t size, Mem::Allocator* pAlloc, bool assert_on_fail = true )
{
	return Mem::Manager::sHandle().New( size, assert_on_fail, pAlloc );
}
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void*	operator new[]( size_t size, Mem::Allocator* pAlloc, bool assert_on_fail = true )
{
	return Mem::Manager::sHandle().New( size, assert_on_fail, pAlloc );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void* 	operator new( size_t size, void* pLocation )
{
	return pLocation;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void* 	operator new[]( size_t size, void* pLocation )
{
	return pLocation;
}

#ifndef __PLAT_NGC__
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void 	operator delete( void* pAddr )
{
	Mem::Manager::sHandle().Delete( pAddr );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void 	operator delete[]( void* pAddr )
{
	Mem::Manager::sHandle().Delete( pAddr );
}
#endif		// __PLAT_NGC__

//#ifdef __PLAT_NGC__
/******************************************************************/
/* only used when exception is thrown in constructor              */
/*                                                                */
/******************************************************************

inline void	operator delete( void* pAddr, Mem::Allocator* pAlloc )
{
	Mem::Manager * mem_man = Mem::Manager::Instance();
	Mem::Manager::sHandle().Delete( pAddr );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************

inline void	operator delete[]( void* pAddr, Mem::Allocator* pAlloc )
{
	Mem::Manager * mem_man = Mem::Manager::Instance();
	Mem::Manager::sHandle().Delete( pAddr );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************

inline void 	operator delete( void*, void* pLocation )
{
	return;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

//#endif		// __PLAT_NGC__
#endif	// __PLAT_WN32__
#endif	//	__CORE_DEFINES_H




