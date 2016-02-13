//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       VelocityComponent.h
//* OWNER:          SPG
//* CREATION DATE:  07/10/03
//****************************************************************************

#ifndef __COMPONENTS_VELOCITYCOMPONENT_H__
#define __COMPONENTS_VELOCITYCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>
#include <gel/components/suspendcomponent.h>

// Replace this with the CRCD of the component you are adding
#define		CRC_VELOCITY CRCD(0x41272956,"Velocity")

//  Standard accessor macros for getting the component either from within an object, or 
//  given an object				 
#define		GetVelocityComponent() ((Obj::CVelocityComponent*)GetComponent(CRC_VELOCITY))
#define		GetVelocityComponentFromObject(pObj) ((Obj::CVelocityComponent*)(pObj)->GetComponent(CRC_VELOCITY))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CVelocityComponent : public CBaseComponent
{
public:
    CVelocityComponent();
    virtual ~CVelocityComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    virtual void					Hide( bool should_hide );
	virtual void					Finalize();
		
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();

private:
	CSuspendComponent*	mp_suspend_component;
};

}

#endif
