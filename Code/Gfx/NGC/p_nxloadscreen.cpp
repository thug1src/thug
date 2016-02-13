/////////////////////////////////////////////////////////////////////////////
// p_NxLoadScreen.cpp - PS2 platform specific interface for the load screen
//
// This is PS2 SPECIFIC!!!!!!  So might get a bit messy
//

#include	"gfx\Nx.h"
#include	"gfx\NxLoadScreen.h"
#include	"gfx\ngc\p_NxTexture.h"
#include	"gfx\ngc\p_NxSprite.h"
#include	"gfx\ngc\NX\sprite.h"
#include	"gfx\ngc\NX\render.h"
#include <sys/file/filesys.h>

#include	"core\macros.h"
#include	"sys/config/config.h"

#include <sys/ngc/p_prim.h>
#include <sys/ngc/p_camera.h>
#include <sys/ngc/p_render.h>
#include <sys/ngc/p_display.h>
#include "gfx/ngc/nx/nx_init.h"
#include	<sys/ngc\p_dvd.h>
#include <sys/File/PRE.h>

bool		gReload = false;
bool		gLoadingLoadScreen = false;
bool		gLoadingBarActive = false;
bool		gLoadingScreenActive = false;
int			gLoadBarTotalFrames;		// Number of frames it takes for loading bar to go to 100%
int			gLoadBarNumFrames;			// Number of frames so far
int			gLoadBarX = 150;			// Bar position
int			gLoadBarY = 400;
int			gLoadBarWidth = 340;		// Bar size
int			gLoadBarHeight = 20;
int			gLoadBarStartColor[4] = {  255,    0,   0,  128 };
int			gLoadBarDeltaColor[4] = { -255,  255,   0,  128 };
int			gLoadBarBorderWidth = 5;	// Border width
int			gLoadBarBorderHeight = 5;	// Border height
int			gLoadBarBorderColor[4] = {  40, 40, 40, 128 };

//static bool				s_loading_screen_on		= false;
static OSThread			s_load_icon_thread;
static OSAlarm			s_alarm;
//static LoadingIconData	s_load_icon_data		= { 0 };
//static char				s_load_icon_thread_stack[4096]	__attribute__ (( aligned( 16 )));
static volatile bool	s_terminate_thread		= false;
static volatile bool	s_terminate_thread_done	= false;
bool				s_thread_exists			= false;

extern char * g_p_buffer;

namespace	Nx
{


Nx::CNgcTexture	*sp_load_screen_texture;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void s_thread_loading_icon_alarm_handler( OSAlarm* alarm, OSContext* context )
{
	// Check the thread has not been resumed yet...
	if( OSIsThreadSuspended( &s_load_icon_thread ))
	{
		OSResumeThread( &s_load_icon_thread );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void* s_threaded_loading_icon( void* data )
{
	while( 1 )
	{
		if( s_terminate_thread )
		{
			// This thread is done...
			s_terminate_thread_done	= true;
			return NULL;
		}

		gLoadBarNumFrames++;

		bool busy = NxNgc::EngineGlobals.gpuBusy;

//		// Don't want to do draw anything whilst display lists are being built. Come back in a bit.
//		if( !busy )
		{
//			NsTexture* p_texture = p_data->m_Rasters[current_image];

//			NsDisplay::begin();
//			NsRender::begin();
//			NsPrim::begin();
//
////		    GXSetVtxAttrFmt( GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );		// 3 = positions & uvs.
////			GXSetVtxAttrFmt( GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0 );
////
//			NsCamera camera2D;
//			camera2D.orthographic( 0, 0, 640, 448 );
//
//			camera2D.begin();
//
////			NsRender::setBlendMode( NsBlendMode_None, NULL, (unsigned char)0 );
////
////			GXClearVtxDesc();
////			GXSetVtxDesc( GX_VA_POS, GX_DIRECT );
////			GXSetVtxDesc( GX_VA_TEX0, GX_DIRECT );
////			GX::SetChanMatColor( GX_COLOR0, (GXColor){ 128,128,128,128 });
////			GX::SetChanCtrl( GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
////
////			p_texture->upload( NsTexture_Wrap_Clamp );
////
////			GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
////			GXPosition3f32((float)p_data->m_X, (float)p_data->m_Y, -1.0f );
////			GXTexCoord2f32( 0.0f, 0.0f );
////			GXPosition3f32((float)p_data->m_X + (float)p_texture->m_width, (float)p_data->m_Y, -1.0f );
////			GXTexCoord2f32( 1.0f, 0.0f );
////			GXPosition3f32((float)p_data->m_X + (float)p_texture->m_width, (float)p_data->m_Y + (float)p_texture->m_height, -1.0f );
////			GXTexCoord2f32( 1.0f, 1.0f );
////			GXPosition3f32((float)p_data->m_X, (float)p_data->m_Y + (float)p_texture->m_height, -1.0f );
////			GXTexCoord2f32( 0.0f, 1.0f );
////			GXEnd();
//
//			GX::SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
//			GX::SetTevSwapMode( GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0 );
//			GX::SetNumTexGens( 0 );
//			GX::SetNumTevStages( 1 );
//			GX::SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0 );
//			GX::SetBlendMode ( GX_BM_NONE, GX_BL_ONE, GX_BL_ONE, GX_LO_CLEAR );
//			GX::SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_ENABLE, GX_TEVPREV );
//			GX::SetTevAlphaIn ( GX_TEVSTAGE0, GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO );
//			GX::SetTevColorIn ( GX_TEVSTAGE0, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO );
//
//			// Set current vertex descriptor to enable position and color0.
//			// Both use 8b index to access their data arrays.
//			GXClearVtxDesc();
//			GXSetVtxDesc( GX_VA_POS, GX_DIRECT );
//			GXSetVtxDesc( GX_VA_CLR0, GX_DIRECT );
//
//			// Set material color.
//			GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
//
//			GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//
//			GX::SetCullMode ( GX_CULL_NONE );
//
//			int cur_width = (gLoadBarWidth - 1);
//			if (gLoadBarNumFrames < gLoadBarTotalFrames)
//			{
//				cur_width = (cur_width * gLoadBarNumFrames) / gLoadBarTotalFrames;
//			}
//
//			int x1 = gLoadBarX;
//			int y1 = gLoadBarY;
//			int x2 = x1 + cur_width;
//			int y2 = y1 + (gLoadBarHeight - 1);
//
//			int end_color[4];
//			if (gLoadBarNumFrames < gLoadBarTotalFrames)
//			{
//				end_color[0] = gLoadBarStartColor[0] + ((gLoadBarDeltaColor[0] * gLoadBarNumFrames) / gLoadBarTotalFrames);
//				end_color[1] = gLoadBarStartColor[1] + ((gLoadBarDeltaColor[1] * gLoadBarNumFrames) / gLoadBarTotalFrames);
//				end_color[2] = gLoadBarStartColor[2] + ((gLoadBarDeltaColor[2] * gLoadBarNumFrames) / gLoadBarTotalFrames);
//				end_color[3] = gLoadBarStartColor[3] + ((gLoadBarDeltaColor[3] * gLoadBarNumFrames) / gLoadBarTotalFrames);
//			} else {
//				end_color[0] = gLoadBarStartColor[0] + gLoadBarDeltaColor[0];
//				end_color[1] = gLoadBarStartColor[1] + gLoadBarDeltaColor[1];
//				end_color[2] = gLoadBarStartColor[2] + gLoadBarDeltaColor[2];
//				end_color[3] = gLoadBarStartColor[3] + gLoadBarDeltaColor[3];
//			}
//
//			int border_x1 = x1 - gLoadBarBorderWidth;
//			int border_y1 = y1 - gLoadBarBorderHeight;
//			int border_x2 = x1 + (gLoadBarWidth - 1) + gLoadBarBorderWidth;
//			int border_y2 = y2 + gLoadBarBorderHeight;
//
//			u32 bc = gLoadBarBorderColor[3]|(gLoadBarBorderColor[2]<<8)|(gLoadBarBorderColor[1]<<16)|(gLoadBarBorderColor[0]<<24);
//			u32 sc = gLoadBarStartColor[3]|(gLoadBarStartColor[2]<<8)|(gLoadBarStartColor[1]<<16)|(gLoadBarStartColor[0]<<24); 
//			u32 ec = end_color[3]|(end_color[2]<<8)|(end_color[1]<<16)|(end_color[0]<<24);
//
//			// Border
//			GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
//				GXPosition3f32(border_x1, border_y1, -1.0f);
//				GXColor1u32( bc );
//				GXPosition3f32(border_x1, border_y2, -1.0f);
//				GXColor1u32( bc );
//				GXPosition3f32(border_x2, border_y2, -1.0f);
//				GXColor1u32( bc );
//				GXPosition3f32(border_x2, border_y1, -1.0f);
//				GXColor1u32( bc );
//			GXEnd();
//			
//			// Bar
//			GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
//				GXPosition3f32(x1, y1, -1.0f);
//				GXColor1u32( sc );
//				GXPosition3f32(x1, y2, -1.0f);
//				GXColor1u32( sc );
//				GXPosition3f32(x2, y2, -1.0f);
//				GXColor1u32( ec );
//				GXPosition3f32(x2, y1, -1.0f);
//				GXColor1u32( ec );
//			GXEnd();
//
//			camera2D.end();
//
//			NsPrim::end();
//			NsRender::end();
//			NsDisplay::end( false );
//
////			current_image = ( current_image + 1 ) % p_data->m_NumFrames ;
		}

		// Go to sleep.
		if( busy )
		{
			// Come back shortly.
			OSSetAlarm( &s_alarm, OSMillisecondsToTicks( 500 ), s_thread_loading_icon_alarm_handler );
		}
		else
		{
			// Come back in the proper time.
			OSSetAlarm( &s_alarm, OSMillisecondsToTicks( (long long int)( 1000.0f / 60.0f ) ), s_thread_loading_icon_alarm_handler );
		}

		OSSuspendThread( &s_load_icon_thread );
	}
}


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
	// See if we need to just flag that a loading screen is active to disable draws.
	if ( just_freeze )
	{
		gLoadingScreenActive = true;
		return;
	}

	// See if we need to clear the screen to black.
	if ( blank )
	{
		for ( int lp = 0; lp < 2; lp++ )
		{
			NsDisplay::begin();
			NsRender::begin();
			NsRender::end();
			NsDisplay::end();
		}

		gLoadingScreenActive = true;
		return;
	}

	NxNgc::EngineGlobals.gpuBusy = true;

	char name[256];
	char pre[256];

	File::PreMgr* pre_mgr = File::PreMgr::Instance();

	// Hmm seems we need to prepend 'images\\' to the start of this string...
	strcpy( name, "images\\" );
	switch( Config::GetLanguage())
	{
		case Config::LANGUAGE_GERMAN:
			strcpy( pre, filename );
			strcat( pre, ".prd" );
			pre_mgr->LoadPre( pre, false );
			strcat( name, "grimages_ngc\\" );
			break;
		case Config::LANGUAGE_FRENCH:
			strcpy( pre, filename );
			strcat( pre, ".prf" );
			pre_mgr->LoadPre( pre, false );
			strcat( name, "frimages_ngc\\" );
			break;
		default:
			break;
	}
	strcat( name, filename );
	
	// ...and append '.img.ngc' to the end.
	strcat( name, ".img.ngc" );
	
	// Load the texture a few lines at a time & render to the display.
	gLoadingLoadScreen = true;
	void *p_FH = File::Open( name, "rb" );

	if( p_FH )
	{
		uint32 version;
		uint32 checksum;
		uint32 width;
		uint32 height;
		uint32 depth;
		uint32 levels;
		uint32 rwidth;
		uint32 rheight;
		uint16 alphamap;
		uint16 hasholes;

		// Load the actual texture

		File::Read( &version,  4, 1, p_FH );
		File::Read( &checksum, 4, 1, p_FH );
		File::Read( &width,    4, 1, p_FH );
		File::Read( &height,   4, 1, p_FH );
		File::Read( &depth,    4, 1, p_FH );
		File::Read( &levels,   4, 1, p_FH );
		File::Read( &rwidth,   4, 1, p_FH );
		File::Read( &rheight,  4, 1, p_FH );
		File::Read( &alphamap, 2, 1, p_FH );
		File::Read( &hasholes, 2, 1, p_FH );

//		Dbg_MsgAssert( version <= TEXTURE_VERSION, ("Illegal texture version number: %d (The newest version we deal with is: %d)\n", version, TEXTURE_VERSION ));
		Dbg_MsgAssert( depth == 32, ("We only deal with 32-bit textures.\n"));

#define LINES 16

		// Load in 8 line chunks.
//		uint16 temp_buffer[width*LINES*2];
//		uint32 tex_buffer[width*LINES];

		uint16 * temp_buffer = (uint16 *)g_p_buffer;
		uint32 * tex_buffer = (uint32 *)&temp_buffer[width*LINES*2];

		NsDisplay::begin();
		NsRender::begin();

		GX::PokeColorUpdate( GX_TRUE );
		GX::PokeAlphaUpdate( GX_FALSE );
		uint yy = 0;
		while ( yy < height )
		{
			int lines = ( height - yy ) > LINES ? LINES : ( height - yy );

			File::Read( temp_buffer, width * 4 * lines, 1, p_FH );

			if ( gReload )
			{
				gReload = false;
				yy = 0;
				File::Close( p_FH );
				void *p_FH = File::Open( name, "rb" );
				File::Read( &version,  4, 1, p_FH );
				File::Read( &checksum, 4, 1, p_FH );
				File::Read( &width,    4, 1, p_FH );
				File::Read( &height,   4, 1, p_FH );
				File::Read( &depth,    4, 1, p_FH );
				File::Read( &levels,   4, 1, p_FH );
				File::Read( &rwidth,   4, 1, p_FH );
				File::Read( &rheight,  4, 1, p_FH );
				File::Read( &alphamap, 2, 1, p_FH );
				File::Read( &hasholes, 2, 1, p_FH );
				NsDisplay::begin();
				NsRender::begin();
				continue;
			}

			// Need to swizzle data we just read.
			for ( int lp = 0; lp < lines; lp += 4 )
			{
				int xx = lp * width;
				for ( uint lp2 = 0; lp2 < width; lp2 += 4 )
				{
					for ( int lp3 = 0; lp3 < 16; lp3++ )
					{
						uint16 p0 = temp_buffer[lp3+(lp2*8)+(xx*2)];
						uint16 p1 = temp_buffer[16+lp3+(lp2*8)+(xx*2)];
						tex_buffer[(lp3&3)+(width*(lp3>>2))+lp2+xx] =	( ( p0 << 16 ) & 0xff000000 ) |
																		( ( p0 << 16 ) & 0x00ff0000 ) | 
																		( ( p1 <<  0 ) & 0x0000ff00 ) | 
																		( ( p1 <<  0 ) & 0x000000ff );
					}
				}
			}

			for ( int yp = 0; yp < lines; yp++ )
			{
				uint y_pos = ( ( height - 1 ) - ( yy + yp ) );
				for ( uint xp = 0; xp < width; xp++ )
				{
					GX::PokeARGB( xp, y_pos, tex_buffer[(width*yp)+xp] );
				}
			}

			yy += lines;
		}
		NsRender::end();
		NsDisplay::end(false);

		File::Close( p_FH );
	}
	gLoadingLoadScreen = false;

	switch( Config::GetLanguage())
	{
		case Config::LANGUAGE_GERMAN:
		case Config::LANGUAGE_FRENCH:
			pre_mgr->UnloadPre( pre );
			break;
		default:
			break;
	}
//	NsDisplay::flush();

	NxNgc::EngineGlobals.gpuBusy = false;

	gLoadingScreenActive = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::s_plat_hide()
{
	if( s_thread_exists )
	{
		s_terminate_thread_done	= false;
		s_terminate_thread = true;

		// Cancel the alarm and resume the thread.
//		OSCancelAlarm( &s_alarm );
//		OSResumeThread( &s_load_icon_thread );

		while( !s_terminate_thread_done );

		s_thread_exists = false;
//		pIconCallback = NULL;

		//VISetBlack( TRUE );
		//VIFlush();
		//VIWaitForRetrace();
		//NsDisplay::flush();
		//VISetBlack( FALSE );
		//VIFlush();
		//VIWaitForRetrace();
	}

//#if !USE_SPRITES
//	NxPs2::EnableFlipCopy(true);
//#endif // USE_SPRITES
	gLoadingBarActive = false;
	gLoadingScreenActive = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::s_plat_clear()
{
//	if (sp_load_screen_sprite)
//	{
//		CEngine::sDestroySprite(sp_load_screen_sprite);
//		sp_load_screen_sprite = NULL;
//	}
//
//	if (sp_load_screen_texture)
//	{
//		delete sp_load_screen_texture;
//		sp_load_screen_texture = NULL;
//	}
	gLoadingBarActive = false;
	gLoadingScreenActive = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::s_plat_start_loading_bar(float seconds)
{
//	NsDisplay::flush();

	gLoadBarTotalFrames = (int) (seconds * 1.15f * Config::FPS());
	gLoadBarNumFrames = 0;

//
////	NxPs2::StartLoadingBar((int) (seconds * Config::FPS()));
//
//	// Start the thread to display the animated loading icon.
////	pIconCallback = s_non_threaded_loading_icon;
//	BOOL rv = true;
//
//	s_terminate_thread = false;
//
//	memset( &s_load_icon_thread, 0, sizeof( OSThread ));
//	rv = OSCreateThread(	&s_load_icon_thread,
//							s_threaded_loading_icon,										// Entry function.
//							NULL,															// Argument for start function.
//							s_load_icon_thread_stack + sizeof( s_load_icon_thread_stack ),	// Stack base (stack grows down).
//							sizeof( s_load_icon_thread_stack ),								// Stack size in bytes.
//							1,																// Thread priority.
//							OS_THREAD_ATTR_DETACH  );										// Thread attributes.
//	Dbg_MsgAssert( rv, ( "Failed to create thread" ));
//
//	if( rv )
//	{
//		s_thread_exists = true;
////		OSResumeThread( &s_load_icon_thread );
//		OSSetAlarm( &s_alarm, OSMillisecondsToTicks( (long long int)( 1000.0f / 60.0f ) ), s_thread_loading_icon_alarm_handler );
//	}
	gLoadingBarActive = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CLoadScreen::s_plat_update_bar_properties()
{
	// Bar size and position
	gLoadBarX = s_bar_x;
	gLoadBarY = s_bar_y;
	gLoadBarWidth = s_bar_width;
	gLoadBarHeight = s_bar_height;

	// Bar colors
	gLoadBarStartColor[0] = s_bar_start_color.r;
	gLoadBarStartColor[1] = s_bar_start_color.g;
	gLoadBarStartColor[2] = s_bar_start_color.b;
	gLoadBarStartColor[3] = s_bar_start_color.a;
	gLoadBarDeltaColor[0] = (int) s_bar_end_color.r - gLoadBarStartColor[0];
	gLoadBarDeltaColor[1] = (int) s_bar_end_color.g - gLoadBarStartColor[1];
	gLoadBarDeltaColor[2] = (int) s_bar_end_color.b - gLoadBarStartColor[2];
	gLoadBarDeltaColor[3] = (int) s_bar_end_color.a - gLoadBarStartColor[3];

	// Border size
	gLoadBarBorderWidth = s_bar_border_width;
	gLoadBarBorderHeight = s_bar_border_height;

	// Border color
	gLoadBarBorderColor[0] = s_bar_border_color.r;
	gLoadBarBorderColor[1] = s_bar_border_color.g;
	gLoadBarBorderColor[2] = s_bar_border_color.b;
	gLoadBarBorderColor[3] = s_bar_border_color.a;
}

} 
 



