#include <string.h>
#include <core/defines.h>
#include <core/debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgraph.h>
#include "texture.h"
#include "texturemem.h"
#include "dma.h"
#include "vif.h"
#include "gif.h"
#include "gs.h"
#include "group.h"
#include "scene.h"
#include "switches.h"
#include <sys\file\filesys.h>
#include <core\crc.h>


// K: I put this back to 0 for the moment because Conv4to32 is corrupting memory
// by writing past the end of the temporary swizzle buffer, which causes a hang 
// when switching themes in the park editor.
// Garrett: Should be fixed now.
#define SWIZZLE_IN_LOAD 1

namespace NxPs2
{

// static array of pointers to sGroup
// (so that we can pre-allocate our sGroups
// to avoid memory fragmentation)
const int vMAX_TEX_GROUPS = 32;
static sGroup* sp_temp_groups[vMAX_TEX_GROUPS];

extern TextureMemoryLayout TexMemLayout;

//------------------------------------------------------------------------------------
//		L O A D   T E X T U R E S
//------------------------------------------------------------------------------------
//
// Performs 2 tasks - it adds texture register settings to the array Textures for all
// textures in the texture file buffer (increasing NumTextures accordingly), and it
// creates a dma subroutine to upload all the textures in the file buffer to vram.
// Currently it handles all the packing (poorly!), but this will eventually be done by
// a preprocess in the tools pipeline.

bool load_textures(sScene* pScene, uint8* pTexBuffer, bool IsSkin, bool IsInstanceable, bool UsesPip, uint32 texDictOffset)
{
	Dbg_MsgAssert( pScene, ( "No scene pointer" ) );
	Dbg_MsgAssert( pTexBuffer, ( "No tex buffer pointer" ) );

	int total_num_textures=0;

#if 0
	// make sure there's no dma list running
	sceGsSyncPath(0,0);
#else
	// GJ:  Calling "sceGsSyncPath()" instead
	// of "WaitForRendering()" will cause the boardshop
	// to crash when going into the "Change Deck" menu.
	// Neither Mick nor Mike seem to know why this
	// would be the case, so it's probably worth 
	// investigating at some point
	void WaitForRendering();
	WaitForRendering();
#endif

//	Dbg_Message("loading textures from file %s\n", Filename);

	// initialise members
	pScene->NumTextures	= 0;
	pScene->NumMeshes	= 0;
	pScene->pMeshes		= NULL;
	pScene->Flags		= 0;
	pScene->pInstances	= NULL;

	// link it in
	pScene->pNext = sScene::pHead;
	sScene::pHead = pScene;

	// set flags
	if (UsesPip)
	{
		pScene->Flags |= SCENEFLAG_USESPIP;
	}
	
	// set the tex buffer pointer
	pScene->pTexBuffer = pTexBuffer;

	// set data pointer
	sScene::pData = pScene->pTexBuffer;

	// version number
	sTexture::sVersion = *(uint32 *)sScene::pData, sScene::pData+=4;
//	printf("Texture version %d\n", sTexture::sVersion);

	// get number of texture groups
	pScene->NumGroups = *(uint32 *)sScene::pData, sScene::pData+=4;

	// get total number of textures, version >= 3
	if (sTexture::sVersion >= 3)
	{
		total_num_textures = *(int *)sScene::pData, sScene::pData+=4;
	}


	// allocate storage for textures
	if (sTexture::sVersion >= 3)
	{
		// if there are no textures, it will assert in Mem::Malloc
		// so catch this assert earlier here.....
		// really there should be some more elegant handling of scenes with no textures, but they are sufficently rare.
		//Dbg_MsgAssert(total_num_textures>0,("Need at least one texture in %s (it has none)\n",Filename));
		if( total_num_textures > 0 )
		{
			pScene->pTextures = (sTexture *)Mem::Malloc(total_num_textures*sizeof(sTexture));
		}
		else
		{
			pScene->pTextures = NULL;
		}
	}
	else
	{
		pScene->pTextures = (sTexture *)Mem::Malloc(2000*sizeof(sTexture));
	}

	if ( !((total_num_textures == 0 ) || ( pScene->pTextures )) )
	{
		Dbg_Message( "couldn't allocate memory for textures" );
		return false;
	}
	
	Dbg_MsgAssert( pScene->NumGroups < vMAX_TEX_GROUPS, ( "Too many tex groups for static array of sGroup pointers (increase vMAX_TEX_GROUPS from %d)", vMAX_TEX_GROUPS ) );
	for (int i=0; i<pScene->NumGroups; i++)
	{
		// pre-allocate space for texture groups...
		sp_temp_groups[i] = (sGroup *)Mem::Malloc(sizeof(sGroup));
		memset(sp_temp_groups[i],0,sizeof(sGroup));
	}

	// allocate prebuilt dma buffer for textures
	#if 0
	int		available = Mem::Available()-128;			  // -128 is needed otherwise it fails trying to allocate all free mem in a region	
	if (available > 500000)
	{
		available = 500000;	 							  // clamp to 500k if we have loads of memory.  No sure why, maybe just to speed up loading when in debug mode
	}
	#else
	// We don't want to allocate all memory as it messes with memory debugging
	// so 
	// only seem to need T*256 + G*48
	// hmm, somethings seems to be T*512 for some reason....
	// but upped it a bit to be safe
	// These only get used for peds and stuff now
	int available = total_num_textures * 512 + pScene->NumGroups * 128  + 512;  // bumped up multipliers, and added 512 byte extra
	#endif
	
	
	pScene->pTexDma = (uint8 *)Mem::Malloc(available);	  // Allocate all available memeory on current context 

	// set dma pointer to new buffer
	dma::pLoc = pScene->pTexDma;

	// test
	sGroup::VramBufferBase = 0x2BC0;

	// iterate over groups
	for (int i=0; i<pScene->NumGroups; i++)
	{
		LoadTextureGroup(NULL, pScene, sp_temp_groups[i], IsSkin, IsInstanceable, UsesPip, texDictOffset);
		sp_temp_groups[i] = NULL;
	}

	//Dbg_Message( "Dma usage = %d Textures = %d, Groups = %d", (int)(dma::pLoc-pScene->pTexDma), total_num_textures, pScene->NumGroups );

	if ( (int)(dma::pLoc-pScene->pTexDma) >= available )
	{
		Dbg_Message( "Texture dictionary used more memory (%d) than available (%d)", (int)(dma::pLoc-pScene->pTexDma), available );
		return false;
	}




	// resize dma buffer so it just fits
	pScene->pTexDma = (uint8 *)Mem::ReallocateShrink(dma::pLoc-pScene->pTexDma, pScene->pTexDma);

	// success!
	return true;
}

// load textures from a file
sScene *LoadTextures(const char *Filename, bool IsSkin, bool IsInstanceable, bool UsesPip, uint32 texDictOffset, uint32 *p_size)
{
//	Dbg_Message( "LoadTextures: file = %s", Filename );
	
	sScene* pScene = (sScene*)Mem::Malloc(sizeof(sScene));
	Dbg_MsgAssert(pScene, ("couldn't allocate scene"));

	uint8* pTexBuffer = (uint8*)File::LoadAlloc( Filename, (int*) p_size );

	bool success = load_textures(pScene, pTexBuffer, IsSkin, IsInstanceable, UsesPip, texDictOffset);
	if ( !success )
	{
		Dbg_MsgAssert( 0, ( "Load textures from file %s failed", Filename ) );
	}

	return pScene;
}

// load textures from a data buffer, rather than opening a file
sScene *LoadTextures(uint32* pData, int dataSize, bool IsSkin, bool IsInstanceable, bool UsesPip, uint32 texDictOffset)
{
//	Dbg_Message( "LoadTextures: data = %p size = %d", pData, dataSize );
	
	sScene* pScene = (sScene*)Mem::Malloc(sizeof(sScene));
	Dbg_MsgAssert(pScene, ("couldn't allocate scene"));

	uint8* pTexBuffer = (uint8*)Mem::Malloc(dataSize);
	Dbg_MsgAssert(pTexBuffer, ("couldn't allocate tex buffer"));
	
	// copy over the data
	memcpy( pTexBuffer, pData, dataSize );

	bool success = load_textures(pScene, pTexBuffer, IsSkin, IsInstanceable, UsesPip, texDictOffset);
	if ( !success )
	{
		Dbg_MsgAssert( 0, ( "Load textures from data buffer failed" ) );
	}

	return pScene;
}

void LoadTextureGroup(void *pFile, sScene *pScene, sGroup *pGroup, bool IsSkin, bool IsInstanceable, bool UsesPip, uint32 texDictOffset)
{
	sTexture *pTex, *pOriginalTexture=NULL;
	uint32 TW, TH, PSM, CPSM;
	uint TBP[7], TBW[7], CBP;
	uint Width[7], Height[7], NumTexBytes[7], NumTexQWords[7], NumClutBytes, NumClutQWords, NumVramBytes[7];
	uint BitsPerTexel, BitsPerClutEntry, PaletteSize, AdjustedWidth[7], AdjustedHeight[7];
	uint PageWidth, PageHeight, AdjustedBitsPerTexel, TexCount=0;
	int i,j,k,NumTexturesThisGroup,MXL;
	uint NextTBP, LastCBP;
	uint8 *pTextureSource;
	static int Shit=0;

	Dbg_MsgAssert( pScene, ( "No pScene" ) );
	Dbg_MsgAssert( pGroup, ( "No pre-allocated pGroup was specified" ) );

	// set the scene it belongs to
	pGroup->pScene = pScene;

	pGroup->profile_color = 0x00c000;		// green = static group	 (world, cars)
	if (IsInstanceable)
	{
		pGroup->profile_color = 0x808000;		// cyan = instancable static group (cars, gameobjs)
	}
	if (IsSkin)
	{
		pGroup->profile_color = 0x000080;		// red = skin group
		if (IsInstanceable)
		{
			pGroup->profile_color = 0x800080;		// magenta = instancable skinned group 
		}
	}


	// zero the mesh pointer in case no meshes get loaded
	pGroup->pMeshes = NULL;
	pGroup->NumMeshes = 0;

	// prepare to build dma info for this group
	dma::Align(0,16);
	pGroup->pUpload[0] = pGroup->pUpload[1] = dma::pLoc;

	// get group checksum
	pGroup->Checksum = *(uint32 *)sScene::pData;
	pGroup->Checksum += texDictOffset;
	sScene::pData+=4;

	// get group flags if present
	if (sTexture::sVersion >= 2)
	{
		pGroup->flags = *(uint32 *)sScene::pData, sScene::pData+=4;
	}
	else if (pGroup->Checksum>=1000 & pGroup->Checksum<2000)
	{
		pGroup->flags = GROUPFLAG_SKY;
	}
	else
	{
		pGroup->flags = 0;
	}

	// get group priority if present
	if (sTexture::sVersion >= 4)
	{
		pGroup->Priority = *(float *)sScene::pData, sScene::pData+=4;
	}

	else
	{
		// set its priority manually
		if (UsesPip)
		{
			switch (pGroup->flags & (GROUPFLAG_SKY|GROUPFLAG_TRANSPARENT))
			{
			case 0:
				pGroup->Priority = 0;
				break;
			case GROUPFLAG_SKY:
				pGroup->Priority = -100;
				break;
			case GROUPFLAG_TRANSPARENT:
				pGroup->Priority = 7500;
				break;
			case GROUPFLAG_TRANSPARENT|GROUPFLAG_SKY:
				pGroup->Priority = -99;
				break;
			}
		}
		if (IsSkin)
		{
			pGroup->Priority = 5000+Shit;
		}
		if (IsInstanceable)
		{
			pGroup->Priority = 2500+Shit;
		}
	}

	// if it has the lowest priority, make a new head
	if (sGroup::pHead==NULL || pGroup->Priority < sGroup::pHead->Priority)
	{
		pGroup->pNext = sGroup::pHead;
		pGroup->pPrev = NULL;
		sGroup::pHead = pGroup;

		if (pGroup->pNext)
			pGroup->pNext->pPrev = pGroup;
		else
			sGroup::pTail = pGroup;
	}

	// othwerwise if it has the highest priority, make a new tail
	else if (pGroup->Priority >= sGroup::pTail->Priority)
	{
		pGroup->pPrev = sGroup::pTail;
		pGroup->pNext = NULL;
		sGroup::pTail->pNext = pGroup;
		sGroup::pTail = pGroup;
	}

	// otherwise find its immediate superior, priority-wise, in the group list
	else
	{
		sGroup *pSup;
		for (pSup=sGroup::pHead; pSup; pSup=pSup->pNext)
		{
			if (pGroup->Priority < pSup->Priority)
				break;
		}

		Dbg_MsgAssert(pSup, ("error inserting into group list\n"));

		pGroup->pNext = pSup;
		pGroup->pPrev = pSup->pPrev;
		pSup->pPrev->pNext = pGroup;
		pSup->pPrev = pGroup;
	}

	// get number of textures
	NumTexturesThisGroup = *(uint32 *)sScene::pData, sScene::pData+=4;

	// set vram usage for this group
	pGroup->VramStart = sGroup::VramBufferBase;
	pGroup->VramEnd   = sGroup::VramBufferBase+0x0A20;

	// advance buffer for next group
	sGroup::VramBufferBase ^= 0x1E20;

	// initialise base pointers
	NextTBP = pGroup->VramStart;
	LastCBP = pGroup->VramEnd;

	// loop over textures
	for (i=0,pTex=pScene->pTextures+pScene->NumTextures; i<NumTexturesThisGroup; i++,pTex++)
	{
		// get texture flags
		if (sTexture::sVersion >= 5)
		{
			pTex->Flags = *(uint32 *)sScene::pData, sScene::pData+=4;
		}

		// get texture checksum
		pTex->Checksum = *(uint32 *)sScene::pData, sScene::pData+=4;

		// set group checksum
		pTex->GroupChecksum = pGroup->Checksum;

		// get dimensions
		TW = *(uint32 *)sScene::pData, sScene::pData+=4;
		TH = *(uint32 *)sScene::pData, sScene::pData+=4;

		// get pixel storage modes
		PSM  = *(uint32 *)sScene::pData, sScene::pData+=4;	// for texels
		CPSM = *(uint32 *)sScene::pData, sScene::pData+=4;	// for clut

		// get maximum mipmap level
		MXL  = *(uint32 *)sScene::pData, sScene::pData+=4;

		// clear buffer pointers
		pTex->mp_texture_buffer = NULL;
		pTex->mp_clut_buffer = NULL;
		pTex->m_texture_buffer_size = 0;
		pTex->m_clut_buffer_size = 0;

		// bail if it's a non-texture
		if (TW == 0xFFFFFFFF)
		{
			printf("non-texture at number %d\n", i);
			// there won't be any more data for this texture
			continue;
		}

		// detect duplicated texture, signalled by MXL of -1
		if (MXL<0)
		{
			// find the original texture
			pOriginalTexture = ResolveTextureChecksum(pTex->Checksum);

			// adjust MXL
			MXL &= 0x7FFFFFFF;

			// signal duplicate
			pTex->Flags &= ~TEXFLAG_ORIGINAL;
		}
		else
		{
			// not a duplicate, must be the original
			pTex->Flags |= TEXFLAG_ORIGINAL;
		}

		// quadword-align (original only)
		if (pTex->Flags & TEXFLAG_ORIGINAL)
		{
			sScene::pData = (uint8 *)(((uint)(sScene::pData+15)) & 0xFFFFFFF0);
		}

		// bits per texel and palette size
		switch (PSM)
		{
		case PSMCT32:
			BitsPerTexel = 32;
			PaletteSize  = 0;
			break;
		case PSMCT24:
			BitsPerTexel = 24;
			PaletteSize  = 0;
			break;
		case PSMCT16:
			BitsPerTexel = 16;
			PaletteSize  = 0;
			break;
		case PSMT8:
			BitsPerTexel = 8;
			PaletteSize  = 256;
			break;
		case PSMT4:
			BitsPerTexel = 4;
			PaletteSize  = 16;
			break;
		default:
			printf("Unknown PSM %d at index %d in texture file\n", PSM, i);
			exit(1);
		}

		// adjust bits per texel for 24-bit
		AdjustedBitsPerTexel = BitsPerTexel;
		if (BitsPerTexel == 24)
		{
			AdjustedBitsPerTexel = 32;
		}

		// bits per clut entry
		if (BitsPerTexel < 16)
			switch (CPSM)
			{
			case PSMCT32:
				BitsPerClutEntry = 32;
				break;
			case PSMCT16:
				BitsPerClutEntry = 16;
				break;
			default:
				printf("Unknown CPSM %d in texture file\n", PSM);
				exit(1);
			}
		else
			BitsPerClutEntry = 0;

		// rearrange original 256-colour cluts according to requirements of CSM1
		if (PSM==PSMT8 && (pTex->Flags & TEXFLAG_ORIGINAL))
		{
			uint32 temp32;
			uint16 temp16;
			for (j=0; j<256; j+=32)
				for (k=0; k<8; k++)
					if (CPSM==PSMCT32)
					{
						temp32 = ((uint32 *)sScene::pData)[j+k+8];
						((uint32 *)sScene::pData)[j+k+8] = ((uint32 *)sScene::pData)[j+k+16];
						((uint32 *)sScene::pData)[j+k+16] = temp32;
					}
					else
					{
						temp16 = ((uint16 *)sScene::pData)[j+k+8];
						((uint16 *)sScene::pData)[j+k+8] = ((uint16 *)sScene::pData)[j+k+16];
						((uint16 *)sScene::pData)[j+k+16] = temp16;
					}
		}

		// calculate texture dimensions
		for (j=0; j<=MXL; j++)
		{
			Width[j]  = (TW>=0 ? 1<<TW : 0) >> j;
			Height[j] = (TH>=0 ? 1<<TH : 0) >> j;
			TBW[j] = (Width[j]+63) >> 6;
			if (BitsPerTexel<16 && TBW[j]<2)
				TBW[j] = 2;
		}

		// get page size
		switch (PSM)
		{
		case PSMCT32:
		case PSMCT24:
			PageWidth  = 64;
			PageHeight = 32;
			break;
		case PSMCT16:
			PageWidth  = 64;
			PageHeight = 64;
			break;
		case PSMT8:
			PageWidth  = 128;
			PageHeight = 64;
			break;
		case PSMT4:
			PageWidth  = 128;
			PageHeight = 128;
			break;
		default:
			printf("Unknown PSM %d at index %d in texture file\n", PSM, i);
			exit(1);
		}

		// calculate adjusted dimensions based on vram page size
		for (j=0; j<=MXL; j++)
		{
			AdjustedWidth[j]  = Width[j];
			AdjustedHeight[j] = Height[j];

			if (AdjustedWidth[j] < PageWidth && AdjustedHeight[j] > PageHeight)
			{
				AdjustedWidth[j] = PageWidth;
			}

			if (AdjustedWidth[j] > PageWidth && AdjustedHeight[j] < PageHeight)
			{
				AdjustedHeight[j] = PageHeight;
			}

			if (TBW[j]<<6 > AdjustedWidth[j])
			{
				AdjustedWidth[j] = TBW[j]<<6;
			}
		}

		// calculate sizes within file
		NumClutBytes  = PaletteSize * BitsPerClutEntry >> 3;
		NumClutQWords = NumClutBytes >> 4;
		for (j=0; j<=MXL; j++)
		{
			NumTexBytes[j]  = Width[j] * Height[j] * BitsPerTexel >> 3;
			NumVramBytes[j] = AdjustedWidth[j] * AdjustedHeight[j] * AdjustedBitsPerTexel >> 3;
			NumTexQWords[j] = (NumTexBytes[j]+15) >> 4;
		}

		// calculate TBP
		TBP[0] = NextTBP;
		for (j=1; j<=MXL; j++)
			TBP[j] = (((TBP[j-1]<<8)+NumVramBytes[j-1]+0x1FFF) & 0xFFFFE000) >> 8;

		// calculate CBP
		if (BitsPerTexel >= 16)
			CBP = LastCBP;
		else if (BitsPerTexel == 4)
			CBP = LastCBP - 1;
		else
			CBP = LastCBP - (CPSM==PSMCT32 ? 4 : 2);

		// calculate next TBP
		NextTBP = (((TBP[MXL]<<8)+NumVramBytes[MXL]+0x1FFF) & 0xFFFFE000) >> 8;

		// bail if VRAM would become over-packed
		if (NextTBP > CBP)
		{
			printf("no room for texture %d\n", i);
			if (pTex->Flags & TEXFLAG_ORIGINAL)
			{
				sScene::pData += NumClutBytes;
				for (j=0; j<=MXL; j++)
				{
					sScene::pData = (uint8 *)(((uint)(sScene::pData+NumTexBytes[j]+15)) & 0xFFFFFFF0);
				}
			}
			continue;
		}

		// add clut upload to dma list if necessary
		if (BitsPerTexel < 16)
		{
			// source
			if (pTex->Flags & TEXFLAG_ORIGINAL)
			{
				pTextureSource = sScene::pData;
			}
			else
			{
				pTextureSource = pOriginalTexture->mp_clut_buffer;
			}

			// save buffer pointer
			pTex->mp_clut_buffer = pTextureSource;
			pTex->m_clut_buffer_size = NumClutBytes;

			// add dma
			dma::BeginTag(dma::cnt,0);
			vif::NOP();
			vif::NOP();
			gif::Tag2(gs::A_D,1,PACKED,0,0,0,4);
			gs::Reg2(gs::BITBLTBUF,PackBITBLTBUF(0,0,0,CBP,1,CPSM));
			gs::Reg2(gs::TRXPOS,	PackTRXPOS(0,0,0,0,0));
			if (PSM==PSMT4)
				gs::Reg2(gs::TRXREG,PackTRXREG(8,2));
			else
				gs::Reg2(gs::TRXREG,PackTRXREG(16,16));
			gs::Reg2(gs::TRXDIR,	0);
			gif::Tag2(0,0,IMAGE,0,0,0,NumClutQWords);
			dma::EndTag();
			dma::Tag(dma::ref, NumClutQWords, (uint)pTextureSource);
			vif::NOP();
			vif::NOP();
		}

		if (pTex->Flags & TEXFLAG_ORIGINAL)
		{
			// step past clut data, and quadword-align
			sScene::pData = (uint8 *)(((uint)(sScene::pData+NumClutBytes+15)) & 0xFFFFFFF0);

			// save buffer pointer
			pTex->mp_texture_buffer = sScene::pData;
		}
		else
		{
			pTex->mp_texture_buffer = pOriginalTexture->mp_texture_buffer;
		}
		pTextureSource = pTex->mp_texture_buffer;


		// offset to improve caching
		for (j=0; j<=MXL; j++)
		{
			if (TexCount & 1)
			{
				if ((BitsPerTexel==32 || BitsPerTexel==8)
						&& (Width[j]  <= (PageWidth>>1))
						&& (Height[j] <= PageHeight)
					||
					(BitsPerTexel==16 || BitsPerTexel==4)
						&& (Width[j]  <=  PageWidth)
						&& (Height[j] <= (PageHeight>>1)))
				{
					TBP[j] += 16;
					//printf("better caching!\n");
				}
			}
			TexCount++;
		}

#if SWIZZLE_IN_LOAD
		bool swizzleTex = BitsPerTexel <= 8;
#endif // SWIZZLE_IN_LOAD

		// loop over mipmaps
		for (j=0; j<=MXL; j++)
		{
#if SWIZZLE_IN_LOAD
			int swizzle_tbw = TBW[j];
			int swizzle_width = Width[j];
			int swizzle_height = Height[j];
			uint32 swizzlePSM = PSM;
			bool swizzleMip = false;

			if (swizzleTex)
			{
				switch (PSM)
				{
				case PSMT8:
					{
						if (TexMemLayout.CanConv8to32(Width[j], Height[j]))
						{
							if (pTex->Flags & TEXFLAG_ORIGINAL)
							{
								Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
								uint8 *pTextureSwizzle = (uint8 *) Mem::Malloc(Width[j] * Height[j]);
								Mem::Manager::sHandle().PopContext();

								TexMemLayout.Conv8to32(Width[j], Height[j], pTextureSource, pTextureSwizzle);
								memcpy(pTextureSource, pTextureSwizzle, Width[j] * Height[j]);

								Mem::Free(pTextureSwizzle);
							}

							// Change size to 32-bit version
							swizzle_width /= 2;
							swizzle_height /= 2;

							// Recalculate Buffer Width since 8-bit version can be artificially high
							swizzle_tbw = swizzle_width / 64;
							if (swizzle_tbw == 0)
								swizzle_tbw = 1;

							swizzleMip = true;
							swizzlePSM=PSMCT32;	// swizzle
							pTex->Flags |= TEXFLAG_MIP_SWIZZLE_32BIT(j);
						}
						break;
					}
				case PSMT4:
					{
						if (TexMemLayout.CanConv4to32(Width[j], Height[j]))
						{
							//Dbg_Message("Swizzling texture checksum %x", pTex->Checksum);

							if (pTex->Flags & TEXFLAG_ORIGINAL)
							{
								Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
								uint8 *pTextureSwizzle = (uint8 *) Mem::Malloc(Width[j] * Height[j] / 2);
								Mem::Manager::sHandle().PopContext();

								TexMemLayout.Conv4to32(Width[j], Height[j], pTextureSource, pTextureSwizzle);
								memcpy(pTextureSource, pTextureSwizzle, Width[j] * Height[j] / 2);

								Mem::Free(pTextureSwizzle);
							}

							// Change size to 32-bit version
							swizzle_width /= 2;
							swizzle_height /= 4;

							// Recalculate Buffer Width since 4-bit version can be artificially high
							swizzle_tbw = swizzle_width / 64;
							if (swizzle_tbw == 0)
								swizzle_tbw = 1;

							swizzleMip = true;
							swizzlePSM=PSMCT32;	// swizzle
							pTex->Flags |= TEXFLAG_MIP_SWIZZLE_32BIT(j);
						}
						else if (TexMemLayout.CanConv4to16(Width[j], Height[j]))
						{
							if (pTex->Flags & TEXFLAG_ORIGINAL)
							{
								Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
								uint8 *pTextureSwizzle = (uint8 *) Mem::Malloc(Width[j] * Height[j] / 2);
								Mem::Manager::sHandle().PopContext();

								TexMemLayout.Conv4to16(Width[j], Height[j], pTextureSource, pTextureSwizzle);
								memcpy(pTextureSource, pTextureSwizzle, Width[j] * Height[j] / 2);

								Mem::Free(pTextureSwizzle);
							}

							// Change size to 16-bit version
							swizzle_width /= 2;
							swizzle_height /= 2;

							// Recalculate Buffer Width since 4-bit version can be artificially high
							swizzle_tbw = swizzle_width / 64;
							if (swizzle_tbw == 0)
								swizzle_tbw = 1;

							swizzleMip = true;
							swizzlePSM=PSMCT16;	// swizzle
							pTex->Flags |= TEXFLAG_MIP_SWIZZLE_16BIT(j);
						}
						break;
					}

				default:
					break;
				}
			}
#endif // SWIZZLE_IN_LOAD


			// add texture upload to dma list for this mipmap level
			dma::BeginTag(dma::cnt,0);
			vif::NOP();
			vif::NOP();
			gif::Tag2(gs::A_D,1,PACKED,0,0,0,4);
#if SWIZZLE_IN_LOAD
			gs::Reg2(gs::BITBLTBUF,PackBITBLTBUF(0,0,0,TBP[j],swizzle_tbw,swizzlePSM));
			gs::Reg2(gs::TRXPOS,	PackTRXPOS(0,0,0,0,0));
			gs::Reg2(gs::TRXREG,	PackTRXREG(swizzle_width,swizzle_height));
#else
			gs::Reg2(gs::BITBLTBUF,PackBITBLTBUF(0,0,0,TBP[j],TBW[j],PSM));
			gs::Reg2(gs::TRXPOS,	PackTRXPOS(0,0,0,0,0));
			gs::Reg2(gs::TRXREG,	PackTRXREG(Width[j],Height[j]));
#endif // SWIZZLE_IN_LOAD
			gs::Reg2(gs::TRXDIR,	0);
			
			if (j==0)
			{
				pTex->mp_dma = dma::pLoc;
				pTex->m_quad_words = NumTexQWords[j];
				pTex->m_render_count = 0;
			}
			
			
			gif::Tag2(0, 0, IMAGE, 0, 0, 0, NumTexQWords[j]);  			// 0..3 words, NumTexQWords is in the lower 15 bits of the first word [0]
			dma::EndTag();									   			// Just Aligns upo to QW boundry
			dma::Tag(dma::ref, NumTexQWords[j], (uint)pTextureSource);  // 0..1 (after alignment) NumTexQWords goes in word 1
			vif::NOP();
			vif::NOP();

			// step past tex data for this level
			pTextureSource += NumTexBytes[j];
		}

		pTex->m_texture_buffer_size = (uint32) pTextureSource - (uint32) pTex->mp_texture_buffer;

		if (pTex->Flags & TEXFLAG_ORIGINAL)
		{
			sScene::pData = pTextureSource;
		}

		// make entry in texture table
		pTex->MXL        = MXL;
		pTex->RegTEX0    = PackTEX0(TBP[0],TBW[0],PSM,TW,TH,(PSM!=PSMCT24),MODULATE,CBP,CPSM,0,0,1);
		pTex->RegMIPTBP1 = PackMIPTBP1(TBP[1],TBW[1],TBP[2],TBW[2],TBP[3],TBW[3]);
		pTex->RegMIPTBP2 = PackMIPTBP2(TBP[4],TBW[4],TBP[5],TBW[5],TBP[6],TBW[6]);

		// advance texture base pointer to 1st page after texture
		// and reduce clut base pointer to 1st page before clut (if there was one)
		TBP[0] = NextTBP;
		LastCBP = CBP;

	}

	// add to total for the scene
	pScene->NumTextures += NumTexturesThisGroup;

	// texflush so we can use textures
	dma::BeginTag(dma::end, 0);
	vif::NOP();
	vif::NOP();
	gif::Tag2(gs::A_D,1,PACKED,0,0,1,1);
	gs::Reg2(gs::TEXFLUSH,	0);
	dma::EndTag();

	if (UsesPip==false)
	{
		Shit++;
	}

}




void DeleteTextures(sScene *pScene)
{
	sScene *pPrev;
	sGroup *pGroup, *pDead;

	// deallocate assets
	Mem::Free(pScene->pTexBuffer);
	Mem::Free(pScene->pTexDma);
	if( pScene->pTextures != NULL )
	{
		Mem::Free(pScene->pTextures);
	}

	// deallocate groups
	pGroup = sGroup::pHead;
	while (pGroup)
	{
		if (pGroup->pScene == pScene)
		{
			if (pGroup == sGroup::pHead)
			{
				pGroup->pNext->pPrev = NULL;
				sGroup::pHead = pGroup->pNext;
			}
			else if (pGroup == sGroup::pTail)
			{
				pGroup->pPrev->pNext = NULL;
				sGroup::pTail = pGroup->pPrev;
			}
			else
			{
				pGroup->pNext->pPrev = pGroup->pPrev;
				pGroup->pPrev->pNext = pGroup->pNext;
			}
			pDead = pGroup;
			pGroup = pGroup->pNext;
			Mem::Free(pDead);
		}
		else
			pGroup = pGroup->pNext;
	}

	// unlink
	if (pScene==sScene::pHead)
		sScene::pHead = pScene->pNext;
	else
	{
		for (pPrev=sScene::pHead; pPrev; pPrev=pPrev->pNext)
			if (pPrev->pNext == pScene)
				break;
		Dbg_MsgAssert(pPrev, ("couldn't find scene to delete\n"));

		pPrev->pNext = pScene->pNext;
	}

	// free
	Mem::Free(pScene);
}





sTexture *ResolveTextureChecksum(uint32 TextureChecksum)
{
	sScene *pScene;
	sTexture *pTex=NULL;
	int i;

	for (pScene=sScene::pHead; pScene; pScene=pScene->pNext)
	{
		for (i=0,pTex=pScene->pTextures; i<pScene->NumTextures; i++,pTex++)
			if ((pTex->Checksum == TextureChecksum) && (pTex->Flags & TEXFLAG_ORIGINAL))
				return pTex;
	}

	return NULL;
}



sTexture *ResolveTextureChecksum(uint32 TextureChecksum, uint32 GroupChecksum)
{
	sScene *pScene;
	sTexture *pTex=NULL;
	int i;

	for (pScene=sScene::pHead; pScene; pScene=pScene->pNext)
	{
		for (i=0,pTex=pScene->pTextures; i<pScene->NumTextures; i++,pTex++)
			if ((pTex->Checksum == TextureChecksum) && (pTex->GroupChecksum == GroupChecksum))
				return pTex;
	}

	return NULL;
}



uint32 sTexture::sVersion;


/////////////////////////////////////////////////////////////////////////////
//
// The following Get() calls are very inefficient.  They should only be
// used when necessary (i.e. initialization).
//
uint16	sTexture::GetWidth() const
{
	sceGsTex0 tex0 = *((sceGsTex0 *) &RegTEX0);
	uint16 TW = tex0.TW;
	//uint16 TW = (uint16) ((RegTEX0 >> 26) & M04);

	return 1 << TW;
}

uint16	sTexture::GetHeight() const
{
	sceGsTex0 tex0 = *((sceGsTex0 *) &RegTEX0);
	uint16 TH = tex0.TH;
	//uint16 TH = (uint16) ((RegTEX0 >> 30) & M04);

	return 1 << TH;
}

uint8	sTexture::GetBitdepth() const
{
	sceGsTex0 tex0 = *((sceGsTex0 *) &RegTEX0);
	uint PSM = tex0.PSM;

	switch (PSM)
	{
	case PSMCT32:
		return 32;
		break;
	case PSMCT24:
		return 24;
		break;
	case PSMCT16:
		return 16;
		break;
	case PSMT8:
		return 8;
		break;
	case PSMT4:
		return 4;
		break;
	default:
		return 0;
	}
}

uint8	sTexture::GetClutBitdepth() const
{
	sceGsTex0 tex0 = *((sceGsTex0 *) &RegTEX0);
	uint CPSM = tex0.CPSM;

	switch (CPSM)
	{
	case PSMCT32:
		return 32;
		break;
	case PSMCT24:
		return 24;
		break;
	case PSMCT16:
		return 16;
		break;
	default:
		return 0;
	}
}

uint8	sTexture::GetNumMipmaps() const
{
	return MXL + 1;
}

bool	sTexture::HasSwizzleMip() const
{
	for (uint mip_idx = 0; mip_idx <= MXL; mip_idx++)
	{
		if (Flags & TEXFLAG_MIP_SWIZZLE(mip_idx))
		{
			return true;
		}
	}

	return false;
}

void	sTexture::ReplaceTextureData(uint8 *p_texture_data)
{
	uint mip_width, mip_height, mip_NumTexBytes;

	int bitdepth = GetBitdepth();
	int width = GetWidth();
	int height = GetHeight();

	uint8 *pTextureSource = mp_texture_buffer;

	// calculate texture dimensions
	for (uint j=0; j<=MXL; j++)
	{
		mip_width  = width >> j;
		mip_height = height >> j;
		mip_NumTexBytes  = mip_width * mip_height * bitdepth >> 3;

		// Do the actual copies, swizzling if necessary
		if (Flags & TEXFLAG_MIP_SWIZZLE_16BIT(j))
		{
			switch (bitdepth)
			{
			case 4:
				TexMemLayout.Conv4to16(mip_width, mip_height, p_texture_data, pTextureSource);
				break;
			case 8:
			default:
				Dbg_MsgAssert(0, ("Can't swizzle a texture of bitdepth %d to 16-bit", bitdepth));
				break;
			}
		}
		else if (Flags & TEXFLAG_MIP_SWIZZLE_32BIT(j))
		{
			switch (bitdepth)
			{
			case 8:
				TexMemLayout.Conv8to32(mip_width, mip_height, p_texture_data, pTextureSource);
				break;
			case 4:
				TexMemLayout.Conv4to32(mip_width, mip_height, p_texture_data, pTextureSource);
				break;
			default:
				Dbg_MsgAssert(0, ("Can't swizzle a texture of bitdepth %d to 32-bit", bitdepth));
				break;
			}
		}
		else
		{
			memcpy(pTextureSource, p_texture_data, mip_NumTexBytes);
		}

		// step past tex data for this level
		pTextureSource += mip_NumTexBytes;
		p_texture_data += mip_NumTexBytes;
	}

}

} // namespace NxPs2

