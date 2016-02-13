#ifndef __NX_INIT_H
#define __NX_INIT_H

#include <core/defines.h>
#include <xgmath.h>

namespace NxXbox
{

void InitialiseEngine( void );
void FatalFileError( uint32 error );

typedef struct
{
	XGMATRIX			world_matrix;
	XGMATRIX			view_matrix;
	XGMATRIX			projection_matrix;

	XGMATRIX			bump_env_matrix;					// Used to set the D3DTSS_BUMPENVMATnn texture states where applicable.

	D3DVIEWPORT8		viewport;
	float				near_plane;
	float				far_plane;
	float				screen_angle;
	float				near_plane_width;
	float				near_plane_height;
	bool				is_orthographic;
	bool				clear_color_buffer;					// Whether the color buffer is cleared during buffer swap and clear process.
	bool				letterbox_active;					// Whether running in 4:3 letterbox mode.
	D3DCOLOR			clear_color;						// The color to which the color buffer is cleared.
	XGVECTOR3			cam_position;
	XGVECTOR3			model_relative_cam_position;		// Used in specular lighting calculations.
	XGVECTOR3			cam_at;
	XGVECTOR3			cam_up;
	XGVECTOR3			cam_right;

	int					render_start_time;					// Time (milliseconds) at which the current frame render started.

	bool				loadingscreen_visible;
	MMRESULT			loadingbar_timer_event;
		
	IDirect3DDevice8*	p_Device;
	IDirect3DSurface8*	p_RenderSurface;
	IDirect3DSurface8*	p_ZStencilSurface;
	IDirect3DSurface8*	p_BlurSurface[4];

	int					backbuffer_width;
	int					backbuffer_height;
	D3DFORMAT			backbuffer_format;
	D3DFORMAT			blurbuffer_format;
	int					zstencil_depth;
	float				screen_conv_x_multiplier;
	float				screen_conv_y_multiplier;
	int					screen_conv_x_offset;
	int					screen_conv_y_offset;
	
	void*				p_memory_resident_font;

	char				screenshot_name[128];

	// For bounding sphere culling calculations.
	float				ViewFrustumTX;
	float				ViewFrustumTY;
	float				ViewFrustumSX;
	float				ViewFrustumSY;
	float				ViewFrustumCX;
	float				ViewFrustumCY;

	uint32				blend_mode_value;
	uint32				blend_op;
	uint32				src_blend;
	uint32				dest_blend;

	uint32				alpha_blend_enable;
	uint32				alpha_test_enable;
	uint32				alpha_ref;
	uint32				specular_enabled;
	bool				lighting_enabled;
	bool				dither_enable;
	bool				z_write_enabled;
	bool				z_test_enabled;
	uint32				z_bias;
	int					cull_mode;
	bool				allow_envmapping;				// Set to true (default) to allow costly environment mapping
	uint32				uv_addressing[4];
	uint32				mip_map_lod_bias[4];
	uint32				min_mag_filter[4];
	void				*p_texture[4];
	DWORD				color_sign[4];
	bool				custom_pipeline_enabled;		// A true value indicates that the fixed function pipeline is not being used.
	DWORD				vertex_shader_id;
	DWORD				pixel_shader_override;
	DWORD				pixel_shader_id;
	float				pixel_shader_constants[20];		// 4 floats per constant.
														// c0 - c3	: material pass color (rgb) and fixed alpha (a) for relevant blend modes
														// c4		: fog denisty (b), 0.5 (a)
	bool				upload_pixel_shader_constants;
	DWORD				vertex_shader_override;
	DWORD				texture_stage_override;
	DWORD				material_override;
	DWORD				blend_mode_override;
	float				ambient_light_color[4];			// In format ready to load to GPU.
	float				directional_light_color[24];	// In format ready to load to GPU (dir0, col0, dir 1, col1, etc).
	uint32				screen_blur;					// [0, 255] - [no blur, max blur]
	uint32				screen_blur_duration;			// How many frames the screen blur has been active for.
	uint32				focus_blur;						// [0, 255] - [no blur, max blur]
	uint32				focus_blur_duration;			// How many frames the focus blur has been active for.

	uint32				fog_enabled;
	D3DCOLOR			fog_color;
	float				fog_start;
	float				fog_end;
}
sEngineGlobals;

extern sEngineGlobals EngineGlobals;



} // namespace NxXbox

#endif // __NX_INIT_H
