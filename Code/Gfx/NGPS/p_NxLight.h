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

namespace NxPs2
{
	class CLightGroup;
}

namespace Nx
{

///////////////////////////////////////////////////////////////////////////////////
// Nx::CPs2ModelLights
class	CPs2ModelLights : public CModelLights
{
public:
								CPs2ModelLights();
	virtual						~CPs2ModelLights();

	NxPs2::CLightGroup *		GetEngineLightGroup() const;

private:	
	// Constants
	enum
	{
		SCENE_LIGHT_INDEX = 2,
	};

	// The engine light group
	NxPs2::CLightGroup *		mp_light_group;

	// Platform-specific calls
	//virtual void				plat_update_lights();
	virtual bool				plat_set_light_ambient_color(const Image::RGBA &rgba);
	virtual bool				plat_set_light_direction(int light_index, const Mth::Vector &direction);
	virtual bool				plat_set_light_diffuse_color(int light_index, const Image::RGBA &rgba);
	virtual Image::RGBA			plat_get_light_ambient_color() const;
	virtual const Mth::Vector &	plat_get_light_direction(int light_index) const;
	virtual Image::RGBA			plat_get_light_diffuse_color(int light_index) const;

	virtual void				plat_update_engine( Mth::Vector & pos, bool add_scene_light );

	virtual void				plat_update_brightness();

	virtual void				plat_enable_ambient_light(bool enable);
	virtual void				plat_enable_diffuse_light(int light_index, bool enable);
	virtual bool				plat_is_ambient_light_enabled() const;
	virtual bool				plat_is_diffuse_light_enabled(int light_index) const;
};


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline NxPs2::CLightGroup *	CPs2ModelLights::GetEngineLightGroup() const
{
	return mp_light_group;
}

}


#endif

