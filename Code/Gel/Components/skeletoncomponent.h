//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       SkeletonComponent.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  10/17/2002
//****************************************************************************

#ifndef __COMPONENTS_SKELETONCOMPONENT_H__
#define __COMPONENTS_SKELETONCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
// Just thinking about it - a generic way of accessing the component				 
#define		CRC_SKELETON CRCD(0x222756d5,"Skeleton")
#define		GetSkeletonComponent() ((Obj::CSkeletonComponent*)GetComponent(CRC_SKELETON))
#define		GetSkeletonComponentFromObject(pObj) ((Obj::CSkeletonComponent*)(pObj)->GetComponent(CRC_SKELETON))

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
namespace Gfx
{
    class CSkeleton;
}

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
class CSkeletonComponent : public CBaseComponent
{
public:
    CSkeletonComponent();
    virtual ~CSkeletonComponent();

public:
    virtual void            Update();
    virtual void            InitFromStructure( Script::CStruct* pParams );
    
    EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	
    static CBaseComponent*	s_create();
    Gfx::CSkeleton*         GetSkeleton();

    bool					GetBonePosition( uint32 boneName, Mth::Vector* pBonePos );
	bool					GetBoneWorldPosition( uint32 boneName, Mth::Vector* pBonePos );
	
	void					SpawnTextureSplat( Mth::Vector& pos, uint32 bone_name_checksum, float size, float radius, float lifetime, float dropdown_length, const char *p_texture_name, bool trail, bool dropdown_vertical );
	void					SpawnCompositeObject ( uint32 bone_name, Script::CArray* pArray, Script::CStruct* pParams, float object_vel_factor, Mth::Vector& relative_vel, Mth::Vector& relative_rotvel, Mth::Vector& offset );

protected:
    void                    init_skeleton( uint32 skeleton_name );
	
protected:
    Gfx::CSkeleton*         mp_skeleton;
};
	
}

#endif
