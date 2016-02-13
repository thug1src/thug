///////////////////////////////////////////////////////////////////////////////////
// NX.CPP - Platform independent interface to the platfrom specific engine code 

#include <gfx/nx.h>

#include <core/debug.h>
#include <core/string/stringutils.h>

#include <gel/objman.h>
                               
#include <gel/scripting/script.h>	   
#include <gel/scripting/struct.h>	   
#include <gel/scripting/symboltable.h>

#include <gfx/nxfont.h>
#include <gfx/nxmesh.h>
#include <gfx/nxmodel.h>
#include <gfx/nxsector.h>
#include <gfx/nxtexman.h>
#include <gfx/nxviewman.h>
#include <gfx/nxwin2d.h>
#include <gfx/nxmiscfx.h>
#include <gfx/nxparticlemgr.h>
#include <gfx/nxquickanim.h>
#include <gfx/nxlightman.h>
#include <gfx\nxweather.h>
#ifdef __PLAT_NGC__
#include <gfx\ngc\p_nxscene.h>
#endif		// __PLAT_NGC__

//#include <sk/modules/skate/skate.h>		// For getting list of movable objects

#include <sys/replay/replay.h>

#ifdef	__PLAT_NGPS__
namespace NxPs2
{
	extern int geom_stats_total;
	extern int geom_stats_inactive ;
	extern int geom_stats_sky;
	extern int geom_stats_transformed;
	extern int geom_stats_skeletal;
	extern int geom_stats_camera_sphere;
	extern int geom_stats_culled;
	extern int geom_stats_leaf_culled;
	extern int geom_stats_clipcull;
	extern int geom_stats_boxcheck;
	extern int geom_stats_occludecheck;
	extern int geom_stats_occluded;
	extern int geom_stats_colored;
	extern int geom_stats_leaf;
	extern int geom_stats_wibbleUV;
	extern int geom_stats_wibbleVC;
	extern int geom_stats_sendcontext;
	extern int geom_stats_sorted;
	extern int geom_stats_shadow;

	extern bool DoLetterbox;
}
#endif

namespace	Nx
{

///////////////////////////////////////////////////////////////////
// Static definitions

CScene *						CEngine::sp_loaded_scenes[CEngine::MAX_LOADED_SCENES];
Lst::HashTable< CParticle >*	CEngine::p_particle_table = NULL;
uint32							CEngine::s_next_avail_sector_checksum = 1;	// 0 is invalid
uint32							CEngine::s_render_mode;
uint32							CEngine::s_wireframe_mode;
uint32							CEngine::s_screen_blur;
uint32							CEngine::s_debug_ignore_1;
uint32							CEngine::s_debug_ignore_0;
Obj::CGeneralManager*			CEngine::sp_moving_object_manager;
CImposterManager*				CEngine::mp_imposter_manager	= NULL;
CNewParticleManager*			CEngine::mp_particle_manager	= NULL;
CWeather*						CEngine::mp_weather = NULL;

/******************************************************************/
/*                                                                */
/* get a sector pointer, based on the checksum of the name		  */
/*                                                                */
/******************************************************************/

CSector	*		CEngine::sGetSector(uint32 sector_checksum)
{
	for (int i = 0; i < MAX_LOADED_SCENES; i++)
	{
		if (sp_loaded_scenes[i])
		{
			CSector *ret_sector = sp_loaded_scenes[i]->GetSector(sector_checksum);
			if (ret_sector) return ret_sector;
		}
	}

	// If we got here, we didn't find anything
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CEngine::sStartEngine()
{		
	for (int i = 0; i < MAX_LOADED_SCENES; i++)
	{
		Dbg_Assert(!sp_loaded_scenes[i]);	// Make sure it was deallocated
	}

	// Allocate CTexts and CWindow2Ds
	Nx::CTextMan::sAllocTextPool();
	Nx::CWindow2DManager::sAllocWindow2DPool();

	CViewportManager::sSetScreenAspect(4.0f/3.0f);
	CViewportManager::sSetDefaultAngle( Script::GetFloat("camera_fov") );
	CViewportManager::sSetScreenAngle( 0.0f);		// set to default

	CViewportManager::sInit();
	CViewportManager::sSetScreenMode(vONE_CAM);

	//sSetMaxMultipassDistance(3000.0f);

	// Allocate particle system table
	p_particle_table = new Lst::HashTable<CParticle>( 6 ); 
						 
	// Create the imposter manager.
	mp_imposter_manager = new CImposterManager;

	// Startup effects allocations.
	Nx::MiscFXInitialize();

	s_plat_start_engine();				

}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CEngine::sSuspendEngine()
{
	printf ("STUB: SuspendEngine\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CEngine::sResumeEngine()
{
	printf ("STUB: ResumeEngine\n");
}
						
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CEngine::sPreRender()
{
	s_plat_pre_render();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CEngine::sPostRender()
{
	s_plat_post_render();

	// Clear away any old cameras
	CViewportManager::sDeleteMarkedCameras();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CEngine::sRenderWorld()
{
	process_particles( Tmr::FrameLength() ); 

	ScreenFlashUpdate();
	TextureSplatUpdate();
	ShatterUpdate();
	
	CLightManager::sUpdateVCLights();
	
	s_plat_render_world();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CScene	*		CEngine::sCreateScene(const char *p_name, CTexDict *p_tex_dict,
									  bool add_super_sectors)
{
	CScene *p_created_scene = s_plat_create_scene(p_name, p_tex_dict, add_super_sectors);

	p_created_scene->SetID(Script::GenerateCRC(p_name)); 	// store the checksum of the scene name

	p_created_scene->SetTexDict(p_tex_dict);					  

	// Put in scene array
	int i;
	for (i = 0; i < MAX_LOADED_SCENES; i++)
	{
		if (!sp_loaded_scenes[i])
		{
			sp_loaded_scenes[i] = p_created_scene;
			break;
		}
	}

	Dbg_MsgAssert(i < MAX_LOADED_SCENES, ("Have more than MAX_LOADED SCENES"));

	return p_created_scene;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CScene	*		CEngine::sLoadScene(const char *p_name, CTexDict *p_tex_dict,
									bool add_super_sectors, bool is_sky, bool is_dictionary, bool is_net)
{
	// first expand the scene file name out to include the path
	// the level directory, and the platform specific extension
	// e.g.,  "can" become levels\can\can.scn.ps2

	

	char	full_name[128];
#ifdef __PLAT_NGPS__
	sprintf(full_name,"levels\\%s\\%s%s.geom.%s",p_name,p_name,is_net?"_net":"",sGetPlatformExtension());
#else
	sprintf(full_name,"levels\\%s\\%s%s.scn.%s",  p_name,p_name,is_net?"_net":"",sGetPlatformExtension());
#endif

	Mem::PushMemProfile((char*)full_name);

	CScene *loaded_scene = s_plat_load_scene(full_name, p_tex_dict, add_super_sectors, is_sky, is_dictionary);

	loaded_scene->SetID(Script::GenerateCRC(p_name)); 	// store the checksum of the scene name

	loaded_scene->SetTexDict(p_tex_dict);					  
					  
	// Do post-load processing
	loaded_scene->PostLoad(full_name);

	// Put in scene array
	int i;
	for (i = 0; i < MAX_LOADED_SCENES; i++)
	{
		if (!sp_loaded_scenes[i])
		{
			sp_loaded_scenes[i] = loaded_scene;
			break;
		}
	}

	Dbg_MsgAssert(i < MAX_LOADED_SCENES, ("Have more than MAX_LOADED SCENES"));
	
	Mem::PopMemProfile(/*p_name*/);

	return loaded_scene;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CScene	*		CEngine::sAddScene(const char *p_scene_name, const char *p_filename)
{
	// first expand the scene file name out to include the path
	// the level directory, and the platform specific extension
	// e.g.,  "can" become levels\can\can.scn.ps2

	char	full_name[128];
#ifdef __PLAT_NGPS__
	sprintf(full_name,"levels\\%s\\%s.geom.%s",p_filename,p_filename,sGetPlatformExtension());
#else
	sprintf(full_name,"levels\\%s\\%s.scn.%s",p_filename,p_filename,sGetPlatformExtension());
#endif

	char	texture_dict_name[128];
	sprintf(texture_dict_name,"levels\\%s\\%s.tex",p_filename,p_filename);

	CScene *p_scene = sGetScene(Script::GenerateCRC(p_scene_name));

	Dbg_MsgAssert(p_scene, ("sAddScene(): Can't find existing scene %s", p_scene_name));

	Dbg_Message("Adding to scene %s with file %s", p_scene_name, full_name);

	// Check if we need to unload old one
	if (strcmp(p_scene->GetAddSceneFilename(), full_name) == 0)
	{
		Dbg_Message("Unloading old added scene %s", full_name);
		p_scene->UnloadAddScene();

		Nx::CTexDict * p_old_tex_dict = Nx::CTexDictManager::sGetTextureDictionary(Crc::GenerateCRCFromString(texture_dict_name));
		if (p_old_tex_dict)		// See if we previously loaded this
		{
			Dbg_Message("Unloading old texture dictionary %s", texture_dict_name);
			Nx::CTexDictManager::sUnloadTextureDictionary(p_old_tex_dict);
		} else {
			Dbg_Assert(0);
		}
	}

	Nx::CTexDict * p_tex_dict = Nx::CTexDictManager::sLoadTextureDictionary(texture_dict_name,true);
	Dbg_MsgAssert(p_tex_dict, ("ERROR loading tex dict for %s\n",texture_dict_name));

	Dbg_Message("Adding to scene %s with file %s", p_scene_name, full_name);

	s_plat_add_scene(p_scene, full_name);

	// Do post-add processing
	p_scene->PostAdd(full_name, p_tex_dict);

	return p_scene;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CEngine::sToggleAddScenes()
{
	for (int i = 0; i < MAX_LOADED_SCENES; i++)
	{
		if (sp_loaded_scenes[i])
		{
			sp_loaded_scenes[i]->ToggleAddScene();
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CEngine::sUnloadScene(CScene *p_scene)
{

	// Wait for the frame to finish rendering before we unload the scene
	// otherwise we might be trying to render something that is no longer there	
	// (Without this line, the engine can hang when doing a quickview, and seems
	// to be more crashy when using particle systems)
	Nx::CEngine::sFinishRendering();	
	
	// Remove Incremental Data first
	// (Doesn't work yet because the replace CSectors are lost and the texture dictionary is still around)
	p_scene->UnloadAddScene();

	// remove platform specific assets
	if (s_plat_unload_scene(p_scene))
	{
//#ifdef __PLAT_NGC__
//		CNgcScene *p_ngc_scene = (CNgcScene*)p_scene;
//		NxNgc::sScene * p_engine_scene = p_ngc_scene->GetEngineScene();
//#endif		// __PLAT_NGC__

		// delete the scene object itself. 
		delete	p_scene;

//#ifdef __PLAT_NGC__
//		// Get the engine specific scene data and pass it to the engine to delete.
//		NxNgc::DeleteScene( p_engine_scene );
//		p_ngc_scene->SetEngineScene( NULL );
//#endif		// __PLAT_NGC__

		// and remove it from the list of scenes
		for (int i = 0; i < MAX_LOADED_SCENES; i++)
		{
			if (sp_loaded_scenes[i] == p_scene)
			{
				sp_loaded_scenes[i] = NULL;
				return true;
			}
		}
	}

	Dbg_Error("Could not find CScene to unload");
	return false;
}


// (Not working yet!!!)							  
// This is useful function to be only used during development
// it attempts to reload just the .scn part of a level
// without reloading the texture dictionary (assumes it stays the same)
// and without reloading the collision (assumes you are in viewer mode, and don't need it)  				  
bool			CEngine::sQuickReloadGeometry(const char *p_scene_name)
{	
	// find the scene
	Nx::CScene	*p_scene = sGetScene(p_scene_name);
	
	if (p_scene)
	{
		CTexDict *p_tex_dict = p_scene->GetTexDict();
		bool	is_sky = p_scene->IsSky();
		
		sUnloadScene(p_scene);
		/*Nx::CScene	*p_new_scene = */ sLoadScene(p_scene_name,p_tex_dict,!is_sky,is_sky);
//		sUnloadScene(p_scene);
//		p_new_scene = sLoadScene(p_scene_name,p_tex_dict,!is_sky,is_sky);
		
		return true;
	}
	return false;
}


// Given the id of a scene, then return the CScene that has this id
// returns NULL if scene not found
CScene *		CEngine::sGetScene(uint32 id)
{
	for (int i = 0; i < MAX_LOADED_SCENES; i++)
	{
		if (sp_loaded_scenes[i] && (sp_loaded_scenes[i]->GetID() == id))
		{
			return sp_loaded_scenes[i];
		}
	}
	return NULL;
}


CScene *		CEngine::sGetScene(const char *p_name)
{
	return		sGetScene(Script::GenerateCRC(p_name));
}


// Get the last non-sky scene.  Kind of application dependent, as it assumes 
// that the "Main" scene is the last non sky scene
// but that's a pretty safe bet for what we are doing now.
// (Mick:  I'm only using this for the "Disco" cheat in THPS4)
// (Mick: now this function is used in ParseNodeArray, so I had to change this from
// using the first non-sky to the last non-sky, so now it's even dodgier....
// we need a better way of saying what is the "main" scene
CScene *  		CEngine::sGetMainScene()
{
	for (int i = MAX_LOADED_SCENES-1; i >= 0; i--)
	{
		if (sp_loaded_scenes[i] && !sp_loaded_scenes[i]->IsSky())
		{
			return sp_loaded_scenes[i];
		}
	}
	return NULL;	
}

// Get the first sky scene.
CScene *  		CEngine::sGetSkyScene()
{
	for (int i = 0; i < MAX_LOADED_SCENES; i++)
	{
		if (sp_loaded_scenes[i] && sp_loaded_scenes[i]->IsSky())
		{
			return sp_loaded_scenes[i];
		}
	}
	return NULL;	
}

  
// Mick:::::   ........  
// a temporary utility function
// unloads all loaded scenes, (level and sky)
// and also unloads the texture dictionaries
// (or at least decrements the reference counts)
bool		CEngine::sUnloadAllScenesAndTexDicts()
{
	for (int i = 0; i < MAX_LOADED_SCENES; i++)
	{
		CScene * p_scene = sp_loaded_scenes[i];
		if (p_scene)
		{
			CTexDict *p_tex_dict = p_scene->GetTexDict();

    		sUnloadScene(p_scene);
			// must unload dictionary after scene
			if (p_tex_dict)
			{
				CTexDictManager::sUnloadTextureDictionary(p_tex_dict);
			}
		}
	}

	// Remove occlusion polys.
	s_plat_remove_all_occlusion_polys();
	
	return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CSprite *		CEngine::sCreateSprite(CWindow2D *p_window)
{
	return s_plat_create_sprite(p_window);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CEngine::sDestroySprite(CSprite *p_sprite)
{
	return s_plat_destroy_sprite(p_sprite);
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTextured3dPoly *CEngine::sCreateTextured3dPoly()
{
	return s_plat_create_textured_3d_poly();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CEngine::sDestroyTextured3dPoly(CTextured3dPoly *p_poly)
{
	return s_plat_destroy_textured_3d_poly(p_poly);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTexture *CEngine::sCreateRenderTargetTexture( int width, int height, int depth, int z_depth )
{
	return s_plat_create_render_target_texture( width, height, depth, z_depth );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CEngine::sProjectTextureIntoScene( Nx::CTexture *p_texture, Nx::CModel *p_model, Nx::CScene *p_scene )
{
	// usually we don't want to know the scene, so we pass in NULL
	// and let the engine figure it out
	// if we have multiple scenes, then we would need to figure this out externally

	if (!p_scene)
	{
		// find the first non-sky scene
		for (int i = 0; i < MAX_LOADED_SCENES; i++)
		{
			if (sp_loaded_scenes[i] && !sp_loaded_scenes[i]->IsSky())
			{
				p_scene = sp_loaded_scenes[i];
				break;
			}
		}
	}

	// It no longer matters if the scene is NULL.
	s_plat_project_texture_into_scene( p_texture, p_model, p_scene );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CEngine::sSetProjectionTextureCamera( Nx::CTexture *p_texture, Gfx::Camera *p_camera )
{
	s_plat_set_projection_texture_camera( p_texture, p_camera );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::sStopProjectionTexture( Nx::CTexture *p_texture )
{
	s_plat_stop_projection_texture( p_texture );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::sAddOcclusionPoly( uint32 num_verts, Mth::Vector *p_vert_array, uint32 checksum )
{
	s_plat_add_occlusion_poly( num_verts, p_vert_array, checksum );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::sEnableOcclusionPoly( uint32 checksum, bool enable )
{
	s_plat_enable_occlusion_poly( checksum, enable );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CEngine::sDebugCheckForHoles()
{

	Tmr::Time start_time, end_time;
	start_time = Tmr::GetTime();

	for (int i = 0; i < MAX_LOADED_SCENES; i++)
	{
		if (sp_loaded_scenes[i] && !sp_loaded_scenes[i]->IsSky())
		{
			sp_loaded_scenes[i]->DebugCheckForHoles();
		}
	}

	end_time = Tmr::GetTime();

	Dbg_Message("CheckForHoles() took %d msecs", end_time - start_time);
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CModel*			CEngine::sInitModel(void)
{
	return s_plat_init_model();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CEngine::sUninitModel(CModel* pModel)
{
	// Wait for any async rendering to finish before we proceed	
#ifndef __PLAT_NGC__
	sFinishRendering();
#endif		// __PLAT_NGC__
	return s_plat_uninit_model(pModel);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CGeom*			CEngine::sInitGeom()
{
	return s_plat_init_geom();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CEngine::sUninitGeom(CGeom* pGeom)
{
	// Wait for any async rendering to finish before we proceed	
#ifndef __PLAT_NGC__
	sFinishRendering();
#endif		// __PLAT_NGC__
	return s_plat_uninit_geom(pGeom);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CQuickAnim*		CEngine::sInitQuickAnim(void)
{
	return s_plat_init_quick_anim();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
			
void			CEngine::sUninitQuickAnim(CQuickAnim* pQuickAnim)
{
	s_plat_uninit_quick_anim(pQuickAnim);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CMesh*			CEngine::sLoadMesh(const char* pFileName, uint32 texDictOffset, bool forceTexDictLookup, bool doShadowVolume)
{
	char baseName[512];
	strcpy( baseName, pFileName );

	char* pEnd;

	bool foundSkin = false;
	pEnd = strstr( baseName, ".skin" );
	foundSkin = ( pEnd != NULL );
	if ( foundSkin )
	{
		*pEnd = NULL;
	}

	bool foundModel = false;
	pEnd = strstr( baseName, ".mdl" );
	foundModel = ( pEnd != NULL );
	if ( foundModel )
	{
		*pEnd = NULL;
	}

	Dbg_Assert( foundSkin || foundModel );

	char meshName[512];
	Str::LowerCase( meshName );
	sprintf( meshName, "%s.%s.%s", baseName, foundSkin ? "skin" : "mdl", sGetPlatformExtension() );

	char textureName[512];
	sprintf( textureName, "%s.tex", baseName );

	// brad - special case for boardshop boards.
	// (ideally, this would be coming in from a
	// higher level, such as with the
	// "forceTexDictLookup" flag
	if ( strstr( meshName, "thps4board_01" ) )
	{
		forceTexDictLookup = true;
	}
	
	Nx::CTexDict* pTexDict = Nx::CTexDictManager::sLoadTextureDictionary(textureName, false, texDictOffset, foundSkin, forceTexDictLookup);
	Dbg_MsgAssert(pTexDict,("ERROR loading tex dict for %s\n",textureName));

	Mem::PushMemProfile((char*)meshName);	
	CMesh* p_mesh = s_plat_load_mesh(meshName, pTexDict, texDictOffset, foundSkin, doShadowVolume);
	Mem::PopMemProfile(/*(char*)meshname*/);	
	return p_mesh;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CMesh*			CEngine::sLoadMesh(uint32 id, uint32* pModelData, int modelDataSize, uint8* pCASData, uint32 textureDictChecksum, uint32* pTextureData, int textureDataSize, uint32 textureDictOffset, bool isSkin, bool doShadowVolume )
{
	Dbg_MsgAssert( id != 0, ( "invalid id checksum" ) );

	bool forceTexDictLookup = false;

	Nx::CTexDict* pTexDict = Nx::CTexDictManager::sLoadTextureDictionary(textureDictChecksum, pTextureData, textureDataSize, false, textureDictOffset, isSkin, forceTexDictLookup);
	Dbg_MsgAssert(pTexDict,("ERROR loading tex dict from buffer"));

	Mem::PushMemProfile((char*)"model_data_buffer");	
	CMesh*	p_mesh = s_plat_load_mesh(id, pModelData, modelDataSize, pCASData, pTexDict, textureDictOffset, isSkin, doShadowVolume);
	Mem::PopMemProfile(/*(char*)meshname*/);	
	return p_mesh;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool			CEngine::sUnloadMesh(CMesh* pMesh)
{
	CTexDict *p_tex_dict = pMesh->GetTextureDictionary();
	
	bool result = s_plat_unload_mesh(pMesh);

	CTexDictManager::sUnloadTextureDictionary( p_tex_dict );

	return result;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CEngine::sSetMeshScalingParameters( SMeshScalingParameters* pParams )
{
	s_plat_set_mesh_scaling_parameters( pParams );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// Set the moving object manager.  Perviously we asked the skater module for 
// this, but now we are independent of Skate, so we have to be told

void	CEngine::sSetMovableObjectManager(Obj::CGeneralManager* p_object_manager)
{
	Dbg_MsgAssert(p_object_manager,("NULL Movable Object Manager"));
	sp_moving_object_manager = p_object_manager;
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Lst::Head<Obj::CObject> & CEngine::sGetMovableObjects()
{

	Dbg_MsgAssert( sp_moving_object_manager, ("NULL sp_moving_object_manager"));
	return sp_moving_object_manager->GetRefObjectList();
}

Lst::HashTable< CParticle > * CEngine::sGetParticleTable()
{
	return p_particle_table;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

SSec::Manager *	CEngine::sGetNearestSuperSectorManager(Mth::Line &collision_line)
{
	// For now, just return the first manager
	for (int i = 0; i < MAX_LOADED_SCENES; i++)
	{
		if (sp_loaded_scenes[i])
		{ 
			SSec::Manager *p_man = sp_loaded_scenes[i]->GetSuperSectorManager();
			if (p_man) return p_man;
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

SSec::Manager *	CEngine::sGetNearestSuperSectorManager(Mth::Vector &collision_point)
{
	// For now, just return the first manager
	for (int i = 0; i < MAX_LOADED_SCENES; i++)
	{
		if (sp_loaded_scenes[i])
		{ 
			SSec::Manager *p_man = sp_loaded_scenes[i]->GetSuperSectorManager();
			if (p_man) return p_man;
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32			CEngine::sGetUnusedSectorChecksum()
{
	// Make sure this checksum isn't being used
	while (sGetSector(s_next_avail_sector_checksum))
		s_next_avail_sector_checksum++;

	return s_next_avail_sector_checksum++;
}

/******************************************************************/
/*                                                                */
/* return platform specifc extension, like ".PS2"				  */
/*                                                                */
/******************************************************************/

const char *	CEngine::sGetPlatformExtension()
{
	return s_plat_get_platform_extension();
}

/******************************************************************/
// Wait for any pending asyncronous rendering to finish, so rendering
// data can be unloaded
// (Might be a wait, or a flush, depending on platform)
// This function should generally return immediatly if there is no
// pending rendering.  However, it might stall for up to a frame if
// there are things waiting to render.
// so if it is to be used in a real-time situation, then you want to
// call it as late as possible in the frame (preferably the last thing)
// so it incurs as little wait as possible
// this might be accomplished by making the "task" that uses it
// be as low priority as possible
/******************************************************************/					  

void	CEngine::sFinishRendering()
{
	s_plat_finish_rendering();
}


void				CEngine::sToggleRenderMode()
{
	s_render_mode++;
	if (s_render_mode == 4)
	{
		s_render_mode = 0;	
	}
}


void				CEngine::sSetDebugIgnore(uint32 ignore_1, uint32 ignore_0)
{
	s_debug_ignore_0 = ignore_0;
	s_debug_ignore_1 = ignore_1;
}

bool				CEngine::sIsVisible(Mth::Vector &center, float radius)
{
	// PATCH for multiple viewports
	// always return true, until we sort out the per-viewport visibility (which might not be needed)				   
	if (Nx::CViewportManager::sGetNumActiveViewports() > 1)
	{
		return true;
	}

	return s_plat_is_visible(center,radius);
}

void				CEngine::sSetMaxMultipassDistance(float dist)
{
	s_plat_set_max_multipass_distance(dist);
}

void				CEngine::sSetScreenBlur(uint32 amount)
{
	Dbg_MsgAssert(amount >=0 && amount <=255,("Screen blur amount out of range of 0..255 (it's %d)",amount));
	s_screen_blur = amount;
	s_plat_set_screen_blur(amount);
}

uint32 CEngine::sGetScreenBlur()
{
	uint32	amount = s_screen_blur; 
	Dbg_MsgAssert(amount >=0 && amount <=255,("s_screen_blur out of range of 0..255 (it's %d), maybe corrupted???",amount));
	return amount;	
}

int CEngine::sGetNumSoundtracks()
{
	return s_plat_get_num_soundtracks();
}

const char* CEngine::sGetSoundtrackName( int soundtrack_number )
{
	const char* p_soundtrack_name_wide = s_plat_get_soundtrack_name( soundtrack_number );
	return p_soundtrack_name_wide;
}

// Added by Ken for use by replay code.
// 
int CEngine::sWriteSectorStatusBitfield(uint32 *p_bitfield, int numUint32s)
{
	return 0;
	
	// Removed since we no longer have saving of replays to mem card.
	// Leaving this code in was causing a problem in that the passed numUint32s
	// was too small for the Skillzilla Park. Rather than increasing numUint32's
	// and hence using up more memory, it was safer just to remove the contents of this function.
	#if 0
	Dbg_MsgAssert(p_bitfield,("NULL p_bitfield"));

	int i;
	for (i=0; i<numUint32s; ++i)
	{
		p_bitfield[i]=0;
	}
		
	int long_index=0;
	int bit_index=0;
	
	for (i = 0; i<MAX_LOADED_SCENES; ++i)
	{
		if (sp_loaded_scenes[i])
		{
			Lst::HashTable<CSector> *p_table=sp_loaded_scenes[i]->GetSectorList();
			Dbg_MsgAssert(p_table,("NULL p_table"));
			
			p_table->IterateStart();
			CSector *p_sector=p_table->IterateNext();
			while (p_sector)
			{
				if (p_sector->GetActiveAtReplayStart())
				{
					Dbg_MsgAssert(long_index<numUint32s,("Too many sectors for bitfield !"));
					p_bitfield[long_index] |= (1<<bit_index);
				}	
				
				++bit_index;
				if (bit_index>=32)
				{
					bit_index=0;
					++long_index;
				}	

				if (p_sector->GetVisibleAtReplayStart())
				{
					Dbg_MsgAssert(long_index<numUint32s,("Too many sectors for bitfield !"));
					p_bitfield[long_index] |= (1<<bit_index);
				}	
				
				++bit_index;
				if (bit_index>=32)
				{
					bit_index=0;
					++long_index;
				}	
				
				p_sector=p_table->IterateNext();
			}
		}
	}	
	
	return long_index*32+bit_index;
	#endif
}

void CEngine::sReadSectorStatusBitfield(uint32 *p_bitfield, int numUint32s)
{
	return;
	
	// Removed since we no longer have saving of replays to mem card.
	// Leaving this code in was causing a problem in that the passed numUint32s
	// was too small for the Skillzilla Park. Rather than increasing numUint32's
	// and hence using up more memory, it was safer just to remove the contents of this function.
	#if 0
	Dbg_MsgAssert(p_bitfield,("NULL p_bitfield"));

	int long_index=0;
	int bit_index=0;
	
	for (int i = 0; i<MAX_LOADED_SCENES; ++i)
	{
		if (sp_loaded_scenes[i])
		{
			Lst::HashTable<CSector> *p_table=sp_loaded_scenes[i]->GetSectorList();
			Dbg_MsgAssert(p_table,("NULL p_table"));
			
			p_table->IterateStart();
			CSector *p_sector=p_table->IterateNext();
			while (p_sector)
			{
				Dbg_MsgAssert(long_index<numUint32s,("sReadSectorStatusBitfield: Too many sectors for bitfield !"));
				if (p_bitfield[long_index] & (1<<bit_index))
				{
					p_sector->SetActiveAtReplayStart(true);
				}	
				else
				{
					p_sector->SetActiveAtReplayStart(false);
				}	
				
				++bit_index;
				if (bit_index>=32)
				{
					bit_index=0;
					++long_index;
				}	

				Dbg_MsgAssert(long_index<numUint32s,("sReadSectorStatusBitfield: Too many sectors for bitfield !"));
				if (p_bitfield[long_index] & (1<<bit_index))
				{
					p_sector->SetVisibleAtReplayStart(true);
				}	
				else
				{
					p_sector->SetVisibleAtReplayStart(false);
				}	
				
				++bit_index;
				if (bit_index>=32)
				{
					bit_index=0;
					++long_index;
				}	
				
				p_sector=p_table->IterateNext();
			}
		}
	}	
	#endif
}

void CEngine::sInitReplayStartState()
{
	//printf("Calling sInitReplayStartState()\n");
	for (int i = 0; i<MAX_LOADED_SCENES; ++i)
	{
		if (sp_loaded_scenes[i])
		{
			Lst::HashTable<CSector> *p_table=sp_loaded_scenes[i]->GetSectorList();
			Dbg_MsgAssert(p_table,("NULL p_table"));
			
			p_table->IterateStart();
			CSector *p_sector=p_table->IterateNext();
			while (p_sector)
			{
				p_sector->SetActiveAtReplayStart(p_sector->IsActive());
				p_sector->SetVisibleAtReplayStart(p_sector->GetVisibility()&0x01);
				// Just to be safe
				p_sector->SetStoredActive(p_sector->IsActive());
				p_sector->SetStoredVisible(p_sector->GetVisibility()&0x01);
				
				p_sector=p_table->IterateNext();
			}
		}
	}	
}

void CEngine::sPrepareSectorsForReplayPlayback(bool store_initial_state)
{
	//printf("Calling sPrepareSectorsForReplayPlayback()\n");
	for (int i = 0; i<MAX_LOADED_SCENES; ++i)
	{
		if (sp_loaded_scenes[i])
		{
			Lst::HashTable<CSector> *p_table=sp_loaded_scenes[i]->GetSectorList();
			Dbg_MsgAssert(p_table,("NULL p_table"));
			
			p_table->IterateStart();
			CSector *p_sector=p_table->IterateNext();
			while (p_sector)
			{
				if (store_initial_state)
				{
					p_sector->SetStoredActive(p_sector->IsActive());
					p_sector->SetStoredVisible(p_sector->GetVisibility()&0x01);
				}	  
				  		
				p_sector->SetActive(p_sector->GetActiveAtReplayStart());
				p_sector->SetVisibility(p_sector->GetVisibleAtReplayStart()?0xff:0x00);
				
				p_sector=p_table->IterateNext();
			}
		}
	}	
}

void CEngine::sRestoreSectorsAfterReplayPlayback()
{
	//printf("Calling sRestoreSectorsAfterReplayPlayback()\n");
	for (int i = 0; i<MAX_LOADED_SCENES; ++i)
	{
		if (sp_loaded_scenes[i])
		{
			Lst::HashTable<CSector> *p_table=sp_loaded_scenes[i]->GetSectorList();
			Dbg_MsgAssert(p_table,("NULL p_table"));
			
			p_table->IterateStart();
			CSector *p_sector=p_table->IterateNext();
			while (p_sector)
			{
				p_sector->SetActive(p_sector->GetStoredActive());
				p_sector->SetVisibility(p_sector->GetStoredVisible()?0xff:0x00);
				
				p_sector=p_table->IterateNext();
			}
		}
	}	
}

bool ScriptToggleAddScenes(Script::CStruct *pParams, Script::CScript *pScript)
{
	// Just call engine function
	CEngine::sToggleAddScenes();

	return true;
}


bool ScriptSetScreenBlur(Script::CStruct *pParams, Script::CScript *pScript)
{
	int amount = 0;
	pParams->GetInteger(NONAME,&amount,false);
	CEngine::sSetScreenBlur(amount);
	return true;
}


// Given a script structure, add some metrics to it
// like number of objects, polygons, collision polygons, etc
void	CEngine::sGetMetrics(Script::CStruct * p_info)
{

	// Do main scene first, as it's more interesting
	if (sGetMainScene())
	{
		Script::CStruct *p_main = new Script::CStruct();
		sGetMainScene()->GetMetrics(p_main);
		p_info->AddStructurePointer(CRCD(0x9b10525c,"MainScene"),p_main);
	}
		
	// But also do the sky, just in case												  
	if (sGetSkyScene())
	{
		Script::CStruct *p_sky = new Script::CStruct();
		sGetSkyScene()->GetMetrics(p_sky);
		p_info->AddStructurePointer(CRCD(0x2c5ea34,"SkyScene"),p_sky);
	}

	#ifdef	__PLAT_NGPS__
	

		Script::CStruct *p_ps2 = new Script::CStruct();
		p_ps2->AddInteger(CRCD(0x3dd01ea1,"total"), NxPs2::geom_stats_total);        
		p_ps2->AddInteger(CRCD(0x742cf11f,"inactive"), NxPs2::geom_stats_inactive );    
		p_ps2->AddInteger(CRCD(0xf9d98b10,"sky"), NxPs2::geom_stats_sky);          
		p_ps2->AddInteger(CRCD(0x6d65d103,"transformed"), NxPs2::geom_stats_transformed);  
		p_ps2->AddInteger(CRCD(0x52aa1a77,"skeletal"), NxPs2::geom_stats_skeletal);     
		p_ps2->AddInteger(CRCD(0xaf792e8d,"camera_sphere"), NxPs2::geom_stats_camera_sphere);
		p_ps2->AddInteger(CRCD(0xa863e48c,"clipcull"), NxPs2::geom_stats_clipcull);     
		p_ps2->AddInteger(CRCD(0x93070c4b,"culled"), NxPs2::geom_stats_culled);     
		p_ps2->AddInteger(CRCD(0xac43724a,"leaf_culled"), NxPs2::geom_stats_leaf_culled);     
		p_ps2->AddInteger(CRCD(0x68306ffe,"boxcheck"), NxPs2::geom_stats_boxcheck);     
		p_ps2->AddInteger(CRCD(0xae81c5f6,"occludecheck"), NxPs2::geom_stats_occludecheck); 
		p_ps2->AddInteger(CRCD(0xba802ab0,"occluded"), NxPs2::geom_stats_occluded);     
		p_ps2->AddInteger(CRCD(0x1bc830f2,"colored"), NxPs2::geom_stats_colored);      
		p_ps2->AddInteger(CRCD(0x3960ff18,"leaf"), NxPs2::geom_stats_leaf);         
		p_ps2->AddInteger(CRCD(0x506a239f,"wibbleUV"), NxPs2::geom_stats_wibbleUV);     
		p_ps2->AddInteger(CRCD(0x169a94b7,"wibbleVC"), NxPs2::geom_stats_wibbleVC);     
		p_ps2->AddInteger(CRCD(0x7b471ba2,"sendcontext"), NxPs2::geom_stats_sendcontext);  
		p_ps2->AddInteger(CRCD(0x18725397,"sorted"), NxPs2::geom_stats_sorted);       
		p_ps2->AddInteger(CRCD(0x8a897dd2,"shadow"), NxPs2::geom_stats_shadow);       
		
		p_info->AddStructurePointer(CRCD(0xf2316b47,"PS2_Info"),p_ps2);

		s_plat_get_metrics(p_info);

#endif


}
					 					  


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void	CEngine::sSetLetterbox(bool letterbox)
{
	s_plat_set_letterbox( letterbox );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void	CEngine::sSetColorBufferClear(bool clear)
{
	s_plat_set_color_buffer_clear( clear );
}



} // namespace Nx




