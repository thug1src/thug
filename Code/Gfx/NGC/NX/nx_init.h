#ifndef __NX_INIT_H
#define __NX_INIT_H

#include <core/defines.h>
#include <sys/ngc/p_camera.h>

namespace NxNgc
{

void InitialiseEngine( void );

typedef struct
{
//	IDirect3DDevice8*	p_Device;
//	IDirect3DSurface8*	p_RenderSurface;
//	IDirect3DSurface8*	p_ZStencilSurface;

	uint32				blend_mode_value;
	uint32				blend_mode_fixed_alpha;

	// These renderstates should go in their own structure.
	bool				alpha_blend_enable;
	bool				alpha_test_enable;
	uint32				alpha_ref;
	bool				lighting_enabled;
	bool				dither_enable;
	bool				z_write_enabled;
	bool				z_test_enabled;
	int					cull_mode;
	bool				poly_culling;

	NsMatrix			local_to_camera;
	NsMatrix			world_to_camera;
	NsMatrix			camera;
	NsMatrix			shadow_camera;
	Vec					object_pos;
	Mtx					current_uploaded;

	float				tx;
	float				ty;
	float				sx;
	float				sy;
	float				cx;
	float				cy;
	float				near;
	float				far;

	GXColor				ambient_light_color;
	GXColor				diffuse_light_color[3];
	float				light_x[3];
	float				light_y[3];
	float				light_z[3];

	float				screen_brightness;	// 0 = black, 1 = normal, 2 = saturated.

	uint32				frameCount;

	volatile bool		gpuBusy;

	// ARAM defines.
	#define aram_zero_size		1024
	#define	aram_dsp_size		1024 * 1900
	#define	aram_stream0_size	1024 * 32
	#define aram_stream1_size	aram_stream0_size
	#define aram_stream2_size	aram_stream1_size
	#define aram_music_size		1024 * 32
	
	uint32				aram_zero;
	uint32				aram_dsp;
	uint32				aram_stream0;
	uint32				aram_stream1;
	uint32				aram_stream2;
	uint32				aram_music;

	bool				use_480p;
	bool				use_60hz;
	bool				use_widescreen;
	bool				letterbox_active;

	bool				reduceColors;

	bool				disableReset;
	bool				resetToIPL;

	uint32				viewport;		// Expressed as a bit ( 1 << v ).

	NsVector			skater_shadow_dir;
	float				skater_height;
	NsVector			ped_shadow_dir;
}
sEngineGlobals;

extern sEngineGlobals EngineGlobals;



} // namespace NxNgc

#endif // __NX_INIT_H
