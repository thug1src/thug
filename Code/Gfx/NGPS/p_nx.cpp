/////////////////////////////////////////////////////////////////////////////
// p_nx.cpp - PS2 platform specific interface to the engine
//
// This is PS2 SPECIFIC!!!!!!  So might get a bit messy
//
// If there is anything in here that is not PS2 specific, then it needs to 
// be in nx.cpp

#include	"gfx\nx.h"
#include	"gfx\NxViewMan.h"
#include	"gfx\NxMiscFX.h"
#include	"gfx\NxParticleMgr.h"
#include	"gfx\NxQuickAnim.h"
#include	"gfx\NGPS\p_NxGeom.h"
#include	"gfx\NGPS\p_NxLightMan.h"
#include	"gfx\NGPS\p_NxMesh.h"
#include	"gfx\NGPS\p_NxModel.h"
#include	"gfx\NGPS\p_NxSector.h"
#include	"gfx\NGPS\p_NxScene.h"
#include	"gfx\NGPS\p_NxSprite.h"
#include	"gfx\NGPS\p_NxTexture.h"
#include	"gfx\NGPS\p_NxTextured3dPoly.h"
#include	"gfx\NGPS\p_NxNewParticleMgr.h"
#include	"core\math.h"
#include    "gel\collision\collision.h"

#include 	"gfx\ngps\nx\nx_init.h"
#include 	"gfx\ngps\nx\material.h"
#include 	"gfx\ngps\nx\mesh.h"
#include 	"gfx\ngps\nx\occlude.h"
#include 	"gfx\ngps\nx\scene.h"
#include 	"gfx\ngps\nx\chars.h"
#include 	"gfx\ngps\nx\render.h"
#include 	"gfx\ngps\nx\geomnode.h"
#include 	"gfx\ngps\nx\immediate.h"
#include 	"gfx\ngps\p_nxweather.h"
#include 	"gfx\ngps\nx\switches.h"

#include <gfx/camera.h>
#include <sys/file/filesys.h>
#include <sys/file/pip.h>
#include <gel/components/collisioncomponent.h>
#include <gel/scripting/struct.h>
#include <gel/object/compositeobject.h>

namespace NxPs2
{
extern void	test_render_start_frame();
extern void	test_render(Mth::Matrix* camera_orient, Mth::Vector* camera_pos,  float view_angle, float screen_aspect);
extern void	test_render_end_frame();
extern void	test_init();			
extern bool IsVisible(Mth::Vector &center, float radius);
extern void	WaitForRendering();
extern uint32	*p_patch_ALPHA;
extern uint32	*p_patch_ALPHA2;
} // namespace NxPs2


namespace	Nx
{

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// Functions


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CEngine::s_plat_start_engine()
{
	NxPs2::InitialiseEngine();
	mp_particle_manager = new CPs2NewParticleManager;
	Nx::CPs2Texture::sInitTables();
	mp_weather = new CPs2Weather;
}


void 		CEngine::s_plat_pre_render()
{
// Moved to StuffAfterGSFinished
//	CPs2LightManager::sUpdateEngine();
//
//	// particle systems effectively have their transform double-buffered for dma purposes
//	// so now is the time to do the buffer flip by updating the transforms
//	mp_particle_manager->UpdateParticles();

	NxPs2::test_render_start_frame();
}

void 		CEngine::s_plat_post_render()
{
	// bit of a hack, see below
//	Gfx::Camera *cur_camera = CViewportManager::sGetCamera( 0 );
//	if (cur_camera)
	{

		
		
		NxPs2::test_render_end_frame();
	}
}

//void shadow_test( float * p_sn, uint16 * p_index_list, int count, float * dir )
//{
//	// Test...
//
//	asm __volatile__(
//		"
//		.set noreorder
//
//		lqc2	vf09, 0x0(%1)		# light dir
//
//		lqc2	vf10, 0x0(%0)		# surface normal
//		addi	%2, %2, -1
//
//dot_loop:
//		vmul.xyz	vf13,	vf09,	vf10	# result = plane * center + plane_w [start]
//		addiu		%0,		%0,		12
//		lqc2		vf10,	0x4(%0)			# surface normal
//		vaddy.x		vf13,	vf13,	vf13y
//		vaddz.x		vf13,	vf13,	vf13z
//
//		qmfc2		$8,		vf13
//		sw			$8,		0x0(%0)
//		addiu		%0,		%0,		4
//
//		bne			%2,		$0,		dot_loop
//		addi		%2,		%2,		-1
//done:
//
//		.set reorder
//		": : "r" (p_sn), "r" (dir), "r" (count) : "$8" ); 
////
////		": "=r" (p_dot) : "r" (p_sn) : "$8", "$9", "$10");
//	
//
////#		lhu		$8, 0(%2)
////#		lhu		$9, 2(%2)
////#		lhu		$10, 4(%2)
////#		addiu	%2, %2, 6
////#
////#		add		$8, $8, %1
////#		add		$9, $9, %1
////#		add		$10, $10, %1
////#		lqc2	vf10, 0x0($8)		# p0
////#		lqc2	vf11, 0x0($8)		# p1
////#		lqc2	vf12, 0x0($8)		# p2
//
//}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CEngine::s_plat_render_world()
{
	NxPs2::RenderPrologue();	 	// should this be in test_render_start_frame? (Garrett: Answer is no, it won't work)
	NxPs2::RenderInitImmediateMode();


//#define NUM_TRIS 1851
////	float	xyz[(NUM_TRIS+2)*3];
//	float	sn[NUM_TRIS*4];
//	uint16	list[NUM_TRIS*3];
//	float	vec[3] = { 1.0f, 1.5f, 2.0f };
//
//	for ( int lp = 0; lp < NUM_TRIS; lp++ )
//	{
//		sn[(lp*4)+0] = 6.0f;
//		sn[(lp*4)+1] = 7.0f;
//		sn[(lp*4)+2] = 8.0f;
//
////		xyz[(lp*3)+0] = ((float)(lp/2));
////		xyz[(lp*3)+1] = ((float)(lp&1));
////		xyz[(lp*3)+2] = 0.0f;
//
//		list[(lp*3)+0] = ( lp + 0 ) * 12;
//		list[(lp*3)+1] = ( lp + 1 ) * 12;
//		list[(lp*3)+2] = ( lp + 2 ) * 12;
//	}
//
//	Tmr::CPUCycles cycles = Tmr::GetTimeInCPUCycles();
//	shadow_test( sn, list, NUM_TRIS, vec );
//	cycles = Tmr::GetTimeInCPUCycles() - cycles;
//	printf( "CPU Cycles for shadow test: %d\n", cycles );

	#if 0
	// (Mike) see if there are any viewports to render, and return if there aren't
	// (Mike) ...perhaps make this trivial by having a CViewportManager::sGetNumActiveCameras() ?
	bool no_viewports = true;
	for (int view_idx = 0; view_idx < CViewportManager::sGetNumActiveViewports(); view_idx++)
	{
		if (CViewportManager::sGetActiveViewport(view_idx)->GetCamera())
		{
			no_viewports = false;
			break;
		}
	}
	if (no_viewports)
	{
		return;
	}
	// (Mike) now we know there's something to render...
	#endif


/*
	#if 1
	// normal 4:3 TV	
	float view_angle = 72.0f;   			// The angle prepended by the width of the screen
	float screen_aspect = 4.0f/3.0f;		// physical ration of width to height on the screen	(4:3 or 16:9)
	#else 
	// anamorphic widescreen
	float view_angle = 80.0f;   			// The angle prepended by the width of the screen
	float screen_aspect = 16.0f/9.0f;	// physical ratio of width to height on the screen	(4:3 or 16:9)
	#endif
*/
//	CViewportManager::sSetScreenMode(vSPLIT_V);
	
	

	bool got_vu0 = NxPs2::OccludeUseVU0();
	Dbg_Assert(got_vu0);
	
	// Note: this method of setting up the camera must change
	// so that the p_nx module does not reference things higher up the hierarchy
	for (int view_idx = 0; view_idx < CViewportManager::sGetNumActiveViewports(); view_idx++)
	{
		Gfx::Camera *cur_camera;
		CViewport *p_cur_viewport = CViewportManager::sGetActiveViewport(view_idx);
		
		Dbg_MsgAssert(p_cur_viewport,("Unable to get viewport %d",view_idx));
		cur_camera = p_cur_viewport->GetCamera();
		
		if (cur_camera)
		{
			
			// Build the occluders first now that we know where the camera is for this frame.
			NxPs2::BuildOccluders( &( cur_camera->GetPos()));

			// Make camera matrix
			Mth::Matrix cam_matrix(cur_camera->GetMatrix());
			cam_matrix[W] = cur_camera->GetPos();

			// Garrett: For some reason, only these clipping values work
			NxPs2::RenderViewport(view_idx, cam_matrix, p_cur_viewport->GetRect(), cur_camera->GetAdjustedHFOV(), p_cur_viewport->GetAspectRatio(),
								  /*-cur_camera->GetNearClipPlane(), -cur_camera->GetFarClipPlane()*/ -1.0f, -100000.0f, s_render_mode > 1);
#	ifdef __USE_PROFILER__
			Sys::CPUProfiler->PushContext( 0, 0, 255 );	 	// Blue (Under Yellow) = Immediate mode rnedering
#	endif // __USE_PROFILER__

			// Draw the immediate mode stuff
			NxPs2::RenderSwitchImmediateMode();
			NxPs2::CImmediateMode::sViewportInit();

// Mick: Moved this here from test_render_end_frame
			NxPs2::Reallocate2DVRAM();

			if (NxPs2::FlipCopyEnabled()) // TT 13470, 13471, 11061 Particle systems displayed when screen is blank
			{
			
				Nx::render_particles();
				mp_particle_manager->RenderParticles();
	
				// Render Weather effects.
				mp_weather->Process( Tmr::FrameLength() );
				mp_weather->Render();
	
				//Nx::CTextured3dPoly::sRenderAll();
				TextureSplatRender();
				ShatterRender();			// Make sure this is always the last immediate draw since it switches DMA lists
			}
						
			NxPs2::RenderSwitchImmediateMode();

#	ifdef __USE_PROFILER__
			Sys::CPUProfiler->PopContext(  );	 
#	endif // __USE_PROFILER__

			if (s_render_mode)
			{
#ifdef __NOPT_ASSERT__
				for (int i = 0; i < MAX_LOADED_SCENES; i++)
				{
					if (sp_loaded_scenes[i])
					{
						sp_loaded_scenes[i]->DebugRenderCollision(s_debug_ignore_1, s_debug_ignore_0);
					}
				}
#endif

				const Lst::Head<Obj::CObject>& composite_obj_list = Nx::CEngine::sGetMovableObjects();
				Lst::Node<Obj::CObject>* p_composite_node = composite_obj_list.GetNext();
				while( p_composite_node )
				{
					Obj::CCompositeObject* p_composite_object = static_cast<Obj::CCompositeObject*>(p_composite_node->GetData());
					Dbg_MsgAssert(p_composite_object, ("Node in CObject list wasn't a CCompositeObject"));

					Obj::CCollisionComponent* p_collision_component = GetCollisionComponentFromObject( p_composite_object );
					if ( p_collision_component )
					{
						CCollObj* p_coll_obj = p_collision_component->GetCollision();
						if (p_coll_obj)
						{
							p_coll_obj->DebugRender( s_debug_ignore_1, s_debug_ignore_0 );
						}
					}

					p_composite_node = p_composite_node->GetNext();
				}
			}	
		}
		else
		{
			//printf ("RENDERING FRAME WITH NO CAMERA (Nothing rendered)\n");
		}
		
		// Screen flash rendering.
		Nx::ScreenFlashRender( view_idx, 0 );
	}

	//NxPs2::RenderEpilogueStart();
	
	if (got_vu0)
	{
		NxPs2::OccludeDisableVU0();
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CScene	*	CEngine::s_plat_create_scene(const char *p_name, CTexDict *p_tex_dict, bool add_super_sectors)
{
	// Create scene class instance
	CPs2Scene *p_ps2_scene = new CPs2Scene;

	NxPs2::CGeomNode *p_geomnode = new NxPs2::CGeomNode;
	p_geomnode->AddToTree(LOADFLAG_RENDERNOW);
	p_geomnode->SetActive(true);
	p_ps2_scene->SetEngineCloneScene(p_geomnode);

	CScene *new_scene = p_ps2_scene;
	new_scene->SetInSuperSectors(add_super_sectors);
	new_scene->SetIsSky(false);

	return new_scene;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CScene *CEngine::s_plat_load_scene_from_memory( void *p_mem, CTexDict *p_tex_dict, bool add_super_sectors, bool is_sky, bool is_dictionary )
{
	// GJ:  This is a stub function that should never be called by the PS2.
	// Currently, it's only needed by the X-box and the NGC
	// because all models are treated as "scenes" on those platforms.
	// On the PS2, models are treated as CPS2Mesh-es...

	Dbg_MsgAssert( 0, ( "This function is not supported on the PS2 right now" ) );

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CScene	*	CEngine::s_plat_load_scene(const char *p_name, CTexDict *p_tex_dict,
									   bool add_super_sectors, bool is_sky, bool is_dictionary)
{
	// Create scene class instance
	CPs2Scene *p_ps2_scene = new CPs2Scene;
	CScene *new_scene = p_ps2_scene;
	new_scene->SetInSuperSectors(add_super_sectors);
	new_scene->SetIsSky(is_sky);

	#ifdef __NOPT_ASSERT__
	CPs2TexDict *p_ps2_tex_dict = static_cast<CPs2TexDict *>(p_tex_dict);
	Dbg_MsgAssert(p_ps2_tex_dict,("Invalid texture dictionsary for %s",p_name));
	#endif
	
	// load scene
	// Pip::Load the new file
	uint8 *p_pipData = (uint8*)Pip::Load(p_name);

	// 1st section is shadow volume data.
//	p_ps2_scene->mp_shadow_volume_header = (Nx::sShadowVolumeHeader*)p_pipData;
//	p_ps2_scene->mp_shadow_volume_header->p_vertex = (Nx::sShadowVertex *)&p_ps2_scene->mp_shadow_volume_header[1];
//		p_ps2_scene->mp_shadow_volume_header->p_connect = (Nx::sShadowConnect *)&p_ps2_scene->mp_shadow_volume_header->p_vertex[p_ps2_scene->mp_shadow_volume_header->num_verts];
//	p_ps2_scene->mp_shadow_volume_header->p_neighbor = (Nx::sShadowNeighbor *)&p_ps2_scene->mp_shadow_volume_header->p_connect[p_ps2_scene->mp_shadow_volume_header->num_faces];
//
//	uint8 * p_geom_data = (uint8*)p_ps2_scene->mp_shadow_volume_header->p_vertex;
//	p_geom_data = &p_geom_data[p_ps2_scene->mp_shadow_volume_header->byte_size];
	// PJR
	// Process my data here.

	// set up load flags
	uint32 load_flags = 0;
	if (!is_dictionary)
	{
		// render it unconditionally as opposed to holding in a database to be instanced later)
		load_flags |= LOADFLAG_RENDERNOW;
	} else {
		Dbg_MsgAssert(!add_super_sectors, ("Why would you need collision SuperSectors for a scene dictionary?"));
	}
	if (is_sky)
	{
		load_flags |= LOADFLAG_SKY;
	}

	// process data in place
	NxPs2::CGeomNode *p_geomnode = NxPs2::CGeomNode::sProcessInPlace(p_pipData, load_flags);
//	NxPs2::CGeomNode *p_geomnode = NxPs2::CGeomNode::sProcessInPlace(p_geom_data, load_flags);
	
	Dbg_MsgAssert(((int)(p_geomnode) & 0xf) == 0,("p_geomnode (0x%x) not multiple of 16 after sProcessInPlace\n"
													,(int)p_geomnode));

	
	p_ps2_scene->SetEngineScene(p_geomnode);

	return new_scene;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CEngine::s_plat_add_scene(CScene *p_scene, const char *p_filename)
{
	Dbg_MsgAssert(/*!p_scene->is_dictionary &&*/ !p_scene->IsSky(), ("Can't add to sky or dictionary scene."));

	// load scene
	// Pip::Load the new file
	uint8 *p_pipData = (uint8*)Pip::Load(p_filename);

	Dbg_MsgAssert(p_pipData, ("s_plat_add_scene(): Can't open file %s", p_filename));

	// process data in place
	NxPs2::CGeomNode *p_geomnode = NxPs2::CGeomNode::sProcessInPlace(p_pipData, LOADFLAG_RENDERNOW);

	CPs2Scene *p_ps2_scene = static_cast<CPs2Scene *>(p_scene);
	p_ps2_scene->SetEngineAddScene(p_geomnode);

	return p_geomnode != NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CEngine::s_plat_unload_scene(CScene *p_scene)
{

	Dbg_MsgAssert(p_scene,("Trying to delete a NULL scene"));

	CPs2Scene * p_ps2_scene = (CPs2Scene*)p_scene;

	NxPs2::CGeomNode *p_geomnode = p_ps2_scene->GetEngineScene();

	if (p_geomnode)
	{
		// clean up the node we created
		p_geomnode->Cleanup();

		// Pip::Unload the file
		Pip::Unload(p_scene->GetSceneFilename());
	}
	
	// Clone scene node
#if 0		// Moved this into ~CPs2Scene so that this CGeomNode isn't destroyed before the cloned sectors
	NxPs2::CGeomNode *p_clone_geomnode = p_ps2_scene->GetEngineCloneScene();
	if (p_clone_geomnode)
	{
		p_clone_geomnode->Cleanup();
		delete p_clone_geomnode;
	}
#endif

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CModel*		CEngine::s_plat_init_model(void)
{
	CPs2Model* pModel = new CPs2Model;

	return pModel;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CEngine::s_plat_uninit_model(CModel* pModel)
{
	Dbg_Assert( pModel );

	delete pModel;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGeom*	CEngine::s_plat_init_geom(void)
{
	CPs2Geom* pGeom = new CPs2Geom;

	return pGeom;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CEngine::s_plat_uninit_geom(CGeom* pGeom)
{
	Dbg_Assert( pGeom );

	delete pGeom;

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
	Dbg_Assert( pQuickAnim );

	delete pQuickAnim;

	return;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CMesh*	CEngine::s_plat_load_mesh(uint32 id, uint32* pModelData, int modelDataSize, uint8* pCASData, Nx::CTexDict* pTexDict, uint32 texDictOffset, bool isSkin, bool doShadowVolume)
{
	CPs2Mesh* pMesh = new CPs2Mesh(pModelData, modelDataSize, pCASData, pTexDict, texDictOffset, isSkin, doShadowVolume);

	return pMesh;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CMesh*	CEngine::s_plat_load_mesh(const char* pMeshFileName, Nx::CTexDict* pTexDict, uint32 texDictOffset, bool isSkin, bool doShadowVolume)
{
	CPs2Mesh* pMesh = new CPs2Mesh(pMeshFileName, pTexDict, texDictOffset, isSkin, doShadowVolume);

	return pMesh;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CEngine::s_plat_unload_mesh(CMesh* pMesh)
{
	Dbg_Assert( pMesh );

	delete pMesh;

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CEngine::s_plat_set_mesh_scaling_parameters( SMeshScalingParameters* pParams )
{
	NxPs2::SetMeshScalingParameters( pParams );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSprite *	CEngine::s_plat_create_sprite(CWindow2D *p_window)
{
	return new CPs2Sprite(p_window);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CEngine::s_plat_destroy_sprite(CSprite *p_sprite)
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
	return new NxPs2::CPs2Textured3dPoly;
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
	return NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CEngine::s_plat_project_texture_into_scene( Nx::CTexture *p_texture, Nx::CModel *p_model, Nx::CScene *p_scene )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CEngine::s_plat_set_projection_texture_camera( Nx::CTexture *p_texture, Gfx::Camera *p_camera )
{
	#if !STENCIL_SHADOW
	// Currently assumes a top down view. So all we need to store is the position of the camera.
	// The width and height of the view frustum (this is an orthographic camera) are constant.
	Mth::Vector pos = p_camera->GetPos();
	NxPs2::SetTextureProjectionCamera( &pos, &pos );
	#endif
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_stop_projection_texture( Nx::CTexture *p_texture )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_add_occlusion_poly( uint32 num_verts, Mth::Vector *p_vert_array, uint32 checksum )
{
	if( num_verts == 4 )
	{
		NxPs2::AddOcclusionPoly( p_vert_array[0], p_vert_array[1], p_vert_array[2], p_vert_array[3], checksum );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_enable_occlusion_poly( uint32 checksum, bool enable )
{
	NxPs2::EnableOcclusionPoly( checksum, enable );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_remove_all_occlusion_polys( void )
{
	NxPs2::RemoveAllOcclusionPolys();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// returns true if the sphere at "center", with the "radius"
// is visible to the current camera
// (note, currently this is the last frame's camera on PS2)
bool CEngine::s_plat_is_visible( Mth::Vector&	center, float radius  )
{
	return NxPs2::IsVisible(center,radius);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_set_max_multipass_distance(float dist)
{
	NxPs2::render::sMultipassMaxDist = dist;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const char *		CEngine::s_plat_get_platform_extension()
{
	return "PS2";	// String literals are statically allocated so can be returned safely, (Bjarne, p90)
}


/******************************************************************/
// Wait for any pending asyncronous rendering to finish, so rendering
// data can be unloaded
/******************************************************************/

void CEngine::s_plat_finish_rendering()
{
		NxPs2::WaitForRendering();		// PS2 Specific, wait for prior frame's DMA to finish	
} 

/******************************************************************/
// Set the amount that the previous frame is blended with this frame
// 0 = none	  	(just see current frame) 	
// 128 = 50/50
// 255 = 100% 	(so you only see the previous frame)												  
/******************************************************************/

void CEngine::s_plat_set_screen_blur(uint32 amount )
{
	amount = (255-amount)/2;
	uint32 alpha = *NxPs2::p_patch_ALPHA;
	alpha &= 0xffffff00;
	alpha |= amount; 
	*NxPs2::p_patch_ALPHA = alpha;	
	*NxPs2::p_patch_ALPHA2 = alpha;	
} 

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	CEngine::s_plat_get_num_soundtracks()
{
	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const char* CEngine::s_plat_get_soundtrack_name( int soundtrack_number )
{
	return NULL;
}



// Add some PS2 specific debug info to the metrics script structure
void	CEngine::s_plat_get_metrics(Script::CStruct * p_info)
{

	// get the scene, and get the root 

	Script::CStruct *p_ps2 = new Script::CStruct();
	
	NxPs2::CGeomMetrics * p_metrics = new NxPs2::CGeomMetrics;

	CPs2Scene * p_ps2_scene = (CPs2Scene *) sGetMainScene();
	if (p_ps2_scene->GetEngineScene())
	{
		p_ps2_scene->GetEngineScene()->CountMetrics(p_metrics);
		p_ps2->AddInteger(CRCD(0xd55cd658,"NULLEngineScene"),0);
	}	
	else
	{
		// K: Added this so that the script refresh_poly_count in lighttool.q
		// can display "N/A" for the values.
		p_ps2->AddInteger(CRCD(0xd55cd658,"NULLEngineScene"),1);
	}

	p_ps2->AddInteger(CRCD(0x3dd01ea1,"total"), p_metrics->m_total);        
	p_ps2->AddInteger(CRCD(0x3960ff18,"leaf"),  p_metrics->m_leaf);  
	p_ps2->AddInteger(CRCD(0x4de5330c,"objects"),  p_metrics->m_object);  
	p_ps2->AddInteger(CRCD(0x94927839,"vert"),  p_metrics->m_verts);  
	p_ps2->AddInteger(CRCD(0x169ee151,"poly"),  p_metrics->m_polys);  
	p_ps2->AddFloat(CRCD(0x58785e10,"verts_per_poly"),  (float)p_metrics->m_verts/(float)p_metrics->m_polys);  
	p_ps2->AddFloat(CRCD(0x3b75d7cc,"polys_per_object"),  (float)p_metrics->m_polys/(float)p_metrics->m_object);  
	p_ps2->AddFloat(CRCD(0xbc56249d,"polys_per_mesh"),  (float)p_metrics->m_polys/(float)p_metrics->m_leaf);  
		   
	p_info->AddStructurePointer(CRCD(0x26861025,"scene"),p_ps2);
	
	delete	p_metrics;


}


void CEngine::s_plat_set_letterbox( bool letterbox )
{
	NxPs2::DoLetterbox = letterbox;
} 
 


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::s_plat_set_color_buffer_clear( bool clear )
{
}



}