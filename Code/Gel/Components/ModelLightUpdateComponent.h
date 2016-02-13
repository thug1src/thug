//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       ModelLightUpdateComponent.h
//* OWNER:          Garrett
//* CREATION DATE:  07/10/03
//****************************************************************************

#ifndef __COMPONENTS_MODELLIGHTUPDATECOMPONENT_H__
#define __COMPONENTS_MODELLIGHTUPDATECOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#define		CRC_MODELLIGHTUPDATE CRCD(0xe3fca939,"ModelLightUpdate")
#define		GetModelLightUpdateComponent() ((Obj::CModelLightUpdateComponent*)GetComponent(CRC_MODELLIGHTUPDATE))
#define		GetModelLightUpdateComponentFromObject(pObj) ((Obj::CModelLightUpdateComponent*)(pObj)->GetComponent(CRC_MODELLIGHTUPDATE))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

	class CModelComponent;

class CModelLightUpdateComponent : public CBaseComponent
{
public:
    CModelLightUpdateComponent();
    virtual ~CModelLightUpdateComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
	virtual	void 					Finalize();
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();

protected:
	CModelComponent *				mp_model_component;
};

}

#endif
