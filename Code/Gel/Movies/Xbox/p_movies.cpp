/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		SK3														**
**																			**
**	Module:			Game Engine (GEL)	 									**
**																			**
**	File name:		p_movies.cpp											**
**																			**
**	Created:		07/24/01	-	dc										**
**																			**
**	Description:	Xbox specific .bik Bink movie streaming code			**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <xtl.h>
#include <core/defines.h>
#include <core/macros.h>
#include <core/singleton.h>
#include <gfx/gfxman.h>
#include <gfx/xbox/nx/render.h>
#include <gel/soundfx/soundfx.h>
#include <gel/music/music.h>
#include <gel/movies/movies.h>
#include <gel/movies/xbox/p_movies.h>
#include <sys/sioman.h>


/*****************************************************************************
**								DBG Information								**
*****************************************************************************/
extern DWORD PixelShader4;

namespace Flx
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/


/*****************************************************************************
**								   Defines									**
*****************************************************************************/




/*****************************************************************************
**								Private Types								**
*****************************************************************************/

typedef struct
{
	float	sx,sy,sz;
	float	rhw;
	uint32	color;
	float	tu,tv;
}
MOVIE_VERT;

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

bool	alphaPixels					= false;
uint32	currentPlaybackSurfaceType	= BINKSURFACE32;
float	playbackWidth				= 640.0f;
float	playbackHeight				= 480.0f;
float	textureWidth				= 640.0f;
float	textureHeight				= 480.0f;
float	playbackWidthScale			= 1.0f;
float	playbackHeightScale			= 1.0f;
HBINK	playbackBinkHandle			= NULL;

DWORD	cullMode;
DWORD	multisampleAntialias;
DWORD	minFilter;

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

LPDIRECT3DTEXTURE8 openPlaybackImage( uint32 width, uint32 height )
{
	// Need to this prevent the single poly being culled.
	D3DDevice_GetRenderState( D3DRS_CULLMODE,				&cullMode );
	D3DDevice_SetRenderState( D3DRS_CULLMODE,				D3DCULL_NONE );

	D3DDevice_GetRenderState( D3DRS_MULTISAMPLEANTIALIAS,	&multisampleAntialias );
	D3DDevice_SetRenderState( D3DRS_MULTISAMPLEANTIALIAS,	FALSE );
	
	D3DDevice_SetRenderState( D3DRS_LIGHTING,				FALSE );

	NxXbox::set_render_state( RS_ZWRITEENABLE,	0 );
	NxXbox::set_render_state( RS_ZTESTENABLE,	0 );
	
	D3DDevice_GetTextureStageState( 0, D3DTSS_MINFILTER,	&minFilter );
	D3DDevice_SetTextureStageState( 0, D3DTSS_MINFILTER,	D3DTEXF_LINEAR );

	textureWidth	= (float)width;
	textureHeight	= (float)height;
	
	// Adjust for our current screen buffer.
	playbackWidth	= (float)NxXbox::EngineGlobals.backbuffer_width;
	playbackHeight	= (float)NxXbox::EngineGlobals.backbuffer_height;
	
	// Create a surface for our texture with the DirectDraw handle.
	LPDIRECT3DTEXTURE8 p_texture_surface;
	if( SUCCEEDED( D3DDevice_CreateTexture( width, height, 1, 0, D3DFMT_LIN_X8R8G8B8, 0, &p_texture_surface )))
	{
		return p_texture_surface;
	}

	return NULL;
}



void closePlaybackImage( LPDIRECT3DTEXTURE8 p_image )
{
	// Restore various states.
	D3DDevice_SetRenderState( D3DRS_CULLMODE,				cullMode );
	D3DDevice_SetRenderState( D3DRS_MULTISAMPLEANTIALIAS,	multisampleAntialias );
	D3DDevice_SetTextureStageState( 0, D3DTSS_MINFILTER,	minFilter );
	
	NxXbox::set_render_state( RS_ZWRITEENABLE,	1 );
	NxXbox::set_render_state( RS_ZTESTENABLE,	1 );

	if( p_image )
	{
		ULONG refcount = p_image->Release();
		Dbg_Assert( refcount == 0 );
	}

	NxXbox::set_texture( 0, NULL );
}



// Advance a Bink file by one frame into a 3D image buffer.
static void decompressFrame( HBINK bink, LPDIRECT3DTEXTURE8 p_image, bool copy_all )
{
	// Decompress the Bink frame.
	BinkDoFrame( bink );

	// Lock the 3D image so that we can copy the decompressed frame into it.
	D3DLOCKED_RECT lock_rect;
	if( SUCCEEDED( p_image->LockRect( 0, &lock_rect, 0, 0 )))
	{
		void*	pixels		= lock_rect.pBits;
		uint32	pixel_pitch	= lock_rect.Pitch;

		// Copy the decompressed frame into the 3D image.
		BinkCopyToBuffer(	bink,
							pixels,
							pixel_pitch,
							bink->Height,
							0,					// Left pixel offset.
							0,					// Top pixel offset.
							currentPlaybackSurfaceType | (( copy_all ) ? BINKCOPYALL : 0 ));

		// Unlock the 3D image.
		p_image->UnlockRect( 0 );
	}
}



// Blit a 3D image onto the render target.
void blitImage( LPDIRECT3DTEXTURE8 p_image, float x_offset, float y_offset, float x_scale, float y_scale, float alpha_level )
{
	if( p_image == NULL )
	{
		return;
	}

	NxXbox::set_blend_mode( NxXbox::vBLEND_MODE_DIFFUSE );
	
	// Turn on clamping so that the linear textures work
	NxXbox::set_render_state( RS_UVADDRESSMODE0, 0x00010001UL );
	
	// Use a default vertex and pixel shader
	NxXbox::set_vertex_shader( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1 );
	NxXbox::set_pixel_shader( PixelShader4 );

	// Select the texture.
	NxXbox::set_texture( 0, NULL );
	NxXbox::set_texture( 0, p_image );

	// Setup up the vertices.
	MOVIE_VERT vertices[4];

	vertices[0].sx		= x_offset;
	vertices[0].sy		= y_offset;
	vertices[0].sz		= 0.0f;
	vertices[0].rhw		= 0.0f;
	vertices[0].color	= ((int)(( alpha_level * 255.0f )) << 24 ) | 0x808080;
	vertices[0].tu		= -0.5f;
	vertices[0].tv		= -0.5f;

	vertices[1]			= vertices[ 0 ];
	vertices[1].sx		= x_offset + (((float)playbackWidth ) * x_scale );
	vertices[1].tu		= ((float)textureWidth ) - 0.5f;

	vertices[2]			= vertices[0];
	vertices[2].sy		= y_offset + (((float)playbackHeight ) * y_scale );
	vertices[2].tv		= ((float)textureHeight ) - 0.5f;

	vertices[3]			= vertices[1];
	vertices[3].sy		= vertices[2].sy;
	vertices[3].tv		= ((float)textureHeight ) - 0.5f;

	// Draw the vertices.
	D3DDevice_DrawVerticesUP( D3DPT_TRIANGLESTRIP, 4, vertices, sizeof( MOVIE_VERT ));
}






static void showFrame( HBINK bink, LPDIRECT3DTEXTURE8 p_image )
{
	// Begin a 3D frame.
	D3DDevice_Clear( 0, 0, D3DCLEAR_TARGET, 0, 1.0f, 0 );

	// Draw the image on the screen (centered)...
	float x = 0.0f;
	float y = 0.0f;
	blitImage( p_image, x, y, playbackWidthScale, playbackHeightScale, 1.0f );

	// End a 3D frame.
	D3DDevice_Swap( D3DSWAP_DEFAULT );

	// Keep playing the movie.
	BinkNextFrame( bink );
}




/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

static bool sDebounceCheck(uint16 *p_debounceFlags, uint8 *p_data, uint16 mask)
{
	if (p_data==NULL)
	{
		return false;
	}
		
	Dbg_MsgAssert(p_debounceFlags,("NULL p_debounceFlags"));
	
	uint16 data=(p_data[2]<<8)|p_data[3];
	if ((data & mask)==0)
	{
		// Button is pressed.
		
		if (*p_debounceFlags & mask)
		{
			// OK to return true, because the button got released at some point in the past.
			return true;
		}	
	}
	else
	{
		// Button is not pressed, so set the appropriate debounce flag so that next time it is
		// pressed it will be detected.
		*p_debounceFlags |= mask;
	}
	return false;	
}

void PMovies_PlayMovie( const char* pName )
{
	// Figure the volume we want to play at.
	Spt::SingletonPtr< Sfx::CSfxManager > sfx_manager;
	float music_vol	= Pcm::GetVolume();
	float sfx_vol	= sfx_manager->GetMainVolume( );
	float vol		= music_vol > sfx_vol ? music_vol : sfx_vol;

	// Incoming movie name is in the form movies\<name>, convert to d:\data\movies\bik\<name>.bik.
	char name_conversion_buffer[256] = "d:\\data\\movies\\bik\\";
	int length		= strlen( pName );
	int backwards	= length;
	while( backwards )
	{
		if( pName[backwards] == '\\' )
		{
			++backwards;
			break;
		}
		--backwards;
	}
	strncpy( name_conversion_buffer + 19, pName + backwards, length - backwards );
	length = strlen( name_conversion_buffer );
	name_conversion_buffer[length] = '.';
	name_conversion_buffer[length + 1] = 'b';
	name_conversion_buffer[length + 2] = 'i';
	name_conversion_buffer[length + 3] = 'k';
	name_conversion_buffer[length + 4] = 0;

	// Stop music and streams.
	Pcm::StopMusic();
	Pcm::StopStreams();
	
	playbackBinkHandle = BinkOpen( name_conversion_buffer, 0 );
	if( playbackBinkHandle == NULL )
	{
		// Movie not there, just quit.
		return;
	}

	// Switch the presentation interval to 30fps.
	DWORD presentation_interval;
	D3DDevice_GetRenderState( D3DRS_PRESENTATIONINTERVAL, &presentation_interval );
	D3DDevice_SetRenderState( D3DRS_PRESENTATIONINTERVAL, D3DPRESENT_INTERVAL_ONE );
	
	// Open a 3D image for the Bink.
	LPDIRECT3DTEXTURE8 p_image = openPlaybackImage( playbackBinkHandle->Width, playbackBinkHandle->Height );
	Dbg_Assert( p_image );

	// K: Call XGetDeviceChanges once before going into the loop, otherwise the first time this function
	// is run it will think new cards and pads have been inserted.
//	bool running_demo_movie=Script::GetInt("RunningDemoMovie");
	bool running_demo_movie=false;
	if (running_demo_movie)
	{
		DWORD insertions=0;
		DWORD removals=0;
		XGetDeviceChanges(XDEVICE_TYPE_MEMORY_UNIT, &insertions, &removals);
		XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &insertions, &removals);
	}				
	
	if( p_image )
	{
		BinkSetSoundOnOff( playbackBinkHandle, 1 );

		// For BinkSetVolume(), 32768 is the 'nornal volume', effectively full volume on systems that don't
		// do amplification, like Xbox.
		BinkSetVolume( playbackBinkHandle, 0, (int)( 16384 * ( vol * 0.01f )));

		// Will need this in the loop...
		Spt::SingletonPtr< SIO::Manager > sio_manager;
		uint16 debounce_flags[4] = { 0,0,0,0 };

		// Start the playback loop.
		while( true )
		{
			if( !BinkWait( playbackBinkHandle ))
			{
				decompressFrame( playbackBinkHandle, p_image, true );
				showFrame( playbackBinkHandle, p_image );

				if( playbackBinkHandle->FrameNum >= ( playbackBinkHandle->Frames - 1 ))
				{
					break;
				}

				// Check for a button being pressed to skip out of movie playback.
				sio_manager->ProcessDevices();

				// If running a demo movie, then pad or mem card insertions must make it quit. (TRC requirement)
				if( running_demo_movie )
				{
					// Use the XGetDeviceChanges function to determine if a card or pad has been inserted ...
					DWORD insertions=0;
					DWORD removals=0;
					if( XGetDeviceChanges( XDEVICE_TYPE_MEMORY_UNIT, &insertions, &removals ))
					{
						if( insertions )
						{
							break;
						}	
					}	
					if( XGetDeviceChanges( XDEVICE_TYPE_GAMEPAD, &insertions, &removals ))
					{
						if( insertions )
						{
							break;
						}	
					}	
				}				

				bool quit = false;
				for( int d = 0; d < 4; ++d )
				{
					SIO::Device* p_device = sio_manager->GetDeviceByIndex( d );
					if( p_device )
					{
						unsigned char* p_data = p_device->GetControlData();

						if( sDebounceCheck( &debounce_flags[d], p_data, 0x0800 ) ||	// Start
							sDebounceCheck( &debounce_flags[d], p_data, 0x0040 ))		// A
						{
							quit = true;
							break;
						}							
						
						// If running a demo movie, then these buttons must also make it quit. (TRC requirement)
						if( running_demo_movie )
						{
							if( sDebounceCheck(&debounce_flags[d], p_data, 0x0100) ||	// Back
								sDebounceCheck(&debounce_flags[d], p_data, 0x0020) ||	// B
								sDebounceCheck(&debounce_flags[d], p_data, 0x0080) ||	// X
								sDebounceCheck(&debounce_flags[d], p_data, 0x0010) ||	// Y
								sDebounceCheck(&debounce_flags[d], p_data, 0x0002) ||	// Black
								sDebounceCheck(&debounce_flags[d], p_data, 0x0001) ||	// White
								sDebounceCheck(&debounce_flags[d], p_data, 0x0200) ||	// Thumbstick 1
								sDebounceCheck(&debounce_flags[d], p_data, 0x0400 ))	// Thumbstick 2
							{
								quit = true;
								break;
							}							
						}
					}
				}
				if( quit )
				{
					break;
				}
			}
		}
	}

	closePlaybackImage( p_image );
	BinkClose( playbackBinkHandle );
	playbackBinkHandle = NULL;

	// Switch the presentation interval bacl to what it was.
	D3DDevice_SetRenderState( D3DRS_PRESENTATIONINTERVAL, presentation_interval );
}

} // namespace Flx

