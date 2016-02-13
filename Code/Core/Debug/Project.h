/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**						   Confidential Information						   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Core Library											**
**																			**
**	Module:			Debug (Dbg_)											**
**																			**
**	File name:		core/debug/project.h									**
**																			**
**	Created:		05/27/99	-	mjb										**
**																			**
*****************************************************************************/


#ifndef __CORE_DEBUG_PROJECT_H
#define __CORE_DEBUG_PROJECT_H

#ifdef __NOPT_DEBUG__

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include "mem_stat.h"

/*****************************************************************************
**								   Defines									**
*****************************************************************************/


/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/

namespace Dbg
{

class Project
{

public :

					Project ( const char& name, const char& description );

	void			SetChildren ( Dbg::Module* children );
	Dbg::Module*		GetChildren ( void ) const;

private :

	const char&			name;
	const char&			description;

	Project*			next;				// pointer to next registered project
	Dbg::Module*			children;			// pointer to children modules 

	Dbg_MEMORY_STATS	stats;

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

extern	Dbg::Project*	RegisteredProjects;

} // namespace Dbg

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/


/*****************************************************************************
**									Macros									**
*****************************************************************************/

#define	Dbg_DefineProject(proj,des)									\
																	\
Dbg::Project&	Dbg_project_##proj ( void )							\
{																	\
	static Dbg::Project project ( *#proj, *des );					\
																	\
	return project;													\
}
																	\
/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

namespace Dbg
{

inline	Project::Project ( const char& name, const char& description )
:	name ( name ), description ( description )
{
	next = RegisteredProjects;
	RegisteredProjects = this;
	children = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void			Project::SetChildren ( Dbg::Module* children_in )
{
	children = children_in;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	Dbg::Module*		Project::GetChildren ( void ) const
{
	return children;
}

} // namespace Dbg


/*****************************************************************************
**									Stubs									**
*****************************************************************************/

#else

#define	Dbg_DefineProject(proj,des)

#endif	//__NOPT_DEBUG__												

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#endif	// __CORE_DEBUG_PROJECT_H




