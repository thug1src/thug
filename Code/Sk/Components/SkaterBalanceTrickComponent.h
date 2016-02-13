//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterBalanceTrickComponent.h
//* OWNER:          Dan
//* CREATION DATE:  328/3
//****************************************************************************

#ifndef __COMPONENTS_SKATERBALANCETRICKCOMPONENT_H__
#define __COMPONENTS_SKATERBALANCETRICKCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#include <sk/objects/skater.h>
#include <sk/objects/manual.h>

#define		CRC_SKATERBALANCETRICK CRCD(0x589e5fb3, "SkaterBalanceTrick")

#define		GetSkaterBalanceTrickComponent() ((Obj::CSkaterBalanceTrickComponent*)GetComponent(CRC_SKATERBALANCETRICK))
#define		GetSkaterBalanceTrickComponentFromObject(pObj) ((Obj::CSkaterBalanceTrickComponent*)(pObj)->GetComponent(CRC_SKATERBALANCETRICK))

namespace Script
{
    class CScript;
    class CStruct;
}
	
namespace Mdl
{
	class Skate;
}
              
namespace Obj
{
	class CAnimationComponent;

class CSkaterBalanceTrickComponent : public CBaseComponent
{
	friend Mdl::Skate;
	friend class CSkater;
	friend class CManual;
	friend class CSkaterCam;
	friend class CSkaterCameraComponent;
	friend class CSkaterCorePhysicsComponent;
	
public:
    CSkaterBalanceTrickComponent();
    virtual ~CSkaterBalanceTrickComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    virtual void            		Finalize();
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	CSkater*						GetSkater() { return static_cast< CSkater* >(GetObject()); }
	
	uint32							GetBalanceTrickType (   ) { return mBalanceTrickType; }
	bool							DoingBalanceTrick (   ) { return mDoingBalanceTrick; }
	
	void							ClearMaxTimes (   );
	void							UpdateRecord (   );
	void							Reset (   );
	void							ExcludeBalanceButtons ( int& numButtonsToIgnore, uint32 pButtonsToIgnore[] );
	float							GetBalanceStat ( uint32 Checksum );
	void							ClearBalanceParameters (   );
	
private:
	void							stop_balance_trick (   );
	void							set_balance_trick_params ( Script::CStruct *pParams );
	void							set_wobble_params ( Script::CStruct *pParams );
	
private:
	uint32 							mBalanceTrickType;				// Nosemanual,manual,grind or slide (checksum of)
	bool   							mDoingBalanceTrick;				// set in script when we are doing a balance trick, but not actually into it yet
	
	Script::CStruct*				mpBalanceParams;
	
	CManual 						mManual;
	CManual 						mGrind;
	CManual 						mLip;
	CManual 						mSkitch;
	
	// peer components
	CAnimationComponent*			mp_animation_component;
};

}

#endif
