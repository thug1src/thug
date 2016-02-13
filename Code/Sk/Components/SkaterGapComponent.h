//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterGapComponent.h
//* OWNER:          Dan
//* CREATION DATE:  3/5/3
//****************************************************************************

#ifndef __COMPONENTS_SKATERGAPCOMPONENT_H__
#define __COMPONENTS_SKATERGAPCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#include <sk/objects/skater.h>

#define		CRC_SKATERGAP CRCD(0x11b4c67f, "SkaterGap")

#define		GetSkaterGapComponent() ((Obj::CSkaterGapComponent*)GetComponent(CRC_SKATERGAP))
#define		GetSkaterGapComponentFromObject(pObj) ((Obj::CSkaterGapComponent*)(pObj)->GetComponent(CRC_SKATERGAP))

namespace Script
{
    class CScript;
    class CStruct;
}

namespace Mdl
{
	class Score;
}
              
namespace Obj
{
	class CGap;
	class CGapTrick;
	class CSkaterCorePhysicsComponent;
	class CSkaterBalanceTrickComponent;
	class CSkaterPhysicsControlComponent;
	class CWalkComponent;

class CSkaterGapComponent : public CBaseComponent
{
public:
    CSkaterGapComponent();
    virtual ~CSkaterGapComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    virtual void            		Finalize();
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	void 							UpdateCancelRequire(int cancel, int require);
	
	void							SetAssociatedScore ( Mdl::Score* p_score ) { mp_score = p_score; }
	void							ClearActiveGaps (   );
	void							ClearPendingGaps (   );
	void							AwardPendingGaps (   );
	
	CSkater*						GetSkater() { return static_cast< CSkater* >(GetObject()); }

	static CBaseComponent*			s_create();
	
private:
	void							get_state_flags ( int& cancel, int& require );
	void							start_gap ( Script::CStruct *pParams, Script::CScript* pScript );
	void							start_gap_trick ( Script::CStruct *pParams, Script::CScript* pScript );
	void							end_gap ( Script::CStruct *pParams, Script::CScript* pScript );
	void							check_gap_tricks ( Script::CStruct *pParams );
	void							clear_gap_tricks ( Script::CStruct *pParams );
	
private:
	// list of active gaps
	Lst::Head< CGap >				m_gap_list;
	
	// list of active gapTricks
	Lst::Head< CGapTrick >			m_gaptrick_list;
	
	// count frames to allow timestamping of gap tricks
	uint32							m_frame_count;
	
	// associated score object
	Mdl::Score*						mp_score;
	
	// peer components
	CSkaterCorePhysicsComponent*	mp_core_physics_component;
	CSkaterBalanceTrickComponent*	mp_balance_trick_component;
	CSkaterPhysicsControlComponent*	mp_physics_control_component;
	CWalkComponent*					mp_walk_component;
};

}

#endif
