//****************************************************************************
//* MODULE:         Sk/Objects
//* FILENAME:       gameobj.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  11/02/2000
//****************************************************************************

#include <sk/objects/gameobj.h>

#include <core/math.h>

#include <gel/objman.h>
#include <gel/components/modelcomponent.h>

#include <gel/components/lockobjcomponent.h>  // needs some stupid special setup


#include <gel/object/compositeobjectmanager.h>
#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>

#include <gfx/nx.h>
#include <gfx/nxparticle.h>

#include <sk/modules/skate/skate.h>
#include <sk/scripting/nodearray.h>

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								DBG Defines									**
*****************************************************************************/

namespace Obj
{	

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

#define		OLD_GO_SYSTEM  0

Obj::CCompositeObject* create_game_obj( CGeneralManager* p_obj_man, Script::CStruct* pNodeData, bool is_level_obj )
{


	#if OLD_GO_SYSTEM	
	CMovingObject* pGameObj = new CMovingObject;
	pGameObj->MovingObjectCreateComponents();
	#else

// Create a composite object from this node, but don't finalize it
		Obj::CCompositeObject* pGameObj = Obj::CCompositeObjectManager::Instance()->CreateCompositeObjectFromNode(
			Script::GetArray("gameobj_composite_structure"), pNodeData, false);

//
    CLockObjComponent* p_lockObjComponent = new CLockObjComponent;
	pGameObj->AddComponent(p_lockObjComponent);

	
	#endif
	
	#if OLD_GO_SYSTEM		
	pGameObj->SetType( SKATE_TYPE_GAME_OBJ );
	Dbg_MsgAssert(pGameObj, ("Failed to create gameobj."));
	p_obj_man->RegisterObject(*pGameObj);
	SkateScript::GetPosition( pNodeData, &pGameObj->m_pos );
	pGameObj->MovingObjectInit( pNodeData, p_obj_man );
	#endif

	Script::RunScript( CRCD(0xf3b84da6,"gameobj_add_components"), pNodeData, pGameObj );

	pGameObj->Finalize();

	// need to synchronize rendered model's position to initial world position
	Obj::CModelComponent* pModelComponent = GetModelComponentFromObject( pGameObj );
	if ( pModelComponent )
	{
		pModelComponent->FinalizeModelInitialization();
	}

//	#if !OLD_GO_SYSTEM
	// This used to be in MovingObjectInit	
	// get any script associated with the object, and run it on the object
	uint32 AIScriptChecksum=0;
	if ( pNodeData->GetChecksum(CRCD(0x2ca8a299,"TriggerScript"),&AIScriptChecksum) )
	{
		Script::CScriptStructure *pScriptParams=NULL;
		pNodeData->GetStructure(CRCD(0x7031f10c,"Params"),&pScriptParams);
		pGameObj->SwitchScript( AIScriptChecksum, pScriptParams );
	}
//	#endif

	return pGameObj;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CCompositeObject* CreateGameObj( CGeneralManager* p_obj_man, Script::CStruct* pNodeData )
{   
	return create_game_obj( p_obj_man, pNodeData, false );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CreateLevelObj( CGeneralManager* p_obj_man, Script::CStruct* pNodeData )
{
	if (pNodeData->ContainsFlag("Bouncy"))
	{
		Obj::CCompositeObjectManager::Instance()->CreateCompositeObjectFromNode(Script::GetArray("bouncy_composite_structure"), pNodeData);
	}
	else
	{
		create_game_obj( p_obj_man, pNodeData, true );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CreateParticleObject( CGeneralManager* p_obj_man, Script::CStruct* pNodeData )
{
	if( pNodeData->ContainsComponentNamed( "HasMotion" ))
	{
		Obj::CCompositeObjectManager::Instance()->CreateCompositeObjectFromNode(
					Script::GetArray("moving_particle_composite_structure"), pNodeData);
	}
	else
	{
		Obj::CCompositeObjectManager::Instance()->CreateCompositeObjectFromNode(
					Script::GetArray("particle_composite_structure"), pNodeData);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CreateParticleEmitter( CGeneralManager* p_obj_man, Script::CStruct* pNodeData )
{
	Dbg_MsgAssert( pNodeData, ( "Couldn't find node script." ) );

	// Run the trigger script, and this will create the game object
	// then need to find the game object	
	uint32 AIScriptChecksum=0;
	pNodeData->GetChecksum( "TriggerScript", &AIScriptChecksum, true );
	uint32 NameChecksum=0;
	pNodeData->GetChecksum( "Name", &NameChecksum, true );
	if ( AIScriptChecksum )
	{
		// Make sure we don't have an unnamed particle system already
		const uint32 PARTICLE_UNNAMED = 0x502335a1;   // Particle_UnNamed			
		Dbg_MsgAssert(!Nx::CEngine::sGetParticleTable()->GetItem(PARTICLE_UNNAMED),("Particle_UnNamed should not exist"));		
			
		// Get the parameters for the TriggerScript, and run it
		// this should create a particle system
		Script::CScriptStructure *pScriptParams=NULL;
		pNodeData->GetStructure("Params",&pScriptParams);
		Script::RunScript(AIScriptChecksum,pScriptParams);		// this will create the particle emitter
		
		// Get the newly created particle system
		Nx::CParticle *p_system = Nx::CEngine::sGetParticleTable()->GetItem(PARTICLE_UNNAMED);
		Dbg_MsgAssert(p_system,("Script did not create Particle_UnNamed!"));

		// Remove the entry from the Hash table			
		Nx::CEngine::sGetParticleTable()->FlushItem(PARTICLE_UNNAMED);

		// Add it back with the name as specifed in this node			
		Nx::CEngine::sGetParticleTable()->PutItem(NameChecksum,p_system);
																  
		p_system->set_checkum(NameChecksum);																  					   
		p_system->SetID(NameChecksum);																  					   
		   
		Mth::Vector	angles;
		Mth::Vector dir (0.0f,1.0f,0.f);
		pNodeData->GetVector("Angles",&angles);
		printf("Angles(%f,%f,%f)\n",angles[X],angles[Y],angles[Z]);
		// 3DSMAX_ANGLES Mick: 3/19/03 - Changed all rotation orders to X,Y,Z
		dir.RotateX(angles[X]);
		dir.RotateY(angles[Y]);
		dir.RotateZ(angles[Z]);
		
		p_system->set_emit_target( dir[X], dir[Y], dir[Z] );

		// Register it with the object manager.....
		// This means that the system can be refered to like a regular moving object
		p_obj_man->RegisterObject(*p_system);
			   
		p_system->MovingObjectInit( pNodeData, p_obj_man ); 	// get LOD and suspend info
		
		// Bastard crap code.... Since I set them up to be objects
		// they now run their scripts with the new component system
		// meaning the scripts run twice, dammit...												
		p_system->GetScript()->ClearScript();
		p_system->GetScript()->ClearEventHandlerTable();
	}
}


} // namespace Obj

