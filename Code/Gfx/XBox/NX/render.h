#ifndef __RENDER_H
#define __RENDER_H

#include <xgmath.h>
#include <core/math.h>
#include <core/math/geometry.h>
#include <gfx/xbox/p_nxmodel.h>
#include "mesh.h"

#define		RS_ZWRITEENABLE			1
#define		RS_ZTESTENABLE			2
#define		RS_ALPHACUTOFF			3
#define		RS_UVADDRESSMODE0		4
#define		RS_UVADDRESSMODE1		5
#define		RS_UVADDRESSMODE2		6
#define		RS_UVADDRESSMODE3		7
#define		RS_MIPLODBIASPASS0		8
#define		RS_MIPLODBIASPASS1		9
#define		RS_MIPLODBIASPASS2		10
#define		RS_MIPLODBIASPASS3		11
#define		RS_CULLMODE				16
#define		RS_ALPHABLENDENABLE		17
#define		RS_ALPHATESTENABLE		18
#define		RS_SPECULARENABLE		19
#define		RS_FOGENABLE			20
#define		RS_ZBIAS				21
#define		RS_MINMAGFILTER0		32
#define		RS_MINMAGFILTER1		33
#define		RS_MINMAGFILTER2		34
#define		RS_MINMAGFILTER3		35

extern		DWORD PixelShader0;
extern		DWORD PixelShader0IVA;
extern		DWORD PixelShader1;
extern		DWORD PixelShader2;
extern		DWORD PixelShader3;
extern		DWORD PixelShader4;
extern		DWORD PixelShader5;
extern		DWORD PixelShaderBrighten;
extern		DWORD PixelShaderBrightenIVA;
extern		DWORD PixelShaderFocusBlur;
extern		DWORD PixelShaderFocusIntegrate;
extern		DWORD PixelShaderFocusLookupIntegrate;
extern		DWORD PixelShaderNULL;
extern		DWORD PixelShaderPointSprite;
extern		DWORD PixelShaderBumpWater;

namespace NxXbox
{
	struct sTextureProjectionDetails
	{
		sTexture		*p_texture;
		Nx::CXboxModel	*p_model;
		sScene			*p_scene;
		XGMATRIX		view_matrix;
		XGMATRIX		projection_matrix;
		XGMATRIX		texture_projection_matrix;
	};


	extern Lst::HashTable< sTextureProjectionDetails > *pTextureProjectionDetailsTable;

	
	typedef enum
	{
		vBLEND_MODE_DIFFUSE,								// ( 0 - 0 ) * 0 + Src
		vBLEND_MODE_ADD,									// ( Src - 0 ) * Src + Dst
		vBLEND_MODE_ADD_FIXED,								// ( Src - 0 ) * Fixed + Dst
		vBLEND_MODE_SUBTRACT,								// ( 0 - Src ) * Src + Dst
		vBLEND_MODE_SUB_FIXED,								// ( 0 - Src ) * Fixed + Dst
		vBLEND_MODE_BLEND,									// ( Src * Dst ) * Src + Dst	
		vBLEND_MODE_BLEND_FIXED,							// ( Src * Dst ) * Fixed + Dst	
		vBLEND_MODE_MODULATE,								// ( Dst - 0 ) * Src + 0
		vBLEND_MODE_MODULATE_FIXED,							// ( Dst - 0 ) * Fixed + 0	
		vBLEND_MODE_BRIGHTEN,								// ( Dst - 0 ) * Src + Dst
		vBLEND_MODE_BRIGHTEN_FIXED,							// ( Dst - 0 ) * Fixed + Dst	
		vBLEND_MODE_GLOSS_MAP,								// Specular = Specular * Src	- special mode for gloss mapping
		vBLEND_MODE_BLEND_PREVIOUS_MASK,					// ( Src - Dst ) * Dst + Dst
		vBLEND_MODE_BLEND_INVERSE_PREVIOUS_MASK,			// ( Dst - Src ) * Dst + Src

		vBLEND_MODE_MODULATE_COLOR					= 15,	// ( Dst - 0 ) * Src(col) + 0	- special mode for the shadow.
		vBLEND_MODE_ONE_INV_SRC_ALPHA				= 17,	//								- special mode for imposter rendering.

		vNUM_BLEND_MODES
	} BlendModes; 

	typedef enum
	{
		vRENDER_OPAQUE								= 1,						
		vRENDER_SEMITRANSPARENT						= 2,
		vRENDER_OCCLUDED							= 4,
		vRENDER_NO_CULLING							= 8,		// Used for instances which have already been culled at a higher level
		vRENDER_SORT_FRONT_TO_BACK					= 16,		// Used to improve pixel rejection tests for opaque rendering only
		vRENDER_SHADOW_VOLUMES						= 32,		// Used to indicate that only shadow volumes should be (special-case) rendered
		vRENDER_BILLBOARDS							= 64,		// Used to indicate that billboards should be rendered
		vRENDER_INSTANCE_PRE_WORLD_SEMITRANSPARENT	= 128,		// Used to indicate that this instance rendering is happening prior to semitransparent world rendering
		vRENDER_INSTANCE_POST_WORLD_SEMITRANSPARENT	= 256,		// Used to indicate that this instance rendering is happening after semitransparent world rendering
	} SceneRenderFlags; 

	BlendModes	GetBlendMode( uint32 blend_checksum );

	void		create_pixel_shaders();
	void		GetPixelShader( sMaterial *p_material, uint32 *p_pixel_shader_id );
	void		set_pixel_shader( uint32 shader_id );
	void		set_pixel_shader( uint32 shader_id, uint32 num_passes );
	void		set_vertex_shader( DWORD shader_id );

	void		set_render_state( uint32 type, uint32 state );
	void		set_blend_mode( uint32 mode );
	void		set_texture( uint32 pass, IDirect3DTexture8 *p_texture, IDirect3DPalette8 *p_palette = NULL );
	
	void		create_texture_projection_details( sTexture *p_texture, Nx::CXboxModel *p_model, sScene *p_scene );
	void		destroy_texture_projection_details( sTexture *p_texture );
	void		set_texture_projection_camera( sTexture *p_texture, XGVECTOR3 *p_pos, XGVECTOR3 *p_at );
	void		calculate_tex_proj_matrix( XGMATRIX *p_tex_view_matrix, XGMATRIX *p_tex_proj_matrix, XGMATRIX *p_tex_transform_matrix, XGMATRIX *p_world_matrix = NULL );
	
	// MSM PERFCHANGE - added scale.
	void	set_camera( Mth::Matrix *p_matrix, Mth::Vector *p_position, float screen_angle, float aspect_ratio, bool render_at_infinity = false );
	void	set_frustum_bbox_transform( Mth::Matrix *p_transform );
	bool	frustum_check_sphere( D3DXVECTOR3 *p_center, float radius );
	float	get_bounding_sphere_nearest_z( void );
	bool	IsVisible( Mth::Vector &center, float radius );
	void	render_shadow_targets();
	void	render_light_glows( bool test );
	void	render_scene( sScene *p_scene, uint32 flags = ( vRENDER_OPAQUE | vRENDER_SEMITRANSPARENT ), uint32 viewport = 0 );

} // namespace NxXbox

#endif // __RENDER_H
