#ifndef	__GFX_P_NX_TEXTURED_3D_POLY_H__
#define	__GFX_P_NX_TEXTURED_3D_POLY_H__

#include <gfx/nxtextured3dpoly.h>
#include <gfx/ngps/nx/sprite.h>

namespace NxPs2
{

// Machine specific implementation of CTextured3dPoly
class	CPs2Textured3dPoly : public Nx::CTextured3dPoly
{
public:
							CPs2Textured3dPoly();
	virtual					~CPs2Textured3dPoly();
private:
	void					plat_render();
	void					plat_set_texture(uint32 texture_checksum);
	
	NxPs2::SSingleTexture*	mp_engine_texture;
};

}	// namespace NxPs2

#endif
				   
