//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       CollisionComponent.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  10/24/2002
//****************************************************************************

#ifndef __COMPONENTS_COLLISIONCOMPONENT_H__
#define __COMPONENTS_COLLISIONCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

#include <gel/collision/collenums.h>

// Just thinking about it - a generic way of accessing the component				 
#define		CRC_COLLISION CRCD(0xa432dc41,"Collision")
#define		GetCollisionComponent() ((Obj::CCollisionComponent*)GetComponent(CRC_COLLISION))
#define		GetCollisionComponentFromObject(pObj) ((Obj::CCollisionComponent*)(pObj)->GetComponent(CRC_COLLISION))

namespace Nx
{
	class CCollObj;
	class CCollObjTriData;
}

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CCollisionComponent : public CBaseComponent
{
public:
    CCollisionComponent();
    virtual ~CCollisionComponent();

public:
    virtual void            		Update();
	virtual void            		Teleport();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	
    static CBaseComponent*			s_create();
	
public:
	Nx::CCollObj*					GetCollision() const;

protected:
	virtual void					InitCollision( Nx::CollType type, Nx::CCollObjTriData *p_coll_tri_data = NULL );
	Nx::CCollObj*					mp_collision;
};

}

#endif
