#include <core/defines.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gfx/2D/ScreenElemMan.h>
#include <gfx/2D/SpriteElement.h>
#include <gfx/2D/Window.h>
#include <gfx/Nx.h>
#include <gfx/nxtexture.h>
#include <gfx/NxTexMan.h>
#include <gfx/NxSprite.h>

namespace Front
{




CSpriteElement::CSpriteElement()
{
	//m_rgba = 0x40909090;
	m_texture = 0;
	mp_sprite = Nx::CEngine::sCreateSprite(NULL);
	
	SetType(CScreenElement::TYPE_SPRITE_ELEMENT);
}




CSpriteElement::~CSpriteElement()
{
	if (mp_sprite)
	{
		Nx::CEngine::sDestroySprite(mp_sprite);
	}
	//Ryan("Destroying CSpriteElement\n");
}




void CSpriteElement::SetProperties(Script::CStruct *pProps)
{
	CScreenElement::SetProperties(pProps);

	uint32 texture_crc;
	if (pProps->GetChecksum("texture", &texture_crc))
		SetTexture(texture_crc);
	float rot_angle;
	if ( pProps->GetFloat( "rot_angle", &rot_angle ) )
		SetRotate( rot_angle );
	// Dbg_MsgAssert(m_texture, ("no texture loaded"));
}

void CSpriteElement::SetMorph( Script::CStruct *pProps )
{
	CScreenElement::SetMorph( pProps );

	float rot_angle;
	if ( pProps->GetFloat( "rot_angle", &rot_angle ) )
		SetRotate( rot_angle, DONT_FORCE_INSTANT );
}


void CSpriteElement::SetTexture(uint32 texture_checksum)
{
	Nx::CTexture *p_texture;

	m_texture = texture_checksum;

	p_texture = Nx::CTexDictManager::sp_sprite_tex_dict->GetTexture(texture_checksum);
	Dbg_MsgAssert(p_texture, ("no texture found for sprite 0x%x %s on element 0x%x %s",
					texture_checksum,Script::FindChecksumName(texture_checksum),
					m_id,Script::FindChecksumName(m_id)));
	mp_sprite->SetTexture(p_texture);
	
	Dbg_MsgAssert(!(m_object_flags & vFORCED_DIMS), ("Trying to override sprite texture size"));
	SetDims(p_texture->GetWidth(), p_texture->GetHeight());

	m_object_flags |= CScreenElement::vCHANGED_STATIC_PROPS;	
}



void CSpriteElement::SetRotate( float angle, EForceInstant forceInstant )
{
	float rot = angle * Mth::PI / 180.0f;
	rot += 0.001f;
	Dbg_Assert( mp_sprite );

	if ( m_target_local_props.GetRotate() != rot )
	{
		m_target_local_props.SetRotate( rot );
		m_object_flags |= vNEEDS_LOCAL_POS_CALC;
	}
	if ( forceInstant )
	{
		if ( m_local_props.GetRotate() != rot )
		{
			// on the next update, this element will arrive at the target alpha
			mp_sprite->SetRotation( rot );
			m_local_props.SetRotate( rot );
			m_object_flags |= vNEEDS_LOCAL_POS_CALC;
		}
	}
}


void CSpriteElement::update()
{
	// HACK
	bool offscreen = false;
	if (m_summed_props.GetScreenUpperLeftY() < -200.0f || m_summed_props.GetScreenUpperLeftY() > 648.0f)
		offscreen = true;
	
	// IsHidden can be relatively slow (it's recursive), so only call once
	bool hidden = IsHidden();

	mp_sprite->SetHidden( hidden );

	// only change state of underlying sprite if there's been a change to CScreenElement/CSpriteElement visual state
	if ( ( m_object_flags & CScreenElement::vDID_SUMMED_POS_CALC ) || ( m_object_flags & CScreenElement::vCHANGED_STATIC_PROPS ) )
	{
		Image::RGBA true_rgba = m_local_props.GetRGBA();
		if (m_summed_props.alpha >= .0001f)
			true_rgba.a = (uint8) ((float) m_local_props.GetRGBA().a * m_summed_props.alpha);
		else
			true_rgba.a = 0;
		
		// GARRETT: 
		// Do all drawing here
		//
		// -screen coordinates of upper-left of sprite: m_summed_props.ulx, m_summed_props.uly
		// -scale to apply: m_summed_props.scale
		// -screen W,H of sprite (if needed): m_summed_props.scale * m_base_w, m_summed_props.scale * m_base_h
		//mp_sprite->SetPos(m_summed_props.ulx, m_summed_props.uly);
		mp_sprite->SetPos(m_summed_props.GetScreenPosX(), m_summed_props.GetScreenPosY());
		mp_sprite->SetSize(m_base_w, m_base_h);
		mp_sprite->SetAnchor(m_just_x, m_just_y);
		mp_sprite->SetScale(m_summed_props.GetScaleX(), m_summed_props.GetScaleY());
		
		if ( m_object_flags & CScreenElement::v3D_POS )
		{
			mp_sprite->SetZValue(m_summed_props.GetScreenPosZ());
			mp_sprite->SetHidden( ( ( m_object_flags & CScreenElement::v3D_CULLED ) || hidden ) );
		}
		else
		{
			mp_sprite->SetPriority( m_z_priority );
			mp_sprite->SetHidden( offscreen || hidden );
		}
		
		mp_sprite->SetRGBA(true_rgba);
		// mp_sprite->SetHidden( ( offscreen || hidden ) );
	

#if 0  // Garrett: Can't find window
		// Update the clip window.  This should only be done at init time, instead.
		CWindowElement *p_window = get_window();
		mp_sprite->SetWindow(p_window->GetClipWindow());
#endif

		// update the rotation angle
		mp_sprite->SetRotation( m_local_props.GetRotate() );
		// mp_sprite->SetRotation(mp_sprite->GetRotation() + 0.001f);

	}
}

void CSpriteElement::WritePropertiesToStruct( Script::CStruct* pStruct )
{
	CScreenElement::WritePropertiesToStruct( pStruct );

	// rotation angle
	pStruct->AddFloat( "rot_angle", m_local_props.GetRotate() );

	// texture name
	pStruct->AddChecksum( "texture", this->m_texture );
}

}
