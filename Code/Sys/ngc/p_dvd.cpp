/*****************************************************************************
**																			**
**			              Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Skate3         											**
**																			**
**	Module:			SYS            	 										**
**																			**
**	File name:		p_dvd.cpp	 											**
**																			**
**	Created:		08/30/2001	dc     										**
**																			**
**	Description:	Gamecube DVD file acceess routines						**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

//#ifndef __NOPT_FINAL__
//#ifndef DVDETH
//#define __SN_FILE__
//#endif		// DVDETH
//#endif		// __NOPT_FINAL__ 

#include <stdarg.h>
#include <gel/mainloop.h>
#include <sys/sioman.h>
#include <gel/music/music.h>
#include <gel/soundfx/soundfx.h>
#include <engine/sounds.h>
#include <gel/music/ngc/p_music.h>
#include "p_assert.h"
#include "p_dvd.h"
#include "p_display.h"
#include "p_debugfont.h"
#include "p_display.h"
#include "p_prim.h"
#include <gel/scripting/symboltable.h>
#include <gfx/ngc/nx/nx_init.h>
#include <gfx/ngc/nx/render.h>
#include <sys\ngc\p_render.h>
#include <sys\ngc\p_dma.h>
#include <sys\ngc\p_buffer.h>
#include <gel/Scripting/script.h>
#include <gfx/NxLoadScreen.h>

#ifdef __SN_FILE__
#include "libsn.h"
bool sn_init = false;
#endif		// __SN_FILE__
 
extern bool gSoundManagerSingletonExists;

extern char * g_p_buffer;

extern void*   hwFrameBuffer1;
extern void*   hwFrameBuffer2;
extern bool	gLoadingScreenActive;
extern bool	gReload;
extern bool	gLoadingLoadScreen;
extern GXColor messageColor;

extern bool g_legal;

/*****************************************************************************
**							  DBG Information								**
*****************************************************************************/

/*****************************************************************************
**								  Externals									**
*****************************************************************************/
int _DVDError = 0;

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define READ_BLOCK_SIZE			32
//#ifdef DVDETH
//#define READ_LARGE_BLOCK_SIZE	(1024 * 2048)
//#else
#define READ_LARGE_BLOCK_SIZE	(8 * 1024)
//#endif		// DVDETH
//static char	byteData[READ_LARGE_BLOCK_SIZE+32];

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

int	DVDError( void )
{
	return _DVDError;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
//void dvd_show_error( int error )
//{
//	NsCamera cam;
//	cam.orthographic( 0, 0, 640, 448 );
//
//	// Draw the screen.
//	NsPrim::begin();
//
//	cam.begin();
//
//	// 320x32
//
//	NsRender::setBlendMode( NsBlendMode_None, NULL, (unsigned char)0 );
////	NsPrim::box( 64, 64, 640-64, 64+80, (GXColor){0,32,64,128} );
////	NsDebugFont::printf( 80, 80, NsFontEffect_Bold, 16, (GXColor){255,255,255,128}, "DVD Error:\n\n%s\n", error_name[error+1] );
//
//	#if ENGLISH == 1
//	NsPrim::box( 80, 48, 640-80, 48+102, (GXColor){0,32,64,128} );
//	#endif
//	#if FRENCH == 1
//	NsPrim::box( 80, 48, 640-80, 48+120, (GXColor){0,32,64,128} );
//	#endif
//	#if GERMAN == 1
//	NsPrim::box( 80, 48, 640-80, 48+138, (GXColor){0,32,64,128} );
//	#endif
//	switch ( error ) {
//		case DVD_STATE_FATAL_ERROR:
//		case DVD_STATE_RETRY:
//			
//			#if ENGLISH == 1
//			NsDebugFont::printf( 80+8+8, 48+8, NsFontEffect_None, 16, (GXColor){255,255,255,128}, "An error has occurred. Turn" );
//			NsDebugFont::printf( 80+8+8+4, 48+8+18, NsFontEffect_None, 16, (GXColor){255,255,255,128}, "the power off and refer to" );
//			NsDebugFont::printf( 80+8+40+4, 48+8+36, NsFontEffect_None, 16, (GXColor){255,255,255,128}, "the Nintendo GameCubeÿ" );
//			NsDebugFont::printf( 80+8+40, 48+8+54, NsFontEffect_None, 16, (GXColor){255,255,255,128}, "Instruction Booklet for" );
//			NsDebugFont::printf( 80+8+48, 48+8+72, NsFontEffect_None, 16, (GXColor){255,255,255,128}, "further instructions." );
//			#endif
//			#if FRENCH == 1
//			NsDebugFont::printf( 80+8+24, 48+8,    NsFontEffect_None, 16, (GXColor){255,255,255,128},   "Une erreur s'est produite." );
//			NsDebugFont::printf( 80+8+16, 48+8+18, NsFontEffect_None, 16, (GXColor){255,255,255,128},   "Il faut ‚teindre la console" );
//			NsDebugFont::printf( 80+8+64, 48+8+36, NsFontEffect_None, 16, (GXColor){255,255,255,128},   "et consulter le guide" );
//			NsDebugFont::printf( 80+8+48, 48+8+54, NsFontEffect_None, 16, (GXColor){255,255,255,128},   "d'instructions GameCube" );
//			NsDebugFont::printf( 80+8+56, 48+8+72, NsFontEffect_None, 16, (GXColor){255,255,255,128},   "Nintendoÿ pour plus de" );
//			NsDebugFont::printf( 80+8+168+4, 48+8+90, NsFontEffect_None, 16, (GXColor){255,255,255,128},   "d‚tails." );
//			#endif
//			#if GERMAN == 1
//			NsDebugFont::printf( 80+8+96, 48+8, NsFontEffect_None, 16, (GXColor){255,255,255,128},      "Es ist ein Fehler" );
//			NsDebugFont::printf( 80+8+40, 48+8+18, NsFontEffect_None, 16, (GXColor){255,255,255,128},   "aufgetreten. Schalte die" );
//			NsDebugFont::printf( 80+8+16, 48+8+36, NsFontEffect_None, 16, (GXColor){255,255,255,128},   "Konsole aus und sieh in der" );
//			NsDebugFont::printf( 80+8+48, 48+8+54, NsFontEffect_None, 16, (GXColor){255,255,255,128},   "Bedienungsanleitung des" );
//			NsDebugFont::printf( 80+8+16, 48+8+72, NsFontEffect_None, 16, (GXColor){255,255,255,128},   "Nintendo GameCubeÿ nach, um" );
//			NsDebugFont::printf( 80+8+64, 48+8+90, NsFontEffect_None, 16, (GXColor){255,255,255,128},   "weitere Informationen" );
//			NsDebugFont::printf( 80+8+140, 48+8+108, NsFontEffect_None, 16, (GXColor){255,255,255,128}, "zu erhalten." );
//			#endif
//			break;
//		case DVD_STATE_COVER_OPEN:
//			#if ENGLISH == 1
//			NsDebugFont::printf( 80+8+4, 48+8, NsFontEffect_None, 16, (GXColor){255,255,255,128}, "Please close the Disc Cover." );
//			#endif
//			#if FRENCH == 1
//			NsDebugFont::printf( 80+8+4+48, 48+8, NsFontEffect_None, 16, (GXColor){255,255,255,128}, "Refermer le couvercle." );
//			#endif
//			#if GERMAN == 1
//			NsDebugFont::printf( 80+8+60, 48+8, NsFontEffect_None, 16, (GXColor){255,255,255,128},   "Bitte den Disc-Deckel" );
//			NsDebugFont::printf( 80+8+156, 48+8+18, NsFontEffect_None, 16, (GXColor){255,255,255,128}, "schlieáen." );
//			#endif
//			break;
//		case DVD_STATE_NO_DISK:
//		case DVD_STATE_WRONG_DISK:
//			#if ENGLISH == 1
//			NsDebugFont::printf( 80+8,    48+8,    NsFontEffect_None, 16, (GXColor){255,255,255,128}, "Please insert the Tony Hawk's" );
//			NsDebugFont::printf( 80+8+48, 48+8+18, NsFontEffect_None, 16, (GXColor){255,255,255,128}, "Pro Skater 3 Game Disc." );
//			#endif
//			#if FRENCH == 1
//			NsDebugFont::printf( 80+8+40, 48+8, NsFontEffect_None, 16, (GXColor){255,255,255,128},    "Ins‚rer le disque de jeu" );
//			NsDebugFont::printf( 80+8+36, 48+8+18, NsFontEffect_None, 16, (GXColor){255,255,255,128}, "Tony Hawk's Pro Skater 3." );
//			#endif
//			#if GERMAN == 1
//			NsDebugFont::printf( 80+8+24, 48+8, NsFontEffect_None, 16, (GXColor){255,255,255,128},    "Bitte lege die Tony Hawk's" );
//			NsDebugFont::printf( (80+8+24+48)-64, 48+8+18, NsFontEffect_None, 16, (GXColor){255,255,255,128}, "Pro Skater 3 Spiel-Disc ein." );
//			#endif
//			break;
//	}
//
//	cam.end();
//
//	NsPrim::end();
//
//	if( NsDisplay::shouldReset())
//	{
//		// Prepare the game side of things for reset.
////		Spt::SingletonPtr< Mlp::Manager > mlp_manager;
////		mlp_manager->PrepareReset();
//
//		// Prepare the system side of things for reset.
//		NsDisplay::doReset();
//	}
//}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static void dvd_read_safe( DVDFileInfo* fileInfo, void* addr, int length, int offset )
{
#ifdef DVDETH
	DVDRead( fileInfo, addr, length, offset );
#else
	DVDReadAsync( fileInfo, addr, length, offset, NULL );
	DVDWaitAsync();
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void DVDWaitAsync()
{
#ifndef DVDETH
	int displayed = 0;
	int reported = 0;
//	void * _myXFB1;
//	void * _myXFB2;
//	void * _copyXFB;
//	void * _dispXFB;
	
	while ( 1 ) {
		s32 error;
		error = DVDGetDriveStatus();

		NsDisplay::doReset();

		if ( error == DVD_STATE_END ) break;
		if ( error == DVD_STATE_BUSY ) continue;
		if ( error == DVD_STATE_CANCELED ) break;
		if ( error == DVD_STATE_PAUSING ) continue;
		if ( error == DVD_STATE_WAITING ) continue;

		_DVDError = error;

		if ( DVDError() && Script::GetInteger( "allow_dvd_errors" ) )
		{
			if ( !displayed )
			{
				NxNgc::EngineGlobals.gpuBusy = true;
				displayed = 1;
				if ( gLoadingLoadScreen ) gReload = true;
				Spt::SingletonPtr< SIO::Manager >	sio_man;
				sio_man->Pause();
				Spt::SingletonPtr< Sfx::CSfxManager > sfx_manager;
				sfx_manager->PauseSounds();
				PCMAudio_Pause( true, Pcm::EXTRA_CHANNEL );
				PCMAudio_Pause( true, Pcm::MUSIC_CHANNEL );

				// Copy current framebuffer to ARAM (only if it's a loading screen).
//				NsDisplay::flush();
				if ( gLoadingScreenActive )
				{
					for ( int vv = 0; vv < 2; vv++ )
					{
						NsDisplay::begin();
						NsRender::begin();

						NsRender::end();
						NsDisplay::end( false );
					}

					// We'll use stream buffer 0 for this.
					uint32 address0 = NxNgc::EngineGlobals.aram_stream0;
					uint32 address1 = NxNgc::EngineGlobals.aram_stream1;
					uint32 address2 = NxNgc::EngineGlobals.aram_stream2;
					uint32 address3 = NxNgc::EngineGlobals.aram_music;

					int buf0_max = ( NsBuffer::get_size() / ( 640 * 2 ) );
					int buf1_max = buf0_max + ( ( 32 * 1024 ) / ( 640 * 2 ) );
					int buf2_max = buf1_max + ( ( 32 * 1024 ) / ( 640 * 2 ) );
					int buf3_max = buf2_max + ( ( 32 * 1024 ) / ( 640 * 2 ) );
					int buf4_max = buf3_max + ( ( 32 * 1024 ) / ( 640 * 2 ) );

					u16 line[640];
					for ( int yy = 0; yy < 448; yy++ )
					{
						for ( int xx = 0; xx < 640; xx++ )
						{
							u32 pixel;
							GX::PeekARGB( xx, yy, &pixel );
							int r = ( pixel >> 0 ) & 255;
							int g = ( pixel >> 8 ) & 255;
							int b = ( pixel >> 16 ) & 255;
							line[xx] = ( ( r >> 3 ) & 0x1f ) | ( ( ( g >> 2 ) & 0x3f ) << 5 ) | ( ( ( b >> 3 ) & 0x1f ) << 11 );
						}

						if ( yy < buf0_max )
						{
							memcpy( &g_p_buffer[(yy*640*2)], line, 640 * 2 );
						}
						if ( ( yy >= buf0_max ) && ( yy < buf1_max ) )
						{
							NsDMA::toARAM( ( address0 + ( ( yy - buf0_max ) * 640 * 2 ) ), line, 640 * 2 );
						}
						if ( ( yy >= buf1_max ) && ( yy < buf2_max ) )
						{
							NsDMA::toARAM( ( address1 + ( ( yy - buf1_max ) * 640 * 2 ) ), line, 640 * 2 );
						}
						if ( ( yy >= buf2_max ) && ( yy < buf3_max ) )
						{
							NsDMA::toARAM( ( address2 + ( ( yy - buf2_max ) * 640 * 2 ) ), line, 640 * 2 );
						}
						if ( ( yy >= buf3_max ) && ( yy < buf4_max ) )
						{
							NsDMA::toARAM( ( address3 + ( ( yy - buf3_max ) * 640 * 2 ) ), line, 640 * 2 );
						}
					}
				}
			}
			//OSReport( "We have to reset now.\n" );

			NxNgc::EngineGlobals.screen_brightness = 1.0f;

			// Render the text.
			NsDisplay::begin();
			NsRender::begin();

			NsCamera cam;
			cam.orthographic( 0, 0, 640, 448 );

			// Draw the screen.
			NsPrim::begin();

			cam.begin();

			GX::SetZMode( GX_FALSE, GX_ALWAYS, GX_TRUE );

			NxNgc::set_blend_mode( NxNgc::vBLEND_MODE_BLEND );

//			if ( NsDisplay::shouldReset() )
//			{
//				// Reset message.
//				Script::RunScript( "ngc_reset" );
//			}
//			else
			{
				// DVD Error message.
				switch ( DVDError() )
				{
					case DVD_STATE_FATAL_ERROR:
						Script::RunScript( "ngc_dvd_fatal" );
						NxNgc::EngineGlobals.disableReset = true;
						break;
					case DVD_STATE_RETRY:
						Script::RunScript( "ngc_dvd_retry" );
						break;
					case DVD_STATE_COVER_OPEN:
						Script::RunScript( "ngc_dvd_cover_open" );
						NxNgc::EngineGlobals.disableReset = false;
						break;
					case DVD_STATE_NO_DISK:
						Script::RunScript( "ngc_dvd_no_disk" );
						NxNgc::EngineGlobals.disableReset = false;
						break;
					case DVD_STATE_WRONG_DISK:
						Script::RunScript( "ngc_dvd_wrong_disk" );
						NxNgc::EngineGlobals.disableReset = false;
						break;
					default:
						break;
				}
			}

			NsDisplay::setBackgroundColor( messageColor );

			cam.end();

			NsPrim::end();

			NsRender::end();
			NsDisplay::end( true );
		}

//
//		_DVDError = 1;
//		if ( error != DVD_STATE_WAITING ) {
//			NsDisplay::begin();
//			dvd_show_error( error );
//			NsDisplay::end( false );
//			NsDisplay::flush();
//		}
//		if ( !displayed ) {
#ifndef __NOPT_FINAL__
			static char * error_name[] = {
				"DVD_STATE_FATAL_ERROR",     // -1
				"DVD_STATE_END",             // 0
				"DVD_STATE_BUSY",            // 1
				"DVD_STATE_WAITING",         // 2
				"DVD_STATE_COVER_CLOSED",    // 3
				"DVD_STATE_NO_DISK",         // 4
				"DVD_STATE_COVER_OPEN",      // 5
				"DVD_STATE_WRONG_DISK",      // 6
				"DVD_STATE_MOTOR_STOPPED",   // 7
				"DVD_STATE_PAUSING",         // 8
				"DVD_STATE_IGNORED",         // 9
				"DVD_STATE_CANCELED",        // 10
				"DVD_STATE_RETRY",           // 11
			};
#endif		// __NOPT_FINAL__

			if ( !reported )
			{
#ifndef __NOPT_FINAL__
				if ( ( error >= -1 ) && ( error <= 11 ) ) {
					OSReport( "Received DVD Error: (%d) %s\n", error, error_name[error+1] );
				} else {
					OSReport( "Received DVD Error: (%d)\n", error );
				}
#endif		// __NOPT_FINAL__
				reported = 1;
			}
//		}
//
//		if( NsDisplay::shouldReset())
//		{
//			// Prepare the game side of things for reset.
////			Spt::SingletonPtr< Mlp::Manager > mlp_manager;
////			mlp_manager->PrepareReset();
//
//			// Prepare the system side of things for reset.
//			NsDisplay::doReset();
//		}
//
//		displayed = 1;
	}

	if ( displayed )
	{
		// Copy current framebuffer from ARAM (only if it's a loading screen).
		if ( gLoadingScreenActive )
		{

			// We'll use stream buffer 0 for this.
			uint32 address0 = NxNgc::EngineGlobals.aram_stream0;
			uint32 address1 = NxNgc::EngineGlobals.aram_stream1;
			uint32 address2 = NxNgc::EngineGlobals.aram_stream2;
			uint32 address3 = NxNgc::EngineGlobals.aram_music;

			int buf0_max = ( NsBuffer::get_size() / ( 640 * 2 ) );
			int buf1_max = buf0_max + ( ( 32 * 1024 ) / ( 640 * 2 ) );
			int buf2_max = buf1_max + ( ( 32 * 1024 ) / ( 640 * 2 ) );
			int buf3_max = buf2_max + ( ( 32 * 1024 ) / ( 640 * 2 ) );
			int buf4_max = buf3_max + ( ( 32 * 1024 ) / ( 640 * 2 ) );

			u16 line[640];
			for ( int yy = 0; yy < 448; yy++ )
			{
				if ( yy < buf0_max )
				{
					memcpy( line, &g_p_buffer[(yy*640*2)], 640 * 2 );
				}
				if ( ( yy >= buf0_max ) && ( yy < buf1_max ) )
				{
					NsDMA::toMRAM( line, ( address0 + ( ( yy - buf0_max ) * 640 * 2 ) ), 640 * 2 );
				}
				if ( ( yy >= buf1_max ) && ( yy < buf2_max ) )
				{
					NsDMA::toMRAM( line, ( address1 + ( ( yy - buf1_max ) * 640 * 2 ) ), 640 * 2 );
				}
				if ( ( yy >= buf2_max ) && ( yy < buf3_max ) )
				{
					NsDMA::toMRAM( line, ( address2 + ( ( yy - buf2_max ) * 640 * 2 ) ), 640 * 2 );
				}
				if ( ( yy >= buf3_max ) && ( yy < buf4_max ) )
				{
					NsDMA::toMRAM( line, ( address3 + ( ( yy - buf3_max ) * 640 * 2 ) ), 640 * 2 );
				}

				for ( int xx = 0; xx < 640; xx++ )
				{
					u32 pixel;
					int r = ( ( line[xx] >>  0 ) & 0x1f ) << 3;
					int g = ( ( line[xx] >>  5 ) & 0x3f ) << 2;
					int b = ( ( line[xx] >> 11 ) & 0x1f ) << 3;
					pixel = ( 255 << 24 ) | ( b << 16 ) | ( g << 8 ) | r;

					GX::PokeARGB( xx, yy, pixel );
				}
			}
		}
		for ( int vv = 0; vv < 2; vv++ )
		{
			//OSReport( "We have to reset now.\n" );

			NxNgc::EngineGlobals.screen_brightness = 1.0f;

			// Render the text.
			NsDisplay::begin();
			NsRender::begin();

			// Draw the screen.
			NsPrim::begin();

			NsPrim::end();

			NsDisplay::setBackgroundColor( (GXColor){0,0,0,0} );

			NsRender::end();
			NsDisplay::end( gLoadingScreenActive ? false : true );
		}

		if ( g_legal ) display_legal();

//		NsDisplay::begin();
//		dvd_show_error( DVD_STATE_BUSY );
//		NsDisplay::end( false );
//		NsDisplay::flush();
//		_DVDError = 0;
		PCMAudio_Pause( false, Pcm::MUSIC_CHANNEL );
		PCMAudio_Pause( false, Pcm::EXTRA_CHANNEL );
   		Spt::SingletonPtr< SIO::Manager >	sio_man;
		sio_man->UnPause();
		NxNgc::EngineGlobals.gpuBusy = false;
	}
	_DVDError = 0;
#endif		// DVDETH
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void DVDCheckAsync( void )
{
#ifndef DVDETH
	s32 error;
	error = DVDGetDriveStatus();

	if ( error == DVD_STATE_END ) return;
	if ( error == DVD_STATE_BUSY ) return;
	if ( error == DVD_STATE_CANCELED ) return;
	if ( error == DVD_STATE_PAUSING ) return;

	DVDWaitAsync();
#endif		// DVDETH
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static char* dvd_file_fix_name( const char * pFilename )
{
	static char buffer[256];
	char *p;

	// Copy to our buffer.
	strcpy ( buffer, pFilename );

	// Replace \\ with /.
	p = buffer;
	while ( *p != '\0' ) {
		if ( *p == '\\' ) *p = '/';
		p++;
	}

	// If this is a .pre file, switch to a .prg file.
	int idx = strlen( buffer );
	if((( buffer[idx - 1] ) == 'e' ) &&
	   (( buffer[idx - 2] ) == 'r' ) &&
	   (( buffer[idx - 3] ) == 'p' ))
	{
		buffer[idx - 1] = 'g';
	}

	// Deal with .prgf & .prgd
	if((( buffer[idx - 2] ) == 'e' ) &&
	   (( buffer[idx - 3] ) == 'r' ) &&
	   (( buffer[idx - 4] ) == 'p' ))
	{
		buffer[idx - 2] = buffer[idx - 1];
		buffer[idx - 1] = buffer[idx];
	}

	return buffer;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int dvd_file_size( NsFile* p_file )
{
	if ( !p_file->m_sizeCached ) {
	    // Get the size of the DVD
	    p_file->m_cachedSize = DVDGetLength(&p_file->m_FileInfo);
		p_file->m_sizeCached = 1;
	}

	return p_file->m_cachedSize;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int dvd_file_open( NsFile* p_file, const char* p_filename )
{
	char* pFixedName;
	
	// Assert if we already have an open NsFile.
	if( p_file->m_FileOpen ) {
		Dbg_MsgAssert( 0, ("NsFile already open.\n"));
	}

	pFixedName = dvd_file_fix_name(p_filename);

#ifdef DVDETH
	DVDOpen( pFixedName, &p_file->m_FileInfo );
#else
	int entry = DVDConvertPathToEntrynum( pFixedName );

	if ( entry == -1 ) {
		// File is not present.
		OSReport ( "NsFile: Open %s does not exist\n", pFixedName );
		return 0;
	}

	OSReport ( "NsFile: Open %s\n", pFixedName );

	while (FALSE == DVDFastOpen( entry, &p_file->m_FileInfo )) {
//		dvd_handle_error( &p_file->m_FileInfo );
//        OSHalt("Cannot open NsFile.\n");
    }
#endif		// DVDETH

	p_file->m_FileOpen		= 1;
	p_file->m_numCacheBytes	= 0;
	p_file->m_seekOffset	= 0;
	dvd_file_size( p_file );

	return 1;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int dvd_file_eof( NsFile* p_file )
{
	return ( p_file->m_seekOffset >= dvd_file_size( p_file ) ) ? 1 : 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
s32 dvd_file_read_anywhere( DVDFileInfo* fileInfo, void* addr, s32 length, s32 offset )
{
	char	byteData[READ_LARGE_BLOCK_SIZE+32];
	char  * bytes;
	int		toread;
	char  * p8;
	int		off;

	// If we're reading to an odd location, or the read offset is not 4-byte
	// aligned, we need a mis-aligned read.
	if ( (((int)addr) & 0x0000001f) || (offset & 3) ) {
		// Mis-aligned.
//		printf ( "mis-aligned read %d bytes, offset %d\n", length, offset );
		bytes = (char *)(OSRoundUp32B(((int)byteData)));
		toread = length;
		p8 = (char *)addr;
		off = offset;
		// Read odd bytes if any.
		if ( offset & 3 ) {
			dvd_read_safe( fileInfo, bytes, offset & 3, off );
			toread -= offset & 3;
			off += offset & 3;
			memcpy ( p8, bytes, offset & 3 );
			DCFlushRange ( p8, offset & 3 );
			p8 += offset & 3;
		}
		// Read as many large blocks as we can.
		while ( toread > READ_LARGE_BLOCK_SIZE ) {
			dvd_read_safe( fileInfo, bytes, READ_LARGE_BLOCK_SIZE, off );
			toread -= READ_LARGE_BLOCK_SIZE;
			off += READ_LARGE_BLOCK_SIZE;
			memcpy ( p8, bytes, READ_LARGE_BLOCK_SIZE );
			DCFlushRange ( p8, READ_LARGE_BLOCK_SIZE );
			p8 += READ_LARGE_BLOCK_SIZE;
		}
		// Read last block.
		if ( toread > 0 ) {
			dvd_read_safe( fileInfo, bytes, OSRoundUp32B(toread), off );
			memcpy ( p8, bytes, toread );
			DCFlushRange ( p8, toread );
		}
		return length;
	} else {
//		printf ( "aligned read %d bytes, offset %d\n", length, offset );
		// Just do a straight read.
		dvd_read_safe( fileInfo, addr, length, offset );
		return length;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void dvd_file_fill_cache( NsFile* p_file )
{
    int	bytesRead;

	bytesRead = dvd_file_read_anywhere(&p_file->m_FileInfo, &p_file->m_cachedBytes[0], READ_BLOCK_SIZE, p_file->m_seekOffset);
	p_file->m_seekOffset += bytesRead;
	Dbg_MsgAssert( bytesRead > 0, ("Error occurred when reading DVD"));
	p_file->m_numCacheBytes = bytesRead;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int dvd_file_read( NsFile* p_file, void* pDest, int numBytes )
{
    int		bytesRemaining;
    int		totalBytesRead;
    int		bytesRead;
    int		bytesRounded;
    int		saveCacheBytes;
	char  * p8;

	p8 = (char *)pDest;
	totalBytesRead = 0;

	// Pull from cache first.
	if ( p_file->m_numCacheBytes >= numBytes ) {
		// All bytes requested are in the cache - copy what we need.
//		OSReport ( "All bytes in cache: %d\n", numBytes );
		p_file->GetCacheBytes ( p8, numBytes );
		return ( numBytes );
	} else {
		if ( p_file->m_numCacheBytes ) {
//			OSReport ( "Emptying cache: %d\n", numCacheBytes );
			// Set number of bytes remaining.
			bytesRemaining = numBytes - p_file->m_numCacheBytes;
			//  Empty cache.
			saveCacheBytes = p_file->m_numCacheBytes;		// Must save ast GetCacheBytes will set it to 0, and we need it for the pointer advance.
			p_file->GetCacheBytes ( p8, p_file->m_numCacheBytes );
			p8 += saveCacheBytes;
			totalBytesRead += saveCacheBytes;
		} else {
			bytesRemaining = numBytes;
		}
	}
	// Read multiples of READ_BLOCK_SIZE bytes.
	bytesRounded = ( bytesRemaining / READ_BLOCK_SIZE ) * READ_BLOCK_SIZE;
	if ( bytesRounded ) {
//		OSReport ( "Block read: %d\n", bytesRounded );
		if ( dvd_file_eof( p_file ) ) return 0;
		bytesRead = dvd_file_read_anywhere(&p_file->m_FileInfo, p8, (s32)bytesRounded, p_file->m_seekOffset);
		p_file->m_seekOffset += bytesRounded;
		Dbg_MsgAssert( bytesRead > 0, ("Error occurred when reading NsFile"));
		totalBytesRead += bytesRead;
		p8 += bytesRounded;
		bytesRemaining -= bytesRounded;
	}

	// Cache odd bytes
	if ( bytesRemaining ) {
		// Fill up the cache.
		if ( dvd_file_eof( p_file ) ) return 0;
		dvd_file_fill_cache( p_file );
		// Pull remaining bytes from cache.
//		OSReport ( "Cache Copy: %d\n", bytesRemaining );
		p_file->GetCacheBytes ( p8, bytesRemaining );
		totalBytesRead += bytesRemaining;
	}

//    OSRestoreInterrupts(enabled);

	return totalBytesRead;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int dvd_file_seek( NsFile* p_file, NsFileSeek type, int offset )
{
	switch ( type ) {
		case NsFileSeek_Start:
			p_file->m_seekOffset = offset;
			break;
		case NsFileSeek_End:
			p_file->m_seekOffset = dvd_file_size( p_file ) - offset;
			break;
		case NsFileSeek_Current:
			p_file->m_seekOffset += offset;
			break;
		default:
			Dbg_MsgAssert( 0, ("Illegal seek type.\n"));
			break;
	}
	p_file->m_numCacheBytes = 0;

	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int dvd_file_close( NsFile* p_file )
{
    // Close the DVD
	DVDClose(&p_file->m_FileInfo);

	p_file->m_FileOpen		= 0;
	p_file->m_sizeCached	= 0;
	p_file->m_numCacheBytes	= 0;

	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int dvd_file_exist( const char * pFilename )
{
#ifdef DVDETH
	DVDFileInfo info;
	if ( DVDOpen( pFilename, &info ) )
	{
		DVDClose( &info );
		return 1;
	}
	else
	{
		return 0;
	}
#else
	// Return 0 if file doen't exist, 1 if we found it.
	return DVDConvertPathToEntrynum( dvd_file_fix_name( pFilename ) ) == -1 ? 0 : 1;
#endif
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
char dvd_file_getcharacter( NsFile* p_file )
{
	char c = EOF;
	dvd_file_read( p_file, &c, 1 );
	return c;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
char * dvd_file_gets( NsFile* p_file, char * s, int n )
{
	register int c = EOF;
	register char * cs;

	cs = s;
	while ( --n > 0 && (c = dvd_file_getcharacter( p_file )) != EOF ) {
		if ((*cs++ = c) == '\n') break;
	}
	*cs = '\0';
	return (c == EOF && cs == s) ? NULL : s;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int dvd_file_tell( NsFile* p_file )
{
	return p_file->m_seekOffset;
}



/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int DVDFileAccess( NsFile* p_file, NsFileAccessType access_type, ... ) 
{
	Dbg_MsgAssert( p_file, ("NULL pointer.\n"));

#ifdef __SN_FILE__
	if ( !sn_init )
	{
		PCinit();
		sn_init = true;
	}


	switch( access_type )
	{
		case FSIZE:
		{
			int current = PClseek( p_file->m_FileOpen, 0, SEEK_CUR );  
			int size = PClseek( p_file->m_FileOpen, 0, SEEK_END );  
			PClseek( p_file->m_FileOpen, current, SEEK_SET );
			return size;
		}

		case FOPEN:
		{
			va_list	vl;
			va_start( vl, access_type );
			char* raw_name	= va_arg( vl, char* );
			va_end( vl );

			char * name = dvd_file_fix_name( raw_name );

			return ( ( ( p_file->m_FileOpen = PCopen( name, 0, 0 ) ) == SN_ERR ) ? 0 : 1 );
		}

		case FREAD:
		{
			va_list	vl;
			va_start( vl, access_type );
			void*	dest		= va_arg( vl, void* );
			int		num_bytes	= va_arg( vl, int );
			va_end( vl );

			return ( ( ( PCread( p_file->m_FileOpen, (char*)dest, num_bytes ) ) == SN_ERR ) ? 0 : 1 );
		}

		case FSEEK:
		{
			va_list	vl;
			va_start( vl, access_type );
			NsFileSeek	type	= va_arg( vl, NsFileSeek );
			int			offset	= va_arg( vl, int );
			va_end( vl );

			switch ( type )
			{
				case NsFileSeek_End:
					return PClseek( p_file->m_FileOpen, offset, SEEK_END );
					break;
				case NsFileSeek_Current:
					return PClseek( p_file->m_FileOpen, offset, SEEK_CUR );
					break;
				default:
				case NsFileSeek_Start:
					return PClseek( p_file->m_FileOpen, offset, SEEK_SET );
					break;
			}
		}

		case FGETC:
		{
			char c = EOF;
			PCread( p_file->m_FileOpen, &c, 1 ); 
			return c;
		}

		case FGETS:
		{
			va_list	vl;
			va_start( vl, access_type );
			char*	dest		= va_arg( vl, char* );
			int		num_chars	= va_arg( vl, int );
			va_end( vl );

			register int c = EOF;
			register char * cs;

			cs = dest;
			char ch;
			ch = EOF;
			PCread( p_file->m_FileOpen, &ch, 1 ); 
			while ( --num_chars > 0 && c != EOF ) {
				if ((*cs++ = c) == '\n') break;
				ch = EOF;
				PCread( p_file->m_FileOpen, &ch, 1 ); 
			}
			*cs = '\0';
			return (int)((c == EOF && cs == dest) ? NULL : dest);
		}

		case FCLOSE:
		{
			return ( ( ( PCclose( p_file->m_FileOpen ) ) == SN_ERR ) ? 0 : 1 );
		}

		case FEOF:
		{
			int current = PClseek( p_file->m_FileOpen, 0, SEEK_CUR );  
			int end = PClseek( p_file->m_FileOpen, 0, SEEK_END );  
			PClseek( p_file->m_FileOpen, current, SEEK_SET );
			return ( current >= end ) ? 1 : 0;
		}

		case FTELL:
		{
			return PClseek( p_file->m_FileOpen, 0, SEEK_CUR );   
		}

		case FEXIST:
		{
			va_list	vl;
			va_start( vl, access_type );
			char* raw_name	= va_arg( vl, char* );
			va_end( vl );

			char * name = dvd_file_fix_name( raw_name );

			int tag = PCopen( name, 0, 0 );
			if ( tag != SN_ERR )
			{
				PCclose( tag );
				return 1;
			}
			else
			{
				return 0;
			}
		}

		default:
		{
			Dbg_MsgAssert( 0, ( "Unknown file access type" ));
			break;
		}
	}
#else

#ifdef DVDETH
	switch( access_type )
	{
		case FSIZE:
		{
			return p_file->m_size;
		}

		case FOPEN:
		{
			va_list	vl;
			va_start( vl, access_type );
			char* name	= va_arg( vl, char* );
			va_end( vl );

			// Read the whole file.
			if ( DVDOpen( dvd_file_fix_name( name ), &p_file->m_FileInfo ) )
			{
				p_file->m_size = DVDGetLength ( &p_file->m_FileInfo );
				Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
				p_file->mp_data = new (Mem::Manager::sHandle().TopDownHeap()) char[p_file->m_size];
				Mem::Manager::sHandle().BottomUpHeap()->PopAlign();


				int size = p_file->m_size;
				int off = 0;
				while ( size )
				{
					if ( size > ( 512 * 1024 ) )
					{
						DVDRead( &p_file->m_FileInfo, &p_file->mp_data[off], ( 512 * 1024 ), off );
						off += ( 512 * 1024 );
						size -= ( 512 * 1024 );
					}
					else
					{
						DVDRead( &p_file->m_FileInfo, &p_file->mp_data[off], size, off );
						size = 0;
					}
				}



//				DVDRead( &p_file->m_FileInfo, p_file->mp_data, p_file->m_size, 0 );
				DVDClose(&p_file->m_FileInfo);
				p_file->m_offset = 0;
				return 1;
			}
			else
			{
				p_file->mp_data = NULL;
				p_file->m_size = 0;
			}
		}

		case FREAD:
		{
			va_list	vl;
			va_start( vl, access_type );
			void*	dest		= va_arg( vl, void* );
			int		num_bytes	= va_arg( vl, int );
			va_end( vl );

			memcpy( dest, &p_file->mp_data[p_file->m_offset], num_bytes );
			p_file->m_offset += num_bytes;

			return num_bytes;
		}

		case FSEEK:
		{
			va_list	vl;
			va_start( vl, access_type );
			NsFileSeek	type	= va_arg( vl, NsFileSeek );
			int			offset	= va_arg( vl, int );
			va_end( vl );

			switch ( type ) {
				case NsFileSeek_Start:
					p_file->m_offset = offset;
					break;
				case NsFileSeek_End:

					p_file->m_offset = p_file->m_size - offset;
					break;
				case NsFileSeek_Current:
					p_file->m_offset += offset;
					break;
				default:
					Dbg_MsgAssert( 0, ("Illegal seek type.\n"));
					break;
			}

			return 1;
		}

		case FGETC:
		{
			char character;
			memcpy( &character, &p_file->mp_data[p_file->m_offset], 1 );
			p_file->m_offset += 1;
			return (int)character;
		}

		case FGETS:
		{
			va_list	vl;
			va_start( vl, access_type );
			char*	dest		= va_arg( vl, char* );
			int		num_chars	= va_arg( vl, int );
			va_end( vl );



			register int c = EOF;
			register char * cs;

			cs = dest;
			memcpy( &c, &p_file->mp_data[p_file->m_offset], 1 );
			p_file->m_offset += 1;
			while ( --num_chars > 0 && c != EOF ) {
				if ((*cs++ = c) == '\n') break;
				memcpy( &c, &p_file->mp_data[p_file->m_offset], 1 );
				p_file->m_offset += 1;
			}
			*cs = '\0';
			return (c == EOF && cs == dest) ? NULL : (int)dest;
		}

		case FCLOSE:
		{
			delete p_file->mp_data;
			return 0;
//			return dvd_file_close( p_file );
		}

		case FEOF:
		{
			return ( p_file->m_offset >= p_file->m_size ) ? 1 : 0;
		}

		case FTELL:
		{
			return p_file->m_offset;
		}

		case FEXIST:
		{
			va_list	vl;
			va_start( vl, access_type );
			char* name	= va_arg( vl, char* );
			va_end( vl );

			DVDFileInfo info;
			if ( DVDOpen( name, &info ) )
			{
				DVDClose( &info );
				return 1;
			}
			else
			{
				return 0;
			}
		}

		default:
		{
			Dbg_MsgAssert( 0, ( "Unknown file access type" ));
			break;
		}
	}
#else
//	if( NsDisplay::shouldReset())
//	{
//		// Prepare the game side of things for reset.
//		Spt::SingletonPtr< Mlp::Manager > mlp_manager;
//		mlp_manager->PrepareReset();
//
//		// Prepare the system side of things for reset.
//		NsDisplay::doReset();
//	}

	switch( access_type )
	{
		case FSIZE:
		{
			return dvd_file_size( p_file );			
		}

		case FOPEN:
		{
			va_list	vl;
			va_start( vl, access_type );
			char* name	= va_arg( vl, char* );
			va_end( vl );

			// Open mode not actually used in this context.
			return dvd_file_open( p_file, name );
		}

		case FREAD:
		{
			va_list	vl;
			va_start( vl, access_type );
			void*	dest		= va_arg( vl, void* );
			int		num_bytes	= va_arg( vl, int );
			va_end( vl );

			return dvd_file_read( p_file, dest, num_bytes );
		}

		case FSEEK:
		{
			va_list	vl;
			va_start( vl, access_type );
			NsFileSeek	type	= va_arg( vl, NsFileSeek );
			int			offset	= va_arg( vl, int );
			va_end( vl );

			return dvd_file_seek( p_file, type, offset );
		}

		case FGETC:
		{
			return (int)dvd_file_getcharacter( p_file );
		}

		case FGETS:
		{
			va_list	vl;
			va_start( vl, access_type );
			char*	dest		= va_arg( vl, char* );
			int		num_chars	= va_arg( vl, int );
			va_end( vl );

			return (int)dvd_file_gets( p_file, dest, num_chars );
		}

		case FCLOSE:
		{
			return dvd_file_close( p_file );
		}

		case FEOF:
		{
			return dvd_file_eof( p_file );
		}

		case FTELL:
		{
			return dvd_file_tell( p_file );
		}

		case FEXIST:
		{
			va_list	vl;
			va_start( vl, access_type );
			char* name	= va_arg( vl, char* );
			va_end( vl );

			return dvd_file_exist( name );
		}

		default:
		{
			Dbg_MsgAssert( 0, ( "Unknown file access type" ));
			break;
		}
	}
#endif		// DVDETH
#endif		// __SN_FILE__

	return 0;
}


