///////////////////////////////////////////////////////////////////////////////
// p_NxSprite.cpp

#include 	"Gfx/xbox/p_NxSprite.h"
#include 	"Gfx/xbox/p_NxTexture.h"

namespace Nx
{

////////////////////////////////////////////////////////////////////////////////////
// Here's a machine specific implementation of CSprite

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CXboxSprite::CXboxSprite()
{
	mp_plat_sprite = new NxXbox::sSprite();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CXboxSprite::~CXboxSprite()
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

void CXboxSprite::plat_initialize()
{
	plat_update_engine();
	plat_update_priority();
	plat_update_hidden();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxSprite::plat_update_hidden()
{
	// Take sprite on or off draw list
	mp_plat_sprite->SetHidden( m_hidden );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxSprite::plat_update_engine()
{
	CXboxTexture *p_xbox_texture = static_cast<CXboxTexture *>( mp_texture );

	if( p_xbox_texture )
	{
		// Rebuild sprite primitives
		mp_plat_sprite->mp_texture	= p_xbox_texture->GetEngineTexture();
	}
	
	mp_plat_sprite->m_xpos		= m_pos_x;
	mp_plat_sprite->m_ypos		= m_pos_y;
	mp_plat_sprite->m_width		= m_width;
	mp_plat_sprite->m_height	= m_height;
	mp_plat_sprite->m_scale_x	= m_scale_x;
	mp_plat_sprite->m_scale_y	= m_scale_y;

	mp_plat_sprite->m_xhot		= (( m_anchor_x + 1.0f ) * 0.5f) * ( m_width );
	mp_plat_sprite->m_yhot		= (( m_anchor_y + 1.0f ) * 0.5f) * ( m_height );

	mp_plat_sprite->m_rot		= m_rotation;
	mp_plat_sprite->m_rgba		= *((uint32 *) &m_rgba);
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxSprite::plat_update_priority()
{
	// Update draw list.
	if( mp_plat_sprite )
	{
		mp_plat_sprite->SetPriority( m_priority );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CSprite::plat_enable_constant_z_value(bool enable)
{
	//NxPs2::SSprite::EnableConstantZValue(enable);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CSprite::plat_set_constant_z_value(Nx::ZBufferValue z)
{
	//NxPs2::SSprite::SetConstantZValue(z);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Nx::ZBufferValue	CSprite::plat_get_constant_z_value()
{
	//return NxPs2::SSprite::GetConstantZValue();
	return 0;
}

} // Namespace Nx  			
				
				
