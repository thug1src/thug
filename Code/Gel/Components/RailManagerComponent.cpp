//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       RailManagerComponent.cpp
//* OWNER:          Dave Cowling
//* CREATION DATE:  02/17/03
//****************************************************************************

// The CEmptyComponent class is an skeletal version of a component
// It is intended that you use this as the basis for creating new
// components.  
// To create a new component called "Watch", (CWatchComponent):
//  - copy emptycomponent.cpp/.h to watchcomponent.cpp/.h
//  - in both files, search and replace "Empty" with "Watch", preserving the case
//  - in WatchComponent.h, update the CRCD value of CRC_WATCH
//  - in CompositeObjectManager.cpp, in the CCompositeObjectManager constructor, add:
//		  	RegisterComponent(CRC_WATCH,			CWatchComponent::s_create); 
//  - and add the include of the header
//			#include <gel/components/watchcomponent.h> 
//  - Add it to build\gel.mkf, like:
//          $(NGEL)/components/WatchComponent.cpp\
//  - Fill in the OWNER (yourself) and the CREATION DATE (today's date) in the .cpp and the .h files
//	- Insert code as needed and remove generic comments
//  - remove these comments
//  - add comments specfic to the component, explaining its usage
#include <gel/components/railmanagercomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/struct.h>

#include <gel/components/nodearraycomponent.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// s_create is what is registered with the component factory 
// object, (currently the CCompositeObjectManager) 
// s_create	returns a CBaseComponent*, as it is to be used
// by factor creation schemes that do not care what type of
// component is being created
// **  after you've finished creating this component, be sure to
// **  add it to the list of registered functions in the
// **  CCompositeObjectManager constructor  
CBaseComponent* CRailManagerComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CRailManagerComponent );	
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// All components set their type, which is a unique 32-bit number
// (the CRC of their name), which is used to identify the component	
CRailManagerComponent::CRailManagerComponent() : CBaseComponent()
{
	SetType( CRC_RAILMANAGER );
	mp_rail_manager = NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CRailManagerComponent::~CRailManagerComponent()
{
	// Remove the CRailManager if present.
	if( mp_rail_manager )
	{
		delete mp_rail_manager;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// InitFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CRailManagerComponent::InitFromStructure( Script::CStruct* pParams )
{
	// There needs to be a NodeArrayComponent attached for the RailManagerComponent to operate.
	CNodeArrayComponent *p_nodearray_component = GetNodeArrayComponentFromObject( GetObject());
	Dbg_MsgAssert( p_nodearray_component, ( "RailManagerComponent created without NodeArrayComponent" ));

	// Remove any existing CRailManager.
	if( mp_rail_manager )
	{
		delete mp_rail_manager;
	}

	// Create a rail manager, and parse the node array for the rails.
	mp_rail_manager = new Obj::CRailManager();
	mp_rail_manager->AddRailsFromNodeArray( p_nodearray_component->GetNodeArray());
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// RefreshFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CRailManagerComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure( pParams );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// The component's Update() function is called from the CCompositeObject's 
// Update() function.  That is called every game frame by the CCompositeObjectManager
// from the s_logic_code function that the CCompositeObjectManager registers
// with the task manger.
void CRailManagerComponent::Update()
{
	// Doing nothing, so tell code to do nothing next time around
	Suspend(true);
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// Given the "Checksum" of a script command, then possibly handle it
// if it's a command that this component will handle	
CBaseComponent::EMemberFunctionResult CRailManagerComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch( Checksum )
	{
/*
        // @script | DoSomething | does some functionality
		case 0xbb4ad101:		// DoSomething
			DoSomething();
			break;

        // @script | ValueIsTrue | returns a boolean value
		case 0x769260f7:		// ValueIsTrue
		{
			return ValueIsTrue() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}
		break;
*/

		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}

	// the "default" case of the switch statement handles
	// unrecognized functions;  if we make it down here,
	// that means that the component already handled it
	// somehow
    return CBaseComponent::MF_TRUE;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CRailManagerComponent::GetDebugInfo( Script::CStruct *p_info )
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert( p_info, ("NULL p_info sent to CRailManagerComponent::GetDebugInfo" ));

	// Add the number of Rail Manager nodes.
	if( mp_rail_manager )
	{
		p_info->AddInteger("RailManager.m_num_nodes", mp_rail_manager->GetNumNodes());
		p_info->AddString("RailManager.m_is_transformed", mp_rail_manager->IsMoving() ? "yes" : "no" );
	}

	// We call the base component's GetDebugInfo, so we can add info from the common base component.
	CBaseComponent::GetDebugInfo( p_info );
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Matrix CRailManagerComponent::UpdateRailManager (   )
{
	Dbg_Assert(mp_rail_manager);

	// form a transformation matrix
	Mth::Matrix total_mat = GetObject()->GetMatrix();
	total_mat[X][W] = 0.0f;
	total_mat[Y][W] = 0.0f;
	total_mat[Z][W] = 0.0f;
	total_mat[W] = GetObject()->GetPos();
	total_mat[W][W] = 1.0f;

	mp_rail_manager->UpdateTransform(total_mat);
	
	return total_mat;
}
	
}
