//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       RibbonComponent.h
//* OWNER:          Dan
//* CREATION DATE:  5/20/3
//****************************************************************************

#ifndef __COMPONENTS_RIBBONCOMPONENT_H__
#define __COMPONENTS_RIBBONCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#define		CRC_RIBBON CRCD(0xee6fc5b, "Ribbon")

#define		GetRibbonComponent() ((Obj::CRibbonComponent*)GetComponent(CRC_RIBBON))
#define		GetRibbonComponentFromObject(pObj) ((Obj::CRibbonComponent*)(pObj)->GetComponent(CRC_RIBBON))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{
	class CSkeletonComponent;

class CRibbonComponent : public CBaseComponent
{
	struct SLink
	{
		Mth::Vector					p;
		Mth::Vector					last_p;
		Mth::Vector					next_p;
	};
	
public:
    CRibbonComponent();
    virtual ~CRibbonComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
	virtual	void 					Finalize();
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();
	
private:
	uint32							m_bone_name;
	int								m_num_links;
	float							m_link_length;
	uint32							m_color;
	
	float							m_last_frame_length;
	
	SLink*							mp_links;
	
	// peer components
	CSkeletonComponent*				mp_skeleton_component;
};

}

#endif
