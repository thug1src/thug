//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       ObjectHookManagerComponent.h
//* OWNER:          Dave Cowling
//* CREATION DATE:  02/18/03
//****************************************************************************

#ifndef __COMPONENTS_OBJECTHOOKMANAGERCOMPONENT_H__
#define __COMPONENTS_OBJECTHOOKMANAGERCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>
#include <sk/objects/objecthook.h>	

// Just thinking about it - a generic way of accessing the component				 
#define		CRC_OBJECTHOOKMANAGER							CRCD( 0x27443708, "ObjectHookManager" )
#define		GetObjectHookManagerComponent()					((Obj::CObjectHookManagerComponent*)GetComponent( CRC_OBJECTHOOKMANAGER ))
#define		GetObjectHookManagerComponentFromObject( pObj )	((Obj::CObjectHookManagerComponent*)(pObj)->GetComponent( CRC_OBJECTHOOKMANAGER ))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CObjectHookManagerComponent : public CBaseComponent
{
public:
										CObjectHookManagerComponent();
    virtual								~CObjectHookManagerComponent();

public:
    virtual void            			Update();
    virtual void            			InitFromStructure( Script::CStruct* pParams );
    virtual void            			RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult		CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 						GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*				s_create();

	virtual Obj::CObjectHookManager*	GetObjectHookManager( void )	{ return mp_object_hook_manager; }

private:
	Obj::CObjectHookManager*			mp_object_hook_manager;
};

}

#endif
