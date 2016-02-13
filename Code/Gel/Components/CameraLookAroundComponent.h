//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       CameraLookAroundComponent.h
//* OWNER:          Dan
//* CREATION DATE:  4/14/3
//****************************************************************************

#ifndef __COMPONENTS_CAMERALOOKAROUNDCOMPONENT_H__
#define __COMPONENTS_CAMERALOOKAROUNDCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>
#include <gel/components/inputcomponent.h>

#define		CRC_CAMERALOOKAROUND CRCD(0x81c7122d, "CameraLookAround")

#define		GetCameraLookAroundComponent() ((Obj::CCameraLookAroundComponent*)GetComponent(CRC_CAMERALOOKAROUND))
#define		GetCameraLookAroundComponentFromObject(pObj) ((Obj::CCameraLookAroundComponent*)(pObj)->GetComponent(CRC_CAMERALOOKAROUND))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CInputComponent;

class CCameraLookAroundComponent : public CBaseComponent
{
	friend class CSkaterCameraComponent;
	friend class CWalkCameraComponent;
	friend class CVehicleCameraComponent;
	
public:
    CCameraLookAroundComponent();
    virtual ~CCameraLookAroundComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    virtual void            		Finalize();
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	void							SetLookaroundHeadingExtra( float lookaround_heading_extra );
	void							ClearLookaround( void );
	void							SetLookaround( float heading, float tilt, float time, float zoom );
	
	bool							IsHeadingTargetZero (   );
	bool							IsTiltTargetZero (   );
	bool							IsLookaroundActive ( void );
	
private:
	bool							mLookaroundLock;
	bool							mLookaroundOverride;	// For when the designer is scripting the lookaround values.
	float							mLookaroundZoom;		// Allows designers to adjust the zoom when overrideing the lookaround values.
	float							mLookaroundTilt;
	float							mLookaroundHeading;
	float							mLookaroundHeadingStartingPoint;
	float							mLookaroundTiltTarget;
	float							mLookaroundHeadingTarget;
	float							mLookaroundTiltDelta;
	float							mLookaroundHeadingDelta;
	float							mLookaroundDeltaTimer;

	CInputComponent*				mp_input_component;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CCameraLookAroundComponent::ClearLookaround( void )
{
	mLookaroundOverride				= false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CCameraLookAroundComponent::IsHeadingTargetZero ( void )
{
	return mp_input_component->GetControlPad().m_scaled_rightX == 0.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CCameraLookAroundComponent::IsTiltTargetZero ( void )
{
	return mp_input_component->GetControlPad().m_scaled_rightY == 0.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool CCameraLookAroundComponent::IsLookaroundActive ( void )
{
	return mLookaroundHeading != 0.0f || !IsHeadingTargetZero()
		|| mLookaroundTilt != 0.0f || !IsTiltTargetZero();
}

}

#endif
