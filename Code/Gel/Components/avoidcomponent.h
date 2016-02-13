//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       AvoidComponent.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  12/02/02
//****************************************************************************

#ifndef __COMPONENTS_AVOIDCOMPONENT_H__
#define __COMPONENTS_AVOIDCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <core/math/slerp.h>

#include <gel/object/basecomponent.h>
#include <gel/components/pedlogiccomponent.h>

// Just thinking about it - a generic way of accessing the component				 
#define		CRC_AVOID CRCD(0x6eb050ed,"Avoid")
#define		GetAvoidComponent() ((CAvoidComponent*)GetComponent(CRC_AVOID))
#define		GetAvoidComponentFromObject(pObj) ((CAvoidComponent*)(pObj)->GetComponent(CRC_AVOID))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CAvoidComponent : public CBaseComponent
{
// share, dammit.
friend class CPedLogicComponent;

public:
    CAvoidComponent();
    virtual ~CAvoidComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
protected:
	void							play_appropriate_jump_anim( float rotAngle, Script::CStruct* pParams );
	bool							jump( Mth::Matrix& mat0, Mth::Matrix& mat1, float rotAng, float jumpDist, float maxHerdDistance );
	float							get_ideal_heading();

protected:
	// Functions/member variables used for getting out of the skater's way:
	float							m_jumpTime;

	Mth::SlerpInterpolator			m_avoidInterpolator;
	float							m_avoidAlpha;
	Mth::Vector						m_avoidOriginalPos;
	Mth::Vector						m_avoidOriginalTargetPos;
	bool							m_avoidOriginalStickToGround;
};

}

#endif
