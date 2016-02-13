///////////////////////////////////////////////////////////////////////////////
// NxParticle.h



#ifndef	__GFX_WEATHER_H__
#define	__GFX_WEATHER_H__

#include <gel/Scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gfx/image/imagebasic.h>

namespace Nx
{

class CWeather
{
public:
							CWeather();
	virtual					~CWeather();

	void					UpdateGrid( void );

	void					Process( float delta_time );
	void					Render( void );

	// Script functions.
	bool					ScriptWeatherSetRainHeight( Script::CStruct* pParams, Script::CScript* pScript );
	bool					ScriptWeatherSetRainFrames( Script::CStruct* pParams, Script::CScript* pScript );
	bool					ScriptWeatherSetRainLength( Script::CStruct* pParams, Script::CScript* pScript );
	bool					ScriptWeatherSetRainBlendMode( Script::CStruct* pParams, Script::CScript* pScript );
	bool					ScriptWeatherSetRainRate( Script::CStruct* pParams, Script::CScript* pScript );
	bool					ScriptWeatherSetRainColor( Script::CStruct* pParams, Script::CScript* pScript );

	bool					ScriptWeatherSetSplashRate( Script::CStruct* pParams, Script::CScript* pScript );
	bool					ScriptWeatherSetSplashLife( Script::CStruct* pParams, Script::CScript* pScript );
	bool					ScriptWeatherSetSplashSize( Script::CStruct* pParams, Script::CScript* pScript );
	bool					ScriptWeatherSetSplashColor( Script::CStruct* pParams, Script::CScript* pScript );
	bool					ScriptWeatherSetSplashBlendMode( Script::CStruct* pParams, Script::CScript* pScript );

	bool					ScriptWeatherSetSnowHeight( Script::CStruct* pParams, Script::CScript* pScript );
	bool					ScriptWeatherSetSnowFrames( Script::CStruct* pParams, Script::CScript* pScript );
	bool					ScriptWeatherSetSnowSize( Script::CStruct* pParams, Script::CScript* pScript );
	bool					ScriptWeatherSetSnowBlendMode( Script::CStruct* pParams, Script::CScript* pScript );
	bool					ScriptWeatherSetSnowRate( Script::CStruct* pParams, Script::CScript* pScript );
	bool					ScriptWeatherSetSnowColor( Script::CStruct* pParams, Script::CScript* pScript );

	bool					ScriptWeatherSetSnowActive( Script::CStruct* pParams, Script::CScript* pScript );
	bool					ScriptWeatherSetRainActive( Script::CStruct* pParams, Script::CScript* pScript );

	// The virtual functions have a stub implementation.
private:
	virtual void			plat_update_grid( void );
	virtual void			plat_process( float delta_time );
	virtual void			plat_render( void );
	virtual void			plat_set_rain_blend_mode( uint32 blendmode_checksum, int fix );
	virtual void			plat_set_splash_blend_mode( uint32 blendmode_checksum, int fix );
	virtual void			plat_set_snow_blend_mode( uint32 blendmode_checksum, int fix );

protected:
	float					m_rain_height;
	int						m_rain_frames;
	float					m_rain_length;
	float					m_rain_drops_per_frame;
	Image::RGBA				m_rain_top_color;
	Image::RGBA				m_rain_bottom_color;

	float					m_splash_per_frame;
	int						m_splash_life;
	float					m_splash_size;
	Image::RGBA				m_splash_color;

	int						m_active_drops;

	bool					m_system_active;

	float					m_snow_height;
	int						m_snow_frames;
	float					m_snow_size;
	float					m_snow_flakes_per_frame;
	Image::RGBA				m_snow_color;

	bool					m_raining;
};

}

#endif // 



