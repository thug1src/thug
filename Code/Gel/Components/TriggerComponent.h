//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       TriggerComponent.h
//* OWNER:          Dan
//* CREATION DATE:  3/3/3
//****************************************************************************

#ifndef __COMPONENTS_TRIGGERCOMPONENT_H__
#define __COMPONENTS_TRIGGERCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#include <sk/engine/feeler.h>
#include <sk/engine/rectfeeler.h>

#define		CRC_TRIGGER CRCD(0xe594f0a2, "Trigger")

#define		GetTriggerComponent() ((Obj::CTriggerComponent*)GetComponent(CRC_TRIGGER))
#define		GetTriggerComponentFromObject(pObj) ((Obj::CTriggerComponent*)(pObj)->GetComponent(CRC_TRIGGER))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	// trigger types for skating/walking
	enum ESkaterTriggerType
	{
		TRIGGER_SKATE_OFF_EDGE = 1,
		TRIGGER_JUMP_OFF,
		TRIGGER_LAND_ON,
		TRIGGER_SKATE_OFF,
		TRIGGER_SKATE_ONTO,
		TRIGGER_BONK,
		TRIGGER_LIP_ON,
		TRIGGER_LIP_OFF,
		TRIGGER_LIP_JUMP,
	};

class CTriggerComponent : public CBaseComponent
{
	// users of CTriggerComponent define their own trigger event types, however 0 is reserved for the event of passing through trigger polys
	enum
	{
		TRIGGER_THROUGH = 0
	};

public:
	typedef int TriggerEventType;
	
public:
    CTriggerComponent();
    virtual ~CTriggerComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	void							CheckFeelerForTrigger ( TriggerEventType type, CLineFeeler& p_feeler );
	void							CheckFeelerForTrigger ( TriggerEventType type, CRectFeeler& feeler );
	void							TripTrigger ( TriggerEventType type, uint32 node_checksum, Script::CArray* p_node_array = NULL, CCompositeObject* p_object = NULL);
	
private:
	static void						s_through_trigger_callback ( CFeeler* p_feeler );
	
private:
	Mth::Vector						m_pos_last_frame;
	TriggerEventType				m_latest_trigger_event_type;
};

}

#endif
