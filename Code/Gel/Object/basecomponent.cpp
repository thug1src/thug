//****************************************************************************
//* MODULE:         Gel/Object
//* FILENAME:       basecomponent.cpp
//* OWNER:          Mick West
//* CREATION DATE:  10/17/2002
//****************************************************************************

#include <gel/object/basecomponent.h>
#include <gel/scripting/struct.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent::CBaseComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent::~CBaseComponent()
{
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CBaseComponent::Finalize()
{
	// Virtual function, can be overridden to provided finialization to 
	// a component after all components have been added to an object
}


void CBaseComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	// Default to just calline InitFromStructure()
	// This is primarily for pre-existing components that we 
	// need to update
	//
	// Components that are created from EmptyComponent.cpp/h should
	// have the funtion automatically overridden

	printf ("WARNING:  Refreshing component %s using InitFromStructure\n");  
	InitFromStructure(pParams);
}

void CBaseComponent::ProcessWait( Script::CScript * pScript )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// By default, a component has no member functions
// so attempting to call one will just return MF_NOT_EXECUTED
CBaseComponent::EMemberFunctionResult CBaseComponent::CallMemberFunction( uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript )
{
	return MF_NOT_EXECUTED;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CBaseComponent::Suspend(bool suspend)
{
	m_flags.Set(BC_NO_UPDATE, suspend);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CBaseComponent::Hide( bool shouldHide )
{
	// the base component does nothing,
	// but some components need to do extra stuff
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CBaseComponent::Teleport()
{
	// the base component does nothing,
	// but some components need to do extra stuff
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Used by the script debugger code to fill in a structure
// for transmitting to the monitor.exe utility running on the PC.
void CBaseComponent::GetDebugInfo(Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info sent to CBaseComponent::GetDebugInfo"));
	
	#ifdef __NOPT_ASSERT__
	p_info->AddInteger("CPUTime",m_update_time);
	#endif
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}
