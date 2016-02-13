#include <core/defines.h>
#include "light.h"

namespace NxPs2
{


// Statics
CLightGroup		CLightGroup::s_default_lights;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CLightGroup::CLightGroup()
{
	if (this != &s_default_lights)	   // can't initialize from itself
	{
		// Don't use new lights unless someone sets them
		m_flags = 0;
	
		// Init to default lights
		m_light_vector = s_default_lights.m_light_vector;
	
		m_diffuse_base_color = s_default_lights.m_diffuse_base_color;
		m_diffuse_mod_color = s_default_lights.m_diffuse_base_color;
	
		m_ambient_base_color = s_default_lights.m_ambient_base_color;
		m_ambient_mod_color = s_default_lights.m_ambient_base_color;
	}
	else
	{
		m_light_vector[0].Set(0,-1,0);
		m_light_vector[1].Set(0,-1,0);
		m_light_vector[2].Set(0,-1,0);
		m_diffuse_base_color[0].Set() ;
		m_diffuse_base_color[1].Set() ;
		m_diffuse_base_color[2].Set() ;
		m_diffuse_mod_color[0].Set()  ;
		m_diffuse_mod_color[1].Set()  ;
		m_diffuse_mod_color[2].Set()  ;
		m_ambient_base_color.Set() ;
		m_ambient_mod_color.Set()  ;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CLightGroup::~CLightGroup()
{
}


} // namespace NxPs2

