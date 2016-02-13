///////////////////////////////////////////////////////////////////////////////////
// NxSprite.H - Neversoft Engine, Rendering portion, Platform independent interface

#ifndef	__GFX_NX_SPRITE_H__
#define	__GFX_NX_SPRITE_H__


#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/hashtable.h>

#include <gfx/nxtexture.h>
#include <gfx/nxviewport.h>
#include <gfx/image/imagebasic.h>


namespace Nx
{

// Forward declarations
class CWindow2D;

//////////////////////////////////////////////////////////////////////////////////
// Nx::CSprite: Class for 2D sprites
class	CSprite
{
public:
							CSprite(CWindow2D *p_window = NULL);
							CSprite(CTexture *p_tex, float xpos, float ypos,
									uint16 width, uint16 height, CWindow2D *p_window = NULL,
									Image::RGBA rgba = Image::RGBA(0x80, 0x80, 0x80, 0x80),
									float xscale = 1.0f, float yscale = 1.0f,
									float xanchor = -1.0f, float yanchor = -1.0f,
									float rot = 0.0f, float pri = 0.0f, bool hide = false);
	virtual					~CSprite();

	// Initialization
	void					Initialize(CTexture *p_tex, float xpos, float ypos,
									   uint16 width, uint16 height, CWindow2D *p_window = NULL,
									   Image::RGBA rgba = Image::RGBA(0x80, 0x80, 0x80, 0x80),
									   float xscale = 1.0f, float yscale = 1.0f,
									   float xanchor = -1.0f, float yanchor = -1.0f,
									   float rot = 0.0f, float pri = 0.0f, bool hide = false);

	// If hidden, it won't be drawn that frame
	void					SetHidden(bool hide);
	bool					IsHidden() const;

	// Texture used for the sprite.  But it is optional.
	CTexture *				GetTexture() const;
	void					SetTexture(CTexture *p_texture);

	// Position on screen.  Assumes screen size of 640x448.
	float					GetXPos() const;
	float					GetYPos() const;
	void					SetPos(float x, float y);

	// Actual width and height of sprite.  If textured, it will also
	// use this for the texture width and height.
	uint16					GetWidth() const;
	uint16					GetHeight() const;
	void					SetSize(uint16 w, uint16 h);

	// Scale of sprite.  A negative scale will flip the axis.
	float					GetXScale() const;
	float					GetYScale() const;
	void					SetScale(float x, float y);

	// The anchor position of the sprite.  Basically, the origin within
	// the sprite.  This point is going to be at the screen position.  It
	// is also the point of rotation.  The range is [-1.0, 1.0], where
	// (-1.0, -1.0) is the top left corner and (1.0, 1.0) is the bottom
	// right corner.
	float					GetXAnchor() const;
	float					GetYAnchor() const;
	void					SetAnchor(float x, float y);

	// Rotation of sprite in radians
	float					GetRotation() const;
	void					SetRotation(float rot);

	// Priority of sprite from 0.0 to 1.0.  Higher priority sprites are
	// drawn on top of lower priority sprites.
	float					GetPriority() const;
	void					SetPriority(float pri);
	Nx::ZBufferValue		GetZValue() const;
	void					SetZValue(Nx::ZBufferValue z);

	// Color of sprite.  All 4 corners are the same.
	Image::RGBA				GetRGBA() const;
	void					SetRGBA(Image::RGBA rgba);

	// Clipping window
	CWindow2D *				GetWindow() const;
	void					SetWindow(CWindow2D *p_window);

	// Constant z-buffer value (only affect sprites using a priority)
	static void				EnableConstantZValue(bool enable);
	static void				SetConstantZValue(Nx::ZBufferValue z);
	static Nx::ZBufferValue	GetConstantZValue();

protected:
	CTexture *				mp_texture;				// Texture

	float					m_pos_x;				// Position
	float					m_pos_y;
	float					m_scale_x;				// Scale
	float					m_scale_y;
	float					m_anchor_x;				// Anchor Position
	float					m_anchor_y;

	float					m_rotation;				// Rotation
	float					m_priority;				// Priority
	ZBufferValue			m_zvalue;				// for zbuffer sort

	uint16					m_width;				// Size
	uint16					m_height;

	bool					m_hidden;				// Hidden
	bool					m_use_zbuffer;
	Image::RGBA				m_rgba;					// Color

	CWindow2D *				mp_window;

private:
	virtual void			plat_initialize();

	virtual void			plat_update_hidden();		// Tell engine of update
	virtual void			plat_update_engine();		// Update engine primitives
	virtual void			plat_update_priority();
	virtual void			plat_update_window();

	static void				plat_enable_constant_z_value(bool enable);
	static void				plat_set_constant_z_value(Nx::ZBufferValue z);
	static Nx::ZBufferValue	plat_get_constant_z_value();
};

/////////////////////////////////////////////////////////
// CSprite inline function
inline bool					CSprite::IsHidden() const
{
	return m_hidden;
}

inline CTexture *			CSprite::GetTexture() const
{
	return mp_texture;
}

inline float				CSprite::GetXPos() const
{
	return m_pos_x;
}

inline float				CSprite::GetYPos() const
{
	return m_pos_y;
}

inline uint16				CSprite::GetWidth() const
{
	return m_width;
}

inline uint16				CSprite::GetHeight() const
{
	return m_height;
}

inline float				CSprite::GetXScale() const
{
	return m_scale_x;
}

inline float				CSprite::GetYScale() const
{
	return m_scale_y;
}

inline float				CSprite::GetXAnchor() const
{
	return m_anchor_x;
}

inline float				CSprite::GetYAnchor() const
{
	return m_anchor_y;
}

inline float				CSprite::GetRotation() const
{
	return m_rotation;
}

inline float				CSprite::GetPriority() const
{
	return m_priority;
}

inline ZBufferValue			CSprite::GetZValue() const
{
	return m_zvalue;
}

inline Image::RGBA			CSprite::GetRGBA() const
{
	return m_rgba;
}

inline CWindow2D *			CSprite::GetWindow() const
{
	return mp_window;
}

}


#endif

