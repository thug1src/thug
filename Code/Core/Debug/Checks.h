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
**	Module:			Debug (Dbg_)											**
**																			**
**	Created:		05/27/99	mjb											**
**																			**
**	File name:		core/debug/checks.h										**
**																			**
*****************************************************************************/

#ifndef __CORE_DEBUG_CHECKS_H
#define __CORE_DEBUG_CHECKS_H

#ifndef __CORE_DEBUG_LOG_H
#include <core/log.h>
#endif

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include "signatrs.h"

#ifdef __NOPT_ASSERT__

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Dbg
{

						

/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/

typedef void ( AssertTrap ) ( char* message );

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

extern	char*		msg_unknown_reason;			// message strings 
extern	char*		msg_null_pointer;			 
extern	char*		msg_misaligned_pointer;			 
extern	char*		msg_pointer_to_free_mem;
extern	char*		msg_type_mismatch;			 

extern 	AssertTrap	default_trap;

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

#ifdef __NOPT_DEBUG__
void	Assert ( char* file, uint line, Signature& sig, char* reason = msg_unknown_reason );
#else
void	Assert ( char* file, uint line, char* reason = msg_unknown_reason );
#endif

void	pad_printf ( const char* text, ... );
void	set_trap( AssertTrap* trap = default_trap );
void	screen_assert( bool on = false );

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Dbg

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/


/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/


/*****************************************************************************
**									Macros									**
*****************************************************************************/

#define Dbg_SetTrap(A)			{ Dbg::set_trap(A); } 
#define Dbg_SetScreenAssert(A)	{ Dbg::screen_assert(A); }

/******************************************************************/
/*                     Assertions                                 */
/*                                                                */
/******************************************************************/

#ifdef __NOPT_DEBUG__


#define Dbg_Assert(_c)  										\
																\
if ( !(_c))														\
{																\
	Dbg::Assert( __FILE__, __LINE__, Dbg_signature );			\
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#define Dbg_MsgAssert( _c, _params )							\
																\
if( !( _c ))													\
{																\
	Dbg::pad_printf _params;									\
	Dbg::Assert( __FILE__, __LINE__,	Dbg_signature,			\
										Dbg::sprintf_pad );		\
}

#else   // __NOPT_DEBUG__

// Mick:  If we have assertions, but are not in DEBUG mode
// then we don not have the Dbg_signature thing
// so we just pass NULL to assert()

#define Dbg_Assert(_c)  										\
																\
if ( !(_c))														\
{																\
	Dbg::Assert ( __FILE__, __LINE__, NULL );					\
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#ifdef __PLAT_WN32__

#ifdef _CONSOLE
#define Dbg_MsgAssert( _c, _params )							\
																\
if( !( _c ))													\
{																\
	printf("\nGame code assert!\n");							\
	printf("File %s, line %d\n",__FILE__,__LINE__);				\
	printf _params;												\
	printf("\nPress CTRL-C to quit ... ");						\
	while(1);													\
}
#else
#define Dbg_MsgAssert( _c, _params )
#endif

#else

#define Dbg_MsgAssert( _c, _params )							\
																\
if( !( _c ))													\
{																\
	Dbg::pad_printf _params;									\
	Dbg::Assert( __FILE__, __LINE__, Dbg::sprintf_pad );		\
}
#endif

#endif	// __NOPT_DEBUG__

#ifdef __PLAT_NGPS__
#define Dbg_MsgLog( _params )								\
{														\
	Dbg::pad_printf _params;							\
	Log::AddEntry(__FILE__,__LINE__,__PRETTY_FUNCTION__,Dbg::sprintf_pad);		\
}
	
#define Dbg_Log( )										\
{														\
	Log::AddEntry(__FILE__,__LINE__,__PRETTY_FUNCTION__);			\
}
#else
#define Dbg_MsgLog( _params )								\
{														\
	Dbg::pad_printf _params;							\
	Log::AddEntry(__FILE__,__LINE__,__FUNCTION__,Dbg::sprintf_pad);		\
}
	
#define Dbg_Log( )										\
{														\
	Log::AddEntry(__FILE__,__LINE__,__FUNCTION__);			\
}
#endif
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#ifdef __NOPT_MEM_DEBUG__

#define Dbg_AssertPtr(_p)											\
{																	\
	if ((_p) == NULL )												\
	{																\
		Dbg::Assert ( __FILE__,	__LINE__,							\
					 Dbg_signature, Dbg::msg_null_pointer );		\
	}																\
	else if ( *((uint64*)(nAlignDown(_p))) == Mem::vFREE_BLOCK )	\
	{																\
		Dbg::Assert ( __FILE__,	__LINE__,							\
					 Dbg_signature, Dbg::msg_pointer_to_free_mem );	\
	}																\
}


#else // __NOPT_MEM_DEBUG__

#ifdef __NOPT_DEBUG__

#define Dbg_AssertPtr(_p)											\
{																	\
	if ((_p) == NULL )												\
	{																\
		Dbg::Assert ( __FILE__,	__LINE__,							\
					 Dbg_signature, Dbg::msg_null_pointer );		\
	}																\
}

#else


#define Dbg_AssertPtr(_p)											\
{																	\
	if ((_p) == NULL )												\
	{																\
		Dbg::Assert ( __FILE__,	__LINE__,							\
					 Dbg::msg_null_pointer );		\
	}																\
}

#endif //__NOPT_DEBUG__

#endif // __NOPT_MEM_DEBUG__

/******************************************************************/
/*                     Type Checking                              */
/*                                                                */
/******************************************************************/

#ifdef __NOPT_FULL_DEBUG__

#define Dbg_AssertType(_c,_t)																\
{																							\
	Dbg_AssertPtr(_c);       																\
	Dbg_MsgAssert( nAligned(_c), Dbg::msg_misaligned_pointer );								\
	Dbg_MsgAssert(((_c)->classStamp != Spt::Class::vDELETED_CLASS ),"Deleted Class" );		\
	Dbg_MsgAssert(((_c)->classStamp == Spt::Class::vREGISTERED_CLASS ), "Corrupt Class" );	\
	Dbg_MsgAssert(((_c)->vClassNode()->IsDerivedOrSame(*_t::sClassNode())),					\
		Dbg::msg_type_mismatch,#_c,(_c)->vClassNode()->GetName(),#_t);						\
}

#define Dbg_AssertThis																		\
{																							\
	Dbg_AssertPtr(this);       																\
	Dbg_MsgAssert( nAligned(this), Dbg::msg_misaligned_pointer );							\
	Dbg_MsgAssert((classStamp != Spt::Class::vDELETED_CLASS ),"Deleted Class" );			\
	Dbg_MsgAssert((classStamp == Spt::Class::vREGISTERED_CLASS ), "Corrupt Class" );		\
	Dbg_MsgAssert((vClassNode()->IsDerivedOrSame(*sClassNode())),							\
		Dbg::msg_type_mismatch,"this", vClassNode()->GetName(),sClassNode()->GetName());	\
}

#else // __NOPT_FULL_DEBUG__

#define Dbg_AssertType(_c,_t)	Dbg_AssertPtr(_c)
#define Dbg_AssertThis			Dbg_AssertPtr(this)

#endif // __NOPT_FULL_DEBUG__


/*****************************************************************************
**									Stubs									**
*****************************************************************************/

#else

#define Dbg_MsgAssert( _c, _params )

#define Dbg_SetTrap(_f)

#ifdef __NOPT_ASSERT__
#define Dbg_SetScreenAssert(A)	{ Dbg::screen_assert(A); }
#else
#define Dbg_SetScreenAssert(_b)
#endif

#define Dbg_Assert(_c)
#define Dbg_AssertPtr(_p)
#define Dbg_AssertType(_c,_t)
#define Dbg_AssertThis

#define Dbg_MsgLog( _params )
#define Dbg_Log( )


#endif	// __NOPT_ASSERT__

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#endif	// __CORE_DEBUG_CHECKS_H


