//****************************************************************************
//* MODULE:         Sk/Objects
//* FILENAME:       ped.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  10/15/2000
//****************************************************************************

/*
	MD:  Derived from CMovingObject, the ped just adds some functionality
	for jumping out of the way of the skater.  That's all!  Other than
	that, the ped inherits all the abilities of the moving object class,
	such as having an animating model, that can follow paths, or play
	sounds, etc...
*/

/*
	GJ:  The above comment used to be true, but now peds are defined as
	having skinned models and animations, running extra scripts to shut
	off extra bones, etc.  Some of this stuff should be moved up to
	the CMovingObject level (in particular, the skinned model/anim stuff)
*/

// start autoduck documentation
// @DOC ped
// @module ped | None
// @subindex Scripting Database
// @index script | ped
    
/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sk/objects/ped.h>

#include <gel/objman.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/symboltable.h>
#include <sk/components/skaterproximitycomponent.h>
#include <gel/components/modelcomponent.h>
#include <gel/components/motioncomponent.h>

#include <sk/modules/skate/skate.h>
#include <sk/objects/movingobject.h>
#include <sk/objects/pathob.h>
#include <sk/scripting/nodearray.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Obj
{

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

const float DEFAULT_PED_TURN_DIST = 2.0f;
const float DEFAULT_PED_MAX_VEL = 2.5f;
const float DEFAULT_PED_ACCELERATION = 4.0f;
const float DEFAULT_PED_DECELERATION = 1.0f;
const float DEFAULT_PED_STOP_DIST = 2.0f;		// stopping distance from wp

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CreatePed(CGeneralManager* p_obj_man, Script::CStruct* pNodeData)
{
	CMovingObject* pPed = new CMovingObject;
	
	pPed->MovingObjectCreateComponents();

	pPed->SetType( SKATE_TYPE_PED );

	Dbg_MsgAssert(pPed, ("Failed to create pedestrian."));
	p_obj_man->RegisterObject(*pPed);
	
	// GJ:  This should come before the model gets initialized
	// get position, from the node that created us:
	SkateScript::GetPosition( pNodeData, &pPed->m_pos );
		
	pPed->MovingObjectInit( pNodeData, p_obj_man );

	Script::RunScript( CRCD(0xf3eec45e,"ped_add_components"), pNodeData, pPed );
	// Finalize the components, after everything has been added	
	pPed->Finalize();
	Script::RunScript( CRCD(0x955a2767,"ped_init_model"), pNodeData, pPed );

	// designer controlled variables:
	// set defaults, to be overridden by script values if they exist:
	Obj::CMotionComponent* pMotionComponent = GetMotionComponentFromObject( pPed );
	
	pMotionComponent->m_max_vel = ( MPH_TO_INCHES_PER_SECOND ( DEFAULT_PED_MAX_VEL ) );
	pMotionComponent->m_acceleration = FEET_TO_INCHES( DEFAULT_PED_ACCELERATION );
	pMotionComponent->m_deceleration = FEET_TO_INCHES( DEFAULT_PED_DECELERATION );
	pMotionComponent->m_stop_dist = FEET_TO_INCHES( DEFAULT_PED_STOP_DIST );
	pMotionComponent->EnsurePathobExists( pPed );
	pMotionComponent->GetPathOb()->m_enter_turn_dist = FEET_TO_INCHES( DEFAULT_PED_TURN_DIST );

	// don't let his pitch get messed up when going up hills...
	pMotionComponent->m_movingobj_flags |= MOVINGOBJ_FLAG_NO_PITCH_PLEASE;

	// need to synchronize rendered model's position to initial world position
	GetModelComponentFromObject( pPed )->FinalizeModelInitialization();

	// now that the skeleton has been updated at least once,
	// we can turn off the extra bones that don't animate:	
	Script::RunScript( CRCD(0x48ca2d26,"ped_disable_bones"), NULL, pPed );
	pPed->SetProfileColor(0xc000c0);				// magenta = ped

	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Obj

