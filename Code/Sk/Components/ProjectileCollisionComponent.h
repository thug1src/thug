//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       ProjectileCollisionComponent.h
//* OWNER:          SPG
//* CREATION DATE:  07/14/03
//****************************************************************************

#ifndef __COMPONENTS_PROJECTILECOLLISIONCOMPONENT_H__
#define __COMPONENTS_PROJECTILECOLLISIONCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>
#include <gel/components/suspendcomponent.h>

// Replace this with the CRCD of the component you are adding
#define		CRC_PROJECTILECOLLISION CRCD(0x7767d6d7,"ProjectileCollision")

//  Standard accessor macros for getting the component either from within an object, or 
//  given an object				 
#define		GetProjectileCollisionComponent() ((Obj::CProjectileCollisionComponent*)GetComponent(CRC_PROJECTILECOLLISION))
#define		GetProjectileCollisionComponentFromObject(pObj) ((Obj::CProjectileCollisionComponent*)(pObj)->GetComponent(CRC_PROJECTILECOLLISION))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CProjectileCollisionComponent : public CBaseComponent
{
public:
    CProjectileCollisionComponent();
    virtual ~CProjectileCollisionComponent();

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
	void							MarkAsDying( void );

private:
	int								get_damage_amount( void );

	CSuspendComponent*	mp_suspend_component;

	float				m_radius;	// Radius for spherical collision detection
	float				m_scale;
	uint32				m_death_script;
	uint32				m_owner_id;
	Mth::Vector			m_vel;
	bool				m_dying;
};

}

#endif
