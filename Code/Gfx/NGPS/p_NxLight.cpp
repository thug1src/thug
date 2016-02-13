/////////////////////////////////////////////////////////////////////////////
// p_NxLight.cpp - PS2 platform specific interface to CModelLights
//
// This is PS2 SPECIFIC!!!!!!  So might get a bit messy

#include <core/defines.h>

#include <gfx/ngps/p_NxLight.h>
#include <gfx/ngps/nx/light.h>


namespace	Nx
{


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// Functions


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2ModelLights::CPs2ModelLights()
{
	mp_light_group = new NxPs2::CLightGroup;

	plat_enable_ambient_light(false);
	for (int i = 0; i < CLightManager::MAX_LIGHTS; i++)
	{
		plat_enable_diffuse_light(i, false);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2ModelLights::~CPs2ModelLights()
{
	if (mp_light_group)
	{
		delete mp_light_group;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CPs2ModelLights::plat_update_engine( Mth::Vector & pos, bool add_scene_light )
{
	// Figure Scene Lighting if required.
	if( add_scene_light )
	{
		// Copy the object position into our position.  If this pointer is set to NULL,
		// we probably won't find any Scene Lights.
		if (mp_pos)
		{
			pos = *mp_pos;
		}
		else
		{
			Dbg_MsgAssert(0, ("No object position pointer for scene lights"));
		}

		// We should check if any calcs are necessary first by seeing if this position is
		// "visible", but for now, we'll do them all.
		Nx::CSceneLight *p_scene_light = CLightManager::sGetOptimumSceneLight( pos );
		if( p_scene_light )
		{
			Dbg_Assert(mp_light_group);

			Mth::Vector light_pos = p_scene_light->GetLightPosition();

			float dist	= Mth::Distance( pos, light_pos );
			float ratio	= dist * p_scene_light->GetLightReciprocalRadius();

			light_pos = - ( pos - light_pos ).Normalize();

			// Figure the direction...
			mp_light_group->SetDirection(SCENE_LIGHT_INDEX, light_pos);

			// ...and the color.
			ratio = sqrtf( 1.0f - ratio ) * p_scene_light->GetLightIntensity();
			Mth::Vector color(ratio * p_scene_light->GetLightColor().r,
							  ratio * p_scene_light->GetLightColor().g,
							  ratio * p_scene_light->GetLightColor().b,
							  0);

			mp_light_group->SetBaseDiffuseColor(SCENE_LIGHT_INDEX, color);

			plat_enable_diffuse_light(SCENE_LIGHT_INDEX, true);
		}
		else
		{
#if 0
			// Disable this light by setting zero color.
			Mth::Vector color(0, 0, 0, 0);

			mp_light_group->SetBaseDiffuseColor(SCENE_LIGHT_INDEX, color);
#endif
			plat_enable_diffuse_light(SCENE_LIGHT_INDEX, false);
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CPs2ModelLights::plat_update_brightness()
{
	mp_light_group->SetAmbientBrightness(m_ambient_brightness);

	for (int i = 0; i < CLightManager::MAX_LIGHTS; i++)
	{
		mp_light_group->SetDiffuseBrightness(i, m_diffuse_brightness[i]);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CPs2ModelLights::plat_set_light_ambient_color(const Image::RGBA &rgba)
{
	Mth::Vector color(((float) rgba.r),
					  ((float) rgba.g),
					  ((float) rgba.b),
					  0);

	mp_light_group->SetBaseAmbientColor(color);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA		CPs2ModelLights::plat_get_light_ambient_color() const
{
	Mth::Vector color = mp_light_group->GetBaseAmbientColor();

	Image::RGBA rgba(color[0], color[1], color[2], 0);
	
	return rgba;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CPs2ModelLights::plat_set_light_direction(int light_index, const Mth::Vector &direction)
{
	mp_light_group->SetDirection(light_index, direction);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Vector &	CPs2ModelLights::plat_get_light_direction(int light_index) const
{
	return mp_light_group->GetDirection(light_index);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CPs2ModelLights::plat_set_light_diffuse_color(int light_index, const Image::RGBA &rgba)
{
	Mth::Vector color(((float) rgba.r),
					  ((float) rgba.g),
					  ((float) rgba.b),
					  0);

	mp_light_group->SetBaseDiffuseColor(light_index, color);
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA		CPs2ModelLights::plat_get_light_diffuse_color(int light_index) const
{
	Mth::Vector color = mp_light_group->GetBaseDiffuseColor(light_index);

	Image::RGBA rgba(color[0], color[1], color[2], 0);
	
	return rgba;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CPs2ModelLights::plat_enable_ambient_light(bool enable)
{
	mp_light_group->EnableAmbientLight(enable);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CPs2ModelLights::plat_enable_diffuse_light(int light_index, bool enable)
{
	mp_light_group->EnableDiffuseLight(light_index, enable);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CPs2ModelLights::plat_is_ambient_light_enabled() const
{
	return mp_light_group->IsAmbientLightEnabled();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CPs2ModelLights::plat_is_diffuse_light_enabled(int light_index) const
{
	return mp_light_group->IsDiffuseLightEnabled(light_index);
}

} 
 
