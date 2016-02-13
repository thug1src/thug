#include <xtl.h>

// Include the following two files for detailed timing data collection.
//#include <xbdm.h>
//#include <d3d8perf.h>

#include "sys/config/config.h"
#include "nx_init.h"
#include "anim.h"
#include "chars.h"
#include "scene.h"
#include "render.h"
#include "instance.h"
#include "gamma.h"
#include "grass.h"

namespace NxXbox
{

sEngineGlobals	EngineGlobals;


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void InitialiseRenderstates( void )
{
	D3DDevice_SetRenderState( D3DRS_LOCALVIEWER,		FALSE );

	D3DDevice_SetRenderState( D3DRS_COLORVERTEX,		FALSE );

	EngineGlobals.lighting_enabled		= false;
	D3DDevice_SetRenderState( D3DRS_LIGHTING,			FALSE );

	EngineGlobals.specular_enabled		= 0;
	D3DDevice_SetRenderState( D3DRS_SPECULARENABLE,		FALSE );
	
	EngineGlobals.cull_mode				= D3DCULL_CW;
	D3DDevice_SetRenderState( D3DRS_CULLMODE,			D3DCULL_CW );

	EngineGlobals.allow_envmapping		= true;

	EngineGlobals.dither_enable			= TRUE;
	D3DDevice_SetRenderState( D3DRS_DITHERENABLE,		TRUE );

	EngineGlobals.z_test_enabled		= TRUE;
	D3DDevice_SetRenderState( D3DRS_ZFUNC,				D3DCMP_LESSEQUAL );

	EngineGlobals.z_write_enabled		= TRUE;
	D3DDevice_SetRenderState( D3DRS_ZWRITEENABLE,		TRUE );
	
	EngineGlobals.alpha_blend_enable	= 1;
	D3DDevice_SetRenderState( D3DRS_ALPHABLENDENABLE,	TRUE );

	EngineGlobals.alpha_test_enable		= 1;
	D3DDevice_SetRenderState( D3DRS_ALPHATESTENABLE,	TRUE );

	D3DDevice_SetRenderState( D3DRS_ALPHAFUNC,			D3DCMP_GREATEREQUAL );

	EngineGlobals.alpha_ref				= 0;
	D3DDevice_SetRenderState( D3DRS_ALPHAREF,			0x00 );

	for( int stage = 0; stage < 4; ++stage )
	{
		EngineGlobals.uv_addressing[stage]		= 0x00000000UL;
		EngineGlobals.mip_map_lod_bias[stage]	= 0x00000000UL;
		EngineGlobals.p_texture[stage]			= NULL;
		EngineGlobals.min_mag_filter[stage]		= 0x00010000UL;
		EngineGlobals.color_sign[stage]			= 0x00000000UL;

		D3DDevice_SetTextureStageState( stage, D3DTSS_MAGFILTER,		D3DTEXF_LINEAR );
		D3DDevice_SetTextureStageState( stage, D3DTSS_MIPFILTER,		D3DTEXF_LINEAR );

		if( stage == 0 )
		{
			// If we are running in 720p or 1080i mode, need max pixel pushing power - avoid anisotropic filtering.
			// MSM PERFCHANGE
			if( EngineGlobals.backbuffer_height > 480 )
			{
				D3DDevice_SetTextureStageState( stage, D3DTSS_MINFILTER,		D3DTEXF_LINEAR );
			}
			else
			{
				D3DDevice_SetTextureStageState( stage, D3DTSS_MINFILTER,		D3DTEXF_ANISOTROPIC );
				D3DDevice_SetTextureStageState( stage, D3DTSS_MAXANISOTROPY, 3 );
			}
		}
		else
		{
			D3DDevice_SetTextureStageState( stage, D3DTSS_MINFILTER,		D3DTEXF_LINEAR );
		}
		D3DDevice_SetTextureStageState( stage, D3DTSS_ADDRESSU,			D3DTADDRESS_WRAP );
		D3DDevice_SetTextureStageState( stage, D3DTSS_ADDRESSV,			D3DTADDRESS_WRAP );
	}

	// Set up material for specular properties for fixed function pipeline.
	D3DMATERIAL8	test_mat;
	ZeroMemory( &test_mat, sizeof( D3DMATERIAL8 ));
	D3DDevice_SetMaterial( &test_mat );

	D3DDevice_SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE,	D3DMCS_COLOR1 );
    D3DDevice_SetRenderState( D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL );
    D3DDevice_SetRenderState( D3DRS_AMBIENTMATERIALSOURCE,	D3DMCS_COLOR1 );
    D3DDevice_SetRenderState( D3DRS_EMISSIVEMATERIALSOURCE,	D3DMCS_MATERIAL );

	// Set these up so they will get reset first time through.
	EngineGlobals.blend_mode_value			= 0xDEADBABE;
	EngineGlobals.blend_op					= 0xDEADBABE;
	EngineGlobals.src_blend					= 0xDEADBABE;
	EngineGlobals.dest_blend				= 0xDEADBABE;

	EngineGlobals.screenshot_name[0]		= 0;
	
	// Build the required vertex shaders.
	CreateWeightedMeshVertexShaders();

	// Build required pixel shaders.
	create_pixel_shaders();

	// Set pixel shader constants.
	EngineGlobals.pixel_shader_constants[16]	= 0.0f;
	EngineGlobals.pixel_shader_constants[17]	= 0.5f;
	EngineGlobals.pixel_shader_constants[18]	= 0.0f;	// Fog denisty.
	EngineGlobals.pixel_shader_constants[19]	= 0.5f;	// Constant 0.5

	EngineGlobals.pixel_shader_override			= 0;
	EngineGlobals.pixel_shader_id				= 0;
	EngineGlobals.upload_pixel_shader_constants = false;
	EngineGlobals.is_orthographic				= false;
	EngineGlobals.custom_pipeline_enabled		= false;
	EngineGlobals.vertex_shader_override		= 0;
	EngineGlobals.texture_stage_override		= 0;
	EngineGlobals.material_override				= 0;
	EngineGlobals.blend_mode_override			= 0;

	EngineGlobals.clear_color_buffer			= true;
	EngineGlobals.clear_color					= 0x00506070;
	EngineGlobals.letterbox_active				= false;

	// Set default directional lights
	EngineGlobals.directional_light_color[0]	= -0.5f;		// Dir0
	EngineGlobals.directional_light_color[1]	= -0.8660254f;
	EngineGlobals.directional_light_color[2]	= 0.0f;
	EngineGlobals.directional_light_color[3]	= 0.0f;
	EngineGlobals.directional_light_color[4]	= 0.586f;		// Col0
	EngineGlobals.directional_light_color[5]	= 0.586f;
	EngineGlobals.directional_light_color[6]	= 0.586f;
	EngineGlobals.directional_light_color[7]	= 1.0f;
	
	EngineGlobals.directional_light_color[8]	= 1.0f;			// Dir1
	EngineGlobals.directional_light_color[9]	= 0.0f;
	EngineGlobals.directional_light_color[10]	= 0.0f;
	EngineGlobals.directional_light_color[11]	= 0.0f;
	EngineGlobals.directional_light_color[12]	= 0.0f;			// Col1
	EngineGlobals.directional_light_color[13]	= 0.0f;
	EngineGlobals.directional_light_color[14]	= 0.0f;
	EngineGlobals.directional_light_color[15]	= 1.0f;
	
	EngineGlobals.directional_light_color[16]	= 1.0f;			// Dir2
	EngineGlobals.directional_light_color[17]	= 0.0f;
	EngineGlobals.directional_light_color[18]	= 0.0f;
	EngineGlobals.directional_light_color[19]	= 0.0f;
	EngineGlobals.directional_light_color[20]	= 0.0f;			// Col2
	EngineGlobals.directional_light_color[21]	= 0.0f;
	EngineGlobals.directional_light_color[22]	= 0.0f;
	EngineGlobals.directional_light_color[23]	= 1.0f;

	// Set default ambient light.
	EngineGlobals.ambient_light_color[0]		= 0.5865f;
	EngineGlobals.ambient_light_color[1]		= 0.5865f;
	EngineGlobals.ambient_light_color[2]		= 0.5865f;
	EngineGlobals.ambient_light_color[3]		= 1.0f;
 
	EngineGlobals.fog_enabled					= false;
	EngineGlobals.fog_color						= 0x00000000UL;
	EngineGlobals.fog_start						= FEET_TO_INCHES( -20.0f );
	EngineGlobals.fog_end						= FEET_TO_INCHES( -2050.0f );
	
	D3DDevice_SetRenderState( D3DRS_FOGENABLE,		EngineGlobals.fog_enabled );
	D3DDevice_SetRenderState( D3DRS_FOGTABLEMODE,	D3DFOG_LINEAR );
//	D3DDevice_SetRenderState( D3DRS_FOGTABLEMODE,	D3DFOG_EXP );
//	D3DDevice_SetRenderState( D3DRS_FOGSTART,		*((DWORD*)( &EngineGlobals.fog_start )));
//	D3DDevice_SetRenderState( D3DRS_FOGEND,			*((DWORD*)( &EngineGlobals.fog_end )));
	D3DDevice_SetRenderState( D3DRS_FOGCOLOR,		EngineGlobals.fog_color );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void InitialiseEngine( void )
{
	D3DPRESENT_PARAMETERS   params;
	DWORD					video_flags = XGetVideoFlags();

	EngineGlobals.loadingbar_timer_event	= 0;
	
	// Setup default values for the screen conversion macro.
	EngineGlobals.screen_conv_x_multiplier	= 640.0f / 640.0f;
	EngineGlobals.screen_conv_y_multiplier	= 480.0f / 480.0f;
	EngineGlobals.screen_conv_x_offset		= 0;
	EngineGlobals.screen_conv_y_offset		= 16;
	
	ZeroMemory( &params, sizeof( D3DPRESENT_PARAMETERS ));

	// This setting required for any multisample presentation.
	params.SwapEffect						= D3DSWAPEFFECT_DISCARD;

	// Let D3D create the depth-stencil buffer for us.
	params.EnableAutoDepthStencil			= TRUE;

	// Select default refresh rate and presentation interval. Note: When we switch to the December SDK
	// we can use the ONE_OR_IMMEDIATE value (if the tearing looks okay).
	if(( Config::GetDisplayType() == Config::DISPLAY_TYPE_PAL ) && ( Config::FPS() == 60 ))
	{
		// PAL 60Hz has been selected - need to set this refresh rate explicitly.
		params.FullScreen_RefreshRateInHz	= 60;
	}
	else
	{
		params.FullScreen_RefreshRateInHz	= D3DPRESENT_RATE_DEFAULT;
	}
//	params.FullScreen_PresentationInterval	= D3DPRESENT_INTERVAL_ONE;
	params.FullScreen_PresentationInterval	= D3DPRESENT_INTERVAL_ONE_OR_IMMEDIATE;
//	params.FullScreen_PresentationInterval	= D3DPRESENT_INTERVAL_IMMEDIATE;

	// Set up the back buffer format.
	params.BackBufferCount					= 1;
	params.BackBufferWidth					= 640;
	params.BackBufferHeight					= 480;
	params.BackBufferFormat					= D3DFMT_LIN_X8R8G8B8;

	// Set up the Z-stencil buffer format and multisample format.
	params.AutoDepthStencilFormat			= D3DFMT_D24S8;
//	params.MultiSampleType					= D3DMULTISAMPLE_NONE;
	params.MultiSampleType					= D3DMULTISAMPLE_2_SAMPLES_MULTISAMPLE_LINEAR;
//	params.MultiSampleType					= D3DMULTISAMPLE_2_SAMPLES_MULTISAMPLE_QUINCUNX;
//	params.MultiSampleType					= D3DMULTISAMPLE_4_SAMPLES_SUPERSAMPLE_LINEAR;

	// Set flag for widescreen where appropriate.
	if( video_flags & XC_VIDEO_FLAGS_WIDESCREEN )
	{
		params.Flags			|= D3DPRESENTFLAG_WIDESCREEN;

		// Optionally set up 720×480 back buffer.
		// Set up 16:9 projection transform.
	}

	
	// Set flag for progrssive scan where appropriate.
	if( video_flags & XC_VIDEO_FLAGS_HDTV_720p )
	{
		params.Flags			|= D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
		params.BackBufferWidth	= 1280;
		params.BackBufferHeight	= 720;

		// Turn off FSAA.
		params.MultiSampleType	= D3DMULTISAMPLE_NONE;

		EngineGlobals.screen_conv_x_multiplier	= 1280.0f / 704.0f;
		EngineGlobals.screen_conv_y_multiplier	= 720.0f / 480.0f;
		EngineGlobals.screen_conv_x_offset		= 32;
		EngineGlobals.screen_conv_y_offset		= 32;
	}
	else if( video_flags & XC_VIDEO_FLAGS_HDTV_480p )
	{
		params.Flags			|= D3DPRESENTFLAG_PROGRESSIVE;
	}
//	else if( video_flags & XC_VIDEO_FLAGS_HDTV_1080i )
//	{
//		params.Flags							|= D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_WIDESCREEN | D3DPRESENTFLAG_FIELD;
//		params.BackBufferWidth					= 1920;
//		params.BackBufferHeight					= 540;
//		params.BackBufferFormat					= D3DFMT_LIN_R5G6B5;
//		params.AutoDepthStencilFormat			= D3DFMT_D16;
//		params.FullScreen_PresentationInterval	= D3DPRESENT_INTERVAL_TWO;
//
//		// Turn off FSAA.
//		params.MultiSampleType	= D3DMULTISAMPLE_NONE;
//
//		EngineGlobals.screen_conv_x_multiplier	= 1920.0f / 704.0f;
//		EngineGlobals.screen_conv_y_multiplier	= 1080.0f / 480.0f;
//		EngineGlobals.screen_conv_x_offset		= 32;
//		EngineGlobals.screen_conv_y_offset		= 16;
//	}
	else
	{
		params.Flags			|= D3DPRESENTFLAG_INTERLACED;
	}
	
	if( params.BackBufferWidth == 640 )
	{
		params.Flags			|= D3DPRESENTFLAG_10X11PIXELASPECTRATIO;
	}
	
	// The default push buffer size is 512k. Double this to reduce stalls from filling the push buffer.
	Direct3D_SetPushBufferSize( 2 * 512 * 1024, 32 * 1024 );
	
	if( D3D_OK != Direct3D_CreateDevice(	D3DADAPTER_DEFAULT,
											D3DDEVTYPE_HAL,
											NULL,
											D3DCREATE_HARDWARE_VERTEXPROCESSING,	// Note: may want to consider adding the PUREDEVICE flag here also.
											&params,
											&EngineGlobals.p_Device ))
	{
		// Failed to start up engine. Bad!
		exit( 0 );
	}

	// Also create the render surface we will use when doing screen blur. (Creating this at 32bit depth also, since still
	// takes up less memory than 720p buffers).
	if( params.BackBufferWidth <= 640 )
	{
		D3DDevice_CreateRenderTarget( 640, 480, D3DFMT_LIN_X8R8G8B8, 0, 0, &EngineGlobals.p_BlurSurface[0] );
		D3DDevice_CreateRenderTarget( 320, 240, D3DFMT_LIN_X8R8G8B8, 0, 0, &EngineGlobals.p_BlurSurface[1] );
		D3DDevice_CreateRenderTarget( 160, 120, D3DFMT_LIN_X8R8G8B8, 0, 0, &EngineGlobals.p_BlurSurface[2] );
		D3DDevice_CreateRenderTarget(  80,  60, D3DFMT_LIN_X8R8G8B8, 0, 0, &EngineGlobals.p_BlurSurface[3] );
	}
	
	// Obtain pointers to the render and Z-stencil surfaces. Doing this increases their reference counts, so release
	// them following the operation.
	D3DDevice_GetRenderTarget( &EngineGlobals.p_RenderSurface );
	D3DDevice_GetDepthStencilSurface( &EngineGlobals.p_ZStencilSurface );

	LPDIRECT3DSURFACE8 pBackBuffer;
    D3DDevice_GetBackBuffer( 0, 0, &pBackBuffer );
	
	// Get back buffer information.
	EngineGlobals.backbuffer_width	= params.BackBufferWidth;
	EngineGlobals.backbuffer_height	= params.BackBufferHeight;
	EngineGlobals.backbuffer_format	= params.BackBufferFormat;
	EngineGlobals.zstencil_depth	= ( params.AutoDepthStencilFormat == D3DFMT_D16 ) ? 16 : 32;
	
	// Get blur buffer information.
	if( EngineGlobals.p_BlurSurface[0] )
	{
		D3DSURFACE_DESC blur_surface_desc;
		EngineGlobals.p_BlurSurface[0]->GetDesc( &blur_surface_desc );
		EngineGlobals.blurbuffer_format	= blur_surface_desc.Format;
	}
	
	// Set our renderstate to a known state.
	InitialiseRenderstates();

	// Set default gamma values.
	SetGammaNormalized( 0.14f, 0.13f, 0.12f );

	// Initialise the memory resident font, used for fatal i/o error messages etc.
	EngineGlobals.p_memory_resident_font = InitialiseMemoryResidentFont();

	// Code to enable detailed timing. Need to link with d3d8i.lib.
//	DmEnableGPUCounter( TRUE );
//	D3DPERF_SetShowFrameRateInterval( 1000 );
//	D3DPERF_GetStatistics()->m_dwDumpFPSInfoMask |= D3DPERF_DUMP_FPS_PERFPROFILE;

	// Now that the D3DDevice is created, it's safe to install vsync handlers.
	Tmr::InstallVSyncHandlers();

	// Load up the bump textures.
//	LoadBumpTextures();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void FatalFileError( uint32 error )
{
	static char*	p_error_message_english[2]	= {	"There's a problem with the disc you're using.",
													"It may be dirty or damaged." };
	static char*	p_error_message_french[2]	= {	"Le disque utilisé présente une anomalie.",
													"Il est peut-être sale ou endommagé." };
	static char*	p_error_message_german[2]	= {	"Bei der benutzten CD ist ein Problem aufgetreten.",
													"Möglicherweise ist sie verschmutzt oder beschädigt." };

	// Turn off the loading bar if it is active.
	if( EngineGlobals.loadingbar_timer_event != 0 )
	{
		timeKillEvent( EngineGlobals.loadingbar_timer_event );
		EngineGlobals.loadingbar_timer_event = 0;
	}

	// Ensure the graphics device has been initialised at this point.
	if( EngineGlobals.p_Device == NULL )
	{
		InitialiseEngine();
	}

	// Wait for any rendering to complete.
	EngineGlobals.p_Device->BlockUntilIdle();

	char*	p_error_message[2];
	switch( Config::GetLanguage())
	{
		case Config::LANGUAGE_FRENCH:
		{
			p_error_message[0] = p_error_message_french[0];
			p_error_message[1] = p_error_message_french[1];
			break;
		}
		case Config::LANGUAGE_GERMAN:
		{
			p_error_message[0] = p_error_message_german[0];
			p_error_message[1] = p_error_message_german[1];
			break;
		}
		default:
		{
			p_error_message[0] = p_error_message_english[0];
			p_error_message[1] = p_error_message_english[1];
			break;
		}

	}

	// Set up the text string used for the error message.
	SText error_text;
	error_text.mp_font		= (SFont*)EngineGlobals.p_memory_resident_font;
	error_text.m_xpos		= 48.0f;
	error_text.m_ypos		= 128.0f;
	error_text.m_xscale		= 0.8f;
	error_text.m_yscale		= 1.0f;
	error_text.m_rgba		= 0x80808080;
	error_text.mp_next		= NULL;

	set_texture( 1, NULL );
	set_texture( 2, NULL );
	set_texture( 3, NULL );

	// Want an infinite loop here.
	while( true )
	{
		D3DDevice_Swap( D3DSWAP_DEFAULT );

		// Now that the swap instruction has been pushed, clear the buffer for next frame.
		D3DDevice_Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0x00000000, 1.0f, 0 );

		set_blend_mode( vBLEND_MODE_BLEND );
		set_texture( 0, NULL );

		set_render_state( RS_UVADDRESSMODE0,	0x00010001UL );
		set_render_state( RS_ZBIAS,				0 );
		set_render_state( RS_ALPHACUTOFF,		1 );
		set_render_state( RS_ZWRITEENABLE,		0 );

		D3DDevice_SetTextureStageState( 0, D3DTSS_COLORSIGN, D3DTSIGN_RUNSIGNED | D3DTSIGN_GUNSIGNED | D3DTSIGN_BUNSIGNED );
		D3DDevice_SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
		D3DDevice_SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU | 0 );

		error_text.mp_string	= p_error_message[0];
		error_text.m_ypos		= error_text.m_ypos - 16.0f;
		error_text.BeginDraw();
		error_text.Draw();
		error_text.EndDraw();

		error_text.mp_string	= p_error_message[1];
		error_text.m_ypos		= error_text.m_ypos + 16.0f;
		error_text.BeginDraw();
		error_text.Draw();
		error_text.EndDraw();
	}
}



} // namespace NxXbox

