/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GFX (Graphics Library)									**
**																			**
**	Module:			Graphics (GFX)		 									**
**																			**
**	File name:		viewport.cpp											**
**																			**
**	Created:		01/29/00	-	mjb										**
**																			**
**	Description:	Graphics viewports										**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/


#include <core/defines.h>
#include <gfx/nxviewport.h>
#include <gfx/nxviewman.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Nx
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/


/*****************************************************************************
**								   Defines									**
*****************************************************************************/


/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/


/*****************************************************************************
**								 Public Data								**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CViewport::CViewport( ) 
: m_Rect(0,0,1,1), m_flags(0), mp_camera(NULL)
{

}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

CViewport::CViewport(const Mth::Rect* rect, Gfx::Camera* cam)
: m_Rect(0,0,1,1), m_flags(0)
{
	

	if (rect)
	{	
		m_Rect.SetOriginX(rect->GetOriginX());
		m_Rect.SetOriginY(rect->GetOriginY());
		m_Rect.SetWidth(rect->GetWidth());
		m_Rect.SetHeight(rect->GetHeight());
	}
	else
	{
		m_Rect.SetOriginX(0);
		m_Rect.SetOriginY(0);
		m_Rect.SetWidth(1);
		m_Rect.SetHeight(1);
	}

	//m_pixel_ratio = parent->GetPixelRatio();

	mp_camera = cam;

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CViewport::~CViewport ( void )
{	
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const float		CViewport::GetAspectRatio() const
{
	return CViewportManager::sGetScreenAspect() * m_Rect.GetWidth() / m_Rect.GetHeight();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CViewport::MarkCameraForDeletion(Gfx::Camera *pCamera)
{
	if (mp_camera == pCamera)
	{
		m_flags |= mCAMERA_MARKED_FOR_DELETION;
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CViewport::DeleteMarkedCamera()
{
	if (m_flags & mCAMERA_MARKED_FOR_DELETION)
	{
		Dbg_Assert(mp_camera);
		delete mp_camera;
		mp_camera = NULL;

		m_flags &= ~mCAMERA_MARKED_FOR_DELETION;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float			CViewport::TransformToScreenCoord(const Mth::Vector & world_pos, float & screen_pos_x, float & screen_pos_y, ZBufferValue & screen_pos_z)
{
	return plat_transform_to_screen_coord(world_pos, screen_pos_x, screen_pos_y, screen_pos_z);
}

///////////////////////////////////////////////////////////////////////////////
// Stub versions of all platform specific functions:

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float			CViewport::plat_transform_to_screen_coord(const Mth::Vector & world_pos, float & screen_pos_x, float & screen_pos_y, ZBufferValue & screen_pos_z)
{
	printf ("STUB: PlatTransformToScreenCoord\n");
	return -1.0f;
}

} // namespace Gfx


