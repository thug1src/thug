//****************************************************************************
//* MODULE:         Gel/Components
//* FILENAME:       CameraComponent.cpp
//* OWNER:          Dave Cowling
//* CREATION DATE:  02/21/03
//****************************************************************************

#include <gel/components/cameracomponent.h>

#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>

namespace Obj
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CBaseComponent* CCameraComponent::s_create()
{
	return static_cast< CBaseComponent* >( new CCameraComponent );	
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CCameraComponent::CCameraComponent() : CBaseComponent()
{
	SetType( CRC_CAMERA );

	// Enabled by default.
	m_enabled = true;

	// Create and attach a Gfx::Camera.
	mp_camera = new Gfx::Camera();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CCameraComponent::~CCameraComponent()
{
	// Destroy the Gfx::Camera.
	delete mp_camera;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CCameraComponent::InitFromStructure( Script::CStruct* pParams )
{
	// cameras must have a very low priority to insure that all objects update before they do (most importantly, the skater)
	CCompositeObjectManager::Instance()->SetObjectPriority(*GetObject(), -1000);
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CCameraComponent::RefreshFromStructure( Script::CStruct* pParams )
{
	InitFromStructure(pParams);
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CCameraComponent::Update()
{
	if( m_enabled )
	{
		// Use the position and orientation of the parent object to position and orient the attached camera.
		Mth::Vector pos = GetObject()->GetPos();
		Mth::Matrix mat = GetObject()->GetMatrix();

		// Set the Display pos of the object to the actual pos, so we can attach a model
		// to the camera
		GetObject()->SetDisplayMatrix(mat);

		mp_camera->SetPos( pos );
		mp_camera->SetMatrix( mat );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CBaseComponent::EMemberFunctionResult CCameraComponent::CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript )
{
	switch ( Checksum )
	{
/*
        // @script | DoSomething | does some functionality
		case 0xbb4ad101:		// DoSomething
			DoSomething();
			break;

        // @script | ValueIsTrue | returns a boolean value
		case 0x769260f7:		// ValueIsTrue
		{
			return ValueIsTrue() ? CBaseComponent::MF_TRUE : CBaseComponent::MF_FALSE;
		}
		break;
*/

		default:
			return CBaseComponent::MF_NOT_EXECUTED;
	}

	// the "default" case of the switch statement handles
	// unrecognized functions;  if we make it down here,
	// that means that the component already handled it
	// somehow
    return CBaseComponent::MF_TRUE;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CCameraComponent::GetDebugInfo(Script::CStruct *p_info)
{
#ifdef	__DEBUG_CODE__
	Dbg_MsgAssert(p_info,("NULL p_info sent to CCameraComponent::GetDebugInfo"));

	// Add any script components to the p_info structure,
	// and they will be displayed in the script debugger (qdebug.exe)
	// you will need to add the names to debugger_names.q, if they are not existing checksums

	/*	Example:
	p_info->AddInteger("m_never_suspend",m_never_suspend);
	p_info->AddFloat("m_suspend_distance",m_suspend_distance);
	*/
	
	// We call the base component's GetDebugInfo, so we can add info from the common base component.
	CBaseComponent::GetDebugInfo(p_info);	  
#endif				 
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CCameraComponent::Enable( bool enable )
{
	m_enabled = enable;

	// Go through and enable other attached components?
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const Mth::Vector &CCameraComponent::GetPosition( void ) const
{
	return GetObject()->GetPos();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CCameraComponent::SetPosition( Mth::Vector& pos )
{
 	GetObject()->SetPos( pos );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Mth::Matrix& CCameraComponent::GetMatrix( void )
{
	return GetObject()->GetMatrix();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CCameraComponent::StoreOldPosition( void )
{
	if( mp_camera )
	{
		mp_camera->StoreOldPos();
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CCameraComponent::SetHFOV( float hfov )
{
	if( mp_camera )
	{
		mp_camera->SetHFOV( hfov );
	}
}



}