//****************************************************************************
//* MODULE:         sk/Components
//* FILENAME:       SkaterFinalizePhysicsComponent.h
//* OWNER:          Dan
//* CREATION DATE:  3/26/3
//****************************************************************************

#ifndef __COMPONENTS_SKATERFINALIZEPHYSICSCOMPONENT_H__
#define __COMPONENTS_SKATERFINALIZEPHYSICSCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <sk/objects/skater.h>

#include <gel/object/basecomponent.h>

#define		CRC_SKATERFINALIZEPHYSICS CRCD(0x9b373e58, "SkaterFinalizePhysics")

#define		GetSkaterFinalizePhysicsComponent() ((Obj::CSkaterFinalizePhysicsComponent*)GetComponent(CRC_SKATERFINALIZEPHYSICS))
#define		GetSkaterFinalizePhysicsComponentFromObject(pObj) ((Obj::CSkaterFinalizePhysicsComponent*)(pObj)->GetComponent(CRC_SKATERFINALIZEPHYSICS))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CSkaterStateComponent;

class CSkaterFinalizePhysicsComponent : public CBaseComponent
{
public:
    CSkaterFinalizePhysicsComponent();
    virtual ~CSkaterFinalizePhysicsComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    virtual void            		Finalize();
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	CSkater*						GetSkater() { return static_cast< CSkater* >(GetObject()); }
	
private:
	CSkaterCorePhysicsComponent* 	mp_core_physics_component;
	CSkaterStateComponent* 			mp_state_component;
};

}

#endif
