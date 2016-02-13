//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       SkitchComponent.h
//* OWNER:          Dave Cowling
//* CREATION DATE:  02/19/03
//****************************************************************************

#ifndef __COMPONENTS_SKITCHCOMPONENT_H__
#define __COMPONENTS_SKITCHCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>
#include <gel/components/objecthookmanagercomponent.h>

// Replace this with the CRCD of the component you are adding
#define		CRC_SKITCH								CRCD( 0x3506ce64, "Skitch" )

//  Standard accessor macros for getting the component either from within an object, or given an object
#define		GetSkitchComponent()					((Obj::CSkitchComponent*)GetComponent( CRC_SKITCH ))
#define		GetSkitchComponentFromObject( pObj )	((Obj::CSkitchComponent*)(pObj)->GetComponent( CRC_SKITCH ))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CSkitchComponent : public CBaseComponent
{
public:
    CSkitchComponent();
    virtual ~CSkitchComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();

	bool							CanSkitch( void )			{ return m_skitch_allowed; }
	void							AllowSkitch( bool allow )	{ m_skitch_allowed = allow; }
	int								GetNearestSkitchPoint( Mth::Vector *p_point, Mth::Vector &pos );
	bool							GetIndexedSkitchPoint( Mth::Vector *p_point, int index );

private:
	bool							m_skitch_allowed;
	CObjectHookManagerComponent*	mp_object_hook_manager_component;
};

}

#endif
