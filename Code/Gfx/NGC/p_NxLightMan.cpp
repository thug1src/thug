/////////////////////////////////////////////////////////////////////////////
// p_NxLightMan.cpp - Ngc platform specific interface to CLightManager

#include <core/defines.h>
#include "gfx/NxLightMan.h"
#include "gfx/Ngc/p_NxLight.h"
#include "gfx/Ngc/nx/nx_init.h"
#include "sys/ngc/p_gx.h"

namespace Nx
{


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// Functions


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CLightManager::s_plat_update_engine( void )
{
	int difr = (int)((float)s_world_lights.m_light_ambient_rgba.r * s_ambient_brightness);
	int difg = (int)((float)s_world_lights.m_light_ambient_rgba.g * s_ambient_brightness);
	int difb = (int)((float)s_world_lights.m_light_ambient_rgba.b * s_ambient_brightness);
	NxNgc::EngineGlobals.ambient_light_color.r = difr > 255 ? 255 : (u8)difr;  
	NxNgc::EngineGlobals.ambient_light_color.g = difg > 255 ? 255 : (u8)difg;  
	NxNgc::EngineGlobals.ambient_light_color.b = difb > 255 ? 255 : (u8)difb;  

//	NxNgc::EngineGlobals.ambient_light_color.r = NxNgc::EngineGlobals.ambient_light_color.r < 128 ? NxNgc::EngineGlobals.ambient_light_color.r * 2 : 255;
//	NxNgc::EngineGlobals.ambient_light_color.g = NxNgc::EngineGlobals.ambient_light_color.g < 128 ? NxNgc::EngineGlobals.ambient_light_color.g * 2 : 255;
//	NxNgc::EngineGlobals.ambient_light_color.b = NxNgc::EngineGlobals.ambient_light_color.b < 128 ? NxNgc::EngineGlobals.ambient_light_color.b * 2 : 255;

	for( int i = 0; i < 2; ++i )
	{
		Image::RGBA	dif = CLightManager::sGetLightDiffuseColor( i );
		int difr = (int)((float)dif.r * s_diffuse_brightness[i]);
		int difg = (int)((float)dif.g * s_diffuse_brightness[i]);
		int difb = (int)((float)dif.b * s_diffuse_brightness[i]);
		NxNgc::EngineGlobals.diffuse_light_color[i].r = difr > 255 ? 255 : (u8)difr;
		NxNgc::EngineGlobals.diffuse_light_color[i].g = difg > 255 ? 255 : (u8)difg;
		NxNgc::EngineGlobals.diffuse_light_color[i].b = difb > 255 ? 255 : (u8)difb;

//		NxNgc::EngineGlobals.diffuse_light_color[i].r = NxNgc::EngineGlobals.diffuse_light_color[i].r < 128 ? NxNgc::EngineGlobals.diffuse_light_color[i].r * 2 : 255;
//		NxNgc::EngineGlobals.diffuse_light_color[i].g = NxNgc::EngineGlobals.diffuse_light_color[i].g < 128 ? NxNgc::EngineGlobals.diffuse_light_color[i].g * 2 : 255;
//		NxNgc::EngineGlobals.diffuse_light_color[i].b = NxNgc::EngineGlobals.diffuse_light_color[i].b < 128 ? NxNgc::EngineGlobals.diffuse_light_color[i].b * 2 : 255;

		Mth::Vector dir = CLightManager::sGetLightDirection( i );
		dir.Normalize();
		NxNgc::EngineGlobals.light_x[i] = dir[X] * 200000.0f;
		NxNgc::EngineGlobals.light_y[i] = dir[Y] * 200000.0f;
		NxNgc::EngineGlobals.light_z[i] = dir[Z] * 200000.0f;
	}
	// Set lighting.
//	GXLightObj light_obj[2];
//
////	GX::SetChanAmbColor( GX_COLOR0A0, NxNgc::EngineGlobals.ambient_light_color );
//
////	GXInitLightColor( &light_obj[0], NxNgc::EngineGlobals.diffuse_light_color[0] );
////	GXInitLightPos( &light_obj[0], NxNgc::EngineGlobals.light_x[0], NxNgc::EngineGlobals.light_y[0], NxNgc::EngineGlobals.light_z[0] );
////	GXLoadLightObjImm( &light_obj[0], GX_LIGHT0 );
////	GXInitSpecularDir( &light_obj[2], -NxNgc::EngineGlobals.light_x[0] / 200000.0f, -NxNgc::EngineGlobals.light_y[0] / 200000.0f, -NxNgc::EngineGlobals.light_z[0] / 200000.0f );
////	GXInitLightShininess( &light_obj[2], 5.0f );
////
////	GXInitLightColor( &light_obj[1], NxNgc::EngineGlobals.diffuse_light_color[1] );
////	GXInitLightPos( &light_obj[1], NxNgc::EngineGlobals.light_x[1], NxNgc::EngineGlobals.light_y[1], NxNgc::EngineGlobals.light_z[1] );
////	GXLoadLightObjImm( &light_obj[1], GX_LIGHT1 );
////	GXInitSpecularDir( &light_obj[2], -NxNgc::EngineGlobals.light_x[1] / 200000.0f, -NxNgc::EngineGlobals.light_y[1] / 200000.0f, -NxNgc::EngineGlobals.light_z[1] / 200000.0f );
////	GXInitLightShininess( &light_obj[2], 5.0f );
//
//    Vec  ldir;
//    f32  theta, phi;
//
//    // Light position
//    theta = (f32)90 * Mth::PI / 180.0F;
//    phi   = (f32)0   * Mth::PI / 180.0F;
//    ldir.x = - 1.0F * cosf(phi) * sinf(theta);
//    ldir.y = - 1.0F * sinf(phi);
//    ldir.z = - 1.0F * cosf(phi) * cosf(theta);
//
//    // Convert light position into view space
//    Vec  dir0;
//    Vec  dir1;
//    MTXMultVecSR(NxNgc::EngineGlobals.current_uploaded, &ldir, &dir0);
//    MTXMultVecSR(NxNgc::EngineGlobals.current_uploaded, &ldir, &dir1);
//
//////	GXInitSpecularDir( &light_obj[0], NxNgc::EngineGlobals.light_x[0] / 200000.0f, NxNgc::EngineGlobals.light_y[0] / 200000.0f, NxNgc::EngineGlobals.light_z[0] / 200000.0f );
////	GXInitSpecularDirv( &light_obj[0], &dir0 );
////	GXInitLightShininess( &light_obj[0], 16.0f );
////	GXInitLightColor( &light_obj[0], (GXColor){0,0,0,0}/*NxNgc::EngineGlobals.diffuse_light_color[0]*/ );
////	GXLoadLightObjImm( &light_obj[0], GX_LIGHT0 );
//
////	GXInitSpecularDir( &light_obj[1], NxNgc::EngineGlobals.light_x[1] / 200000.0f, NxNgc::EngineGlobals.light_y[1] / 200000.0f, NxNgc::EngineGlobals.light_z[1] / 200000.0f );
//	GXInitSpecularDirv( &light_obj[0], &dir0 );
//	GXInitLightShininess( &light_obj[0], 5.0f );
//	GXInitLightColor( &light_obj[0], NxNgc::EngineGlobals.diffuse_light_color[1] );
//	GXLoadLightObjImm( &light_obj[0], GX_LIGHT0 );
//
//	GXSetChanAmbColor( GX_COLOR0A0, (GXColor){0,0,0,255} );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CLightManager::s_plat_update_lights( void )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CLightManager::s_plat_set_light_ambient_color()
{
	int difr = (int)((float)s_world_lights.m_light_ambient_rgba.r * s_ambient_brightness);
	int difg = (int)((float)s_world_lights.m_light_ambient_rgba.g * s_ambient_brightness);
	int difb = (int)((float)s_world_lights.m_light_ambient_rgba.b * s_ambient_brightness);
	NxNgc::EngineGlobals.ambient_light_color.r = difr > 255 ? 255 : (u8)difr;
	NxNgc::EngineGlobals.ambient_light_color.g = difg > 255 ? 255 : (u8)difg;
	NxNgc::EngineGlobals.ambient_light_color.b = difb > 255 ? 255 : (u8)difb;
	
	NxNgc::EngineGlobals.ambient_light_color.r = NxNgc::EngineGlobals.ambient_light_color.r < 128 ? NxNgc::EngineGlobals.ambient_light_color.r * 2 : 255;
	NxNgc::EngineGlobals.ambient_light_color.g = NxNgc::EngineGlobals.ambient_light_color.g < 128 ? NxNgc::EngineGlobals.ambient_light_color.g * 2 : 255;
	NxNgc::EngineGlobals.ambient_light_color.b = NxNgc::EngineGlobals.ambient_light_color.b < 128 ? NxNgc::EngineGlobals.ambient_light_color.b * 2 : 255;
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Image::RGBA	CLightManager::s_plat_get_light_ambient_color()
{
	return s_world_lights.m_light_ambient_rgba;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CLightManager::s_plat_set_light_direction( int light_index )
{
	NxNgc::EngineGlobals.light_x[light_index] = -s_world_lights.m_light_direction[light_index][X];
	NxNgc::EngineGlobals.light_y[light_index] = -s_world_lights.m_light_direction[light_index][Y];
	NxNgc::EngineGlobals.light_z[light_index] = -s_world_lights.m_light_direction[light_index][Z];

	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const Mth::Vector & CLightManager::s_plat_get_light_direction( int light_index )
{
	static Mth::Vector dir;
	dir.Set( s_world_lights.m_light_direction[light_index][X], s_world_lights.m_light_direction[light_index][Y], s_world_lights.m_light_direction[light_index][Z] );
	return dir;
}




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CLightManager::s_plat_set_light_diffuse_color( int light_index )
{
	NxNgc::EngineGlobals.diffuse_light_color[light_index].r = (u8)((float)s_world_lights.m_light_diffuse_rgba[light_index].r * s_diffuse_brightness[light_index]);
	NxNgc::EngineGlobals.diffuse_light_color[light_index].g = (u8)((float)s_world_lights.m_light_diffuse_rgba[light_index].g * s_diffuse_brightness[light_index]);
	NxNgc::EngineGlobals.diffuse_light_color[light_index].b = (u8)((float)s_world_lights.m_light_diffuse_rgba[light_index].b * s_diffuse_brightness[light_index]);
	
	NxNgc::EngineGlobals.diffuse_light_color[light_index].r = NxNgc::EngineGlobals.diffuse_light_color[light_index].r < 128 ? NxNgc::EngineGlobals.diffuse_light_color[light_index].r * 2 : 255;
	NxNgc::EngineGlobals.diffuse_light_color[light_index].g = NxNgc::EngineGlobals.diffuse_light_color[light_index].g < 128 ? NxNgc::EngineGlobals.diffuse_light_color[light_index].g * 2 : 255;
	NxNgc::EngineGlobals.diffuse_light_color[light_index].b = NxNgc::EngineGlobals.diffuse_light_color[light_index].b < 128 ? NxNgc::EngineGlobals.diffuse_light_color[light_index].b * 2 : 255;

	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Image::RGBA	CLightManager::s_plat_get_light_diffuse_color( int light_index )
{
	return s_world_lights.m_light_diffuse_rgba[light_index];
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CLightManager::s_plat_update_colors( void )
{
	s_plat_set_light_ambient_color();
	for( int i = 0; i < MAX_LIGHTS; ++i )
	{
		s_plat_set_light_diffuse_color( i );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CModelLights *CLightManager::s_plat_create_model_lights()
{
	return new CNgcModelLights;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CLightManager::s_plat_free_model_lights( CModelLights *p_model_lights )
{
	Dbg_Assert( p_model_lights );

	delete p_model_lights;

	return true;
}



} 
 

