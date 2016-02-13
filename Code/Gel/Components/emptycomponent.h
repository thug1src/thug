//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       EmptyComponent.h
//* OWNER:          ???
//* CREATION DATE:  ??/??/??
//****************************************************************************

#ifndef __COMPONENTS_EMPTYCOMPONENT_H__
#define __COMPONENTS_EMPTYCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

// Replace this with the CRCD of the component you are adding
#define		CRC_EMPTY CRCD(0x9738c23b,"Empty")

//  Standard accessor macros for getting the component either from within an object, or 
//  given an object				 
#define		GetEmptyComponent() ((Obj::CEmptyComponent*)GetComponent(CRC_EMPTY))
#define		GetEmptyComponentFromObject(pObj) ((Obj::CEmptyComponent*)(pObj)->GetComponent(CRC_EMPTY))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CEmptyComponent : public CBaseComponent
{
public:
    CEmptyComponent();
    virtual ~CEmptyComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
//	virtual	void 					Finalize();
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
};

}

#endif
