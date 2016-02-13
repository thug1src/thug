#ifndef	__GFX_P_NX_TEXTURED_3D_POLY_H__
#define	__GFX_P_NX_TEXTURED_3D_POLY_H__

#include <gfx/nxtextured3dpoly.h>

namespace Nx
{

// Machine specific implementation of CTextured3dPoly
class	CXboxTextured3dPoly : public Nx::CTextured3dPoly
{
public:
		   					CXboxTextured3dPoly();
	virtual					~CXboxTextured3dPoly();
private:
	virtual void			plat_render();
};

}	// namespace NxXbox

#endif
				   
