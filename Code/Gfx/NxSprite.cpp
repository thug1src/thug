///////////////////////////////////////////////////////////////////////////////
// NxSprite.cpp

#include <core/defines.h>

#include "gfx/NxSprite.h"

namespace	Nx
{

/////////////////////////////////////////////////////////////////////////////
// CSprite

/////////////////////////////////////////////////////////////////////////////
// These functions are the platform independent part of the interface.
					 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSprite::CSprite(CWindow2D *p_window) :
	mp_texture(NULL),
	m_pos_x(-1.0f),
	m_pos_y(-1.0f),
	m_scale_x(1.0f),
	m_scale_y(1.0f),
	m_anchor_x(-1.0f),
	m_anchor_y(-1.0f),
	m_rotation(0.0f),
	m_priority(0.0f),
	m_width(0),
	m_height(0),
	m_rgba(0x80, 0x80, 0x80, 0x80),
	mp_window(p_window)
{
	m_hidden = true;	// until we get more information
	m_use_zbuffer = false;
}

CSprite::CSprite(CTexture *p_tex, float xpos, float ypos,
				uint16 width, uint16 height, CWindow2D *p_window, Image::RGBA rgba,
				float xscale, float yscale, float xanchor,
				float yanchor, float rot, float pri, bool hide)
{
	Initialize(p_tex, xpos, ypos, width, height, p_window, rgba, xscale, yscale,
			   xanchor, yanchor, rot, pri, hide);

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSprite::~CSprite()
{
	// TODO: Possibly free texture
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSprite::Initialize(CTexture *p_tex, float xpos, float ypos,
								uint16 width, uint16 height, CWindow2D *p_window,
								Image::RGBA rgba, float xscale, float yscale, float xanchor,
								float yanchor, float rot, float pri, bool hide)
{
	mp_texture = p_tex;
	m_pos_x = xpos;
	m_pos_y = ypos;
	m_width = width;
	m_height = height;
	m_rgba = rgba;
	m_scale_x = xscale;
	m_scale_y = yscale;
	m_anchor_x = xanchor;
	m_anchor_y = yanchor;
	m_rotation = rot;
	m_priority = pri;

	m_hidden = hide;
	m_use_zbuffer = false;
	mp_window = p_window;

	plat_initialize();			// This calls nothing when called from the constructor
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSprite::SetHidden(bool hide)
{
	m_hidden = hide;

	plat_update_hidden();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSprite::SetTexture(CTexture *p_texture)
{
	mp_texture = p_texture;
	if (p_texture)
	{
		m_width = p_texture->GetWidth();
		m_height = p_texture->GetHeight();
	}

	plat_update_engine();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSprite::SetPos(float x, float y)
{
	m_pos_x = x;
	m_pos_y = y;

	plat_update_engine();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSprite::SetSize(uint16 width, uint16 height)
{
	m_width = width;
	m_height = height;

	plat_update_engine();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSprite::SetScale(float x, float y)
{
	m_scale_x = x;
	m_scale_y = y;

	plat_update_engine();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSprite::SetAnchor(float x, float y)
{
	m_anchor_x = x;
	m_anchor_y = y;

	plat_update_engine();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSprite::SetRotation(float rot)
{
	m_rotation = rot;

	plat_update_engine();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSprite::SetPriority(float pri)
{
	m_priority = pri;
	m_use_zbuffer = false;

	plat_update_priority();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		 CSprite::SetZValue(ZBufferValue z)
{
	m_zvalue = z;
	m_use_zbuffer = true;

	plat_update_priority();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSprite::SetRGBA(Image::RGBA rgba)
{
	m_rgba = rgba;

	plat_update_engine();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSprite::SetWindow(CWindow2D *p_window)
{
	mp_window = p_window;

	plat_update_window();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CSprite::EnableConstantZValue(bool enable)
{
	plat_enable_constant_z_value(enable);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CSprite::SetConstantZValue(Nx::ZBufferValue z)
{
	plat_set_constant_z_value(z);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Nx::ZBufferValue	CSprite::GetConstantZValue()
{
	return plat_get_constant_z_value();
}

///////////////////////////////////////////////////////////////////////////////
// Stub versions of all platform specific functions are provided here:
// so engine implementors can leave certain functionality until later
						
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSprite::plat_initialize()
{
	printf ("STUB: PlatInitialize\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSprite::plat_update_hidden()
{
	printf ("STUB: PlatUpdateHidden\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSprite::plat_update_engine()
{
	printf ("STUB: PlatUpdateEngine\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSprite::plat_update_priority()
{
	printf ("STUB: PlatUpdatePriority\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CSprite::plat_update_window()
{
	printf ("STUB: PlatUpdateWindow\n");
}

}


