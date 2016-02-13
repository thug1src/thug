#ifndef __GFX_NX_LOAD_SCREEN_H__
#define __GFX_NX_LOAD_SCREEN_H__

#include <gfx/image/imagebasic.h>

namespace Script
{
	class CScriptStructure;
	class CScript;
}

namespace Nx
{

class CLoadScreen
{
public:
	static void					sDisplay(const char* filename, bool just_freeze, bool blank);
	static void					sStartLoadingBar(float seconds);
	static void					sHide();
	static bool					sIsActive() { return s_active; }

	static void					sSetLoadingBarPos(int x, int y);
	static void					sSetLoadingBarSize(int width, int height);
	static void					sSetLoadingBarStartColor(Image::RGBA color);
	static void					sSetLoadingBarEndColor(Image::RGBA color);
	static void					sSetLoadingBarBorder(int width, int height);
	static void					sSetLoadingBarBorderColor(Image::RGBA color);

private:
	static void					s_clear();		// Clears out old memory (called by both Display and Hide)

	// The platform dependent calls
	static void					s_plat_display(const char *filename, bool just_freeze, bool blank);
	static void					s_plat_start_loading_bar(float seconds);
	static void					s_plat_hide();

	static void					s_plat_update_bar_properties();

	static void					s_plat_clear();

	static bool					s_active;			// Set to true when loading screen is enabled
	static float				s_load_time;		// Amount of time it takes for loading bar to go to 100%
	static int					s_bar_x;			// Bar position
	static int					s_bar_y;
	static int					s_bar_width;		// Bar size
	static int					s_bar_height;
	static Image::RGBA			s_bar_start_color;
	static Image::RGBA			s_bar_end_color;
	static int					s_bar_border_width;	// Border width
	static int					s_bar_border_height;// Border height
	static Image::RGBA			s_bar_border_color;
};

bool ScriptSetLoadingBarPos(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptSetLoadingBarSize(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptSetLoadingBarStartColor(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptSetLoadingBarEndColor(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptSetLoadingBarBorder(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptSetLoadingBarBorderColor(Script::CScriptStructure *pParams, Script::CScript *pScript);

}

#endif //__GFX_NX_LOAD_SCREEN_H__
