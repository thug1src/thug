#include <string.h>
#include <core/defines.h>
#include <core/debug.h>
#include <core/string/stringutils.h>
#include <sys/file/filesys.h>
#include <sifdev.h>
#include <stdlib.h>
#include <gfx/nxfontman.h>
#include "chars.h"
#include "dma.h"
#include "vif.h"
#include "vu1.h"
#include "gif.h"
#include "gs.h"
#include "vu1code.h"
#include "texturemem.h"
#include "render.h"
#include "switches.h"

/*


	
.fnt file format (by Ryan)
--------------------------

	4	File size in bytes
	4	Number of characters
	4	Default height
	4	Default base
	
	?	Character table (see below)
	?	Texture (see below)

	Character
	2	Baseline (how many pixels down relative to top of image)
	2	Ascii value

	Texture
	4	Size of texture
	2	Width
	2	Height
	2	Bit depth
	6	Padding
	W*H	Raw data
	0-3	Padding for uint32 alignment
	1K	Palette data
	4	Number of subtextures
	?	Subtexture table (see below)

	Subtexture
	2	X
	2	Y
	2	W
	2	H

	
	
	
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


float CurrentScale;
uint32 FontVramStart;
uint32 FontVramBase;
SFont *pFontList;
SFont *pButtonsFont = NULL;


// Garrett: Now uses both 648K buffers at once, so we'd need to undo this if we want to
// implement double buffering.
const uint32 FontVramSize = 0xA20 * 2;	// 648K * 2

extern TextureMemoryLayout TexMemLayout;

SFont *SText::spOverrideFont = NULL;

void * OpenExtended(const char *Filename)
{
	char *ExtendedFilename;

	// allocate memory for extended name
	ExtendedFilename = (char *)Mem::Malloc(strlen(Filename)+17);	// 12 for "fonts/" plus 4 for ".fnt" plus 1 for \0
	Dbg_MsgAssert(ExtendedFilename, ("couldn't malloc memory for extended filename fonts/%s", Filename));

	// copy and extend name
	strcpy(ExtendedFilename, "fonts/");
	strcat(ExtendedFilename, Filename);
	strcat(ExtendedFilename, ".fnt.ps2");

	// open file
	void *pFile = File::Open(ExtendedFilename, "rb");
	Dbg_MsgAssert(pFile>=0, ("couldn't open file %s", Filename));

	// free memory
	Mem::Free(ExtendedFilename);

	return pFile;
}





SFont * LoadFont(const char *Filename)
{
	SFont *pFont;
	SChar *pChar;
	uint8 *pData;
	void *pFile;
	int i,j,Len,NumChars,Width,Height,NumQWords,NumBytes;
	uint32 TW,TH,TBW;
	uint32 temp;

		  
	uint8*	old_dma_loc = dma::pLoc;		  

	// open the font file
	pFile = OpenExtended(Filename);


	// allocate memory for the font structure
	pFont = (SFont *)Mem::Malloc(sizeof(SFont));
	Dbg_MsgAssert(pFont, ("couldn't malloc memory for SFont"));


	// allocate a temporary buffer
	uint8 FontBuf[2048];
	Dbg_MsgAssert(FontBuf, ("couldn't malloc memory for FontBuf"));


	// load file header
	Len = File::Read(FontBuf, 1, 16, pFile);
	Dbg_MsgAssert(Len==16, ("couldn't read file header from font file %s", Filename));
	NumChars			 = ((uint32 *)FontBuf)[1];
	pFont->DefaultHeight = ((uint32 *)FontBuf)[2] << 4;		// *16 for texel coords
	pFont->DefaultBase	 = ((uint32 *)FontBuf)[3] << 4;		// *16 for texel coords


	// clear character map to zero
	memset(pFont->Map, 0, 256);
	memset(pFont->SpecialMap, 0, 32);


	// allocate memory for character table
	pFont->pChars = (SChar *)Mem::Malloc(NumChars*sizeof(SChar));
	Dbg_MsgAssert(pFont->pChars, ("couldn't malloc memory for font character table"));


	// load character map and character table
	Len = File::Read(FontBuf, 1, NumChars<<2, pFile);
	Dbg_MsgAssert(Len==(NumChars<<2), ("couldn't read character table in font file %s", Filename));

	for (i=0,pChar=pFont->pChars,pData=FontBuf; i<NumChars; i++,pChar++,pData+=4)
	{
		pChar->Baseline = ((uint16 *)pData)[0] << 4;		// *16 for texel coords
		sint16 ascii_val = ((sint16 *)pData)[1];
		if (ascii_val >= 0)
			pFont->Map[(uint8) ascii_val] = i;
		else
		{
			Dbg_Assert(ascii_val >= -32)
			pFont->SpecialMap[(uint8) (-ascii_val - 1)] = i;
		}
	}

	// now, if there is a null character in the font, make characters that could not be found
	// in the font display that instead of 'A'
	if (pFont->SpecialMap[31] != 0)
	{
		for (i = 0; i < 256; i++) 
		{
			if (pFont->Map[i] == 0 && i != 'A' && i != 'a') pFont->Map[i] = pFont->SpecialMap[31];
			if (i < 31 && pFont->SpecialMap[i] == 0) pFont->SpecialMap[i] = pFont->SpecialMap[31];
		}
	}	
	
	// load texture header
	Len = File::Read(FontBuf, 1, 16, pFile);
	Dbg_MsgAssert(Len==16, ("couldn't read texture header from font file %s", Filename));
	Width  = ((uint16 *)FontBuf)[2];
	Height = ((uint16 *)FontBuf)[3];


	// allocate memory for vif stream
	pFont->VifSize = 1216 + (((Width*Height+15)>>4)<<4);
	pFont->pVifData = (uint8 *)Mem::Malloc(pFont->VifSize);
	Dbg_MsgAssert(pFont->pVifData, ("couldn't malloc memory for vif data"));
	Dbg_MsgAssert(((uint32)pFont->pVifData & 15) == 0, ("vif data not qword aligned"));



	// build dma data in this buffer
	dma::pLoc = pFont->pVifData;

	// Calculate memory needed
	int min_width = Width, min_height = Height;
	TexMemLayout.getMinSize(min_width, min_height, PSMT8);
	pFont->m_NumTexVramBlocks = TexMemLayout.getImageSize(min_width, min_height, PSMT8);
	pFont->m_NumClutVramBlocks = 4;


	// set base pointers and texture buffer width
	pFont->m_CBP = FontVramBase;
	pFont->m_TBP = FontVramBase + pFont->m_NumClutVramBlocks;
	TBW = (Width+63) >> 6;
	if (TBW<2)
		TBW=2;


	// texture upload format:
	//
	// GIFtag PACKED 4
	// BITBLTBUF ?
	// TRXPOS 0
	// TRXREG w,h
	// TRXDIR 0
	//
	// GIFtag IMAGE (w*h+15)/16
	// ... (tex data)

	// texture upload header
	NumQWords = (Width*Height+15)>>4;
	gif::Tag2(gs::A_D,1,PACKED,0,0,0,4);
	pFont->mp_TexBitBltBuf = gs::Reg2_pData(gs::BITBLTBUF,PackBITBLTBUF(0,0,0,pFont->m_TBP,TBW,PSMT8));
	gs::Reg2(gs::TRXPOS,	PackTRXPOS(0,0,0,0,0));
	gs::Reg2(gs::TRXREG,	PackTRXREG(Width,Height));
	gs::Reg2(gs::TRXDIR,	0);
	gif::Tag2(0, 0, IMAGE, 0, 0, 0, NumQWords);

	// read texture bitmap data
	NumBytes = (Width*Height+3)&0xFFFFFFFC;
	Len = File::Read(dma::pLoc, 1, NumBytes, pFile);
	Dbg_MsgAssert(Len==NumBytes, ("couldn't read texture bitmap from font file %s", Filename));
	dma::pLoc += NumQWords<<4;


	// clut upload format:
	//
	// GIFtag PACKED 4
	// BITBLTBUF ?
	// TRXPOS 0
	// TRXREG w,h
	// TRXDIR 0
	//
	// GIFtag IMAGE 64
	// ... (clut data)

	// clut upload header
	gif::Tag2(gs::A_D,1,PACKED,0,0,0,4);
	pFont->mp_ClutBitBltBuf = gs::Reg2_pData(gs::BITBLTBUF,PackBITBLTBUF(0,0,0,pFont->m_CBP,1,PSMCT32));
	gs::Reg2(gs::TRXPOS,	PackTRXPOS(0,0,0,0,0));
	gs::Reg2(gs::TRXREG,	PackTRXREG(16,16));
	gs::Reg2(gs::TRXDIR,	0);
	gif::Tag2(0,0,IMAGE,0,0,0,64);

	// read clut bitmap data
	Len = File::Read(dma::pLoc, 1, 1024, pFile);
	Dbg_MsgAssert(Len==1024, ("couldn't read clut bitmap from font file %s", Filename));
	
	// swizzle the clut data
	for (i=0; i<256; i+=32)
		for (j=0; j<8; j++)
		{
			temp = ((uint32 *)dma::pLoc)[i+j+8];
			((uint32 *)dma::pLoc)[i+j+8] = ((uint32 *)dma::pLoc)[i+j+16];
			((uint32 *)dma::pLoc)[i+j+16] = temp;
		}
	
	// step dma location
	dma::pLoc += 1024;


	// skip numsubtextures, and load subtextures
	Len = File::Read(FontBuf, 1, (NumChars<<3)+4, pFile);
	Dbg_MsgAssert(Len==(NumChars<<3)+4, ("couldn't read subtexture table from font file %s", Filename));
	for (i=0,pChar=pFont->pChars,pData=FontBuf+4; i<NumChars; i++,pChar++,pData+=8)
	{
		pChar->u0 = (((uint16 *)pData)[0] << 4) + 8;
		pChar->v0 = (((uint16 *)pData)[1] << 4) + 8;
		pChar->u1 = pChar->u0 + (((uint16 *)pData)[2] << 4);
		pChar->v1 = pChar->v0 + (((uint16 *)pData)[3] << 4);
	}

	// set font GS context
	for (TW=0; (1<<TW)<Width; TW++)
		;
	for (TH=0; (1<<TH)<Height; TH++)
		;
	pFont->RegTEX0 = PackTEX0(pFont->m_TBP,TBW,PSMT8,TW,TH,1,MODULATE,pFont->m_CBP,PSMCT32,0,0,1);
	pFont->RegTEX1 = PackTEX1(1,0,LINEAR,LINEAR,0,0,0);


	// update vram base pointer
	//FontVramBase = (((pFont->TBP<<8) + (TBW << 6)*Height + 0x1FFF) & 0xFFFFE000) >> 8;
	FontVramBase +=	(pFont->m_NumTexVramBlocks + pFont->m_NumClutVramBlocks);
	Dbg_MsgAssert(FontVramBase < (FontVramStart + FontVramSize), ("Font: 2D VRAM Buffer Full: %x.", FontVramBase));


	// add font to font list
	pFont->pNext = pFontList;
	pFontList = pFont;

	// we're done with the font file now
	File::Close(pFile);
	
	// And restore the CurrentDMALoc, which we were just using temporarily.
	dma::pLoc = old_dma_loc;	  

	// this will serve as the default spacing
	pFont->mSpaceSpacing = (pFont->pChars[pFont->Map['I']].u1 - pFont->pChars[pFont->Map['I']].u0) >> 4;
	
	return pFont;
}



void UnloadFont(SFont *pFont)
{
	SFont *pPrevFont;
	int found=0;

	// find font and unchain from list
	if (pFontList == pFont)
	{
		found=1;
		pFontList = pFontList->pNext;
	}
	else
	{
		for (pPrevFont=pFontList; pPrevFont->pNext; pPrevFont=pPrevFont->pNext)
		{
			if (pPrevFont->pNext == pFont)
			{
				found=1;
				pPrevFont->pNext = pFont->pNext;
				break;
			}
		}
	}

	Dbg_MsgAssert(found, ("attempt to unload font which has not been loaded"));

	// free memory
	Mem::Free(pFont->pChars);
	Mem::Free(pFont->pVifData);
	Mem::Free(pFont);

	// and re-do VRAM allocation
	VRAMNeedsUpdating();
}

/////////////////////////////////////////////////////////////////////////////
//

void SFont::ReallocateVRAM()
{
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
	gs::ModifyTBP(gs::TEX0_1, (uint32 *) &RegTEX0, m_TBP, m_CBP);
}

/////////////////////////////////////////////////////////////////////////////
//

void SFont::UpdateDMA()
{
	// Modify the packets
	gs::ModifyTBP(gs::BITBLTBUF, mp_TexBitBltBuf, m_TBP);
	if (mp_ClutBitBltBuf)
	{
		gs::ModifyTBP(gs::BITBLTBUF, mp_ClutBitBltBuf, m_CBP);
	}
}

uint32 SFont::GetDefaultHeight() const
{
	return DefaultHeight;
}

uint32 SFont::GetDefaultBase() const
{
	return DefaultBase;
}

void SFont::QueryString(char *String, float &width, float &height)
{
	SChar *pChar;
	char *pLetter;
	uint16 u0,v0,x0,u1,v1,x1;

	x0 = 0;

	//sint16 char_spacing = 16 + (mCharSpacing<<4);
	sint16 char_spacing = mCharSpacing<<4;
	sint16 space_spacing = mSpaceSpacing<<4;
	
	for (pLetter=String;; pLetter++)
	{
		pChar = NULL;
		// may be overridden by the '\b' tag
		SFont *p_font = this;
		
		// acount for tags (might be multiple ones in a row)
		bool got_char_tag = false; // tag resulting in output of character
		while (*pLetter == '\\' && !got_char_tag)
		{
			pLetter++;
			if (*pLetter == '\\')
				break;

			switch(*pLetter)
			{
				case '\\':
					got_char_tag = true;
					break;
				case 'c':
				case 'C':
					pLetter += 2; // skip over "c#"
					break;
				case 's':
				case 'S':
				{
					pLetter++; // skip "s"
					uint digit = Str::DehexifyDigit(pLetter);
					pChar = pChars + SpecialMap[digit];
					got_char_tag = true;
					break;
				}
				case 'b':
				case 'B':
				{
					pLetter++; // skip "b"
					uint digit = Str::DehexifyDigit(pLetter);
					
					// switch over to buttons font, the regular font will be used again on the next character

					p_font = pButtonsFont;
					Dbg_Assert(p_font);
					pChar = p_font->pChars + p_font->SpecialMap[digit];
					got_char_tag = true;
					break;
				}
				case 'm':
				case 'M':
				{
					pLetter++; // skip "m"
					char button_char = Nx::CFontManager::sMapMetaCharacterToButton(pLetter);
					uint digit = Str::DehexifyDigit(&button_char);
					
					p_font = pButtonsFont;
					Dbg_Assert(p_font);
					pChar = p_font->pChars + p_font->SpecialMap[digit];
					got_char_tag = true;
					break;
				}
				default:
					Dbg_MsgAssert(0, ("unknown tag %c", *pLetter));
					break;
			}
		} // end while
		
		if (*pLetter == '\0') break;

		if (*pLetter != ' ' || pChar)
		{
			if (!pChar)
				pChar = p_font->pChars + p_font->Map[(uint8)*pLetter];
			u0 = pChar->u0;
			v0 = pChar->v0;
			u1 = pChar->u1;
			v1 = pChar->v1;
			x1 = x0+u1-u0;
		}
		else
		{
			x1 = x0 + space_spacing;
		}
		
		x0 = x1+char_spacing;
	}

	width  = (float)x0/16.0f;
	height = (float)DefaultHeight/16.0f;
}


/////////////////////////////////////////////////////////////////////////////
//

SText::SText(float pri) : SDraw2D(pri, true)
{
}

/////////////////////////////////////////////////////////////////////////////
//

SText::~SText()
{
}

/////////////////////////////////////////////////////////////////////////////
//

void SText::BeginDraw()
{
	//printf("Trying to draw: %s with %x at (%f, %f) of scale %f\n", mp_string, m_rgba, m_xpos, m_ypos, m_scale);

	// Subsequent processing within Draw() will use this font
	// Draw() may call this function to temporarily switch fonts
	SFont *p_font = (spOverrideFont) ? spOverrideFont : mp_font;
	
	// dma tag
	dma::BeginTag(dma::cnt, 0);

	vu1::Buffer = vu1::Loc;

	Dbg_Assert(GetScissorWindow());

	// GS context
	gs::BeginPrim(ABS,0,0);
	gs::Reg1(gs::SCISSOR_1, GetScissorWindow()->GetScissorReg());
	gs::Reg1(gs::TEX0_1,	 p_font->RegTEX0);
	gs::Reg1(gs::TEX1_1,	 p_font->RegTEX1);
	gs::Reg1(gs::ALPHA_1,	 PackALPHA(0,1,0,1,0));
	gs::Reg1(gs::COLCLAMP,	 PackCOLCLAMP(1));
	gs::Reg1(gs::RGBAQ,	 (uint64)m_rgba);
	gs::EndPrim(0);

	// unpack giftag
	vif::UNPACK(0, V4_32, 1, ABS, UNSIGNED, 0);
	gif::BeginTagImmediate(gs::UV|gs::XYZ2<<4, 2, PACKED, SPRITE|TME|FST|ABE, 1, VU1_ADDR(GSPrim));

	// begin unpack for uv/xyz's
	vif::BeginUNPACK(0, V4_32, ABS, SIGNED, 1);
}

/////////////////////////////////////////////////////////////////////////////
//

void SText::Draw()
{
	SChar *pChar = NULL;
	char *pLetter;
	uint16 u0,v0,u1,v1;
	uint32 x0,y0,x1,y1,yt,z;

	Dbg_Assert(GetScissorWindow());

	x0 = (int)(m_xpos * s_screen_scale_x * 16.0f) + XOFFSET + ((GetScissorWindow()->GetScissorReg() & 0x7FF) << 4);
	y0 = (int)(m_ypos * s_screen_scale_y * 16.0f) + YOFFSET + ((GetScissorWindow()->GetScissorReg() >> 32 & 0x7FF) << 4);
	z = s_constant_z_enabled ? s_constant_z : GetZValue();

	//sint16 char_spacing = 16 + (mp_font->mCharSpacing<<4);
	float char_spacing = (float) (mp_font->mCharSpacing<<4) * m_xscale * s_screen_scale_x;
	float space_spacing = (float) (mp_font->mSpaceSpacing<<4) * m_xscale * s_screen_scale_x;
	
	// XXX
	//printf("   spacing=%f, %d\n", char_spacing, mp_font->mCharSpacing);

	for (pLetter=mp_string;; pLetter++)
	{
		pChar = NULL;
		SFont *p_font = mp_font;
		
		// acount for tags (might be multiple ones in a row)
		bool got_char_tag = false; // tag resulting in output of character
		while (*pLetter == '\\' && !got_char_tag)
		{
			pLetter++;

			switch(*pLetter)
			{
				case '\\':
					got_char_tag = true;
					break;
				case 'c':
				case 'C':
				{
					pLetter++;	// skip "c"					
					uint digit = Str::DehexifyDigit(pLetter);
					pLetter++; // skip "#"

					// BIG MIKE	(remove #if below, replace active_color_gs_register_thingy with GS code)
					#if 1
					// set active color from font
					uint32	temp_rgba = m_rgba;
					if (digit != 0 && !m_color_override)
					{
						m_rgba = mp_font->mRGBATab[digit-1];
					}
					// This method is not optimal, but it's not done very often
					// we just turn off text, and turn it on with new colors
					EndDraw();
					BeginDraw();
					m_rgba = temp_rgba;
					#endif

					break;
				}
				case 's':
				case 'S':
				{
					pLetter++;	// skip "s"
					uint digit = Str::DehexifyDigit(pLetter);
					
					pChar = mp_font->pChars + mp_font->SpecialMap[digit];
					got_char_tag = true;
					
					break;
				}
				case 'b':
				case 'B':
				{
					// 'B' stands for button, accesses the button font

					pLetter++; // skip "b"
					uint digit = Str::DehexifyDigit(pLetter);
					
					// switch to the buttons font!
					
					p_font = pButtonsFont;
					Dbg_Assert(p_font);
					pChar = p_font->pChars + p_font->SpecialMap[digit];
					got_char_tag = true;
					
					spOverrideFont = p_font;
					EndDraw();
					BeginDraw();
					// we can now return to using the regular font (no override)
					spOverrideFont = NULL;
					
					break;
				}
				default:
					Dbg_MsgAssert(0, ("unknown tag %c", *pLetter));
					break;
			}
		} // end while

		if (*pLetter == '\0') break;
		
		if (*pLetter != ' ' || pChar)
		{
			if (!pChar)
				pChar = p_font->pChars + p_font->Map[(uint8) *pLetter];
			yt = y0 + (int)((float)(p_font->DefaultBase - pChar->Baseline) * m_yscale * s_screen_scale_y);
			u0 = pChar->u0;
			v0 = pChar->v0;
			u1 = pChar->u1;
			v1 = pChar->v1;
			x1 = x0+(int)((float)(u1-u0)*m_xscale * s_screen_scale_x);
			y1 = yt+(int)((float)(v1-v0)*m_yscale * s_screen_scale_y);
		}
		else
		{
			x0 += (int) (space_spacing + char_spacing);
			continue;
		}

		if (!NeedsCulling() || !((x0 | x1 | y0 | y1) & 0xFFFF0000))
		{
			vif::StoreV4_32(u0,v0,0,0);
			vif::StoreV4_32(x0,yt,z,0);
			vif::StoreV4_32(u1,v1,0,0);
			vif::StoreV4_32(x1,y1,z,0);
		}

		x0 = x1 + (int) char_spacing;

		// kick and start a new buffer if this one is full
		if (((dma::pLoc - gif::pTag)>>4) >= 250)
		{
			vif::EndUNPACK();
			gif::EndTagImmediate(1);
			vif::MSCAL(VU1_ADDR(Parser));
			vif::UNPACK(0, V4_32, 1, ABS, UNSIGNED, 0);
			gif::BeginTagImmediate(gs::UV|gs::XYZ2<<4, 2, PACKED, SPRITE|TME|FST|ABE, 1, VU1_ADDR(GSPrim));
			vif::BeginUNPACK(0, V4_32, ABS, SIGNED, 1);
		}
		
		if (p_font != mp_font)
		{
			// we just used the button font, so return to the regular one
			EndDraw();
			BeginDraw();
		}
	} // end for
}

/////////////////////////////////////////////////////////////////////////////
//

void SText::EndDraw(void)
{
	vif::EndUNPACK();
	gif::EndTagImmediate(1);
	vif::MSCAL(VU1_ADDR(Parser));
	dma::EndTag();
}



} // namespace NxPs2

