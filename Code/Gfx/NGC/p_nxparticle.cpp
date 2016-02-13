//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxParticle.cpp
//* OWNER:          Paul Robinson
//* CREATION DATE:  3/27/2002
//****************************************************************************

#include <core/defines.h>
#include <gfx/NxTexMan.h>
#include <gfx/Ngc/p_nxtexture.h>

#include "gfx/Ngc/nx/mesh.h"
#include "gfx/Ngc/p_nxparticleflat.h"
#include "gfx/Ngc/p_nxparticleshaded.h"
#include "gfx/Ngc/p_nxparticlesmooth.h"
#include "gfx/Ngc/p_nxparticleglow.h"
#include "gfx/Ngc/p_nxparticlestar.h"
#include "gfx/Ngc/p_nxparticlesmoothstar.h"
#include "gfx/Ngc/p_nxparticleline.h"
#include "gfx/Ngc/p_nxparticleribbon.h"
#include "gfx/Ngc/p_nxparticleribbontrail.h"
#include "gfx/Ngc/p_nxparticlesmoothribbon.h"
#include "gfx/Ngc/p_nxparticleglowribbontrail.h"


namespace Nx
{

NxNgc::BlendModes get_texture_blend( uint32 blend_checksum )
{
	NxNgc::BlendModes rv = NxNgc::vBLEND_MODE_DIFFUSE;
	switch ( blend_checksum )
	{
		case 0x54628ed7:		// Blend
			rv = NxNgc::vBLEND_MODE_BLEND;
			break;
		case 0x02e58c18:		// Add
			rv = NxNgc::vBLEND_MODE_ADD;
			break;
		case 0xa7fd7d23:		// Sub
		case 0xdea7e576:		// Subtract
			rv = NxNgc::vBLEND_MODE_SUBTRACT;
			break;
		case 0x40f44b8a:		// Modulate
			rv = NxNgc::vBLEND_MODE_MODULATE;
			break;
		case 0x68e77f40:		// Brighten
			rv = NxNgc::vBLEND_MODE_BRIGHTEN;
			break;
		case 0x18b98905:		// FixBlend
			rv = NxNgc::vBLEND_MODE_BLEND_FIXED;
			break;
		case 0xa86285a1:		// FixAdd
			rv = NxNgc::vBLEND_MODE_ADD_FIXED;
			break;
		case 0x0d7a749a:		// FixSub
		case 0x0eea99ff:		// FixSubtract
			rv = NxNgc::vBLEND_MODE_SUB_FIXED;
			break;
		case 0x90b93703:		// FixModulate
			rv = NxNgc::vBLEND_MODE_MODULATE_FIXED;
			break;
		case 0xb8aa03c9:		// FixBrighten
			rv = NxNgc::vBLEND_MODE_BRIGHTEN_FIXED;
			break;
		case 0x515e298e:		// Diffuse
		case 0x806fff30:		// None
			rv = NxNgc::vBLEND_MODE_DIFFUSE;
			break;
		default:
			Dbg_MsgAssert(0,("Illegal blend mode specified. Please use (fix)blend/add/sub/modulate/brighten or diffuse/none."));
			break;
	}
	return rv;
}

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
				CNgcParticleShaded* p_particle = new CNgcParticleShaded( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0x8addac1f:		// Smooth
			{
				CNgcParticleSmooth* p_particle = new CNgcParticleSmooth( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0x15834eea:		// Glow
			{
				CNgcParticleGlow* p_particle = new CNgcParticleGlow( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0x3624a5eb:		// Star
			{
				CNgcParticleStar* p_particle = new CNgcParticleStar( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0x97cb7a9:		// SmoothStar
			{
				CNgcParticleSmoothStar* p_particle = new CNgcParticleSmoothStar( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0x2eeb4b09:		// Line
			{
				CNgcParticleLine* p_particle = new CNgcParticleLine( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0xee6fc5b:		// Ribbon
			{
				CNgcParticleRibbon* p_particle = new CNgcParticleRibbon( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0xc4d5a4cb:		// RibbonTrail
			{
				CNgcParticleRibbonTrail* p_particle = new CNgcParticleRibbonTrail( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0x3f109fcc:		// SmoothRibbon
			{
				CNgcParticleSmoothRibbon* p_particle = new CNgcParticleSmoothRibbon( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0x7ec7252d:		// GlowRibbonTrail
			{
				CNgcParticleGlowRibbonTrail* p_particle = new CNgcParticleGlowRibbonTrail( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
		case 0xaab555bb:		// Flat
		default:
			{
				CNgcParticleFlat* p_particle = new CNgcParticleFlat( checksum, max_particles, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
				return static_cast<CParticle*>( p_particle );
			}
			break;
	}
}

} // Nx

				





