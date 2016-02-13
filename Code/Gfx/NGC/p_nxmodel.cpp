//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxModel.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  12/21/2002
//****************************************************************************

#include <core/math.h>

#include "gfx/nxmodel.h"
#include "gfx/skeleton.h"
#include <gfx/Ngc/p_nxmesh.h>
#include "gfx/Ngc/p_nxmodel.h"
#include "gfx/Ngc/p_nxscene.h"
#include "gfx/Ngc/nx/import.h"
#include "gfx/Ngc/nx/render.h"
#include "gfx/Ngc/p_nxgeom.h"

#include <sys/ngc/p_display.h>

			   
#include <gel/assman/assman.h>
#include <gel/assman/skinasset.h>

int			test_num_bones			= 0;
Mth::Matrix	*p_test_bone_matrices	= NULL;
Mth::Matrix	test_root_matrix;

namespace Nx
{

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/
						
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CNgcModel::plat_init_skeleton( int numBones )
{
//	if ( !mp_instance ) return false;
//	Mth::Matrix * p_bone = new Mth::Matrix[numBones];
//
//	mp_instance->SetBoneTransforms( p_bone );
//	for ( int i = 0; i < numBones; i++ )
//	{
//		p_bone[i].Identity();
//	}
    
	return true;
}


///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//	
///*
//	bool CNgcModel::plat_load_file(const char* p_fileName)
//{
//	// Machine specific code here ............
//	
//	// TODO:  Make this more generalized
//
//	// Load in the texture dictionary for the model.
//	Lst::HashTable< NxNgc::sTexture > *p_texture_table = NxNgc::LoadTextureFile( "models/testskin/testskin.tex.xbx" );
//
//    return true;
//}
//*/
//
//
//bool CNgcModel::plat_load_mesh( CMesh* pMesh )
//{
//	// The skeleton must exist by this point (unless it's a hacked up car).
//	int numBones;
//	numBones = mp_skeleton ? mp_skeleton->GetNumBones() : 1;
//	
//	Mth::Matrix temp;
//	CNgcMesh *p_Ngc_mesh = static_cast<CNgcMesh*>( pMesh );
//	mp_instance = new NxNgc::CInstance( p_Ngc_mesh->GetScene()->GetEngineScene(), temp, numBones, mp_boneTransforms );
//
//    return true;
//}
//	
//	
//	
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//bool CNgcModel::plat_unload_mesh( void )
//{
//	if ( mp_instance != NULL )
//	{
//		delete mp_instance;
//		mp_instance = NULL;
//	}
//
//	return true;
//}
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//	
//bool CNgcModel::plat_set_render_mode(ERenderMode mode)
//{
//	// Machine specific code here ............
//    return true;
//}
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//	
//bool CNgcModel::plat_set_color(uint8 r, uint8 g, uint8 b, uint8 a)
//{
//	// Machine specific code here ............
//    return true;
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//bool CNgcModel::plat_set_visibility(uint32 mask)
//{
//    return true;
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//bool CNgcModel::plat_set_active( bool active )
//{
//	if( mp_instance )
//	{
//		mp_instance->SetActive( active );
//	}
//	return true;
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//bool CNgcModel::plat_set_scale(float scaleFactor)
//{
//	// Machine specific code here ............
//    return true;
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//bool CNgcModel::plat_replace_texture(char* p_srcFileName, char* p_dstFileName)
//{
//	// Machine specific code here ............
//    return true;
//}
//
//
//
//void ConvertMatrix(Mth::Matrix* pMatrix, void *pMat)
//{
//}
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//bool CNgcModel::plat_render( Mth::Matrix *pRootMatrix, Mth::Matrix *pBoneMatrices, int numBones )
//{
//	if( mp_instance )
//	{
//		mp_instance->SetTransform( *pRootMatrix );
//	}
//    return true;
//}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CNgcModel::plat_prepare_materials( void )
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CNgcModel::plat_refresh_materials( void )
{
	int numGeoms = GetNumGeoms();
	for ( int i = 0; i < numGeoms; i++ )
	{
		NxNgc::CInstance *p_instance = NULL;
		CNgcGeom * p_ngc_geom = static_cast<CNgcGeom*>( mp_geom[i] ); 
		p_instance = p_ngc_geom->GetInstance();
		NxNgc::sScene *p_scene = p_instance->GetScene();

		// Setup material data texture pointers.
		for ( unsigned int lp = 0; lp < p_scene->mp_scene_data->m_num_materials; lp++ )
		{
			NxNgc::sMaterialHeader * p_mat = &p_scene->mp_material_header[lp];
			NxNgc::sTextureDL * p_dl = &p_scene->mp_texture_dl[lp]; 
			NxNgc::sMaterialPassHeader * p_pass = &p_scene->mp_material_pass[p_mat->m_pass_item];

			GX::ResolveDLTexAddr( p_dl, p_pass, p_mat->m_passes );

//			GX::begin( p_dl->mp_dl, p_dl->m_dl_size );
//			multi_mesh( p_mat, p_pass, true, true );
//			p_dl->m_dl_size = GX::end();

			// See if any textures have alpha, but the texture DL doesn't set it up. If so, flag material as
			// not using display list.
			bool direct = false;
			for ( int p = 0; p < p_mat->m_passes; p++, p_pass++ )
			{
				if ( p_pass->m_texture.p_data )
				{
//					bool current_alpha = ( p_pass->m_texture.p_data->flags & NxNgc::sTexture::TEXTURE_FLAG_HAS_ALPHA ) ? true : false;
//					bool dl_alpha = p_dl->m_alpha_offset[p] ? true : false;
//					if ( current_alpha != dl_alpha )
//					{
//						// Must use direct setup mode.
//						direct = true;
//					}
					if ( p_pass->m_texture.p_data->flags & NxNgc::sTexture::TEXTURE_FLAG_REPLACED )
					{
						direct = true;
					}

				}
			}
			if ( direct )
			{
				p_mat->m_flags |= (1<<3);
			}
			else
			{
				p_mat->m_flags &= ~(1<<3);
			}
		}
	}

	return true;
}

/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CNgcModel::CNgcModel()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CNgcModel::~CNgcModel()
{
	if ( mp_instance && mp_instance->GetBoneTransforms() )
	{
		delete mp_instance->GetBoneTransforms();
		mp_instance->SetBoneTransforms( NULL );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector CNgcModel::plat_get_bounding_sphere()
{
	Mth::Vector sphere, sphere1, sum, diff;
	float dist;

	// this should probably never happen
	if (m_numGeoms == 0)
		return Mth::Vector(0.0f, 0.0f, 0.0f, 0.0f);

	// combine the spheres of all geoms
	// (this should really be done once at load time)

	// start with first sphere
	sphere = mp_geom[0]->GetBoundingSphere();

	// loop over remaining spheres, expanding as necessary
	for (int i=1; i<m_numGeoms; i++)
	{
		// get next sphere
		sphere1 = mp_geom[i]->GetBoundingSphere();

		// centre-to-centre vector, and distance
		diff = sphere1-sphere;
		dist = diff.Length();

		// test for sphere1 inside sphere
		if (dist+sphere1[3] <= sphere[3])
			continue;			// keep sphere

		// test for sphere inside sphere1
		if (dist+sphere[3] <= sphere1[3])
		{
			sphere = sphere1;	// replace sphere
			continue;
		}

		// otherwise make a larger sphere that contains both
		sum       = sphere+sphere1;
		sphere    = 0.5f * (sum + (diff[3]/dist) * diff);
		sphere[3] = 0.5f * (dist + sum[3]);
	}

	return sphere;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CNgcModel::plat_set_bounding_sphere( const Mth::Vector& boundingSphere )
{
	// loop over all spheres
	for ( int i = 0; i < m_numGeoms; i++ )
	{
		mp_geom[i]->SetBoundingSphere( boundingSphere );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // Nx

				

