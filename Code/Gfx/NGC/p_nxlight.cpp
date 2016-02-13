/////////////////////////////////////////////////////////////////////////////
// p_NxLight.cpp - Ngc platform specific interface to CModelLights

#include <core/defines.h>

#include <gfx/Ngc/p_NxLight.h>
#include <gfx/Ngc/nx/nx_init.h>
#include "sys/ngc/p_gx.h"


namespace	Nx
{


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// Functions


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNgcModelLights::CNgcModelLights()
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
CNgcModelLights::~CNgcModelLights()
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcModelLights::plat_update_brightness()
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcModelLights::plat_update_engine( Mth::Vector & pos, bool add_scene_light )
{
	if( m_flags & mUSE_MODEL_AMBIENT )
	{
		// Use the local ambient color, modulate it with the local ambient brightness.
		int difr = (int)((float)m_ambient_color.r * m_ambient_brightness);
		int difg = (int)((float)m_ambient_color.g * m_ambient_brightness);
		int difb = (int)((float)m_ambient_color.b * m_ambient_brightness);
		NxNgc::EngineGlobals.ambient_light_color.r = difr > 255 ? 255 : (u8)difr; 
		NxNgc::EngineGlobals.ambient_light_color.g = difg > 255 ? 255 : (u8)difg; 
		NxNgc::EngineGlobals.ambient_light_color.b = difb > 255 ? 255 : (u8)difb; 
	}
	else
	{
		// Use the default ambient color, but modulate it with the local ambient brightness.
		Image::RGBA	amb = CLightManager::sGetLightAmbientColor();
		int difr = (int)((float)amb.r * m_ambient_brightness);
		int difg = (int)((float)amb.g * m_ambient_brightness);
		int difb = (int)((float)amb.b * m_ambient_brightness);
		NxNgc::EngineGlobals.ambient_light_color.r = difr > 255 ? 255 : (u8)difr;  
		NxNgc::EngineGlobals.ambient_light_color.g = difg > 255 ? 255 : (u8)difg;  
		NxNgc::EngineGlobals.ambient_light_color.b = difb > 255 ? 255 : (u8)difb;  
	}

//	NxNgc::EngineGlobals.ambient_light_color.r = NxNgc::EngineGlobals.ambient_light_color.r < 128 ? NxNgc::EngineGlobals.ambient_light_color.r * 2 : 255;
//	NxNgc::EngineGlobals.ambient_light_color.g = NxNgc::EngineGlobals.ambient_light_color.g < 128 ? NxNgc::EngineGlobals.ambient_light_color.g * 2 : 255;
//	NxNgc::EngineGlobals.ambient_light_color.b = NxNgc::EngineGlobals.ambient_light_color.b < 128 ? NxNgc::EngineGlobals.ambient_light_color.b * 2 : 255;

	for( int i = 0; i < 2; ++i )
	{
		if( m_flags & (( i == 0 ) ? mUSE_MODEL_DIFFUSE_0 : mUSE_MODEL_DIFFUSE_1 ))
		{
			// Use the local directional color, modulate it with the local directional brightness.
			int difr = (int)((float)m_diffuse_color[i].r * m_diffuse_brightness[i] * 0.5f);
			int difg = (int)((float)m_diffuse_color[i].g * m_diffuse_brightness[i] * 0.5f);
			int difb = (int)((float)m_diffuse_color[i].b * m_diffuse_brightness[i] * 0.5f);
			NxNgc::EngineGlobals.diffuse_light_color[i].r = difr > 255 ? 255 : (u8)difr;
			NxNgc::EngineGlobals.diffuse_light_color[i].g = difg > 255 ? 255 : (u8)difg;
			NxNgc::EngineGlobals.diffuse_light_color[i].b = difb > 255 ? 255 : (u8)difb;

			// Use the local direction.
			m_diffuse_direction[i].Normalize();
			NxNgc::EngineGlobals.light_x[i] = m_diffuse_direction[i][X] * 200000.0f;
			NxNgc::EngineGlobals.light_y[i] = m_diffuse_direction[i][Y] * 200000.0f;
			NxNgc::EngineGlobals.light_z[i] = m_diffuse_direction[i][Z] * 200000.0f;
		}
		else
		{
			// Use the default directional color, but modulate it with the local directional brightness.
			Image::RGBA	dif = CLightManager::sGetLightDiffuseColor( i );
			int difr = (int)((float)dif.r * m_diffuse_brightness[i] * 0.5f);
			int difg = (int)((float)dif.g * m_diffuse_brightness[i] * 0.5f);
			int difb = (int)((float)dif.b * m_diffuse_brightness[i] * 0.5f);
			NxNgc::EngineGlobals.diffuse_light_color[i].r = difr > 255 ? 255 : (u8)difr;
			NxNgc::EngineGlobals.diffuse_light_color[i].g = difg > 255 ? 255 : (u8)difg;
			NxNgc::EngineGlobals.diffuse_light_color[i].b = difb > 255 ? 255 : (u8)difb;

			// Use the default direction.
			Mth::Vector dir = CLightManager::sGetLightDirection( i );
			dir.Normalize();
			NxNgc::EngineGlobals.light_x[i] = dir[X] * 200000.0f;
			NxNgc::EngineGlobals.light_y[i] = dir[Y] * 200000.0f;
			NxNgc::EngineGlobals.light_z[i] = dir[Z] * 200000.0f;
		}
//		NxNgc::EngineGlobals.diffuse_light_color[i].r = NxNgc::EngineGlobals.diffuse_light_color[i].r < 128 ? NxNgc::EngineGlobals.diffuse_light_color[i].r * 2 : 255;
//		NxNgc::EngineGlobals.diffuse_light_color[i].g = NxNgc::EngineGlobals.diffuse_light_color[i].g < 128 ? NxNgc::EngineGlobals.diffuse_light_color[i].g * 2 : 255;
//		NxNgc::EngineGlobals.diffuse_light_color[i].b = NxNgc::EngineGlobals.diffuse_light_color[i].b < 128 ? NxNgc::EngineGlobals.diffuse_light_color[i].b * 2 : 255;
	}

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
			NxNgc::EngineGlobals.light_x[2] = -light_pos[X] * 200000.0f;
			NxNgc::EngineGlobals.light_y[2] = -light_pos[Y] * 200000.0f;
			NxNgc::EngineGlobals.light_z[2] = -light_pos[Z] * 200000.0f;

			// ...and the color.
			ratio = sqrtf( 1.0f - ratio ) * p_scene_light->GetLightIntensity();
			int difr = (int)((float)p_scene_light->GetLightColor().r * ratio * 0.5f);
			int difg = (int)((float)p_scene_light->GetLightColor().g * ratio * 0.5f);
			int difb = (int)((float)p_scene_light->GetLightColor().b * ratio * 0.5f);
			NxNgc::EngineGlobals.diffuse_light_color[2].r	= difr > 255 ? 255 : (u8)difr;
			NxNgc::EngineGlobals.diffuse_light_color[2].g	= difg > 255 ? 255 : (u8)difg;
			NxNgc::EngineGlobals.diffuse_light_color[2].b	= difb > 255 ? 255 : (u8)difb;
		}
		else
		{
			// Disbale this light by setting zero color.
			NxNgc::EngineGlobals.diffuse_light_color[2].r = 0;
			NxNgc::EngineGlobals.diffuse_light_color[2].g = 0;
			NxNgc::EngineGlobals.diffuse_light_color[2].b = 0;
		}
	}

//	// Set lighting.
//	GXLightObj light_obj[2];
//
//	GX::SetChanAmbColor( GX_COLOR0A0, NxNgc::EngineGlobals.ambient_light_color );
//	GX::SetChanAmbColor( GX_COLOR1A1, NxNgc::EngineGlobals.ambient_light_color );
//
//	GXInitLightColor( &light_obj[0], NxNgc::EngineGlobals.diffuse_light_color[0] );
//	GXInitLightPos( &light_obj[0], NxNgc::EngineGlobals.light_x[0], NxNgc::EngineGlobals.light_y[0], NxNgc::EngineGlobals.light_z[0] );
//	GXLoadLightObjImm( &light_obj[0], GX_LIGHT0 );
//
//	GXInitLightColor( &light_obj[1], NxNgc::EngineGlobals.diffuse_light_color[1] );
//	GXInitLightPos( &light_obj[1], NxNgc::EngineGlobals.light_x[1], NxNgc::EngineGlobals.light_y[1], NxNgc::EngineGlobals.light_z[1] );
//	GXLoadLightObjImm( &light_obj[1], GX_LIGHT1 );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcModelLights::plat_set_light_ambient_color( const Image::RGBA &rgba )
{
	m_ambient_color = rgba;
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Image::RGBA	CNgcModelLights::plat_get_light_ambient_color() const
{
	return m_ambient_color;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcModelLights::plat_set_light_direction( int light_index, const Mth::Vector &direction )
{
	m_diffuse_direction[light_index] = direction;
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const Mth::Vector &	CNgcModelLights::plat_get_light_direction( int light_index ) const
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
bool CNgcModelLights::plat_set_light_diffuse_color( int light_index, const Image::RGBA &rgba )
{
	m_diffuse_color[light_index] = rgba;
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Image::RGBA	CNgcModelLights::plat_get_light_diffuse_color( int light_index ) const
{
	return m_diffuse_color[light_index];
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcModelLights::plat_enable_ambient_light( bool enable )
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
void CNgcModelLights::plat_enable_diffuse_light( int light_index, bool enable )
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
bool CNgcModelLights::plat_is_ambient_light_enabled() const
{
	return ( m_flags & mUSE_MODEL_AMBIENT ) > 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcModelLights::plat_is_diffuse_light_enabled( int light_index ) const
{
	return (( light_index == 0 ) ? (( m_flags & mUSE_MODEL_DIFFUSE_0 ) > 0 ) : (( m_flags & mUSE_MODEL_DIFFUSE_1 ) > 0 ));
}

} 
 

