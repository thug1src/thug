///////////////////////////////////////////////////////////////////////////////////
// p_NxLightMan.H - Neversoft Engine, Rendering portion, Platform dependent interface

#ifndef	__GFX_P_NX_LIGHT_MAN_H__
#define	__GFX_P_NX_LIGHT_MAN_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <core/hashtable.h>

#include <gfx/NxLightMan.h>


namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

///////////////////////////////////////////////////////////////////////////////////
// Nx::CPs2LightManager
class	CPs2LightManager : public CLightManager
{
public:

	// Does any once-per-frame light update
	static void					sUpdateEngine( void );

	// Model Lights List functions
	static bool					sAddToModelLightsList(CModelLights *p_model_lights);
	static bool					sRemoveFromModelLightsList(CModelLights *p_model_lights);

private:	

	// once-per-frame update functions called from sUpdateEngine()
	static void					s_update_model_lights();

	// Model light list
	static Lst::Head< CModelLights > s_model_lights_list;
};

}


#endif

