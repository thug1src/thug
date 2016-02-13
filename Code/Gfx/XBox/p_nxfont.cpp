///////////////////////////////////////////////////////////////////////////////
// p_NxFont.cpp

#include 	"gfx/xbox/p_nxfont.h"
#include 	"sys/config/config.h"

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

CXboxFont::CXboxFont() : mp_plat_font(NULL)
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CXboxFont::~CXboxFont()
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
bool CXboxFont::plat_load(const char *filename)
{
#	ifdef __PAL_BUILD__
	// <Sigh>. Have to trap the small font, which needs to be loaded from a different
	// location for the French build.
	if( Config::GetLanguage() == Config::LANGUAGE_FRENCH )
	{
		if( strstr( filename, "small" ))
		{
			mp_plat_font = NxXbox::LoadFont( "small_fr" );
		}
		else
		{
			mp_plat_font = NxXbox::LoadFont( filename );
		}
	}
	else
	{
		mp_plat_font = NxXbox::LoadFont(filename);
	}
#	else
	mp_plat_font = NxXbox::LoadFont(filename);
#	endif

	return (mp_plat_font != NULL);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CXboxFont::plat_set_spacings(int charSpacing, int spaceSpacing)
{
	mp_plat_font->mCharSpacing = charSpacing;
	if (spaceSpacing > 0)
		mp_plat_font->mSpaceSpacing = spaceSpacing;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CXboxFont::plat_set_rgba_table(Image::RGBA *pTab)
{
	for (int i = 0; i < 16; i++)
		mp_plat_font->mRGBATab[i] = *((uint32 *) &pTab[i]);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CXboxFont::plat_mark_as_button_font(bool isButton)
{
	NxXbox::pButtonsFont = (isButton) ? mp_plat_font : NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CXboxFont::plat_unload()
{
	NxXbox::UnloadFont(mp_plat_font);
	mp_plat_font = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint32 CXboxFont::plat_get_default_height() const
{
	Dbg_Assert(mp_plat_font);

	return mp_plat_font->GetDefaultHeight();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint32 CXboxFont::plat_get_default_base() const
{
	Dbg_Assert(mp_plat_font);

	return mp_plat_font->GetDefaultBase();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
//void CXboxFont::plat_begin_text(uint32 rgba, float Scale)
//{
//	Dbg_Assert(mp_plat_font);
//
//	mp_plat_font->BeginText(rgba, Scale);
//}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
//void CXboxFont::plat_draw_string(char *String, float x0, float y0)
//{
//	Dbg_Assert(mp_plat_font);
//
//	mp_plat_font->DrawString(String, x0, y0);
//}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
//void CXboxFont::plat_end_text(void)
//{
//	Dbg_Assert(mp_plat_font);
//
//	mp_plat_font->EndText();
//}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxFont::plat_query_string(char *String, float &width, float &height) const
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
CXboxText::CXboxText()
{
	mp_plat_text	= new NxXbox::SText();
	m_zvalue		= 0;

	plat_initialize();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxText::~CXboxText()
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

void CXboxText::plat_initialize()
{
	plat_update_engine();
	plat_update_priority();
	plat_update_hidden();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CXboxText::plat_update_hidden()
{
	mp_plat_text->SetHidden(m_hidden);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxText::plat_update_engine()
{
	CXboxFont *p_xbox_font = static_cast<CXboxFont *>( mp_font );

	mp_plat_text->mp_string	= m_string;
	if( p_xbox_font)
	{
		mp_plat_text->mp_font = p_xbox_font->GetEngineFont();
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
void CXboxText::plat_update_priority( void )
{
	// Update draw list
	mp_plat_text->SetPriority( m_priority );

	if( m_use_zbuffer )
	{
		// Convert the 16:16 fixed z value to a float here.
		float z = (float)m_zvalue / 65536.0f;
		mp_plat_text->SetZValue( z );
	}
	else
	{
		mp_plat_text->SetZValue( 0.0f );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CTextMan::s_plat_alloc_text_pool( void )
{
	for( int i = 0; i < vMAX_TEXT_INSTANCES; ++i )
	{
	   	CXboxText *p_text		= new CXboxText;
		p_text->mp_next			= sp_dynamic_text_list;
		sp_dynamic_text_list	= p_text;
	}
}






} // Namespace Nx  			
				
				
