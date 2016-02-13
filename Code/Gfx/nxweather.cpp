///////////////////////////////////////////////////////////////////////////////
// NxWeather.cpp

#include <core/defines.h>
#include <core/singleton.h>
#include <core/math.h>
#include <gfx/NxWeather.h>
//#include <gel/scripting/struct.h>
//#include <gel/scripting/script.h>
//#include <sk/modules/frontend/frontend.h>
//#include <gel/components/suspendcomponent.h>
//#include <gfx/nx.h>
//#include <gfx/debuggfx.h>
//#include <sys/replay/replay.h>

#define next_random() ((((float)rand() / RAND_MAX ) * 2.0f ) - 1.0f)

//NsVector pright;
//NsVector pup;
//NsVector pat;

namespace	Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CWeather::plat_update_grid( void )
{
	printf ("STUB: plat_update_grid\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CWeather::plat_process( float delta_time )
{
	printf ("STUB: plat_process\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CWeather::plat_render( void )
{
	printf ("STUB: plat_render\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWeather::plat_set_rain_blend_mode( uint32 blendmode_checksum, int fix )
{
	printf ("STUB: plat_set_rain_blend_mode\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWeather::plat_set_splash_blend_mode( uint32 blendmode_checksum, int fix )
{
	printf ("STUB: plat_set_splash_blend_mode\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWeather::plat_set_snow_blend_mode( uint32 blendmode_checksum, int fix )
{
	printf ("STUB: plat_set_snow_blend_mode\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CWeather::CWeather()
{
	m_rain_height = 2000.0f;
	m_rain_frames = 40;
	m_rain_length = 100.0f;
	m_rain_drops_per_frame = 0.0f;
	m_rain_top_color.r = 0x00;
	m_rain_top_color.g = 0x00;
	m_rain_top_color.b = 0x00;
	m_rain_top_color.a = 0x00;
	m_rain_bottom_color.r = 0xff;
	m_rain_bottom_color.g = 0xff;
	m_rain_bottom_color.b = 0xff;
	m_rain_bottom_color.a = 0xff;

	m_splash_per_frame = 0.0f;
	m_splash_life = 8;
	m_splash_size = 16.0f;
	m_splash_color.r = 0x80;
	m_splash_color.g = 0x80;
	m_splash_color.b = 0x80;
	m_splash_color.a = 0x80;

	m_snow_height = 500.0f;
	m_snow_frames = 254;
	m_snow_size = 4.0f;
	m_snow_flakes_per_frame = 0.0f;
	m_snow_color.r = 0x80;
	m_snow_color.g = 0x80;
	m_snow_color.b = 0x80;
	m_snow_color.a = 0x80;

	m_raining = true;

	m_system_active = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CWeather::~CWeather()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWeather::UpdateGrid( void )
{
	plat_update_grid();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWeather::Process( float delta_time )
{
	plat_process( delta_time );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CWeather::Render( void )
{
	plat_render();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetRainHeight | Sets the height of the rain above the ground.
// @parm height | The height of the rain in inches - defaults to 2000 inches.
bool CWeather::ScriptWeatherSetRainHeight( Script::CStruct* pParams, Script::CScript* pScript )
{
	m_rain_height = 2000.0f;		// Default
	pParams->GetFloat( NONAME, &m_rain_height );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetRainFrames | Sets the number of frames it takes a rain drop to fall.
// @parm frames | Number of frames for the rain to fall from RainHeight to the ground. Defaults to 40 frames.
bool CWeather::ScriptWeatherSetRainFrames( Script::CStruct* pParams, Script::CScript* pScript )
{
	m_rain_frames = 40;		// Default
	pParams->GetInteger( NONAME, &m_rain_frames );
	if ( m_rain_frames < 0 ) m_rain_frames = 0;
	if ( m_rain_frames > 254 ) m_rain_frames = 254;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetRainLength | Sets the length of the rain drops (the rain drop is a line).
// @parm length | Length of the rain drops in inches. Defaults to 100 inches.
bool CWeather::ScriptWeatherSetRainLength( Script::CStruct* pParams, Script::CScript* pScript )
{
	m_rain_length = 100.0f;		// Default
	pParams->GetFloat( NONAME, &m_rain_length );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetRainBlendMode | Sets the blendmode of the rain particles.
// @parm blendmode | The name of the blend mode. Type are: blend/add/sub/modulate/brighten &
// fixblend/fixadd/fixsub/fixmodulate/fixbrighten & diffuse (no blend at all). Defaults to fixadd.
// @parmopt int | Fixed alpha value. Defaults to 64. Range is 0-255. Only required if using fix blend modes.
bool CWeather::ScriptWeatherSetRainBlendMode( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 blendmode_checksum = 0;
	if ( !pParams->GetChecksum( NONAME, &blendmode_checksum ) )
	{
		return false;
	}
	int fix = 32;		// Default
	pParams->GetInteger( NONAME, &fix );

	plat_set_rain_blend_mode( blendmode_checksum, fix );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetRainRate | Sets the rate that the rain drops fall in drops per frame.
// @parm rate | The number of new drops of rain per frame. Defaults to 0 (off).
bool CWeather::ScriptWeatherSetRainRate( Script::CStruct* pParams, Script::CScript* pScript )
{
	m_rain_drops_per_frame = 0.0f;		// Default
	pParams->GetFloat( NONAME, &m_rain_drops_per_frame );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetRainColor | Sets the color of the rain.
// @parm top | The color of the rain at the top of the droplet. Defaults to 0x00000000
// @parm bottom | The color of the rain at the bottom of the droplet. Defaults to 0xffffffff
bool CWeather::ScriptWeatherSetRainColor( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 color;

	color = 0x00000000;
	pParams->GetInteger( "top", (int*)&color ); 

	m_rain_top_color.r = (uint8)((color)&0xff);
	m_rain_top_color.g = (uint8)((color>>8)&0xff);
	m_rain_top_color.b = (uint8)((color>>16)&0xff);
	m_rain_top_color.a = (uint8)((color>>24)&0xff); 

	color = 0xffffffff;
	pParams->GetInteger( "bottom", (int*)&color ); 

	m_rain_bottom_color.r = (uint8)((color)&0xff);
	m_rain_bottom_color.g = (uint8)((color>>8)&0xff);
	m_rain_bottom_color.b = (uint8)((color>>16)&0xff);
	m_rain_bottom_color.a = (uint8)((color>>24)&0xff); 

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetSplashRate | Sets the rate that the splashes appear.
// @parm rate | The number of splashes per frame. 1 is the maximum. Defaults to 0 (off).
bool CWeather::ScriptWeatherSetSplashRate( Script::CStruct* pParams, Script::CScript* pScript )
{
	m_splash_per_frame = 0.0f;		// Default
	pParams->GetFloat( NONAME, &m_splash_per_frame );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetSplashLife | Sets the number of frames a splash will stay on screen.
// @parm life | Number of frames the splash is on screen. Defaults to 8. Maximum is 32.
bool CWeather::ScriptWeatherSetSplashLife( Script::CStruct* pParams, Script::CScript* pScript )
{
	m_splash_life = 8;		// Default
	pParams->GetInteger( NONAME, &m_splash_life );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetSplashSize | Sets the size of the splashes on-screen in inches.
// @parm size | Sets the size of a splash on screen - defaults to 16 inches.
bool CWeather::ScriptWeatherSetSplashSize( Script::CStruct* pParams, Script::CScript* pScript )
{
	m_splash_size = 16.0f;		// Default
	pParams->GetFloat( NONAME, &m_splash_size );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetSplashColor | Sets the color of the splash.
// @parm color | The color of the splash. Defaults to 0x80808080.
bool CWeather::ScriptWeatherSetSplashColor( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 color;

	color = 0x80808080;
	pParams->GetInteger( NONAME, (int*)&color ); 

	m_splash_color.r = (uint8)((color)&0xff);
	m_splash_color.g = (uint8)((color>>8)&0xff);
	m_splash_color.b = (uint8)((color>>16)&0xff);
	m_splash_color.a = (uint8)((color>>24)&0xff); 

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetSplashBlendMode | Sets the blendmode of the splashes.
// @parm blendmode | The name of the blend mode. Type are: blend/add/sub/modulate/brighten &
// fixblend/fixadd/fixsub/fixmodulate/fixbrighten & diffuse (no blend at all). Defaults to fixadd.
// @parmopt int | Fixed alpha value. Defaults to 64. Range is 0-255. Only required if using fix blend modes.
bool CWeather::ScriptWeatherSetSplashBlendMode( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 blendmode_checksum = 0;
	if ( !pParams->GetChecksum( NONAME, &blendmode_checksum ) )
	{
		return false;
	}
	int fix = 32;		// Default
	pParams->GetInteger( NONAME, &fix );

	plat_set_splash_blend_mode( blendmode_checksum, fix );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetSnowHeight | Sets the height of the snow above the ground.
// @parm height | The height of the snow in inches - defaults to 500 inches.
bool CWeather::ScriptWeatherSetSnowHeight( Script::CStruct* pParams, Script::CScript* pScript )
{
	m_snow_height = 500.0f;		// Default
	pParams->GetFloat( NONAME, &m_snow_height );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetSnowFrames | Sets the number of frames it takes a snow flake to fall.
// @parm frames | Number of frames for the snow to fall from SnowHeight to the ground. Defaults to 254 frames.
bool CWeather::ScriptWeatherSetSnowFrames( Script::CStruct* pParams, Script::CScript* pScript )
{
	m_snow_frames = 254;		// Default
	pParams->GetInteger( NONAME, &m_snow_frames );

	if ( m_snow_frames < 0 ) m_snow_frames = 0;
	if ( m_snow_frames > 254 ) m_snow_frames = 254;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetSnowSize | Sets the size of the snow flake image.
// @parm size | Size of the snow flakes in inches. Defaults to 4 inches.
bool CWeather::ScriptWeatherSetSnowSize( Script::CStruct* pParams, Script::CScript* pScript )
{
	m_snow_size = 4.0f;		// Default
	pParams->GetFloat( NONAME, &m_snow_size );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetSnowBlendMode | Sets the blendmode of the snow flakes.
// @parm blendmode | The name of the blend mode. Type are: blend/add/sub/modulate/brighten &
// fixblend/fixadd/fixsub/fixmodulate/fixbrighten & diffuse (no blend at all). Defaults to fixadd.
// @parmopt int | Fixed alpha value. Defaults to 64. Range is 0-255. Only required if using fix blend modes.
bool CWeather::ScriptWeatherSetSnowBlendMode( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 blendmode_checksum = 0;
	if ( !pParams->GetChecksum( NONAME, &blendmode_checksum ) )
	{
		return false;
	}
	int fix = 32;		// Default
	pParams->GetInteger( NONAME, &fix );

	plat_set_snow_blend_mode( blendmode_checksum, fix );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetSnowRate | Sets the rate that the snow flakes fall in flakes per frame.
// @parm rate | The number of new flakes of snow per frame. Defaults to 0 (off).
bool CWeather::ScriptWeatherSetSnowRate( Script::CStruct* pParams, Script::CScript* pScript )
{
	m_snow_flakes_per_frame = 0.0f;		// Default
	pParams->GetFloat( NONAME, &m_snow_flakes_per_frame );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetSnowColor | Sets the color of the snow.
// @parm col | The color of the snow. Defaults to 0x80808080
bool CWeather::ScriptWeatherSetSnowColor( Script::CStruct* pParams, Script::CScript* pScript )
{
	uint32 color;

	color = 0x80808080;
	pParams->GetInteger( NONAME, (int*)&color ); 

	m_snow_color.r = (uint8)((color)&0xff);
	m_snow_color.g = (uint8)((color>>8)&0xff);
	m_snow_color.b = (uint8)((color>>16)&0xff);
	m_snow_color.a = (uint8)((color>>24)&0xff);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetSnowActive | Sets weather system to snow mode.
bool CWeather::ScriptWeatherSetSnowActive( Script::CStruct* pParams, Script::CScript* pScript )
{
	m_raining = false;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// @script | WeatherSetRainActive | Sets weather system to rain mode.
bool CWeather::ScriptWeatherSetRainActive( Script::CStruct* pParams, Script::CScript* pScript )
{
	m_raining = true;

	return true;
}

}

