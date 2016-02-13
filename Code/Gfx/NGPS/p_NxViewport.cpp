///////////////////////////////////////////////////////////////////////////////
// p_NxViewport.cpp

#include 	"Gfx/NxViewMan.h"
#include 	"Gfx/NGPS/p_NxViewport.h"

#include 	"Gfx/NGPS/NX/render.h"
#include 	"Gfx/NGPS/NX/sprite.h"
#include 	"Gfx/NGPS/NX/switches.h"

namespace Nx
{

////////////////////////////////////////////////////////////////////////////////////
// Here's a machine specific implementation of CViewport

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Viewport::CPs2Viewport()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Viewport::CPs2Viewport( const Mth::Rect* rect, Gfx::Camera* cam) :
	CViewport(rect, cam)
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Viewport::~CPs2Viewport()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CPs2Viewport::update_render_vars()
{
	int view_idx = CViewportManager::sGetActiveViewportNumber(this);
	Dbg_MsgAssert(view_idx >= 0, ("Can't find viewport in active viewport list"));

	if (!NxPs2::render::VarsUpToDate(view_idx) && mp_camera)
	{
		Mth::Matrix cam_matrix(mp_camera->GetMatrix());
		cam_matrix[W] = mp_camera->GetPos();

		NxPs2::render::SetupVars(view_idx, cam_matrix, GetRect(), mp_camera->GetAdjustedHFOV(), GetAspectRatio(),
							  /*-mp_camera->GetNearClipPlane(), -mp_camera->GetFarClipPlane()*/ -1.0f, -100000.0f);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float			CPs2Viewport::plat_transform_to_screen_coord(const Mth::Vector & world_pos, float & screen_pos_x, float & screen_pos_y, ZBufferValue & screen_pos_z)
{
	float zdistance;
	float scale;

	update_render_vars();

	// First check frustum
	Mth::Vector screen_pos(NxPs2::render::WorldToFrustum.Transform(world_pos));

	//if ((Tmr::GetVblanks() % 200) == 0)
	//{
	//	Dbg_Message("Homogenious (%g, %g, %g, %g)", screen_pos[X], screen_pos[Y], screen_pos[Z], screen_pos[W]);
	//}

	if ((screen_pos[X] > screen_pos[W]) || (screen_pos[X] < -screen_pos[W]) ||
		(screen_pos[Y] > screen_pos[W]) || (screen_pos[Y] < -screen_pos[W]) ||
		(screen_pos[Z] > screen_pos[W]) || (screen_pos[Z] < -screen_pos[W]))
	{
		return -1.0f;
	} else {
		zdistance = -screen_pos[Z]; //(screen_pos[Z] - screen_pos[W]) / (2.0f * screen_pos[W]);
	}

	// Now find screen coordinates
	screen_pos = NxPs2::render::WorldToIntViewport.Transform(world_pos);

	//if ((Tmr::GetVblanks() % 200) == 0)
	//{
	//	Dbg_Message("Converted (%f, %f, %f, %f) to (%g, %g, %g, %g)", world_pos[X], world_pos[Y], world_pos[Z], world_pos[W],
	//																  screen_pos[X], screen_pos[Y], screen_pos[Z], screen_pos[W]);
	//}

	// Get into homogenious space
	float oow = 1.0f / screen_pos[W];
	screen_pos *= oow;

	// Convert the int back to float
	screen_pos_x = (((float) ( *((int *) &(screen_pos[X])) & 0xFFFF )) - XOFFSET) / 16.0f;
	screen_pos_y = (((float) ( *((int *) &(screen_pos[Y])) & 0xFFFF )) - YOFFSET) / 16.0f;
	screen_pos_z = (ZBufferValue) ( *((int *) &(screen_pos[Z])) & 0xFFFFFF );

	// Convert back to NTSC, if necessary
	screen_pos_x /= NxPs2::SDraw2D::GetScreenScaleX();
	screen_pos_y /= NxPs2::SDraw2D::GetScreenScaleY();

	//if ((Tmr::GetVblanks() % 200) == 0)
	//{
	//	Dbg_Message("Divide by W (%g, %g, %x, %g) Z distance: %g", screen_pos_x, screen_pos_y, screen_pos_z, screen_pos[W], zdistance);
	//}

	// Calculate scale
	scale = CViewportManager::sGet2DIn3DSpaceNoscaleDistance() / zdistance;
	scale = Mth::Min(scale, CViewportManager::sGet2DIn3DSpaceMaxScale());
	scale = Mth::Max(scale, CViewportManager::sGet2DIn3DSpaceMinScale());

#if 0		// This scales 1 sprite or text pixel to an inch!
	float hfov;
	if (mp_camera)
	{
		hfov = mp_camera->GetAdjustedHFOV();
	} else {
		hfov = CViewportManager::sGetScreenAngle();
	}

	// Calculate scale
	float h = ((float) HRES) / (tanf(Mth::DegToRad(hfov / 2.0f)));
	scale = h / zdistance;

	if ((Tmr::GetVblanks() % 300) == 0)
	{
		Dbg_Message("H %f, ZDistance %f, Scale %f, HFOC %f", h, zdistance, scale, hfov);
	}
#endif

	return scale;
}

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//

} // Namespace Nx  			
				
				
