/////////////////////////////////////////////////////////////////////////////
// p_NxLightMan.cpp - PS2 platform specific interface to CLightManager
//
// This is PS2 SPECIFIC!!!!!!  So might get a bit messy

#include <core/defines.h>

#include <gfx/ngps/p_NxLight.h>
#include <gfx/ngps/p_NxLightMan.h>
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
void			CLightManager::s_plat_update_engine( void )
{
	CPs2LightManager::sUpdateEngine();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CLightManager::s_plat_update_lights()
{
	s_plat_set_light_ambient_color();
	for (int i = 0; i < MAX_LIGHTS; i++)
	{
		s_plat_set_light_direction(i);
		s_plat_set_light_diffuse_color(i);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CLightManager::s_plat_update_colors()
{
	s_plat_set_light_ambient_color();
	for (int i = 0; i < MAX_LIGHTS; i++)
	{
		s_plat_set_light_diffuse_color(i);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CLightManager::s_plat_set_light_ambient_color()
{
	Mth::Vector color(((float) s_world_lights.m_light_ambient_rgba.r),
					  ((float) s_world_lights.m_light_ambient_rgba.g),
					  ((float) s_world_lights.m_light_ambient_rgba.b),
					  0);

	NxPs2::CLightGroup::sSetDefaultAmbientColor(color);

	//color *= s_ambient_brightness;
	//NxPs2::CLightGroup::sSetDefaultModAmbientColor(color);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA		CLightManager::s_plat_get_light_ambient_color()
{
	Mth::Vector color(NxPs2::CLightGroup::sGetDefaultAmbientColor());

	Image::RGBA rgb;

	rgb.r = (uint8) color[X];
	rgb.g = (uint8) color[Y];
	rgb.b = (uint8) color[Z];
	rgb.a = 0x80;

	return rgb;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CLightManager::s_plat_set_light_direction(int light_index)
{
	NxPs2::CLightGroup::sSetDefaultDirection(light_index, s_world_lights.m_light_direction[light_index]);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Vector & CLightManager::s_plat_get_light_direction(int light_index)
{
	return NxPs2::CLightGroup::sGetDefaultDirection(light_index);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CLightManager::s_plat_set_light_diffuse_color(int light_index)
{
	Mth::Vector color(((float) s_world_lights.m_light_diffuse_rgba[light_index].r),
					  ((float) s_world_lights.m_light_diffuse_rgba[light_index].g),
					  ((float) s_world_lights.m_light_diffuse_rgba[light_index].b),
					  0);

	NxPs2::CLightGroup::sSetDefaultDiffuseColor(light_index, color);
	
	//color *= s_diffuse_brightness[light_index];
	//NxPs2::CLightGroup::sSetDefaultModDiffuseColor(light_index, color);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA		CLightManager::s_plat_get_light_diffuse_color(int light_index)
{
	Mth::Vector color(NxPs2::CLightGroup::sGetDefaultDiffuseColor(light_index));

	Image::RGBA rgb;

	rgb.r = (uint8) color[X];
	rgb.g = (uint8) color[Y];
	rgb.b = (uint8) color[Z];
	rgb.a = 0x80;

	return rgb;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CModelLights *	CLightManager::s_plat_create_model_lights()
{
	CModelLights *p_model_lights = new CPs2ModelLights;

	if (p_model_lights)
	{
		CPs2LightManager::sAddToModelLightsList(p_model_lights);
	}

	return p_model_lights;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CLightManager::s_plat_free_model_lights(CModelLights *p_model_lights)
{
	Dbg_Assert(p_model_lights);

	CPs2LightManager::sRemoveFromModelLightsList(p_model_lights);

	delete p_model_lights;

	return true;
}

///////////////////////////////////////////////////////////////////////////////////
// Nx::CPs2LightManager

// Model light list
Lst::Head< CModelLights > CPs2LightManager::s_model_lights_list;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CPs2LightManager::sUpdateEngine( void )
{
	s_update_model_lights();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CPs2LightManager::sAddToModelLightsList(CModelLights *p_model_lights)
{
	Dbg_Assert(p_model_lights);

	Lst::Node< CModelLights > *node = new Lst::Node< CModelLights > (p_model_lights);
	if (node)
	{
		s_model_lights_list.AddToTail(node);
		return true;
	}
	else
	{
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CPs2LightManager::sRemoveFromModelLightsList(CModelLights *p_model_lights)
{
	Dbg_Assert(p_model_lights);

	Lst::Node< CModelLights > *obj_node;

	for(obj_node = s_model_lights_list.GetNext(); obj_node; obj_node = obj_node->GetNext())
	{
		if (obj_node->GetData() == p_model_lights)
		{
			delete obj_node;
			return true;
		}
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CPs2LightManager::s_update_model_lights()
{
	Mth::Vector dummy_pos(0, 0, 0, 1);		// Function requires this to be passed in, even though not used
	Lst::Node< CModelLights > *obj_node;

	for(obj_node = s_model_lights_list.GetNext(); obj_node; obj_node = obj_node->GetNext())
	{
		CModelLights *p_model_lights = obj_node->GetData();
		
		Dbg_Assert(p_model_lights);

		p_model_lights->UpdateEngine(dummy_pos, true);
	}
}

} 
 
