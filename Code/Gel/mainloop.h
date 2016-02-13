/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GEL (Game Engine Library)								**
**																			**
**	Module:			Main Loop Module (ML)									**
**																			**
**	File name:		gel/mainloop.h											**
**																			**
**	Created: 		05/27/99	-	mjb										**
**																			**
*****************************************************************************/

#ifndef __GEL_MAINLOOP_H
#define __GEL_MAINLOOP_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/singleton.h>
#include <core/task.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/
   
namespace Mlp
{



/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class  Manager  : public Spt::Class
{
	

public :

	enum ELogicMask
	{
		mNONE			= 0,
		mSKATE_MOD 		= (1<<1), // skate module
		mGAME_OBJECTS	= (1<<2), // game objects
		mGFX_MANAGER	= (1<<3), // gfx manager
	};

	void				RegisterRenderStartHook ( Tsk::BaseHook* start_hook );
	void				RegisterRenderEndHook ( Tsk::BaseHook* end_hook );

	Tsk::BaseHook*		GetRenderStartHook( void );
	Tsk::BaseHook*		GetRenderEndHook( void );
	
	void				MainLoop ( void );
	void				QuitLoop ( void );
	void				DoGameLogic( void );

	void				PauseDisplayTasks( bool pause = true );
	void				RemoveAllDisplayTasks ( void );
	void				RemoveAllLogicTasks ( void );
	void				RemoveAllSystemTasks ( void );
	
	void				PushLogicTasks ( void );
	void				PushDisplayTasks ( void );
	void				PushSystemTasks ( void );
	
	void				PopLogicTasks ( void );
	void				PopDisplayTasks ( void );
	void				PopSystemTasks ( void );

	void				AddLogicTask ( Tsk::BaseTask& task );
	void				AddDisplayTask ( Tsk::BaseTask& task );
	void				AddSystemTask ( Tsk::BaseTask& task );

	void				AddLogicPushTask ( Tsk::BaseTask& task );
	void				AddDisplayPushTask ( Tsk::BaseTask& task );
	void				AddSystemPushTask ( Tsk::BaseTask& task );

	void				ProfileTasks(int n=1);
	void				SetLogicMask ( uint mask );

	bool				IsProfiling(){return currently_profiling;}
	
private :
						Manager ( void );
						~Manager ( void );

	void				game_logic ( void );
	void				render_frame ( void );
	void				service_system ( void );

	bool				done;
	Tsk::Stack			logic_task_stack;
	Tsk::Stack			display_task_stack;
	Tsk::Stack			system_task_stack;
	Tsk::BaseHook*		start_render_hook;
	Tsk::BaseHook*		end_render_hook;

	bool				display_tasks_paused;
	
	int				trigger_profiling;
	int				currently_profiling;
	
	DeclareSingletonClass( Manager );
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
**									Macros									**
*****************************************************************************/


/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

inline	void		Manager::RegisterRenderStartHook ( Tsk::BaseHook* start_hook )
{
   	

    start_render_hook = start_hook;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		Manager::RegisterRenderEndHook ( Tsk::BaseHook* end_hook )
{
   	
	
	end_render_hook = end_hook;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Tsk::BaseHook*	Manager::GetRenderStartHook( void )
{
	

	return start_render_hook;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Tsk::BaseHook*	Manager::GetRenderEndHook( void )
{
	

	return end_render_hook;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		Manager::AddLogicTask ( Tsk::BaseTask& task )
{
   	
	
	Dbg_AssertType ( &task, Tsk::BaseTask );

	logic_task_stack.AddTask ( task );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		Manager::SetLogicMask ( uint mask )
{
   	

	logic_task_stack.SetMask ( (uint) mask );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		Manager::AddDisplayTask ( Tsk::BaseTask& task )
{
   	
	
	Dbg_AssertType ( &task, Tsk::BaseTask );

	display_task_stack.AddTask ( task );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		Manager::AddSystemTask ( Tsk::BaseTask& task )
{
   	
	
	Dbg_AssertType ( &task, Tsk::BaseTask );

	system_task_stack.AddTask ( task );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		Manager::AddSystemPushTask ( Tsk::BaseTask& task )
{
   	
	
	Dbg_AssertType ( &task, Tsk::BaseTask );

	system_task_stack.AddPushTask ( task );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		Manager::AddLogicPushTask ( Tsk::BaseTask& task )
{
   	
	
	Dbg_AssertType ( &task, Tsk::BaseTask );

	logic_task_stack.AddPushTask ( task );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		Manager::AddDisplayPushTask ( Tsk::BaseTask& task )
{
   	
	
	Dbg_AssertType ( &task, Tsk::BaseTask );

	display_task_stack.AddPushTask ( task );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		Manager::RemoveAllDisplayTasks ( void )
{
   	

	display_task_stack.RemoveAllTasks ();
}

inline void		Manager::PauseDisplayTasks( bool pause )
{
   	
	display_tasks_paused = pause;

}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		Manager::RemoveAllLogicTasks ( void )
{
   	

	logic_task_stack.RemoveAllTasks ();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		Manager::RemoveAllSystemTasks ( void )
{
   	

	system_task_stack.RemoveAllTasks ();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		Manager::PushLogicTasks ( void )
{
   	

	logic_task_stack.Push ();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		Manager::PushDisplayTasks ( void )
{
   	

	display_task_stack.Push ();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		Manager::PushSystemTasks ( void )
{
   	

	system_task_stack.Push ();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		Manager::PopLogicTasks ( void )
{
   	

	logic_task_stack.Pop ();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		Manager::PopDisplayTasks ( void )
{
   	

	display_task_stack.Pop ();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void		Manager::PopSystemTasks ( void )
{
   	

	system_task_stack.Pop ();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mlp

#endif	// __GEL_MAINLOOP_H


