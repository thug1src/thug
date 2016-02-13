//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       SkaterScoreComponent.cpp
//* OWNER:          Dan
//* CREATION DATE:  3/12/3
//****************************************************************************

#ifndef __COMPONENTS_SKATERSCORECOMPONENT_H__
#define __COMPONENTS_SKATERSCORECOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <sk/objects/skater.h>

#include <gel/object/basecomponent.h>

#define		CRC_SKATERSCORE CRCD(0xd262802f, "SkaterScore")

#define		GetSkaterScoreComponent() ((Obj::CSkaterScoreComponent*)GetComponent(CRC_SKATERSCORE))
#define		GetSkaterScoreComponentFromObject(pObj) ((Obj::CSkaterScoreComponent*)(pObj)->GetComponent(CRC_SKATERSCORE))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CSkaterScoreComponent : public CBaseComponent
{
public:
    CSkaterScoreComponent();
    virtual ~CSkaterScoreComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
	CSkater*						GetSkater() { return static_cast< CSkater* >(GetObject()); }
	Mdl::Score*						GetScore() { return mp_score; }
	
	void							Reset (   );
	
private:
	Mdl::Score*						mp_score;
};

}

#endif
