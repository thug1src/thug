///////////////////////////////////////////////////////////////////////////////
// p_NxImposter.cpp

#include 	"sys/timer.h"
#include 	"gfx/xbox/p_NxGeom.h"
#include 	"gfx/xbox/p_NxImposter.h"
#include 	"gfx/xbox/nx/nx_init.h"
#include 	"gfx/xbox/nx/render.h"

namespace Nx
{

	
const int XBOX_IMPOSTER_UPDATE_LIMIT		= 30;
const int XBOX_IMPOSTER_MAX_U_TEXTURE_SIZE	= 128;
const int XBOX_IMPOSTER_MAX_V_TEXTURE_SIZE	= 128;
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static void frustum_project_box( Mth::CBBox &bbox, XGMATRIX *p_view_matrix, Mth::Vector *p_max_x, Mth::Vector *p_max_y, Mth::Vector *p_mid )
{
	float	max_projected_xx, max_projected_xz;		// The camera space position of the point with the greatest x axis value when projected to z = mid_z.
	float	max_projected_x_mid_z;					// The projected x axis value when this point is projected to z = mid_z;

	float	max_projected_yy, max_projected_yz;		// The camera space position of the point furthest along the y axis when projected to z = mid_z.
	float	max_projected_y_mid_z;					// The projected y axis value when this point is projected to z = mid_z;

	float	min_x = bbox.GetMin().GetX();
	float	min_y = bbox.GetMin().GetY();
	float	min_z = bbox.GetMin().GetZ();
	float	max_x = bbox.GetMax().GetX();
	float	max_y = bbox.GetMax().GetY();
	float	max_z = bbox.GetMax().GetZ();

	// Project the midpoint of the box, since this is the point through which the imposter polygon will pass.
	XGVECTOR3 mid_in( min_x + ( 0.5f * ( max_x - min_x )), min_y + ( 0.5f * ( max_y - min_y )), min_z + ( 0.5f * ( max_z - min_z )));
	XGVECTOR4 mid_out;
	XGVec3Transform( &mid_out, &mid_in, p_view_matrix );

	for( uint32 v = 0; v < 8; ++v )
	{
		XGVECTOR3 in;
		XGVECTOR4 out;

		in.x = ( v & 0x04 ) ? max_x : min_x;
		in.y = ( v & 0x02 ) ? max_y : min_y;
		in.z = ( v & 0x01 ) ? max_z : min_z;
		
		XGVec3Transform( &out, &in, p_view_matrix );

		out.x = fabsf( out.x );
		out.y = fabsf( out.y );

		float projected_x_mid_z	= out.x * ( mid_out.z / out.z );
		float projected_y_mid_z	= out.y * ( mid_out.z / out.z );

		if( v == 0 )
		{
			max_projected_x_mid_z	= projected_x_mid_z;
			max_projected_xx		= out.x;
			max_projected_xz		= out.z;

			max_projected_y_mid_z	= projected_y_mid_z;
			max_projected_yy		= out.y;
			max_projected_yz		= out.z;
		}
		else
		{
			if( projected_x_mid_z > max_projected_x_mid_z )
			{
				max_projected_xx		= out.x;
				max_projected_xz		= out.z;
				max_projected_x_mid_z	= projected_x_mid_z;
			}

			if( projected_y_mid_z > max_projected_y_mid_z )
			{
				max_projected_yy		= out.y;
				max_projected_yz		= out.z;
				max_projected_y_mid_z	= projected_y_mid_z;
			}
		}
	}

	p_max_x->Set( max_projected_xx, 0.0f, max_projected_xz, 0.0f );
	p_max_y->Set( 0.0f, max_projected_yy, max_projected_yz, 0.0f );
	p_mid->Set( mid_out.x, mid_out.y, mid_out.z );
}
	
	
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CImposterGroup* CImposterManager::plat_create_imposter_group( void )
{
	return new CXboxImposterGroup;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterManager::plat_pre_render_imposters( void )
{
	// Set up the common material attributes for the imposters.
//	NxXbox::set_blend_mode( NxXbox::vBLEND_MODE_BLEND );
	NxXbox::set_blend_mode( NxXbox::vBLEND_MODE_ONE_INV_SRC_ALPHA );

	NxXbox::set_vertex_shader( D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2( 0 ));
	NxXbox::set_pixel_shader( PixelShader2 );

	NxXbox::set_render_state( RS_UVADDRESSMODE0,	0x00010001UL );
	NxXbox::set_render_state( RS_ALPHACUTOFF,		1 );
	NxXbox::set_render_state( RS_ZWRITEENABLE,		1 );
	NxXbox::set_render_state( RS_ZTESTENABLE,		1 );
	NxXbox::set_render_state( RS_CULLMODE,			D3DCULL_NONE );

	D3DDevice_SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
	D3DDevice_SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU | 0 );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CImposterManager::plat_post_render_imposters( void )
{
	// Clean up the common material attributes for the imposters.
	NxXbox::set_texture( 0, NULL );
	NxXbox::set_render_state( RS_CULLMODE, D3DCULL_CW );
}



/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Here's a machine specific implementation of the CImposterGroup

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxImposterGroup::CXboxImposterGroup()
{
	mp_texture					= NULL;
	m_update_count				= Mth::Rnd( XBOX_IMPOSTER_UPDATE_LIMIT );
	mp_removed_textures_list	= new Lst::Head <sRemovedTextureDetails>;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxImposterGroup::~CXboxImposterGroup()
{
	if( mp_texture )
	{
		mp_texture->Release();
		mp_texture = NULL;
	}

	// Remove all nodes from the removed textures table.
	Lst::Node<sRemovedTextureDetails> *p_node, *p_next;
	for( p_node = mp_removed_textures_list->GetNext(); p_node; p_node = p_next )
	{
		p_next = p_node->GetNext();

		sRemovedTextureDetails	*p_details	= p_node->GetData();
		IDirect3DTexture8		*p_texture	= p_details->mp_texture;
		p_texture->Release();

		delete p_details;
		delete p_node;
	}

	// Remove the table itself.
	delete mp_removed_textures_list;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxImposterGroup::process_removed_textures( void )
{
	Lst::Node<sRemovedTextureDetails> *p_node, *p_next;
	for( p_node = mp_removed_textures_list->GetNext(); p_node; p_node = p_next )
	{
		p_next = p_node->GetNext();

		sRemovedTextureDetails	*p_details	= p_node->GetData();
		int						time		= p_details->m_time_removed;
		if((( NxXbox::EngineGlobals.render_start_time - time ) > 250 ) || ( time > NxXbox::EngineGlobals.render_start_time ))
		{
			IDirect3DTexture8*	p_texture	= p_details->mp_texture;
			p_texture->Release();

			delete p_details;
			delete p_node;
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxImposterGroup::plat_process( void )
{
	process_removed_textures();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxImposterGroup::plat_create_imposter_polygon( void )
{
	Dbg_Assert( !m_imposter_polygon_exists );

	// Generate a camera matrix that will point the camera directly at the midpoint of the bounding box.
	XGMATRIX	cam_mat;
	XGVECTOR3	look_at( m_composite_bbox_mid[X], m_composite_bbox_mid[Y], m_composite_bbox_mid[Z] );
	XGMatrixLookAtRH( &cam_mat, &NxXbox::EngineGlobals.cam_position, &look_at, &NxXbox::EngineGlobals.cam_up );

	// Using this camera matrix, project all eight corners of the bounding box, in order to determine the best size for the texture.
	Mth::Vector proj_max_x, proj_max_y, proj_mid;
	frustum_project_box( m_composite_bbox, &cam_mat, &proj_max_x, &proj_max_y, &proj_mid );

	// Now project the minimum and maximum x and y values onto the projection plane - the plane through the midpoint of the box.
	float max_projected_x	= proj_max_x[X] * ( proj_mid[Z] / proj_max_x[Z] );
	float max_projected_y	= proj_max_y[Y] * ( proj_mid[Z] / proj_max_y[Z] );

	// Calculate the maximum width and height at the near plane.
	float wnp				= 2.0f * proj_max_x[X] * ( NxXbox::EngineGlobals.near_plane / fabsf( proj_max_x[Z] ));
	float hnp				= 2.0f * proj_max_y[Y] * ( NxXbox::EngineGlobals.near_plane / fabsf( proj_max_y[Z] ));

	m_tex_width				= 16 + Ftoi_ASM(( 640.0f * wnp ) / NxXbox::EngineGlobals.near_plane_width );
	m_tex_height			= 16 + Ftoi_ASM(( 480.0f * hnp ) / NxXbox::EngineGlobals.near_plane_height );

	// Round texture to the nearest 16 pixel limit.
	m_tex_width				= (( m_tex_width + 0x0F ) & ~0x0F );
	m_tex_height			= (( m_tex_height + 0x0F ) & ~0x0F );

	// Clamp texture to maximum allowed size.
	m_tex_width				= ( m_tex_width > XBOX_IMPOSTER_MAX_U_TEXTURE_SIZE ) ? XBOX_IMPOSTER_MAX_U_TEXTURE_SIZE : m_tex_width;
	m_tex_height			= ( m_tex_height > XBOX_IMPOSTER_MAX_V_TEXTURE_SIZE ) ? XBOX_IMPOSTER_MAX_V_TEXTURE_SIZE : m_tex_height;

	// Calculate the corresponding projection matrix.
	XGMATRIX proj_mat;
	XGMatrixPerspectiveRH( &proj_mat, wnp, hnp, NxXbox::EngineGlobals.near_plane, NxXbox::EngineGlobals.far_plane );

	// Set the calculated view and projection matrices.
	D3DDevice_SetTransform( D3DTS_VIEW, &cam_mat );
	D3DDevice_SetTransform( D3DTS_PROJECTION, &proj_mat );

	// Create a render target texture into which the object will be drawn.
	HRESULT hr;
	if( mp_texture == NULL )
	{
		hr = D3DDevice_CreateTexture( m_tex_width, m_tex_height, 1, 0, D3DFMT_LIN_A8R8G8B8, 0, &mp_texture );
		Dbg_Assert( hr == D3D_OK );
	}

	// Create a corresponding depth texture (we need this only for the render - then it can be removed).
	IDirect3DTexture8* p_depth_buffer;
	hr = D3DDevice_CreateTexture( m_tex_width, m_tex_height, 1, 0, D3DFMT_LIN_D24S8, 0, &p_depth_buffer );
	Dbg_Assert( hr == D3D_OK );

	// This call will increase the reference count of the IDirect3DTexture8 object.
	LPDIRECT3DSURFACE8 p_surface, p_depth_surface;
	mp_texture->GetSurfaceLevel( 0, &p_surface );
	p_depth_buffer->GetSurfaceLevel( 0, &p_depth_surface );

	// This call will increase the reference count of the IDirect3DSurface8 object.
	D3DDevice_SetRenderTarget( p_surface, p_depth_surface );
			
	// Clear the render target.
	D3DDevice_Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0x00000000, 1.0f, 0 );
//	D3DDevice_Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0x80800000, 1.0f, 0 );

	// The imposter polygon has now been created.
	m_imposter_polygon_exists = true;

	// Set the camera position at the time of creation.
	m_cam_pos.Set( NxXbox::EngineGlobals.cam_position.x, NxXbox::EngineGlobals.cam_position.y, NxXbox::EngineGlobals.cam_position.z );

	// Build a list of meshes, so we can sort them dynamically into draw order.
	NxXbox::sMesh*	mesh_list[256];
	uint32			num_meshes = 0;

	Lst::Node<Nx::CGeom> *node, *next;
	for( node = mp_geom_list->GetNext(); node; node = next )
	{
		next = node->GetNext();
		Nx::CXboxGeom *p_xbox_geom = static_cast<Nx::CXboxGeom*>( node->GetData());

		for( uint32 m = 0; m < p_xbox_geom->m_num_mesh; ++m )
		{
			Dbg_Assert( num_meshes < 256 );
			NxXbox::sMesh* p_mesh	= p_xbox_geom->m_mesh_array[m];
			mesh_list[num_meshes++]	= p_mesh;
		}
	}

	if( num_meshes > 0 )
	{
		// Sort the array of pointers into draw order.
		qsort( mesh_list, num_meshes, sizeof( NxXbox::sMesh* ), NxXbox::sort_by_material_draw_order );
	}

	// Render each mesh into the render target.
	NxXbox::EngineGlobals.blend_mode_override = NxXbox::vBLEND_MODE_ONE_INV_SRC_ALPHA;
	NxXbox::sMaterial*	p_material = NULL;
	for( uint32 m = 0; m < num_meshes; ++m )
	{
		NxXbox::sMesh* p_mesh = mesh_list[m];
		if( p_mesh->mp_material != p_material )
		{
			p_material	= p_mesh->mp_material;
			p_material->Submit();
		}
		p_mesh->Submit();
	}
	NxXbox::EngineGlobals.blend_mode_override = 0;

	// Can now set the meshes in the geom inactive.
	for( node = mp_geom_list->GetNext(); node; node = next )
	{
		next = node->GetNext();
		Nx::CXboxGeom *p_xbox_geom = static_cast<Nx::CXboxGeom*>( node->GetData());
		p_xbox_geom->SetActive( false );
	}

	// Remove references to surfaces.
	p_surface->Release();
	p_depth_surface->Release();
	p_depth_buffer->Release();

	// Restore the default render target.
	D3DDevice_SetRenderTarget( NxXbox::EngineGlobals.p_RenderSurface, NxXbox::EngineGlobals.p_ZStencilSurface );

	// Restore the view and projection transforms.
	D3DDevice_SetTransform( D3DTS_VIEW, &NxXbox::EngineGlobals.view_matrix );
	D3DDevice_SetTransform( D3DTS_PROJECTION, &NxXbox::EngineGlobals.projection_matrix );

	// Now figure the vertex positions for the polygon.
	Mth::Vector at = m_composite_bbox_mid - m_cam_pos;
	at.Normalize();
	Mth::Vector right	= Mth::CrossProduct( Mth::Vector( NxXbox::EngineGlobals.cam_up.x, NxXbox::EngineGlobals.cam_up.y, NxXbox::EngineGlobals.cam_up.z ), at );
	right.Normalize();
	Mth::Vector up		= Mth::CrossProduct( at, right );

	Mth::Vector	verts[4];
	verts[0]			= m_composite_bbox_mid - ( max_projected_x * right ) + ( max_projected_y * up );
	verts[1]			= m_composite_bbox_mid + ( max_projected_x * right ) + ( max_projected_y * up );
	verts[2]			= m_composite_bbox_mid + ( max_projected_x * right ) - ( max_projected_y * up );
	verts[3]			= m_composite_bbox_mid - ( max_projected_x * right ) - ( max_projected_y * up );

	for( int v = 0; v < 4; ++v )
	{
		m_vertex_buffer[v].x	= verts[v][X];
		m_vertex_buffer[v].y	= verts[v][Y];
		m_vertex_buffer[v].z	= verts[v][Z];
	}

	// The texture is a linear format, so the uv's are in texel space.
	m_vertex_buffer[0].u		= (float)m_tex_width;
	m_vertex_buffer[0].v		= 0.0f;
	m_vertex_buffer[1].u		= 0.0f;
	m_vertex_buffer[1].v		= 0.0f;
	m_vertex_buffer[2].u		= 0.0f;
	m_vertex_buffer[2].v		= (float)m_tex_height;
	m_vertex_buffer[3].u		= (float)m_tex_width;
	m_vertex_buffer[3].v		= (float)m_tex_height;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxImposterGroup::plat_remove_imposter_polygon( void )
{
	if( m_imposter_polygon_exists )
	{
		m_imposter_polygon_exists = false;

		if( mp_texture )
		{
			// At this point move the texture resource into a list of removed textures. Here it will remain
			// for sufficient time to ensure that the GPU will no longer try to access it during push buffer processing.
			sRemovedTextureDetails	*p_new_details			= new sRemovedTextureDetails;
			p_new_details->mp_texture						= mp_texture;
			p_new_details->m_time_removed					= NxXbox::EngineGlobals.render_start_time;

			Lst::Node<sRemovedTextureDetails> *p_new_node	= new Lst::Node<sRemovedTextureDetails>( p_new_details );
			mp_removed_textures_list->AddToTail( p_new_node );

			mp_texture = NULL;
		}

		Lst::Node<Nx::CGeom> *node, *next;
		for( node = mp_geom_list->GetNext(); node; node = next )
		{
			next = node->GetNext();
			Nx::CXboxGeom *p_xbox_geom = static_cast<Nx::CXboxGeom*>( node->GetData());

			// Can now set the meshes in the geom active.
			p_xbox_geom->SetActive( true );
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxImposterGroup::plat_update_imposter_polygon( void )
{
	// Calculate the new vector from bounding box midpoint to camera.
	Mth::Vector new_vec( NxXbox::EngineGlobals.cam_position.x - m_composite_bbox_mid[X], 
						 NxXbox::EngineGlobals.cam_position.y - m_composite_bbox_mid[Y],
						 NxXbox::EngineGlobals.cam_position.z - m_composite_bbox_mid[Z] );
	new_vec.Normalize();

	// Calculate the old vector from bounding box midpoint to camera when the imposter was created.
	Mth::Vector old_vec = m_cam_pos - m_composite_bbox_mid;
	old_vec.Normalize();

	float angle_change = acosf( Mth::DotProduct( new_vec, old_vec ));

	// Rebuild the imposter polygon if the angle change is beyond some limit.
	if( angle_change > Mth::DegToRad( 5.0f ))
	{
		RemoveImposterPolygon();
		CreateImposterPolygon();
		return true;
	}

	// Check intermittently to see if the projected screen area of the imposter has changed sufficienty
	// to warrant regenerating a new texture.
	if( m_update_count >= XBOX_IMPOSTER_UPDATE_LIMIT )
	{
		m_update_count = 0;

		// Generate a camera matrix that will point the camera directly at the midpoint of the bounding box.
		XGMATRIX	cam_mat;
		XGVECTOR3	look_at( m_composite_bbox_mid[X], m_composite_bbox_mid[Y], m_composite_bbox_mid[Z] );
		XGMatrixLookAtRH( &cam_mat, &NxXbox::EngineGlobals.cam_position, &look_at, &NxXbox::EngineGlobals.cam_up );

		// Using this camera matrix, project all eight corners of the bounding box, in order to determine the best size for the texture.
		Mth::Vector proj_max_x, proj_max_y, proj_mid;
		frustum_project_box( m_composite_bbox, &cam_mat, &proj_max_x, &proj_max_y, &proj_mid );

		// Calculate the maximum width and height at the near plane.
		float wnp		= 2.0f * proj_max_x[X] * ( NxXbox::EngineGlobals.near_plane / fabsf( proj_max_x[Z] ));
		float hnp		= 2.0f * proj_max_y[Y] * ( NxXbox::EngineGlobals.near_plane / fabsf( proj_max_y[Z] ));

		// Round texture to the nearest 16 pixel limit.
		int tex_width	= 16 + Ftoi_ASM(( 640.0f * wnp ) / NxXbox::EngineGlobals.near_plane_width );
		int tex_height	= 16 + Ftoi_ASM(( 480.0f * hnp ) / NxXbox::EngineGlobals.near_plane_height );
		tex_width		= ( tex_width + 0x0F ) & ~0x0F;
		tex_height		= ( tex_height + 0x0F ) & ~0x0F;

		// Clamp texture to maximum allowed size.
		tex_width		= ( tex_width > XBOX_IMPOSTER_MAX_U_TEXTURE_SIZE ) ? XBOX_IMPOSTER_MAX_U_TEXTURE_SIZE : tex_width;
		tex_height		= ( tex_height > XBOX_IMPOSTER_MAX_V_TEXTURE_SIZE ) ? XBOX_IMPOSTER_MAX_V_TEXTURE_SIZE : tex_height;

		if(( tex_width != m_tex_width ) || ( tex_height != m_tex_height ))
		{
			RemoveImposterPolygon();
			CreateImposterPolygon();
			return true;
		}
	}

	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxImposterGroup::plat_draw_imposter_polygon( void )
{
	// Have to clear texture 0 before switching to the imposter texture, because it is a linear format.
	NxXbox::set_texture( 0, NULL );
	NxXbox::set_texture( 0, mp_texture );

	NxXbox::EngineGlobals.p_Device->DrawPrimitiveUP( D3DPT_QUADLIST, 1, m_vertex_buffer, sizeof( sImposterPolyVert ));
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
float CXboxImposterGroup::plat_check_distance( void )
{
	// First check the visibility, using the bounding sphere.
	bool visible = NxXbox::frustum_check_sphere((D3DXVECTOR3*)&( m_composite_bbox_mid[X] ), m_composite_bsphere_radius );

	if( !visible )
		return 0.0f;

	float	min_x = m_composite_bbox.GetMin().GetX();
	float	min_y = m_composite_bbox.GetMin().GetY();
	float	min_z = m_composite_bbox.GetMin().GetZ();
	float	max_x = m_composite_bbox.GetMax().GetX();
	float	max_y = m_composite_bbox.GetMax().GetY();
	float	max_z = m_composite_bbox.GetMax().GetZ();

	// The camera-space distance to the nearest point on the composite bounding box of the imposter.
	float	nearest = NxXbox::EngineGlobals.far_plane;

	for( uint32 v = 0; v < 8; ++v )
	{
		XGVECTOR3 test_in( ( v & 0x04 ) ? max_x : min_x, ( v & 0x02 ) ? max_y : min_y, ( v & 0x01 ) ? max_z : min_z );
		XGVECTOR4 test_mid;
		
		XGVec3Transform( &test_mid, &test_in, &NxXbox::EngineGlobals.view_matrix );

		// Do z-checking here.
		if( -test_mid.z < m_switch_distance )
		{
			return -test_mid.z;
		}
		else if( -test_mid.z < nearest )
		{
			nearest = -test_mid.z;
		}
	}
	return nearest;
}




} // Namespace Nx  			
				
				
