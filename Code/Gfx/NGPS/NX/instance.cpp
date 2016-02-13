#include "instance.h"
#include "scene.h"
#include "dma.h"
#include "mesh.h"
#include <core/math.h>

namespace NxPs2
{


CInstance::CInstance(sScene *pScene, Mth::Matrix transform, bool color_per_material)
{
	SetTransform(transform);
	mp_bone_transforms = NULL;
	m_num_bones = 0;
	m_flags = INSTANCEFLAG_ACTIVE;
	mp_next = pScene->pInstances;
	pScene->pInstances = this;
	m_lastFrame = 0;
	m_field = 0;
	mp_scene = pScene;
	mp_light_group = NULL;
	if (color_per_material)
	{
		m_flags |= INSTANCEFLAG_COLORPERMATERIAL;
		mp_color_array = new uint32[mp_scene->NumMeshes];
		for (int i = 0; i < mp_scene->NumMeshes; i++)
		{
			mp_color_array[i] = 0x80808080;
		}
	}
	else
	{
		m_colour = 0x80808080;
	}
}


CInstance::CInstance(sScene *pScene, Mth::Matrix transform, bool color_per_material, int numBones, Mth::Matrix *pBoneTransforms)
{
	SetTransform(transform);
	mp_bone_transforms = pBoneTransforms;
	m_num_bones = numBones;
	m_flags = INSTANCEFLAG_ACTIVE;
	mp_next = pScene->pInstances;
	pScene->pInstances = this;
	m_lastFrame = 0;
	m_field = 0;
	mp_scene = pScene;
	mp_light_group = NULL;
	if (color_per_material)
	{
		m_flags |= INSTANCEFLAG_COLORPERMATERIAL;
		mp_color_array = new uint32[mp_scene->NumMeshes];
		for (int i = 0; i < mp_scene->NumMeshes; i++)
		{
			mp_color_array[i] = 0x80808080;
		}
	}
	else
	{
		m_colour = 0x80808080;
	}
}


CInstance::~CInstance()
{
	if (HasColorPerMaterial())
	{
		delete [] mp_color_array;
	}

	// remove from list structure...
	// head of instance list
	if (mp_scene->pInstances == this)
	{
		mp_scene->pInstances = mp_next;
	}
	// body of instance list
	else
	{
		CInstance *pInstance;
		for (pInstance=mp_scene->pInstances; pInstance; pInstance=pInstance->mp_next)
			if (pInstance->mp_next == this)
				break;
		Dbg_MsgAssert(pInstance, ("couldn't locate CInstance for deletion"));
		pInstance->mp_next = mp_next;
	}
}


void CInstance::SetActive(bool Active)
{
	if (Active)
	{
		m_flags |= INSTANCEFLAG_ACTIVE;
	}
	else
	{
		m_flags &= ~INSTANCEFLAG_ACTIVE;
	}
}

void CInstance::EnableShadow(bool Enabled)
{
	if (Enabled)
	{
		m_flags |= INSTANCEFLAG_CASTSSHADOW;
	}
	else
	{
		m_flags &= ~INSTANCEFLAG_CASTSSHADOW;
	}
}

void CInstance::SetWireframe(bool Wireframe)
{
	if (Wireframe)
	{
		m_flags |= INSTANCEFLAG_WIREFRAME;
	}
	else
	{
		m_flags &= ~INSTANCEFLAG_WIREFRAME;
	}
}

Mth::Matrix* CInstance::GetBoneTransforms(void)
{
	return m_field ? mp_bone_transforms : ( mp_bone_transforms + m_num_bones );
}

void CInstance::SetBoneTransforms(Mth::Matrix* pMat)
{
	// GJ:  toggle field, but only the first time
	// (otherwise, the double-buffer gets messed
	// up if SetBoneTransforms() is called more
	// than once per frame...)
	uint8 thisFrame = ( render::Frame & 0xff );
	if ( m_lastFrame != thisFrame )
	{
		m_field = !m_field;
		m_lastFrame = thisFrame;
	}

	int startIndex = m_field ? 0 : m_num_bones;

	Mth::Matrix* pSrc = pMat;
	Mth::Matrix* pDst = mp_bone_transforms + startIndex;

	// copy the given matrices into the correct half of the matrix double-buffer
	for ( int i = 0; i < m_num_bones; i++ )
	{
		*pDst++ = *pSrc++;
	}
}


void CInstance::SetColor(uint32 rgba)
{
	Dbg_Assert(!HasColorPerMaterial());

	m_colour = rgba;
}

uint32 CInstance::GetColor()
{
	Dbg_Assert(!HasColorPerMaterial());

	return m_colour;
}

void CInstance::SetMaterialColor(uint32 material_name, int pass, uint32 rgba)
{
	Dbg_Assert(HasColorPerMaterial());

	int i;
	sMesh *pMesh;

	// Array is in mesh order
	for (pMesh = mp_scene->pMeshes, i = 0; i < mp_scene->NumMeshes; pMesh++, i++)
	{
		if ((pMesh->MaterialName==material_name) && (pMesh->Pass == pass))
		{
			mp_color_array[i] = rgba;
		}
	}
}

void CInstance::SetMaterialColorByIndex(int mesh_idx, uint32 rgba)
{
	Dbg_Assert(HasColorPerMaterial());
	Dbg_MsgAssert(mesh_idx < mp_scene->NumMeshes, ("Mesh index %d is out of range", mesh_idx));

	mp_color_array[mesh_idx] = rgba;
}

uint32 CInstance::GetMaterialColor(uint32 material_name, int pass)
{
	Dbg_Assert(HasColorPerMaterial());

	int i;
	sMesh *pMesh;

	// Array is in mesh order
	for (pMesh = mp_scene->pMeshes, i = 0; i < mp_scene->NumMeshes; pMesh++, i++)
	{
		if ((pMesh->MaterialName==material_name) && (pMesh->Pass == pass))
		{
			return mp_color_array[i];
		}
	}

	return 0x80808080;
}

uint32 CInstance::GetMaterialColorByIndex(int mesh_idx)
{
	Dbg_Assert(HasColorPerMaterial());

	return mp_color_array[mesh_idx];
}

Mth::Vector CInstance::GetBoundingSphere()
{

	if (m_flags & INSTANCEFLAG_EXPLICITSPHERE)
	{
		return m_bounding_sphere;
	}
	else
	{
		Mth::Vector sphere(mp_scene->Sphere);
		sphere[3] += 24.0f;
		return sphere;
	}
}

void CInstance::SetBoundingSphere(float x, float y, float z, float r)
{
	//printf( "Stub:  SetBoundingSphere %f %f %f %f\n", x, y, z, r );
	m_bounding_sphere.Set(x,y,z,r);
	m_flags |= INSTANCEFLAG_EXPLICITSPHERE;
}


void CInstance::SqueezeADC()
{
	int i, num_meshes = mp_scene->NumMeshes;
	sMesh *p_mesh = mp_scene->pMeshes;

	for (i=0; i<num_meshes; i++,p_mesh++)
	{
		dma::SqueezeADC(p_mesh->pSubroutine);
		dma::SqueezeNOP(p_mesh->pSubroutine);
	}
}



// uv manipulation for skins

void CInstance::SetUVOffset(uint32 material_name, int pass, float u_offset, float v_offset)
{
	mp_scene->SetUVOffset(material_name, pass, u_offset, v_offset);
}


void CInstance::SetUVMatrix(uint32 material_name, int pass, const Mth::Matrix &mat)
{
	mp_scene->SetUVMatrix(material_name, pass, mat);
}


} // namespace NxPs2

