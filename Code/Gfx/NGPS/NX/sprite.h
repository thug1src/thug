#ifndef __SPRITE_H
#define __SPRITE_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
namespace NxPs2
{


// This is a temporary structure until we can merge the 2D textures
// with texture groups
struct SSingleTexture
{
public:
	// Flags
	enum
	{
		mLOAD_SCREEN		= 0x0001,
		mALLOC_VRAM			= 0x0002,
		mAUTO_ALLOC			= 0x0004,
		mSPRITE				= 0x0008,
		mINVERTED			= 0x0010,
	};
					SSingleTexture();
					SSingleTexture(uint8* p_buffer, int buffer_size, bool sprite, bool alloc_vram, bool load_screen = false);	// .img.ps2 buffer
					SSingleTexture(const char *Filename, bool sprite, bool alloc_vram, bool load_screen = false);				// .img.ps2 file
					SSingleTexture(uint8* p_buffer, int width, int height, int bitdepth, int clut_bitdepth, int mipmaps,
								   bool sprite, bool alloc_vram, bool load_screen = false);		// raw buffer
					SSingleTexture(const SSingleTexture & src_texture);		// Copy constructor
					~SSingleTexture();

	bool			InitTexture(bool copy);
	void			FlipTextureVertical();

	bool			IsInVRAM() const;

	// Manual allocate
	bool			AddToVRAM();
	bool			RemoveFromVRAM();

	// Auto allocate
	void			SetAutoAllocate();
	void			UseTexture(bool use);

	uint16			GetOrigWidth() const;
	uint16			GetOrigHeight() const;

	uint16			GetWidth() const;
	uint16			GetHeight() const;
	uint8			GetBitdepth() const;
	uint8			GetClutBitdepth() const;
	uint8			GetNumMipmaps() const;
	bool			IsTransparent() const;
	bool			IsInverted() const;

	uint8 *			GetTextureBuffer() const { return mp_PixelData; }
	uint32			GetTextureBufferSize() const { return m_PixelDataSize; }
	uint8 *			GetClutBuffer() const { return mp_ClutData; }
	uint32			GetClutBufferSize() const { return m_ClutDataSize; }

	void			ReallocateVRAM();
	void			UpdateDMA();

	uint32			m_checksum;
	uint32			m_flags;
	uint8 *			mp_VifData;
	uint8 *			mp_TexBuffer;
	uint8 *			mp_PixelData;
	uint8 *			mp_ClutData;
	uint32			m_PixelDataSize;
	uint32			m_ClutDataSize;
	uint32			m_VifSize;
	int				m_TexBufferSize;
	uint64			m_RegTEX0, m_RegTEX1;
	uint32 *		mp_TexBitBltBuf;		// Note these register pointers point to data in the DMA buffer,
	uint32 *		mp_ClutBitBltBuf;		// so they should only be used for writing, and only after drawing is done.

	uint16			m_orig_width, m_orig_height;
	uint16			m_NumTexVramBlocks;
	uint16			m_NumClutVramBlocks;
	uint16			m_TBP;
	uint16			m_CBP;

	SSingleTexture	*mp_next;

	#ifdef			__NOPT_ASSERT__
	char			m_name[128];
	#endif

	#ifdef			__NOPT_ASSERT__
	static void		sPrintAllTextureInfo();
	static void		sPrintAllocatedTextureInfo();
	#endif

	static SSingleTexture	*sp_stexture_list;

protected:

	bool			setup_reg_and_dma(uint8 *pData, uint32 TW, uint32 TH, uint32 PSM, uint32 CPSM, uint32 MXL, bool copy);

	// Update usage for auto VRAM allocate
	void			inc_usage_count();
	void			dec_usage_count();

	static uint32	bitdepth_to_psm(int bitdepth);

	int				m_usage_count;
};

struct SScissorWindow
{
	void			SetScissorReg(uint64 reg);
	void			SetScissor(uint16 x0, uint16 y0, uint16 x1, uint16 y1);
	uint64			GetScissorReg() const { return m_RegSCISSOR; }

private:
	uint64			m_RegSCISSOR;
};

struct SDraw2D
{
					SDraw2D(float pri = 0.0f, bool hide = true);
	virtual			~SDraw2D();

	void			SetPriority(float pri);
	float			GetPriority(void) const;

	void			SetZValue(uint32 z);
	uint32			GetZValue(void) const;

	virtual void	SetHidden(bool hide);		// Virtual because we need to check any vram issues
	bool			IsHidden(void) const;

	bool			NeedsCulling() const;

	void			SetScissorWindow(SScissorWindow *p_scissor);
	SScissorWindow *GetScissorWindow() const { return mp_scissor_window; }

	// Statics
	static void		DrawAll(void);

	static void		EnableConstantZValue(bool enable);
	static void		SetConstantZValue(uint32 z);
	static uint32	GetConstantZValue();

	static void		CalcScreenScale();
	static float	GetScreenScaleX();
	static float	GetScreenScaleY();

protected:
	// Scale for PAL conversion
	static float	s_screen_scale_x;
	static float	s_screen_scale_y;

	// Constant Z
	static bool		s_constant_z_enabled;
	static uint32	s_constant_z;

private:
	void			InsertSortedDrawList(SDraw2D **p_list);
	void			InsertDrawList(SDraw2D **p_list);
	void			RemoveDrawList(SDraw2D **p_list);

	static void		DrawList(SDraw2D *p_list);

	virtual void	BeginDraw(void) = 0;
	virtual void	Draw(void) = 0;
	virtual void	EndDraw(void) = 0;

	// Not even the derived classes should have direct access
	bool			m_hidden;
	bool			m_use_zbuffer;
	float			m_pri;
	uint32			m_z;
	SScissorWindow *mp_scissor_window;

	// members
	SDraw2D			*mp_next;

	// 2D draw list (sorted by priority)
	static SDraw2D	*sp_2D_sorted_draw_list;
	// 2D zbuffer draw list
	static SDraw2D	*sp_2D_zbuffer_draw_list;
};


struct SSprite : public SDraw2D
{
public:
					SSprite(float pri = 0.0f);
	virtual			~SSprite();

	virtual void	SetHidden(bool hide);		// Virtual because we need to check any vram issues

	void			SetTexture(SSingleTexture *p_texture);

	float			m_xpos;
	float			m_ypos;
	uint16			m_width;
	uint16			m_height;
	float			m_scale_x;
	float			m_scale_y;
	float			m_xhot;
	float			m_yhot;
	float			m_rot;
	uint32			m_rgba;

protected:

	SSingleTexture	*mp_texture;

private:
	void			BeginDraw(void);
	void			Draw(void);
	void			EndDraw(void);
};


//
// Inlines
//

inline bool		SSingleTexture::IsInVRAM() const
{
	return m_flags & mALLOC_VRAM;
}

inline bool		SSingleTexture::IsInverted() const
{
	return m_flags & mINVERTED;
}

inline float	SDraw2D::GetScreenScaleX()
{
	return s_screen_scale_x;
}

inline float	SDraw2D::GetScreenScaleY()
{
	return s_screen_scale_y;
}

inline uint32	SDraw2D::GetZValue() const
{
	return m_z;
}

inline bool		SDraw2D::IsHidden(void) const
{
	return m_hidden;
}

inline bool		SDraw2D::NeedsCulling() const
{
	return m_use_zbuffer;		// Assuming that only items using the ZBuffer need it
}

extern uint32		FontVramStart;
extern uint32		FontVramBase;
extern const uint32	FontVramSize;


} // namespace NxPs2


#endif // __CHARS_H
