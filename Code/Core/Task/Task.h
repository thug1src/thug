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
**	Module:			Task (TSK_)												**
**																			**
**	File name:		core/task/task.h										**
**																			**
**	Created: 		05/27/99	-	mjb										**
**																			**
*****************************************************************************/

#ifndef	__CORE_TASK_TASK_H
#define	__CORE_TASK_TASK_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/list.h>
#include <core/task/list.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Tsk
{



/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/


class  BaseTask : public  Spt::Access 
{
	

	friend class List;


public:

	typedef Lst::Node< BaseTask >	Node;
	
	bool			InList( void ) const;
	void			Remove( void );
	
	virtual	void	vCall( void ) const = 0;
	virtual void *	GetCode(void) const = 0;

	void			SetPriority( Node::Priority pri ) const;
	virtual			~BaseTask( void );
	void			SetMask(uint mask);
	uint			GetMask();

protected:
					BaseTask( Node::Priority pri );
	
	Node*			node;			// link node

private:

	List*			tlist;			// task list to which we belong
	uint			stamp;			// process stamp
	uint			m_mask;	 		// mask indicating what types of thing this is
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
nTemplateSubClass( _T, Task, BaseTask )
{
	

public:

	typedef void	(Code)( const Task< _T >& );

	Task( Code* const code, _T& data, Node::Priority pri = Lst::Node< BaseTask >::vNORMAL_PRIORITY );
		
	virtual void	vCall( void ) const;
	_T&				GetData( void ) const;
	virtual void *	GetCode(void) const;
	
private :
	Code* const		code;			// tasks entry point
	_T&				data;			// task defined data
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
Task< _T >::Task ( Code* const _code, _T& _data, Node::Priority pri )
: BaseTask( pri ), code( _code ), data( _data )
{
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline 
void		Task< _T >::vCall( void ) const
{
	
	
	Dbg_AssertPtr( code );
	code( *this );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline 
_T&		Task< _T >::GetData( void ) const
{
	
	
	Dbg_AssertType( &data, _T );
	return data;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/



template < class _T > inline 
void *		Task< _T >::GetCode( void ) const
{
	
	
	return (void*)code;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void		BaseTask::SetMask( uint mask )
{
	
	m_mask = mask;
}

inline uint		BaseTask::GetMask( void )
{
	
	return m_mask;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Tsk

#endif	// __CORE_TASK_TASK_H


