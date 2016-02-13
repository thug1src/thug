#include "billboard.h"
#include "nx_init.h"

namespace NxXbox
{

sBillboardManager BillboardManager;
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sBillboardManager::sBillboardManager( void )
{
	m_num_batches	= 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sBillboardManager::~sBillboardManager( void )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static int sort_batches_by_draw_order( const void *p1, const void *p2 )
{
	sBillboardMaterialBatch *p_batch0 = *((sBillboardMaterialBatch**)p1 );
	sBillboardMaterialBatch *p_batch1 = *((sBillboardMaterialBatch**)p2 );

	// Deal with NULL pointers first (caused by removing batches from the list).
	if( p_batch0 == NULL )
	{
		if( p_batch1 == NULL )
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}
	else if( p_batch1 == NULL )
	{
		return -1;
	}

	return p_batch0->GetDrawOrder() < p_batch1->GetDrawOrder() ? -1 : p_batch0->GetDrawOrder() > p_batch1->GetDrawOrder() ? 1 : 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sBillboardMaterialBatch *sBillboardManager::GetBatch( sMaterial *p_material )
{
	for( int b = 0; b < m_num_batches; ++b )
	{
		if( mp_batches[b]->GetMaterial() == p_material )
		{
			return mp_batches[b];
		}
	}
	return NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sBillboardManager::AddEntry( sMesh *p_mesh )
{
	if( p_mesh->mp_material )
	{
		// Make sure it is textured.
		if( p_mesh->mp_material->mp_tex[0] != NULL )
		{
			sBillboardMaterialBatch *p_batch = GetBatch( p_mesh->mp_material );
			if( p_batch == NULL )
			{
				Dbg_Assert( m_num_batches < ( MAX_BILLBOARD_BATCHES - 1 ));

				// Need to create a new batch for this material.
				p_batch = new sBillboardMaterialBatch( p_mesh->mp_material );
				mp_batches[m_num_batches++]	= p_batch;

				// Resort the list of batches.
				SortBatches();
			}

			// Now add the mesh to this batch.
			p_batch->AddEntry( p_mesh );
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sBillboardManager::RemoveEntry( sMesh *p_mesh )
{
	if( p_mesh->mp_material )
	{
		sBillboardMaterialBatch *p_batch = GetBatch( p_mesh->mp_material );
		if( p_batch )
		{
			// Removes the entry if it exists.
			p_batch->RemoveEntry( p_mesh );

			// If the batch is now empty, remove it.
			if( p_batch->IsEmpty())
			{
				for( int i = 0; i < m_num_batches; ++i )
				{
					if( mp_batches[i] == p_batch )
					{
						delete p_batch;
						mp_batches[i] = NULL;
						break;
					}
				}

				// Resort the batches (will move the NULL pointer to the end).
				SortBatches();

				// Important not to decrement this until after the resort has been performed.
				--m_num_batches;
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sBillboardManager::SortBatches( void )
{
	qsort( mp_batches, m_num_batches, sizeof( sBillboardMaterialBatch* ), sort_batches_by_draw_order );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sBillboardManager::SetCameraMatrix( void )
{
	Mth::Vector up( 0.0f, 1.0f, 0.0f );

	m_at.Set( NxXbox::EngineGlobals.view_matrix.m[0][2], NxXbox::EngineGlobals.view_matrix.m[1][2], NxXbox::EngineGlobals.view_matrix.m[2][2] );
	m_screen_right		= Mth::CrossProduct( m_at, up ).Normalize();
	m_screen_up			= Mth::CrossProduct( m_screen_right, m_at ).Normalize();
	m_at_xz				= Mth::Vector( m_at[X], 0.0f, m_at[Z] ).Normalize();
	m_screen_right_xz	= Mth::Vector( m_screen_right[X], 0.0f, m_screen_right[Z] ).Normalize();
}




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sBillboardManager::Render( uint32 flags )
{
	// Render the opaque billboards if requested.
	if( flags & vRENDER_OPAQUE )
	{
		for( int b = 0; b < m_num_batches; ++b )
		{
			if( !( mp_batches[b]->mp_material->m_flags[0] & 0x40 ))
			{
				mp_batches[b]->Render();
			}
		}
	}

	// Render the semitransparent billboards if requested.
	if( flags & vRENDER_SEMITRANSPARENT )
	{
		for( int b = 0; b < m_num_batches; ++b )
		{
			if( mp_batches[b]->mp_material->m_flags[0] & 0x40 )
			{
				mp_batches[b]->Render();
			}
		}
	}

}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sBillboardMaterialBatch::sBillboardMaterialBatch( sMaterial *p_material )
{
	mp_material		= p_material;
	mp_entries[0]	= new Lst::Head <sBillboardEntry>;
	mp_entries[1]	= new Lst::Head <sBillboardEntry>;
	mp_entries[2]	= new Lst::Head <sBillboardEntry>;

	// We can calculate what the per-entry size will be based on the material properties.
	m_entry_size	 = 4 * sizeof( float ) * 3;							// Four vertex positions.
	m_entry_size	+= 4 * sizeof( D3DCOLOR );							// Four vertex colors.
	m_entry_size	+= 4 * sizeof( float ) * 2 * p_material->m_passes;	// Four uv pairs for each pass.
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sBillboardMaterialBatch::~sBillboardMaterialBatch( void )
{
	delete mp_entries[0];
	delete mp_entries[1];
	delete mp_entries[2];
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool sBillboardMaterialBatch::IsEmpty( void )
{
	return ( mp_entries[0]->IsEmpty() && mp_entries[1]->IsEmpty() && mp_entries[2]->IsEmpty());
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
float sBillboardMaterialBatch::GetDrawOrder( void )
{
	return mp_material ? mp_material->m_draw_order : 0.0f;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sMaterial *sBillboardMaterialBatch::GetMaterial( void )
{
	return mp_material;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sBillboardMaterialBatch::AddEntry( sMesh *p_mesh )
{
	// Create a new billboard entry.
	sBillboardEntry *p_entry = new sBillboardEntry( p_mesh );

	// And a new node.
	Lst::Node<sBillboardEntry> *node = new Lst::Node<sBillboardEntry>( p_entry );

	mp_entries[p_entry->m_type]->AddToTail( node );

	// Now process the mesh.
	ProcessMesh( p_mesh );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sBillboardMaterialBatch::ProcessMesh( sMesh *p_mesh )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sBillboardMaterialBatch::RemoveEntry( sMesh *p_mesh )
{
	for( int e = 0; e < 3; ++e )
	{
		Lst::Node<sBillboardEntry> *node, *next;
		node = mp_entries[e]->GetNext();
		while( node )
		{
			next = node->GetNext();
			sBillboardEntry *p_entry = node->GetData();
			if( p_entry->mp_mesh == p_mesh )
			{
				// This is the entry. Delete the node and the entry.
				delete node;
				delete p_entry;
				return;
			}
			node = next;
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sBillboardMaterialBatch::Render( void )
{
	static float vector_upload[12] = {	0.0f, 0.0f, 0.0f, 1.0f,
										0.0f, 0.0f, 0.0f, 1.0f,
										0.0f, 0.0f, 0.0f, 1.0f };

	if( mp_material )
	{
		mp_material->Submit();

		// Set up correct vertex shader.
		NxXbox::set_vertex_shader( BillboardScreenAlignedVS );

		// Lock out vertex shader changes.
		EngineGlobals.vertex_shader_override = BillboardScreenAlignedVS;

		// Set up correct pixel shader - this assumes that all meshes with the same material will have the same pixel shader.
		for( int e = 0; e < 3; ++e )
		{
			if( mp_entries[e]->GetNext())
			{
				sBillboardEntry *p_entry = mp_entries[e]->GetNext()->GetData();
				set_pixel_shader( p_entry->mp_mesh->m_pixel_shader );
				break;
			}
		}

		// Load up the combined world->view_projection matrix.
		XGMATRIX	dest_matrix;
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
	
		// World space transformation matrix.
		XGMatrixIdentity( &worldMatrix );

		// Calculate composite world->view->projection matrix.
		XGMatrixMultiply( &temp_matrix, &viewMatrix, &worldMatrix );
		XGMatrixMultiply( &dest_matrix, &projMatrix, &temp_matrix );

		// Load up the combined world, camera & projection matrix.
		D3DDevice_SetVertexShaderConstantFast( 0, (void*)&dest_matrix, 4 );

		Lst::Node<sBillboardEntry> *node, *next;

		// First do the screen aligned billboards.
		vector_upload[0]	= BillboardManager.m_screen_right[X];
		vector_upload[1]	= BillboardManager.m_screen_right[Y];
		vector_upload[2]	= BillboardManager.m_screen_right[Z];
		vector_upload[4]	= BillboardManager.m_screen_up[X];
		vector_upload[5]	= BillboardManager.m_screen_up[Y];
		vector_upload[6]	= BillboardManager.m_screen_up[Z];
		vector_upload[8]	= BillboardManager.m_at[X];
		vector_upload[9]	= BillboardManager.m_at[Y];
		vector_upload[10]	= BillboardManager.m_at[Z];
		D3DDevice_SetVertexShaderConstantFast( 4, (void*)( &vector_upload[0] ), 3 );

		for( node = mp_entries[0]->GetNext(); node; node = next )
		{
			next = node->GetNext();
			sBillboardEntry *p_entry = node->GetData();

			// Only render if the mesh is active.
			if( p_entry->mp_mesh->m_flags & sMesh::MESH_FLAG_ACTIVE )
			{
				// Deal with material color override.
				if( p_entry->mp_mesh->m_flags & sMesh::MESH_FLAG_MATERIAL_COLOR_OVERRIDE )
					p_entry->mp_mesh->HandleColorOverride();
				else
					set_pixel_shader( p_entry->mp_mesh->m_pixel_shader );

				// Deal with vertex color wibble if present.
				if( p_entry->mp_mesh->mp_vc_wibble_data )
				{
					p_entry->mp_mesh->wibble_vc();
					IDirect3DVertexBuffer8*	p_submit_buffer = p_entry->mp_mesh->mp_vertex_buffer[p_entry->mp_mesh->m_current_write_vertex_buffer];
					p_entry->mp_mesh->SwapVertexBuffers();
					D3DDevice_SetStreamSource( 0, p_submit_buffer, p_entry->mp_mesh->m_vertex_stride );
					D3DDevice_DrawIndexedVertices( p_entry->mp_mesh->m_primitive_type, p_entry->mp_mesh->m_num_indices[0], p_entry->mp_mesh->mp_index_buffer[0] );
				}
				else
				{
					D3DDevice_SetStreamSource( 0, p_entry->mp_mesh->mp_vertex_buffer[0], p_entry->mp_mesh->m_vertex_stride );
					D3DDevice_DrawIndexedVertices( p_entry->mp_mesh->m_primitive_type, p_entry->mp_mesh->m_num_indices[0], p_entry->mp_mesh->mp_index_buffer[0] );
				}
			}
		}

		// Next do the the y axis aligned billboards.
		vector_upload[0]	= BillboardManager.m_screen_right_xz[X];
		vector_upload[1]	= BillboardManager.m_screen_right_xz[Y];
		vector_upload[2]	= BillboardManager.m_screen_right_xz[Z];
		vector_upload[4]	= 0.0f;
		vector_upload[5]	= 1.0f;
		vector_upload[6]	= 0.0f;
		vector_upload[8]	= BillboardManager.m_at_xz[X];
		vector_upload[9]	= BillboardManager.m_at_xz[Y];
		vector_upload[10]	= BillboardManager.m_at_xz[Z];
		D3DDevice_SetVertexShaderConstantFast( 4, (void*)vector_upload, 3 );

		for( node = mp_entries[1]->GetNext(); node; node = next )
		{
			next = node->GetNext();
			sBillboardEntry *p_entry = node->GetData();

			// Only render if the mesh is active.
			if( p_entry->mp_mesh->m_flags & sMesh::MESH_FLAG_ACTIVE )
			{
				// Deal with material color override.
				if( p_entry->mp_mesh->m_flags & sMesh::MESH_FLAG_MATERIAL_COLOR_OVERRIDE )
					p_entry->mp_mesh->HandleColorOverride();
				else
					set_pixel_shader( p_entry->mp_mesh->m_pixel_shader );

				// Deal with vertex color wibble if present.
				if( p_entry->mp_mesh->mp_vc_wibble_data )
				{
					p_entry->mp_mesh->wibble_vc();
					IDirect3DVertexBuffer8*	p_submit_buffer = p_entry->mp_mesh->mp_vertex_buffer[p_entry->mp_mesh->m_current_write_vertex_buffer];
					p_entry->mp_mesh->SwapVertexBuffers();
					D3DDevice_SetStreamSource( 0, p_submit_buffer, p_entry->mp_mesh->m_vertex_stride );
					D3DDevice_DrawIndexedVertices( p_entry->mp_mesh->m_primitive_type, p_entry->mp_mesh->m_num_indices[0], p_entry->mp_mesh->mp_index_buffer[0] );
				}
				else
				{
					D3DDevice_SetStreamSource( 0, p_entry->mp_mesh->mp_vertex_buffer[0], p_entry->mp_mesh->m_vertex_stride );
					D3DDevice_DrawIndexedVertices( p_entry->mp_mesh->m_primitive_type, p_entry->mp_mesh->m_num_indices[0], p_entry->mp_mesh->mp_index_buffer[0] );
				}
			}
		}

		// Now do the arbitrary axis aligned billboards.
		for( node = mp_entries[2]->GetNext(); node; node = next )
		{
			next = node->GetNext();
			sBillboardEntry *p_entry		= node->GetData();
			sBillboardData *p_entry_data	= p_entry->mp_mesh->mp_billboard_data;
		
			// Only render if the mesh is active.
			if( p_entry->mp_mesh->m_flags & sMesh::MESH_FLAG_ACTIVE )
			{
				// For this type we need to calculate the vector perpendicular to both the view vector and the axis of rotation.
				Mth::Vector axis_right = Mth::CrossProduct( BillboardManager.m_at, p_entry_data->m_pivot_axis ).Normalize();
				Mth::Vector axis_at = Mth::CrossProduct( p_entry_data->m_pivot_axis, axis_right ).Normalize();

				// Begin state block to set vertex shader constants for bone transforms.
				DWORD *p_push;
				EngineGlobals.p_Device->BeginState( 15, &p_push );

				// 1 here isn't the parameter for SET_TRANSFORM_CONSTANT_LOAD; rather, it's the number of dwords written to that register.
				p_push[0]	= D3DPUSH_ENCODE( D3DPUSH_SET_TRANSFORM_CONSTANT_LOAD, 1 ); 

				// Here is the actual parameter for SET_TRANSFORM_CONSTANT_LOAD. Always add 96 to the constant register.
				p_push[1]	= 4 + 96;
				p_push[2]	= D3DPUSH_ENCODE( D3DPUSH_SET_TRANSFORM_CONSTANT, 12 );

				p_push[3]	= *((DWORD*)&axis_right[X] );
				p_push[4]	= *((DWORD*)&axis_right[Y] );
				p_push[5]	= *((DWORD*)&axis_right[Z] );

				p_push[7]	= *((DWORD*)&p_entry_data->m_pivot_axis[X] );
				p_push[8]	= *((DWORD*)&p_entry_data->m_pivot_axis[Y] );
				p_push[9]	= *((DWORD*)&p_entry_data->m_pivot_axis[Z] );

				p_push[11]	= *((DWORD*)&axis_at[X] );
				p_push[12]	= *((DWORD*)&axis_at[Y] );
				p_push[13]	= *((DWORD*)&axis_at[Z] );

				p_push		+= 15;
				EngineGlobals.p_Device->EndState( p_push );

				// Deal with material color override.
				if( p_entry->mp_mesh->m_flags & sMesh::MESH_FLAG_MATERIAL_COLOR_OVERRIDE )
					p_entry->mp_mesh->HandleColorOverride();
				else
					set_pixel_shader( p_entry->mp_mesh->m_pixel_shader );

				// Deal with vertex color wibble if present.
				if( p_entry->mp_mesh->mp_vc_wibble_data )
				{
					p_entry->mp_mesh->wibble_vc();
					IDirect3DVertexBuffer8*	p_submit_buffer = p_entry->mp_mesh->mp_vertex_buffer[p_entry->mp_mesh->m_current_write_vertex_buffer];
					p_entry->mp_mesh->SwapVertexBuffers();
					D3DDevice_SetStreamSource( 0, p_submit_buffer, p_entry->mp_mesh->m_vertex_stride );
					D3DDevice_DrawIndexedVertices( p_entry->mp_mesh->m_primitive_type, p_entry->mp_mesh->m_num_indices[0], p_entry->mp_mesh->mp_index_buffer[0] );
				}
				else
				{
					D3DDevice_SetStreamSource( 0, p_entry->mp_mesh->mp_vertex_buffer[0], p_entry->mp_mesh->m_vertex_stride );
					D3DDevice_DrawIndexedVertices( p_entry->mp_mesh->m_primitive_type, p_entry->mp_mesh->m_num_indices[0], p_entry->mp_mesh->mp_index_buffer[0] );
				}
			}
		}

		// Finally do the world aligned billboards.
//		for( node = mp_entries[3]->GetNext(); node; node = next )
//		{
//			next = node->GetNext();
//			sBillboardEntry *p_entry = node->GetData();
		
			// Only render if the mesh is active.
//			if( p_entry->mp_mesh->m_flags & sMesh::MESH_FLAG_ACTIVE )
//			{
				// For this type we need to calculate the vector perpendicular to both the view vector and the axis of rotation.
//				Mth::Vector at( p_entry->m_pivot_axis[X] - EngineGlobals.cam_position.x,
//								p_entry->m_pivot_axis[Y] - EngineGlobals.cam_position.y,
//								p_entry->m_pivot_axis[Z] - EngineGlobals.cam_position.z );
//				at.Normalize();

//				Mth::Vector axis_right = Mth::CrossProduct( at, BillboardManager.m_screen_up ).Normalize();
//				Mth::Vector axis_at = Mth::CrossProduct( BillboardManager.m_screen_up, axis_right ).Normalize();

//				vector_upload[0]	= axis_right[X];
//				vector_upload[1]	= axis_right[Y];
//				vector_upload[2]	= axis_right[Z];
//				vector_upload[4]	= BillboardManager.m_screen_up[X];
//				vector_upload[5]	= BillboardManager.m_screen_up[Y];
//				vector_upload[6]	= BillboardManager.m_screen_up[Z];
//				vector_upload[8]	= axis_at[X];
//				vector_upload[9]	= axis_at[Y];
//				vector_upload[10]	= axis_at[Z];
//				D3DDevice_SetVertexShaderConstant( 4, (void*)vector_upload, 3 );

//				D3DDevice_SetStreamSource( 0, p_entry->mp_mesh->mp_vertex_buffer[0], p_entry->mp_mesh->m_vertex_stride );
//				D3DDevice_DrawIndexedVertices( p_entry->mp_mesh->m_primitive_type, p_entry->mp_mesh->m_num_indices[0], p_entry->mp_mesh->mp_index_buffer[0] );
//			}
//		}

		// Undo vertex shader lock.
		EngineGlobals.vertex_shader_override = 0;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sBillboardMaterialBatch::Reset( void )
{
	Lst::Node<sBillboardEntry> *node, *next;

	for( int e = 0; e < 3; ++e )
	{
		if( mp_entries[e] )
		{
			for( node = mp_entries[e]->GetNext(); node; node = next )
			{
				next = node->GetNext();
				delete node;
			}
		}
	}

	mp_material		= NULL;
	m_entry_size	= 0;
}







/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sBillboardEntry::sBillboardEntry( sMesh *p_mesh )
{
	Dbg_Assert( p_mesh->mp_billboard_data != NULL );

	mp_mesh			= p_mesh;
	m_type			= p_mesh->mp_billboard_data->m_type;

	// For now set the pivot point as the center of the bounding sphere.
//	m_pivot_pos.Set( p_mesh->m_sphere_center.x, p_mesh->m_sphere_center.y, p_mesh->m_sphere_center.z );

	// Frig the verts for now.
//	m_verts[0][0]	= -12.0f;
//	m_verts[0][1]	= -24.0f;
//	m_verts[0][2]	= 0.0f;

//	m_verts[1][0]	= -12.0f;
//	m_verts[1][1]	= 24.0f;
//	m_verts[1][2]	= 0.0f;

//	m_verts[2][0]	= 12.0f;
//	m_verts[2][1]	= 24.0f;
//	m_verts[2][2]	= 0.0f;

//	m_verts[3][0]	= 12.0f;
//	m_verts[3][1]	= -24.0f;
//	m_verts[3][2]	= 0.0f;


//	switch( rand() & 0x03 )
//	{
//		case 0:
//		{
//			m_type = vBILLBOARD_TYPE_SCREEN_ALIGNED;
//			break;
//		}
//		case 1:
//		{
//			m_type = vBILLBOARD_TYPE_Y_AXIS_ALIGNED;
//			break;
//		}
//		case 2:
//		case 3:
//		{
//			m_type = vBILLBOARD_TYPE_ARBITRARY_AXIS_ALIGNED;
//			m_pivot_axis = Mth::Vector((float)( rand() - rand()), (float)( rand() - rand()), (float)( rand() - rand())).Normalize();
//			break;
//		}
//	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sBillboardEntry::~sBillboardEntry( void )
{
}








} // namespace NxXbox

