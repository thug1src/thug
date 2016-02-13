//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       WeaponComponent.h
//* OWNER:          Dave
//* CREATION DATE:  06/10/03
//****************************************************************************

#ifndef __COMPONENTS_WEAPONCOMPONENT_H__
#define __COMPONENTS_WEAPONCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#define		CRC_WEAPON CRCD( 0x96cc5819, "Weapon" )

#define		GetWeaponComponent()				((Obj::CWeaponComponent*)GetComponent(CRC_WEAPON))
#define		GetWeaponComponentFromObject(pObj)	((Obj::CWeaponComponent*)(pObj)->GetComponent(CRC_WEAPON))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CWeaponComponent : public CBaseComponent
{
public:
									CWeaponComponent();
									virtual ~CWeaponComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
//	virtual	void 					Finalize();
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	CCompositeObject*				GetCurrentTarget( Mth::Vector& start_pos, Mth::Vector* p_reticle_max );
	void							SetCurrentTarget( CCompositeObject* p_obj )	{ mp_current_target = p_obj; }
	void							SetSightPos( Mth::Vector& p )				{ m_sight_pos = p; }
	void							SetSightMatrix( Mth::Matrix& s )			{ m_sight_matrix = s; }
	void							DrawReticle( void );
	void							ProcessStickyTarget( float heading_change, float tilt_change, float *p_extra_heading_change, float *p_extra_tilt_change );
	void							Fire( void );

	float							GetSpinModulator( void )					{ return m_spin_modulator; }
	float							GetTiltModulator( void )					{ return m_tilt_modulator; }

	static CBaseComponent*			s_create();

private:

	Mth::Matrix						m_sight_matrix;
	Mth::Vector						m_sight_pos;
	Mth::Vector						m_reticle_max;			// Position of reticle max point from last collision check.
	CCompositeObject*				mp_current_target;

	float							m_spin_modulator;
	float							m_tilt_modulator;
};

}

#endif
