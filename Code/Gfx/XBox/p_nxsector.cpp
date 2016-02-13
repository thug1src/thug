///////////////////////////////////////////////////////////////////////////////
// p_NxSector.cpp

#include	<sys/file/filesys.h>

#include 	"gfx/xbox/p_NxSector.h"
#include 	"gfx/xbox/p_NxGeom.h"
#include 	"gfx/NxMiscFX.h"
#include 	"gfx/xbox/nx/grass.h"
#include 	"gfx/xbox/nx/billboard.h"

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

//CXboxSector::CXboxSector() : mp_init_mesh_list( NULL ), m_mesh_array( NULL ), m_visible( 0xDEADBEEF )
CXboxSector::CXboxSector()
{
	m_pos_offset.Set( 0.0f, 0.0f, 0.0f );
}




#define MemoryRead( dst, size, num, src )	CopyMemory(( dst ), ( src ), (( num ) * ( size )));	\
											( src ) += (( num ) * ( size ))

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxSector::LoadFromMemory( void **pp_mem )
{
	Dbg_Assert( mp_geom );

	uint8		*p_data	= (uint8*)( *pp_mem );
	CXboxGeom	*p_geom = static_cast<CXboxGeom*>( mp_geom );
	
	// Read sector checksum.
	uint32 sector_checksum;
	MemoryRead( &sector_checksum, sizeof( uint32 ), 1, p_data );
	SetChecksum( sector_checksum );

	// Read bone index.
	int bone_idx;
	MemoryRead( &bone_idx, sizeof( int ), 1, p_data );

	// Read sector flags.
	MemoryRead( &m_flags, sizeof( int ), 1, p_data );

	// Read number of meshes.
	MemoryRead( &p_geom->m_num_mesh, sizeof( uint ), 1, p_data );

	// Read bounding box.
	float bbox[6];
	MemoryRead( &bbox[0], sizeof( float ), 6, p_data );
	Mth::Vector	inf( bbox[0], bbox[1], bbox[2] );
	Mth::Vector	sup( bbox[3], bbox[4], bbox[5] );
	p_geom->m_bbox.Set( inf, sup );
		
	// Read bounding sphere.
	float bsphere[4];
	MemoryRead( &bsphere[0], sizeof( float ), 4, p_data );
		
	// Read billboard data if present.
	uint32		billboard_type;
	Mth::Vector	billboard_origin;
	Mth::Vector billboard_pivot_pos;
	Mth::Vector billboard_pivot_axis;
	if( m_flags & 0x00800000UL )
	{
		MemoryRead( &billboard_type,			sizeof( uint32 ), 1, p_data );
		MemoryRead( &billboard_origin[X],		sizeof( float ) * 3, 1, p_data );
		MemoryRead( &billboard_pivot_pos[X],	sizeof( float ) * 3, 1, p_data );
		MemoryRead( &billboard_pivot_axis[X],	sizeof( float ) * 3, 1, p_data );

		billboard_origin[Z]		= -billboard_origin[Z];
		billboard_pivot_pos[Z]	= -billboard_pivot_pos[Z];
	}

	// Read num vertices.
	int num_vertices;
	MemoryRead( &num_vertices, sizeof( int ), 1, p_data );
	
	// Read vertex data stride.
	int vertex_data_stride;
	MemoryRead( &vertex_data_stride, sizeof( int ), 1, p_data );

	// We want all the temporary buffer allocations to come off of the top down heap.
	Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().TopDownHeap());
	
	// Grab a buffer for the raw vertex data position stream, and read it.
	float* p_vertex_positions = new float[num_vertices * 3];
	MemoryRead( p_vertex_positions, sizeof( float ) * 3, num_vertices, p_data );
	
	// Grab a buffer for the raw vertex data normal stream (if present), and read it.
	float* p_vertex_normals = ( m_flags & 0x04 ) ? new float[num_vertices * 3] : NULL;
	if( p_vertex_normals )
	{
		MemoryRead( p_vertex_normals, sizeof( float ) * 3, num_vertices, p_data );
	}

	// Grab a buffer for the raw vertex data weights stream (if present), and read it.
	uint32* p_vertex_weights = ( m_flags & 0x10 ) ? new uint32[num_vertices] : NULL;
	if( p_vertex_weights )
	{
		MemoryRead( p_vertex_weights, sizeof( uint32 ), num_vertices, p_data );
	}
	
	// Grab a buffer for the raw vertex data bone indices stream (if present), and read it.
	uint16* p_vertex_bone_indices = ( m_flags & 0x10 ) ? new uint16[num_vertices * 4] : NULL;
	if( p_vertex_bone_indices )
	{
		MemoryRead( p_vertex_bone_indices, sizeof( uint16 ) * 4, num_vertices, p_data );
	}

	// Grab a buffer for the raw vertex texture coordinate stream (if present), and read it.
	int		num_tc_sets			= 0;
	float*	p_vertex_tex_coords	= NULL;
	if( m_flags & 0x01 )
	{
		MemoryRead( &num_tc_sets, sizeof( int ), 1, p_data );
		
		if( num_tc_sets > 0 )
		{
			p_vertex_tex_coords = new float[num_vertices * 2 * num_tc_sets];
			MemoryRead( p_vertex_tex_coords, sizeof( float ) * 2 * num_tc_sets, num_vertices, p_data );
		}
	}
	
	// Grab a buffer for the raw vertex colors stream (if present), and read it.
	DWORD* p_vertex_colors = ( m_flags & 0x02 ) ? new DWORD[num_vertices] : NULL;
	if( p_vertex_colors )
	{
		MemoryRead( p_vertex_colors, sizeof( DWORD ), num_vertices, p_data );
	}

	// Grab a buffer for the vertex color wibble stream (if present), and read it.
	char* p_vc_wibble_indices = ( m_flags & 0x800 ) ? new char[num_vertices] : NULL;
	if( p_vc_wibble_indices )
	{
		MemoryRead( p_vc_wibble_indices, sizeof( char ), num_vertices, p_data );
	}
	
	// Remove TopDownHeap context.
	Mem::Manager::sHandle().PopContext();

	// Preprocess verts that require cutscene scaling.
	NxXbox::ApplyMeshScaling( p_vertex_positions, num_vertices );

	for( uint m = 0; m < p_geom->m_num_mesh; ++m )
	{
		unsigned long	material_checksum;
		unsigned int	flags, num_lod_index_levels;

		NxXbox::sMesh*	p_mesh = new NxXbox::sMesh;

		// Read bounding sphere and box data.
		float		rad;
		Mth::Vector inf, sup, cen;
		MemoryRead( &cen[X], sizeof( float ), 3, p_data );
		MemoryRead( &rad, sizeof( float ), 1, p_data );
		MemoryRead( &inf[X], sizeof( float ), 3, p_data );
		MemoryRead( &sup[X], sizeof( float ), 3, p_data );

		// Read and deal with flags, including skater shadow flag.
		MemoryRead( &flags, sizeof( uint32 ), 1, p_data );
		if( flags & 0x400 )
		{
			p_mesh->m_flags |= NxXbox::sMesh::MESH_FLAG_NO_SKATER_SHADOW;
		}
		if( flags & NxXbox::sMesh::MESH_FLAG_UNLIT )
		{
			p_mesh->m_flags |= NxXbox::sMesh::MESH_FLAG_UNLIT;
		}

		// The material checksum for this mesh.
		MemoryRead( &material_checksum,	sizeof( unsigned long ), 1, p_data );

		// How many levels of LOD indices? Should be at least 1!
		MemoryRead( &num_lod_index_levels, sizeof( unsigned int ), 1, p_data );

		// Can have up to 8 levels of LOD indices.
		uint16*	p_indices[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
		int		num_indices[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

		for( unsigned int lod_level = 0; lod_level < num_lod_index_levels; ++lod_level )
		{
			MemoryRead( &num_indices[lod_level], sizeof( int ), 1, p_data );
		
			// Again, we want all the temporary buffer allocations to come off of the top down heap.
			Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().TopDownHeap());
			p_indices[lod_level] = new uint16[num_indices[lod_level]];
			Mem::Manager::sHandle().PopContext();

			MemoryRead( p_indices[lod_level], sizeof( uint16 ), num_indices[lod_level], p_data );
		}

		// Set the load order of this mesh.
		p_mesh->m_load_order = m;

		// Set up the mesh.
		p_mesh->Initialize(	num_vertices,
							p_vertex_positions,
							p_vertex_normals,
							p_vertex_tex_coords,
							num_tc_sets,
							p_vertex_colors,
							num_lod_index_levels,
							num_indices,
							p_indices,
							material_checksum,
							p_geom->mp_scene->GetEngineScene(),
							p_vertex_bone_indices,
							p_vertex_weights,
							( flags & 0x800 ) ? p_vc_wibble_indices : NULL );
		
		// Set the bounding data (sphere and box) for the mesh.
		p_mesh->SetBoundingData( cen, rad, inf, sup );

		// Add the bounding data to the scene.
		p_geom->mp_scene->GetEngineScene()->m_bbox.AddPoint( inf );
		p_geom->mp_scene->GetEngineScene()->m_bbox.AddPoint( sup );

		// Set up as a billboard if required.
		if( m_flags & 0x00800000UL )
		{
			p_mesh->SetBillboardData( billboard_type, billboard_pivot_pos, billboard_pivot_axis );
			NxXbox::BillboardManager.AddEntry( p_mesh );
		}

		// Flag the mesh as being a shadow volume if applicable.
		if( m_flags & 0x200000 )
		{
			p_mesh->m_flags |= NxXbox::sMesh::MESH_FLAG_SHADOW_VOLUME;
		}

		// Add the mesh to the attached CXboxGeom.
		p_geom->AddMesh( p_mesh );

		// Set the mesh bone index (mostly not applicable).
		p_mesh->SetBoneIndex( bone_idx );
	
		// Test code - if the mesh is mapped with a grass texture, add grass meshes.
		AddGrass( p_geom, p_mesh );
		
		// Done with the raw index data.
		for( int lod_level = 0; lod_level < 8; ++lod_level )
		{
			delete[] p_indices[lod_level];
		}
	}

	// Recount the number of meshes in case any have been added.
	p_geom->m_num_mesh = p_geom->mp_init_mesh_list->CountItems();
	
	// Done with the raw vertex data.
	delete[] p_vc_wibble_indices;
	delete[] p_vertex_colors;
	delete[] p_vertex_tex_coords;
	delete[] p_vertex_bone_indices;
	delete[] p_vertex_weights;
	delete[] p_vertex_normals;
	delete[] p_vertex_positions;
	
	// Set the data pointer to the new position on return.
	*pp_mem = p_data;

	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxSector::LoadFromFile( void* p_file )
{
	Dbg_Assert( mp_geom );

	CXboxGeom *p_geom = static_cast<CXboxGeom*>( mp_geom );
	
	// Read sector checksum.
	uint32 sector_checksum;
	File::Read( &sector_checksum, sizeof( uint32 ), 1, p_file );
	SetChecksum( sector_checksum );

	// Read bone index.
	int bone_idx;
	File::Read( &bone_idx, sizeof( int ), 1, p_file );

	// Read sector flags.
	File::Read( &m_flags, sizeof( int ), 1, p_file );

	// Read number of meshes.
	File::Read( &p_geom->m_num_mesh, sizeof( uint ), 1, p_file );

	// Read bounding box.
	float bbox[6];
	File::Read( &bbox[0], sizeof( float ), 6, p_file );
	Mth::Vector	inf( bbox[0], bbox[1], bbox[2] );
	Mth::Vector	sup( bbox[3], bbox[4], bbox[5] );
	p_geom->m_bbox.Set( inf, sup );
		
	// Read bounding sphere.
	float bsphere[4];
	File::Read( &bsphere[0], sizeof( float ), 4, p_file );
		
	// Read billboard data if present.
	uint32		billboard_type;
	Mth::Vector	billboard_origin;
	Mth::Vector billboard_pivot_pos;
	Mth::Vector billboard_pivot_axis;
	if( m_flags & 0x00800000UL )
	{
		File::Read( &billboard_type,			sizeof( uint32 ), 1, p_file );
		File::Read( &billboard_origin[X],		sizeof( float ) * 3, 1, p_file );
		File::Read( &billboard_pivot_pos[X],	sizeof( float ) * 3, 1, p_file );
		File::Read( &billboard_pivot_axis[X],	sizeof( float ) * 3, 1, p_file );

		billboard_origin[Z]		= -billboard_origin[Z];
		billboard_origin[W]		= 0.0f;

		billboard_pivot_pos[Z]	= -billboard_pivot_pos[Z];
		billboard_pivot_pos[W]	= 0.0f;

		billboard_pivot_axis[W]	= 0.0f;
	}

	// Read num vertices.
	int num_vertices;
	File::Read( &num_vertices, sizeof( int ), 1, p_file );
	
	// Read vertex data stride.
	int vertex_data_stride;
	File::Read( &vertex_data_stride, sizeof( int ), 1, p_file );

	// We want all the temporary buffer allocations to come off of the top down heap.
	Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().TopDownHeap());
	
	// Grab a buffer for the raw vertex data position stream, and read it.
	float* p_vertex_positions = new float[num_vertices * 3];
	File::Read( p_vertex_positions, sizeof( float ) * 3, num_vertices, p_file );
	
	// Grab a buffer for the raw vertex data normal stream (if present), and read it.
	float* p_vertex_normals = ( m_flags & 0x04 ) ? new float[num_vertices * 3] : NULL;
	if( p_vertex_normals )
	{
		File::Read( p_vertex_normals, sizeof( float ) * 3, num_vertices, p_file );
	}

	// Grab a buffer for the raw vertex data weights stream (if present), and read it.
	uint32* p_vertex_weights = ( m_flags & 0x10 ) ? new uint32[num_vertices] : NULL;
	if( p_vertex_weights )
	{
		File::Read( p_vertex_weights, sizeof( uint32 ), num_vertices, p_file );
	}
	
	// Grab a buffer for the raw vertex data bone indices stream (if present), and read it.
	uint16* p_vertex_bone_indices = ( m_flags & 0x10 ) ? new uint16[num_vertices * 4] : NULL;
	if( p_vertex_bone_indices )
	{
		File::Read( p_vertex_bone_indices, sizeof( uint16 ) * 4, num_vertices, p_file );
	}

	// Grab a buffer for the raw vertex texture coordinate stream (if present), and read it.
	int		num_tc_sets			= 0;
	float*	p_vertex_tex_coords	= NULL;
	if( m_flags & 0x01 )
	{
		File::Read( &num_tc_sets, sizeof( int ), 1, p_file );
		
		if( num_tc_sets > 0 )
		{
			p_vertex_tex_coords = new float[num_vertices * 2 * num_tc_sets];
			File::Read( p_vertex_tex_coords, sizeof( float ) * 2 * num_tc_sets, num_vertices, p_file );
		}
	}
	
	// Grab a buffer for the raw vertex colors stream (if present), and read it.
	DWORD* p_vertex_colors = ( m_flags & 0x02 ) ? new DWORD[num_vertices] : NULL;
	if( p_vertex_colors )
	{
		File::Read( p_vertex_colors, sizeof( DWORD ), num_vertices, p_file );
	}

	// Grab a buffer for the vertex color wibble stream (if present), and read it.
	char* p_vc_wibble_indices = ( m_flags & 0x800 ) ? new char[num_vertices] : NULL;
	if( p_vc_wibble_indices )
	{
		File::Read( p_vc_wibble_indices, sizeof( char ), num_vertices, p_file );
	}
	
	// Remove TopDownHeap context.
	Mem::Manager::sHandle().PopContext();

	for( uint m = 0; m < p_geom->m_num_mesh; ++m )
	{
		unsigned long	material_checksum;
		unsigned int	flags, num_lod_index_levels;

		NxXbox::sMesh*	p_mesh = new NxXbox::sMesh;

		// Read bounding sphere and box data.
		float		rad;
		Mth::Vector cen;
		File::Read( &cen[X], sizeof( float ), 3, p_file );
		File::Read( &rad, sizeof( float ), 1, p_file );
		File::Read( &inf[X], sizeof( float ), 3, p_file );
		File::Read( &sup[X], sizeof( float ), 3, p_file );

		// Read and deal with flags, including skater shadow flag.
		File::Read( &flags, sizeof( uint32 ), 1, p_file );
		if( flags & 0x400 )
		{
			p_mesh->m_flags |= NxXbox::sMesh::MESH_FLAG_NO_SKATER_SHADOW;
		}
		if( flags & NxXbox::sMesh::MESH_FLAG_UNLIT )
		{
			p_mesh->m_flags |= NxXbox::sMesh::MESH_FLAG_UNLIT;
		}

		// The material checksum for this mesh.
		File::Read( &material_checksum,	sizeof( unsigned long ), 1, p_file );

		// How many levels of LOD indices? Should be at least 1!
		File::Read( &num_lod_index_levels, sizeof( unsigned int ), 1, p_file );

		// Can have up to 8 levels of LOD indices.
		uint16*	p_indices[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
		int		num_indices[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

		for( unsigned int lod_level = 0; lod_level < num_lod_index_levels; ++lod_level )
		{
			File::Read( &num_indices[lod_level], sizeof( int ), 1, p_file );
		
			// Again, we want all the temporary buffer allocations to come off of the top down heap.
			Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().TopDownHeap());
			p_indices[lod_level] = new uint16[num_indices[lod_level]];
			Mem::Manager::sHandle().PopContext();

			File::Read( p_indices[lod_level], sizeof( uint16 ), num_indices[lod_level], p_file );
		}

		// Set the load order of this mesh.
		p_mesh->m_load_order = m;

		// Set up the mesh.
		p_mesh->Initialize(	num_vertices,
							p_vertex_positions,
							( m_flags & 0x00800000UL ) ? NULL : p_vertex_normals,		// No normals allowed for billboards.
							p_vertex_tex_coords,
							num_tc_sets,
							p_vertex_colors,
							num_lod_index_levels,
							num_indices,
							p_indices,
							material_checksum,
							p_geom->mp_scene->GetEngineScene(),
							p_vertex_bone_indices,
							p_vertex_weights,
							( flags & 0x800 ) ? p_vc_wibble_indices : NULL );
		
		// Set the bounding data (sphere and box) for the mesh.
		p_mesh->SetBoundingData( cen, rad, inf, sup );

		// Add the bounding data to the scene.
		p_geom->mp_scene->GetEngineScene()->m_bbox.AddPoint( inf );
		p_geom->mp_scene->GetEngineScene()->m_bbox.AddPoint( sup );

		// Set up as a billboard if required.
		if( m_flags & 0x00800000UL )
		{
			p_mesh->SetBillboardData( billboard_type, billboard_pivot_pos, billboard_pivot_axis );
			NxXbox::BillboardManager.AddEntry( p_mesh );
		}

		// Flag the mesh as being a shadow volume if applicable.
		if( m_flags & 0x200000 )
		{
			p_mesh->m_flags |= NxXbox::sMesh::MESH_FLAG_SHADOW_VOLUME;
		}

		// Add the mesh to the attached CXboxGeom.
		p_geom->AddMesh( p_mesh );

		// Set the mesh bone index (mostly not applicable).
		p_mesh->SetBoneIndex( bone_idx );
	
		// Test code - if the mesh is mapped with a grass texture, add grass meshes.
		AddGrass( p_geom, p_mesh );
		
		// Done with the raw index data.
		for( int lod_level = 0; lod_level < 8; ++lod_level )
		{
			delete[] p_indices[lod_level];
		}
	}

	// Recount the number of meshes in case any have been added.
	p_geom->m_num_mesh = p_geom->mp_init_mesh_list->CountItems();
	

	// Test code for creating imposters.
#	if 0
	if( p_vertex_weights == NULL )
	{
		char *p_ok_sectors[] = {	"NJ_Houses_North_09",
									"NJ_Houses_North_10" };
/*
char *p_ok_sectors[] = { "CP_telephonepole43",
								"CP_telephonepole51",
								"CP_telephonepole52",
								"NJ_TelePole_Node03",
								"NJ_TelePole_Node02",
								"NJ_TelePole_Node04",
								"NJ_Telepole_00",
								"NJ_Telepole_01",
								"NJ_Telepole_02",
								"NJ_Telepole_03",
								"NJ_Telepole_04",
								"NJ_Telepole_05",
								"NJ_Telepole_06",
								"NJ_Telepole_07",
								"NJ_Telepole_08",
								"NJ_Telepole_09",
								"NJ_Telepole_12",
								"NJ_Telepole_13",
								"NJ_Telepole_14",
								"NJ_Telepole_15",
								"NJ_Telepole_16",
								"NJ_Telepole_17",
								"NJ_Telepole_18",
								"NJ_Telepole_19",
								"NJ_Telepole_20",
								"NJ_Telepole_21",
								"NJ_Telepole_22",
								"NJ_Telepole_23",
								"NJ_Telepole_24",
								"NJ_Telepole_25",
								"NJ_Telepole_26",
								"CP_telephonepole11",
								"NJ_Telepole_27",
								"NJ_Telepole_28",
								"NJ_Telepole_29",
								"NJ_Telepole_30",
								"NJ_Telepole_31",
								"NJ_Telepole_32" };
*/

		for( int s = 0; s < ( sizeof( p_ok_sectors ) / sizeof( char* )); ++s )
		{
			uint32 checksum = Crc::GenerateCRCFromString( p_ok_sectors[s] );
			if( checksum == m_checksum )
			{
//				Nx::CEngine::sGetImposterManager()->AddGeomToImposter( checksum, p_geom );
				Nx::CEngine::sGetImposterManager()->AddGeomToImposter( 0xb88905d6UL, p_geom );
			}
		}
	}
#	endif

	// Done with the raw vertex data.
	delete[] p_vc_wibble_indices;
	delete[] p_vertex_colors;
	delete[] p_vertex_tex_coords;
	delete[] p_vertex_bone_indices;
	delete[] p_vertex_weights;
	delete[] p_vertex_normals;
	delete[] p_vertex_positions;
	
	return true;
}



/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Here's a machine specific implementation of the CSector
// and we will also have a CXboxSector, CNgcSector, even a CPcSector
// maybe in the future we will have a CPS3Sector?




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxSector::plat_set_visibility(uint32 mask)
{
/*
	// Set values
	m_visible = mask;

	if( m_mesh_array )
	{
		for( uint m = 0; m < m_num_mesh; ++m )
		{
			NxXbox::sMesh *p_mesh = m_mesh_array[m];
			p_mesh->SetVisibility( mask );
		}
	}
*/
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxSector::plat_set_active( bool on )
{
/*
	// Set values
	m_active = on;

	if( m_mesh_array )
	{
		for( uint m = 0; m < m_num_mesh; ++m )
		{
			NxXbox::sMesh *p_mesh = m_mesh_array[m];
			p_mesh->SetActive( on );
		}
	}
*/
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxSector::plat_set_color( Image::RGBA rgba )
{
/*
	if( m_mesh_array == NULL )
	{
		return;
	}

	for( uint32 i = 0; i < m_num_mesh; ++i )
	{
		NxXbox::sMesh *p_mesh = m_mesh_array[i];
		p_mesh->m_flags |= NxXbox::sMesh::MESH_FLAG_MATERIAL_COLOR_OVERRIDE;
		p_mesh->m_material_color_override[0] = (float)rgba.r / 255.0f;
		p_mesh->m_material_color_override[1] = (float)rgba.g / 255.0f;
		p_mesh->m_material_color_override[2] = (float)rgba.b / 255.0f;
	}
*/
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxSector::plat_clear_color( void )
{
/*
	if( m_mesh_array == NULL )
	{
		return;
	}

	for( uint32 i = 0; i < m_num_mesh; ++i )
	{
		NxXbox::sMesh *p_mesh = m_mesh_array[i];
		p_mesh->m_flags &= ~NxXbox::sMesh::MESH_FLAG_MATERIAL_COLOR_OVERRIDE;
	}
*/
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxSector::plat_set_world_position( const Mth::Vector& pos )
{
/*
	Mth::Vector new_offset = pos - m_pos_offset;

	// Go through and adjust the individual meshes.
	for( uint32 i = 0; i < m_num_mesh; ++i )
	{
		NxXbox::sMesh *p_mesh = m_mesh_array[i];
		p_mesh->Move( new_offset );
	}
*/
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const Mth::CBBox &CXboxSector::plat_get_bounding_box( void ) const
{
	static Mth::CBBox dummy;
	
	//	return m_bbox;
	return dummy;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const Mth::Vector &CXboxSector::plat_get_world_position( void ) const
{
	return m_pos_offset;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxSector::plat_set_shatter( bool on )
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
CSector *CXboxSector::plat_clone( bool instance, CScene *p_dest_scene )
{
	CXboxSector *p_xbox_sector = new CXboxSector();

	/*


	// Copies over much of the standard stuff, individual stuff will be overwritten later.
	CopyMemory( p_xbox_sector, this, sizeof( CXboxSector ));

	if( instance )
	{
		Dbg_Assert( 0 );
	}
	else
	{
		// Need to create a new set of meshes. First create the mesh pointer array...
		p_xbox_sector->m_mesh_array = new NxXbox::sMesh*[m_num_mesh];

		// ...then clone the meshes themselves.
		for( uint32 i = 0; i < m_num_mesh; ++i )
		{
			p_xbox_sector->m_mesh_array[i] = m_mesh_array[i]->Clone();
		}

		// Grab a temporary workspace buffer.
		NxXbox::sScene *p_scene								= ( static_cast<CXboxScene*>( p_dest_scene ))->GetEngineScene();
		NxXbox::sMesh **p_temp_opaque_mesh_buffer			= new NxXbox::sMesh*[p_scene->m_num_opaque_entries];
		NxXbox::sMesh **p_temp_semitransparent_mesh_buffer	= new NxXbox::sMesh*[p_scene->m_num_semitransparent_entries];

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
		p_scene->CountMeshes( p_xbox_sector->m_num_mesh, p_xbox_sector->m_mesh_array );

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
		p_scene->AddMeshes( p_xbox_sector->m_num_mesh, p_xbox_sector->m_mesh_array );

		// Sort the meshes.
		p_scene->SortMeshes();
	}
*/
	return p_xbox_sector;

}

} // Namespace Nx  			
				
				
