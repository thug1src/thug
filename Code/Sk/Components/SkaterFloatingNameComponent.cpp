//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterFloatingNameComponent.h
//* OWNER:			Dan
//* CREATION DATE:  3/13/3
//****************************************************************************

#include <sk/components/skaterfloatingnamecomponent.h>
#include <sk/gamenet/gamenet.h>
#include <sk/modules/skate/gamemode.h>

#include <gfx/2d/screenelement2.h>
#include <gfx/2d/screenelemman.h>

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

CBaseComponent* CSkaterFloatingNameComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CSkaterFloatingNameComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkaterFloatingNameComponent::CSkaterFloatingNameComponent() : CBaseComponent()
{
	SetType( CRC_SKATERFLOATINGNAME );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CSkaterFloatingNameComponent::~CSkaterFloatingNameComponent()
{
	Script::CStruct* pParams = pParams = new Script::CStruct;
	pParams->AddChecksum(CRCD(0x40c698af, "id"), m_screen_element_id);
	
	Script::RunScript(CRCD(0x2575b406, "destroy_object_label"), pParams);
	
	delete pParams;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterFloatingNameComponent::InitFromStructure( Script::CStruct* pParams )
{
	Dbg_MsgAssert(GetObject()->GetType() == SKATE_TYPE_SKATER, ("CSkaterFloatingNameComponent added to non-skater composite object"));
	
	switch (GetObject()->GetID())
	{
		case 0:
			m_screen_element_id = CRCD(0xe797a186, "skater_name_0");
			break;
		case 1:
			m_screen_element_id = CRCD(0x90909110, "skater_name_1");
			break;
		case 2:
			m_screen_element_id = CRCD(0x999c0aa, "skater_name_2");
			break;
		case 3:
			m_screen_element_id = CRCD(0x7e9ef03c, "skater_name_3");
			break;
		case 4:
			m_screen_element_id = CRCD(0xe0fa659f, "skater_name_4");
			break;
		case 5:
			m_screen_element_id = CRCD(0x97fd5509, "skater_name_5");
			break;
		case 6:
			m_screen_element_id = CRCD(0xef404b3, "skater_name_6");
			break;
		case 7:
			m_screen_element_id = CRCD(0x79f33425, "skater_name_7");
			break;
		default:
			Dbg_MsgAssert(false, ("CSkaterFloatingNameComponent in CCompositeObject with ID of 8 or greater"));
			return;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterFloatingNameComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterFloatingNameComponent::Update()
{
	if (!GameNet::Manager::Instance()->InNetGame() || !GameNet::Manager::Instance()->ShouldDrawPlayerNames())
	{
		Suspend(true);
		return;
	}
	
	if (!GetSkater()->IsInWorld()) return;
	
	GameNet::PlayerInfo* player = GameNet::Manager::Instance()->GetPlayerByObjectID(GetObject()->GetID());
	
	float offset;
	if (GameNet::Manager::Instance()->GetCurrentlyObservedPlayer() == player)
	{
		offset = FEET(8.0f);
	}
	else
	{
		offset = FEET(10.0f);
	}

	Script::CStruct* pParams = new Script::CStruct;
	pParams->AddChecksum(CRCD(0x40c698af, "id"), m_screen_element_id);

	int color_index;
	if (Mdl::Skate::Instance()->GetGameMode()->IsTeamGame())
	{
		color_index = player->m_Team + 2;
	}
	else
	{
		color_index = GetObject()->GetID() + 2;
	}

	char text[64];
	sprintf(text, "\\c%d%s", color_index, GetSkater()->GetDisplayName());
	pParams->AddString(CRCD(0xc4745838, "text"), text);
	pParams->AddVector(CRCD(0x4b491900, "pos3D"), GetObject()->m_pos[X], GetObject()->m_pos[Y] + offset, GetObject()->m_pos[Z]);

	Front::CScreenElement *p_name_elem = Front::CScreenElementManager::Instance()->GetElement(m_screen_element_id);
	if (p_name_elem)
	{
		p_name_elem->SetProperties(pParams);
	}
	else
	{
		Script::RunScript(CRCD(0x6a060cf0, "create_object_label"), pParams);
	}
	
	delete pParams;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CSkaterFloatingNameComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkaterFloatingNameComponent::GetDebugInfo ( Script::CStruct *p_info )
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CSkaterFloatingNameComponent::GetDebugInfo"));
	
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}
	
}
