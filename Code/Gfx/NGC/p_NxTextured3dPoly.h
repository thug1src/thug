#ifndef	__GFX_P_NX_TEXTURED_3D_POLY_H__
#define	__GFX_P_NX_TEXTURED_3D_POLY_H__

#include 	<gfx/nxtextured3dpoly.h>

namespace NxNgc
{

// Machine specific implementation of CTextured3dPoly
class	CNgcTextured3dPoly : public Nx::CTextured3dPoly
{
public:
		   					CNgcTextured3dPoly();
	virtual					~CNgcTextured3dPoly();
private:
	void					plat_render();
	void					plat_set_texture(uint32 texture_checksum);
};

}	// namespace NxNgc

#endif
				   
