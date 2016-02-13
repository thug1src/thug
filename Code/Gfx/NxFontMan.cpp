#include <core/defines.h>
#include <core/String/stringutils.h>
#include <gfx/NxFontMan.h>
#include <gel/Scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>

namespace Nx
{


CFontManager::FontEntry 				CFontManager::s_font_tab[vMAX_FONT_ENTRIES];
Lst::HashTable<CFontManager::FontEntry>	CFontManager::s_font_lookup(4);
char									CFontManager::s_meta_button_map[NUM_META_BUTTON_ENTRIES];
bool									CFontManager::s_meta_button_map_initialized = false;


void CFontManager::sLoadFont(const char *pName, int charSpacing, int spaceSpacing, Image::RGBA *pColorTab, bool isButtonFont)
{
	Dbg_Assert(strlen(pName) < FontEntry::vMAX_NAME_SIZE);

	// See if this font is already loaded.
	if ( sGetFont( pName ) )
	{
		sGetFont( pName )->SetSpacings(charSpacing, spaceSpacing); 
		if (pColorTab) sGetFont( pName )->SetRGBATable(pColorTab); 
		sGetFont( pName )->MarkAsButtonFont(isButtonFont); 
		return;
	}

	for (int i = 0; i < vMAX_FONT_ENTRIES; i++)
	{
		if (!s_font_tab[i].mp_font)
		{
			s_font_tab[i].mp_font = s_plat_load_font(pName);
			s_font_tab[i].mp_font->SetSpacings(charSpacing, spaceSpacing);
			if (pColorTab)
				s_font_tab[i].mp_font->SetRGBATable(pColorTab);
			s_font_tab[i].mp_font->MarkAsButtonFont(isButtonFont);
						
			strcpy(s_font_tab[i].mName, pName);
			s_font_lookup.PutItem(Script::GenerateCRC(pName), &s_font_tab[i]);
			break;
		}
	}

	if (!s_meta_button_map_initialized)
	{
		#ifdef __PLAT_XBOX__
		Script::CArray *p_array = Script::GetArray("meta_button_map_xbox", Script::ASSERT);
		#else
		#ifdef __PLAT_NGC__
		Script::CArray *p_array = Script::GetArray("meta_button_map_gamecube");
		#else
		Script::CArray *p_array = Script::GetArray("meta_button_map_ps2", Script::ASSERT);
		#endif
		#endif
		
		if ( p_array )
		{
			for (uint i = 0; i < NUM_META_BUTTON_ENTRIES; i++)
			{
				int index = (i < p_array->GetSize()) ? p_array->GetInteger(i) : 0;

				if (index <= 9)
					s_meta_button_map[i] = '0' + index;
				else
					s_meta_button_map[i] = 'a' + index - 10;
			}

			s_meta_button_map_initialized = true;
		}
	}
}




void CFontManager::sUnloadFont(const char *pName)
{
	for (int i = 0; i < vMAX_FONT_ENTRIES; i++)
	{
		if (strcmp(s_font_tab[i].mName, pName) == 0)
		{
			s_font_lookup.FlushItem(Script::GenerateCRC(pName));
			
			s_plat_unload_font(s_font_tab[i].mp_font);
			s_font_tab[i].mp_font = NULL;
			
			break;
		}
	}
}


// returns pointer to font if loaded, NULL if not
Nx::CFont *CFontManager::sGetFont(uint32 checksum)
{
	FontEntry *p_entry = s_font_lookup.GetItem(checksum);
	if (p_entry)
		return p_entry->mp_font;
	else
		return NULL;
}

// returns pointer to font if loaded, NULL if not
Nx::CFont *CFontManager::sGetFont(const char *pName)
{
	return sGetFont(Script::GenerateCRC(pName));
}

// returns pointer to name if loaded, NULL if not
const char *CFontManager::sTestFontLoaded(uint32 checksum)
{
	FontEntry *p_entry = s_font_lookup.GetItem(checksum);
	if (p_entry)
		return p_entry->mName;
	else
		return NULL;
}




char CFontManager::sMapMetaCharacterToButton(const char *pMetaChar)
{
	Dbg_MsgAssert(s_meta_button_map_initialized, ("meta button character table not initialized"));
	return s_meta_button_map[Str::DehexifyDigit(pMetaChar)];
}




bool ScriptLoadFont(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	bool is_buttons_font = pParams->ContainsFlag("buttons_font");
	
	const char *p_name;
	
	
	if (!pParams->GetText(NONAME, &p_name) && !is_buttons_font)
		Dbg_MsgAssert(0, ("no font specified"));
	
	
	if (is_buttons_font)
	{
		#ifdef __PLAT_XBOX__
		p_name = "ButtonsXbox";
		#else
			#ifdef __PLAT_NGC__
			p_name = "ButtonsNgc";
			#else
			p_name = "ButtonsPs2";
			#endif
		#endif
	}

	Mem::PushMemProfile((char*)p_name);
	
	float char_spacing = 0;
	pParams->GetFloat("char_spacing", &char_spacing);
	
	float space_spacing = 0;
	pParams->GetFloat("space_spacing", &space_spacing);
	
	Script::CArray *p_script_col_tab;

   	// 15Jan02 JCB - Make this local to avoid fragmentation when allocating from the heap.
	Image::RGBA rgba_tab[16];
	Image::RGBA* p_rgba_tab = &rgba_tab[0];

	if (pParams->GetArray("color_tab", &p_script_col_tab))
	{
		for (int i = 0; i < (int) p_script_col_tab->GetSize() && i < 16; i++)
		{
			Script::CArray *p_entry = p_script_col_tab->GetArray(i);
			uint32 rgba = 0;
			int size = p_entry->GetSize();
			Dbg_MsgAssert(size >= 3 && size <= 4, ("wrong size %d for color array", size));
			for (int j = 0; j < size; j++) 
			{
				rgba |= (p_entry->GetInteger(j) & 255) << (j*8);
			}
			p_rgba_tab[i] = *((Image::RGBA *) &rgba);
		}
	}
	
	Nx::CFontManager::sLoadFont(p_name, (int) char_spacing, (int) space_spacing, p_rgba_tab, is_buttons_font);

	Mem::PopMemProfile();		

	return true;
}




bool ScriptUnloadFont(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	const char *p_name;
	if (!pParams->GetText(NONAME, &p_name))
		Dbg_MsgAssert(0, ("no font specified"));
	
	Nx::CFontManager::sUnloadFont(p_name);
	return true;
}




}

