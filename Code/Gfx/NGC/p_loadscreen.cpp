///*****************************************************************************
//**																			**
//**			              Neversoft Entertainment.			                **
//**																		   	**
//**				   Copyright (C) 2000 - All Rights Reserved				   	**
//**																			**
//******************************************************************************
//**																			**
//**	Project:		Gfx														**
//**																			**
//**	Module:			LoadScreen			 									**
//**																			**
//**	File name:		p_loadscreen.cpp										**
//**																			**
//**	Created by:		05/08/02	-	SPG										**
//**																			**
//**	Description:	PS2-specific loading screen calls						**
//**																			**
//*****************************************************************************/
//
///*****************************************************************************
//**							  	  Includes									**
//*****************************************************************************/
//
//#include <core/defines.h>
//
//#include <sys/file/filesys.h>
//#include <sys/timer.h>
//
//#include <gel/mainloop.h>
//
//#include <gfx/ngc/p_memview.h>
//#include <sys/ngc/p_display.h>
//#include <sys/ngc/p_camera.h>
//#include <sys/ngc/p_render.h>
//#include <sys/ngc/p_prim.h>
//#include <sys/ngc/p_texman.h>
//#include <sys/ngc/p_file.h>
//#include <sys/ngc/p_dvd.h>
//
//#include "sys/ngc/p_profile.h"
//
//#include <gfx/nx.h>
//
//NsProfile _gpu( "GPU", 256 );
//NsProfile _cpu( "CPU", 256 );
//static volatile int _open = 0;
//
//NsCamera			camera2D;
//int heaperror = 0;
//
///*****************************************************************************
//**								DBG Information								**
//*****************************************************************************/
//
//extern void	(*pIconCallback)( void );
//
////extern void* copyXFB;
////extern void* dispXFB;
////extern void* myXFB1;
////extern void* myXFB2;
//
//namespace LoadScreen
//{
//
///*****************************************************************************
//**								  Externals									**
//*****************************************************************************/
//
///*****************************************************************************
//**								   Defines									**
//*****************************************************************************/
//
//#define vMAX_PIXEL_DATA_SIZE	( 32767 * 16 )
//
//#define	ENABLE_LOADING_ICON		1
//
//
///*****************************************************************************
//**								Private Types								**
//*****************************************************************************/
//
//class LoadingIconData
//{
//public:
//	int			m_X;
//	int			m_Y;
//	int			m_FrameDelay;
//	NsTexture**	m_Rasters;
//	int			m_NumFrames;
//};
//
///*****************************************************************************
//**								 Private Data								**
//*****************************************************************************/
//
//static bool				s_loading_screen_on		= false;
//static OSThread			s_load_icon_thread;
//static OSAlarm			s_alarm;
//static LoadingIconData	s_load_icon_data		= { 0 };
////static char				s_load_icon_thread_stack[4096]	__attribute__ (( aligned( 16 )));
//static volatile bool	s_terminate_thread		= false;
//static volatile bool	s_terminate_thread_done	= false;
//static bool				s_thread_exists			= false;
//
///*****************************************************************************
//**								 Public Data								**
//*****************************************************************************/
//
///*****************************************************************************
//**							  Private Prototypes							**
//*****************************************************************************/
//
///*****************************************************************************
//**							  Private Functions								**
//*****************************************************************************/
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void s_thread_loading_icon_alarm_handler( OSAlarm* alarm, OSContext* context )
//{
//	// Check the thread has not been resumed yet...
//	if( OSIsThreadSuspended( &s_load_icon_thread ))
//	{
//		OSResumeThread( &s_load_icon_thread );
//	}
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void s_non_threaded_loading_icon( void )
//{
//	LoadingIconData* p_data = (LoadingIconData*)&s_load_icon_data;
//
//	static int			current_image = 0;
//	static Tmr::Time	time = 0;
//
//	Tmr::Time			time_now = Tmr::GetVblanks();
//
//	if(( time_now < time ) || ( time_now > ( time +	p_data->m_FrameDelay )))
//	{
//		time = time_now;
//
//		NsTexture* p_texture = p_data->m_Rasters[current_image];
//
//		NsDisplay::begin();
//
//		NsRender::begin();
//
////		NsPrim::begin();
//	    GXSetVtxAttrFmt( GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );		// 3 = positions & uvs.
//		GXSetVtxAttrFmt( GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0 );
//
//		camera2D.orthographic( 0, 0, 640, 448 );
//
//		NsRender::setBlendMode( NsBlendMode_None, p_texture, (unsigned char)0 );
//
//		GXClearVtxDesc();
//		GXSetVtxDesc( GX_VA_POS, GX_DIRECT );
//		GXSetVtxDesc( GX_VA_TEX0, GX_DIRECT );
//		GX::SetChanMatColor( GX_COLOR0, (GXColor){ 128,128,128,128 });
//		GX::SetChanCtrl( GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//
//		p_texture->upload( NsTexture_Wrap_Clamp );
//
//		GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
//		GXPosition3f32((float)p_data->m_X, (float)p_data->m_Y, -1.0f );
//		GXTexCoord2f32( 0.0f, 0.0f );
//		GXPosition3f32((float)p_data->m_X + (float)p_texture->m_width, (float)p_data->m_Y, -1.0f );
//		GXTexCoord2f32( 1.0f, 0.0f );
//		GXPosition3f32((float)p_data->m_X + (float)p_texture->m_width, (float)p_data->m_Y + (float)p_texture->m_height, -1.0f );
//		GXTexCoord2f32( 1.0f, 1.0f );
//		GXPosition3f32((float)p_data->m_X, (float)p_data->m_Y + (float)p_texture->m_height, -1.0f );
//		GXTexCoord2f32( 0.0f, 1.0f );
//		GXEnd();
//
////		NsPrim::end();
//
//		camera2D.end();
//
//		NsRender::end();
//
//		NsDisplay::end( false );
//
//		current_image = ( current_image + 1 ) % p_data->m_NumFrames ;
//	}
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void* s_threaded_loading_icon( void* data )
//{
//	LoadingIconData* p_data = (LoadingIconData*)data;
//
//	int		current_image = 0;
//
//	while( 1 )
//	{
//		if( s_terminate_thread )
//		{
//			// This thread is done...
//			s_terminate_thread_done	= true;
//			return NULL;
//		}
//
//		bool busy = /*NsModel::busy() | DVDError() |*/ heaperror;
//
//		// Don't want to do draw anything whilst display lists are being built. Come back in a bit.
//		if( !busy )
//		{
//			NsTexture* p_texture = p_data->m_Rasters[current_image];
//
//			NsDisplay::begin();
//			NsRender::begin();
//
//		    GXSetVtxAttrFmt( GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0 );		// 3 = positions & uvs.
//			GXSetVtxAttrFmt( GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0 );
//
//			camera2D.orthographic( 0, 0, 640, 448 );
//
//			NsRender::setBlendMode( NsBlendMode_None, p_texture, (unsigned char)0 );
//
//			GXClearVtxDesc();
//			GXSetVtxDesc( GX_VA_POS, GX_DIRECT );
//			GXSetVtxDesc( GX_VA_TEX0, GX_DIRECT );
//			GX::SetChanMatColor( GX_COLOR0, (GXColor){ 128,128,128,128 });
//			GX::SetChanCtrl( GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//
//			p_texture->upload( NsTexture_Wrap_Clamp );
//
//			GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
//			GXPosition3f32((float)p_data->m_X, (float)p_data->m_Y, -1.0f );
//			GXTexCoord2f32( 0.0f, 0.0f );
//			GXPosition3f32((float)p_data->m_X + (float)p_texture->m_width, (float)p_data->m_Y, -1.0f );
//			GXTexCoord2f32( 1.0f, 0.0f );
//			GXPosition3f32((float)p_data->m_X + (float)p_texture->m_width, (float)p_data->m_Y + (float)p_texture->m_height, -1.0f );
//			GXTexCoord2f32( 1.0f, 1.0f );
//			GXPosition3f32((float)p_data->m_X, (float)p_data->m_Y + (float)p_texture->m_height, -1.0f );
//			GXTexCoord2f32( 0.0f, 1.0f );
//			GXEnd();
//
//			camera2D.end();
//
//			NsRender::end();
//			NsDisplay::end( false );
//
//			current_image = ( current_image + 1 ) % p_data->m_NumFrames ;
//		}
//
//		// Go to sleep.
//		if( busy )
//		{
//			// Come back shortly.
//			OSSetAlarm( &s_alarm, OSMillisecondsToTicks( 50 ), s_thread_loading_icon_alarm_handler );
//		}
//		else
//		{
//			// Come back in the proper time.
//			OSSetAlarm( &s_alarm, OSMillisecondsToTicks( s_load_icon_data.m_FrameDelay ), s_thread_loading_icon_alarm_handler );
//		}
//
//		OSSuspendThread( &s_load_icon_thread );
//	}
//}
//
//
///*****************************************************************************
//**							  Public Functions								**
//*****************************************************************************/
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void SetLoadingIconProperties( int x, int y, int frame_delay, int num_frames, char* basename, char* ext )
//{
////	int i;
////
////	if( s_load_icon_data.m_NumFrames > 0  )
////	{   
////		for( i = 0; i < s_load_icon_data.m_NumFrames; i++ )
////		{
////			delete s_load_icon_data.m_Rasters[i];
////		}
////		delete [] s_load_icon_data.m_Rasters;
////	}
////
////	s_load_icon_data.m_X			= x;
////	s_load_icon_data.m_Y			= y;
////	s_load_icon_data.m_FrameDelay	= ( frame_delay * 1000 ) / 60;		// Convert from frames to milliseconds.
//////	s_load_icon_data.m_FrameDelay	= frame_delay;
////	s_load_icon_data.m_NumFrames	= num_frames;
////	s_load_icon_data.m_Rasters		= new NsTexture*[s_load_icon_data.m_NumFrames];
////
////	for( i = 0; i < s_load_icon_data.m_NumFrames; i++ )
////	{
////		NsFile file;
////
////		char image_name[64];
////		sprintf( image_name, "%s%d.%s", basename, i+1, ext );
////
//// 		file.open( image_name );
////		int filesize		= file.size();
////		uint8 *pData		= (uint8*)new uint8[filesize + 31];
////		pData				= (uint8*)OSRoundUp32B( pData );
////		file.read( pData, filesize );
////		file.close();
////
////		NsTexture *pTex		= (NsTexture*)pData;
////		pData				= (uint8*)&pTex[1];
////		pTex->m_pImage		= pData;
////		pData			   += ( pTex->m_width * pTex->m_height * pTex->m_depth ) / 8;
////		pTex->m_pPalette	= pData;
////
////		s_load_icon_data.m_Rasters[i] = pTex;
////	}
//}
//
//void _start( void )
//{
//	_gpu.start();
//	_open = 1;
//}
//
//void _end( void )
//{
//	_gpu.stop();
//	_open = 0;
//}
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void Display( char* filename, bool just_freeze, bool blank )
//{
////	float y = 0;
////
////	while ( 1 )
////	{
////		Nx::CEngine::sPreRender();		
////					 
//////		Nx::CEngine::sRenderWorld();		
////
//////--------------------------------------------------
////
////		NsCamera cam;
////		cam.orthographic( 0, 0, 640, 448 );
////
////		// Draw the screen.
////		NsPrim::begin();
////
////		cam.begin();
////
////		NsRender::setBlendMode( NsBlendMode_None, NULL, (unsigned char)0 );
////
////		NsPrim::box( 64, 64 + y, 640-64, 64+80 + y, (GXColor){0,32,64,128} );
////
////		y += 1.0f;
////		if ( y > 256.0f ) y -= 256.0f;
////
////		cam.end();
////
////		NsPrim::end();
////
//////--------------------------------------------------
////
////		Nx::CEngine::sPostRender();		
////	}
//	
////	Spt::SingletonPtr< Mlp::Manager > mlp_man;
////	Tmr::Time start_time;
////
////#if 0
////	NsDisplay::setRenderStartCallback( _start );
////	NsDisplay::setRenderEndCallback( _end );
////
////	camera2D.orthographic ( 0, 0, 640, 448 );
////	int col = 0;
////
////	NsDisplay::setRenderStartCallback( _start );
////	NsDisplay::setRenderEndCallback( _end );
////
////	while ( 1 ) {
////		NsDisplay::setBackgroundColor ( (GXColor){0,0,255,0} );
////		col+=2;
////		if (col == 256) col = 0;
////
////		// Draw the screen.
////		NsDisplay::begin();
////		NsRender::begin();
////		NsPrim::begin();
////
////		camera2D.begin();
////
////		_cpu.start();
////
////		NsRender::setBlendMode( NsBlendMode_None, NULL, (unsigned char)128 );
////
////		
////		GXClearVtxDesc();
////		GXSetVtxDesc( GX_VA_POS, GX_DIRECT );
////
////		// Set material color.
////		GX::SetChanMatColor( GX_COLOR0A0, (GXColor){255,255,col,128} );
////		GX::SetChanAmbColor( GX_COLOR0A0, (GXColor){255,255,255,255} );
////
////		GX::SetChanCtrl( GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
////
////		
////		for (int lp = 0; lp < 3000; lp++) {
////			float l = 64+col;
////			float t = 64;
////			float r = 64+2+col;
////			float b = 64+2;
//////			NsPrim::box ( 64+col, 64, 80+col, 80, (GXColor){255,255,col,128} );
////		
////			// Send coordinates.
////			GXBegin( GX_QUADS, GX_VTXFMT0, 4 ); 
////				GXPosition3f32(l, t, -1.0f);
////				GXPosition3f32(r, t, -1.0f);
////				GXPosition3f32(r, b, -1.0f);
////				GXPosition3f32(l, b, -1.0f);
////			GXEnd();
////		}
////
////		_cpu.stop();
////
//////		while ( _open );
////		_gpu.histogram( 576, 32, 608, 416, (GXColor){255,0,0,128} );
////		_cpu.histogram( 544, 32, 576, 416, (GXColor){0,255,0,128} );
////
////		camera2D.end();
////
////		NsPrim::end();
////		NsRender::end();
////		NsDisplay::end( true );
////	}
////#endif
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////
////	if( s_loading_screen_on )
////	{
////		return;
////	}
////
////	start_time = Tmr::GetTime();
////
////	if( !just_freeze && !blank )
////	{
////		// Load in the screen.
////		NsFile	file;
////		NsTexture * pTex = (NsTexture *)file.load( filename );
////
////		// Skip data.
////		uint8 * pData = (uint8 *)&pTex[1];
////		pTex->m_pImage = pData;
////		pData += ( pTex->m_width * pTex->m_height * pTex->m_depth ) / 8;
////		pTex->m_pPalette = pData;
////
////		camera2D.orthographic ( 0, 0, 640, 448 );
////		NsDisplay::setBackgroundColor ( (GXColor){0,0,0,0} );
////
////		for ( int lp = 0; lp < 2; lp++ )
////		{
////			// Draw the screen.
////			NsDisplay::begin();
////			NsRender::begin();
////			NsPrim::begin();
////
////			camera2D.begin();
////
////			NsRender::setBlendMode( NsBlendMode_None, pTex, (unsigned char)0 );
////			NsPrim::sprite ( pTex, 0.0f, 0.0f, -1.0f );
////
////			camera2D.end();
////
////			NsPrim::end();
////			NsRender::end();
////			NsDisplay::end( false );
////		}
////
////		// Wait for it to finish.
////		NsDisplay::flush();
////
////		// We're done.
////		delete pTex;
////
////		// Start the thread to display the animated loading icon.
////#		if ENABLE_LOADING_ICON
//////		pIconCallback = s_non_threaded_loading_icon;
////		BOOL rv = true;
////
////		s_terminate_thread = false;
////
////		memset( &s_load_icon_thread, 0, sizeof( OSThread ));
////		rv = OSCreateThread(	&s_load_icon_thread,
////								s_threaded_loading_icon,										// Entry function.
////								&s_load_icon_data,												// Argument for start function.
////								s_load_icon_thread_stack + sizeof( s_load_icon_thread_stack ),	// Stack base (stack grows down).
////								sizeof( s_load_icon_thread_stack ),								// Stack size in bytes.
////								1,																// Thread priority.
////								OS_THREAD_ATTR_DETACH  );										// Thread attributes.
////		Dbg_MsgAssert( rv, ( "Failed to create thread" ));
////
////		if( rv )
////		{
////			s_thread_exists = true;
//////			OSResumeThread( &s_load_icon_thread );
////			OSSetAlarm( &s_alarm, OSMillisecondsToTicks( s_load_icon_data.m_FrameDelay ), s_thread_loading_icon_alarm_handler );
////		}
////#		endif // ENABLE_LOADING_ICON
////	}
////	else
////	{
////		// Wait a couple of VBlanks for RW stuff to disappear
////		// Mick: so there is no pending page flip
////		while( Tmr::GetVblanks() - start_time < 2 );		
////		
////		// not loading, just freezing, maybe should copy current buffer over second buffer?
////		if( blank )
////		{
////			// Need to clear the screen here.
//////			RpSkySuspend( );	 // Turn or Renderwware inputs and stuff
//////		    Flx::clearGsMem(0x00, 0x00, 0x00, 640, 448*2);
//////			RpSkyResume( );		 // and trun em back on, with the screen nice and blank...
////		}
////	}
////	s_loading_screen_on = true;
////
////	mlp_man->PauseDisplayTasks( true ); 
//}
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//
//void Hide( void )
//{
//#	if ENABLE_LOADING_ICON
//	if( s_thread_exists )
//	{
//		s_terminate_thread_done	= false;
//		s_terminate_thread = true;
//
//		// Cancel the alarm and resume the thread.
////		OSCancelAlarm( &s_alarm );
////		OSResumeThread( &s_load_icon_thread );
//
//		while( !s_terminate_thread_done );
//
//		s_thread_exists = false;
////		pIconCallback = NULL;
//
//		VISetBlack( TRUE );
//		VIFlush();
//		VIWaitForRetrace();
//		NsDisplay::flush();
//		VISetBlack( FALSE );
//		VIFlush();
//		VIWaitForRetrace();
//	}
//#	endif // ENABLE_LOADING_ICON
//
//	Spt::SingletonPtr< Mlp::Manager >	mlp_man;
//	mlp_man->PauseDisplayTasks( false ); 
//
//	if( !s_loading_screen_on )
//	{
//		return;
//	}
//
//	s_loading_screen_on = false;
//}
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//
//void	StartProgressBar( char* tick_image, int num_ticks, int x, int y )
//{
//}
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//
//void	ProgressTick( int num_ticks )
//{
//}
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//
//void	EndProgressBar( void )
//{
//}
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//
//} // namespace LoadScreen





