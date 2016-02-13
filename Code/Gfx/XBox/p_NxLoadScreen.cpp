/////////////////////////////////////////////////////////////////////////////
// p_NxLoadScreen.cpp - Xbox platform specific interface for the load screen
//
//

#include	"gfx\Nx.h"
#include	"gfx\NxLoadScreen.h"
#include	"gfx\xbox\p_NxTexture.h"
#include	"gfx\xbox\p_NxSprite.h"
#include	"gfx\xbox\NX\sprite.h"
#include	"gfx\xbox\NX\scene.h"
#include	"gfx\xbox\NX\render.h"
#include	"sys\config\config.h"

#include	"core\macros.h"

namespace	Nx
{


Nx::CXboxTexture	*sp_load_screen_texture;
Nx::CXboxSprite	*sp_load_screen_sprite;

static float		loadingBarTotalSeconds;
static float		loadingBarCurrentSeconds;
static float		loadingBarDeltaSeconds;
static int			loadingBarStartColor[3];		// r,g,b
static int			loadingBarEndColor[3];			// r,g,b
static uint32		loadingBarColors[1280][3];		// r,g,b
static bool			loadingBarColorsSet = false;
static int			loadingBarWidth;
static uint32		loadingBarBorderColor;


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// Functions

#define USE_SPRITES 0

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CALLBACK loadingBarTimerCallback( UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2 )
{
	if( NxXbox::EngineGlobals.loadingbar_timer_event != 0 )
	{
		loadingBarCurrentSeconds += loadingBarDeltaSeconds;
		float mult			= loadingBarCurrentSeconds / loadingBarTotalSeconds;
		mult				= ( mult > 1.0f ) ? 1.0f : mult;
		int bar_width		= (int)( loadingBarWidth * mult );
		
		// Get pointer to front buffer memory.
		IDirect3DSurface8	*p_buffer;
		D3DLOCKED_RECT		locked_rect;
		NxXbox::EngineGlobals.p_Device->GetBackBuffer( -1, D3DBACKBUFFER_TYPE_MONO, &p_buffer );
		p_buffer->LockRect( &locked_rect, NULL, D3DLOCK_TILED );
		uint32 *p_screen = (uint32*)locked_rect.pBits;

		// ----------------------------------
		// | ||||||||||||||||||||||          |
		// ----------------------------------
		// ^ ^                    ^          ^
		// a b                    c			 d
		// 
		// a = surround start
		// b = bar start
		// c = bar end
		// d = surround end
		int bar_start			= ( 640 - loadingBarWidth ) / 2;
		int surround_start		= bar_start - 5;
		int bar_end				= bar_start + bar_width;
		int surround_end		= surround_start + loadingBarWidth + 10;

		const int HDTV_OFFSET	= 48;	
		
		bar_start				= (int)SCREEN_CONV_X( bar_start );
		surround_start			= (int)SCREEN_CONV_X( surround_start );
		bar_end					= (int)SCREEN_CONV_X( bar_end );
		surround_end			= (int)SCREEN_CONV_X( surround_end );
		
		if( NxXbox::EngineGlobals.backbuffer_width > 640 )
		{
			bar_start			+= HDTV_OFFSET;
			surround_start		+= HDTV_OFFSET;
			bar_end				+= HDTV_OFFSET;
			surround_end		+= HDTV_OFFSET;
		}
		
		int base_y				= (int)SCREEN_CONV_Y( 410 );
		
		for( int i = 0; i < 20; ++i )
		{
			uint32 *p_loop = p_screen + (( base_y + i ) * NxXbox::EngineGlobals.backbuffer_width );

			if(( i < 5 ) || ( i >= 15 ))
			{
				for( int j = surround_start; j < surround_end; ++j )
					p_loop[j] = loadingBarBorderColor;
			}
			else
			{
				for( int j = surround_start; j < ( surround_start + 5 ); ++j )
					p_loop[j] = loadingBarBorderColor;

				for( int j = bar_end; j < surround_end; ++j )
					p_loop[j] = loadingBarBorderColor;

				for( int j = bar_start; j < bar_end; ++j )			
				{
					uint32 idx			= ( j - bar_start >= 1279 ) ? 1279 : ( j - bar_start );
					uint32 write_value	= 0x80000000UL | ( loadingBarColors[idx][0] << 16 ) | ( loadingBarColors[idx][1] << 8 ) | ( loadingBarColors[idx][2] << 0 );
					p_loop[j]			= write_value;
				}
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static bool is_power_of_two( uint32 a )
{
	if( a == 0 )
	{
		return false;
	}
	return (( a & ( a - 1 )) == 0 );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CLoadScreen::s_plat_display(const char* filename, bool just_freeze, bool blank)
{
	// Wait for asyncronous rendering to finish.
	NxXbox::EngineGlobals.p_Device->BlockUntilIdle();

	if( !just_freeze )
	{
		if( blank )
		{
			D3DDevice_Clear( 0, NULL, D3DCLEAR_TARGET, 0x00000000UL, 1.0f, 0 );
			D3DDevice_Swap( D3DSWAP_DEFAULT );
		}
		else
		{
			// Engine stuff
			Dbg_Assert(!sp_load_screen_texture);
			Dbg_Assert(!sp_load_screen_sprite);

			sp_load_screen_texture = new CXboxTexture();

#			ifdef __PAL_BUILD__
			switch( Config::GetLanguage())
			{
				case Config::LANGUAGE_FRENCH:
				{
					char t_filename[200];
					sprintf( t_filename, "PALImages/FRImages/%s", filename );
					if( !sp_load_screen_texture->LoadTexture( t_filename, true ))
					{
						Dbg_Error( "Can't load texture %s", t_filename );
						return;
					}
					break;
				}
				case Config::LANGUAGE_GERMAN:
				{
					char t_filename[200];
					sprintf( t_filename, "PALImages/GRImages/%s", filename );
					if( !sp_load_screen_texture->LoadTexture( t_filename, true ))
					{
						Dbg_Error( "Can't load texture %s", t_filename );
						return;
					}
					break;
				}
				default:
				{
					if( !sp_load_screen_texture->LoadTexture( filename, true ))
					{
						Dbg_Error( "Can't load texture %s", filename );
						return;
					}
					break;
				}
			}
#			else
			if( !sp_load_screen_texture->LoadTexture( filename, true ))
			{
				Dbg_Error( "Can't load texture %s", filename );
				return;
			}
#			endif // __PAL_BUILD__

			// Copy into frame buffer.
			float x_offset		= 0.0f;
			float y_offset		= 0.0f;
			float x_scale		= 1.0f;
			float y_scale		= 1.0f;
			float alpha_level	= 1.0f;
	
			D3DDevice_SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
			D3DDevice_SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
			D3DDevice_SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			D3DDevice_SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );

			// Store the stage zero minfilter, since it may be anisotropic.
			DWORD	stage_zero_minfilter;
			D3DDevice_GetTextureStageState( 0, D3DTSS_MINFILTER, &stage_zero_minfilter );

			// Turn on texture filtering when scaling...
			D3DDevice_SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
			D3DDevice_SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );

			// Turn on clamping so that the linear textures work
			NxXbox::set_render_state( RS_UVADDRESSMODE0, 0x00010001UL );

			D3DDevice_SetRenderState( D3DRS_LIGHTING,				FALSE );
	
			// Use a default vertex shader
			NxXbox::set_pixel_shader( 0 );
			NxXbox::set_vertex_shader( D3DFVF_XYZRHW | D3DFVF_TEX1 );
			NxXbox::set_blend_mode( NxXbox::vBLEND_MODE_DIFFUSE );

			// Select the texture (flush first, since the screen texture is linear).
			NxXbox::set_texture( 0, NULL );
			NxXbox::set_texture( 0, sp_load_screen_texture->GetEngineTexture()->pD3DTexture, sp_load_screen_texture->GetEngineTexture()->pD3DPalette );

			// Setup up the vertices.
			typedef struct
			{
				float	sx,sy,sz;
				float	rhw;
				float	tu,tv;
			}
			LOADSCREEN_VERT;
	
			// Get the width height from the texture itself, since we may be using a texture not designed especially
			// for this screen dimension.
			float tex_w = (float)sp_load_screen_texture->GetEngineTexture()->ActualWidth;
			float tex_h = (float)sp_load_screen_texture->GetEngineTexture()->ActualHeight;
			float scr_w = (float)NxXbox::EngineGlobals.backbuffer_width;
			float scr_h = (float)NxXbox::EngineGlobals.backbuffer_height;

			LOADSCREEN_VERT	vertices[4];

			// The texture coordinate addressing will differ depending on whether this is a linear texture or not.
			if( is_power_of_two( sp_load_screen_texture->GetEngineTexture()->ActualWidth ) &&
				is_power_of_two( sp_load_screen_texture->GetEngineTexture()->ActualHeight ))
			{
				// Not a linear texture, will be swizzled, max uv is (1.0, 1.0).
				vertices[0].sx		= x_offset;
				vertices[0].sy		= y_offset;
				vertices[0].sz		= 0.0f;
				vertices[0].rhw		= 0.0f;
				vertices[0].tu		= 0.0f;
				vertices[0].tv		= 1.0f;
				vertices[1]			= vertices[0];
				vertices[1].sy		= y_offset + ( scr_h * y_scale );
				vertices[1].tv		= 0.0f;
				vertices[2]			= vertices[0];
				vertices[2].sx		= x_offset + ( scr_w * x_scale );
				vertices[2].tu		= 1.0f;
				vertices[3]			= vertices[2];
				vertices[3].sy		= vertices[1].sy;
				vertices[3].tv		= 0.0f;
			}
			else
			{
				// Linear texture, won't be swizzled, max uv is (tex_w, tex_h).
				vertices[0].sx		= x_offset;
				vertices[0].sy		= y_offset;
				vertices[0].sz		= 0.0f;
				vertices[0].rhw		= 0.0f;
				vertices[0].tu		= -0.5f;
				vertices[0].tv		= tex_h - 0.5f;
				vertices[1]			= vertices[0];
				vertices[1].sy		= y_offset + ( scr_h * y_scale );
				vertices[1].tv		= -0.5f;
				vertices[2]			= vertices[0];
				vertices[2].sx		= x_offset + ( scr_w * x_scale );
				vertices[2].tu		= tex_w - 0.5f;
				vertices[3]			= vertices[2];
				vertices[3].sy		= vertices[1].sy;
				vertices[3].tv		= -0.5f;
			}

			// Draw the vertices, and make sure they're displayed.
			D3DDevice_DrawVerticesUP( D3DPT_TRIANGLESTRIP, 4, vertices, sizeof( LOADSCREEN_VERT ));
			D3DDevice_Swap( D3DSWAP_DEFAULT );

			// Done with texture
			delete sp_load_screen_texture;
			sp_load_screen_texture = NULL;

			// Reflush linear texture out.
			NxXbox::set_texture( 0, NULL );

			// Restore the stage zero minfilter.
			D3DDevice_SetTextureStageState( 0, D3DTSS_MINFILTER, stage_zero_minfilter );
		}
	}

	// Indicate that the loading screen is visible, to stop any more rendering until it is hidden.
	NxXbox::EngineGlobals.loadingscreen_visible = true;

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::s_plat_hide()
{
	// Remove the loading bar.
	if( NxXbox::EngineGlobals.loadingbar_timer_event != 0 )
	{
		timeKillEvent( NxXbox::EngineGlobals.loadingbar_timer_event );
		NxXbox::EngineGlobals.loadingbar_timer_event = 0;
	}

	// Indicate that the loading screen is no longer visible.
	NxXbox::EngineGlobals.loadingscreen_visible = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::s_plat_clear()
{
	if( sp_load_screen_sprite )
	{
		CEngine::sDestroySprite(sp_load_screen_sprite);
		sp_load_screen_sprite = NULL;
	}

	if( sp_load_screen_texture )
	{
		delete sp_load_screen_texture;
		sp_load_screen_texture = NULL;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CLoadScreen::s_plat_start_loading_bar( float seconds )
{
	loadingBarTotalSeconds		= seconds * 0.6f;
	loadingBarCurrentSeconds	= 0.0f;
	loadingBarDeltaSeconds		= 0.03f;	// 30 milliseconds.
	
	s_plat_update_bar_properties();

	// Set up the timer event.
	if( NxXbox::EngineGlobals.loadingbar_timer_event == 0 )
	{
		NxXbox::EngineGlobals.loadingbar_timer_event = timeSetEvent( (uint32)( loadingBarDeltaSeconds * 1000.0f ),	// Delay (ms).
																	 0,												// Ignored resolution (ms).
																	 loadingBarTimerCallback,						// Callback function.
																	 0,												// Callback data.
																	 TIME_PERIODIC | TIME_CALLBACK_FUNCTION );
		Dbg_Assert( NxXbox::EngineGlobals.loadingbar_timer_event != 0 );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CLoadScreen::s_plat_update_bar_properties( void )
{

	loadingBarStartColor[0] = s_bar_start_color.r;
	loadingBarStartColor[1] = s_bar_start_color.g;
	loadingBarStartColor[2] = s_bar_start_color.b;

	loadingBarEndColor[0]	= s_bar_end_color.r;
	loadingBarEndColor[1]	= s_bar_end_color.g;
	loadingBarEndColor[2]	= s_bar_end_color.b;

	loadingBarWidth			= s_bar_width;
	loadingBarBorderColor	= 0x80000000 | ((uint32)s_bar_border_color.r << 16 ) | ((uint32)s_bar_border_color.g << 8 )  | (uint32)s_bar_border_color.b;

	// Build the interpolated color array.
	int last_color = (int)( loadingBarWidth * NxXbox::EngineGlobals.screen_conv_x_multiplier );
	for( int i = 0; i < last_color; ++i )
	{
		for( int c = 0; c < 3; ++c )
		{
			loadingBarColors[i][c] = loadingBarStartColor[c] + ((( loadingBarEndColor[c] - loadingBarStartColor[c] ) * i ) / last_color );
		}
	}
}


} 
 
