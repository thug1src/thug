//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       ModelLightUpdateComponent.cpp
//* OWNER:          Garrett
//* CREATION DATE:  07/10/03
//****************************************************************************

// The CModelLightUpdateComponent class is used to update the brightness of
// the model lights if nothing else is currently doing it.  Because it has
// to do extra collision checks, this component should only be used if there
// is no other way to get the brightness from an existing feeler.  For now,
// all you need to do is create the component on an object that already has
// a ModelComponent with "UseModelLights" set in the InitStructure.

#include <gel/components/ModelLightUpdateComponent.h>
#include <gel/components/ModelComponent.h>

#include <gfx/nxlight.h>
#include <gfx/nxmodel.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>

#include <sk/engine/feeler.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CModelLightUpdateComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CModelLightUpdateComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CModelLightUpdateComponent::CModelLightUpdateComponent() : CBaseComponent()
{
	SetType( CRC_MODELLIGHTUPDATE );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CModelLightUpdateComponent::~CModelLightUpdateComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModelLightUpdateComponent::InitFromStructure( Script::CStruct* pParams )
{
	// ** Add code to parse the structure, and initialize the component

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModelLightUpdateComponent::Finalize()
{
	mp_model_component =  GetModelComponentFromObject( GetObject() );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModelLightUpdateComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CModelLightUpdateComponent::Update()
{
	// Find the closest ground collision and extract the brightness from it.
	// logic extracted from CAdjustComponent::uber_frig

	CFeeler feeler;
	
	feeler.m_start = GetObject()->m_pos;
	feeler.m_end = GetObject()->m_pos;

	// Very minor adjustment to move origin away from vert walls
	feeler.m_start += GetObject()->m_matrix[Y] * 0.001f;
	
	feeler.m_start[Y] += 8.0f;
	feeler.m_end[Y] -= FEET(400);
		   
	if (!feeler.GetCollision()) return;

	Dbg_Assert(mp_model_component);

	if (feeler.IsBrightnessAvailable())
	{
		Nx::CModelLights *p_model_lights = mp_model_component->GetModel()->GetModelLights();
		if (p_model_lights)
		{
			p_model_lights->SetBrightness(feeler.GetBrightness());
		} 
	}
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CModelLightUpdateComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
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

void CModelLightUpdateComponent::GetDebugInfo(Script::CStruct *p_info)
{
	Dbg_MsgAssert(p_info,("NULL p_info sent to CModelLightUpdateComponent::GetDebugInfo"));

	// Add any script components to the p_info structure,
	// and they will be displayed in the script debugger (qdebug.exe)
	// you will need to add the names to debugger_names.q, if they are not existing checksums

	/*	Example:
	p_info->AddInteger(CRCD(0x7cf2a233,"m_never_suspend"),m_never_suspend);
	p_info->AddFloat(CRCD(0x519ab8e0,"m_suspend_distance"),m_suspend_distance);
	*/
	
// we call the base component's GetDebugInfo, so we can add info from the common base component										 
	CBaseComponent::GetDebugInfo(p_info);	  
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
}
