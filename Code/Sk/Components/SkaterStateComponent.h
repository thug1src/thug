//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterStateComponent.h
//* OWNER:          Dan
//* CREATION DATE:  3/31/3
//****************************************************************************

#ifndef __COMPONENTS_SKATERSTATECOMPONENT_H__
#define __COMPONENTS_SKATERSTATECOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>
#include <core/timestampedflag.h>

#include <sk/objects/skaterflags.h>

#include <gel/object/basecomponent.h>

#include <gfx/nxflags.h>

#define		CRC_SKATERSTATE CRCD(0x43686585, "SkaterState")

#define		GetSkaterStateComponent() ((Obj::CSkaterStateComponent*)GetComponent(CRC_SKATERSTATE))
#define		GetSkaterStateComponentFromObject(pObj) ((Obj::CSkaterStateComponent*)(pObj)->GetComponent(CRC_SKATERSTATE))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	enum
	{
		vFIREBALL,
		vNUM_POWERUPS
	};
	
	enum EPhysicsStateType
	{
		NO_STATE = -1,
		SKATING,
		WALKING,
#		ifdef TESTING_GUNSLINGER
		RIDING
#		endif
	};

class CSkaterStateComponent : public CBaseComponent
{
	friend class CSkaterNonLocalNetLogicComponent;
	friend class CSkaterLocalNetLogicComponent;
	friend class CSkaterCorePhysicsComponent;
	friend class CSkaterAdjustPhysicsComponent;
	friend class CSkaterFinalizePhysicsComponent;
	friend class CSkaterFlipAndRotateComponent;
	friend class CSkaterPhysicsControlComponent;
	friend class CWalkComponent;
				  
public:
    CSkaterStateComponent();
    virtual ~CSkaterStateComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
public:
	void							Reset (   );
	
	bool							DoingTrick (   ) { return m_doing_trick; }
	bool							HasPowerup( uint32 type );
	void							ClearPowerups( void );
	void							PickedUpPowerup( uint32 type );
	void							SetDoingTrick ( bool doing_trick ) { m_doing_trick = doing_trick; }
	EStateType						GetState (   ) { return m_state; }
	bool							GetFlag ( ESkaterFlag flag ) { return m_skater_flags[flag].Get(); }
	ETerrainType					GetTerrain (   ) { return m_terrain; }
	Mth::Vector&					GetCameraDisplayNormal (   ) { return m_camera_display_normal; }
	Mth::Vector&					GetCameraCurrentNormal (   ) { return m_camera_current_normal; }
	Mth::Vector&					GetSpineVel (   ) { return m_spine_vel; }
	bool							GetJumpedOutOfLipTrick (   ) { return mJumpedOutOfLipTrick; }
	float							GetHeight (   ) { return m_height; }
	bool							GetDriving (   ) { return m_driving; }
	bool							GetPhysicsState (   ) { return m_physics_state; }
	
private:
	EStateType						m_state;
	
	EPhysicsStateType				m_physics_state;

	CTimestampedFlag				m_skater_flags [ NUM_ESKATERFLAGS ];
	
	bool							m_doing_trick;
	
	ETerrainType					m_terrain;						// current terrain type on which the skater is skating
	
	Mth::Vector						m_spine_vel;					// velocity to move over the spine
	
	bool							mJumpedOutOfLipTrick;			// Gets set when the player jumps out of a lip trick, and reset when they land.
																	// Controlled by the c-code, not script.
																	// Used by the camera.
	
	Mth::Vector						m_camera_display_normal;		// skater's up vector for the purposes of the camera
	Mth::Vector						m_camera_current_normal;		// skater's up vector for the purposes of the camera
	
	bool							m_driving;						// true if the skater is driving a car
	
	float							m_height;
	bool							m_powerups[vNUM_POWERUPS];
};

}

#endif
