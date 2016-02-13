//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       FloatingLabelComponent.h
//* OWNER:			Dan
//* CREATION DATE:  3/13/3
//****************************************************************************

#ifndef __COMPONENTS_FLOATINGLABELCOMPONENT_H__
#define __COMPONENTS_FLOATINGLABELCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#define		CRC_FLOATINGLABEL CRCD(0xeada1621, "FloatingLabel")

#define		GetFloatingLabelComponent() ((Obj::CFloatingLabelComponent*)GetComponent(CRC_FLOATINGLABEL))
#define		GetFloatingLabelComponentFromObject(pObj) ((Obj::CFloatingLabelComponent*)(pObj)->GetComponent(CRC_FLOATINGLABEL))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CFloatingLabelComponent : public CBaseComponent
{
public:
    CFloatingLabelComponent();
    virtual ~CFloatingLabelComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	void							SetColor ( int color_index ) { m_color_index = color_index; }
	void							SetString ( char* string ) { strncpy(m_string, string, 64); }
	
private:
	char							m_string[64];
	int								m_color_index;
	float							m_y_offset;
	uint32							m_screen_element_id;
};



}

#endif
