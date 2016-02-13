#ifndef __RENDER_H
#define __RENDER_H

#include <core/math.h>
#include <core/math/geometry.h>
#include <gfx/ngc/p_nxmodel.h>
#include "mesh.h"
#include "scene.h"
#include <sys/ngc/p_vector.h>
#include <sys/ngc/p_matrix.h>

#define		RS_ZWRITEENABLE			1
#define		RS_ZTESTENABLE			2
#define		RS_ALPHACUTOFF			3
#define		RS_UVADDRESSMODE0		4
#define		RS_UVADDRESSMODE1		5
#define		RS_UVADDRESSMODE2		6
#define		RS_UVADDRESSMODE3		7

namespace NxNgc
{
	struct sTextureProjectionDetails
	{
		sTexture		*p_texture;
		Nx::CNgcModel	*p_model;
		sScene			*p_scene;
		NsMatrix		view_matrix;
		NsMatrix		projection_matrix;
		NsMatrix		texture_projection_matrix;
	};

	extern Lst::HashTable< sTextureProjectionDetails > *pTextureProjectionDetailsTable;
	
	typedef enum
	{
		vBLEND_MODE_DIFFUSE,						// ( 0 - 0 ) * 0 + Src
		vBLEND_MODE_ADD,							// ( Src - 0 ) * Src + Dst
		vBLEND_MODE_ADD_FIXED,						// ( Src - 0 ) * Fixed + Dst
		vBLEND_MODE_SUBTRACT,						// ( 0 - Src ) * Src + Dst
		vBLEND_MODE_SUB_FIXED,						// ( 0 - Src ) * Fixed + Dst
		vBLEND_MODE_BLEND,							// ( Src * Dst ) * Src + Dst	
		vBLEND_MODE_BLEND_FIXED,					// ( Src * Dst ) * Fixed + Dst	
		vBLEND_MODE_MODULATE,						// ( Dst - 0 ) * Src + 0
		vBLEND_MODE_MODULATE_FIXED,					// ( Dst - 0 ) * Fixed + 0	
		vBLEND_MODE_BRIGHTEN,						// ( Dst - 0 ) * Src + Dst
		vBLEND_MODE_BRIGHTEN_FIXED,					// ( Dst - 0 ) * Fixed + Dst	
		vBLEND_MODE_GLOSS_MAP,						// Specular = Specular * Src	- special mode for gloss mapping
		vBLEND_MODE_BLEND_PREVIOUS_MASK,			// ( Src - Dst ) * Dst + Dst
		vBLEND_MODE_BLEND_INVERSE_PREVIOUS_MASK,	// ( Dst - Src ) * Dst + Src

		vNUM_BLEND_MODES
	} BlendModes; 

	typedef enum
	{
		vRENDER_OPAQUE			= 1,						
		vRENDER_SEMITRANSPARENT	= 2,
		vRENDER_OCCLUDED		= 4,
		vRENDER_NO_CULLING		= 8,		// Used for instances which have already been culled at a higher level
		vRENDER_SHADOW_1ST_PASS = 16,
		vRENDER_SHADOW_2ND_PASS = 32,
		vRENDER_NEW_TEST		= 64,
		vRENDER_INSTANCE_PRE_WORLD_SEMITRANSPARENT	= 128,		// Used to indicate that this instance rendering is happening prior to semitransparent world rendering
		vRENDER_INSTANCE_POST_WORLD_SEMITRANSPARENT	= 256,		// Used to indicate that this instance rendering is happening after semitransparent world rendering
		vRENDER_TRANSFORM		= 512,
		vRENDER_LIT				= 1024,
		vNUM_SCENE_RENDER_FLAGS
	} SceneRenderFlags; 

	void	init_render_system();
	void	set_render_state( uint32 type, uint32 state );
	void	set_blend_mode( uint64 mode );

	void	create_texture_projection_details( sTexture *p_texture, Nx::CNgcModel *p_model, sScene *p_scene );
	void	destroy_texture_projection_details( sTexture *p_texture );
	void	set_texture_projection_camera( sTexture *p_texture, NsVector * p_pos, NsVector * p_at );
	void	calculate_tex_proj_matrix( NsMatrix * p_tex_view_matrix, NsMatrix * p_tex_proj_matrix, NsMatrix * p_tex_transform_matrix, NsMatrix * p_world_matrix = NULL );
	
	void	set_camera( Mth::Matrix *p_matrix, Mth::Vector *p_position, float screen_angle, float aspect_ratio, float scale = 1.0f );
	void	set_frustum_bbox_transform( Mth::Matrix *p_transform );
	bool	frustum_check_box( Mth::CBBox& box );
	bool	frustum_check_sphere( Mth::Vector& p_sphere );
	bool	frustum_check_sphere( Mth::Vector& p_sphere, float * p_z );
	void	render_scene( sScene *p_scene, s16* p_posNormBuffer, uint32 flags = ( vRENDER_OPAQUE | vRENDER_SEMITRANSPARENT ), Mth::Matrix * p_bone_xform = NULL, Mth::Matrix * p_instance_xform = NULL, int num_bones = 0 );
	bool	IsVisible( Mth::Vector &sphere );
	void	render_shadow_targets();

	int		cull_scene( sScene *p_scene, uint32 flags = 0 );
	void	make_scene_visible( sScene *p_scene );

	void	render_begin( void );
	void	render_end( void );
} // namespace NxNgc

#endif // __RENDER_H

