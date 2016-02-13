///////////////////////////////////////////////////////////////////////////////////
// NxLightMan.H - Neversoft Engine, Rendering portion, Platform independent interface

#ifndef	__GFX_NX_LIGHT_MAN_H__
#define	__GFX_NX_LIGHT_MAN_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <core/math.h>
#include <gfx/image/imagebasic.h>

#ifndef __CORE_LIST_HASHTABLE_H
#include <core/hashtable.h>
#endif

#ifndef	__GFX_NXSECTOR_H__
#include <gfx/NxSector.h>
#endif


namespace Script
{
	class CScriptStructure;
	class CScript;
	class CStruct;
}

namespace Nx
{

// Forward declarations
class CModelLights;
class CSector;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
class CSceneLight
{
public:

	inline void						SetLightPosition( Mth::Vector &pos )	{ m_pos = pos; }
	inline Mth::Vector &			GetLightPosition( void )				{ return m_pos; }

	inline void						SetNameChecksum( uint32 cs )			{ m_name_checksum = cs; }
	inline uint32					GetNameChecksum( void )					{ return m_name_checksum; }

	inline void						SetLightColor( Image::RGBA col )		{ m_color = col; }
	inline Image::RGBA &			GetLightColor( void )					{ return m_color; }

	inline void						SetLightIntensity( float i )			{ m_intensity = i; }
	inline float					GetLightIntensity( void )				{ return m_intensity; }

	inline void						SetLightRadius( float r )				{ m_radius = r; m_reciprocal_radius = 1.0f / r; }
	inline float					GetLightRadius( void )					{ return m_radius; }
	inline float					GetLightReciprocalRadius( void )		{ return m_reciprocal_radius; }

protected:

	Mth::Vector						m_pos;
	float							m_radius;
	float							m_reciprocal_radius;
	float							m_intensity;				// Intensity in range [0.0, 1.0].
	Image::RGBA						m_color;
	uint32							m_name_checksum;			// Provides a link between the light and the node that created it.
};


struct SVCLight
{
	Mth::Vector						m_pos;
	float							m_intensity;		// [0.0, 1.0].
	float							m_radius;
	uint32							m_checksum;			// Checksum of the name of the node that corresponds to this light.
	Image::RGBA						m_color;
	Lst::HashTable< Nx::CSector >	*mp_sector_list;	// List of all the sectors that this light can affect.

									SVCLight();
									~SVCLight();


	void							SetPosition( Mth::Vector & pos )			{ m_pos = pos; }
	void							SetNameChecksum( uint32 cs )				{ m_checksum = cs; }
	void							SetIntensity( float i );
	void							SetColor( Image::RGBA col );
	void							SetRadius( float radius );
	void							ResetSectorList( void );
	void							CalculateSectorList( void );
	bool							AffectsSector( Nx::CSector *p_sector );
	Lst::HashTable< Nx::CSector >	*GetSectorList( void );
};

///////////////////////////////////////////////////////////////////////////////////
// Nx::CLightManager
class	CLightManager
{
public:

	// Constants
	enum
	{
		MAX_LIGHTS				= 2,
		MAX_SCENE_LIGHTS		= 256,	// These are static, designer placed lights in the level.
		MAX_VC_LIGHTS			= 150,
		WORLD_LIGHTS_STACK_SIZE	= 4,
	};

	// Upload lights to the engine (if applicable)
	static void					sUpdateEngine( void );

	// Create and destroy Lights
	static void					sClearLights();

	// World lights
	static bool					sSetLightAmbientColor(const Image::RGBA rgba);
	static Image::RGBA			sGetLightAmbientColor();

	static bool					sSetLightDirection(int light_index, const Mth::Vector &direction);
	static const Mth::Vector &	sGetLightDirection(int light_index);

	static bool					sSetLightDiffuseColor(int light_index, const Image::RGBA rgba);
	static Image::RGBA			sGetLightDiffuseColor(int light_index);

	static void					sSetAmbientLightModulationFactor(float factor);
	static float				sGetAmbientLightModulationFactor();
	static void					sSetDiffuseLightModulationFactor(int light_index, float factor);
	static float				sGetDiffuseLightModulationFactor(int light_index);

	// Push and pop world lights on stack
	static void					sPushWorldLights();
	static bool					sPopWorldLights();

	// Model Lights
	static CModelLights *		sCreateModelLights();
	static bool					sFreeModelLights(CModelLights *p_model_lights);

	// This is here temporarily
	static bool					sSetBrightness(float brightness);

	// Static scene lights.
	static CSceneLight *		sAddSceneLight( void );
	static CSceneLight *		sGetSceneLight( uint32 checksum );
	static CSceneLight *		sGetOptimumSceneLight( Mth::Vector & pos );
	static void					sClearSceneLights( void );

	static void					sClearVCLights();
	static void					sAddVCLight(uint32 name, Mth::Vector &pos, float intensity,
											Image::RGBA col, float outer_radius);

	static void					sClearCurrentFakeLightsCommand();	
	static void					sUpdateVCLights();
	static void					sFakeLight(uint32 id, Script::CStruct *pParams);
	static void					sFakeLights(const uint16 *p_nodes, int numNodes, int period, Script::CStruct *pParams);
	
protected:	

	// Set of lights
	struct SLightSet
	{
		// Ambient and diffuse lights
		Image::RGBA				m_light_ambient_rgba;
		Mth::Vector				m_light_direction[MAX_LIGHTS];
		Image::RGBA				m_light_diffuse_rgba[MAX_LIGHTS];

		// Modulation info
		float					m_ambient_light_modulation_factor;
		float					m_diffuse_light_modulation_factor[MAX_LIGHTS];
	};

	// World lights
	static SLightSet			s_world_lights;						// Current world lights
	static SLightSet			s_world_lights_stack[WORLD_LIGHTS_STACK_SIZE];
	static int					s_world_lights_stack_index;			// index to top of stack (-1 if empty)

	static CSceneLight			s_scene_lights[MAX_SCENE_LIGHTS];
	static int					s_num_scene_lights;

	static SVCLight				sp_vc_lights[MAX_VC_LIGHTS];
	static int					s_num_vc_lights;

	static uint16 *				sp_fake_lights_nodes;
	static int					s_num_fake_lights_nodes;
	static int					s_next_fake_light_node_to_process;
	static int					s_fake_lights_period;
	static int					s_fake_lights_current_time;
	static Script::CStruct *	sp_fake_lights_params;
	
	// These are here temporarily
	static float				s_brightness;
	static float				s_brightness_target;
	static float				s_min_brightness;
	static float				s_ambient_brightness;
	static float				s_diffuse_brightness[MAX_LIGHTS];

	static SVCLight *			s_get_vclight_from_checksum( uint32 cs );
	static void					s_recalculate_sector_lighting( Nx::CSector *p_sector );
	static void					s_apply_lighting( Nx::CScene * p_scene, uint32 checksum = 0 );

	// Platform-specific calls
	static void					s_plat_update_engine();
	static void					s_plat_update_lights();
	static void					s_plat_update_colors();
	static bool					s_plat_set_light_ambient_color();
	static bool					s_plat_set_light_direction(int light_index);
	static bool					s_plat_set_light_diffuse_color(int light_index);
	static Image::RGBA			s_plat_get_light_ambient_color();
	static const Mth::Vector &	s_plat_get_light_direction(int light_index);
	static Image::RGBA			s_plat_get_light_diffuse_color(int light_index);

	static CModelLights *		s_plat_create_model_lights();
	static bool					s_plat_free_model_lights(CModelLights *p_model_lights);

	// Friends
	friend CModelLights;
	friend CSceneLight;
};

bool ScriptSetLightAmbientColor(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptSetLightDirection(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptSetLightDiffuseColor(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptGetLightCurrentColor(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptPushWorldLights(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptPopWorldLights(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptDrawDirectionalLightLines(Script::CScriptStructure *pParams, Script::CScript *pScript);

}


#endif

