//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       StaticVehicleComponent.h
//* OWNER:          Dan
//* CREATION DATE:  8/6/3
//****************************************************************************

#ifndef __COMPONENTS_STATICVEHICLECOMPONENT_H__
#define __COMPONENTS_STATICVEHICLECOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#include <gel/components/vehiclecomponent.h>

#define		CRC_STATICVEHICLE CRCD(0x9649d4f9, "StaticVehicle")

#define		GetStaticVehicleComponent() ((Obj::CStaticVehicleComponent*)GetComponent(CRC_STATICVEHICLE))
#define		GetStaticVehicleComponentFromObject(pObj) ((Obj::CStaticVehicleComponent*)(pObj)->GetComponent(CRC_STATICVEHICLE))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CStaticVehicleComponent : public CBaseComponent
{
public:
    CStaticVehicleComponent();
    virtual ~CStaticVehicleComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
	virtual	void 					Finalize();
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
private:
	Mth::Vector						mp_wheels [ CVehicleComponent::vVP_NUM_WHEELS ];
};

}

#endif
