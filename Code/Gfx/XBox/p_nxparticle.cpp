//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxParticle.cpp
//* OWNER:          Dave Cowling
//* CREATION DATE:  3/27/2002
//****************************************************************************

#include <core/defines.h>
#include "gfx/xbox/nx/nx_init.h"
#include "gfx/xbox/p_nxparticle.h"
#include "gfx/xbox/p_nxparticleline.h"
#include "gfx/xbox/p_nxparticleflat.h"
#include "gfx/xbox/p_nxparticleshaded.h"
#include "gfx/xbox/p_nxparticlesmooth.h"
#include "gfx/xbox/p_nxparticleglow.h"
#include "gfx/xbox/p_nxparticlestar.h"
#include "gfx/xbox/p_nxparticlesmoothstar.h"
#include "gfx/xbox/p_nxparticleribbon.h"
#include "gfx/xbox/p_nxparticlesmoothribbon.h"
#include "gfx/xbox/p_nxparticleribbontrail.h"
#include "gfx/xbox/p_nxparticleglowribbontrail.h"

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CParticle *plat_create_particle( uint32 checksum, uint32 type_checksum, int max_particles, int max_streams, uint32 texture_checksum, uint32 blendmode_checksum, int fix, int num_segments, float split, int history )
{
	switch( type_checksum )
	{
		case 0x2eeb4b09:	// Line
		{
			CXboxParticleLine *p_particle = new CXboxParticleLine( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split );
			return static_cast<CParticle*>( p_particle );
		}

		case 0xaab555bb:	// Flat
		{
			CXboxParticleFlat *p_particle = new CXboxParticleFlat( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split );
			return static_cast<CParticle*>( p_particle );
		}

		case 0xf4d8d486:	// Shaded
		{
			CXboxParticleShaded *p_particle = new CXboxParticleShaded( checksum, max_particles, texture_checksum, blendmode_checksum, fix );
			return static_cast<CParticle*>( p_particle );
		}
		
		case 0x8addac1f:	// Smooth
		{
			CXboxParticleSmooth *p_particle = new CXboxParticleSmooth( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split );
			return static_cast<CParticle*>( p_particle );
		}
		
		case 0x15834eea:	// Glow
		{
			CXboxParticleGlow *p_particle = new CXboxParticleGlow( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split );
			return static_cast<CParticle*>( p_particle );
		}

		case 0x3624a5eb:	// Star
		{
			CXboxParticleStar *p_particle = new CXboxParticleStar( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split );
			return static_cast<CParticle*>( p_particle );
		}
		
		case 0x97cb7a9:		// SmoothStar
		{
			CXboxParticleSmoothStar *p_particle = new CXboxParticleSmoothStar( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split );
			return static_cast<CParticle*>( p_particle );
		}

		case 0xee6fc5b:		// Ribbon
		{
			CXboxParticleRibbon *p_particle = new CXboxParticleRibbon( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
			return static_cast<CParticle*>( p_particle );
		}

		case 0x3f109fcc:	// SmoothRibbon
		{
			CXboxParticleSmoothRibbon *p_particle = new CXboxParticleSmoothRibbon( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
			return static_cast<CParticle*>( p_particle );
		}

		case 0xc4d5a4cb:	// RibbonTrail
		{
			CXboxParticleRibbonTrail *p_particle = new CXboxParticleRibbonTrail( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
			return static_cast<CParticle*>( p_particle );
		}

		case 0x7ec7252d:	// GlowRibbonTrail
		{
			CXboxParticleGlowRibbonTrail *p_particle = new CXboxParticleGlowRibbonTrail( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
			return static_cast<CParticle*>( p_particle );
		}

		case 0xdedfc057:	// NewFlat
		{
			// Just default to old flat for now.
			CXboxParticleFlat *p_particle = new CXboxParticleFlat( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split );
			return static_cast<CParticle*>( p_particle );
		}

		default:
		{
			Dbg_MsgAssert( 0, ( "Unsupported particle type" ));
			break;
		}
	}
	return NULL;
}

} // Nx

				


