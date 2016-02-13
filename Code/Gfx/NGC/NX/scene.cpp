#include <sys/file/filesys.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "texture.h"
#include "mesh.h"
#include "scene.h"
#include "render.h"
#include <sys/ngc/p_gx.h>
#include "nx_init.h"
#include <sys/timer.h>
#include <dolphin\gd.h>
#include <gel/music/Ngc/p_music.h>

#define ENV_MAP_SCROLL_SCALE ( 1.0f / 32768.0f )

int g_dl = 1;

bool gOverDraw = false;

namespace NxNgc
{

static s32	last_audio_update = 0;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static void do_audio_update( void )
{
	s32 t = OSGetTick();
	if(( t < last_audio_update ) || ( OSDiffTick( t, last_audio_update ) > (s32)( OS_TIMER_CLOCK / 60 )))
	{
		last_audio_update = t;
		Pcm::PCMAudio_Update();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static int sort_by_material_draw_order( const void *p1, const void *p2 )
{
	sMesh			*p_mesh1		= *((sMesh**)p1 );
	sMesh			*p_mesh2		= *((sMesh**)p2 );
	sMaterialHeader	*p_material1	= p_mesh1->mp_dl->m_material.p_header;
	sMaterialHeader	*p_material2	= p_mesh2->mp_dl->m_material.p_header;
	
	Dbg_Assert( p_material1 != NULL );
	Dbg_Assert( p_material2 != NULL );

	if( p_material1->m_draw_order == p_material2->m_draw_order )
	{
		// Have to do some special case processing for the situation where two or more meshes have the same draw order, but only some are
		// marked as being dynamically sorted. In such a situation the non dynamically sorted ones should come first.
		if( ( p_material1->m_flags & (1<<0) ) == ( p_material2->m_flags & (1<<0) ) )
		{
			if( p_material1 == p_material2 )
			{
				// Same material, no further sorting required.
				return 0;
			}
			else
			{
				// If the blend mode is the same, sort by material address.
				if( p_material1->m_base_blend == p_material2->m_base_blend )
				{
					// If the pixel shaders are the same, sort by material address, otherwise sort by pixel shader value.
					if( p_material1->m_texture_dl_id == p_material2->m_texture_dl_id )
					{
						return ((uint32)p_material1 > (uint32)p_material2 ) ? 1 : -1;
					}
					else
					{
						return ((uint32)p_material1->m_texture_dl_id < (uint32)p_material2->m_texture_dl_id ) ? 1 : -1;
					}
				}
				else
				{
					return ((uint32)p_material1->m_base_blend > (uint32)p_material2->m_base_blend ) ? 1 : -1;
				}
			}
		}
		else if( p_material1->m_flags & (1<<0) )
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

#define INDEX_WORKBUFFER_SIZE	1700
#define MAX_STRIPS				1024
#define MAX_INDEX_WIDTH			7

// Maximum index width:
// 1 position
// 1 normal
// 2 colors
// 4 tex coord
	
static void hide_mesh( uint32 mask, sCASData *p_cas_data, uint32 num_entries, sMesh * p_mesh )
{
	uint16	index_workbuffer[INDEX_WORKBUFFER_SIZE][MAX_INDEX_WIDTH];
	uint32	new_indices_index = 0;

	int		index_strip_len[MAX_STRIPS];
	int		num_strips = 0;

	// Clear strips.
	for ( int lp = 0; lp < MAX_STRIPS; lp++ )
	{
		index_strip_len[lp] = 0;
	}

	unsigned char * p_start = (unsigned char *)&p_mesh->mp_dl[1];
	unsigned char * p_end = &p_start[p_mesh->mp_dl->m_size];
	p_start = &p_start[p_mesh->mp_dl->m_index_offset];		// Skip to actual 1st GDBegin.
	unsigned char * p8 = p_start;

	uint8 begin_token = p8[0];		// Save token for use when rebuilding mesh.

	int stride = p_mesh->mp_dl->m_index_stride;

	int w;

	while ( p8 < p_end )
	{
		if ( ( p8[0] & 0xf8 ) == GX_TRIANGLESTRIP )
		{
			// Found a triangle strip - parse it.
			int num_verts = ( p8[1] << 8 ) | p8[2];
			p8 += 3;		// Skip GDBegin

			uint16 idx0[MAX_INDEX_WIDTH];
			uint16 idx1[MAX_INDEX_WIDTH];
			uint16 idx2[MAX_INDEX_WIDTH];

			for ( w = 0; w < stride; w++ ) { idx1[w] = ( p8[0] << 8 ) | p8[1]; p8 += 2; }
			for ( w = 0; w < stride; w++ ) { idx2[w] = ( p8[0] << 8 ) | p8[1]; p8 += 2; }

			for ( int v = 2; v < num_verts; v++ )
			{
				// Read next index to form triangle.
				for ( w = 0; w < stride; w++ ) idx0[w] = idx1[w];
				for ( w = 0; w < stride; w++ ) idx1[w] = idx2[w];
				for ( w = 0; w < stride; w++ ) { idx2[w] = ( p8[0] << 8 ) | p8[1]; p8 += 2; } 

				// There shuold be no degenerate triangles.
				// ...check against every CAS entry.
				bool keep = true;
				for( uint32 entry = 0; entry < num_entries; ++entry )
				{
					// Check this CAS entry has the correct mask...
					if( p_cas_data[entry].mask & mask )
					{
						// ...and the right mesh...
						uint32 mesh = p_cas_data[entry].data0 >> 16;
	
						// The CAS data is ordered first in increasing mesh size - so we can early out here if applicable.
						if ( mesh > p_mesh->mp_dl->m_mesh_index )
						{
							break;
						}
	
						if ( mesh == p_mesh->mp_dl->m_mesh_index )
						{
							// ...and the correct index.
							uint32 i0 = ( p_cas_data[entry].data0 & 0xFFFF );
	
							// The CAS data is next ordered in increasing size of i0 - so we can early out here if applicable.
							if ( i0 > idx0[0] )
							{
								break;
							}
	
							uint32 i1 = ( p_cas_data[entry].data1 >> 16 );
							uint32 i2 = ( p_cas_data[entry].data1 & 0xFFFF );
							if ( ( i0 == idx0[0] ) && ( i1 == idx1[0] ) && ( i2 == idx2[0] ) )
							{
								keep = false;

								break;
							}
						}
					}
				}

				// Copy index.
				if ( keep )
				{
					if ( index_strip_len[num_strips] )
					{
						// Already put 1st triangle in, just add index.
						for ( w = 0; w < stride; w++ ) index_workbuffer[new_indices_index][w] = idx2[w];
						new_indices_index++;
						index_strip_len[num_strips]++;
					}
					else
					{
						// 1st triangle, so add all 3 verts.
						for ( w = 0; w < stride; w++ ) index_workbuffer[new_indices_index][w] = idx0[w];
						new_indices_index ++;
						for ( w = 0; w < stride; w++ ) index_workbuffer[new_indices_index][w] = idx1[w];
						new_indices_index ++;
						for ( w = 0; w < stride; w++ ) index_workbuffer[new_indices_index][w] = idx2[w];
						new_indices_index ++;
						index_strip_len[num_strips] += 3;
					}
				}
				else
				{
					// If we don't keep this tri & we already started a strip, we need
					// to start a new strip.
					if ( index_strip_len[num_strips] )
					{
						// Need to start a new strip.
						num_strips++;
					}
				}
			}
			// Start a new strip if we put data in this one.
			if ( index_strip_len[num_strips] )
			{
				// Need to start a new strip.
				num_strips++;
			}
		}
		else
		{
			break;
		}
	}
	do_audio_update();

	Dbg_MsgAssert( new_indices_index <= INDEX_WORKBUFFER_SIZE, ( "Too many indices in new mesh: %d\n", new_indices_index ) );

	// See if we'll fit in the existing buffer.
	uint32 new_size = ( 3 * num_strips ) + ( sizeof( uint16 ) * stride * new_indices_index ) + p_mesh->mp_dl->m_index_offset;

//	OSReport( "New: %d Old: %d Diff: %d\n", new_size, p_mesh->mp_dl->m_size, p_mesh->mp_dl->m_size - new_size );

//	if ( new_size > p_mesh->mp_dl->m_original_dl_size )
//	{
//		// Need to allocate a new buffer for this.
//	}

	if ( new_size <= p_mesh->mp_dl->m_original_dl_size )
	{
		p8 = p_start;
		int buffer_index = 0;

		for ( int strip = 0; strip < num_strips; strip++ )
		{
			*p8++ = begin_token;
			*p8++ = (uint8)(index_strip_len[strip] >> 8 );
			*p8++ = (uint8)(index_strip_len[strip] & 0xff );
			for ( int index = 0; index < index_strip_len[strip]; index++ )
			{
				for ( w = 0; w < stride; w++ )
				{
					*p8++ = (uint8)(index_workbuffer[buffer_index][w] >> 8 );
					*p8++ = (uint8)(index_workbuffer[buffer_index][w] & 0xff );
				}
				buffer_index++;
			}
		}
		// Set new size of display list.
		int new_rounded_size = ( new_size + 31 ) & ~31;
		p_mesh->mp_dl->m_size = new_rounded_size;

		// Pad the DL with 0s to avoid spurious crap at the end & flush cache.
		int pad_size = new_rounded_size - new_size;
		for ( int pad = 0; pad < pad_size; pad++ ) *p8++ = 0;

		DCFlushRange( &p_mesh->mp_dl[1], new_rounded_size );
	}
	else
	{
#ifdef __NOPT_FINAL__
		OSReport( "Warning: DL too big after hide_mesh - New: %d Old: %d Diff: %d\n", new_size, p_mesh->mp_dl->m_size, p_mesh->mp_dl->m_size - new_size );
#else
		Dbg_MsgAssert( false, ( "Error: DL too big after hide_mesh - New: %d Old: %d Diff: %d\n", new_size, p_mesh->mp_dl->m_size, p_mesh->mp_dl->m_size - new_size ) );
#endif		// __NOPT_FINAL__
	}
	do_audio_update();
}

sScene::sScene( void )
{
	m_flags					= 0;

	m_num_meshes			= 0;		// No meshes as yet.
	m_num_filled_meshes		= 0;
	mpp_mesh_list			= NULL;

	mp_hierarchyObjects		= NULL;
	m_numHierarchyObjects	= 0;

	mp_scene_data			= NULL;
	mp_dl					= NULL;

	mp_blend_dl				= NULL;
	mp_material_header		= NULL;

	mp_hierarchyObjects		= NULL;
	m_numHierarchyObjects	= 0;

	m_is_dictionary			= false;
}

sScene::~sScene( void )
{
	// Remove the material table.
//	if( mp_material_array )
//	{
//		delete mp_material_array;
//	}
//
//	if( m_opaque_meshes != NULL )
//	{
//		delete [] m_opaque_meshes;
//	}
//	if( m_semitransparent_meshes != NULL )
//	{
//		delete [] m_semitransparent_meshes;
//	}
//
//	if ( mp_hierarchyObjects )
//	{
//		delete [] mp_hierarchyObjects;
//	}
//

//	// Go through, and see if any DLs got allocated.
//	int dl = 0;
//	for( uint s = 0; s < mp_scene_data->m_num_objects; ++s )
//	{
//		int num_mesh = mp_dl[dl].mp_object_header->m_num_meshes;
//		for ( int m = 0; m < num_mesh; m++ )
//		{
//			if ( mp_dl[dl].mp_pos_pool )
//			{
//			}
//		}
//	}


	if ( mp_scene_data && !( m_flags & SCENE_FLAG_CLONED_GEOM ) )
	{
		delete [] mp_scene_data;
	}

	if ( mpp_mesh_list )
	{
		delete [] mpp_mesh_list;
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
		if ( m_num_filled_meshes < m_num_meshes )
		{
			mpp_mesh_list[m_num_filled_meshes] = pp_meshes[m];
			++m_num_filled_meshes;
		}
		else
		{
			Dbg_MsgAssert( false, ( "Too many meshes being added." ) );
		}
	}

//	for( int m = 0; m < num_meshes; ++m )
//	{
//		sMaterialHeader *		p_material = pp_meshes[m]->mp_dl->m_material.p_header;
//		sMaterialPassHeader *	p_pass = &mp_material_pass[p_material->m_pass_item];
//
//		bool transparent = (( p_material ) && ( p_pass->m_flags & (1<<3) ));
//
//		if ( transparent )
//		{
//			if ( m_num_filled_meshes < m_num_meshes )
//			{
//				mpp_mesh_list[m_num_filled_meshes] = pp_meshes[m];
//				++m_num_filled_meshes;
//			}
//			else
//			{
//				Dbg_MsgAssert( false, ( "Too many meshes being added." ) );
//			}
//		}
//	}
//
//	for( int m = 0; m < num_meshes; ++m )
//	{
//		sMaterialHeader *		p_material = pp_meshes[m]->mp_dl->m_material.p_header;
//		sMaterialPassHeader *	p_pass = &mp_material_pass[p_material->m_pass_item];
//
//		bool transparent = (( p_material ) && ( p_pass->m_flags & (1<<3) ));
//
//		if ( !transparent )
//		{
//			if ( m_num_filled_meshes < m_num_meshes )
//			{
//				mpp_mesh_list[m_num_filled_meshes] = pp_meshes[m];
//				++m_num_filled_meshes;
//			}
//			else
//			{
//				Dbg_MsgAssert( false, ( "Too many meshes being added." ) );
//			}
//		}
//	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sScene::CountMeshes( int num_meshes, sMesh **pp_meshes )
{
	m_num_meshes += (uint16)num_meshes;


//	// Count each mesh.
//	for( int m = 0; m < num_meshes; ++m )
//	{
//		++m_num_meshes;
////		bool transparent = (( pp_meshes[m]->mp_material ) && ( pp_meshes[m]->mp_material->Flags[0] & 0x40 ));
////		if( transparent )
////		{
////			++m_num_semitransparent_entries;
////		}
////		else
////		{
////			++m_num_opaque_entries;
////		}
//	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sScene::CreateMeshArrays( void )
{
	if ( m_num_meshes > 0 )
	{
		mpp_mesh_list = new sMesh*[m_num_meshes];
	}

//	if( m_num_semitransparent_entries > 0 )
//	{
//		m_semitransparent_meshes = new sMesh*[m_num_semitransparent_entries];
//	}
//	
//	if( m_num_opaque_entries > 0 )
//	{
//		m_opaque_meshes = new sMesh*[m_num_opaque_entries];
//	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sScene::FigureBoundingVolumes( void )
{
//	// Figure bounding sphere assuming bounding box has already been set up (during individual mesh initialisation).
//	Mth::Vector radial	= ( m_bbox.GetMax() - m_bbox.GetMin() ) * 0.5f;
//	Mth::Vector center	= m_bbox.GetMin() + radial;
//	m_sphere_center.set( center[X], center[Y], center[Z] );
//	m_sphere_radius		= sqrtf(( radial[X] * radial[X] ) +	( radial[Y] * radial[Y] ) + ( radial[Z] * radial[Z] ));
}








void sScene::RemoveMeshes( int num_meshes, sMesh **pp_meshes )
{
	int num_opaque_entries_removed					= 0;
	int num_pre_semitransparent_entries_removed		= 0;
	int num_dynamic_semitransparent_entries_removed	= 0;
	int num_post_semitransparent_entries_removed	= 0;
	
	int opaque		= m_num_opaque_meshes;
	int presemi		= opaque + m_num_pre_semitrans_meshes;
	int dynsemi		= presemi + m_num_dynamic_semitrans_meshes;
	int postsemi	= dynsemi + m_num_post_semitrans_meshes;

	for( int m = 0; m < num_meshes; ++m )
	{
		sMesh *p_mesh = pp_meshes[m];

		bool found = false;
		// Search Opaque
		for ( int i = 0; i < opaque; i++ )
		{
			if( mpp_mesh_list[i] == p_mesh )
			{
				found = true;
				mpp_mesh_list[i] = NULL;
				++num_opaque_entries_removed;
				--m_num_opaque_meshes;
				break;
			}
		}
		if( found ) continue;

		// Search Pre Semitransparent
		for ( int i = opaque; i < presemi; i++ )
		{
			if( mpp_mesh_list[i] == p_mesh )
			{
				found = true;
				mpp_mesh_list[i] = NULL;
				++num_pre_semitransparent_entries_removed;
				--m_num_pre_semitrans_meshes;
				break;
			}
		}

		if( found ) continue;

		// Search Dynamic Semitransparent
		for ( int i = presemi; i < dynsemi; i++ )
		{
			if( mpp_mesh_list[i] == p_mesh )
			{
				found = true;
				mpp_mesh_list[i] = NULL;
				++num_dynamic_semitransparent_entries_removed;
				--m_num_dynamic_semitrans_meshes;
				break;
			}
		}

		if( found ) continue;

		// Search Post Semitransparent
		for ( int i = dynsemi; i < postsemi; i++ )
		{
			if( mpp_mesh_list[i] == p_mesh )
			{
				found = true;
				mpp_mesh_list[i] = NULL;
				++num_post_semitransparent_entries_removed;
				--m_num_post_semitrans_meshes;
				break;
			}
		}

		Dbg_Assert( found );	
	}
	
	// Now go through and compact the arrays.

//	int total = m_num_opaque_meshes + m_num_pre_semitrans_meshes + m_num_dynamic_semitrans_meshes + m_num_post_semitrans_meshes;
	int total_removed = num_opaque_entries_removed +
						num_pre_semitransparent_entries_removed +
						num_dynamic_semitransparent_entries_removed	+
						num_post_semitransparent_entries_removed;

	for ( int lp = 0; lp < total_removed; lp++ )
	{
		for ( int i = 0; i < m_num_filled_meshes; i++ )
		{
			if( !mpp_mesh_list[i] )
			{
//				m_num_filled_meshes--;
//				mpp_mesh_list[i] = mpp_mesh_list[m_num_filled_meshes];


				// Only worth copying if there is anything beyond this mesh.
				if( i < ( m_num_filled_meshes - 1 ))
				{
					memcpy( &mpp_mesh_list[i], &mpp_mesh_list[i + 1], sizeof( sMesh* ) * ( m_num_filled_meshes - ( i + 1 )));
				}
				m_num_filled_meshes--;
				break;
			}
		}
	}
//	SortMeshes();
//	m_num_filled_meshes -= total_removed;
}



void sScene::SortMeshes( void )
{
	// Sort the list of meshes.
	qsort( mpp_mesh_list, m_num_filled_meshes, sizeof( sMesh* ), sort_by_material_draw_order );

	if ( mp_scene_data )
	{
		m_num_opaque_meshes				= 0;
		m_num_pre_semitrans_meshes		= 0;
		m_num_dynamic_semitrans_meshes	= 0;
		m_num_post_semitrans_meshes		= 0;

		// Force opaque to be before semitrans.
		sMesh *p_mesh[8192];
		int mesh_entry = 0;
		Dbg_MsgAssert( m_num_filled_meshes < 8192, ( "Too many meshes for temporary sort buffer." ) );

		// Find 1st dynamic entry.
		int first_dynamic = -1;
		for( int i = 0; i < m_num_filled_meshes; ++i )
		{
			if ( mpp_mesh_list[i] )
			{
				bool sorted = mpp_mesh_list[i]->mp_dl->m_material.p_header->m_flags & (1<<0) ? true : false;

				if ( ( first_dynamic == -1 ) && sorted ) first_dynamic = i;
			}
		}


		for ( int pass = 0; pass < 4; pass++ )
		{
			for( int i = 0; i < m_num_filled_meshes; ++i )
			{
				if ( mpp_mesh_list[i] )
				{
					bool transparent = ( mp_material_pass[mpp_mesh_list[i]->mp_dl->m_material.p_header->m_pass_item].m_flags & (1<<3) );
//					bool transparent = ( mpp_mesh_list[i]->mp_dl->m_material.p_header->m_flags & (1<<3) ) ? true : false;

					bool sorted = mpp_mesh_list[i]->mp_dl->m_material.p_header->m_flags & (1<<0) ? true : false;

					if ( ( first_dynamic == -1 ) && sorted ) first_dynamic = i;

					switch ( pass )
					{
						case 0:		// Opaque
							if ( !transparent )
							{
								p_mesh[mesh_entry] = mpp_mesh_list[i];
								mpp_mesh_list[i] = NULL;
								mesh_entry++;
								m_num_opaque_meshes++;
							}
							break;
						case 1:		// Pre semi
							if ( transparent && !sorted && ( i < first_dynamic ) )
							{
//								if ( mpp_mesh_list[i]->mp_dl ) printf( "Pre Semi: %d, %8.3f\n", i, mpp_mesh_list[i]->mp_dl->m_material.p_header->m_draw_order );
								p_mesh[mesh_entry] = mpp_mesh_list[i];
								mpp_mesh_list[i] = NULL;
								mesh_entry++;
								m_num_pre_semitrans_meshes++;
							}
							break;
						case 2:		// Dynamic semi
							if ( transparent && sorted )
							{
//								if ( mpp_mesh_list[i]->mp_dl ) printf( "Dyn Semi: %d, %8.3f\n", i, mpp_mesh_list[i]->mp_dl->m_material.p_header->m_draw_order );
								p_mesh[mesh_entry] = mpp_mesh_list[i];
								mpp_mesh_list[i] = NULL;
								mesh_entry++;
								m_num_dynamic_semitrans_meshes++;
							}
							break;
						case 3:		// Post semi (everything else)...
//							if ( transparent && !sorted && ( ( i >= first_dynamic ) && ( first_dynamic != -1 ) ) )
							{
//								if ( mpp_mesh_list[i]->mp_dl ) printf( "Pst Semi: %d, %8.3f\n", i, mpp_mesh_list[i]->mp_dl->m_material.p_header->m_draw_order );
								p_mesh[mesh_entry] = mpp_mesh_list[i];
								mpp_mesh_list[i] = NULL;
								mesh_entry++;
								m_num_post_semitrans_meshes++;
							}
							break;
						default:
							Dbg_MsgAssert( false, ( "This should never happen." ) );
							break;
					}
				}
			}
		}
		Dbg_MsgAssert( m_num_filled_meshes == mesh_entry, ( "Sorted meshes differs from actual meshes." ) );

//		float order = p_mesh[0]->mp_dl->m_material.p_header->m_draw_order;
		for( int i = 0; i < m_num_filled_meshes; ++i )
		{
//			Dbg_MsgAssert( p_mesh[i]->mp_dl->m_material.p_header->m_draw_order >= order, ( "Out of order on entry %d: last=%f, current = %f", i, order, p_mesh[i]->mp_dl->m_material.p_header->m_draw_order ) );
//			order = p_mesh[i]->mp_dl->m_material.p_header->m_draw_order;
			mpp_mesh_list[i] = p_mesh[i];
		}
	}
	else
	{
		m_num_opaque_meshes				= m_num_filled_meshes;
		m_num_pre_semitrans_meshes		= 0;
		m_num_dynamic_semitrans_meshes	= 0;
		m_num_post_semitrans_meshes		= 0;
	}
}






sMaterial *sScene::GetMaterial( uint32 checksum )
{
//	for ( int lp = 0; lp < m_num_materials; lp++ )
//	{
//		if( mp_material_array[lp].Checksum == checksum )
//		{
//			return &mp_material_array[lp];
//		}
//	}
	return NULL;
}



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

	// For each mesh, need to find all cas data which references verts in that mesh.
	for( int m = 0; m < this->m_num_filled_meshes; ++m )
	{
		sMesh *p_mesh = mpp_mesh_list[m];

		hide_mesh( mask, p_cas_data, num_entries, p_mesh );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sScene *LoadScene( const char *Filename, sScene *pScene )
{
	return NULL;
}



void DeleteScene( sScene *pScene )
{
//	// Iterate through the table of materials, deleting them.
//	for ( int lp = 0; lp < pScene->m_num_materials; lp++ )
//	{
//		if( pScene->mp_material_array[lp].mp_wibble_vc_params	)
//		{
//			for( uint32 i = 0; i < pScene->mp_material_array[lp].m_num_wibble_vc_anims; ++i )
//			{
//				delete [] pScene->mp_material_array[lp].mp_wibble_vc_params[i].mp_keyframes;
//			}
//			delete [] pScene->mp_material_array[lp].mp_wibble_vc_params;
//		}
//		if( pScene->mp_material_array[lp].mp_wibble_vc_colors	)
//		{
//			delete [] pScene->mp_material_array[lp].mp_wibble_vc_colors;
//		}
//		if( pScene->mp_material_array[lp].m_pUVControl	)
//		{
//			delete [] pScene->mp_material_array[lp].m_pUVControl;
//		}
//	}
//	delete pScene->mp_material_array;
//	pScene->mp_material_array = NULL;
//
//	// Delete the scene itself.
	delete pScene;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void MaterialBuild(  sMesh * p_mesh, sScene * p_scene, bool bl, bool tx )
{
	if ( !p_mesh->mp_dl->m_material.p_header ) return;

	// Construct the DLs.
	uint32 size;

	Mem::Manager::sHandle().TopDownHeap()->PushAlign( 32 );
#define DL_BUILD_SIZE (8*1024)
	uint8 * p_build_dl = new (Mem::Manager::sHandle().TopDownHeap()) uint8[DL_BUILD_SIZE];

	// Build the texture upload DL.
	DCFlushRange ( p_build_dl, DL_BUILD_SIZE );

	GX::begin( p_build_dl, DL_BUILD_SIZE );

//	multi_mesh( p_mesh, p_scene );

	multi_mesh( p_mesh->mp_dl->m_material.p_header,
				&p_scene->mp_material_pass[p_mesh->mp_dl->m_material.p_header->m_pass_item],
				bl,
				tx );

	size = GX::end();

	DCFlushRange ( p_build_dl, DL_BUILD_SIZE );

	if ( size && ( size <= p_scene->mp_texture_dl[p_mesh->mp_dl->m_material.p_header->m_texture_dl_id].m_dl_size ) )
	{
//		p_mesh->mp_dl->mp_texture_dl = new uint8[size];

		memcpy ( p_scene->mp_texture_dl[p_mesh->mp_dl->m_material.p_header->m_texture_dl_id].mp_dl, p_build_dl, size );
		DCFlushRange ( p_scene->mp_texture_dl[p_mesh->mp_dl->m_material.p_header->m_texture_dl_id].mp_dl, size );

		p_scene->mp_texture_dl[p_mesh->mp_dl->m_material.p_header->m_texture_dl_id].m_dl_size = (uint16)size;
	}
	else
	{
//		p_mesh->mp_dl->mp_texture_dl = NULL;
//		p_mesh->mp_dl->m_texture_dl_size = 0;
	}

	Mem::Manager::sHandle().TopDownHeap()->PopAlign();

	// Done constructing DL.
	delete p_build_dl;
}

int g_material_id = -1;
GXBool g_comploc = (GXBool)2;

void ResetMaterialChange( void )
{
	g_material_id = -1; 
	g_comploc = (GXBool)2; 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

//static void MyGXProject ( 
//    f32  x,          // model coordinates
//    f32  y,
//    f32  z,
//    f32  mtx[3][4],  // model-view matrix
//    f32* pm,         // projection matrix, as returned by GXGetProjectionv
//    f32* vp,         // viewport, as returned by GXGetViewportv
//    f32* sx,         // screen coordinates
//    f32* sy,
//    f32* sz ) 
//{
//    Vec   peye;
//    f32   xc, yc, zc, wc;
//
//    ASSERTMSG(pm && vp && sx && sy && sz, GXERR_GET_NULL_PTR);
//
//    // transform to eye space
//    peye.x = mtx[0][0]*x + mtx[0][1]*y + mtx[0][2]*z + mtx[0][3];
//    peye.y = mtx[1][0]*x + mtx[1][1]*y + mtx[1][2]*z + mtx[1][3];
//    peye.z = mtx[2][0]*x + mtx[2][1]*y + mtx[2][2]*z + mtx[2][3];
//
//	// My addition: Just a frig to stop stuff messing up as it gets close to the near plane.
//	peye.z -= 512.0f;
//	if ( peye.z > -1.0f ) peye.z = -1.0f;
//
//    // transform to clip space
//    if (pm[0] == (f32)GX_PERSPECTIVE) { // perspective
//        xc = peye.x * pm[1] + peye.z * pm[2];
//        yc = peye.y * pm[3] + peye.z * pm[4];
//        zc = peye.z * pm[5] + pm[6];
//        wc = 1.0f / -peye.z;
//    } else { // ortho
//        xc = peye.x * pm[1] + pm[2];
//        yc = peye.y * pm[3] + pm[4];
//        zc = peye.z * pm[5] + pm[6];
//        wc = 1.0f;
//    }
//
//    // compute screen scale and offset
//    *sx = xc * vp[2]/2 * wc + vp[0] + vp[2]/2;
//    *sy = -yc * vp[3]/2 * wc + vp[1] + vp[3]/2;
//    *sz = zc * (vp[5] - vp[4]) * wc + vp[5];
//}
//

void MaterialSubmit( sMesh * p_mesh, sScene *pScene = NULL )
{
	if ( !pScene->mp_scene_data ) return;
	if ( !p_mesh->mp_dl->m_material.p_header ) return;
//	if ( !p_mesh->mp_dl->mp_texture_dl ) return;

	if ( gOverDraw )
	{
		GX::SetFog( GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, (GXColor){0,0,0,0} );
		GX::SetTevOrder( GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0 );

		GX::SetTevColorInOp( GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_KONST,
							 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV );
		
		GX::SetTevAlphaInOpSwap( GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST,
								 GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE, GX_TEVPREV,
								 GX_TEV_SWAP0, GX_TEV_SWAP0 );

		GX::SetBlendMode ( GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR, GX_TRUE, GX_FALSE, GX_TRUE );

		GX::SetTevKSel( GX_TEVSTAGE0, GX_TEV_KCSEL_K0, GX_TEV_KASEL_K0_A, GX_TEV_KCSEL_K0, GX_TEV_KASEL_K0_A );
		GX::SetTevKColor( GX_KCOLOR0, (GXColor){8,8,8,255} );
		GX::SetChanCtrl( GX_ALPHA0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE ); 
		GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255});
		GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){255,255,255,255});

		GX::SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		GX::SetTexChanTevIndCull( 0, 1, 1, 0, GX_CULL_NONE ); 
		return;
	}







//	if ( g_dl /*&& !( p_mesh->mp_dl->m_material.p_header->m_flags & (1<<3) )*/ )
//	if ( g_dl/* && !( p_mesh->mp_dl->m_material.p_header->m_flags & (1<<3) )*/ )
	if ( g_dl && !( p_mesh->mp_dl->m_material.p_header->m_flags & ((1<<3)|(1<<4)) ) )
	{
////		if ( p_mesh->mp_dl->m_material.p_header->m_material_dl_id != g_material_id )
//		{
//			g_material_id = p_mesh->mp_dl->m_material.p_header->m_material_dl_id;
//
//			if ( pScene->mp_blend_dl[g_material_id].mp_dl && pScene->mp_blend_dl[g_material_id].m_dl_size )
//			{
////				GX::CallDisplayList( pScene->mp_blend_dl[g_material_id].mp_dl, pScene->mp_blend_dl[g_material_id].m_dl_size );
//				multi_mesh( p_mesh->mp_dl->m_material.p_header,
//							&pScene->mp_material_pass[p_mesh->mp_dl->m_material.p_header->m_pass_item],
//							true,
//							false );
//			}
//		}

		int tex_id = (int)p_mesh->mp_dl->m_material.p_header->m_texture_dl_id;
		GX::CallDisplayList( pScene->mp_texture_dl[tex_id].mp_dl, pScene->mp_texture_dl[tex_id].m_dl_size );
	}
	else
	{
		multi_mesh( p_mesh->mp_dl->m_material.p_header,
					&pScene->mp_material_pass[p_mesh->mp_dl->m_material.p_header->m_pass_item],
					true,
					true,
					p_mesh->mp_dl->m_material.p_header->m_flags & (1<<6) ? true : false,
					p_mesh->mp_dl->mp_object_header->m_num_skin_verts ? false : true );
	}

	// See if we need to upload env mapping matrices.
	sMaterialHeader * p_mat = p_mesh->mp_dl->m_material.p_header;
	sMaterialPassHeader *p_pass = &pScene->mp_material_pass[p_mat->m_pass_item];

	// Set Comploc
	u8 alphacutoff = p_mesh->mp_dl->m_material.p_header->m_alpha_cutoff;
	GXBool comploc;
//	if ( p_pass->m_texture.p_data && ( p_pass->m_texture.p_data->flags & NxNgc::sTexture::TEXTURE_FLAG_HAS_HOLES ) )
	if ( alphacutoff > 0 )
	{
		comploc = GX_FALSE;
	}
	else
	{
		comploc = GX_TRUE;
	}
	comploc = GX_FALSE;

	if ( g_comploc != comploc )
	{
		g_comploc = comploc;
		GX::SetZCompLoc( comploc );
	}

//	GX::SetAlphaCompare(GX_GEQUAL, alphacutoff, GX_AOP_AND, GX_GEQUAL, alphacutoff );
//	GX::SetZCompLoc( GX_FALSE );
	GX::SetAlphaCompare(GX_GEQUAL, alphacutoff, GX_AOP_AND, GX_GEQUAL, alphacutoff );

	for ( int lp = 0; lp < p_mat->m_passes; lp++, p_pass++ )
	{
		if ( p_pass->m_flags & ( (1<<1) | (1<<2) ) )
		{
			// UV Wibbled or Environment mapped.
			GXTexMtx mtx;
			switch ( lp )
			{
				case 0:
					mtx = GX_TEXMTX0;
					break;
				case 1:
					mtx = GX_TEXMTX1;
					break;
				case 2:
					mtx = GX_TEXMTX2;
					break;
				case 3:
					mtx = GX_TEXMTX3;
					break;
				default:
					mtx = GX_IDENTITY;
					break;
			}

			if( p_pass->m_flags & (1<<1) )
			{
				// Env mapping.
				Mtx s, t, e, mv;

				MTXInvXpose((Mtx)&EngineGlobals.local_to_camera, mv );

				// Project bounding sphere into screen space to get a metric to scroll the environment map.
				f32				p[GX_PROJECTION_SZ];
				f32				vp[GX_VIEWPORT_SZ];
				float			rx, ry;	//, rz;

				GX::GetProjectionv( p );
				GX::GetViewportv( vp );

				float x = EngineGlobals.local_to_camera.getPosX();
				float y = EngineGlobals.local_to_camera.getPosY();
//				float z = EngineGlobals.local_to_camera.getPosZ();

				rx = -x;	//EngineGlobals.local_to_camera.getRightX()*x + EngineGlobals.local_to_camera.getRightY()*y + EngineGlobals.local_to_camera.getRightZ()*z;
				ry = -y;	//EngineGlobals.local_to_camera.getUpX()*x + EngineGlobals.local_to_camera.getUpY()*y + EngineGlobals.local_to_camera.getUpZ()*z;


//				MyGXProject( p_mesh->mp_dl->m_sphere[X],
//						   p_mesh->mp_dl->m_sphere[Y],
//						   p_mesh->mp_dl->m_sphere[Z],
//						   (Mtx)&EngineGlobals.local_to_camera,
//						   p, vp, &rx, &ry, &rz );

				float u;
				float v;

				float ut = (float)p_pass->m_u_tile;
				float vt = (float)p_pass->m_v_tile;
				ut = ( ut * (1.0f / (float)(1<<12)) );
				vt = ( vt * (1.0f / (float)(1<<12)) );

				u = ( rx * ( ENV_MAP_SCROLL_SCALE * ut ) );
				v = ( ry * ( ENV_MAP_SCROLL_SCALE * vt ) );

				u -= (float)(int)u;		// Keep within +/-1.
				v -= (float)(int)v;

				// Create the rotational component.
				MTXScale( s, 0.5f * ut, -0.5f * vt, 0.0f );
				MTXTrans( t, 0.5f, 0.5f, 1.0f );
				MTXConcat( t, s, e );

				MTXConcat(e, mv, e);

				e[0][3] += u;
				e[1][3] += v;

				GX::LoadTexMtxImm(e, mtx, GX_MTX2x4);
			}
			else
			{
				if ( p_pass->m_uv_enabled )
				{
					// Explicit matrix.
					Mtx real;
//					MTXCopy( p_pass->mp_explicit_wibble->m_matrix, real );

//					real[0][3] = ( p_pass->mp_explicit_wibble->m_matrix[0][3] * -real[0][0] ) + ( p_pass->mp_explicit_wibble->m_matrix[1][3] * -real[0][1] );
//					real[1][3] = ( p_pass->mp_explicit_wibble->m_matrix[0][3] * -real[1][0] ) + ( p_pass->mp_explicit_wibble->m_matrix[1][3] * -real[1][1] );

					real[0][0] = ( (float)p_pass->m_uv_mat[0] ) * ( 1.0f / ((float)(1<<12)) );
					real[0][1] = -( (float)p_pass->m_uv_mat[1] ) * ( 1.0f / ((float)(1<<12)) );
					real[0][2] = 1.0f;
					real[0][3] = ( (float)p_pass->m_uv_mat[2] ) * ( 1.0f / ((float)(1<<12)) ) - 1.0f;

					real[1][0] = ( (float)p_pass->m_uv_mat[1] ) * ( 1.0f / ((float)(1<<12)) );
					real[1][1] = ( (float)p_pass->m_uv_mat[0] ) * ( 1.0f / ((float)(1<<12)) );
					real[1][2] = 1.0f;
					real[1][3] = ( (float)p_pass->m_uv_mat[3] ) * ( 1.0f / ((float)(1<<12)) ) - 1.0f;
					GX::LoadTexMtxImm( real, mtx, GX_MTX2x4 );
				}
				else
				{
					// Wibbled.
					float uoff, voff, t;

					t = (float)Tmr::GetTime() * 0.001f;

					Mtx	m;

					sMaterialUVWibble * p_uv = &pScene->mp_uv_wibble[p_pass->m_uv_wibble_index];

					uoff	= ( t * p_uv->m_u_vel ) + ( p_uv->m_u_amp * sinf( p_uv->m_u_freq * t + p_uv->m_u_phase ));
					voff	= ( t * p_uv->m_v_vel ) + ( p_uv->m_v_amp * sinf( p_uv->m_v_freq * t + p_uv->m_v_phase ));

					// Reduce offset mod 16 and put it in the range -8 to +8.
					uoff	+= 8.0f;
					uoff	-= (float)(( (int)uoff >> 4 ) << 4 );
					voff	+= 8.0f;
					voff	-= (float)(( (int)voff >> 4 ) << 4 );

					uoff = ( uoff < 0.0f ) ? ( uoff + 8.0f ) : ( uoff - 8.0f ); 
					voff = ( voff < 0.0f ) ? ( voff + 8.0f ) : ( voff - 8.0f ); 

					MTXTrans( m, uoff, voff, 0.0f );

					GX::LoadTexMtxImm(m, mtx, GX_MTX2x4);
				}
			}
		}
	}
}

} // namespace NxNgc

