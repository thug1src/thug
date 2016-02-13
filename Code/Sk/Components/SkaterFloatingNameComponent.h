//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterFloatingNameComponent.h
//* OWNER:			Dan
//* CREATION DATE:  3/13/3
//****************************************************************************

#ifndef __COMPONENTS_SKATERFLOATINGNAMECOMPONENT_H__
#define __COMPONENTS_SKATERFLOATINGNAMECOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#include <sk/objects/skater.h>

#define		CRC_SKATERFLOATINGNAME CRCD(0x125044e2, "SkaterFloatingName")

#define		GetSkaterFloatingNameComponent() ((Obj::CSkaterFloatingNameComponent*)GetComponent(CRC_SKATERFLOATINGNAME))
#define		GetSkaterFloatingNameComponentFromObject(pObj) ((Obj::CSkaterFloatingNameComponent*)(pObj)->GetComponent(CRC_SKATERFLOATINGNAME))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CSkaterFloatingNameComponent : public CBaseComponent
{
public:
    CSkaterFloatingNameComponent();
    virtual ~CSkaterFloatingNameComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	CSkater*						GetSkater() { return static_cast< CSkater* >(GetObject()); }
	
private:
	uint32							m_screen_element_id;
};

}

#endif
