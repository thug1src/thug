//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       TriggerComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/3/3
//****************************************************************************

#include <gel/components/triggercomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/components/nodearraycomponent.h>

#include <sk/engine/feeler.h>
#include <sk/scripting/nodearray.h>
#include <sk/modules/skate/skate.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CTriggerComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CTriggerComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTriggerComponent::CTriggerComponent() : CBaseComponent()
{
	SetType( CRC_TRIGGER );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CTriggerComponent::~CTriggerComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTriggerComponent::InitFromStructure( Script::CStruct* pParams )
{
	m_pos_last_frame = GetObject()->GetPos();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTriggerComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTriggerComponent::Update()
{
	if (m_pos_last_frame == GetObject()->GetPos()) return;
	
	if (GetObject()->HasTeleported())
	{
		m_pos_last_frame = GetObject()->GetPos();
		return;
	}
	
	CFeeler feeler;
	
	feeler.m_start = m_pos_last_frame;
	feeler.m_end = GetObject()->GetPos();
	
	feeler.SetIgnore(0, mFD_NON_COLLIDABLE | mFD_TRIGGER);
	
	feeler.SetCallback(s_through_trigger_callback);
	feeler.SetCallbackData(this);
	
	feeler.GetCollision();
	
	m_pos_last_frame = GetObject()->GetPos();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CTriggerComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
        // @script | TriggerType | checks if the trigger type is the 
        // same as the specified type.  can also accept an array of types
        // to check
        // @uparmopt name | trigger type
        // @uparmopt [] | array of trigger types
		case CRCC(0x3a717382, "TriggerType"):
		{
			Script::CArray* pArray = NULL;
			pParams->GetArray(NO_NAME, &pArray);
			if (pArray)
			{
				for (int n = pArray->GetSize(); n--; )
				{
					if (m_latest_trigger_event_type == Script::GetInteger(pArray->GetChecksum(n))) return CBaseComponent::MF_TRUE;
				}
				return CBaseComponent::MF_FALSE;
			}
			else
			{
				int type;
				pParams->GetInteger(NO_NAME, &type, Script::ASSERT);
				return m_latest_trigger_event_type == type ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;   
			} 	
			break;
		}
		
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTriggerComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CTriggerComponent::GetDebugInfo"));

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTriggerComponent::CheckFeelerForTrigger ( TriggerEventType type, CLineFeeler& feeler )
{
	if (!feeler.GetTrigger() || !feeler.GetNodeChecksum()) return;

	if (feeler.GetCallbackObject())
	{
		CNodeArrayComponent* p_node_array_component = GetNodeArrayComponentFromObject(feeler.GetCallbackObject());
		
		TripTrigger(type, feeler.GetNodeChecksum(), p_node_array_component ? p_node_array_component->GetNodeArray() : NULL, feeler.GetCallbackObject());
	}
	else if (feeler.IsMovableCollision())
	{
		if (feeler.GetMovingObject())
		{
			CNodeArrayComponent* p_node_array_component = GetNodeArrayComponentFromObject(feeler.GetMovingObject());
			TripTrigger(type, feeler.GetNodeChecksum(), p_node_array_component ? p_node_array_component->GetNodeArray() : NULL, feeler.GetMovingObject());
		}
	}
	else
	{
		TripTrigger(type, feeler.GetNodeChecksum());
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTriggerComponent::CheckFeelerForTrigger ( TriggerEventType type, CRectFeeler& feeler )
{
	for (int n = feeler.GetNumCollisionSurfaces(); n--; )
	{
		const Nx::S2DCollSurface& coll_surface = feeler.GetCollisionSurface(n);
		if (!coll_surface.trigger) continue;
		
		// only trip the node's trigger if we haven't yet
		int m;
		for (m = n + 1; m < feeler.GetNumCollisionSurfaces(); m++)
		{
			const Nx::S2DCollSurface& handled_coll_surface = feeler.GetCollisionSurface(m);
			if (!coll_surface.trigger) continue;
			if (coll_surface.node_name == handled_coll_surface.node_name) break;
		}
		if (m == feeler.GetNumCollisionSurfaces())
		{
			TripTrigger(type, coll_surface.node_name);
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTriggerComponent::TripTrigger ( TriggerEventType type, uint32 node_checksum, Script::CArray* p_node_array, CCompositeObject* p_object)
{
	int node;
	Script::CStruct* p_node = NULL;
	
	// find the triggered node
	if (!p_node_array)
	{
		node = SkateScript::FindNamedNode(node_checksum);
		Dbg_MsgAssert(node != -1, ("Can't find node %s, maybe .SCN out of sync with .QN ?",Script::FindChecksumName(node_checksum)));
		p_node_array = Script::GetArray(CRCD(0xc472ecc5, "NodeArray"));
		p_node = p_node_array->GetStructure(node);
	}
	else
	{
		for (node = p_node_array->GetSize(); node--; )
		{
			p_node = p_node_array->GetStructure(node);
			
			uint32 checksum;
			p_node->GetChecksum(CRCD(0xa1dc81f9, "name"), &checksum);
			if (checksum == node_checksum) break;
		}
	}
	
	Dbg_MsgAssert(node != -1, ("Cannot find node %s", Script::FindChecksumName(node_checksum)));
	
	// get the triggered object's trigger script parameters
	Script::CStruct* p_model_trigger_script_params = NULL;
	if (p_object)
	{
		Script::CStruct* p_object_node = Script::GetArray(CRCD(0xc472ecc5, "NodeArray"))->GetStructure(SkateScript::FindNamedNode(p_object->GetID()));
		if (p_object_node)
		{
			p_object_node->GetStructure(CRCD(0x5180a4db, "ModelTriggerScriptParams"), &p_model_trigger_script_params);
		}
	}
	
	// get the trigger script
	uint32 script_checksum = 0;
	p_node->GetChecksum(CRCD(0x2ca8a299, "TriggerScript"), &script_checksum);
	if (script_checksum == 0) return;
	
	// if this is a net game, don't trigger things from "Absent in netgame" nodes
	if (Mdl::Skate::Instance()->ShouldBeAbsentNode(p_node)) return;
	
	m_latest_trigger_event_type = type;
	
	GetObject()->SpawnAndRunScript(
		script_checksum,
		node,
		p_node->ContainsFlag(CRCD(0x20209c31, "NetEnabled")),
		p_node->ContainsFlag(CRCD(0x23627fd7, "Permanent")),
		p_model_trigger_script_params
	);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTriggerComponent::s_through_trigger_callback ( CFeeler* p_feeler )
{
	static_cast< CTriggerComponent* >(p_feeler->GetCallbackData())->CheckFeelerForTrigger(TRIGGER_THROUGH, *p_feeler);
}

}
