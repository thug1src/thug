#ifndef __GFX_2D_TEXTELEMENT_H__
#define __GFX_2D_TEXTELEMENT_H__

#ifndef __GEL_OBJECT_H
#include <gel/object.h>
#endif
#include <gfx/2D/ScreenElement2.h>

#define TEXT_ELEMENT_MAX_LENGTH 96

namespace Nx
{
	class CFont;
	class CText;
}

namespace Front
{
	class CBlurEffect;

	enum
	{
		MAX_EDITABLE_TEXT_BLOCK_LENGTH = 512,
	};


class CTextElement : public CScreenElement
{
	friend class CScreenElementManager;
	friend class CTextBlockElement;

public:
							CTextElement();
	virtual					~CTextElement();

	void					SetProperties(Script::CStruct *pProps);
	void					SetMorph(Script::CStruct *pProps);
	
	void 					SetFont(uint32 font_checksum);
	void					SetText(const char *pText);
	char*					GetText() { return mp_text; }
	int						GetLength();

    bool					Concatenate( const char *pText );
	bool					Backspace();
	
	void					AttachBlurEffect();
	void					DetachBlurEffect();
	CBlurEffect	*			GetBlurEffect() {return mp_blur_effect;}

	void					SetShadowState(bool shadowOn);
	void					SetShadowOff(float offX, float offY);
	void					SetShadowRGBA(Image::RGBA shadowRGBA) {m_shadow_rgba = shadowRGBA;}

	void					SetEncodedRGBAOverride(bool override) {m_override_encoded_rgba = override;}

protected:

	void					create_text_instances(int numInstances, bool shadowOnly = false);
	void					destroy_text_instances(bool shadowOnly = false);
	
	enum
	{
		vMAX_TEXT_LENGTH	= TEXT_ELEMENT_MAX_LENGTH,
	};
	
	void					update();

	char *					mp_text;

	uint32					m_font_checksum;
	Nx::CFont *				mp_font;
	Nx::CText **			mpp_text_req_tab;
	Nx::CText **			mpp_shadow_req_tab;
	int						m_num_tab_entries;

	CBlurEffect	*			mp_blur_effect;

	bool					m_use_shadow;
	float					m_shadow_off_x;
	float					m_shadow_off_y;
	Image::RGBA				m_shadow_rgba;

	bool					m_override_encoded_rgba;
	bool					m_previous_override_rgba_state;
};




class CTextBlockElement : public CScreenElement
{
	friend class CScreenElementManager;

public:
							CTextBlockElement();
	virtual					~CTextBlockElement();

	void					SetProperties(Script::CStruct *pProps);
	void					SetMorph(Script::CStruct *pProps);
	
	void 					SetFont(uint32 font_checksum);
	void					SetText(const char **ppTextLines, int numLines);
	void					SetText(const char *pTextLine);

	bool					GetText( char* p_text, int size );
	int						GetLength();
	bool					Backspace();
	bool					Concatenate( const char* pText, bool enforce_max_width = false, bool last_line = false );

protected:

	enum EParseResult
	{ 
		NORMAL,
		NEXT_LINE,
		END_OF_BUFFER,
	};

	enum
	{
		MAX_LINES			= 20,
		MAX_CHARS			= TEXT_ELEMENT_MAX_LENGTH,
	};
	
	EParseResult 			parse_next_word(char *pWordBuf, const char **ppSource);
	void 					read_in_text_line(const char *pText);
	void					update();
	void 					forward_properties_to_children();

	uint32					m_font;

	char **					mpp_parsed_lines;
	// Line of text "outputted" so far. With a lot of text lines, only most recent ones are used.
	int						m_virtual_out_line;	
	int 					m_out_line;	// line of mpp_parsed_lines currently being written to
	int 					m_out_char; // on m_out_line
	float 					m_current_line_width;

	int						m_num_visible_lines;

	float					m_internal_just_x, m_internal_just_y;
	float					m_internal_scale;
    float					m_line_spacing_scale;
	float					m_total_height; // of contained TextElements
	int						m_total_out_lines; // number of contained TextElements
	
	CBlurEffect	*			mp_blur_effect;
	bool					m_use_shadow;
	bool					m_allow_expansion;
	float					m_shadow_off_x;
	float					m_shadow_off_y;
	Image::RGBA				m_shadow_rgba;
	
	bool					m_override_encoded_rgba;
	bool					m_previous_override_rgba_state;
};




}

#endif
