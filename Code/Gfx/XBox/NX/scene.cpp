#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file/filesys.h>
#include <sys/timer.h>
#include "texture.h"
#include "mesh.h"
#include "scene.h"

namespace NxXbox
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int sort_by_material_draw_order( const void *p1, const void *p2 )
{
	sMesh		*p_mesh1		= *((sMesh**)p1 );
	sMesh		*p_mesh2		= *((sMesh**)p2 );
	sMaterial	*p_material1	= p_mesh1->mp_material;
	sMaterial	*p_material2	= p_mesh2->mp_material;
	
	Dbg_Assert( p_material1 != NULL );
	Dbg_Assert( p_material2 != NULL );

	if( p_material1->m_draw_order == p_material2->m_draw_order )
	{
		// Have to do some special case processing for the situation where two or more meshes have the same draw order, but only some are
		// marked as being dynamically sorted. In such a situation the non dynamically sorted ones should come first.
		if( p_material1->m_sorted == p_material2->m_sorted )
		{
			if( p_material1 == p_material2 )
			{
				// Same material, no further sorting required.
				return 0;
			}
			else
			{
				// If the pixel shaders are the same, sort by material address, otherwise sort by pixel shader value.
				if( p_mesh1->m_pixel_shader == p_mesh2->m_pixel_shader )
				{
					return ((uint32)p_material1 > (uint32)p_material2 ) ? 1 : -1;
				}
				else
				{
					return ((uint32)p_mesh1->m_pixel_shader > (uint32)p_mesh2->m_pixel_shader ) ? 1 : -1;
				}
			}
		}
		else if( p_material1->m_sorted )
		{
			return 1;
		}
		return -1;
	}
	return ( p_material1->m_draw_order > p_material2->m_draw_order ) ? 1 : -1;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sScene::sScene( void )
{
	m_flags								= 0;

	// No meshes as yet.
	m_num_mesh_entries					= 0;
	m_num_semitransparent_mesh_entries	= 0;
	m_num_filled_mesh_entries			= 0;
	m_first_dynamic_sort_entry			= 0xFFFFFFFFUL;
	m_first_semitransparent_entry		= 0xFFFFFFFFUL;
	m_num_dynamic_sort_entries			= 0;
	m_is_dictionary						= false;

	m_meshes							= NULL;
	pMaterialTable						= NULL;

	mp_hierarchyObjects					= NULL;
	m_numHierarchyObjects				= 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sScene::~sScene( void )
{
	// Remove the material table.
	if( pMaterialTable )
	{
		delete pMaterialTable;
	}

	if( m_meshes != NULL )
	{
		delete [] m_meshes;
	}

	if( mp_hierarchyObjects )
	{
		delete [] mp_hierarchyObjects;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sScene::FigureBoundingVolumes( void )
{
	// Figure bounding sphere assuming bounding box has already been set up (during individual mesh initialisation).
	Mth::Vector radial	= ( m_bbox.GetMax() - m_bbox.GetMin() ) * 0.5f;
	Mth::Vector center	= m_bbox.GetMin() + radial;
	m_sphere_center		= D3DXVECTOR3( center[X], center[Y], center[Z] );
	m_sphere_radius		= sqrtf(( radial[X] * radial[X] ) +	( radial[Y] * radial[Y] ) + ( radial[Z] * radial[Z] ));
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sScene::CountMeshes( int num_meshes, sMesh **pp_meshes )
{
	// Count each mesh.
	for( int m = 0; m < num_meshes; ++m )
	{
		++m_num_mesh_entries;
		bool transparent = (( pp_meshes[m]->mp_material ) && ( pp_meshes[m]->mp_material->m_flags[0] & 0x40 ));
		if( transparent )
		{
			++m_num_semitransparent_mesh_entries;
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sScene::CreateMeshArrays( void )
{
	if( m_num_mesh_entries > 0 )
	{
		m_meshes = new sMesh*[m_num_mesh_entries];
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sScene::AddMeshes( int num_meshes, sMesh **pp_meshes )
{
	// Add each mesh.
	for( int m = 0; m < num_meshes; ++m )
	{
		Dbg_Assert( m_num_filled_mesh_entries < m_num_mesh_entries );
		m_meshes[m_num_filled_mesh_entries] = pp_meshes[m];
		++m_num_filled_mesh_entries;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sScene::RemoveMeshes( int num_meshes, sMesh **pp_meshes )
{
	int num_mesh_entries_removed				= 0;
	
	for( int m = 0; m < num_meshes; ++m )
	{
		sMesh *p_mesh = pp_meshes[m];

		bool found = false;
		for( int i = 0; i < m_num_mesh_entries; ++i )
		{
			if( m_meshes[i] == p_mesh )
			{
				found = true;
				m_meshes[i] = NULL;
				++num_mesh_entries_removed;
				break;
			}
		}

		Dbg_Assert( found );	
	}
	
	// Now go through and compact the arrays.
	while( num_mesh_entries_removed > 0 )
	{
		for( int i = 0; i < m_num_mesh_entries; ++i )
		{
			if( m_meshes[i] == NULL )
			{
				// Only worth copying if there is anything beyond this mesh.
				if( i < ( m_num_mesh_entries - 1 ))
				{
					CopyMemory( &m_meshes[i], &m_meshes[i + 1], sizeof( sMesh* ) * ( m_num_mesh_entries - ( i + 1 )));
				}
				--num_mesh_entries_removed;
				--m_num_mesh_entries;
				--m_num_filled_mesh_entries;
				break;
			}
		}
	}
	
	// Finally, scan through and mark the start and end of the dynamically sorted set of meshes.
	m_num_semitransparent_mesh_entries	= 0;
	m_num_dynamic_sort_entries			= 0;
	m_first_semitransparent_entry		= m_num_mesh_entries;
	m_first_dynamic_sort_entry			= m_num_mesh_entries;
	
	for( int i = 0; i < m_num_mesh_entries; ++i )
	{
		bool transparent = (( m_meshes[i]->mp_material ) && ( m_meshes[i]->mp_material->m_flags[0] & 0x40 ));
		if( transparent )
		{
			++m_num_semitransparent_mesh_entries;

			if( i < m_first_semitransparent_entry )
			{
				m_first_semitransparent_entry = i;
			}

			if( m_meshes[i]->mp_material->m_sorted )
			{
				++m_num_dynamic_sort_entries;
				if( i < m_first_dynamic_sort_entry )
				{
					m_first_dynamic_sort_entry = i;
				}
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sScene::SortMeshes( void )
{
	// Sort the list of meshes.
	qsort( m_meshes, m_num_mesh_entries, sizeof( sMesh* ), sort_by_material_draw_order );

	// Now scan through and mark the start and end of the dynamically sorted set of meshes.
	m_num_semitransparent_mesh_entries	= 0;
	m_first_semitransparent_entry		= m_num_mesh_entries;
	m_first_dynamic_sort_entry			= m_num_mesh_entries;
	m_num_dynamic_sort_entries			= 0;
	for( int i = 0; i < m_num_mesh_entries; ++i )
	{
		bool transparent = (( m_meshes[i]->mp_material ) && ( m_meshes[i]->mp_material->m_flags[0] & 0x40 ));
		if( transparent )
		{
			++m_num_semitransparent_mesh_entries;

			if( i < m_first_semitransparent_entry )
			{
				m_first_semitransparent_entry = i;
			}

			if( m_meshes[i]->mp_material->m_sorted )
			{
				++m_num_dynamic_sort_entries;
				if( i < m_first_dynamic_sort_entry )
				{
					m_first_dynamic_sort_entry = i;
				}
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sMesh *sScene::GetMeshByLoadOrder( int load_order )
{
	for( int i = 0; i < m_num_mesh_entries; ++i )
	{
		if( m_meshes[i]->m_load_order == load_order )
		{
			return m_meshes[i];
		}
	}
	return NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sMaterial *sScene::GetMaterial( uint32 checksum )
{
	if( pMaterialTable )
	{
		pMaterialTable->IterateStart();
		sMaterial *p_mat = pMaterialTable->IterateNext();
		while( p_mat )
		{
			if( p_mat->m_checksum == checksum )
			{
				return p_mat;
			}
			p_mat = pMaterialTable->IterateNext();
		}
	}
	return NULL;
}



const uint32	INDEX_WORKBUFFER_SIZE	= 3072;
static uint16	index_workbuffer[INDEX_WORKBUFFER_SIZE];

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sScene::HidePolys( uint32 mask, sCASData *p_cas_data, uint32 num_entries )
{
	if(( num_entries == 0 ) || ( mask == 0 ))
	{
		return;
	}

//	Tmr::Time start = Tmr::GetTimeInUSeconds();
	
	int indices_added[64];
	ZeroMemory( indices_added, sizeof( int ) * 64 );
	
	// Check against every CAS entry.
	sCASData *p_cas_entry = p_cas_data;
	for( uint32 entry = 0; entry < num_entries; ++entry, ++p_cas_entry )
	{
		// Check this CAS entry has the correct mask...
		if( p_cas_entry->mask & mask )
		{
			// Get the mesh this entry references.
			uint32	load_order	= p_cas_entry->data0 >> 16;
			sMesh	*p_mesh		= GetMeshByLoadOrder( load_order );

			Dbg_Assert( load_order < 32 );
			
			if( p_mesh == NULL )
			{
				continue;
			}
			
			// Get the indices of the poly referenced.
//			uint32 i0 = ( p_cas_entry->data0 & 0xFFFF ) - p_mesh->m_index_base;
			uint32 i0 = p_cas_entry->data0 & 0xFFFF;
//			uint32 i1 = ( p_cas_entry->data1 >> 16 ) - p_mesh->m_index_base;
			uint32 i1 = p_cas_entry->data1 >> 16;
//			uint32 i2 = ( p_cas_entry->data1 & 0xFFFF ) - p_mesh->m_index_base;
			uint32 i2 = p_cas_entry->data1 & 0xFFFF;

			// For every vertex index in this mesh...
			uint16 *p_indices	= p_mesh->mp_index_buffer[0];
			uint32 index0		= p_indices[p_cas_entry->start_index] & 0x7FFF;
			uint32 index1		= p_indices[p_cas_entry->start_index + 1] & 0x7FFF;
			for( uint32 i = p_cas_entry->start_index + 2; i < p_mesh->m_num_indices[0]; ++i )
			{
				uint32 index2 = p_indices[i] & 0x7FFF;
				if(( index0 == i0 ) && ( index1 == i1 ) && ( index2 == i2 ))
				{
					// Indicate this mesh will need rebuilding.
					indices_added[load_order] = indices_added[load_order] + 1;

					// Flag the second index of the poly.
					p_indices[i - 1] |= 0x8000;
					break;
				}
				index0	= index1;
				index1	= index2;
			}
		}
	}

	// Now rebuild those meshes that need it.
	for( int m = 0; m < m_num_mesh_entries; ++m )
	{
		sMesh *p_mesh = m_meshes[m];
		if( indices_added[p_mesh->m_load_order] > 0 )
		{
			uint16 *p_indices						= p_mesh->mp_index_buffer[0];
			uint32 new_indices_index				= 0;
			for( uint32 i = 0; i < p_mesh->m_num_indices[0]; ++i )
			{
				uint16 index = p_indices[i];
				
				Dbg_Assert( new_indices_index < INDEX_WORKBUFFER_SIZE );
				index_workbuffer[new_indices_index++]	= index & 0x7FFF;

				if( index & 0x8000 )
				{
					Dbg_Assert( new_indices_index < INDEX_WORKBUFFER_SIZE );
					index_workbuffer[new_indices_index++]	= index & 0x7FFF;
					Dbg_Assert( new_indices_index < INDEX_WORKBUFFER_SIZE );
					index_workbuffer[new_indices_index++]	= index & 0x7FFF;
				}
			}

			// Create new index buffer, on the same heap as the existing index buffer.
			Mem::Allocator::BlockHeader*	p_bheader	= Mem::Allocator::BlockHeader::sRead( p_mesh->mp_index_buffer[0] );
			Mem::Allocator*					p_allocater	= p_bheader->mpAlloc;
			Mem::Manager::sHandle().PushContext( p_allocater );

			delete [] p_mesh->mp_index_buffer[0];
			p_mesh->m_num_indices[0]	= (uint16)new_indices_index;
			p_mesh->mp_index_buffer[0]	= new uint16[p_mesh->m_num_indices[0]];

			Mem::Manager::sHandle().PopContext();

			// And copy in the new indices from the workbuffer.
			CopyMemory( p_mesh->mp_index_buffer[0], index_workbuffer, sizeof( uint16 ) * p_mesh->m_num_indices[0] );
		}
	}

//	Tmr::Time end = Tmr::GetTimeInUSeconds() - start;
//	printf( "HidePolys() took %ld u seconds\n", end );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sScene *LoadScene( const char *Filename, sScene *pScene )
{
	return NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void DeleteScene( sScene *pScene )
{
	// Iterate through the table of materials, deleting them.
	if( pScene->pMaterialTable )
	{
		pScene->pMaterialTable->IterateStart();
		sMaterial* p_mat = pScene->pMaterialTable->IterateNext();
		while( p_mat )
		{
			delete p_mat;
			p_mat = pScene->pMaterialTable->IterateNext();
		}
	}
	
	// Delete the scene itself.
	delete pScene;
}



sScene *sScene::pHead;


} // namespace NxXbox

