#ifndef __GFX_2D_FONTMAN_H__
#define __GFX_2D_FONTMAN_H__

#include <core/hashtable.h>

#include <gfx/NxFont.h>

namespace Script
{
	class CScriptStructure;
	class CScript;
}

namespace Nx
{

class CFontManager
{
public:
	static void					sLoadFont(const char *pName, int charSpacing = 0, 
										  int spaceSpacing = 0, Image::RGBA *pColorTab = NULL,
										  bool isButtonFont = false);
	static void					sUnloadFont(const char *pName);
	static Nx::CFont *			sGetFont(const char *pName);
	static Nx::CFont *			sGetFont(uint32 checksum);

	static const char *			sTestFontLoaded(uint32 checksum);
	
	static char					sMapMetaCharacterToButton(const char *pMetaChar);

private:
	// Constants
	enum {
		vMAX_FONT_ENTRIES 		= 16,
		NUM_META_BUTTON_ENTRIES = 32,
	};


	struct FontEntry
	{
		enum {
			vMAX_NAME_SIZE = 24
		};

								FontEntry() : mp_font(NULL) { }

		char					mName[vMAX_NAME_SIZE];
		Nx::CFont *				mp_font;
	};
	
	static FontEntry 					s_font_tab[vMAX_FONT_ENTRIES];
	static Lst::HashTable<FontEntry>	s_font_lookup;

	static char					s_meta_button_map[NUM_META_BUTTON_ENTRIES];
	static bool					s_meta_button_map_initialized;
	
	// The platform dependent calls
	static Nx::CFont *			s_plat_load_font(const char *pName);
	static void					s_plat_unload_font(Nx::CFont *pFont);
};



bool ScriptLoadFont(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptUnloadFont(Script::CScriptStructure *pParams, Script::CScript *pScript);

}

#endif // FONTMAN
