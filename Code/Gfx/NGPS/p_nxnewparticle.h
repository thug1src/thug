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
**	Description:	PS2 implementation of new parametric particle system	**
**																			**
*****************************************************************************/

#ifndef __GFX_NGPS_P_NXNEWPARTICLE_H__
#define __GFX_NGPS_P_NXNEWPARTICLE_H__

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <gfx/nxnewparticle.h>
#include <gfx/ngps/nx/sprite.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/



namespace Nx
{

                        
/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/


class CParticleStream
{
public:
	int						m_num_particles;
	float					m_rate;
	float					m_interval;
	float					m_oldest_age;
	uint32					m_seed[4];
	void					AdvanceSeed(int num_places);
};

class CSystemDmaData
{
public:
	uint32		m_GScontext[24];
	uint32		m_u0,  m_v0,  m_u1,  m_v1;
	Mth::Vector	m_p0;
	Mth::Vector	m_p1;
	Mth::Vector	m_p2;
	Mth::Vector	m_s0;
	Mth::Vector	m_s1;
	Mth::Vector	m_s2;
	Mth::Vector	m_c0;
	Mth::Vector	m_c1;
	Mth::Vector	m_c2;
	uint32		m_tagx,m_tagy,m_tagz,m_tagw;
} nAlign(128);


class CPs2NewParticle : public CNewParticle
{
private:
	void				update_position();

	bool				m_emitting;
	int					m_max_streams;
	int					m_num_streams;
	CParticleStream*	mp_stream;
	CParticleStream*	mp_newest_stream;
	CParticleStream*	mp_oldest_stream;
	Mth::Matrix 		m_rotation;
	CSystemDmaData		m_systemDmaData;
	NxPs2::SSingleTexture*	mp_engine_texture;


protected:
	void	plat_render( void );
	void	plat_update( void );
	void	plat_build( void );
	void	plat_destroy( void );
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

#endif	// __GFX_NGPS_P_NXNEWPARTICLE_H__


