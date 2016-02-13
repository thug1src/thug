#include "nx_init.h"
#include "instance.h"
#include "render.h"
#include "sys/ngc/p_buffer.h"


namespace NxNgc
{

sEngineGlobals	EngineGlobals;
	
	


void InitialiseRenderstates( void )
{
//	EngineGlobals.lighting_enabled		= false;
//	D3DDevice_SetRenderState( D3DRS_LIGHTING, FALSE );
//
//	EngineGlobals.cull_mode				= D3DCULL_NONE;
//	D3DDevice_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
//
//	EngineGlobals.dither_enable			= TRUE;
//	D3DDevice_SetRenderState( D3DRS_DITHERENABLE, TRUE );
//
//	EngineGlobals.z_test_enabled		= TRUE;
//	D3DDevice_SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
//
//	EngineGlobals.z_write_enabled		= TRUE;
//	D3DDevice_SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
//	
//	EngineGlobals.alpha_blend_enable	= TRUE;
//	D3DDevice_SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
//
//	EngineGlobals.alpha_test_enable		= TRUE;
//	D3DDevice_SetRenderState( D3DRS_ALPHATESTENABLE, TRUE );
//
//	EngineGlobals.alpha_ref				= 0;
//	D3DDevice_SetRenderState( D3DRS_ALPHAREF, 0x00 );
//
//	D3DDevice_SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
//    D3DDevice_SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
//    D3DDevice_SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR );
//
//    D3DDevice_SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP );
//    D3DDevice_SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP );
//
//    D3DDevice_SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL );
//    D3DDevice_SetRenderState( D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL );
//    D3DDevice_SetRenderState( D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL );
//
//	// Set these up so they will get reset first time through.
//	EngineGlobals.blend_mode_value			= 0xDEADBABE;
//	EngineGlobals.blend_mode_fixed_alpha	= 0xDEADBABE;

	init_render_system();
}


void InitialiseEngine( void )
{

//	D3DPRESENT_PARAMETERS   params;
//	DWORD					video_flags = XGetVideoFlags();
//
//	ZeroMemory( &params, sizeof( D3DPRESENT_PARAMETERS ));
//
//	// This setting required for any multisample presentation.
//	params.SwapEffect						= D3DSWAPEFFECT_DISCARD;
//
//	// Let D3D create the depth-stencil buffer for us.
//	params.EnableAutoDepthStencil			= TRUE;
//
//	// Select default refresh rate and presentation interval. Note: When we switch to the December SDK
//	// we can use the ONE_OR_IMMEDIATE value (if the tearing looks okay).
//	params.FullScreen_RefreshRateInHz		= D3DPRESENT_RATE_DEFAULT;
//	params.FullScreen_PresentationInterval	= D3DPRESENT_INTERVAL_ONE;
//
//	// Set up the back buffer format.
//	params.BackBufferCount					= 1;
//	params.BackBufferWidth					= 640;
//	params.BackBufferHeight					= 480;
////	params.BackBufferFormat					= D3DFMT_LIN_R5G6B5;
//	params.BackBufferFormat					= D3DFMT_X8R8G8B8;
//
//	// Set up the Z-stencil buffer format and multisample format.
//	params.AutoDepthStencilFormat			= D3DFMT_D24S8;
//    params.MultiSampleType					= D3DMULTISAMPLE_2_SAMPLES_MULTISAMPLE_LINEAR;
//
//	// Set flag for progrssive scan where appropriate.
//	if( video_flags & XC_VIDEO_FLAGS_HDTV_480p )
//	{
//		params.Flags						= D3DPRESENTFLAG_PROGRESSIVE;
//	}
//	else
//	{
//		params.Flags						= D3DPRESENTFLAG_INTERLACED;
//	}
//	
//	if( D3D_OK != Direct3D_CreateDevice(	D3DADAPTER_DEFAULT,
//											D3DDEVTYPE_HAL,
//											NULL,
//											D3DCREATE_HARDWARE_VERTEXPROCESSING,	// Note: may want to consider adding the PUREDEVICE flag here also.
//											&params,
//											&EngineGlobals.p_Device ))
//	{
//		// Failed to start up engine. Bad!
//		return;
//	}
//
//	// Obtain pointers to the render and Z-stencil surfaces. Doing this increases their reference counts, so release
//	// them following the operation.
//	D3DDevice_GetRenderTarget( &EngineGlobals.p_RenderSurface );
//	D3DDevice_GetDepthStencilSurface( &EngineGlobals.p_ZStencilSurface );
//
//    D3DResource_Release((D3DResource*)&EngineGlobals.p_RenderSurface );
//    D3DResource_Release((D3DResource*)&EngineGlobals.p_ZStencilSurface );
//
	// Set default directional lights
	EngineGlobals.light_x[0] = -0.5f;		// Dir0
	EngineGlobals.light_y[0] = -0.8660254f;
	EngineGlobals.light_z[0] = 0.0f;
	EngineGlobals.diffuse_light_color[0] = (GXColor){143,143,143,255};
	
	EngineGlobals.light_x[1] = 1.0f;		// Dir1 
	EngineGlobals.light_y[1] = 0.0f;
	EngineGlobals.light_z[1] = 0.0f;
	EngineGlobals.diffuse_light_color[1] = (GXColor){0,0,0,255};

	// Set default ambient light.
	EngineGlobals.ambient_light_color = (GXColor){150,150,150,255};

	EngineGlobals.gpuBusy = false;

	EngineGlobals.screen_brightness = 1.0f;

	EngineGlobals.frameCount = 0;

	EngineGlobals.use_480p			= false;
	EngineGlobals.use_60hz			= false;
	EngineGlobals.letterbox_active	= false;

	EngineGlobals.reduceColors = true;

	// Set our renderstate to a known state.
	InitialiseRenderstates();

	// Create our instance table.
	InitialiseInstanceTable();

	EngineGlobals.disableReset = false;
	EngineGlobals.resetToIPL = false; 

	NsBuffer::init( ( 1024 * 512 ) - ( 50 * 1024 ) );
}


} // namespace NxNgc

