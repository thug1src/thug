#ifndef __INSTANCE_H
#define __INSTANCE_H


#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include "scene.h"
#include <core\math.h>


namespace Script
{
	extern int GetInt(uint32, bool);
}

namespace NxPs2
{

// Forward declarations
class	CLightGroup;

#define INSTANCEFLAG_ACTIVE				(1<<0)
#define INSTANCEFLAG_CASTSSHADOW		(1<<1)
#define INSTANCEFLAG_EXPLICITSPHERE		(1<<2)
#define INSTANCEFLAG_WIREFRAME			(1<<3)
#define INSTANCEFLAG_COLORPERMATERIAL	(1<<4)

class CInstance
{
public:
	void SetTransform(Mth::Matrix transform) { m_transform = transform; }
	Mth::Matrix *GetTransform(void) { return &m_transform; }
	int GetNumBones(void) { return m_num_bones; }
	Mth::Matrix *GetBoneTransforms(void);
	void SetBoneTransforms(Mth::Matrix *pMat);
	CInstance *GetNext() { return mp_next; }
	sScene *GetScene() { return mp_scene; }
	void SetActive(bool Active);
	void EnableShadow(bool Enabled);
	void SetWireframe(bool Wireframe);
	bool IsActive(void) const; 
	bool CastsShadow(void) const; 
	bool IsWireframe(void) const; 
	bool HasColorPerMaterial(void) const; 
	void SetColor(uint32 rgba);
	void SetMaterialColor(uint32 material_name, int pass, uint32 rgba);
	void SetMaterialColorByIndex(int mesh_idx, uint32 rgba);
	uint32 GetColor();
	uint32 GetMaterialColor(uint32 material_name, int pass);
	uint32 GetMaterialColorByIndex(int mesh_idx);
	void SetLightGroup(CLightGroup *p_light_group);
	CLightGroup *GetLightGroup();
	Mth::Vector GetBoundingSphere();
	void SetBoundingSphere(float x, float y, float z, float r);
	void SqueezeADC();
	void SetUVOffset(uint32 material_name, int pass, float u_offset, float v_offset);
	void SetUVMatrix(uint32 material_name, int pass, const Mth::Matrix &mat);
	CInstance(sScene *pScene, Mth::Matrix transform, bool color_per_material);
	CInstance(sScene *pScene, Mth::Matrix transform, bool color_per_material, int numBones, Mth::Matrix *pBoneTransforms);
	~CInstance();

private:
	Mth::Matrix m_transform;
	Mth::Matrix *mp_bone_transforms;
	Mth::Vector m_bounding_sphere;
	uint32 m_flags;
	union
	{
		uint32 m_colour;
		uint32 *mp_color_array;			// Use this only when INSTANCEFLAG_COLORPERMATERIAL is set; in Mesh order
	};
	int m_num_bones;
	CInstance *mp_next;
	sScene *mp_scene;
	CLightGroup *mp_light_group;		// optional group of lights
	uint8 m_lastFrame;
	char m_field;
};


//  IsActive needs to be inline, otherwise it takes 1% of rendering cpu time
inline bool CInstance::IsActive(void) const
{
	return (m_flags & INSTANCEFLAG_ACTIVE) ? true : false;
}

inline bool CInstance::CastsShadow(void) const
{
	return (m_flags & INSTANCEFLAG_CASTSSHADOW) ? true : false;
}

inline bool CInstance::IsWireframe(void) const
{
	#ifdef	__NOPT_ASSERT__
	if (Script::GetInt(CRCD(0xb4ff073e,"wireframe_skins"),false))
	{
		return true;
	}
	#endif
	return (m_flags & INSTANCEFLAG_WIREFRAME) ? true : false;
}

inline bool CInstance::HasColorPerMaterial(void) const
{
	return m_flags & INSTANCEFLAG_COLORPERMATERIAL;
}

inline void CInstance::SetLightGroup(CLightGroup *p_light_group)
{
	mp_light_group = p_light_group;
}

inline CLightGroup * CInstance::GetLightGroup()
{
	return mp_light_group;
}

} // namespace NxPs2


#endif // __INSTANCE_H

