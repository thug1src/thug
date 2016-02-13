/*****************************************************************************
**																			**
**			              Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Core Library											**
**																			**
**	Module:			Task Manager (TSK)										**
**																			**
**	File name:		tstack.cpp												**
**																			**
**	Created by:		05/27/99	-	mjb										**
**																			**
**	Description:	Task Stack Support										**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/task.h>

/*****************************************************************************
**							  DBG Information								**
*****************************************************************************/

namespace Tsk
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/


/*****************************************************************************
**								   Defines									**
*****************************************************************************/


/*****************************************************************************
**								Private Types								**
*****************************************************************************/

class  Stack::StackElement : public  List 
{
	

	friend class Stack;

public :

							StackElement( void );
							~StackElement( void );

private:

	Lst::Node< StackElement >*	m_node;

};

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/


/*****************************************************************************
**								 Public Data								**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/


void Stack::remove_dead_entries( void )
{
	

	Lst::Search< StackElement >	iterator;
	StackElement*	dead_entry;
	 
	Dbg_MsgAssert( HaveAccess(),( "List currently being processed" ));

	while (( dead_entry = iterator.FirstItem( m_dead )))
	{
		delete dead_entry;
	}
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

Stack::StackElement::StackElement( void )
{
	

	m_node = new Lst::Node< StackElement >( this );
	Dbg_AssertType( m_node, Lst::Node< StackElement > );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Stack::StackElement::~StackElement ( void )
{
	

	Dbg_AssertType( m_node, Lst::Node< StackElement > );
	delete m_node;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Stack::Stack ( void )
{
	

	m_pushedList = false;
	m_poppedList = false;

	Push();								// create initial list
	m_add = m_run;	

	m_pushedList = false;
	m_poppedList = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Stack::~Stack ( void )
{
	
	
	Dbg_MsgAssert( HaveAccess(),( "List currently being processed" ));
	
	Dbg_Code
	(
		if ( m_stack.CountItems() != 1 )
		{
			Dbg_Warning( "Task Stack contains %d Task Lists", m_stack.CountItems());
		}
	)

	while ( m_stack.CountItems() )
	{
		Pop ();
	}

	remove_dead_entries();

	Dbg_MsgAssert (( m_run == NULL ),( "Task Stack not empty" ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Stack::Push( void )
{
	/*
		On entry:
		m_add -> top list on stack (the one being processed)
		m_run -> probably the same
		
		On exit:		
		m_run -> new top list
		m_add -> old top list (the one being processed)
	*/
	
	
	Dbg_MsgAssert(!m_pushedList,( "push already called this frame"));

	m_run = new StackElement;

	Dbg_AssertType( m_run, StackElement );

	m_stack.AddToHead( m_run->m_node ); // deferred push
	m_pushedList = true;

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Stack::Pop( void )
{
	/*
		On entry:
		m_add -> top list on stack (the one being processed)
		m_run -> top list on stack
		
		On exit:		
		m_run and m_add -> the new top list (originally second to top)
		m_dead -> list to be destroyed (still being processed)
	*/

		
	Dbg_AssertType( m_run, StackElement );
	// this assert is problematic because the task system does not call Process() on the input
	// task list stacks for inactive devices, though Push() and Pop() are called
	//Dbg_MsgAssert(!m_poppedList,( "pop already called this frame"));
	
	// remove item
	
	Lst::Search< StackElement >	iterator;
	iterator.FirstItem( m_stack );

	// must use this test, m_pushedList is not reliable
	if (m_add == m_run)
		m_add = iterator.NextItem();
	else
	{
		// otherwise, current list is retained, m_add stays the same
		m_pushedList = false;
	}


	// m_run may have been just added this frame, but that shouldn't matter
	Dbg_MsgAssert( m_run->IsEmpty(),( "Tasks List not Empty" )); 	
	if( HaveAccess() )
	{
		delete m_run;
	}
	else
	{
		m_run->m_node->Remove();
		m_dead.AddToHead( m_run->m_node ); // deferred kill
	}

	m_run = m_add;
	m_poppedList = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Stack::Process( bool time )
{
	
	
	Forbid();

	Dbg_AssertType( m_run, StackElement );

	m_add = m_run;							// push now in effect 
	m_run->Process( time, m_mask_off );

	Permit();
	
	remove_dead_entries();
	
	m_pushedList = false;
	m_poppedList = false;
}

void		Stack::Dump( void )
{
	
	
	Forbid();

	Dbg_AssertType( m_run, StackElement );

	m_add = m_run;							// push now in effect 
	m_run->Dump();

	Permit();
	
	remove_dead_entries();
	
	m_pushedList = false;
	m_poppedList = false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Stack::AddTask( BaseTask& task )
{
	
	
	Dbg_AssertType( &task, BaseTask );
	Dbg_AssertType( m_add, StackElement );

	m_add->AddTask( task );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Stack::AddPushTask( BaseTask& task )
{
	
	
	Dbg_AssertType( &task, BaseTask );
	Dbg_AssertType( m_run, StackElement );

	m_run->AddTask( task );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Stack::RemoveAllTasks( void )
{
	
	
	// we may really be removing all tasks in list about to be pushed
	if (m_pushedList)
	{
		RemoveAllPushTasks();
		Dbg_MsgAssert( m_run->IsEmpty(),( "should now be empty" )); 	
		return;
	}
	
	Dbg_AssertType( m_add, StackElement );

	m_add->RemoveAllTasks();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Stack::RemoveAllPushTasks( void )
{
	
	
	Dbg_AssertType( m_run, StackElement );

	m_run->RemoveAllTasks();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Tsk

