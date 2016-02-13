/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Gfx														**
**																			**
**	Module:			LoadScreen			 									**
**																			**
**	File name:		p_loadscreen.cpp										**
**																			**
**	Created by:		05/08/02	-	SPG										**
**																			**
**	Description:	XBox-specific loading screen calls						**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <xtl.h>
#include <core/defines.h>
#include <sys/file/filesys.h>
#include <sys/timer.h>
#include <gel/mainloop.h>
#include <gfx/gfxman.h>
#include <gfx/nxviewport.h>
#include <gel/scripting/script.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace LoadScreen
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

// Skate3 code, disabled for now.
#if 0
extern "C" RwBool _rwXbSetRenderState(RwRenderState nState, void *pParam);
extern "C" RwBool _rwXbGetRenderState(RwRenderState nState, void *pParam);
extern "C" RwBool _rwXbRenderStateTextureRaster(RwRaster *raster);
#endif

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

class LoadingIconData
{
public:
	int	m_X;
	int	m_Y;
	int m_FrameDelay;
//	Skate3 code, disabled for now.
#	if 0
	RwRaster** m_Rasters;
#	endif
	int m_NumFrames;
};

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

static bool				s_loading_screen_on		= false;
static MMRESULT			s_timer_event			= 0;
static LoadingIconData	s_load_icon_data		= { 0 };


/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/
void CALLBACK loadIconTimerCallback( UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2 )
{
	static int current_image = 0;

//	Skate3 code, disabled for now.
#	if 0
	
	// Here is where the load icon will be written to the screen.
	RwRect rect;
	rect.x = s_load_icon_data.m_X;
	rect.y = s_load_icon_data.m_Y;

	rect.w = SCREEN_CONV_X( 32.0f ) - SCREEN_CONV_X( 0.0f );
	rect.h = SCREEN_CONV_Y( 32.0f ) - SCREEN_CONV_Y( 0.0f );

	Spt::SingletonPtr< Gfx::Manager > gfx_manager;

	_rwXbSetRenderState( rwRENDERSTATETEXTURERASTER, (void *)NULL );

	RwRaster* p_context_raster;
	p_context_raster = gfx_manager->DefaultViewport().GetFrameBuffer();

	RwRasterPushContext( p_context_raster );

	RwRasterRenderScaled( s_load_icon_data.m_Rasters[current_image], &rect );
	RwRasterShowRaster( p_context_raster, NULL, 0 );

	RwRasterPopContext();

#	endif
	
	current_image = ( current_image + 1 ) % s_load_icon_data.m_NumFrames ;
}



/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void SetLoadingIconProperties( int x, int y, int frame_delay, int num_frames, char* basename, char* ext )
{
	int i;

//	Skate3 code, disabled for now.
#	if 0

	if( s_load_icon_data.m_NumFrames > 0  )
	{   
		for( i = 0; i < s_load_icon_data.m_NumFrames; i++ )
		{
			RwRasterDestroy( s_load_icon_data.m_Rasters[i] );
		}
		delete [] s_load_icon_data.m_Rasters;
	}

	s_load_icon_data.m_X			= SCREEN_CONV_X( x );
	s_load_icon_data.m_Y			= SCREEN_CONV_Y( y );
	s_load_icon_data.m_FrameDelay = frame_delay;
	s_load_icon_data.m_NumFrames = num_frames;
	s_load_icon_data.m_Rasters = new RwRaster*[s_load_icon_data.m_NumFrames];

	RwImageSetPath("");
	for( i = 0; i < s_load_icon_data.m_NumFrames; i++ )
	{
		char image_name[64];
		//sprintf( image_name, "Images/PanelSprites/wheel_0%d.png", i+1 );
		sprintf( image_name, "%s%d.%s", basename, i+1, ext );
		s_load_icon_data.m_Rasters[i] = RwRasterRead( image_name );
	}

#	endif
}

void Display( char* filename, bool just_freeze, bool blank )
{
	// Redundancy Check
	if( s_loading_screen_on )
	{
		return;
	}
    
	// Set filename up for foreign languages.
	char name[256];

	// Seems that filename may sometimes be NULL.
	if( filename )
	{
		char * p8 = &filename[strlen(filename)];

		if( IsEnglish())
		{
			strcpy ( name, filename );
		}
		else
		{
			switch ( XGetLanguage() ) {
				case XC_LANGUAGE_FRENCH:
					while ( ( *p8 != '\\' ) && ( *p8 != '/' ) ) p8--;
					sprintf( name, "images\\frimages\\%s", &p8[1] );
					break;
				case XC_LANGUAGE_GERMAN:
					while ( ( *p8 != '\\' ) && ( *p8 != '/' ) ) p8--;
					sprintf( name, "images\\grimages\\%s", &p8[1] );
					break;
				default:
					strcpy ( name, filename );
					break;
			}
		}
	}

//	Tmr::Time start_time = Tmr::GetTime();

//	Skate3 code, disabled for now.
#	if 0
    if (!just_freeze && ! blank)
	{
		RwImageSetPath( "" );
		RwRaster* raster = RwRasterRead( name );
	
		if( raster == NULL )
		{
			// Try the english version.
			raster = RwRasterRead( filename );
			if( raster == NULL )
			{
				Dbg_Printf( "Could not load %s\n", filename );
				return;
			}
		}

		// Wait a few VBlanks for RW stuff to disappear.
//		while( Tmr::GetTime() - start_time < Tmr::VBlanks( 4 ));

		RwRect rect;
		rect.x = SCREEN_CONV_X( 0 );
		rect.y = SCREEN_CONV_Y( 0 );
		rect.w = SCREEN_CONV_X( 640 ) - SCREEN_CONV_X( 0 );
		rect.h = SCREEN_CONV_Y( 448 ) - SCREEN_CONV_Y( 0 );

		Spt::SingletonPtr< Gfx::Manager > gfx_manager;

		_rwXbSetRenderState( rwRENDERSTATETEXTURERASTER, (void *)NULL );

		RwRaster* p_context_raster;
		p_context_raster = gfx_manager->DefaultViewport().GetFrameBuffer();

		RwInt32 format = p_context_raster->cFormat & 0x0000FF00L;
		RwRGBA black = { 0, 0, 0, 0 }; 

		RwRasterPushContext( p_context_raster );
		
		RwRasterClear( RwRGBAToPixel( &black, format ));		// 0: Clear to black
		RwRasterShowRaster( p_context_raster, NULL, 0 );

		RwRasterClear( RwRGBAToPixel( &black, format ));        // 1: Clear to black 
		RwRasterShowRaster( p_context_raster, NULL, 0 );

		RwRasterRenderScaled( raster, &rect );			        // 2: Draw image
		RwRasterShowRaster( p_context_raster, NULL, 0 );

		RwRasterRenderScaled( raster, &rect );                  // 3: Draw image
		RwRasterShowRaster( p_context_raster, NULL, 0 );
		RwRasterPopContext();

		RwRasterDestroy( raster );

		// Set up the timer event last so that the icon will only ever appear once the loadscreen is visible.
		if( s_timer_event == 0 )
		{
			s_timer_event = timeSetEvent(	( 1000 * s_load_icon_data.m_FrameDelay ) / 60,	// Delay (ms).
											0,												// Ignored resolution (ms).
											loadIconTimerCallback,							// Callback function.
											0,												// Callback data.
											TIME_PERIODIC | TIME_CALLBACK_FUNCTION );


			Dbg_Assert( s_timer_event != 0 );
		}
	}
	else
	{
		if (just_freeze)
		{
			Script::RunScript( "PreFreezeScreen" );
//			if( skyFrameBit & 0x1 )
//			{
//				copy_buffer( 1 );
//			}
//			else
//			{
//				copy_buffer( 0 );
//			}

			Script::RunScript( "PostFreezeScreen" );
		}
		else if( blank )
		{
			RwRect rect;
			rect.x = SCREEN_CONV_X( 0 );
			rect.y = SCREEN_CONV_Y( 0 );
			rect.w = SCREEN_CONV_X( 640 ) - SCREEN_CONV_X( 0 );
			rect.h = SCREEN_CONV_Y( 448 ) - SCREEN_CONV_Y( 0 );

			Spt::SingletonPtr< Gfx::Manager > gfx_manager;

			_rwXbSetRenderState( rwRENDERSTATETEXTURERASTER, (void *)NULL );

			RwRaster* p_context_raster;
			p_context_raster = gfx_manager->DefaultViewport().GetFrameBuffer();

			RwInt32 format = p_context_raster->cFormat & 0x0000FF00L;
			RwRGBA black = { 0, 0, 0, 0 }; 

			RwRasterPushContext( p_context_raster );

			RwRasterClear( RwRGBAToPixel( &black, format ));		// 0: Clear to black
			RwRasterShowRaster( p_context_raster, NULL, 0 );

			RwRasterClear( RwRGBAToPixel( &black, format ));        // 1: Clear to black 
			RwRasterShowRaster( p_context_raster, NULL, 0 );

			RwRasterPopContext();
		}
	}

#	endif
	
	Spt::SingletonPtr< Mlp::Manager > mlp_man;
	s_loading_screen_on = true;
	mlp_man->PauseDisplayTasks( true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Hide( void )
{
	if( s_timer_event != 0 )
	{
		timeKillEvent( s_timer_event );
		s_timer_event = 0;
	}

	Spt::SingletonPtr< Mlp::Manager > mlp_man;
	mlp_man->PauseDisplayTasks( false ); 

	if( !s_loading_screen_on )
	{
		return;
	}

	s_loading_screen_on = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	StartProgressBar( char* tick_image, int num_ticks, int x, int y )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	ProgressTick( int num_ticks )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	EndProgressBar( void )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace LoadScreen




