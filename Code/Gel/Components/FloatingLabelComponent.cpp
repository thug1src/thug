//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       FloatingLabelComponent.h
//* OWNER:			Dan
//* CREATION DATE:  3/13/3
//****************************************************************************

#include <gel/components/floatinglabelcomponent.h>
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

CBaseComponent* CFloatingLabelComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CFloatingLabelComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CFloatingLabelComponent::CFloatingLabelComponent() : CBaseComponent()
{
	SetType( CRC_FLOATINGLABEL );
	
	strcpy(m_string, "Unset Label");
	m_color_index = 2;
	m_y_offset = 10.0f * 12.0f;
	m_screen_element_id = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CFloatingLabelComponent::~CFloatingLabelComponent()
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

void CFloatingLabelComponent::InitFromStructure( Script::CStruct* pParams )
{
	const char* string;
	if (pParams->GetString(CRCD(0x61414d56, "string"), &string))
	{
		strncpy(m_string, string, 64);
	}
	pParams->GetInteger(CRCD(0x99a9b716, "color"), &m_color_index);
	pParams->GetFloat(CRCD(0x14975800, "y_offset"), &m_y_offset);
	pParams->GetChecksum(CRCD(0x727f9552, "screen_element_id"), &m_screen_element_id);
	
	Dbg_MsgAssert(m_screen_element_id, ("FloatingLabelComponent has bad screen_elemend_id"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CFloatingLabelComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CFloatingLabelComponent::Update()
{
	Script::CStruct* pParams = new Script::CStruct;
	pParams->AddChecksum(CRCD(0x40c698af, "id"), m_screen_element_id);
	
	char text[64];
	sprintf(text, "\\c%d%s", m_color_index, m_string);

	pParams->AddString(CRCD(0xc4745838, "text"), text);
	pParams->AddVector(CRCD(0x4b491900, "pos3D"), GetObject()->m_pos[X], GetObject()->m_pos[Y] + m_y_offset, GetObject()->m_pos[Z]);

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

CBaseComponent::EMemberFunctionResult CFloatingLabelComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
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

void CFloatingLabelComponent::GetDebugInfo ( Script::CStruct *p_info )
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CFloatingLabelComponent::GetDebugInfo"));
	
	p_info->AddString(CRCD(0x2bd21fa8, "m_string"), m_string);
	p_info->AddInteger(CRCD(0x6d83427f, "m_color_index"), m_color_index);
	p_info->AddFloat(CRCD(0x1bcdee78, "m_y_offset"), m_y_offset);
	p_info->AddChecksum(CRCD(0x5f6e2f93, "m_screen_element_id"), m_screen_element_id);
	
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}
	
}
