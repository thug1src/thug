//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxModel.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  12/21/2002
//****************************************************************************

#include <core/math.h>

#include "gfx/nxmodel.h"
#include "gfx/skeleton.h"
#include <gfx/xbox/p_nxmesh.h>
#include "gfx/xbox/p_nxmodel.h"
#include "gfx/xbox/p_nxscene.h"
#include "gfx/xbox/p_nxgeom.h"
#include "gfx/xbox/nx/texture.h"
#include "gfx/xbox/nx/render.h"
			   
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
bool CXboxModel::plat_init_skeleton( int num_bones )
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



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
/*
	bool CXboxModel::plat_load_file(const char* p_fileName)
{
	// Machine specific code here ............
	
	// TODO:  Make this more generalized

	// Load in the texture dictionary for the model.
	Lst::HashTable< NxXbox::sTexture > *p_texture_table = NxXbox::LoadTextureFile( "models/testskin/testskin.tex.xbx" );

    return true;
}
*/


/*
bool CXboxModel::plat_load_mesh( CMesh* pMesh )
{
	// The skeleton must exist by this point (unless it's a hacked up car).
	int numBones;
	numBones = mp_skeleton ? mp_skeleton->GetNumBones() : 1;
	
	Mth::Matrix temp;
	CXboxMesh *p_xbox_mesh = static_cast<CXboxMesh*>( pMesh );
	mp_instance = new NxXbox::CInstance( p_xbox_mesh->GetScene()->GetEngineScene(), temp, numBones, mp_boneTransforms );

    return true;
}
*/	
	
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
bool CXboxModel::plat_unload_mesh( void )
{
	if ( mp_instance != NULL )
	{
		delete mp_instance;
		mp_instance = NULL;
	}

	return true;
}
*/



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
bool CXboxModel::plat_set_render_mode(ERenderMode mode)
{
	// Machine specific code here ............
    return true;
}
*/



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
bool CXboxModel::plat_set_color(uint8 r, uint8 g, uint8 b, uint8 a)
{
	// Machine specific code here ............
    return true;
}
*/



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
bool CXboxModel::plat_set_visibility(uint32 mask)
{
    return true;
}
*/



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
bool CXboxModel::plat_set_active( bool active )
{
	if( mp_instance )
	{
		mp_instance->SetActive( active );
	}
	return true;
}
*/



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
bool CXboxModel::plat_set_scale(float scaleFactor)
{
	// Machine specific code here ............
    return true;
}
*/



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
bool CXboxModel::plat_replace_texture(char* p_srcFileName, char* p_dstFileName)
{
	// Machine specific code here ............
    return true;
}
*/



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
bool CXboxModel::plat_render( Mth::Matrix *pRootMatrix, Mth::Matrix *pBoneMatrices, int numBones )
{
	if( mp_instance )
	{
		mp_instance->SetTransform( *pRootMatrix );
	}
    return true;
}
*/



/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxModel::CXboxModel()
{
	mp_instance = NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxModel::~CXboxModel()
{
	// make sure it's deleted
//	plat_unload_mesh();

	if( mp_instance && mp_instance->GetBoneTransforms())
	{
		delete mp_instance->GetBoneTransforms();
		mp_instance->SetBoneTransforms( NULL );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxModel::plat_set_bounding_sphere( const Mth::Vector& boundingSphere )
{
	// Loop over all spheres.
	for( int i = 0; i < m_numGeoms; i++ )
	{
		mp_geom[i]->SetBoundingSphere( boundingSphere );
	}
}


	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Mth::Vector CXboxModel::plat_get_bounding_sphere( void )
{
	Mth::Vector sphere, sphere1, sum, diff;
	float dist;

	// This should probably never happen.
	if( m_numGeoms == 0 )
		return Mth::Vector( 0.0f, 0.0f, 0.0f, 0.0f );

	// Combine the spheres of all geoms (this should really be done once at load time).

	// Start with first sphere.
	sphere = mp_geom[0]->GetBoundingSphere();

	// Loop over remaining spheres, expanding as necessary.
	for( int i = 1; i < m_numGeoms; ++i )
	{
		// Get next sphere.
		sphere1 = mp_geom[i]->GetBoundingSphere();

		// Centre-to-centre vector, and distance.
		diff	= sphere1-sphere;
		dist	= diff.Length();

		// Test for sphere1 inside sphere.
		if( dist + sphere1[3] <= sphere[3] )
			continue;			// keep sphere

		// Test for sphere inside sphere1.
		if( dist + sphere[3] <= sphere1[3] )
		{
			sphere = sphere1;	// replace sphere
			continue;
		}

		// Otherwise make a larger sphere that contains both.
		sum			= sphere+sphere1;
		sphere		= 0.5f * ( sum + ( diff[3] / dist ) * diff );
		sphere[3]	= 0.5f * ( dist + sum[3] );
	}

	sphere[3] *= 2.0f;
	
	return sphere;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // Nx

				
