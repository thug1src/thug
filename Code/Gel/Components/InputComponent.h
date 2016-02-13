//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       InputComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/18/3
//****************************************************************************

#ifndef __COMPONENTS_INPUTCOMPONENT_H__
#define __COMPONENTS_INPUTCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>
#include <gel/inpman.h>

#include <sk/objects/skaterpad.h>

#define		CRC_INPUT CRCD(0x27d7cd28, "Input")

#define		GetInputComponent() ((Obj::CInputComponent*)GetComponent(CRC_INPUT))
#define		GetInputComponentFromObject(pObj) ((Obj::CInputComponent*)(pObj)->GetComponent(CRC_INPUT))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	
class CInputComponent : public CBaseComponent
{
public:
    CInputComponent();
    virtual ~CInputComponent();

public:
    virtual void            			Update();
    virtual void            			InitFromStructure( Script::CStruct* pParams );
    virtual void            			RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   	CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 						GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*				s_create();
	
	uint32								GetInputMask (   ) const { return m_input_mask; }
	CControlPad&						GetControlPad (   ) { return m_pad; }
	CControlPad&						GetControlPad2 (   ) { return m_pad2; }
	
	void								DisableInput (   ) { m_input_disabled = true; }
	void								EnableInput (   ) { m_input_disabled = false; }
	void								NetDisableInput (   ) { m_net_input_disabled = true; }
	void								NetEnableInput (   ) { m_net_input_disabled = false; }
	bool								IsInputDisabled (   ) { return m_input_disabled || m_net_input_disabled; }
	
	void								BindToController ( int controller );
	Inp::Handler< CInputComponent >*	GetInputHandler (   ) { return m_input_handler; }
	
	void								Debounce ( uint32 Checksum, float time, bool clear );
	
	void								PauseDevice (   ) { m_input_handler->m_Device->Pause(); }
	void								UnPauseDevice (   ) { m_input_handler->m_Device->UnPause(); }
	SIO::Device*						GetDevice (   ) { return m_input_handler->m_Device; }
	
private:
	static void							s_input_logic_code ( const Inp::Handler < CInputComponent >& handler );
	static void							s_input_logic_code2 ( const Inp::Handler < CInputComponent >& handler );
	
	void								handle_input ( Inp::Data* input );
	void								handle_input2 ( Inp::Data* input );
	void								build_input_mask ( Inp::Data* input );
	void 								update_input_mask ( Inp::Data* input, Inp::Data::AnalogButtonIndex button, Inp::Data::AnalogButtonMask mask, uint32 trigger_event, uint32 release_event, CSkaterButton* skater_button  );
	
	bool								ignore_button_presses (   );
	
private:
	uint32								m_last_mask;
	uint32								m_input_mask;
	CControlPad							m_pad;
	CControlPad							m_pad2;
	
	Inp::Handler< CInputComponent >*	m_input_handler;
	Inp::Handler< CInputComponent >*	m_input_handler2;
	
	bool								m_input_disabled;
	bool								m_net_input_disabled;
	
	bool								m_input_events_enabled;
};

}

#endif
