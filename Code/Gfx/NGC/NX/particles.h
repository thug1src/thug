#ifndef	__PARTICLE_H__
#define	__PARTICLE_H__

#include "gfx\Ngc\nx\scene.h"
#include "gfx\Ngc\nx\instance.h"
#include "gfx/ngc/nx/mesh.h"

namespace NxNgc
{


struct sParticleSystem
{
public:
				sParticleSystem( uint32 max_particles, uint32 texture_checksum, uint32 blendmode_checksum, int fix );
				~sParticleSystem( void );

	void		Render();
#ifdef SHORT_VERT
	s16			*GetVertexWriteBuffer( void );
#else
	float		*GetVertexWriteBuffer( void );
#endif		// SHORT_VERT
	GXColor		*GetColorWriteBuffer( void );
	sMaterial*	GetMaterial( void )			{ return mp_material; }
	NxNgc::sTexture * GetTexture( void )	{ return mp_engine_texture; }

	sScene*		mp_scene;

	float* 		mp_sphere;

	CInstance*	mp_instance;

	sMaterial*	mp_material;

	sMesh*		mp_mesh;

	NxNgc::sTexture * mp_engine_texture;
};

}

#endif


