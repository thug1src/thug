//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxGeom.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  3/4/2002
//****************************************************************************

#include <gfx/Ngc/p_nxgeom.h>

#include <core/math.h>

#include <gfx/skeleton.h>
#include <gfx/Ngc/p_nxmesh.h>
#include <gfx/Ngc/p_nxmodel.h>
#include <gfx/Ngc/p_nxtexture.h>
#include <gfx/Ngc/nx/instance.h>
#include <gel/music/Ngc/p_music.h>

#include <gel/collision/collision.h>

namespace Nx
{

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/
						
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
	
	

/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNgcGeom::CNgcGeom() : mp_init_mesh_list( NULL ), m_mesh_array( NULL ), m_visible( 0x55 )
{
	mp_instance = NULL;
//	mp_sector	= NULL;
	m_active	= true;
	m_mod.offset[X] = 0.0f;
	m_mod.offset[Y] = 0.0f;
	m_mod.offset[Z] = 0.0f;
	m_mod.offset[W] = 0.0f;
	m_mod.scale[X] = 1.0f;
	m_mod.scale[Y] = 1.0f;
	m_mod.scale[Z] = 1.0f;
	m_mod.scale[W] = 1.0f;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNgcGeom::~CNgcGeom( void )
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
void CNgcGeom::InitMeshList( void )
{
	Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().TopDownHeap());
	mp_init_mesh_list = new Lst::Head< NxNgc::sMesh >;
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcGeom::AddMesh( NxNgc::sMesh *mesh )
{
	Dbg_Assert( mp_init_mesh_list );

	Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().TopDownHeap());
	Lst::Node< NxNgc::sMesh > *node = new Lst::Node< NxNgc::sMesh > (mesh);
	mp_init_mesh_list->AddToTail( node );
	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcGeom::CreateMeshArray( void )
{
	if( !m_mesh_array )
	{
		Dbg_Assert( mp_init_mesh_list );

		m_num_mesh = mp_init_mesh_list->CountItems();
		if (m_num_mesh)
		{
			Lst::Node< NxNgc::sMesh > *mesh, *next;

			m_mesh_array = new NxNgc::sMesh*[m_num_mesh];
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
Lst::Head< NxNgc::sMesh >	*CNgcGeom::GetMeshList( void )
{
	return mp_init_mesh_list;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcGeom::RegisterMeshArray( bool just_count )
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
void CNgcGeom::DestroyMeshArray( void )
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
			NxNgc::sMesh *p_mesh = m_mesh_array[m];

			// Delete pools.
			if ( p_mesh->m_flags & NxNgc::sMesh::MESH_FLAG_CLONED_POS )
			{
				delete p_mesh->mp_dl->mp_pos_pool;
			}

			if ( p_mesh->m_flags & NxNgc::sMesh::MESH_FLAG_CLONED_COL )
			{
				delete p_mesh->mp_dl->mp_col_pool;
			}

			// Delete copied dl if it's there.
			if ( p_mesh->m_flags & NxNgc::sMesh::MESH_FLAG_CLONED_DL )
			{
				delete p_mesh->mp_dl;
			}

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
NxNgc::sScene* CNgcGeom::GenerateScene( void )
{
	NxNgc::sScene *p_scene = new NxNgc::sScene();

	memcpy( p_scene, mp_scene->GetEngineScene(), sizeof( NxNgc::sScene ) );

	p_scene->m_flags				= SCENE_FLAG_CLONED_GEOM;
	p_scene->m_is_dictionary		= false;

	p_scene->m_num_meshes			= 0;		// No meshes as yet.
	p_scene->m_num_filled_meshes	= 0;
	p_scene->mpp_mesh_list			= NULL;

	p_scene->mp_hierarchyObjects	= NULL;
	p_scene->m_numHierarchyObjects	= 0;

	p_scene->CountMeshes( m_num_mesh, m_mesh_array );
	p_scene->CreateMeshArrays();
	p_scene->AddMeshes( m_num_mesh, m_mesh_array );
	p_scene->SortMeshes();



	// Set the bounding box and bounding sphere.
//	p_scene->m_bbox = m_bbox;
//	p_scene->FigureBoundingVolumes();
	
	return p_scene;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CNgcGeom::plat_load_geom_data(CMesh* pMesh, CModel* pModel, bool color_per_material)
{
//	// The skeleton must exist by this point (unless it's a hacked up car).
//	int numBones;
//	numBones = pModel->GetSkeleton() ? pModel->GetSkeleton()->GetNumBones() : 1;
//	
//	Mth::Matrix temp;
//	CNgcMesh *p_Ngc_mesh = static_cast<CNgcMesh*>( pMesh );
//	mp_instance = new NxNgc::CInstance( p_Ngc_mesh->GetScene()->GetEngineScene(), temp, numBones, pModel->GetBoneTransforms() );
//
//    return true;
	
	
	// The skeleton must exist by this point (unless it's a hacked up car).
	int numBones;
	numBones = pModel->GetNumBones();
	
	Mth::Matrix temp;
	CNgcMesh *p_Ngc_mesh = static_cast<CNgcMesh*>( pMesh );
	
	// Store a pointer to the CNgcMesh, used for obtaining CAS poly removal data.
	mp_mesh = p_Ngc_mesh;

//	CNgcModel *p_Ngc_model = static_cast<CNgcModel*>( pModel );
	NxNgc::CInstance *p_instance = new NxNgc::CInstance( p_Ngc_mesh->GetScene()->GetEngineScene(), temp, numBones, pModel->GetBoneTransforms());
	SetInstance( p_instance );
	p_instance->SetModel( pModel );

    return true;


//	int numBones;
//	numBones = pModel->GetSkeleton() ? pModel->GetSkeleton()->GetNumBones() : 0;
//
//	NxNgc::Mat temp;
//	NxNgc::MatIdentity(temp);
//	Dbg_Assert( mp_instance == NULL );
//
//	Dbg_Assert( pMesh );
//	Dbg_Assert( pMesh->GetTextureDictionary() );
//	NxNgc::sScene* pScene = ((Nx::CNgcTexDict*)pMesh->GetTextureDictionary())->GetEngineTextureDictionary();
//	Dbg_Assert( pScene );
//
//	if ( numBones )
//	{
//		mp_matrices = ((CNgcModel*)pModel)->GetMatrices();
//	}
//	mp_instance = new NxNgc::CInstance(pScene, temp, numBones, mp_matrices ); 
//
//	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcGeom::plat_set_active( bool active )
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
				NxNgc::sMesh *p_mesh = m_mesh_array[m];
				p_mesh->SetActive( active );
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcGeom::plat_is_active( void ) const
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
void CNgcGeom::plat_set_world_position( const Mth::Vector& pos )
{
//	return;
	// Ensure w component is correct.

	if( mp_instance )
	{
		mp_instance->GetTransform()->SetPos( pos );
	}
	else
	{
		// Go through and adjust the individual meshes.
//		for( uint32 i = 0; i < m_num_mesh; ++i )
		{
//			NxNgc::sMesh *p_mesh = m_mesh_array[i];
//			p_mesh->mp_mod = &m_mod;

			Mth::Vector delta_pos = pos - m_mod.offset;

			m_mod.offset[X] = pos[X];
			m_mod.offset[Y] = pos[Y];
			m_mod.offset[Z] = pos[Z];

			int num = plat_get_num_render_verts();
			Mth::Vector * p_verts = new Mth::Vector[num];
			plat_get_render_verts( p_verts );
			for ( int lp = 0; lp < num; lp++ )
			{
				p_verts[lp] += delta_pos;
			}
			plat_set_render_verts( p_verts );
			delete p_verts;


//			Mth::Vector delta_pos( pos[X] - m_offset[X], pos[Y] - m_offset[Y], pos[Z] - m_offset[Z], 1.0f );
//			p_mesh->mp_dl->m_sphere[X] += delta_pos[X];
//			p_mesh->mp_dl->m_sphere[Y] += delta_pos[Y];
//			p_mesh->mp_dl->m_sphere[Z] += delta_pos[Z];
//			p_mesh->SetPosition( proper_pos );
		}

//		if ( mp_scene->GetEngineScene()->mp_scene_data )
//		{
//			Mth::Vector proper_pos( pos[X] - m_offset[X], pos[Y] - m_offset[Y], pos[Z] - m_offset[Z], 1.0f );
//	
//			float * pos = mp_scene->GetEngineScene()->mp_pos_pool;
//			for ( int lp = 0; lp += mp_scene->GetEngineScene()->mp_scene_data->m_num_pos; lp++ )
//			{
//				pos[0] += proper_pos[X];
//				pos[1] += proper_pos[Y];
//				pos[2] += proper_pos[Z];
//				pos += 3;
//			}
//			m_offset[X] = proper_pos[X];
//			m_offset[Y] = proper_pos[Y];
//			m_offset[Z] = proper_pos[Z];
//		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Vector &	CNgcGeom::plat_get_world_position() const
{
	static Mth::Vector pos;
	pos.Set( m_mod.offset[X], m_mod.offset[Y], m_mod.offset[Z], 1.0f );
	return pos;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::CBBox & CNgcGeom::plat_get_bounding_box( void ) const
{
	Dbg_Assert(mp_coll_tri_data);
	return mp_coll_tri_data->GetBBox();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcGeom::plat_set_bounding_sphere( const Mth::Vector& boundingSphere )
{
	if( mp_instance )
	{
		NxNgc::sScene *p_scene = mp_instance->GetScene();
		if( p_scene )
		{
			if ( mp_instance->GetBoneTransforms() && p_scene->m_numHierarchyObjects )
			{
				// Car or something...
				p_scene->m_sphere = boundingSphere;
			}
			else
			{
				if ( p_scene->mpp_mesh_list[0] )
				{
					p_scene->mpp_mesh_list[0]->mp_dl->mp_object_header->m_sphere = boundingSphere;
				}
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const Mth::Vector CNgcGeom::plat_get_bounding_sphere( void ) const
{
	if( mp_instance )
	{
		NxNgc::sScene *p_scene = mp_instance->GetScene();
		if( p_scene )
		{
			if ( mp_instance->GetBoneTransforms() && p_scene->m_numHierarchyObjects )
			{
				// Car or something...
				return p_scene->m_sphere;
			}
			else
			{
				if ( p_scene->mpp_mesh_list[0] )
				{
					return p_scene->mpp_mesh_list[0]->mp_dl->mp_object_header->m_sphere;
				}
			}
		}
	}
	
	return Mth::Vector( 0.0f, 0.0f, 0.0f, 10000.0f );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcGeom::plat_render(Mth::Matrix* pRootMatrix, Mth::Matrix* pBoneMatrices, int numBones)
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
void CNgcGeom::plat_set_bone_matrix_data( Mth::Matrix* pBoneMatrices, int numBones )
{
	if( mp_instance )
	{
		Mth::Matrix* p_bone_matrices	= mp_instance->GetBoneTransforms();
		memcpy( p_bone_matrices, pBoneMatrices, numBones * sizeof( Mth::Matrix ));
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcGeom::plat_hide_polys( uint32 mask )
{
	if( mp_mesh )
	{
		// Obtain a pointer to the Ngc scene.
		NxNgc::sScene *p_engine_scene = GetInstance()->GetScene();

		// Request the scene to hide the relevant polys.
		p_engine_scene->HidePolys( mask, mp_mesh->GetCASData(), mp_mesh->GetNumCASData());
	}
	
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcGeom::plat_set_visibility( uint32 mask )
{
	// Set values
	m_visible = mask & 0xff;

	if( mp_instance )
	{
		mp_instance->SetVisibility( mask & 0xff );
	}	
//	else
//	{
		if( m_mesh_array )
		{
			for( uint m = 0; m < m_num_mesh; ++m )
			{
				NxNgc::sMesh *p_mesh = m_mesh_array[m];
				p_mesh->SetVisibility( mask & 0xff );
			}
		}
//	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint32 CNgcGeom::plat_get_visibility( void ) const
{
	return m_visible | 0xFFFFFF00;
}






/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcGeom::plat_set_color( Image::RGBA rgba )
{
	if( mp_instance && mp_instance->GetBoneTransforms() )
	{
		if( mp_instance && mp_instance->GetScene())
		{
			for( int m = 0; m < mp_instance->GetScene()->m_num_filled_meshes; ++m )
			{
				NxNgc::sMesh			*p_mesh	= mp_instance->GetScene()->mpp_mesh_list[m];
				NxNgc::sMaterialHeader	*p_mat	= p_mesh->mp_dl->m_material.p_header;
				if( p_mat )
				{
					NxNgc::sMaterialPassHeader * p_pass = &mp_instance->GetScene()->mp_material_pass[p_mat->m_pass_item];
					for ( int pass = 0; pass < p_mat->m_passes; pass++, p_pass++ )
					{
						p_pass->m_color.r = rgba.r;
						p_pass->m_color.g = rgba.g;
						p_pass->m_color.b = rgba.b;

//						NxNgc::sTextureDL* p_dl = &mp_instance->GetScene()->mp_texture_dl[p_mat->m_texture_dl_id];
//						GX::begin( p_dl->mp_dl, p_dl->m_dl_size );
//						multi_mesh( p_mat, p_pass, true, true );
//						p_dl->m_dl_size = GX::end();

//						int size = mp_instance->GetScene()->mp_texture_dl[p_mat->m_texture_dl_id].m_dl_size;
//						void * p_dl = mp_instance->GetScene()->mp_texture_dl[p_mat->m_texture_dl_id].mp_dl;
//						GX::ChangeMaterialColor( (unsigned char *)p_dl, size, p_pass->m_color, pass );

						p_mat->m_flags |= (1<<4);
					}
				}
			}
		}
	}
	else if( m_mesh_array != NULL )
	{
		for( uint32 i = 0; i < m_num_mesh; ++i )
		{
			NxNgc::sMesh *p_mesh = m_mesh_array[i];
			p_mesh->m_base_color.r = rgba.r;
			p_mesh->m_base_color.g = rgba.g;
			p_mesh->m_base_color.b = rgba.b;
			p_mesh->m_base_color.a = rgba.a;
		}
	}

//	// Oofa, nasty hack.
//	if( mp_instance )
//	{
//		// Grab the engine scene from the geom, and set all meshes to the color.
//		NxNgc::sScene *p_scene = mp_instance->GetScene();
//		for( int i = 0; i < p_scene->m_num_opaque_entries; ++i )
//		{
//			NxNgc::sMesh *p_mesh = p_scene->m_opaque_meshes[i];
//			p_mesh->m_flags |= NxNgc::sMesh::MESH_FLAG_MATERIAL_COLOR_OVERRIDE;
//			p_mesh->m_material_color_override.r = rgba.r;
//			p_mesh->m_material_color_override.g = rgba.g;
//			p_mesh->m_material_color_override.b = rgba.b;
//			p_mesh->m_material_color_override.a = rgba.a;
//		}
//		for( int i = 0; i < p_scene->m_num_semitransparent_entries; ++i )
//		{
//			NxNgc::sMesh *p_mesh = p_scene->m_semitransparent_meshes[i];
//			p_mesh->m_flags |= NxNgc::sMesh::MESH_FLAG_MATERIAL_COLOR_OVERRIDE;
//			p_mesh->m_material_color_override.r = rgba.r;
//			p_mesh->m_material_color_override.g = rgba.g;
//			p_mesh->m_material_color_override.b = rgba.b;
//			p_mesh->m_material_color_override.a = rgba.a;
//		}
//	}
//	else if( m_mesh_array != NULL )
//	{
//		for( uint32 i = 0; i < m_num_mesh; ++i )
//		{
//			NxNgc::sMesh *p_mesh = m_mesh_array[i];
//			p_mesh->m_flags |= NxNgc::sMesh::MESH_FLAG_MATERIAL_COLOR_OVERRIDE;
//			p_mesh->m_material_color_override.r = rgba.r;
//			p_mesh->m_material_color_override.g = rgba.g;
//			p_mesh->m_material_color_override.b = rgba.b;
//			p_mesh->m_material_color_override.a = rgba.a;
//		}
//	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcGeom::plat_clear_color( void )
{
	if( mp_instance && mp_instance->GetBoneTransforms() )
	{
		if( mp_instance && mp_instance->GetScene())
		{
			for( int m = 0; m < mp_instance->GetScene()->m_num_filled_meshes; ++m )
			{
				NxNgc::sMesh			*p_mesh	= mp_instance->GetScene()->mpp_mesh_list[m];
				NxNgc::sMaterialHeader	*p_mat	= p_mesh->mp_dl->m_material.p_header;
				if( p_mat )
				{
					NxNgc::sMaterialPassHeader * p_pass = &mp_instance->GetScene()->mp_material_pass[p_mat->m_pass_item];
					for ( int pass = 0; pass < p_mat->m_passes; pass++, p_pass++ )
					{
						p_pass->m_color.r = 128;
						p_pass->m_color.g = 128;
						p_pass->m_color.b = 128;
//						NxNgc::sTextureDL* p_dl = &mp_instance->GetScene()->mp_texture_dl[p_mat->m_texture_dl_id];
//						GX::begin( p_dl->mp_dl, p_dl->m_dl_size );
//						multi_mesh( p_mat, p_pass, true, true );
//						p_dl->m_dl_size = GX::end();

//						int size = mp_instance->GetScene()->mp_texture_dl[p_mat->m_texture_dl_id].m_dl_size;
//						void * p_dl = mp_instance->GetScene()->mp_texture_dl[p_mat->m_texture_dl_id].mp_dl;
//						GX::ChangeMaterialColor( (unsigned char *)p_dl, size, p_pass->m_color, pass );
						p_mat->m_flags |= (1<<4);
					}
				}
			}
		}
	}
	else if( m_mesh_array != NULL )
	{
		for( uint32 i = 0; i < m_num_mesh; ++i )
		{
			NxNgc::sMesh *p_mesh = m_mesh_array[i];
			p_mesh->m_base_color.r = 128;
			p_mesh->m_base_color.g = 128;
			p_mesh->m_base_color.b = 128;
			p_mesh->m_base_color.a = 255;
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Image::RGBA	CNgcGeom::plat_get_color( void ) const
{
	Image::RGBA rgba( 0, 0, 0, 0 );

	if( mp_instance && mp_instance->GetBoneTransforms() )
	{
		if( mp_instance && mp_instance->GetScene())
		{
			if ( mp_instance->GetScene()->m_num_filled_meshes > 0 )
			{
				NxNgc::sMesh			*p_mesh	= mp_instance->GetScene()->mpp_mesh_list[0];
				NxNgc::sMaterialHeader	*p_mat	= p_mesh->mp_dl->m_material.p_header;
				if( p_mat )
				{
					NxNgc::sMaterialPassHeader * p_pass = &mp_instance->GetScene()->mp_material_pass[p_mat->m_pass_item];

					rgba.r = p_pass->m_color.r;
					rgba.g = p_pass->m_color.g;
					rgba.b = p_pass->m_color.b;
					rgba.a = p_pass->m_color.a;

				}
			}
		}
	}
	else if( m_mesh_array != NULL )
	{
		NxNgc::sMesh *p_mesh = m_mesh_array[0];
		rgba.r = p_mesh->m_base_color.r;
		rgba.g = p_mesh->m_base_color.g;
		rgba.b = p_mesh->m_base_color.b;
		rgba.a = p_mesh->m_base_color.a;
	}

	return rgba;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcGeom::plat_set_material_color( uint32 mat_name_checksum, int pass, Image::RGBA rgba )
{
	bool something_changed = false;

	if( mp_instance && mp_instance->GetScene())
	{
		for( int m = 0; m < mp_instance->GetScene()->m_num_filled_meshes; ++m )
		{
			NxNgc::sMesh			*p_mesh	= mp_instance->GetScene()->mpp_mesh_list[m];
			NxNgc::sMaterialHeader	*p_mat	= p_mesh->mp_dl->m_material.p_header;
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
				if( p_mat->m_materialNameChecksum == mat_name_checksum )
					want_this_material = true;
				else if(( p_mat->m_materialNameChecksum > mat_name_checksum ) && (( p_mat->m_materialNameChecksum - mat_name_checksum ) <= 0x7F ))
				{
					uint32 checksum_diff		= p_mat->m_materialNameChecksum - mat_name_checksum;
					int render_separate_pass	= checksum_diff & 0x07;
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
//					Dbg_Assert((uint32)adjusted_pass < p_mat->m_passes );
		
					if ( adjusted_pass < p_mat->m_passes )
					{
						NxNgc::sMaterialPassHeader * p_pass = &mp_instance->GetScene()->mp_material_pass[p_mat->m_pass_item+adjusted_pass];

						if( !( p_pass->m_flags & (1<<4) ))		// COLOR_LOCKED
						{
							p_pass->m_color.r = rgba.r;
							p_pass->m_color.g = rgba.g;
							p_pass->m_color.b = rgba.b;

//							void * p_dl = mp_instance->GetScene()->mp_texture_dl[p_mat->m_texture_dl_id].mp_dl;
//							int size = mp_instance->GetScene()->mp_texture_dl[p_mat->m_texture_dl_id].m_dl_size;
//
//							GX::ChangeMaterialColor( (unsigned char *)p_dl, size, p_pass->m_color, adjusted_pass );

							// Flag as being direct for ever more.
							p_mat->m_flags |= (1<<4);

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
CNgcGeom *CNgcGeom::plat_clone( bool instance, CScene *p_dest_scene )
{
//	instance = true;

	CNgcGeom *p_clone = NULL;

	// This is a CNgcGeom 'hanging' from a sector. Create a new CNgcGeom which will store the CInstance.
	p_clone = new CNgcGeom();
	p_clone->mp_scene = mp_scene;

	// Create new meshes for the clone.
	p_clone->m_mesh_array	= m_num_mesh ? new NxNgc::sMesh*[m_num_mesh] : NULL;
	p_clone->m_num_mesh		= m_num_mesh;
//	NxNgc::sMesh * p_mesh0 = NULL;
	for( uint32 m = 0; m < p_clone->m_num_mesh; ++m )
	{
//		if ( mp_instance )
//		{
//			// Instances don't need display lists cloning...
//			p_clone->m_mesh_array[m] = m_mesh_array[m]->Clone( false );
//		}
//		else
//		{
			p_clone->m_mesh_array[m] = m_mesh_array[m]->Clone( instance );
//		}

//		if ( m_mesh_array[m]->m_localMeshIndex == 0 )
//		{
//			p_mesh0 = p_clone->m_mesh_array[m];
//		}
	}
//	if ( p_mesh0 )
//	{
//		for( uint32 m = 0; m < p_clone->m_num_mesh; ++m )
//		{
//			if ( p_clone->m_mesh_array[m]->m_localMeshIndex != 0 )
//			{
//				p_clone->m_mesh_array[m]->mp_posBuffer = p_mesh0->mp_posBuffer;
//				p_clone->m_mesh_array[m]->mp_normBuffer = p_mesh0->mp_normBuffer;
//				p_clone->m_mesh_array[m]->mp_colBuffer = p_mesh0->mp_colBuffer;
//			}
//		}
//	}

	if( instance == false )
	{
		p_clone->SetActive( true );

		// In this situation, we need to add the individual meshes to the scene.
		// Grab a temporary workspace buffer.
		Nx::CNgcScene *p_Ngc_scene						= static_cast<CNgcScene*>( p_dest_scene );
		NxNgc::sScene *p_scene								= p_Ngc_scene->GetEngineScene();
		NxNgc::sMesh **p_temp_mesh_buffer				= ( p_scene->m_num_meshes > 0 ) ? new NxNgc::sMesh*[p_scene->m_num_meshes] : NULL;

		// Set the scene pointer for the clone.
		p_clone->SetScene( p_Ngc_scene );
		
		// Copy meshes over into the temporary workspace buffer.
		for( int i = 0; i < p_scene->m_num_meshes; ++i )
		{
			p_temp_mesh_buffer[i] = p_scene->mpp_mesh_list[i];
		}

		// Store how many meshes were present.
		int old_num_entries			= p_scene->m_num_meshes;
		
		// Delete current mesh arrays.
		delete [] p_scene->mpp_mesh_list;

		// Important to set these to NULL.
		p_scene->mpp_mesh_list			= NULL;
		
		// Include new meshes in count.
		p_scene->CountMeshes( m_num_mesh, m_mesh_array );

		// Allocate new mesh arrays.
		p_scene->CreateMeshArrays();

		// Copy old mesh data back in.
		for( int i = 0; i < old_num_entries; ++i )
		{
			p_scene->mpp_mesh_list[i] = p_temp_mesh_buffer[i];
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
		NxNgc::sScene *p_scene = p_clone->GenerateScene();

		p_clone->SetActive( true );
	
		// Create the instance.
		Mth::Matrix temp;
		temp.Identity();
		NxNgc::CInstance *p_instance = new NxNgc::CInstance( p_scene, temp, 1, NULL );
		
		// This instance will be the only object maintaining a reference to the attached scene, so we want to delete
		// the scene when the instance gets removed.
		p_instance->SetFlag( NxNgc::CInstance::INSTANCE_FLAG_DELETE_ATTACHED_SCENE );

		// Hook the clone up to the instance.		
		p_clone->SetInstance( p_instance );

		p_clone->mp_scene = NULL;
	}

	return p_clone;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcGeom::plat_set_model_lights( CModelLights* p_model_lights )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNgcGeom *CNgcGeom::plat_clone( bool instance, CModel* pDestModel )
{
	CNgcGeom *p_clone = NULL;

	// This is a CNgcGeom 'hanging' from a sector. Create a new CNgcGeom which will store the CInstance.
	Nx::CNgcScene *p_Ngc_scene = new CNgcScene;
	p_Ngc_scene->SetEngineScene( mp_instance->GetScene() );

	p_clone = new CNgcGeom();
	p_clone->mp_scene = p_Ngc_scene;

	int num_mesh = mp_instance->GetScene()->m_num_filled_meshes;

	// Create new meshes for the clone.
	p_clone->m_mesh_array	= num_mesh ? new NxNgc::sMesh*[num_mesh] : NULL;
	p_clone->m_num_mesh		= num_mesh;
	for( uint32 m = 0; m < p_clone->m_num_mesh; ++m )
	{
		p_clone->m_mesh_array[m] = mp_instance->GetScene()->mpp_mesh_list[m]->Clone( instance );
	}
	// Create a new scene which will be attached via the instance.
	p_clone->m_bbox			= m_bbox;
	NxNgc::sScene *p_scene = p_clone->GenerateScene();

	// Kill the temp scene.
	p_Ngc_scene->SetEngineScene( NULL );
	delete p_Ngc_scene;
	p_clone->mp_scene = NULL;

	p_clone->SetActive( true );

	int numBones;
	numBones = pDestModel->GetNumBones();

	Mth::Matrix * p_bone = new Mth::Matrix[numBones];

	for ( int i = 0; i < numBones; i++ )
	{
		p_bone[i].Identity();
	}

//	((CNgcModel*)pDestModel)->GetInstance()->SetBoneTransforms( p_bone );

	// Create the instance.
	Mth::Matrix temp;
	temp.Identity();
	NxNgc::CInstance *p_instance = new NxNgc::CInstance( p_scene, temp, numBones, p_bone );
	
	((CNgcModel*)pDestModel)->SetInstance( p_instance );

	// This instance will be the only object maintaining a reference to the attached scene, so we want to delete
	// the scene when the instance gets removed.
	p_instance->SetFlag( NxNgc::CInstance::INSTANCE_FLAG_DELETE_ATTACHED_SCENE );

	// Hook the clone up to the instance.		
	p_clone->SetInstance( p_instance );

	return p_clone;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CNgcGeom::plat_set_orientation(const Mth::Matrix& orient)
{
	if( mp_instance )
	{
		// Set values
		mp_instance->GetTransform()->GetRight() = orient[X];
		mp_instance->GetTransform()->GetUp() = orient[Y];
		mp_instance->GetTransform()->GetAt() = orient[Z];
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CNgcGeom::plat_rotate_y(Mth::ERot90 rot)
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
			*((Mth::ERot90*)&m_mod.offset[W]) = rot;

			int num = plat_get_num_render_verts();
			Mth::Vector * p_verts = new Mth::Vector[num];
			plat_get_render_verts( p_verts );
			for ( int lp = 0; lp < num; lp++ )
			{
				float x;
				float z;
				switch ( rot )
				{
					case Mth::ROT_90:
						x = p_verts[lp][X] - m_mod.offset[X];
						z = p_verts[lp][Z] - m_mod.offset[Z];
						p_verts[lp][X] =  z + m_mod.offset[X];
						p_verts[lp][Z] = -x + m_mod.offset[Z];
						break;
					case Mth::ROT_180:
						x = p_verts[lp][X] - m_mod.offset[X];
						z = p_verts[lp][Z] - m_mod.offset[Z];
						p_verts[lp][X] = -x + m_mod.offset[X];
						p_verts[lp][Z] = -z + m_mod.offset[Z];
						break;
					case Mth::ROT_270:
						x = p_verts[lp][X] - m_mod.offset[X];
						z = p_verts[lp][Z] - m_mod.offset[Z];
						p_verts[lp][X] = -z + m_mod.offset[X];
						p_verts[lp][Z] = x + m_mod.offset[Z];
						break;
					default:
					case Mth::ROT_0:
						// Do nothing.
						break;
				}
			}
			plat_set_render_verts( p_verts );
			delete p_verts;
		}
	}
	*((Mth::ERot90*)&m_mod.offset[W]) = rot;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CNgcGeom::plat_set_transform(const Mth::Matrix& transform)
{
	// Set values
	*mp_instance->GetTransform() = transform;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Matrix &	CNgcGeom::plat_get_transform() const
{
	return *mp_instance->GetTransform();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int CNgcGeom::build_index_map( int * p_pos_index, int * p_col_index )
{
	int num_used = 0;
	// Need to go through every index and find unique pos/color pairs.
	if ( mp_scene->GetEngineScene()->mp_scene_data )
	{
		for ( uint mesh = 0; mesh < m_num_mesh; mesh++ )
		{
			NxNgc::sMesh * p_mesh = m_mesh_array[mesh];

			// Parse display list looking for this pos index, then get the color index.
			unsigned char * p_start = (unsigned char *)&p_mesh->mp_dl[1];
			unsigned char * p_end = &p_start[p_mesh->mp_dl->m_size];
			p_start = &p_start[p_mesh->mp_dl->m_index_offset];		// Skip to actual 1st GDBegin.
			unsigned char * p8 = p_start;

			int stride = p_mesh->mp_dl->m_index_stride * 2;
			int offset = p_mesh->mp_dl->m_color_offset * 2;

			while ( p8 < p_end )
			{
				if ( ( p8[0] & 0xf8 ) == GX_TRIANGLESTRIP )
				{
					// Found a triangle strip - parse it.
					int num_idx = ( p8[1] << 8 ) | p8[2];
					p8 += 3;		// Skip GDBegin

					for ( int v = 0; v < num_idx; v++ )
					{
						int index = ( ( p8[0] << 8 ) | p8[1] ) + p_mesh->mp_dl->m_array_base;
						int cindex = ( p8[offset+0] << 8 ) | p8[offset+1];

						// See if this is a unique pair.
						bool found = false;
						for ( int lp = 0; lp < num_used; lp++ )
						{
							if ( ( p_pos_index[lp] == index ) && ( p_col_index[lp] == cindex ) )
							{
								found = true;
								break;
							}
						}
						if ( !found )
						{
							p_pos_index[num_used] = index;
							p_col_index[num_used] = cindex;
							num_used++;
						}
						p8 += stride;
					}
				}
				else
				{
					break;
				}
			}
		}
	}
	return num_used;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
#define MAX_INDEX 2500

int CNgcGeom::plat_get_num_render_verts( void )
{
	if ( mp_scene->GetEngineScene()->mp_scene_data )
	{
		NxNgc::sMesh * p_mesh = m_mesh_array[0];

		if ( p_mesh->m_flags & NxNgc::sMesh::MESH_FLAG_INDEX_COUNT_SET )
		{
			return p_mesh->mp_dl->m_num_indices;
		}
	}

	int pos_index[MAX_INDEX];
	int col_index[MAX_INDEX];

	int num_verts = build_index_map( pos_index, col_index );
//	Pcm::PCMAudio_Update();
	do_audio_update();

	return num_verts;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CNgcGeom::plat_get_render_verts( Mth::Vector *p_verts )
{
	// See if pool has been copied already.
	if ( mp_scene->GetEngineScene()->mp_scene_data )
	{
		NxNgc::sMesh * p_mesh = m_mesh_array[0];

		if ( p_mesh->m_flags & NxNgc::sMesh::MESH_FLAG_CLONED_POS )
		{
			for ( int lp = 0; lp < p_mesh->mp_dl->m_num_indices; lp++ )
			{
				p_verts[lp][X] = p_mesh->mp_dl->mp_pos_pool[(lp*3)+0];
				p_verts[lp][Y] = p_mesh->mp_dl->mp_pos_pool[(lp*3)+1];
				p_verts[lp][Z] = p_mesh->mp_dl->mp_pos_pool[(lp*3)+2];
				p_verts[lp][W] = 1.0f;
			}
//			Pcm::PCMAudio_Update();
			do_audio_update();
			return;
		}
	}

	int pos_index[MAX_INDEX];
	int col_index[MAX_INDEX];

	int num_verts = build_index_map( pos_index, col_index );
//	Pcm::PCMAudio_Update();
	do_audio_update();

	// Copy from original pool.
	for ( int lp = 0; lp < num_verts; lp++ )
	{
		p_verts[lp][X] = mp_scene->GetEngineScene()->mp_pos_pool[(pos_index[lp]*3)+0];
		p_verts[lp][Y] = mp_scene->GetEngineScene()->mp_pos_pool[(pos_index[lp]*3)+1];
		p_verts[lp][Z] = mp_scene->GetEngineScene()->mp_pos_pool[(pos_index[lp]*3)+2];
		p_verts[lp][W] = 1.0f;

	}
//	Pcm::PCMAudio_Update();
	do_audio_update();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcGeom::plat_get_render_colors( Image::RGBA *p_colors )
{
	// See if pool has been copied already.
	if ( mp_scene->GetEngineScene()->mp_scene_data )
	{
		NxNgc::sMesh * p_mesh = m_mesh_array[0];

		if ( p_mesh->m_flags & NxNgc::sMesh::MESH_FLAG_CLONED_COL )
		{
			for ( int lp = 0; lp < p_mesh->mp_dl->m_num_indices; lp++ )
			{
				p_colors[lp].r = ( p_mesh->mp_dl->mp_col_pool[lp] >> 24 ) & 0xff;
				p_colors[lp].g = ( p_mesh->mp_dl->mp_col_pool[lp] >> 16 ) & 0xff;
				p_colors[lp].b = ( p_mesh->mp_dl->mp_col_pool[lp] >>  8 ) & 0xff;
				p_colors[lp].a = ( p_mesh->mp_dl->mp_col_pool[lp] >>  0 ) & 0xff;
			}
//			Pcm::PCMAudio_Update();
			do_audio_update();
			return;
		}
	}

	int pos_index[MAX_INDEX];
	int col_index[MAX_INDEX];

	int num_verts = build_index_map( pos_index, col_index );
//	Pcm::PCMAudio_Update();
	do_audio_update();

	// Copy from original pool.
	for ( int lp = 0; lp < num_verts; lp++ )
	{
		p_colors[lp].r = ( mp_scene->GetEngineScene()->mp_col_pool[col_index[lp]] >> 24 ) & 0xff;
		p_colors[lp].g = ( mp_scene->GetEngineScene()->mp_col_pool[col_index[lp]] >> 16 ) & 0xff;
		p_colors[lp].b = ( mp_scene->GetEngineScene()->mp_col_pool[col_index[lp]] >>  8 ) & 0xff;
		p_colors[lp].a = ( mp_scene->GetEngineScene()->mp_col_pool[col_index[lp]] >>  0 ) & 0xff;
	}
//	Pcm::PCMAudio_Update();
	do_audio_update();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcGeom::plat_set_render_verts( Mth::Vector *p_verts )
{
	// See if we need to create a new pool.
	bool need_to_create = false;
	if ( mp_scene->GetEngineScene()->mp_scene_data )
	{
		NxNgc::sMesh * p_mesh = m_mesh_array[0];
		if ( ( p_mesh->m_flags & NxNgc::sMesh::MESH_FLAG_CLONED_POS ) == 0 )
		{
			need_to_create = true;
		}
	}

	// Create new pos pool if necessary.
	float * p_new_pos;
	int num_verts;
	if ( need_to_create && mp_scene->GetEngineScene()->mp_scene_data )
	{
		int pos_index[MAX_INDEX];
		int col_index[MAX_INDEX];

		num_verts = build_index_map( pos_index, col_index );
//		Pcm::PCMAudio_Update();
		do_audio_update();

		// Allocate space for new pool.
		p_new_pos = new float[3*num_verts];

		for ( uint mesh = 0; mesh < m_num_mesh; mesh++ )
		{
			NxNgc::sMesh * p_mesh = m_mesh_array[mesh];

			if ( !p_mesh->mp_dl->mp_pos_pool )
			{
				p_mesh->mp_dl->mp_pos_pool = p_new_pos;

				if ( mesh == 0 )
				{
					p_mesh->m_flags |= NxNgc::sMesh::MESH_FLAG_CLONED_POS;
					p_mesh->m_flags |= NxNgc::sMesh::MESH_FLAG_INDEX_COUNT_SET;
					p_mesh->mp_dl->m_num_indices = num_verts;
				}

				// Remap the DL.
				// Parse display list looking for this pos index, then get the color index.
				unsigned char * p_start = (unsigned char *)&p_mesh->mp_dl[1];
				unsigned char * p_end = &p_start[p_mesh->mp_dl->m_size];
				p_start = &p_start[p_mesh->mp_dl->m_index_offset];		// Skip to actual 1st GDBegin.
				unsigned char * p8 = p_start;

				int stride = p_mesh->mp_dl->m_index_stride * 2;
				int offset = p_mesh->mp_dl->m_color_offset * 2;

				while ( p8 < p_end )
				{
					if ( ( p8[0] & 0xf8 ) == GX_TRIANGLESTRIP )
					{
						// Found a triangle strip - parse it.
						int num_idx = ( p8[1] << 8 ) | p8[2];
						p8 += 3;		// Skip GDBegin

						for ( int v = 0; v < num_idx; v++ )
						{
							// Find the remapped index.
							int index = ( ( p8[0] << 8 ) | p8[1] ) + p_mesh->mp_dl->m_array_base;
							int cindex = ( p8[offset+0] << 8 ) | p8[offset+1];

							for ( int lp = 0; lp < num_verts; lp++ )
							{
								if ( ( pos_index[lp] == index ) && ( col_index[lp] == cindex ) )
								{
									p8[0] = ( lp >> 8 ) & 0xff;
									p8[1] = lp & 0xff;
									break;
								}
							}
							p8 += stride;
						}
					}
					else
					{
						break;
					}
				}
				DCFlushRange( &p_mesh->mp_dl[1], p_mesh->mp_dl->m_size );
			}
			p_mesh->mp_dl->m_array_base = 0;

//			Pcm::PCMAudio_Update();
			do_audio_update();
		}
	}
	else
	{
		p_new_pos = m_mesh_array[0]->mp_dl->mp_pos_pool;
		num_verts = m_mesh_array[0]->mp_dl->m_num_indices;
	}

	// Copy actual data over.
	Mth::CBBox bbox;
	for ( int v = 0; v < num_verts; v++ )
	{
		p_new_pos[(v*3)+0] = p_verts[v][X];
		p_new_pos[(v*3)+1] = p_verts[v][Y];
		p_new_pos[(v*3)+2] = p_verts[v][Z];
		bbox.AddPoint( p_verts[v] );
	}
	DCFlushRange( p_new_pos, sizeof( float ) * 3 * num_verts );

	if ( mp_scene->GetEngineScene()->mp_scene_data )
	{
		for ( uint mesh = 0; mesh < m_num_mesh; mesh++ )
		{
			NxNgc::sMesh * p_mesh = m_mesh_array[mesh];

			// Now refigure the bounding sphere.
			Mth::Vector sphere_center	= bbox.GetMin() + ( 0.5f * ( bbox.GetMax() - bbox.GetMin()));
			p_mesh->mp_dl->m_sphere[X] = sphere_center[X];
			p_mesh->mp_dl->m_sphere[Y] = sphere_center[Y];
			p_mesh->mp_dl->m_sphere[Z] = sphere_center[Z];
			p_mesh->mp_dl->m_sphere[W] = ( bbox.GetMax() - sphere_center ).Length(); 

			p_mesh->m_bottom_y = bbox.GetMin()[Y];
		}
	}
//	Pcm::PCMAudio_Update();
	do_audio_update();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcGeom::plat_set_render_colors( Image::RGBA *p_colors )
{
	// See if we need to create a new pool.
	bool need_to_create = false;
	if ( mp_scene->GetEngineScene()->mp_scene_data )
	{
		NxNgc::sMesh * p_mesh = m_mesh_array[0];
		if ( !( p_mesh->m_flags & NxNgc::sMesh::MESH_FLAG_CLONED_COL ) )
		{
			need_to_create = true;
		}
	}

	// Create new col pool if necessary.
	uint32 * p_new_col;
	int num_verts;
	if ( need_to_create && mp_scene->GetEngineScene()->mp_scene_data )
	{
		int pos_index[MAX_INDEX];
		int col_index[MAX_INDEX];

		num_verts = build_index_map( pos_index, col_index );
//		Pcm::PCMAudio_Update();
		do_audio_update();

		// Allocate space for new pool.
		p_new_col = new uint32[num_verts];

		for ( uint mesh = 0; mesh < m_num_mesh; mesh++ )
		{
			NxNgc::sMesh * p_mesh = m_mesh_array[mesh];

			if ( !p_mesh->mp_dl->mp_col_pool )
			{
				p_mesh->mp_dl->mp_col_pool = p_new_col;

				if ( mesh == 0 )
				{
					p_mesh->m_flags |= NxNgc::sMesh::MESH_FLAG_CLONED_COL;
					p_mesh->m_flags |= NxNgc::sMesh::MESH_FLAG_INDEX_COUNT_SET;
					p_mesh->mp_dl->m_num_indices = num_verts;
				}

				// Remap the DL.
				// Parse display list looking for this pos index, then get the color index.
				unsigned char * p_start = (unsigned char *)&p_mesh->mp_dl[1];
				unsigned char * p_end = &p_start[p_mesh->mp_dl->m_size];
				p_start = &p_start[p_mesh->mp_dl->m_index_offset];		// Skip to actual 1st GDBegin.
				unsigned char * p8 = p_start;

				int stride = p_mesh->mp_dl->m_index_stride * 2;
				int offset = p_mesh->mp_dl->m_color_offset * 2;

				while ( p8 < p_end )
				{
					if ( ( p8[0] & 0xf8 ) == GX_TRIANGLESTRIP )
					{
						// Found a triangle strip - parse it.
						int num_idx = ( p8[1] << 8 ) | p8[2];
						p8 += 3;		// Skip GDBegin

						for ( int v = 0; v < num_idx; v++ )
						{
							// Find the remapped index.
							int index = ( ( p8[0] << 8 ) | p8[1] ) + p_mesh->mp_dl->m_array_base;
							int cindex = ( p8[offset+0] << 8 ) | p8[offset+1];

							for ( int lp = 0; lp < num_verts; lp++ )
							{
								if ( ( pos_index[lp] == index ) && ( col_index[lp] == cindex ) )
								{
									p8[offset+0] = ( lp >> 8 ) & 0xff;
									p8[offset+1] = lp & 0xff;
									if ( p_mesh->mp_dl->m_material.p_header->m_flags & (1<<6)/*m_correctable*/ )
									{
										p8[offset+2] = ( lp >> 8 ) & 0xff;
										p8[offset+3] = lp & 0xff;
									}
									break;
								}
							}
							p8 += stride;
						}
					}
					else
					{
						break;
					}
				}
				DCFlushRange( &p_mesh->mp_dl[1], p_mesh->mp_dl->m_size );
			}
//			p_mesh->mp_dl->m_array_base = 0;

//			Pcm::PCMAudio_Update();
			do_audio_update();
		}
	}
	else
	{
		p_new_col = m_mesh_array[0]->mp_dl->mp_col_pool;
		num_verts = m_mesh_array[0]->mp_dl->m_num_indices;
	}

	// Copy actual data over.
	for ( int v = 0; v < num_verts; v++ )
	{
		p_new_col[v] = ( p_colors[v].r << 24 ) | ( p_colors[v].g << 16 ) | ( p_colors[v].b << 8 ) | p_colors[v].a;
	}
	DCFlushRange( p_new_col, sizeof( uint32 ) * num_verts );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcGeom::plat_set_scale(const Mth::Vector & scale)
{
	// Ensure w component is correct.

	if( mp_instance )
	{
//		mp_instance->GetTransform()->SetScale( scale );
	}
	else
	{
		int num = plat_get_num_render_verts();
		Mth::Vector * p_verts = new Mth::Vector[num];
		plat_get_render_verts( p_verts );
		for ( int lp = 0; lp < num; lp++ )
		{
			p_verts[lp][X] = ( ( p_verts[lp][X] - m_mod.offset[X] ) * scale[X] ) + m_mod.offset[X];
			p_verts[lp][Y] = ( ( p_verts[lp][Y] - m_mod.offset[Y] ) * scale[Y] ) + m_mod.offset[Y];
			p_verts[lp][Z] = ( ( p_verts[lp][Z] - m_mod.offset[Z] ) * scale[Z] ) + m_mod.offset[Z];
		}
		plat_set_render_verts( p_verts );
		delete p_verts;

//		// Go through and adjust the individual meshes.
//		for( uint32 i = 0; i < m_num_mesh; ++i )
//		{
//			NxNgc::sMesh *p_mesh = m_mesh_array[i];
//			p_mesh->mp_mod = &m_mod;
//
////			Mth::Vector delta_pos( pos[X] - m_offset[X], pos[Y] - m_offset[Y], pos[Z] - m_offset[Z], 1.0f );
////			p_mesh->mp_dl->m_sphere[X] += delta_pos[X];
////			p_mesh->mp_dl->m_sphere[Y] += delta_pos[Y];
////			p_mesh->mp_dl->m_sphere[Z] += delta_pos[Z];
////			p_mesh->SetPosition( proper_pos );
//		}
//
////		if ( mp_scene->GetEngineScene()->mp_scene_data )
////		{
////			Mth::Vector proper_pos( pos[X] - m_offset[X], pos[Y] - m_offset[Y], pos[Z] - m_offset[Z], 1.0f );
////	
////			float * pos = mp_scene->GetEngineScene()->mp_pos_pool;
////			for ( int lp = 0; lp += mp_scene->GetEngineScene()->mp_scene_data->m_num_pos; lp++ )
////			{
////				pos[0] += proper_pos[X];
////				pos[1] += proper_pos[Y];
////				pos[2] += proper_pos[Z];
////				pos += 3;
////			}
////			m_offset[X] = proper_pos[X];
////			m_offset[Y] = proper_pos[Y];
////			m_offset[Z] = proper_pos[Z];
////		}
	}
	m_mod.scale = scale;



//	Dbg_MsgAssert( m_mesh_array, ( "Invalid for instanced sectors" ));
//
//	m_scale = scale;
//
//	if( m_mesh_array )
//	{
//		NxNgc::sMesh *p_mesh = m_mesh_array[0];
//
//		// Scale each vertex.
//#ifdef SHORT_VERT
//		for( uint32 v = 0; v < p_mesh->m_num_vertex; ++v )
//		{
//			p_mesh->mp_posBuffer[(v*3)+0] = (s16)( (float)p_mesh->mp_posBuffer[(v*3)+0] * scale[X] );
//			p_mesh->mp_posBuffer[(v*3)+1] = (s16)( (float)p_mesh->mp_posBuffer[(v*3)+1] * scale[Y] );
//			p_mesh->mp_posBuffer[(v*3)+2] = (s16)( (float)p_mesh->mp_posBuffer[(v*3)+2] * scale[Z] );
//		}
//		DCFlushRange( p_mesh->mp_posBuffer, 3 * sizeof( s16 ) * p_mesh->m_num_vertex );
//#else
//		for( uint32 v = 0; v < p_mesh->m_num_vertex; ++v )
//		{
//			p_mesh->mp_posBuffer[(v*3)+0] = ( ( p_mesh->mp_posBuffer[(v*3)+0] - p_mesh->m_offset_x ) * scale[X] ) + p_mesh->m_offset_x;
//			p_mesh->mp_posBuffer[(v*3)+1] = ( ( p_mesh->mp_posBuffer[(v*3)+1] - p_mesh->m_offset_y ) * scale[Y] ) + p_mesh->m_offset_y;
//			p_mesh->mp_posBuffer[(v*3)+2] = ( ( p_mesh->mp_posBuffer[(v*3)+2] - p_mesh->m_offset_z ) * scale[Z] ) + p_mesh->m_offset_z;
//		}
//		DCFlushRange( p_mesh->mp_posBuffer, 3 * sizeof( float ) * p_mesh->m_num_vertex );
//#endif		// SHORT_VERT 
//	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Mth::Vector CNgcGeom::plat_get_scale() const
{
	return m_mod.scale;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcGeom::plat_set_uv_wibble_params( float u_vel, float u_amp, float u_freq, float u_phase,
										   float v_vel, float v_amp, float v_freq, float v_phase )
{
//	if( m_mesh_array )
//	{
//		for( uint32 m = 0; m < m_num_mesh; ++m )
//		{
//			NxNgc::sMesh		*p_mesh	= m_mesh_array[m];
//			NxNgc::sMaterial	*p_mat	= p_mesh->mp_material;
//			if( p_mat )
//			{
//				// Find the first pass that wibbles.
//				for( uint32 p = 0; p < p_mat->m_passes; ++p )
//				{
//					if( p_mat->mp_UVWibbleParams[p] )
//					{
//						p_mat->mp_UVWibbleParams[p]->m_UVel			= u_vel;
//						p_mat->mp_UVWibbleParams[p]->m_VVel			= v_vel;
//						p_mat->mp_UVWibbleParams[p]->m_UAmplitude	= u_amp;
//						p_mat->mp_UVWibbleParams[p]->m_VAmplitude	= v_amp;
//						p_mat->mp_UVWibbleParams[p]->m_UFrequency	= u_freq;
//						p_mat->mp_UVWibbleParams[p]->m_VFrequency	= v_freq;
//						p_mat->mp_UVWibbleParams[p]->m_UPhase		= u_phase;
//						p_mat->mp_UVWibbleParams[p]->m_VPhase		= v_phase;
//						break;
//					}
//				}
//			}
//		}
//	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcGeom::plat_use_explicit_uv_wibble( bool yes )
{
//	if( mp_instance && mp_instance->GetScene())
//	{
//		for( int m = 0; m < mp_instance->GetScene()->m_num_mesh_entries; ++m )
//		{
//			NxNgc::sMesh		*p_mesh	= mp_instance->GetScene()->m_meshes[m];
//			NxNgc::sMaterial	*p_mat	= p_mesh->mp_material;
//			if( p_mat )
//			{
//				for( uint32 p = 0; p < p_mat->m_passes; ++p )
//				{
//					if( p_mat->mp_UVWibbleParams[p] )
//					{
//						p_mat->m_flags[p] |= MATFLAG_EXPLICIT_UV_WIBBLE;
//					}
//				}
//			}
//		}
//	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcGeom::plat_set_uv_wibble_offsets( float u_offset, float v_offset )
{
//	if( m_mesh_array )
//	{
//		for( uint32 m = 0; m < m_num_mesh; ++m )
//		{
//			NxNgc::sMesh		*p_mesh	= m_mesh_array[m];
//			NxNgc::sMaterial	*p_mat	= p_mesh->mp_material;
//			if( p_mat )
//			{
//				// Find the first pass that wibbles.
//				for( uint32 p = 0; p < p_mat->m_passes; ++p )
//				{
//					if( p_mat->mp_UVWibbleParams[p] )
//					{
//						p_mat->mp_UVWibbleParams[p]->m_UVMatrix[2] = u_offset;
//						p_mat->mp_UVWibbleParams[p]->m_UVMatrix[3] = v_offset;
//						plat_use_explicit_uv_wibble( true );
//						break;
//					}
//				}
//			}
//		}
//	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcGeom::plat_set_uv_wibble_offsets( uint32 mat_name_checksum, int pass, float u_offset, float v_offset )
{
//	if( m_mesh_array )
//	{
//		for( uint32 m = 0; m < m_num_mesh; ++m )
//		{
//			NxNgc::sMesh		*p_mesh	= m_mesh_array[m];
//			NxNgc::sMaterial	*p_mat	= p_mesh->mp_material;
//			if( p_mat && ( p_mat->m_name_checksum == mat_name_checksum ))
//			{
//				// Find the first pass that wibbles.
////				for( uint32 p = 0; p < p_mat->m_passes; ++p )
////				{
////					if( p_mat->mp_UVWibbleParams[p] )
////					{
////						p_mat->mp_UVWibbleParams[p]->m_UWibbleOffset = u_offset;
////						p_mat->mp_UVWibbleParams[p]->m_VWibbleOffset = v_offset;
////						plat_use_explicit_uv_wibble( true );
////						break;
////					}
////				}
//			}
//		}
//	}
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CNgcGeom::plat_set_uv_matrix( uint32 mat_name_checksum, int pass, const Mth::Matrix& mat )
{
	if( mp_instance && mp_instance->GetScene())
	{
		for( int m = 0; m < mp_instance->GetScene()->m_num_filled_meshes; ++m )
		{
			NxNgc::sMesh			*p_mesh	= mp_instance->GetScene()->mpp_mesh_list[m];
			NxNgc::sMaterialHeader	*p_mat	= p_mesh->mp_dl->m_material.p_header;
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
				if( p_mat->m_materialNameChecksum == mat_name_checksum )
				{
					want_this_material = true;
				}
				else if(( p_mat->m_materialNameChecksum > mat_name_checksum ) && (( p_mat->m_materialNameChecksum - mat_name_checksum ) <= 0x7F ))
				{
					uint32 checksum_diff		= p_mat->m_materialNameChecksum - mat_name_checksum;
					int render_separate_pass	= checksum_diff & 0x07;
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
					if ( adjusted_pass < p_mat->m_passes )
					{
						// Create the wibble params if they don't exist already.
						NxNgc::sMaterialPassHeader * p_pass = &mp_instance->GetScene()->mp_material_pass[p_mat->m_pass_item+adjusted_pass];

						// Force this to be used.
						p_pass->m_flags |= (1<<2);
						p_pass->m_uv_enabled = 1;
						p_mat->m_flags |= (1<<4);
	//
	//
	//
	////					// Set the matrix values.
	//					p_pass->mp_explicit_wibble->m_matrix[0][0] = mat[0][0];
	//					p_pass->mp_explicit_wibble->m_matrix[0][1] = mat[0][1];
	//					p_pass->mp_explicit_wibble->m_matrix[1][0] = mat[1][0];
	//					p_pass->mp_explicit_wibble->m_matrix[1][1] = mat[1][1];
	//					p_pass->mp_explicit_wibble->m_matrix[2][0] = 1.0f;
	//					p_pass->mp_explicit_wibble->m_matrix[2][1] = 1.0f;
	//					p_pass->mp_explicit_wibble->m_matrix[0][3] = mat[3][0];
	//					p_pass->mp_explicit_wibble->m_matrix[1][3] = mat[3][1];

						p_pass->m_uv_mat[0] = (short)(mat[0][0] * ((float)(1<<12)));
						p_pass->m_uv_mat[1] = (short)(mat[0][1] * ((float)(1<<12)));
						p_pass->m_uv_mat[2] = (short)(mat[3][0] * ((float)(1<<12)));
						p_pass->m_uv_mat[3] = (short)(mat[3][1] * ((float)(1<<12)));
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
bool CNgcGeom::plat_allocate_uv_matrix_params( uint32 mat_name_checksum, int pass )
{
	if( mp_instance && mp_instance->GetScene())
	{
		for( int m = 0; m < mp_instance->GetScene()->m_num_filled_meshes; ++m )
		{
			NxNgc::sMesh			*p_mesh	= mp_instance->GetScene()->mpp_mesh_list[m];
			NxNgc::sMaterialHeader	*p_mat	= p_mesh->mp_dl->m_material.p_header;
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
				if( p_mat->m_materialNameChecksum == mat_name_checksum )
					want_this_material = true;
				else if(( p_mat->m_materialNameChecksum > mat_name_checksum ) && (( p_mat->m_materialNameChecksum - mat_name_checksum ) <= 0x7F ))
				{
					uint32 checksum_diff		= p_mat->m_materialNameChecksum - mat_name_checksum;
					int render_separate_pass	= checksum_diff & 0x07;
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
					if ( adjusted_pass < p_mat->m_passes )
					{
						NxNgc::sMaterialPassHeader * p_pass = &mp_instance->GetScene()->mp_material_pass[p_mat->m_pass_item+adjusted_pass];
	
						p_pass->m_flags |= (1<<2);
						p_pass->m_uv_enabled = 1;
						p_pass->m_uv_mat[0] = (1<<12);
						p_pass->m_uv_mat[1] = 0;
						p_pass->m_uv_mat[2] = 0;
						p_pass->m_uv_mat[3] = 0;
					}

//					if ( p_pass->mp_explicit_wibble == NULL )
//					{
//						p_pass->mp_explicit_wibble = new NxNgc::sExplicitMaterialUVWibble;
//						p_pass->mp_explicit_wibble->m_matrix[0][0] = 1.0f;
//						p_pass->mp_explicit_wibble->m_matrix[0][1] = 0.0f;
//						p_pass->mp_explicit_wibble->m_matrix[0][2] = 1.0f;
//						p_pass->mp_explicit_wibble->m_matrix[0][3] = 0.0f;
//						p_pass->mp_explicit_wibble->m_matrix[1][0] = 0.0f;
//						p_pass->mp_explicit_wibble->m_matrix[1][1] = 1.0f;
//						p_pass->mp_explicit_wibble->m_matrix[1][2] = 1.0f;
//						p_pass->mp_explicit_wibble->m_matrix[1][3] = 0.0f;
//						p_pass->m_flags |= (1<<2);
//						return true;
//					}
//					// Create the wibble params if they don't exist already.
//					if( p_mat->mp_UVWibbleParams[pass] == NULL )
//					{
//						p_mat->mp_UVWibbleParams[pass]	= new NxNgc::sUVWibbleParams;
//
//						// Need to set flags to indicate that uv wibble is now in effect.
//						p_mat->m_uv_wibble				= true;
//						p_mat->m_flags[pass]		   |= MATFLAG_UV_WIBBLE | MATFLAG_EXPLICIT_UV_WIBBLE;
//						return true;
//					}
				}
			}
		}
	}
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#if 0
	if( mp_instance && mp_instance->GetScene())
	{
		for( int m = 0; m < mp_instance->GetScene()->m_num_filled_meshes; ++m )
		{
			NxNgc::sMesh			*p_mesh	= mp_instance->GetScene()->mpp_mesh_list[m];
			NxNgc::sMaterialHeader	*p_mat	= p_mesh->mp_dl->m_material.p_header;
			if( p_mat )
			{
				bool	want_this_material	= false;

				// We are searching for materials with a matching name. However there is one caveat in that the
				// conversion process sometimes creates new materials for geometry flagged as 'render as separate.
				// In such a case, the new material name checksum = original name checksum + pass + 1.
				if( p_mat->m_materialNameChecksum == mat_name_checksum )
					want_this_material = true;
				else if( p_mat->m_materialNameChecksum == ( mat_name_checksum + pass + 1 ) )
				{
					want_this_material = true;

					pass = 0;
				}

				if( want_this_material )
				{
//					// Create the wibble params if they don't exist already.
//					if( p_mat->mp_UVWibbleParams[pass] == NULL )
//					{
//						p_mat->mp_UVWibbleParams[pass]	= new NxNgc::sUVWibbleParams;
//
//						// Need to set flags to indicate that uv wibble is now in effect.
//						p_mat->m_uv_wibble				= true;
//						p_mat->m_flags[pass]		   |= MATFLAG_UV_WIBBLE | MATFLAG_EXPLICIT_UV_WIBBLE;
//						return true;
//					}
				}
			}
		}
	}
#endif
} // Nx
				

