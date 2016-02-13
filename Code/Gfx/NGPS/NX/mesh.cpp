#include <core/defines.h>
#include <core/math.h>
#include <stdio.h>
#include <stdlib.h>
#include "group.h"
#include "scene.h"
#include "dma.h"
#include "vif.h"
#include "vu1.h"
#include "gif.h"
#include "gs.h"
#include "texture.h"
#include "material.h"
#include "mesh.h"
#include "vu1code.h"
#include "dmacalls.h"
#include "render.h"
#include <sys/file/filesys.h>
#include <sys/file/memfile.h>
#include <sys/mem/memman.h>
#include <gfx/nx.h>
										  
namespace NxPs2
{


#ifdef	__NOPT_ASSERT__
extern uint32		gPassMask1; 		// 1<<6 | 1<<7  (0x40, 0x80)
extern uint32		gPassMask0; 		// 1<<6 | 1<<7  (0x40, 0x80)
#endif 


#if 0

//----------------------------------------------------------------------------------------
//		Mesh Group Functions
//----------------------------------------------------------------------------------------

sMesh *sGroup::GetMeshArray()
{
	return &(Meshes[FirstMeshIndex]);
}


sGroup* NewMeshGroup(uint32 group_ID, uint32 num_meshes)
{
	uint32 i;
	int Group = -1;

	// resolve ID
	for (i=0; i<NumTextureGroups; i++)
	{
		if (Groups[i].ID == group_ID)
		{
			Group = i;
			break;
		}
	}
	Dbg_MsgAssert(Groups[i].ID==group_ID, ("Couldn't find group ID #%d\n", group_ID));

	// get number of meshes
	// set group info
	Groups[Group].NumMeshes = num_meshes;
	Groups[Group].FirstMeshIndex = NumMeshes;

	NumMeshes += num_meshes;

	return &(Groups[Group]);
}

#endif


//----------------------------------------------------------------------------------------
//		Mesh scaling parameters
//		GJ:  Used for vertex scaling at load-time (needed for cutscene head scaling)
//		I decided to make global functions to set these from the cutscene code,
//		so that I wouldn't have to pass extra parameters through the various loading
//		functions
//----------------------------------------------------------------------------------------

bool			s_meshScalingEnabled = false;
char*			s_pWeightIndices = NULL;
float*			s_pWeights = NULL;
Mth::Vector*	s_pBonePositions = NULL;
Mth::Vector*	s_pBoneScales = NULL;
int				s_currentVertIndex = 0;

void SetMeshScalingParameters( Nx::SMeshScalingParameters* pParams )
{
	Dbg_Assert( pParams );

	s_meshScalingEnabled = true;
	s_pWeights = pParams->pWeights;
	s_pWeightIndices = pParams->pWeightIndices;
	s_pBoneScales = pParams->pBoneScales;
	s_pBonePositions = pParams->pBonePositions;
	s_currentVertIndex = 0;
}

void DisableMeshScaling()
{
	s_meshScalingEnabled = false;
	s_pWeights = NULL;
	s_pWeightIndices = NULL;
	s_pBoneScales = NULL;
	s_pBonePositions = NULL;
	s_currentVertIndex = 0;
}

//----------------------------------------------------------------------------------------
//		L O A D   M E S H E S
//----------------------------------------------------------------------------------------

void * LoadMeshes(void *pFile, sScene *pScene, bool IsSkin, bool IsInstanceable, uint32 texDictOffset)
{
	int i;

	// initialise sScene bounding sphere to avoid rewriting it multiple times
	pScene->Sphere[3] = -1.0f;

	int skinnedVertexCount=0;

	// iterate over mesh groups
	for (i=0; i<pScene->NumGroups; i++)
	{
		pFile = LoadMeshGroup(pFile, pScene, IsSkin, IsInstanceable, texDictOffset, skinnedVertexCount);
	}
	// Check hierarchy data
	int num_hier = -1;
	MEM_Read(&num_hier, sizeof(int), 1, pFile);
	Dbg_MsgAssert(num_hier == 0, ("num_hier = %d", num_hier));
	
	// clear it out for future use
	DisableMeshScaling();
	
	return pFile;
}




void *LoadMeshGroup(void *pFile, sScene *pScene, bool IsSkin, bool IsInstanceable, uint32 texDictOffset, int& skinnedVertexCount)
{
	sMesh *pMesh;
	sMaterial *pMat;
	sGroup *pGroup;
	uint32 MaterialChecksum, GroupChecksum;
	int NumMeshesThisGroup,i,j;
	Mth::Vector sphere;

	// get group checksum and number of meshes
	MEM_Read(&GroupChecksum, sizeof(uint32), 1, pFile);
	MEM_Read(&NumMeshesThisGroup, sizeof(uint32), 1, pFile);

	GroupChecksum += texDictOffset;

	// resolve checksum
	for (pGroup=sGroup::pHead; pGroup; pGroup=pGroup->pNext)
		if (pGroup->Checksum == GroupChecksum)
			break;
	Dbg_MsgAssert(pGroup, ("Couldn't find group checksum #%0X\n", GroupChecksum));

	// set mesh info in the group
	pGroup->pMeshes = pScene->pMeshes+pScene->NumMeshes;
	pGroup->NumMeshes = NumMeshesThisGroup;

	// loop over meshes
	for (i=0,pMesh=pGroup->pMeshes; i<NumMeshesThisGroup; i++,pMesh++)
	{
		// start a subroutine for this mesh
		dma::BeginSub3D();

		// start a ret tag
		dma::BeginTag(dma::ret, 0);

		// get object checksum
		MEM_Read(&pMesh->Checksum, sizeof(uint32), 1, pFile);

		// get object sphere for mesh version 2 onwards
		if (sMesh::Version >= 2)
		{
			unsigned int lod_stuff_not_used_here;
			MEM_Read(&lod_stuff_not_used_here, sizeof(unsigned int), 1, pFile);
			MEM_Read(&lod_stuff_not_used_here, sizeof(unsigned int), 1, pFile);

			// Just process parent/child stuff
			int num_child, hier_data;
			MEM_Read(&hier_data, sizeof(unsigned int), 1, pFile);
			MEM_Read(&num_child, sizeof(unsigned int), 1, pFile);
			for (int j = 0; j < num_child; j++)
			{
				MEM_Read(&hier_data, sizeof(unsigned int), 1, pFile);
			}

			MEM_Read(&sphere, sizeof(Mth::Vector), 1, pFile);

			// write sphere into sScene
			if (pScene->Sphere[3] < 0.0f)
				pScene->Sphere = sphere;

			#if 0
			// GJ:  the following assertion is no longer valid...
			// there are occasionally multiple NxObjects inside a
			// single MDL file, and each mesh will have a different
			// bounding sphere depending on which NxObject is belongs
			// to...  Mike agreed to rewrite the bounding sphere
			// calculation code so that all meshes will have a
			// new bounding sphere that encompasses all the individual
			// meshes' bounding spheres...

			// check object spheres from different meshes match ok
			Dbg_MsgAssert((pScene->Sphere[0]==sphere[0] && pScene->Sphere[1]==sphere[1] &&
						   pScene->Sphere[2]==sphere[2] && pScene->Sphere[3]==sphere[3]),
						  ("object bounding sphere differs between meshes"));
			#else
			// ...so, if the object spheres differ, just combine them...
			if (pScene->Sphere[0]!=sphere[0] || pScene->Sphere[1]!=sphere[1] ||
				pScene->Sphere[2]!=sphere[2] || pScene->Sphere[3]!=sphere[3])
			{
				//printf("\n\n");
				//printf("**********************************************\n");
				//printf("*                                            *\n");
				//printf("*  Tell Mike if you see this line printing!  *\n");
				//printf("*                                            *\n");
				//printf("**********************************************\n");
				//printf("\n\n");
				//printf("combining spheres (%g,%g,%g,%g) and (%g,%g,%g,%g)\n",
				//	   pScene->Sphere[0], pScene->Sphere[1], pScene->Sphere[2], pScene->Sphere[3],
				//	   sphere[0], sphere[1], sphere[2], sphere[3]);

				Mth::Vector x0, x1, d;
				if (pScene->Sphere[3] > sphere[3])
				{
					x0 = pScene->Sphere; 
					x1 = sphere;
				}
				else
				{
					x0 = sphere;
					x1 = pScene->Sphere;
				}
				float r0=x0[3], r1=x1[3];

				// (x0,r0) is the larger sphere, (x1,r1) the smaller
				// d is the vector between centres
				d = x1 - x0;

				// square of distance between centres
				float D2 = DotProduct(d,d);

				// (r0-r1)^2
				float R2 = (r0-r1)*(r0-r1);

				// m = max { (r1-r0+|d|)/2, 0 }
				float m = 0.0f;
				if (R2-D2 < 0)
				{
					m = (r1-r0 + sqrtf(D2)) * 0.5f;
				}

				pScene->Sphere    = x0 + m * d;
				pScene->Sphere[3] = r0 + m;

				//printf("result is (%g,%g,%g,%g)\n",
				//	   pScene->Sphere[0], pScene->Sphere[1], pScene->Sphere[2], pScene->Sphere[3]);
			
				//Dbg_MsgAssert( 0, ( "Please tell Mike if you see this assert firing off!" ) );
			}
			#endif
		}

		// get material checksum and look it up
		MEM_Read(&MaterialChecksum, sizeof(uint32), 1, pFile);
		for (j=0,pMat=pScene->pMaterials; j<pScene->NumMaterials; j++,pMat++)
			if (pMat->Checksum == MaterialChecksum)
				break;
		Dbg_MsgAssert(pMat->Checksum==MaterialChecksum, ("couldn't find material with checksum %08X\n", MaterialChecksum));
		pMesh->pMaterial = pMat;

		// get mesh flags
		MEM_Read(&pMesh->Flags, sizeof(uint32), 1, pFile);

		// make it active
		pMesh->SetActive(true);

		// get bounding volume data
		MEM_Read(&pMesh->ObjBox, sizeof(float) * 3, 1, pFile);
		MEM_Read(&pMesh->Box,    sizeof(float) * 3, 1, pFile);
		MEM_Read(&pMesh->Sphere, sizeof(float) * 4, 1, pFile);

		// pass number
		if (sMesh::Version >= 5)
		{
			MEM_Read(&pMesh->Pass, sizeof(int), 1, pFile);
		}

		// material name
		if (sMesh::Version >= 6)
		{
			MEM_Read(&pMesh->MaterialName, sizeof(int), 1, pFile);
		}
		
		// import vertices of this mesh
		pFile = LoadVertices(pFile, pMesh, pMat, pGroup, skinnedVertexCount);

		// end the dma tag
		dma::EndTag();

		// end the model subroutine
		pMesh->pSubroutine = dma::EndSub3D();
	}

	// add to total number for scene
	pScene->NumMeshes += NumMeshesThisGroup;
	return pFile;
}



//----------------------------------------------------------------------------------------
//		L O A D   V E R T I C E S
//----------------------------------------------------------------------------------------

int UnpackOffset;

void * LoadVertices(void *pFile, sMesh *pMesh, sMaterial *pMat, sGroup *pGroup, int& skinnedIndexCount)
{
	uint32 NumVertices;
	uint REGS, NREG, PRIM, ADDR;
	sTexture *pTex;
	uint32 i;
	uint8  VertexData[64];
	int    STOffset, ColourOffset, NormalOffset, SkinOffset, XYZOffset, VertexSize;
	int    texture_width=0, texture_height=0;

	// get number of vertices for this mesh
	MEM_Read(&NumVertices, sizeof(uint32), 1, pFile);

	// skip mesh if it has no vertices
	if (NumVertices==0)
		return pFile;

	// work out giftag fields
	REGS = gs::XYZF2;
	NREG = 1;
	PRIM = TRISTRIP|ABE|FGE;
	ADDR = VU1_ADDR(Proj);
	if (pMesh->Flags & MESHFLAG_COLOURS)
	{
		REGS = REGS<<4 | gs::RGBAQ;
		NREG++;
	}
	if (pMesh->Flags & MESHFLAG_NORMALS)
	{
		if (pMat->Flags & MATFLAG_ENVIRONMENT)
		{
			ADDR = VU1_ADDR(Refl);
			REGS = REGS<<4 | gs::ST;
			PRIM |= TME;
		}
		else
		{
			REGS = REGS<<4 | gs::NOP;
		}
		NREG++;
	}
	if ((pMesh->Flags & MESHFLAG_TEXTURE) && !(pMat->Flags & MATFLAG_ENVIRONMENT))
	{
		REGS = REGS<<4 | gs::ST;
		NREG++;
		PRIM |= TME;
		ADDR = VU1_ADDR(PTex);
	}
	if (pMat->Flags & MATFLAG_SMOOTH)
		PRIM |= IIP;

	// override everything for skinned models!
	if (pMesh->Flags & MESHFLAG_SKINNED)
	{
		if (pMat->RegCLAMP & PackCLAMP(3,3,0,0,0,0))
		{
			// clamped case, send texture coords as 32 bits floating point STs
			REGS = gs::XYZ2<<16 | gs::RGBAQ<<12 | gs::NOP<<8 | gs::NOP<<4 | gs::ST;
			NREG = 5;
			PRIM |= TME;
			ADDR = VU1_ADDR(Skin);
		}
		else
		{
			// standard case, non-clamped, compress texture coords to 16 bit fixed point UVs
			REGS = gs::XYZ2<<16 | gs::RGBAQ<<12 | gs::NOP<<8 | gs::NOP<<4 | gs::UV;
			NREG = 5;
			PRIM |= TME|FST;
			ADDR = VU1_ADDR(Skin);
		}
	}

	// output GS context for material
	gs::BeginPrim(REL,0,0);
	gs::Reg1(gs::ALPHA_1,	pMat->RegALPHA);
	gs::Reg1(gs::TEST_1, PackTEST(1,AGEQUAL,pMat->Aref,KEEP,0,0,1,ZGEQUAL));

	// texture registers if necessary
	if (pMat->Flags & MATFLAG_TEXTURED)
	{
		pTex = pMat->pTex;
		gs::Reg1(gs::TEX0_1,	pTex->RegTEX0);
		gs::Reg1(gs::TEX1_1,	pMat->RegTEX1);
		gs::Reg1(gs::CLAMP_1,	pMat->RegCLAMP);
		if (pTex->MXL > 0)
		{
			gs::Reg1(gs::MIPTBP1_1,pTex->RegMIPTBP1);
			if (pTex->MXL > 3)
			{
				gs::Reg1(gs::MIPTBP2_1,pTex->RegMIPTBP2);
			}
		}

		// extract texture dimensions
		texture_width  = 1 << ((pTex->RegTEX0 >> 26) & 0xF);
		texture_height = 1 << ((pTex->RegTEX0 >> 30) & 0xF);
	}
	gs::EndPrim(0);

	// set maximum vu buffer size
	vu1::MaxBuffer = pMesh->Flags&MESHFLAG_SKINNED ? 240 : 240;

	// begin a batch of vertices
	BeginModelMultiPrim(REGS, NREG, PRIM, NumVertices, ADDR);

	// work out vertex size and data offsets
	VertexSize = (pMesh->Flags & MESHFLAG_SKINNED) ? 8 : 16;
	XYZOffset = SkinOffset = NormalOffset = ColourOffset = STOffset = 0;
	if (pMesh->Flags & MESHFLAG_SKINNED)
	{
		VertexSize   += 8;
		XYZOffset    += 8;
	}
	if (pMesh->Flags & MESHFLAG_NORMALS)
	{
		VertexSize   += 4;
		XYZOffset    += 4;
		SkinOffset   += 4;
	}
	if (pMesh->Flags & MESHFLAG_COLOURS)
	{
		VertexSize   += 4;
		XYZOffset    += 4;
		SkinOffset   += 4;
		NormalOffset += 4;
	}
	if (pMesh->Flags & MESHFLAG_TEXTURE)
	{
		int size = (pMesh->Flags & MESHFLAG_SKINNED) ? 4 : 8;
		VertexSize   += size;
		XYZOffset    += size;
		SkinOffset   += size;
		NormalOffset += size;
		ColourOffset += size;
	}
	// loop over vertices
	for (i=0; i<NumVertices; i++)
	{
		UnpackOffset=1;

		MEM_Read(VertexData, 1, VertexSize, pFile);

		BeginVertex(pMesh, pMat);

		// special case for skinned models
		if (pMesh->Flags & MESHFLAG_SKINNED)
		{
			// texture coords
			UnpackOffset = 1;
			if ((pMesh->Flags & MESHFLAG_TEXTURE) && !(pMat->Flags & MATFLAG_ENVIRONMENT))
			{
				if (pMat->RegCLAMP & PackCLAMP(3,3,0,0,0,0))
				{
					VertexSTFloat(VertexData+STOffset);
				}
				else
				{
					VertexUV(VertexData+STOffset, texture_width<<4, texture_height<<4);
				}
			}

			// weights
			UnpackOffset = 2;
			VertexWeights(VertexData+SkinOffset);
			
			// normal and transform offsets
			UnpackOffset = 3;
			VertexSkinNormal(VertexData+NormalOffset, VertexData+SkinOffset+4);

			// colour
			UnpackOffset = 4;
			if (pMesh->Flags & MESHFLAG_COLOURS)
			{
				VertexRGBA(VertexData+ColourOffset);
			}

			// position
			UnpackOffset = 5;
			VertexXYZ(VertexData+XYZOffset, true, skinnedIndexCount);
		}
		else
		{
			// texture coords
			if ((pMesh->Flags & MESHFLAG_TEXTURE) && !(pMat->Flags & MATFLAG_ENVIRONMENT))
			{
				VertexST16(VertexData+STOffset);
			}

			// normal normals
			if (pMesh->Flags & MESHFLAG_NORMALS)
			{
				VertexNormal(VertexData+NormalOffset);
			}

			// colour
			if (pMesh->Flags & MESHFLAG_COLOURS)
			{
				VertexRGBA(VertexData+ColourOffset);
			}

			// position
			VertexXYZ(VertexData+XYZOffset, false, skinnedIndexCount);
		}

		EndVertex();
		
		// current vert being processed
		s_currentVertIndex++;
	}

	// accumulate number of vertices
	sMesh::TotalNumVertices += NumVertices;

	// finish batch of vertices
	EndModelMultiPrim();
	return pFile;
}






//---------------------------------------------------------
//		M O D E L   P R I M   C O N S T R U C T I O N
//---------------------------------------------------------


uint   MeshRegs, MeshNReg, MeshPrim, MeshNLoop, MeshAddr;
uint   NumVerticesThisBuffer, NumOutputVertices;
uint32 *pColour;
sint32 *pST32;
sint16 *pST16, *pVertex, *pNormal, *pSkinData;
uint8  *pWeights;
uint16 *pUV;



// begin model prim (i.e. begin a vertex packet)
void BeginModelPrim(uint32 Regs, uint NReg, uint Prim, uint Pre, uint Addr)
{
	vif::STCYCL(1,NReg);
	vif::UNPACK(0,V4_32,1,REL,UNSIGNED,0);
	gif::BeginTag1(Regs, NReg, PACKED, Prim, Pre, Addr);
}


// end model prim (i.e. end a vertex packet)
void EndModelPrim(uint Eop)
{
	gif::EndTag1(Eop);
}


// begin model prim (i.e. begin a vertex packet)
void BeginModelPrimImmediate(uint32 Regs, uint NReg, uint Prim, uint Pre, uint Addr)
{
	vif::STCYCL(1,NReg);
	vif::UNPACK(0,V4_32,1,ABS,UNSIGNED,0);
	gif::BeginTag1(Regs, NReg, PACKED, Prim, Pre, Addr);
}


// end model prim (i.e. end a vertex packet)
void EndModelPrimImmediate(uint Eop)
{
	gif::EndTag1(Eop);
}


// begin a model prim which may span multiple VU1 buffers
void BeginModelMultiPrim(uint32 Regs, uint NReg, uint Prim, uint NLoop, uint Addr)
{
	// record tag info
	MeshRegs  = Regs;
	MeshNReg  = NReg;
	MeshPrim  = Prim;
	MeshNLoop = NLoop;
	MeshAddr  = Addr;

	// work out number of vertices to unpack first
	// (and go ahead with it for now, even if there's only room for 1 vertex)
	NumVerticesThisBuffer = (vu1::MaxBuffer - ((vu1::Loc-vu1::Buffer)&0x3FF) - 1) / NReg;
	if (NumVerticesThisBuffer > MeshNLoop)
		NumVerticesThisBuffer = MeshNLoop;

	BeginModelPrim(Regs, NReg, Prim, 1, Addr);
	NumOutputVertices = 0;
}


void EndModelMultiPrim(void)
{
	EndModelPrim(1);
	vif::MSCAL(VU1_ADDR(Parser));
	vif::FLUSH();
}


int RestartTristrip=0;

void BeginVertex(sMesh *pMesh, sMaterial *pMat)
{
	if (NumOutputVertices == NumVerticesThisBuffer)
	{
		// end the model prim
		if ((pMesh->Flags & MESHFLAG_SKINNED) && (768-((vu1::Buffer+vif::UnpackSize*vif::CycleLength+1)&1023) < vu1::MaxBuffer))
		{
			EndModelPrim(0);
			vif::UNPACK(0, V4_32, 1, REL, UNSIGNED, 0);
			gif::Tag1(0, 0, PACKED, 0, 0, 1, 0, VU1_ADDR(Jump));		// reset address to 0
			vu1::Loc = 0;

			// start VU1 execution
			vif::MSCAL(VU1_ADDR(Parser));
			vif::FLUSH();
		}
		else
		{
			EndModelPrim(1);

			// start VU1 execution
			vif::MSCAL(VU1_ADDR(Parser));
			vif::FLUSH();
		}

		// reduce number still to go
		MeshNLoop -= NumOutputVertices;

		// work out number of vertices to unpack next
		NumVerticesThisBuffer = vu1::MaxBuffer / MeshNReg;
		if (NumVerticesThisBuffer > MeshNLoop)
			NumVerticesThisBuffer = MeshNLoop;

		// start a new buffalo'd

		// add a dummy gs context
		gs::BeginPrim(REL,0,0);
		gs::Reg1(gs::A_D_NOP,0);
		gs::EndPrim(0);

		BeginModelPrim(MeshRegs, MeshNReg, MeshPrim, 1, MeshAddr);
		NumOutputVertices = 0;

		// signal tristrip restart
		RestartTristrip=1;
	}
}


void VertexST16(uint8 *pData)
{
	static sint16 s0,t0,s1,t1;

	if (NumOutputVertices==0)
	{
		vif::BeginUNPACK(0, V2_16, REL, SIGNED, UnpackOffset);
		pST16 = (sint16 *)dma::pLoc;
		if (RestartTristrip)
		{
			*pST16++ = s0;
			*pST16++ = t0;
			*pST16++ = s1;
			*pST16++ = t1;
		}
		dma::pLoc = (uint8 *)pST16 + NumVerticesThisBuffer * 4;
		vif::EndUNPACK();
	}
	*pST16++ = (sint16) (int) (((float *)pData)[0] * 4096.0f);
	*pST16++ = (sint16) (int) (((float *)pData)[1] * 4096.0f);
	s0=s1, s1=pST16[-2];
	t0=t1, t1=pST16[-1];

	UnpackOffset++;
}


void VertexSTFloat(uint8 *pData)
{
	static sint32 s0,t0,s1,t1;

	if (NumOutputVertices==0)
	{
		vif::STMASK(0xE0);
		vif::BeginUNPACK(1, V2_32, REL, SIGNED, UnpackOffset);
		pST32 = (sint32 *)dma::pLoc;
		if (RestartTristrip)
		{
			*pST32++ = s0;
			*pST32++ = t0;
			*pST32++ = s1;
			*pST32++ = t1;
		}
		dma::pLoc = (uint8 *)pST32 + NumVerticesThisBuffer * 8;
		vif::EndUNPACK();
	}
	*(float *)pST32++ = (float)((sint16 *)pData)[0] * 0.000244140625f;
	*(float *)pST32++ = (float)((sint16 *)pData)[1] * 0.000244140625f;
	s0=s1, s1=pST32[-2];
	t0=t1, t1=pST32[-1];

	UnpackOffset++;
}


void VertexUV(uint8 *pData, int textureWidth16, int textureHeight16)
{
	static uint16 u0,v0,u1,v1;

	if (NumOutputVertices==0)
	{
		vif::BeginUNPACK(0, V2_16, REL, UNSIGNED, UnpackOffset);
		pUV = (uint16 *)dma::pLoc;
		if (RestartTristrip)
		{
			*pUV++ = u0;
			*pUV++ = v0;
			*pUV++ = u1;
			*pUV++ = v1;
		}
		dma::pLoc = (uint8 *)pUV + NumVerticesThisBuffer * 4;
		vif::EndUNPACK();
	}
	*pUV++ = (uint16)((int)((float)((sint16 *)pData)[0] * (float)textureWidth16  * 0.000244140625f) + 0x00002000);
	*pUV++ = (uint16)((int)((float)((sint16 *)pData)[1] * (float)textureHeight16 * 0.000244140625f) + 0x00002000);
	u0=u1, u1=pUV[-2];
	v0=v1, v1=pUV[-1];

	UnpackOffset++;
}


void VertexNormal(uint8 *pData)
{
	static sint16 nx0,ny0,nz0,nx1,ny1,nz1;

	if (NumOutputVertices==0)
	{
		vif::BeginUNPACK(0, V3_16, REL, SIGNED, UnpackOffset);
		pNormal = (sint16 *)dma::pLoc;
		if (RestartTristrip)
		{
			*pNormal++ = nx0;
			*pNormal++ = ny0;
			*pNormal++ = nz0;
			*pNormal++ = nx1;
			*pNormal++ = ny1;
			*pNormal++ = nz1;
		}
		dma::pLoc = (uint8 *)pNormal + NumVerticesThisBuffer * 6;
		vif::EndUNPACK();
	}
	*pNormal++ = ((sint16 *)pData)[0];
	*pNormal++ = ((sint16 *)pData)[1];
	//*pNormal++ = ((sint16 *)pData)[2];
	sint16 coord = (sint16)sqrtf(32767.0f*32767.0f - (float)((sint16 *)pData)[0] * (float)((sint16 *)pData)[0]
												   - (float)((sint16 *)pData)[1] * (float)((sint16 *)pData)[1]);
	if (((sint16 *)pData)[0] & 0x0001)
	{
		coord = -coord;
	}
	*pNormal++ = coord;
	nx0=nx1, nx1=pNormal[-3];
	ny0=ny1, ny1=pNormal[-2];
	nz0=nz1, nz1=pNormal[-1];

	UnpackOffset++;
}


#if 0

void VertexWeights(uint8 *pData)
{
	static sint16 wa0,wb0,wc0,wa1,wb1,wc1;

	if (NumOutputVertices==0)
	{
		vif::BeginUNPACK(0, V3_16, REL, SIGNED, UnpackOffset);
		pWeights = (sint16 *)dma::pLoc;
		if (RestartTristrip)
		{
			*pWeights++ = wa0;
			*pWeights++ = wb0;
			*pWeights++ = wc0;
			*pWeights++ = wa1;
			*pWeights++ = wb1;
			*pWeights++ = wc1;
		}
		dma::pLoc = (uint8 *)pWeights + NumVerticesThisBuffer * 6;
		vif::EndUNPACK();
	}
	*pWeights++ = ((sint16 *)pData)[0];
	*pWeights++ = ((sint16 *)pData)[1];
	*pWeights++ = 0x7FFF - pWeights[-1] - pWeights[-2];
	wa0=wa1, wa1=pWeights[-3];
	wb0=wb1, wb1=pWeights[-2];
	wc0=wc1, wc1=pWeights[-1];

	UnpackOffset++;
}

#else

void VertexWeights(uint8 *pData)
{
	static uint8 wa0,wb0,wc0,wa1,wb1,wc1;

	if (NumOutputVertices==0)
	{
		vif::BeginUNPACK(0, V3_8, REL, UNSIGNED, UnpackOffset);
		pWeights = dma::pLoc;
		if (RestartTristrip)
		{
			*pWeights++ = wa0;
			*pWeights++ = wb0;
			*pWeights++ = wc0;
			*pWeights++ = wa1;
			*pWeights++ = wb1;
			*pWeights++ = wc1;
		}
		dma::pLoc = pWeights + NumVerticesThisBuffer * 3;
		vif::EndUNPACK();
	}
	wa0=wa1;
	wb0=wb1;
	wc0=wc1;
	wa1 = (uint8)(sint8)(((float)((sint16 *)pData)[0] + 0.5f) * 0.007818608f);
	wb1 = (uint8)(sint8)(((float)((sint16 *)pData)[1] + 0.5f) * 0.007818608f);
	if (wb1==0)
	{
		wa1=255, wb1=1;
	}
	wc1 = 256 - wa1 - wb1;
	*pWeights++ = wa1;
	*pWeights++ = wb1;
	*pWeights++ = wc1;

	UnpackOffset++;
}

#endif



void VertexSkinNormal(uint8 *pNormData, uint8 *pTOData)
{
	static sint16 nx0,ny0,nz0,nx1,ny1,nz1;

	if (NumOutputVertices==0)
	{
		vif::BeginUNPACK(0, V3_16, REL, SIGNED, UnpackOffset);
		pSkinData = (sint16 *)dma::pLoc;
		if (RestartTristrip)
		{
			*pSkinData++ = nx0;
			*pSkinData++ = ny0;
			*pSkinData++ = nz0;
			*pSkinData++ = nx1;
			*pSkinData++ = ny1;
			*pSkinData++ = nz1;
		}
		dma::pLoc = (uint8 *)pSkinData + NumVerticesThisBuffer * 6;
		vif::EndUNPACK();
	}

	*pSkinData++ = (sint16) (((sint32) ((sint16 *)pNormData)[0]) & 0xFFFFFC00 | (((uint32)((uint8 *)pTOData)[0]) << 2));
	*pSkinData++ = (sint16) (((sint32) ((sint16 *)pNormData)[1]) & 0xFFFFFC00 | (((uint32)((uint8 *)pTOData)[1]) << 2));
	//*pSkinData++ = (sint16) (((sint32) ((sint16 *)pNormData)[2]) & 0xFFFFFC00 | (((uint32)((uint8 *)pTOData)[2]) << 2));
	sint16 coord = (sint16)sqrtf(32767.0f*32767.0f - (float)((sint16 *)pNormData)[0] * (float)((sint16 *)pNormData)[0]
												   - (float)((sint16 *)pNormData)[1] * (float)((sint16 *)pNormData)[1]);
	if (((sint16 *)pNormData)[0] & 0x0001)
	{
		coord = -coord;
	}
	*pSkinData++ = (sint16) ((sint32)coord & 0xFFFFFC00) | (((uint8 *)pTOData)[2] << 2);

	nx0=nx1, nx1=pSkinData[-3];
	ny0=ny1, ny1=pSkinData[-2];
	nz0=nz1, nz1=pSkinData[-1];

	UnpackOffset++;
}



void VertexRGBA(uint8 *pData)
{
	static uint32 rgba0,rgba1;

	if (NumOutputVertices==0)
	{
		vif::BeginUNPACK(0, V4_8, REL, UNSIGNED, UnpackOffset);
		pColour = (uint32 *)dma::pLoc;
		if (RestartTristrip)
		{
			*pColour++ = rgba0;
			*pColour++ = rgba1;
		}
		dma::pLoc = (uint8 *)pColour + NumVerticesThisBuffer * 4;
		vif::EndUNPACK();
	}
	*pColour++ = ((uint32 *)pData)[0];
	rgba0=rgba1, rgba1=pColour[-1];

	UnpackOffset++;
}

Mth::Vector get_bone_scale( int bone_index )
{
	Mth::Vector returnVec( 1.0f, 1.0f, 1.0f, 1.0f );

	if ( bone_index >= 29 && bone_index <= 33 )
	{
		// this only works with the thps5 skeleton, whose
		// head bones are between 29 and 33...
		// (eventually, we can remove the subtract 29
		// once the exporter is massaging the data correctly)
		returnVec = s_pBoneScales[ bone_index - 29 ];
		
		// Y & Z are reversed...  odd!
		Mth::Vector tempVec = returnVec;
		returnVec[Y] = tempVec[Z];
		returnVec[Z] = tempVec[Y];
	}
	else if ( bone_index == -1 )
	{
		// implies that it's not weighted to a bone
		return returnVec;
	}
	else
	{
		// implies that it's weighted to the wrong bone
		return returnVec;
	}

	return returnVec;
}

Mth::Vector get_bone_pos( int bone_index )
{
	Mth::Vector returnVec( 0.0f, 0.0f, 0.0f, 1.0f );
	
	if ( bone_index >= 29 && bone_index <= 33 )
	{
		// this only works with the thps5 skeleton, whose
		// head bones are between 29 and 33...
		// (eventually, we can remove the subtract 29
		// once the exporter is massaging the data correctly)
		returnVec = s_pBonePositions[ bone_index - 29 ];
	}
	else if ( bone_index == -1 )
	{
		// implies that it's not weighted to a bone
		return returnVec;
	}
	else
	{
		// implies that it's weighted to the wrong bone
		return returnVec;
	}

	// need to get into fixed point
	returnVec.Scale( 16.0f );
	returnVec[W] = 1.0f;

	return returnVec;
}

void VertexXYZ(uint8 *pData, bool IsSkin, int& skinnedVertexCount)
{
	static sint64 x0,y0,z0,x1,y1,z1,x2,y2,z2;
	sint64 det0,det1,det2;

	if (NumOutputVertices==0)
	{
		vif::STMOD(1);
		vif::BeginUNPACK(0, V4_16, REL, SIGNED, UnpackOffset);
		pVertex = (sint16 *)dma::pLoc;
		if (RestartTristrip)
		{
			*pVertex++ = x1;
			*pVertex++ = y1;
			*pVertex++ = z1;
			*pVertex++ = 0x8000;
			*pVertex++ = x2;
			*pVertex++ = y2;
			*pVertex++ = z2;
			*pVertex++ = 0x8000;
		}
		dma::pLoc = (uint8 *)pVertex + NumVerticesThisBuffer * 8;
		vif::EndUNPACK();
		vif::STMOD(0);
	}
	
	if (IsSkin)
	{
		if ( s_meshScalingEnabled )
		{
			float x = (float)((sint16 *)pData)[0];
			float y = (float)((sint16 *)pData)[1];
			float z = (float)((sint16 *)pData)[2];

    		Mth::Vector origPos( x, y, z, 1.0f );

			Mth::Vector bonePos0 = get_bone_pos( s_pWeightIndices[s_currentVertIndex * 3] );
			Mth::Vector bonePos1 = get_bone_pos( s_pWeightIndices[s_currentVertIndex * 3 + 1] );
			Mth::Vector bonePos2 = get_bone_pos( s_pWeightIndices[s_currentVertIndex * 3 + 2] );

			// need to scale each vert relative to its parent bone
			Mth::Vector localPos0 = origPos - bonePos0;
			Mth::Vector localPos1 = origPos - bonePos1;
			Mth::Vector localPos2 = origPos - bonePos2;
			localPos0.Scale( get_bone_scale( s_pWeightIndices[s_currentVertIndex * 3] ) );
			localPos1.Scale( get_bone_scale( s_pWeightIndices[s_currentVertIndex * 3 + 1] ) );
			localPos2.Scale( get_bone_scale( s_pWeightIndices[s_currentVertIndex * 3 + 2] ) );
			localPos0 += bonePos0;
			localPos1 += bonePos1;
			localPos2 += bonePos2;
			
			Mth::Vector scaledPos = ( localPos0 * s_pWeights[s_currentVertIndex * 3] )
				+ ( localPos1 * s_pWeights[s_currentVertIndex * 3 + 1] )
				+ ( localPos2 * s_pWeights[s_currentVertIndex * 3 + 2] );

			x = scaledPos[X];
			y = scaledPos[Y];
			z = scaledPos[Z];

			*pVertex++ = (sint16)x;
			*pVertex++ = (sint16)y;
			*pVertex++ = (sint16)z;
		}
		else
		{
			*pVertex++ = (((sint16 *)pData)[0] * 1);
			*pVertex++ = (((sint16 *)pData)[1] * 1);
			*pVertex++ = (((sint16 *)pData)[2] * 1);
		}
	}
	else
	{
		*pVertex++ = (sint16) (((float *)pData)[0] * SUB_INCH_PRECISION);
		*pVertex++ = (sint16) (((float *)pData)[1] * SUB_INCH_PRECISION);
		*pVertex++ = (sint16) (((float *)pData)[2] * SUB_INCH_PRECISION);
	}
	
	if (IsSkin)
	{
		// GJ:  if it's a skinned model, then check to see
		// if the skinnedVertexCount matches one of the ones
		// that requires a pointer to the ADC bit
		AddCASFlag( skinnedVertexCount, (uint16*)pVertex );
	}
	
	skinnedVertexCount++;

	if (IsSkin)
	{
		*pVertex++ = ((uint16 *)pData)[3] ? 0x8000 : 0x0000;
	}
	else
	{
		*pVertex++ = ((uint32 *)pData)[3] ? 0x8000 : 0x0000;
	}
	
	// advance vertex queue and cull triangle if zero area
	x0=x1, y0=y1, z0=z1;
	x1=x2, y1=y2, z1=z2;
	x2=pVertex[-4], y2=pVertex[-3], z2=pVertex[-2];
	det0 = y1*z2+y2*z0+y0*z1-y2*z1-y0*z2-y1*z0;
	det1 = z1*x2+z2*x0+z0*x1-z2*x1-z0*x2-z1*x0;
	det2 = x1*y2+x2*y0+x0*y1-x2*y1-x0*y2-x1*y0;
	if (det0*det0 + det1*det1 + det2*det2 == 0)
		pVertex[-1] = 0x8000;

	UnpackOffset++;
}


void EndVertex(void)
{
	if (NumOutputVertices==0 && RestartTristrip)
	{
		RestartTristrip = 0;
	}

	NumOutputVertices++;
}




void sMesh::SetActive(bool active)
{
	if (active)
	{
		Flags |= MESHFLAG_ACTIVE;
	}
	else
	{
		Flags &= ~MESHFLAG_ACTIVE;
	}
}


// Note, Meshflags and passmasks are off by 2 bits
//#define MESHFLAG_PASS_BIT_0	(1<<8)
//#define MESHFLAG_PASS_BIT_1	(1<<9)

//uint32		gPassMask1 = 0; 		// 1<<6 | 1<<7  (0x40, 0x80)
//uint32		gPassMask0 = 0; 		// 1<<6 | 1<<7  (0x40, 0x80)

bool sMesh::IsActive()
{
	#ifdef	__NOPT_ASSERT__
	// Mick: debug check for global pass on/off
	// Note, shifting flags by 2, so mesh flags match geomnode flags
	// Eventually, meshes will be replaced by leaf nodes in CGeomNode tree
//	if (((Flags>>2) & gPassMask1) != gPassMask0)
//	{	
//		return false;
//	}
	#endif
	

	
	return (Flags&MESHFLAG_ACTIVE) ? true : false;
}

const int MAX_CAS_DATA_LOOKUP = 10000;

// array of pointers to sCASData*
sCASData** sCASDataLookupTable = NULL;

void CreateCASDataLookupTable()
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());

	Dbg_MsgAssert( sCASDataLookupTable == NULL, ( "CAS data lookup table already exists" ) );

	sCASDataLookupTable = (sCASData**)Mem::Malloc( MAX_CAS_DATA_LOOKUP * sizeof(sCASData*) );

	for ( int i = 0; i < MAX_CAS_DATA_LOOKUP; i++ )
	{
		sCASDataLookupTable[i] = NULL;
	}

	Mem::Manager::sHandle().PopContext();
}

void DestroyCASDataLookupTable()
{
	if ( sCASDataLookupTable )
	{
		Mem::Free( sCASDataLookupTable );
		sCASDataLookupTable = NULL;
	}
}

void SetCASDataLookupData( int index, sCASData* pData )
{
	Dbg_MsgAssert( sCASDataLookupTable != NULL, ( "CAS data lookup table doesn't exist" ) );

	Dbg_MsgAssert( index >= 0 && index < MAX_CAS_DATA_LOOKUP, ( "Out of range lookup index %d (must be between 0 and %d)", index, MAX_CAS_DATA_LOOKUP ) );

	Dbg_MsgAssert( sCASDataLookupTable[index] == NULL, ( "Lookup index %d already used", index, MAX_CAS_DATA_LOOKUP ) );

	sCASDataLookupTable[index] = pData;
}

void AddCASFlag( int skinnedVertexIndex, uint16* pADCBit )
{
	if ( !sCASDataLookupTable )
	{
		return;
	}

	Dbg_MsgAssert( skinnedVertexIndex >= 0 && skinnedVertexIndex < MAX_CAS_DATA_LOOKUP, ( "Out of range lookup index %d (must be between 0 and %d", skinnedVertexIndex, MAX_CAS_DATA_LOOKUP ) );

	if ( sCASDataLookupTable[skinnedVertexIndex] != NULL )
	{
		sCASDataLookupTable[skinnedVertexIndex]->pADCBit = pADCBit;
	}
}

int sMesh::TotalNumVertices;
uint32 sMesh::Version;



} // namespace NxPs2


