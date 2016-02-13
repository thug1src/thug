#include <core/defines.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/vecpair.h>
#include <gfx/NxFontMan.h>
#include <gfx/NxFont.h>
#include <gfx/2D/ScreenElement2.h>
#include <gfx/2D/ScreenElemMan.h>
#include <gfx/2D/TextElement.h>
#include <gfx/2D/BlurEffect.h>
#include <gfx/2D/Window.h>

#define BLUR_EFFECT_ON 1

namespace Front
{




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTextElement::CTextElement() :
	m_shadow_rgba(0, 0, 0, 128)
{
	//Ryan("I am a new CTextElement\n");
	m_font_checksum = 0;
	mp_font = NULL;
	mp_text = NULL;
	mp_blur_effect = NULL;
	m_use_shadow = false;
	create_text_instances(1);

	m_override_encoded_rgba = false;
	m_previous_override_rgba_state = false;
	
	SetType(CScreenElement::TYPE_TEXT_ELEMENT);
}




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTextElement::~CTextElement()
{
	//Ryan("Destroying CTextElement\n");
	DetachBlurEffect();
	destroy_text_instances();
	if (mp_text)
		delete mp_text;
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextElement::SetProperties(Script::CStruct *pProps)
{
	CScreenElement::SetProperties(pProps);

	uint32 font_crc;
	if (pProps->GetChecksum(CRCD(0x2f6bf72d,"font"), &font_crc))
		SetFont(font_crc);
	Dbg_MsgAssert(m_font_checksum, ("no font loaded"));
	
	const char *p_text;
	if (pProps->GetText(CRCD(0xc4745838,"text"), &p_text))
		SetText(p_text);

	Dbg_MsgAssert(m_id != Obj::CBaseManager::vNO_OBJECT_ID, ("what, no ID"));

	if (pProps->ContainsFlag(CRCD(0xd1653c6c,"blur_effect")))
		AttachBlurEffect();
	if (pProps->ContainsFlag(CRCD(0x4b73e4f8,"no_blur_effect")))
		DetachBlurEffect();

	Image::RGBA blur_rgba;
	if (resolve_rgba(pProps, CRCD(0x4f0e1182,"blur_rgba"), &blur_rgba) && mp_blur_effect)
		mp_blur_effect->SetRGBA(blur_rgba);

	Script::CPair shadow_offs;
	if (pProps->GetPair(CRCD(0x2a1fe0cc,"shadow_offs"), &shadow_offs))
	{
		SetShadowOff(shadow_offs.mX, shadow_offs.mY);
	}
	
	if (pProps->ContainsFlag(CRCD(0x8a897dd2,"shadow")))
		SetShadowState(true);
	if (pProps->ContainsFlag(CRCD(0x95b5b9a0,"no_shadow")))
		SetShadowState(false);
	
	resolve_rgba(pProps, CRCD(0x1e7bb1f5,"shadow_rgba"), &m_shadow_rgba);
	
	if ( pProps->ContainsFlag( CRCD(0xbf51a0c,"remember_override_rgba_state") ) )
	{
		Dbg_MsgAssert( !pProps->ContainsFlag( CRCD(0x61b7b58b,"restore_override_rgba_state") ), ( "restore_override_rgba_state and remember_override_rgba_state?" ) );
		m_previous_override_rgba_state = m_override_encoded_rgba;
	}
	if ( pProps->ContainsFlag( CRCD(0x61b7b58b,"restore_override_rgba_state") ) )
	{
		Dbg_MsgAssert( !pProps->ContainsFlag( CRCD(0xbf51a0c,"remember_override_rgba_state") ), ( "restore_override_rgba_state and remember_override_rgba_state?" ) );
		m_override_encoded_rgba = m_previous_override_rgba_state;
	}
	
	if (pProps->ContainsFlag(CRCD(0xb7686f92,"override_encoded_rgba")))
		m_override_encoded_rgba = true;
	if (pProps->ContainsFlag(CRCD(0xd25bc6cd,"dont_override_encoded_rgba")))
		m_override_encoded_rgba = false;
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextElement::SetMorph(Script::CStruct *pProps)
{	
	if (mp_blur_effect)
	{
		mp_blur_effect->SetMorph(pProps);
	}

	CScreenElement::SetMorph(pProps);
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextElement::SetFont(uint32 font_checksum)
{
	m_font_checksum = font_checksum;
	mp_font = Nx::CFontManager::sGetFont(m_font_checksum);
	Dbg_MsgAssert(Nx::CFontManager::sTestFontLoaded(m_font_checksum), ("font %s isn't loaded", Script::FindChecksumName(m_font_checksum)));
	m_object_flags |= CScreenElement::vCHANGED_STATIC_PROPS;	
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextElement::SetText(const char *pText)
{
	int new_length = strlen(pText) + 1;
	Dbg_MsgAssert(new_length < vMAX_TEXT_LENGTH, ("string too long %d", new_length));

	// if the old string is not big enough, then we need to allocate a new one	
	// otherwize, overwrite it, as allocations are slow
	if (!mp_text || (int)(strlen(mp_text)) < new_length-1)		  // note, does not account for actual allocated size, but will work is string are same size
	{
		if (mp_text)
			delete mp_text;	
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
		mp_text = new char[new_length];
		Mem::Manager::sHandle().PopContext();
	}
	// copy pText to mp_text, replacing '\m' tags with '\b' tags
	const char *p_in = pText;
	char *p_out = mp_text;
	while(*p_in)
	{
		if (*p_in == '\\' && *(p_in+1) == '\\')
		{
			// output "\\" tag unchanged
			*p_out++ = *p_in++;
			*p_out++ = *p_in++;
		}
		// ignore sticky space
		else if (*p_in == '\\' && *(p_in+1) == '_')
		{
			p_in++;
			p_in++;
			*p_out++ = ' ';
		}
		else if (*p_in == '\\' && *(p_in+1) == 'm')
		{
			*p_out++ = *p_in++;
			*p_out++ = 'b';
			p_in++;
			*p_out++ = Nx::CFontManager::sMapMetaCharacterToButton(p_in++);
		}
		else
		{
			*p_out++ = *p_in++;
		}
	}
	*p_out = '\0';
	
	if( mp_font )
	{
		float w, h;
		mp_font->QueryString(mp_text, w, h);	// width and height in pixels

		if (!(m_object_flags & vFORCED_DIMS)) 
		{
			SetDims(w, h);
		}
	}

	m_object_flags |= CScreenElement::vCHANGED_STATIC_PROPS;	
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CTextElement::GetLength()
{
	return strlen( mp_text );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTextElement::Concatenate( const char *pText )
{
	int old_length = 0;
	if ( mp_text )
		old_length += strlen( mp_text );
	int length  = old_length + strlen( pText );
	
	if ( length + 1 >= vMAX_TEXT_LENGTH )
	{
		// new string would be too long...
		return false;
	}

	if ( length > 0 )
	{
		char new_string[vMAX_TEXT_LENGTH];
		
		// initialize string
		strcpy( new_string, "" );
		if ( mp_text )
			strcpy( new_string, mp_text );

		// add new text
		strcat( new_string, pText );
	
		SetText( new_string );
		return true;
	}
	return false;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTextElement::Backspace()
{
	if ( !mp_text )
		return false;
	
	int length = strlen( mp_text );

	if ( length == 0 )
		return false;

	// copy string
	char new_string[vMAX_TEXT_LENGTH];
	strcpy( new_string, mp_text );
	
	// take care of escaped backslash char (ASCII code 92)
	if ( length > 1 && new_string[length - 1] == 92 && new_string[length - 2] == 92 )
		new_string[length - 2] = '\0';
	
	// terminate
	new_string[length - 1] = '\0';
	SetText( new_string );
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextElement::AttachBlurEffect()
{
	#if BLUR_EFFECT_ON
	if (!mp_blur_effect)
	{
		destroy_text_instances();
		
		// create blur effect according to specs
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
		mp_blur_effect = new CBlurEffect();
		// mp_blur_effect->SetRGBA(m_rgba);
		mp_blur_effect->SetRGBA( m_local_props.GetRGBA() );
		Mem::Manager::sHandle().PopContext();
	
		// get number of CText objects needed
		// create 'em
		create_text_instances(mp_blur_effect->GetNumEntries() + 1);
	}
	#endif
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextElement::DetachBlurEffect()
{
	if (mp_blur_effect)
	{
		destroy_text_instances();
		delete mp_blur_effect;
		mp_blur_effect = NULL;
		create_text_instances(1);
	}
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextElement::SetShadowState(bool shadowOn)
{
	if (shadowOn && !m_use_shadow)
	{
		m_use_shadow = true;
		create_text_instances(m_num_tab_entries, true);
	}
	else if (!shadowOn && m_use_shadow)
	{
		m_use_shadow = false;
		destroy_text_instances(true);
	}
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextElement::SetShadowOff(float offX, float offY)
{
	m_shadow_off_x = offX;
	m_shadow_off_y = offY;
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextElement::update()
{
	//Dbg_Message("I am drawing text element at (%.1f,%.1f), scale %.1f\n", m_summed_props.ulx, m_summed_props.uly, m_summed_props.scalex);
#if 0
	if( mp_font )
	{
		mp_font->BeginText( *((uint32 *) &m_local_props.GetRGBA() ), m_summed_props.scalex);
		mp_font->DrawString(mp_text, m_summed_props.ulx, m_summed_props.uly);
		mp_font->EndText();
	}
#else

	if (!mp_text) return;
	
	bool offscreen = false;
	// HACK
	if (m_summed_props.GetScreenUpperLeftY() < -200.0f || m_summed_props.GetScreenUpperLeftY() > 648.0f)
		offscreen = true;

	if (m_num_tab_entries)
	{
		Image::RGBA true_rgba = m_local_props.GetRGBA();
		if (m_summed_props.alpha >= .0001f)
			true_rgba.a = (uint8) ((float) m_local_props.GetRGBA().a * m_summed_props.alpha);
		else
			true_rgba.a = 0;
		
		/* 
			-pass current pos, scale, alpha (local) to blur effect
			-for each entry in table
				-get pos, scale, alpa (in local, relative to current)
		*/
		
		mpp_text_req_tab[0]->SetFont(mp_font);
		mpp_text_req_tab[0]->SetPos(m_summed_props.GetScreenUpperLeftX(), m_summed_props.GetScreenUpperLeftY());
		mpp_text_req_tab[0]->SetRGBA(true_rgba, m_override_encoded_rgba);
		mpp_text_req_tab[0]->SetScale(m_summed_props.GetScaleX(), m_summed_props.GetScaleY());
		mpp_text_req_tab[0]->SetString(mp_text);
		if (m_object_flags & CScreenElement::v3D_POS)
		{
			mpp_text_req_tab[0]->SetZValue(m_summed_props.GetScreenPosZ());
			mpp_text_req_tab[0]->SetHidden( ( ( m_object_flags & CScreenElement::v3D_CULLED ) || IsHidden() ) );
		} else {
			mpp_text_req_tab[0]->SetPriority(m_z_priority);
			mpp_text_req_tab[0]->SetHidden( ( offscreen || IsHidden() ) );
		}

#if 0  // Garrett: Can't find window
		// Update the clip window.  This should only be done at init time, instead.
		CWindowElement *p_window = get_window();
		mpp_text_req_tab[0]->SetWindow(p_window->GetClipWindow());
#endif


		if (m_use_shadow)
		{
			Image::RGBA true_rgba = m_shadow_rgba;
			if (m_summed_props.alpha >= .0001f)
				true_rgba.a = (uint8) ((float) m_shadow_rgba.a * m_summed_props.alpha);
			else
				true_rgba.a = 0;
			mpp_shadow_req_tab[0]->SetFont(mp_font);
			mpp_shadow_req_tab[0]->SetPos(m_summed_props.GetScreenUpperLeftX() + m_shadow_off_x, m_summed_props.GetScreenUpperLeftY() + m_shadow_off_y);
			mpp_shadow_req_tab[0]->SetRGBA(true_rgba, m_override_encoded_rgba);
			mpp_shadow_req_tab[0]->SetScale(m_summed_props.GetScaleX(), m_summed_props.GetScaleY());
			mpp_shadow_req_tab[0]->SetString(mp_text);
			if (m_object_flags & CScreenElement::v3D_POS)
			{
				mpp_shadow_req_tab[0]->SetZValue(m_summed_props.GetScreenPosZ());
				mpp_shadow_req_tab[0]->SetHidden( ( ( m_object_flags & CScreenElement::v3D_CULLED ) || IsHidden() ) );
			} else {
				mpp_shadow_req_tab[0]->SetPriority(m_z_priority - .005f);
				mpp_shadow_req_tab[0]->SetHidden( ( offscreen || IsHidden() ) );
			}
		}
	}
	if(mp_blur_effect)
	{
		/* 
			-pass current center pos, scale, alpha (local) to blur effect
			-for each entry in table
				-get center pos, scale, alpa (in local, relative to current)
		*/

		Dbg_MsgAssert(!(m_object_flags & CScreenElement::v3D_POS), ("Can't use motion blur on 3D positioned text"));

		mp_blur_effect->Update();

		// work out absolute center of this element
		float center_x = m_summed_props.GetScreenUpperLeftX() + m_summed_props.GetScaleX() * m_base_w / 2.0f;
		float center_y = m_summed_props.GetScreenUpperLeftY() + m_summed_props.GetScaleY() * m_base_h / 2.0f;

		for (int i = 1; i < m_num_tab_entries; i++)
		{
			ConcatProps &blur_entry = mp_blur_effect->GetInfo(i-1);
			
			Image::RGBA true_rgba = mp_blur_effect->GetRGBA();
			true_rgba.a = (uint8) ((float) true_rgba.a * blur_entry.alpha);
			
			float draw_pos_x = center_x - (blur_entry.GetScreenPosX() + m_base_w * blur_entry.GetScaleX() / 2.0f) * m_summed_props.GetScaleX();
			float draw_pos_y = center_y - (blur_entry.GetScreenPosY() + m_base_h * blur_entry.GetScaleY() / 2.0f) * m_summed_props.GetScaleY();
			float draw_scale_x = m_summed_props.GetScaleX() * blur_entry.GetScaleX();
			float draw_scale_y = m_summed_props.GetScaleY() * blur_entry.GetScaleY();
			
			mpp_text_req_tab[i]->SetPos(draw_pos_x, draw_pos_y);
			mpp_text_req_tab[i]->SetRGBA(true_rgba, m_override_encoded_rgba);
			mpp_text_req_tab[i]->SetScale(draw_scale_x, draw_scale_y);
			mpp_text_req_tab[i]->SetPriority(m_z_priority - i * .01f);
			
			mpp_text_req_tab[i]->SetFont(mp_font);
			mpp_text_req_tab[i]->SetString(mp_text);
			mpp_text_req_tab[i]->SetHidden( ( offscreen || IsHidden() ) );

			if (m_use_shadow)
			{
				Image::RGBA true_rgba = m_shadow_rgba;
				if (m_summed_props.alpha >= .0001f)
					true_rgba.a = (uint8) ((float) m_shadow_rgba.a * m_summed_props.alpha);
				else
					true_rgba.a = 0;
				mpp_shadow_req_tab[i]->SetPos(draw_pos_x + m_shadow_off_x * draw_scale_x, 
											draw_pos_y + m_shadow_off_y * draw_scale_y);
				mpp_shadow_req_tab[i]->SetRGBA(true_rgba, m_override_encoded_rgba);
				mpp_shadow_req_tab[i]->SetScale(draw_scale_x, draw_scale_y);
				mpp_shadow_req_tab[i]->SetPriority(m_z_priority - i * .01f - .005f);

				mpp_shadow_req_tab[i]->SetFont(mp_font);
				mpp_shadow_req_tab[i]->SetString(mp_text);
				mpp_shadow_req_tab[i]->SetHidden( ( offscreen || IsHidden() ) );
			}
		}
	}	
#endif
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextElement::create_text_instances(int numEntries, bool shadow_only)
{
	Dbg_Assert(!mpp_shadow_req_tab);

	if (m_use_shadow)
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
		mpp_shadow_req_tab = new Nx::CText*[numEntries];
		Mem::Manager::sHandle().PopContext();
		for (int i = 0; i < numEntries; i++)
		{
			mpp_shadow_req_tab[i] = Nx::CTextMan::sGetTextInstance();
		}
	}
	
	if (!shadow_only)
	{
		Dbg_Assert(!m_num_tab_entries);
	
		m_num_tab_entries = numEntries;
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
		mpp_text_req_tab = new Nx::CText*[numEntries];
		Mem::Manager::sHandle().PopContext();
		for (int i = 0; i < numEntries; i++)
		{
			mpp_text_req_tab[i] = Nx::CTextMan::sGetTextInstance();
		}
	}
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextElement::destroy_text_instances(bool shadow_only)
{
	if (mpp_shadow_req_tab)
	{
		for (int i = 0; i < m_num_tab_entries; i++)
		{
			Nx::CTextMan::sFreeTextInstance(mpp_shadow_req_tab[i]);
		}
		delete mpp_shadow_req_tab;
		mpp_shadow_req_tab = NULL;
	}
	
	if (!shadow_only)
	{
		Dbg_Assert(m_num_tab_entries);
		
		for (int i = 0; i < m_num_tab_entries; i++)
		{
			Nx::CTextMan::sFreeTextInstance(mpp_text_req_tab[i]);
		}
		delete mpp_text_req_tab;
		m_num_tab_entries = 0;
	}
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTextBlockElement::CTextBlockElement() :
	m_shadow_rgba(0, 0, 0, 128)
{
	//Ryan("I am a new CTextBlockElement\n");
	m_font = 0;
	m_internal_scale = 1.0f;
    m_line_spacing_scale = 1.0f;
	m_total_height = 0.0f;
	m_total_out_lines = 0;

	mp_blur_effect = NULL;
	m_allow_expansion = false;

	m_override_encoded_rgba = false;
	m_previous_override_rgba_state = false;
	
	SetType(CScreenElement::TYPE_TEXT_BLOCK_ELEMENT);
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTextBlockElement::~CTextBlockElement()
{	
	//Ryan("Destroying CTextBlockElement\n");
	if (mp_blur_effect)
		delete mp_blur_effect;
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextBlockElement::SetProperties(Script::CStruct *pProps)
{
	CScreenElement::SetProperties(pProps);

	// must find this before setting font
	if (pProps->GetFloat(CRCD(0x1fe341d2,"internal_scale"), &m_internal_scale))
		m_object_flags |= CScreenElement::vCHANGED_STATIC_PROPS;

    pProps->GetFloat(CRCD(0xe3fa22fc,"line_spacing"), &m_line_spacing_scale);
    
	uint32 font_crc;
	if (pProps->GetChecksum(CRCD(0x2f6bf72d,"font"), &font_crc))
		SetFont(font_crc);
	Dbg_MsgAssert(m_font, ("no font loaded"));
	

	if (resolve_just(pProps, CRCD(0x67e093e4,"internal_just"), &m_internal_just_x, &m_internal_just_y))
		m_object_flags |= CScreenElement::vCHANGED_STATIC_PROPS;
	

	const char *pp_line_tab[32];
	Script::CArray *p_text_array;
	
	if (pProps->ContainsFlag( CRCD(0x322839a2,"allow_expansion") ))
		m_allow_expansion = true;
	
	if (pProps->GetText(CRCD(0xc4745838,"text"), pp_line_tab))
	{
		SetText(pp_line_tab, 1);
	}
	else if (pProps->GetArray(CRCD(0xc4745838,"text"), &p_text_array))
	{
		int num_entries = p_text_array->GetSize();
		for (int i = 0; i < num_entries; i++)
			pp_line_tab[i] = p_text_array->GetString(i);

		SetText(pp_line_tab, num_entries);
	}

	/*
		Property changes to be forwared to children follow:	
	*/
	
	if (pProps->ContainsFlag(CRCD(0xd1653c6c,"blur_effect")) && !mp_blur_effect)
	{
		#if BLUR_EFFECT_ON
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
		mp_blur_effect = new CBlurEffect();
		// mp_blur_effect->SetRGBA(m_rgba);
		mp_blur_effect->SetRGBA( m_local_props.GetRGBA() );
		Mem::Manager::sHandle().PopContext();
		#endif
	}
	else if (pProps->ContainsFlag(CRCD(0x4b73e4f8,"no_blur_effect")) && mp_blur_effect)
	{
		delete mp_blur_effect;
		mp_blur_effect = NULL;
	}
	
	Image::RGBA blur_rgba;
	if (resolve_rgba(pProps, CRCD(0x4f0e1182,"blur_rgba"), &blur_rgba) && mp_blur_effect)
	{
		mp_blur_effect->SetRGBA(blur_rgba);
	}

	Script::CPair shadow_offs;
	if (pProps->GetPair(CRCD(0x2a1fe0cc,"shadow_offs"), &shadow_offs))
	{
		m_shadow_off_x = shadow_offs.mX;
		m_shadow_off_y = shadow_offs.mY;
	}
	
	if (pProps->ContainsFlag(CRCD(0x8a897dd2,"shadow")))
		m_use_shadow = true;
	if (pProps->ContainsFlag(CRCD(0x95b5b9a0,"no_shadow")))
		m_use_shadow = false;

	resolve_rgba(pProps, CRCD(0x1e7bb1f5,"shadow_rgba"), &m_shadow_rgba);
	
	if ( pProps->ContainsFlag( CRCD(0xbf51a0c,"remember_override_rgba_state") ) )
	{
		Dbg_MsgAssert( !pProps->ContainsFlag( CRCD(0x61b7b58b,"restore_override_rgba_state") ), ( "restore_override_rgba_state and remember_override_rgba_state?" ) );
		m_previous_override_rgba_state = m_override_encoded_rgba;
	}
	if ( pProps->ContainsFlag( CRCD(0x61b7b58b,"restore_override_rgba_state") ) )
	{
		Dbg_MsgAssert( !pProps->ContainsFlag( CRCD(0xbf51a0c,"remember_override_rgba_state") ), ( "restore_override_rgba_state and remember_override_rgba_state?" ) );
		m_override_encoded_rgba = m_previous_override_rgba_state;
	}
	
	if (pProps->ContainsFlag(CRCD(0xb7686f92,"override_encoded_rgba")))
		m_override_encoded_rgba = true;
	if (pProps->ContainsFlag(CRCD(0xd25bc6cd,"dont_override_encoded_rgba")))
		m_override_encoded_rgba = false;
	
	forward_properties_to_children();
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextBlockElement::SetMorph(Script::CStruct *pProps)
{	
	if (mp_blur_effect)
	{
		// Distribute blur effect settings to children, account for different 
		// vertical positions of children relative to center of this
		
		// cheap hack, update self so that contained elements have proper position
		update();
		
		Tmr::Time target_time;
		const CBlurEffect::Props &blur_target = mp_blur_effect->SetMorph(pProps, &target_time);
		//printf("%f\n", blur_target.topAlpha);
		// hook up blur effect to children
		
		CScreenElementPtr p_child = CScreenElement::GetFirstChild();
		while(p_child)
		{
			
			Dbg_MsgAssert(p_child->GetType() == CScreenElement::TYPE_TEXT_ELEMENT,("TextBlockElement %s has child %s that is NOT a CTextElement (it's %s)\n",
			Script::FindChecksumName(GetID()), Script::FindChecksumName(p_child->GetID()), Script::FindChecksumName(p_child->GetType())));

			
			// "child blur target"
			CBlurEffect::Props cbt = blur_target;

			// find center of child relative to center of this
			// (we know we'll be getting top of child, since that's the just this class sets)
			float child_center_x, child_center_y;
			p_child->GetLocalPos(&child_center_x, &child_center_y);
			child_center_y += p_child->GetBaseH() / 2.0f;
			float disp_y = m_base_h / 2.0f - child_center_y;
			cbt.maxDisplacementY += blur_target.bottomScaleY * disp_y - disp_y;
			
			CBlurEffect *p_child_blur = ((CTextElement *) ((CScreenElement *) p_child))->GetBlurEffect();
			Dbg_Assert(p_child_blur);
			p_child_blur->SetAllTargetProps(cbt, target_time);
			p_child = p_child->GetNextSibling();
		}
	}

	CScreenElement::SetMorph(pProps);
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextBlockElement::SetFont(uint32 font_checksum)
{
	Dbg_MsgAssert(!m_font, ("font already set"));
	m_font = font_checksum;
	Dbg_MsgAssert(Nx::CFontManager::sTestFontLoaded(m_font), ("font %s isn't loaded", Script::FindChecksumName(m_font)));

	// how many lines will fit?
	
	Nx::CFont* p_font = Nx::CFontManager::sGetFont(m_font);
	Dbg_MsgAssert(p_font, ("no font found"));
	
	// see if word wraps past end of element
	float word_w, word_h;
	p_font->QueryString("AAA", word_w, word_h);	// width and height in pixels

	m_num_visible_lines = (int) m_base_h / (int) (word_h * m_internal_scale);
	m_object_flags |= CScreenElement::vCHANGED_STATIC_PROPS;
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextBlockElement::SetText(const char **ppTextLines, int numLines)
{
	Dbg_MsgAssert(m_object_flags & vFORCED_DIMS, ("no dimensions have been set"));
	Dbg_MsgAssert(numLines < 256, ("large number of lines, probably not good"));

//	char parsed_lines[MAX_LINES][MAX_CHARS]; 
								  
	CScreenElementManager* p_manager = CScreenElementManager::Instance();
	
	// destroy any children that are present
	SetChildLockState( UNLOCK );
	CScreenElementPtr p_child = GetFirstChild();
	while ( p_child )
	{
		CScreenElementPtr p_next = p_child->GetNextSibling();
		p_manager->DestroyElement(p_child->GetID());
		p_child = p_next;
	}
	Dbg_Assert( !GetFirstChild() );

	Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().FrontEndHeap() );
	mpp_parsed_lines = new char*[MAX_LINES];
	// (Mick) in order to avoid doing multiple allocations, we just
	// allocate a single array big enough for all the lines
	mpp_parsed_lines[0] = new char[MAX_CHARS * MAX_LINES];
	Mem::Manager::sHandle().PopContext();
	// then calculate the pointers for all the other lines, as an offset to this
	for ( int l = 1; l < MAX_LINES; l++ )
	{
		mpp_parsed_lines[l] = mpp_parsed_lines[0] + MAX_CHARS*l;
	}
	for( int i = 0; i < MAX_LINES; i++ )
	{
		mpp_parsed_lines[i][0] = '\0';
	}

	m_out_char = 0;
	m_virtual_out_line = 0;
	m_out_line = 0;
	m_current_line_width = 0.0f;
	
	// reset number of visible lines
	if ( m_allow_expansion )
	{
		m_base_h = 0.0f;
		m_num_visible_lines = 0;
	}
	
	for ( int in_line = 0; in_line < numLines; in_line++ )
		read_in_text_line( ppTextLines[in_line] );
		
	// printf("line %d: %s\n", m_out_line, mpp_parsed_lines[m_out_line]);
	// want to handle the "no text" case
	if (numLines)
	{
		m_virtual_out_line++;
		m_out_line++;
	}

	// we only to want to use the N most recent lines,
	// where N is the number of lines that will fit
	int buf_use_line = 0;
	m_total_out_lines = m_out_line;
	if ( m_virtual_out_line >= m_num_visible_lines )
	{
		// expand the element if we're supposed to
		if ( m_allow_expansion )
		{
			Nx::CFont* p_font = Nx::CFontManager::sGetFont(m_font);
			Dbg_MsgAssert(p_font, ("no font found"));
			
			// change the height of the element
			float word_w, word_h;
			p_font->QueryString("AAA", word_w, word_h );	// width and height in pixels
			m_base_h = m_virtual_out_line * (word_h * m_internal_scale * m_line_spacing_scale );

			m_num_visible_lines = (int) m_base_h / (int) (word_h * m_internal_scale * m_line_spacing_scale );

			// re-parse the text now that m_num_visible_lines has changed
// Mick: no need for this, since they are fixed size, we just overwrite them
//			for (int l = 0; l < MAX_LINES; l++)
//			{
//				delete mpp_parsed_lines[l];
//				mpp_parsed_lines[l] = new char[MAX_CHARS];
//			}
			m_out_char = 0;
			m_virtual_out_line = 0;
			m_out_line = 0;
			m_current_line_width = 0.0f;
			for (int i = 0; i < numLines; i++)
				read_in_text_line(ppTextLines[i]);

			// printf("changing m_total_out_lines from %i to %i\n", m_total_out_lines, m_num_visible_lines);
			m_total_out_lines = m_num_visible_lines;
		}
		else 
		{
			Dbg_MsgAssert( m_num_visible_lines, ("Text block element too small for one line") );
            buf_use_line = m_virtual_out_line % m_num_visible_lines;
			m_total_out_lines = m_num_visible_lines;
		}
	}


	// create new children
	m_total_height = 0.0f;
	for (int i = 0; i < m_total_out_lines; i++)
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
		CTextElement *p_new_elem = new CTextElement();
		Mem::Manager::sHandle().PopContext();
		p_manager->RegisterObject(*p_new_elem);
		p_manager->SetParent(this, p_new_elem);
		p_new_elem->SetFont(m_font);
		p_new_elem->SetText(&mpp_parsed_lines[buf_use_line][0]);
		p_new_elem->SetScale(m_internal_scale, m_internal_scale);
	
		
		m_total_height += p_new_elem->GetBaseH() * m_internal_scale * m_line_spacing_scale;
		buf_use_line++;
		if (buf_use_line >= m_num_visible_lines)
			buf_use_line = 0;
	}

	// delete the lines, now we've used them
// Mick: Now we just delete the array of chars, and the array of pointers
//	for (int l = 0; l < MAX_LINES; l++)
//	{
//		delete mpp_parsed_lines[l];
//		mpp_parsed_lines[l] = NULL;			
//	}
	delete mpp_parsed_lines[0];
	delete mpp_parsed_lines;

	
	SetChildLockState(LOCK);

	m_object_flags |= CScreenElement::vCHANGED_STATIC_PROPS;
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextBlockElement::SetText(const char *pTextLine)
{
	SetText(&pTextLine, 1);
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTextBlockElement::GetText( char* p_text, int size )
{
	strcpy( p_text, "" );
	int total_length = 0;
	
	CScreenElementPtr p_child = GetFirstChild();
	while( p_child )
	{
		Dbg_MsgAssert(p_child->GetType() == CScreenElement::TYPE_TEXT_ELEMENT,("TextBlockElement %s has child %s that is NOT a CTextElement (it's %s)\n",
		Script::FindChecksumName(GetID()), Script::FindChecksumName(p_child->GetID()), Script::FindChecksumName(p_child->GetType())));

	
		CScreenElementPtr p_next = p_child->GetNextSibling();
		CTextElement* p_text_element = (CTextElement*)p_child.Convert();

		char* p_text_element_string = p_text_element->GetText();
		total_length += strlen( p_text_element_string );
		
		if ( p_next )
			total_length += 1;
	
		if ( total_length < size )
		{
			strcat( p_text, p_text_element_string );
			
			// add a space between lines if this isn't the last line
			if ( p_next )
				strcat( p_text, " " );
		}
		else
		{
			Dbg_MsgAssert( 0, ( "TextBlockElement::GetText - text too long" ) );
			return false;
		}
		p_child = p_next;
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CTextBlockElement::GetLength()
{
	// printf( "CTextBlockElement::GetLength called on %s\n", Script::FindChecksumName( m_id ) );
	int total_length = 0;
	
	CScreenElementPtr p_child = GetFirstChild();
	while( p_child )
	{
		Dbg_MsgAssert(p_child->GetType() == CScreenElement::TYPE_TEXT_ELEMENT,("TextBlockElement %s has child %s that is NOT a CTextElement (it's %s)\n",
		Script::FindChecksumName(GetID()), Script::FindChecksumName(p_child->GetID()), Script::FindChecksumName(p_child->GetType())));
		
		CScreenElementPtr p_next = p_child->GetNextSibling();
		CTextElement* p_text_element = (CTextElement*)p_child.Convert();
		total_length += p_text_element->GetLength();
		
		if ( p_next )
			total_length += 1;
		
		p_child = p_next;
	}	
	return total_length;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTextBlockElement::Backspace()
{
	int length = GetLength();

	if ( length == 0 )
		return false;

	Dbg_MsgAssert( length < MAX_EDITABLE_TEXT_BLOCK_LENGTH, ( "Backspace failed - string too long" ) );
	char new_string[MAX_EDITABLE_TEXT_BLOCK_LENGTH];
	GetText( new_string, MAX_EDITABLE_TEXT_BLOCK_LENGTH );
	
	if ( !new_string )
		return false;
	
	// take care of escaped backslash char (ASCII code 92)
	if ( length > 1 && new_string[length - 1] == 92 && new_string[length - 2] == 92 )
		new_string[length - 2] = '\0';
	
	// terminate
	new_string[length - 1] = '\0';
	SetText( new_string );
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
/*
	Called from read_in_text_line()
*/
CTextBlockElement::EParseResult CTextBlockElement::parse_next_word(char *pWordBuf, const char **ppSource)
{
	bool keep_scanning = true;
	EParseResult result = NORMAL; 
	
	char *p_out = pWordBuf;
	
	while (keep_scanning)
	{
		switch(**ppSource)
		{
			case ' ':
				*p_out++ = **ppSource;
				(*ppSource)++;
				keep_scanning = false;
				break;
			case '\\':
			{
				(*ppSource)++;
				if (**ppSource == '_')
				{
					*p_out++ = ' ';
					(*ppSource)++;
				}
				else if (**ppSource == 'n')
				{
					(*ppSource)++;
					result = NEXT_LINE;
					keep_scanning = false;
				}
				else
				{
					// is \?, where ? is some character
					// output both
					*p_out++ = '\\';
					*p_out++ = **ppSource;
					(*ppSource)++;
				}
				break;
			} // end case
			case '\0':
				result = END_OF_BUFFER;
				keep_scanning = false;
				break;
			default:
			{
				*p_out++ = **ppSource;
				(*ppSource)++;
				break;
			}
		} // end switch

		Dbg_Assert((int) p_out - (int) pWordBuf < MAX_CHARS - 1);
	}

	*p_out++ = '\0';
	return result;
}




/*
	Called from SetText()
*/
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


void CTextBlockElement::read_in_text_line(const char *pText)
{
	Nx::CFont* p_font = Nx::CFontManager::sGetFont(m_font);
	Dbg_MsgAssert(p_font, ("no font found"));
	
	const char *p_in = pText;

	char p_word[MAX_CHARS];

	EParseResult last_result = NORMAL;
	
	// printf("reading in: %s\n", pText);
	
	while ( last_result != END_OF_BUFFER )
	{
		EParseResult result = parse_next_word( p_word, &p_in );
		
		// see if word wraps past end of element
		float word_w, word_h;
		p_font->QueryString(p_word, word_w, word_h);	// width and height in pixels
		word_w *= m_internal_scale;
		
		// printf("result = %d, word = '%s', screen_w = %.2f\n", result, p_word, word_w);

		// if adding this word makes the line too long
		// or we had a newline char or similar
		// or this word will overflow the buffer (unlikely, but possible)
		if ( ( m_current_line_width > 0.0f && m_current_line_width + word_w > m_base_w )
			 ||  ( last_result == NEXT_LINE )
			 ||  ( m_out_char + strlen( p_word ) >= MAX_CHARS )
		   )
		{
			// printf("m_current_line_width = %f, word_w = %f, m_base_w = %f\n", m_current_line_width, word_w, m_base_w);
			// printf("line %d: %s\n", m_out_line, mpp_parsed_lines[m_out_line]);
			
			// if last character on line is a space, do away with it
			if ( mpp_parsed_lines[m_out_line][m_out_char-1] == ' ' )
				mpp_parsed_lines[m_out_line][m_out_char-1] = '\0';
			
			// Wrap to next	line.
			// Once we fill all visible lines, we start recycling the buffer,
			// keeping only the most recent lines, hence the virtual line stuff.
			m_virtual_out_line++;
			m_out_line++;
			if ( m_out_line >= m_num_visible_lines )
				m_out_line = 0;
			m_out_char = 0;
			m_current_line_width = 0.0f;
		}
		
		Dbg_Assert(m_out_line < MAX_LINES); // need to increase MAX_LINES if triggered
		Dbg_MsgAssert(m_out_char + strlen(p_word) < MAX_CHARS, ("overflow, max allowed = %d, needed = %d\n%s%s\n", MAX_CHARS, m_out_char + strlen(p_word),&mpp_parsed_lines[m_out_line][0],p_word));
		char *p_out = &mpp_parsed_lines[m_out_line][0] + m_out_char;
		strcpy(p_out, p_word);
		m_out_char += strlen(p_word);
		m_current_line_width += word_w;

		last_result = result;
	}
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextBlockElement::update()
{
	if (m_object_flags & CScreenElement::vCHANGED_STATIC_PROPS)
	{
		Dbg_Assert(m_total_height > .00001f || !m_total_out_lines);
		
		// position children
		float y = (m_internal_just_y + 1.0f) * (m_base_h - m_total_height) / 2.0f;
		CScreenElementPtr p_child = GetFirstChild();
		for (int i = 0; i < m_total_out_lines; i++)
		{
			Dbg_MsgAssert(p_child->GetType() == CScreenElement::TYPE_TEXT_ELEMENT,("TextBlockElement %s has child %s that is NOT a CTextElement (it's %s)\n",
			Script::FindChecksumName(GetID()), Script::FindChecksumName(p_child->GetID()), Script::FindChecksumName(p_child->GetType())));
			
			p_child->SetJust(m_internal_just_x, -1.0f);
			// p_child->SetRGBA(m_rgba);
			p_child->SetRGBA( m_local_props.GetRGBA() );
			p_child->SetPos((m_internal_just_x + 1.0f) * m_base_w / 2.0f, y);

			y += p_child->GetBaseH() * m_internal_scale * m_line_spacing_scale;
			p_child = p_child->GetNextSibling();
		}
		forward_properties_to_children();
	}
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTextBlockElement::forward_properties_to_children()
{
	CScreenElementPtr p_child = CScreenElement::GetFirstChild();
	while(p_child)
	{
		
		Dbg_MsgAssert(p_child->GetType() == CScreenElement::TYPE_TEXT_ELEMENT,("TextBlockElement %s has child %s that is NOT a CTextElement (it's %s)\n",
			Script::FindChecksumName(GetID()), Script::FindChecksumName(p_child->GetID()), Script::FindChecksumName(p_child->GetType())));
		
		if (mp_blur_effect)
			((CTextElement *) ((CScreenElement *) p_child))->AttachBlurEffect();
		else
			((CTextElement *) ((CScreenElement *) p_child))->DetachBlurEffect();
		if (m_use_shadow)
			((CTextElement *) ((CScreenElement *) p_child))->SetShadowState(true);
		else
			((CTextElement *) ((CScreenElement *) p_child))->SetShadowState(false);
		((CTextElement *) ((CScreenElement *) p_child))->SetShadowOff(m_shadow_off_x, m_shadow_off_y);
		((CTextElement *) ((CScreenElement *) p_child))->SetShadowRGBA(m_shadow_rgba);
		((CTextElement *) ((CScreenElement *) p_child))->SetEncodedRGBAOverride(m_override_encoded_rgba);
		p_child = p_child->GetNextSibling();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// if enforce_max_width is true, the function will fail if a single word is
// bigger than the width of a line.  That is, if the text would go outside
// the specified width and there's no way to wrap, concatenation will fail.
// Only the end of the string is checked.  The text already in the element
// is assumed to be safe.  Thus it's possible to have a textblock element go
// outside it's box by using SetText directly, rather than using concatenate
bool CTextBlockElement::Concatenate( const char* pText, bool enforce_max_width, bool last_line )
{
#ifdef __NOPT_ASSERT__
	int length = GetLength();
	Dbg_MsgAssert( length < (int)( MAX_EDITABLE_TEXT_BLOCK_LENGTH - strlen( pText ) ), ( "TextBlock too long to concatenate" ) );
#endif
	
	// setup new, unwrapped string
	char* p_new_string = new char[MAX_EDITABLE_TEXT_BLOCK_LENGTH];
	GetText( p_new_string, Front::MAX_EDITABLE_TEXT_BLOCK_LENGTH );
	strcat( p_new_string, pText );

	// enforce max width but allow them to add a space.
	if ( (enforce_max_width && strcmp( pText, " " ) != 0) || last_line)
	{
		Nx::CFont* p_font = Nx::CFontManager::sGetFont( m_font );
		Dbg_MsgAssert( p_font, ( "no font found" ) );

		// parse through the new text word by word
		const char *p_in = p_new_string;
		char p_word[MAX_CHARS];

		// skip up to the last word
		EParseResult last_result = NORMAL;
		while ( last_result != END_OF_BUFFER )
		{			
			last_result = parse_next_word( p_word, &p_in );
		}
			
		// check that each word is not longer than the max width
		float word_w, word_h;
		p_font->QueryString( p_word, word_w, word_h );	// width and height in pixels
		word_w *= m_internal_scale;
		if ( word_w > m_base_w )
		{
			// too long!
			delete p_new_string;
			return false;
		}
	}
	
	SetText( p_new_string );
	delete p_new_string;
	return true;
}


}
