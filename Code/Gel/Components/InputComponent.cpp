//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       InputComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/18/3
//****************************************************************************

#include <gel/components/inputcomponent.h>
#include <gel/components/vibrationcomponent.h>
#include <gel/components/trickcomponent.h>

#include <gfx/2d/screenelemman.h>
								
#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>

#include <sk/modules/frontend/frontend.h>
#include <sk/gamenet/gamenet.h>

#include <gel/objtrack.h>

namespace Obj
{
	extern bool DebugSkaterScripts;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CInputComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CInputComponent);	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CInputComponent::CInputComponent() : CBaseComponent()
{
	SetType( CRC_INPUT);
	
	m_input_handler = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CInputComponent::~CInputComponent()
{
	if (m_input_handler)
	{
		if (m_input_handler->InList())
		{
			m_input_handler->Remove();
		}
		delete m_input_handler;
	}
	if (m_input_handler2)
	{
		if (m_input_handler2->InList())
		{
			m_input_handler2->Remove();
		}
		delete m_input_handler2;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CInputComponent::InitFromStructure( Script::CStruct* pParams )
{
	int i;
	if (pParams->GetInteger(CRCD(0x67e6859a, "player"), &i))
	{
		BindToController(Mdl::Skate::Instance()->m_device_server_map[i]);
	}
	else if (pParams->GetInteger(CRCD(0xb30d9965, "controller"), &i))
	{
		BindToController(i);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CInputComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CInputComponent::Update()
{
	// Doing nothing, so tell code to do nothing next time around
	Suspend(true);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CInputComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
		// @script | Input_Debounce | 
		case CRCC(0xbf7d6c46, "Input_Debounce"):
		{
			uint32 ButtonChecksum = 0;
			if (!pParams->GetChecksum(NO_NAME, &ButtonChecksum)) break;

			float time = 1.0f;
			pParams->GetFloat(CRCD(0x906b67ba, "time"), &time);
			DUMPF(time);

			int	clear = 0;
			pParams->GetInteger(CRCD(0x1a4e0ef9, "clear"), &clear);

			Debounce(ButtonChecksum, time, clear);
			break;
		}
			
		// @script | DisablePlayerInput | 
		case CRCC(0x6e24f5fd, "DisablePlayerInput"):
			DisableInput();
			break;
			
        // @script | EnablePlayerInput | 
		case CRCC(0x56c6c0cd, "EnablePlayerInput"):	  
			EnableInput();
			break;

		// @script | NetDisablePlayerInput | 
		case CRCC(0x6df51774, "NetDisablePlayerInput"):
			NetDisableInput();
			break;

		// @script | NetEnablePlayerInput | 
		case CRCC(0x31c6454f, "NetEnablePlayerInput"):	  
			NetEnableInput();
			break;
				
		// @script | PlayerInputIsDisabled	| return true if the player input is disabled
		case CRCC(0x660746a, "PlayerInputIsDisabled"):
			return IsInputDisabled() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;	

        // @script | LeftPressed | true if left is being pressed on the pad
		case CRCC(0x78238598, "LeftPressed"):
			return m_pad.m_left.GetPressed() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			break;
			
        // @script | RightPressed | true if right is being pressed on the pad
		case CRCC(0x1d604a9d, "RightPressed"):
			return m_pad.m_right.GetPressed() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			break;
			
        // @script | UpPressed | true if up is being pressed on the pad
		case CRCC(0x89aa9b5f, "UpPressed"):
			return m_pad.m_up.GetPressed() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			break;
			
        // @script | DownPressed | true if down is being pressed on the pad
		case CRCC(0x83028fc5, "DownPressed"):
			return m_pad.m_down.GetPressed() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			break;
			
        // @script | HeldLongerThan | true if the specified button has been
        // held longer than the specified amount of time
        // @parm name | Button | button name
        // @uparm 1.0 | time value (default is milliseconds)
        // @flag seconds | time in seconds
        // @flag frames | time in frames
		case CRCC(0x2366469c, "HeldLongerThan"):
		{
			uint32 ButtonChecksum;
			pParams->GetChecksum(CRCD(0xc5f953c2, "Button"), &ButtonChecksum, Script::ASSERT);
				
			float time;
			pParams->GetFloat(NO_NAME, &time, Script::ASSERT);
	
			Tmr::Time TestTime = 0;
			if (pParams->ContainsFlag(CRCD(0xd029f619, "seconds")) || pParams->ContainsFlag(CRCD(0x49e0ee96, "second")))
			{
				TestTime = static_cast< Tmr::Time >(time * 1000.0f);
			}	
			else if (pParams->ContainsFlag(CRCD(0x19176c5, "frames")) || pParams->ContainsFlag(CRCD(0x4a07c332, "frame")))
			{
				TestTime =static_cast< Tmr::Time >(time * (1000.0f / 60.0f));
			}
			else
			{
				TestTime =static_cast< Tmr::Time >(time);	
			}

			CSkaterButton* pButt = m_pad.GetButton(ButtonChecksum);
			if (pButt)
			{
				return (pButt->GetPressed() && pButt->GetPressedTime() > TestTime) ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
			}
			else
			{
				return CBaseComponent::MF_FALSE;
			}	
			break;
		}
			
        // @script | EnableInputEvents |  Enable the sending of TRIGGER_ and RELEASE events from an Input component
		case	CRCC(0x1090a150,"EnableInputEvents"):
		{
			m_input_events_enabled = true;			
			break;
		}
		
        // @script | DisableInputEvents |  Disable the sending of TRIGGER_ and RELEASE events from an Input component
		case	CRCC(0x28729460,"DisableInputEvents"):
		{
			m_input_events_enabled = false;			
			break;
		}
			
		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}
    return CBaseComponent::MF_TRUE;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CInputComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CInputComponent::GetDebugInfo"));
	
	if (m_input_handler && m_input_handler->m_Device)
	{
		p_info->AddInteger("port", m_input_handler->m_Device->GetPort());
		p_info->AddInteger("slot", m_input_handler->m_Device->GetSlot());
		p_info->AddInteger("index", m_input_handler->m_Device->GetIndex());
		p_info->AddInteger("m_input_events_enabled",m_input_events_enabled);
	}

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CInputComponent::BindToController ( int controller )
{
	if (!m_input_handler)
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().SkaterInfoHeap());

		m_input_handler = new Inp::Handler< CInputComponent >(0, CInputComponent::s_input_logic_code, *this, Tsk::BaseTask::Node::vNORMAL_PRIORITY - 1);
		Inp::Manager::Instance()->AddHandler(*m_input_handler);
		m_input_handler2 = new Inp::Handler< CInputComponent >(1, CInputComponent::s_input_logic_code2, *this, Tsk::BaseTask::Node::vNORMAL_PRIORITY - 1);
		Inp::Manager::Instance()->AddHandler(*m_input_handler2);

		Mem::Manager::sHandle().PopContext();
	}

	Inp::Manager::Instance()->ReassignHandler(*m_input_handler, controller);
	Inp::Manager::Instance()->ReassignHandler(*m_input_handler2, 1);
	
	if (CVibrationComponent* p_vibration_component = GetVibrationComponentFromObject(GetObject()))
	{
		p_vibration_component->SetDevice(m_input_handler->m_Device);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CInputComponent::Debounce ( uint32 Checksum, float time, bool clear )
{
	CSkaterButton* button = m_pad.GetButton(Checksum);
	
	float debounce_time = Tmr::GetTime() + (time * 1000.0f);
	
	if (CTrickComponent* p_trick_component = GetTrickComponentFromObject(GetObject()))
	{
		p_trick_component->Debounce(Inp::GetButtonIndex(Checksum), debounce_time);
	}
	
	button->SetDebounce(static_cast< int >(debounce_time));
	
	if (clear)
	{
		button->SetPressed(false);
		button->ClearTrigger();
		button->ClearRelease();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CInputComponent::ignore_button_presses (   )
{
	// if any operable menus are up in net games, ignore input
	if( !GameNet::Manager::Instance()->InNetGame()) return false;
	
	if (Front::CScreenElementManager::Instance()->GetElement(CRCD(0x2edb780f, "controller_unplugged_dialog_anchor"))) return true;
	
	Front::CScreenElement* p_root_window
		= Front::CScreenElementManager::Instance()->GetElement(CRCD(0x56a1eae3, "root_window"));
	
	if (p_root_window)
	{
		Script::CStruct* pTags = new Script::CStruct();
		p_root_window->CopyTagsToScriptStruct(pTags);
		
		uint32 menu_state;
		pTags->GetChecksum(CRCD(0xcdc00e63, "menu_state"), &menu_state);
		delete pTags;
		
		if (menu_state == CRCD(0xf649d637, "on")) return true;
	}

	if (Front::CScreenElementManager::Instance()->GetElement(CRCD(0x31631b98, "keyboard_anchor"))) return true;

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CInputComponent::handle_input ( Inp::Data* input )
{
	if (GetObject()->IsPaused()) return;
	
	build_input_mask(input);

	bool ignore = ignore_button_presses();
	if (IsInputDisabled() || ignore)
	{
		m_pad.Zero();
		if (ignore)
		{
			m_input_mask = 0;
		}
	}
	else
	{
		#ifdef __NOPT_ASSERT__
		m_pad.Update(input, DebugSkaterScripts && GetObject()->GetID() == 0);
		#else
		m_pad.Update(input);
		#endif
	}
}


void CInputComponent::handle_input2 ( Inp::Data* input )
{
	if (GetObject()->IsPaused()) return;
	
//	build_input_mask(input);

	bool ignore = ignore_button_presses();
	if (IsInputDisabled() || ignore)
	{
		m_pad2.Zero();
		if (ignore)
		{
			m_input_mask = 0;
		}
	}
	else
	{
		#ifdef __NOPT_ASSERT__
		m_pad2.Update(input, DebugSkaterScripts && GetObject()->GetID() == 0);
		#else
		m_pad2.Update(input);
		#endif
	}
}


	
void CInputComponent::update_input_mask ( Inp::Data* input, Inp::Data::AnalogButtonIndex button, Inp::Data::AnalogButtonMask mask, uint32 trigger_event, uint32 release_event, CSkaterButton* skater_button )
{
	if (skater_button && Tmr::GetTime() < skater_button->GetDebounceTime()) return;
	
	if (input->m_Event[button])
	{
		if (!(m_last_mask & mask))
		{
				// Fire Event to self, trigger_event
			if (m_input_events_enabled && !m_input_disabled && !m_net_input_disabled)
			{
						
						//uint32	id = GetObject()->GetID();
						//Obj::CTracker::Instance()->LaunchEvent(trigger_event, id, id);
						GetObject()->SelfEvent(trigger_event);
			}
		}
		m_input_mask |= mask;
	}
	else
	{
		if ((m_last_mask & mask))
		{
			if (m_input_events_enabled && !m_input_disabled && !m_net_input_disabled)
			{
						// Fire Event to self, release_event
						//uint32	id = GetObject()->GetID();
						//Obj::CTracker::Instance()->LaunchEvent(release_event, id, id);
						GetObject()->SelfEvent(release_event);						
			}
		}
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CInputComponent::build_input_mask ( Inp::Data* input )
	{
	m_last_mask = m_input_mask;
	m_input_mask = 0;
	update_input_mask(input, Inp::Data::vA_UP,			Inp::Data::mA_UP, 		CRCD(0x5c8faa09,"Trigger_UP"),        CRCD(0xb27804a2,"Release_UP"),		&m_pad.m_up			);	
	update_input_mask(input, Inp::Data::vA_DOWN,		Inp::Data::mA_DOWN, 	CRCD(0xd26adae9,"Trigger_DOWN"),      CRCD(0xaef7dfd3,"Release_DOWN"),      &m_pad.m_down		);	
	update_input_mask(input, Inp::Data::vA_LEFT,		Inp::Data::mA_LEFT, 	CRCD(0xb4f2adba,"Trigger_LEFT"),      CRCD(0xc86fa880,"Release_LEFT"),      &m_pad.m_left		);	
	update_input_mask(input, Inp::Data::vA_RIGHT,		Inp::Data::mA_RIGHT, 	CRCD(0xedbbc2b,"Trigger_RIGHT"),      CRCD(0xc8abf89c,"Release_RIGHT"),     &m_pad.m_right 		);	
	update_input_mask(input, Inp::Data::vA_CIRCLE,		Inp::Data::mA_CIRCLE, 	CRCD(0xb069b600,"Trigger_CIRCLE"),    CRCD(0xe5aae06b,"Release_CIRCLE"),    &m_pad.m_circle		);	
	update_input_mask(input, Inp::Data::vA_SQUARE,		Inp::Data::mA_SQUARE, 	CRCD(0xa93dbbd0,"Trigger_SQUARE"),    CRCD(0xfcfeedbb,"Release_SQUARE"),    &m_pad.m_square		);	
	update_input_mask(input, Inp::Data::vA_TRIANGLE,	Inp::Data::mA_TRIANGLE, CRCD(0x7f695a77,"Trigger_TRIANGLE"),  CRCD(0x7b684919,"Release_TRIANGLE"),  &m_pad.m_triangle	);	
	update_input_mask(input, Inp::Data::vA_X,			Inp::Data::mA_X, 		CRCD(0x42717176,"Trigger_X"),         CRCD(0xbbbef674,"Release_X"),         &m_pad.m_x	 		);	
	update_input_mask(input, Inp::Data::vA_L1,			Inp::Data::mA_L1, 		CRCD(0xc6547217,"Trigger_L1"),        CRCD(0x28a3dcbc,"Release_L1"),        &m_pad.m_L1	 		);	
	update_input_mask(input, Inp::Data::vA_L2,			Inp::Data::mA_L2, 		CRCD(0x5f5d23ad,"Trigger_L2"),        CRCD(0xb1aa8d06,"Release_L2"),        &m_pad.m_L2 		);	
	update_input_mask(input, Inp::Data::vA_L3,			Inp::Data::mA_L3, 		CRCD(0x285a133b,"Trigger_L3"),        CRCD(0xc6adbd90,"Release_L3"),        &m_pad.m_L3	 		);	
	update_input_mask(input, Inp::Data::vA_R1,			Inp::Data::mA_R1, 		CRCD(0x12154dc8,"Trigger_R1"),        CRCD(0xfce2e363,"Release_R1"),        &m_pad.m_R1 		);	
	update_input_mask(input, Inp::Data::vA_R2,			Inp::Data::mA_R2, 		CRCD(0x8b1c1c72,"Trigger_R2"),        CRCD(0x65ebb2d9,"Release_R2"),        &m_pad.m_R2 		);	
	update_input_mask(input, Inp::Data::vA_R3,			Inp::Data::mA_R3, 		CRCD(0xfc1b2ce4,"Trigger_R3"),        CRCD(0x12ec824f,"Release_R3"),        &m_pad.m_R3			);	
	update_input_mask(input, Inp::Data::vA_BLACK,		Inp::Data::mA_BLACK, 	CRCD(0x33947317,"Trigger_BLACK"),     CRCD(0xf5e437a0,"Release_BLACK"),     NULL				);	
	update_input_mask(input, Inp::Data::vA_WHITE,		Inp::Data::mA_WHITE, 	CRCD(0xf8de049b,"Trigger_WHITE"),     CRCD(0x3eae402c,"Release_WHITE"),     NULL		 		);	
	update_input_mask(input, Inp::Data::vA_Z,			Inp::Data::mA_Z,	 	CRCD(0xac7f105a,"Trigger_Z"),         CRCD(0x55b09758,"Release_Z"),     	NULL		 		);	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CInputComponent::s_input_logic_code ( const Inp::Handler < CInputComponent >& handler )
{
	handler.GetData().handle_input(handler.m_Input);
}

void CInputComponent::s_input_logic_code2 ( const Inp::Handler < CInputComponent >& handler )
{
	handler.GetData().handle_input2(handler.m_Input);
}


}
