//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_NxWeather.h
//* OWNER:          Dave Cowling
//* CREATION DATE:  6/19/2003
//****************************************************************************

#ifndef	__GFX_P_NX_WEATHER_H__
#define	__GFX_P_NX_WEATHER_H__
    
#include "gfx/nxweather.h"

namespace Nx
{

#define SUB_INCH_PRECISION		16.0f
#define WEATHER_CEL_SIZE		128
#define WEATHER_CEL_SIZE_SHIFT	7
#define HEIGHT_TOLERANCE		(sint32)( 30.0f * SUB_INCH_PRECISION )
#define DROP_SIZE				16
#define DROP_SIZE_SHIFT			4
#define DROP_LAYERS				4
#define NUM_SPLASH_ACTIVE		32

typedef struct
{
	unsigned short start;
	unsigned short end;
	unsigned short index;
}
sRowEntry;

class CXboxWeather : public CWeather
{
public:
							CXboxWeather();
	virtual 				~CXboxWeather();

private:					// It's all private, as it is machine specific
	void					plat_update_grid( void );
	virtual void			plat_process( float delta_time );
	virtual void			plat_render( void );
	void					plat_set_rain_blend_mode( uint32 blendmode_checksum, int fix );
	void					plat_set_splash_blend_mode( uint32 blendmode_checksum, int fix );
	void					plat_set_snow_blend_mode( uint32 blendmode_checksum, int fix );

	void					plat_render_splashes( float skx, float skz );
	void					plat_render_rain( float skx, float skz );
	void					plat_render_snow( float skx, float skz );

	NxXbox::sTexture*		mp_rain_texture;
	NxXbox::sTexture*		mp_snow_texture;

	unsigned char *			mp_roof_height_index;
	sRowEntry *				mp_roof_row;
	sint32					m_roof_height[256];
	int						m_width;
	int						m_height;

	float					m_min_x;
	float					m_min_z;

	float					m_rain_rate;
	float					m_splash_rate;
	float					m_snow_rate;

	Mth::Vector				m_grid_base;

	unsigned char			m_drop_time[DROP_SIZE*DROP_SIZE*DROP_LAYERS];
	unsigned char			m_x_offset[DROP_SIZE*DROP_SIZE*DROP_LAYERS];
	unsigned char			m_z_offset[DROP_SIZE*DROP_SIZE*DROP_LAYERS];

	uint32					m_seq;

	uint32					m_rain_blend;
	uint32					m_splash_blend;
	uint32					m_snow_blend;

	float					m_splash_x[NUM_SPLASH_ACTIVE];
	float					m_splash_y[NUM_SPLASH_ACTIVE];
	float					m_splash_z[NUM_SPLASH_ACTIVE];
	int						m_splash_current_life[NUM_SPLASH_ACTIVE]; 
	int						m_current_splash;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
} // Nx


#endif

