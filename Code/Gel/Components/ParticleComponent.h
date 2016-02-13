//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       ParticleComponent.h
//* OWNER:          SPG
//* CREATION DATE:  03/21/03
//****************************************************************************

#ifndef __COMPONENTS_PARTICLECOMPONENT_H__
#define __COMPONENTS_PARTICLECOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>
#include <gel/components/suspendcomponent.h>
#include <gfx/nxnewparticle.h>

// Replace this with the CRCD of the component you are adding
#define		CRC_PARTICLE CRCD(0xa9db601e,"Particle")

//  Standard accessor macros for getting the component either from within an object, or 
//  given an object				 
#define		GetParticleComponent() ((Obj::CParticleComponent*)GetComponent(CRC_PARTICLE))
#define		GetParticleComponentFromObject(pObj) ((Obj::CParticleComponent*)(pObj)->GetComponent(CRC_PARTICLE))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CParticleComponent : public CBaseComponent
{
public:
    CParticleComponent();
    virtual ~CParticleComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    virtual void					Hide( bool should_hide );
	virtual void					Finalize();
		
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();

private:
	uint32				m_update_script;
	Nx::CNewParticle*	mp_particle;
	CSuspendComponent*	mp_suspend_component;
	Tmr::Time			m_birth_time;
	uint32				m_system_lifetime;
};

}

#endif
