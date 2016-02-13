//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       HorseCameraComponent.h
//* OWNER:          ???
//* CREATION DATE:  ??/??/??
//****************************************************************************

#ifndef __COMPONENTS_HORSECAMERACOMPONENT_H__
#define __COMPONENTS_HORSECAMERACOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>
#include <gel/components/horsecomponent.h>
#include <gel/components/inputcomponent.h>

// Just thinking about it - a generic way of accessing the component				 
#define		CRC_HORSECAMERA CRCD(0xf1593757,"HorseCamera")
#define		GetHorseCameraComponent() ((Obj::CHorseCameraComponent*)GetComponent(CRC_HORSECAMERA))
#define		GetHorseCameraComponentFromObject(pObj) ((Obj::CHorseCameraComponent*)(pObj)->GetComponent(CRC_HORSECAMERA))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CHorseCameraComponent : public CBaseComponent
{
public:

	enum EStateType
	{
		STATE_NORMAL,
		STATE_AIMING,
	};
	
									CHorseCameraComponent();
    virtual							~CHorseCameraComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
	virtual void					Finalize();
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	EStateType						GetState( void )	{ return m_state; }
	CHorseComponent*				GetSubject( void )	{ return mp_subject_vehicle_component; }

private:
	void							reset_camera( void );
	void							calculate_attractor_direction( void );
	void							calculate_orientation_matrix( void );
	void							calculate_position( void );
	void							get_controller_input( void );
	void							do_reticle( void );

private:

	EStateType						m_state;

	float							m_frame_length;

	float							m_spin_modulator;
	float							m_tilt_modulator;

	float							m_lookaround_heading;
	float							m_lookaround_heading_delta;
	float							m_lookaround_heading_angular_speed;
	float							m_lookaround_tilt;
	float							m_lookaround_tilt_delta;
	float							m_lookaround_tilt_angular_speed;

	CCompositeObject*				mp_subject;

	CCompositeObject*				mp_best_target;

	// Peer components.
	CInputComponent*				mp_input_component;
	CHorseComponent*				mp_subject_vehicle_component;
	
	Mth::Vector		   				m_direction;
	Mth::Vector		   				m_attractor_direction;
	
	Mth::Vector						m_pos;
	Mth::Matrix						m_orientation_matrix;
	
	float							m_offset_height;
	float							m_offset_distance;
	float							m_offset_tilt;
	
	float							m_alignment_rate;
};

}

#endif
