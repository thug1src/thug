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
**	Module:			List (LST_)												**
**																			**
**	File name:		core/list/head.h										**
**																			**
**	Created: 		05/27/99	-	mjb										**
**																			**
*****************************************************************************/

#ifndef	__CORE_LIST_HEAD_H
#define	__CORE_LIST_HEAD_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/list/node.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Lst
{



/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

/***********************************************************************
 *
 * Class:			Head
 *
 * Description:		Linked-list head node. 
 *
 ***********************************************************************/

nTemplateSubClass( _T, Head, Node< _T > )
{

public:
					Head( void );
	virtual			~Head( void );

	void			Merge( Head< _T >* dest );		// Source list will be empty after merge
	uint			CountItems( void );
	Node< _T >*		GetItem( uint number );			// Zero-based ( 0 will return first node )
	
	void			AddNode( Node< _T >* node );	// Using priority
	void			AddNodeFromTail( Node< _T >* node );	// Using priority, search backwards from tail (i.e same-priorties are appended rather than pre-pended)
	bool			AddUniqueSequence( Node< _T >* node );	// Only add if priority is unique
																		// and priority decreases
	void			AddToTail( Node< _T >* node );
	void			AddToHead( Node< _T >* node );

	void			RemoveAllNodes( void );
	void			DestroyAllNodes( void );  		// ONLY USE FOR INHERITED LISTS
	bool			IsEmpty( void );

	Node< _T >*		FirstItem();					// get first node, or NULL if none 


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
Head< _T >::Head( void )
: Node< _T > ( reinterpret_cast < _T* >( vHEAD_NODE ) ) 
{
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline	
Head< _T >::~Head( void )
{
	

	Dbg_MsgAssert( IsEmpty(),( "List is not empty" ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline	
void	Head< _T >::AddNode( Node< _T >* node )
{
	
	
	Dbg_AssertType( node, Node< _T > );
	Dbg_MsgAssert( this->is_head (),( "Object is not a list" ));
	Dbg_MsgAssert( !node->InList (),( "Object is already in a list" ));

	Node< _T >*		node_ptr =	this;
	Priority		new_pri	 =	node->GetPri();

	while (( node_ptr = node_ptr->GetNext() ))
	{
		if ( node_ptr->GetPri() <= new_pri )
		{
			node_ptr->Insert( node );
			return;
		}
	}

	Insert( node );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline	
void	Head< _T >::AddNodeFromTail( Node< _T >* node )
{
	
	
	Dbg_AssertType( node, Node< _T > );
	Dbg_MsgAssert( this->is_head (),( "Object is not a list" ));
	Dbg_MsgAssert( !node->InList (),( "Object is already in a list" ));

	Node< _T >*		node_ptr =	this;
	Priority		new_pri	 =	node->GetPri();

	while (( node_ptr = node_ptr->GetPrev() ))
	{
		if ( node_ptr->GetPri() >= new_pri )
		{
			node_ptr->Append( node );
			return;
		}
	}

	Append( node );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline
bool	Head< _T >::AddUniqueSequence( Node< _T >* node )
{
	
	
	Dbg_AssertType( node, Node< _T > );
	Dbg_MsgAssert( this->is_head (),( "Object is not a list" ));
	Dbg_MsgAssert( !node->InList (),( "Object is already in a list" ));

	Node< _T >*		node_ptr =	this;
	Priority		new_pri	 =	node->GetPri();

	while (( node_ptr = node_ptr->GetNext() ))
	{
		if ( node_ptr->GetPri() == new_pri )
		{
			return false;
		}
		else if ( node_ptr->GetPri() > new_pri )
		{
			node_ptr->Insert( node );
			return true;
		}
	}

	Insert( node );
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline	
void		Head< _T >::Merge( Head< _T >* dest )
{
	
	
	Dbg_AssertType( dest, Head< _T > );
	Dbg_MsgAssert( this->is_head (),( "Object is not a list" ));
	Dbg_MsgAssert( dest->is_head (),( "Object is not a list" ));

	Node< _T >*		first = next;	
	Node< _T >*		last = prev;
	Node< _T >*		node = dest->GetPrev();
	
	if ( this == first )			// source list is empty
	{
	   	return;
	}
	
	node->SetNext( first );
	first->SetPrev( node );

	last->SetNext( dest );
	dest->SetPrev( last );
	
	node_init();					// make the source list empty
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline	
Node< _T >*		Head< _T >::GetItem( uint number )
{
	
	
	Dbg_MsgAssert( this->is_head (),( "Object is not a list" ));

	Node< _T >*		node = GetNext();

	while ( node )
	{
		if ( number-- == 0 )
		{
			return node;
		}

		node = node->GetNext();
	}

	Dbg_Warning( "Item requested (%d) out of range (%d)", number, CountItems() );

	return NULL;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Return the firs node in the list that this is the head of

template < class _T > inline	
Node< _T >*		Head< _T >::FirstItem( )
{
	
	
	Dbg_MsgAssert( this->is_head (),( "Object is not a list" ));

	return GetNext();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template <class _T > inline	
uint		Head< _T >::CountItems( void )
{
	
	
	Dbg_MsgAssert( this->is_head (),( "Object is not a list" ));

	uint			count = 0;
	Node< _T >*	node = GetNext();

	while ( node )
	{
		count++;
		node = node->GetNext();
	}

	return count;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template <class _T> inline	
void		Head< _T >::RemoveAllNodes( void )
{
	
	
	Dbg_MsgAssert( this->is_head (),( "Object is not a list" ));

	Node< _T >*		next_nd;
	Node< _T >*		node = GetNext();

	while ( node )
	{
		next_nd = node->GetNext();
		node->Remove();
		node = next_nd;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template <class _T> inline	
void		Head< _T >::DestroyAllNodes( void )
{
	
	
	Dbg_MsgAssert( this->is_head (),( "Object is not a list" ));

	Node< _T >*		next_nd;
	Node< _T >*		node = GetNext();

	while ( node )
	{
		next_nd = node->GetNext();
//		node->Remove();
		delete node;
		node = next_nd;
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
template <class _T> inline	
void		Head< _T >::AddToTail( Node< _T >* node )
{
	
	
	Dbg_AssertType( node, Node< _T > );
	Dbg_MsgAssert( this->is_head (),( "Object is not a list" ));
	Dbg_MsgAssert( !node->InList (),( "Node is already in a list" ));

	Insert ( node );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template <class _T> inline	
void		Head< _T >::AddToHead ( Node< _T >* node )
{
	
	
	Dbg_AssertType( node, Node< _T > );
	Dbg_MsgAssert( this->is_head(),(( "Object is not a list" )));
	Dbg_MsgAssert( !node->InList(),(( "Node is already in a list" )));

	Append( node );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template <class _T> inline
bool		Head< _T >::IsEmpty( void )
{
	
	
	Dbg_MsgAssert ( this->is_head(),( "Object is not a list" ));

	return ( !InList() );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Lst

#endif	//	__CORE_LIST_HEAD_H
