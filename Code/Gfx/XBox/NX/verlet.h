#ifndef __VERLET_H
#define __VERLET_H

#include <core/math.h>

void			UpdateVerletSystem( void );
void			DrawVerletSystem( void );



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sVerletParticle
{
						sVerletParticle( void );
						~sVerletParticle( void );

	Mth::Vector			m_pos;
	Mth::Vector			m_old_pos;
	Mth::Vector			m_acc;
};



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sVerletConstraint
{
	int					m_particle0;
	int					m_particle1;
	float				m_elasticity;
	float				m_constraint_dist;
	void				Apply( sVerletParticle *p_particles );
};



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sVerletSystem
{
									sVerletSystem( int num_particles );
									~sVerletSystem();

	void							TimeStep( void );
	void							Verlet( void );
	void							SatisfyConstraints( void );
	void							AccumulateForces( void );

	sVerletConstraint*				AddConstraint( void );	

	int								m_num_particles;
	sVerletParticle*				mp_particles;
	Mth::Vector						m_gravity;
	float							m_timestep;

	Lst::Head <sVerletConstraint>*	mp_constraints;
};





#endif // __NX_INIT_H
