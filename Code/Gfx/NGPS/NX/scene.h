#ifndef __SCENE_H
#define __SCENE_H


#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <core/math.h>

namespace NxPs2
{


#define SCENEFLAG_INSTANCEABLE (1<<0)
#define SCENEFLAG_USESPIP      (1<<1)


struct sShadowVertex
{
	float	x;
	float	y;
	float	z;
	int		idx;
};

struct sShadowConnect
{
	uint16	corner[3];
};

struct sShadowNeighbor
{
	sint16	edge[3];
};

struct sShadowVolumeHeader
{
	uint32				version;
	uint32				num_faces;
	uint32				num_verts;
	uint32				byte_size;
	sShadowVertex *		p_vertex;
	sShadowConnect *	p_connect;
	sShadowNeighbor *	p_neighbor;
	uint32				pad;
};

struct sScene
{
	Mth::Vector			Sphere;

	uint32				Flags;
	int					NumGroups;

	int					NumTextures;
	uint8				*pTexBuffer;
	uint8				*pTexDma;
	struct sTexture		*pTextures;

	int					NumMaterials;
	struct sMaterial	*pMaterials;

	int 				NumMeshes;
	uint8 				*pMeshDma;
	struct sMesh		*pMeshes;
	class  CInstance	*pInstances;

	sScene				*pNext;

	static sScene		*pHead;
	static uint8		*pData;

	sShadowVolumeHeader	*mp_shadow_volume_header;
public:
	void SetUVOffset(uint32 material_name, int pass, float u_offset, float v_offset);
	void SetUVMatrix(uint32 material_name, int pass, const Mth::Matrix &mat);
};

struct sCASData;

sScene *LoadScene(uint32 *pData, int dataSize, sScene *pScene, bool IsSkin, bool Instanced, uint32 texDictOffset, bool doShadowVolume);
sScene *LoadScene(const char *Filename, sScene *pScene, bool IsSkin, bool Instanced, uint32 texDictOffset, bool doShadowVolume);
void DeleteScene(sScene *pScene);


} // namespace NxPs2


#endif // __SCENE_H

