//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxParticle.h
//* OWNER:          Paul Robinson
//* CREATION DATE:  3/27/2002
//****************************************************************************

#ifndef	__GFX_P_NX_PARTICLE_H__
#define	__GFX_P_NX_PARTICLE_H__
    
#include "gfx/nxparticle.h"
#include "gfx/ngc/nx/render.h"
                   
namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
NxNgc::BlendModes get_texture_blend( uint32 blend_checksum );
	
} // Nx

#endif 





