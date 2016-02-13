///////////////////////////////////////////////////////////////////////////////////
// p_NxLight.H - Neversoft Engine, Rendering portion, Platform dependent interface

#ifndef	__GFX_P_NX_LIGHT_H__
#define	__GFX_P_NX_LIGHT_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <core/math.h>
#include <gfx/image/imagebasic.h>
#include <gfx/nxlight.h>

namespace Nx
{

///////////////////////////////////////////////////////////////////////////////////
// Nx::CXboxModelLights
class	CXboxModelLights : public CModelLights
{
public:
								CXboxModelLights();
	virtual						~CXboxModelLights();

private:	

	// Platform-specific calls
	virtual void				plat_update_engine( Mth::Vector & pos, bool add_scene_light );
	virtual bool				plat_set_light_ambient_color(const Image::RGBA &rgba);
	virtual bool				plat_set_light_direction(int light_index, const Mth::Vector &direction);
	virtual bool				plat_set_light_diffuse_color(int light_index, const Image::RGBA &rgba);
	virtual Image::RGBA			plat_get_light_ambient_color() const;
	virtual const Mth::Vector &	plat_get_light_direction(int light_index) const;
	virtual Image::RGBA			plat_get_light_diffuse_color(int light_index) const;

	virtual void				plat_update_brightness();

	virtual void				plat_enable_ambient_light(bool enable);
	virtual void				plat_enable_diffuse_light(int light_index, bool enable);
	virtual bool				plat_is_ambient_light_enabled() const;
	virtual bool				plat_is_diffuse_light_enabled(int light_index) const;

	uint32						m_flags;
	Image::RGBA					m_ambient_color;
	Image::RGBA					m_diffuse_color[CLightManager::MAX_LIGHTS];
	Mth::Vector					m_diffuse_direction[CLightManager::MAX_LIGHTS];
};


}


#endif

