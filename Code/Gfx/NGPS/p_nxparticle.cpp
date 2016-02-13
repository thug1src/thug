//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxParticle.cpp
//* OWNER:          Paul Robinson
//* CREATION DATE:  3/27/2002
//****************************************************************************

#include <core/defines.h>
#include "gfx/ngps/nx/render.h"
#include "gfx/ngps/nx/dma.h"
#include "gfx/ngps/nx/vif.h"
#include "gfx/ngps/nx/vu1.h"
#include "gfx/ngps/nx/gif.h"
#include "gfx/ngps/nx/gs.h"

#include "gfx/ngps/nx/line.h"
#include <gfx/NxTexMan.h>
#include <gfx/ngps/p_nxtexture.h>


#include "gfx/ngps/nx/vu1code.h"


#include "gfx/ngps/nx/mesh.h"
#include "gfx/ngps/p_nxparticleflat.h"
#include "gfx/ngps/p_nxparticleshaded.h"
#include "gfx/ngps/p_nxparticlesmooth.h"
#include "gfx/ngps/p_nxparticleglow.h"
#include "gfx/ngps/p_nxparticlestar.h"
#include "gfx/ngps/p_nxparticlesmoothstar.h"
#include "gfx/ngps/p_nxparticleline.h"
#include "gfx/ngps/p_nxparticleribbon.h"
#include "gfx/ngps/p_nxparticleribbontrail.h"
#include "gfx/ngps/p_nxparticlesmoothribbon.h"
#include "gfx/ngps/p_nxparticleglowribbontrail.h"
#include "gfx/ngps/p_nxparticlenewflat.h"


namespace Nx
{


CParticle* plat_create_particle( uint32 checksum, uint32 type_checksum, int max_particles, int max_streams, uint32 texture_checksum, uint32 blendmode_checksum, int fix, int num_segments, float split, int history )
{
	// Types to add:
	// 1. x Flat color quad
	// 2. x Gouraud quad
	// 3. x Quad with center point & outer color
	// 4. x n sided glow (center color + outer color)
	// 5. x n sided 2 layer glow ( center color + mid color + outer color)
	// 6. x n spiked star (center color + mid color + spike color)
	// 7. Lines (current color + previous color)
	// 8. Ribbons - volumetric lines made from quads (current color + previous color)

	switch ( type_checksum )
	{
		case 0xf4d8d486:		// Shaded
			{
				CPs2ParticleShaded* p_particle = new CPs2ParticleShaded( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0x8addac1f:		// Smooth
			{
				CPs2ParticleSmooth* p_particle = new CPs2ParticleSmooth( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0x15834eea:		// Glow
			{
				CPs2ParticleGlow* p_particle = new CPs2ParticleGlow( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0x3624a5eb:		// Star
			{
				CPs2ParticleStar* p_particle = new CPs2ParticleStar( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0x97cb7a9:		// SmoothStar
			{
				CPs2ParticleSmoothStar* p_particle = new CPs2ParticleSmoothStar( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0x2eeb4b09:		// Line
			{
				CPs2ParticleLine* p_particle = new CPs2ParticleLine( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0xee6fc5b:		// Ribbon
			{
				CPs2ParticleRibbon* p_particle = new CPs2ParticleRibbon( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0xc4d5a4cb:		// RibbonTrail
			{
				CPs2ParticleRibbonTrail* p_particle = new CPs2ParticleRibbonTrail( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0x3f109fcc:		// SmoothRibbon
			{
				CPs2ParticleSmoothRibbon* p_particle = new CPs2ParticleSmoothRibbon( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0x7ec7252d:		// GlowRibbonTrail
			{
				CPs2ParticleGlowRibbonTrail* p_particle = new CPs2ParticleGlowRibbonTrail( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0xdedfc057:		// NewFlat
			{
				CPs2ParticleNewFlat* p_particle = new CPs2ParticleNewFlat( checksum, max_streams, texture_checksum, blendmode_checksum, fix );
				return static_cast<CParticle*>( p_particle );
			}
		case 0xaab555bb:		// Flat
		default:
			{
				CPs2ParticleFlat* p_particle = new CPs2ParticleFlat( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
	}
}

} // Nx

				




