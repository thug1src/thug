//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       NodeArrayComponent.h
//* OWNER:          Dave Cowling
//* CREATION DATE:  02/17/03
//****************************************************************************

#ifndef __COMPONENTS_NODEARRAYCOMPONENT_H__
#define __COMPONENTS_NODEARRAYCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>

// Just thinking about it - a generic way of accessing the component				 
#define		CRC_NODEARRAY							CRCD( 0xc472ecc5, "NodeArray" )
#define		GetNodeArrayComponent()					((Obj::CNodeArrayComponent*)GetComponent( CRC_NODEARRAY ))
#define		GetNodeArrayComponentFromObject( pObj )	((Obj::CNodeArrayComponent*)(pObj)->GetComponent( CRC_NODEARRAY ))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CNodeArrayComponent : public CBaseComponent
{
public:
    CNodeArrayComponent();
    virtual ~CNodeArrayComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();

	virtual Script::CArray*			GetNodeArray( void )	{ return mp_node_array; }

private:
	Script::CArray*					mp_node_array;
};

}

#endif
