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
**	File name:		core/list/node.h										**
**																			**
**	Created: 		05/27/99	-	mjb										**
**																			**
*****************************************************************************/

#ifndef	__CORE_LIST_Node_H
#define	__CORE_LIST_Node_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/support.h>

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
 * Class:			Lst::Node
 *
 * Description:		Linked-list node. 
 *
 ***********************************************************************/

nTemplateBaseClass( _T, Node )
{
	

public:

	// GJ:  Note we have to cast vINT_MIN to an sint
	// ( ((sint)vINT_MIN) / 2 ) ends up positive.
		
	enum 
	{
		vNORMAL_PRIORITY	= 0,
		vHEAD_NODE			= vUINT32_MAX
	};

	enum
	{
		vSYSTEM_TASK_PRIORITY_PROCESS_MODULES		= -1000,
		vSYSTEM_TASK_PRIORITY_FLUSH_DEAD_OBJECTS	= 1000,
	};
	
	enum
	{
		vLOGIC_TASK_PRIORITY_REPLAY_END_FRAME				= -3000,
		vLOGIC_TASK_PRIORITY_PROCESS_NETWORK_METRICS		= -2500,
		vLOGIC_TASK_PRIORITY_SERVER_SEND_NETWORK_DATA		= -2000,
		vLOGIC_TASK_PRIORITY_CLIENT_SEND_NETWORK_DATA	    = -2000,
		vLOGIC_TASK_PRIORITY_TIMEOUT_CONNECTIONS			= -2000,
		vLOGIC_TASK_PRIORITY_CLIENT_ADD_NEW_PLAYERS			= -2000,
		vLOGIC_TASK_PRIORITY_SERVER_ADD_NEW_PLAYERS		    = -2000,
		vLOGIC_TASK_PRIORITY_FRONTEND					    = -2000,
		vLOGIC_TASK_PRIORITY_SCRIPT_DEBUGGER				= -1500,
		vLOGIC_TASK_PRIORITY_LOCKED_OBJECT_MANAGER_LOGIC    = -1100,
		vLOGIC_TASK_PRIORITY_PARTICLE_MANAGER_LOGIC		    = -1000,
		vLOGIC_TASK_PRIORITY_HANDLE_KEYBOARD			    = -1000,
		vLOGIC_TASK_PRIORITY_OBJECT_UPDATE				    = -1000,
		vLOGIC_TASK_PRIORITY_SCORE_UPDATE				    = -1000,
        vLOGIC_TASK_PRIORITY_COMPOSITE_MANAGER              = -900,
		vLOGIC_TASK_PRIORITY_PROCESS_NETWORK_DATA	    	= 0,
		vLOGIC_TASK_PRIORITY_SKATER_SERVER				    = 1000,
		vLOGIC_TASK_PRIORITY_TRANSFER_NETWORK_DATA		    = 1000,
		vLOGIC_TASK_PRIORITY_RECEIVE_NETWORK_DATA		    = 1000,
		vLOGIC_TASK_PRIORITY_PROCESS_HANDLERS			    = 2000,
		vLOGIC_TASK_PRIORITY_REPLAY_START_FRAME				= 3000,
    };
	
	enum
	{
		vDISPLAY_TASK_PRIORITY_PARK_EDITOR_DISPLAY	= -1000,
		vDISPLAY_TASK_PRIORITY_CPARKEDITOR_DISPLAY	= -1000,
		vDISPLAY_TASK_PRIORITY_IMAGE_VIEWER_DISPLAY	= -1000,
		vDISPLAY_TASK_PRIORITY_PANEL_DISPLAY		= -500,
	};
		
	enum
	{
		vHANDLER_PRIORITY_OBSERVER_INPUT_LOGIC		= -1000,
		vHANDLER_PRIORITY_FRONTEND_INPUT_LOGIC0		= -1000,
		vHANDLER_PRIORITY_FRONTEND_INPUT_LOGIC1		= -1000,
		vHANDLER_PRIORITY_IMAGE_VIEWER_INPUT_LOGIC	= 1000,
		vHANDLER_PRIORITY_VIEWER_SHIFT_INPUT_LOGIC	= 1000,
	};
		
	typedef sint		Priority;

						Node ( _T* d, Priority p = vNORMAL_PRIORITY );
	virtual				~Node ( void );

	void				Insert ( Node< _T >* node );
	void				Append ( Node< _T >* node );
	void 				Remove ( void );	
	
	void				SetPri  ( const Priority priority );
	void				SetNext ( Node< _T >* node );
	void				SetPrev ( Node< _T >* node );

	Priority			GetPri  ( void ) const;
	Node< _T >*			GetNext ( void ) const;
	Node< _T >*			GetPrev ( void ) const;
	_T*					GetData ( void ) const;	

	Node< _T >*			LoopNext ( void ) const;
	Node< _T >*			LoopPrev ( void ) const;

	bool				InList ( void ) const;

protected:

	bool				is_head ( void ) const;
	void				node_init ( void );

private:

	_T*					data;
	Priority			pri;
	Node< _T >*			next; 
	Node< _T >*			prev;
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
**							Private Inline Functions						**
*****************************************************************************/

template < class _T > inline
void		Node< _T >::node_init( void )
{
	
	
	next = this;
	prev = this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline
bool		Node< _T >::is_head( void ) const
{
	

	return ( data == reinterpret_cast<void*>( vHEAD_NODE ));
}

/*****************************************************************************
**							Public Inline Functions							**
*****************************************************************************/

template < class _T > inline
Node< _T >::Node( _T* d, Priority p )
: data( d ), pri( p )
{
	

	node_init();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// Must define inline functions before they are used
template < class _T > inline	
void 	Node< _T >::Remove( void )
{
	

	prev->next = next;
	next->prev = prev;
	
	node_init();		// so we know that the node is not in a list
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline
Node< _T >::~Node( void )
{
	

	Remove();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// Mick:  Moved this here from the end of the file
// as it's intended to b inline
// yet it is used by other functions below

template < class _T > inline	
bool		Node< _T >::InList( void ) const
{
	

	return ( prev != this );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline	
void	Node< _T >::Insert ( Node< _T >* node )
{
	
	
	Dbg_AssertType( node, Node< _T > );
	Dbg_MsgAssert( !node->InList(),("node is already in a list" ));

	node->prev = prev;
	node->next = this;
	prev->next = node;
	prev = node;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline	
void	Node< _T >::Append( Node< _T >* node )
{
	
	
	Dbg_AssertType( node, Node< _T > );
	Dbg_MsgAssert( !node->InList(),( "node is already in a list" ));

	node->prev = this;
	node->next = next;
	next->prev = node;
	next = node;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline	
void	Node< _T >::SetPri( const Priority priority )
{
	

	pri = priority;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline	
void	Node< _T >::SetNext( Node< _T >* node )
{
	
	
	Dbg_AssertType( node, Node< _T > );

	next = node;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline	
void	Node< _T >::SetPrev( Node< _T >* node )
{
	
	
	Dbg_AssertType( node, Node< _T > );

	prev = node;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline	
typename Node< _T >::Priority Node< _T >::GetPri( void ) const
{
	return pri;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline	
Node< _T >*		Node< _T >::GetNext( void ) const
{
	
	
	if ( next->is_head() )
	{
		return NULL;
	}

	return next;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline	
Node< _T >*		Node< _T >::GetPrev( void ) const
{
	

	if ( prev->is_head() )
	{
		return NULL;
	}

	return prev;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline	
_T*		Node< _T >::GetData( void ) const
{
	
	
	return data;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline	
Node< _T >*		Node< _T >::LoopNext( void ) const
{
	

	Node< _T >*		next_node = next;

	if ( next_node->is_head() )
	{
		next_node = next_node->next;	// skip head node
	
		if ( next_node->is_head() )
		{
			return NULL;				// list is empty
		}
	}

	return next_node;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline	
Node< _T >*		Node< _T >::LoopPrev( void ) const
{
	

	Node< _T >*		prev_node = prev;

	if ( prev_node->is_head() )
	{
		prev_node = prev_node->prev;	// skip head node
	
		if ( prev_node->is_head() )
		{
			return NULL;				// list is empty
		}
	}

	return prev_node;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Lst

#endif	//	__CORE_LIST_Node_H
