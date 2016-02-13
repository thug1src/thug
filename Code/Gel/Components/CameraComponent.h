//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       CameraComponent.h
//* OWNER:          Dave Cowling
//* CREATION DATE:  02/21/03
//****************************************************************************

#ifndef __COMPONENTS_CAMERACOMPONENT_H__
#define __COMPONENTS_CAMERACOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#include <gel/object/basecomponent.h>
#include <gfx/camera.h>

// Replace this with the CRCD of the component you are adding.
#define		CRC_CAMERA								CRCD( 0xc4e311fa, "Camera" )

//  Standard accessor macros for getting the component either from within an object, or given an object.
#define		GetCameraComponent()					((Obj::CCameraComponent*)GetComponent( CRC_CAMERA ))
#define		GetCameraComponentFromObject( pObj )	((Obj::CCameraComponent*)(pObj)->GetComponent( CRC_CAMERA ))

namespace Script
{
    class CScript;
    class CStruct;
}
              
namespace Obj
{

class CCameraComponent : public CBaseComponent
{
public:
    CCameraComponent();
    virtual ~CCameraComponent();

public:
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );

	static CBaseComponent*			s_create();

	void							SetHFOV( float hfov );

	void							Enable( bool enable );
	Gfx::Camera*					GetCamera( void )		{ return mp_camera; }
	const Mth::Vector &				GetPosition( void ) const ;
	void							SetPosition( Mth::Vector& pos );
	void							StoreOldPosition( void );
	Mth::Matrix&					GetMatrix( void );

private:

	bool							m_enabled;
	Gfx::Camera*					mp_camera;
};

}

#endif
