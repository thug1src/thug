//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       SpecialItemComponent.cpp
//* OWNER:          ???
//* CREATION DATE:  ???
//****************************************************************************

#include <gel/components/specialitemcomponent.h>

#include <gel/objman.h>
#include <gel/components/lockobjcomponent.h>
#include <gel/components/modelcomponent.h>
#include <gel/net/client/netclnt.h>
#include <gel/object/compositeobject.h>

#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

#include <gfx/nxmodel.h>

#include <sk/gamenet/gamenet.h>
#include <sk/objects/gameobj.h>
#include <sk/modules/skate/skate.h>

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

CBaseComponent* CSpecialItemComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSpecialItemComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// All components set their type, which is a unique 32-bit number
// (the CRC of their name), which is used to identify the component	
CSpecialItemComponent::CSpecialItemComponent() : CBaseComponent()
{
	SetType( CRC_SPECIALITEM );
	
	for ( int i = 0; i < vMAX_SPECIAL_ITEMS; i++ )
	{
		mp_special_items[i] = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSpecialItemComponent::~CSpecialItemComponent()
{	
	// Cleanup the special items
	DestroyAllSpecialItems();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// InitFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CSpecialItemComponent::InitFromStructure( Script::CStruct* pParams )
{
	// ** Add code to parse the structure, and initialize the component

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// RefreshFromStructure is passed a Script::CStruct that contains a
// number of parameters to initialize this component
// this currently is the contents of a node
// but you can pass in anything you like.	
void CSpecialItemComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	// Default to just calline InitFromStructure()
	// but if that does not handle it, then will need to write a specific 
	// function here. 
	// The user might only want to update a single field in the structure
	// and we don't want to be asserting becasue everything is missing 
	
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// The component's Update() function is called from the CCompositeObject's 
// Update() function.  That is called every game frame by the CCompositeObjectManager
// from the s_logic_code function that the CCompositeObjectManager registers
// with the task manger.
void CSpecialItemComponent::Update()
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
CBaseComponent::EMemberFunctionResult CSpecialItemComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		// @script | CreateSpecialItem | attaches a new special item to the skater in the specified slot
		// @parm int | index | Index of special item to create
		// @parm structure | params | Special item creation data (should look like an entry in the node array)
		// @parmopt name | bone | | The bone to which the special item is attached; otherwise, it will be attached to the root
		// @parmopt vector | offset | (0, 0, 0) | An additional offset to the specified bone (or root) applied to the special item's position
		case 0x827c6bd1:  // CreateSpecialItem
			{
				int index;
				uint32 bone = 0;
				pParams->GetInteger( CRCD(0x7f8c98fe,"index"), &index, true );
				pParams->GetChecksum( "bone", &bone );

				uint32 checksum;
				pParams->GetChecksum( "params", &checksum, Script::ASSERT );
				Script::CStruct* pSpecialItemParams;
				pSpecialItemParams = Script::GetStructure( checksum, Script::ASSERT );
								
				CCompositeObject* pMovingObject = CreateSpecialItem( index, pSpecialItemParams );
				
				// special items need a lock object to work...
				Script::CStruct* pLockParams = new Script::CStruct;
				pLockParams->AppendStructure( pParams ); 			// pass along any bone or offset parameters...
				pLockParams->AddChecksum( "id", GetObject()->GetID() );				  	
				Obj::CLockObjComponent* pLockObjComponent = GetLockObjComponentFromObject( pMovingObject );
				Dbg_MsgAssert( pLockObjComponent, ( "No lock obj component" ) );
				pLockObjComponent->InitFromStructure( pLockParams );				 		
				delete pLockParams;

				GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
				if( gamenet_man->InNetGame())
				{
					Net::Client* client;
					GameNet::MsgSpecialItem msg;
					Net::MsgDesc msg_desc;
	
                    client = gamenet_man->GetClient( 0 );
					Dbg_Assert( client );
	
					msg.m_ObjId = GetObject()->GetID();
					msg.m_Params = checksum;
					msg.m_Index = index;
					msg.m_Bone = bone;
					
					msg_desc.m_Data = &msg;
					msg_desc.m_Length = sizeof( GameNet::MsgSpecialItem );
					msg_desc.m_Id = GameNet::MSG_ID_CREATE_SPECIAL_ITEM;
					client->EnqueueMessageToServer( &msg_desc );
				}
			}
			break;
		
		// @script | DestroySpecialItem | Destroys a special item attached to the skater, if it exists
		// @parm int | index | Index of special item to destroy
		case 0x742188fa:  // DestroySpecialItem
			{
				int index;
				pParams->GetInteger( CRCD(0x7f8c98fe,"index"), &index, true );

				this->DestroySpecialItem( index );
				
				GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
				if( gamenet_man->InNetGame())
				{
					Net::Client* client;
					GameNet::MsgSpecialItem msg;
					Net::MsgDesc msg_desc;
	
                    client = gamenet_man->GetClient( 0 );
					Dbg_Assert( client );
	
					msg.m_ObjId = GetObject()->GetID();
					msg.m_Index = index;
					
					msg_desc.m_Data = &msg;
					msg_desc.m_Length = sizeof( GameNet::MsgSpecialItem );
					msg_desc.m_Id = GameNet::MSG_ID_DESTROY_SPECIAL_ITEM;
					client->EnqueueMessageToServer( &msg_desc );
				}
			}
			break;
		
		// @script | DestroyAllSpecialItems | Destroys all special items currently attached to the skater 
		case 0xa1baa25e:  // DestroyAllSpecialItems
			{
				this->DestroyAllSpecialItems();
			}
			break;
		
		// @script | SpecialItemExists | Returns whether a special item exists
		// @parm int | index | Index of special item to check
		case 0xce887492:  // SpecialItemExists
			{
				int index;
				if ( pParams->GetInteger( CRCD(0x7f8c98fe,"index"), &index, false ) )
				{
					Dbg_MsgAssert( index >= 0 && index < vMAX_SPECIAL_ITEMS, ( "Special item index %d must be between 0 and %d", index, vMAX_SPECIAL_ITEMS ) );
					return ( mp_special_items[index] ) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
				}
				else
				{
					for ( int i = 0; i < vMAX_SPECIAL_ITEMS; i++ )
					{
						if ( mp_special_items[i] )
						{
							return CBaseComponent::MF_TRUE;
						}
					}
					return CBaseComponent::MF_FALSE;
				}
			}
			break;

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

void CSpecialItemComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSpecialItemComponent::GetDebugInfo"));

	// Add any script components to the p_info structure,
	// and they will be displayed in the script debugger (qdebug.exe)
	// you will need to add the names to debugger_names.q, if they are not existing checksums

	/*	Example:
	p_info->AddInteger("m_never_suspend",m_never_suspend);
	p_info->AddFloat("m_suspend_distance",m_suspend_distance);
	*/
	
// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CCompositeObject* CSpecialItemComponent::CreateSpecialItem( int index, Script::CStruct* pNodeData )
{
	Dbg_MsgAssert( index >= 0 && index < vMAX_SPECIAL_ITEMS, ( "Special item index %d must be between 0 and %d", index, vMAX_SPECIAL_ITEMS ) );
	Dbg_MsgAssert( pNodeData, ( "Missing parameter" ) );

	if ( mp_special_items[index] )
	{
		// TODO:  Maybe this should just be a warning?
//		Dbg_MsgAssert( 0, ( "Special item index %d not empty", index ) );
		DestroySpecialItem( index );
	}

//	Dbg_Message( "Adding special item to index %d", index );
	
	// eventually, we'll want to create a generic composite object
	// (maybe by running a script)
	Mem::PushMemProfile("Game Objects");
	Obj::CCompositeObject* pGameObj = Obj::CreateGameObj( Mdl::Skate::Instance()->GetObjectManager(), pNodeData );
	Dbg_Assert( pGameObj );
	Mem::PopMemProfile();

	// GJ:  This is a special-case function that's used for
	// replays...  I'm not bothering to move it over to the
	// CCompositeObject stuff, because we've already made the
	// decision not to make the replay backwards-compatible.
//	pGameObj->FlagAsSpecialItem();
	
	mp_special_items[index] = pGameObj;

	// add the skater's id to the object's tags
	// so that the object knows which skater it's tied to
	Script::CStruct* pTempStructure;
	pTempStructure = new Script::CStruct;
	pTempStructure->AddChecksum( "parentId", GetObject()->GetID() );
	uint32 cleanupScript;
	if ( pNodeData->GetChecksum( CRCD(0x40764820,"CleanupScript"), &cleanupScript ) )
	{
		pTempStructure->AddChecksum( CRCD(0x40764820,"CleanupScript"), cleanupScript );
	}
	mp_special_items[index]->SetTagsFromScript( pTempStructure );
	delete pTempStructure;

	// in case there's any scaling applied to the model,
	// apply that to the special item's model as well...
	Obj::CModelComponent* pModelComponent = GetModelComponentFromObject( GetObject() );
	Nx::CModel* pModel = NULL;
	if ( pModelComponent )
	{
		pModel = pModelComponent->GetModel();
	}
	
	Obj::CModelComponent* pGameObjModelComponent = GetModelComponentFromObject( pGameObj );
	Nx::CModel* pGameObjModel = NULL;
	if ( pGameObjModelComponent )
	{
		pGameObjModel = pGameObjModelComponent->GetModel();
	}

	// (Mick) ... but only if it has a model (might be just collision)
	if ( pModel && pGameObjModel )		
	{
		Mth::Vector scaleVec = pModel->GetScale();
		pGameObjModel->SetScale( scaleVec );		// (Mick) - see check above
	}
	
	return pGameObj;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSpecialItemComponent::DestroySpecialItem( int index )
{
	Dbg_MsgAssert( index >= 0 && index < vMAX_SPECIAL_ITEMS, ( "Special item index %d must be between 0 and %d", index, vMAX_SPECIAL_ITEMS ) );
	
	if ( mp_special_items[index] )
	{
		// run the cleanup script if it exists...
		Script::CStruct* pStruct = new Script::CStruct;
		mp_special_items[index]->CopyTagsToScriptStruct(pStruct);
		uint32 cleanupScript;
		if ( pStruct->GetChecksum( CRCD(0x40764820,"CleanupScript"), &cleanupScript, Script::NO_ASSERT ) )
		{
			// run a cleanup script if it exists
			Script::RunScript( cleanupScript, NULL, mp_special_items[index] );
		}
		delete pStruct;

		// don't actually delete it yet...
		// instead, just mark it as dead, so 
		// that if any objects still refer to
		// it on this frame, it won't crash...
		mp_special_items[index]->MarkAsDead();
		mp_special_items[index] = NULL;
		
		// THPS4:  flushes dead objects so that we can
		// recreate it on the same frame
		// (theoretically, we should have been
		// able to call "wait 1 gameframe",
		// but it turns out that CScript::Update()
		// can be called more than once per frame,
		// if someone uses the MakeSkaterGoto
		// command)
		// THPS5:  doesn't seem to be a problem any
		// more, perhaps due to the subtle changes in
		// order which object/component logic gets
		// run.  having this line in here does cause
		// some issues now, in that it's possible
		// for an object to indirectly delete itself from
		// its update() function, which is bad.  i've
		// taken this line out and will fix any
		// related bugs as they arise.
//		Mdl::Skate::Instance()->GetObjectManager()->FlushDeadObjects();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSpecialItemComponent::DestroyAllSpecialItems()
{
	// This also gets called when we switch levels...
	// (from DeleteResources)
		
	for ( int index = 0; index < vMAX_SPECIAL_ITEMS; index++ )
	{
		DestroySpecialItem( index );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
}
