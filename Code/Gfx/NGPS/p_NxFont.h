///////////////////////////////////////////////////////////////////////////////
// p_NxFont.h

#ifndef	__GFX_P_NX_FONT_H__
#define	__GFX_P_NX_FONT_H__

#include 	"Gfx/NxFont.h"
#include 	"Gfx/NGPS/NX/chars.h"

namespace Nx
{

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Here's a machine specific implementation of the CFont
class	CPs2Font : public CFont
{
public:
								CPs2Font();
	virtual						~CPs2Font();

	NxPs2::SFont *				GetEngineFont() const;

private:		// It's all private, as it is machine specific
	virtual	bool				plat_load(const char *filename);
	virtual void				plat_set_spacings(int charSpacing, int spaceSpacing);
	virtual void				plat_set_rgba_table(Image::RGBA *pTab);
	virtual void 				plat_mark_as_button_font(bool isButton);
	virtual void				plat_unload();

	virtual	uint32				plat_get_default_height() const;
	virtual	uint32				plat_get_default_base() const;
	virtual void				plat_query_string(char *String, float &width, float &height) const;

	// Machine specific members
	NxPs2::SFont *				mp_plat_font;		// Pointer to engine font
};

/////////////////////////////////////////////////////////////////////////////////////
// Here's a machine specific implementation of the CText
class	CPs2Text : public CText
{
public:
								CPs2Text(CWindow2D *p_window = NULL);
	virtual						~CPs2Text();

private:
	//
	virtual void				plat_initialize();

	virtual void				plat_update_hidden();		// Tell engine of update
	virtual void				plat_update_engine();		// Update engine primitives
	virtual void				plat_update_priority();
	virtual void				plat_update_window();

	// Machine specific members
	NxPs2::SText *				mp_plat_text;		// Pointer to engine text
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline NxPs2::SFont *		CPs2Font::GetEngineFont() const
{
	return mp_plat_font;
}

} // Namespace Nx  			

#endif
