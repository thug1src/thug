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
**	File name:		NxNewParticle.h											**
**																			**
**	Created by:		3/24/03	-	SPG											**
**																			**
**	Description:	New parametric particle system  	 					**
**																			**
*****************************************************************************/

#ifndef __GFX_NXNEWPARTICLE_H__
#define __GFX_NXNEWPARTICLE_H__

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/math.h>
#include <core/math/geometry.h>

#include <gfx/image/imagebasic.h>


/*****************************************************************************
**								   Defines									**
*****************************************************************************/


namespace Nx
{

						
enum
{
	vBOX_START,
	vBOX_MID,
	vBOX_END,
	vNUM_BOXES
};

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class CParticleParams
{
public:
	CParticleParams( void );

	// Particle Params
	uint32			m_Name;
	uint32			m_Type;	// NEWFLAT is the only one for now
	bool			m_UseMidpoint;
	float			m_Radius[vNUM_BOXES];
	float			m_RadiusSpread[vNUM_BOXES];
	int				m_MaxStreams;
	float			m_EmitRate;
	float			m_Lifetime;		// in seconds
	float			m_MidpointPct;	// percent of lifetime at which midpoint occurs

	// Spatial params
	Mth::Matrix		m_RotMatrix;
	Mth::Vector		m_BoxPos[vNUM_BOXES];
	Mth::Vector		m_BoxDims[vNUM_BOXES];

	// Local data (CParticleComponent is the only class that supports this)
	bool			m_LocalCoord;
	Mth::Vector		m_LocalBoxPos[vNUM_BOXES];

	// Color params
	bool			m_UseMidcolor;
	float			m_ColorMidpointPct;	// percent of lifetime at which midcolor occurs
	Image::RGBA		m_Color[vNUM_BOXES];

	// Material params
	uint32			m_BlendMode;
	int				m_FixedAlpha;
	int				m_AlphaCutoff;
	uint32			m_Texture;

	// LOD params
	int				m_LODDistance1;
	int				m_LODDistance2;
	int				m_SuspendDistance;

	// Hidden flag
	bool			m_Hidden;
	bool			m_Suspended;

};

class CNewParticle
{
public:
	CNewParticle( void );
	virtual	~CNewParticle( void );

	void	Initialize( CParticleParams* params );
	CParticleParams*	GetParameters( void );

	void	SetEmitRate( float emit_rate );
	void	SetLifetime( float lifetime );
	void	SetMidpointPct( float midpoint_pct );
	void	SetMidpointColorPct( float midpoint_pct );

	void	SetRadius( int which, float radius );
	void	SetBoxPos( int which, Mth::Vector* pos );
	void	SetBoxDims( int which, Mth::Vector* dims );
	void	SetColor( int which, Image::RGBA* color );
	
	void	CalculateBoundingVolumes( void );

	void	Render( void );
	void	Update( void );
	void	Destroy( void );
	void	Hide(bool should_hide);
	void	Suspend( bool	should_suspend );
	
	uint32	GetName() {return m_params.m_Name;}
	
protected:
	
	CParticleParams	m_params;

	Mth::CBBox		m_bbox;
	Mth::Vector		m_bsphere;

	virtual void	plat_build( void );
	virtual void	plat_destroy( void );
	virtual void	plat_hide( bool should_hide );
	virtual	void	plat_render( void );
	virtual	void	plat_update( void );

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

#endif	// __GFX_NXNEWPARTICLE_H__


