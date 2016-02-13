//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       VehicleCameraComponent.h
//* OWNER:          ???
//* CREATION DATE:  ??/??/??
//****************************************************************************

#ifndef __COMPONENTS_VEHICLECAMERACOMPONENT_H__
#define __COMPONENTS_VEHICLECAMERACOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>
#include <gel/components/vehiclecomponent.h>

// Just thinking about it - a generic way of accessing the component				 
#define		CRC_VEHICLECAMERA CRCD(0x2c747d8a, "VehicleCamera")
#define		GetVehicleCameraComponent() ((Obj::CVehicleCameraComponent*)GetComponent(CRC_VEHICLECAMERA))
#define		GetVehicleCameraComponentFromObject(pObj) ((Obj::CVehicleCameraComponent*)(pObj)->GetComponent(CRC_VEHICLECAMERA))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CCameraLookAroundComponent;

class CVehicleCameraComponent : public CBaseComponent
{
public:
    CVehicleCameraComponent();
    virtual ~CVehicleCameraComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
	virtual void					Finalize (   );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
private:
	void							reset_camera (   );
	void							calculate_attractor_direction (   );
	void							calculate_dependent_variables (   );
	void							apply_state (   );

private:
	CCompositeObject*				mp_subject;
	CVehicleComponent*				mp_subject_vehicle_component;
	
	Mth::Vector		   				m_direction;
	Mth::Vector		   				m_attractor_direction;
	
	Mth::Vector						m_pos;
	Mth::Matrix						m_orientation_matrix;
	
	float							m_offset_height;
	float							m_offset_distance;
	float							m_angle;
	
	float							m_alignment_rate;
	
	CCameraLookAroundComponent*		mp_camera_lookaround_component;
};

}

#endif
