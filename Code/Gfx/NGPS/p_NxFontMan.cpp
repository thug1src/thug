/////////////////////////////////////////////////////////////////////////////
// p_NxFontMan.cpp - PS2 platform specific interface to the Font Manager
//
// This is PS2 SPECIFIC!!!!!!  So might get a bit messy
//

#include	"gfx\nx.h"
#include	"gfx\NxFontMan.h"
#include	"gfx\NGPS\p_NxFont.h"

#include 	"gfx\ngps\nx\chars.h"

namespace	Nx
{


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// Functions


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CFont	*	CFontManager::s_plat_load_font(const char *pName)
{
	CPs2Font *p_new_font;

	p_new_font = new CPs2Font;
	p_new_font->Load(pName);

	return p_new_font;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CFontManager::s_plat_unload_font(CFont *pFont)
{
	pFont->Unload();
}

} 
 
