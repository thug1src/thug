#include <string.h>
#include <core/defines.h>
#include <core/debug.h>
#include <sys/file/filesys.h>
#include <core/math.h>
#include <core/math/vector.h>
#include <eestruct.h>
#include <sifdev.h>
#include <stdlib.h>
#include "sprite.h"
#include "dma.h"
#include "vif.h"
#include "vu1.h"
#include "gif.h"
#include "gs.h"
#include "vu1code.h"
#include "texturemem.h"
#include "render.h"
#include "switches.h"
#include <gel/scripting/symboltable.h>

/*


	
.img file format
--------------------------
	
Offset		Size (Bytes)	Field
---------------------------------------------------------------
0x00			4			Version (currently 1)
0x04			4			Basename of file in CRC format
0x08			4			PS2 TW (log2 Width)
0x0c			4			PS2 TH (log2 Height)
0x10			4			PS2 PSM (Bit depth)
0x14			4			PS2 CPSM (CLUT bit depth)
0x18			4			PS2 MXL (Number of mipmaps)
0x1c			2			Original image width
0x1e			2			Original image height
0x20			Upto 1K		CLUT data
0x20+CLUT_size	W*H*depth		Texture mipmaps

	
	
	
font->vram upload format
------------------------


	GIFtag PACKED 4
	BITBLTBUF ?
	TRXPOS 0
	TRXREG w,h
	TRXDIR 0
	
	GIFtag IMAGE (w*h+15)/16
	... (tex data)
	
	GIFtag PACKED 4
	BITBLTBUF ?
	TRXPOS 0
	TRXREG w,h
	TRXDIR 0
	
	GIFtag IMAGE 64
	... (clut data)
	
	
	
	total size in bytes = 12*16+((w*h+15)/16)*16+1024 = 1216+((w*h+15)/16)*16
	
	this will be referenced as follows:
	
	dma ref
	NOP
	DIRECT 76+(w*h+15)/16
	
	
*/


namespace NxPs2
{

/******************************************************************/
/*                                                                */
/* SSingleTexture												  */
/*                                                                */
/******************************************************************/

SSingleTexture *	SSingleTexture::sp_stexture_list = NULL;

/////////////////////////////////////////////////////////////////////////////
//

SSingleTexture::SSingleTexture()
{
	Dbg_Error("Must supply a filename when creating a SSingleTexture");
}

/////////////////////////////////////////////////////////////////////////////
//
// .img.ps2 buffer

SSingleTexture::SSingleTexture(uint8* p_buffer, int buffer_size, bool sprite, bool alloc_vram, bool load_screen) :
	m_flags(0),
	m_usage_count(0)
{
	#ifdef			__NOPT_ASSERT__
	strcpy(m_name,"texture from buffer");
	#endif

	mp_TexBitBltBuf = mp_ClutBitBltBuf = NULL;

	// Set flags
	if (load_screen)
	{
		m_flags |= mLOAD_SCREEN;
	}
	if (sprite)
	{
		m_flags |= mSPRITE;
	}
	if ((sprite && alloc_vram) && !load_screen)
	{
		m_flags |= mALLOC_VRAM;
	}

	// open the font file
	if (!sprite)
	{
		Dbg_Assert(!alloc_vram);
	}
	
	mp_PixelData = mp_ClutData = NULL;
	m_PixelDataSize = m_ClutDataSize = 0;
	
	mp_TexBuffer = (uint8*)Mem::Malloc( buffer_size );
	Dbg_Assert( mp_TexBuffer );
	memcpy( mp_TexBuffer, p_buffer, buffer_size );
	m_TexBufferSize = buffer_size;
	
	// Initialize all the data
	InitTexture(false);

	// add texture to texture list
	if (sprite && !load_screen)
	{
		mp_next = sp_stexture_list;
		sp_stexture_list = this;
	}

	if (load_screen) {
		FlipTextureVertical();
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// .img.ps2 file

SSingleTexture::SSingleTexture(const char *Filename, bool sprite, bool alloc_vram, bool load_screen) :
	m_flags(0),
	m_usage_count(0)
{
	char ExtendedFilename[128];
		  
	#ifdef			__NOPT_ASSERT__
	strcpy(m_name,Filename);
	#endif

	mp_TexBitBltBuf = mp_ClutBitBltBuf = NULL;

	// Set flags
	if (load_screen)
	{
		m_flags |= mLOAD_SCREEN;
	}
	if (sprite)
	{
		m_flags |= mSPRITE;
	}
	if ((sprite && alloc_vram) && !load_screen)
	{
		m_flags |= mALLOC_VRAM;
	}

	// open the font file
	if (sprite)
	{
//		strcpy(ExtendedFilename, "images/");
//		strcat(ExtendedFilename, Filename);
		strcpy(ExtendedFilename, Filename);
	} else {
		Dbg_Assert(!alloc_vram);

		strcpy(ExtendedFilename, Filename);
	}
	strcat(ExtendedFilename, ".img.ps2");
	
	mp_PixelData = mp_ClutData = NULL;
	m_PixelDataSize = m_ClutDataSize = 0;
	
	#if 0
	void *pFile = File::Open(ExtendedFilename, "rb");
	// get file size and allocate texture buffer
	m_TexBufferSize = File::GetFileSize(pFile);
	mp_TexBuffer = (uint8 *)Mem::Malloc(m_TexBufferSize);
	File::Read(mp_TexBuffer, 1, m_TexBufferSize, pFile);
	File::Close(pFile);
	#else
	mp_TexBuffer = (uint8*)File::LoadAlloc(ExtendedFilename, &m_TexBufferSize);
	#endif
	
	// Initialize all the data
	InitTexture(false);

	// add texture to texture list
	if (sprite && !load_screen)
	{
		mp_next = sp_stexture_list;
		sp_stexture_list = this;
	}

	if (load_screen) {
		FlipTextureVertical();
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// raw buffer

SSingleTexture::SSingleTexture(uint8* p_buffer, int width, int height, int bitdepth, int clut_bitdepth, int mipmaps,
							   bool sprite, bool alloc_vram, bool load_screen):
	m_flags(0),
	m_usage_count(0)
{
	#ifdef			__NOPT_ASSERT__
	strcpy(m_name,"texture from raw data");
	#endif

	mp_TexBitBltBuf = mp_ClutBitBltBuf = NULL;

	// Set flags
	if (load_screen)
	{
		m_flags |= mLOAD_SCREEN;
	}
	if (sprite)
	{
		m_flags |= mSPRITE;
	}
	if ((sprite && alloc_vram) && !load_screen)
	{
		m_flags |= mALLOC_VRAM;
	}

	// open the font file
	if (!sprite)
	{
		Dbg_Assert(!alloc_vram);
	}
	
	mp_PixelData = mp_ClutData = NULL;
	m_PixelDataSize = m_ClutDataSize = 0;
	mp_TexBuffer = NULL;		// Since we aren't allocating any memory
	m_TexBufferSize = 0;
	
	m_checksum = 0;
	m_orig_width = width;
	m_orig_height = height;

	// Initialize all the data
	//InitTexture(false);
	uint32 TW, TH, PSM, CPSM, MXL;

	TW = TextureMemoryLayout::numBits(width - 1);		// Rounds up to the nearest 2^N
	TH = TextureMemoryLayout::numBits(height - 1);

	PSM = bitdepth_to_psm(bitdepth);
	CPSM = bitdepth_to_psm(clut_bitdepth);

	MXL = mipmaps - 1;

	// Raw data is inverted vertically from the way we expect textures.
	m_flags |= mINVERTED;

	setup_reg_and_dma(p_buffer, TW, TH, PSM, CPSM, MXL, false);

	// add texture to texture list
	if (sprite && !load_screen)
	{
		mp_next = sp_stexture_list;
		sp_stexture_list = this;
	}

	if (load_screen) {
		FlipTextureVertical();
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// copy constructor

SSingleTexture::SSingleTexture(const SSingleTexture & src_texture)
{
	// Copy checksum and flags
	m_checksum = src_texture.m_checksum;
	m_flags = src_texture.m_flags;

	#ifdef			__NOPT_ASSERT__
	strcpy(m_name,src_texture.m_name);
	#endif

	// Init some of the data before we call InitTexture()
	m_usage_count = 0;
	mp_TexBitBltBuf = mp_ClutBitBltBuf = NULL;
	mp_PixelData = mp_ClutData = NULL;
	m_PixelDataSize = m_ClutDataSize = 0;

	// Copy file buffer
	m_TexBufferSize = src_texture.m_TexBufferSize;
	if (m_TexBufferSize && src_texture.mp_TexBuffer)
	{
		mp_TexBuffer = (uint8 *)Mem::Malloc(m_TexBufferSize);
		memcpy(mp_TexBuffer, src_texture.mp_TexBuffer, m_TexBufferSize);

		// This routine will initialize and allocate the rest
		InitTexture(true);
	}
	else
	{
		Dbg_MsgAssert(0, ("Can't copy SSingleTexture that was created with a raw buffer"));
		//InitTexture(true);
	}

	// add texture to texture list
	if ((m_flags & mSPRITE) && !(m_flags & mLOAD_SCREEN))
	{
		mp_next = sp_stexture_list;
		sp_stexture_list = this;
	}
}

/////////////////////////////////////////////////////////////////////////////
//

SSingleTexture::~SSingleTexture()
{
	// find texture and unchain from list
	if (sp_stexture_list == this)
	{
		sp_stexture_list = mp_next;
	} else if (sp_stexture_list) {
		SSingleTexture *p_texture = sp_stexture_list;

		while(p_texture->mp_next)
		{
			if (p_texture->mp_next == this)
			{
				p_texture->mp_next = mp_next;
				break;
			}

			p_texture = p_texture->mp_next;
		}
	}

	// free memory
	if (mp_VifData) Mem::Free(mp_VifData);
	if (mp_TexBuffer) Mem::Free(mp_TexBuffer);

	Dbg_MsgAssert(m_usage_count == 0, ("Trying to remove SSingleTexture %x that is still being used", m_checksum));

	// and re-do VRAM allocation
	VRAMNeedsUpdating();
}

/////////////////////////////////////////////////////////////////////////////
//
#define STEX_USE_CALL 1

const static uint32 TEXTURE_VERSION = 1;

TextureMemoryLayout TexMemLayout;

bool SSingleTexture::InitTexture(bool copy)
{
	uint32 TW, TH, PSM, CPSM, MXL = 0;

	uint8 *pData = mp_TexBuffer;

	uint32 version;
	version = *(uint32 *)pData, pData+=4;
	Dbg_MsgAssert(version >= TEXTURE_VERSION, ("Wrong version in .img file: %d. Game uses: %d", version, TEXTURE_VERSION));
	m_checksum = *(uint32 *)pData, pData+=4;

	// get dimensions
	TW = *(uint32 *)pData, pData+=4;
	TH = *(uint32 *)pData, pData+=4;

	// get pixel storage modes
	PSM  = *(uint32 *)pData, pData+=4;	// for texels
	CPSM = *(uint32 *)pData, pData+=4;	// for clut

	// get maximum mipmap level
	if (version >= 2)
	{
		MXL  = *(uint32 *)pData, pData+=4;
	}
	Dbg_Assert(MXL == 0);
	m_orig_width = *(uint16 *)pData, pData+=2;
	m_orig_height = *(uint16 *)pData, pData+=2;

	// bail if it's a non-texture
	Dbg_MsgAssert((TW != 0xFFFFFFFF), ("No texture in .img file\n"));

	// quadword-align
	pData = (uint8 *)(((uint)(pData+15)) & 0xFFFFFFF0);

	return setup_reg_and_dma(pData, TW, TH, PSM, CPSM, MXL, copy);
}

bool SSingleTexture::setup_reg_and_dma(uint8 *pData, uint32 TW, uint32 TH, uint32 PSM, uint32 CPSM, uint32 MXL, bool copy)
{
	uint TBW;
	uint Width, Height, NumTexBytes, NumTexQWords, NumClutBytes, NumClutQWords, NumVramBytes;
	uint BitsPerTexel, BitsPerClutEntry, PaletteSize;
	uint NextTBP;
	int j,k;

	uint8 *old_dma_loc = dma::pLoc;		  

	// initialise base pointers
	if (IsInVRAM())
	{
		NextTBP = FontVramBase;
	} else {
		NextTBP = FontVramStart;
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
		printf("Unknown PSM %d in texture file\n", PSM);
		exit(1);
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

	// rearrange 256-colour cluts according to requirements of CSM1
	if (!copy && PSM==PSMT8)
	{
		uint32 temp32;
		uint16 temp16;
		for (j=0; j<256; j+=32)
			for (k=0; k<8; k++)
				if (CPSM==PSMCT32)
				{
					temp32 = ((uint32 *)pData)[j+k+8];
					((uint32 *)pData)[j+k+8] = ((uint32 *)pData)[j+k+16];
					((uint32 *)pData)[j+k+16] = temp32;
				}
				else
				{
					temp16 = ((uint16 *)pData)[j+k+8];
					((uint16 *)pData)[j+k+8] = ((uint16 *)pData)[j+k+16];
					((uint16 *)pData)[j+k+16] = temp16;
				}
	}

	// calculate texture dimensions
	Width = (TW>=0 ? 1<<TW : 0);
	Height = (TH>=0 ? 1<<TH : 0);
	TBW = (Width+63) >> 6;
	if (BitsPerTexel<16 && TBW<2)
		TBW = 2;

	// Calculate memory needed
	int min_width = Width, min_height = Height;
	TexMemLayout.getMinSize(min_width, min_height, PSM);
	m_NumTexVramBlocks = TexMemLayout.getImageSize(min_width, min_height, PSM);
	NumVramBytes = m_NumTexVramBlocks * 256;
	//NumVramBytes = (TBW << 6) * Height * BitsPerTexel >> 3;

	// calculate sizes within file
	NumClutBytes  = PaletteSize * BitsPerClutEntry >> 3;
	NumClutQWords = NumClutBytes >> 4;
	NumTexBytes  = Width * Height * BitsPerTexel >> 3;
	NumTexQWords = NumTexBytes >> 4;

	//Dbg_Message("Texture size (%d, %d) of bitdepth %d and BufferWidth %d", Width, Height, BitsPerTexel, TBW);
	//Dbg_MsgAssert(calcVramBytes <= NumVramBytes, ("Need more memory: %d vs. needed %d", NumVramBytes, calcVramBytes));

	// allocate memory for vif stream
#if STEX_USE_CALL
	m_VifSize = 272;
#else
	m_VifSize = 1024 + NumClutBytes + NumTexBytes; //256; //1216 + (((Width*Height+15)>>4)<<4);
#endif // STEX_USE_CALL
	mp_VifData = (uint8 *)Mem::Malloc(m_VifSize);
	Dbg_MsgAssert(mp_VifData, ("couldn't malloc memory for vif data"));
	Dbg_MsgAssert(((uint32)mp_VifData & 15) == 0, ("vif data not qword aligned"));

	// build dma data in this buffer
	dma::pLoc = mp_VifData;

	// calculate TBP
	m_TBP = NextTBP;

	// calculate next TBP
	//NextTBP = (((TBP<<8)+NumVramBytes+0x1FFF) & 0xFFFFE000) >> 8;
	NextTBP = m_TBP + m_NumTexVramBlocks;

	// calculate CBP
	m_CBP = NextTBP;
	if (BitsPerTexel >= 16)
		m_NumClutVramBlocks = 0;
	else if (BitsPerTexel == 4)
		m_NumClutVramBlocks = 4;
	else
		m_NumClutVramBlocks = (CPSM==PSMCT32 ? 4 : 4);

	NextTBP += m_NumClutVramBlocks;

	if (IsInVRAM() && Script::GetInt( "vram_debug", false ))
	{
		Dbg_Message("VRAM Used = %d blocks, texture Size = %d blocks, blocks left = %d out of %d", 
				NextTBP - FontVramStart, m_NumTexVramBlocks + m_NumClutVramBlocks, FontVramSize - (NextTBP - FontVramStart),FontVramSize);  										
	}
							  
	// bail if VRAM would become over-packed
	if (IsInVRAM() && (NextTBP > (FontVramStart + FontVramSize)))
	{
		Dbg_MsgAssert(0, ("SingleTexture: 2D VRAM Buffer Full: %x.", NextTBP));
		//pData += NumClutBytes;
		//pData = (uint8 *)(((uint)(pData+NumTexBytes+15)) & 0xFFFFFFF0);
		// And restore the CurrentDMALoc, which we were just using temporarily.
		dma::pLoc = old_dma_loc;		  
		return false;
	}

	// add clut upload to dma list if necessary
	if (BitsPerTexel < 16)
	{
#if STEX_USE_CALL
		dma::BeginTag(dma::cnt,0);
		vif::NOP();
		vif::DIRECT(6);
		gif::Tag2(gs::A_D,1,PACKED,0,0,0,4);
		mp_ClutBitBltBuf = gs::Reg2_pData(gs::BITBLTBUF,PackBITBLTBUF(0,0,0,m_CBP,1,CPSM));
		gs::Reg2(gs::TRXPOS,	PackTRXPOS(0,0,0,0,0));
		if (PSM==PSMT4)
			gs::Reg2(gs::TRXREG,PackTRXREG(8,2));
		else
			gs::Reg2(gs::TRXREG,PackTRXREG(16,16));
		gs::Reg2(gs::TRXDIR,	0);
		gif::Tag2(0,0,IMAGE,0,0,0,NumClutQWords);
		dma::EndTag();
		dma::Tag(dma::ref, NumClutQWords, (uint)pData);
		vif::NOP();
		vif::DIRECT(NumClutQWords);
#else
		// clut upload header
		gif::Tag2(gs::A_D,1,PACKED,0,0,0,4);
		mp_ClutBitBltBuf = gs::Reg2_pData(gs::BITBLTBUF,PackBITBLTBUF(0,0,0,m_CBP,1,CPSM));
		gs::Reg2(gs::TRXPOS,	PackTRXPOS(0,0,0,0,0));
		if (PSM==PSMT4)
			gs::Reg2(gs::TRXREG,PackTRXREG(8,2));
		else
			gs::Reg2(gs::TRXREG,PackTRXREG(16,16));
		gs::Reg2(gs::TRXDIR,	0);
		gif::Tag2(0,0,IMAGE,0,0,0,NumClutQWords);

		// read clut bitmap data
		memcpy(dma::pLoc, pData, NumClutBytes);

		// step dma location
		dma::pLoc += NumClutBytes;
#endif // STEX_USE_CALL

		// Save pointer
		mp_ClutData = pData;
		m_ClutDataSize = NumClutBytes;

		// step past clut data, and quadword-align
		pData = (uint8 *)(((uint)(pData+NumClutBytes+15)) & 0xFFFFFFF0);
	}

#if STEX_USE_CALL
	// add texture upload to dma list
	dma::BeginTag(dma::cnt,0);
	vif::NOP();
	vif::DIRECT(6);
	gif::Tag2(gs::A_D,1,PACKED,0,0,0,4);
	mp_TexBitBltBuf = gs::Reg2_pData(gs::BITBLTBUF,PackBITBLTBUF(0,0,0,m_TBP,TBW,PSM));
	gs::Reg2(gs::TRXPOS,	PackTRXPOS(0,0,0,0,0));
	gs::Reg2(gs::TRXREG,	PackTRXREG(Width,Height));
	gs::Reg2(gs::TRXDIR,	0);
	gif::Tag2(0, 0, IMAGE, 0, 0, 0, NumTexQWords);
	dma::EndTag();
	dma::Tag(dma::ref, NumTexQWords, (uint)pData);
	vif::NOP();
	vif::DIRECT(NumTexQWords);
#else
	// texture upload header
	gif::Tag2(gs::A_D,1,PACKED,0,0,0,4);
	mp_TexBitBltBuf = gs::Reg2_pData(gs::BITBLTBUF,PackBITBLTBUF(0,0,0,m_TBP,TBW,PSM));
	gs::Reg2(gs::TRXPOS,	PackTRXPOS(0,0,0,0,0));
	gs::Reg2(gs::TRXREG,	PackTRXREG(Width,Height));
	gs::Reg2(gs::TRXDIR,	0);
	gif::Tag2(0, 0, IMAGE, 0, 0, 0, NumTexQWords);

	// read texture bitmap data
	memcpy(dma::pLoc, pData, NumTexBytes);
	dma::pLoc += NumTexQWords<<4;
#endif // STEX_USE_CALL

	// Handle mipmaps, if any (but don't make any DMA packets for them)
	if (!(m_flags & mSPRITE) && (MXL > 0))
	{
		for (j = 1; j < (int) MXL; j++)
		{
			int mip_width = (TW>=0 ? 1<<TW : 0) >> j;
			int mip_height = (TH>=0 ? 1<<TH : 0) >> j;

			// Add in mip sizes
			NumTexBytes += mip_width * mip_height * BitsPerTexel >> 3;
		}
	}

	// Save pointer
	mp_PixelData = pData;
	m_PixelDataSize = NumTexBytes;

	// make entry in texture table
	m_RegTEX0    = PackTEX0(m_TBP,TBW,PSM,TW,TH,1,MODULATE,m_CBP,CPSM,0,0,1);
	m_RegTEX1	 = PackTEX1(1,0,LINEAR,LINEAR,0,0,0);

	// update vram base pointer
	if (IsInVRAM())
	{
		FontVramBase = NextTBP;
	}

#if STEX_USE_CALL
	// Since this data is called, do a return
	dma::Tag(dma::ret, 0, 0);
	vif::NOP();
	vif::NOP();
#endif // STEX_USE_CALL

	#ifdef	__NOPT_ASSERT__
	uint ActualSize = (uint) dma::pLoc - (uint) mp_VifData;
	Dbg_MsgAssert(ActualSize <= m_VifSize, ("Did not allocate enough DMA space for textures: allocated=%d actual=%d", m_VifSize, ActualSize));
	//Dbg_Message("Texture packet size = %d", (int) dma::pLoc - (int) pVifData);
	#endif

	// And restore the CurrentDMALoc, which we were just using temporarily.
	dma::pLoc = old_dma_loc;		  

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Need to do this to textures that are copied directly into the frame buffer
//

void	SSingleTexture::FlipTextureVertical()
{
	int startrow = 0;
	int endrow = m_orig_height - 1;
	int rowsize = (m_orig_width * GetBitdepth()) / 8;

	uint8 *p_temp_buffer = new uint8[rowsize];
	while (startrow < endrow)
	{
		// Swap rows
		memcpy(p_temp_buffer,						&(mp_PixelData[startrow * rowsize]),rowsize);
		memcpy(&(mp_PixelData[startrow * rowsize]),	&(mp_PixelData[endrow * rowsize]),	rowsize);
		memcpy(&(mp_PixelData[endrow * rowsize]),	p_temp_buffer,						rowsize);

		startrow++;
		endrow--;
	}

	delete p_temp_buffer;
}

/////////////////////////////////////////////////////////////////////////////
//
bool SSingleTexture::AddToVRAM()
{
	Dbg_MsgAssert(!(m_flags & mAUTO_ALLOC), ("Can't use AddToVRAM() on a auto allocated texture"));

	#ifdef			__NOPT_ASSERT__
//	printf ("++++ Adding %s ++++\n",m_name);
	#endif 
	m_flags |= mALLOC_VRAM;

	VRAMNeedsUpdating();

	return true;
}

/////////////////////////////////////////////////////////////////////////////
//
bool SSingleTexture::RemoveFromVRAM()
{
	Dbg_MsgAssert(!(m_flags & mAUTO_ALLOC), ("Can't use RemoveFromVRAM() on a auto allocated texture"));

	#ifdef			__NOPT_ASSERT__
//	printf ("---- removing %s ----\n",m_name);
	#endif 
	m_flags &= ~mALLOC_VRAM;

	VRAMNeedsUpdating();

	return true;
}

/////////////////////////////////////////////////////////////////////////////
//

void SSingleTexture::SetAutoAllocate()
{
	m_flags |= mAUTO_ALLOC;
}

/////////////////////////////////////////////////////////////////////////////
//

void SSingleTexture::inc_usage_count()
{
	m_usage_count++;
	m_flags |= mALLOC_VRAM;
}

/////////////////////////////////////////////////////////////////////////////
//

void SSingleTexture::dec_usage_count()
{
	if (--m_usage_count == 0)
	{
		m_flags &= ~mALLOC_VRAM;
	}

	Dbg_MsgAssert(m_usage_count >= 0, ("Usage Count negative on  SSingleTexture %x", m_checksum));
}

/////////////////////////////////////////////////////////////////////////////
//

void SSingleTexture::UseTexture(bool use)
{
	Dbg_MsgAssert(m_flags & mAUTO_ALLOC, ("Can't use UseTexture() on a non-auto allocated texture"));

	bool old_alloc = IsInVRAM();

	// Update usage
	if (use)
	{
		inc_usage_count();
	}
	else
	{
		dec_usage_count();
	}

	// See if we need to update DMA
	if (old_alloc != use)
	{
		VRAMNeedsUpdating();
	}
}

/////////////////////////////////////////////////////////////////////////////
//

uint16	SSingleTexture::GetOrigWidth() const
{
	return m_orig_width;
}

/////////////////////////////////////////////////////////////////////////////
//

uint16	SSingleTexture::GetOrigHeight() const
{
	return m_orig_height;
}

/////////////////////////////////////////////////////////////////////////////
//

void	SSingleTexture::ReallocateVRAM()
{
	// Don't realloc load screens
	if (!IsInVRAM()) return;

	// calculate TBP
	m_TBP = FontVramBase;
	FontVramBase += m_NumTexVramBlocks;

	// calculate CBP
	m_CBP = FontVramBase;
	FontVramBase += m_NumClutVramBlocks;

	// bail if VRAM would become over-packed
	if (FontVramBase > (FontVramStart + FontVramSize))
	{
		Dbg_MsgAssert(0, ("SingleTexture Realloc: 2D VRAM Buffer Full: %x.", FontVramBase));
	}

	// Modify the data
	gs::ModifyTBP(gs::TEX0_1, (uint32 *) &m_RegTEX0, m_TBP, m_CBP);
}

/////////////////////////////////////////////////////////////////////////////
//

void	SSingleTexture::UpdateDMA()
{
	// Modify the packets
	gs::ModifyTBP(gs::BITBLTBUF, mp_TexBitBltBuf, m_TBP);
	if (mp_ClutBitBltBuf)
	{
		gs::ModifyTBP(gs::BITBLTBUF, mp_ClutBitBltBuf, m_CBP);
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// The following Get() calls are very inefficient.  They should only be
// used when necessary (i.e. initialization).
//
uint16	SSingleTexture::GetWidth() const
{
	sceGsTex0 tex0 = *((sceGsTex0 *) &m_RegTEX0);
	uint16 TW = tex0.TW;
	//uint16 TW = (uint16) ((RegTEX0 >> 26) & M04);

	return 1 << TW;
}

uint16	SSingleTexture::GetHeight() const
{
	sceGsTex0 tex0 = *((sceGsTex0 *) &m_RegTEX0);
	uint16 TH = tex0.TH;
	//uint16 TH = (uint16) ((RegTEX0 >> 30) & M04);

	return 1 << TH;
}

uint8	SSingleTexture::GetBitdepth() const
{
	sceGsTex0 tex0 = *((sceGsTex0 *) &m_RegTEX0);
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

uint8	SSingleTexture::GetClutBitdepth() const
{
	sceGsTex0 tex0 = *((sceGsTex0 *) &m_RegTEX0);
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

uint8	SSingleTexture::GetNumMipmaps() const
{
	return 1;		// By definition for this class
}

bool	SSingleTexture::IsTransparent() const
{
	//Dbg_Error("Not working yet");
	return false;
}

uint32	SSingleTexture::bitdepth_to_psm(int bitdepth)
{
	switch (bitdepth)
	{
	case 32:
		return PSMCT32;
		break;
	case 24:
		return PSMCT24;
		break;
	case 16:
		return PSMCT16;
		break;
	case 8:
		return PSMT8;
		break;
	case 4:
		return PSMT4;
		break;
	default:
		Dbg_MsgAssert(0, ("Bitdepth %d is not supported on PS2", bitdepth));
		return PSMCT32;
		break;
	}
}

#ifdef			__NOPT_ASSERT__
void	SSingleTexture::sPrintAllTextureInfo()
{
	for (SSingleTexture *pTexture=SSingleTexture::sp_stexture_list; pTexture; pTexture=pTexture->mp_next)
	{
		Dbg_Message("Texture name %s: using vram: %d, blocks %d", pTexture->m_name, pTexture->IsInVRAM(),
					pTexture->m_NumTexVramBlocks + pTexture->m_NumClutVramBlocks);
	}
}

void	SSingleTexture::sPrintAllocatedTextureInfo()
{
	for (SSingleTexture *pTexture=SSingleTexture::sp_stexture_list; pTexture; pTexture=pTexture->mp_next)
	{
		if (!pTexture->IsInVRAM())
		{
			continue;
		}

		Dbg_Message("Texture name %s: using vram: <true>, blocks %d", pTexture->m_name,
					pTexture->m_NumTexVramBlocks + pTexture->m_NumClutVramBlocks);
	}
}

#endif

/******************************************************************/
/*                                                                */
/* SScissorWindow												  */
/*                                                                */
/******************************************************************/

SScissorWindow DefaultScissorWindow;

void SetTextWindow(uint16 x0, uint16 x1, uint16 y0, uint16 y1)
{
	DefaultScissorWindow.SetScissor(x0, y0, x1, y1);
}

void SScissorWindow::SetScissor(uint16 x0, uint16 y0, uint16 x1, uint16 y1)
{
	m_RegSCISSOR = (uint64)x0 | (uint64)x1<<16 | (uint64)y0<<32 | (uint64)y1<<48;
}

/******************************************************************/
/*                                                                */
/* SDraw2D														  */
/*                                                                */
/******************************************************************/

float		SDraw2D::s_screen_scale_x = 1.0f;
float		SDraw2D::s_screen_scale_y = 1.0f;

bool		SDraw2D::s_constant_z_enabled = false;
uint32		SDraw2D::s_constant_z = 0x7FFFFF;

SDraw2D *	SDraw2D::sp_2D_sorted_draw_list = NULL;
SDraw2D *	SDraw2D::sp_2D_zbuffer_draw_list = NULL;


/////////////////////////////////////////////////////////////////////////////
//

SDraw2D::SDraw2D(float pri, bool hide) :
	m_hidden(hide),
	m_use_zbuffer(false),
	m_pri(pri),
	m_z(0xFFFFFF),
	mp_scissor_window(&DefaultScissorWindow)
{
	mp_next = NULL;

	// add to sorted draw list
	if (!m_hidden)
	{
		InsertSortedDrawList(&sp_2D_sorted_draw_list);
	}
}

/////////////////////////////////////////////////////////////////////////////
//

SDraw2D::~SDraw2D()
{
	// Try removing from draw list
	if (m_use_zbuffer)
	{
		RemoveDrawList(&sp_2D_zbuffer_draw_list);
	} else {
		RemoveDrawList(&sp_2D_sorted_draw_list);
	}
}

/////////////////////////////////////////////////////////////////////////////
//

void SDraw2D::SetPriority(float pri)
{
	if (m_pri != pri)
	{
		m_pri = pri;

		// By removing and re-inserting, we re-sort the list
		if (!m_hidden)
		{
			if (m_use_zbuffer)
			{
				RemoveDrawList(&sp_2D_zbuffer_draw_list);
			} else {
				RemoveDrawList(&sp_2D_sorted_draw_list);
			}
			InsertSortedDrawList(&sp_2D_sorted_draw_list);
		}

		m_use_zbuffer = false;
		m_z = 0xFFFFFF;
	}
}

/////////////////////////////////////////////////////////////////////////////
//

void SDraw2D::SetZValue(uint32 z)
{
	if (m_z != z)
	{
		m_z = z;

		// Switch lists, if necessary
		if (!m_use_zbuffer && !m_hidden)
		{
			RemoveDrawList(&sp_2D_sorted_draw_list);
			InsertDrawList(&sp_2D_zbuffer_draw_list);
		}

		m_use_zbuffer = true;
		m_pri = 0.0f;
	}
}

/////////////////////////////////////////////////////////////////////////////
//

void SDraw2D::SetHidden(bool hide)
{
	if (m_hidden != hide)
	{
		m_hidden = hide;
		if (m_use_zbuffer)
		{
			if (hide)
			{
				RemoveDrawList(&sp_2D_zbuffer_draw_list);
			} else {
				InsertDrawList(&sp_2D_zbuffer_draw_list);
			}
		} else {
			if (hide)
			{
				RemoveDrawList(&sp_2D_sorted_draw_list);
			} else {
				InsertSortedDrawList(&sp_2D_sorted_draw_list);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
//

void SDraw2D::SetScissorWindow(SScissorWindow *p_scissor)
{
	if (p_scissor)
	{
		mp_scissor_window = p_scissor;
	} else {
		mp_scissor_window = &DefaultScissorWindow;
	}
}

/////////////////////////////////////////////////////////////////////////////
//

void SDraw2D::DrawAll()
{
	// Figure out screen scale (for PAL mode)
	CalcScreenScale();

	// Init certain GS registers for 2D drawing
	dma::BeginTag(dma::cnt, 0);
	gs::BeginPrim(ABS,0,0);
	gs::Reg1(gs::CLAMP_1,PackCLAMP(CLAMP,CLAMP,0,0,0,0));		// Man, I hope this is the last time I fix the CLAMP bug
	gs::Reg1(gs::XYOFFSET_1, PackXYOFFSET(XOFFSET, YOFFSET));
	gs::Reg1(gs::TEST_1, PackTEST(1,AGREATER,0,KEEP,0,0,1,ZGEQUAL));
	gs::EndPrim(0);
	dma::EndTag();

	// Draw items needing ZBuffer first
	DrawList(sp_2D_zbuffer_draw_list);
	if (s_constant_z_enabled)
	{
		DrawList(sp_2D_sorted_draw_list);
	}

	// Turn off ZBuffer
	dma::BeginTag(dma::cnt, 0);
	gs::BeginPrim(ABS,0,0);
	gs::Reg1(gs::TEST_1, PackTEST(0,0,0,0,0,0,1,ZGEQUAL));
	gs::EndPrim(0);
	dma::EndTag();

	// Draw the sorted list now if we aren't using constant Z
	if (!s_constant_z_enabled)
	{
		DrawList(sp_2D_sorted_draw_list);
	}
}

/////////////////////////////////////////////////////////////////////////////
//

void SDraw2D::DrawList(SDraw2D *pDraw)
{
	while (pDraw)
	{
		if (!pDraw->m_hidden)
		{
			pDraw->BeginDraw();
			pDraw->Draw();
			pDraw->EndDraw();
		}
		pDraw = pDraw->mp_next;
	}
}

/////////////////////////////////////////////////////////////////////////////
//

void SDraw2D::EnableConstantZValue(bool enable)
{
	s_constant_z_enabled = enable;
}

/////////////////////////////////////////////////////////////////////////////
//

void SDraw2D::SetConstantZValue(uint32 z)
{
	s_constant_z = z;
}

/////////////////////////////////////////////////////////////////////////////
//

uint32 SDraw2D::GetConstantZValue()
{
	return s_constant_z;
}


/////////////////////////////////////////////////////////////////////////////
// Figure out screen scale (mainly for PAL mode)
//

void SDraw2D::CalcScreenScale()
{
	if (Config::NTSC())
	{
		s_screen_scale_x = 1.0f;
		s_screen_scale_y = 1.0f;
	} else {
		s_screen_scale_x = ((float) HRES_PAL) / ((float) HRES_NTSC);
		s_screen_scale_y = ((float) VRES_PAL) / ((float) VRES_NTSC);
	}
}

/////////////////////////////////////////////////////////////////////////////
//

void SDraw2D::InsertSortedDrawList(SDraw2D **p_list)
{
	Dbg_Assert(!m_use_zbuffer);

	if (!*p_list ||							// Empty List
		(m_pri <= (*p_list)->m_pri))		// Start List
	{
		mp_next = *p_list;
		*p_list = this;
	} else {				// Insert
		SDraw2D *p_cur = *p_list;
	
		// Find where to insert
		while(p_cur->mp_next)
		{
			if (m_pri <= p_cur->mp_next->m_pri)
				break;

			p_cur = p_cur->mp_next;
		}

		// Insert at this point
		mp_next = p_cur->mp_next;
		p_cur->mp_next = this;
	}
}

/////////////////////////////////////////////////////////////////////////////
//

void SDraw2D::InsertDrawList(SDraw2D **p_list)
{
	if (!*p_list)							// Empty List
	{
		mp_next = *p_list;
		*p_list = this;
	} else {								// Insert at end of list
		SDraw2D *p_cur = *p_list;

		while(p_cur->mp_next)
		{
			p_cur = p_cur->mp_next;
		}

		mp_next = p_cur->mp_next;
		p_cur->mp_next = this;
	}
}

/////////////////////////////////////////////////////////////////////////////
//

void SDraw2D::RemoveDrawList(SDraw2D **p_list)
{
	// Take out from draw list
	if (*p_list == this)
	{
		*p_list = mp_next;
	} 
	else if (*p_list) 
	{
		SDraw2D *p_cur = *p_list;

		while(p_cur->mp_next)
		{
			if (p_cur->mp_next == this)
			{
				p_cur->mp_next = mp_next;
				break;
			}

			p_cur = p_cur->mp_next;
		}
	}
}


/******************************************************************/
/*                                                                */
/* SSprite														  */
/*                                                                */
/******************************************************************/

/////////////////////////////////////////////////////////////////////////////
//

SSprite::SSprite(float pri) : SDraw2D(pri, true)
{
}

/////////////////////////////////////////////////////////////////////////////
//

SSprite::~SSprite()
{
	// Deallocates the underlying texture, if there was one
	SetHidden(true);
}

/////////////////////////////////////////////////////////////////////////////
//

void SSprite::SetHidden(bool hide)
{
	bool old_hide = IsHidden();

	// Do the actual hide call
	SDraw2D::SetHidden(hide);

	// Make sure vram status is correct if we changed hidden states
	if (mp_texture && (old_hide != hide))
	{
		mp_texture->UseTexture(!hide);
	}
}

/////////////////////////////////////////////////////////////////////////////
//

void SSprite::SetTexture(SSingleTexture *p_texture)
{
	// Set auto VRAM allocate
	if (p_texture && (mp_texture != p_texture))
	{
		p_texture->SetAutoAllocate();

		// Change usage counts if we aren't hidden
		if (!IsHidden())
		{
			p_texture->UseTexture(true);
			if (mp_texture)
			{
				mp_texture->UseTexture(false);
			}
		}
	}

	mp_texture = p_texture;
}

/////////////////////////////////////////////////////////////////////////////
//

void SSprite::BeginDraw()
{

	// dma tag
	dma::BeginTag(dma::cnt, 0);

	vu1::Buffer = vu1::Loc;

	int tme_flag = (mp_texture) ? TME : 0;

	Dbg_Assert(GetScissorWindow());

	// GS context
	gs::BeginPrim(ABS,0,0);
	gs::Reg1(gs::SCISSOR_1, GetScissorWindow()->GetScissorReg());
	if (mp_texture)
	{
		Dbg_MsgAssert(mp_texture->IsInVRAM(), ("Can't draw sprite when its texture 0x%x, (%s) is not in VRAM.",mp_texture->m_checksum,mp_texture->m_name));

		gs::Reg1(gs::TEX0_1,	 mp_texture->m_RegTEX0);
		gs::Reg1(gs::TEX1_1,	 mp_texture->m_RegTEX1);
	}
	gs::Reg1(gs::ALPHA_1,	 PackALPHA(0,1,0,1,0));
	gs::Reg1(gs::COLCLAMP,	 PackCOLCLAMP(1));
	gs::Reg1(gs::RGBAQ,	 (uint64)m_rgba);
	gs::EndPrim(0);

	// unpack giftag
	vif::UNPACK(0, V4_32, 1, ABS, UNSIGNED, 0);
	if (m_rot == 0.0f)
	{
		gif::BeginTagImmediate(gs::UV|gs::XYZ2<<4, 2, PACKED, SPRITE|tme_flag|FST|ABE, 1, VU1_ADDR(GSPrim));
	} else {
		gif::BeginTagImmediate(gs::UV|gs::XYZ2<<4, 2, PACKED, TRISTRIP|tme_flag|FST|ABE, 1, VU1_ADDR(GSPrim));
	}

	// begin unpack for uv/xyz's
	vif::BeginUNPACK(0, V4_32, ABS, SIGNED, 1);
}


/////////////////////////////////////////////////////////////////////////////
//

void SSprite::Draw()
{
	Dbg_Assert(GetScissorWindow());

	const int start_screen_x = XOFFSET + ((GetScissorWindow()->GetScissorReg() & 0x7FF) << 4) + ((int) (m_xpos * s_screen_scale_x * 16.0f));
	const int start_screen_y = YOFFSET + ((GetScissorWindow()->GetScissorReg() >> 32 & 0x7FF) << 4) + ((int) (m_ypos * s_screen_scale_y * 16.0f));

	uint16 u0,v0,u1,v1;
	float x0,y0,x1,y1;
	uint32 z;

	uint16 tex_height = (mp_texture) ? mp_texture->GetHeight() : m_height;

	u0 = /* 8 +*/ (0);
	v0 = /*-8 +*/ (tex_height << 4);
	u1 = /* 8 +*/ (m_width << 4);
	v1 = /*-8 +*/ ((tex_height - m_height) << 4);

	// Check for flip
	float abs_scale_x = m_scale_x;// * s_screen_scale_x;
	float abs_scale_y = m_scale_y;// * s_screen_scale_y;
	if (abs_scale_x < 0.0f)
	{
		uint16 temp = u0;
		u0 = u1;
		u1 = temp;
		abs_scale_x = -abs_scale_x;
	}
	if (abs_scale_y < 0.0f)
	{
		uint16 temp = v0;
		v0 = v1;
		v1 = temp;
		abs_scale_y = -abs_scale_y;
	}

	x0 = -m_xhot * 16.0f * abs_scale_x;
	y0 = -m_yhot * 16.0f * abs_scale_y;

	x1 = x0 + ((float) (u1 - u0) * abs_scale_x);
	y1 = y0 + ((float) (v0 - v1) * abs_scale_y);

	z = s_constant_z_enabled ? s_constant_z : GetZValue();

	// Flip V if texture is inverted in VRAM
	if (mp_texture && mp_texture->IsInverted())
	{
		uint16 temp = v0;
		v0 = v1;
		v1 = temp;
	}

	// Check for rotation
	if (m_rot != 0.0f)
	{
		Mth::Vector p0(x0, y0, 0.0f, 0.0f);
		Mth::Vector p1(x1, y0, 0.0f, 0.0f);
		Mth::Vector p2(x0, y1, 0.0f, 0.0f);
		Mth::Vector p3(x1, y1, 0.0f, 0.0f);

		p0.RotateZ(m_rot);
		p1.RotateZ(m_rot);
		p2.RotateZ(m_rot);
		p3.RotateZ(m_rot);

//printf("Before (%d, %d) - (%d, %d)\n", x0, y0, x1, y1);
//printf("After (%d, %d) - (%d, %d)\n", (int) p0.GetX(), (int) p0.GetY(), (int) p3.GetX(), (int) p3.GetY());

		int xi0 = ((int) (p0.GetX() * s_screen_scale_x)) + start_screen_x;
		int xi1 = ((int) (p1.GetX() * s_screen_scale_x)) + start_screen_x;
		int xi2 = ((int) (p2.GetX() * s_screen_scale_x)) + start_screen_x;
		int xi3 = ((int) (p3.GetX() * s_screen_scale_x)) + start_screen_x;
		int yi0 = ((int) (p0.GetY() * s_screen_scale_y)) + start_screen_y;
		int yi1 = ((int) (p1.GetY() * s_screen_scale_y)) + start_screen_y;
		int yi2 = ((int) (p2.GetY() * s_screen_scale_y)) + start_screen_y;
		int yi3 = ((int) (p3.GetY() * s_screen_scale_y)) + start_screen_y;

		// Cull, if necessary
		if (NeedsCulling() && ((xi0 | xi1 | xi2 | xi3 | yi0 | yi1 | yi2 | yi3) & 0xFFFF0000))
		{
			return;
		}

		// Do a TRISTRIP
		vif::StoreV4_32(u0,v0,0,0);
		vif::StoreV4_32(xi0, yi0, z, 0);
		vif::StoreV4_32(u1,v0,0,0);
		vif::StoreV4_32(xi1, yi1, z, 0);
		vif::StoreV4_32(u0,v1,0,0);
		vif::StoreV4_32(xi2, yi2, z, 0);
		vif::StoreV4_32(u1,v1,0,0);
		vif::StoreV4_32(xi3, yi3, z, 0);
	} else {
		// Just outputting two points for a SPRITE
		int xi0 = ((int) (x0 * s_screen_scale_x)) + start_screen_x;
		int xi1 = ((int) (x1 * s_screen_scale_x)) + start_screen_x;
		int yi0 = ((int) (y0 * s_screen_scale_y)) + start_screen_y;
		int yi1 = ((int) (y1 * s_screen_scale_y)) + start_screen_y;

		// Cull, if necessary
		if (NeedsCulling() && ((xi0 | xi1 | yi0 | yi1) & 0xFFFF0000))
		{
			return;
		}

		vif::StoreV4_32(u0,v0,0,0);
		vif::StoreV4_32(xi0,yi0,z,0);
		vif::StoreV4_32(u1,v1,0,0);
		vif::StoreV4_32(xi1,yi1,z,0);
	}
}

/////////////////////////////////////////////////////////////////////////////
//

void SSprite::EndDraw(void)
{
	vif::EndUNPACK();
	gif::EndTagImmediate(1);
	vif::MSCAL(VU1_ADDR(Parser));
	dma::EndTag();
}

} // namespace NxPs2

