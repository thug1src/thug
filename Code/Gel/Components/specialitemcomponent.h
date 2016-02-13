//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       SpecialItemComponent.h
//* OWNER:          gj
//* CREATION DATE:  02/13/03
//****************************************************************************

#ifndef __COMPONENTS_SPECIALITEMCOMPONENT_H__
#define __COMPONENTS_SPECIALITEMCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

// Just thinking about it - a generic way of accessing the component				 
#define		CRC_SPECIALITEM CRCD(0xf27a294d,"SpecialItem")
#define		GetSpecialItemComponent() ((Obj::CSpecialItemComponent*)GetComponent(CRC_SPECIALITEM))
#define		GetSpecialItemComponentFromObject(pObj) ((Obj::CSpecialItemComponent*)(pObj)->GetComponent(CRC_SPECIALITEM))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CSpecialItemComponent : public CBaseComponent
{
public:
    CSpecialItemComponent();
    virtual ~CSpecialItemComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();

public:
	CCompositeObject*				CreateSpecialItem( int index, Script::CStruct* pNodeData );
	void 							DestroySpecialItem( int index );
	void 							DestroyAllSpecialItems();

protected:
	enum
	{
		vMAX_SPECIAL_ITEMS = 2
	};
	CCompositeObject*				mp_special_items[vMAX_SPECIAL_ITEMS];
};

}

#endif
