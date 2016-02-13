//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       SkaterProximityComponent.h
//* OWNER:          ???
//* CREATION DATE:  ??/??/??
//****************************************************************************

#ifndef __COMPONENTS_SKATERPROXIMITYCOMPONENT_H__
#define __COMPONENTS_SKATERPROXIMITYCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

// Replace this with the CRCD of the component you are adding
#define		CRC_SKATERPROXIMITY CRCD(0x27457739,"SkaterProximity")

//  Standard accessor macros for getting the component either from within an object, or 
//  given an object				 
#define		GetSkaterProximityComponent() ((Obj::CSkaterProximityComponent*)GetComponent(CRC_SkaterProximity))
#define		GetSkaterProximityComponentFromObject(pObj) ((Obj::CSkaterProximityComponent*)(pObj)->GetComponent(CRC_SkaterProximity))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CSkaterProximityComponent : public CBaseComponent
{

// Bitfield flags, so we don't trigger things twice
enum	{
	INNER=0x0001,
	OUTER=0x0002,
	INNERAVOID=0x0004,
	OUTERAVOID=0x0008,
};

public:
    CSkaterProximityComponent();
    virtual ~CSkaterProximityComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	float					GetDistToLocalSkaterSquared();
	float					GetDistToNearestSkaterSquared();

protected:
	float					mInnerRadiusSqr;
	float					mOuterRadiusSqr;

	float					mInnerAvoidRadiusSqr;
	float					mOuterAvoidRadiusSqr;
	
	uint32					m_flags;
	
	
};

}

#endif
