//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxGeom.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  3/4/2002
//****************************************************************************


#include <gfx/xbox/p_nxgeom.h>

#include <core/math.h>

#include <gfx/skeleton.h>
#include <gfx/xbox/p_nxmesh.h>
#include <gfx/xbox/p_nxmodel.h>
#include <gfx/xbox/p_nxtexture.h>
#include <gfx/xbox/nx/instance.h>

#include <xbdm.h>


namespace Nx
{

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/
						
/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxGeom::CXboxGeom() : mp_init_mesh_list( NULL ), m_mesh_array( NULL ), m_visible( 0xDEADBEEF )
{
	mp_instance = NULL;
//	mp_sector	= NULL;
	m_scale.Set( 1.0f, 1.0f, 1.0f );
	m_active	= true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxGeom::~CXboxGeom( void )
{
	DestroyMeshArray();

	if( mp_instance != NULL )
	{
		delete mp_instance;
		mp_instance = NULL;
	}
}



/////////////////////////////////////////////////////////////////////////////////////
// Mesh List Functions
//
// The mesh list is only for initialization of the CGeom.  As we find all the
// meshes for the CGeoms, we add them to the temporary lists.  When we are
// done, the list is copied into a permanent array.
//
// All the list functions use the TopDownHeap for memory allocation.

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::InitMeshList( void )
{
	Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().TopDownHeap());
	mp_init_mesh_list = new Lst::Head< NxXbox::sMesh >;
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::AddMesh( NxXbox::sMesh *mesh )
{
	Dbg_Assert( mp_init_mesh_list );

	Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().TopDownHeap());
	Lst::Node< NxXbox::sMesh > *node = new Lst::Node< NxXbox::sMesh > (mesh);
	mp_init_mesh_list->AddToTail( node );
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::CreateMeshArray( void )
{
	if( !m_mesh_array )
	{
		Dbg_Assert( mp_init_mesh_list );

		m_num_mesh = mp_init_mesh_list->CountItems();
		if (m_num_mesh)
		{
			Lst::Node< NxXbox::sMesh > *mesh, *next;

			m_mesh_array = new NxXbox::sMesh*[m_num_mesh];
			int k = 0;
			for( mesh = mp_init_mesh_list->GetNext(); mesh; mesh = next )
			{
				next			= mesh->GetNext();
				m_mesh_array[k]	= mesh->GetData();
				delete mesh;
				k++;
			}
		}

		// Delete temporary list.
		delete mp_init_mesh_list;
		mp_init_mesh_list = NULL;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxGeom::RegisterMeshArray( bool just_count )
{
	// Tells the engine to count or add the meshes for this sector to the rendering list for supplied scene id.
	if( just_count )
	{
		mp_scene->GetEngineScene()->CountMeshes( m_num_mesh, m_mesh_array );
	}
	else
	{
		mp_scene->GetEngineScene()->AddMeshes( m_num_mesh, m_mesh_array );
	}
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::DestroyMeshArray( void )
{
	if( m_mesh_array )
	{
		// Tells the engine to remove the meshes for this sector from the rendering list for supplied scene id.
		if( mp_scene && mp_scene->GetEngineScene())
		{
			mp_scene->GetEngineScene()->RemoveMeshes( m_num_mesh, m_mesh_array );
		}

		// Now actually go through and delete each mesh.
		for( uint32 m = 0; m < m_num_mesh; ++m )
		{
			NxXbox::sMesh *p_mesh = m_mesh_array[m];
			delete p_mesh;
		}

		delete [] m_mesh_array;
		m_mesh_array = NULL;
	}
}



/******************************************************************/
/*                                                                */
/* Generates an entirely new engine scene.                        */
/* Used when instance-cloning CGeoms.                             */
/*                                                                */
/******************************************************************/
NxXbox::sScene* CXboxGeom::GenerateScene( void )
{
	NxXbox::sScene *p_scene = new NxXbox::sScene();

	p_scene->CountMeshes( m_num_mesh, m_mesh_array );
	p_scene->CreateMeshArrays();
	p_scene->AddMeshes( m_num_mesh, m_mesh_array );
	p_scene->SortMeshes();

	// Set the bounding box and bounding sphere.
	p_scene->m_bbox = m_bbox;
	p_scene->FigureBoundingVolumes();
	
	return p_scene;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxGeom::plat_load_geom_data( CMesh *pMesh, CModel *pModel, bool color_per_material )
{
	// The skeleton must exist by this point (unless it's a hacked up car).
	int			numBones = pModel->GetNumBones();
	Mth::Matrix	temp;
	
	temp.Identity();

	CXboxMesh *p_xbox_mesh = static_cast<CXboxMesh*>( pMesh );
	
	// Store a pointer to the CXboxMesh, used for obtaining CAS poly removal data.
	mp_mesh = p_xbox_mesh;

	CXboxModel *p_xbox_model = static_cast<CXboxModel*>( pModel );

	NxXbox::CInstance *p_instance = new NxXbox::CInstance( p_xbox_mesh->GetScene()->GetEngineScene(), temp, numBones, pModel->GetBoneTransforms());
	
	SetInstance( p_instance );
	p_instance->SetModel( pModel );

    return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_finalize( void )
{
	// Scan through and remove degenerate indices...
	if( mp_instance )
	{
		NxXbox::sScene *p_scene = mp_instance->GetScene();
		if( p_scene )
		{
			for( int m = 0; m < p_scene->m_num_mesh_entries; ++m )
			{
				NxXbox::sMesh *p_mesh = p_scene->m_meshes[m];
				p_mesh->Crunch();
			}
		}
	}	
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_set_active( bool active )
{
	if( mp_instance )
	{
		mp_instance->SetActive( active );
	}	
	else
	{
		m_active = active;
		if( m_mesh_array )
		{
			for( uint m = 0; m < m_num_mesh; ++m )
			{
				NxXbox::sMesh *p_mesh = m_mesh_array[m];
				p_mesh->SetActive( active );
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxGeom::plat_is_active( void ) const
{
	if( mp_instance )
	{
		return mp_instance->GetActive();
	}	
	return m_active;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const Mth::Vector &CXboxGeom::plat_get_world_position( void ) const
{
	static Mth::Vector pos;
	
	if( mp_instance )
	{
		pos = mp_instance->GetTransform()->GetPos();
	}
	else
	{
		pos.Set( 0.0f, 0.0f, 0.0f, 1.0f );

		NxXbox::sMesh *p_mesh = m_mesh_array[0];
		if( p_mesh )
		{
			p_mesh->GetPosition( &pos );
		}
	}
	Dbg_Assert( pos[W] == 1.0f );
	return pos;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_set_world_position( const Mth::Vector& pos )
{
	// Ensure w component is correct.
	Mth::Vector proper_pos( pos[X], pos[Y], pos[Z], 1.0f );

	if( mp_instance )
	{
		mp_instance->GetTransform()->SetPos( proper_pos );
	}
	else
	{
		// Go through and adjust the individual meshes.
		for( uint32 i = 0; i < m_num_mesh; ++i )
		{
			NxXbox::sMesh *p_mesh = m_mesh_array[i];
			p_mesh->SetPosition( proper_pos );
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const Mth::CBBox & CXboxGeom::plat_get_bounding_box( void ) const
{
	return m_bbox;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_set_bounding_sphere( const Mth::Vector& boundingSphere )
{
	if( mp_instance )
	{
		NxXbox::sScene *p_scene = mp_instance->GetScene();
		if( p_scene )
		{
			p_scene->m_sphere_center.x	= boundingSphere[X];
			p_scene->m_sphere_center.y	= boundingSphere[Y];
			p_scene->m_sphere_center.z	= boundingSphere[Z];
			p_scene->m_sphere_radius	= boundingSphere[W];
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const Mth::Vector CXboxGeom::plat_get_bounding_sphere( void ) const
{
	if( mp_instance )
	{
		NxXbox::sScene *p_scene = mp_instance->GetScene();
		if( p_scene )
		{
			return Mth::Vector( p_scene->m_sphere_center.x, p_scene->m_sphere_center.y,  p_scene->m_sphere_center.z, p_scene->m_sphere_radius );
		}
	}
	
	return Mth::Vector( 0.0f, 0.0f, 0.0f, 10000.0f );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_set_orientation( const Mth::Matrix& orient )
{
	if( mp_instance )
	{
		Mth::Matrix *p_matrix = mp_instance->GetTransform();
		Mth::Matrix new_orientation = *p_matrix;
		new_orientation[X] = orient[X];
		new_orientation[Y] = orient[Y];
		new_orientation[Z] = orient[Z];
		mp_instance->SetTransform( new_orientation );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const Mth::Matrix &CXboxGeom::plat_get_orientation( void ) const
{
	static Mth::Matrix orientation;
	
	if( mp_instance )
	{
		orientation = *( mp_instance->GetTransform());
		orientation[W] = Mth::Vector( 0.0f, 0.0f, 0.0f, 1.0f );
	}
	else
	{
		orientation.Identity();
	}

	return orientation;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_rotate_y( Mth::ERot90 rot )
{
	if( rot != Mth::ROT_0 )
	{
		if( mp_instance )
		{
			Mth::Matrix *p_matrix = mp_instance->GetTransform();

			// Zero out translation component.
			Mth::Matrix	instance_transform = *p_matrix;
			instance_transform[W] = Mth::Vector(0.0f, 0.0f, 0.0f, 1.0f);

			// Build rotation matrix.
			Mth::Matrix	rot_mat;
			float		rad = (float)((int)rot) * ( Mth::PI * 0.5f );
			Mth::CreateRotateYMatrix( rot_mat, rad );

			// Rotate matrix.
			rot_mat	= instance_transform * rot_mat;

			// Replace translation component.
			rot_mat[W] = p_matrix->GetPos();

			mp_instance->SetTransform( rot_mat );
		}
		else
		{
			// Go through and adjust the individual meshes.
			for( uint32 i = 0; i < m_num_mesh; ++i )
			{
				NxXbox::sMesh *p_mesh = m_mesh_array[i];
				p_mesh->SetYRotation( rot );
			}
		}
	}
}




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxGeom::plat_render(Mth::Matrix* pRootMatrix, Mth::Matrix* pBoneMatrices, int numBones)
{
	if( mp_instance )
	{
		mp_instance->SetTransform( *pRootMatrix );
	}
    return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_set_bone_matrix_data( Mth::Matrix* pBoneMatrices, int numBones )
{
	if( mp_instance )
	{
		Mth::Matrix* p_bone_matrices	= mp_instance->GetBoneTransforms();
		CopyMemory( p_bone_matrices, pBoneMatrices, numBones * sizeof( Mth::Matrix ));
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxGeom::plat_hide_polys( uint32 mask )
{
	if( mp_mesh )
	{
		// Obtain a pointer to the Xbox scene.
		NxXbox::sScene *p_engine_scene = GetInstance()->GetScene();

		// Request the scene to hide the relevant polys.
		p_engine_scene->HidePolys( mask, mp_mesh->GetCASData(), mp_mesh->GetNumCASData());
	}
	
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_set_visibility( uint32 mask )
{
	// Set values
	m_visible = mask;

	if( m_mesh_array )
	{
		for( uint m = 0; m < m_num_mesh; ++m )
		{
			NxXbox::sMesh *p_mesh = m_mesh_array[m];
			p_mesh->SetVisibility((uint8)mask );
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint32 CXboxGeom::plat_get_visibility( void ) const
{
	return m_visible | 0xFFFFFF00;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_set_color( Image::RGBA rgba )
{
	// If we are setting the color (which is used as a multiplier) to (0x80,0x80,0x80), it is effectively
	// just turning the coloring off...
	// Note that some indoor stuff is set to 0x808081, since on Ps2 it needs to be set to something
	// other than 0x808080 in order to ensure that it doesn't inherit the color from the parent node.
	// This is not an issue for Xbox, so it treats the two values the same.
	if((( rgba.r == 0x80 ) || ( rgba.r == 0x81 )) && ( rgba.g == 0x80 ) && ( rgba.b == 0x80 ))
	{
		plat_clear_color();
		return;
	}

	// Oofa, nasty hack.
	if( mp_instance )
	{
		// Grab the engine scene from the geom, and set all meshes to the color.
		NxXbox::sScene *p_scene = mp_instance->GetScene();
		for( int i = 0; i < p_scene->m_num_mesh_entries; ++i )
		{
			NxXbox::sMesh *p_mesh = p_scene->m_meshes[i];
			p_mesh->m_flags |= NxXbox::sMesh::MESH_FLAG_MATERIAL_COLOR_OVERRIDE;
			p_mesh->m_material_color_override[0] = (float)rgba.r / 255.0f;
			p_mesh->m_material_color_override[1] = (float)rgba.g / 255.0f;
			p_mesh->m_material_color_override[2] = (float)rgba.b / 255.0f;
		}
	}
	else if( m_mesh_array != NULL )
	{
		for( uint32 i = 0; i < m_num_mesh; ++i )
		{
			NxXbox::sMesh *p_mesh = m_mesh_array[i];
			p_mesh->m_flags |= NxXbox::sMesh::MESH_FLAG_MATERIAL_COLOR_OVERRIDE;
			p_mesh->m_material_color_override[0] = (float)rgba.r / 255.0f;
			p_mesh->m_material_color_override[1] = (float)rgba.g / 255.0f;
			p_mesh->m_material_color_override[2] = (float)rgba.b / 255.0f;
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_clear_color( void )
{
	// Oofa, nasty hack.
	if( mp_instance )
	{
		// Grab the engine scene from the geom, and clear all meshes of the flag.
		NxXbox::sScene *p_scene = mp_instance->GetScene();
		for( int i = 0; i < p_scene->m_num_mesh_entries; ++i )
		{
			NxXbox::sMesh *p_mesh = p_scene->m_meshes[i];
			p_mesh->m_flags &= ~NxXbox::sMesh::MESH_FLAG_MATERIAL_COLOR_OVERRIDE;
		}
	}
	else if( m_mesh_array != NULL )
	{
		for( uint32 i = 0; i < m_num_mesh; ++i )
		{
			NxXbox::sMesh *p_mesh = m_mesh_array[i];
			p_mesh->m_flags &= ~NxXbox::sMesh::MESH_FLAG_MATERIAL_COLOR_OVERRIDE;
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxGeom::plat_set_material_color( uint32 mat_name_checksum, int pass, Image::RGBA rgba )
{
	bool something_changed = false;

	if( mp_instance && mp_instance->GetScene())
	{
		for( int m = 0; m < mp_instance->GetScene()->m_num_mesh_entries; ++m )
		{
			NxXbox::sMesh		*p_mesh	= mp_instance->GetScene()->m_meshes[m];
			NxXbox::sMaterial	*p_mat	= p_mesh->mp_material;
			if( p_mat )
			{
				bool	want_this_material	= false;
				int		adjusted_pass		= pass;

				// We are searching for materials with a matching name. However there is a caveat in that the
				// conversion process sometimes creates new materials for geometry flagged as 'render as separate', or for
				// geometry which is mapped with only certain passes of a multipass material (or both cases).
				// In such a case, the new material name checksum will differ from the original material name checksum,
				// with the following bits having significance (i.e. consider bitflags = new_namechecksum - old_namechecksum ):
				//
				// Bits 3->6	Pass flags indicating which passes of the original material this material uses
				// Bits 0->2	Absolute number ('render as separate' flagged geometry) indicating which single pass of the material this material represents.
				if( p_mat->m_name_checksum == mat_name_checksum )
					want_this_material = true;
				else if(( p_mat->m_name_checksum > mat_name_checksum ) && (( p_mat->m_name_checksum - mat_name_checksum ) <= 0x7F ))
				{
					uint32 checksum_diff		= p_mat->m_name_checksum - mat_name_checksum;
					uint32 render_separate_pass	= checksum_diff & 0x07;
					uint32 pass_flags			= checksum_diff >> 3;

					if( render_separate_pass )
					{
						if( render_separate_pass == ( pass + 1 ))
						{
							want_this_material = true;

							// Readjust the pass to zero, since this material was formed as a single pass of a multipass material.
							adjusted_pass = 0;
						}
					}
					else if( pass_flags )
					{
						// This material was created during scene conversion from another material with more passes.
						if( pass_flags & ( 1 << pass ))
						{
							want_this_material = true;
							for( int p = 0; p < pass; ++p )
							{
								if(( pass_flags & ( 1 << p )) == 0 )
								{
									// Readjust the pass down by 1, since this material was created as a subset of another material.
									--adjusted_pass;
								}
							}
						}
					}
				}

				if( want_this_material )
				{
					if((uint32)adjusted_pass < p_mat->m_passes )
					{
						if( !( p_mat->m_flags[adjusted_pass] & MATFLAG_PASS_COLOR_LOCKED ))
						{
							p_mat->m_color[adjusted_pass][0] = (float)rgba.r / 255.0f;
							p_mat->m_color[adjusted_pass][1] = (float)rgba.g / 255.0f;
							p_mat->m_color[adjusted_pass][2] = (float)rgba.b / 255.0f;

							something_changed = true;
						}
					}
				}
			}
		}
	}
	return something_changed;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Image::RGBA	CXboxGeom::plat_get_color( void ) const
{
	Image::RGBA rgba( 0, 0, 0, 0 );

	NxXbox::sMesh *p_mesh = NULL;

	if( mp_instance )
	{
		// Grab the engine scene from the geom, and get a mesh color.
		NxXbox::sScene *p_scene = mp_instance->GetScene();

		if( p_scene->m_num_mesh_entries > 0 )
			p_mesh = p_scene->m_meshes[0];
	}
	else if( m_mesh_array != NULL )
	{
		p_mesh = m_mesh_array[0];
	}

	if( p_mesh && ( p_mesh->m_flags & NxXbox::sMesh::MESH_FLAG_MATERIAL_COLOR_OVERRIDE ))
	{
		rgba.r = (uint8)( p_mesh->m_material_color_override[0] * 255.0f );
		rgba.g = (uint8)( p_mesh->m_material_color_override[1] * 255.0f );
		rgba.b = (uint8)( p_mesh->m_material_color_override[2] * 255.0f );
	}
	
	return rgba;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int CXboxGeom::plat_get_num_render_verts( void )
{
	Dbg_MsgAssert( m_mesh_array, ( "Invalid for instanced sectors" ));
	
	int total_verts = 0;

	if( m_mesh_array )
	{
		for( uint32 m = 0; m < m_num_mesh; ++m )
		{
			NxXbox::sMesh *p_mesh = m_mesh_array[m];
			total_verts += p_mesh->m_num_vertices;
		}
	}
	return total_verts;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_get_render_verts( Mth::Vector *p_verts )
{
	Dbg_MsgAssert( m_mesh_array, ( "Invalid for instanced sectors" ));

	if( m_mesh_array )
	{
		for( uint32 m = 0; m < m_num_mesh; ++m )
		{
			NxXbox::sMesh *p_mesh = m_mesh_array[m];

			// Obtain a read-only lock on the mesh data.
			D3DVECTOR *p_pos;
			p_mesh->mp_vertex_buffer[0]->Lock( 0, 0, (BYTE**)&p_pos, D3DLOCK_READONLY | D3DLOCK_NOFLUSH );

			// Copy over every vertex position in this mesh.
			for( uint32 v = 0; v < p_mesh->m_num_vertices; ++v )
			{
				p_verts->Set( p_pos->x, p_pos->y, p_pos->z );

				++p_verts;
				p_pos = (D3DVECTOR*)((BYTE*)p_pos + p_mesh->m_vertex_stride );
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_get_render_colors( Image::RGBA *p_colors )
{
	Dbg_MsgAssert( m_mesh_array, ( "Invalid for instanced sectors" ));

	if( m_mesh_array )
	{
		for( uint32 m = 0; m < m_num_mesh; ++m )
		{
			NxXbox::sMesh *p_mesh = m_mesh_array[m];

			// Obtain a read-only lock on the mesh data.
			Image::RGBA *p_col;
			p_mesh->mp_vertex_buffer[0]->Lock( 0, 0, (BYTE**)&p_col, D3DLOCK_READONLY | D3DLOCK_NOFLUSH );
			p_col = (Image::RGBA*)((BYTE*)p_col + p_mesh->m_diffuse_offset );

			// Copy over every vertex color in this mesh, swapping red and blue.
			for( uint32 v = 0; v < p_mesh->m_num_vertices; ++v )
			{
				p_colors->r = p_col->b;
				p_colors->g = p_col->g;
				p_colors->b = p_col->r;
				p_colors->a = p_col->a;

				++p_colors;
				p_col = (Image::RGBA*)((BYTE*)p_col + p_mesh->m_vertex_stride );
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_set_render_verts( Mth::Vector *p_verts )
{
	Dbg_MsgAssert( m_mesh_array, ( "Invalid for instanced sectors" ));

	if( m_mesh_array )
	{
		for( uint32 m = 0; m < m_num_mesh; ++m )
		{
			NxXbox::sMesh *p_mesh = m_mesh_array[m];

			// Obtain a writeable lock on the mesh data.
			D3DVECTOR *p_pos;
			p_mesh->mp_vertex_buffer[0]->Lock( 0, 0, (BYTE**)&p_pos, D3DLOCK_NOFLUSH );

			// Will need to store the min and max points in order to calculate the new bounding sphere for the mesh.
			Mth::CBBox bbox;

			// Copy over every vertex position in this mesh.
			for( uint32 v = 0; v < p_mesh->m_num_vertices; ++v )
			{
				p_pos->x = p_verts->GetX();
				p_pos->y = p_verts->GetY();
				p_pos->z = p_verts->GetZ();

				// Add this point to the bounding box.
				bbox.AddPoint( *p_verts );

				++p_verts;
				p_pos = (D3DVECTOR*)((BYTE*)p_pos + p_mesh->m_vertex_stride );
			}

			// Now refigure the bounding sphere.
			Mth::Vector sphere_center	= bbox.GetMin() + ( 0.5f * ( bbox.GetMax() - bbox.GetMin()));
			p_mesh->m_sphere_center.x	= sphere_center[X];
			p_mesh->m_sphere_center.y	= sphere_center[Y];
			p_mesh->m_sphere_center.z	= sphere_center[Z];
			p_mesh->m_sphere_radius		= ( bbox.GetMax() - sphere_center ).Length();
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_set_render_colors( Image::RGBA *p_colors )
{
	Dbg_MsgAssert( m_mesh_array, ( "Invalid for instanced sectors" ));

	if( m_mesh_array )
	{
		for( uint32 m = 0; m < m_num_mesh; ++m )
		{
			NxXbox::sMesh*	p_mesh			= m_mesh_array[m];
			Image::RGBA*	p_colors_save	= p_colors;

			// The mesh may contain more than one vertex set, usually in the case of vertex wibbling.
			for( uint32 v = 0; v < p_mesh->m_num_vertex_buffers; ++v )
			{
				p_colors = p_colors_save;

				// Obtain a writeable lock on the mesh data.
				Image::RGBA *p_col;
				p_mesh->mp_vertex_buffer[v]->Lock( 0, 0, (BYTE**)&p_col, D3DLOCK_NOFLUSH );
				p_col = (Image::RGBA*)((BYTE*)p_col + p_mesh->m_diffuse_offset );

				// Copy over every vertex color in this mesh, swapping red and blue.
				for( uint32 v = 0; v < p_mesh->m_num_vertices; ++v )
				{
					p_col->b = p_colors->r;
					p_col->g = p_colors->g;
					p_col->r = p_colors->b;
					p_col->a = p_colors->a;

					++p_colors;
					p_col = (Image::RGBA*)((BYTE*)p_col + p_mesh->m_vertex_stride );
				}
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_set_scale( const Mth::Vector & scale )
{
	Dbg_MsgAssert( m_mesh_array, ( "Invalid for instanced sectors" ));

	m_scale = scale;

	if( m_mesh_array )
	{
		for( uint32 m = 0; m < m_num_mesh; ++m )
		{
			NxXbox::sMesh *p_mesh = m_mesh_array[m];

			Mth::Vector current_pos( 0.0f, 0.0f, 0.0f, 1.0f );
			p_mesh->GetPosition( &current_pos );
			
			// Obtain a writeable lock on the mesh data.
			D3DVECTOR *p_pos;
			p_mesh->mp_vertex_buffer[0]->Lock( 0, 0, (BYTE**)&p_pos, D3DLOCK_NOFLUSH );

			// Scale over every vertex position in this mesh.
			for( uint32 v = 0; v < p_mesh->m_num_vertices; ++v )
			{
				p_pos->x	= (( p_pos->x - current_pos[X] ) * scale[X] ) + current_pos[X];
				p_pos->y	= (( p_pos->y - current_pos[Y] ) * scale[Y] ) + current_pos[Y];
				p_pos->z	= (( p_pos->z - current_pos[Z] ) * scale[Z] ) + current_pos[Z];

				p_pos		= (D3DVECTOR*)((BYTE*)p_pos + p_mesh->m_vertex_stride );
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Mth::Vector CXboxGeom::plat_get_scale() const
{
	return m_scale;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_set_uv_wibble_params( float u_vel, float u_amp, float u_freq, float u_phase,
										   float v_vel, float v_amp, float v_freq, float v_phase )
{
	if( m_mesh_array )
	{
		for( uint32 m = 0; m < m_num_mesh; ++m )
		{
			NxXbox::sMesh		*p_mesh	= m_mesh_array[m];
			NxXbox::sMaterial	*p_mat	= p_mesh->mp_material;
			if( p_mat )
			{
				// Find the first pass that wibbles.
				for( uint32 p = 0; p < p_mat->m_passes; ++p )
				{
					if( p_mat->mp_UVWibbleParams[p] )
					{
						p_mat->mp_UVWibbleParams[p]->m_UVel			= u_vel;
						p_mat->mp_UVWibbleParams[p]->m_VVel			= v_vel;
						p_mat->mp_UVWibbleParams[p]->m_UAmplitude	= u_amp;
						p_mat->mp_UVWibbleParams[p]->m_VAmplitude	= v_amp;
						p_mat->mp_UVWibbleParams[p]->m_UFrequency	= u_freq;
						p_mat->mp_UVWibbleParams[p]->m_VFrequency	= v_freq;
						p_mat->mp_UVWibbleParams[p]->m_UPhase		= u_phase;
						p_mat->mp_UVWibbleParams[p]->m_VPhase		= v_phase;
						break;
					}
				}
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_use_explicit_uv_wibble( bool yes )
{
	if( mp_instance && mp_instance->GetScene())
	{
		for( int m = 0; m < mp_instance->GetScene()->m_num_mesh_entries; ++m )
		{
			NxXbox::sMesh		*p_mesh	= mp_instance->GetScene()->m_meshes[m];
			NxXbox::sMaterial	*p_mat	= p_mesh->mp_material;
			if( p_mat )
			{
				for( uint32 p = 0; p < p_mat->m_passes; ++p )
				{
					if( p_mat->mp_UVWibbleParams[p] )
					{
						p_mat->m_flags[p] |= MATFLAG_EXPLICIT_UV_WIBBLE;
					}
				}
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_set_uv_wibble_offsets( float u_offset, float v_offset )
{
	if( m_mesh_array )
	{
		for( uint32 m = 0; m < m_num_mesh; ++m )
		{
			NxXbox::sMesh		*p_mesh	= m_mesh_array[m];
			NxXbox::sMaterial	*p_mat	= p_mesh->mp_material;
			if( p_mat )
			{
				// Find the first pass that wibbles.
				for( uint32 p = 0; p < p_mat->m_passes; ++p )
				{
					if( p_mat->mp_UVWibbleParams[p] )
					{
						p_mat->mp_UVWibbleParams[p]->m_UVMatrix[2] = u_offset;
						p_mat->mp_UVWibbleParams[p]->m_UVMatrix[3] = v_offset;
						plat_use_explicit_uv_wibble( true );
						break;
					}
				}
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxGeom::plat_set_uv_wibble_offsets( uint32 mat_name_checksum, int pass, float u_offset, float v_offset )
{
	if( m_mesh_array )
	{
		for( uint32 m = 0; m < m_num_mesh; ++m )
		{
			NxXbox::sMesh		*p_mesh	= m_mesh_array[m];
			NxXbox::sMaterial	*p_mat	= p_mesh->mp_material;
			if( p_mat && ( p_mat->m_name_checksum == mat_name_checksum ))
			{
				// Find the first pass that wibbles.
//				for( uint32 p = 0; p < p_mat->m_passes; ++p )
//				{
//					if( p_mat->mp_UVWibbleParams[p] )
//					{
//						p_mat->mp_UVWibbleParams[p]->m_UWibbleOffset = u_offset;
//						p_mat->mp_UVWibbleParams[p]->m_VWibbleOffset = v_offset;
//						plat_use_explicit_uv_wibble( true );
//						break;
//					}
//				}
			}
		}
	}
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxGeom::plat_set_uv_matrix( uint32 mat_name_checksum, int pass, const Mth::Matrix& mat )
{
	if( mp_instance && mp_instance->GetScene())
	{
		for( int m = 0; m < mp_instance->GetScene()->m_num_mesh_entries; ++m )
		{
			NxXbox::sMesh		*p_mesh	= mp_instance->GetScene()->m_meshes[m];
			NxXbox::sMaterial	*p_mat	= p_mesh->mp_material;
			if( p_mat )
			{
				bool	want_this_material	= false;
				int		adjusted_pass		= pass;

				// We are searching for materials with a matching name. However there is a caveat in that the
				// conversion process sometimes creates new materials for geometry flagged as 'render as separate', or for
				// geometry which is mapped with only certain passes of a multipass material (or both cases).
				// In such a case, the new material name checksum will differ from the original material name checksum,
				// with the following bits having significance (i.e. consider bitflags = new_namechecksum - old_namechecksum ):
				//
				// Bits 3->6	Pass flags indicating which passes of the original material this material uses
				// Bits 0->2	Absolute number ('render as separate' flagged geometry) indicating which single pass of the material this material represents.
				if( p_mat->m_name_checksum == mat_name_checksum )
				{
					want_this_material = true;
				}
				else if(( p_mat->m_name_checksum > mat_name_checksum ) && (( p_mat->m_name_checksum - mat_name_checksum ) <= 0x7F ))
				{
					uint32 checksum_diff		= p_mat->m_name_checksum - mat_name_checksum;
					uint32 render_separate_pass	= checksum_diff & 0x07;
					uint32 pass_flags			= checksum_diff >> 3;

					if( render_separate_pass )
					{
						if( render_separate_pass == ( pass + 1 ))
						{
							want_this_material = true;

							// Readjust the pass to zero, since this material was formed as a single pass of a multipass material.
							adjusted_pass = 0;
						}
					}
					else if( pass_flags )
					{
						// This material was created during scene conversion from another material with more passes.
						if( pass_flags & ( 1 << pass ))
						{
							want_this_material = true;
							for( int p = 0; p < pass; ++p )
							{
								if(( pass_flags & ( 1 << p )) == 0 )
								{
									// Readjust the pass down by 1, since this material was created as a subset of another material.
									--adjusted_pass;
								}
							}
						}
					}
				}

				if( want_this_material )
				{
					if((uint32)adjusted_pass < p_mat->m_passes )
					{
						// Create the wibble params if they don't exist already.
						if( p_mat->mp_UVWibbleParams[adjusted_pass] == NULL )
						{
							p_mat->mp_UVWibbleParams[adjusted_pass]	= new NxXbox::sUVWibbleParams;

							// Need to set flags to indicate that uv wibble is now in effect.
							p_mat->m_uv_wibble				= true;
							p_mat->m_flags[adjusted_pass]  |= MATFLAG_UV_WIBBLE | MATFLAG_EXPLICIT_UV_WIBBLE;
						}

						// Also need to switch vertex shaders if the current vertex shader does not support UV Transforms.
						if( p_mesh->m_vertex_shader[0] == WeightedMeshVS_VXC_1Weight )
						{
							p_mesh->m_vertex_shader[0] = WeightedMeshVS_VXC_1Weight_UVTransform;
						}
						else if( p_mesh->m_vertex_shader[0] == WeightedMeshVS_VXC_2Weight )
						{
							p_mesh->m_vertex_shader[0] = WeightedMeshVS_VXC_2Weight_UVTransform;
						}
						else if( p_mesh->m_vertex_shader[0] == WeightedMeshVS_VXC_3Weight )
						{
							p_mesh->m_vertex_shader[0] = WeightedMeshVS_VXC_3Weight_UVTransform;
						}

						// Set the matrix values.
						p_mat->mp_UVWibbleParams[adjusted_pass]->m_UVMatrix[0]	= mat[0][0];
						p_mat->mp_UVWibbleParams[adjusted_pass]->m_UVMatrix[1]	= mat[0][1];
						p_mat->mp_UVWibbleParams[adjusted_pass]->m_UVMatrix[2]	= mat[3][0];
						p_mat->mp_UVWibbleParams[adjusted_pass]->m_UVMatrix[3]	= mat[3][1];
					}
				}
			}
		}
	}
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxGeom::plat_allocate_uv_matrix_params( uint32 mat_name_checksum, int pass )
{
	if( mp_instance && mp_instance->GetScene())
	{
		bool changed_something = false;

		for( int m = 0; m < mp_instance->GetScene()->m_num_mesh_entries; ++m )
		{
			NxXbox::sMesh		*p_mesh	= mp_instance->GetScene()->m_meshes[m];
			NxXbox::sMaterial	*p_mat	= p_mesh->mp_material;
			if( p_mat )
			{
				bool	want_this_material	= false;
				int		adjusted_pass		= pass;

				// We are searching for materials with a matching name. However there is a caveat in that the
				// conversion process sometimes creates new materials for geometry flagged as 'render as separate', or for
				// geometry which is mapped with only certain passes of a multipass material (or both cases).
				// In such a case, the new material name checksum will differ from the original material name checksum,
				// with the following bits having significance (i.e. consider bitflags = new_namechecksum - old_namechecksum ):
				//
				// Bits 3->6	Pass flags indicating which passes of the original material this material uses
				// Bits 0->2	Absolute number ('render as separate' flagged geometry) indicating which single pass of the material this material represents.
				if( p_mat->m_name_checksum == mat_name_checksum )
				{
					want_this_material = true;
				}
				else if(( p_mat->m_name_checksum > mat_name_checksum ) && (( p_mat->m_name_checksum - mat_name_checksum ) <= 0x7F ))
				{
					uint32 checksum_diff		= p_mat->m_name_checksum - mat_name_checksum;
					uint32 render_separate_pass	= checksum_diff & 0x07;
					uint32 pass_flags			= checksum_diff >> 3;

					if( render_separate_pass )
					{
						if( render_separate_pass == ( pass + 1 ))
						{
							want_this_material = true;

							// Readjust the pass to zero, since this material was formed as a single pass of a multipass material.
							adjusted_pass = 0;
						}
					}
					else if( pass_flags )
					{
						// This material was created during scene conversion from another material with more passes.
						if( pass_flags & ( 1 << pass ))
						{
							want_this_material = true;
							for( int p = 0; p < pass; ++p )
							{
								if(( pass_flags & ( 1 << p )) == 0 )
								{
									// Readjust the pass down by 1, since this material was created as a subset of another material.
									--adjusted_pass;
								}
							}
						}
					}
				}

				if( want_this_material )
				{
					if((uint32)adjusted_pass < p_mat->m_passes )
					{
						// Create the wibble params if they don't exist already.
						if( p_mat->mp_UVWibbleParams[adjusted_pass] == NULL )
						{
							p_mat->mp_UVWibbleParams[adjusted_pass]	= new NxXbox::sUVWibbleParams;

							// Need to set flags to indicate that uv wibble is now in effect.
							p_mat->m_uv_wibble						= true;
							p_mat->m_flags[adjusted_pass]		   |= MATFLAG_UV_WIBBLE | MATFLAG_EXPLICIT_UV_WIBBLE;

							changed_something = true;
						}
					}
				}
			}
		}
		return changed_something;
	}
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxGeom::plat_set_model_lights( CModelLights* p_model_lights )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxGeom *CXboxGeom::plat_clone( bool instance, CScene *p_dest_scene )
{
	CXboxGeom *p_clone = NULL;
	
	// This is a CXboxGeom 'hanging' from a sector. Create a new CXboxGeom which will store the CInstance.
	p_clone = new CXboxGeom();
	
	// Create new meshes for the clone.
	p_clone->m_mesh_array	= new NxXbox::sMesh*[m_num_mesh];
	p_clone->m_num_mesh		= m_num_mesh;
	for( uint32 m = 0; m < p_clone->m_num_mesh; ++m )
	{
		p_clone->m_mesh_array[m] = m_mesh_array[m]->Clone( instance );
	}
	
	if( instance == false )
	{
		p_clone->SetActive( true );

		// In this situation, we need to add the individual meshes to the scene.
		// Grab a temporary workspace buffer.
		Nx::CXboxScene *p_xbox_scene						= static_cast<CXboxScene*>( p_dest_scene );
		NxXbox::sScene *p_scene								= p_xbox_scene->GetEngineScene();
		NxXbox::sMesh **p_temp_mesh_buffer					= ( p_scene->m_num_mesh_entries > 0 ) ? new NxXbox::sMesh*[p_scene->m_num_mesh_entries] : NULL;

		// Set the scene pointer for the clone.
		p_clone->SetScene( p_xbox_scene );
		
		// Copy meshes over into the temporary workspace buffer.
		for( int i = 0; i < p_scene->m_num_mesh_entries; ++i )
		{
			p_temp_mesh_buffer[i]	= p_scene->m_meshes[i];
		}

		// Store how many meshes were present.
		int old_num_mesh_entries	= p_scene->m_num_mesh_entries;
		
		// Delete current mesh array.
		delete [] p_scene->m_meshes;

		// Important to set this to NULL.
		p_scene->m_meshes			= NULL;
		
		// Include new meshes in count.
		p_scene->CountMeshes( m_num_mesh, m_mesh_array );

		// Allocate new mesh arrays.
		p_scene->CreateMeshArrays();

		// Copy old mesh data back in.
		for( int i = 0; i < old_num_mesh_entries; ++i )
		{
			p_scene->m_meshes[i] = p_temp_mesh_buffer[i];
		}

		// Remove temporary arrays.
		delete [] p_temp_mesh_buffer;
		
		// Add new meshes.
		p_scene->AddMeshes( p_clone->m_num_mesh, p_clone->m_mesh_array );

		// Sort the meshes.
		p_scene->SortMeshes();
	}
	else
	{
		// Create a new scene which will be attached via the instance.
		p_clone->m_bbox			= m_bbox;
		NxXbox::sScene *p_scene = p_clone->GenerateScene();

		p_clone->SetActive( true );
	
		// Create the instance.
		Mth::Matrix temp;
		temp.Identity();
		NxXbox::CInstance *p_instance = new NxXbox::CInstance( p_scene, temp, 1, NULL );
		
		// This instance will be the only object maintaining a reference to the attached scene, so we want to delete
		// the scene when the instance gets removed.
		p_instance->SetFlag( NxXbox::CInstance::INSTANCE_FLAG_DELETE_ATTACHED_SCENE );

		// Hook the clone up to the instance.		
		p_clone->SetInstance( p_instance );
	}
	return p_clone;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxGeom* CXboxGeom::plat_clone( bool instance, CModel* p_dest_model )
{
	CXboxGeom* p_clone = NULL;

	Nx::CXboxScene* p_xbox_scene = new CXboxScene;
	p_xbox_scene->SetEngineScene( mp_instance->GetScene() );

	p_clone				= new CXboxGeom();
	p_clone->mp_scene	= p_xbox_scene;

	int num_mesh		= mp_instance->GetScene()->m_num_mesh_entries;

	// Create new meshes for the clone.
	p_clone->m_mesh_array	= num_mesh ? new NxXbox::sMesh*[num_mesh] : NULL;
	p_clone->m_num_mesh		= num_mesh;
	for( uint32 m = 0; m < p_clone->m_num_mesh; ++m )
	{
		p_clone->m_mesh_array[m] = mp_instance->GetScene()->m_meshes[m]->Clone( instance );
	}

	// Create a new scene which will be attached via the instance.
	p_clone->m_bbox			= m_bbox;
	NxXbox::sScene* p_scene = p_clone->GenerateScene();

	// Kill the temp scene.
	p_xbox_scene->SetEngineScene( NULL );
	delete p_xbox_scene;
	p_clone->mp_scene		= NULL;

	p_clone->SetActive( true );

	// Create the new bone array.
	int				num_bones	= p_dest_model->GetNumBones();
	Mth::Matrix*	p_bones		= new Mth::Matrix[num_bones];
	for( int b = 0; b < num_bones; ++b )
	{
		p_bones[b].Identity();
	}

	// Create the instance.
	Mth::Matrix temp;
	temp.Identity();
	NxXbox::CInstance* p_instance	= new NxXbox::CInstance( p_scene, temp, num_bones, p_bones );
	
	Nx::CXboxModel* p_xbox_model	= static_cast<CXboxModel*>( p_dest_model );
	((CXboxModel*)p_dest_model )->SetInstance( p_instance );

	// This instance will be the only object maintaining a reference to the attached scene, so we want to delete
	// the scene when the instance gets removed.
	p_instance->SetFlag( NxXbox::CInstance::INSTANCE_FLAG_DELETE_ATTACHED_SCENE );

	// Hook the clone up to the instance.		
	p_clone->SetInstance( p_instance );

	return p_clone;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // Nx
				
