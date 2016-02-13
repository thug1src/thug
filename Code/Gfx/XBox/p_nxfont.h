///////////////////////////////////////////////////////////////////////////////
// p_NxFont.h

#ifndef	__GFX_P_NX_FONT_H__
#define	__GFX_P_NX_FONT_H__

#include 	"gfx/nxfont.h"
#include 	"gfx/xbox/nx/chars.h"

namespace Nx
{

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Here's a machine specific implementation of the CFont
class CXboxFont : public CFont
{
public:
								CXboxFont();
	virtual						~CXboxFont();
	NxXbox::SFont				*GetEngineFont() const;

private:		// It's all private, as it is machine specific
	virtual	bool				plat_load(const char *filename);
	virtual void				plat_set_spacings(int charSpacing, int spaceSpacing);
	virtual void				plat_set_rgba_table(Image::RGBA *pTab);
	virtual void 				plat_mark_as_button_font(bool isButton);
	virtual void				plat_unload();

	virtual	uint32				plat_get_default_height() const;
	virtual	uint32				plat_get_default_base() const;
//	virtual void				plat_begin_text(uint32 rgba, float Scale);
//	virtual void				plat_draw_string(char *String, float x0, float y0);
//	virtual void				plat_end_text(void);
	virtual void				plat_query_string(char *String, float &width, float &height) const;

	// Machine specific members
	NxXbox::SFont *				mp_plat_font;		// Pointer to engine font
};


/////////////////////////////////////////////////////////////////////////////////////
// Here's a machine specific implementation of the CText
class	CXboxText : public CText
{
public:
								CXboxText();
	virtual						~CXboxText();

private:
	//
	virtual void				plat_initialize();

	virtual void				plat_update_hidden();		// Tell engine of update
	virtual void				plat_update_engine();		// Update engine primitives
	virtual void				plat_update_priority();

	// Machine specific members
	NxXbox::SText				*mp_plat_text;		// Pointer to engine text
};


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
inline NxXbox::SFont *CXboxFont::GetEngineFont() const
{
	return mp_plat_font;
}


} // Namespace Nx  			

#endif
