
/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Nx Viewport Manager										**
**																			**
**	File name:		gfx/nxviewman.cpp										**
**																			**
**	Created by:		04/30/02	-	grj										**
**																			**
**	Description:	Viewport Manager										**
**																			**
*****************************************************************************/

// start autoduck documentation
// @DOC nxviewman
// @module nxviewman | None
// @subindex Scripting Database
// @index script | nxviewman

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <string.h>
#include <core/defines.h>
#include <gfx/NxViewMan.h>
#include <gel/Scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>

namespace Nx
{

// This defines the width to height ratio of the camera's *view* window, and
// as such does not need to be changed for different screen modes or systems.
const float vBASE_ASPECT_RATIO = 1.428566f;

const float	CViewportManager::s_viewport_rects[vNUM_VIEWPORTS][4] =
{
//    x      y      w     h
	{ 0.0f,  0.0f,  1.0f, 1.0f},	// vMAIN
	{ 0.0f,  0.0f,  1.0f, 1.0f},	// vSECONDPLAYER (2nd player fullscreen)
	{ 0.0f,  0.0f,  0.5f, 1.0f},	// vSPLIT_V_LEFT
	{ 0.5f,  0.0f,  0.5f, 1.0f},	// vSPLIT_V_RIGHT
	{ 0.0f,  0.0f,  1.0f, 0.5f},	// vSPLIT_H_TOP
	{ 0.0f,  0.5f,  1.0f, 0.5f},	// vSPLIT_H_BOTTOM
	{ 0.0f,  0.0f,  0.5f, 0.5f},	// vSPLIT_QUARTER_UL
	{ 0.5f,  0.0f,  0.5f, 0.5f},	// vSPLIT_QUARTER_UR
	{ 0.0f,  0.5f,  0.5f, 0.5f},	// vSPLIT_QUARTER_LL
	{ 0.5f,  0.5f,  0.5f, 0.5f}		// vSPLIT_QUARTER_LR
};

const int 	CViewportManager::s_num_active_viewports[vNUM_SCREEN_MODES]
							 = { 1, 2, 2, 4, 1, 1 };
const int	CViewportManager::s_screen_mode_viewport_indexes[vNUM_SCREEN_MODES][vMAX_NUM_ACTIVE_VIEWPORTS]
					= { { vMAIN,			 -1,				-1,				   -1 },				// vONE_CAM
						{ vSPLIT_V_LEFT,	 vSPLIT_V_RIGHT,	-1,				   -1 },				// vSPLIT_V
						{ vSPLIT_H_TOP,		 vSPLIT_H_BOTTOM,	-1,				   -1 },				// vSPLIT_H
						{ vSPLIT_QUARTER_UL, vSPLIT_QUARTER_UR, vSPLIT_QUARTER_LL, vSPLIT_QUARTER_LR },	// vSPLIT_QUARTERS
						{ vMAIN,			 -1,				-1,				   -1 },				// vHORSE1
						{ -1,				 vSECONDPLAYER,		-1,				   -1 },				// vHORSE2
					  };


CViewport*	CViewportManager::sp_viewports[vNUM_VIEWPORTS];
const int*	CViewportManager::sp_active_viewport_indexes = s_screen_mode_viewport_indexes[0];

ScreenMode	CViewportManager::s_screen_mode;
int			CViewportManager::s_num_active_viewports_cached=0;

float		CViewportManager::s_default_angle;
float		CViewportManager::s_screen_angle;
float		CViewportManager::s_screen_angle_factor = 1.0f;
float		CViewportManager::s_screen_aspect;

float		CViewportManager::s_2D_in_3D_space_noscale_distance = 300.0f;
float		CViewportManager::s_2D_in_3D_space_max_scale = 3.0f;
float		CViewportManager::s_2D_in_3D_space_min_scale = 0.0f;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void 		CViewportManager::sInit( void )
{
	// Create viewports
	for ( uint32 i = 0; i < vNUM_VIEWPORTS; i++ )
	{
		Mth::Rect theRect( s_viewport_rects[i][0], s_viewport_rects[i][1], s_viewport_rects[i][2], s_viewport_rects[i][3] );

		sp_viewports[i] = s_create_viewport(&theRect);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CViewportManager::sCleanup( void )
{
	for ( uint32 i = 0; i < vNUM_VIEWPORTS; i++ )
	{
		if ( sp_viewports[i] )
		{
			delete sp_viewports[i];
			sp_viewports[i] = NULL;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CViewport*	CViewportManager::sGetActiveViewport(int view)
{
	Dbg_MsgAssert(view < s_num_active_viewports[s_screen_mode], ("Active viewport number is invalid %d", view));

	int found_views = 0;

	// Go through array, and don't count invalid viewports
	for ( uint32 i = 0; i < vNUM_VIEWPORTS; i++ )
	{
		if (sp_active_viewport_indexes[i] >= 0)
		{
			if (found_views == view)
			{
				return sp_viewports[sp_active_viewport_indexes[i]];
			}
			found_views++;
		}
	}

	Dbg_MsgAssert(0, ("Cant find active viewport # %d", view));

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int			CViewportManager::sGetActiveViewportNumber(CViewport *p_viewport)
{
	int found_views = 0;

	// Go through array, and don't count invalid viewports
	for ( uint32 i = 0; i < vNUM_VIEWPORTS; i++ )
	{
		if (sp_active_viewport_indexes[i] >= 0)
		{
			if (sp_viewports[sp_active_viewport_indexes[i]] == p_viewport)
			{
				return found_views;
			}
			found_views++;
		}
	}

	// Not found
	return -1;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Gfx::Camera* CViewportManager::sGetCamera(int index)
{
	//Dbg_MsgAssert(sp_active_viewport_indexes[index] >= 0, ("Viewport index invalid %d from camera %d, screen mode %d ", sp_active_viewport_indexes[index], index, s_screen_mode));

	if (sp_active_viewport_indexes[index] == -1)
	{
		return NULL;
		//printf ("CAMERA WARNING: - Getting invalid camera %d when in screen mode %d (Returning default camera 0)\n",index,s_screen_mode);  
		//index = 0;
	}

	return sp_viewports[sp_active_viewport_indexes[index]]->GetCamera();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CViewportManager::sSetCamera(int index, Gfx::Camera* pCamera)
{
	Dbg_Assert( index >= 0 && index < vMAX_NUM_ACTIVE_VIEWPORTS );
	if (sp_active_viewport_indexes[index] == -1)
	{
		//printf ("CAMERA WARNING: - Setting invalid camera %d when in screen mode %d (Ignoring)\n",index,s_screen_mode);  
		return;
	}
	sp_viewports[sp_active_viewport_indexes[index]]->SetCamera(pCamera);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CViewportManager::sSetCameraAllScreenModes(int index, Gfx::Camera* pCamera)
{
	for (int mode = 0; mode < vNUM_SCREEN_MODES; mode++)
	{
		if (s_screen_mode_viewport_indexes[mode][index] >= 0)
		{
			//Dbg_Message("******************* SetCameraAllScreenMode: Setting viewport mode %d camera #%d to Camera (%x)", mode, index, pCamera);
			sp_viewports[s_screen_mode_viewport_indexes[mode][index]]->SetCamera(pCamera);
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

ScreenMode	CViewportManager::sGetScreenMode()
{
	return s_screen_mode;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Gfx::Camera* CViewportManager::sGetActiveCamera(int cam)
{
	CViewport *p_viewport = sGetActiveViewport(cam);
	Dbg_MsgAssert(p_viewport, ("sGetActiveCamera(): Active viewport index %d invalid for screen mode %d ", cam, s_screen_mode));

	return p_viewport->GetCamera();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Gfx::Camera* CViewportManager::sGetClosestCamera(const Mth::Vector &pos, float *distParam)
{
	int i;
	int numCams = sGetNumActiveCameras( );
	Gfx::Camera *p_camera;
	Gfx::Camera *p_closestCamera = NULL;
	float closestDist = -1.0f;
	float tempDist;

	for ( i = 0; i < numCams; i++ )
	{
		p_camera = sGetActiveCamera( i );
		if ( p_camera )
		{
			tempDist = Mth::DistanceSqr( pos, p_camera->GetPos() );
			if ( ( closestDist < 0 ) || ( tempDist < closestDist ) )
			{
				closestDist = tempDist;
				p_closestCamera = p_camera;
			}
		}
	}

	//Dbg_MsgAssert( p_closestCamera,( "No cameras or what?" ));
	if ( p_closestCamera == NULL )
	{
		return NULL;
	}

	if ( distParam )
	{
		*distParam = sqrtf(closestDist);
	}
	return p_closestCamera;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int			CViewportManager::sGetNumActiveCameras(void)
{

	return s_num_active_viewports[s_screen_mode];		// Same number of cams as there are viewports
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CViewportManager::sMarkCameraForDeletion(Gfx::Camera *pCamera)
{
	// Check all viewports in case the screen mode is changed after this
	for ( uint32 i = 0; i < vNUM_VIEWPORTS; i++ )
	{
		if ( sp_viewports[i] )
		{
			if (sp_viewports[i]->MarkCameraForDeletion(pCamera))
			{
				return true;
			}
		}
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CViewportManager::sDeleteMarkedCameras()
{
	// Check all viewports in case the screen mode was changed
	for ( uint32 i = 0; i < vNUM_VIEWPORTS; i++ )
	{
		if ( sp_viewports[i] )
		{
			sp_viewports[i]->DeleteMarkedCamera();
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void    	CViewportManager::sSetScreenMode( ScreenMode mode )
{
	// Update active viewport indexes
	sp_active_viewport_indexes = s_screen_mode_viewport_indexes[mode];

	s_screen_mode = mode;
	
	s_num_active_viewports_cached = s_num_active_viewports[s_screen_mode];
	
	 
	// Now update cameras
	for ( int i = 0; i < s_num_active_viewports[mode]; i++ )
	{
		CViewport *p_viewport = sGetActiveViewport(i);
		Dbg_MsgAssert(p_viewport, ("SetScreenMode: Couldn't find active viewport # %d for mode %d", i, mode));

		// no camera has been bound
		Gfx::Camera *p_camera = p_viewport->GetCamera();
		if ( !p_camera )
		{
			//return;
			continue;	   			// just continue, even if there is no camera set up yet.....
		}

		//Dbg_Message("******************* SetScreenMode: Setting Camera %d (%x) of mode %d", i, p_camera, mode);

		// set up proper aspect ratio for camera
		switch (mode)
		{
			case vSPLIT_V:
				// camera "window" at same height, half width as full screen camera
				//p_camera->SetFocalLength ( Script::GetFloat("Skater_Cam_Focal_Length"), vBASE_ASPECT_RATIO / 2.0f);
				p_camera->SetHFOV(Script::GetFloat("Skater_Cam_Horiz_FOV") / 2.0f);
				break;
			case vSPLIT_H:
				// camera "window" at same width, half height as full screen camera
				//p_camera->SetFocalLength ( Script::GetFloat("Skater_Cam_Focal_Length") * 2.0f, vBASE_ASPECT_RATIO * 2.0f);
				p_camera->SetHFOV(Script::GetFloat("Skater_Cam_Horiz_FOV"));
				break;
			default:
				// camera "window" at same width, height as full screen camera
				//p_camera->SetFocalLength ( Script::GetFloat("Skater_Cam_Focal_Length"), vBASE_ASPECT_RATIO);
				p_camera->SetHFOV(Script::GetFloat("Skater_Cam_Horiz_FOV"));
				break;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CViewport *	CViewportManager::s_create_viewport(const Mth::Rect* rect, Gfx::Camera* cam)
{
	return s_plat_create_viewport(rect, cam);
}


// some fundamental things about the TV
//
// s_screen_aspect is the aspect ratio of the TV
// this is the physical aspect ratio, and not the pixel ratio
// thus it will be either 4:3 (4.0f/3.0f) or 16:9 (16.0f/9.0f) 
//
// s_screen_angle is the default view angle for a full screen viewport
// currently (2/8/02) it is set at 72 degrees when screen_aspect is 4:3
// when we have a wider aspect ratio, we want a wider screen angle
// however, we don't want to go too wide, or we will render too much
// 80 degrees is okay, providing us with reduced vertical coverage, but increased
// horizontal coverage (so skater seems bigger on screen, and you can see more of the level)


void				CViewportManager::sSetScreenAspect(float aspect)
{
	Dbg_MsgAssert(aspect > 0.1f && aspect <5.0f,("Unlikely value (%f) for screen aspect",aspect));
	s_screen_aspect = aspect;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float				CViewportManager::sGetScreenAspect()
{
	return s_screen_aspect;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


void				CViewportManager::sSetDefaultAngle(float angle)
{
	Dbg_MsgAssert(angle > 0.01f && angle < 180.0f,("Unlikely value (%f) for screen default angle",angle));
	s_default_angle = angle;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float				CViewportManager::sGetDefaultAngle()
{
	return s_default_angle;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CViewportManager::sSetScreenAngle(float angle)
{
	if (angle == 0.0f)
	{
		angle = s_default_angle;
	}

	Dbg_MsgAssert(angle > 0.01f && angle < 180.0f,("Unlikely value (%f) for screen angle",angle));
	s_screen_angle = angle;

	s_screen_angle_factor = tanf(Mth::DegToRad(angle / 2.0f)) / tanf(Mth::DegToRad(s_default_angle / 2.0f));

	//Dbg_Message("Screen angle %f; Screen angle factor %f", s_screen_angle, s_screen_angle_factor);

	// Update cameras
	// Garrett: This isn't the right way to do it since it won't touch all the possible cameras.
	// It would be better to go through a list of all the cameras.
	for (int view_idx = 0; view_idx < sGetNumActiveViewports(); view_idx++)
	{
		CViewport *p_cur_viewport = CViewportManager::sGetActiveViewport(view_idx);
		if (p_cur_viewport)
		{
			Gfx::Camera *p_cur_camera = p_cur_viewport->GetCamera();

			if (p_cur_camera)
			{
				p_cur_camera->UpdateAdjustedHFOV();
			}
		}
	}

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float				CViewportManager::sGetScreenAngle()
{
	return s_screen_angle;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float				CViewportManager::sGetScreenAngleFactor()
{
	return s_screen_angle_factor;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CViewportManager::sSet2DIn3DSpaceNoscaleDistance(float distance)
{
	s_2D_in_3D_space_noscale_distance = distance;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CViewportManager::sSet2DIn3DSpaceMaxScale(float scale)
{
	s_2D_in_3D_space_max_scale = scale;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CViewportManager::sSet2DIn3DSpaceMinScale(float scale)
{
	s_2D_in_3D_space_min_scale = scale;
}

// @script | SetScreenMode | Sets the way the screen will be split up into separate
// viewports in the game. If there is more than one viewport, and multiple skaters
// are active, then each skater will get his own viewport
// @uparm One_Camera | the mode to use - one of the following: <nl>
// One_Camera <nl>
// Split_Vertical <nl>
// Split_Horizontal <nl>
// Split_Quarters
bool ScriptSetScreenMode(Script::CScriptStructure *pParams, Script::CScript *pScript)
{


	uint32 mode;
	pParams->GetChecksum(NONAME, &mode, true);

	switch (mode)
	{
		case 0x6f70ea65: // "one_camera"
			CViewportManager::sSetScreenMode(vONE_CAM);
			break;
		case 0x9b62e5a6: // "split_vertical"
			CViewportManager::sSetScreenMode(vSPLIT_V);
			break;
		case 0xb07b580e: // "split_horizontal"
			CViewportManager::sSetScreenMode(vSPLIT_H);
			break;
		case 0xb6624a73: // "split_quarters"
			CViewportManager::sSetScreenMode(vSPLIT_QUARTERS);
			break;
		case 0x6f2d1231: // "horse1"
			CViewportManager::sSetScreenMode(vHORSE1);
			break;
		case 0xf624438b: // "horse2"
			CViewportManager::sSetScreenMode(vHORSE2);
			break;
		default:
			Dbg_MsgAssert(0,( "invalid screen mode"));
			break;
	}
	return true;
}

bool ScriptGetScreenMode(Script::CScriptStructure *pParams, Script::CScript *pScript)
{

	Script::CStruct* p_return_params;

	p_return_params = pScript->GetParams();
	switch( CViewportManager::sGetScreenMode())
	{
		case vSPLIT_V:
			p_return_params->AddChecksum( "screen_mode", Script::GenerateCRC( "split_vertical" ));
			break;
		case vSPLIT_H:
			p_return_params->AddChecksum( "screen_mode", Script::GenerateCRC( "split_horizontal" ));
			break;
		case vSPLIT_QUARTERS:
			p_return_params->AddChecksum( "screen_mode", Script::GenerateCRC( "split_quarters" ));
			break;
		case vONE_CAM:
			p_return_params->AddChecksum( "screen_mode", Script::GenerateCRC( "one_camera" ));
			break;
		case vHORSE1:
			p_return_params->AddChecksum( "screen_mode", Script::GenerateCRC( "horse1" ));
			break;
		case vHORSE2:
			p_return_params->AddChecksum( "screen_mode", Script::GenerateCRC( "horse2" ));
			break;
			
		default:
			Dbg_MsgAssert(0,( "invalid screen mode"));
			break;
	}

	return true;
}

// @script | Set2DIn3DSpaceParams | Sets the parameters for the 2D Screen Elements in 3D space
// @parmopt float | noscale_distance | 100.0 | Distance at which the 3D scale is equal to 1.0 (in inches)
// @parmopt float | max_scale | 3.0 | Maximum 3D scale
// @parmopt float | min_scale | 0.0 | Minimum 3D scale
bool ScriptSet2DIn3DSpaceParams(Script::CStruct *pParams, Script::CScript *pScript)
{
	float distance, scale;

	if (pParams->GetFloat(CRCD(0x33aa4915,"noscale_distance"), &distance))
	{
		CViewportManager::sSet2DIn3DSpaceNoscaleDistance(distance);
	}

	if (pParams->GetFloat(CRCD(0xa0a0db70,"max_scale"), &scale))
	{
		CViewportManager::sSet2DIn3DSpaceMaxScale(scale);
	}

	if (pParams->GetFloat(CRCD(0x774b6931,"min_scale"), &scale))
	{
		CViewportManager::sSet2DIn3DSpaceMinScale(scale);
	}

	return true;
}

}

