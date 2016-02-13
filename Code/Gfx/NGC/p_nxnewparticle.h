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
**	File name:		p_NxNewParticle.h										**
**																			**
**	Created by:		3/24/03	-	SPG											**
**																			**
**	Description:	Ngc implementation of new parametric particle system	**
**																			**
*****************************************************************************/

#ifndef __GFX_Ngc_P_NXNEWPARTICLE_H__
#define __GFX_Ngc_P_NXNEWPARTICLE_H__

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <gfx/nxnewparticle.h>
#include <gfx/Ngc/nx/material.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/


namespace Nx
{

                        
/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
class CParticleStream
{
public:
	int						m_num_particles;
	float					m_rate;
	float					m_interval;
	float					m_oldest_age;
//	uint32					m_rand_seed;
//	uint32					m_rand_a;
//	uint32					m_rand_b;
	uint32					m_rand_current;
	void					AdvanceSeed( int num_places );
};



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
class CNgcNewParticle : public CNewParticle
{
	bool				m_emitting;
	int					m_max_streams;
	int					m_num_streams;
	CParticleStream*	mp_stream;
	CParticleStream*	mp_newest_stream;
	CParticleStream*	mp_oldest_stream;
	Mth::Matrix 		m_rotation;
	Mth::Matrix			m_new_matrix;
//	NxNgc::sMaterial*	mp_material;
	NxNgc::sTexture *	mp_texture;
	uint8				m_blend;
	uint8				m_fix;

	Mth::Vector			m_s0;
	Mth::Vector			m_s1;
	Mth::Vector			m_s2;
	Mth::Vector			m_p0;
	Mth::Vector			m_p1;
	Mth::Vector			m_p2;

protected:
	void	plat_build( void );
	void	plat_destroy( void );
	void	plat_render( void );
	void	plat_update( void );
	void	update_position( void );
};



/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Nx

#endif	// __GFX_Ngc_P_NXNEWPARTICLE_H__



