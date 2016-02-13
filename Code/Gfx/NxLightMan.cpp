///////////////////////////////////////////////////////////////////////////////
// NxLightMan.cpp

// start autoduck documentation
// @DOC NxLightMan
// @module NxLightMan | None
// @subindex Scripting Database
// @index script | NxLightMan

#include <core/defines.h>
#include <gel/Scripting/script.h>
#include <gel/scripting/struct.h>

#include <gfx/debuggfx.h> // for AddDebugLine( )
#include <sk/objects/skater.h>
#include <sk/gamenet/gamenet.h>
#include <sk/scripting/nodearray.h>
#include <core/math/math.h>

// This should eventually be removed, only needed for a call to GamePaused, which should not be
// called from here anyway.
#include <sk/modules/frontend/frontend.h>

#include <gfx/nx.h>
#include <gfx/NxGeom.h>

#include "gfx/NxLightMan.h"

namespace	Nx
{

/////////////////////////////////////////////////////////////////////////////
// CLightManager

// World lights
CLightManager::SLightSet CLightManager::s_world_lights =
{
					Image::RGBA(72, 72, 72, 0x80),															// Ambient RGBA
					{ Mth::Vector(0.5f, 0.8660254f, 0.0f, 0.0f), Mth::Vector(0.0f, 1.0f, 0.0f, 0.0f) },		// Light direction
					{ Image::RGBA(75, 75, 75, 0x80), Image::RGBA(0, 0, 0, 0x80) },							// Diffuse RGBA
					0.5f,																					// Ambient modulation
					{ 0.7f, 1.0f }																			// Diffuse modulation
};
CLightManager::SLightSet CLightManager::s_world_lights_stack[WORLD_LIGHTS_STACK_SIZE];
int						 CLightManager::s_world_lights_stack_index = -1;

// The following are temp variables.  These should be done per model
float				CLightManager::s_brightness = 1.0f;
float				CLightManager::s_brightness_target = 1.0f;

float				CLightManager::s_min_brightness = 0.0f;
float				CLightManager::s_ambient_brightness = 1.0f;
float				CLightManager::s_diffuse_brightness[MAX_LIGHTS] = { 1.0f, 1.0f };

CSceneLight			CLightManager::s_scene_lights[MAX_SCENE_LIGHTS];
int					CLightManager::s_num_scene_lights	= 0;

SVCLight			CLightManager::sp_vc_lights[MAX_VC_LIGHTS];
int					CLightManager::s_num_vc_lights = 0;
uint16 *			CLightManager::sp_fake_lights_nodes=NULL;
int					CLightManager::s_num_fake_lights_nodes=0;
int					CLightManager::s_next_fake_light_node_to_process=0;
int					CLightManager::s_fake_lights_period=0;
int					CLightManager::s_fake_lights_current_time=0;
Script::CStruct *	CLightManager::sp_fake_lights_params=NULL;


/////////////////////////////////////////////////////////////////////////////
// These functions are the platform independent part of the interface.
					 
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void				CLightManager::sUpdateEngine( void )
{
	s_plat_update_engine();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CLightManager::sClearLights()
{
	s_ambient_brightness = 1.0f;

	s_world_lights.m_light_ambient_rgba = Image::RGBA(0, 0, 0, 0x80);
	for (int i = 0; i < MAX_LIGHTS; i++)
	{
		s_world_lights.m_light_direction[i] = Mth::Vector(0.0f, 1.0f, 0.0f, 0.0f);
		s_world_lights.m_light_diffuse_rgba[i] = Image::RGBA(0, 0, 0, 0x80);

		s_diffuse_brightness[i] = 1.0f;
	}

	s_plat_update_lights();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CLightManager::sSetLightAmbientColor(const Image::RGBA rgba)
{
	s_world_lights.m_light_ambient_rgba = rgba;

	return s_plat_set_light_ambient_color();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA			CLightManager::sGetLightAmbientColor()
{
	//return s_light_ambient_rgba;
	return s_plat_get_light_ambient_color();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CLightManager::sSetLightDirection(int light_index, const Mth::Vector &direction)
{
	Dbg_Assert(light_index < MAX_LIGHTS);

	s_world_lights.m_light_direction[light_index] = direction;

	return s_plat_set_light_direction(light_index);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const Mth::Vector &	CLightManager::sGetLightDirection(int light_index)
{
	Dbg_Assert(light_index < MAX_LIGHTS);

	//return s_light_direction[light_index];
	return s_plat_get_light_direction(light_index);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CLightManager::sSetLightDiffuseColor(int light_index, const Image::RGBA rgba)
{
	s_world_lights.m_light_diffuse_rgba[light_index] = rgba;

	return s_plat_set_light_diffuse_color(light_index);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Image::RGBA			CLightManager::sGetLightDiffuseColor(int light_index)
{
	Dbg_Assert(light_index < MAX_LIGHTS);

	//return s_light_diffuse_rgba[light_index];
	return s_plat_get_light_diffuse_color(light_index);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CLightManager::sSetAmbientLightModulationFactor(float factor)
{
	s_world_lights.m_ambient_light_modulation_factor = factor;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CLightManager::sSetDiffuseLightModulationFactor(int light_index, float factor)
{
	Dbg_Assert(light_index < MAX_LIGHTS);

	s_world_lights.m_diffuse_light_modulation_factor[light_index] = factor;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float				CLightManager::sGetAmbientLightModulationFactor()
{
	return s_world_lights.m_ambient_light_modulation_factor;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float				CLightManager::sGetDiffuseLightModulationFactor(int light_index)
{
	Dbg_Assert(light_index < MAX_LIGHTS);

	return s_world_lights.m_diffuse_light_modulation_factor[light_index];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CLightManager::sPushWorldLights()
{
	if (++s_world_lights_stack_index < WORLD_LIGHTS_STACK_SIZE)
	{
		// Put on top of stack
		s_world_lights_stack[s_world_lights_stack_index] = s_world_lights;
	}
	else
	{
		--s_world_lights_stack_index;
		Dbg_MsgAssert(0, ("World light stack full"));
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CLightManager::sPopWorldLights()
{
	if (s_world_lights_stack_index >= 0)
	{
		// Get from top of stack and update the engine
		s_world_lights = s_world_lights_stack[s_world_lights_stack_index--];
		s_plat_update_lights();

		return true;
	}
	else
	{
		Dbg_MsgAssert(0, ("World light stack empty"));
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CModelLights *		CLightManager::sCreateModelLights()
{
	return s_plat_create_model_lights();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CLightManager::sFreeModelLights(CModelLights *p_model_lights)
{
	return s_plat_free_model_lights(p_model_lights);
}

/******************************************************************/
/*                                                                */
/*   This will eventually be done through CModel                  */
/*                                                                */
/******************************************************************/

bool				CLightManager::sSetBrightness(float brightness)
{
#if 0
	// Don't do anything now
	return true;
#endif

	s_brightness_target = brightness;

	// We've found the target brightness, so move actual brightness towards this to avoid sudden unnatural transitions.
	const float BRIGHTNESS_SPEED = 0.02f;
	if( s_brightness < s_brightness_target )
	{
		s_brightness += BRIGHTNESS_SPEED;
		if( s_brightness > s_brightness_target )
		{
			s_brightness = s_brightness_target;
		}
	}
	else if( s_brightness > s_brightness_target )
	{
		s_brightness -= BRIGHTNESS_SPEED;
		if( s_brightness < s_brightness_target )
		{
			s_brightness = s_brightness_target;
		}
	}

	s_ambient_brightness = 1.0f + ((2.0f * s_brightness) - 1.0f) * s_world_lights.m_ambient_light_modulation_factor;
	if (s_ambient_brightness < s_min_brightness)
	{
		s_ambient_brightness = s_min_brightness;
	}

	for (int i = 0; i < MAX_LIGHTS; i++)
	{
		s_diffuse_brightness[i] = 1.0f + ((2.0f * s_brightness) - 1.0f) * s_world_lights.m_diffuse_light_modulation_factor[i];
		if (s_diffuse_brightness[i] < s_min_brightness)
		{
			s_diffuse_brightness[i] = s_min_brightness;
		}
	}

	s_plat_update_colors();

	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CSceneLight * CLightManager::sAddSceneLight( void )
{
	if( s_num_scene_lights < MAX_SCENE_LIGHTS )
	{
		++s_num_scene_lights;
		return &s_scene_lights[s_num_scene_lights - 1];
	}
	return NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CLightManager::sClearSceneLights( void )
{
	s_num_scene_lights = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CLightManager::sClearVCLights()
{

// Need to flush the entries from each sector list before we "ignore" the light
// otherwise the hash items from one level hang around for another level, confusing us.
	
	for( int i = 0; i < s_num_vc_lights; ++i )
	{
		sp_vc_lights[i].ResetSectorList();
	}
	
	s_num_vc_lights=0;
}	

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CLightManager::sAddVCLight(uint32 name, Mth::Vector &pos, float intensity,
								Image::RGBA col, float outer_radius)
{
	Dbg_MsgAssert(s_num_vc_lights < MAX_VC_LIGHTS, ("Too many level lights that affect geometry (%d max)",MAX_VC_LIGHTS));
	if (s_num_vc_lights < MAX_VC_LIGHTS)
	{
		SVCLight *p_new_light = &sp_vc_lights[s_num_vc_lights++];
	
		p_new_light->SetNameChecksum( name );
		p_new_light->SetPosition( pos );
	
		p_new_light->SetIntensity( intensity * 0.01f );
		p_new_light->SetColor( col );
	
		// Setting the radius will implicitly calculate the sector list for this light.
		p_new_light->SetRadius( outer_radius );
	
		// Calculate which sectors will be affected.
		p_new_light->CalculateSectorList();
	
		if( intensity > 0.0f )
		{
			s_apply_lighting( Nx::CEngine::sGetMainScene(), name );
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CLightManager::sClearCurrentFakeLightsCommand()
{
	if (sp_fake_lights_nodes)
	{
		Mem::Free(sp_fake_lights_nodes);
		sp_fake_lights_nodes=NULL;
	}	
	s_num_fake_lights_nodes=0;
	s_next_fake_light_node_to_process=0;
	
	s_fake_lights_period=0;
	s_fake_lights_current_time=0;
	
	if (sp_fake_lights_params)
	{
		delete sp_fake_lights_params;
		sp_fake_lights_params=NULL;
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// Gets called from ScriptFakeLights in cfuncs.cpp
void CLightManager::sFakeLights(const uint16 *p_nodes, int numNodes, int period, Script::CStruct *pParams)
{
	sClearCurrentFakeLightsCommand();

	sp_fake_lights_params=new Script::CStruct;
	sp_fake_lights_params->AppendStructure(pParams);
	
	if (numNodes)
	{
		sp_fake_lights_nodes=(uint16*)Mem::Malloc(numNodes*sizeof(uint16));
		for (int i=0; i<numNodes; ++i)
		{
			sp_fake_lights_nodes[i]=p_nodes[i];
		}	
	}	
	s_num_fake_lights_nodes=numNodes;
	
	s_fake_lights_period=period;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// This gets called every frame from CEngine::sRenderWorld()
// It calls sFakeLight for each of the nodes in the array sp_fake_lights_nodes, but
// staggers the calls across frames so that they don't cause a CPU time glitch.
// Once sFakeLight ahs been called for all of them it calls sClearCurrentFakeLightsCommand()
void CLightManager::sUpdateVCLights()
{
	if (!sp_fake_lights_nodes)
	{
		return;
	}
	
	// TODO: At some point all the calls to GamePaused need to be removed from
	// the logic functions that are currently called from CEngine::sRenderWorld()
	if (Mdl::FrontEnd::Instance()->GamePaused())
	{
		return;
	}

	// If finished, clean up & stop.	
	if (s_fake_lights_current_time > s_fake_lights_period)
	{
		sClearCurrentFakeLightsCommand();
		return;
	}	
	
	// Calculate how many nodes ought to have been processed by now, based on the ratio of s_fake_lights_current_time
	// to s_fake_lights_period
	int required_num_nodes_processed;
	if (s_fake_lights_period)
	{
		required_num_nodes_processed = (s_num_fake_lights_nodes * s_fake_lights_current_time) / s_fake_lights_period;
	}	
	else
	{
		required_num_nodes_processed = s_num_fake_lights_nodes;
	}
	
	// Call sFakeLight for each node starting from where it got to last time, until the required number have
	// been processed.
	for (; s_next_fake_light_node_to_process<required_num_nodes_processed; ++s_next_fake_light_node_to_process)
	{
		Script::CStruct *p_node=SkateScript::GetNode(sp_fake_lights_nodes[s_next_fake_light_node_to_process]);
		uint32 id=0;
		p_node->GetChecksum(CRCD(0xa1dc81f9,"Name"),&id);
		sFakeLight(id,sp_fake_lights_params);
	}
	
	++s_fake_lights_current_time;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CSceneLight *CLightManager::sGetSceneLight( uint32 checksum )
{
	for( int l = 0; l < s_num_scene_lights; ++l )
	{
		if( s_scene_lights[l].GetNameChecksum() == checksum )
		{
			return &s_scene_lights[l];
		}
	}
	return NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CSceneLight * CLightManager::sGetOptimumSceneLight( Mth::Vector & pos )
{
	if( s_num_scene_lights > 0 )
	{
		float	best_ratio		= 1.0f;
		int		best_index		= -1;

		for( int l = 0; l < s_num_scene_lights; ++l )
		{
			// Only interested in lights that are active.
			if( s_scene_lights[l].GetLightIntensity() > 0.0f )
			{
				float dist = Mth::Distance( pos, s_scene_lights[l].GetLightPosition());
				if( dist < s_scene_lights[l].GetLightRadius())
				{
					// Potentially a usable light.
					float ratio = dist * s_scene_lights[l].GetLightReciprocalRadius();
					if( ratio < best_ratio )
					{
						// I wonder whether we should also consider intensity here? A further light, with a higher
						// intensity may have more effect than a nearer light with smaller intensity.
						best_ratio = ratio;
						best_index = l;
					}
				}
			}
		}

		if( best_index >= 0 )
		{
			return &s_scene_lights[best_index];
		}
	}
	return NULL;
}



/////////////////////////////////////////////////////////////////////////////
// The script functions

// @script | SetLightAmbientColor | Sets the global light ambient color
// @parm int | r | Red
// @parm int | g | Green
// @parm int | b | Blue
bool ScriptSetLightAmbientColor(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	int r, g, b;
	if (!pParams->GetInteger("r", &r))
	{
		Dbg_MsgAssert(0, ("Can't find 'r' color"));
	}
	if (!pParams->GetInteger("g", &g))
	{
		Dbg_MsgAssert(0, ("Can't find 'g' color"));
	}
	if (!pParams->GetInteger("b", &b))
	{
		Dbg_MsgAssert(0, ("Can't find 'b' color"));
	}
	
	Image::RGBA rgb(r, g, b, 0x80);

	return CLightManager::sSetLightAmbientColor(rgb);
}


// @script | SetLightDirection | Sets the unit direction vector of a global light
// @parm int | index | Light number
// @parm vector | direction | Unit direction vector (overrides heading and pitch)
// @parm float | heading | Heading angle (in degrees)
// @parm float | pitch | Pitch angle (in degrees)
bool ScriptSetLightDirection(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	int index;
	if (!pParams->GetInteger(CRCD(0x7f8c98fe,"index"), &index))
	{
		Dbg_MsgAssert(0, ("Can't find 'index' of light"));
	}

	float heading, pitch;
	Mth::Vector direction(0, 0, 1, 0);
	if (pParams->GetVector(CRCD(0xc1b52e4c,"direction"), &direction))
	{
		direction[W] = 0.0f;		// This is the only way to force this to be a vector (as opposed to a point)
		//Dbg_Message("************ direction (%f, %f, %f)", direction[X], direction[Y], direction[Z]);
	} else if (pParams->GetFloat(CRCD(0xfd4bc03e,"heading"), &heading) && pParams->GetFloat(CRCD(0xd8604126,"pitch"), &pitch)) {
		direction.RotateX(Mth::DegToRad(pitch));
		direction.RotateY(Mth::DegToRad(heading));
		//Dbg_Message("************ heading and pitch direction (%f, %f, %f)", direction[X], direction[Y], direction[Z]);
	} else {
		Dbg_MsgAssert(0, ("Can't find 'direction' or 'heading' and 'pitch' of light"));
	}

	return CLightManager::sSetLightDirection(index, direction);
}


// @script | SetLightDiffuseColor | Sets the diffuse color of a global light
// @parm int | index | Light number
// @parm int | r | Red
// @parm int | g | Green
// @parm int | b | Blue
bool ScriptSetLightDiffuseColor(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	int index;
	if (!pParams->GetInteger(CRCD(0x7f8c98fe,"index"), &index))
	{
		Dbg_MsgAssert(0, ("Can't find 'index' of light"));
	}

	int r, g, b;
	if (!pParams->GetInteger(CRCD(0x93f60062,"r"), &r))
	{
		Dbg_MsgAssert(0, ("Can't find 'r' color"));
	}
	if (!pParams->GetInteger(CRCD(0xfe2be489,"g"), &g))
	{
		Dbg_MsgAssert(0, ("Can't find 'g' color"));
	}
	if (!pParams->GetInteger(CRCD(0x8e411006,"b"), &b))
	{
		Dbg_MsgAssert(0, ("Can't find 'b' color"));
	}
	
	Image::RGBA rgb(r, g, b, 0x80);

	return CLightManager::sSetLightDiffuseColor(index, rgb);
}


// @script | GetLightCurrentColor | Gets the rgb values for ambient and directional lights
bool ScriptGetLightCurrentColor(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	int r=0, g=0, b=0;
    Image::RGBA rgb(r, g, b, 0x80);
    
    rgb = CLightManager::sGetLightAmbientColor();

    pScript->GetParams()->AddInteger(CRCD(0xed664897,"ambient_red"), rgb.r);
    pScript->GetParams()->AddInteger(CRCD(0x908bf647,"ambient_green"), rgb.g);
    pScript->GetParams()->AddInteger(CRCD(0x5f4fba78,"ambient_blue"), rgb.b);

    rgb = CLightManager::sGetLightDiffuseColor(0);

    pScript->GetParams()->AddInteger(CRCD(0xe213aeb7,"red_0"), rgb.r);
    pScript->GetParams()->AddInteger(CRCD(0x6ef19631,"green_0"), rgb.g);
    pScript->GetParams()->AddInteger(CRCD(0x5ec44213,"blue_0"), rgb.b);

    rgb = CLightManager::sGetLightDiffuseColor(1);

    pScript->GetParams()->AddInteger(CRCD(0x95149e21,"red_1"), rgb.r);
    pScript->GetParams()->AddInteger(CRCD(0x19f6a6a7,"green_1"), rgb.g);
    pScript->GetParams()->AddInteger(CRCD(0x29c37285,"blue_1"), rgb.b);
    
    /*float heading=0, pitch=0;
    Mth::Vector direction(0, 0, 1, 0);

    direction = CLightManager::sGetLightDirection(0);

    //printf("x=%i y=%i z=%i", direction[0], direction[1], direction[2]);

    //Do the opposite of these two lines
    //direction.RotateX(Mth::DegToRad(pitch));
    //direction.RotateY(Mth::DegToRad(heading));
        
    pScript->GetParams()->AddInteger("pitch_0", pitch);
    pScript->GetParams()->AddInteger("heading_0", heading);
    
    */

    return true;
}

// @script | PushWorldLights | Pushes current world lights onto stack, so they can be restored later.
bool ScriptPushWorldLights(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	CLightManager::sPushWorldLights();
	return true;
}

// @script | PopWorldLights | Restores last pushed world lights to the current world lights.
bool ScriptPopWorldLights(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	return CLightManager::sPopWorldLights();
}

// @script | DrawDirectionalLightLines | Draws two lines to represent the directional lights
bool ScriptDrawDirectionalLightLines(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	float heading;
	pParams->GetFloat( CRCD(0xfd4bc03e,"heading"), &heading, Script::ASSERT );

    float pitch;
	pParams->GetFloat( CRCD(0xd8604126,"pitch"), &pitch, Script::ASSERT );

    pitch = -pitch * Mth::PI / 180.0f;
    heading = ((-heading+90) * Mth::PI / 180.0f);
    
    int r, g, b;
    pParams->GetInteger( CRCD(0x93f60062,"r"), &r, Script::ASSERT );
    pParams->GetInteger( CRCD(0xfe2be489,"g"), &g, Script::ASSERT );
    pParams->GetInteger( CRCD(0x8e411006,"b"), &b, Script::ASSERT );
    Image::RGBA rgb(r, g, b, 0x80);
    //rgb = CLightManager::sGetLightDiffuseColor(0);

    Obj::CSkater *pSkater = Mdl::Skate::Instance()->GetLocalSkater();
	
	Mth::Vector pos = pSkater->GetPos();
	float x = pos.GetX();
	float y = ( pos.GetY() + 75 );
	float z = pos.GetZ();

    float hypyh = 50;
    
    float y0 = ( (sin(pitch) * hypyh) + y );
    float hypxz = (cos(pitch) * hypyh);
    float x0 = ( (cos(heading) * hypxz) + x );
    float z0 = ( (sin(heading) * hypxz) + z );
    
    Mth::Vector skaterpos(x, y, z, 0);
    Mth::Vector direction0(x0, y0, z0, 0);
    
    //printf("x=%i y=%i z=%i\n", direction0[0], direction0[1], direction0[2]);
    
    Gfx::AddDebugLine( skaterpos, direction0, MAKE_RGB( rgb.r, rgb.g, rgb.b ), MAKE_RGB( rgb.r, rgb.g, rgb.b ), 1 );
    
    return true;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
SVCLight *CLightManager::s_get_vclight_from_checksum( uint32 cs )
{
	for( int i = 0; i < s_num_vc_lights; ++i )
	{
		if( sp_vc_lights[i].m_checksum == cs )
		{
			return &sp_vc_lights[i];
		}
	}
	return NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
SVCLight::SVCLight( void )
{
	mp_sector_list = new Lst::HashTable< Nx::CSector >( 4 );

	m_radius	= 0.0f;
	m_intensity = 0.0f;

	// Default the color to pure white.
	m_color.r	= 0x80;
	m_color.g	= 0x80;
	m_color.b	= 0x80;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
SVCLight::~SVCLight( void )
{
	ResetSectorList();
	delete mp_sector_list;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SVCLight::SetRadius( float radius )
{
	Dbg_Assert( radius >= 0.0f );

	if( radius != m_radius )
	{
		m_radius = radius;

		// Deal with the SceneLight that may have been created from this node.
		Nx::CSceneLight *p_light = Nx::CLightManager::sGetSceneLight( m_checksum );
		if( p_light )
		{
			p_light->SetLightRadius( radius );
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SVCLight::SetIntensity( float i )
{
	Dbg_Assert(( i >= 0.0f ) && ( i <= 1.0f ));

	if( i != m_intensity )
	{
		m_intensity = i;

		// Deal with the SceneLight that may have been created from this node.
		Nx::CSceneLight *p_light = Nx::CLightManager::sGetSceneLight( m_checksum );
		if( p_light )
		{
			p_light->SetLightIntensity( i );
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SVCLight::SetColor( Image::RGBA col )
{
	m_color = col;

	// Deal with the SceneLight that may have been created from this node.
	Nx::CSceneLight *p_light = Nx::CLightManager::sGetSceneLight( m_checksum );
	if( p_light )
	{
		p_light->SetLightColor( col );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
Lst::HashTable< Nx::CSector > *SVCLight::GetSectorList( void )
{
	return mp_sector_list;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SVCLight::ResetSectorList( void )
{
	mp_sector_list->FlushAllItems();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool SVCLight::AffectsSector( Nx::CSector *p_sector )
{
	Nx::CSector *p_match = mp_sector_list->GetItem((uint32)p_sector );
    return ( p_match != NULL );	
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SVCLight::CalculateSectorList( void )
{
	// First reset the sector list.
	ResetSectorList();
	
	Nx::CScene *p_scene = Nx::CEngine::sGetMainScene();
	if( p_scene )
	{
		Lst::HashTable< Nx::CSector > *p_sector_list = p_scene->GetSectorList();
		if( p_sector_list )
		{
			p_sector_list->IterateStart();	
			Nx::CSector *p_sector = p_sector_list->IterateNext();
			while( p_sector )
			{
				// Only consider this sector if it is not in the "NoLevelLights" lightgroup.
				// and not in the Indoor group
				if( p_sector->GetLightGroup() != CRCD( 0xed82767b, "NoLevelLights" )
					&& p_sector->GetLightGroup() != CRCD(0xec6542d3,"indoor"))
				{
					Nx::CGeom *p_geom = p_sector->GetGeom();
					if( p_geom )
					{
						// See of the radius of the light will intersect the radius of the bounding box of the geometry.
						Mth::CBBox	bbox	= p_sector->GetBoundingBox();
						Mth::Vector	mid		= ( bbox.GetMax() + bbox.GetMin()) / 2.0f;
						float		r		= ( bbox.GetMax() - mid ).Length();
						float		dist	= ( mid - m_pos ).Length();
						if( dist < ( r + m_radius ))
						{
							mp_sector_list->PutItem((uint32)p_sector, p_sector );
						}
					}
				}
				p_sector = p_sector_list->IterateNext();
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CLightManager::s_recalculate_sector_lighting( Nx::CSector *p_sector )
{
	Nx::CGeom *p_geom = p_sector->GetGeom();
	if( p_geom == NULL )
	{
		return;
	}

	const int MAX_LIGHTS_AFFECTING_SECTOR = 8;

	int			current_lights_affecting_sector = 0;
	int			light_index[MAX_LIGHTS_AFFECTING_SECTOR];

	// Build a list of lights that affect this sector, to the maximum of MAX_LIGHTS_AFFECTING_SECTOR.
	for( int l = 0; l < s_num_vc_lights; ++l )
	{
		if(( sp_vc_lights[l].m_intensity > 0.0f ) && sp_vc_lights[l].AffectsSector( p_sector ))
		{
			light_index[current_lights_affecting_sector] = l;
			if( ++current_lights_affecting_sector >= MAX_LIGHTS_AFFECTING_SECTOR )
			{
				break;
			}
		}
	}

	if( current_lights_affecting_sector == 0 )
	{
		// No lights are affecting this sector, which can happen in the situation where the last remaining
		// light affecting a sector is turned off. In this case we just want to restore the original
		// vertex colors.
		int verts = p_geom->GetNumRenderVerts();
		if( verts > 0 )
		{
			Image::RGBA	*p_colors	= new Image::RGBA[verts];
			p_geom->GetOrigRenderColors( p_colors );
			p_geom->SetRenderColors( p_colors );
			delete [] p_colors;
		}
	}
	else if( current_lights_affecting_sector > 0 )
	{
		int verts = p_geom->GetNumRenderVerts();
		if( verts > 0 )
		{
			Mth::Vector	*p_verts	= new Mth::Vector[verts];
			Image::RGBA	*p_colors	= new Image::RGBA[verts];

			p_geom->GetRenderVerts( p_verts );
			p_geom->GetOrigRenderColors( p_colors );
					
			Image::RGBA *p_color = p_colors;
			Mth::Vector *p_vert = p_verts;
			for( int i = 0; i < verts; ++i )
			{
				float ir = 0.0f;
				float ig = 0.0f;
				float ib = 0.0f;
			
				for( int l = 0; l < current_lights_affecting_sector; ++l )
				{
					SVCLight *p_current_light = &sp_vc_lights[light_index[l]];

					float dist_sqr	= ( *p_vert - p_current_light->m_pos ).LengthSqr();
					if( dist_sqr < ( p_current_light->m_radius * p_current_light->m_radius ))
					{
						float dist		= sqrtf( dist_sqr );
						float intensity	= p_current_light->m_intensity * (( p_current_light->m_radius - dist ) / p_current_light->m_radius );

						ir += intensity * (float)p_current_light->m_color.r;
						ig += intensity * (float)p_current_light->m_color.g;
						ib += intensity * (float)p_current_light->m_color.b;
					}
				}

				// Apply to each r,g,b idividually, so we don't loose original color.
				Image::RGBA rgb = *p_color;
				float v = (float)rgb.r + ir;
				rgb.r = (uint8)(( v > 255.0f ) ? 255.0f : v );
							
				v = (float)rgb.g + ig;
				rgb.g = (uint8)(( v > 255.0f ) ? 255.0f : v );

				v = (float)rgb.b + ib;
				rgb.b = (uint8)(( v > 255.0f ) ? 255.0f : v );

				*p_color = rgb;

				++p_color;
				++p_vert;
			}

			p_geom->SetRenderColors( p_colors );
						
			delete [] p_verts;
			delete [] p_colors;
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CLightManager::s_apply_lighting( Nx::CScene * p_scene, uint32 checksum )
{
	// Now requires a checksum.
	if( checksum == 0 )
		return;

	// Now deal with geometry.
	SVCLight *p_scene_light = s_get_vclight_from_checksum( checksum );
	if( p_scene_light == NULL )
	{
		return;
	}

	Lst::HashTable< Nx::CSector > * p_sector_list = p_scene_light->GetSectorList();
	if( p_sector_list )
	{
		p_sector_list->IterateStart();	
		Nx::CSector *p_sector = p_sector_list->IterateNext();		
		while( p_sector )
		{
			Nx::CGeom *p_geom = p_sector->GetGeom();
			if( p_geom )
			{
				s_recalculate_sector_lighting( p_sector );
			}
			p_sector = p_sector_list->IterateNext();
		}
	}

#	if 0
	// Set up an array of lights, (eventually) culled from the node array.
	s_num_vc_lights = 0;

	Script::CArray *p_nodearray=Script::GetArray("NodeArray");
	for (uint32 i=0; i<p_nodearray->GetSize(); ++i)
	{
		Script::CStruct *p_node_struct=p_nodearray->GetStructure(i);
		Dbg_MsgAssert(p_node_struct,("Error getting node from node array for rail generation"));

		uint32 class_checksum = 0;		
		p_node_struct->GetChecksum( CRCD(0x12b4e660, "Class"), &class_checksum );
		if (class_checksum == CRCD(0xa0e52802,"LevelLight"))
		{
			SkateScript::GetPosition(p_node_struct,&sp_vc_lights[s_num_vc_lights].m_pos);
			
			sp_vc_lights[s_num_vc_lights].m_value = 100.0f;  		// default values
			sp_vc_lights[s_num_vc_lights].m_radius = 300.0f;
			
			p_node_struct->GetFloat(CRCD(0x2689291c,"Brightness"),&sp_vc_lights[s_num_vc_lights].m_value);	
			p_node_struct->GetFloat(CRCD(0xc48391a5,"Radius"),&sp_vc_lights[s_num_vc_lights].m_radius);	
			
			s_num_vc_lights++;
		}
	}
	Dbg_Message( "Found %d LevelLights\n", s_num_vc_lights );

	if( s_num_vc_lights == 0 )
	{
		Dbg_Message( "Did not find any LevelLight nodes, so using Pedestrian nodes....\n" );

		// Scan through the node array creating all things that need to be created.
		Script::CArray *p_nodearray=Script::GetArray("NodeArray");
		for (uint32 i=0; i<p_nodearray->GetSize(); ++i)
		{
			Script::CStruct *p_node_struct=p_nodearray->GetStructure(i);
			Dbg_MsgAssert(p_node_struct,("Error getting node from node array for rail generation"));
	
			uint32 class_checksum = 0;		
			p_node_struct->GetChecksum( CRCD(0x12b4e660, "Class"), &class_checksum );
			if (class_checksum == CRCD(0xa0dfac98,"Pedestrian"))
			{
				SkateScript::GetPosition(p_node_struct,&sp_vc_lights[s_num_vc_lights].m_pos);
				sp_vc_lights[s_num_vc_lights].m_value = 100.0f;
				sp_vc_lights[s_num_vc_lights].m_radius = 300.0f;
				s_num_vc_lights++;
			}
		}
	}

	// Go through each object, find the nearest light that intersects the object
	// and the  apply lighting from that
	Lst::HashTable< Nx::CSector > * p_sector_list = p_scene->GetSectorList();
	
	Image::RGBA rgb;
	
	if (p_sector_list)
	{
		p_sector_list->IterateStart();	
		Nx::CSector *p_sector = p_sector_list->IterateNext();		
		while(p_sector)
		{

			Nx::CGeom 	*p_geom = p_sector->GetGeom();
			
			if (p_geom)
			{
				// Need to find a light (if any) such that the radius of the light
				// will intersect the radius of the bounding box of the geometry
				Mth::CBBox bbox = p_sector->GetBoundingBox();
				Mth::Vector	mid = (bbox.GetMax() + bbox.GetMin())/2.0f;
				float	radius = (bbox.GetMax() - bbox.GetMin()).Length();

				int best_light = -1;
				float best_dist = 100000000000.0f;				
				for (int light = 0; light < s_num_vc_lights; light++)
				{
					float dist = (mid - sp_vc_lights[light].m_pos).Length();					
					if (dist < best_dist && dist < (radius+sp_vc_lights[light].m_radius))
					{
						best_dist = dist;
						best_light = light;
					}
				}
				
				if (best_light != -1)
				{
					
					Mth::Vector	light_pos = sp_vc_lights[best_light].m_pos;
					float	light_radius = sp_vc_lights[best_light].m_radius;
					float	light_radius_squared = light_radius * light_radius;
					float	light_value = sp_vc_lights[best_light].m_value * amount;
					
					
					//
					// Do renderable geometry
					int verts = p_geom->GetNumRenderVerts();
				
					if (verts)
					{
						Mth::Vector	*p_verts = new Mth::Vector[verts];
						Image::RGBA	*p_colors = new Image::RGBA[verts];
						p_geom->GetRenderVerts(p_verts);
						// Note: getting the original render colors will allocate lots of memory
						// to store all the origianl colors the firs tiem it is called
						//p_geom->GetOrigRenderColors(p_colors);
						
						// For lighting, we get the current colors, so needs to be applied in conjunction with a compressVC
						p_geom->GetRenderColors(p_colors);
					
						Image::RGBA *p_color = p_colors;
						Mth::Vector *p_vert = p_verts;
						for (int i = 0; i < verts; i++)
						{
				//			CalculateVertexLighting(*p_vert, *p_color);

							float dist_sqr = (*p_vert - light_pos).LengthSqr();
							float intensity = (light_radius_squared-dist_sqr)/light_radius_squared * light_value;
							if (intensity<0.0f)
							{
								intensity = 0.0f;
							}
							
							rgb = *p_color;
							float v;

							// Apply to each r,g,b idividually, so we don't loose original color							
							v = (float) rgb.r + intensity;
							if (v > 255.0f) v = 255.0f;
							rgb.r = (uint8) v; 
							
							v = (float) rgb.g + intensity;
							if (v > 255.0f) v = 255.0f;
							rgb.g = (uint8) v; 
							
							v = (float) rgb.b + intensity;
							if (v > 255.0f) v = 255.0f;
							rgb.b = (uint8) v; 
							
							
							 
							*p_color = rgb;		//(*p_color & 0xff000000) | color;
							//*(uint32*)p_color = Mth::Rnd(32767);		//(*p_color & 0xff000000) | color;
				
							p_color++;
							p_vert++;
						} // end for
				
						p_geom->SetRenderColors(p_colors);
						
						delete [] p_verts;
						delete [] p_colors;
					} // end if
					else
					{
						// debuggery
						//p_geom->SetColor(Image::RGBA(0,0,100,0));
					}
				}
			}
			p_sector = p_sector_list->IterateNext();
		}
	}
#	endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// K: Factored this out so that the FakeLights command could also adjust a set of prefixed nodes.
void CLightManager::sFakeLight(uint32 id, Script::CStruct *pParams)
{
	Nx::CScene *p_scene = Nx::CEngine::sGetMainScene();
	
	float	amount, radius;
	int		red, grn, blu;

	// Get the SVCLight created from this node.
	SVCLight *p_light = s_get_vclight_from_checksum( id );
	if( p_light )
	{
		// Determine the intensity required.
		if( pParams->GetFloat( CRCD(0x9e497fc6,"percent"), &amount ))
		{
			// Want to adjust the intensity. Convert from percentage to [0.0, 1.0].
			amount = amount * 0.01f;

			// Adjust the light intensity for this amount.
			p_light->SetIntensity( amount );
		}

		if( pParams->GetFloat( CRCD(0x999151b,"outer_radius"), &radius ))
		{
			p_light->SetRadius( radius );

			// Changing the radius requires that the sector list be rebuilt.
			p_light->CalculateSectorList();
		}

		if( pParams->GetInteger( CRCD(0x59ea070,"red"), &red ))
		{
			// We must have a green and blue value too.
			pParams->GetInteger( CRCD(0x2f6511de,"green"), &grn, true );
			pParams->GetInteger( CRCD(0x61c9354b,"blue"), &blu, true );

			Image::RGBA col( red, grn, blu, 0xFF );

			// Adjust the light color.
			p_light->SetColor( col );
		}

		s_apply_lighting( p_scene, id );
	}
	else
	{
		// Perhaps there is just a light for objects?
		Nx::CSceneLight *p_obj_light = Nx::CLightManager::sGetSceneLight( id );
		if( p_obj_light )
		{
			// Determine the intensity required.
			if( pParams->GetFloat( CRCD(0x9e497fc6,"percent"), &amount ))
			{
				// Want to adjust the intensity. Convert from percentage to [0.0, 1.0].
				amount = amount * 0.01f;

				// Adjust the light intensity for this amount.
				p_obj_light->SetLightIntensity( amount );
			}

			if( pParams->GetFloat( CRCD(0x999151b,"outer_radius"), &radius ))
			{
				p_obj_light->SetLightRadius( radius );
			}

			if( pParams->GetInteger( CRCD(0x59ea070,"red"), &red ))
			{
				// We must have a green and blue value too.
				pParams->GetInteger( CRCD(0x2f6511de,"green"), &grn, true );
				pParams->GetInteger( CRCD(0x61c9354b,"blue"), &blu, true );

				Image::RGBA col( red, grn, blu, 0xFF );

				// Adjust the light color.
				p_obj_light->SetLightColor( col );
			}
		}
	}
}

}


