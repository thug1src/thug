#include <sifdev.h>
#include <stdio.h>
#include <stdlib.h>
#include <libdma.h>
#include <libgraph.h>
#include <errno.h>
#include <string.h>
#include <core/defines.h>
#include "texture.h"
#include "material.h"
#include "mesh.h"
#include "dma.h"
#include "vif.h"
#include "vu1.h"
#include "gif.h"
#include "gs.h"
#include "scene.h"
#include "mikemath.h"
#include "vu1code.h"
#include "switches.h"
#include <sys/file/filesys.h>
#include <sys/file/memfile.h>

#include <core/debug.h>

namespace NxPs2
{

bool load_scene(uint8* pData, int dataSize, sScene *pScene, bool IsSkin, bool IsInstanceable, uint32 texDictOffset, bool doShadowVolume)
{
	pScene->mp_shadow_volume_header = NULL;

	// make sure there's no dma list running
	sceGsSyncPath(0,0);

	// Load the scene file onto the dma buffer so we can use it as a memory mapped file
	void *pMemory = pData;
	void *pFile = pMemory;
	
	int	mesh_size = dataSize * 10/100 + 2000;		// adding the 2000 to cope with very small files						
										
	// version numbers
	uint32 vert_version;
	MEM_Read(&sMaterial::Version, sizeof(uint32), 1, pFile);
	MEM_Read(&sMesh::Version, sizeof(uint32), 1, pFile);
	MEM_Read(&vert_version, sizeof(uint32), 1, pFile);

	// allocate and import materials
	pFile = LoadMaterials(pFile, pScene, texDictOffset);

	// get number of mesh groups
	int NumGroups;
	MEM_Read(&NumGroups, sizeof(uint32), 1, pFile);
	if ( NumGroups != pScene->NumGroups )
	{
		Dbg_Message( "Scene mismatch in number of groups" );
		return false;
	}

	int total_num_meshes = 0;
	// get number of meshes and allocate storage for meshes
	if (sMesh::Version >= 4)
	{
		MEM_Read(&total_num_meshes, sizeof(uint32), 1, pFile);
		mesh_size = total_num_meshes * sizeof(sMesh);
	}
	
	if ( pScene->NumMeshes != 0 )
	{
		Dbg_Message( "pScene->NumMeshes is %d when loading scene, texDictOffset = %d", pScene->NumMeshes, texDictOffset );
		return false;
	}
										  
	pScene->pMeshes = (sMesh *)Mem::Malloc(mesh_size);

	if ( pScene->pMeshes == NULL )
	{
		Dbg_Message( "couldn't allocate memory for meshes" );
		return false;
	}

	// allocate prebuilt dma buffer
//	int	dma_size = Mem::Available()-128;

	int	dma_size = dataSize;  // Really should be around 80%, based on observations, but we know it's no bigger
	
	if (dma_size < 300000)
	{
		dma_size = dma_size *110/100;	 	// Patch for ironman problem
	}
			 
	pScene->pMeshDma = (uint8 *)Mem::Malloc(dma_size);
	if ( pScene->pMeshDma == NULL )
	{
		Dbg_Message( "couldn't allocate memory for mesh dma" );
		return false;
	}
	
	// set dma buffer
	dma::pLoc = pScene->pMeshDma;

	// allocate and import meshes
	sMesh::TotalNumVertices=0;
	pFile = LoadMeshes(pFile, pScene, IsSkin, IsInstanceable, texDictOffset);

//	printf ("DMA Mesh (used %d,  file size = %d, NumMeshes = %d, total_num_meshes = %d, available %d)\n",
//	(int)(dma::pLoc-pScene->pMeshDma), dataSize, pScene->NumMeshes, total_num_meshes, dma_size);
	
	Dbg_MsgAssert( (int)(dma::pLoc-pScene->pMeshDma) < dma_size,("DMA overflow all available memory (used %d, available %d)\n",(int)(dma::pLoc-pScene->pMeshDma),dma_size));
	
	
	if ( (int)(pScene->NumMeshes*sizeof(sMesh)) > mesh_size )
	{
		Dbg_Message( "Mesh overflow file = %s, file size = %d, guessed = %d, actually %d", dataSize, mesh_size, pScene->NumMeshes * sizeof(sMesh) );
		return false;
	}
										
	// reduce dma memory to fit
	Mem::Manager::sHandle().ReallocateShrink( dma::pLoc-pScene->pMeshDma, pScene->pMeshDma);
	
	// reduce mesh memory to fit
	if (sMesh::Version < 4)
	{
		Mem::Manager::sHandle().ReallocateShrink( pScene->NumMeshes*sizeof(sMesh), pScene->pMeshes);
	}
	
	// Hook up shadow volume data.
	pScene->mp_shadow_volume_header = (sShadowVolumeHeader *)pFile;

	#if STENCIL_SHADOW
	if ( pScene->mp_shadow_volume_header && pScene->mp_shadow_volume_header->byte_size )
	{
		int size = pScene->mp_shadow_volume_header->byte_size + sizeof ( sShadowVolumeHeader );
		char * p8 = new char[size];
		memcpy( p8, pScene->mp_shadow_volume_header, size );
		pScene->mp_shadow_volume_header = (sShadowVolumeHeader *)p8;
		pScene->mp_shadow_volume_header->p_vertex = (sShadowVertex *)&pScene->mp_shadow_volume_header[1];
		pScene->mp_shadow_volume_header->p_connect = (sShadowConnect *)&pScene->mp_shadow_volume_header->p_vertex[pScene->mp_shadow_volume_header->num_verts];
		pScene->mp_shadow_volume_header->p_neighbor = (sShadowNeighbor *)&pScene->mp_shadow_volume_header->p_connect[pScene->mp_shadow_volume_header->num_faces];
	}
	else
	{
		pScene->mp_shadow_volume_header = NULL;
	}
	#else
	pScene->mp_shadow_volume_header = NULL;
	#endif

	// deal with instances
	pScene->pInstances = NULL;
	if (IsInstanceable)
	{
		pScene->Flags |= SCENEFLAG_INSTANCEABLE;
	}

	// success
	return true;
}

// load scene from a file
sScene* LoadScene(const char* Filename, sScene *pScene, bool IsSkin, bool IsInstanceable, uint32 texDictOffset, bool doShadowVolume)
{
	int data_size;
	uint8* pData = (uint8*)File::LoadAlloc(Filename,&data_size,dma::pRuntimeBuffer,NON_DEBUG_DMA_BUFFER_SIZE);	
	
	// theoretically, the data pointer returned is
	// exactly the same as the dma::pRuntimeBuffer
	Dbg_MsgAssert( pData == dma::pRuntimeBuffer, ( "Unexpected data pointer was found" ) );

	bool success = load_scene(pData, data_size, pScene, IsSkin, IsInstanceable, texDictOffset, doShadowVolume);
	if ( !success )
	{
		Dbg_MsgAssert( 0, ( "Load scene from file %s failed", Filename ) );
	}

	return pScene;
}

// load scene from a data buffer, rather than opening a file
sScene* LoadScene(uint32* pData, int dataSize, sScene *pScene, bool IsSkin, bool IsInstanceable, uint32 texDictOffset, bool doShadowVolume)
{
	Dbg_MsgAssert( dataSize <= NON_DEBUG_DMA_BUFFER_SIZE, ( "Data to copy is too large to fit in buffer" ) ); 

	// copy over the data
	memcpy( dma::pRuntimeBuffer, pData, dataSize );
	
	bool success = load_scene((uint8*)pData, dataSize, pScene, IsSkin, IsInstanceable, texDictOffset, doShadowVolume);
	if ( !success )
	{
		Dbg_MsgAssert( 0, ( "Load scene from data buffer failed" ) );
	}

	return pScene;
}

void DeleteScene(sScene *pScene)
{
	// deallocate assets
	Mem::Free(pScene->pMeshDma);
	Mem::Free(pScene->pMaterials);
	Mem::Free(pScene->pMeshes);
	Mem::Free(pScene->mp_shadow_volume_header);
	// and what of groups?

	// don't unlink or free, because the textures still exist

	// GJ:  a little pre-emptive debugging, since scenes
	// and textures aren't necessarily destroyed at the same time
	pScene->pMeshDma = NULL;
	pScene->pMaterials = NULL;
	pScene->pMeshes = NULL;

	Dbg_MsgAssert(!pScene->pInstances, ("Trying to delete sScene that still has CInstances attached to it.  Must delete CInstances first."));
}



void sScene::SetUVOffset(uint32 material_name, int pass, float u_offset, float v_offset)
{
	#if 1
	sMesh *pMesh;
	Mth::Matrix mat;
	int i;

	mat.Identity();
	mat[3][0] = u_offset;
	mat[3][1] = v_offset;

	for (pMesh=this->pMeshes, i=0; i<this->NumMeshes; pMesh++, i++)
	{
		if (pMesh->MaterialName==material_name && pMesh->Pass==pass)
		{
			dma::TransformSTs(pMesh->pSubroutine, mat);
		}
	}
	#else
	
	// just some test code for me to play around with...

	sMesh *pMesh;
	Mth::Matrix mat;
	int i;

	mat.Identity();

	//mat[0][0] = cosf(2.0f * 3.14159265f * u_offset);
	//mat[0][1] = sinf(2.0f * 3.14159265f * u_offset);
	//mat[1][0] = -mat[0][1];
	//mat[1][1] = mat[0][0];

	if (u_offset>0.0f)
		mat[0][0] = 1.01f;
	else if (u_offset<0.0f)
		mat[0][0] = 1.0f/1.01f;
	if (v_offset>0.0f)
		mat[1][1] = 1.01f;
	else if (v_offset<0.0f)
		mat[1][1] = 1.0f/1.01f;

	for (pMesh=this->pMeshes, i=0; i<this->NumMeshes; pMesh++, i++)
	{
		if (pMesh->MaterialName==material_name && pMesh->Pass==pass)
		{
			dma::TransformSTs(pMesh->pSubroutine, mat);
		}
	}
	#endif
}




void sScene::SetUVMatrix(uint32 material_name, int pass, const Mth::Matrix &mat)
{
	sMesh *pMesh;
	int i;

	for (pMesh=this->pMeshes, i=0; i<this->NumMeshes; pMesh++, i++)
	{
		if (pMesh->MaterialName==material_name && pMesh->Pass==pass)
		{
			dma::TransformSTs(pMesh->pSubroutine, mat);
		}
	}
}




sScene *sScene::pHead;
uint8  *sScene::pData;


} // namespace NxPs2

