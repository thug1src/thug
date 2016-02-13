///////////////////////////////////////////////////////////////////////////////
// NxFont.h



#ifndef	__GFX_NXFONT_H__
#define	__GFX_NXFONT_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <gfx/NxViewport.h>
#include <gfx/image/imagebasic.h>

#define		__STATIC_FONT_STRINGS__ 0			// set to 1 to have the old statically allocated strings

namespace Nx
{

// Forward declarations
class CTextMan;
class CWindow2D;

//////////////////////////////////////////////////////////////////////////////
// The CFont class is the platform independent abstract base class
// of the platform specific CFont classes
class	CFont
{
public:
							CFont();
	virtual					~CFont();

	bool					Load(const char *filename);
	void					SetSpacings(int charSpacing, int spaceSpacing);
	void					SetRGBATable(Image::RGBA *pTab);
	void					MarkAsButtonFont(bool isButton);
	void					Unload();

	uint32					GetChecksum() const;
	uint32					GetDefaultHeight() const;
	uint32					GetDefaultBase() const;
	void					QueryString(char *String, float &width, float &height) const;

	// Member variables (protected so the p-classes can access them)
protected:
	uint32					m_checksum;

	// The virtual functions have a stub implementation.
private:
	virtual	bool			plat_load(const char *filename);
	virtual void			plat_set_spacings(int charSpacing, int spaceSpacing);
	virtual void			plat_set_rgba_table(Image::RGBA *pTab);
	virtual void 			plat_mark_as_button_font(bool isButton);
	virtual	void			plat_unload();

	virtual	uint32			plat_get_default_height() const;
	virtual	uint32			plat_get_default_base() const;
	virtual void			plat_query_string(char *String, float &width, float &height) const;
};

//////////////////////////////////////////////////////////////////////////////
// Holds a string of text that needs to be displayed this frame
class CText
{
public:
							CText(CWindow2D *p_window = NULL);
	virtual					~CText();

	// If hidden, it won't be drawn that frame
	void					SetHidden(bool hide);
	bool					IsHidden() const;

	// Font used for the text.
	CFont *					GetFont() const;
	void					SetFont(CFont *p_font);

	// Actual text
	const char *			GetString() const;
	void					SetString(const char *p_string);
	void					ClearString();

	// Position on screen.  Assumes screen size of 640x448.
	float					GetXPos() const;
	float					GetYPos() const;
	void					SetPos(float x, float y);

	// Scale of text.
	float					GetXScale() const;
	float					GetYScale() const;
	void					SetScale(float scale_x, float scale_y);

	// Priority of text.  Higher priority text are
	// drawn on top of lower priority text.
	float					GetPriority() const;
	void					SetPriority(float pri);
	Nx::ZBufferValue		GetZValue() const;
	void					SetZValue(Nx::ZBufferValue z);

	// Color of text.
	Image::RGBA				GetRGBA() const;
	void					SetRGBA(Image::RGBA rgba, bool colorOverride);

	// Clipping window
	CWindow2D *				GetWindow() const;
	void					SetWindow(CWindow2D *p_window);

protected:
	// Constants
	enum {
		vMAX_TEXT_CHARS = 96
	};

	CFont *					mp_font;
	#if						__STATIC_FONT_STRINGS__
	char					m_string[vMAX_TEXT_CHARS];
	#else
	char *					m_string;
	#endif
	float					m_xpos;
	float					m_ypos;
	float					m_xscale;
	float					m_yscale;
	float					m_priority;
	ZBufferValue			m_zvalue;		  // for zbuffer sort
	Image::RGBA				m_rgba;
	bool					m_color_override; // colors encoded in text will be overridden if true

	bool					m_hidden;
	bool					m_use_zbuffer;

	CWindow2D *				mp_window;

	// For free list in CTextMan
	CText *					mp_next;

private:
	//
	virtual void			plat_initialize();

	virtual void			plat_update_hidden();		// Tell engine of update
	virtual void			plat_update_engine();		// Update engine primitives
	virtual void			plat_update_priority();
	virtual void			plat_update_window();

	friend CTextMan;

};

//////////////////////////////////////////////////////////////////////////////
// Static class for memory management of CTexts
class CTextMan
{
public:
	//
	static void				sAllocTextPool();
	static CText *			sGetTextInstance();
	static void				sFreeTextInstance(CText *p_text);

private:
	// Constants
	enum {
		vMAX_TEXT_INSTANCES = 512
	};

	// Because it is static, it is declared here, but defined in p_NxFont.cpp
	static void				s_plat_alloc_text_pool();
	static CText *			s_plat_get_text_instance();
	static void				s_plat_free_text_instance(CText *p_text);

	// Array of Text requests
	static CText *			sp_dynamic_text_list;
};

/////////////////////////////////////////////////////////
// CText inline function
inline bool					CText::IsHidden() const
{
	return m_hidden;
}

inline CFont *				CText::GetFont() const
{
	return mp_font;
}

inline float				CText::GetXPos() const
{
	return m_xpos;
}

inline float				CText::GetYPos() const
{
	return m_ypos;
}

inline float				CText::GetXScale() const
{
	return m_xscale;
}

inline float				CText::GetYScale() const
{
	return m_yscale;
}

inline float				CText::GetPriority() const
{
	return m_priority;
}

inline ZBufferValue			CText::GetZValue() const
{
	return m_zvalue;
}

inline Image::RGBA			CText::GetRGBA() const
{
	return m_rgba;
}

inline CWindow2D *			CText::GetWindow() const
{
	return mp_window;
}

}

#endif // 

