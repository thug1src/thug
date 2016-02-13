//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       VibrationComponent.h
//* OWNER:          Dan
//* CREATION DATE:  2/25/3
//****************************************************************************

#ifndef __COMPONENTS_VIBRATIONCOMPONENT_H__
#define __COMPONENTS_VIBRATIONCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <sys/siodev.h>

#include <gel/object/basecomponent.h>

#define		CRC_VIBRATION CRCD(0x404eb936, "Vibration")

#define		GetVibrationComponent() ((Obj::CVibrationComponent*)GetComponent(CRC_VIBRATION))
#define		GetVibrationComponentFromObject(pObj) ((Obj::CVibrationComponent*)(pObj)->GetComponent(CRC_VIBRATION))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CVibrationComponent : public CBaseComponent
{
public:
	static const int vVB_NUM_ACTUATORS = 2;
	
private:	
	// contains the information which defines the state of a vibration actuator
	struct SVibrationTimer
	{
		bool active;
		Tmr::Time start_time;
		Tmr::Time duration;
	};
	
public:
    CVibrationComponent();
    virtual ~CVibrationComponent();

public:
    virtual void            		Update (   );
    virtual void            		InitFromStructure ( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure ( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction ( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo ( Script::CStruct* p_info );

	static CBaseComponent*			s_create (   );
	
	void							StopAllVibration (   );
	void							Reset (   );
	void							SetActive (   );
	void							SetInactive (   );
	void							SetActiveState ( bool state );
	void							SetDevice ( SIO::Device* p_input_device );
	
private:
	bool							m_active_state;
	
	SVibrationTimer					mp_vibration_timers[vVB_NUM_ACTUATORS];
	
	SIO::Device*					mp_input_device;
};

}

#endif
