///////////////////////////////////////////////////////////////////////////////////
// NxLight.H - Neversoft Engine, Rendering portion, Platform independent interface

#ifndef	__GFX_NX_LIGHT_H__
#define	__GFX_NX_LIGHT_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <core/math.h>
#include <gfx/image/imagebasic.h>
#include <gfx/nxlightman.h>


namespace Nx
{


///////////////////////////////////////////////////////////////////////////////////
// Nx::CModelLights
class	CModelLights
{
public:
								CModelLights();
	virtual						~CModelLights();

	// lights
	bool						SetLightAmbientColor(const Image::RGBA &rgba);
	Image::RGBA					GetLightAmbientColor();

	bool						SetLightDirection(int light_index, const Mth::Vector &direction);
	const Mth::Vector &			GetLightDirection(int light_index);

	bool						SetLightDiffuseColor(int light_index, const Image::RGBA &rgba);
	Image::RGBA					GetLightDiffuseColor(int light_index);

	// Upload values to engine (where applicable)
	void						UpdateEngine( Mth::Vector & pos, bool add_scene_light = false );
	
	// Enable lights
	void						EnableAmbientLight(bool enable);
	void						EnableDiffuseLight(int light_index, bool enable);
	bool						IsAmbientLightEnabled() const;
	bool						IsDiffuseLightEnabled(int light_index) const;

	// Sets the modulation brightness
	bool						SetBrightness(float brightness);
	float						GetBrightness() const;
	void						UpdateBrightness();

	// Position pointer
	bool						SetPositionPointer(Mth::Vector *p_pos);
	Mth::Vector *				GetPositionPointer();

protected:	
	// Internal flags.
	enum {
		mUSE_MODEL_AMBIENT		= 0x0001,					// Use ambient light
		mUSE_MODEL_DIFFUSE_0	= 0x0002,					// Use diffuse light 0
		mUSE_MODEL_DIFFUSE_1	= 0x0004,					// rest of diffuse must follow 0
	};

	//Image::RGBA					m_light_ambient_rgba;
	//Mth::Vector					m_light_direction[CLightManager::MAX_LIGHTS];
	//Image::RGBA					m_light_diffuse_rgba[CLightManager::MAX_LIGHTS];

	// Modulation brightness
	float						m_brightness;
	float						m_brightness_target;
	float						m_ambient_brightness;
	float						m_diffuse_brightness[CLightManager::MAX_LIGHTS];

	// Position pointer (for scene lights)
	Mth::Vector *				mp_pos;

private:
	// Platform-specific calls
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


}


#endif

