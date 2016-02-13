///////////////////////////////////////////////////////////////////////////////
// NxLoadScreen.cpp

// start autoduck documentation
// @DOC NxLoadScreen
// @module NxLoadScreen | None
// @subindex Scripting Database
// @index script | NxLoadScreen

#include <core/defines.h>
#include <gfx/NxLoadScreen.h>

#include <gel/Scripting/script.h>
#include <gel/scripting/struct.h>

namespace Nx
{

bool			CLoadScreen::s_active = false;		// Set to true when loading screen is enabled
float			CLoadScreen::s_load_time;			// Amount of time it takes for loading bar to go to 100%
int				CLoadScreen::s_bar_x = 258;			// Bar position
int				CLoadScreen::s_bar_y = 400;
int				CLoadScreen::s_bar_width = 140;		// Bar size
int				CLoadScreen::s_bar_height = 8;
Image::RGBA		CLoadScreen::s_bar_start_color = Image::RGBA(0, 76, 129, 128);
Image::RGBA		CLoadScreen::s_bar_end_color = Image::RGBA(172, 211, 115, 128);
int				CLoadScreen::s_bar_border_width = 5;// Border width
int				CLoadScreen::s_bar_border_height = 5;// Border height
Image::RGBA		CLoadScreen::s_bar_border_color = Image::RGBA(40, 40, 40, 128);



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::sDisplay(const char* filename, bool just_freeze, bool blank)
{
	// Get rid of old stuff, if any
	s_clear();

	s_plat_display(filename, just_freeze, blank);

	//sStartLoadingBar(38.0f);

	s_active = true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::sStartLoadingBar(float seconds)
{
	if (seconds <= 0.0f)
	{
		return;
	}

	s_load_time = seconds;

	s_plat_update_bar_properties();			// Just in case
	s_plat_start_loading_bar(seconds);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


void CLoadScreen::sHide()
{
	// Turn off drawing
	s_plat_hide();

	// Get rid of old stuff
	s_clear();

	s_active = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::sSetLoadingBarPos(int x, int y)
{
	s_bar_x = x;
	s_bar_y = y;

	s_plat_update_bar_properties();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::sSetLoadingBarSize(int width, int height)
{
	s_bar_width = width;
	s_bar_height = height;

	s_plat_update_bar_properties();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::sSetLoadingBarStartColor(Image::RGBA color)
{
	s_bar_start_color = color;

	s_plat_update_bar_properties();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::sSetLoadingBarEndColor(Image::RGBA color)
{
	s_bar_end_color = color;

	s_plat_update_bar_properties();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::sSetLoadingBarBorder(int width, int height)
{
	s_bar_border_width = width;
	s_bar_border_height = height;

	s_plat_update_bar_properties();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::sSetLoadingBarBorderColor(Image::RGBA color)
{
	s_bar_border_color = color;

	s_plat_update_bar_properties();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::s_clear()
{
	s_plat_clear();
}

/////////////////////////////////////////////////////////////////////////////
// The script functions

// @script | SetLoadingBarPos | Sets the position of the loading bar
// @parm int | x | X position of bar
// @parm int | y | Y position of bar
bool ScriptSetLoadingBarPos(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	int x, y;
	if (!pParams->GetInteger("x", &x))
	{
		Dbg_MsgAssert(0, ("Can't find 'x' position of bar"));
	}
	if (!pParams->GetInteger("y", &y))
	{
		Dbg_MsgAssert(0, ("Can't find 'y' position of bar"));
	}

	CLoadScreen::sSetLoadingBarPos(x, y);

	return true;
}

// @script | SetLoadingBarSize | Sets the size of the loading bar
// @parm int | width | Width of bar
// @parm int | height | Heigh of bar
bool ScriptSetLoadingBarSize(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	int width, height;
	if (!pParams->GetInteger("width", &width))
	{
		Dbg_MsgAssert(0, ("Can't find 'width' of bar"));
	}
	if (!pParams->GetInteger("height", &height))
	{
		Dbg_MsgAssert(0, ("Can't find 'height' of bar"));
	}

	CLoadScreen::sSetLoadingBarSize(width, height);

	return true;
}

// @script | SetLoadingBarStartColor | Sets the color of the left side of the loading bar
// @parm int | r | Red
// @parm int | g | Green
// @parm int | b | Blue
bool ScriptSetLoadingBarStartColor(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	int r, g, b;
	if (!pParams->GetInteger("r", &r))
	{
		Dbg_MsgAssert(0, ("Can't find 'r' color"));
	}
	if (!pParams->GetInteger("g", &g))
	{
		Dbg_MsgAssert(0, ("Can't find 'g' color"));
	}
	if (!pParams->GetInteger("b", &b))
	{
		Dbg_MsgAssert(0, ("Can't find 'b' color"));
	}
	
	Image::RGBA rgb(r, g, b, 0x80);

	CLoadScreen::sSetLoadingBarStartColor(rgb);

	return true;
}

// @script | SetLoadingBarEndColor | Sets the color of the right side of the loading bar
// @parm int | r | Red
// @parm int | g | Green
// @parm int | b | Blue
bool ScriptSetLoadingBarEndColor(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	int r, g, b;
	if (!pParams->GetInteger("r", &r))
	{
		Dbg_MsgAssert(0, ("Can't find 'r' color"));
	}
	if (!pParams->GetInteger("g", &g))
	{
		Dbg_MsgAssert(0, ("Can't find 'g' color"));
	}
	if (!pParams->GetInteger("b", &b))
	{
		Dbg_MsgAssert(0, ("Can't find 'b' color"));
	}
	
	Image::RGBA rgb(r, g, b, 0x80);

	CLoadScreen::sSetLoadingBarEndColor(rgb);

	return true;
}

// @script | SetLoadingBarBorder | Sets the size of the loading bar border
// @parm int | width | Width of bar border
// @parm int | height | Heigh of bar border
bool ScriptSetLoadingBarBorder(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	int width, height;
	if (!pParams->GetInteger("width", &width))
	{
		Dbg_MsgAssert(0, ("Can't find 'width' of bar border"));
	}
	if (!pParams->GetInteger("height", &height))
	{
		Dbg_MsgAssert(0, ("Can't find 'height' of bar border"));
	}

	CLoadScreen::sSetLoadingBarBorder(width, height);

	return true;
}

// @script | SetLoadingBarBorderColor | Sets the color of the loading bar border
// @parm int | r | Red
// @parm int | g | Green
// @parm int | b | Blue
bool ScriptSetLoadingBarBorderColor(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	int r, g, b;
	if (!pParams->GetInteger("r", &r))
	{
		Dbg_MsgAssert(0, ("Can't find 'r' color"));
	}
	if (!pParams->GetInteger("g", &g))
	{
		Dbg_MsgAssert(0, ("Can't find 'g' color"));
	}
	if (!pParams->GetInteger("b", &b))
	{
		Dbg_MsgAssert(0, ("Can't find 'b' color"));
	}
	
	Image::RGBA rgb(r, g, b, 0x80);

	CLoadScreen::sSetLoadingBarBorderColor(rgb);

	return true;
}

}

