///////////////////////////////////////////////////////////////////////////////
// NxLight.cpp


#include <core/defines.h>

#include "gfx/NxLight.h"

namespace	Nx
{

/////////////////////////////////////////////////////////////////////////////
// CModelLights

/////////////////////////////////////////////////////////////////////////////
// These functions are the platform independent part of the interface.
					 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CModelLights::CModelLights()
{
	m_brightness = m_brightness_target = 0.0f;

	m_ambient_brightness = 1.0f;

	for (int i = 0; i < CLightManager::MAX_LIGHTS; i++)
	{
		m_diffuse_brightness[i] = 1.0f;
	}

	mp_pos = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CModelLights::~CModelLights()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CModelLights::SetLightAmbientColor(const Image::RGBA &rgba)
{
	//m_light_ambient_rgba = rgba;

	bool success = plat_set_light_ambient_color(rgba);

	// need to call this whenever the color/brightness changes
	plat_update_brightness();

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA			CModelLights::GetLightAmbientColor()
{
	return plat_get_light_ambient_color();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CModelLights::SetLightDirection(int light_index, const Mth::Vector &direction)
{
	Dbg_Assert(light_index < CLightManager::MAX_LIGHTS);

	//m_light_direction[light_index] = direction;

	return plat_set_light_direction(light_index, direction);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Vector &	CModelLights::GetLightDirection(int light_index)
{
	Dbg_Assert(light_index < CLightManager::MAX_LIGHTS);

	return plat_get_light_direction(light_index);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CModelLights::SetLightDiffuseColor(int light_index, const Image::RGBA &rgba)
{
	Dbg_Assert(light_index < CLightManager::MAX_LIGHTS);

	//m_light_diffuse_rgba[light_index] = rgba;

	bool success = plat_set_light_diffuse_color(light_index, rgba);

	// need to call this whenever the color/brightness changes
	plat_update_brightness();

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA			CModelLights::GetLightDiffuseColor(int light_index)
{
	Dbg_Assert(light_index < CLightManager::MAX_LIGHTS);

	return plat_get_light_diffuse_color(light_index);
}



/******************************************************************/
/*                                                                */
/* The 'add_scene_light' parameter is used optionally to		  */
/* determine whether a nearby Scene Light should be added to the  */
/* lighting calc (based on the 'pos' position).					  */
/*                                                                */
/******************************************************************/
void				CModelLights::UpdateEngine( Mth::Vector & pos, bool add_scene_light )
{
	plat_update_engine( pos, add_scene_light );
}







/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CModelLights::EnableAmbientLight(bool enable)
{
	plat_enable_ambient_light(enable);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CModelLights::EnableDiffuseLight(int light_index, bool enable)
{
	plat_enable_diffuse_light(light_index, enable);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CModelLights::IsAmbientLightEnabled() const
{
	return plat_is_ambient_light_enabled();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CModelLights::IsDiffuseLightEnabled(int light_index) const
{
	return plat_is_diffuse_light_enabled(light_index);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CModelLights::SetBrightness(float brightness)
{
	m_brightness_target = brightness;

	// We've found the target brightness, so move actual brightness towards this to avoid sudden unnatural transitions.
	const float BRIGHTNESS_SPEED = 0.02f;
	if( m_brightness < m_brightness_target )
	{
		m_brightness += BRIGHTNESS_SPEED;
		if( m_brightness > m_brightness_target )
		{
			m_brightness = m_brightness_target;
		}
	}
	else if( m_brightness > m_brightness_target )
	{
		m_brightness -= BRIGHTNESS_SPEED;
		if( m_brightness < m_brightness_target )
		{
			m_brightness = m_brightness_target;
		}
	}

	m_ambient_brightness = 1.0f + ((2.0f * m_brightness) - 1.0f) * CLightManager::s_world_lights.m_ambient_light_modulation_factor;
	if (m_ambient_brightness < 0.0f)
	{
		m_ambient_brightness = 0.0f;
	}

	for (int i = 0; i < CLightManager::MAX_LIGHTS; i++)
	{
		m_diffuse_brightness[i] = 1.0f + ((2.0f * m_brightness) - 1.0f) * CLightManager::s_world_lights.m_diffuse_light_modulation_factor[i];
		if (m_diffuse_brightness[i] < 0.0f)
		{
			m_diffuse_brightness[i] = 0.0f;
		}
	}

	// need to call this whenever the color/brightness changes
	plat_update_brightness();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CModelLights::UpdateBrightness()
{
	m_ambient_brightness = 1.0f + ((2.0f * m_brightness) - 1.0f) * CLightManager::s_world_lights.m_ambient_light_modulation_factor;
	if (m_ambient_brightness < 0.0f)
	{
		m_ambient_brightness = 0.0f;
	}

	for (int i = 0; i < CLightManager::MAX_LIGHTS; i++)
	{
		m_diffuse_brightness[i] = 1.0f + ((2.0f * m_brightness) - 1.0f) * CLightManager::s_world_lights.m_diffuse_light_modulation_factor[i];
		if (m_diffuse_brightness[i] < 0.0f)
		{
			m_diffuse_brightness[i] = 0.0f;
		}
	}

	// GJ: the normal PS2 lighting code assumes that the collision 
	// code will be run once a frame to copy over the m_ambient_base_color 
	// into the m_ambient_mod_color.  unfortunately, the cutscene objects' 
	// ref objects don't go through this code, so I had to add this function
	// to explicitly copy over the data...  there's probably a cleaner way
	// to do it, but I didn't want to break the existing code.
	
	// need to call this whenever the color/brightness changes
    plat_update_brightness();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CModelLights::SetPositionPointer(Mth::Vector *p_pos)
{
	mp_pos = p_pos;
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector *		CModelLights::GetPositionPointer()
{
	return mp_pos;
}

/////////////////////////////////////////////////////////////////////////////
// These functions are the stubs of the platform dependent part of the interface.

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CModelLights::plat_set_light_ambient_color(const Image::RGBA &rgba)
{
	printf ("STUB: CModelLights::SetLightAmbientColor\n");

	return false;
}

bool				CModelLights::plat_set_light_direction(int light_index, const Mth::Vector &direction)
{
	printf ("STUB: CModelLights::SetLightDirection\n");

	return false;
}

bool				CModelLights::plat_set_light_diffuse_color(int light_index, const Image::RGBA &rgba)
{
	printf ("STUB: CModelLights::SetLightDiffuseColor\n");

	return false;
}

Image::RGBA			CModelLights::plat_get_light_ambient_color() const
{
	printf ("STUB: CModelLights::GetLightAmbientColor\n");

	return Image::RGBA(0, 0, 0, 0);
}

const Mth::Vector &	CModelLights::plat_get_light_direction(int light_index) const
{
	printf ("STUB: CModelLights::GetLightDirection\n");

	static Mth::Vector stub(0, 0, 0, 0);

	return stub;
}

Image::RGBA			CModelLights::plat_get_light_diffuse_color(int light_index) const
{
	printf ("STUB: CModelLights::GetLightDiffuseColor\n");

	return Image::RGBA(0, 0, 0, 0);
}

void				CModelLights::plat_update_brightness()
{
	printf ("STUB: CModelLights::UpdateBrightness\n");
}

void				CModelLights::plat_enable_ambient_light(bool enable)
{
	printf ("STUB: CModelLights::EnableAmbientLight\n");
}

void				CModelLights::plat_enable_diffuse_light(int light_index, bool enable)
{
	printf ("STUB: CModelLights::EnableDiffuseLight\n");
}

bool				CModelLights::plat_is_ambient_light_enabled() const
{
	printf ("STUB: CModelLights::IsAmbientLightEnabled\n");

	return false;
}

bool				CModelLights::plat_is_diffuse_light_enabled(int light_index) const
{
	printf ("STUB: CModelLights::IsDiffuseLightEnabled\n");

	return false;
}

void CModelLights::plat_update_engine( Mth::Vector & pos, bool add_scene_light )
{
	printf ("STUB: CModelLights::plat_update_engine\n");
}

}


