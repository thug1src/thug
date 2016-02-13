/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Core Library											**
**																			**
**	Module:			Template Singleton class								**
**																			**
**	File name:		core/singleton.h										**
**																			**
**	Created:		10/17/00	-	mjb										**
**																			**
**																			**
*****************************************************************************/

#ifndef __CORE_SINGLETON_H
#define __CORE_SINGLETON_H

/*

Singletons
(Documentation by Mick)

A singleton is a single instance of a class which is dynamically
allocated just once.

To make a class such as CStats a singleton class, you do this:
	
	
	class CStats
	{
		// Normal Class definition goes here...
		...
		
		// Constructor and destructor MUST BE PRIVATE, for it to be a singleton
		private:
			 CStats( void ); 		
			~CStats( void );
		
		// Macro to include the singleton code 		
		DeclareSingletonClass( CStats );	
	}

This macro just places some static functions in the class, and a static pointer to 
the single instance of class itself (initially NULL)

You can then initialize the singleton by the line

	Spt::SingletonPtr< CStats >	p_stats( true );

This is somewhere at a high level in the code (currently they are all in main())

The original design of our singletons had in mind that there would be reference counting
of the references tot he singleton, and when it was no longer referenced, it would
delete itself.  So, in many places in the code, the singletons are reference by creating a
"Spt::SingletonPtr", which is a smart pointer to the instance of the singleton.  

For example:

	Spt::SingletonPtr< CStats > p_stats;
	p_stats->Update();				 

In the ultimate version of this idea, the game would be comprised of many modules,
systems and subsystems.  Each would be a singleton.  Each could be dependent on
other singletons, but more that one module could depend on a particular singleton,
so you coudl not rely on a module to de-initialize the singletons it needed
as they might be needed elsewhere.

So, each module would include a Spt::SingletonPtr for each of the modules that it
needed, so when a module was created it would increase the reference on the singletons
and create any that did not exists.  Then when it shut down, the pointers would automatically
go out of scope, and any singleton that was not referenced would automatically be deleted,
so there would never be more systems active than there needed to be.
				 
However, it turned out that this was generally overkill.  Most systems were initialized at the start,
and those that were not, we wanted to have control over the initialization of them.
The use of smart pointers here also obfuscated the code, and introduced a significant
execution overhead that was totally unnecessary.

So, the prefered method to access a singleton now is:

	CStats::Instance()->Update();

This simply references the instance directly (as it's inline), and so
does no addition processing.  It's easier to type as well.


*/



/*****************************************************************************
**								   Includes									**
*****************************************************************************/
	
#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/support.h>

#ifndef	__SYS_MEM_MEMPTR_H
#include <sys/mem/memptr.h>
#endif
		  
/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Spt
{
		  
/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

			  
template< class _T >
class SingletonPtr : public Spt::Class							
{

public:

							SingletonPtr( bool create = false );
	virtual					~SingletonPtr( void );


#if ( defined ( __PLAT_XBOX__ ) || defined ( __PLAT_WN32__ ))
							SingletonPtr( const SingletonPtr< _T > & rhs );
	SingletonPtr< _T >&		operator= ( const SingletonPtr< _T >& rhs );

#else

template < class _NewT >						                    			// template copy contructor
							SingletonPtr( const SingletonPtr< _NewT >& rhs ); 	// needed to support inheritance correctly

template < class _NewT > 	
	SingletonPtr< _T >&		operator= ( const SingletonPtr< _NewT >& rhs );		// template assignment operator

#endif

	_T*						operator-> () const;
	_T&						operator* () const;

private:
	
	Mem::Ptr< _T >			mp_instance;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
template < class _T > inline
SingletonPtr< _T >::SingletonPtr( bool create )
: mp_instance ( _T::sSgltnInstance( create ) )
{
	
#ifdef __NOPT_FULL_DEBUG__
	SetName( const_cast< char* >( sClassNode()->GetName() ));
#endif
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
template < class _T > inline
SingletonPtr< _T >::~SingletonPtr()
{
	
	
	mp_instance->sSgltnDelete();
}

#if ( defined ( __PLAT_XBOX__ ) || defined ( __PLAT_WN32__ ))
   
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
template < class _T > inline
SingletonPtr< _T >::SingletonPtr ( const SingletonPtr< _T >& rhs )
: mp_instance ( _T::sSgltnInstance() )
{
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
template < class _T > inline
SingletonPtr< _T >& SingletonPtr< _T >::operator= ( const SingletonPtr< _T >& rhs )
{
	
	
	return	*this;
}

#else

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
template < class _T > template < class _NewT > inline
SingletonPtr< _T >::SingletonPtr< _T >( const SingletonPtr< _NewT >& rhs )
: mp_instance ( _NewT::sSgltnInstance() )
{
	

	Dbg_MsgAssert( false,( "Microsoft VC++ sucks - don't do this (yet)" ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > template < class _NewT > inline
SingletonPtr< _T >&	SingletonPtr< _T >::operator= ( const SingletonPtr< _NewT >& rhs ) 
{
	

	Dbg_MsgAssert( false,( "Microsoft VC++ sucks - don't do this (yet)" ));

	return *this;	
}

#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
template < class _T > inline
_T*	SingletonPtr< _T >::operator-> () const
{
	

	return	mp_instance.Addr();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
template < class _T > inline
_T& SingletonPtr< _T >::operator* () const
{
	

	return	*mp_instance;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
#define	DeclareSingletonClass(_T)											\
private:																	\
	static _T*	sSgltnInstance( bool create = false );						\
	static void sSgltnDelete( void );  										\
																			\
	static _T* 		sp_sgltn_instance;										\
	static uint 	s_sgltn_count;											\
public:																		\
	inline static void	Create(void) {sp_sgltn_instance = new _T;}			\
	inline static _T*	Instance(void) {return sp_sgltn_instance;}			\
	inline bool	Initilized(void) {return (sp_sgltn_instance!=NULL);}		\
																			\
	friend class Spt::SingletonPtr< _T >;									\


#define DefinePlacementSingletonClass(_T, _P, _N )							\
																			\
_T*		_T::sp_sgltn_instance = NULL;										\
uint	_T::s_sgltn_count = 0;												\
																			\
_T*	_T::sSgltnInstance( bool create )										\
{																			\
																			\
																			\
	if ( !sp_sgltn_instance && create ) 									\
	{																		\
		sp_sgltn_instance = new (_P) _T;									\
	}																		\
																			\
	Dbg_AssertType( sp_sgltn_instance, _T );								\
																			\
	++s_sgltn_count;														\
	return	sp_sgltn_instance;												\
}																			\
																			\
void _T::sSgltnDelete( void )												\
{																			\
													\
																			\
	Dbg_MsgAssert(( s_sgltn_count > 0 ),( "Reference count imbalance" ));	\
																			\
	if ( s_sgltn_count > 1 )												\
	{																		\
		s_sgltn_count--;													\
		return;																\
	}																		\
																			\
	delete sp_sgltn_instance;												\
	sp_sgltn_instance = NULL;												\
	s_sgltn_count = 0;														\
}																			\

														 
#define	DefineSingletonClass(_T,_N)		\
		DefinePlacementSingletonClass(_T, (Mem::Allocator*)NULL, _N )

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


/*****************************************************************************
**									Macros									**
*****************************************************************************/

} // namespace Spt

#endif	//	__CORE_SINGLETON_H





