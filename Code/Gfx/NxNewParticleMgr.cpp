/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Skate5													**
**																			**
**	Module:			Gfx			 											**
**																			**
**	File name:		NxNewParticleMgr.cpp									**
**																			**
**	Created by:		3/24/03	-	SPG											**
**																			**
**	Description:	New parametric particle system manager					**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <gfx/NxNewParticleMgr.h>
#include <gel/scripting/checksum.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Nx
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

CNewParticle*	CNewParticleManager::plat_create_particle( void )
{
	Dbg_Assert( 0 );
	Dbg_Printf( "Stub plat_create_particle\n" );
	return NULL;
}

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

CNewParticleManager::CNewParticleManager( void )
{
	mp_particle_table = new Lst::HashTable< CNewParticle >( 8 );
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CNewParticleManager::~CNewParticleManager( void )
{
	delete mp_particle_table;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CNewParticle*	CNewParticleManager::CreateParticle( CParticleParams* params, bool generate_name )
{
	CNewParticle* particle;
	 
	particle = plat_create_particle();
	if( particle )
	{
		if( generate_name )
		{
			uint32 name;

			name = params->m_Name;
			// Ensure a unique name for this system
			while(( mp_particle_table->GetItem( name, false ) != NULL ))
			{
				name++;
			}

			params->m_Name = name;
		}

		particle->Initialize( params );
		mp_particle_table->PutItem( params->m_Name, particle );
	}

	return particle;
}

// Given an actual particle system
// remove it from the Particle
void	CNewParticleManager::KillParticle( CNewParticle* p_particle)
{
	Dbg_MsgAssert(p_particle,("NULL p_particle passed to KillParticle"));
	Dbg_MsgAssert(mp_particle_table->GetItem(p_particle->GetName()) == p_particle,("entry in particle table for %s does not match where it came from",Script::FindChecksumName(p_particle->GetName())));	
	mp_particle_table->FlushItem(p_particle->GetName());	
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CNewParticleManager::RenderParticles( void )
{
	mp_particle_table->IterateStart();
	CNewParticle *p_particle = mp_particle_table->IterateNext();
	while( p_particle )
	{
		p_particle->Render();
		p_particle = mp_particle_table->IterateNext();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void			CNewParticleManager::UpdateParticles( void )
{
	mp_particle_table->IterateStart();
	CNewParticle *p_particle = mp_particle_table->IterateNext();
	while( p_particle )
	{
		p_particle->Update();
		p_particle = mp_particle_table->IterateNext();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void 			CNewParticleManager::Cleanup( void )
{
	// Goes through and removes all particle systems.
	mp_particle_table->IterateStart();
	CNewParticle *p_particle = mp_particle_table->IterateNext();
	while( p_particle )
	{
		CNewParticle *p_next	= mp_particle_table->IterateNext();
		p_particle->Destroy();
		delete p_particle;
		p_particle		= p_next;
	}
	mp_particle_table->FlushAllItems();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Nx




