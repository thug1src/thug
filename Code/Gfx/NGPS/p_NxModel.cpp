//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxModel.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  12/21/2001
//****************************************************************************

#include <gfx/ngps/p_nxmodel.h>

#include <gfx/skeleton.h>
#include "gfx/nxgeom.h"

namespace Nx
{

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/
						
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
bool CPs2Model::plat_init_skeleton( int numBones )
{
	Dbg_Assert( mp_matrices == NULL );

	// matrix buffer is double-buffered
	mp_matrices = new Mth::Matrix[numBones * 2];
	for ( int i = 0; i < numBones * 2; i++ )
	{
		mp_matrices[i].Identity();
	}
    
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector CPs2Model::plat_get_bounding_sphere()
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

void CPs2Model::plat_set_bounding_sphere( const Mth::Vector& boundingSphere )
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
	
CPs2Model::CPs2Model()
{
	mp_matrices = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
CPs2Model::~CPs2Model()
{	
	if ( mp_matrices != NULL )
	{
		delete[] mp_matrices;
		mp_matrices = NULL;
	}
}

/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Matrix* CPs2Model::GetMatrices()
{
	return mp_matrices;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // Nx

				
