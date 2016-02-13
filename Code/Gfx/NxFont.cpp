///////////////////////////////////////////////////////////////////////////////
// NxFont.cpp

#include "gfx/NxFont.h"

namespace	Nx
{
// These functions are the platform independent part of the interface to 
// the platform specific code
// parameter checking can go here....
// although we might just want to have these functions inline, or not have them at all?
					 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CFont::CFont()
{
	m_checksum = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CFont::~CFont()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CFont::Load(const char *filename)
{
	return plat_load(filename);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CFont::SetSpacings(int charSpacing, int spaceSpacing)
{
	plat_set_spacings(charSpacing, spaceSpacing);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CFont::SetRGBATable(Image::RGBA *pTab)
{
	Dbg_Assert(pTab);
	plat_set_rgba_table(pTab);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CFont::MarkAsButtonFont(bool isButton)
{
	plat_mark_as_button_font(isButton);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CFont::Unload()
{
	plat_unload();

	m_checksum = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32	CFont::GetChecksum() const
{
	return m_checksum;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32	CFont::GetDefaultHeight() const
{
	return plat_get_default_height();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32	CFont::GetDefaultBase() const
{
	return plat_get_default_base();
}

#if 0
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CFont::BeginText(uint32 rgba, float Scale)
{
	plat_begin_text(rgba, Scale);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CFont::DrawString(char *String, float x0, float y0)
{
	plat_draw_string(String, x0, y0);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CFont::EndText(void)
{
	plat_end_text();
}
#endif // 0

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CFont::QueryString(char *String, float &width, float &height) const
{
	plat_query_string(String, width, height);
}

///////////////////////////////////////////////////////////////////////////////
// Stub versions of all platform specific functions are provided here:
// so engine implementors can leave certain functionality until later
						
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CFont::plat_load(const char *filename)
{
	printf ("STUB: PlatLoad\n");
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CFont::plat_set_spacings(int charSpacing, int spaceSpacing)
{
	printf("STUB A DUB DUB");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CFont::plat_set_rgba_table(Image::RGBA *pTab)
{
	printf("STUB A DUB DUB");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CFont::plat_mark_as_button_font(bool isButton)
{
	printf("STUB A DUB DUB");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CFont::plat_unload()
{
	printf ("STUB: PlatUnload\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32	CFont::plat_get_default_height() const
{
	printf ("STUB: PlatGetDefaultHeight\n");
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32	CFont::plat_get_default_base() const
{
	printf ("STUB: PlatGetDefaultBase\n");
	return 0;
}

#if 0
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CFont::plat_begin_text(uint32 rgba, float Scale)
{
	Dbg_Assert(0);	// Don't call anymore
	printf ("STUB: PlatBeginText\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CFont::plat_draw_string(char *String, float x0, float y0)
{
	Dbg_Assert(0);	// Don't call anymore
	printf ("STUB: PlatDrawString\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CFont::plat_end_text(void)
{
	Dbg_Assert(0);	// Don't call anymore
	printf ("STUB: PlatEndText\n");
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CFont::plat_query_string(char *String, float &width, float &height) const
{
	printf ("STUB: PlatQueryString\n");
}

///////////////////////////////////////////////////////////////////////////////
// CText

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CText::CText(CWindow2D *p_window) : 
	mp_font(NULL),
	m_xpos(0.0f),
	m_ypos(0.0f),
	m_xscale(1.0f),
	m_yscale(1.0f),
	m_priority(0.0f),
	m_rgba(128, 128, 128, 128),
	m_color_override(false),
	m_hidden(true),
	m_use_zbuffer(false),
	mp_window(p_window),
	mp_next(NULL)
{
	#if						__STATIC_FONT_STRINGS__
	m_string[0] = '\0';
	#else
	m_string = NULL;
	#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CText::~CText() 
{
	#if						__STATIC_FONT_STRINGS__
	#else
	if (m_string)
	{
		delete [] m_string;
	}
	#endif	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CText::SetHidden(bool hide)
{
	m_hidden = hide;

	plat_update_hidden();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CText::SetFont(CFont *p_font)
{
	mp_font = p_font;

	plat_update_engine();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CText::SetString(const char *p_string)
{
	Dbg_MsgAssert(strlen(p_string) < vMAX_TEXT_CHARS, ("CText: string too long %d", strlen(p_string)));

	#if						__STATIC_FONT_STRINGS__
	strcpy(m_string, p_string);
	#else
	if (!m_string)
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
		m_string = new char[vMAX_TEXT_CHARS];
		Mem::Manager::sHandle().PopContext();
	}
	strcpy(m_string, p_string);
	#endif

	plat_update_engine();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CText::ClearString()
{
	#if						__STATIC_FONT_STRINGS__
	#else
	if (m_string)
	{
		delete m_string;
		m_string = NULL;
	}
	#endif
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CText::SetPos(float x, float y)
{
	m_xpos = x;
	m_ypos = y;

	plat_update_engine();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CText::SetScale(float scale_x, float scale_y)
{
	m_xscale = scale_x;
	m_yscale = scale_y;

	plat_update_engine();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CText::SetPriority(float pri)
{
	m_priority = pri;
	m_use_zbuffer = false;

	plat_update_priority();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CText::SetZValue(ZBufferValue z)
{
	m_zvalue = z;
	m_use_zbuffer = true;

	plat_update_priority();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CText::SetRGBA(Image::RGBA rgba, bool colorOverride)
{
	m_rgba = rgba;
	m_color_override = colorOverride;

	plat_update_engine();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CText::SetWindow(CWindow2D *p_window)
{
	mp_window = p_window;

	plat_update_window();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CText::plat_initialize()
{
	printf ("STUB: PlatInitialize\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CText::plat_update_hidden()
{
	printf ("STUB: PlatUpdateHidden\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CText::plat_update_engine()
{
	printf ("STUB: PlatUpdateEngine\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CText::plat_update_priority()
{
	printf ("STUB: PlatUpdatePriority\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CText::plat_update_window()
{
	printf ("STUB: PlatUpdateWindow\n");
}

///////////////////////////////////////////////////////////////////////////////
// CTextMan

CText *			CTextMan::sp_dynamic_text_list = NULL;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CTextMan::sAllocTextPool()
{
	s_plat_alloc_text_pool();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CText *			CTextMan::sGetTextInstance()
{
	CText *p_text = sp_dynamic_text_list;

	if (p_text)
	{
		sp_dynamic_text_list = p_text->mp_next;
		//p_text->AddToDrawList();
	} else {
		Dbg_MsgAssert(0, ("Out of CText Instances"));
	}

	return p_text;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CTextMan::sFreeTextInstance(CText *p_text)
{
	//p_text->RemoveFromDrawList();
	p_text->SetHidden(true);

	// Clear text, otherwise it hangs around
	// it would get re-used, but it's fragmenting memory	
	p_text->ClearString();

	p_text->mp_next = sp_dynamic_text_list;
	sp_dynamic_text_list = p_text;
}

}


