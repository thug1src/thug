//////////////////////////////////////////////////////////////////////////////////////////////
// NxTextured3dPoly.h - Neversoft Engine, Rendering portion, Platform independent interface

#ifndef	__GFX_NX_TEXTURED_3D_POLY_H__
#define	__GFX_NX_TEXTURED_3D_POLY_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#ifndef __CORE_MATH_VECTOR_H
#include <core/math/vector.h>
#endif

namespace Nx
{

// Class for displaying a single textured 3d poly. Currently used by the pedestrian shadows, which
// are just simple circular textures oriented to the ground.
class	CTextured3dPoly
{
protected:
	Mth::Vector mp_pos[4];
	
public:
							CTextured3dPoly();
	virtual					~CTextured3dPoly();

	void					SetTexture(const char *p_textureName);
	void 					SetTexture(uint32 texture_checksum);
	
	void					SetPos(const Mth::Vector &pos, float width, float height, const Mth::Vector &normal, float angle=0.0f);

	void					Render();

	static void				sRenderAll();
		
private:
	virtual void			plat_render() {}
	virtual void			plat_set_texture(uint32 texture_checksum) {}
	
};

}

#endif

