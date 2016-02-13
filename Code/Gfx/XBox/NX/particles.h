
#ifndef	__PARTICLE_H__
#define	__PARTICLE_H__

#include "gfx\xbox\nx\scene.h"

namespace NxXbox
{

enum eParticleType
{
	PARTICLE_TYPE_LINE,
	PARTICLE_TYPE_FLAT,
	PARTICLE_TYPE_SHADED,
	PARTICLE_TYPE_SMOOTH,
	PARTICLE_TYPE_GLOW,
	PARTICLE_TYPE_STAR,
	PARTICLE_TYPE_SMOOTHSTAR,
	PARTICLE_TYPE_RIBBON,
	PARTICLE_TYPE_SMOOTHRIBBON,
	PARTICLE_TYPE_RIBBONTRAIL,
	PARTICLE_TYPE_GLOWRIBBONTRAIL,
};


struct sParticleSystem
{
public:
				sParticleSystem( uint32 max_particles, eParticleType type, uint32 texture_checksum, uint32 blendmode_checksum, int fix, int num_segments = 0, int history = 0 );
				~sParticleSystem( void );

	sMaterial	*mp_material;
};

}

#endif

