//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       VehicleSoundComponent.h
//* OWNER:          Dan
//* CREATION DATE:  7/2/3
//****************************************************************************

#ifndef __COMPONENTS_VEHICLESOUNDCOMPONENT_H__
#define __COMPONENTS_VEHICLESOUNDCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#define		CRC_VEHICLESOUND CRCD(0x28c97f3c, "VehicleSound")

#define		GetVehicleSoundComponent() ((Obj::CVehicleSoundComponent*)GetComponent(CRC_VEHICLESOUND))
#define		GetVehicleSoundComponentFromObject(pObj) ((Obj::CVehicleSoundComponent*)(pObj)->GetComponent(CRC_VEHICLESOUND))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CVehicleComponent;
	class CSoundComponent;

class CVehicleSoundComponent : public CBaseComponent
{
private:
	static const int vVS_MAX_NUM_GEARS = 6;
		
public:
    CVehicleSoundComponent();
    virtual ~CVehicleSoundComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
	virtual	void 					Finalize();
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();

private:
	void							update_collision_sounds (   );
	void							update_tire_sounds (   );
	void							update_engine_sounds (   );
	
	void							fetch_script_parameters (   );
	float							get_param ( uint32 checksum );
	float							get_global_param ( uint32 checksum );
	
	
private:
	Script::CStruct*				m_sound_setup_struct;
	
	bool							m_use_default_sounds;
	
	uint32							m_engine_sound_id;
	uint32							m_tire_sound_id;
	Script::CStruct*				m_collide_sound_struct;
	
	uint32							m_engine_sound_checksum;
	uint32							m_tire_sound_checksum;
	
	int								m_num_gears;
	struct SGear
	{
		float						upshift_point;
		float						downshift_point;
		float						bottom_rpm;
		float						top_rpm;
	}								m_gears[vVS_MAX_NUM_GEARS];
	
	float							m_effective_speed;
	int								m_effective_gear;
	float							m_effective_engine_rpm;
	Tmr::Time						m_gear_shift_time_stamp;
	Tmr::Time						m_airborne_time_stamp;
	
	float							m_effective_tire_slip_vel;
	
	Tmr::Time						m_latest_collision_sound_time_stamp;
	
	// peer components
	CVehicleComponent*				mp_vehicle_component;
	CSoundComponent*				mp_sound_component;
};

}

#endif
