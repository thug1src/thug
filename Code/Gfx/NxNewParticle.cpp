/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		skate5													**
**																			**
**	Module:			Gfx			 											**
**																			**
**	File name:		NxNewParticle.cpp										**
**																			**
**	Created by:		3/24/03	-	SPG											**
**																			**
**	Description:	New parametric particle system							**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <gfx/NxNewParticle.h>

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

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static Mth::Vector extrema( Mth::Vector p0, Mth::Vector p1, Mth::Vector p2, float t1, float t2 )
{
	Mth::Vector e1, e2, u, a, p;
	float		q;

    e1	= t1 * ( p2 - p0 );
    e2	= t2 * ( p1 - p0 );
	q	= 1.0f / ( t1 * t2 * ( t2 - t1 )); 
	u	= ( t2 * e2 - t1 * e1 ) * q;			// Intitial velocity.
	a	= ( e1 - e2 ) * q;						// Twice initial acceleration.

	for( int i = 0; i < 3; i++ )
    {
		float t	= -0.5f * u[i] / a[i];				// Time of extremum for given coordinate.
		p[i]	= p0[i]  + u[i] * t + a[i] * t * t;	// Value of coordinate at time t.
    }
	p[3] = 0.0f;

	return p;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static Mth::Vector componentwise_min( Mth::Vector& p0, Mth::Vector& p1 )
{
	Mth::Vector r = p0;

	if( p1[X] < p0[X] )
		r[X] = p1[X];
	if( p1[Y] < p0[Y] )
		r[Y] = p1[Y];
	if( p1[Z] < p0[Z] )
		r[Z] = p1[Z];
	if( p1[W] < p0[W] )
		r[W] = p1[W];

	return r;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static Mth::Vector componentwise_max( Mth::Vector& p0, Mth::Vector& p1 )
{
	Mth::Vector r = p0;

	if( p1[X] > p0[X] )
		r[X] = p1[X];
	if( p1[Y] > p0[Y] )
		r[Y] = p1[Y];
	if( p1[Z] > p0[Z] )
		r[Z] = p1[Z];
	if( p1[W] > p0[W] )
		r[W] = p1[W];

	return r;
}

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

CParticleParams::CParticleParams( void )
{
	int i;

	m_Type = CRCD(0xdedfc057,"NEWFLAT");
	m_UseMidpoint = true;
	m_LocalCoord = false;
	for( i = 0; i < vNUM_BOXES; i++ )
	{
		m_Radius[i] = 1.0f;
		m_RadiusSpread[i] = 0.0f;

		m_BoxPos[i][X] = 0.0f;
		m_BoxPos[i][Y] = 0.0f;
		m_BoxPos[i][Z] = 0.0f;
		m_BoxPos[i][W] = 0.0f;

		m_BoxDims[i][X] = 20.0f;
		m_BoxDims[i][Y] = 20.0f;
		m_BoxDims[i][Z] = 20.0f;
		m_BoxDims[i][W] = 0.0f;

		m_LocalBoxPos[i][X] = 0.0f;
		m_LocalBoxPos[i][Y] = 0.0f;
		m_LocalBoxPos[i][Z] = 0.0f;
		m_LocalBoxPos[i][W] = 0.0f;

		m_Color[i].r = 128;
		m_Color[i].g = 128;
		m_Color[i].b = 128;
		m_Color[i].a = 128;
	}

	m_MaxStreams = 2;
	m_EmitRate = 300.0f;
	m_Lifetime = 4.0f;
	m_MidpointPct = 50.0f;
	
    m_UseMidcolor = false;
	m_ColorMidpointPct = 50.0f;
	m_BlendMode = CRCD(0xa86285a1,"FixAdd");
	m_AlphaCutoff = 1;
	m_Texture = 0;
	m_FixedAlpha = 0;

	m_LODDistance1 = 400;
	m_LODDistance2 = 401;
	m_SuspendDistance = 0;

	m_Hidden = false;
	m_Suspended = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CNewParticle::CNewParticle( void )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CNewParticle::~CNewParticle( void )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::CalculateBoundingVolumes( void )
{
	// Calculate the bounding box.
	Mth::Vector p0, p1, p2;				// box centres
	Mth::Vector s0, s1, s2;				// box dimensions
	Mth::Vector p0_lo, p1_lo, p2_lo;	// corresponding minimum possible coords
	Mth::Vector p0_hi, p1_hi, p2_hi;	// corresponding maximum possible coords
	Mth::Vector AABB_min, AABB_max;		// AABB corners
	Mth::Vector one( 1.0f, 1.0f, 1.0f, 1.0f );

	p0 = m_params.m_BoxPos[0];
	p1 = m_params.m_BoxPos[1];
	p2 = m_params.m_BoxPos[2];

	p0[W] = m_params.m_Radius[0];
	p1[W] = m_params.m_Radius[1];
	p2[W] = m_params.m_Radius[2];

	s0 = 0.5f * m_params.m_BoxDims[0];
	s1 = 0.5f * m_params.m_BoxDims[1];
	s2 = 0.5f * m_params.m_BoxDims[2];

	s0[W] = 0.5f * m_params.m_RadiusSpread[0];
	s1[W] = 0.5f * m_params.m_RadiusSpread[1];
	s2[W] = 0.5f * m_params.m_RadiusSpread[2];

	p0_lo = p0 - s0 - p0[W]*one - s0[W]*one;
	p1_lo = p1 - s1 - p1[W]*one - s1[W]*one; 
	p2_lo = p2 - s2 - p2[W]*one - s2[W]*one;

	p0_hi = p0 + s0 + p0[W]*one + s0[W]*one;
	p1_hi = p1 + s1 + p1[W]*one + s1[W]*one;
	p2_hi = p2 + s2 + p2[W]*one + s2[W]*one;

	AABB_min = componentwise_min( p0_lo, p2_lo );
	AABB_max = componentwise_max( p0_hi, p2_hi );

	if( m_params.m_UseMidpoint )
	{ 
		Mth::Vector p;

		p = extrema( p0_lo, p1_lo, p2_lo,
					m_params.m_Lifetime * m_params.m_MidpointPct * 0.01f,
					m_params.m_Lifetime );

		AABB_min = componentwise_min( AABB_min, p ); 
	 
		p = extrema( p0_hi, p1_hi, p2_hi,
					m_params.m_Lifetime * m_params.m_MidpointPct * 0.01f,
					m_params.m_Lifetime );

		AABB_max = componentwise_max( AABB_max, p ); 
	}

	// Set the bounding box.
	m_bbox.Set( AABB_min, AABB_max );

	// Now calculate the bounding sphere.
	Mth::Vector diag		= ( AABB_max - AABB_min ) * 0.5f;
	diag[W]					= 0.0f;
	m_bsphere				= AABB_min + diag;
	m_bsphere[W]			= diag.Length();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::Initialize( CParticleParams* params )
{
	m_params = *params;
	CalculateBoundingVolumes();
	plat_build();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CParticleParams*	CNewParticle::GetParameters( void )
{
	return &m_params;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::Render( void )
{
	if (!m_params.m_Hidden && !m_params.m_Suspended  )
	{
		plat_render();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::Update( void )
{
	if (!m_params.m_Hidden && !m_params.m_Suspended)
	{
		plat_update();
	}
	
	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::Destroy( void )
{
	plat_destroy();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::Hide( bool  should_hide )
{
	if (m_params.m_Hidden != should_hide)
	{
		m_params.m_Hidden = should_hide;
		plat_hide(should_hide);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/



void	CNewParticle::Suspend( bool	should_suspend )
{
	if (m_params.m_Suspended != should_suspend)
	{
		// if we were not hidden, then hide/unhide based on the value of the suspend flag
		if (!m_params.m_Hidden)
		{
			plat_hide(should_suspend);
		}	
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::SetEmitRate( float emit_rate )
{
	m_params.m_EmitRate = emit_rate;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::SetLifetime( float lifetime )
{
	m_params.m_Lifetime = lifetime;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::SetMidpointPct( float midpoint_pct )
{
	m_params.m_MidpointPct = midpoint_pct;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::SetRadius( int which, float radius )
{
	Dbg_Assert( which < vNUM_BOXES );

	m_params.m_Radius[which] = radius;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::SetBoxPos( int which, Mth::Vector* pos )
{
	Dbg_Assert( pos );
	Dbg_Assert( which < vNUM_BOXES );

	m_params.m_BoxPos[which] = *pos;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::SetBoxDims( int which, Mth::Vector* dims )
{
	Dbg_Assert( dims );
	Dbg_Assert( which < vNUM_BOXES );

	m_params.m_BoxDims[which] = *dims;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::SetColor( int which, Image::RGBA* color )
{
	Dbg_Assert( which < vNUM_BOXES );
	Dbg_Assert( color );

	m_params.m_Color[which] = *color;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::SetMidpointColorPct( float midpoint_pct )
{
	m_params.m_ColorMidpointPct = midpoint_pct;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::plat_render( void )
{
	Dbg_Printf( "STUB: plat_render\n" );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::plat_update( void )
{
	Dbg_Printf( "STUB: plat_update\n" );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::plat_build( void )
{
	Dbg_Printf( "STUB: plat_build\n" );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::plat_destroy( void )
{
	Dbg_Printf( "STUB: plat_destroy\n" );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CNewParticle::plat_hide( bool should_hide )
{
	Dbg_Printf( "STUB: plat_destroy\n" );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Nx




