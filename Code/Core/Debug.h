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
**	Module:			Debug (DBG)												**
**																			**
**	Created:		05/27/99	mjb											**
**																			**
**	File name:		core/debug.h											**
**																			**
*****************************************************************************/

#ifndef __CORE_DEBUG_H
#define __CORE_DEBUG_H


#ifdef __NOPT_DEBUG__

// for now - always turn on messages and assertion
#define	__NOPT_MESSAGES__
#define	__NOPT_ASSERT__

#else

// Mick - added assertions always on, regardless of debug mode.
#define	__NOPT_ASSERT__

// Gary - ...  unless you're working on the tools, 
// in which case the asserts prevent the code from compiling...
#ifdef __PLAT_WN32__
#ifndef _CONSOLE // Ken: Asserts now compile for console apps. (see levelassetlister project)
	#undef __NOPT_ASSERT__
#endif	
#endif

#endif	// __NOPT_DEBUG__

#ifdef __NOPT_CDROM__OLD
#undef __NOPT_ASSERT__
#endif

// Temporary switch off assertions flag				 
#ifdef __NOPT_NOASSERTIONS__
#undef __NOPT_ASSERT__
#endif

// no assertions on final build ("final=")
#ifdef __NOPT_FINAL__
#undef __NOPT_ASSERT__
#endif


#ifdef __NOPT_ASSERT__
#define	__DEBUG_CODE__
#endif


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sys/mem/memdbg.h>

#include "debug/messages.h"
#include "debug/checks.h"

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Dbg
{



#ifdef __NOPT_DEBUG__
 
/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/


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

void	SetUp( void );
void	CloseDown( void );

#ifdef __NOPT_FULL_DEBUG__
void*	NewClassNode( size_t size );
void	DeleteClassNode( void* pMem );
void*	NewInstanceNode( size_t size );
void	DeleteInstanceNode( void* pMem );
#endif // __NOPT_FULL_DEBUG__

/*****************************************************************************
**									Macros									**
*****************************************************************************/

#define Dbg_Code(X)		X

/*****************************************************************************
**									Stubs									**
*****************************************************************************/

#else	// __NOPT_DEBUG__ 

#ifndef	__NOPT_ASSERT__
inline void SetUp( void ) {};
inline void CloseDown( void ) {};
#else
void	SetUp( void );
void	CloseDown( void );
#endif // __NOPT_ASSERT

#define Dbg_Code(X)


#endif	// __NOPT_DEBUG__

} // namespace Dbg

#endif	// __CORE_DEBUG_H
