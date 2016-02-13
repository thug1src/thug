/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GFX (Graphics Library)									**
**																			**
**	Module:			Graphics  (GFX)											**
**																			**
**	File name:		gfx/viewport.h											**
**																			**
**	Created: 		01/29/00	-	mjb										**
**																			**
*****************************************************************************/

#ifndef __GFX_VIEWPORT_H
#define __GFX_VIEWPORT_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/support.h>
#include <core/math.h>
#include <gfx/camera.h>
#include <gel/objptr.h>
			  
/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Nx
{

// Since a ZBuffer value is a platform-specific type, we need "void" type for it.
// Since we do have to give it a size, the type has been assigned to a uint32.
// It should not be treated as a uint32 above the p-line, though.
typedef uint32 ZBufferValue;

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class  CViewport
{
	
	public :
						CViewport();
						CViewport(const Mth::Rect* Rect, Gfx::Camera* cam = NULL);
	virtual				~CViewport();

	Gfx::Camera *		GetCamera();
	void				SetCamera(Gfx::Camera* cam);

	const Mth::Rect &	GetRect() const;

	const		float	GetOriginX() const;
	const		float	GetOriginY() const;
	const		float	GetWidth() const;
	const		float	GetHeight() const;

	const		float	GetAspectRatio() const;

	// These functions are used to prevent the camera from being deleted prematurely
				bool	MarkCameraForDeletion(Gfx::Camera *pCamera);
				void	DeleteMarkedCamera();

				float	TransformToScreenCoord(const Mth::Vector & world_pos, float & screen_pos_x, float & screen_pos_y, ZBufferValue & screen_pos_z);

protected:	
	// Internal flags
	enum
	{
		mCAMERA_MARKED_FOR_DELETION		= 0x00000001,
	};

	
	Mth::Rect			m_Rect;
	uint32				m_flags;

	Obj::CSmtPtr<Gfx::Camera> mp_camera;			// Smart pointer to camera   
//	Gfx::Camera *		mp_camera;

private :	
	virtual float		plat_transform_to_screen_coord(const Mth::Vector & world_pos, float & screen_pos_x, float & screen_pos_y, ZBufferValue & screen_pos_z);
};

/*****************************************************************************
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

inline const Mth::Rect & CViewport::GetRect() const
{
	return m_Rect;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const float	CViewport::GetOriginX ( void ) const
{
   	

	return m_Rect.GetOriginX();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const float	CViewport::GetOriginY ( void ) const
{
   	

	return m_Rect.GetOriginY();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const float	CViewport::GetWidth ( void ) const
{
   	

	return m_Rect.GetWidth();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const float	CViewport::GetHeight ( void ) const
{
   	

	return m_Rect.GetHeight();
}
	
#if 0
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
			
inline const float		CViewport::GetPixelRatio ( void ) const
{
   	

	return 	m_pixel_ratio;
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// Returns a pointer to the viewport's camera
// since this uses a smart pointer, it will be NULL
// if the camera has been deleted
			
inline Gfx::Camera *	CViewport::GetCamera ( void )
{
	return 	mp_camera.Convert();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
			
inline void 			CViewport::SetCamera ( Gfx::Camera* cam )
{
	DeleteMarkedCamera();			// In case there is an old camera
	mp_camera = cam;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
			
} // namespace Nx

#endif	// __GFX_VIEWPORT_H
