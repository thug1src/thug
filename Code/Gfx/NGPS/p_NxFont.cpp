///////////////////////////////////////////////////////////////////////////////
// p_NxFont.cpp

#include 	"Gfx/NGPS/p_NxFont.h"
#include 	"Gfx/NGPS/p_NxWin2D.h"

namespace Nx
{

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Here's a machine specific implementation of the CFont

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Font::CPs2Font() : mp_plat_font(NULL)
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Font::~CPs2Font()
{
	if (mp_plat_font)
	{
		plat_unload();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CPs2Font::plat_load(const char *filename)
{
	mp_plat_font = NxPs2::LoadFont(filename);

	return (mp_plat_font != NULL);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Font::plat_set_spacings(int charSpacing, int spaceSpacing)
{
	mp_plat_font->mCharSpacing = charSpacing;
	if (spaceSpacing > 0)
		mp_plat_font->mSpaceSpacing = spaceSpacing;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Font::plat_set_rgba_table(Image::RGBA *pTab)
{
	for (int i = 0; i < 16; i++)
		mp_plat_font->mRGBATab[i] = *((uint32 *) &pTab[i]);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CPs2Font::plat_mark_as_button_font(bool isButton)
{
	NxPs2::pButtonsFont = (isButton) ? mp_plat_font : NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CPs2Font::plat_unload()
{
	NxPs2::UnloadFont(mp_plat_font);
	mp_plat_font = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32		CPs2Font::plat_get_default_height() const
{
	Dbg_Assert(mp_plat_font);

	return mp_plat_font->GetDefaultHeight();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32		CPs2Font::plat_get_default_base() const
{
	Dbg_Assert(mp_plat_font);

	return mp_plat_font->GetDefaultBase();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CPs2Font::plat_query_string(char *String, float &width, float &height) const
{
	Dbg_Assert(mp_plat_font);

	mp_plat_font->QueryString(String, width, height);
}

/////////////////////////////////////////////////////////////////////////////////////
// Here's a machine specific implementation of the CText

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Text::CPs2Text(CWindow2D *p_window) : CText(p_window)
{
	mp_plat_text = new NxPs2::SText();

	plat_initialize();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Text::~CPs2Text()
{
	if (mp_plat_text)
	{
		delete mp_plat_text;
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CPs2Text::plat_initialize()
{
	plat_update_engine();
	plat_update_priority();
	plat_update_hidden();
	plat_update_window();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CPs2Text::plat_update_hidden()
{
	mp_plat_text->SetHidden(m_hidden);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CPs2Text::plat_update_engine()
{
	CPs2Font *p_ps2_font = static_cast<CPs2Font *>(mp_font);

	mp_plat_text->mp_string	= m_string;
	if (p_ps2_font) {
		mp_plat_text->mp_font	= p_ps2_font->GetEngineFont();
	}

	mp_plat_text->m_xpos	= m_xpos;
	mp_plat_text->m_ypos	= m_ypos;
	mp_plat_text->m_xscale	= m_xscale;
	mp_plat_text->m_yscale	= m_yscale;
	mp_plat_text->m_rgba	= *((uint32 *) &m_rgba);
	mp_plat_text->m_color_override = m_color_override;
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CPs2Text::plat_update_priority()
{
	// Update draw list
	if (m_use_zbuffer)
	{
		mp_plat_text->SetZValue((uint32) m_zvalue);
	} else {
		mp_plat_text->SetPriority(m_priority);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CPs2Text::plat_update_window()
{
	CPs2Window2D *p_ps2_window = static_cast<CPs2Window2D *>(mp_window);

	if (p_ps2_window)
	{
		mp_plat_text->SetScissorWindow(p_ps2_window->GetEngineWindow());
	} else {
		mp_plat_text->SetScissorWindow(NULL);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CTextMan::s_plat_alloc_text_pool()
{
   	CPs2Text *p_text_array = new CPs2Text[vMAX_TEXT_INSTANCES];

	for (int i = 0; i < vMAX_TEXT_INSTANCES; i++)
	{
//	   	CPs2Text *p_text = new CPs2Text(NULL);
	   	CPs2Text *p_text = &p_text_array[i];
		p_text->mp_next = sp_dynamic_text_list;
		sp_dynamic_text_list = p_text;
	}
}

} // Namespace Nx  			
				
				
