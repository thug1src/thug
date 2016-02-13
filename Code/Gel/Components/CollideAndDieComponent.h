//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       VelocityComponent.h
//* OWNER:          SPG
//* CREATION DATE:  07/10/03
//****************************************************************************

#ifndef __COMPONENTS_COLLIDEANDDIECOMPONENT_H__
#define __COMPONENTS_COLLIDEANDDIECOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>
#include <gel/components/suspendcomponent.h>

// Replace this with the CRCD of the component you are adding
#define		CRC_COLLIDEANDDIE CRCD(0x6259b52b,"CollideAndDie")

//  Standard accessor macros for getting the component either from within an object, or 
//  given an object				 
#define		GetCollideAndDieComponent() ((Obj::CCollideAndDieComponent*)GetComponent(CRC_COLLIDE_AND_DIE))
#define		GetCollideAndDieComponentFromObject(pObj) ((Obj::CCollideAndDieComponent*)(pObj)->GetComponent(CRC_COLLIDE_AND_DIE))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CCollideAndDieComponent : public CBaseComponent
{
public:
    CCollideAndDieComponent();
    virtual ~CCollideAndDieComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    virtual void					Hide( bool should_hide );
	virtual void					Finalize();
		
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();

	void							SetCollisionRadius( float radius );

private:
	CSuspendComponent*	mp_suspend_component;

	float				m_radius;	// Radius for spherical collision detection
	float				m_scale;
	Tmr::Time			m_birth_time;
	uint32				m_death_script;
	Tmr::Time			m_death_time;
	bool				m_dying;
	bool				m_first_frame;
	Mth::Vector			m_vel;
	Mth::Vector			m_last_pos;
};

}

#endif
