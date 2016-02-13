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
**	Module:			Support (SPT)											**
**																			**
**	Created:		06/10/99	mjb											**
**																			**
**	File name:		core/support/class.h									**
**																			**
**	Description:	Base class from which  classes are derived if they want	**
**                  their memory zeroed	when instantiated (via new)        	**
**																			**
*****************************************************************************/

#ifndef __CORE_SUPPORT_CLASS_H
#define __CORE_SUPPORT_CLASS_H

#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define ZERO_CLASS_MEM	TRUE
 
namespace Mem
{
	class Allocator;
}

namespace Spt
{

class Class
{
	public:
	
	#if (defined ( ZERO_CLASS_MEM ) && !defined ( __PLAT_WN32__ ))
		void*			operator new( size_t size );
		void*			operator new[] ( size_t size );
		void*			operator new( size_t size, bool assert_on_fail );
		void*			operator new[] ( size_t size, bool assert_on_fail );
		void*			operator new( size_t size, Mem::Allocator* pAlloc, bool assert_on_fail = true );
		void*			operator new[]( size_t size, Mem::Allocator* pAlloc, bool assert_on_fail = true );
		void*			operator new( size_t size, void* pLocation );
		void*			operator new[]( size_t size, void* pLocation );
	
	#endif // ZERO_CLASS_MEM
};

} // namespace Spt

#endif // __CORE_SUPPORT_CLASS_H

