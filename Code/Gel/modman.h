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
**	Module:			Module  (MDL)											**
**																			**
**	File name:		gel/modman.h											**
**																			**
**	Created: 		05/27/99	-	mjb										**
**																			**
*****************************************************************************/

#ifndef __GEL_MODMAN_H
#define __GEL_MODMAN_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/support.h>
#include <core/singleton.h>
#include <core/list.h>
#include <core/task.h>
#include <gel/module.h>
					  
/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Mdl
{



/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class  Manager  : public Spt::Class
{
	

public :
	
	void						RegisterModule ( Module &module );
	void						UnregisterModule ( Module &module );

	void						StartModule ( Module &module );
	void						RestartModule( Module &module );
	void						StopModule ( Module &module );
	void						StartAllModules ( void );
	void						StopAllModules ( void );

	void						LockAllModules ( void );
	void						UnlockAllModules ( void );

	Tsk::BaseTask&				GetProcessModulesTask ( void ) const;
	

private :
								Manager ( void );
								~Manager ( void );

	static Tsk::Task< Manager >::Code	process_modules;
								
	Tsk::Task< Manager >*		process_modules_task;
	Lst::Head< Module >			module_list;
	bool						control_change;
		
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
**								Inline Functions							**
*****************************************************************************/

inline	Tsk::BaseTask&	Manager::GetProcessModulesTask ( void ) const
{
   	
	
	Dbg_AssertType ( process_modules_task, Tsk::BaseTask );

	return	*process_modules_task;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mdl

#endif	// __GEL_MODMAN_H
