/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 2002 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Sk/Engine												**
**																			**
**	File name:		RectFeeler.h											**
**																			**
**	Created: 		02/14/2003	-	Dan										**
**																			**
*****************************************************************************/

#ifndef	__SK_ENGINE_RECTFEELER_H
#define	__SK_ENGINE_RECTFEELER_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/math.h>

#include <gel/collision/collision.h>

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class	CRectFeeler	: public Mth::Rectangle
{
public:
	CRectFeeler (   );
	CRectFeeler ( const Mth::Vector& corner, const Mth::Vector& first_edge, const Mth::Vector& second_edge );
	
	void							SetRectangle ( const Mth::Rectangle& rect );
	
	bool							GetCollision ( bool movables = false );
	
	int								GetNumCollisionSurfaces (   ) const { return m_coll_data.num_surfaces; }
	const Nx::S2DCollSurface&		GetCollisionSurface ( int n ) const;
	
	int								GetNumMergedCollisionSurfaces (   ) const { return m_merged_coll_data.num_surfaces; }
	const Nx::S2DCollSurface&		GetMergedCollisionSurface ( int n ) const;
	
	void							MergeCollisionSurfaces (   );
	
	void							SetIgnore ( uint16 ignore_1, uint16 ignore_0 );
	
	void							SetCache ( Nx::CCollCache* p_cache ) { mp_cache = p_cache; }
	void							ClearCache (   ) { mp_cache = NULL; }
	
	static void						sSetDefaultCache ( Nx::CCollCache* p_cache ) { sp_default_cache = p_cache; }
	static void						sClearDefaultCache (   ) { sp_default_cache = NULL; }
	
	// ReduceCollisionSurfaces (   )
	
	void							DebugLines ( int r = 255, int g = 255, int b = 255, int num_frames = 0 ) const;

private:
	void							init (   );
	
	bool							very_close ( const Mth::Vector p, const Mth::Vector q ) const;

private:
	
	Nx::S2DCollData					m_coll_data;
	Nx::S2DCollData					m_merged_coll_data;
	
	Nx::CCollCache*					mp_cache;
	
	static Nx::CCollCache*			sp_default_cache;
};

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

inline void							CRectFeeler::SetRectangle ( const Mth::Rectangle& rect )
{
	m_corner = rect.m_corner;
	m_first_edge = rect.m_first_edge;
	m_second_edge = rect.m_second_edge;
}

inline const Nx::S2DCollSurface&	CRectFeeler::GetCollisionSurface ( int n ) const
{
	Dbg_Assert(n < m_coll_data.num_surfaces);
	return m_coll_data.p_surfaces[n];
}

inline const Nx::S2DCollSurface&	CRectFeeler::GetMergedCollisionSurface ( int n ) const
{
	Dbg_Assert(n < m_merged_coll_data.num_surfaces);
	Dbg_Assert(!isnanf(m_merged_coll_data.p_surfaces[n].ends[0].point[W]));
	Dbg_Assert(!isnanf(m_merged_coll_data.p_surfaces[n].ends[1].point[W]));
	return m_merged_coll_data.p_surfaces[n];
}

inline void							CRectFeeler::SetIgnore ( uint16 ignore_1, uint16 ignore_0 )
{
	m_coll_data.ignore_1 = ignore_1;
	m_coll_data.ignore_0 = ignore_0;
}

inline bool							CRectFeeler::very_close ( const Mth::Vector p, const Mth::Vector q ) const
{
	return Mth::Abs(p[X] - q[X]) < 0.1f && Mth::Abs(p[Y] - q[Y]) < 0.1f && Mth::Abs(p[Z] - q[Z]) < 0.1f;
}

#endif
