//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       SkitchComponent.cpp
//* OWNER:          Dave Cowling
//* CREATION DATE:  02/19/03
//****************************************************************************

#include <gel/components/skitchcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CBaseComponent* CSkitchComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkitchComponent );	
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CSkitchComponent::CSkitchComponent() : CBaseComponent()
{
	SetType( CRC_SKITCH );

	// Skitching allowed by default.
	m_skitch_allowed = true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CSkitchComponent::~CSkitchComponent()
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkitchComponent::InitFromStructure( Script::CStruct* pParams )
{
	// There needs to be a ObjectHookManagerComponent attached for the SkitchComponent to operate.
	mp_object_hook_manager_component = GetObjectHookManagerComponentFromObject( GetObject());
	Dbg_MsgAssert( mp_object_hook_manager_component, ( "SkitchComponent created without ObjectHookManagerComponent" ));
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkitchComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure( pParams );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CSkitchComponent::Update()
{
	// Doing nothing, so tell code to do nothing next time around
	Suspend(true);
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CBaseComponent::EMemberFunctionResult CSkitchComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		// @script | Obj_AllowSkitching | Enable skitching for this object 
		// @flag off | disable skitching
		case 0xc38cce3b:  // Obj_AllowSkitching
		{
			AllowSkitch(( pParams->ContainsFlag( "off" )) ? false : true );
			break;
		}

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
void CSkitchComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert( p_info, ( "NULL p_info sent to CSkitchComponent::GetDebugInfo" ));
	
	p_info->AddInteger( "m_skitch_allowed", m_skitch_allowed ? 1 : 0 );

	// we call the base component's GetDebugInfo, so we can add info from the common base component
	CBaseComponent::GetDebugInfo( p_info );
#endif				 
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int CSkitchComponent::GetNearestSkitchPoint( Mth::Vector *p_point, Mth::Vector &pos )
{
	int nearest_node_number = -1;	// -1 means not found yet

	// Only continue if we have a parent object (we always should) and a object hook manager component.
	CCompositeObject *p_parent_object = GetObject();
	if( mp_object_hook_manager_component && p_parent_object )
	{
		Obj::CObjectHookManager *p_object_hook_man = mp_object_hook_manager_component->GetObjectHookManager();
		if( p_object_hook_man )
		{
			// Form a transformation matrix.
			Mth::Matrix total_mat	= p_parent_object->GetMatrix();
			total_mat[X][W]			= 0.0f;
			total_mat[Y][W]			= 0.0f;
			total_mat[Z][W]			= 0.0f;
			total_mat[W]			= p_parent_object->GetPos();
			total_mat[W][W]			= 1.0f;

			p_object_hook_man->UpdateTransform( total_mat );

			int node_number = 0;
			CObjectHookNode *p_node		= p_object_hook_man->GetFirstNode();
			CObjectHookNode *p_nearest	= p_node;
			float nearest_dist_squared	= 100000000000000.0f;	
			Mth::Vector nearest_point(0.0f, 0.0f, 0.0f);

			while( p_node )
			{
				// Get the postition of the hook.
				Mth::Vector point = p_object_hook_man->GetPos( p_node );

				// Project the point onto the base plane of the object so we can ignore the height.
				point -= p_parent_object->GetPos();
				point.ProjectToPlane( p_parent_object->GetMatrix()[Y] );
				point += p_parent_object->GetPos();
			
				float dist_squared = ( point - pos ).LengthSqr();
				if( dist_squared < nearest_dist_squared )
				{
					nearest_dist_squared	= dist_squared;
					nearest_point			= point;
					p_nearest				= p_node; 
					nearest_node_number		= node_number;
				}
				p_node = p_node->GetNext();
				node_number++;
			}		
			*p_point = nearest_point;
		 }
	}

	return nearest_node_number;
}



/******************************************************************/
/*                                                                */
/* Given the index of a skitch point, then get the position of	  */
/* that point.													  */
/* Adjust the point to lay on the ground plane of the object.	  */
/* Returns the position in *p_point returns true if object still  */
/* exists, and false otherwise.									  */
/*                                                                */
/******************************************************************/
bool CSkitchComponent::GetIndexedSkitchPoint( Mth::Vector *p_point, int index )
{
	// Only continue if we have a parent object (we always should) and a object hook manager component.
	CCompositeObject *p_parent_object = GetObject();
	if( mp_object_hook_manager_component && p_parent_object )
	{
		Obj::CObjectHookManager *p_object_hook_man = mp_object_hook_manager_component->GetObjectHookManager();
		if( p_object_hook_man )
		{
			// Form a transformation matrix.
			Mth::Matrix total_mat	= p_parent_object->GetMatrix();
			total_mat[X][W]			= 0.0f;
			total_mat[Y][W]			= 0.0f;
			total_mat[Z][W]			= 0.0f;
			total_mat[W]			= p_parent_object->GetPos();
			total_mat[W][W]			= 1.0f;

			p_object_hook_man->UpdateTransform( total_mat );

			int node_number = 0;
			CObjectHookNode *p_node = p_object_hook_man->GetFirstNode();
		
			while( p_node )
			{
				if( node_number == index )
				{
					Mth::Vector point = p_object_hook_man->GetPos( p_node );

					// Project the point onto the base plane of the object so we can ignore the height.
					point -= p_parent_object->GetPos();
					point.ProjectToPlane( p_parent_object->GetMatrix()[Y] );
					point += p_parent_object->GetPos();

					*p_point = point;

					// There was no break here in the original code in MovingObject, but I see no reason
					// to continue the loop iteration at this point.
					break;
				}
				p_node = p_node->GetNext();
				node_number++;
			}
			return true;
		 }
	}
	return false;
}








/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/










}
