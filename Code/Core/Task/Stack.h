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
**	File name:		core/task/stack.h										**
**																			**
**	Created:		05/27/99	-	mjb										**
**																			**
*****************************************************************************/

#ifndef	__CORE_TASK_STACK_H
#define	__CORE_TASK_STACK_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/task/task.h>
#include <core/task/list.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Tsk
{



/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class  Stack : public  Spt::Access 
{
	

public :
							Stack ( void );
							~Stack ( void );

	void					Push ( void );
	void					Pop ( void );
	void					Process ( bool time = false);
	void					Dump ( void );
	void					AddTask ( BaseTask& task );
	void					AddPushTask ( BaseTask& task );
	void					RemoveAllPushTasks ( void );
	void					RemoveAllTasks ( void );
	void 					SetMask(uint mask);

private :

	uint					m_mask_off; 			// mask to supress execution of tasks 

	void					remove_dead_entries( void );

	class StackElement;					// forward declaration

	Lst::Head< StackElement >	m_stack;
	Lst::Head< StackElement >	m_dead;	// dead element list
	StackElement*				m_run;	// current run task list
	StackElement*				m_add;	// current add task list

	// these get reset when Process() is called
	bool					m_pushedList;
	bool					m_poppedList;
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

inline void Stack::SetMask(uint mask)
{
	m_mask_off = mask;
}

} // namespace Tsk

#endif	// __CORE_TASK_STACK_H


