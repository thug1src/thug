#ifndef __CHARS_H
#define __CHARS_H

#include <Gfx/NGPS/NX/sprite.h>

namespace NxPs2
{


typedef struct
{
	uint16 u0,v0,u1,v1;
	uint16 Baseline;
}
SChar;


struct SFont
{
public:
	uint32		GetDefaultHeight() const;
	uint32		GetDefaultBase() const;

	void		QueryString(char *String, float &width, float &height);

	void		ReallocateVRAM();
	void		UpdateDMA();

	//char Name[16];
	uint32		DefaultHeight, DefaultBase;
	SChar		*pChars;
	uint8		Map[256];
	uint8		SpecialMap[32];
	uint8		*pVifData;
	uint32		VifSize;
	uint64		RegTEX0, RegTEX1;
	uint32 		*mp_TexBitBltBuf;		// Note these register pointers point to data in the DMA buffer,
	uint32 		*mp_ClutBitBltBuf;		// so they should only be used for writing, and only after drawing is done.
	uint16		m_NumTexVramBlocks;
	uint16		m_NumClutVramBlocks;
	uint16		m_TBP;
	uint16		m_CBP;
	sint16		mCharSpacing;
	sint16		mSpaceSpacing;
	uint32		mRGBATab[16];
	SFont		*pNext;
};



SFont *		LoadFont(const char *Filename);
void		UnloadFont(SFont *);
void		SetTextWindow(uint16 x0, uint16 x1, uint16 y0, uint16 y1);


struct SText : public SDraw2D
{
public:
					SText(float pri = 0.0f);
	virtual			~SText();

	SFont			*mp_font;

	char			*mp_string;
	float			m_xpos;
	float			m_ypos;
	float			m_xscale;
	float			m_yscale;
	uint32			m_rgba;
	bool			m_color_override;
	
	// used in conjunction with BeginDraw()
	// if set, use specified font instead of mp_font
	// if not, use mp_font
	static SFont *	spOverrideFont;

private:

	void			BeginDraw();
	void			Draw(void);
	void			EndDraw(void);
};

extern uint32		FontVramStart;
extern uint32		FontVramBase;
extern const uint32	FontVramSize;
extern SFont		*pFontList;
extern SFont *		pButtonsFont;


} // namespace NxPs2


#endif // __CHARS_H
