/////////////////////////////////////////////////////////////////////////////
// p_NxLight.cpp - Xbox platform specific interface to CModelLights

#include <core/defines.h>

#include <gfx/xbox/p_NxLight.h>
#include <gfx/xbox/nx/nx_init.h>


namespace	Nx
{


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// Functions


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxModelLights::CXboxModelLights()
{
	m_flags = 0;

	EnableAmbientLight( false );
	for( int i = 0; i < CLightManager::MAX_LIGHTS; ++i )
	{
		EnableDiffuseLight( i, false );
	}
	
	// Set valid default direction.
	m_diffuse_direction[0].Set( 0.0f, 1.0f, 0.0f );
	m_diffuse_direction[1].Set( 0.0f, 1.0f, 0.0f );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxModelLights::~CXboxModelLights()
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxModelLights::plat_update_brightness()
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxModelLights::plat_update_engine( Mth::Vector & pos, bool add_scene_light )
{
	if( m_flags & mUSE_MODEL_AMBIENT )
	{
		// Use the local ambient color, modulate it with the local ambient brightness.
		NxXbox::EngineGlobals.ambient_light_color[0] = m_ambient_color.r * ( 1.0f / 128.0f ) * m_ambient_brightness;
		NxXbox::EngineGlobals.ambient_light_color[1] = m_ambient_color.g * ( 1.0f / 128.0f ) * m_ambient_brightness;
		NxXbox::EngineGlobals.ambient_light_color[2] = m_ambient_color.b * ( 1.0f / 128.0f ) * m_ambient_brightness;
	}
	else
	{
		// Use the default ambient color, but modulate it with the local ambient brightness.
		Image::RGBA	amb = CLightManager::sGetLightAmbientColor();
		NxXbox::EngineGlobals.ambient_light_color[0] = amb.r * ( 1.0f / 128.0f ) * m_ambient_brightness;
		NxXbox::EngineGlobals.ambient_light_color[1] = amb.g * ( 1.0f / 128.0f ) * m_ambient_brightness;
		NxXbox::EngineGlobals.ambient_light_color[2] = amb.b * ( 1.0f / 128.0f ) * m_ambient_brightness;
	}

	for( int i = 0; i < 2; ++i )
	{
		if( m_flags & (( i == 0 ) ? mUSE_MODEL_DIFFUSE_0 : mUSE_MODEL_DIFFUSE_1 ))
		{
			// Use the local directional color, modulate it with the local directional brightness.
			NxXbox::EngineGlobals.directional_light_color[( i * 8 ) + 4] = m_diffuse_color[i].r * ( 1.0f / 128.0f ) * m_diffuse_brightness[i];
			NxXbox::EngineGlobals.directional_light_color[( i * 8 ) + 5] = m_diffuse_color[i].g * ( 1.0f / 128.0f ) * m_diffuse_brightness[i];
			NxXbox::EngineGlobals.directional_light_color[( i * 8 ) + 6] = m_diffuse_color[i].b * ( 1.0f / 128.0f ) * m_diffuse_brightness[i];

			// Use the local direction.
			NxXbox::EngineGlobals.directional_light_color[( i * 8 ) + 0] = -m_diffuse_direction[i][X];
			NxXbox::EngineGlobals.directional_light_color[( i * 8 ) + 1] = -m_diffuse_direction[i][Y];
			NxXbox::EngineGlobals.directional_light_color[( i * 8 ) + 2] = -m_diffuse_direction[i][Z];
		}
		else
		{
			// Use the default directional color, but modulate it with the local directional brightness.
			Image::RGBA	dif = CLightManager::sGetLightDiffuseColor( i );
			NxXbox::EngineGlobals.directional_light_color[( i * 8 ) + 4] = dif.r * ( 1.0f / 128.0f ) * m_diffuse_brightness[i];
			NxXbox::EngineGlobals.directional_light_color[( i * 8 ) + 5] = dif.g * ( 1.0f / 128.0f ) * m_diffuse_brightness[i];
			NxXbox::EngineGlobals.directional_light_color[( i * 8 ) + 6] = dif.b * ( 1.0f / 128.0f ) * m_diffuse_brightness[i];

			// Use the default direction.
			Mth::Vector dir = CLightManager::sGetLightDirection( i );
			NxXbox::EngineGlobals.directional_light_color[( i * 8 ) + 0] = -dir[X];
			NxXbox::EngineGlobals.directional_light_color[( i * 8 ) + 1] = -dir[Y];
			NxXbox::EngineGlobals.directional_light_color[( i * 8 ) + 2] = -dir[Z];
		}
	}

	// Figure Scene Lighting if required.
	if( add_scene_light )
	{
		Nx::CSceneLight *p_scene_light = CLightManager::sGetOptimumSceneLight( pos );
		if( p_scene_light )
		{
			Mth::Vector light_pos = p_scene_light->GetLightPosition();

			float dist	= Mth::Distance( pos, light_pos );
			float ratio	= dist * p_scene_light->GetLightReciprocalRadius();

			light_pos = ( pos - light_pos ).Normalize();

			// Figure the direction...
			NxXbox::EngineGlobals.directional_light_color[( 2 * 8 ) + 0] = light_pos[X];
			NxXbox::EngineGlobals.directional_light_color[( 2 * 8 ) + 1] = light_pos[Y];
			NxXbox::EngineGlobals.directional_light_color[( 2 * 8 ) + 2] = light_pos[Z];

			// ...and the color.
			ratio															= sqrtf( 1.0f - ratio ) * ( 1.0f / 255.0f ) * p_scene_light->GetLightIntensity();
			NxXbox::EngineGlobals.directional_light_color[( 2 * 8 ) + 4]	= ratio * p_scene_light->GetLightColor().r;
			NxXbox::EngineGlobals.directional_light_color[( 2 * 8 ) + 5]	= ratio * p_scene_light->GetLightColor().g;
			NxXbox::EngineGlobals.directional_light_color[( 2 * 8 ) + 6]	= ratio * p_scene_light->GetLightColor().b;
		}
		else
		{
			// Disbale this light by setting zero color.
			NxXbox::EngineGlobals.directional_light_color[( 2 * 8 ) + 4] = 0.0f;
			NxXbox::EngineGlobals.directional_light_color[( 2 * 8 ) + 5] = 0.0f;
			NxXbox::EngineGlobals.directional_light_color[( 2 * 8 ) + 6] = 0.0f;
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxModelLights::plat_set_light_ambient_color( const Image::RGBA &rgba )
{
	m_ambient_color = rgba;
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Image::RGBA	CXboxModelLights::plat_get_light_ambient_color() const
{
	return m_ambient_color;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxModelLights::plat_set_light_direction( int light_index, const Mth::Vector &direction )
{
	m_diffuse_direction[light_index] = direction;
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const Mth::Vector &	CXboxModelLights::plat_get_light_direction( int light_index ) const
{
	if( plat_is_diffuse_light_enabled( light_index ))
		return m_diffuse_direction[light_index];
	else
		return Nx::CLightManager::sGetLightDirection( light_index );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxModelLights::plat_set_light_diffuse_color( int light_index, const Image::RGBA &rgba )
{
	m_diffuse_color[light_index] = rgba;
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Image::RGBA	CXboxModelLights::plat_get_light_diffuse_color( int light_index ) const
{
	return m_diffuse_color[light_index];
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxModelLights::plat_enable_ambient_light( bool enable )
{
	if( enable )
		m_flags |= mUSE_MODEL_AMBIENT;
	else
		m_flags &= ~mUSE_MODEL_AMBIENT;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxModelLights::plat_enable_diffuse_light( int light_index, bool enable )
{
	if( enable )
		m_flags |= ( light_index == 0 ) ? mUSE_MODEL_DIFFUSE_0 : mUSE_MODEL_DIFFUSE_1;
	else
		m_flags &= ~(( light_index == 0 ) ? mUSE_MODEL_DIFFUSE_0 : mUSE_MODEL_DIFFUSE_1 );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxModelLights::plat_is_ambient_light_enabled() const
{
	return ( m_flags & mUSE_MODEL_AMBIENT ) > 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxModelLights::plat_is_diffuse_light_enabled( int light_index ) const
{
	return (( light_index == 0 ) ? (( m_flags & mUSE_MODEL_DIFFUSE_0 ) > 0 ) : (( m_flags & mUSE_MODEL_DIFFUSE_1 ) > 0 ));
}

} 
 
