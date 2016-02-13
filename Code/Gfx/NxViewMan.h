#ifndef __GFX_NX_VIEWMAN_H__
#define __GFX_NX_VIEWMAN_H__

#include <gfx/camera.h>
#include <gfx/nxviewport.h>

namespace Script
{
	class CScriptStructure;
	class CScript;
}

namespace Nx
{

extern const float vBASE_ASPECT_RATIO;

enum ScreenMode
{
	vONE_CAM = 0,
	vSPLIT_V,
	vSPLIT_H,
	vSPLIT_QUARTERS,
	vHORSE1,
	vHORSE2,
	vNUM_SCREEN_MODES
};

enum ViewportType
{
	vMAIN = 0,
	vSECONDPLAYER,		// 2nd player fullscreen, for horse
	vSPLIT_V_LEFT,
	vSPLIT_V_RIGHT,
	vSPLIT_H_TOP,
	vSPLIT_H_BOTTOM,
	vSPLIT_QUARTER_UL,
	vSPLIT_QUARTER_UR,
	vSPLIT_QUARTER_LL,
	vSPLIT_QUARTER_LR,
	vNUM_VIEWPORTS
};
	
class CViewportManager
{
public:
	enum
	{
		vMAX_NUM_ACTIVE_VIEWPORTS	= 4,
	};

	static void					sInit(void);
    static void                 sCleanup(void);

	static CViewport*			sGetActiveViewport(int view = 0);
	static int					sGetActiveViewportNumber(CViewport *p_viewport);
	static int					sGetNumActiveViewports();

	static Gfx::Camera*			sGetCamera(int index = 0);
	static void					sSetCamera(int index, Gfx::Camera* pCamera);
	static void					sSetCameraAllScreenModes(int index, Gfx::Camera* pCamera);

	static Gfx::Camera*			sGetActiveCamera(int cam = 0);
	static Gfx::Camera*			sGetClosestCamera(const Mth::Vector &pos, float *distParam = NULL);
	static int					sGetNumActiveCameras();

	static bool					sMarkCameraForDeletion(Gfx::Camera *pCamera);
	static void					sDeleteMarkedCameras();

	// it would be nice to have descriptions of what these 'modes' are...
	// for now, i'm assuming screen mode 0 is where the camera is free from the skater.
	static ScreenMode			sGetScreenMode();
	static void					sSetScreenMode(ScreenMode mode);

	// some fundamental things about the TV
	static	void				sSetScreenAspect(float aspect);
	static	float				sGetScreenAspect();
	static	void				sSetDefaultAngle(float angle);
	static	float				sGetDefaultAngle();
	static	void				sSetScreenAngle(float angle);
	static	float				sGetScreenAngle();
	static	float				sGetScreenAngleFactor();

	// some parameters for 2D objects in 3D space
	static	void				sSet2DIn3DSpaceNoscaleDistance(float distance);
	static	float				sGet2DIn3DSpaceNoscaleDistance();
	static	void				sSet2DIn3DSpaceMaxScale(float scale);
	static	float				sGet2DIn3DSpaceMaxScale();
	static	void				sSet2DIn3DSpaceMinScale(float scale);
	static	float				sGet2DIn3DSpaceMinScale();

protected:
	static CViewport *			s_create_viewport(const Mth::Rect* rect, Gfx::Camera* cam = NULL);

private:
	// The platform dependent calls
	static CViewport *			s_plat_create_viewport(const Mth::Rect* rect, Gfx::Camera* cam);

	static CViewport*			sp_viewports[vNUM_VIEWPORTS];
	static const int*			sp_active_viewport_indexes;
	static int					s_num_active_viewports_cached;

	static const float			s_viewport_rects[vNUM_VIEWPORTS][4];
	static const int 			s_num_active_viewports[vNUM_SCREEN_MODES];
	static const int			s_screen_mode_viewport_indexes[vNUM_SCREEN_MODES][vMAX_NUM_ACTIVE_VIEWPORTS];

	static ScreenMode			s_screen_mode;

	static	float				s_screen_aspect;				 // aspect ratio, usually 4/3 (1.3333333) or 16/9 (1.7777777)
	static	float				s_screen_angle;					 // angle subtended by the width of the screen usually 72 degrees
	static	float				s_default_angle;				 // the default value for s_screen_angle
	static	float				s_screen_angle_factor;			 // factor to multiply all the tan(cam_angle/2) camera values by

	static	float				s_2D_in_3D_space_noscale_distance;	// z distance at which 3D scale would be 1.0
	static	float				s_2D_in_3D_space_max_scale;			// maximum 3D scale
	static	float				s_2D_in_3D_space_min_scale;			// minimum 3D scale
};



bool ScriptSetScreenMode(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptGetScreenMode(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSet2DIn3DSpaceParams(Script::CStruct *pParams, Script::CScript *pScript);

///////////
// Inlines
///////////

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float		CViewportManager::sGet2DIn3DSpaceNoscaleDistance()
{
	return s_2D_in_3D_space_noscale_distance;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float		CViewportManager::sGet2DIn3DSpaceMaxScale()
{
	return s_2D_in_3D_space_max_scale;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float		CViewportManager::sGet2DIn3DSpaceMinScale()
{
	return s_2D_in_3D_space_min_scale;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline int			CViewportManager::sGetNumActiveViewports()
{
//	return s_num_active_viewports[s_screen_mode];
	return s_num_active_viewports_cached;
}


}

#endif // VIEWMAN
