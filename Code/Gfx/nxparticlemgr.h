//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxParticleMgr.h
//* OWNER:          Paul Robinson
//* CREATION DATE:  3/27/2002
//****************************************************************************

#ifndef	__GFX_P_NX_PARTICLE_MGR_H__
#define	__GFX_P_NX_PARTICLE_MGR_H__
    
#include "gfx/nxparticle.h"
                   
namespace Nx
{

void process_particles( float delta_time );
void render_particles( void );
CParticle* get_particle( uint32 checksum );
CParticle* create_particle( uint32 checksum, uint32 type_checksum, int max_particles, int max_streams, uint32 texture_checksum, uint32 blendmode_checksum, int fix, int num_segments, float split, int history, int perm );
void	   destroy_particle( uint32 checksum );
void	   destroy_particle_when_empty( uint32 checksum );
void destroy_all_particles(  );
void destroy_all_temp_particles(  );

void plat_process_particles( float delta_time );
CParticle* plat_get_particle( uint32 checksum );
CParticle* plat_create_particle( uint32 checksum, uint32 type_checksum, int max_particles, int max_streams, uint32 texture_checksum, uint32 blendmode_checksum, int fix, int num_segments, float split, int history );
} // Nx

#endif 




