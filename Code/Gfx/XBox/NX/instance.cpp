#include <gfx\NxLight.h>
#include <gfx\DebugGfx.h>
#include <gfx\DebugGfx.h>
#include <gel/scripting/symboltable.h>
#include "nx_init.h"
#include "instance.h"
#include "anim.h"
#include "render.h"
#include "occlude.h"
#include "anim_vertdefs.h"

namespace NxXbox
{

static Mth::Matrix	*pLastBoneTransforms	= NULL;
CInstance			*pFirstInstance			= NULL;


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static int sort_by_bone_transform( const void *p1, const void *p2 )
{
	CInstance	*p_mesh1		= *((CInstance**)p1 );
	CInstance	*p_mesh2		= *((CInstance**)p2 );

	Mth::Matrix *p_mat1			= p_mesh1->GetBoneTransforms();
	Mth::Matrix *p_mat2			= p_mesh2->GetBoneTransforms();
	
	if(( p_mat1 == NULL ) || ( p_mesh1->GetScene()->m_numHierarchyObjects > 0 ))
	{
		if(( p_mat2 == NULL ) || ( p_mesh2->GetScene()->m_numHierarchyObjects > 0 ))
		{
			int num_st1 = p_mesh1->GetScene()->m_num_semitransparent_mesh_entries;
			int num_st2 = p_mesh2->GetScene()->m_num_semitransparent_mesh_entries;

			if( num_st1 == num_st2 )
			{
				// Try sorting on the material draw order of the first semitransparent mesh.
				if( num_st1 > 0 )
				{
					NxXbox::sMaterial	*p_material1 = p_mesh1->GetScene()->m_meshes[p_mesh1->GetScene()->m_first_semitransparent_entry]->mp_material;
					NxXbox::sMaterial	*p_material2 = p_mesh2->GetScene()->m_meshes[p_mesh2->GetScene()->m_first_semitransparent_entry]->mp_material;

					if( p_material1 && p_material2 )
					{
						return ( p_material1->m_draw_order > p_material2->m_draw_order ) ? 1 : (( p_material1->m_draw_order < p_material2->m_draw_order ) ? -1 : 0 );
					}
				}
				return 0;
			}
	
			return ( num_st1 > num_st2 ) ? 1 : -1;
		}
		return 1;
	}
	else if(( p_mat2 == NULL ) || ( p_mesh2->GetScene()->m_numHierarchyObjects > 0 ))
	{
		return -1;
	}

	// At this stage we know both instances have bone transforms.
	if( p_mat1 == p_mat2 )
	{
		int num_st1 = p_mesh1->GetScene()->m_num_semitransparent_mesh_entries;
		int num_st2 = p_mesh2->GetScene()->m_num_semitransparent_mesh_entries;
	
		if( num_st1 == num_st2 )
			return 0;
	
		return ( num_st1 > num_st2 ) ? 1 : -1;
	}

	return((uint32)p_mat1 > (uint32)p_mat2 ) ? 1 : -1;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void render_instance( CInstance* p_instance, uint32 flags )
{
	// Seed the static pointer off to NULL, otherwise if there is only one object with bone transforms, it will never update.
	pLastBoneTransforms = NULL;

	if( p_instance->GetActive())
	{
		set_frustum_bbox_transform( p_instance->GetTransform());

		bool render = true;
		if( !( flags & vRENDER_NO_CULLING ))
		{
			// Check whether this instance is visible.
			render = false;
			if( frustum_check_sphere( &p_instance->GetScene()->m_sphere_center, p_instance->GetScene()->m_sphere_radius ))
			{
				if( !TestSphereAgainstOccluders( &p_instance->GetScene()->m_sphere_center, p_instance->GetScene()->m_sphere_radius ))
				{
					render = true;
				}
			}
		}

		if( render )
		{
			if( p_instance->GetBoneTransforms() != NULL )
			{
				startup_weighted_mesh_vertex_shader();
				p_instance->Render( vRENDER_OPAQUE | vRENDER_SEMITRANSPARENT );
				shutdown_weighted_mesh_vertex_shader();
			}
			else
			{
				p_instance->Render( vRENDER_OPAQUE | vRENDER_SEMITRANSPARENT );
			}
		}

		// Restore world transform to identity.
		D3DDevice_SetTransform( D3DTS_WORLD, &EngineGlobals.world_matrix );
		set_frustum_bbox_transform( NULL );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void render_instances( uint32 flags )
{
	#define INSTANCE_ARRAY_SIZE	4096
	static CInstance *p_instances[INSTANCE_ARRAY_SIZE];
	int current_index = 0;

	Mth::Matrix t;
	t.Identity();
	
	// Seed the static pointer off to NULL, otherwise if there is only one object with bone transforms, it will never update.
	pLastBoneTransforms = NULL;
	
	// First go through and build a list of the visible instances.
	CInstance *p_instance = pFirstInstance;
	while( p_instance )
	{
		if( p_instance->GetActive())
		{
			// Check to see whether this instance is of the type we want to render - opaque or semitransparent.
			if((( flags & vRENDER_OPAQUE ) && ( p_instance->GetScene()->m_num_mesh_entries > p_instance->GetScene()->m_num_semitransparent_mesh_entries )) ||
			   (( flags & vRENDER_SEMITRANSPARENT ) && ( p_instance->GetScene()->m_num_semitransparent_mesh_entries > 0 )))
			{
				// Check whether this instance is visible - if so, place it in the visible array.
				t.SetPos( p_instance->GetTransform()->GetPos());
				set_frustum_bbox_transform( &t );
				
				// For skinned objects, we have no idea how the skeleton transforms will be affecting the final position of each object.
				// This can sometimes result in small objects getting culled incorrectly. For these objects, increase the size of
				// the bounding sphere slightly.
				float radius = p_instance->GetScene()->m_sphere_radius;
				if( p_instance->GetBoneTransforms() && ( p_instance->GetScene()->m_numHierarchyObjects == 0 ))
				{
					radius = ( radius < 72.0f ) ? 72.0f : radius;
				}
				
				// The logic code already sets the active flag based on visibility, so there is no need to perform a second visibility check
				// at this point.
				// We do, however, want to test against occluders.
				if( !TestSphereAgainstOccluders( &p_instance->GetScene()->m_sphere_center, radius ))
				{
					Dbg_Assert( current_index < INSTANCE_ARRAY_SIZE );
					p_instances[current_index++] = p_instance;
				}
			}
		}
		p_instance = p_instance->GetNextInstance();
	}

	// Now sort the list based on bone transform and number of semitransparent objects in scene.
	if( current_index > 0 )
	{
		qsort( p_instances, current_index, sizeof( CInstance* ), sort_by_bone_transform );

		int i = 0;

		// First render the instances with bone transforms.
		startup_weighted_mesh_vertex_shader();
		for( ; i < current_index; ++i )
		{
			if(( p_instances[i]->GetBoneTransforms() == NULL ) || ( p_instances[i]->GetScene()->m_numHierarchyObjects > 0 ))
				break;

			if(( flags & vRENDER_OPAQUE ) || (( flags & vRENDER_SEMITRANSPARENT ) && ( flags & vRENDER_INSTANCE_PRE_WORLD_SEMITRANSPARENT )))
				p_instances[i]->Render( flags );
		}
		shutdown_weighted_mesh_vertex_shader();
		
		// Then the instances with no bone transforms. These will require fixed function lighting.
		D3DDevice_SetRenderState( D3DRS_LIGHTING,				TRUE );
		D3DDevice_SetRenderState( D3DRS_COLORVERTEX,			TRUE );
		D3DDevice_LightEnable( 0, TRUE );
		D3DDevice_LightEnable( 1, TRUE );

		Mth::Vector current_cam_pos( EngineGlobals.cam_position.x, EngineGlobals.cam_position.y, EngineGlobals.cam_position.z );

		// Get the (square of) the distance at which env mapping will be disabled, in inches.
		float env_map_disable_distance	= Script::GetFloat( CRCD( 0x4e7dc608, "EnvMapDisableDistance")) * 12.0f;
		env_map_disable_distance		= env_map_disable_distance * env_map_disable_distance;

		for( ; i < current_index; ++i )
		{
			// Calculate the distance from the current camera to the instance.
			float dist_squared = Mth::DistanceSqr( p_instances[i]->GetTransform()->GetPos(), current_cam_pos );

			// At forty feet, environment mapping is turned off.
			EngineGlobals.allow_envmapping = ( dist_squared > env_map_disable_distance ) ? false : true;

			if(( flags & vRENDER_OPAQUE ) || (( flags & vRENDER_SEMITRANSPARENT ) && ( flags & vRENDER_INSTANCE_POST_WORLD_SEMITRANSPARENT )))
				p_instances[i]->Render( flags );
		}

		// Restore environment mapping..
		EngineGlobals.allow_envmapping = true;

		// Shut down fixed function lighting.
		D3DDevice_SetRenderState( D3DRS_LIGHTING,				FALSE );
		D3DDevice_SetRenderState( D3DRS_COLORVERTEX,			FALSE );
		D3DDevice_LightEnable( 0, FALSE );
		D3DDevice_LightEnable( 1, FALSE );
	}

	// Restore world transform to identity.
	D3DDevice_SetTransform( D3DTS_WORLD, &EngineGlobals.world_matrix );
	set_frustum_bbox_transform( NULL );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CInstance::CInstance( sScene *pScene, Mth::Matrix &transform, int numBones, Mth::Matrix *pBoneTransforms )
{
	SetTransform( transform );
	mp_bone_transforms	= pBoneTransforms;
	m_num_bones			= numBones;
	mp_scene			= pScene;
	mp_model			= NULL;
	m_active			= true;
	m_flags				= 0;

	mp_next_instance	= pFirstInstance;
	pFirstInstance		= this;

	// Check to see whether this instance is allowed to be lit or not (for non-skinned instances).
	// Currently, we assume that instances with a model containing valid normals requires lighting, except in the
	// situation where that model contains a mesh specifically flagged not to be lit.
	if(( pBoneTransforms == NULL ) || ( GetScene()->m_numHierarchyObjects > 0 ))
	{
		for( int m = 0; m < pScene->m_num_mesh_entries; ++m )
		{
			NxXbox::sMesh *p_mesh = pScene->m_meshes[m];
			if(( p_mesh->m_vertex_shader[0] & D3DFVF_NORMAL ) == 0 )
				return;
			if( p_mesh->m_flags & sMesh::MESH_FLAG_UNLIT )
				return;
		}
		m_flags |= CInstance::INSTANCE_FLAG_LIGHTING_ALLOWED;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CInstance::~CInstance()
{
	if( m_flags & INSTANCE_FLAG_DELETE_ATTACHED_SCENE )
	{
		if( mp_scene )
		{
			delete mp_scene;
			mp_scene = NULL;
		}
	}
	
	// Remove this instance from the list.
	CInstance **pp_instance = &pFirstInstance;
	while( *pp_instance )
	{
		if( *pp_instance == this )
		{
			*pp_instance = mp_next_instance;
			break;
		}
		else
		{
			pp_instance = &(( *pp_instance )->mp_next_instance );
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CInstance::Render( uint32 flags )
{
	const int			MAX_SUPPORTED_BONES	= 58;
	static float		root_matrix[12];

	static D3DLIGHT8 l0 =
	{
		D3DLIGHT_DIRECTIONAL,
		{ 0.0f, 0.0f, 0.0f, 1.0f },	// Diffuse color
		{ 1.0f, 1.0f, 1.0f, 0.0f },	// Specular color
		{ 0.0f, 0.0f, 0.0f, 0.0f },	// Ambient color
		{ 0.0f, 0.0f, 0.0f },		// Position
		{ 0.0f, 0.0f, 0.0f },		// Direction
		0.0f, 0.0f,					// Range, falloff
		0.0f, 0.0f, 0.0f,			// Attenuation0, 1, 2
		0.0f, 0.0f					// Theta, Phi
	};
	static D3DLIGHT8 l1 =
	{
		D3DLIGHT_DIRECTIONAL,
		{ 0.0f, 0.0f, 0.0f, 1.0f },	// Diffuse color
		{ 0.0f, 0.0f, 0.0f, 0.0f },	// Specular color
		{ 0.0f, 0.0f, 0.0f, 0.0f },	// Ambient color
		{ 0.0f, 0.0f, 0.0f },		// Position
		{ 0.0f, 0.0f, 0.0f },		// Direction
		0.0f, 0.0f,					// Range, falloff
		0.0f, 0.0f, 0.0f,			// Attenuation0, 1, 2
		0.0f, 0.0f					// Theta, Phi
	};

	if(( GetBoneTransforms() == NULL ) || ( GetScene()->m_numHierarchyObjects > 0 ))
	{
		// Is a non-skinned object.
		pLastBoneTransforms = NULL;
		
		// Do the lighting setup here.
		if((( m_flags & CInstance::INSTANCE_FLAG_LIGHTING_ALLOWED ) == 0 ) || ( GetScene()->m_flags & SCENE_FLAG_RENDERING_SHADOW ))
		{
			D3DDevice_SetRenderState( D3DRS_LIGHTING, FALSE );
		}
		else
		{
			Nx::CModelLights *p_lights = GetModel() ? GetModel()->GetModelLights() : NULL;
			if( p_lights )
			{
				// Obtain position from the transform, for calculating Scene Lighting.
				p_lights->UpdateEngine( GetTransform()->GetPos(), true );
			}
			else
			{
				Nx::CLightManager::sUpdateEngine();
			}

			D3DDevice_SetRenderState( D3DRS_LIGHTING, TRUE );
			
			l0.Diffuse.r	= EngineGlobals.directional_light_color[4];
			l0.Diffuse.g	= EngineGlobals.directional_light_color[5];
			l0.Diffuse.b	= EngineGlobals.directional_light_color[6];
			l0.Direction.x	= EngineGlobals.directional_light_color[0];
			l0.Direction.y	= EngineGlobals.directional_light_color[1];
			l0.Direction.z	= EngineGlobals.directional_light_color[2];
			D3DDevice_SetLight( 0, &l0 );

			l1.Diffuse.r	= EngineGlobals.directional_light_color[12];
			l1.Diffuse.g	= EngineGlobals.directional_light_color[13];
			l1.Diffuse.b	= EngineGlobals.directional_light_color[14];
			l1.Direction.x	= EngineGlobals.directional_light_color[8];
			l1.Direction.y	= EngineGlobals.directional_light_color[9];
			l1.Direction.z	= EngineGlobals.directional_light_color[10];
			D3DDevice_SetLight( 1, &l1 );

			D3DDevice_SetRenderState( D3DRS_AMBIENT, D3DCOLOR_RGBA(	Ftoi_ASM( EngineGlobals.ambient_light_color[0] * 255.0f ),
																	Ftoi_ASM( EngineGlobals.ambient_light_color[1] * 255.0f ),
																	Ftoi_ASM( EngineGlobals.ambient_light_color[2] * 255.0f ),
																	0xFF ));
		}

		// If the object has a 'fake skeleton', like the cars with rotating wheels, then set up accordingly.
		if( GetScene()->m_numHierarchyObjects )
		{
			int num_bones = ( GetNumBones() < MAX_SUPPORTED_BONES ) ? GetNumBones() : MAX_SUPPORTED_BONES;
			for( int lp = 0; lp < num_bones; ++lp )
			{
				Mth::Matrix temp = *GetTransform();
				temp = GetBoneTransforms()[lp] * temp;
				set_frustum_bbox_transform( &temp );
				D3DDevice_SetTransform( D3DTS_WORLD, (D3DXMATRIX*)&temp );

				// Scan through the meshes, setting only those with the current bone active.
				for( int m = 0; m < GetScene()->m_num_mesh_entries; ++m )
				{
					NxXbox::sMesh *p_mesh = GetScene()->m_meshes[m];
					if( p_mesh->GetBoneIndex() == lp )
						p_mesh->SetActive( true );
					else
						p_mesh->SetActive( false );
				}
				render_scene( GetScene(), flags | vRENDER_NO_CULLING );
			}
		}
		else
		{
			// Has no skeleton.
			set_frustum_bbox_transform( GetTransform());
			D3DDevice_SetTransform( D3DTS_WORLD, (D3DXMATRIX*)GetTransform());

			render_scene( GetScene(), flags | vRENDER_NO_CULLING );
		}
	}
	else
	{
		// Has bone transforms, is therefore an animated object such as a skater, pedestrian etc.
		set_frustum_bbox_transform( GetTransform());

		// We only want to upload all the bone transforms if they have changed from the last call.
		bool upload_bone_transforms = false;
		
		if( pLastBoneTransforms != GetBoneTransforms())
		{
			// Okay, the bone transforms have changed from the last call.
			upload_bone_transforms = true;
			
			// Do the lighting setup here (if not rendering shadow, which requires no lighting).
			if(( GetScene()->m_flags & SCENE_FLAG_RENDERING_SHADOW ) == 0 )
			{
				Nx::CModelLights *p_lights = GetModel() ? GetModel()->GetModelLights() : NULL;
				if( p_lights )
				{
					p_lights->UpdateEngine( GetTransform()->GetPos(), true );
				}
				else
				{
					Nx::CLightManager::sUpdateEngine();
				}
			}
			
			pLastBoneTransforms = GetBoneTransforms();

			int num_bones = ( GetNumBones() < MAX_SUPPORTED_BONES ) ? GetNumBones() : MAX_SUPPORTED_BONES;
		}
				
		root_matrix[0]	= GetTransform()->GetRight()[X];
		root_matrix[1]	= GetTransform()->GetUp()[X];
		root_matrix[2]	= GetTransform()->GetAt()[X];
		root_matrix[3]	= GetTransform()->GetPos()[X];
		root_matrix[4]	= GetTransform()->GetRight()[Y];
		root_matrix[5]	= GetTransform()->GetUp()[Y];
		root_matrix[6]	= GetTransform()->GetAt()[Y];
		root_matrix[7]	= GetTransform()->GetPos()[Y];
		root_matrix[8]	= GetTransform()->GetRight()[Z];
		root_matrix[9]	= GetTransform()->GetUp()[Z];
		root_matrix[10]	= GetTransform()->GetAt()[Z];
		root_matrix[11]	= GetTransform()->GetPos()[Z];
		
		setup_weighted_mesh_vertex_shader( &root_matrix, &GetBoneTransforms()[0][Mth::RIGHT][X], upload_bone_transforms ? GetNumBones() : 0 );

		if( GetScene()->m_flags & SCENE_FLAG_RENDERING_SHADOW )
		{
			// Set the simple vertex shader that does no normal transform or lighting.
			set_vertex_shader( WeightedMeshVertexShader_SBWrite );
			EngineGlobals.vertex_shader_override = 1;

			// Set the simple pixel shader that just writes constant (1,1,1,1) out.
			set_pixel_shader( PixelShaderNULL );
			EngineGlobals.pixel_shader_override = 1;

			// No backface culling.
			set_render_state( RS_CULLMODE, D3DCULL_NONE );

			// Lock out material changes.
			EngineGlobals.material_override = 1;

			render_scene( GetScene(), flags | vRENDER_NO_CULLING );

//			RenderShadowVolume();
		}
		else
		{
			render_scene( GetScene(), flags | vRENDER_NO_CULLING );

			// Render the self-shadowing pass here, if so flagged.
			if( GetScene()->m_flags & SCENE_FLAG_SELF_SHADOWS )
			{
				static float texture_projection_matrix[16];

				// Calculate distance to the camera - the self shadowing fades out beyond a certain distance, (and also fades out
				// close up, to avoid the visible streaks). Also need to take into account the view angle since that directly affects
				// the relative screen space size of the object.
				const float SHADOW_FADE_CLOSE_START			= 150.0f;
				const float SHADOW_FADE_CLOSE_COMPLETE		= 120.0f;
				const float SHADOW_FADE_FAR_START			= 500.0f;
				const float SHADOW_FADE_FAR_COMPLETE		= 600.0f;
				const float DEFAULT_SCREEN_ANGLE			= 0.72654f;	// tan( 72 / 2 )

				Mth::Vector pos_to_cam	= GetTransform()->GetPos() - Mth::Vector( NxXbox::EngineGlobals.cam_position.x, NxXbox::EngineGlobals.cam_position.y, NxXbox::EngineGlobals.cam_position.z );
				float		dist		= pos_to_cam.Length() * ( tanf( Mth::DegToRad( EngineGlobals.screen_angle * 0.5f )) / DEFAULT_SCREEN_ANGLE );

				if(( dist > SHADOW_FADE_CLOSE_COMPLETE ) && ( dist < SHADOW_FADE_FAR_COMPLETE ))
				{
					// Find which set of details relates to this instance.
					pTextureProjectionDetailsTable->IterateStart();
					sTextureProjectionDetails *p_details = pTextureProjectionDetailsTable->IterateNext();
					while( p_details )
					{
						if( p_details->p_model == mp_model )
							break;

						p_details = pTextureProjectionDetailsTable->IterateNext();
					}

					if( p_details )
					{
						// Need to incorporate the world matrix into the texture projection matrix, since we will be using the bone-transformed
						// vertex positions, which are in object space.
						XGMATRIX world_matrix;
						world_matrix.m[0][0] = GetTransform()->GetRight()[X];
						world_matrix.m[0][1] = GetTransform()->GetRight()[Y];
						world_matrix.m[0][2] = GetTransform()->GetRight()[Z];
						world_matrix.m[0][3] = 0.0f;
						world_matrix.m[1][0] = GetTransform()->GetUp()[X];
						world_matrix.m[1][1] = GetTransform()->GetUp()[Y];
						world_matrix.m[1][2] = GetTransform()->GetUp()[Z];
						world_matrix.m[1][3] = 0.0f;
						world_matrix.m[2][0] = GetTransform()->GetAt()[X];
						world_matrix.m[2][1] = GetTransform()->GetAt()[Y];
						world_matrix.m[2][2] = GetTransform()->GetAt()[Z];
						world_matrix.m[2][3] = 0.0f;
						world_matrix.m[3][0] = GetTransform()->GetPos()[X];
						world_matrix.m[3][1] = GetTransform()->GetPos()[Y];
						world_matrix.m[3][2] = GetTransform()->GetPos()[Z];
						world_matrix.m[3][3] = 1.0f;
						calculate_tex_proj_matrix( &p_details->view_matrix, &p_details->projection_matrix, &p_details->texture_projection_matrix, &world_matrix );

						// Upload that matrix to the GPU also.
						texture_projection_matrix[0]	= p_details->texture_projection_matrix.m[0][0];
						texture_projection_matrix[1]	= p_details->texture_projection_matrix.m[1][0];
						texture_projection_matrix[2]	= p_details->texture_projection_matrix.m[2][0];
						texture_projection_matrix[3]	= p_details->texture_projection_matrix.m[3][0];
						texture_projection_matrix[4]	= p_details->texture_projection_matrix.m[0][1];
						texture_projection_matrix[5]	= p_details->texture_projection_matrix.m[1][1];
						texture_projection_matrix[6]	= p_details->texture_projection_matrix.m[2][1];
						texture_projection_matrix[7]	= p_details->texture_projection_matrix.m[3][1];
						texture_projection_matrix[8]	= p_details->texture_projection_matrix.m[0][2];
						texture_projection_matrix[9]	= p_details->texture_projection_matrix.m[1][2];
						texture_projection_matrix[10]	= p_details->texture_projection_matrix.m[2][2];
						texture_projection_matrix[11]	= p_details->texture_projection_matrix.m[3][2];
						texture_projection_matrix[12]	= p_details->texture_projection_matrix.m[0][3];
						texture_projection_matrix[13]	= p_details->texture_projection_matrix.m[1][3];
						texture_projection_matrix[14]	= p_details->texture_projection_matrix.m[2][3];
						texture_projection_matrix[15]	= p_details->texture_projection_matrix.m[3][3];

						// We can upload this matrix to the space taken up by the directional lighting details, since this render pass requires no lighting.
						D3DDevice_SetVertexShaderConstantFast( VSCONST_REG_DIR_LIGHT_OFFSET, (void*)texture_projection_matrix, 4 );

						// Scan through each mesh in this scene, setting the vertex shader to be the equivalent vertex shader
						// for shadow buffering.
						sScene *p_scene = GetScene();
						for( int m = 0; m < p_scene->m_num_mesh_entries; ++m )
						{
							sMesh *p_mesh = p_scene->m_meshes[m];
							if(( p_mesh->m_vertex_shader[0] == WeightedMeshVS_VXC_1Weight ) ||
							   ( p_mesh->m_vertex_shader[0] == WeightedMeshVS_VXC_Specular_1Weight ) ||
							   ( p_mesh->m_vertex_shader[0] == WeightedMeshVS_VXC_1Weight_UVTransform ))
							{
								p_mesh->PushVertexShader( WeightedMeshVS_VXC_1Weight_SBPassThru );
							}
							else if(( p_mesh->m_vertex_shader[0] == WeightedMeshVS_VXC_2Weight ) ||
									( p_mesh->m_vertex_shader[0] == WeightedMeshVS_VXC_Specular_2Weight ) ||
								    ( p_mesh->m_vertex_shader[0] == WeightedMeshVS_VXC_2Weight_UVTransform ))
							{
								p_mesh->PushVertexShader( WeightedMeshVS_VXC_2Weight_SBPassThru );
							}
							else
							{
								p_mesh->PushVertexShader( WeightedMeshVS_VXC_3Weight_SBPassThru );
							}
						}

						float max = 0.25f;
						if( dist < SHADOW_FADE_CLOSE_START )
						{
							max = (( dist - SHADOW_FADE_CLOSE_COMPLETE ) / ( SHADOW_FADE_CLOSE_START - SHADOW_FADE_CLOSE_COMPLETE )) * max;
						}
						else if( dist > SHADOW_FADE_FAR_START )
						{
							max = (( SHADOW_FADE_FAR_COMPLETE - dist ) / ( SHADOW_FADE_FAR_COMPLETE - SHADOW_FADE_FAR_START )) * max;
						}

						// Set ambient color multipliers in pixel shader C0 and C1.
						NxXbox::EngineGlobals.pixel_shader_constants[0]		= 1.0f - max;		// Always present
						NxXbox::EngineGlobals.pixel_shader_constants[1]		= 1.0f - max;
						NxXbox::EngineGlobals.pixel_shader_constants[2]		= 1.0f - max;
						NxXbox::EngineGlobals.pixel_shader_constants[3]		= 1.0f;
						NxXbox::EngineGlobals.pixel_shader_constants[4]		= max;		// The part the shadow affects.
						NxXbox::EngineGlobals.pixel_shader_constants[5]		= max;
						NxXbox::EngineGlobals.pixel_shader_constants[6]		= max;
						NxXbox::EngineGlobals.pixel_shader_constants[7]		= 1.0f;
						NxXbox::EngineGlobals.upload_pixel_shader_constants	= true;

						// Set the pixel shader that will do the shadow attenuation.			
						extern DWORD PixelShader_ShadowBuffer;
						set_pixel_shader( PixelShader_ShadowBuffer );
						EngineGlobals.pixel_shader_override = 1;

						// Set the material properties used for shadow modulation.
						set_blend_mode( vBLEND_MODE_MODULATE_COLOR );
						set_texture( 0, NULL );
						set_texture( 1, NULL );
						set_texture( 2, NULL );
						EngineGlobals.material_override = 1;

						// Set texture stage 3 to use the shadow buffer.
						EngineGlobals.texture_stage_override |= ( 1 << 3 );
						set_texture( 3, NULL );
						set_texture( 3, p_details->p_texture->pD3DTexture );
						set_render_state( RS_UVADDRESSMODE0 + 3, 0x00020002UL );
						D3DDevice_SetTextureStageState( 3, D3DTSS_BORDERCOLOR, 0x00000000UL );
						D3DDevice_SetTextureStageState( 3, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
						D3DDevice_SetTextureStageState( 3, D3DTSS_MINFILTER, D3DTEXF_LINEAR );

						D3DDevice_SetTextureStageState( 3, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
						D3DDevice_SetTextureStageState( 3, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU | 3 );

						// Set shadowbuffer state.
						D3DDevice_SetRenderState( D3DRS_SHADOWFUNC, D3DCMP_GREATER );

						// Render the shadow pass with fogging disabled.
						if( EngineGlobals.fog_enabled )
						{
							D3DDevice_SetRenderState( D3DRS_FOGENABLE, FALSE );
							render_scene( GetScene(), flags | vRENDER_NO_CULLING );
							D3DDevice_SetRenderState( D3DRS_FOGENABLE, TRUE );
						}
						else
						{
							render_scene( GetScene(), flags | vRENDER_NO_CULLING );
						}

						// Restore shadowbuffer state.
						D3DDevice_SetRenderState( D3DRS_SHADOWFUNC, D3DCMP_NEVER );

						// Scan through each mesh in this scene, restoring the vertex shader to the original.
						for( int m = 0; m < p_scene->m_num_mesh_entries; ++m )
						{
							sMesh *p_mesh = p_scene->m_meshes[m];
							p_mesh->PopVertexShader();
						}
					}
				}
			}
		}

		// Restore state.
		EngineGlobals.vertex_shader_override	= 0;
		EngineGlobals.pixel_shader_override		= 0;
		EngineGlobals.material_override			= 0;
		EngineGlobals.texture_stage_override	&= ~( 1 << 3 );
		set_texture( 3, NULL );

//		shutdown_weighted_mesh_vertex_shader();
	}
}



// Test code...
void CInstance::RenderShadowVolume( void )
{
	if( GetBoneTransforms())
	{
		XGMATRIX root_matrix = *((XGMATRIX*)GetTransform());

		sScene*	p_scene = GetScene();

		// Process each mesh in turn.
		for( int m = 0; m < p_scene->m_num_mesh_entries; ++m )
		{
			sMesh* p_mesh	= p_scene->m_meshes[m];

			// Lock the vertex buffer so we can read the data.
			BYTE* p_byte;
			p_mesh->mp_vertex_buffer[0]->Lock( 0, 0, &p_byte, D3DLOCK_READONLY );

			// Grab a buffer for the transformed points.
			XGVECTOR4*	p_t_buffer	= new XGVECTOR4[p_mesh->m_num_vertices];

			for( int v = 0; v < p_mesh->m_num_vertices; ++v )
			{
				// Get untransformed point.
				XGVECTOR3*	p_source_vertex	= (XGVECTOR3*)( p_byte + ( v * p_mesh->m_vertex_stride ));

				// Get and unpack weights.
				uint32		packed_weights	= *((uint32*)( p_byte + ( v * p_mesh->m_vertex_stride ) + 12 ));
				float		w0				= (( packed_weights >> 0 ) & 0x7ff ) * ( 1.0f / 1023.0f );
				float		w1				= (( packed_weights >> 11 ) & 0x7ff ) * ( 1.0f / 1023.0f );
				float		w2				= (( packed_weights >> 22 ) & 0x3ff ) * ( 1.0f / 511.0f );

				// Get and unpack matrix indices.
				uint32		packed_indices	= *((uint32*)( p_byte + ( v * p_mesh->m_vertex_stride ) + 16 ));
				uint32		i0				= ( packed_indices >> 0 ) & 0xff;
				uint32		i1				= ( packed_indices >> 8 ) & 0xff;
				uint32		i2				= ( packed_indices >> 16 ) & 0xff;

				// Get bone matrix pointers.
				XGMATRIX*	p_bone_mat0		= (XGMATRIX*)( GetBoneTransforms() + i0 );
				XGMATRIX*	p_bone_mat1		= (XGMATRIX*)( GetBoneTransforms() + i1 );
				XGMATRIX*	p_bone_mat2		= (XGMATRIX*)( GetBoneTransforms() + i2 );

				XGVECTOR4	tp0;
				XGVECTOR4	tp1;
				XGVECTOR4	tp2;

				// Transform point by bone matrix 0, 1 and 2 and scale with weight 0, 1, 2.
				XGVec3Transform( &tp0, p_source_vertex, p_bone_mat0 );
				XGVec3Scale((XGVECTOR3*)&tp0, (XGVECTOR3*)&tp0, w0 );

				XGVec3Transform( &tp1, p_source_vertex, p_bone_mat1 );
				XGVec3Scale((XGVECTOR3*)&tp1, (XGVECTOR3*)&tp1, w1 );

				XGVec3Transform( &tp2, p_source_vertex, p_bone_mat2 );
				XGVec3Scale((XGVECTOR3*)&tp2, (XGVECTOR3*)&tp2, w2 );

				// Obtain cumulative result.
				tp0 = tp0 + tp1 + tp2;

				// Tranform point by object transform.
				XGVec3Transform( p_t_buffer + v, (XGVECTOR3*)&tp0, &root_matrix );
			}
			delete [] p_t_buffer;
		}
	}
}






} // namespace NxXbox

