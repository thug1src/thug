/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GEL (Game Engine Library)								**
**																			**
**	Module:			Module (MDL)		 									**
**																			**
**	File name:		modman.cpp												**
**																			**
**	Created:		05/27/99	-	mjb										**
**																			**
**	Description:	Module manager code										**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/singleton.h>

#ifdef __NOPT_DEBUG__
#include <sys/timer.h>
#endif

#include <gel/modman.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Mdl
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


/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

DefineSingletonClass( Manager, "Module Manager" );

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

Manager::Manager ( void )
{
	

	process_modules_task = new Tsk::Task< Manager >( process_modules, *this, Tsk::BaseTask::Node::vSYSTEM_TASK_PRIORITY_PROCESS_MODULES );
	Dbg_AssertType( process_modules_task, Tsk::Task< Manager > );

	control_change = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Manager::~Manager( void )
{
	
	
	StopAllModules();
		
	Dbg_AssertType( process_modules_task, Tsk::Task< Manager > );
	delete process_modules_task;
	
	Dbg_MsgAssert( module_list.IsEmpty(),( 
		"%d module%s still registered\n",
			module_list.CountItems(),
			module_list.CountItems() > 1 ? "s" : "" ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::process_modules( const Tsk::Task< Manager >& task )
{
	

	Module*					next;
	Module*					current;
	Lst::Search< Module >	sh;

	Manager&	manager = task.GetData();

	if ( !manager.control_change )
	{
		return; // nothing to do
	}

	manager.control_change = false;
	
	next = sh.FirstItem( manager.module_list );

	while ( next )
	{
		current = next;
		next = sh.NextItem();			// get next item before excuting callback

		switch ( current->state )
		{
			case Module::vSTOPPED :
			{
				if (( current->command == Module::vSTART ) ||
					( current->command == Module::vRESTART ))
				{
#ifndef __PLAT_XBOX__
#ifndef __PLAT_NGC__
#ifdef __NOPT_DEBUG__
				Dbg_Notify ( "Starting module %s @ %d", current->GetName(), 
					Tmr::GetTime() ); // should use Game Clock not system clock
#endif
#endif
#endif
					current->v_start_cb();
					current->state = Module::vRUNNING;
					current->command = Module::vNONE;
				}
				
				break;
			}
			
			case Module::vRUNNING :
			{
				if ( current->command == Module::vSTOP )
				{
#ifndef __PLAT_XBOX__
#ifndef __PLAT_NGC__
#ifdef __NOPT_DEBUG__
					Dbg_Notify ( "Stopping module %s @ %d", current->GetName(), 
						 Tmr::GetTime() ); // should use Game Clock not system clock
#endif
#endif
#endif
					current->v_stop_cb();
					current->state = Module::vSTOPPED;
					current->command = Module::vNONE;
				}
				else if ( current->command == Module::vRESTART ) 
				{
#ifndef __PLAT_XBOX__
#ifndef __PLAT_NGC__
#ifdef __NOPT_DEBUG__
					Dbg_Notify ( "Restarting module %s @ %d", current->GetName(), 
						Tmr::GetTime() ); // should use Game Clock not system clock
#endif
#endif
#endif
					current->v_stop_cb();
					current->v_start_cb();               
					current->command = Module::vNONE;
				}

				break;
			}        

			default:
			{
				Dbg_MsgAssert( false,( "Invalid module control state" ));
			}
		}
	}
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

void	Manager::RegisterModule( Module &module )
{
	
	
	Dbg_AssertType ( &module, Module );

	module_list.AddToTail ( module.node );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::UnregisterModule( Module &module )
{
	
	
	Dbg_AssertType( &module, Module );

	Dbg_MsgAssert( module.node->InList(),( "Module not Registered with Manager" ));

	module.node->Remove();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::StartModule( Module &module )
{
	
	
	Dbg_MsgAssert( module.node->InList(),( "Module not Registered with Manager" ));

	if ( !module.Locked() )
	{
		module.command = Module::vSTART;
		control_change = true;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::RestartModule( Module &module )
{
	
	
	Dbg_MsgAssert( module.node->InList(),( "Module not Registered with Manager" ));

	if ( !module.Locked() )
	{
		module.command = Module::vRESTART;
		control_change = true;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::StopModule( Module &module )
{
	

	if ( !module.Locked() )
	{
		module.command = Module::vSTOP;
		control_change = true;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::StartAllModules( void )
{
	

	Module*					next;
	Module*					mdl;
	Lst::Search< Module >	sh;

	next = sh.FirstItem( module_list );

	while ( next )
	{
		mdl = next;				
		next = sh.NextItem();	// get next item before excuting callback

		if ( !mdl->Locked() )
		{
			if ( mdl->state == Module::vSTOPPED )
			{
				mdl->v_start_cb();
				mdl->command = Module::vNONE;
				mdl->state = Module::vRUNNING;
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::StopAllModules( void )
{
	

	Module*					next;
	Module*					mdl;
	Lst::Search< Module >	sh;

	next = sh.FirstItem( module_list );

	while ( next )
	{
		mdl = next;				
		next = sh.NextItem();	// get next item before excuting callback
								
		if ( !mdl->Locked() )
		{
			if ( mdl->state == Module::vRUNNING )
			{
				mdl->v_stop_cb();
				mdl->command = Module::vNONE;
				mdl->state = Module::vSTOPPED;
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::LockAllModules( void )
{
	

	Module*					mdl;
	Lst::Search< Module >	sh;

	mdl = sh.FirstItem( module_list );

	while ( mdl )
	{
		mdl->Lock();
		mdl = sh.NextItem();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			Manager::UnlockAllModules ( void )
{
	

	Module*					mdl;
	Lst::Search< Module >	sh;

	mdl = sh.FirstItem ( module_list );

	while ( mdl )
	{
		mdl->Unlock();
		mdl = sh.NextItem();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mdl
