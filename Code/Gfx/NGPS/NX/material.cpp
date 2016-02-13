#include <string.h>
#include <core/defines.h>
#include <core/debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file/filesys.h>
#include <sys/file/memfile.h>
#include "texture.h"
#include "material.h"
#include "dma.h"
#include "vif.h"
#include "gs.h"
#include "scene.h"
#include "switches.h"



namespace NxPs2
{


//----------------------------------------------------------------------------------------
//		L O A D   M A T E R I A L S
//----------------------------------------------------------------------------------------

void * LoadMaterials(void *pFile, sScene *pScene, uint32 texDictOffset)
{
	sMaterial *pMat;
	sTexture *pTex;
	uint32 TextureChecksum, GroupChecksum;
	uint32 MXL, MMAG, MMIN, K, L, NumSeqs, seq, NumKeys;
	int i;

	// get number of materials
	MEM_Read(&pScene->NumMaterials, sizeof(uint32), 1, pFile);

	// allocate storage for materials
	pScene->pMaterials = (sMaterial *)Mem::Malloc(pScene->NumMaterials*sizeof(sMaterial));
	Dbg_MsgAssert(pScene->pMaterials, ("couldn't allocate memory for materials"));

	// loop over materials
	for (i=0,pMat=pScene->pMaterials; i<pScene->NumMaterials; i++,pMat++)
	{
		// get material checksum
		MEM_Read(&pMat->Checksum, sizeof(uint32), 1, pFile);

		if (sMaterial::Version >= 5)
		{
			// get material flags (moved up here in version 5)
			MEM_Read(&pMat->Flags, 1, sizeof(uint32), pFile);
		}

		// get Aref
		if (sMaterial::Version >= 2)
		{
			if (sMaterial::Version >= 3)
			{
				MEM_Read(&pMat->Aref, 1, 4, pFile);
			}
			else
			{
				MEM_Read(&pMat->Aref, 1, 1, pFile);
			}

		}
		else
		{
			pMat->Aref = 1;
		}

		if (sMaterial::Version >= 5)
		{
			if (pMat->Flags & MATFLAG_ANIMATED_TEX)
			{
				Dbg_Assert( 0 );
				/*
				NxAnimatedTexture* anim_tex;
	
				anim_tex = &material->m_Passes[0].m_AnimatedTexture;
				pOutFile->Write((const char*) &anim_tex->m_NumKeyframes, sizeof( int ), false);
				pOutFile->Write((const char*) &anim_tex->m_Period, sizeof( int ), false);
				pOutFile->Write((const char*) &anim_tex->m_Iterations, sizeof( int ), false);
				pOutFile->Write((const char*) &anim_tex->m_Phase, sizeof( int ), false);
				for( j = 0; j < anim_tex->m_NumKeyframes; j++ )
				{
					NxAnimatedTextureKeyframe* frame;
	
					frame = &anim_tex->m_Keyframes[j];
					pOutFile->Write((const char*) &frame->m_Time, sizeof( unsigned int ), false);
	
					tex_checksum = material->m_Passes[0].GetTextureChecksum( j, Utils::vPLATFORM_PS2 );
					pOutFile->Write((const char*) &tex_checksum, sizeof( unsigned long ), false);
				}*/
				
			}
			else
			{
				// get texture checksum
				MEM_Read(&TextureChecksum, 1, sizeof(uint32), pFile);
			}
		}
		else
		{
			// get texture checksum
			MEM_Read(&TextureChecksum, 1, sizeof(uint32), pFile);
		}
		

		if (sMaterial::Version >= 3)
		{
			// get group checksum
			MEM_Read(&GroupChecksum, 1, sizeof(uint32), pFile);

			GroupChecksum += texDictOffset;
		}

		if (sMaterial::Version < 5)
		{
			// get material flags (moved above in version 5 because the parser needs the flags
			// to correctly parse subsequent data
			MEM_Read(&pMat->Flags, 1, sizeof(uint32), pFile);
		}

		// get ALPHA register value
		MEM_Read(&pMat->RegALPHA, 1, sizeof(uint64), pFile);

		// get CLAMP register value
		if (sMaterial::Version >= 2)
		{
			uint32 ClampU,ClampV;
			MEM_Read(&ClampU, 1, sizeof(uint32), pFile);
			MEM_Read(&ClampV, 1, sizeof(uint32), pFile);
			pMat->RegCLAMP = PackCLAMP(ClampU,ClampV,0,0,0,0);
		}
		else
			pMat->RegCLAMP = PackCLAMP(REPEAT,REPEAT,0,0,0,0);

		// step past material colours, currently unsupported
		MEM_Seek(pFile, 36, SEEK_CUR);

		// step past uv wibble data, currently unsupported
		if (pMat->Flags & MATFLAG_UV_WIBBLE)
		{
			MEM_Seek(pFile, 40, SEEK_CUR);
		}

		// step past vc wibble data, currently unsupported
		if (pMat->Flags & MATFLAG_VC_WIBBLE)
		{
			MEM_Read(&NumSeqs, sizeof(uint32), 1, pFile);
			for (seq=0; seq<NumSeqs; seq++)
			{ 
				MEM_Read(&NumKeys, sizeof(uint32), 1, pFile);
				MEM_Seek(pFile, 20 * NumKeys, SEEK_CUR);
			}
		}

		// if textured...
		if (TextureChecksum)
		{
			// resolve texture checksum
			if (sMaterial::Version >= 3)
			{
				pTex = ResolveTextureChecksum(TextureChecksum, GroupChecksum);
				Dbg_MsgAssert(pTex, ("error: couldn't find texture checksum %08X with group checksum %08X\n",
																			TextureChecksum, GroupChecksum));
			}
			else
			{
				pTex = ResolveTextureChecksum(TextureChecksum);
				Dbg_MsgAssert(pTex, ("error: couldn't find texture checksum %08X\n", TextureChecksum));
			}

			// set texture pointer
			pMat->pTex = pTex;

			// get mipmap info
			MXL  = pTex->MXL;
			MEM_Read(&MMAG, sizeof(uint32), 1, pFile);
			MEM_Read(&MMIN, sizeof(uint32), 1, pFile);
			MEM_Read(&K, sizeof(uint32), 1, pFile);
			K = (int) ((*(float *)&K) * 16.0f) & 0xFFF;
			if (sMaterial::Version > 1)
				pMat->RegTEX1 = PackTEX1(0,MXL,MMAG,MMIN,0,0,K);
			else
			{
				MEM_Read(&L, sizeof(uint32), 1, pFile);
				pMat->RegTEX1 = PackTEX1(0,MXL,MMAG,MMIN,0,L,K);
			}
		}
		// otherwise just step past mipmap info
		else
		{
			pMat->pTex = 0;
			MEM_Seek(pFile, 12+4*(sMaterial::Version==1), SEEK_CUR);
		}

		// get reflection map scale values
		if (sMaterial::Version >= 4)
		{
			float temp;
			MEM_Read(&temp, sizeof(float), 1, pFile);
			pMat->RefMapScaleU = (sint16)(int)(temp * 256.0f);
			MEM_Read(&temp, sizeof(float), 1, pFile);
			pMat->RefMapScaleV = (sint16)(int)(temp * 256.0f);
		}

	}
	return pFile;
}


uint32 sMaterial::Version;


} // namespace NxPs2

