//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       ShadowComponent.h
//* OWNER:			Gary
//* CREATION DATE:  2/06/03
//****************************************************************************

#ifndef __COMPONENTS_SHADOWCOMPONENT_H__
#define __COMPONENTS_SHADOWCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

// For EShadowType
#include <gfx/shadow.h>

#include <core/math.h>
			
#include <gel/object/basecomponent.h>

// Just thinking about it - a generic way of accessing the component				 
#define		CRC_SHADOW CRCD(0x8a897dd2,"shadow")
#define		GetShadowComponent() ((CShadowComponent*)GetComponent(CRC_SHADOW))
#define		GetShadowComponentFromObject(pObj) ((Obj::CShadowComponent*)(pObj)->GetComponent(CRC_SHADOW))

namespace Gfx
{
	class CShadow;
}

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class	CModelComponent;

class CShadowComponent : public CBaseComponent
{
public:
    CShadowComponent();
    virtual ~CShadowComponent();

public:
    virtual void					Finalize( );
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void					Hide( bool should_hide );
	virtual void					Teleport();

    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	
	static CBaseComponent*			s_create();

protected: 
	void							update_shadow();
		
		
public:		
	void							SwitchOnShadow(Gfx::EShadowType mode);
	void 							SwitchOffShadow();
	void 							HideShadow( bool should_hide );
	void							SetShadowPos( const Mth::Vector& vector );
	void							SetShadowNormal( const Mth::Vector& vector );
	void							SetShadowScale( float scale );
	void							SetShadowDirection( const Mth::Vector& vector );
	
	void							SwitchOnSkaterShadow();

protected:
	Gfx::CShadow *					mp_shadow;
	Mth::Vector						m_shadowPos;
	Mth::Vector						m_shadowNormal;
	uint32							m_shadowType;
	CModelComponent * 				mp_model_component;
};

}

#endif
