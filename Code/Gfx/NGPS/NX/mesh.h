#ifndef __MESH_H
#define __MESH_H


#include <core/math.h>

namespace Nx
{
	struct SMeshScalingParameters;
}
					  
namespace NxPs2
{


#define MESHFLAG_TEXTURE		(1<<0)
#define MESHFLAG_COLOURS		(1<<1)
#define MESHFLAG_NORMALS		(1<<2)
#define MESHFLAG_ST16			(1<<3)
#define MESHFLAG_SKINNED		(1<<4)
#define MESHFLAG_AXIAL			(1<<6)
#define MESHFLAG_SHORTAXIS		(1<<7)
#define MESHFLAG_PASS_BIT_0		(1<<8)
#define MESHFLAG_PASS_BIT_1		(1<<9)
#define MESHFLAG_NO_SHADOW		(1<<10)
#define MESHFLAG_ACTIVE			(1<<16)
#define MESHFLAG_UNLIT			(1<<17)
#define MESHFLAG_COLOR_LOCKED	(1<<18)
#define MESHFLAG_BILLBOARD		(1<<23)
#define MESHFLAG_SINGLESIDED	(1<<24)
#define MESHFLAG_PASS_BIT_2		(1<<25)
#define MESHFLAG_PASS_BIT_3		(1<<26)

#define MESHFLAG_PASS_BITS		(MESHFLAG_PASS_BIT_0 | MESHFLAG_PASS_BIT_1 | MESHFLAG_PASS_BIT_2 | MESHFLAG_PASS_BIT_3)


#define OBJFLAG_TEXTURE			(1<<0)
#define OBJFLAG_COLOURS			(1<<1)
#define OBJFLAG_NORMALS			(1<<2)
#define OBJFLAG_TRANSPARENT		(1<<3)


struct sGroup;
struct sMaterial;

struct sCASData
{
	uint32	mask;
//	int		vertIndex;	
	uint16* pADCBit;
};

void CreateCASDataLookupTable();
void DestroyCASDataLookupTable();
void SetCASDataLookupData( int index, sCASData* pData );

void AddCASFlag(int skinnedVertexIndex, uint16* pADCBit);

struct sMesh
{
public:
	uint32			GetChecksum() const { return Checksum; }
	uint32			GetFlags() const { return Flags; }
	void			SetActive(bool active);
	bool			IsActive(void);

	uint32 Checksum, Flags;
	sMaterial *pMaterial;
	uint8 *pSubroutine;
	Mth::Vector ObjBox,Box,Sphere;
	uint32*	pVCWibbleInfo;
	int Pass;
	uint32 MaterialName;

	static int TotalNumVertices;
	static uint32 Version;
};

// GJ:  for doing cutscene head scaling
void SetMeshScalingParameters( Nx::SMeshScalingParameters* pParams );

void * LoadMeshes(void *pFile, struct sScene *pScene, bool IsSkin, bool IsInstanceable, uint32 texDictOffset);
void * LoadMeshGroup(void *pFile, struct sScene *pScene, bool IsSkin, bool IsInstanceable, uint32 texDictOffset, int& skinnedVertexIndex );
void * LoadVertices(void *pFile, sMesh *pMesh, struct sMaterial *pMat, sGroup *pGroup, int& skinnedVertexIndex);
void BeginModelPrim(uint32 Regs, uint NReg, uint Prim, uint Pre, uint Addr);
void EndModelPrim(uint Eop);
void BeginModelPrimImmediate(uint32 Regs, uint NReg, uint Prim, uint Pre, uint Addr);
void EndModelPrimImmediate(uint Eop);
void BeginModelMultiPrim(uint32 Regs, uint NReg, uint Prim, uint NLoop, uint Addr);
void EndModelMultiPrim(void);
void BeginVertex(sMesh *pMesh, struct sMaterial *pMat);
void VertexST16(uint8 *pData);
void VertexSTFloat(uint8 *pData);
void VertexUV(uint8 *pData, int textureWidth16, int textureHeight16);
void VertexNormal(uint8 *pData);
void VertexWeights(uint8 *pData);
void VertexSkinNormal(uint8 *pNormData, uint8 *pTOData);
void VertexRGBA(uint8 *pData);
void VertexXYZ(uint8 *pData, bool IsSkin, int& skinnedVertexIndex);
void EndVertex(void);


extern uint NumMeshesInGroup[32];


} // namespace NxPs2

#endif // __MESH_H

