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
**	File name:		core/debug/module.h										**
**																			**
**	Created:		05/27/99	-	mjb										**
**																			**
*****************************************************************************/

#ifndef __CORE_DEBUG_MODULE_H
#define __CORE_DEBUG_MODULE_H


#ifdef __NOPT_DEBUG__

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include "mem_stat.h"

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

// Unfortunately, Visual C++ does not support the __PRETTY_FUNCTION__ predefined
// name. The most information we can get at is __FILE__.
#if ( defined ( __PLAT_XBOX__ ) || defined ( __PLAT_WN32__ ))
#define __PRETTY_FUNCTION__	__FILE__
#endif

/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/

namespace Dbg
{

class Project;

class Module
{

public :

						Module ( Project& proj, const char& prefix, const char& description );
	
	void				SetNext ( const Module* next );
	void				SetSibling ( const Module* sibling );

	const Module*		GetNext ( void )	const;
	const Module*		GetSibling ( void ) const;

private :

	const Module*		m_next;			// pointer to next registered module
	const Module*		m_sibling;		// pointer to next module in same project

	const char&			m_prefix;		// module prefix
	const char&			m_description;	// module description
	
	Project&			m_project;		// project that this modules belongs to
	Dbg_MEMORY_STATS	m_stats;
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

extern	Module*		RegisteredModules;

} // namespace Dbg

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/


/*****************************************************************************
**									Macros									**
*****************************************************************************/


/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

namespace Dbg
{

inline	Module::Module( Project& proj, const char& pref, const char& desc )
:	m_prefix ( pref ),
	m_description ( desc ),
	m_project ( proj )
				
{
	m_next = Dbg::RegisteredModules;		// add module to main registration list
	Dbg::RegisteredModules = this;

	m_sibling = m_project.GetChildren();	// add module to its project parent
	m_project.SetChildren( this );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void				Module::SetNext( const Module* next )
{
	m_next = next;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void				Module::SetSibling( const Module* sibling )
{
	m_sibling = sibling;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	const Module*		Module::GetNext( void ) const
{
	return m_next;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	const Module*		Module::GetSibling( void ) const
{
	return m_sibling;
}

} // namespace Dbg

/*****************************************************************************
**									Stubs									**
*****************************************************************************/

#else

#endif	//__NOPT_DEBUG__												

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#endif	// __CORE_DEBUG_MODULE_H




