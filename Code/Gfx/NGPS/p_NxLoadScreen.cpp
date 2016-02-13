/////////////////////////////////////////////////////////////////////////////
// p_NxLoadScreen.cpp - PS2 platform specific interface for the load screen
//
// This is PS2 SPECIFIC!!!!!!  So might get a bit messy
//

#include	"gfx\Nx.h"
#include	"gfx\NxLoadScreen.h"
#include	"gfx\NGPS\p_NxTexture.h"
#include	"gfx\NGPS\p_NxSprite.h"
#include	"gfx\NGPS\NX\sprite.h"
#include	"gfx\NGPS\NX\render.h"
#include	"gfx\NGPS\NX\loadscreen.h"
#include	"gfx\NGPS\NX\dma.h"
#include	"gfx\NGPS\NX\dmacalls.h"
#include	"gfx\NGPS\NX\nx_init.h"

#include	"gel\movies\NGPS\disp.h"  // for clear screen

#include	"core\macros.h"
#include <sys/config/config.h>
#include <sys/file/filesys.h>
#include <sys/timer.h>

#include <libgraph.h>

namespace NxPs2
{
	void	WaitForRendering();
}


namespace	Nx
{


Nx::CPs2Texture	*sp_load_screen_texture;
Nx::CPs2Sprite	*sp_load_screen_sprite;

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// Functions

#define USE_SPRITES 0

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::s_plat_display(const char* filename, bool just_freeze, bool blank)
{
	// Engine stuff
	Dbg_Assert(!sp_load_screen_texture);
	Dbg_Assert(!sp_load_screen_sprite);

	char adjustedFilename[144];

	if (!just_freeze && !blank)
	{
		sp_load_screen_texture = new CPs2Texture(true);

		if (Config::NTSC())
		{
			strcpy(adjustedFilename, filename);
		} else {
			// Find correct directory to use based on language
			switch (Config::GetLanguage())
			{
			case Config::LANGUAGE_FRENCH:
				sprintf(adjustedFilename, "PALimages/FRImages/%s", filename);
				break;
			case Config::LANGUAGE_SPANISH:
				sprintf(adjustedFilename, "PALimages/SPImages/%s", filename);
				break;
			case Config::LANGUAGE_GERMAN:
				sprintf(adjustedFilename, "PALimages/GRImages/%s", filename);
				break;
			case Config::LANGUAGE_ITALIAN:
				sprintf(adjustedFilename, "PALimages/ITImages/%s", filename);
				break;
			default:
				sprintf(adjustedFilename, "PALimages/%s", filename);
				break;
			}

			// Since LoadTexture() will assert if the file doesn't exist, we need to test ahead of time
			char ExtendedFilename[160];
			sprintf(ExtendedFilename, "images/%s.img.ps2", adjustedFilename);

			if (!File::Exist(ExtendedFilename))
			{
				// Can't find it, so lets try the base directory before giving up
				sprintf(adjustedFilename, "PALimages/%s", filename);
			}
		}

		if (!sp_load_screen_texture->LoadTexture(adjustedFilename, true))
		{
			Dbg_Error("Can't load texture %s", adjustedFilename);
		}
	
	#if USE_SPRITES
		sp_load_screen_sprite = static_cast<Nx::CPs2Sprite *>(CEngine::sCreateSprite());
		sp_load_screen_sprite->SetTexture(sp_load_screen_texture);
		sp_load_screen_sprite->SetPos(0, 0);
		sp_load_screen_sprite->SetScale(1, 1);
		sp_load_screen_sprite->SetRGBA(Image::RGBA(128, 128, 128, 128));
		sp_load_screen_sprite->SetHidden(false);
	#else
		// Copy directly into display buffer
		sceGsLoadImage gs_image;
		NxPs2::SSingleTexture *p_engine_texture = sp_load_screen_texture->GetSingleTexture();
	
		FlushCache( 0 );
		sceGsSyncPath( 0, 0 );
		
		int display_addr = (SCREEN_CONV_X( 640 ) * SCREEN_CONV_Y( 448 ) * 4) / (64 * 4);
	
		const int download_parts = 2;
		int partial_height = sp_load_screen_texture->GetHeight() / download_parts;
		int partial_size = sp_load_screen_texture->GetWidth() * partial_height * 2;
		for (int i = 0; i < download_parts; i++)
		{
			sceGsSetDefLoadImage( &gs_image, display_addr, SCREEN_CONV_X( 640 )/64, SCE_GS_PSMCT16S,
							   0, partial_height * i, sp_load_screen_texture->GetWidth(), partial_height );
					
			FlushCache( 0 );		
			sceGsExecLoadImage( &gs_image, ( u_long128 * )(p_engine_texture->mp_PixelData + (partial_size * i)) );
			sceGsSyncPath( 0, 0 );
		}
	
		// Done with texture
		delete sp_load_screen_texture;
		sp_load_screen_texture = NULL;
	}
	
	if (blank)
	{
		// This is esentially the same as playing a movie, without playing a movie
		// the ResetEngine handles clearing the screen 
		NxPs2::SuspendEngine();
		NxPs2::ResetEngine();		
	}
	
	
	
	NxPs2::EnableFlipCopy(false);
#endif // USE_SPRITES
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::s_plat_start_loading_bar(float seconds)
{
	NxPs2::StartLoadingBar((int) (seconds * Config::FPS()));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::s_plat_hide()
{
	NxPs2::RemoveLoadingBar();
	//Tmr::VSync();
#if !USE_SPRITES
	NxPs2::EnableFlipCopy(true);
#endif // USE_SPRITES
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::s_plat_update_bar_properties()
{
	// Bar size and position
	NxPs2::gLoadBarX = s_bar_x;
	NxPs2::gLoadBarY = s_bar_y;
	NxPs2::gLoadBarWidth = s_bar_width;
	NxPs2::gLoadBarHeight = s_bar_height;

	// Bar colors
	NxPs2::gLoadBarStartColor[0] = s_bar_start_color.r;
	NxPs2::gLoadBarStartColor[1] = s_bar_start_color.g;
	NxPs2::gLoadBarStartColor[2] = s_bar_start_color.b;
	NxPs2::gLoadBarStartColor[3] = s_bar_start_color.a;
	NxPs2::gLoadBarDeltaColor[0] = (int) s_bar_end_color.r - NxPs2::gLoadBarStartColor[0];
	NxPs2::gLoadBarDeltaColor[1] = (int) s_bar_end_color.g - NxPs2::gLoadBarStartColor[1];
	NxPs2::gLoadBarDeltaColor[2] = (int) s_bar_end_color.b - NxPs2::gLoadBarStartColor[2];
	NxPs2::gLoadBarDeltaColor[3] = (int) s_bar_end_color.a - NxPs2::gLoadBarStartColor[3];

	// Border size
	NxPs2::gLoadBarBorderWidth = s_bar_border_width;
	NxPs2::gLoadBarBorderHeight = s_bar_border_height;

	// Border color
	NxPs2::gLoadBarBorderColor[0] = s_bar_border_color.r;
	NxPs2::gLoadBarBorderColor[1] = s_bar_border_color.g;
	NxPs2::gLoadBarBorderColor[2] = s_bar_border_color.b;
	NxPs2::gLoadBarBorderColor[3] = s_bar_border_color.a;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::s_plat_clear()
{
	if (sp_load_screen_sprite)
	{
		CEngine::sDestroySprite(sp_load_screen_sprite);
		sp_load_screen_sprite = NULL;
	}

	if (sp_load_screen_texture)
	{
		delete sp_load_screen_texture;
		sp_load_screen_texture = NULL;
	}

	NxPs2::RemoveLoadingBar();
}

} 
 
