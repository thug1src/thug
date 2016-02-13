///////////////////////////////////////////////////////////////////////////////
// p_NxFont.cpp

#include 	"gfx/Ngc/p_nxfont.h"

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

CNgcFont::CNgcFont() : mp_plat_font(NULL)
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CNgcFont::~CNgcFont()
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
bool CNgcFont::plat_load(const char *filename)
{
	mp_plat_font = NxNgc::LoadFont(filename);

	return (mp_plat_font != NULL);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CNgcFont::plat_set_spacings(int charSpacing, int spaceSpacing)
{
	mp_plat_font->mCharSpacing = charSpacing;
	if (spaceSpacing > 0)
		mp_plat_font->mSpaceSpacing = spaceSpacing;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CNgcFont::plat_set_rgba_table(Image::RGBA *pTab)
{
	for (int i = 0; i < 16; i++)
		mp_plat_font->mRGBATab[i] = *((uint32 *) &pTab[i]);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CNgcFont::plat_mark_as_button_font(bool isButton)
{
	NxNgc::pButtonsFont = (isButton) ? mp_plat_font : NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcFont::plat_unload()
{
	NxNgc::UnloadFont(mp_plat_font);
	mp_plat_font = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint32 CNgcFont::plat_get_default_height() const
{
	Dbg_Assert(mp_plat_font);

	return mp_plat_font->GetDefaultHeight();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint32 CNgcFont::plat_get_default_base() const
{
	Dbg_Assert(mp_plat_font);

	return mp_plat_font->GetDefaultBase();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
//void CNgcFont::plat_begin_text(uint32 rgba, float Scale)
//{
//	Dbg_Assert(mp_plat_font);
//
//	mp_plat_font->BeginText(rgba, Scale);
//}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
//void CNgcFont::plat_draw_string(char *String, float x0, float y0)
//{
//	Dbg_Assert(mp_plat_font);
//
//	mp_plat_font->DrawString(String, x0, y0);
//}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
//void CNgcFont::plat_end_text(void)
//{
//	Dbg_Assert(mp_plat_font);
//
//	mp_plat_font->EndText();
//}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcFont::plat_query_string(char *String, float &width, float &height) const
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
CNgcText::CNgcText()
{
	mp_plat_text = new NxNgc::SText();

	plat_initialize();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CNgcText::~CNgcText()
{
	if( mp_plat_text )
	{
		delete mp_plat_text;
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CNgcText::plat_initialize()
{
	plat_update_engine();
	plat_update_priority();
	plat_update_hidden();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CNgcText::plat_update_hidden()
{
	mp_plat_text->SetHidden(m_hidden);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CNgcText::plat_update_engine()
{
	CNgcFont *p_Ngc_font = static_cast<CNgcFont *>( mp_font );

	mp_plat_text->mp_string	= m_string;
	if( p_Ngc_font)
	{
		mp_plat_text->mp_font = p_Ngc_font->GetEngineFont();
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
void CNgcText::plat_update_priority( void )
{
	// Update draw list
	mp_plat_text->SetPriority( m_priority );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CTextMan::s_plat_alloc_text_pool( void )
{
	for( int i = 0; i < vMAX_TEXT_INSTANCES; ++i )
	{
	   	CNgcText *p_text		= new CNgcText;
		p_text->mp_next			= sp_dynamic_text_list;
		sp_dynamic_text_list	= p_text;
	}
}






} // Namespace Nx  			
				
				

