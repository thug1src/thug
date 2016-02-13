//****************************************************************************
//* MODULE:         Sk/Objects
//* FILENAME:       car.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  10/24/2000
//****************************************************************************

/*
	MD:  The car is just a moving object with a special ground collision 
	function, and the functionality to rotate wheels based on the car's 
	velocity and turn the wheels based on the change in the car's direction. 
*/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sk/objects/car.h>

#include <gel/mainloop.h>
#include <gel/objman.h>				
#include <gel/objsearch.h>				
                         
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>	 
#include <gel/scripting/struct.h>	 
#include <gel/scripting/symboltable.h>

#include <gel/components/carphysicscomponent.h>
#include <gel/components/collisioncomponent.h>
#include <gel/components/modelcomponent.h>
#include <gel/components/motioncomponent.h>
#include <gel/components/skeletoncomponent.h>
#include <gel/components/suspendcomponent.h>
#include <gel/components/nodearraycomponent.h>
#include <gel/components/railmanagercomponent.h>
#include <gel/components/objecthookmanagercomponent.h>
#include <gel/components/skitchcomponent.h>

#include <gfx/debuggfx.h>
#include <gfx/gfxutils.h>
#include <gfx/nx.h>
#include <gfx/nxgeom.h>
#include <gfx/nxhierarchy.h>
#include <gfx/nxmodel.h>
#include <gfx/skeleton.h>

#include <sk/engine/feeler.h>
#include <sk/modules/skate/skate.h>
#include <sk/objects/pathob.h>
#include <sk/objects/skater.h>
#include <sk/scripting/nodearray.h>	 

#include <sys/profiler.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Obj
{

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

#define DEFAULT_CAR_TURN_DIST				20.0f
#define DEFAULT_CAR_MAX_VEL 				30.0f
#define	DEFAULT_CAR_ACCELERATION			18.0f
#define DEFAULT_CAR_HEIGHT_OFFSET  			0.0f
#define DEFAULT_CAR_DECELERATION			32.0f
#define DEFAULT_EMERGENCY_DECELERATION		20.0f
#define DEFAULT_CAR_MIN_STOP_VEL			4.0f		//mph at which the brakes overpower momentum.
#define DEFAULT_CAR_STOP_DIST_SPEED_MULT	1.0f		//stopping distance required depends on speed*this

#define MAX_CAR_SPEED 85
	
#define MAX_WHEEL_Y_ROT ( 70.0f )
#define MAX_CAR_X_ROT ( 5.0f )
#define MIN_CAR_X_ROT ( -5.0f )
#define STEP_CAR_X_ROT ( 0.125f )
#define ACCEL_TO_CAR_ROT_FACTOR ( 3.0f )

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCar::update_wheels()
{
#if 1
	// rotate based on forward speed
	GetCarPhysicsComponent()->m_wheelRotationX += ( ( Tmr::FrameRatio() ) * ( GetMotionComponent()->m_vel_z * 90.0f ) / ( MPH_TO_IPS( MAX_CAR_SPEED ) ) );
#else
	// rotate at constant rate
	GetCarPhysicsComponent()->m_wheelRotationX += ( 2.0f * Tmr::FrameRatio() );
#endif
	
	// make sure it's within bounds
	while ( GetCarPhysicsComponent()->m_wheelRotationX > 360.0f )
	{
		GetCarPhysicsComponent()->m_wheelRotationX -= 360.0f;
	}

	while ( GetCarPhysicsComponent()->m_wheelRotationX < 0.0f )
	{
		GetCarPhysicsComponent()->m_wheelRotationX += 360.0f;
	}

	if ( GetMotionComponent()->GetPathOb() )
	{
		if ( GetMotionComponent()->GetPathOb()->m_y_rot )
		{
			Mth::Vector radiusVec = GetMotionComponent()->GetPathOb()->m_nav_pos - GetMotionComponent()->GetPathOb()->m_circle_center;
			float turningRadius = radiusVec.Length();

#ifdef __PLAT_NGPS__
			float ang = asinf( GetCarPhysicsComponent()->m_distanceBetweenTires / turningRadius );
#else
			float a_ang = GetCarPhysicsComponent()->m_distanceBetweenTires / turningRadius;
			if ( a_ang > 1.0f ) a_ang = 1.0f;
			if ( a_ang < -1.0f ) a_ang = -1.0f;
			float ang = asinf( a_ang );
#endif		// __PLAT_NGPS__
			GetCarPhysicsComponent()->m_targetWheelRotationY = Mth::RadToDeg(-ang);
			if ( GetMotionComponent()->GetPathOb()->m_y_rot < 0.0f )
			{
				GetCarPhysicsComponent()->m_targetWheelRotationY = -GetCarPhysicsComponent()->m_targetWheelRotationY;
			}
		}
		else
		{
			GetCarPhysicsComponent()->m_targetWheelRotationY = 0.0f;
		}
		
	    GetCarPhysicsComponent()->m_wheelRotationY = Mth::FRunFilter( GetCarPhysicsComponent()->m_targetWheelRotationY, GetCarPhysicsComponent()->m_wheelRotationY, Tmr::FrameRatio() );
	}																				  
	else
	{
		GetCarPhysicsComponent()->m_wheelRotationY = 0.0f;
	}

	float deltaZ = GetMotionComponent()->m_vel_z - GetCarPhysicsComponent()->m_old_vel_z;

	GetCarPhysicsComponent()->m_old_vel_z = GetMotionComponent()->m_vel_z;

	if ( Script::GetInt( CRCD(0x4cf13d8f,"car_debug"), Script::NO_ASSERT ) )
	{
		GetCarPhysicsComponent()->m_minCarRot = Script::GetFloat( CRCD(0x877db8da,"accelCarRot"), Script::ASSERT );
		GetCarPhysicsComponent()->m_maxCarRot = Script::GetFloat( CRCD(0xfb3464df,"decelCarRot"), Script::ASSERT );
		GetCarPhysicsComponent()->m_stepCarRot = Script::GetFloat( CRCD(0xfd8bc8d6,"speedCarRot"), Script::ASSERT );
		GetCarPhysicsComponent()->m_accelToCarRotFactor = Script::GetFloat( CRCD(0xb2c7447b,"accelCarRotFactor"), Script::ASSERT );
//		GetCarPhysicsComponent()->m_enter_turn_dist = Script::GetFloat( "turnDist", Script::ASSERT );
	}

	// vACCELERATION_TO_ROTATION_CONSTANT = magic number 
	// that happens to look good, but it can be tweaked 
	// later if necessary (or moved into script on a 
	// per-car basis).
	GetCarPhysicsComponent()->m_targetCarRotationX = -deltaZ / GetCarPhysicsComponent()->m_accelToCarRotFactor;

	if ( GetCarPhysicsComponent()->m_targetCarRotationX > GetCarPhysicsComponent()->m_maxCarRot )
	{
		GetCarPhysicsComponent()->m_targetCarRotationX = GetCarPhysicsComponent()->m_maxCarRot;
	}
	if ( GetCarPhysicsComponent()->m_targetCarRotationX < GetCarPhysicsComponent()->m_minCarRot )
	{
		GetCarPhysicsComponent()->m_targetCarRotationX = GetCarPhysicsComponent()->m_minCarRot;
	}

    GetCarPhysicsComponent()->m_carRotationX = Mth::FRunFilter( GetCarPhysicsComponent()->m_targetCarRotationX, GetCarPhysicsComponent()->m_carRotationX, GetCarPhysicsComponent()->m_stepCarRot * Tmr::FrameRatio() );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CalculateCarHierarchyMatrices( Gfx::CSkeleton* pSkeleton,
                                    Nx::CModel *p_renderedModel, 
									float carRotationX, 
									float wheelRotationX, 
									float wheelRotationY)
{
	Dbg_Assert( p_renderedModel );
	Gfx::CSkeleton* pCarSkeleton = pSkeleton;
	Dbg_Assert( pCarSkeleton );

	// TODO:  Possibly set up the setup matrix only once,
	// and then prerotate the rotation?
	
	Nx::CHierarchyObject* pHierarchyObjects = p_renderedModel->GetHierarchy();
	Mth::Matrix* pMatrices = pCarSkeleton->GetMatrices();

	float carRotationXInRadians = Mth::DegToRad( carRotationX );
	float wheelRotationYInRadians = Mth::DegToRad( wheelRotationY );
	float wheelRotationXInRadians = Mth::DegToRad( wheelRotationX );

	Nx::CHierarchyObject* pCurrentHierarchyObject = pHierarchyObjects;
	for ( int i = 0; i < pCarSkeleton->GetNumBones(); i++ )
	{
		Mth::Matrix* pMatrix = pMatrices + pCurrentHierarchyObject->GetBoneIndex();
		
		// initialize the setup matrix once per frame
		*pMatrix = pCurrentHierarchyObject->GetSetupMatrix();	

		switch ( pCurrentHierarchyObject->GetChecksum() )
		{
			// front wheels
			case 0x6e2f434e:	// car_wheel01
			case 0xf72612f4:	// car_wheel02
				
				// apply steering to the front tires
				pMatrix->RotateZLocal( wheelRotationYInRadians );

				// spin wheels
				pMatrix->RotateXLocal( wheelRotationXInRadians );

				break;

			// back wheels
			case 0x80212262:	// car_wheel03
			case 0x1e45b7c1:	// car_wheel04
				
				// spin wheels
				pMatrix->RotateXLocal( wheelRotationXInRadians );
				break;

			// middle wheels
			case 0x69428757:	// car_wheel05
			case 0xf04bd6ed:	// car_wheel06
				
				// spin wheels
				pMatrix->RotateXLocal( wheelRotationXInRadians );
				break;

			case 0x88c21962:	// car

				// rock the car back and forth
				pMatrix->RotateXLocal( carRotationXInRadians );
				break;

			case 0x92e19120:	// car_shadow
				// do nothing
				break;

			default:
				Dbg_MsgAssert( 0, ( "Unrecognized bone name %s while animating car parts (bone %d)\nCar parts MUST be named be car_wheel01, car_wheel02, car_wheel03, car_wheel04, car_wheel05, car_wheel06, car, or car_shadow", Script::FindChecksumName(pCurrentHierarchyObject->GetChecksum() ), i) );
				break;
		}

		// get children into object space
		if ( i != 0 )
		{
			*pMatrix *= *( pMatrices + pCurrentHierarchyObject->GetParentIndex() );
		}

		pCurrentHierarchyObject++;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCar::display_wheels()
{
	CalculateCarHierarchyMatrices(this->GetSkeleton(),GetModel(),GetCarPhysicsComponent()->m_carRotationX,GetCarPhysicsComponent()->m_wheelRotationX,GetCarPhysicsComponent()->m_wheelRotationY);
	
	// scale down the shadow to nothing...  must be done after all the 
	// children's matrices have been calculated...  this also assumes
	// the shadow is the first matrix...  if it's not, we'd have to
	// search for the correct matrix by name
	if ( !GetCarPhysicsComponent()->m_shadowEnabled )
	{
		Mth::Matrix mat;
		mat.Ident();
		mat.Scale(Mth::Vector(0.0f,0.0f,0.0f,1.0f));
		
		Mth::Matrix* pMatrices = GetSkeleton()->GetMatrices();
		*pMatrices = mat;
	}
}

#if 0
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCar::debug_wheels()
{
	Dbg_Assert( GetModel() );
	Gfx::CSkeleton* pCarSkeleton = this->GetSkeleton();
	Dbg_Assert( pCarSkeleton );

	// draw skeleton
	for ( int i = 0; i < 6; i++ )
	{
		Mth::Matrix objMatrix = m_matrix;
		objMatrix[Mth::POS] = m_pos;

		Mth::Matrix wsMatrix = *(pCarSkeleton->GetMatrices() + i) * objMatrix;

		Mth::Vector pos = wsMatrix[Mth::POS];
		pos[Y] += 48.0f;
		wsMatrix[Mth::POS] = Mth::Vector( 0.0f, 0.0f, 0.0f, 1.0f );
        
		// set up bounding box
		SBBox theBox;
		theBox.m_max.Set(10.0f, 10.0f, 10.0f);
		theBox.m_min.Set(-10.0f, -10.0f, -10.0f);    

		// For now, draw a bounding box
		Gfx::AddDebugBox( wsMatrix, pos, &theBox, NULL, 1, NULL ); 
	}
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCar::InitCar( CGeneralManager* p_obj_man, Script::CStruct* pNodeData )
{
	p_obj_man->RegisterObject(*this);

	MovingObjectCreateComponents();

	uint32 skeletonName = CRCD(0x88c21962,"car");
	bool use_skeletal_cars = false;

	if ( pNodeData->GetChecksum( CRCD(0x09794932,"skeletonName"), &skeletonName, false ) )
	{
		use_skeletal_cars = true;
	}

	if ( use_skeletal_cars )
	{
//		m_model_restoration_info.mSkeletonName=skeletonName;
		// component-based
		Dbg_MsgAssert( GetSkeletonComponent() == NULL, ( "Skeleton component already exists" ) );
		Script::CStruct* pSkeletonStruct = new Script::CStruct;
		pSkeletonStruct->AddChecksum( CRCD(0xb6015ea8,"component"), CRC_SKELETON );
		pSkeletonStruct->AddChecksum( CRCD(0x222756d5,"skeleton"), skeletonName );
		this->CreateComponentFromStructure(pSkeletonStruct, NULL);
		delete pSkeletonStruct;

#ifdef	__NOPT_ASSERT__
		Gfx::CSkeleton* pSkeleton = GetSkeletonComponent()->GetSkeleton();
		Dbg_Assert( pSkeleton );
		Dbg_Assert( pSkeleton->GetNumBones() > 0 );
#endif
	}
	else
	{
//		m_model_restoration_info.mSkeletonName=0;
	}	
	
	MovingObjectInit( pNodeData, p_obj_man );
	
	Obj::CModelComponent* pModelComponent = new Obj::CModelComponent;
	this->AddComponent( pModelComponent );
	pModelComponent->InitFromStructure( pNodeData );
	
	Dbg_Assert( GetModel() );

	// set up the skeleton for this car model, if any
	// needs to come after the geom is loaded (because
	// that's where the hierarchy info is)
	if ( use_skeletal_cars )
	{		
		const char* p_model_name;
		pNodeData->GetText( CRCD(0x286a8d26,"model"), &p_model_name, true );

		// sanity check on number of bones in skeleton
		Dbg_MsgAssert( GetModel()->GetNumObjectsInHierarchy()==GetSkeleton()->GetNumBones(), 
					   ( "Expected to find %d bones in the %s skeleton (found %d in %s - %s)",
						 GetSkeleton()->GetNumBones(),
						 Script::FindChecksumName( skeletonName ),
						 GetModel()->GetNumObjectsInHierarchy(),
						 Script::FindChecksumName( GetID() ),
						 p_model_name ) );
		
		GetCarPhysicsComponent()->init_car_skeleton();		
	}
	else
	{
		if ( GetModel()->GetNumObjectsInHierarchy()!=0 )
		{
			uint32 name = 0;
			pNodeData->GetChecksum( CRCD(0xa1dc81f9,"name"), &name, Script::ASSERT );
			Dbg_MsgAssert( 0, ( "Skeletal model requires a skeleton %s", Script::FindChecksumName(name) ) );
		}
	}

	GetCarPhysicsComponent()->InitFromStructure( pNodeData );

	if ( !pNodeData->ContainsFlag( CRCD(0x0bf29bc0,"NoCollision") ) )
	{
		// GJ:  collision component should be added after the model component,
		// because we want the same m_pos/m_matrix that is used for displaying
		// the model
		Dbg_MsgAssert( GetCollisionComponent() == NULL, ( "Collision component already exists" ) );
		Script::CStruct* pCollisionStruct = new Script::CStruct;
		pCollisionStruct->AddChecksum( CRCD(0xb6015ea8,"component"), CRC_COLLISION );
		pCollisionStruct->AddChecksum( CRCD(0x2d7e583b,"collisionMode"), CRCD(0x6aadf154,"geometry") );
		this->CreateComponentFromStructure(pCollisionStruct, NULL);
		delete pCollisionStruct;
	}

	// designer controlled variables:
	// set defaults, to be overridden by script values if they exist:
	GetMotionComponent()->m_max_vel = ( MPH_TO_INCHES_PER_SECOND ( DEFAULT_CAR_MAX_VEL ) );
	GetMotionComponent()->m_acceleration = FEET_TO_INCHES( DEFAULT_CAR_ACCELERATION );
	GetMotionComponent()->m_deceleration = FEET_TO_INCHES( DEFAULT_CAR_DECELERATION );
	
	// stick to ground tests against wheels, rather than origin
	// (eventually, we'll move this functionality into a custom
	// sticktoground component)
	GetMotionComponent()->m_point_stick_to_ground = false;

	GetMotionComponent()->EnsurePathobExists( this );
	GetMotionComponent()->GetPathOb()->m_enter_turn_dist = FEET_TO_INCHES( DEFAULT_CAR_TURN_DIST );
	
	// Add a NodeArrayComponent to the Car, so it will request loading of the associated node array.
	Obj::CNodeArrayComponent *p_node_array_component = new Obj::CNodeArrayComponent;
	this->AddComponent( p_node_array_component );
	p_node_array_component->InitFromStructure( pNodeData );

	if ( !pNodeData->ContainsFlag( CRCD(0x1fb9e477,"NoRail") ) )
	{
		// Add a RailManagerComponent to the Car, so it can be used for grinding etc.
		Obj::CRailManagerComponent *p_rail_manager_component = new Obj::CRailManagerComponent;
		this->AddComponent( p_rail_manager_component );
		p_rail_manager_component->InitFromStructure( pNodeData );
	}

	if ( !pNodeData->ContainsFlag( CRCD(0x59793e2b,"NoSkitch") ) )
	{
		// Add an ObjectHookManagerComponent to the Car, for use by the SkitchComponent.
		Obj::CObjectHookManagerComponent *p_object_hook_manager_component = new Obj::CObjectHookManagerComponent;
		this->AddComponent( p_object_hook_manager_component );
		p_object_hook_manager_component->InitFromStructure( pNodeData );

		// Add a SkitchComponent to the Car, so it can be used for skitching etc.
		Obj::CSkitchComponent *p_skitch_component = new Obj::CSkitchComponent;
		this->AddComponent( p_skitch_component );
		p_skitch_component->InitFromStructure( pNodeData );
	}

	// Finalize the object, saying we've added all the components
	Finalize();
	
	// need to synchronize rendered model's position to initial world position
	GetModelComponent()->FinalizeModelInitialization();

	SetProfileColor(0xc0c000);				// cyan = car
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CCar::DoGameLogic()
{
	Dbg_MsgLog(("DoGameLogic for car '%s'",Script::FindChecksumName(GetID())));
	
	if ( GetSuspendComponent()->SkipLogic( ) )
	{
		return;
	}
	
	if ( IsDead() )
	{
		return;
	}

	// Don't update the wheels if we should not animate
	// (and this cunningly lets the SuspendComponment actually work, by clearing the initial_animations	
	if (GetSuspendComponent()->should_animate( ))
	{
		// check to see if it's got a hierarchy
		// if it does, then update the wheels
		Gfx::CSkeleton* pCarSkeleton = this->GetSkeleton();
		if ( pCarSkeleton )
		{
			update_wheels();
	
			display_wheels();
			
//			debug_wheels();
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCar::CallMemberFunction( uint32 Checksum, Script::CScriptStructure *pParams, Script::CScript *pScript )
{   
	return CMovingObject::CallMemberFunction( Checksum, pParams, pScript );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCar::CCar ( void )
{
	m_type = SKATE_TYPE_CAR;

	Obj::CCarPhysicsComponent* pCarPhysicsComponent = new Obj::CCarPhysicsComponent;
	AddComponent( pCarPhysicsComponent );
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCar::~CCar ( void )
{   
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CreateCar(CGeneralManager* p_obj_man, Script::CStruct* pNodeData)
{
	CCar* pCar = new CCar;
    Dbg_MsgAssert(pCar, ("Failed to create car."));
	
	// get position, from the node that created us:
	SkateScript::GetPosition( pNodeData, &pCar->m_pos );
    
	pCar->InitCar(p_obj_man, pNodeData);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CreateCar(CGeneralManager* p_obj_man, int nodeNumber)
{
	Script::CStruct* pNodeData = SkateScript::GetNode(nodeNumber);
	CreateCar(p_obj_man, pNodeData);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Obj
