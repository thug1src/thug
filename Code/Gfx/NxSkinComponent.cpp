//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       NxSkinComponent.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  12/21/2001
//****************************************************************************

#include "gfx/nxskincomponent.h"

namespace Nx
{

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Stub versions of all platform specific functions are provided here:
// so engine implementors can leave certain functionality until later
						
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkinComponent::plat_set_color(uint8 r, uint8 g, uint8 b, uint8 a)
{
    // STUB
    printf("STUB FUNCTION called:  CSkinComponent::set_color %d %d %d %d", r, g, b, a);
}
         
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkinComponent::plat_set_visibility(uint32 mask)
{
    // STUB
    printf("STUB FUNCTION called:  CSkinComponent::set_visibility %08x", mask);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkinComponent::plat_set_scale(float scaleFactor)
{
    // STUB
    printf("STUB FUNCTION called:  CSkinComponent::set_scale %f", scaleFactor);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkinComponent::plat_replace_texture(char* p_srcFileName, char* p_dstFileName)
{
    Dbg_Assert(p_srcFileName);
    Dbg_Assert(p_dstFileName);
    
    // STUB
    printf("STUB FUNCTION called:  CSkinComponent::replace_texture %s %s", p_srcFileName, p_dstFileName);
}

/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

// These functions are the platform independent part of the interface to 
// the platform specific code
// parameter checking can go here....
// although we might just want to have these functions inline, or not have them at all?

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkinComponent::CSkinComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSkinComponent::~CSkinComponent()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkinComponent::SetColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
    plat_set_color(r,g,b,a);
}
         
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkinComponent::SetVisibility(uint32 mask)
{
    plat_set_visibility(mask);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkinComponent::SetScale(float scaleFactor)
{
    plat_set_scale(scaleFactor);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CSkinComponent::ReplaceTexture(char* p_srcFileName, char* p_dstFileName)
{
    plat_replace_texture(p_srcFileName, p_dstFileName);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // Nx



