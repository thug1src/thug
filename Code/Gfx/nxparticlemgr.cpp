//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       p_nxParticleMgr.cpp
//* OWNER:          Paul Robinson
//* CREATION DATE:  3/27/2002
//****************************************************************************

#include <core/defines.h>
#include "gfx/nx.h"
#include "gfx/nxparticlemgr.h"

namespace Nx
{
void process_particles( float delta_time )
{
//	plat_process_particles( delta_time );
	if( CEngine::sGetParticleTable() )
	{
		CParticle *p_particle;
		CEngine::sGetParticleTable()->IterateStart();
		while(( p_particle = CEngine::sGetParticleTable()->IterateNext()))
		{
			if ( p_particle->IsActive() ) p_particle->process( delta_time );
		}
	}
}

void render_particles( void )
{
//	plat_process_particles( delta_time );
	if( CEngine::sGetParticleTable() )
	{
		CParticle *p_particle;
		CEngine::sGetParticleTable()->IterateStart();
		while(( p_particle = CEngine::sGetParticleTable()->IterateNext()))
		{
			if ( p_particle->IsActive() ) p_particle->render();
		}
	}
}

CParticle* get_particle( uint32 checksum )
{
	CParticle *p_particle = NULL;
	if( CEngine::sGetParticleTable() )
	{
		p_particle = CEngine::sGetParticleTable()->GetItem( checksum );
	}
	return p_particle;
}

CParticle* create_particle( uint32 checksum, uint32 type_checksum, int max_particles, int max_streams, uint32 texture_checksum, uint32 blendmode_checksum, int fix, int num_segments, float split, int history, int perm )
{
	// Types to add:
	// 1. Flat color quad
	// 2. Gouraud quad
	// 3. n sided glow (center color + outer color)
	// 4. n sided 2 layer glow ( center color + mid color + outer color)
	// 5. n spiked star (center color + mid color + spike color)
	// 6. Lines (current color + previous color)
	// 7. Ribbons - volumetric lines made from quads (current color + previous color)

	CParticle *p_particle =  plat_create_particle( checksum, type_checksum, max_particles, max_streams, texture_checksum, blendmode_checksum, fix, num_segments, split, history );
	
	p_particle->SetPerm(perm);
	
	p_particle->SetActive( true );
	p_particle->SetEmitting( false ); 	// This is already set to false, this si just a reminder

	// Add to our list of particles.
	CEngine::sGetParticleTable()->PutItem( checksum, p_particle );
	return p_particle;
}

void destroy_particle( uint32 checksum )
{
	CParticle * p = get_particle( checksum );	 	// get item
	CEngine::sGetParticleTable()->FlushItem( checksum );		// remove reference from hash table
	delete p;										// deleted particle, will remove itself from the table
}

void destroy_particle_when_empty( uint32 checksum )
{
	CParticle * p = get_particle( checksum );	 	// get item
	p->delete_when_empty();
}

void destroy_all_particles(  )
{
	if( CEngine::sGetParticleTable() )
	{
		CParticle *p_particle;
		CEngine::sGetParticleTable()->IterateStart();
		while(( p_particle = CEngine::sGetParticleTable()->IterateNext()))
		{
			delete p_particle;
		}
	}
}

void destroy_all_temp_particles(  )
{
	if( CEngine::sGetParticleTable() )
	{
		CParticle *p_particle;
		CEngine::sGetParticleTable()->IterateStart();
		while(( p_particle = CEngine::sGetParticleTable()->IterateNext()))
		{
			if (!p_particle->IsPerm())
			{
				delete p_particle;
			}
		}
	}
}



} // Nx
