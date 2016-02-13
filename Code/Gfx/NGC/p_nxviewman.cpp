/////////////////////////////////////////////////////////////////////////////
// p_NxViewMan.cpp - Ngc platform specific interface to CViewportManager
//
// This is Ngc SPECIFIC!!!!!!  So might get a bit messy

#include <core/defines.h>

#include "gfx/NxViewMan.h"
#include "gfx/NGC/p_NxViewport.h"

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
	return new CNgcViewport(rect, cam);
}

} 
 

