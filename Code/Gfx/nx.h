///////////////////////////////////////////////////////////////////////////////////
// NX.H - Neversoft Engine, Rendering portion, Platform independent interface

#ifndef	__GFX_NX_H__
#define	__GFX_NX_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/hashtable.h>

#include <gfx/NxScene.h>
#include <gfx/NxSector.h>
#include <gfx/NxSprite.h>
#include <gfx/NxImposter.h>
#include <gfx/NxNewParticleMgr.h>
#include <gfx/nxtexture.h>
#include <gfx/nxtextured3dpoly.h>
#include <gfx/nxweather.h>

namespace Obj
{
	class CObject;
}

namespace Gfx
{
	class Camera;
}

namespace Script
{
	class CStruct;
	class CScript;
}

namespace Nx
{
	// forward declarations
	class CGeom;
	class CMesh;
	class CModel;
	class CQuickAnim;
	class CWindow2D;
	class CParticle;

// GJ:  The following is for doing cutscene head scaling...
// These parameters will only be active during the loading
// of the next CMesh.
struct SMeshScalingParameters
{
	char* 			pWeightIndices;
	float* 			pWeights;
	Mth::Vector* 	pBonePositions;
	Mth::Vector* 	pBoneScales;
};

////////////////////////////////////////////////////////////////////////
// Globally accessible Engine functions ar public static functions, 
// just use them like:
//
//  Nx::CSector * p_sector = Nx::CEngine::sGetSector(checksum);
//
// // NOT // I might change this later so the engine is a class, so it would become:
// // NOT // Nx::CSector * p_sector = Nx::CEngine::Instance()->GetSector(checksum);


// The engine class is all static
class CEngine
{
public:
	static CSector	*			sGetSector(uint32 sector_checksum);			// get a sector pointer, based on the checksum of the name
	static void					sStartEngine();
	static void					sSuspendEngine();
	static void					sResumeEngine();
	static void					sPreRender();
	static void					sPostRender();
	static void					sRenderWorld();
	
	static void					sFinishRendering();

	static void					sSetScreenBlur(uint32 amount);	
	static uint32				sGetScreenBlur();	
	
	static CScene	*			sCreateScene(const char *p_name, CTexDict *p_tex_dict,
											 bool add_super_sectors = true);	// creates an empty scene
	static CScene	*			sLoadScene(const char *p_name, CTexDict *p_tex_dict, bool add_super_sectors = true,
										   bool is_sky = false, bool is_dictionary = false, bool is_net=false);	// load a platform specific scene file
	static CScene	*			sAddScene(const char *p_scene_name, const char *p_filename);	// add a scene file to an existing scene
	static void					sToggleAddScenes();
	static bool					sUnloadScene(CScene *p_scene);
	static bool					sQuickReloadGeometry(const char *p_scene_name);	

	static SSec::Manager * 		sGetNearestSuperSectorManager(Mth::Line &);		// Returns the closest SuperSector Manager
	static SSec::Manager * 		sGetNearestSuperSectorManager(Mth::Vector &);	// Returns the closest SuperSector Manager

	static	const char *		sGetPlatformExtension();   					// return platform specifc extension, like ".PS2"
	
	static bool					sUnloadAllScenesAndTexDicts();				// unloads all loaded scenes and their associated texture dicts
	
	static	CScene *			sGetScene(uint32 id);		  				// get scene, given the checksum id of the name
	static	CScene *			sGetScene(const char *p_name);				// get scene, given name
	static 	CScene *  			sGetMainScene();							// get first non-sky scene
	static 	CScene *  			sGetSkyScene();								// get first sky scene

	static CSprite *			sCreateSprite(CWindow2D *p_window = NULL);
	static bool					sDestroySprite(CSprite *p_sprite);
								
	static CTextured3dPoly *	sCreateTextured3dPoly();
	static bool					sDestroyTextured3dPoly(CTextured3dPoly *p_poly);
	
											  
	static CTexture *			sCreateRenderTargetTexture( int width, int height, int depth, int z_depth );
	static void					sProjectTextureIntoScene( Nx::CTexture *p_texture, Nx::CModel *p_model, Nx::CScene *p_scene = NULL);
	static void					sSetProjectionTextureCamera( Nx::CTexture *p_texture, Gfx::Camera *p_camera );
	static void					sStopProjectionTexture( Nx::CTexture *p_texture );

	static void					sAddOcclusionPoly( uint32 num_verts, Mth::Vector *p_vert_array, uint32 checksum = 0 );
	static void					sEnableOcclusionPoly( uint32 checksum, bool enable );
	
	static CModel*				sInitModel();
	static bool					sUninitModel(CModel* pModel);

	static CGeom*				sInitGeom();
	static bool					sUninitGeom(CGeom* pGeom);

	static CQuickAnim*			sInitQuickAnim();
	static void					sUninitQuickAnim(CQuickAnim* pQuickAnim);

	static CMesh*				sLoadMesh(const char* pFileName, uint32 texDictOffset, bool forceTexDictLookup, bool doShadowVolume);
	static CMesh*				sLoadMesh(uint32 id, uint32* pModelData, int modelDataSize, uint8* pCASData, uint32 textureDictChecksum, uint32* pTextureData, int textureDataSize, uint32 textureDictOffset, bool isSkin, bool doShadowVolume );
	static bool					sUnloadMesh(CMesh* pMesh);

	static void					sSetMeshScalingParameters( SMeshScalingParameters* pParams );

	static	void				sSetMovableObjectManager(Obj::CGeneralManager* p_object_manager);
	static Lst::Head<Obj::CObject> & sGetMovableObjects();					// Returns the list of all movable objects
	static 	Lst::HashTable< CParticle > * sGetParticleTable();

	static CImposterManager*	sGetImposterManager( void )	{ return mp_imposter_manager; }
	static CNewParticleManager*	sGetParticleManager( void )	{ return mp_particle_manager; }

	static uint32				sGetUnusedSectorChecksum();					// Gets a unique sector checksum for cloning

	static bool					sIsVisible(Mth::Vector &center, float radius);										
										
	static void					sSetMaxMultipassDistance(float dist);		// Sets distance at which multipass will stop drawing

	static void					sSetLetterbox(bool lettterbox);

	static void					sSetColorBufferClear(bool clear);				// Indicates whether a per-frame color buffer clear is required (in addition to z-buffer clear).
	static CWeather*			sGetWeather( void ) { return mp_weather; }

	// K: Added so that park editor can update the grid after geometry has changed.
	static bool					ScriptWeatherUpdateGrid( Script::CStruct* pParams, Script::CScript* pScript ) { mp_weather->UpdateGrid(); return true; }
	
	static bool					ScriptWeatherSetRainHeight( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetRainHeight( pParams, pScript ); }
	static bool					ScriptWeatherSetRainFrames( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetRainFrames( pParams, pScript ); } 
	static bool					ScriptWeatherSetRainLength( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetRainLength( pParams, pScript ); } 
	static bool					ScriptWeatherSetRainBlendMode( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetRainBlendMode( pParams, pScript ); } 
	static bool					ScriptWeatherSetRainRate( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetRainRate( pParams, pScript ); } 
	static bool					ScriptWeatherSetRainColor( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetRainColor( pParams, pScript ); }  

	static bool					ScriptWeatherSetSplashRate( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetSplashRate( pParams, pScript ); } 
	static bool					ScriptWeatherSetSplashLife( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetSplashLife( pParams, pScript ); } 
	static bool					ScriptWeatherSetSplashSize( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetSplashSize( pParams, pScript ); } 
	static bool					ScriptWeatherSetSplashColor( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetSplashColor( pParams, pScript ); } 
	static bool					ScriptWeatherSetSplashBlendMode( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetSplashBlendMode( pParams, pScript ); } 

	static bool					ScriptWeatherSetSnowHeight( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetSnowHeight( pParams, pScript ); }
	static bool					ScriptWeatherSetSnowFrames( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetSnowFrames( pParams, pScript ); } 
	static bool					ScriptWeatherSetSnowSize( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetSnowSize( pParams, pScript ); } 
	static bool					ScriptWeatherSetSnowBlendMode( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetSnowBlendMode( pParams, pScript ); } 
	static bool					ScriptWeatherSetSnowRate( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetSnowRate( pParams, pScript ); } 
	static bool					ScriptWeatherSetSnowColor( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetSnowColor( pParams, pScript ); }  
	
	static bool					ScriptWeatherSetSnowActive( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetSnowActive( pParams, pScript ); }  
	static bool					ScriptWeatherSetRainActive( Script::CStruct* pParams, Script::CScript* pScript ) { return mp_weather->ScriptWeatherSetRainActive( pParams, pScript ); }  

	//Some Debugging functions
	static	void				sToggleRenderMode();
	static	void				sSetRenderMode(int mode) {s_render_mode = mode;}
	static	void				sSetWireframeMode(int mode) {s_wireframe_mode = mode;}
	static	void				sSetDebugIgnore(uint32 ignore_1, uint32 ignore_0);
	static	void				sDebugCheckForHoles();
	static  uint32				GetRenderMode() {return s_render_mode;}
	static  uint32				GetWireframeMode() {return s_wireframe_mode;}
	
	// Debug inspection stuff								
	static	void				sGetMetrics(Script::CStruct* p_info);

	// soundtrack stuff (hook to xbox specific funcs)
	static	int					sGetNumSoundtracks();
	static	const char*			sGetSoundtrackName( int soundtrack_number );

	// Added by Ken, for use by replay code, which needs to store the status of the sectors
	// so that they can be restored when the replay finishes.
	static	int					sWriteSectorStatusBitfield(uint32 *p_bitfield, int numUint32s);
	static	void				sReadSectorStatusBitfield(uint32 *p_bitfield, int numUint32s);
	
	static	void				sInitReplayStartState();
	static	void				sPrepareSectorsForReplayPlayback(bool store_initial_state);
	static	void				sRestoreSectorsAfterReplayPlayback();
	
	
	
	// Constants
	enum
	{
		MAX_LOADED_SCENES = 4
	};

private:
	// Platform specific function calls	
	static void					s_plat_start_engine();
	static void					s_plat_suspend_engine();
	static void					s_plat_resumeEngine();
	static void					s_plat_pre_render();
	static void					s_plat_post_render();
	static void					s_plat_render_world();
	
	static void					s_plat_set_screen_blur(uint32 amount);

	static void					s_plat_set_letterbox(bool letterbox);
	static void					s_plat_set_color_buffer_clear(bool clear);
			  
	static CScene	*			s_plat_create_scene(const char *p_name, CTexDict *p_tex_dict,
													bool add_super_sectors = true);	// creates an empty scene
	static CScene	*			s_plat_load_scene(const char *p_name, CTexDict *p_tex_dict, bool add_super_sectors,
												  bool is_sky, bool is_dictionary);		// load a platform specific scene file
	static CScene	*			s_plat_load_scene_from_memory(void *p_data, CTexDict *p_tex_dict, bool add_super_sectors,
												  bool is_sky, bool is_dictionary);		// load a platform specific scene file
	static bool					s_plat_add_scene(CScene *p_scene, const char *p_filename);
	static bool					s_plat_unload_scene(CScene *p_scene);

	static CSprite *			s_plat_create_sprite(CWindow2D *p_window);
	static bool					s_plat_destroy_sprite(CSprite *p_sprite);

	static CTextured3dPoly *	s_plat_create_textured_3d_poly();
	static bool					s_plat_destroy_textured_3d_poly(CTextured3dPoly *p_poly);

	static CTexture *			s_plat_create_render_target_texture( int width, int height, int depth, int z_depth );
	static void					s_plat_project_texture_into_scene( Nx::CTexture *p_texture, Nx::CModel *p_model, Nx::CScene *p_scene );
	static void					s_plat_set_projection_texture_camera( Nx::CTexture *p_texture, Gfx::Camera *p_camera );
	static void					s_plat_stop_projection_texture( Nx::CTexture *p_texture );

	static void					s_plat_add_occlusion_poly( uint32 num_verts, Mth::Vector *p_vert_array, uint32 checksum = 0 );
	static void					s_plat_enable_occlusion_poly( uint32 checksum, bool enable );
	static void					s_plat_remove_all_occlusion_polys( void );

	static	const char *		s_plat_get_platform_extension();

	static CModel*				s_plat_init_model();
	static bool					s_plat_uninit_model(CModel* pModel);

	static CGeom*				s_plat_init_geom();
	static bool					s_plat_uninit_geom(CGeom* pGeom);

	static CQuickAnim*			s_plat_init_quick_anim();
	static void					s_plat_uninit_quick_anim(CQuickAnim* pQuickAnim);

	static CMesh*				s_plat_load_mesh(const char* pMeshFileName, Nx::CTexDict* pTexDict, uint32 texDictOffset, bool isSkin, bool doShadowVolume);
	static CMesh*				s_plat_load_mesh(uint32 id, uint32* pModelData, int modelDataSize, uint8* pCASData, Nx::CTexDict* pTexDict, uint32 texDictOffset, bool isSkin, bool doShadowVolume);
	static bool					s_plat_unload_mesh(CMesh* pMesh);
	
	static void					s_plat_set_mesh_scaling_parameters( SMeshScalingParameters* pParams );

	static bool					s_plat_is_visible(Mth::Vector& center, float radius);										

	static void					s_plat_set_max_multipass_distance(float dist);

	static	void				s_plat_finish_rendering();
		
	static 	int					s_plat_get_num_soundtracks();
	static	const char*			s_plat_get_soundtrack_name( int soundtrack_number );
	
	#ifdef	__PLAT_NGPS__
	// a debug function just for the PS2 - although we could well have it on all platforms
	static 	void 				s_plat_get_metrics(Script::CStruct * p_info);
	#endif

	// Member variables
	static CScene *				sp_loaded_scenes[MAX_LOADED_SCENES];

	static uint32				s_next_avail_sector_checksum;				// next checksum available for clones
	
	static	uint32				s_screen_blur;
	static	uint32				s_render_mode;
	static	uint32				s_wireframe_mode;
	static	uint32				s_debug_ignore_0;
	static	uint32				s_debug_ignore_1;
	
	static 	Lst::HashTable< CParticle > *p_particle_table;
	static  Obj::CGeneralManager* 		sp_moving_object_manager;

	static CImposterManager*	mp_imposter_manager;	
	static CNewParticleManager*	mp_particle_manager;
	static CWeather*			mp_weather;
};


bool ScriptToggleAddScenes(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptSetScreenBlur(Script::CStruct *pParams, Script::CScript *pScript);


}


#endif

