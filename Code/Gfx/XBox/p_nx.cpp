/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:																**
**																			**
**	Module:						 		 									**
**																			**
**	File name:		gfx/xbox/p_nx.cpp										**
**																			**
**	Created:		01/16/2002	-	dc										**
**																			**
**	Description:	Xbox platform specific interface to the engine			**
**					This is Xbox SPECIFIC!!!!!! If there is anything in		**
**					here that is not Xbox specific, then it needs to be		**
**					in nx.cpp												**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include	<xtl.h>
#include	<xgraphics.h>
#include	<sys/file/filesys.h>

#include	"gfx\camera.h"
#include	"gfx\gfxman.h"
#include	"gfx\nx.h"
#include	"gfx\nxtexman.h"
#include	"gfx\nxviewman.h"
#include	"gfx\NxQuickAnim.h"
#include	"gfx\NxParticleMgr.h"
#include	"gfx\NxMiscFX.h"
#include	"gfx\debuggfx.h"
#include	"gfx\xbox\p_NxSector.h"
#include	"gfx\xbox\p_NxScene.h"
#include	"gfx\xbox\p_NxModel.h"
#include	"gfx\xbox\p_NxGeom.h"
#include	"gfx\xbox\p_NxMesh.h"
#include	"gfx\xbox\p_NxSprite.h"
#include	"gfx\xbox\p_NxTexture.h"
#include	"gfx\xbox\p_NxParticle.h"
#include	"gfx\xbox\p_NxTextured3dPoly.h"
#include	"gfx\xbox\p_NxNewParticleMgr.h"
#include	"gfx\xbox\p_NxWeather.h"
#include	"core\math.h"
#include 	"sk\engine\SuperSector.h"					
#include 	"gel\scripting\script.h"

#include 	"gfx\xbox\nx\nx_init.h"
#include 	"gfx\xbox\nx\texture.h"
#include 	"gfx\xbox\nx\material.h"
#include 	"gfx\xbox\nx\render.h"
#include 	"gfx\xbox\nx\screenfx.h"
#include 	"gfx\xbox\nx\occlude.h"
#include 	"gfx\xbox\nx\scene.h"
#include 	"gfx\xbox\nx\chars.h"

#include	"gel\music\xbox\p_soundtrack.h"

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

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


/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_start_engine( void )
{
	NxXbox::InitialiseEngine();

	mp_particle_manager	= new CXboxNewParticleManager;
	mp_weather			= new CXboxWeather;

	// If the user selected widescreen from the dashboard, reset the default screen angle and aspect ratio.
	if( XGetVideoFlags() & ( XC_VIDEO_FLAGS_WIDESCREEN | XC_VIDEO_FLAGS_HDTV_720p ))
	{
		Script::RunScript( "screen_setup_widescreen" );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_pre_render( void )
{
	// The screen clear is now added to the push buffer directly after the swap, in s_plat_post_render().

	// No rendering should take place whilst the loading screen is visible.
//	if( !NxXbox::EngineGlobals.loadingscreen_visible )
//	{
//		D3DCOLOR col = 0x00506070;
//
//		if( NxXbox::EngineGlobals.screen_blur > 0 )
//		{
//			D3DDevice_Clear( 0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, col, 1.0f, 0 );
//			++NxXbox::EngineGlobals.screen_blur_duration;
//		}
//		else
//		{
//			D3DDevice_Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, col, 1.0f, 0 );
//			NxXbox::EngineGlobals.screen_blur_duration = 0;
//		}
//	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_post_render( void )
{
	// No rendering should take place whilst the loading screen is visible.
	if( !NxXbox::EngineGlobals.loadingscreen_visible )
	{
		D3DDevice_Swap( D3DSWAP_DEFAULT );

		if( NxXbox::EngineGlobals.screenshot_name[0] != 0 )
		{
			Spt::SingletonPtr< Gfx::Manager > gfx_manager;
			gfx_manager->ScreenShot( NxXbox::EngineGlobals.screenshot_name );
			NxXbox::EngineGlobals.screenshot_name[0] = 0;
		}

		// Now that the swap instruction has been pushed, clear the buffer for next frame.
		if( NxXbox::EngineGlobals.screen_blur > 0 )
		{
			D3DDevice_Clear( 0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, NxXbox::EngineGlobals.clear_color, 1.0f, 0 );
			++NxXbox::EngineGlobals.screen_blur_duration;
		}
		else
		{
			if( NxXbox::EngineGlobals.letterbox_active )
			{
				D3DDevice_Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0x00000000UL, 1.0f, 0 );
			}
			else if( NxXbox::EngineGlobals.clear_color_buffer )
			{
				D3DDevice_Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, NxXbox::EngineGlobals.clear_color, 1.0f, 0 );
			}
			else
			{
				D3DDevice_Clear( 0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, NxXbox::EngineGlobals.clear_color, 1.0f, 0 );
			}
			NxXbox::EngineGlobals.screen_blur_duration = 0;
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_render_world( void )
{
	// Remove the loading bar.
	if( NxXbox::EngineGlobals.loadingbar_timer_event != 0 )
	{
		timeKillEvent( NxXbox::EngineGlobals.loadingbar_timer_event );
		NxXbox::EngineGlobals.loadingbar_timer_event = 0;
	}

	// No rendering should take place whilst the loading screen is visible.
	if( !NxXbox::EngineGlobals.loadingscreen_visible )
	{
		// Store time at start of render, used as reference throughout.
		NxXbox::EngineGlobals.render_start_time = (int)Tmr::GetTime();

		// Render objects of interest for the render target (shadow objects).
		NxXbox::set_render_state( RS_ZWRITEENABLE,	1 );
		NxXbox::set_render_state( RS_ZTESTENABLE,	1 );

		NxXbox::render_shadow_targets();

		CEngine::sGetImposterManager()->ProcessImposters();

		// Start up the screen blur if we're using it.
		NxXbox::start_screen_blur();

		int num_viewports = CViewportManager::sGetNumActiveViewports();
		for( int v = 0; v < num_viewports; ++v )
		{
			CViewport	*p_cur_viewport	= CViewportManager::sGetActiveViewport( v );
			Gfx::Camera	*p_cur_camera	= p_cur_viewport->GetCamera();
			
			// Check for a valid camera.
			if( p_cur_camera == NULL )
			{
				continue;
			}
		
			NxXbox::EngineGlobals.viewport.X		= (DWORD)( p_cur_viewport->GetOriginX() * NxXbox::EngineGlobals.backbuffer_width );
			NxXbox::EngineGlobals.viewport.Y		= (DWORD)( p_cur_viewport->GetOriginY() * NxXbox::EngineGlobals.backbuffer_height );
			NxXbox::EngineGlobals.viewport.Width	= (DWORD)( p_cur_viewport->GetWidth() * NxXbox::EngineGlobals.backbuffer_width );
			NxXbox::EngineGlobals.viewport.Height	= (DWORD)( p_cur_viewport->GetHeight() * NxXbox::EngineGlobals.backbuffer_height );
			NxXbox::EngineGlobals.viewport.MinZ		= 0.0f;
			NxXbox::EngineGlobals.viewport.MaxZ		= 1.0f;

			if( NxXbox::EngineGlobals.letterbox_active )
			{	
				NxXbox::EngineGlobals.viewport.Y		+= (DWORD)( NxXbox::EngineGlobals.backbuffer_height / 8 );
				NxXbox::EngineGlobals.viewport.Height	-= (DWORD)( NxXbox::EngineGlobals.backbuffer_height / 4 );
			}

			D3DDevice_SetViewport( &NxXbox::EngineGlobals.viewport );

			// There is no bounding box transform for rendering the world.
			NxXbox::set_frustum_bbox_transform( NULL );
		
			// Set up the camera..
			float aspect_ratio = p_cur_viewport->GetAspectRatio();

			NxXbox::set_camera( &( p_cur_camera->GetMatrix()), &( p_cur_camera->GetPos()), p_cur_camera->GetAdjustedHFOV(), aspect_ratio );

			// Render the non-sky world scenes.
			for( int i = 0; i < MAX_LOADED_SCENES; i++ )
			{
				if( sp_loaded_scenes[i] )
				{
					CXboxScene *pXboxScene = static_cast<CXboxScene *>( sp_loaded_scenes[i] );

					if( !pXboxScene->IsSky())
					{
						// Build relevant occlusion poly list, now that the camera is set.
						NxXbox::BuildOccluders( &( p_cur_camera->GetPos()), v );

						NxXbox::set_render_state( RS_ZWRITEENABLE,	1 );
						NxXbox::set_render_state( RS_ZTESTENABLE,	1 );

						// Flag this scene as receiving shadows.
						pXboxScene->GetEngineScene()->m_flags |= SCENE_FLAG_RECEIVE_SHADOWS;

						NxXbox::render_scene( pXboxScene->GetEngineScene(), NxXbox::vRENDER_OPAQUE |
																			NxXbox::vRENDER_OCCLUDED |
																			NxXbox::vRENDER_SORT_FRONT_TO_BACK |
																			NxXbox::vRENDER_BILLBOARDS, v );
					}
				}
			}

			NxXbox::set_render_state( RS_ZWRITEENABLE,	0 );
			Nx::TextureSplatRender();
			NxXbox::set_render_state( RS_ZWRITEENABLE,	1 );

			CEngine::sGetImposterManager()->DrawImposters();

			// Render all opaque instances.
			NxXbox::render_instances( NxXbox::vRENDER_OPAQUE );
		
			// Now that opaque geometry is drawn, do the render tests for the light glows.
			NxXbox::render_light_glows( true );

			// Render the sky, followed by all the non-sky semitransparent scene geometry. There is no bounding box transform for rendering the world.
			NxXbox::set_frustum_bbox_transform( NULL );

			// Set up the sky camera.
			Mth::Vector	centre_pos( 0.0f, 0.0f, 0.0f );
			NxXbox::set_camera( &( p_cur_camera->GetMatrix()), &centre_pos, p_cur_camera->GetAdjustedHFOV(), aspect_ratio, true );

			// Render the sky. We have to fudge the fog here to ensure the sky is fully fogged, since it is rendered with a non-standard projection
			// matrix to ensure a constant z=1, but which breaks the fog interpolation value.
			float fog_start = NxXbox::EngineGlobals.fog_start;
			float fog_end	= NxXbox::EngineGlobals.fog_end;
			NxXbox::EngineGlobals.fog_start	= -20.0f;
			NxXbox::EngineGlobals.fog_end	= -21.0f;
			D3DDevice_SetRenderState( D3DRS_FOGSTART,	*((DWORD*)( &NxXbox::EngineGlobals.fog_start )));
			D3DDevice_SetRenderState( D3DRS_FOGEND,		*((DWORD*)( &NxXbox::EngineGlobals.fog_end )));

			NxXbox::set_render_state( RS_ZWRITEENABLE,	0 );
			NxXbox::set_render_state( RS_ZTESTENABLE,	1 );
			for( int i = 0; i < MAX_LOADED_SCENES; i++ )
			{
				if( sp_loaded_scenes[i] )
				{
					CXboxScene *pXboxScene = static_cast<CXboxScene *>( sp_loaded_scenes[i] );
					if( pXboxScene->IsSky())
					{
						// No anisotropic filtering for the sky.
						DWORD stage_zero_minfilter, stage_zero_mipfilter;
						D3DDevice_GetTextureStageState( 0, D3DTSS_MINFILTER, &stage_zero_minfilter );
						D3DDevice_GetTextureStageState( 0, D3DTSS_MIPFILTER, &stage_zero_mipfilter );
						D3DDevice_SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
						D3DDevice_SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTEXF_NONE );

						NxXbox::render_scene( pXboxScene->GetEngineScene(), NxXbox::vRENDER_OPAQUE | NxXbox::vRENDER_SEMITRANSPARENT, v );
			
						D3DDevice_SetTextureStageState( 0, D3DTSS_MIPFILTER, stage_zero_mipfilter );
						D3DDevice_SetTextureStageState( 0, D3DTSS_MINFILTER, stage_zero_minfilter );
					}
				}
			}
			NxXbox::set_render_state( RS_ZWRITEENABLE,	1 );

			// Restore fog values.
			NxXbox::EngineGlobals.fog_start	= fog_start;
			NxXbox::EngineGlobals.fog_end	= fog_end;
			D3DDevice_SetRenderState( D3DRS_FOGSTART,	*((DWORD*)( &NxXbox::EngineGlobals.fog_start )));
			D3DDevice_SetRenderState( D3DRS_FOGEND,		*((DWORD*)( &NxXbox::EngineGlobals.fog_end )));

			// Revert to the regular camera.
			NxXbox::set_camera( &( p_cur_camera->GetMatrix()), &( p_cur_camera->GetPos()), p_cur_camera->GetAdjustedHFOV(), aspect_ratio );

			// Render all semitransparent instances.
			NxXbox::render_instances( NxXbox::vRENDER_SEMITRANSPARENT | NxXbox::vRENDER_INSTANCE_PRE_WORLD_SEMITRANSPARENT );

			// Render the non-sky semitransparent scene geometry.
			// Setting the depth clip control to clamp here means that semitransparent periphary objects that would usually cull out
			// are now drawn correctly (since they will clamp at 1.0, and the z test is <=).
			D3DDevice_SetRenderState( D3DRS_DEPTHCLIPCONTROL, D3DDCC_CLAMP );
			NxXbox::set_render_state( RS_ZWRITEENABLE,	1 );
			NxXbox::set_render_state( RS_ZTESTENABLE,	1 );
			for( int i = 0; i < MAX_LOADED_SCENES; i++ )
			{
				if( sp_loaded_scenes[i] )
				{
					CXboxScene *pXboxScene = static_cast<CXboxScene *>( sp_loaded_scenes[i] );
					if( !pXboxScene->IsSky())
					{
						// Build relevant occlusion poly list, now that the camera is set.
						NxXbox::render_scene( pXboxScene->GetEngineScene(), NxXbox::vRENDER_SEMITRANSPARENT |
																			NxXbox::vRENDER_OCCLUDED |
																			NxXbox::vRENDER_BILLBOARDS, v );
					}
				}
			}
			D3DDevice_SetRenderState( D3DRS_DEPTHCLIPCONTROL, D3DDCC_CULLPRIMITIVE );

			// Render the particles.
			NxXbox::set_render_state( RS_ZWRITEENABLE,	0 );
			render_particles();

			// New style particles. Update should probably be somewhere else.
			mp_particle_manager->UpdateParticles();
			mp_particle_manager->RenderParticles();

			// Render weather effects.
			mp_weather->Process( Tmr::FrameLength());
			mp_weather->Render();

			NxXbox::set_render_state( RS_ZWRITEENABLE,	1 );

			// We want shatter objects to be rendered after the sky since they will typically be semitransparent.
			Nx::ShatterRender();

			// Render all semitransparent instances.
			NxXbox::render_instances( NxXbox::vRENDER_SEMITRANSPARENT | NxXbox::vRENDER_INSTANCE_POST_WORLD_SEMITRANSPARENT );

			// Render the light glows (the visibility tests were performed earlier).
			NxXbox::render_light_glows( false );

			// Render the shadow volumes.
//			for( int i = 0; i < MAX_LOADED_SCENES; i++ )
//			{
//				if( sp_loaded_scenes[i] )
//				{
//					CXboxScene *pXboxScene = static_cast<CXboxScene *>( sp_loaded_scenes[i] );
//					if( !pXboxScene->IsSky())
//					{
//						NxXbox::render_scene( pXboxScene->GetEngineScene(), NxXbox::vRENDER_SHADOW_VOLUMES, v );
//					}
//				}
//			}
			
			Nx::ScreenFlashRender( v, 0 );
		}

		// This x86 instruction writes back and invalidates the cache. This should hopefully mean that all updated vertex buffer
		// data (vc wibble) are flushed out by the time the GPU hits them.
		_asm
		{
			wbinvd;
		}

		// Draw debug lines.
#		ifdef __NOPT_ASSERT__
		Gfx::DebugGfx_Draw();
#		endif

		// Reset viewport so text appears on full screen.
		NxXbox::EngineGlobals.viewport.X		= 0;
		NxXbox::EngineGlobals.viewport.Y		= 0;
		NxXbox::EngineGlobals.viewport.Width	= NxXbox::EngineGlobals.backbuffer_width;
		NxXbox::EngineGlobals.viewport.Height	= NxXbox::EngineGlobals.backbuffer_height;
		NxXbox::EngineGlobals.viewport.MinZ		= 0.0f;
		NxXbox::EngineGlobals.viewport.MaxZ		= 1.0f;

		if( NxXbox::EngineGlobals.letterbox_active )
		{	
			NxXbox::EngineGlobals.viewport.Y		+= (DWORD)( NxXbox::EngineGlobals.backbuffer_height / 8 );
			NxXbox::EngineGlobals.viewport.Height	-= (DWORD)( NxXbox::EngineGlobals.backbuffer_height / 4 );
		}

		D3DDevice_SetViewport( &NxXbox::EngineGlobals.viewport );

//		Obj::CSkater *pSkater = Mdl::Skate::Instance()->GetLocalSkater();
//		Mth::Vector pos = pSkater->GetPos();
//		NxXbox::set_focus_blur_focus( pos, 0.0f, 10.0f * 12.0f, 100.0f * 12.0f );
//		NxXbox::finish_focus_blur();

		// Horrible hack - this should be somewhere else ASAP.
		NxXbox::SDraw2D::DrawAll();

		// Finish the screen blur if we're using it.
		NxXbox::finish_screen_blur();
	}

	// Reset viewport so screen clear will work.
	NxXbox::EngineGlobals.viewport.X		= 0;
	NxXbox::EngineGlobals.viewport.Y		= 0;
	NxXbox::EngineGlobals.viewport.Width	= NxXbox::EngineGlobals.backbuffer_width;
	NxXbox::EngineGlobals.viewport.Height	= NxXbox::EngineGlobals.backbuffer_height;
	NxXbox::EngineGlobals.viewport.MinZ		= 0.0f;
	NxXbox::EngineGlobals.viewport.MaxZ		= 1.0f;
	D3DDevice_SetViewport( &NxXbox::EngineGlobals.viewport );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CScene	*CEngine::s_plat_create_scene( const char *p_name, CTexDict *p_tex_dict, bool add_super_sectors )
{
	// Create scene class instance
	CXboxScene	*p_xbox_scene	= new CXboxScene;
	CScene		*p_new_scene	= p_xbox_scene;
	p_new_scene->SetInSuperSectors( add_super_sectors );
	p_new_scene->SetIsSky( false );

	// Create a new sScene so the engine can track assets for this scene.
	NxXbox::sScene *p_engine_scene = new NxXbox::sScene();
	p_xbox_scene->SetEngineScene( p_engine_scene );

	return p_new_scene;
}



#define MemoryRead( dst, size, num, src )	CopyMemory(( dst ), ( src ), (( num ) * ( size )));	\
											( src ) += (( num ) * ( size ))

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CScene *CEngine::s_plat_load_scene_from_memory( void *p_mem, CTexDict *p_tex_dict, bool add_super_sectors, bool is_sky, bool is_dictionary )
{
	uint8			*p_data = (uint8*)p_mem;
	CSector			*pSector;
	CXboxSector		*pXboxSector;

	// Create a new sScene so the engine can track assets for this scene.
	NxXbox::sScene *p_engine_scene = new NxXbox::sScene();

	// Set the dictionary flag.
	p_engine_scene->m_is_dictionary	= is_dictionary;

	// Version numbers.
	uint32 mat_version, mesh_version, vert_version;
	MemoryRead( &mat_version, sizeof( uint32 ), 1, p_data );
	MemoryRead( &mesh_version, sizeof( uint32 ), 1, p_data );
	MemoryRead( &vert_version, sizeof( uint32 ), 1, p_data );

	// Import materials (they will now be associated at the engine-level with this scene).
	p_engine_scene->pMaterialTable = NxXbox::LoadMaterialsFromMemory( (void**)&p_data, p_tex_dict->GetTexLookup());

	// Read number of sectors.
	int num_sectors;
	MemoryRead( &num_sectors, sizeof( int ), 1, p_data );

	// Figure optimum hash table lookup size.
	uint32 optimal_table_size	= num_sectors * 2;
	uint32 test					= 2;
	uint32 size					= 1;
	for( ;; test <<= 1, ++size )
	{
		// Check if this iteration of table size is sufficient, or if we have hit the maximum size.
		if(( optimal_table_size <= test ) || ( size >= 12 ))
		{
			break;
		}
	}

	// Create scene class instance, using optimum size sector table.
	CScene* new_scene = new CXboxScene( size );
	new_scene->SetInSuperSectors( add_super_sectors );
	new_scene->SetIsSky( is_sky );

	// Get a scene id from the engine.
	CXboxScene *p_new_xbox_scene = static_cast<CXboxScene *>( new_scene );
	p_new_xbox_scene->SetEngineScene( p_engine_scene );

	for( int s = 0; s < num_sectors; ++s )
	{
		// Create a new sector to hold the incoming details.
		pSector						= p_new_xbox_scene->CreateSector();
		pXboxSector					= static_cast<CXboxSector*>( pSector );
		
		// Generate a hanging geom for the sector, used for creating level objects etc.
		CXboxGeom	*p_xbox_geom	= new CXboxGeom();
		p_xbox_geom->SetScene( p_new_xbox_scene );
		pXboxSector->SetGeom( p_xbox_geom );
		
		// Prepare CXboxGeom for receiving data.
		p_xbox_geom->InitMeshList();
		
		// Load sector data.
		if( pXboxSector->LoadFromMemory( (void**)&p_data ))
		{
			new_scene->AddSector( pSector );
		}
	}

	// At this point get the engine scene to figure it's bounding volumes.
	p_engine_scene->FigureBoundingVolumes();
	
	// Read hierarchy information.
	int num_hierarchy_objects;
	MemoryRead( &num_hierarchy_objects, sizeof( int ), 1, p_data );

	if( num_hierarchy_objects > 0 )
	{
		p_engine_scene->mp_hierarchyObjects = new CHierarchyObject[num_hierarchy_objects];
		MemoryRead( p_engine_scene->mp_hierarchyObjects, sizeof( CHierarchyObject ), num_hierarchy_objects, p_data );
		p_engine_scene->m_numHierarchyObjects = num_hierarchy_objects;
	}

	return new_scene;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CScene *CEngine::s_plat_load_scene( const char *p_name, CTexDict *p_tex_dict, bool add_super_sectors, bool is_sky, bool is_dictionary )
{
	CSector*		pSector;
	CXboxSector*	pXboxSector;

	Dbg_Message( "loading scene from file %s\n", p_name );

	// Create a new NxXbox::sScene so the engine can track assets for this scene.
	NxXbox::sScene *p_engine_scene = new NxXbox::sScene();

	// Set the dictionary flag.
	p_engine_scene->m_is_dictionary	= is_dictionary;

	// Open the scene file.
	void* p_file = File::Open( p_name, "rb" );
	if( !p_file )
	{
		Dbg_MsgAssert( p_file, ( "Couldn't open scene file %s\n", p_name ));
		return NULL;
	}
	
	// Version numbers.
	uint32 mat_version, mesh_version, vert_version;
	File::Read( &mat_version, sizeof( uint32 ), 1, p_file );
	File::Read( &mesh_version, sizeof( uint32 ), 1, p_file );
	File::Read( &vert_version, sizeof( uint32 ), 1, p_file );

	// Import materials (they will now be associated at the engine-level with this scene).
	p_engine_scene->pMaterialTable = NxXbox::LoadMaterials( p_file, p_tex_dict->GetTexLookup());

	// Read number of sectors.
	int num_sectors;
	File::Read( &num_sectors, sizeof( int ), 1, p_file );

	// Figure optimum hash table lookup size.
	uint32 optimal_table_size	= num_sectors * 2;
	uint32 test					= 2;
	uint32 size					= 1;
	for( ;; test <<= 1, ++size )
	{
		// Check if this iteration of table size is sufficient, or if we have hit the maximum size.
		if(( optimal_table_size <= test ) || ( size >= 12 ))
		{
			break;
		}
	}

	// Create scene class instance, using optimum size sector table.
	CScene* new_scene = new CXboxScene( size );
	new_scene->SetInSuperSectors( add_super_sectors );
	new_scene->SetIsSky( is_sky );

	// Get a scene id from the engine.
	CXboxScene *p_new_xbox_scene = static_cast<CXboxScene *>( new_scene );
	p_new_xbox_scene->SetEngineScene( p_engine_scene );

	for( int s = 0; s < num_sectors; ++s )
	{
		// Create a new sector to hold the incoming details.
		pSector						= p_new_xbox_scene->CreateSector();
		pXboxSector					= static_cast<CXboxSector*>( pSector );
		
		// Generate a hanging geom for the sector, used for creating level objects etc.
		CXboxGeom	*p_xbox_geom	= new CXboxGeom();
		p_xbox_geom->SetScene( p_new_xbox_scene );
		pXboxSector->SetGeom( p_xbox_geom );
		
		// Prepare CXboxGeom for receiving data.
		p_xbox_geom->InitMeshList();
		
		// Load sector data.
		if( pXboxSector->LoadFromFile( p_file ))
		{
			new_scene->AddSector( pSector );
		}
	}

	// At this point get the engine scene to figure it's bounding volumes.
	p_engine_scene->FigureBoundingVolumes();
	
	// Read hierarchy information.
	int num_hierarchy_objects;
	File::Read( &num_hierarchy_objects, sizeof( int ), 1, p_file );

	if( num_hierarchy_objects > 0 )
	{
		p_engine_scene->mp_hierarchyObjects = new CHierarchyObject[num_hierarchy_objects];
		File::Read( p_engine_scene->mp_hierarchyObjects, sizeof( CHierarchyObject ), num_hierarchy_objects, p_file );
		p_engine_scene->m_numHierarchyObjects = num_hierarchy_objects;
	}
	
	File::Close( p_file );

	return new_scene;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CEngine::s_plat_unload_scene( CScene *p_scene )
{
	Dbg_MsgAssert( p_scene,( "Trying to delete a NULL scene" ));

	CXboxScene *p_xbox_scene = (CXboxScene*)p_scene;

	// Ask the engine to remove the associated meshes for each sector in the scene.
	p_xbox_scene->DestroySectorMeshes();

	// Get the engine specific scene data and pass it to the engine to delete.
	NxXbox::DeleteScene( p_xbox_scene->GetEngineScene());
	p_xbox_scene->SetEngineScene( NULL );
	
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CEngine::s_plat_add_scene( CScene *p_scene, const char *p_filename )
{
	// Function to incrementally add geometry to a scene - should NOT be getting called on Xbox.
	Dbg_Assert( 0 );
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
//CTexDict* CEngine::s_plat_load_textures( const char* p_name )
//{
//	NxXbox::LoadTextureFile( p_name );
//	return NULL;
//}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CModel* CEngine::s_plat_init_model( void )
{
	CXboxModel *pModel = new CXboxModel;
	
	return pModel;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CEngine::s_plat_uninit_model( CModel* pModel )
{
	delete pModel;

	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CGeom* CEngine::s_plat_init_geom( void )
{
	CXboxGeom *pGeom = new CXboxGeom;
	return pGeom;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CEngine::s_plat_uninit_geom( CGeom *p_geom )
{
	delete p_geom;
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CQuickAnim* CEngine::s_plat_init_quick_anim()
{
	CQuickAnim* pQuickAnim = new CQuickAnim;
	return pQuickAnim;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_uninit_quick_anim(CQuickAnim* pQuickAnim)
{
	delete pQuickAnim;
	return;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CMesh* CEngine::s_plat_load_mesh( const char* pMeshFileName, Nx::CTexDict* pTexDict, uint32 texDictOffset, bool isSkin, bool doShadowVolume )
{
	// Load the scene.
	Nx::CScene *p_scene = Nx::CEngine::s_plat_load_scene( pMeshFileName, pTexDict, false, false, false );

	// Store the checksum of the scene name.
	p_scene->SetID(Script::GenerateCRC( pMeshFileName )); 	// store the checksum of the scene name

	p_scene->SetTexDict( pTexDict );
	p_scene->PostLoad( pMeshFileName );
	
	// Disable any scaling.
	NxXbox::DisableMeshScaling();

	CXboxMesh *pMesh = new CXboxMesh( pMeshFileName );

	Nx::CXboxScene *p_xbox_scene = static_cast<Nx::CXboxScene*>( p_scene );
	pMesh->SetScene( p_xbox_scene );
	pMesh->SetTexDict( pTexDict );

	return pMesh;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CMesh* CEngine::s_plat_load_mesh( uint32 id, uint32 *p_model_data, int model_data_size, uint8 *p_cas_data, Nx::CTexDict* pTexDict, uint32 texDictOffset, bool isSkin, bool doShadowVolume )
{
	// Convert the id into a usable string.
	Dbg_Assert( id > 0 );
	char id_as_string[16];
	sprintf( id_as_string, "%d\n", id );

	// Load the scene.
	Nx::CScene *p_scene = Nx::CEngine::s_plat_load_scene_from_memory( p_model_data, pTexDict, false, false, false );

	// Store the checksum of the scene name.
	p_scene->SetID( Script::GenerateCRC( id_as_string ));

	p_scene->SetTexDict( pTexDict );
	p_scene->PostLoad( id_as_string );
	
	CXboxMesh *pMesh = new CXboxMesh();

	// Set CAS data for mesh.
	pMesh->SetCASData( p_cas_data );

	// Disable any scaling.
	NxXbox::DisableMeshScaling();

	Nx::CXboxScene *p_xbox_scene = static_cast<Nx::CXboxScene*>( p_scene );
	pMesh->SetScene( p_xbox_scene );
	pMesh->SetTexDict( pTexDict );
	return pMesh;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CEngine::s_plat_unload_mesh( CMesh *pMesh )
{
	delete pMesh;
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_set_mesh_scaling_parameters( SMeshScalingParameters* pParams )
{
	NxXbox::SetMeshScalingParameters( pParams );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CSprite *CEngine::s_plat_create_sprite( CWindow2D *p_window )
{
	return new CXboxSprite;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CEngine::s_plat_destroy_sprite( CSprite *p_sprite )
{
	delete p_sprite;
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTextured3dPoly *	CEngine::s_plat_create_textured_3d_poly()
{
	return new CXboxTextured3dPoly;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CEngine::s_plat_destroy_textured_3d_poly(CTextured3dPoly *p_poly)
{
	delete p_poly;
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Nx::CTexture *CEngine::s_plat_create_render_target_texture( int width, int height, int depth, int z_depth )
{
	// Create the CXBoxTexture (just a container class for the NxXbox::sTexture).
	CXboxTexture *p_texture = new CXboxTexture();

	// Create the NxXbox::sTexture.
	NxXbox::sTexture *p_engine_texture = new NxXbox::sTexture;
	p_texture->SetEngineTexture( p_engine_texture );
	
	// Set the texture as a render target.
	p_engine_texture->SetRenderTarget( width, height, depth, z_depth );
	
	return p_texture;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_project_texture_into_scene( Nx::CTexture *p_texture, Nx::CModel *p_model, Nx::CScene *p_scene )
{
	Nx::CXboxTexture	*p_xbox_texture	= static_cast<CXboxTexture*>( p_texture );
	Nx::CXboxModel		*p_xbox_model	= static_cast<CXboxModel*>( p_model );
//	Nx::CXboxScene		*p_xbox_scene	= static_cast<CXboxScene*>( p_scene );
//	NxXbox::create_texture_projection_details( p_xbox_texture->GetEngineTexture(), p_xbox_model, p_xbox_scene->GetEngineScene());
	NxXbox::create_texture_projection_details( p_xbox_texture->GetEngineTexture(), p_xbox_model, NULL );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_set_projection_texture_camera( Nx::CTexture *p_texture, Gfx::Camera *p_camera )
{
	Nx::CXboxTexture	*p_xbox_texture	= static_cast<CXboxTexture*>( p_texture );
	XGVECTOR3			pos( p_camera->GetPos()[X], p_camera->GetPos()[Y], p_camera->GetPos()[Z] );
	XGVECTOR3			at = pos + D3DXVECTOR3( p_camera->GetMatrix()[Z][X], p_camera->GetMatrix()[Z][Y], p_camera->GetMatrix()[Z][Z] );
	
	NxXbox::set_texture_projection_camera( p_xbox_texture->GetEngineTexture(), &pos, &at );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_stop_projection_texture( Nx::CTexture *p_texture )
{
	Nx::CXboxTexture *p_xbox_texture = static_cast<CXboxTexture*>( p_texture );
	NxXbox::destroy_texture_projection_details( p_xbox_texture->GetEngineTexture());
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_add_occlusion_poly( uint32 num_verts, Mth::Vector *p_vert_array, uint32 checksum )
{
	if( num_verts == 4 )
	{
		NxXbox::AddOcclusionPoly( p_vert_array[0], p_vert_array[1], p_vert_array[2], p_vert_array[3], checksum );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_enable_occlusion_poly( uint32 checksum, bool enable )
{
	NxXbox::EnableOcclusionPoly( checksum, enable );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_remove_all_occlusion_polys( void )
{
	NxXbox::RemoveAllOcclusionPolys();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// returns true if the sphere at "center", with the "radius"
// is visible to the current camera
// (note, currently this is the last frame's camera on PS2)
bool CEngine::s_plat_is_visible( Mth::Vector& center, float radius )
{
	return NxXbox::IsVisible( center, radius );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_set_max_multipass_distance( float dist )
{
	// Has no meaning for Xbox.
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const char* CEngine::s_plat_get_platform_extension( void )
{
	// String literals are statically allocated so can be returned safely, (Bjarne, p90)
	return "Xbx";
}


/******************************************************************/
// Wait for any pending asyncronous rendering to finish, so rendering
// data can be unloaded
/******************************************************************/
void CEngine::s_plat_finish_rendering()
{
	// Wait for asyncronous rendering to finish.
	NxXbox::EngineGlobals.p_Device->BlockUntilIdle();
} 

/******************************************************************/
// Set the amount that the previous frame is blended with this frame
// 0 = none	  	(just see current frame) 	
// 128 = 50/50
// 255 = 100% 	(so you only see the previous frame)												  
/******************************************************************/
void CEngine::s_plat_set_screen_blur( uint32 amount )
{
	// Only set the blur if we have a blur buffer into which to render.
	if( NxXbox::EngineGlobals.p_BlurSurface[0] )
	{
		NxXbox::EngineGlobals.screen_blur = amount;
	}
} 



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int	CEngine::s_plat_get_num_soundtracks( void )
{
	return Pcm::GetNumSoundtracks();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
const char* CEngine::s_plat_get_soundtrack_name( int soundtrack_number )
{
	static char buf[128];

	const WCHAR* p_soundtrack_name_wide = Pcm::GetSoundtrackName( soundtrack_number );

	if( p_soundtrack_name_wide )
	{
		// If p_soundtrack_name_wide contains characters not recognized by the XBox then wsprintfA
		// will write an empty string. It does this for the test bad soundtrack names provided by
		// the certification tool.
		// However, I'm not sure if wsprintf will write an empty string if there is just one bad
		// character in the name, or if it will only write an empty string if they are all bad.
		// So just to be sure, do a wsprintfA for each character in turn. That way, any good characters
		// will definitely be printed OK, and all bad characters will appear as xbox null characters.
		char *p_dest = buf;
		int count=0;
		WCHAR p_temp[2]; // A buffer for holding one WCHAR at a time for sending to wsprintfA.
		p_temp[1]	= 0;
		const WCHAR *p_scan = p_soundtrack_name_wide;

		while( *p_scan ) // WCHAR strings are terminated by a 0 just like normal strings, except its a 2byte 0.
		{
			p_temp[0] = *p_scan++;
				
			char p_one_char[10];
			wsprintfA( p_one_char, "%ls", p_temp);

			// p_one_char now contains a one char string.
			if( count < 99 )
			{
				if( *p_one_char )
				{
					*p_dest = *p_one_char;
				}
				else
				{
					// Bad char, so write a ~ so that it appears as a xbox null char.
					*p_dest='~';
				}	
				++p_dest;
				++count;
			}
		}	
		*p_dest = 0;
	}
	else
	{
		// In theory this should never happen, but make sure p_buf contains something if it does.
		sprintf( buf, "~" );
	}	
		
	int len = strlen( buf );
	for( int c = 0; c < len; ++c )
	{
		// Force any special characters (arrows or button icons) to be displayed
		// as the xbox NULL character by changing them to an invalid character.
		switch( buf[c] )
		{
			case '¡': case '¢': case '£': case '¤':
			case '¥': case '¦': case '§': case '¨':
			case '©': case 'ª': case '«': case '¬':
				buf[c] = '~';
				break;
			default:
				break;
		}	
	}
	return buf;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_set_letterbox( bool letterbox )
{
	// Letterbox mode is designed for use on a regular 4:3 screen.
	// It should use the same, wider viewing angle as for widescreen mode, but shrink the resultant image down
	// vertically by 25%.
	if( letterbox )
	{
		if( NxXbox::EngineGlobals.letterbox_active == false )
		{
			// Need to adjust the screen y offset and multiplier to ensure sprites are scaled properly for this mode.
			NxXbox::EngineGlobals.screen_conv_y_offset		+= ( NxXbox::EngineGlobals.backbuffer_height / 4 ) / 2;
			NxXbox::EngineGlobals.screen_conv_y_multiplier	= 0.75f;
			NxXbox::EngineGlobals.letterbox_active			= true;
		}
	}
	else
	{
		if( NxXbox::EngineGlobals.letterbox_active == true )
		{
			// Restore the screen y offset and multiplier.
			NxXbox::EngineGlobals.screen_conv_y_offset		-= ( NxXbox::EngineGlobals.backbuffer_height / 4 ) / 2;
			NxXbox::EngineGlobals.screen_conv_y_multiplier	= 1.0f;
			NxXbox::EngineGlobals.letterbox_active			= false;
		}
	}
} 



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_set_color_buffer_clear( bool clear )
{
//	NxXbox::EngineGlobals.clear_color_buffer = clear;
}

} // namespace Nx
