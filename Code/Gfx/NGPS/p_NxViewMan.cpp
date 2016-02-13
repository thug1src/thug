/////////////////////////////////////////////////////////////////////////////
// p_NxViewMan.cpp - PS2 platform specific interface to CViewportManager
//
// This is PS2 SPECIFIC!!!!!!  So might get a bit messy

#include <core/defines.h>

#include "gfx/NxViewMan.h"
#include "gfx/NGPS/p_NxViewport.h"

namespace	Nx
{


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// Functions


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CViewport *		CViewportManager::s_plat_create_viewport(const Mth::Rect* rect, Gfx::Camera* cam)
{
	return new CPs2Viewport(rect, cam);
}

} 
 
