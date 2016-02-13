
///////////////////////////////////////////////////////////////////////////////
// p_NxSector.cpp

#include	<sys/file/filesys.h>

#include 	"gfx/ngc/p_NxSector.h"
#include 	"gfx/ngc/p_NxGeom.h"
#include 	"gfx/Image/ImageBasic.h"
#include 	"gfx/ngc/nx/render.h"
#include 	"gfx/ngc/nx/mesh.h"
#include 	"gfx/NxMiscFX.h"
#include 	"gfx\ngc\nx\nx_init.h"
#include	<dolphin/gd.h>

//NxNgc::sMesh *	p_u_mat[256];
//int				u_mat_count[256];
//int				num_u_mat = 0;

namespace Nx
{

#ifdef SHORT_VERT

static int round_float( float f, int shift, float off, float cen )
{
	float mul = ((float)( 1 << shift ));
	int i_f = (int)( ( f - cen ) * mul );

	int adjust = 0;
	if ( i_f > 0 )
	{
		adjust = 1;
	}
//	if ( i_f < 0 )
//	{
//		adjust = -1;
//	}

	i_f = (int)( ( f - off ) * mul );
	i_f += adjust;
	return i_f;

//	float mul = ((float)( 1 << shift ));
//	int i_f = (int)( f * mul );
//	float d_f = ( f * mul ) - ((float)i_f);
//
//	if ( d_f >= 0.0f )
//	{
//		if( d_f < 0.5f )
//		{
//			return i_f;
//		}
//		else
//		{
//			return i_f + 1;
//		}
//	}
//	else
//	{
//		if( d_f < -0.5f )
//		{
//			return i_f;
//		}
//		else
//		{
//			return i_f - 1;
//		}
//	}
}
#endif		// SHORT_VERT

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

//CNgcSector::CNgcSector() : mp_init_mesh_list( NULL ), m_mesh_array( NULL ), m_visible( 0xDEADBEEF )
CNgcSector::CNgcSector()
{
	m_pos_offset.Set( 0.0f, 0.0f, 0.0f );
//	m_active = true;						// default to be active....
//	mp_scene = NULL;
}




#define MemoryRead( dst, size, num, src )	memcpy(( dst ), ( src ), (( num ) * ( size )));	\
											( src ) += (( num ) * ( size ))

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
NxNgc::sObjectHeader* CNgcSector::LoadFromMemory( NxNgc::sObjectHeader* p_data )
{
//	uint8		*p_file_data	= (uint8*)( *pp_mem );
//
//	Dbg_Assert( mp_geom );
//
//	CNgcGeom *p_geom = static_cast<CNgcGeom*>( mp_geom );
//	
//	// Read sector checksum.
//	uint32 sector_checksum;
//	MemoryRead( &sector_checksum, sizeof( uint32 ), 1, p_file_data );
//	
//	SetChecksum( sector_checksum );
//
//	// Read bone index.
//	int bone_idx;
//	MemoryRead( &bone_idx, sizeof( int ), 1, p_file_data );
//
//	// Read sector flags.
//	uint32 flags;
//	MemoryRead( &flags, sizeof( int ), 1, p_file_data );
//	m_flags = flags;
//
//	// Read number of meshes.
//	int num_mesh;
//	MemoryRead( &num_mesh, sizeof( uint ), 1, p_file_data );
//	p_geom->m_num_mesh = num_mesh;
//
//	// Read bounding box.
//	float bbox[6];
//	MemoryRead( &bbox[0], sizeof( float ), 6, p_file_data );
//	Mth::Vector	inf( bbox[0], bbox[1], bbox[2] );
//	Mth::Vector	sup( bbox[3], bbox[4], bbox[5] );
//	p_geom->m_bbox.Set( inf, sup );
//		
//	// Read bounding sphere.
//	float bsphere[4];
//	MemoryRead( &bsphere[0], sizeof( float ), 4, p_file_data );
//		
//	// Read num vertices.
//	int num_vertices;
//	MemoryRead( &num_vertices, sizeof( int ), 1, p_file_data );
//	
//	// Read vertex data stride.
//	int vertex_data_stride;
//	MemoryRead( &vertex_data_stride, sizeof( int ), 1, p_file_data );
//	
//	// Only read vertex/normal pools if skinned.
//	int dbytes = 0;
//	int sbytes = 0;
//	float* p_vertex_positions = NULL;
//	s16* p_vertex_normals = NULL;
//	uint32* p_double = NULL;
//	uint32* p_temp = NULL;
//	int num_double = 0;
//	int num_single = 0;
//#ifdef SHORT_VERT
//	s16* p_vertex_positions16 = NULL;
//	int shift = 0;
//	float off_x = 0.0f;
//	float off_y = 0.0f;
//	float off_z = 0.0f;
//#endif		// SHORT_VERT
//	if ( !(m_flags & 0x10) )
//	{
//		// Grab a buffer for the raw vertex data position stream, and read it.
//#ifdef SHORT_VERT
//		p_vertex_positions = new (Mem::Manager::sHandle().TopDownHeap()) float[num_vertices * 3];
//#else
//		p_vertex_positions = new float[num_vertices * 3];
//#endif		// SHORT_VERT
//		MemoryRead( p_vertex_positions, sizeof( float ) * 3, num_vertices, p_file_data );
//
//#ifdef SHORT_VERT
//		int lp;
//
//		// Calculate the largest dimension.
//		float smallest_x = p_vertex_positions[0];
//		float smallest_y = p_vertex_positions[1];
//		float smallest_z = p_vertex_positions[2];
//		float largest_x = p_vertex_positions[0];
//		float largest_y = p_vertex_positions[1];
//		float largest_z = p_vertex_positions[2];
//		for ( lp = 1; lp < num_vertices; lp++ )
//		{
//			if ( p_vertex_positions[(lp*3)+0] > largest_x ) largest_x = p_vertex_positions[(lp*3)+0];
//			if ( p_vertex_positions[(lp*3)+1] > largest_y ) largest_y = p_vertex_positions[(lp*3)+1];
//			if ( p_vertex_positions[(lp*3)+2] > largest_z ) largest_z = p_vertex_positions[(lp*3)+2];
//			if ( p_vertex_positions[(lp*3)+0] < smallest_x ) smallest_x = p_vertex_positions[(lp*3)+0];
//			if ( p_vertex_positions[(lp*3)+1] < smallest_y ) smallest_y = p_vertex_positions[(lp*3)+1];
//			if ( p_vertex_positions[(lp*3)+2] < smallest_z ) smallest_z = p_vertex_positions[(lp*3)+2];
//		}
//		float biggest = largest_x;
//		if ( largest_y > biggest ) biggest = largest_y;
//		if ( largest_z > biggest ) biggest = largest_z;
//		if ( -smallest_x > biggest ) biggest = -smallest_x;
//		if ( -smallest_y > biggest ) biggest = -smallest_y;
//		if ( -smallest_z > biggest ) biggest = -smallest_z;
//
//		float c_x = ( smallest_x + largest_x ) / 2.0f;
//		float c_y = ( smallest_y + largest_y ) / 2.0f;
//		float c_z = ( smallest_z + largest_z ) / 2.0f;
//
//		// Work out the shift amount.
//		if ( biggest > ( ( 1 << 15 ) - 1 ) )
//		{
//			// Work out how to offset it.
//			off_x = ( smallest_x + largest_x ) / 2.0f;
//			off_y = ( smallest_y + largest_y ) / 2.0f;
//			off_z = ( smallest_z + largest_z ) / 2.0f;
//			biggest = ( largest_x - smallest_x ) / 2.0f;
//			if ( ( ( largest_y - smallest_y ) / 2.0f ) > biggest ) biggest = ( ( largest_y - smallest_y ) / 2.0f );
//			if ( ( ( largest_z - smallest_z ) / 2.0f ) > biggest ) biggest = ( ( largest_z - smallest_z ) / 2.0f );
//
//			if ( ( largest_x - smallest_x ) > 65535 )
//			{
//				OSReport( "Cannot deal with meshes larger than 65535 X units, this mesh is %6.3f\n", ( largest_x - smallest_x ) );
//				while ( 1 == 1 );
//			}
//
//			if ( ( largest_y - smallest_y ) > 65535 )
//			{
//				OSReport( "Cannot deal with meshes larger than 65535 Y units, this mesh is %6.3f\n", ( largest_y - smallest_y ) );
//				while ( 1 == 1 );
//			}
//
//			if ( ( largest_z - smallest_z ) > 65535 )
//			{
//				OSReport( "Cannot deal with meshes larger than 65535 Z units, this mesh is %6.3f\n", ( largest_z - smallest_z ) );
//				while ( 1 == 1 );
//			}
//			shift = 0;
//		}
//
//		if ( biggest > ( ( 1 << 14 ) - 1 ) )
//		{
//			shift = 0;
//		}
//		else
//		if ( biggest > ( ( 1 << 13 ) - 1 ) )
//		{
//			shift = 1;
//		}
//		else
//		if ( biggest > ( ( 1 << 12 ) - 1 ) )
//		{
//			shift = 2;
//		}
//		else
//		if ( biggest > ( ( 1 << 11 ) - 1 ) )
//		{
//			shift = 3;
//		}
//		else
//		if ( biggest > ( ( 1 << 10 ) - 1 ) )
//		{
//			shift = 4;
//		}
//		else
//		{
//			shift = 5;
//		}
////		if ( biggest > ( ( 1 << 9 ) - 1 ) )
////		{
////			shift = 5;
////		}
////		else
////		{
////			shift = 6;
////		}
//
//		// Now, we need to squeeze the vertex positions down to s16s (1:9:6)
//		p_vertex_positions16 = new s16[num_vertices * 3];
//		for ( lp = 0; lp < num_vertices; lp++ )
//		{
//			p_vertex_positions16[(lp*3)+0] = (s16)( round_float( p_vertex_positions[(lp*3)+0], shift, off_x, c_x ) );
//			p_vertex_positions16[(lp*3)+1] = (s16)( round_float( p_vertex_positions[(lp*3)+1], shift, off_y, c_y ) );
//			p_vertex_positions16[(lp*3)+2] = (s16)( round_float( p_vertex_positions[(lp*3)+2], shift, off_z, c_z ) );
//		}
//#endif		// SHORT_VERT
//
//		// Grab a buffer for the raw vertex data normal stream (if present), and read it.
//		p_vertex_normals = ( m_flags & 0x04 ) ? new s16[num_vertices * 3] : NULL;
//		if( p_vertex_normals )
//		{
//			MemoryRead( p_vertex_normals, sizeof( s16 ) * 3, num_vertices, p_file_data );
//		}
//	} else {
//		int bytes = 0;
//		// This is a skinned model, read in the double data.
//		MemoryRead( &bytes, sizeof( int ), 1, p_file_data );
//		MemoryRead( &num_double, sizeof( int ), 1, p_file_data );
//
//		dbytes = ( bytes + 3 ) / 4;
//		p_temp = new uint32[dbytes];
//		MemoryRead( p_temp, bytes, 1, p_file_data );
//
//		// Read the single data.
//		MemoryRead( &bytes, sizeof( int ), 1, p_file_data );
//		MemoryRead( &num_single, sizeof( int ), 1, p_file_data );
//
//		sbytes = ( bytes + 3 ) / 4;
//		p_double = new /*(Mem::Manager::sHandle().TopDownHeap())*/ uint32[dbytes + sbytes];
//		memcpy( p_double, p_temp, dbytes * 4 );
//		MemoryRead( &p_double[dbytes], bytes, 1, p_file_data );
//
//		delete p_temp;
//	}
//
////	// Grab a buffer for the raw vertex data weights stream (if present), and read it.
////	float* p_vertex_weights = ( m_flags & 0x10 ) ? new float[num_vertices * 3] : NULL;
////	if( p_vertex_weights )
////	{
////		MemoryRead( p_vertex_weights, sizeof( float ) * 3, num_vertices, p_file_data );
////	}
////	
////	// Grab a buffer for the raw vertex data bone indices stream (if present), and read it.
////	uint16* p_vertex_bone_indices = ( m_flags & 0x10 ) ? new uint16[num_vertices * 3] : NULL;
////	if( p_vertex_bone_indices )
////	{
////		MemoryRead( p_vertex_bone_indices, sizeof( uint16 ) * 3, num_vertices, p_file_data );
////	}
//
//	uint16 * p_temp_col_remap = NULL;
//	uint16 * p_temp_uv_remap = NULL;
//
//	// Grab a buffer for the raw vertex texture coordinate stream (if present), and read it.
//	int		num_tc_sets			= 0;
//	float*	p_vertex_tex_coords	= NULL;
//	if( m_flags & 0x01 )
//	{
//		MemoryRead( &num_tc_sets, sizeof( int ), 1, p_file_data );
//		
//		float * p_temp_tex_coords = new (Mem::Manager::sHandle().TopDownHeap()) float[num_vertices * 2 * num_tc_sets];
//		p_vertex_tex_coords = new float[num_vertices * 2 * num_tc_sets];
//		MemoryRead( p_temp_tex_coords, sizeof( float ) * 2 * num_tc_sets, num_vertices, p_file_data );
//
//		// Convert to friendlier non-interleaved format.
//		for ( int lp = 0; lp < num_vertices; lp++ )
//		{
//			for ( int lp2 = 0; lp2 < num_tc_sets; lp2++ )
//			{
//				p_vertex_tex_coords[(lp2*2*num_vertices)+(lp*2)+0] = p_temp_tex_coords[(lp2*2)+(lp*2*num_tc_sets)+0];
//				p_vertex_tex_coords[(lp2*2*num_vertices)+(lp*2)+1] = p_temp_tex_coords[(lp2*2)+(lp*2*num_tc_sets)+1];
//			}
//		}
//
//		// Create temp uv mapping buffer.
//		p_temp_uv_remap = num_tc_sets ? new (Mem::Manager::sHandle().TopDownHeap()) uint16[num_vertices*num_tc_sets] : NULL;
//
//		for ( int c = 0; c < num_vertices; c++ )
//		{
//			for ( int u = 0; u < num_tc_sets; u++ )
//			{
//				p_temp_uv_remap[(u*num_vertices)+c] = (u*num_vertices)+c;
//			}
//		}
//
//		bool reduce = true;
//		if ( m_flags & 0x04 ) reduce = false;	// No uv reduction if normals are present.
//
//		if ( reduce )
//		{
//			// Copy back to temp coords.
//			memcpy( p_temp_tex_coords, p_vertex_tex_coords, num_vertices * num_tc_sets * 2 * sizeof( float ) );
//			delete [] p_vertex_tex_coords;
//			float * p_unique_tex_coords = new (Mem::Manager::sHandle().TopDownHeap()) float[num_vertices * 2 * num_tc_sets];
//
//			// Now, squeeze down to only unique uvs.
//			int numUnique = 0;
//			for ( int lp = 0; lp < num_vertices; lp++ )
//			{
//				for ( int lp2 = 0; lp2 < num_tc_sets; lp2++ )
//				{
//					bool found = false;
//					for ( int lp3 = 0; lp3 < numUnique; lp3++ )
//					{
//						if ( ( p_unique_tex_coords[(lp3*2)+0] == p_temp_tex_coords[(lp2*2*num_vertices)+(lp*2)+0] ) &&
//							 ( p_unique_tex_coords[(lp3*2)+1] == p_temp_tex_coords[(lp2*2*num_vertices)+(lp*2)+1] ) )
//						{
//							// Found a match.
//							p_temp_uv_remap[(lp2*num_vertices)+lp] = lp3;
//							found = true;
//							break;
//						}
//					}
//					if ( !found )
//					{
//						p_unique_tex_coords[(numUnique*2)+0] = p_temp_tex_coords[(lp2*2*num_vertices)+(lp*2)+0];
//						p_unique_tex_coords[(numUnique*2)+1] = p_temp_tex_coords[(lp2*2*num_vertices)+(lp*2)+1];
//						p_temp_uv_remap[(lp2*num_vertices)+lp] = numUnique;
//						numUnique++;
//					}
//				}
//			}
//			// Allocate only the space we need.
//			p_vertex_tex_coords = new float[numUnique * 2];
//			memcpy( p_vertex_tex_coords, p_unique_tex_coords, numUnique * 2 * sizeof( float ) );
//			delete [] p_unique_tex_coords;
//		}
//
//		delete p_temp_tex_coords;
//	}
//	
//	// Grab a buffer for the raw vertex colors stream (if present), and read it.
//	uint32* p_vertex_colors = ( m_flags & 0x02 ) ? new uint32[num_vertices] : NULL;
//	if( p_vertex_colors )
//	{
//		MemoryRead( p_vertex_colors, sizeof( uint32 ), num_vertices, p_file_data );
//	
//		// Create temp col mapping buffer.
//		p_temp_col_remap = p_vertex_colors ? new (Mem::Manager::sHandle().TopDownHeap()) uint16[num_vertices] : NULL;
//
//		for ( int c = 0; c < num_vertices; c++ )
//		{
//			p_temp_col_remap[c] = c;
//		}
//
//		bool reduce = true;
//		if ( m_flags & 0x04 ) reduce = false;	// No color reduction if normals are present.
//		if ( m_flags & 0x800 ) reduce = false;	// No color reduction if vc wibble data is present.
//		if ( !NxNgc::EngineGlobals.reduceColors ) reduce = false;	// No color reduction if vc wibble data is present.
//
//		if ( reduce )
//		{
//			uint32 * p_temp_colors = new (Mem::Manager::sHandle().TopDownHeap()) uint32[num_vertices];
//			memcpy( p_temp_colors, p_vertex_colors, num_vertices * 4 );
//			delete [] p_vertex_colors;
//			uint32* p_unique_colors = new (Mem::Manager::sHandle().TopDownHeap()) uint32[num_vertices];
//
//			// Now, squeeze down to only unique uvs.
//			int numUnique = 0;
//			for ( int lp = 0; lp < num_vertices; lp++ )
//			{
//				bool found = false;
//				for ( int lp2 = 0; lp2 < numUnique; lp2++ )
//				{
//					if ( p_unique_colors[lp2] == p_temp_colors[lp] )
//					{
//						// Found a match.
//						p_temp_col_remap[lp] = lp2;
//						found = true;
//						break;
//					}
//				}
//				if ( !found )
//				{
//					p_unique_colors[numUnique] = p_temp_colors[lp];
//					p_temp_col_remap[lp] = numUnique;
//					numUnique++;
//				}
//			}
//			// Allocate only the space we need.
//			p_vertex_colors = new uint32[numUnique];
//			memcpy( p_vertex_colors, p_unique_colors, numUnique * 4 );
//			delete [] p_unique_colors;
//			delete p_temp_colors;
//		}
//	}
//
//	// Grab a buffer for the raw vertex colors stream (if present), and read it.
//	char* p_vc_wibble_indices = ( m_flags & 0x800 ) ? new char[num_vertices] : NULL;
//	if( p_vc_wibble_indices )
//	{
//		MemoryRead( p_vc_wibble_indices, sizeof( char ), num_vertices, p_file_data );
//	}
//	
//	NxNgc::sMesh*	p_mesh0 = NULL;
//
//	for( uint m = 0; m < p_geom->m_num_mesh; ++m )
//	{
//		unsigned long	material_checksum;
//		uint32			flags;
//		int				num_indices;
//		float			sphere[4];
//
//		NxNgc::sMesh*	p_mesh = new NxNgc::sMesh;
//		if ( m == 0 )
//		{
//			p_mesh0 = p_mesh;
//			p_mesh0->mp_envBuffer = NULL;
//		}
//		
//		MemoryRead( &material_checksum,	sizeof( unsigned long ), 1, p_file_data );
//		MemoryRead( &flags,	sizeof( uint32 ), 1, p_file_data );
//		MemoryRead( sphere, sizeof( float ), 4, p_file_data );
//		MemoryRead( &num_indices, sizeof( int ), 1, p_file_data );
//
//		uint16* p_indices = new uint16[num_indices];
//		MemoryRead( p_indices, sizeof( uint16 ), num_indices, p_file_data );
//
//		// Deal with skater shadow flag.
//		if( flags & (0x4000|0x400) )
//		{
//			p_mesh->m_flags |= NxNgc::sMesh::MESH_FLAG_NO_SKATER_SHADOW;
//		}
//		// Deal with vertex color wibble flag.
//		if( flags & (0x800) )
//		{
//			p_mesh->m_flags |= NxNgc::sMesh::MESH_FLAG_VERTEX_COLOR_WIBBLE;
//		}
//		
//		if( m_flags & (0x80000) )
//		{
//			p_mesh->m_flags |= NxNgc::sMesh::MESH_FLAG_GRASS;
//		}
//		
//		// Create the mesh from the raw data.
//		p_mesh->Initialize( num_vertices,
//							p_vertex_positions,
//#ifdef SHORT_VERT
//							p_vertex_positions16,
//							shift,
//							off_x,
//							off_y,
//							off_z,
//#endif		// SHORT_VERT
//							p_vertex_normals,
//							p_vertex_tex_coords,
//							p_mesh0->mp_envBuffer,
//							num_tc_sets,
//							p_vertex_colors,
//							num_indices,
//							p_indices,		// pos
//							p_temp_col_remap,		// color
//							p_temp_uv_remap,		// uv
//							material_checksum,
//							p_geom->mp_scene->GetEngineScene(),
//							num_double,
//							p_double,
//							num_single,
//							m,
//							GX_TRIANGLESTRIP,
//							bone_idx,
//							p_vc_wibble_indices );
//
//		// Assign env buffer to mesh 0 if assigned.
//		if ( ( m > 0 ) && p_mesh->mp_envBuffer ) p_mesh0->mp_envBuffer = p_mesh->mp_envBuffer;
//		
//		// Add the mesh to the sector.
//		p_geom->AddMesh( p_mesh );
//	
//		// Done with the raw index data.
////		delete[] p_indices;

//		if ( p_data->m_size )
//		{
//			// Setup the DL.
//			p_mesh->mp_dl = p_data;
//			p_data = (NxNgc::sDLHeader*)(&((char*)&p_data[1])[p_data->m_size]);
//
//			p_mesh->mp_dl->mp_material_dl = NULL;
//			p_mesh->mp_dl->m_material_dl_size = 0;
//
//			// Find the material.
//			NxNgc::sMaterialHeader * p_mat_list = p_geom->mp_scene->GetEngineScene()->mp_material_list;
//			bool found = false;
//			for ( unsigned int lp = 0; lp < p_geom->mp_scene->GetEngineScene()->mp_scene_data->m_num_materials; lp++ )
//			{
//				if ( p_mat_list->m_checksum == p_mesh->mp_dl->m_material.checksum )
//				{
//					// Found it!
//					p_mesh->mp_dl->m_material.p_header = p_mat_list;
//
//					// Construct the DL.
//					Mem::Manager::sHandle().TopDownHeap()->PushAlign( 32 );
//#define DL_BUILD_SIZE (4*1024)
//					uint8 * p_build_dl = new (Mem::Manager::sHandle().TopDownHeap()) uint8[DL_BUILD_SIZE];
//					DCFlushRange ( p_build_dl, DL_BUILD_SIZE );
//
//					GXBeginDisplayList ( p_build_dl, DL_BUILD_SIZE );
//					GXResetWriteGatherPipe();
//
////					// ---------------- Begin Generate display list.
//////					GXSetNumTevStages( 1 );
////					GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
////					GXSetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//////					GXSetNumTexGens( 0 );
////					GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
////					GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
////////					GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
////					GXSetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR );
////					GXSetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA );
////					GXSetTevColorIn ( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC );
////					GXSetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_VTX, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
////					GXSetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//
//					NxNgc::sMaterialHeader * p_mat = p_mesh->mp_dl->m_material.p_header;
//					NxNgc::sMaterialPassHeader * p_pass = (NxNgc::sMaterialPassHeader *)&p_mat[1];
//
//					int lp;
//
//					NxNgc::MaterialBegin(); for ( lp = 0; lp < p_mat->m_passes; lp++ ) NxNgc::MaterialLayer( p_pass, lp, MATERIAL_GROUP_CP );	NxNgc::MaterialEnd( &p_mesh->mp_dl->m_tev_stages, &p_mesh->mp_dl->m_tex_gens );
//					NxNgc::MaterialBegin(); for ( lp = 0; lp < p_mat->m_passes; lp++ ) NxNgc::MaterialLayer( p_pass, lp, MATERIAL_GROUP_SU );   NxNgc::MaterialEnd( &p_mesh->mp_dl->m_tev_stages, &p_mesh->mp_dl->m_tex_gens );
//					NxNgc::MaterialBegin(); for ( lp = 0; lp < p_mat->m_passes; lp++ ) NxNgc::MaterialLayer( p_pass, lp, MATERIAL_GROUP_B );    NxNgc::MaterialEnd( &p_mesh->mp_dl->m_tev_stages, &p_mesh->mp_dl->m_tex_gens );
//					NxNgc::MaterialBegin(); for ( lp = 0; lp < p_mat->m_passes; lp++ ) NxNgc::MaterialLayer( p_pass, lp, MATERIAL_GROUP_C );    NxNgc::MaterialEnd( &p_mesh->mp_dl->m_tev_stages, &p_mesh->mp_dl->m_tex_gens );
//					NxNgc::MaterialBegin(); for ( lp = 0; lp < p_mat->m_passes; lp++ ) NxNgc::MaterialLayer( p_pass, lp, MATERIAL_GROUP_A );    NxNgc::MaterialEnd( &p_mesh->mp_dl->m_tev_stages, &p_mesh->mp_dl->m_tex_gens );
//
//					GXSetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_VTX, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//					GXSetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//					GXSetAlphaCompare(GX_GREATER, p_mat->m_alpha_cutoff, GX_AOP_AND, GX_GREATER, p_mat->m_alpha_cutoff );
//					// ---------------- End Generate display list.
//
//					uint32 size = GXEndDisplayList();
//
////					// If we already have a display list & this one will fit, just use the existing piece of memory.
////					uint8 * p_dl;
////					if ( mp_display_list && ( size <= m_display_list_size ) )
////					{
////						p_dl = mp_display_list;
////					}
////					else
////					{
////						if ( mp_display_list )
////						{
////							//gMatBytes -= m_display_list_size;
////							delete mp_display_list;
////						}
//						p_mesh->mp_dl->mp_material_dl = new uint8[size];
//						//gMatBytes += size;
////					}
//
//					memcpy ( p_mesh->mp_dl->mp_material_dl, p_build_dl, size );
//					DCFlushRange ( p_mesh->mp_dl->mp_material_dl, size );
//
//					delete p_build_dl;
//
//					p_mesh->mp_dl->m_material_dl_size = size;
//					Mem::Manager::sHandle().TopDownHeap()->PopAlign();
//
//
//
//
//
//					found = true;
//					break;
//				}
//				p_mat_list = (NxNgc::sMaterialHeader *)&(((char*)(&p_mat_list[1]))[p_mat_list->m_skip_bytes]);
//			}
//			if ( !found )
//			{
//				Dbg_MsgAssert( 0, ( "Unable to find material checksum 0x%08x\n", p_mesh->mp_dl->m_material.checksum ));
//				p_mesh->mp_dl->m_material.p_header = NULL;
//			}
//		}
//	}

//	if ( p_temp_col_remap ) delete [] p_temp_col_remap;
//	if ( p_temp_uv_remap ) delete [] p_temp_uv_remap;
////	if ( p_double ) delete p_double;
//
//	// Done with the raw vertex data.
////	delete[] p_vertex_colors;
////	delete[] p_vertex_tex_coords;
////	delete[] p_vertex_normals;
//#ifdef SHORT_VERT
//	delete[] p_vertex_positions;
//#endif		// SHORT_VERT
//
//	// Set the data pointer to the new position on return.
//	*pp_mem = p_file_data;
//
//	return p_data;

	Dbg_MsgAssert( false, ( "Not yet implented!!!!!!!" ) );
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
NxNgc::sObjectHeader* CNgcSector::LoadFromFile( NxNgc::sObjectHeader* p_data )
{
	Dbg_Assert( mp_geom );

	CNgcGeom *p_geom = static_cast<CNgcGeom*>( mp_geom );
	
	// Hook up vars.
	p_geom->m_num_mesh = p_data->m_num_meshes;
//  	p_geom->mp_scene->GetEngineScene()->mp_dl->mp_object_header = p_data;

	char * p_skin = (char *)&p_data[1];
	int nbytes = p_data->m_skin.num_bytes;
	p_data->m_skin.p_data = p_skin;
	NxNgc::sDLHeader* p_dl = (NxNgc::sDLHeader*)&p_skin[nbytes];

	m_flags = p_dl->m_flags;
	SetChecksum( p_dl->m_checksum );

	for( uint m = 0; m < p_geom->m_num_mesh; ++m )
	{
		NxNgc::sMesh*	p_mesh = new NxNgc::sMesh;

		p_geom->AddMesh( p_mesh );

		// Setup mesh flags.
		if ( *((uint32*)(&p_dl->mp_col_pool)) & ( 0x00000100 | 0x00000400 ) )
		{
			p_mesh->m_flags |= NxNgc::sMesh::MESH_FLAG_NO_SKATER_SHADOW;
		}
		p_dl->mp_col_pool = NULL;

		// Setup bottom y.
		p_mesh->m_bottom_y = *((float*)(&p_dl->mp_pos_pool));
		p_dl->mp_pos_pool = NULL;

		if ( p_dl->m_size )
		{
			// Setup the DL.
			p_mesh->mp_dl = p_dl;
			p_mesh->mp_dl->mp_object_header = p_data;
			p_mesh->m_bone_idx = p_mesh->mp_dl->mp_object_header->m_bone_index;

			// Fix up the bounding sphere for billboards.
			switch ( p_mesh->mp_dl->mp_object_header->m_billboard_type )
			{
				case 1:
				case 2:
					p_mesh->mp_dl->m_sphere[X] += p_mesh->mp_dl->mp_object_header->m_origin[X];
					p_mesh->mp_dl->m_sphere[Y] += p_mesh->mp_dl->mp_object_header->m_origin[Y];
					p_mesh->mp_dl->m_sphere[Z] += p_mesh->mp_dl->mp_object_header->m_origin[Z];
					break;
				default:
					break;
			}

			p_dl = (NxNgc::sDLHeader*)&(((char *)&p_dl[1])[p_dl->m_size]);

			// Find the material.
			NxNgc::sMaterialHeader * p_mat_list = p_geom->mp_scene->GetEngineScene()->mp_material_header;
			bool found = false;
			for ( unsigned int lp = 0; lp < p_geom->mp_scene->GetEngineScene()->mp_scene_data->m_num_materials; lp++ )
			{
				if ( p_mat_list->m_checksum == p_mesh->mp_dl->m_material.checksum )
				{
					// Found it!
					p_mesh->mp_dl->m_material.p_header = p_mat_list;

//					NxNgc::MaterialBuild( p_mesh, p_geom->mp_scene->GetEngineScene(), false, true ); 

					found = true;
					break;
				}
				++p_mat_list;
			}
			if ( !found )
			{
				Dbg_MsgAssert( 0, ( "Unable to find material checksum 0x%08x\n", p_mesh->mp_dl->m_material.checksum ));
				p_mesh->mp_dl->m_material.p_header = NULL;
			}
		}
	}

	return (NxNgc::sObjectHeader*)p_dl;
}



/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Here's a machine specific implementation of the CSector
// and we will also have a CNgcSector, CNgcSector, even a CPcSector
// maybe in the future we will have a CPS3Sector?

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcSector::plat_set_color(Image::RGBA rgba)
{
	// Set values
	m_rgba = rgba;

	// Engine call here
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CNgcSector::plat_clear_color()
{
	// Set to white
	plat_set_color(Image::RGBA(128, 128, 128, 128));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcSector::plat_set_visibility(uint32 mask)
{
//	// Set values
//	m_visible = mask;
//
//	// Engine call here
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CNgcSector::plat_set_active( bool on )
{
//	// Set values
//	m_active = on;
//
//	if( m_mesh_array )
//	{
//		for( uint m = 0; m < m_num_mesh; ++m )
//		{
//			NxNgc::sMesh *p_mesh = m_mesh_array[m];
//			p_mesh->SetActive( on );
//		}
//	}
}
 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

//bool CNgcSector::plat_is_active() const
//{
//	return m_active;
//}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

 void CNgcSector::plat_set_world_position(const Mth::Vector& pos)
 {
/*
	Mth::Vector new_offset = pos - m_pos_offset;

	// Go through and adjust the individual meshes.
	for( uint32 i = 0; i < m_num_mesh; ++i )
	{
		NxNgc::sMesh *p_mesh = m_mesh_array[i];
		p_mesh->Move( new_offset );
	}
*/
 }

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const Mth::CBBox &CNgcSector::plat_get_bounding_box( void ) const
{
	static Mth::CBBox dummy;
	
	//	return m_bbox;
	return dummy;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Vector &	CNgcSector::plat_get_world_position() const
{
	return m_pos_offset;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

//void CNgcSector::plat_set_y_rotation(Mth::ERot90 rot)
//{
//	// Engine call here
//	// Garrett: TEMP just set the world matrix
//	Mth::Matrix rot_mat;
//	CreateRotateYMatrix(rot_mat, (float) rot * (Mth::PI * 0.5f));
//
//	m_world_matrix[X] = rot_mat[X];
//	m_world_matrix[Y] = rot_mat[Y];
//	m_world_matrix[Z] = rot_mat[Z];
//}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CNgcSector::plat_set_shatter(bool on)	
{
	if( on && mp_geom )
	{
		Shatter( mp_geom );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSector * CNgcSector::plat_clone(bool instance, CScene *p_dest_scene)
{
	CNgcSector *p_Ngc_sector = new CNgcSector();

	/*


	// Copies over much of the standard stuff, individual stuff will be overwritten later.
	CopyMemory( p_Ngc_sector, this, sizeof( CNgcSector ));

	if( instance )
	{
		Dbg_Assert( 0 );
	}
	else
	{
		// Need to create a new set of meshes. First create the mesh pointer array...
		p_Ngc_sector->m_mesh_array = new NxNgc::sMesh*[m_num_mesh];

		// ...then clone the meshes themselves.
		for( uint32 i = 0; i < m_num_mesh; ++i )
		{
			p_Ngc_sector->m_mesh_array[i] = m_mesh_array[i]->Clone();
		}

		// Grab a temporary workspace buffer.
		NxNgc::sScene *p_scene								= ( static_cast<CNgcScene*>( p_dest_scene ))->GetEngineScene();
		NxNgc::sMesh **p_temp_opaque_mesh_buffer			= new NxNgc::sMesh*[p_scene->m_num_opaque_entries];
		NxNgc::sMesh **p_temp_semitransparent_mesh_buffer	= new NxNgc::sMesh*[p_scene->m_num_semitransparent_entries];

		// Copy meshes over into the temporary workspace buffer.
		for( int i = 0; i < p_scene->m_num_opaque_entries; ++i )
		{
			p_temp_opaque_mesh_buffer[i] = p_scene->m_opaque_meshes[i];
		}
		for( int i = 0; i < p_scene->m_num_semitransparent_entries; ++i )
		{
			p_temp_semitransparent_mesh_buffer[i] = p_scene->m_semitransparent_meshes[i];
		}

		// Delete current mesh arrays.
		delete [] p_scene->m_opaque_meshes;
		delete [] p_scene->m_semitransparent_meshes;

		// Include new meshes in count.
		p_scene->CountMeshes( p_Ngc_sector->m_num_mesh, p_Ngc_sector->m_mesh_array );

		// Allocate new mesh arrays.
		p_scene->CreateMeshArrays();

		// Copy old mesh data back in.
		for( int i = 0; i < p_scene->m_num_opaque_entries; ++i )
		{
			p_scene->m_opaque_meshes[i] = p_temp_opaque_mesh_buffer[i];
		}
		for( int i = 0; i < p_scene->m_num_semitransparent_entries; ++i )
		{
			p_scene->m_semitransparent_meshes[i] = p_temp_semitransparent_mesh_buffer[i];
		}

		// Add new meshes.
		p_scene->AddMeshes( p_Ngc_sector->m_num_mesh, p_Ngc_sector->m_mesh_array );

		// Sort the meshes.
		p_scene->SortMeshes();
	}
*/
	return p_Ngc_sector;
}




} // Namespace Nx  			
				
				
