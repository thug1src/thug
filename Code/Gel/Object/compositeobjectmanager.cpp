//****************************************************************************
//* MODULE:         Gel/Object
//* FILENAME:       CompositeObjectManager.cpp
//* OWNER:          Mick West
//* CREATION DATE:  10/17/2002
//****************************************************************************

#include <gel/object/compositeobjectmanager.h>

#include <core/list/node.h>
#include <core/list/search.h>
#include <gel/mainloop.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>

#include <sk/modules/frontend/frontend.h>

// move these to the component factory
#include <gel/components/skeletoncomponent.h>
#include <gel/components/modelcomponent.h>
#include <gel/components/modellightupdatecomponent.h>
#include <gel/components/bouncycomponent.h>
#include <gel/components/particlecomponent.h>
#include <gel/components/soundcomponent.h>
#include <gel/components/animationcomponent.h>
#include <gel/components/collisioncomponent.h>
#include <gel/components/lockobjcomponent.h>
#include <gel/components/carphysicscomponent.h>
#include <gel/components/motioncomponent.h>
#include <gel/components/suspendcomponent.h>
#include <gel/components/trickcomponent.h>
#include <gel/components/avoidcomponent.h>
#include <gel/components/pedlogiccomponent.h>
#include <gel/components/rigidbodycomponent.h>
#include <gel/components/shadowcomponent.h>
#include <gel/components/streamcomponent.h>
#include <gel/components/specialitemcomponent.h>
#include <gel/components/vehiclecomponent.h>
#include <gel/components/vehiclecameracomponent.h>
#include <gel/components/nodearraycomponent.h>
#include <gel/components/railmanagercomponent.h>
#include <gel/components/objecthookmanagercomponent.h>
#include <gel/components/skitchcomponent.h>
//#include <gel/components/nearcomponent.h>
#include <gel/components/cameracomponent.h>
#include <gel/components/skatercameracomponent.h>
#include <gel/components/vibrationcomponent.h>
#include <gel/components/proximtriggercomponent.h>
#include <gel/components/triggercomponent.h>
#include <gel/components/floatinglabelcomponent.h>
#include <gel/components/inputcomponent.h>
#include <gel/components/walkcomponent.h>
#include <gel/components/walkcameracomponent.h>
#include <gel/components/cameralookaroundcomponent.h>
#include <gel/components/movablecontactcomponent.h>
#include <gel/components/vehiclesoundcomponent.h>
#include <gel/components/statsmanagercomponent.h>
#include <gel/components/velocitycomponent.h>
#include <gel/components/collideanddiecomponent.h>
#include <gel/components/setdisplaymatrixcomponent.h>
#include <gel/components/staticvehiclecomponent.h>
#include <sk/components/skaterstancepanelcomponent.h>
#include <sk/components/skaterloopingsoundcomponent.h>
#include <sk/components/skatersoundcomponent.h>
#include <sk/components/skatergapcomponent.h>
#include <sk/components/skaterrotatecomponent.h>
#include <sk/components/skaterphysicscontrolcomponent.h>
#include <sk/components/skaternonlocalnetlogiccomponent.h>
#include <sk/components/skaterlocalnetlogiccomponent.h>
#include <sk/components/skaterscorecomponent.h>
#include <sk/components/skatermatrixqueriescomponent.h>
#include <sk/components/skaterfloatingnamecomponent.h>
#include <sk/components/skaterstatehistorycomponent.h>
#include <sk/components/skatercorephysicscomponent.h>
#include <sk/components/skateradjustphysicscomponent.h>
#include <sk/components/skaterfinalizephysicscomponent.h>
#include <sk/components/skaterproximitycomponent.h>
#include <sk/components/skatercleanupstatecomponent.h>
#include <sk/components/skaterendruncomponent.h>
#include <sk/components/skaterbalancetrickcomponent.h>
#include <sk/components/skaterflipandrotatecomponent.h>
#include <sk/components/skaterstatecomponent.h>
#include <sk/components/goaleditorcomponent.h>
#include <sk/components/raileditorcomponent.h>
#include <sk/components/editorcameracomponent.h>
#include <sk/components/projectilecollisioncomponent.h>
#include <sk/components/skaterruntimercomponent.h>

#ifdef TESTING_GUNSLINGER
#include <gel/components/horsecomponent.h>
#include <gel/components/horsecameracomponent.h>
#include <gel/components/ridercomponent.h>
#include <gel/components/weaponcomponent.h>
#endif



namespace Obj
{

DefineSingletonClass( CCompositeObjectManager, "Composite Manager" );

CBaseComponent*	CCompositeObjectManager::mp_components_by_type[vMAX_COMPONENTS];

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CCompositeObjectManager::CCompositeObjectManager()
{
    mp_logic_task = new Tsk::Task< CCompositeObjectManager > ( CCompositeObjectManager::s_logic_code, *this, Tsk::BaseTask::Node::vLOGIC_TASK_PRIORITY_COMPOSITE_MANAGER );
	
    Mlp::Manager* mlp_manager =  Mlp::Manager::Instance(); 
	mlp_manager->AddLogicTask( *mp_logic_task );

	// Register the default component objects
	// This also might be better moved to a CComponentFactory class
	// but let's see how it works first.
	
	RegisterComponent(CRC_SKATERSTANCEPANEL,	CSkaterStancePanelComponent::s_create);
	RegisterComponent(CRC_SKATERLOOPINGSOUND,	CSkaterLoopingSoundComponent::s_create);
	RegisterComponent(CRC_SKATERSOUND,			CSkaterSoundComponent::s_create);
	RegisterComponent(CRC_SKATERGAP,			CSkaterGapComponent::s_create);
	RegisterComponent(CRC_SKATERROTATE,			CSkaterRotateComponent::s_create);
	RegisterComponent(CRC_SKATERPHYSICSCONTROL,	CSkaterPhysicsControlComponent::s_create);
	RegisterComponent(CRC_SKATERLOCALNETLOGIC,	CSkaterLocalNetLogicComponent::s_create);
	RegisterComponent(CRC_SKATERNONLOCALNETLOGIC,	CSkaterNonLocalNetLogicComponent::s_create);
	RegisterComponent(CRC_SKATERSCORE,			CSkaterScoreComponent::s_create);
	RegisterComponent(CRC_SKATERMATRIXQUERIES,	CSkaterMatrixQueriesComponent::s_create);
	RegisterComponent(CRC_SKATERFLOATINGNAME,	CSkaterFloatingNameComponent::s_create);
	RegisterComponent(CRC_SKATERSTATEHISTORY,	CSkaterStateHistoryComponent::s_create);
	RegisterComponent(CRC_SKATERCOREPHYSICS,	CSkaterCorePhysicsComponent::s_create);
	RegisterComponent(CRC_SKATERADJUSTPHYSICS,	CSkaterAdjustPhysicsComponent::s_create);
	RegisterComponent(CRC_SKATERFINALIZEPHYSICS,	CSkaterFinalizePhysicsComponent::s_create);
	RegisterComponent(CRC_SKATERCLEANUPSTATE,	CSkaterCleanupStateComponent::s_create);
	RegisterComponent(CRC_SKATERENDRUN,			CSkaterEndRunComponent::s_create);
	RegisterComponent(CRC_SKATERBALANCETRICK,	CSkaterBalanceTrickComponent::s_create);
	RegisterComponent(CRC_SKATERFLIPANDROTATE,	CSkaterFlipAndRotateComponent::s_create);
	RegisterComponent(CRC_SKATERSTATE,			CSkaterStateComponent::s_create);
	RegisterComponent(CRC_GOALEDITOR,			CGoalEditorComponent::s_create); 	
	RegisterComponent(CRC_RAILEDITOR,			CRailEditorComponent::s_create); 	
	RegisterComponent(CRC_SKATERPROXIMITY,		CSkaterProximityComponent::s_create);
	RegisterComponent(CRC_EDITORCAMERA,			CEditorCameraComponent::s_create);
	RegisterComponent(CRC_SKATERRUNTIMER,		CSkaterRunTimerComponent::s_create);
	
	RegisterComponent(CRC_MODEL,				CModelComponent::s_create); 
	RegisterComponent(CRC_MODELLIGHTUPDATE,		CModelLightUpdateComponent::s_create); 
	RegisterComponent(CRC_BOUNCY,				CBouncyComponent::s_create); 
	RegisterComponent(CRC_SKELETON,		    	CSkeletonComponent::s_create); 
	RegisterComponent(CRC_ANIMATION,			CAnimationComponent::s_create); 
	RegisterComponent(CRC_SOUND,		    	CSoundComponent::s_create); 
	RegisterComponent(CRC_COLLISION,			CCollisionComponent::s_create); 
	RegisterComponent(CRC_LOCKOBJ,				CLockObjComponent::s_create, CLockObjComponent::s_register); 
	RegisterComponent(CRC_CARPHYSICS,			CCarPhysicsComponent::s_create);
	RegisterComponent(CRC_MOTION,				CMotionComponent::s_create); 
	RegisterComponent(CRC_SUSPEND,				CSuspendComponent::s_create, CSuspendComponent::s_register); 
	RegisterComponent(CRC_TRICK,				CTrickComponent::s_create);
	RegisterComponent(CRC_AVOID,				CAvoidComponent::s_create);
	RegisterComponent(CRC_PEDLOGIC,				CPedLogicComponent::s_create);
	RegisterComponent(CRC_RIGIDBODY,			CRigidBodyComponent::s_create);
	RegisterComponent(CRC_SHADOW,				CShadowComponent::s_create);
	RegisterComponent(CRC_STREAM,				CStreamComponent::s_create);
	RegisterComponent(CRC_SPECIALITEM,			CSpecialItemComponent::s_create);
	RegisterComponent(CRC_VEHICLE,				CVehicleComponent::s_create); 
	RegisterComponent(CRC_VEHICLECAMERA,		CVehicleCameraComponent::s_create); 
	RegisterComponent(CRC_NODEARRAY,			CNodeArrayComponent::s_create); 
	RegisterComponent(CRC_RAILMANAGER,			CRailManagerComponent::s_create); 
	RegisterComponent(CRC_OBJECTHOOKMANAGER,	CObjectHookManagerComponent::s_create); 
	RegisterComponent(CRC_SKITCH,				CSkitchComponent::s_create);
//	RegisterComponent(CRC_NEAR,					CNearComponent::s_create);
	RegisterComponent(CRC_CAMERA,				CCameraComponent::s_create);
	RegisterComponent(CRC_SKATERCAMERA,			CSkaterCameraComponent::s_create);
	RegisterComponent(CRC_VIBRATION,			CVibrationComponent::s_create);
//	RegisterComponent(CRC_PROXIMTRIGGER,		CProximTriggerComponent::s_create);
	RegisterComponent(CRC_TRIGGER,				CTriggerComponent::s_create);
	RegisterComponent(CRC_FLOATINGLABEL,		CFloatingLabelComponent::s_create);
	RegisterComponent(CRC_INPUT,				CInputComponent::s_create);
	RegisterComponent(CRC_PARTICLE,				CParticleComponent::s_create);
	RegisterComponent(CRC_WALK,					CWalkComponent::s_create);
	RegisterComponent(CRC_WALKCAMERA,			CWalkCameraComponent::s_create);
	RegisterComponent(CRC_CAMERALOOKAROUND,		CCameraLookAroundComponent::s_create);
	RegisterComponent(CRC_MOVABLECONTACT,		CMovableContactComponent::s_create);
    RegisterComponent(CRC_STATSMANAGER,			CStatsManagerComponent::s_create); 
    RegisterComponent(CRC_VEHICLESOUND,			CVehicleSoundComponent::s_create); 
	RegisterComponent(CRC_VELOCITY,				CVelocityComponent::s_create);
	RegisterComponent(CRC_COLLIDEANDDIE,		CCollideAndDieComponent::s_create);
	RegisterComponent(CRC_PROJECTILECOLLISION,	CProjectileCollisionComponent::s_create);
	RegisterComponent(CRC_SETDISPLAYMATRIX,		CSetDisplayMatrixComponent::s_create);
	RegisterComponent(CRC_STATICVEHICLE,		CStaticVehicleComponent::s_create);


#	ifdef TESTING_GUNSLINGER
	RegisterComponent(CRC_HORSE,				CHorseComponent::s_create);
	RegisterComponent(CRC_HORSECAMERA,			CHorseCameraComponent::s_create);
	RegisterComponent(CRC_RIDER,				CRiderComponent::s_create);
	RegisterComponent(CRC_WEAPON,				CWeaponComponent::s_create);
#	endif
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CCompositeObjectManager::~CCompositeObjectManager()
{
    if ( mp_logic_task )
    {
        mp_logic_task->Remove();
        delete mp_logic_task;
    }
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CCompositeObject*	CCompositeObjectManager::CreateCompositeObject()
{
	Obj::CCompositeObject *p_composite = new Obj::CCompositeObject;
	
	// printf("+++> Frame: %i\n", Tmr::GetRenderFrame());
	
	// DODGY SETTING OF COMPOSITE TYPE
	// FOR TEMP PATCHING OF SYSTEMS THAT ASSUME EVERYTHING IS A CMovingObject
	p_composite->SetType(11);	// SKATE_TYPE_COMPOSITE;

    RegisterObject( *p_composite );
	return p_composite;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Given a composite object definition array, and a "node" style structure
// with a "name" and "position"
// then create the Composite object at that position, with the id or the name
// and create the componets for it from the array
// NOTE: this is kind of a specific utility function, added
// to ease the transition from old nodearray style contruction of objects 

CCompositeObject* CCompositeObjectManager::CreateCompositeObjectFromNode(Script::CArray *pArray, Script::CStruct *pNodeData, bool finalize)
{
	Obj::CCompositeObject* pObj = CreateCompositeObject();
	pObj->InitFromStructure(pNodeData);	
	pObj->CreateComponentsFromArray(pArray, pNodeData);	
	if (finalize)
	{
		pObj->Finalize();		// Finalize the interfaces between components
	}
	// printf("+++> Creating Composite Object:  %s\n", Script::FindChecksumName(pObj->GetID()));
	return pObj;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Registering components simply adds them to the list of components that
// the compositeobject manager can create
// All that is passed in is a 32 bit id (of the component, e.g. CRC_MODEL)
// and the callback function pointer to the s_create function
	
void	CCompositeObjectManager::RegisterComponent(uint32 id, CBaseComponent *(p_create_function)(), void(p_register_function)()) 
{
	Dbg_MsgAssert(m_num_components < vMAX_COMPONENTS,("Too many components (%d)",vMAX_COMPONENTS));
	m_registered_components[m_num_components].mComponentID = id;
	m_registered_components[m_num_components].mpCreateFunction = p_create_function;
	m_num_components++;

	// I'm letting the component manager control calling the "register" function
	// no good reason, just better encapsulation.  Prevents bugs.
	if (p_register_function)
	{
		p_register_function();
	}
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent*		CCompositeObjectManager::CreateComponent(uint32 id)
{
	for (uint32 i=0;i<m_num_components;i++)
	{
		if (m_registered_components[i].mComponentID == id)
		{
			return m_registered_components[i].mpCreateFunction();	
		}
	}
	Dbg_MsgAssert(0,("Component %s not registered",Script::FindChecksumName(id)));
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCompositeObjectManager::Update()
{
	Lst::Search< CObject > sh;
	
	uint32 stamp_mask = m_stamp_bit_manager.RequestBit();
	
	// clear the appropriate stamp bit
	for (CObject* pObject = sh.FirstItem(m_object_list); pObject; pObject = sh.NextItem())
	{
		pObject->ClearStampBit(stamp_mask);
	}
	
	// traverse the object list
	do
	{
		// clear the appropriate list changed bit
		m_list_changed &= ~stamp_mask;
		
        CObject* pNextObject = sh.FirstItem(m_object_list);
		
		while (pNextObject)
		{
			CObject* pObject = pNextObject;
			pNextObject = sh.NextItem();
			
			if (!pObject->CheckStampBit(stamp_mask))
			{
				pObject->SetStampBit(stamp_mask);
				
				#ifdef __NOPT_ASSERT__
				Tmr::CPUCycles time_before = Tmr::GetTimeInCPUCycles();
				CSmtPtr< CObject > p_smart_object = pObject;
				uint32 obj_id = pObject->GetID(); 
				#endif
				
				static_cast< CCompositeObject* >(pObject)->Update();
				
				#ifdef __NOPT_ASSERT__
				Dbg_MsgAssert(p_smart_object, ("Object %s has deleted itself in its Update() function", Script::FindChecksumName(obj_id)));
				// Convert to microseconds by dividing by 150
				static_cast< CCompositeObject* >(pObject)->SetUpdateTime((Tmr::GetTimeInCPUCycles() - time_before) / 150);
				#endif
				
				if (m_list_changed & stamp_mask) break;
			}
		}
	}
	while (m_list_changed & stamp_mask);
	
	m_stamp_bit_manager.ReturnBit(stamp_mask);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCompositeObjectManager::Pause(bool paused)
{
	Lst::Search< CObject >	sh;
	
	uint32 stamp_mask = m_stamp_bit_manager.RequestBit();
	
	// clear the appropriate stamp bit
	for (CObject* pObject = sh.FirstItem(m_object_list); pObject; pObject = sh.NextItem())
	{
		pObject->ClearStampBit(stamp_mask);
	}
	
	// traverse the object list
	do
	{
		// clear the appropriate list changed bit
		m_list_changed &= ~stamp_mask;
		
        CObject* pNextObject = sh.FirstItem(m_object_list);
		
		while (pNextObject)
		{
			CObject* pObject = pNextObject;
			pNextObject = sh.NextItem();
			
			if (!pObject->CheckStampBit(stamp_mask))
			{
				pObject->SetStampBit(stamp_mask);
				
				static_cast< CCompositeObject* >(pObject)->Pause(paused);
				
				if (m_list_changed & stamp_mask) break;
			}
		}
	}
	while (m_list_changed & stamp_mask);
	
	m_stamp_bit_manager.ReturnBit(stamp_mask);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CCompositeObjectManager::s_logic_code ( const Tsk::Task< CCompositeObjectManager >& task )
{
    Dbg_AssertType ( &task, Tsk::Task< CCompositeObjectManager > );

	CCompositeObjectManager& obj_man = task.GetData();

	obj_man.Update();    
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CBaseComponent* CCompositeObjectManager::GetFirstComponentByType( uint32 id )
{
	for( uint32 i = 0; i < m_num_components; ++i )
	{
		if( m_registered_components[i].mComponentID == id )
		{
			return mp_components_by_type[i];
		}
	}
	Dbg_MsgAssert( 0, ( "Component %s not registered", Script::FindChecksumName( id )));
	return NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CCompositeObjectManager::AddComponentByType( CBaseComponent *p_component )
{
	uint32 id = p_component->GetType();

	for( uint32 i = 0; i < m_num_components; ++i )
	{
		if( m_registered_components[i].mComponentID == id )
		{
			p_component->SetNextSameType( mp_components_by_type[i] );
			mp_components_by_type[i] = p_component;
			return;
		}
	}
	Dbg_MsgAssert( 0, ( "Component %s not registered", Script::FindChecksumName( id )));
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CCompositeObjectManager::RemoveComponentByType( CBaseComponent *p_component )
{
	uint32 id = p_component->GetType();

	for( uint32 i = 0; i < m_num_components; ++i )
	{
		if( m_registered_components[i].mComponentID == id )
		{
			if( mp_components_by_type[i] == p_component )
			{
				mp_components_by_type[i] = p_component->GetNextSameType();
				p_component->SetNextSameType( NULL );
				return;
			}
			else
			{
				CBaseComponent *p_loop = mp_components_by_type[i];
				while( p_loop )
				{
					if( p_loop->GetNextSameType() == p_component )
					{
						p_loop->SetNextSameType( p_component->GetNextSameType());
						p_component->SetNextSameType( NULL );
						return;
					}
					p_loop = p_loop->GetNextSameType();
				}
			}
		}
	}
	Dbg_MsgAssert( 0, ( "Component %s not registered", Script::FindChecksumName( id )));
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

	
}
