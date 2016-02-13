//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterStancePanelComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  2/25/3
//****************************************************************************

#include <sk/components/skaterstancepanelcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>

#include <gfx/2d/spriteelement.h>
#include <gfx/2d/screenelemman.h>

namespace Obj
{
	
// Component giving script control through a composite object over the input pad vibrators of the composite object's input handler.
	
// Only composite objects corresponding to local clients should be given this component.
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CSkaterStancePanelComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterStancePanelComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterStancePanelComponent::CSkaterStancePanelComponent() : CBaseComponent()
{
	SetType( CRC_SKATERSTANCEPANEL );
	
	mp_core_physics_component = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterStancePanelComponent::~CSkaterStancePanelComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterStancePanelComponent::InitFromStructure( Script::CStruct* pParams )
{
	m_last_stance = -1;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterStancePanelComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterStancePanelComponent::Finalize (   )
{
	mp_core_physics_component = GetSkaterCorePhysicsComponentFromObject(GetObject());
	
	Dbg_Assert(mp_core_physics_component);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterStancePanelComponent::Update()
{
	Front::CSpriteElement* p_stance_icon = static_cast< Front::CSpriteElement* >(Front::CScreenElementManager::Instance()->GetElement(
		CRCD(0x968ae5dd, "the_boardstance_sprite") + static_cast< CSkater* >(GetObject())->GetHeapIndex()
	).Convert());
	
	Dbg_Assert(p_stance_icon);
	
	int stance = determine_stance();
	
	if (stance == m_last_stance) return;
	m_last_stance = stance;
	
	if (stance == 5)
	{
		p_stance_icon->SetAlpha(0.0f, Front::CScreenElement::FORCE_INSTANT);
	}
	else
	{
		const static uint32 sp_texture_checksums[] =
		{
            CRCD(0x33a15296, "nollie_icon"),
            CRCD(0xbe91f3b6, "fakie_icon"),
			CRCD(0x9f9d3907, "switch_icon"),
            CRCD(0xb793ef40, "sw_pressure_icon"),
            CRCD(0x4e304fa1, "pressure_icon")
		};
		
		p_stance_icon->SetTexture(sp_texture_checksums[stance]);
		p_stance_icon->SetAlpha(1.0f, Front::CScreenElement::FORCE_INSTANT);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterStancePanelComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
        // @script | InNollie | true if in nollie
		case CRCC(0x1eb61dce, "InNollie"):
			return m_in_nollie ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			break;

        // @script | NollieOn | sets nollie to on (check with InNollie)
		case CRCC(0x41eb710b, "NollieOn"):
			if (m_in_nollie == false)
			{
				m_in_nollie = true;
				GetObject()->BroadcastEvent(CRCD(0x8157ab31, "SkaterEnterNollie"));
			}
			break;

        // @script | NollieOff | sets nollie to off (check with InNollie)
		case CRCC(0xfb9b7c9c, "NollieOff"):
			if (m_in_nollie == true)
			{
				m_in_nollie = false;
				GetObject()->BroadcastEvent(CRCD(0x3f70881a, "SkaterExitNollie"));
			}
			break;

        // @script | InPressure | true if in pressure
        case CRCC(0x9fab9d0b,"InPressure"):
			return m_in_pressure ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			break;

        // @script | PressureOn | sets pressure to on (check with InPressure)
		case CRCC(0xa23d710,"PressureOn"):
			if (m_in_pressure == false)
			{
				m_in_pressure = true;
				GetObject()->BroadcastEvent(CRCD(0x2e8de921,"SkaterEnterPressure"));
			}
			break;

        // @script | PressureOff | sets pressure to off (check with InPressure)
		case CRCC(0x71b57dd6,"PressureOff"):
			if (m_in_pressure == true)
			{
				m_in_pressure = false;
				GetObject()->BroadcastEvent(CRCD(0xfa9adb1d,"SkaterExitPressure"));
			}
			break;
			
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
	
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterStancePanelComponent::GetDebugInfo ( Script::CStruct *p_info )
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterStancePanelComponent::GetDebugInfo"));
	
	p_info->AddInteger("stance", determine_stance());

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

}
