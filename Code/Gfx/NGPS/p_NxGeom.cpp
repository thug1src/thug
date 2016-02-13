//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxGeom.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  3/4/2002
//****************************************************************************

#include <gfx/ngps/p_nxgeom.h>

#include <gel/scripting/array.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/vecpair.h>
#include <gel/scripting/symboltable.h>
								
#include <gfx/skeleton.h>
#include <gfx/ngps/p_nxlight.h>
#include <gfx/ngps/p_nxmesh.h>
#include <gfx/ngps/p_nxmodel.h>
#include <gfx/ngps/p_nxscene.h>
#include <gfx/ngps/p_nxtexture.h>
#include <gfx/ngps/nx/geomnode.h>
#include <gfx/ngps/nx/instance.h>
#include <gfx/ngps/nx/group.h>
#include <gfx/ngps/nx/geomnode.h>

#include <gel/collision/collision.h>

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

CPs2Geom::CPs2Geom()
{
	// Machine specific code here ............
	mp_oldInstance = NULL;
	mp_instance = NULL;
	mp_matrices = NULL;
	m_rootMatrix.Ident();	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Geom::~CPs2Geom()
{
	if ( mp_oldInstance != NULL )
	{
		delete mp_oldInstance;
		mp_oldInstance = NULL;
	}

	if ( mp_instance != NULL )
	{
		if ( m_cloned == vINSTANCE )
		{
			mp_instance->DeleteInstance();
		}
		else if ( m_cloned == vCOPY )
		{
			mp_instance->DeleteCopy();
		}
		mp_instance = NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPs2Geom::plat_load_geom_data(CMesh* pMesh, CModel* pModel, bool color_per_material)
{
	int numBones;
	numBones = pModel->GetNumBones();

	Mth::Matrix temp;
	temp.Identity();
	Dbg_Assert( mp_oldInstance == NULL );

	// attempting to move skins over to .geom.ps2
	mp_matrices = ((CPs2Model*)pModel)->GetMatrices();
    if ( ((CPs2Mesh*)pMesh)->GetGeomNode() )
	{
		if ( numBones )
		{
			mp_instance = ((CPs2Mesh*)pMesh)->GetGeomNode()->CreateInstance( &m_rootMatrix, numBones, (Mth::Matrix *)mp_matrices );
		}
		else
		{
			mp_instance = ((CPs2Mesh*)pMesh)->GetGeomNode()->CreateInstance( &m_rootMatrix );
		}
	}
	else
	{
		Dbg_Assert( pMesh );
		Dbg_Assert( pMesh->GetTextureDictionary() );
		NxPs2::sScene* pScene = ((Nx::CPs2TexDict*)pMesh->GetTextureDictionary())->GetEngineTextureDictionary();
		Dbg_Assert( pScene );

		if ( numBones )
		{
			mp_oldInstance = new NxPs2::CInstance(pScene, temp, color_per_material, numBones, mp_matrices );
		}
		else
		{
			mp_oldInstance = new NxPs2::CInstance(pScene, temp, color_per_material);
		}
	}

	// remember the source mesh, so that we can do poly-hiding on it
	mp_sourceMesh = (CPs2Mesh*)pMesh;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_finalize()
{
	if (mp_oldInstance)
	{
		// only supported on old-style instances
		mp_oldInstance->SqueezeADC();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGeom *	CPs2Geom::plat_clone(bool instance, CScene* pDestScene)
{
	Dbg_MsgAssert(mp_oldInstance == NULL, ("Wrong version of CPs2Geom::plat_clone() for CInstances"));
	Dbg_MsgAssert(mp_matrices == NULL, ("Wrong version of CPs2Geom::plat_clone() for matrix arrays"));

	CPs2Scene *p_ps2_dest_scene = static_cast<CPs2Scene *>(pDestScene);
	NxPs2::CGeomNode *p_scene_geom = (p_ps2_dest_scene) ? p_ps2_dest_scene->GetEngineCloneScene() : NULL;

	// Copy into new sector
	CPs2Geom *p_new_geom = new CPs2Geom(*this);

	// Copy engine data
	if (p_new_geom && mp_instance)
	{
		if (instance)
		{
			p_new_geom->mp_instance = mp_instance->CreateInstance(&p_new_geom->m_rootMatrix, p_scene_geom);
		}
		else
		{
			p_new_geom->mp_instance = mp_instance->CreateCopy(p_scene_geom);
			//p_new_geom->mp_instance->SetMatrix(&p_new_geom->m_rootMatrix);
		}
	}

	return p_new_geom;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGeom *	CPs2Geom::plat_clone(bool instance, CModel* pDestModel)
{
	Dbg_MsgAssert(mp_instance == NULL, ("Wrong version of CPs2Geom::plat_clone() for CGeomNodes"));
	Dbg_Assert(mp_oldInstance);

	// Copy into new geom
	CPs2Geom *p_new_geom = new CPs2Geom(*this);

	Dbg_Assert(p_new_geom);

	int numBones;
	numBones = pDestModel->GetNumBones();

	Mth::Matrix temp;
	temp.Identity();

	p_new_geom->mp_matrices = ((CPs2Model*)pDestModel)->GetMatrices();

	NxPs2::sScene* pScene = mp_oldInstance->GetScene();
	Dbg_Assert( pScene );

	if ( numBones )
	{
		p_new_geom->mp_oldInstance = new NxPs2::CInstance(pScene, temp, mp_oldInstance->HasColorPerMaterial(), numBones, p_new_geom->mp_matrices );
	}
	else
	{
		p_new_geom->mp_oldInstance = new NxPs2::CInstance(pScene, temp, mp_oldInstance->HasColorPerMaterial());
	}

	Dbg_Assert(p_new_geom->mp_oldInstance);

	// Copy the colors from instance
	if (mp_oldInstance->HasColorPerMaterial())
	{
		int num_colors = mp_oldInstance->GetScene()->NumMeshes;
		for (int i = 0; i < num_colors; i++)
		{
			p_new_geom->mp_oldInstance->SetMaterialColorByIndex(i, mp_oldInstance->GetMaterialColorByIndex(i));
		}
	}
	else
	{
		p_new_geom->mp_oldInstance->SetColor(mp_oldInstance->GetColor());
	}

	return p_new_geom;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CPs2Geom::plat_get_checksum()
{
	Dbg_Assert(mp_instance);

	return mp_instance->GetChecksum();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_set_color(Image::RGBA rgba)
{
	// Engine call here
	if ( mp_instance )
	{
		mp_instance->SetColored(true);
//		printf( "Color 0x%x\n", *((uint32 *) &rgba ));
		mp_instance->SetColor(*((uint32 *) &rgba));
	}
	else if (mp_oldInstance)
	{
		if (mp_oldInstance->HasColorPerMaterial())
		{
			for (int i = 0; i < mp_oldInstance->GetScene()->NumMeshes; i++)
			{
				mp_oldInstance->SetMaterialColorByIndex(i, *((uint32 *) &rgba));
			}
		}
		else
		{
			mp_oldInstance->SetColor(*((uint32 *) &rgba));
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA CPs2Geom::plat_get_color() const
{
	// Engine call here
	if ( mp_instance && mp_instance->IsColored() )
	{
		uint32 raw_data = mp_instance->GetColor();
		return *((Image::RGBA *) &raw_data);
	}
	else if (mp_oldInstance)
	{
		uint32 raw_data = mp_oldInstance->GetColor();
		return *((Image::RGBA *) &raw_data);
	}
	else
	{
		return Image::RGBA(128, 128, 128, 128);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_clear_color()
{
	// Engine call here
	if ( mp_instance )
	{
		mp_instance->SetColored(false);
	} else {
		// Set to white
		plat_set_color(Image::RGBA(128, 128, 128, 128));
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CPs2Geom::plat_set_material_color(uint32 mat_checksum, int pass, Image::RGBA rgba)
{
	if (mp_oldInstance)
	{
		if (mp_oldInstance->HasColorPerMaterial())
		{
			mp_oldInstance->SetMaterialColor(mat_checksum, pass, *((uint32 *) &rgba));
		}
		else
		{
//			Dbg_Message( "%s %d doesn't have multicolor set", Script::FindChecksumName(mat_checksum), pass );
		}
		return true;
	}
	else
	{
		Dbg_MsgAssert(0, ("Trying to set the material color on a Geom that doesn't support it"));
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA	CPs2Geom::plat_get_material_color(uint32 mat_checksum, int pass)
{
	if (mp_oldInstance && mp_oldInstance->HasColorPerMaterial())
	{
		uint32 raw_data = mp_oldInstance->GetMaterialColor(mat_checksum, pass);
		return *((Image::RGBA *) &raw_data);
	}
	else
	{
		Dbg_MsgAssert(0, ("Trying to get the material color on a Geom that doesn't support it"));
		return Image::RGBA(128, 128, 128, 128);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_set_visibility(uint32 mask)
{
	// Engine call here
	if (mp_instance)
	{
		mp_instance->SetVisibility(mask & 0xFF);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CPs2Geom::plat_get_visibility() const
{
	// Engine call here
	if ( mp_instance )
	{
		return mp_instance->GetVisibility() | 0xFFFFFF00;		// To keep format compatible
	}
	else
	{
		return 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_set_active( bool active )
{
	if ( mp_oldInstance )
	{
		if ( active != mp_oldInstance->IsActive() )
		{
			mp_oldInstance->SetActive( active );
		}
	}

	// New engine
	if ( mp_instance )
	{
		if ( active != mp_instance->IsActive() )
		{
			mp_instance->SetActive( active );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPs2Geom::plat_is_active() const
{
	if ( mp_instance )
	{
		return mp_instance->IsActive();
	}
	else
	{
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::CBBox & CPs2Geom::plat_get_bounding_box() const
{
#if 0	// Enable this after THPS4 and do something about needing a static
	if ( mp_instance )
	{
		static Mth::CBBox bbox;
		bbox = mp_instance->GetBoundingBox();
		return bbox;
	} 
	else
#endif
	{
		// Garrett: TODO: change to renderable bounding box
		Dbg_Assert(mp_coll_tri_data);
		return mp_coll_tri_data->GetBBox();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Vector CPs2Geom::plat_get_bounding_sphere() const
{
	if ( mp_instance )
	{
		// CGeomNode-style
		return mp_instance->GetBoundingSphere();
	}
	else if ( mp_oldInstance )
	{
		// CInstance-style
		return mp_oldInstance->GetBoundingSphere();
	}
	else
	{
		return Mth::Vector(0.0f, 0.0f, 0.0f, 1.0e+10f);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_set_bounding_sphere( const Mth::Vector& boundingSphere )
{
	if ( mp_instance )
	{
		// CGeomNode-style
		mp_instance->SetBoundingSphere( boundingSphere[X], boundingSphere[Y], boundingSphere[Z], boundingSphere[W] );
	}
	else if ( mp_oldInstance )
	{
		// CInstance-style
		mp_oldInstance->SetBoundingSphere( boundingSphere[X], boundingSphere[Y], boundingSphere[Z], boundingSphere[W] );
	}
	else
	{
		Dbg_MsgAssert( 0, ( "Shouldn't get here" ) );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_set_world_position(const Mth::Vector& pos)
{
	// Garrett: we may need to do this in integer format instead to make it lossless
	Mth::Vector delta_pos(pos - m_rootMatrix[W]);

	// Set values
	m_rootMatrix[W][X] = pos[X];
	m_rootMatrix[W][Y] = pos[Y];
	m_rootMatrix[W][Z] = pos[Z];	
	//m_rootMatrix[W][W] = 1.0f;

	// Engine call here
	if (m_cloned != vCOPY)
	{
		update_engine_matrix();
	} else {
		mp_instance->Translate(delta_pos);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Vector &	CPs2Geom::plat_get_world_position() const
{
	return m_rootMatrix[W];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_set_orientation(const Mth::Matrix& orient)
{
	// Set values
	m_rootMatrix[X] = orient[X];
	m_rootMatrix[Y] = orient[Y];
	m_rootMatrix[Z] = orient[Z];

	// Engine call here
	if (m_cloned != vCOPY)
	{
		update_engine_matrix();
	} else {
		Dbg_MsgAssert(0, ("Don't call SetOrientation() on a CGeom copy"));
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Matrix &	CPs2Geom::plat_get_orientation() const
{
	Dbg_MsgAssert(0, ("CPs2Geom::plat_get_orientation() not implemented yet"));
	return m_rootMatrix;			// This is not correct
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_rotate_y(Mth::ERot90 rot)
{
	// Engine call here
	// Garrett: TEMP just set the world matrix
	Mth::Matrix orig_rot_mat(m_rootMatrix), rot_mat;
	float rad = (float) ((int) rot) * (Mth::PI * 0.5f);
	CreateRotateYMatrix(rot_mat, rad);

	orig_rot_mat[W] = Mth::Vector(0.0f, 0.0f, 0.0f, 1.0f);

	rot_mat = orig_rot_mat * rot_mat;
	//Dbg_Message("[X] (%f, %f, %f, %f)", rot_mat[X][X], rot_mat[X][Y],  rot_mat[X][Z],  rot_mat[X][W]);
	//Dbg_Message("[Y] (%f, %f, %f, %f)", rot_mat[Y][X], rot_mat[Y][Y],  rot_mat[Y][Z],  rot_mat[Y][W]);
	//Dbg_Message("[Z] (%f, %f, %f, %f)", rot_mat[Z][X], rot_mat[Z][Y],  rot_mat[Z][Z],  rot_mat[Z][W]);
	//Dbg_Message("[W] (%f, %f, %f, %f)", rot_mat[W][X], rot_mat[W][Y],  rot_mat[W][Z],  rot_mat[W][W]);
	
	m_rootMatrix[X] = rot_mat[X];
	m_rootMatrix[Y] = rot_mat[Y];
	m_rootMatrix[Z] = rot_mat[Z];

	if (m_cloned != vCOPY)
	{
		update_engine_matrix();
	}
	else
	{
		// Garrett: We may need to change the world pos to integer to avoid float losses
		mp_instance->RotateY(m_rootMatrix[W], rot);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_set_transform(const Mth::Matrix& transform)
{
	// Set values
	m_rootMatrix = transform;

	// Engine call here
	if (m_cloned != vCOPY)
	{
		update_engine_matrix();
	}
	else
	{
		Dbg_MsgAssert(0, ("Don't call SetTransform() on a CGeom copy"));
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Matrix &	CPs2Geom::plat_get_transform() const
{
	return m_rootMatrix;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_set_scale(const Mth::Vector & scale)
{
	// Engine call here
	Mth::Matrix orig_rot_mat(m_rootMatrix);

	// orientation
	m_rootMatrix[X] *= scale[X];
	m_rootMatrix[Y] *= scale[Y];
	m_rootMatrix[Z] *= scale[Z];

#if 0		// Don't do this now since it is a local space scale
	// position (since this is what will happen in the copy geometry)
	m_rootMatrix[Mth::POS][X] *= scale[X];
	m_rootMatrix[Mth::POS][Y] *= scale[Y];
	m_rootMatrix[Mth::POS][Z] *= scale[Z];
#endif

	if (m_cloned != vCOPY)
	{
		update_engine_matrix();
	}
	else
	{
		Mth::Vector delta_scale;

		delta_scale[X] = scale[X] /  orig_rot_mat[X].Length();
		delta_scale[Y] = scale[Y] /  orig_rot_mat[Y].Length();
		delta_scale[Z] = scale[Z] /  orig_rot_mat[Z].Length();

		mp_instance->Scale(m_rootMatrix[W], delta_scale);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Mth::Vector CPs2Geom::plat_get_scale() const
{
	Mth::Vector scale;

	scale[X] = m_rootMatrix[X].Length();
	scale[Y] = m_rootMatrix[Y].Length();
	scale[Z] = m_rootMatrix[Z].Length();

	return scale;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::update_engine_matrix()
{
	if (m_cloned != vCOPY)
	{
		mp_instance->SetMatrix(&m_rootMatrix);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// not needed anymore
#if 0
void ConvertMatrix(Mth::Matrix* pMatrix, Mth::Matrix* pMat)
{
	/*
	// right
	(*pMat)[0][0] = (*pMatrix)[0][0];
	(*pMat)[0][1] = (*pMatrix)[0][1];
	(*pMat)[0][2] = (*pMatrix)[0][2];
	(*pMat)[0][3] = (*pMatrix)[0][3];

	// up
	(*pMat)[1][0] = (*pMatrix)[1][0];
	(*pMat)[1][1] = (*pMatrix)[1][1];
	(*pMat)[1][2] = (*pMatrix)[1][2];
	(*pMat)[1][3] = (*pMatrix)[1][3];

	// at
	(*pMat)[2][0] = (*pMatrix)[2][0];
	(*pMat)[2][1] = (*pMatrix)[2][1];
	(*pMat)[2][2] = (*pMatrix)[2][2];
	(*pMat)[2][3] = (*pMatrix)[2][3];

	// position
	(*pMat)[3][0] = (*pMatrix)[3][0];
	(*pMat)[3][1] = (*pMatrix)[3][1];
	(*pMat)[3][2] = (*pMatrix)[3][2];
	(*pMat)[3][3] = (*pMatrix)[3][3];

	*/
	
	// the copy operator is more efficient
	// (on PS2, does 4 quad copies.  Other platform does the same as above
	*pMat  = *pMatrix;				 

	// clear out the final column
	(*pMat)[0][3] = 0.0f;//(*pMatrix)[3][0];
	(*pMat)[1][3] = 0.0f;//(*pMatrix)[3][0];
	(*pMat)[2][3] = 0.0f;//(*pMatrix)[3][0];
	(*pMat)[3][3] = 1.0f;//(*pMatrix)[3][0];
		
	#if 0
	// right
	(*pMat)[0][0] = 1.0f;//(*pMatrix)[0][0];
	(*pMat)[0][1] = 0.0f;//(*pMatrix)[1][0];
	(*pMat)[0][2] = 0.0f;//(*pMatrix)[2][0];
	(*pMat)[0][3] = 0.0f;//(*pMatrix)[3][0];

	// up
	(*pMat)[1][0] = 0.0f;//(*pMatrix)[0][1];
	(*pMat)[1][1] = 1.0f;//(*pMatrix)[1][1];
	(*pMat)[1][2] = 0.0f;//(*pMatrix)[2][1];
	(*pMat)[1][3] = 0.0f;//(*pMatrix)[3][1];

	// at
	(*pMat)[2][0] = 0.0f;//(*pMatrix)[0][2];
	(*pMat)[2][1] = 0.0f;//(*pMatrix)[1][2];
	(*pMat)[2][2] = 1.0f;//(*pMatrix)[2][2];
	(*pMat)[2][3] = 0.0f;//(*pMatrix)[3][2];

	// position
//	(*pMat)[3][0] = (*pMatrix)[3][0];
//	(*pMat)[3][1] = (*pMatrix)[3][1];
//	(*pMat)[3][2] = (*pMatrix)[3][2];
//	(*pMat)[3][3] = 1.0f;//(*pMatrix)[3][3];
	#endif
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPs2Geom::plat_render(Mth::Matrix* pRootMatrix, Mth::Matrix* pBoneMatrices, int numBones)
{
	// implies it's got a skin
	if ( mp_oldInstance )
	{
		Dbg_Assert( mp_oldInstance );
		
		if ( numBones > 0 )
		{
			mp_oldInstance->SetBoneTransforms( pBoneMatrices ); 
		}

		mp_oldInstance->SetTransform(*pRootMatrix);
	
		Dbg_MsgAssert( numBones <= mp_oldInstance->GetNumBones(), ( "Bone mismatch Trying to render %d bones, but model was initialized with %d bones", 
																	numBones, 
																	mp_oldInstance->GetNumBones() ) );
	}

	if ( mp_instance )
	{
		m_rootMatrix = *pRootMatrix;

		if ( numBones )
		{
			mp_instance->SetBoneTransforms( pBoneMatrices );
		}
		
		Dbg_MsgAssert( numBones <= mp_instance->GetNumBones(), ( "Bone mismatch Trying to render %d bones, but model was initialized with %d bones", 
																	numBones, 
																	mp_instance->GetNumBones() ) );
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPs2Geom::plat_hide_polys( uint32 mask )
{
	if ( mp_instance )
	{
	// Only supporting old style geom for now
	//	mp_instance->HidePolys( mask );
	}
	else
	{
		Dbg_Assert( mp_sourceMesh );
		NxPs2::CGeomNode* pGeomNode = mp_sourceMesh->GetGeomNode();
		if ( pGeomNode )
		{
		// Only supporting old style geom for now
		//	pGeomNode->HidePolys( mask );
		}
		else
		{
			mp_sourceMesh->HidePolys( mask );
		}
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPs2Geom::plat_enable_shadow( bool enabled )
{
	if ( mp_instance )
	{
		// only supporting old style geom for now
	}
	else
	{
		mp_oldInstance->EnableShadow( enabled );
	}
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_set_model_lights(CModelLights *p_model_lights)
{
	NxPs2::CLightGroup *p_light_group;
	if (p_model_lights)
	{
		CPs2ModelLights *p_ps2_model_lights;
		p_ps2_model_lights = static_cast<CPs2ModelLights *>(p_model_lights);
		p_light_group = p_ps2_model_lights->GetEngineLightGroup();
	} else {
		p_light_group = NULL;
	}


	// Model lights only work on CGeomNodes
	if (mp_instance)
	{
		Dbg_Assert(!mp_instance->IsLeaf());
		mp_instance->SetLightGroup(p_light_group);
	}
	else if (mp_oldInstance)
	{
		mp_oldInstance->SetLightGroup(p_light_group);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CPs2Geom::plat_get_num_render_verts()
{
	Dbg_Assert(mp_instance);

	return mp_instance->GetNumVerts();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_get_render_verts(Mth::Vector *p_verts)
{
	Dbg_Assert(mp_instance);

	mp_instance->GetVerts(p_verts);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CPs2Geom::plat_get_num_render_polys()								
{
	return mp_instance->GetNumPolys();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CPs2Geom::plat_get_num_render_base_polys()								
{
	return mp_instance->GetNumBasePolys();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_get_render_colors(Image::RGBA *p_colors)
{
	Dbg_Assert(mp_instance);

	mp_instance->GetColors((uint32 *) p_colors);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_set_render_verts(Mth::Vector *p_verts)
{
	Dbg_Assert(mp_instance);

	mp_instance->SetVerts(p_verts);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_set_render_colors(Image::RGBA *p_colors)
{
	Dbg_Assert(mp_instance);

	mp_instance->SetColors((uint32 *) p_colors);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_set_uv_wibble_params(float u_vel, float u_amp, float u_freq, float u_phase,
										 float v_vel, float v_amp, float v_freq, float v_phase)
{
	Dbg_Assert(mp_instance);

	NxPs2::CGeomNode *p_node = mp_instance;

	// Get leaf
	while (!p_node->IsLeaf())
	{
		p_node = p_node->GetChild();
	}

	// Do on all siblings
	while (p_node)
	{
		if (p_node->IsUVWibbled())
		{
			p_node->SetUVWibbleParams(u_vel, u_amp, u_freq, u_phase, v_vel, v_amp, v_freq, v_phase);
		}

		p_node = p_node->GetSibling();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_use_explicit_uv_wibble(bool yes)
{
	Dbg_Assert(mp_instance);

	NxPs2::CGeomNode *p_node = mp_instance;

	// Get leaf
	while (!p_node->IsLeaf())
	{
		p_node = p_node->GetChild();
	}

	// Do on all siblings
	while (p_node)
	{
		if (p_node->IsUVWibbled())
		{
			p_node->UseExplicitUVWibble(yes);
		}

		p_node = p_node->GetSibling();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Geom::plat_set_uv_wibble_offsets(float u_offset, float v_offset)
{
	Dbg_Assert(mp_instance);

	NxPs2::CGeomNode *p_node = mp_instance;

	// Get leaf
	while (!p_node->IsLeaf())
	{
		p_node = p_node->GetChild();
	}

	// Do on all siblings
	while (p_node)
	{
		if (p_node->IsUVWibbled())
		{
			p_node->SetUVWibbleOffsets(u_offset, v_offset);
			// If we are setting the offsets, then we should want to use the explicit mode
			p_node->UseExplicitUVWibble(true);
		}

		p_node = p_node->GetSibling();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPs2Geom::plat_set_uv_wibble_offsets(uint32 mat_checksum, int pass, float u_offset, float v_offset)
{
	if (mp_oldInstance)
	{
		mp_oldInstance->SetUVOffset(mat_checksum, pass, u_offset, v_offset);
		return true;
	}
	else
	{
		Dbg_MsgAssert(0, ("Can't set the UV offset with material checksum on CGeomNode"));
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPs2Geom::plat_set_uv_matrix(uint32 mat_checksum, int pass, const Mth::Matrix &mat)
{
	if (mp_oldInstance)
	{
		mp_oldInstance->SetUVMatrix(mat_checksum, pass, mat);
		return true;
	}
	else
	{
		Dbg_MsgAssert(0, ("Can't set the UV matrix with material checksum on CGeomNode"));
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // Nx
				
