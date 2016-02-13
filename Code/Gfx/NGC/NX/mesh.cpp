#include <sys/file/filesys.h>
#include <sys/timer.h>

#include <stdio.h>
#include <stdlib.h>
#include "import.h"
#include "scene.h"
#include "mesh.h"
#include "nx_init.h"
#include "sys/ngc/p_aram.h"
#include "sys/ngc/p_dma.h"
#include <sk/modules/skate/skate.h>

#include <dolphin.h>

extern bool gCorrectColor;

namespace NxNgc
{

extern sint32 *pVertex;
int TotalNumVertices;

uint NumMeshes;


// Globals used for cutscene head scaling.
bool			s_meshScalingEnabled	= false;
char*			s_pWeightIndices		= NULL;
float*			s_pWeights				= NULL;
Mth::Vector*	s_pBonePositions		= NULL;
Mth::Vector*	s_pBoneScales			= NULL;
int				s_currentVertIndex		= 0;


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SetMeshScalingParameters( Nx::SMeshScalingParameters* pParams )
{
	Dbg_Assert( pParams );

	s_meshScalingEnabled	= true;
	s_pWeights				= pParams->pWeights;
	s_pWeightIndices		= pParams->pWeightIndices;
	s_pBoneScales			= pParams->pBoneScales;
	s_pBonePositions		= pParams->pBonePositions;
	s_currentVertIndex		= 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void DisableMeshScaling( void )
{
	s_meshScalingEnabled	= false;
	s_pWeights				= NULL;
	s_pWeightIndices		= NULL;
	s_pBoneScales			= NULL;
	s_pBonePositions		= NULL;
	s_currentVertIndex		= 0;
}

	
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static inline Mth::Vector get_bone_scale( int bone_index )
{
	Mth::Vector returnVec( 1.0f, 1.0f, 1.0f, 1.0f );

	if( bone_index >= 29 && bone_index <= 33 )
	{
		// this only works with the thps5 skeleton, whose
		// head bones are between 29 and 33...
		// (eventually, we can remove the subtract 29
		// once the exporter is massaging the data correctly)
		returnVec = s_pBoneScales[ bone_index - 29 ];
		
		// Y & Z are reversed...  odd!
		Mth::Vector tempVec = returnVec;
		returnVec[Y] = tempVec[Z];
		returnVec[Z] = tempVec[Y];
	}
	else if( bone_index == -1 )
	{
		// implies that it's not weighted to a bone
		return returnVec;
	}
	else
	{
		// implies that it's weighted to the wrong bone
		return returnVec;
	}
	return returnVec;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static inline Mth::Vector get_bone_pos( int bone_index )
{
	Mth::Vector returnVec( 0.0f, 0.0f, 0.0f, 1.0f );
	
	if( bone_index >= 29 && bone_index <= 33 )
	{
		// this only works with the thps5 skeleton, whose
		// head bones are between 29 and 33...
		// (eventually, we can remove the subtract 29
		// once the exporter is massaging the data correctly)
		returnVec = s_pBonePositions[ bone_index - 29 ];
	}
	else if( bone_index == -1 )
	{
		// implies that it's not weighted to a bone
		return returnVec;
	}
	else
	{
		// implies that it's weighted to the wrong bone
		return returnVec;
	}
	returnVec[W] = 1.0f;

	return returnVec;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void ApplyMeshScaling( NxNgc::sObjectHeader * p_object )
{
	if( s_meshScalingEnabled )
	{
		float orig_pool[3*1500];
		int pool_size = 0;
	
		// First, build an array from the skin data.
	
		// Singles.
		uint32*	p32 = (uint32 *)p_object->m_skin.p_data;
		for ( uint32 lp = 0; lp < p_object->m_num_single_lists; lp++ ) {
			uint32		pairs = *p32++;
			/*uint32		mtx = **/p32++;
			s16*		p_pn = (s16 *)p32;
	
			for ( uint32 lp2 = 0; lp2 < pairs; lp2++ )
			{
				orig_pool[(pool_size*3)+0] = (float)p_pn[(lp2*6)+0] / (float)(1 << 6);
				orig_pool[(pool_size*3)+1] = (float)p_pn[(lp2*6)+1] / (float)(1 << 6);
				orig_pool[(pool_size*3)+2] = (float)p_pn[(lp2*6)+2] / (float)(1 << 6);
				pool_size++;
			}
	
			p32 = (uint32 *)&p_pn[pairs*6];
		}
	
		// Doubles.
		for ( uint32 lp = 0; lp < p_object->m_num_double_lists; lp++ ) {
			uint32		pairs = *p32++;
			/*uint32		mtx = **/p32++;
			s16*		p_pn = (s16 *)p32;
			s16*		p_weight = (s16 *)&p_pn[(6*pairs)];
	
			for ( uint32 lp2 = 0; lp2 < pairs; lp2++ )
			{
				orig_pool[(pool_size*3)+0] = (float)p_pn[(lp2*6)+0] / (float)(1 << 6);
				orig_pool[(pool_size*3)+1] = (float)p_pn[(lp2*6)+1] / (float)(1 << 6);
				orig_pool[(pool_size*3)+2] = (float)p_pn[(lp2*6)+2] / (float)(1 << 6);
				pool_size++;
			}
	
			p32 = (uint32 *)&p_weight[pairs*2];
		}
	
		// Now scale the verts.
		float * p_vertices = orig_pool;
		for( int v = 0; v < pool_size; ++v, p_vertices += 3 )
		{
			float x = p_vertices[0];
			float y = p_vertices[1];
			float z = p_vertices[2];

    		Mth::Vector origPos( x, y, z, 1.0f );

			Mth::Vector bonePos0 = get_bone_pos( s_pWeightIndices[v * 3] );
			Mth::Vector bonePos1 = get_bone_pos( s_pWeightIndices[v * 3 + 1] );
			Mth::Vector bonePos2 = get_bone_pos( s_pWeightIndices[v * 3 + 2] );

			// Need to scale each vert relative to its parent bone.
			Mth::Vector localPos0 = origPos - bonePos0;
			Mth::Vector localPos1 = origPos - bonePos1;
			Mth::Vector localPos2 = origPos - bonePos2;

			localPos0.Scale( get_bone_scale( s_pWeightIndices[v * 3] ) );
			localPos1.Scale( get_bone_scale( s_pWeightIndices[v * 3 + 1] ) );
			localPos2.Scale( get_bone_scale( s_pWeightIndices[v * 3 + 2] ) );

			localPos0 += bonePos0;
			localPos1 += bonePos1;
			localPos2 += bonePos2;
			
			Mth::Vector scaledPos = ( localPos0 * s_pWeights[v * 3] ) +
									( localPos1 * s_pWeights[v * 3 + 1] ) +
									( localPos2 * s_pWeights[v * 3 + 2] );

			p_vertices[0] = scaledPos[X];
			p_vertices[1] = scaledPos[Y];
			p_vertices[2] = scaledPos[Z];
		} 	

		// Now copy back.
		pool_size = 0;
	
		// Singles.
		p32 = (uint32 *)p_object->m_skin.p_data;
		for ( uint32 lp = 0; lp < p_object->m_num_single_lists; lp++ ) {
			uint32		pairs = *p32++;
			/*uint32		mtx = **/p32++;
			s16*		p_pn = (s16 *)p32;
	
			for ( uint32 lp2 = 0; lp2 < pairs; lp2++ )
			{
				p_pn[(lp2*6)+0] = (s16)(orig_pool[(pool_size*3)+0] * (float)(1 << 6 ));
				p_pn[(lp2*6)+1] = (s16)(orig_pool[(pool_size*3)+1] * (float)(1 << 6 ));
				p_pn[(lp2*6)+2] = (s16)(orig_pool[(pool_size*3)+2] * (float)(1 << 6 ));
				pool_size++;
			}
	
			p32 = (uint32 *)&p_pn[pairs*6];
		}
	
		// Doubles.
		for ( uint32 lp = 0; lp < p_object->m_num_double_lists; lp++ ) {
			uint32		pairs = *p32++;
			/*uint32		mtx = **/p32++;
			s16*		p_pn = (s16 *)p32;
			s16*		p_weight = (s16 *)&p_pn[(6*pairs)];
	
			for ( uint32 lp2 = 0; lp2 < pairs; lp2++ )
			{
				p_pn[(lp2*6)+0] = (s16)(orig_pool[(pool_size*3)+0] * (float)(1 << 6 ));
				p_pn[(lp2*6)+1] = (s16)(orig_pool[(pool_size*3)+1] * (float)(1 << 6 ));
				p_pn[(lp2*6)+2] = (s16)(orig_pool[(pool_size*3)+2] * (float)(1 << 6 ));
				pool_size++;
			}
	
			p32 = (uint32 *)&p_weight[pairs*2];
		}
	
		// Accumulation.
		for ( uint32 lp = 0; lp < p_object->m_num_add_lists; lp++ ) {
			uint32		singles = *p32++;
			/*uint32		mtx = **/p32++;
			s16*		p_pn = (s16 *)p32;
			s16*		p_weight = (s16 *)&p_pn[(6*singles)];
			uint16*		p_index = (uint16 *)&p_weight[singles];
	
			for ( uint32 lp2 = 0; lp2 < singles; lp2++ )
			{
				int index = p_index[lp2];
				p_pn[(lp2*6)+0] = (s16)(orig_pool[(index*3)+0] * (float)(1 << 6 ));
				p_pn[(lp2*6)+1] = (s16)(orig_pool[(index*3)+1] * (float)(1 << 6 ));
				p_pn[(lp2*6)+2] = (s16)(orig_pool[(index*3)+2] * (float)(1 << 6 ));
			}
	
			p32 = (uint32 *)&p_index[singles];
		}
	}
}








sMesh::sMesh( void )
{
	m_flags				= 0;
	SetActive( true );
//	mp_vc_wibble_data	= NULL;
//
//	m_offset_x = 0.0f;
//	m_offset_y = 0.0f;
//	m_offset_z = 0.0f;

	m_visibility_mask = 0xff;

	mp_dl = NULL;

	// It's the end of teh project - time for a hack!!!
	Mdl::Skate * p_skate_mod =  Mdl::Skate::Instance();
	if (p_skate_mod->m_requested_level == CRCD(0x9f2bafb7,"load_skateshop"))
	{
		m_base_color.r = 64;
		m_base_color.g = 64;
		m_base_color.b = 64;
		m_base_color.a = 128;
	}
	else
	{
		m_base_color.r = 128;
		m_base_color.g = 128;
		m_base_color.b = 128;
		m_base_color.a = 128;
	}
}



sMesh::~sMesh( void )
{
	if( !( m_flags & MESH_FLAG_IS_INSTANCE ))
	{
		if( m_flags & MESH_FLAG_IS_CLONED )
		{
//			if ( m_localMeshIndex == 0 )
//			{
//				if( mp_posBuffer )
//				{
//					delete [] mp_posBuffer;
//					mp_posBuffer = NULL;
//				}
//				if( mp_normBuffer )
//				{
//					delete [] mp_normBuffer;
//					mp_normBuffer = NULL;
//				}
//				if( mp_colBuffer )
//				{
//					delete [] mp_colBuffer;
//					mp_colBuffer = NULL;
//				}
//			}
		}
		else
		{
//			if ( mp_dl->mp_texture_dl )
//			{
//				delete [] mp_dl->mp_texture_dl;
//			}

//			if( mp_display_list	)
//			{
//				delete mp_display_list;
//				mp_display_list = NULL;
//			}

//			if ( mp_dl && mp_dl->dl_owner )
//			{
//				if ( mp_dl->mp_material_dl )
//				delete [] mp_dl->mp_material_dl;
//			}

//			// If we're the 0th mesh, we can delete the shared pools.
//			if ( m_localMeshIndex == 0 )
//			{
//				if( mp_posBuffer )
//				{
//					delete [] mp_posBuffer;
//					mp_posBuffer = NULL;
//				}
//				if( mp_normBuffer )
//				{
//					delete [] mp_normBuffer;
//					mp_normBuffer = NULL;
//				}
//				if( mp_uvBuffer )
//				{
//					delete [] mp_uvBuffer;
//					mp_uvBuffer = NULL;
//				}
//				if( mp_envBuffer )
//				{
//					delete [] mp_envBuffer;
//					mp_envBuffer = NULL;
//				}
//				if( mp_colBuffer )
//				{
//					delete [] mp_colBuffer;
//					mp_colBuffer = NULL;
//				}
//				if( mp_doubleWeight )
//				{
//					delete [] mp_doubleWeight;
//					mp_doubleWeight = NULL;
//				}
//				if( mp_vc_wibble_data )
//				{
//					delete mp_vc_wibble_data;
//					mp_vc_wibble_data = NULL;
//				}
//			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sMesh::wibble_normals( void )
{
//	// Angle in the range [-PI/16, PI/16], period is 1 second.
//	float time = (float)Tmr::GetTime() * 0.0005f;
//
//	if( m_flags & MESH_FLAG_WATER_HACK )
//	{
//		BYTE		*p_byte;
//		float		*p_normal;
//		float		*p_pos;
//		mp_vertex_buffer2->Lock( 0, 0, &p_byte, 0 );
//		p_pos		= (float*)( p_byte + 0 );
//		p_normal	= (float*)( p_byte + m_normal_offset );
//
//		for( uint32 i = 0; i < m_num_vertices; ++i )
//		{
//			float x				= p_pos[0] - m_sphere[0];
//			float z				= p_pos[2] - m_sphere[2];
//			
//			float time_offset_x	= time + (( x / m_sphere_radius ) * 0.5f );
//			float time_offset_z	= time + (( z / m_sphere_radius ) * 0.5f );
//
//			float angle_x		= ( Mth::PI * ( 1.0f / 64.0f ) * (float)fabs( sinf( time_offset_x * Mth::PI ))) - ( Mth::PI * ( 1.0f / 128.0f ));
//			float angle_z		= ( Mth::PI * ( 1.0f / 64.0f ) * (float)fabs( sinf( time_offset_z * Mth::PI ))) - ( Mth::PI * ( 1.0f / 129.0f ));
//			
//			Mth::Vector	n( sinf( angle_x ), cosf(( angle_x + angle_z ) * 0.5f ), sinf( angle_z ));
//			n.Normalize();
//			
//			p_normal[0]			= n[X];
//			p_normal[1]			= n[Y];
//			p_normal[2]			= n[Z];
//			
//			p_pos				= (float*)((BYTE*)p_pos + m_vertex_stride );
//			p_normal			= (float*)((BYTE*)p_normal + m_vertex_stride );
//		}
//	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sMesh::wibble_vc( void )
{
//	if( mp_vc_wibble_data )
//	{
//		// Grab byte pointer to current 'write' vertex buffer.
////		GXColor		*p_byte = (char *)mp_colBuffer;
//		uint32		*p_color = mp_colBuffer;
////		p_color = (GXColor*)( p_byte + m_diffuse_offset );
//
//		// Scan through each vertex, setting the new color.
//		for( uint32 i = 0; i < m_num_vertex; ++i )
//		{
//			// An index of zero means no update for this vert.
//			uint32 index	= mp_vc_wibble_data[i];
//			if( ( index > 0 ) && ( m_flags & MESH_FLAG_VERTEX_COLOR_WIBBLE ) && mp_material->mp_wibble_vc_colors )
//			{
//				*p_color	= mp_material->mp_wibble_vc_colors[index - 1];
//			}
//			p_color++;
//		}
//	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sMesh::Submit( void )
{
//	wibble_vc();
////	wibble_normals();
//
//	if ( mp_display_list )
//	{
//		GXCallDisplayList ( mp_display_list, m_display_list_size ); 
//	}
////	// Horribly inefficient, but will have to do for now until we get bucket rendering going.
////	if( mp_material )
////	{
////		mp_material->Submit();
////	}
////	
////	// Set pixel and vertex shader.
////	D3DDevice_SetPixelShader( m_pixel_shader );
////	D3DDevice_SetVertexShader( m_fvf_flags );
////
////	// Set the stream source.
////	D3DDevice_SetStreamSource( 0, mp_vertex_buffer, m_vertex_stride );
////
////	// Submit.
////	D3DDevice_DrawIndexedVertices( m_primitive_type, m_num_indices, mp_index_buffer );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sMesh *sMesh::Clone( bool instance )
{
	sMesh *p_clone = new sMesh();

	// Copy over basic details.
	memcpy( p_clone, this, sizeof( sMesh ));
	p_clone->m_flags &= ~(MESH_FLAG_CLONED_POS|MESH_FLAG_CLONED_COL);

	if( instance )
	{
		p_clone->m_flags |= MESH_FLAG_IS_INSTANCE;
	}
	else
	{
		p_clone->m_flags |= MESH_FLAG_IS_CLONED;

		// Clone the display list.
		Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
		p_clone->mp_dl = (NxNgc::sDLHeader*)new char[sizeof(NxNgc::sDLHeader)+mp_dl->m_size];
		Mem::Manager::sHandle().BottomUpHeap()->PopAlign();
		memcpy( p_clone->mp_dl, mp_dl, sizeof(NxNgc::sDLHeader)+mp_dl->m_size );
		p_clone->m_flags |= NxNgc::sMesh::MESH_FLAG_CLONED_DL;



//		p_clone->m_flags |= MESH_FLAG_IS_INSTANCE;

//		if ( m_localMeshIndex == 0 )
//		{
//			if ( mp_posBuffer )
//			{
//#ifdef SHORT_VERT
//				p_clone->mp_posBuffer = new s16[3*m_num_vertex];
//				memcpy( p_clone->mp_posBuffer, mp_posBuffer, 3 * m_num_vertex * sizeof( s16 ) );
//				DCFlushRange( p_clone->mp_posBuffer, 3 * m_num_vertex * sizeof( s16 ) );
//#else
//				p_clone->mp_posBuffer = new float[3*m_num_vertex];
//				memcpy( p_clone->mp_posBuffer, mp_posBuffer, 3 * m_num_vertex * sizeof( float ) );
//				DCFlushRange( p_clone->mp_posBuffer, 3 * m_num_vertex * sizeof( float ) );
//#endif		// SHORT_VERT 
//			}
//			if ( mp_normBuffer )
//			{
//				p_clone->mp_normBuffer = new s16[3*m_num_vertex];
//				memcpy( p_clone->mp_normBuffer, mp_normBuffer, 3 * m_num_vertex * sizeof( s16 ) );
//				DCFlushRange( p_clone->mp_normBuffer, 3 * m_num_vertex * sizeof( s16 ) );
//			}
//
//			if ( mp_colBuffer )
//			{
//				p_clone->mp_colBuffer = new uint32[m_num_vertex];
//				memcpy( p_clone->mp_colBuffer, mp_colBuffer, m_num_vertex * sizeof( uint32 ) );
//				DCFlushRange( p_clone->mp_colBuffer, m_num_vertex * sizeof( uint32 ) );
//			}
//		}
	}
	return p_clone;

//	Dbg_MsgAssert( false, ( "Not yet implmented!!!!!!!!" ) );
//	return NULL;
}



///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void sMesh::Initialize( int num_vertices,
//						float* p_positions,
//#ifdef SHORT_VERT
//						s16* p_positions16,
//						int shift,
//						float off_x,
//						float off_y,
//						float off_z,
//#endif		// SHORT_VERT
//						s16* p_normals,
//						float* p_tex_coords,
//						float* p_env_buffer,
//						int num_tc_sets,
//						uint32* p_colors,
//						int num_indices,
//						uint16* p_pos_indices,
//						uint16* p_col_indices,
//						uint16* p_uv_indices,
//						unsigned long material_checksum,
//						void *p_scene,
//						int num_double,
//						uint32 * p_double,
//						int num_single,
//						int localMeshIndex,
//						GXPrimitive primitive_type,
//						int bone_idx,
//						char *p_vc_wibble_anims )
//
//{
//	m_num_indices		= 0;
//	m_vertex_stride		= 0;
//	m_num_vertex		= num_vertices;
//
//	m_bone_idx			= bone_idx;
//
////	m_primitive_type	= D3DPT_TRIANGLESTRIP;
////	m_fvf_flags			= 0;
////	m_pixel_shader		= 0;
//	
////	mp_posBuffer = NULL;	//p_positions;
////	mp_normBuffer = p_normals;
////	mp_uvBuffer = NULL;	//p_tex_coords;
////	mp_colBuffer = NULL;	//p_colors;
////	mp_index_buffer = NULL;	//p_indices;
//
//	mp_normBuffer = p_normals;
//	mp_uvBuffer = p_tex_coords;
//	mp_colBuffer = p_colors;
//	m_num_uv_sets = num_tc_sets;
//
//	m_numDouble = num_double;
//	m_numSingle = num_single;
//	mp_doubleWeight = p_double;
//
//	m_localMeshIndex = localMeshIndex;
//
//	// First thing to do is grab the material pointer for this mesh.
//	mp_material			= ((sScene*)p_scene )->GetMaterial( material_checksum );
////	if( mp_material == NULL )
////	{
////		exit( 0 );
////
////		// Try with the dummy material for now.
////		// mp_material = GetMaterial( 0 );
////	}
//	
////	if( num_indices == 0 )
////	{
////		return;
////	}
////	
////	// Figure the min and max indices for this mesh.
////	uint16 min_index	= p_indices[0];
////	uint16 max_index	= p_indices[0];
////	for( int i = 1; i < num_indices; ++i )
////	{
////		if( p_indices[i] > max_index )
////		{
////			max_index = p_indices[i];
////		}
////		else if( p_indices[i] < min_index )
////		{
////			min_index = p_indices[i];
////		}
////	}
//	
//	// See if any vertex color alpha valus are less than any of the texture's cutoff values.
//	// If so, make sure that the texture is specified as having holes.
//	for ( int lp = 0; lp < num_vertices; lp++ )
//	{
//		if ( ( p_colors[lp] & 255 ) < mp_material->AlphaCutoff )
//		{
//			for ( int pass = 0; pass < mp_material->Passes; pass++ )
//			{
////				if ( mp_material->pTex[pass] ) mp_material->pTex[pass]->HasHoles = 1;
//				if ( mp_material->pTex[pass] )
//				{
//					if ( !mp_material->pTex[pass]->HasHoles )
//					{
//						mp_material->pTex[pass]->HasHoles = 1;
//						OSReport( "Material found that was incorrectly flagged.\n" );
//					}
//				}
//			}
//			break;
//		}
//	}
//
//	// See if any of the material layers require environment mapping.
//	mp_envBuffer = NULL;
//	for ( int pass = 0; pass < mp_material->Passes; pass++ )
//	{
//		if ( mp_material->Flags[pass] & MATFLAG_ENVIRONMENT )
//		{
//			if ( p_env_buffer )
//			{
//				// Already allocated one, so just assign it.
//				mp_envBuffer = p_env_buffer;
//			}
//			else
//			{
//				// Has environment mapping, so allocate an environment map uv buffer.
//				mp_envBuffer = new float[2*num_vertices];
//			}
//			// Break so that we only allocate once.
//			break;
//		}
//	}
//
//	// Set the bounding box.
//	if ( p_positions )
//	{
//		// Non-skinned.
//		for( int v = 0; v < num_vertices; ++v )
//		{
//			// Do bounding box setting.
//			Mth::Vector p( p_positions[(v*3)+0], p_positions[(v*3)+1], p_positions[(v*3)+2] );
//			m_bbox.AddPoint( p );
//		}
//	}
//	else
//	{
//		// Skinned.
//		uint32*	p32 = p_double;
//		for ( int lp = 0; lp < num_double; lp++ ) {
//			uint32		pairs = *p32++;
//			/*uint32		mtx = **/p32++;
//			float*		p_pn = (float *)p32;
//			float*			p_weight = (float *)&p_pn[(6*pairs)];
//
//			for( uint v = 0; v < pairs; ++v )
//			{
//				// Do bounding box setting.
//				Mth::Vector p( p_pn[(v*6)+0], p_pn[(v*6)+1], p_pn[(v*6)+2] );
//				m_bbox.AddPoint( p );
//			}
//
//			p32 = (uint32 *)&p_weight[pairs*2];
//		}
//	}
//	float x = ( m_bbox.GetMax().GetX() + m_bbox.GetMin().GetX() ) / 2;
//	float y = ( m_bbox.GetMax().GetY() + m_bbox.GetMin().GetY() ) / 2;
//	float z = ( m_bbox.GetMax().GetZ() + m_bbox.GetMin().GetZ() ) / 2;
//	float r = ( m_bbox.GetMax().GetX() - m_bbox.GetMin().GetX() ) * ( m_bbox.GetMax().GetX() - m_bbox.GetMin().GetX() );
//	r += ( m_bbox.GetMax().GetY() - m_bbox.GetMin().GetY() ) * ( m_bbox.GetMax().GetY() - m_bbox.GetMin().GetY() );
//	r += ( m_bbox.GetMax().GetZ() - m_bbox.GetMin().GetZ() ) * ( m_bbox.GetMax().GetZ() - m_bbox.GetMin().GetZ() );
//	r = sqrtf( r );
//	r = r / 2.0f;
//	m_sphere[0] = x;
//	m_sphere[1] = y;
//	m_sphere[2] = z;
//	m_sphere[3] = r;
//
//	// Set the position buffer & offset.
//#ifdef SHORT_VERT
//	mp_posBuffer = p_positions16;
//#else
//	mp_posBuffer = p_positions;
//#endif		// SHORT_VERT
//
//	if ( p_positions )
//	{
//#ifdef SHORT_VERT
//		// Non-skinned.
//		switch ( shift )
//		{
//			case 1:
//				m_vertex_format = GX_VTXFMT2;
//				break;
//			case 2:
//				m_vertex_format = GX_VTXFMT3;
//				break;
//			case 3:
//				m_vertex_format = GX_VTXFMT4;
//				break;
//			case 4:
//				m_vertex_format = GX_VTXFMT5;
//				break;
//			case 5:
//				m_vertex_format = GX_VTXFMT6;
//				break;
//			case 6:
//				m_vertex_format = GX_VTXFMT7;
//				break;
//			case 0:
//			default:
//				m_vertex_format = GX_VTXFMT1;
//				break;
//		}
//#else
//		m_vertex_format = GX_VTXFMT6;
//#endif		// SHORT_VERT
//	}
//	else
//	{
//		// Skinned.
//		m_vertex_format = GX_VTXFMT7;
//	}
//
//#ifdef SHORT_VERT
//	m_offset_x = off_x;
//	m_offset_y = off_y;
//	m_offset_z = off_z;
//#endif		// SHORT_VERT
//
////	if( max_index >= num_vertices )
////	{
////		// Error!
////		exit( 0 );
////	}
//	
//	// Now figure the total number of vertices required for this mesh, to span the min->max indices.
////	uint16 vertices_for_this_mesh = max_index - min_index + 1;
//
//	// Create the index buffer (should be 16byte aligned for best performance).
////	mp_index_buffer	= new uint16[num_indices];
//
//	// Set our count of the number of indices.
//	m_num_indices = num_indices - 2;		// Only used for skinned - subtract count & trailing 0.
//
//	// Copy in index data, normaling the indices for this vertex buffer (i.e. so the lowest index will reference
//	// vertex 0 in the buffer built specifically for this mesh).
////	for( int i = 0; i < num_indices; ++i )
////	{
////		mp_index_buffer[i] = p_indices[i];	// - min_index;
////	}
//
//	// Use the material flags to figure the vertex format.
//	int vertex_size			= 3 * sizeof( float );
//
//	// Include weights (for weighted animation) if present.
////	if( p_weights )
////	{
////		Dbg_Assert( p_matrix_indices );
////		vertex_size	+= sizeof( float ) * 4;
////	}
//
//	// Include indices (for weighted animation) if present.
////	if( p_matrix_indices )
////	{
////		Dbg_Assert( p_weights );
////		vertex_size	+= sizeof( uint16 ) * 4;
////	}
//	
//	// Texture coordinates.
//	uint32	tex_coord_pass	= 0;
////	bool	env_mapped		= false;
//	if( p_tex_coords )
//	{
//		for( uint32 pass = 0; pass < mp_material->Passes; ++pass )
//		{
////			if( mp_material->Flags[pass] & MATFLAG_ENVIRONMENT )
////			{
////				env_mapped		= true;
////			}
//
//			// Only need UV's for this stage if it is *not* environment mapped.
//			if(( mp_material->pTex[pass] ) /*&& ( !( mp_material->Flags[pass] & MATFLAG_ENVIRONMENT ))*/)
//			{
//				// Will need uv for this pass and all before it.
//				tex_coord_pass	= pass + 1; 
//			}
//		}
//	}
//	else
//	{
//		for( uint32 pass = 0; pass < mp_material->Passes; ++pass )
//		{
////			if( mp_material->Flags[pass] & MATFLAG_ENVIRONMENT )
////			{
////				env_mapped		= true;
////			}
//		}
//	}
//
//	if( tex_coord_pass > 0 )
//	{
//		vertex_size += 2 * sizeof( float ) * tex_coord_pass;
//	}
//
//	// Assume no normals for now.
//	if( 0 )
//	{
//		// We want vertex colors in this mesh,
//		if( 0 )
//		{
//			// The raw vertex data does not contain vertex colors.
//			exit( 0 );
//		}
//		else
//		{
//			// The raw vertex data does contain vertex colors.
//		}
//	}
//
//	bool use_colors = false;
//	if( p_colors )
//	{
//		// We want vertex colors in this mesh,
//		if( 0 )
//		{
//			// The raw vertex data does not contain vertex colors.
//			exit( 0 );
//		}
//		else
//		{
//			// The raw vertex data does contain vertex colors.
//			vertex_size	+= sizeof( uint32 );
//			use_colors	= true;
//		}
//	}
//	
//	// Create the vertex color wibble array if data is present.
//	if( p_vc_wibble_anims )
//	{
//		mp_vc_wibble_data = p_vc_wibble_anims;
//	}
//
////	if ( p_double )
////	{
////		mp_doubleWeight = 1;
////	}
////	else
////	{
////		mp_doubleWeight = 0;
////	}
//
//	m_primitive_type = primitive_type;
//
//	Build( num_indices, p_pos_indices, p_col_indices, p_uv_indices );
//
//	// Find the index list in the display list data.
//	mp_index_buffer = NULL;
//	{
//		unsigned char * p8;
//		unsigned char search[3];
//
//		p8 = (unsigned char *)mp_display_list;
//
//		search[0] = *p8++;
//		search[1] = *p8++;
//		for ( int ss = 2; ss < m_display_list_size; ss++ ) {
//			search[2] = *p8++;
//
//			if (	( ( search[0] & 0xf8 ) == m_primitive_type ) &&
//					( ( ( search[1] << 8 ) | search[2] ) == (int)*p_pos_indices ) ) {
//				// Found it!!!
//				if ( num_double )
//				{
//					// Skinned mesh.
//					mp_index_buffer = (unsigned short *)p8;
//				}
//				else
//				{
//					// Non-skinned mesh.
//					mp_index_buffer = (unsigned short *)&p8[-2];
//				}
//				break;
//			}
//			search[0] = search[1];
//			search[1] = search[2];
//		}
//	}
//
////	if ( p_double )
////	{
////		double_bytes = dbytes;
////		single_bytes = sbytes;
////		
////		if ( m_localMeshIndex == 0 )
////		{
////			NsARAM::HEAPTYPE heap = NsARAM::BOTTOMUP;
////
////			if ( Mem::Manager::sHandle().GetContextAllocator() == Mem::Manager::sHandle().SkaterGeomHeap(0) )
////			{
////				heap = NsARAM::SKATER0;
////			}
////			if ( Mem::Manager::sHandle().GetContextAllocator() == Mem::Manager::sHandle().SkaterGeomHeap(1) )
////			{
////				heap = NsARAM::SKATER1;
////			}
////
////			//OSReport( "Allocating %d ARAM bytes %s\n", dbytes + sbytes, Mem::Manager::sHandle().GetContextName() );
////			mp_doubleWeight = NsARAM::alloc( (dbytes + sbytes) * 4, heap );
////			NsDMA::toARAM( mp_doubleWeight, p_double, ( dbytes + sbytes ) * 4 );
////		}
////	}
////	else
////	{
////		mp_doubleWeight = 0;
////		double_bytes = dbytes;
////		single_bytes = sbytes;
////	}
//
//	delete p_pos_indices;
//}

//void sMesh::Build( int num_indices, uint16* p_pos_indices, uint16* p_col_indices, uint16* p_uv_indices, bool rebuild )
//{
//	Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
//	EngineGlobals.gpuBusy = true;
//	// Turn the index list into a display list.
//	mp_display_list = NULL;
//#define DL_BUILD_SIZE (32*1024)
//	uint8 * p_build_dl = new (Mem::Manager::sHandle().TopDownHeap()) uint8[DL_BUILD_SIZE];
//	DCFlushRange ( p_build_dl, DL_BUILD_SIZE );
//
//	GXBeginDisplayList ( p_build_dl, DL_BUILD_SIZE );
//	GXResetWriteGatherPipe();
//
//	GXClearVtxDesc();
//	GXSetVtxDesc( GX_VA_POS, GX_INDEX16 );
//	m_vertex_stride = 1;
//	if ( mp_normBuffer || mp_doubleWeight )
//	{
//		GXSetVtxDesc( GX_VA_NRM, GX_INDEX16 );
//		m_vertex_stride++;
//	}
//	if ( mp_colBuffer )
//	{
//		GXSetVtxDesc( GX_VA_CLR0, GX_INDEX16 );
//		m_vertex_stride++;
//		if ( !mp_doubleWeight && gCorrectColor )
//		{
//			// Before we go wading in and allow a 2nd color index list, see if it's necessary.
//			// Check every poly in the mesh and see if the color difference is greater than a threshold.
//			bool needs_fixing = false;
//			uint32 color0;
//			uint32 color1 = mp_colBuffer[p_col_indices[p_pos_indices[0]]];
//			uint32 color2 = mp_colBuffer[p_col_indices[p_pos_indices[1]]];
//			for ( int lp = 2; lp < num_indices; lp++ )
//			{
//				// Grab new color to form triangle.
//				color0 = color1;
//				color1 = color2;
//				color2 = mp_colBuffer[p_col_indices[p_pos_indices[lp]]];
//
//				int diff0_a = abs( ( ( color1 >> 24 ) & 255 ) - ( ( color0 >> 24 ) & 255 ) );
//				int diff1_a = abs( ( ( color2 >> 24 ) & 255 ) - ( ( color1 >> 24 ) & 255 ) );
//				int diff2_a = abs( ( ( color0 >> 24 ) & 255 ) - ( ( color2 >> 24 ) & 255 ) );
//				int diff0_r = abs( ( ( color1 >> 16 ) & 255 ) - ( ( color0 >> 16 ) & 255 ) );
//				int diff1_r = abs( ( ( color2 >> 16 ) & 255 ) - ( ( color1 >> 16 ) & 255 ) );
//				int diff2_r = abs( ( ( color0 >> 16 ) & 255 ) - ( ( color2 >> 16 ) & 255 ) );
//				int diff0_g = abs( ( ( color1 >>  8 ) & 255 ) - ( ( color0 >>  8 ) & 255 ) );
//				int diff1_g = abs( ( ( color2 >>  8 ) & 255 ) - ( ( color1 >>  8 ) & 255 ) );
//				int diff2_g = abs( ( ( color0 >>  8 ) & 255 ) - ( ( color2 >>  8 ) & 255 ) );
//				int diff0_b = abs( ( ( color1 >>  0 ) & 255 ) - ( ( color0 >>  0 ) & 255 ) );
//				int diff1_b = abs( ( ( color2 >>  0 ) & 255 ) - ( ( color1 >>  0 ) & 255 ) );
//				int diff2_b = abs( ( ( color0 >>  0 ) & 255 ) - ( ( color2 >>  0 ) & 255 ) );
//
//				int wdiff = diff0_a;
//				if ( diff1_a > wdiff ) wdiff = diff1_a;
//				if ( diff2_a > wdiff ) wdiff = diff2_a;
//				if ( diff0_r > wdiff ) wdiff = diff0_r;
//				if ( diff1_r > wdiff ) wdiff = diff1_r;
//				if ( diff2_r > wdiff ) wdiff = diff2_r;
//				if ( diff0_g > wdiff ) wdiff = diff0_g;
//				if ( diff1_g > wdiff ) wdiff = diff1_g;
//				if ( diff2_g > wdiff ) wdiff = diff2_g;
//				if ( diff0_b > wdiff ) wdiff = diff0_b;
//				if ( diff1_b > wdiff ) wdiff = diff1_b;
//				if ( diff2_b > wdiff ) wdiff = diff2_b;
//
//				if ( wdiff > 8 ) needs_fixing = true;
//			}
//
//			if ( needs_fixing )
//			{
//				m_flags |= MESH_FLAG_DOUBLE_COLOR_IDX;
//				GXSetVtxDesc( GX_VA_CLR1, GX_INDEX16 );
//				m_vertex_stride++;
//			}
//		}
//	}
//
//	int coord = 0;
//	for ( uint layer = 0; layer < mp_material->Passes; layer++ )
//	{
//		GXSetVtxDesc( (GXAttr)(((int)GX_VA_TEX0)+coord), GX_INDEX16 );
//		m_vertex_stride++;
//		coord++;
//	}
//
//	if ( rebuild )
//	{
//		// Send coordinates.
//		GXBegin( (GXPrimitive)m_primitive_type, (GXVtxFmt)m_vertex_format, num_indices ); 
//		for ( int lp = 0; lp < num_indices; lp++ )
//		{
//			// Send coordinates.
//			int pindex = p_pos_indices[lp];
//			int cindex = p_col_indices[pindex];
//			GXPosition1x16(pindex);
//			if ( mp_normBuffer || mp_doubleWeight ) GXNormal1x16(pindex);
//			if ( mp_colBuffer ) GXColor1x16(cindex);
//			if ( m_flags & MESH_FLAG_DOUBLE_COLOR_IDX ) GXColor1x16(cindex);
//			for ( uint layer = 0; layer < mp_material->Passes; layer++ )
//			{
//				int uindex = pindex;
//				if ( p_uv_indices ) uindex = p_uv_indices[pindex+(layer*m_num_vertex)];
//				GXTexCoord1x16(uindex);
//			}
//		}
//		GXEnd();
//	}
//	else
//	{
//		// Parse index list & turn into gxbegin/end pairs.
//		int v = 0;
//		while ( p_pos_indices[v] )
//		{
//			uint16 num = p_pos_indices[v];
//			v++;
//
//			GXBegin( (GXPrimitive)m_primitive_type, (GXVtxFmt)m_vertex_format, num ); 
//
//			for ( int vv = 0; vv < num; vv++ )
//			{
//				// Send coordinates.
//				int pindex = p_pos_indices[v+vv];
//				int cindex = p_col_indices[pindex];
//				GXPosition1x16(pindex);
//				if ( mp_normBuffer || mp_doubleWeight ) GXNormal1x16(pindex);
//				if ( mp_colBuffer ) GXColor1x16(cindex);
//				if ( m_flags & MESH_FLAG_DOUBLE_COLOR_IDX ) GXColor1x16(cindex);
//				for ( uint layer = 0; layer < mp_material->Passes; layer++ )
//				{
//					int uindex = pindex;
//					if ( p_uv_indices ) uindex = p_uv_indices[pindex+(layer*m_num_vertex)];
//					GXTexCoord1x16(uindex);
//				}
//			}
//			v += num;
//			GXEnd();
//		}			
//	}
//
//	int size = GXEndDisplayList();
//	
//	mp_index_buffer = NULL;
//
//	uint8 * p_dl = new uint8[size];
//	memcpy ( p_dl, p_build_dl, size );
//	DCFlushRange ( p_dl, size );
//
//	delete p_build_dl;
//
//	mp_display_list = p_dl;
//	m_display_list_size = size;
//	EngineGlobals.gpuBusy = false;
//
//	Mem::Manager::sHandle().BottomUpHeap()->PopAlign();
//}
//
//void sMesh::Rebuild( int num_indices, uint16* p_indices )
//{
//	uint16 * p_temp_col_remap = NULL;
//	uint16 * p_temp_uv_remap = NULL;
//	p_temp_uv_remap = m_num_uv_sets ? new (Mem::Manager::sHandle().TopDownHeap()) uint16[m_num_vertex*m_num_uv_sets] : NULL;
//	p_temp_col_remap = mp_colBuffer ? new (Mem::Manager::sHandle().TopDownHeap()) uint16[m_num_vertex] : NULL;
//
//	for ( int c = 0; c < m_num_vertex; c++ )
//	{
//		for ( int u = 0; u < m_num_uv_sets; u++ )
//		{
//			p_temp_uv_remap[(u*m_num_vertex)+c] = (u*m_num_vertex)+c;
//		}
//	}
//
//	for ( int c = 0; c < m_num_vertex; c++ )
//	{
//		p_temp_col_remap[c] = c;
//	}
//
//	delete [] mp_display_list;
//
////	Build( num_indices, p_indices, p_indices, p_indices, true );
//	Build( num_indices, p_indices, p_temp_col_remap, p_temp_uv_remap, true );
//	// Update to new number of indices.
//	m_num_indices = num_indices;
//
//	delete [] p_temp_col_remap;
//	delete [] p_temp_uv_remap;
//}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sMesh::SetPosition( Mth::Vector &pos )
{


//	// Figure what we need to add to each vertex, based on current position.
//	Mth::Vector offset(	pos[X] - m_offset_x,
//						pos[Y] - m_offset_y,
//						pos[Z] - m_offset_z );
//	m_offset_x = pos[X];
//	m_offset_y = pos[Y];
//	m_offset_z = pos[Z];
//
//	// We also need to adjust the bounding box and sphere information for this mesh.
//	m_sphere[0] += offset[X];
//	m_sphere[1] += offset[Y];
//	m_sphere[2] += offset[Z];
//
//	m_bbox.SetMin( m_bbox.GetMin() + offset );
//	m_bbox.SetMax( m_bbox.GetMax() + offset );
//
//#ifndef SHORT_VERT
//	if ( m_localMeshIndex != 0 ) return;
//
//	for( uint32 bv = 0; bv < m_num_vertex; ++bv )
//	{
//		mp_posBuffer[(bv*3)+0] += offset[X];
//		mp_posBuffer[(bv*3)+1] += offset[Y];
//		mp_posBuffer[(bv*3)+2] += offset[Z];
//	}
//	DCFlushRange( mp_posBuffer, 3 * sizeof( float ) * m_num_vertex );
//#endif		// SHORT_VERT
//
}
	

	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sMesh::GetPosition( Mth::Vector *p_pos )
{
//	p_pos->Set( m_offset_x, m_offset_y, m_offset_z );
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sMesh::SetYRotation( Mth::ERot90 rot )
{
//	if( ( rot > Mth::ROT_0 ) && ( m_localMeshIndex == 0 ) )
//	{
//		s16 nx, nz;
//#ifdef SHORT_VERT
//		s16 xo = 0;
//		s16 zo = 0;
//		s16 x, z;
//#else
//		float x, z;
//		float xo = m_offset_x;
//		float zo = m_offset_z;
//#endif		// SHORT_VERT 
//		switch( rot )
//		{
//			case Mth::ROT_90:
//			{
//				if ( m_localMeshIndex == 0 )
//				{
//					for( uint32 v = 0; v < m_num_vertex; ++v )
//					{
//						x = mp_posBuffer[(v*3)+0] - xo;
//						z = mp_posBuffer[(v*3)+2] - zo;
//						mp_posBuffer[(v*3)+0] = z + xo;
//						mp_posBuffer[(v*3)+2] = -x + zo;
//						if ( mp_normBuffer )
//						{
//							nx = mp_normBuffer[(v*3)+0];
//							nz = mp_normBuffer[(v*3)+2];
//							mp_normBuffer[(v*3)+0] = nz;
//							mp_normBuffer[(v*3)+2] = -nx;
//						}
//					}
//				}
//				// Adjust the bounding sphere information for this mesh.
//				m_sphere[0]	-= m_offset_x;
//				m_sphere[2]	-= m_offset_z;
//				float t				= m_sphere[0];
//				m_sphere[0]	= m_sphere[2] + m_offset_x;
//				m_sphere[2]	= -t + m_offset_z;
//
//				// Adjust the bounding box information for this mesh.
//				Mth::Vector m[4];
//
//				m[0].Set( m_bbox.GetMin()[X], 0.0f, m_bbox.GetMin()[Z] );
//				m[1].Set( m_bbox.GetMin()[X], 0.0f, m_bbox.GetMax()[Z] );
//				m[2].Set( m_bbox.GetMax()[X], 0.0f, m_bbox.GetMin()[Z] );
//				m[3].Set( m_bbox.GetMax()[X], 0.0f, m_bbox.GetMax()[Z] );
//
//				for( int i = 0; i < 4; ++i )
//				{
//					m[i][X] -= m_offset_x;
//					m[i][Z] -= m_offset_z;
//					float t = m[i][X];
//					m[i][X] = m[i][Z];
//					m[i][Z] = -t;
//					m[i][X] += m_offset_x;
//					m[i][Z] += m_offset_z;
//				}
//
//				Mth::Vector mmin( m[0][X], m_bbox.GetMin()[Y], m[0][Z] );
//				Mth::Vector mmax( m[0][X], m_bbox.GetMax()[Y], m[0][Z] );
//				for( int i = 1; i < 4; ++i )
//				{
//					if( m[i][X] < mmin[X] )
//						mmin[X] = m[i][X];
//					if( m[i][X] > mmax[X] )
//						mmax[X] = m[i][X];
//
//					if( m[i][Z] < mmin[Z] )
//						mmin[Z] = m[i][Z];
//					if( m[i][Z] > mmax[Z] )
//						mmax[Z] = m[i][Z];
//				}
//
//				m_bbox.SetMin( mmin );
//				m_bbox.SetMax( mmax );
//				break;
//			}
//			case Mth::ROT_180:
//			{
//				if ( m_localMeshIndex == 0 )
//				{
//					for( uint32 v = 0; v < m_num_vertex; ++v )
//					{
//						x = mp_posBuffer[(v*3)+0] - xo;
//						z = mp_posBuffer[(v*3)+2] - zo;
//						mp_posBuffer[(v*3)+0] = -x + xo;
//						mp_posBuffer[(v*3)+2] = -z + zo;
//						if ( mp_normBuffer )
//						{
//							nx = mp_normBuffer[(v*3)+0];
//							nz = mp_normBuffer[(v*3)+2];
//							mp_normBuffer[(v*3)+0] = -nx;
//							mp_normBuffer[(v*3)+2] = -nz;
//						}
//					}
//				}
//				// Adjust the bounding sphere information for this mesh.
//				m_sphere[0]	-= m_offset_x;
//				m_sphere[2]	-= m_offset_z;
//				m_sphere[0]	= -m_sphere[0] + m_offset_x;
//				m_sphere[2]	= -m_sphere[2] + m_offset_z;
//
//				// Adjust the bounding box information for this mesh.
//				Mth::Vector m[4];
//
//				m[0].Set( m_bbox.GetMin()[X], 0.0f, m_bbox.GetMin()[Z] );
//				m[1].Set( m_bbox.GetMin()[X], 0.0f, m_bbox.GetMax()[Z] );
//				m[2].Set( m_bbox.GetMax()[X], 0.0f, m_bbox.GetMin()[Z] );
//				m[3].Set( m_bbox.GetMax()[X], 0.0f, m_bbox.GetMax()[Z] );
//
//				for( int i = 0; i < 4; ++i )
//				{
//					m[i][X] -= m_offset_x;
//					m[i][Z] -= m_offset_z;
//					m[i][X] = -m[i][X];
//					m[i][Z] = -m[i][Z];
//					m[i][X] += m_offset_x;
//					m[i][Z] += m_offset_z;
//				}
//
//				Mth::Vector mmin( m[0][X], m_bbox.GetMin()[Y], m[0][Z] );
//				Mth::Vector mmax( m[0][X], m_bbox.GetMax()[Y], m[0][Z] );
//				for( int i = 1; i < 4; ++i )
//				{
//					if( m[i][X] < mmin[X] )
//						mmin[X] = m[i][X];
//					if( m[i][X] > mmax[X] )
//						mmax[X] = m[i][X];
//
//					if( m[i][Z] < mmin[Z] )
//						mmin[Z] = m[i][Z];
//					if( m[i][Z] > mmax[Z] )
//						mmax[Z] = m[i][Z];
//				}
//
//				m_bbox.SetMin( mmin );
//				m_bbox.SetMax( mmax );
//				break;
//			}
//			case Mth::ROT_270:
//			{
//				if ( m_localMeshIndex == 0 )
//				{
//					for( uint32 v = 0; v < m_num_vertex; ++v )
//					{
//						x = mp_posBuffer[(v*3)+0] - xo;
//						z = mp_posBuffer[(v*3)+2] - zo;
//						mp_posBuffer[(v*3)+0] = -z + xo;
//						mp_posBuffer[(v*3)+2] = x + zo;
//						if ( mp_normBuffer )
//						{
//							nx = mp_normBuffer[(v*3)+0];
//							nz = mp_normBuffer[(v*3)+2];
//							mp_normBuffer[(v*3)+0] = -nz;
//							mp_normBuffer[(v*3)+2] = nx;
//						}
//					}
//				}
//				// Adjust the bounding sphere information for this mesh.
//				m_sphere[0]	-= m_offset_x;
//				m_sphere[2]	-= m_offset_z;
//				float t				= m_sphere[0];
//				m_sphere[0]	= -m_sphere[2] + m_offset_x;
//				m_sphere[2]	= t + m_offset_z;
//
//				// Adjust the bounding box information for this mesh.
//				Mth::Vector m[4];
//
//				m[0].Set( m_bbox.GetMin()[X], 0.0f, m_bbox.GetMin()[Z] );
//				m[1].Set( m_bbox.GetMin()[X], 0.0f, m_bbox.GetMax()[Z] );
//				m[2].Set( m_bbox.GetMax()[X], 0.0f, m_bbox.GetMin()[Z] );
//				m[3].Set( m_bbox.GetMax()[X], 0.0f, m_bbox.GetMax()[Z] );
//
//				for( int i = 0; i < 4; ++i )
//				{
//					m[i][X] -= m_offset_x;
//					m[i][Z] -= m_offset_z;
//					float t = m[i][X];
//					m[i][X] = -m[i][Z];
//					m[i][Z] = t;
//					m[i][X] += m_offset_x;
//					m[i][Z] += m_offset_z;
//				}
//
//				Mth::Vector mmin( m[0][X], m_bbox.GetMin()[Y], m[0][Z] );
//				Mth::Vector mmax( m[0][X], m_bbox.GetMax()[Y], m[0][Z] );
//				for( int i = 1; i < 4; ++i )
//				{
//					if( m[i][X] < mmin[X] )
//						mmin[X] = m[i][X];
//					if( m[i][X] > mmax[X] )
//						mmax[X] = m[i][X];
//
//					if( m[i][Z] < mmin[Z] )
//						mmin[Z] = m[i][Z];
//					if( m[i][Z] > mmax[Z] )
//						mmax[Z] = m[i][Z];
//				}
//
//				m_bbox.SetMin( mmin );
//				m_bbox.SetMax( mmax );
//				break;
//			}
//			default:
//				// Do nothing.
//				break;
//		}
//#ifdef SHORT_VERT
//		DCFlushRange( mp_posBuffer, 3 * m_num_vertex * sizeof( s16 ) );
//#else
//		DCFlushRange( mp_posBuffer, 3 * m_num_vertex * sizeof( float ) );
//#endif		// SHORT_VERT
//		if ( mp_normBuffer ) DCFlushRange( mp_normBuffer, 3 * m_num_vertex * sizeof( s16 ) );
//	}
}
	
} // namespace NxNgc




