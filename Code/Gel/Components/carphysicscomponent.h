//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       CarPhysicsComponent.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  11/4/2002
//****************************************************************************

#ifndef __COMPONENTS_CARPHYSICSCOMPONENT_H__
#define __COMPONENTS_CARPHYSICSCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

// Just thinking about it - a generic way of accessing the component				 
#define		CRC_CARPHYSICS CRCD(0xc695136d,"CarPhysics")
#define		GetCarPhysicsComponent() ((Obj::CCarPhysicsComponent*)GetComponent(CRC_CARPHYSICS))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CCarPhysicsComponent : public CBaseComponent
{
public:
    CCarPhysicsComponent();
    virtual ~CCarPhysicsComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	
	static CBaseComponent*			s_create();

public:
	void							init_car_skeleton();
	
public:
	// public only during the transition
	
	// for wheel rotation
	float							m_wheelRotationX;
	float							m_wheelRotationY;
	float							m_targetWheelRotationY;
	float							m_carRotationX;
	float							m_targetCarRotationX;
	float							m_distanceBetweenTires;
	float							m_old_vel_z;

	// for car rocking back and forth
	float							m_minCarRot;
	float							m_maxCarRot;
	float							m_stepCarRot;
	float							m_accelToCarRotFactor;

	bool							m_shadowEnabled;
};

}

#endif
