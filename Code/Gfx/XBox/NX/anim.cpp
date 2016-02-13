#include <xtl.h>

#include <stdio.h>
#include <stdlib.h>

#include <core/defines.h>
#include <core/debug.h>
#include <core/HashTable.h>
#include <core/math.h>
#include <core/math/geometry.h>
#include <sys/file/filesys.h>
#include <sys/timer.h>

#include "nx_init.h"
#include "mesh.h"
#include "scene.h"
#include "anim.h"
#include "anim_vertdefs.h"

#include "WeightedMeshVS_VXC_1Weight.h"
#include "WeightedMeshVS_VXC_2Weight.h"
#include "WeightedMeshVS_VXC_3Weight.h"

#include "WeightedMeshVS_VXC_Specular_1Weight.h"
#include "WeightedMeshVS_VXC_Specular_2Weight.h"
#include "WeightedMeshVS_VXC_Specular_3Weight.h"

#include "WeightedMeshVS_VXC_1Weight_UVTransform.h"
#include "WeightedMeshVS_VXC_2Weight_UVTransform.h"
#include "WeightedMeshVS_VXC_3Weight_UVTransform.h"

#include "WeightedMeshVS_VXC_1Weight_SBPassThru.h"
#include "WeightedMeshVS_VXC_2Weight_SBPassThru.h"
#include "WeightedMeshVS_VXC_3Weight_SBPassThru.h"

#include "WeightedMeshVertexShader_SBWrite.h"
#include "ShadowBufferStaticGeomVS.h"
#include "BillboardScreenAlignedVS.h"
#include "ParticleFlatVS.h"
#include "ParticleNewFlatVS.h"
#include "ParticleNewFlatPointSpriteVS.h"

DWORD WeightedMeshVS_VXC_1Weight;
DWORD WeightedMeshVS_VXC_2Weight;
DWORD WeightedMeshVS_VXC_3Weight;
DWORD WeightedMeshVS_VXC_Specular_1Weight;
DWORD WeightedMeshVS_VXC_Specular_2Weight;
DWORD WeightedMeshVS_VXC_Specular_3Weight;
DWORD WeightedMeshVS_VXC_1Weight_UVTransform;
DWORD WeightedMeshVS_VXC_2Weight_UVTransform;
DWORD WeightedMeshVS_VXC_3Weight_UVTransform;
DWORD WeightedMeshVertexShader_SBWrite;
DWORD WeightedMeshVS_VXC_1Weight_SBPassThru;
DWORD WeightedMeshVS_VXC_2Weight_SBPassThru;
DWORD WeightedMeshVS_VXC_3Weight_SBPassThru;
DWORD WeightedMeshVertexShader_VXC_SBPassThru;
DWORD BillboardScreenAlignedVS;
DWORD ParticleFlatVS;
DWORD ParticleNewFlatVS;
DWORD ParticleNewFlatPointSpriteVS;
DWORD ShadowBufferStaticGeomVS;

namespace NxXbox
{
	// Vertex color attenuation, 4 sets of tex coords.
	static DWORD WeightedMeshVertexShaderVertColUV4Decl[] = {
	D3DVSD_STREAM( 0 ),
	D3DVSD_REG( VSD_REG_POS,		D3DVSDT_FLOAT3 ),		// Position.
	D3DVSD_REG( VSD_REG_WEIGHTS,	D3DVSDT_NORMPACKED3 ),	// Weights.
	D3DVSD_REG( VSD_REG_INDICES,	D3DVSDT_SHORT4 ),		// Indices.
	D3DVSD_REG( VSD_REG_NORMAL,		D3DVSDT_NORMPACKED3 ),	// Normals.
	D3DVSD_REG( VSD_REG_COLOR,		D3DVSDT_D3DCOLOR ),		// Diffuse color.
	D3DVSD_REG( VSD_REG_TEXCOORDS0,	D3DVSDT_FLOAT2 ),		// Texture coordinates 0.
	D3DVSD_REG( VSD_REG_TEXCOORDS1,	D3DVSDT_FLOAT2 ),		// Texture coordinates 1.
	D3DVSD_REG( VSD_REG_TEXCOORDS2,	D3DVSDT_FLOAT2 ),		// Texture coordinates 2.
	D3DVSD_REG( VSD_REG_TEXCOORDS3,	D3DVSDT_FLOAT2 ),		// Texture coordinates 3.
	D3DVSD_END() };
	
	// Billboards.
	static DWORD BillboardVSDecl[] = {
	D3DVSD_STREAM( 0 ),
	D3DVSD_REG( 0,	D3DVSDT_FLOAT3 ),		// Position (actually pivot position).
	D3DVSD_REG( 1,	D3DVSDT_FLOAT3 ),		// Normal (actually position of point relative to pivot).
	D3DVSD_REG( 2,	D3DVSDT_D3DCOLOR ),		// Diffuse color.
	D3DVSD_REG( 3,	D3DVSDT_FLOAT2 ),		// Texture coordinates 0.
	D3DVSD_REG( 4,	D3DVSDT_FLOAT2 ),		// Texture coordinates 1.
	D3DVSD_REG( 5,	D3DVSDT_FLOAT2 ),		// Texture coordinates 2.
	D3DVSD_REG( 6,	D3DVSDT_FLOAT2 ),		// Texture coordinates 3.
	D3DVSD_END() };

	// Particles.
	static DWORD ParticleFlatVSDecl[] = {
	D3DVSD_STREAM( 0 ),
	D3DVSD_REG( 0,	D3DVSDT_D3DCOLOR ),		// Diffuse color (start)
	D3DVSD_REG( 1,	D3DVSDT_D3DCOLOR ),		// Diffuse color (end)
	D3DVSD_REG( 2,	D3DVSDT_SHORT2 ),		// Indices.
	D3DVSD_END() };

	// New, Ps2 style particles using PointSprites.
	static DWORD NewParticleFlatVSDecl[] = {
	D3DVSD_STREAM( 0 ),
	D3DVSD_REG( 0,	D3DVSDT_FLOAT4 ),		// Random 4-element 'R' vector.
	D3DVSD_REG( 1,	D3DVSDT_FLOAT2 ),		// Time and color interpolator.
	D3DVSD_REG( 2,	D3DVSDT_D3DCOLOR ),		// Diffuse color (start)
	D3DVSD_REG( 3,	D3DVSDT_D3DCOLOR ),		// Diffuse color (end)
	D3DVSD_END() };

	// Shadow buffer, static geom.
	static DWORD ShadowBufferStaticGeomVSDecl[] = {
	D3DVSD_STREAM( 0 ),
	D3DVSD_REG( 0,	D3DVSDT_FLOAT3 ),		// Position.
	D3DVSD_REG( 1,	D3DVSDT_D3DCOLOR ),		// Diffuse color.
	D3DVSD_REG( 2,	D3DVSDT_FLOAT2 ),		// Texture coordinates 0.
	D3DVSD_REG( 3,	D3DVSDT_FLOAT2 ),		// Texture coordinates 1.
	D3DVSD_REG( 4,	D3DVSDT_FLOAT2 ),		// Texture coordinates 2.
	D3DVSD_END() };
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
DWORD GetVertexShader( bool vertex_colors, bool specular, uint32 max_weights_used )
{
	Dbg_Assert( max_weights_used > 0 );
	
	if( vertex_colors )
	{
		if( max_weights_used == 1 )
		{
			return ( specular ) ? WeightedMeshVS_VXC_Specular_1Weight : WeightedMeshVS_VXC_1Weight;
		}
		else if( max_weights_used == 2 )
		{
			return ( specular ) ? WeightedMeshVS_VXC_Specular_2Weight : WeightedMeshVS_VXC_2Weight;
		}
		else if( max_weights_used == 3 )
		{
			return ( specular ) ? WeightedMeshVS_VXC_Specular_3Weight : WeightedMeshVS_VXC_3Weight;
		}
	}

	Dbg_Assert( 0 );
	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CreateWeightedMeshVertexShaders( void )
{
	static bool created_shaders = false;

	if( !created_shaders )
	{
		created_shaders = true;

		if( D3D_OK != D3DDevice_CreateVertexShader(	WeightedMeshVertexShaderVertColUV4Decl,
													dwWeightedMeshVS_VXC_1WeightVertexShader,	// Defined in the header file from xsasm.
													&WeightedMeshVS_VXC_1Weight,
													0 ))
		{
			exit( 0 );
		}
		if( D3D_OK != D3DDevice_CreateVertexShader(	WeightedMeshVertexShaderVertColUV4Decl,
													dwWeightedMeshVS_VXC_2WeightVertexShader,	// Defined in the header file from xsasm.
													&WeightedMeshVS_VXC_2Weight,
													0 ))
		{
			exit( 0 );
		}
		if( D3D_OK != D3DDevice_CreateVertexShader(	WeightedMeshVertexShaderVertColUV4Decl,
													dwWeightedMeshVS_VXC_3WeightVertexShader,	// Defined in the header file from xsasm.
													&WeightedMeshVS_VXC_3Weight,
													0 ))
		{
			exit( 0 );
		}

		if( D3D_OK != D3DDevice_CreateVertexShader(	WeightedMeshVertexShaderVertColUV4Decl,
													dwWeightedMeshVS_VXC_Specular_1WeightVertexShader,	// Defined in the header file from xsasm.
													&WeightedMeshVS_VXC_Specular_1Weight,
													0 ))
		{
			exit( 0 );
		}
		if( D3D_OK != D3DDevice_CreateVertexShader(	WeightedMeshVertexShaderVertColUV4Decl,
													dwWeightedMeshVS_VXC_Specular_2WeightVertexShader,	// Defined in the header file from xsasm.
													&WeightedMeshVS_VXC_Specular_2Weight,
													0 ))
		{
			exit( 0 );
		}
		if( D3D_OK != D3DDevice_CreateVertexShader(	WeightedMeshVertexShaderVertColUV4Decl,
													dwWeightedMeshVS_VXC_Specular_3WeightVertexShader,	// Defined in the header file from xsasm.
													&WeightedMeshVS_VXC_Specular_3Weight,
													0 ))
		{
			exit( 0 );
		}

		if( D3D_OK != D3DDevice_CreateVertexShader(	WeightedMeshVertexShaderVertColUV4Decl,
													dwWeightedMeshVS_VXC_1Weight_UVTransformVertexShader,	// Defined in the header file from xsasm.
													&WeightedMeshVS_VXC_1Weight_UVTransform,
													0 ))
		{
			exit( 0 );
		}

		if( D3D_OK != D3DDevice_CreateVertexShader(	WeightedMeshVertexShaderVertColUV4Decl,
													dwWeightedMeshVS_VXC_2Weight_UVTransformVertexShader,	// Defined in the header file from xsasm.
													&WeightedMeshVS_VXC_2Weight_UVTransform,
													0 ))
		{
			exit( 0 );
		}

		if( D3D_OK != D3DDevice_CreateVertexShader(	WeightedMeshVertexShaderVertColUV4Decl,
													dwWeightedMeshVS_VXC_3Weight_UVTransformVertexShader,	// Defined in the header file from xsasm.
													&WeightedMeshVS_VXC_3Weight_UVTransform,
													0 ))
		{
			exit( 0 );
		}

		if( D3D_OK != D3DDevice_CreateVertexShader(	WeightedMeshVertexShaderVertColUV4Decl,
													dwWeightedMeshVertexShader_SBWriteVertexShader,	// Defined in the header file from xsasm.
													&WeightedMeshVertexShader_SBWrite,
													0 ))
		{
			exit( 0 );
		}

		if( D3D_OK != D3DDevice_CreateVertexShader(	WeightedMeshVertexShaderVertColUV4Decl,
													dwWeightedMeshVS_VXC_1Weight_SBPassThruVertexShader,	// Defined in the header file from xsasm.
													&WeightedMeshVS_VXC_1Weight_SBPassThru,
													0 ))
		{
			exit( 0 );
		}

		if( D3D_OK != D3DDevice_CreateVertexShader(	WeightedMeshVertexShaderVertColUV4Decl,
													dwWeightedMeshVS_VXC_2Weight_SBPassThruVertexShader,	// Defined in the header file from xsasm.
													&WeightedMeshVS_VXC_2Weight_SBPassThru,
													0 ))
		{
			exit( 0 );
		}

		if( D3D_OK != D3DDevice_CreateVertexShader(	WeightedMeshVertexShaderVertColUV4Decl,
													dwWeightedMeshVS_VXC_3Weight_SBPassThruVertexShader,	// Defined in the header file from xsasm.
													&WeightedMeshVS_VXC_3Weight_SBPassThru,
													0 ))
		{
			exit( 0 );
		}

		if( D3D_OK != D3DDevice_CreateVertexShader(	BillboardVSDecl,
													dwBillboardScreenAlignedVSVertexShader,					// Defined in the header file from xsasm.
													&BillboardScreenAlignedVS,
													0 ))
		{
			exit( 0 );
		}


		if( D3D_OK != D3DDevice_CreateVertexShader(	ParticleFlatVSDecl,
													dwParticleFlatVSVertexShader,							// Defined in the header file from xsasm.
													&ParticleFlatVS,
													0 ))
		{
			exit( 0 );
		}

		if( D3D_OK != D3DDevice_CreateVertexShader(	ParticleFlatVSDecl,
													dwParticleNewFlatVSVertexShader,						// Defined in the header file from xsasm.
													&ParticleNewFlatVS,
													0 ))
		{
			exit( 0 );
		}

		if( D3D_OK != D3DDevice_CreateVertexShader(	NewParticleFlatVSDecl,
													dwParticleNewFlatPointSpriteVSVertexShader,				// Defined in the header file from xsasm.
													&ParticleNewFlatPointSpriteVS,
													0 ))
		{
			exit( 0 );
		}
		if( D3D_OK != D3DDevice_CreateVertexShader(	ShadowBufferStaticGeomVSDecl,
													dwShadowBufferStaticGeomVSVertexShader,					// Defined in the header file from xsasm.
													&ShadowBufferStaticGeomVS,
													0 ))
		{
			exit( 0 );
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void setup_weighted_mesh_vertex_shader( void *p_root_matrix, void *p_bone_matrices, int num_bone_matrices )
{
	XGMATRIX	dest_matrix;
	XGMATRIX	inverse_view_matrix;
	XGMATRIX	temp_matrix;
	XGMATRIX	projMatrix;
	XGMATRIX	viewMatrix;
	XGMATRIX	worldMatrix;

	// Projection matrix.
	XGMatrixTranspose( &projMatrix, &EngineGlobals.projection_matrix );
	
	// View matrix.
	XGMatrixTranspose( &viewMatrix, &EngineGlobals.view_matrix );
    viewMatrix.m[3][0] = 0.0f;
    viewMatrix.m[3][1] = 0.0f;
    viewMatrix.m[3][2] = 0.0f;
    viewMatrix.m[3][3] = 1.0f;
	
	// World space transformation matrix. (3x3 rotation plus 3 element translation component).
	worldMatrix.m[0][0] = ((float*)p_root_matrix )[0];
	worldMatrix.m[0][1] = ((float*)p_root_matrix )[1];
	worldMatrix.m[0][2] = ((float*)p_root_matrix )[2];
	worldMatrix.m[0][3] = ((float*)p_root_matrix )[3];
	worldMatrix.m[1][0] = ((float*)p_root_matrix )[4];
	worldMatrix.m[1][1] = ((float*)p_root_matrix )[5];
	worldMatrix.m[1][2] = ((float*)p_root_matrix )[6];
	worldMatrix.m[1][3] = ((float*)p_root_matrix )[7];
	worldMatrix.m[2][0] = ((float*)p_root_matrix )[8];
	worldMatrix.m[2][1] = ((float*)p_root_matrix )[9];
	worldMatrix.m[2][2] = ((float*)p_root_matrix )[10];
	worldMatrix.m[2][3] = ((float*)p_root_matrix )[11];
	worldMatrix.m[3][0] = 0.0f;
	worldMatrix.m[3][1] = 0.0f;
	worldMatrix.m[3][2] = 0.0f;
	worldMatrix.m[3][3] = 1.0f;

	// Calculate composite world->view->projection matrix.
	XGMatrixMultiply( &temp_matrix, &viewMatrix, &worldMatrix );
	XGMatrixMultiply( &dest_matrix, &projMatrix, &temp_matrix );

	// Switch to 192 constant mode, removing the lock on the reserved constants c-38 and c-37.
//	D3DDevice_SetShaderConstantMode( D3DSCM_192CONSTANTS | D3DSCM_NORESERVEDCONSTANTS );

	// Load up the combined world, camera & projection matrix.
	D3DDevice_SetVertexShaderConstantFast( VSCONST_REG_TRANSFORM_OFFSET, (void*)&dest_matrix, VSCONST_REG_TRANSFORM_SIZE );
	D3DDevice_SetVertexShaderConstantFast( VSCONST_REG_WORLD_TRANSFORM_OFFSET,	(void*)&worldMatrix, VSCONST_REG_WORLD_TRANSFORM_SIZE );

	// We want to transform the light directions by the inverse of the world transform - this means we don't have to transform
	// the normal by the world transform for every vertex in the vertex shader. However, the function D3DXVec3TransformNormal
	// (used below) does the inverse transform for us, so need to actually figure the inverse...
//	XGMATRIX inverse_world_transform = worldMatrix;
//	D3DXMatrixInverse( &inverse_world_transform, NULL, &worldMatrix );

	float directional_light_color[24];
	CopyMemory( directional_light_color, EngineGlobals.directional_light_color, sizeof( float ) * 24 );
	
	XGVec3TransformNormal((XGVECTOR3*)&directional_light_color[0],	(XGVECTOR3*)&EngineGlobals.directional_light_color[0], &worldMatrix );
	XGVec3TransformNormal((XGVECTOR3*)&directional_light_color[8],	(XGVECTOR3*)&EngineGlobals.directional_light_color[8], &worldMatrix );
	XGVec3TransformNormal((XGVECTOR3*)&directional_light_color[16],	(XGVECTOR3*)&EngineGlobals.directional_light_color[16], &worldMatrix );
	
	XGVec3Normalize((XGVECTOR3*)&directional_light_color[0], (XGVECTOR3*)&directional_light_color[0] ); 
	XGVec3Normalize((XGVECTOR3*)&directional_light_color[8], (XGVECTOR3*)&directional_light_color[8] ); 
	XGVec3Normalize((XGVECTOR3*)&directional_light_color[16], (XGVECTOR3*)&directional_light_color[16] ); 

	// Load up the directional light data.
	D3DDevice_SetVertexShaderConstantFast( VSCONST_REG_DIR_LIGHT_OFFSET, (void*)directional_light_color, 6 );

	// Load up the ambient light color.
	D3DDevice_SetVertexShaderConstantFast( VSCONST_REG_AMB_LIGHT_OFFSET, (void*)EngineGlobals.ambient_light_color, 1 );
	
	// Calculate and load up the model-relative camera position.
	EngineGlobals.model_relative_cam_position = XGVECTOR3( EngineGlobals.cam_position.x - worldMatrix.m[0][3],
														   EngineGlobals.cam_position.y - worldMatrix.m[1][3],
														   EngineGlobals.cam_position.z - worldMatrix.m[2][3] );
	XGVec3TransformNormal( &EngineGlobals.model_relative_cam_position, &EngineGlobals.model_relative_cam_position, &worldMatrix );

	float specular_attribs[4] = { EngineGlobals.model_relative_cam_position.x,
								  EngineGlobals.model_relative_cam_position.y,
								  EngineGlobals.model_relative_cam_position.z,
								  0.0f };
	D3DDevice_SetVertexShaderConstantFast( VSCONST_REG_CAM_POS_OFFSET, (void*)&specular_attribs, 1 );

	// Safety check here to limit number of bones to that available.
	if( num_bone_matrices > 55 )
	{
		num_bone_matrices = 55;
	}

	DWORD*	p_bone_element	= (DWORD*)p_bone_matrices;

	// Begin state block to set vertex shader constants for bone transforms.
	DWORD *p_push;
	EngineGlobals.p_Device->BeginState(( num_bone_matrices * ( 12 + 1 )) + 3, &p_push );

	// 1 here isn't the parameter for SET_TRANSFORM_CONSTANT_LOAD; rather, it's the number of dwords written to that register.
	p_push[0] = D3DPUSH_ENCODE( D3DPUSH_SET_TRANSFORM_CONSTANT_LOAD, 1 ); 

	// Here is the actual parameter for SET_TRANSFORM_CONSTANT_LOAD. Always add 96 to the constant register.
	p_push[1] = VSCONST_REG_MATRIX_OFFSET + 96;

	p_push += 2;

	while( num_bone_matrices > 0 )
	{
		// A 3x4 matrix is 12 dwords. You can encode a maximum of 32 dwords to D3DPUSH_SET_TRANSFORM_CONSTANT before needing another D3DPUSH_ENCODE.
		p_push[0]		= D3DPUSH_ENCODE( D3DPUSH_SET_TRANSFORM_CONSTANT, 12 );

		p_push[1]		= p_bone_element[0];
		p_push[2]		= p_bone_element[4];
		p_push[3]		= p_bone_element[8];
		p_push[4]		= p_bone_element[12];

		p_push[5]		= p_bone_element[1];
		p_push[6]		= p_bone_element[5];
		p_push[7]		= p_bone_element[9];
		p_push[8]		= p_bone_element[13];

		p_push[9]		= p_bone_element[2];
		p_push[10]		= p_bone_element[6];
		p_push[11]		= p_bone_element[10];
		p_push[12]		= p_bone_element[14];

		--num_bone_matrices;

		p_bone_element	+= 16;
		p_push			+= 13;
	}

	EngineGlobals.p_Device->EndState( p_push );

	// Load up the replacement registers for c-38 and c-37
	// The z value is 2^24 - to take 1.0f to the max z buffer value for a 24bit z buffer.
	static float homogenous_to_screen_reg[8] = { 320.0f, -240.0f, 1.6777215e7f, 0.0f, 320.53125f, 240.53125f, 0.0f, 0.0f };
	
	if( EngineGlobals.is_orthographic )
	{
		homogenous_to_screen_reg[0] = 128.0f;
		homogenous_to_screen_reg[1] = -128.0f;

		homogenous_to_screen_reg[2] = 1.6777215e7f;
		
		homogenous_to_screen_reg[4] = 128.53125f;
		homogenous_to_screen_reg[5] = 128.53125f;
	}
	else
	{
		homogenous_to_screen_reg[0] = (float)EngineGlobals.viewport.Width * 0.5f;
		homogenous_to_screen_reg[1] = (float)EngineGlobals.viewport.Height * -0.5f;

		homogenous_to_screen_reg[2] = ( EngineGlobals.zstencil_depth == 16 ) ? 65535.0f : 1.6777215e7f;

		homogenous_to_screen_reg[4] = (float)NxXbox::EngineGlobals.viewport.X + ((float)NxXbox::EngineGlobals.viewport.Width * 0.5f ) + 0.53125f;
		homogenous_to_screen_reg[5] = (float)NxXbox::EngineGlobals.viewport.Y + ((float)NxXbox::EngineGlobals.viewport.Height * 0.5f ) + 0.53125f;
	}

	D3DDevice_SetVertexShaderConstantFast( 94, (void*)homogenous_to_screen_reg, 2 );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void startup_weighted_mesh_vertex_shader( void )
{
	// Switch to 192 constant mode, removing the lock on the reserved constants c-38 and c-37.
	D3DDevice_SetShaderConstantMode( D3DSCM_192CONSTANTS | D3DSCM_NORESERVEDCONSTANTS );

	// Flag the custom pipeline is in operation.
	EngineGlobals.custom_pipeline_enabled = true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void shutdown_weighted_mesh_vertex_shader( void )
{
	// Switch back to 96 constant mode.
	D3DDevice_SetShaderConstantMode( D3DSCM_96CONSTANTS );

	// Flag the custom pipeline is no longer in operation.
	EngineGlobals.custom_pipeline_enabled = false;
}


} // namespace NxXbox