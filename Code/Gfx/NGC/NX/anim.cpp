#include <stdio.h>
#include <stdlib.h>

#include <core/defines.h>
#include <core/debug.h>
#include <core/HashTable.h>
#include <core/math.h>
#include <core/math/geometry.h>
#include <sys/file/filesys.h>

#include "nx_init.h"
#include "mesh.h"
#include "scene.h"
#include "anim.h"

namespace NxNgc
{
//	// Material color attenuation.
//	static DWORD WeightedMeshVertexShader0Decl[] = {
//	D3DVSD_STREAM( 0 ),
//	D3DVSD_REG( VSD_REG_POS,		D3DVSDT_FLOAT3 ),	// Position.
//	D3DVSD_REG( VSD_REG_WEIGHTS,	D3DVSDT_FLOAT4 ),	// Weights.
//	D3DVSD_REG( VSD_REG_INDICES,	D3DVSDT_SHORT4 ),	// Indices.
//	D3DVSD_REG( VSD_REG_NORMAL,		D3DVSDT_FLOAT3 ),	// Normals.
//	D3DVSD_REG( VSD_REG_TEXCOORDS,	D3DVSDT_FLOAT2 ),	// Texture coordinates.
//	D3DVSD_END() };
//
//	// Vertex color attenuation.
//	static DWORD WeightedMeshVertexShader1Decl[] = {
//	D3DVSD_STREAM( 0 ),
//	D3DVSD_REG( VSD_REG_POS,		D3DVSDT_FLOAT3 ),	// Position.
//	D3DVSD_REG( VSD_REG_WEIGHTS,	D3DVSDT_FLOAT4 ),	// Weights.
//	D3DVSD_REG( VSD_REG_INDICES,	D3DVSDT_SHORT4 ),	// Indices.
//	D3DVSD_REG( VSD_REG_NORMAL,		D3DVSDT_FLOAT3 ),	// Normals.
//	D3DVSD_REG( VSD_REG_COLOR,		D3DVSDT_D3DCOLOR ),	// Diffuse color.
//	D3DVSD_REG( VSD_REG_TEXCOORDS,	D3DVSDT_FLOAT2 ),	// Texture coordinates.
//	D3DVSD_END() };
//
//
//
//DWORD WeightedMeshVertexShader0;
//DWORD WeightedMeshVertexShader1;
//
//
////DWORD GetVertexShader( bool vertex_colors )
////{
////	if( vertex_colors )
////	{
////		return WeightedMeshVertexShader1;
////	}
////
////	return WeightedMeshVertexShader0;
////}
//
//

//void CreateWeightedMeshVertexShaders( void )
//{
//	static bool created_shaders = false;
//
//	if( !created_shaders )
//	{
//		created_shaders = true;
//
//		if( D3D_OK != D3DDevice_CreateVertexShader(	WeightedMeshVertexShader0Decl,
//													dwWeightedmeshvertexshader0VertexShader,	// Defined in the header file from xsasm.
//													&WeightedMeshVertexShader0,
//													0 ))
//		{
//			exit( 0 );
//		}
//
//		if( D3D_OK != D3DDevice_CreateVertexShader(	WeightedMeshVertexShader1Decl,
//													dwWeightedmeshvertexshader1VertexShader,	// Defined in the header file from xsasm.
//													&WeightedMeshVertexShader1,
//													0 ))
//		{
//			exit( 0 );
//		}
//	}
//}




//static void MatrixInvertOrthoNormalized( D3DXMATRIX *matrix, const D3DXMATRIX *matrixIn )
//{
//    /*
//     * Inverse of upper left 3x3 sub matrix
//     * is a simple transpose
//     */
////    matrix->right.x = matrixIn->right.x;
////    matrix->right.y = matrixIn->up.x;
////    matrix->right.z = matrixIn->at.x;
////    matrix->up.x = matrixIn->right.y;
////    matrix->up.y = matrixIn->up.y;
////    matrix->up.z = matrixIn->at.y;
////    matrix->at.x = matrixIn->right.z;
////    matrix->at.y = matrixIn->up.z;
////    matrix->at.z = matrixIn->at.z;
//	matrix->m[0][0] = matrixIn->m[0][0];
//	matrix->m[0][1] = matrixIn->m[1][0];
//	matrix->m[0][2] = matrixIn->m[2][0];
//	matrix->m[0][3] = 0.0f;
//	matrix->m[1][0] = matrixIn->m[0][1];
//	matrix->m[1][1] = matrixIn->m[1][1];
//	matrix->m[1][2] = matrixIn->m[2][1];
//	matrix->m[1][3] = 0.0f;
//	matrix->m[2][0] = matrixIn->m[0][2];
//	matrix->m[2][1] = matrixIn->m[1][2];
//	matrix->m[2][2] = matrixIn->m[2][2];
//	matrix->m[2][3] = 0.0f;
//
//    /*
//     * calculate translation componennt of inverse
//     */
////    matrix->pos.x = -RwV3dDotProductMacro(&matrixIn->pos, &matrixIn->right);
////    matrix->pos.y = -RwV3dDotProductMacro(&matrixIn->pos, &matrixIn->up);
////    matrix->pos.z = -RwV3dDotProductMacro(&matrixIn->pos, &matrixIn->at);
//    matrix->m[3][0] = -(( matrixIn->m[3][0] * matrixIn->m[0][0] ) + ( matrixIn->m[3][1] * matrixIn->m[0][1] ) + ( matrixIn->m[3][2] * matrixIn->m[0][2] ));
//    matrix->m[3][1] = -(( matrixIn->m[3][0] * matrixIn->m[1][0] ) + ( matrixIn->m[3][1] * matrixIn->m[1][1] ) + ( matrixIn->m[3][2] * matrixIn->m[1][2] ));
//    matrix->m[3][2] = -(( matrixIn->m[3][0] * matrixIn->m[2][0] ) + ( matrixIn->m[3][1] * matrixIn->m[2][1] ) + ( matrixIn->m[3][2] * matrixIn->m[2][2] ));
//    matrix->m[3][3] = 1.0f;
//}





//void setup_weighted_mesh_vertex_shader( void *p_root_matrix, void *p_bone_matrices, int num_bone_matrices )
//{
//	D3DXMATRIX	dest_matrix;
//	D3DXMATRIX	inverse_view_matrix;
//	D3DXMATRIX	temp_matrix;
//	D3DXMATRIX	projMatrix;
//	D3DXMATRIX	viewMatrix;
//	D3DXMATRIX	worldMatrix;
//
//	// Projection matrix.
//	projMatrix.m[0][0] = EngineGlobals.projection_matrix.m[0][0];
//    projMatrix.m[0][1] = EngineGlobals.projection_matrix.m[1][0];
//    projMatrix.m[0][2] = EngineGlobals.projection_matrix.m[2][0];
//    projMatrix.m[0][3] = EngineGlobals.projection_matrix.m[3][0];
//    projMatrix.m[1][0] = EngineGlobals.projection_matrix.m[0][1];
//	projMatrix.m[1][1] = EngineGlobals.projection_matrix.m[1][1];
//    projMatrix.m[1][2] = EngineGlobals.projection_matrix.m[2][1];
//    projMatrix.m[1][3] = EngineGlobals.projection_matrix.m[3][1];
//
//    projMatrix.m[2][0] = 0.0f;
//    projMatrix.m[2][1] = 0.0f;
////    projMatrix.m[2][2] = camera->farPlane / ( camera->farPlane - camera->nearPlane );
//    projMatrix.m[2][2] = 20000.0f / ( 20000.0f - 2.0f );
////	projMatrix.m[2][3] = -projMatrix.m[2][2] * camera->nearPlane;
//	projMatrix.m[2][3] = -projMatrix.m[2][2] * 2.0f;
//
//    projMatrix.m[3][0] = 0.0f;
//    projMatrix.m[3][1] = 0.0f;
//    projMatrix.m[3][2] = 1.0f;
//    projMatrix.m[3][3] = 0.0f;
//
//	// View matrix.
//	D3DXMATRIX	view_matrix = EngineGlobals.view_matrix;
//	
//	viewMatrix.m[0][0] = -view_matrix.m[0][0];
//    viewMatrix.m[0][1] = view_matrix.m[1][0];
//    viewMatrix.m[0][2] = -view_matrix.m[2][0];
//    viewMatrix.m[0][3] = view_matrix.m[3][0];
//    viewMatrix.m[1][0] = -view_matrix.m[0][1];
//    viewMatrix.m[1][1] = view_matrix.m[1][1];
//    viewMatrix.m[1][2] = -view_matrix.m[2][1];
//    viewMatrix.m[1][3] = view_matrix.m[3][1];
//    viewMatrix.m[2][0] = view_matrix.m[0][2];
//    viewMatrix.m[2][1] = -view_matrix.m[1][2];
//    viewMatrix.m[2][2] = view_matrix.m[2][2];
//    viewMatrix.m[2][3] = -view_matrix.m[3][2];
//    viewMatrix.m[3][0] = 0.0f;
//    viewMatrix.m[3][1] = 0.0f;
//    viewMatrix.m[3][2] = 0.0f;
//    viewMatrix.m[3][3] = 1.0f;
//	
//	// World space transformation matrix.
//	worldMatrix.m[0][0] = ((float*)p_root_matrix )[0];
//	worldMatrix.m[0][1] = ((float*)p_root_matrix )[1];
//	worldMatrix.m[0][2] = ((float*)p_root_matrix )[2];
//	worldMatrix.m[0][3] = ((float*)p_root_matrix )[3];
//    worldMatrix.m[1][0] = ((float*)p_root_matrix )[4];
//    worldMatrix.m[1][1] = ((float*)p_root_matrix )[5];
//    worldMatrix.m[1][2] = ((float*)p_root_matrix )[6];
//    worldMatrix.m[1][3] = ((float*)p_root_matrix )[7];
//    worldMatrix.m[2][0] = ((float*)p_root_matrix )[8];
//    worldMatrix.m[2][1] = ((float*)p_root_matrix )[9];
//    worldMatrix.m[2][2] = ((float*)p_root_matrix )[10];
//    worldMatrix.m[2][3] = ((float*)p_root_matrix )[11];
//    worldMatrix.m[3][0] = 0.0f;
//    worldMatrix.m[3][1] = 0.0f;
//    worldMatrix.m[3][2] = 0.0f;
//    worldMatrix.m[3][3] = 1.0f;
//
//	// Calculate composite world->view->projection matrix.
//	D3DXMatrixMultiply( &temp_matrix, &viewMatrix, &worldMatrix );
//	D3DXMatrixMultiply( &dest_matrix, &projMatrix, &temp_matrix );
//
//	// Switch to 192 constant mode, removing the lock on the reserved constants c-38 and c-37.
//	D3DDevice_SetShaderConstantMode( D3DSCM_192CONSTANTS | D3DSCM_NORESERVEDCONSTANTS );
//
//	// Load up the combined world, camera & projection matrix.
//	for( int i = 0; i < 16; ++i )
//	{
//		if( fabsf(((float*)dest_matrix.m )[i] ) < 0.001f )
//		{
//			((float*)dest_matrix.m )[i] = 0.0f;
//		}
//	}
//	
//	D3DDevice_SetVertexShaderConstant( VSCONST_REG_TRANSFORM_OFFSET, (void*)&dest_matrix, VSCONST_REG_TRANSFORM_SIZE );
//	D3DDevice_SetVertexShaderConstant( VSCONST_REG_WORLD_TRANSFORM_OFFSET,	(void*)&worldMatrix, VSCONST_REG_WORLD_TRANSFORM_SIZE );
//
//	// Load up the directional light data.
//	static float directional_light_color[24] = {
//	0.0f, -1.0f, 0.0f, 0.0f,	// Dir 0
//	0.7f, 0.6f, 0.5f, 1.0f,		// Col 0
//	1.0f, 0.0f, 0.0f, 0.0f,		// Dir 1
//	0.35f, 0.4f, 0.4f, 1.0f,		// Col 1
//	0.0f, 0.0f, 1.0f, 0.0f,		// Dir 2
//	0.3f, 0.3f, 0.2f, 1.0f };	// Col 2
//	
//	D3DDevice_SetVertexShaderConstant( VSCONST_REG_DIR_LIGHT_OFFSET,		(void*)directional_light_color, VSCONST_REG_DIR_LIGHT_SIZE );
//
//	// Load up the ambient light color.
//	static float ambient_light_color[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
//	D3DDevice_SetVertexShaderConstant( VSCONST_REG_AMB_LIGHT_OFFSET, (void*)ambient_light_color, 1 );
//	
//	// Load up the material color.
//	static float material_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
//	D3DDevice_SetVertexShaderConstant( VSCONST_REG_MATERIAL_OFFSET, (void*)material_color, 1 );
//
//	// Load up the bone transforms, in sets of 40.
//	int offset = 0;
//	while( num_bone_matrices > 0 )
//	{
//		if( num_bone_matrices >= 10 )
//		{
//			D3DDevice_SetVertexShaderConstant( VSCONST_REG_MATRIX_OFFSET + offset, (float*)p_bone_matrices + ( offset * 4 ), 10 * 3 );
//			offset += 10 * 3;
//			num_bone_matrices -= 10;
//		}
//		else
//		{
//			D3DDevice_SetVertexShaderConstant( VSCONST_REG_MATRIX_OFFSET + offset, (float*)p_bone_matrices + ( offset * 4 ), num_bone_matrices * 3 );
//			num_bone_matrices = 0;
//		}
//	}
//
//	// Load up the replacement registers for c-38 and c-37.
//	static float homogenous_to_screen_reg[8] = { 320.0f, -240.0f, 1.67772e7f, 0.0f, 320.03125f, 240.03125f, 0.0f, 0.0f };
//	D3DDevice_SetVertexShaderConstant( 94, (void*)homogenous_to_screen_reg, 2 );
//}



//void shutdown_weighted_mesh_vertex_shader( void )
//{
//	// Switch back to 96 constant mode.
//	D3DDevice_SetShaderConstantMode( D3DSCM_96CONSTANTS );
//}


} // namespace NxNgc