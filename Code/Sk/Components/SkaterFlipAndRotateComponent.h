//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterFlipAndRotateComponent.h
//* OWNER:          Dan
//* CREATION DATE:  3/31/3
//****************************************************************************

#ifndef __COMPONENTS_SKATERFLIPANDROTATECOMPONENT_H__
#define __COMPONENTS_SKATERFLIPANDROTATECOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <sk/objects/skater.h>

#include <gel/object/basecomponent.h>

#define		CRC_SKATERFLIPANDROTATE CRCD(0x488af07a, "SkaterFlipAndRotate")

#define		GetSkaterFlipAndRotateComponent() ((Obj::CSkaterFlipAndRotateComponent*)GetComponent(CRC_SKATERFLIPANDROTATE))
#define		GetSkaterFlipAndRotateComponentFromObject(pObj) ((Obj::CSkaterFlipAndRotateComponent*)(pObj)->GetComponent(CRC_SKATERFLIPANDROTATE))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CAnimationComponent;
	class CSkaterCorePhysicsComponent;
	class CModelComponent;

class CSkaterFlipAndRotateComponent : public CBaseComponent
{
public:
    CSkaterFlipAndRotateComponent();
    virtual ~CSkaterFlipAndRotateComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    virtual void            		Finalize();
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	   	
	CSkater*						GetSkater() { return static_cast< CSkater* >(GetObject()); }

public:
	void							Reset (   );
	
	bool							RotateSkateboard ( uint32 objId, bool rotate, uint32 time, bool propagate );
	bool							IsBoardRotated (   ) { return m_rotate_board; }
	
	void							DoAnyFlipRotateOrBoardRotateAfters (   );
									  	
	void							ToggleFlipState (   );
	void							ApplyFlipState (   );

private:
	bool							m_rotate_board;
	
	bool							mFlipAfter;
	bool							mRotateAfter;
	bool							mBoardRotateAfter;
	
	// peer components
	CAnimationComponent*			mp_animation_component;
	CModelComponent*				mp_model_component;
	CSkaterCorePhysicsComponent*	mp_core_physics_component;
};

}

#endif
