#include <gel/object/compositeobjectmanager.h>
#include <gel/components/skeletoncomponent.h>


#include "verlet.h"
#include "render.h"


const int	NUM_PARTICLES	= 16;
const float	PARTICLES_DIST	= 5.0f;


sVerletSystem*				pSystem				= NULL;
Obj::CSkeletonComponent*	pSkeletonComponent	= NULL;
float						ground				= -110.0f;


void CreateSystem( void )
{
	Obj::CCompositeObject *p_obj = (Obj::CCompositeObject *)Obj::CCompositeObjectManager::Instance()->GetObjectByID( 0 );
	if( p_obj )
	{
		pSkeletonComponent = GetSkeletonComponentFromObject( p_obj );
	}

	if( pSkeletonComponent )
	{
		pSystem = new sVerletSystem( NUM_PARTICLES );

		pSystem->m_timestep	= ( 1.0f / 60.0f );
		pSystem->m_gravity	= Mth::Vector( 0.0f, -1200.0f, 0.0f );

		// Get the skater's head position.
		Mth::Vector bone_pos;
		pSkeletonComponent->GetBoneWorldPosition( CRCD(0xe638eebc,"Bone_Forefinger_Tip_R"), &bone_pos );

		for( int i = 0; i < NUM_PARTICLES; ++i )
		{
			pSystem->mp_particles[i].m_pos		= bone_pos - ( Mth::Vector( 0.0f, PARTICLES_DIST * i, 0.0f ));
			pSystem->mp_particles[i].m_old_pos	= pSystem->mp_particles[i].m_pos;
		}

		for( int i = 0; i < ( NUM_PARTICLES - 1 ); ++i )
		{
			sVerletConstraint*	p_c = pSystem->AddConstraint();
			p_c->m_particle0		= i;
			p_c->m_particle1		= i + 1;
			p_c->m_constraint_dist	= PARTICLES_DIST;
		}
	}
}








void UpdateVerletSystem( void )
{
	if( pSystem == NULL )
	{
		CreateSystem();
	}

	if( pSystem )
	{
		// Set particle 0 to always be at the skater's head position.
		Mth::Vector bone_pos;
		pSkeletonComponent->GetBoneWorldPosition( CRCD(0xe638eebc,"Bone_Forefinger_Tip_R"), &bone_pos );
		pSystem->mp_particles[0].m_pos = bone_pos;

		pSystem->TimeStep();
	}
}

void DrawVerletSystem( void )
{
	struct sVerletDrawVert
	{
		float		x, y, z;
		D3DCOLOR	col;
	};

	if( pSystem )
	{
		sVerletDrawVert	dv[NUM_PARTICLES];
		for( int i = 0; i < NUM_PARTICLES; ++i )
		{
			dv[i].x		= pSystem->mp_particles[i].m_pos[X];
			dv[i].y		= pSystem->mp_particles[i].m_pos[Y];
			dv[i].z		= pSystem->mp_particles[i].m_pos[Z];
			dv[i].col	= 0xFFFFFFFF;
		}

		NxXbox::set_vertex_shader( D3DFVF_XYZ | D3DFVF_DIFFUSE );
		NxXbox::set_pixel_shader( PixelShader5 );

		NxXbox::EngineGlobals.p_Device->DrawVerticesUP( D3DPT_LINESTRIP, NUM_PARTICLES, dv, sizeof( sVerletDrawVert ));
	}
}




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sVerletParticle::sVerletParticle( void )
{
	m_pos		= Mth::Vector( 0.0f, 0.0f, 0.0f );
	m_old_pos	= m_pos;
	m_acc		= m_pos;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sVerletParticle::~sVerletParticle( void )
{
}




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sVerletConstraint::Apply( sVerletParticle *p_particles )
{
	sVerletParticle* p_p0			= p_particles + m_particle0;
	sVerletParticle* p_p1			= p_particles + m_particle1;

	Mth::Vector		delta			= p_p1->m_pos - p_p0->m_pos;
	float			delta_length	= delta.Length();
	float			diff			= ( delta_length - m_constraint_dist ) / delta_length;

	if( m_particle0 == 0 )
	{
		// Move the points.
		p_p0->m_pos += delta * 0.0f * diff;
		p_p1->m_pos -= delta * 1.0f * diff;
	}
	else
	{
		// Move the points.
		p_p0->m_pos += delta * 0.5f * diff;
		p_p1->m_pos -= delta * 0.5f * diff;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sVerletSystem::sVerletSystem( int num_particles )
{
	m_num_particles = num_particles;
	mp_particles	= new sVerletParticle[num_particles];
	mp_constraints	= new Lst::Head <sVerletConstraint>;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sVerletSystem::~sVerletSystem( void )
{
	delete [] mp_particles;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
sVerletConstraint* sVerletSystem::AddConstraint( void )
{
	sVerletConstraint*				p_constraint	= new sVerletConstraint;
	Lst::Node<sVerletConstraint>*	p_node			= new Lst::Node<sVerletConstraint>( p_constraint );

	mp_constraints->AddToTail( p_node );

	return p_constraint;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sVerletSystem::Verlet( void )
{
	for( int i = 0; i < m_num_particles; ++i )
	{
		Mth::Vector &x		= mp_particles[i].m_pos;
		Mth::Vector t		= x;
		Mth::Vector &o		= mp_particles[i].m_old_pos;
		Mth::Vector a		= mp_particles[i].m_acc;
		x					= ( x * 2.0f ) - o + ( a * m_timestep * m_timestep );
		o					= t;
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sVerletSystem::AccumulateForces( void )
{
	// All particles are influenced by gravity.
	for( int i = 0; i < m_num_particles; ++i )
	{
		mp_particles[i].m_acc = m_gravity;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sVerletSystem::SatisfyConstraints( void )
{
	Lst::Node<sVerletConstraint>* p_node, *p_next;

	for( int i = 0; i < 2; ++i )
	{
		// Satisfy distance constraints.
		for( p_node = mp_constraints->GetNext(); p_node; p_node = p_next )
		{
			p_next = p_node->GetNext();
			sVerletConstraint *p_constraint = p_node->GetData();
			p_constraint->Apply( mp_particles );
		}

		// Satisfy ground constraints.
		for( int i = 0; i < m_num_particles; ++i )
		{
			if( mp_particles[i].m_pos[Y] < ground )
			{
				mp_particles[i].m_pos[Y] = ground;
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sVerletSystem::TimeStep( void )
{
	AccumulateForces();
	Verlet();
	SatisfyConstraints();
}
