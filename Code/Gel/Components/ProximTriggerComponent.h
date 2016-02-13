//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       ProximTriggerComponent.h
//* OWNER:          Dan
//* CREATION DATE:  2/28/3
//****************************************************************************

#ifndef __COMPONENTS_PROXIMTRIGGERCOMPONENT_H__
#define __COMPONENTS_PROXIMTRIGGERCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#define		CRC_PROXIMTRIGGER CRCD(0x98952fb6, "ProximTrigger")

#define		GetProximTriggerComponent() ((Obj::CProximTriggerComponent*)GetComponent(CRC_PROXIMTRIGGER))
#define		GetProximTriggerComponentFromObject(pObj) ((Obj::CProximTriggerComponent*)(pObj)->GetComponent(CRC_PROXIMTRIGGER))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CCameraComponent;

class CProximTriggerComponent : public CBaseComponent
{
public:
    CProximTriggerComponent();
    virtual ~CProximTriggerComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	void							SetScriptTarget ( CCompositeObject* p_script_target_object );
	
	void							SetActive (   ) { m_active = true; }
	void							SetInactive (   ) { m_active = false; }
	
	void							SetViewport ( int viewport ) { m_viewport = viewport; }
	int								GetViewport (   ) { return m_viewport; }
	
	void							ProximUpdate();
	
private:
	// target of the scripts run by proxim nodes which do not have a proxim object ID
	CObject*                        mp_script_target_object;
	
	// if the trigger is active
	bool							m_active;
	
	// the viewport which this component corresponds to; all proxim trigger components on a given viewport share a proxim trigger flag so that switching
	// camera will triggered inside/outside effects correctly; only one proxim trigger component with a given viewport should be active at any one time
	int								m_viewport;
	
	// peer components
	CCameraComponent*				mp_camera_component;
};

}

#endif
