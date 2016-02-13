//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       camera.cpp
//* OWNER:          Mark Burton
//* CREATION DATE:  11/11/1999
//****************************************************************************

#ifndef __GFX_CAMERA_H
#define __GFX_CAMERA_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/support.h>
#include <core/math.h>
#ifdef __PLAT_NGC__
#include "sys/ngc/p_camera.h"
#endif		// __PLAT_NGC__
#ifndef __GEL_OBJECT_H
#include <gel/object.h>
#endif

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Script
{
	class CStruct;
}

namespace Gfx
{

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class Camera : public Obj::CObject
{
	
public:
	static const float vUSE_PIXEL_RATIO;

	Camera( void );
	virtual ~Camera( void );

public:	
	void			SetPos(const Mth::Vector& pos);		// Set the position of the camera
	Mth::Vector	&	GetPos();							// Return the position of the camera
	void			SetMatrix(const Mth::Matrix& mat);	// Set the orientation matrix
	Mth::Matrix	&	GetMatrix();						// Return the orientation matrix


	void			SetHFOV(float h_fov);
	float			GetHFOV() const;
	void			UpdateAdjustedHFOV();
	float			GetAdjustedHFOV();

#if 0
	void			SetFOV ( float v_fov, float h_aspect = vUSE_PIXEL_RATIO );
	void			SetViewPlane ( float h_fov, float v_fov );
	void			SetFocalLength ( float focal_length, float h_aspect = vUSE_PIXEL_RATIO );

	float			GetFOV ( void );
	float			GetFocalLength ( void );
	float			GetAspectRatio ( void );
#endif

	void			SetNearFarClipPlanes ( float nearPlane, float farPlane );
	void			SetFogNearPlane ( float nearPlane );
	
	float			GetNearClipPlane() const;
	float			GetFarClipPlane() const;

	static const float	vDEFAULT_NEARZ;
	static const float	vDEFAULT_FARZ;

	void			StoreOldPos( void );
	Mth::Vector		m_old_pos;

public:
	bool			CallMemberFunction( uint32 checksum, Script::CStruct* pParams, Script::CScript* pScript ); 

protected:  	
	Mth::Vector		m_pos;								// camera position
	Mth::Matrix		m_matrix;							// orientation matrix

	float			m_h_fov;							// horizontal field of view angle (degrees)
	float			m_adj_h_fov;						// screen adjusted horizontal field of view angle (degrees)

	float			m_near_clip;						// near clip plane
	float			m_far_clip;							// far clip plane
};

/*****************************s************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float Camera::GetHFOV() const
{
	return m_h_fov;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void Camera::SetNearFarClipPlanes ( float nearPlane, float farPlane ) 
{
	Dbg_MsgAssert( farPlane > 0.0f,( "Far Plane <= 0" ));
	Dbg_MsgAssert( nearPlane > 0.0f,( "Near Plane <= 0" ));
	Dbg_MsgAssert( farPlane > nearPlane,( "Far Plane <= Near Plane" ));

	m_far_clip = farPlane;
	m_near_clip = nearPlane;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void Camera::SetFogNearPlane ( float nearPlane ) 
{
//	printf ("STUBBED:  SetFogNearPlane\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float Camera::GetNearClipPlane() const
{
	return m_near_clip;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float Camera::GetFarClipPlane() const
{
	return m_far_clip;
}

} // namespace Gfx

#endif	// __GFX_CAMERA_H
