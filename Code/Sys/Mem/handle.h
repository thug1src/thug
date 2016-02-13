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
**	File name:		core/sys/mem/handle.h									**
**																			**
*****************************************************************************/

#ifndef	__SYS_MEM_HANDLE_H
#define	__SYS_MEM_HANDLE_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/support.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Mem
{



/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/

nTemplateBaseClass( _T, Handle )
{
	

	
public :

								Handle( _T* ptr = NULL );
								~Handle( void );

	template < class _NewT >						                    // template copy contructor
								Handle( Handle< _NewT >& rhs ); 		// needed to support inheritance correctly
	
	template < class _NewT > 	
		Handle< _T >&			operator = ( Handle< _NewT >& rhs );	// template assignment operator
		Handle< _T >&			operator = ( _T* ptr );

		_T*						GetPointer( void ) const;
		
protected :

		_T*						m_ptr;
		uint					m_id;
	
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

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

template < class _T > inline   
Handle< _T >::Handle< _T >( _T* ptr ) 
: m_ptr ( ptr ), m_id ( Allocator::sGetId( ptr ))
{
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
#if ( defined ( __PLAT_XBOX__ ) || defined ( __PLAT_WN32__ ))
// This version to fix Visual C++ compiler lack of ANSI C++ compliance.
#else
template < class _T > template < class _NewT > inline
Handle< _T >::Handle< _T >( Handle< _NewT >& rhs )
: m_ptr ( rhs.m_ptr ), m_id ( rhs.m_id )
{
	
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline	
Handle< _T >::~Handle( void )
{
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#if ( defined ( __PLAT_XBOX__ ) || defined ( __PLAT_WN32__ ))
// This version to fix Visual C++ compiler lack of ANSI C++ compliance.
#else
 template < class _T > template < class _NewT > inline
Handle< _T >&		Handle< _T >::operator = ( Handle< _NewT >& rhs ) 
{
	

	m_ptr = rhs.m_ptr;
	m_id  = rhs.m_id;

	return *this;	
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline
Handle< _T >&		Handle< _T >::operator = ( _T* ptr ) 
{
	

	m_ptr = ptr;
	m_id  = Allocator::sGetId( ptr );

	return *this;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline
_T*		Handle< _T >::GetPointer ( void ) const 
{
	

	if (( m_ptr ) && ( m_id == Allocator::sGetId( m_ptr )))
	{
		Dbg_AssertType( m_ptr, _T );
		return m_ptr;
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mem

#endif  // __SYS_MEM_HANDLE_H								
