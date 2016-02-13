///////////////////////////////////////////////////////////////////////////////
// p_NxSprite.cpp

#include 	"Gfx/NGPS/p_NxSprite.h"
#include 	"Gfx/NGPS/p_NxTexture.h"
#include 	"Gfx/NGPS/p_NxWin2D.h"

namespace Nx
{

////////////////////////////////////////////////////////////////////////////////////
// Here's a machine specific implementation of CSprite

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Sprite::CPs2Sprite(CWindow2D *p_window) : CSprite(p_window)
{
	mp_plat_sprite = new NxPs2::SSprite();

	plat_initialize();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPs2Sprite::~CPs2Sprite()
{
	delete mp_plat_sprite;
}

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CPs2Sprite::plat_initialize()
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

void		CPs2Sprite::plat_update_hidden()
{
	// Take sprite on or off draw list
	mp_plat_sprite->SetHidden(m_hidden);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CPs2Sprite::plat_update_engine()
{
	CPs2Texture *p_ps2_texture = static_cast<CPs2Texture *>(mp_texture);

	// Rebuild sprite primitives
	if (p_ps2_texture)
	{
		mp_plat_sprite->SetTexture(p_ps2_texture->GetSingleTexture());
	} else {
		mp_plat_sprite->SetTexture(NULL);
	}

	mp_plat_sprite->m_xpos		= m_pos_x;
	mp_plat_sprite->m_ypos		= m_pos_y;
	mp_plat_sprite->m_width		= m_width;
	mp_plat_sprite->m_height	= m_height;
	mp_plat_sprite->m_scale_x	= m_scale_x;
	mp_plat_sprite->m_scale_y	= m_scale_y;

	mp_plat_sprite->m_xhot		= ((m_anchor_x + 1.0f) * 0.5f) * (m_width);
	mp_plat_sprite->m_yhot		= ((m_anchor_y + 1.0f) * 0.5f) * (m_height);

	mp_plat_sprite->m_rot		= m_rotation;
	mp_plat_sprite->m_rgba		= *((uint32 *) &m_rgba);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CPs2Sprite::plat_update_priority()
{
	// Update draw list
	if (m_use_zbuffer)
	{
		mp_plat_sprite->SetZValue((uint32) m_zvalue);
	} else {
		mp_plat_sprite->SetPriority(m_priority);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CPs2Sprite::plat_update_window()
{
	CPs2Window2D *p_ps2_window = static_cast<CPs2Window2D *>(mp_window);

	if (p_ps2_window)
	{
		mp_plat_sprite->SetScissorWindow(p_ps2_window->GetEngineWindow());
	} else {
		mp_plat_sprite->SetScissorWindow(NULL);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CSprite::plat_enable_constant_z_value(bool enable)
{
	NxPs2::SSprite::EnableConstantZValue(enable);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CSprite::plat_set_constant_z_value(Nx::ZBufferValue z)
{
	NxPs2::SSprite::SetConstantZValue(z);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Nx::ZBufferValue	CSprite::plat_get_constant_z_value()
{
	return NxPs2::SSprite::GetConstantZValue();
}

} // Namespace Nx  			
				
				
