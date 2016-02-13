//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       VibrationComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  2/25/3
//****************************************************************************

#include <gel/components/vibrationcomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>

#include <sk/objects/skater.h>
#include <sk/modules/frontend/frontend.h>

namespace Obj
{
	
// Component giving script control through a composite object over the input pad vibrators of the composite object's input handler.
	
// Only composite objects corresponding to local clients should be given this component.
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CBaseComponent* CVibrationComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CVibrationComponent );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CVibrationComponent::CVibrationComponent() : CBaseComponent()
{
	SetType( CRC_VIBRATION );
	
	m_active_state = false;
	
	for (int n = vVB_NUM_ACTUATORS; n--; )
	{
		mp_vibration_timers[n].active = false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CVibrationComponent::~CVibrationComponent()
{
	if (mp_input_device)
	{
		// turn off all vibration
		for (int i = vVB_NUM_ACTUATORS; i--; )
		{
			mp_input_device->ActivateActuator(i, 0);
		}
		
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVibrationComponent::InitFromStructure( Script::CStruct* pParams )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVibrationComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVibrationComponent::Update()
{
	Dbg_Assert(mp_input_device);
	
	if (!m_active_state)
	{
		Suspend(true);
		return;
	}
	
	// check the duration of active actuators
	for (int i = vVB_NUM_ACTUATORS; i--; )
	{
		// actuator inactive or has no associated duration
		if (!mp_vibration_timers[i].active) continue;
		
		// duration not yet up
		if (Tmr::ElapsedTime(mp_vibration_timers[i].start_time) < mp_vibration_timers[i].duration) continue;
		
		// NOTE: ignoring all replay code in VibrationComponent for now
		// Replay::WritePadVibration(i, 0);
		
		// turn off actuator
		mp_input_device->ActivateActuator(i, 0);
		
		mp_vibration_timers[i].active = false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBaseComponent::EMemberFunctionResult CVibrationComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
        // @script | Vibrate | 
        // @parm int | Actuator | actuator num
        // @parmopt float | Percent | 0 | 
        // @flag Off | turn off both actuators
        // @parmopt float | Duration | 0 | time to vibrate
		case CRCC(0xdf8a5f0a, "Vibrate"):
		{
			// NOTE: ignoring pause state in VibrationComponent for now
			// Don't let it be switched on if skater is paused
			if (!pParams->ContainsFlag(CRCD(0xd443a2bc, "Off")) && (Mdl::FrontEnd::Instance()->GamePaused() || GetObject()->IsPaused())) break;
		
			Dbg_MsgAssert(mp_input_device, ("No associated input device"));
			
			if (!m_active_state) break;
			
			int actuator_idx = 0;
			pParams->GetInteger(CRCD(0x114fc131, "actuator"), &actuator_idx);
			Dbg_MsgAssert(actuator_idx >= 0 && actuator_idx < vVB_NUM_ACTUATORS, ("%s\nActuator must be between 0 and %d", pScript->GetScriptInfo(), vVB_NUM_ACTUATORS));

			float percent = 0;
			pParams->GetFloat(CRCD(0x9e497fc6, "percent"), &percent);
			Dbg_MsgAssert(percent >= 0 && percent <= 100, ("\n%s\nPercent must be between 0 and 100", pScript->GetScriptInfo()));
		
			if (pParams->ContainsFlag("off"))
			{
				// This will switch off vibration, including if the game is paused.
				// Ie, unpausing won't restore the old vibration. Hooray!
				mp_input_device->StopAllVibrationIncludingSaved();
				
				for (int i = vVB_NUM_ACTUATORS; i--; )
				{
					mp_vibration_timers[i].active = false;
				}
				
				// NOTE: ignoring all replay code in VibrationComponent for now
				// for (int i = vVB_NUM_ACTUATORS; i--; )
				// {
					// Replay::WritePadVibration(i, 0);
				// }
			}	
			else
			{
				// NOTE: ignoring all replay code in VibrationComponent for now
				// Replay::WritePadVibration(Actuator, (int)Percent);
				mp_input_device->ActivateActuator(actuator_idx, static_cast<int>(percent));
			}	
		
			// record duration so that we can later turn off the actuator; a duration of 0 gives an unbounded duration
			float duration = 0;
			if (pParams->GetFloat(CRCD(0x79a07f3f, "duration"), &duration))
			{
				mp_vibration_timers[actuator_idx].active = true;
				mp_vibration_timers[actuator_idx].duration = static_cast<Tmr::Time>(1000.0f * duration);
				mp_vibration_timers[actuator_idx].start_time = Tmr::GetTime();
			}
			else
			{
				mp_vibration_timers[actuator_idx].active = false;
			}
			
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

void CVibrationComponent::GetDebugInfo ( Script::CStruct *p_info )
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CVibrationComponent::GetDebugInfo"));
	
	p_info->AddInteger("m_active_state", m_active_state);
	Script::CArray* p_timers_array = new Script::CArray;
	p_timers_array->SetArrayType(vVB_NUM_ACTUATORS, ESYMBOLTYPE_STRUCTURE);
	for (int i = vVB_NUM_ACTUATORS; i--; )
	{
		Script::CStruct* p_timer_structure = new Script::CStruct;
		p_timer_structure->AddInteger("active", mp_vibration_timers[i].active);
		p_timer_structure->AddInteger("duration", mp_vibration_timers[i].duration);
		p_timers_array->SetStructure(i, p_timer_structure);
	}
	p_info->AddArray("mp_vibration_timers", p_timers_array);

	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVibrationComponent::StopAllVibration (   )
{
	if (!m_active_state) return;
	
	Dbg_Assert(mp_input_device);
	
	// NOTE: no real expanation for this, so I'm ignoring it until it breaks the code	
	// #ifndef __PLAT_NGC__
	for (int i = vVB_NUM_ACTUATORS; i--; )
	{
		// NOTE: ignoring all replay code in VibrationComponent for now
		// Replay::WritePadVibration(i, 0);
		
		mp_input_device->ActivateActuator(i, 0);
		
		mp_vibration_timers[i].active = false;
	}
	// #endif // __PLAT_NGC__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVibrationComponent::Reset (   )
{
	Dbg_Assert(mp_input_device);
	
	mp_input_device->ResetActuators();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVibrationComponent::SetActive (   )
{
	if (m_active_state) return;
	
	m_active_state = true;
	Suspend(false);
	
	if (!mp_input_device) return;
	mp_input_device->EnableActuators();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVibrationComponent::SetInactive (   )
{
	if (!m_active_state) return;
	
	m_active_state = false;
	Suspend(true);
	
	if (!mp_input_device) return;
	mp_input_device->StopAllVibrationIncludingSaved();
	mp_input_device->DisableActuators();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVibrationComponent::SetActiveState ( bool state )
{
	if (state == m_active_state) return;
	
	if (state)
	{
		SetActive();
	}
	else
	{
		SetInactive();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CVibrationComponent::SetDevice ( SIO::Device* p_input_device )
{
	Dbg_Assert(p_input_device);
	
	mp_input_device = p_input_device;
	
	if (m_active_state)
	{
		mp_input_device->EnableActuators();
	}
	else
	{
		mp_input_device->DisableActuators();
	}
}

}
