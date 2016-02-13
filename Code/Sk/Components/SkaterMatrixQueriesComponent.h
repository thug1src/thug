//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterMatrixQueriesComponent.h
//* OWNER:          Dan
//* CREATION DATE:  3/12/3
//****************************************************************************

#ifndef __COMPONENTS_SKATERMATRIXQUERIESCOMPONENT_H__
#define __COMPONENTS_SKATERMATRIXQUERIESCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#include <sk/objects/skater.h>

#define		CRC_SKATERMATRIXQUERIES CRCD(0xbf518e27, "SkaterMatrixQueries")

#define		GetSkaterMatrixQueriesComponent() ((Obj::CSkaterMatrixQueriesComponent*)GetComponent(CRC_SKATERMATRIXQUERIES))
#define		GetSkaterMatrixQueriesComponentFromObject(pObj) ((Obj::CSkaterMatrixQueriesComponent*)(pObj)->GetComponent(CRC_SKATERMATRIXQUERIES))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CSkaterMatrixQueriesComponent : public CBaseComponent
{
public:
    CSkaterMatrixQueriesComponent();
    virtual ~CSkaterMatrixQueriesComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    virtual void            		Finalize();
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	CSkater*						GetSkater() { return static_cast< CSkater* >(GetObject()); }
	
	void							ResetLatestMatrix (   ) { m_latest_matrix = GetObject()->GetMatrix(); }
	
private:
	Mth::Matrix						m_latest_matrix;
	float							m_last_slope;
	
	// peer components
	CSkaterCorePhysicsComponent*	mp_core_physics_component;
};

}

#endif
