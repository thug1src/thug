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
**	File name:		RectFeeler.cpp											**
**																			**
**	Created: 		02/14/2003	-	Dan										**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/math.h>
		
#include <gfx/debuggfx.h>

#include <sk/engine/rectfeeler.h>

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

Nx::CCollCache* CRectFeeler::sp_default_cache = NULL;

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

// BUG: getting false collisions when rectangle is flush with axes

CRectFeeler::CRectFeeler (   )
{
	init();
}

CRectFeeler::CRectFeeler ( const Mth::Vector& corner, const Mth::Vector& first_edge, const Mth::Vector& second_edge )
{
	m_corner = corner;
	m_first_edge = first_edge;
	m_second_edge = second_edge;
	
	init();
}

void CRectFeeler::init (   )
{
	mp_cache = NULL;
	m_coll_data.num_surfaces = 0;
	m_merged_coll_data.num_surfaces = 0;
	
	m_coll_data.ignore_1 = mFD_NON_COLLIDABLE;
	m_coll_data.ignore_0 = 0;
}

bool CRectFeeler::GetCollision ( bool movables )
{
	Dbg_MsgAssert(!movables, ("Collision test against movables not yet supported by CRectFeeler"));
	
	Rectangle& rect = *this;
	
	return Nx::CCollObj::sFindRectangleStaticCollision(rect, &m_coll_data, mp_cache ? mp_cache : sp_default_cache);
}

void CRectFeeler::MergeCollisionSurfaces (   )
{
	// precomputed values
	bool p_surfaces_used[Nx::MAX_NUM_2D_COLLISIONS_REPORTED];
	Mth::Vector p_line_directions[Nx::MAX_NUM_2D_COLLISIONS_REPORTED];
	for (int n = m_coll_data.num_surfaces; n--; )
	{
		p_surfaces_used[n] = false;
		
		p_line_directions[n] = m_coll_data.p_surfaces[n].ends[1].point - m_coll_data.p_surfaces[n].ends[0].point;
		p_line_directions[n].Normalize();
	}
	
	// drop duplicate surfaces
	for (int i = m_coll_data.num_surfaces; i--; )
	{
		Nx::S2DCollSurface& surface_i = m_coll_data.p_surfaces[i];
		
		if (p_surfaces_used[i]) continue;
		
		for (int j = i; j--; )
		{
			Nx::S2DCollSurface& surface_j = m_coll_data.p_surfaces[j];
			if ((very_close(surface_i.ends[0].point, surface_j.ends[0].point) && very_close(surface_i.ends[1].point, surface_j.ends[1].point))
				|| (very_close(surface_i.ends[0].point, surface_j.ends[1].point) && very_close(surface_i.ends[1].point, surface_j.ends[0].point)))
			{
				if (Mth::DotProduct(surface_i.normal, surface_j.normal) > 0.99f)
				{
					p_surfaces_used[j] = true;
					continue;
				}
			}
		}
	}
	
	// we build a merged surface list out of the collision data surface list, attempting to reduce the number of surfaces by merging those which
	// are equivalent for the purpose of collision lines; that is, merge chains of collision lines which are parallel and have equal surface normals
	
	// for each surface, first attempt to absorb as yet unvisited surfaces; then, attempt to absorb the surface into a surface previously accepted into
	// the merged surface list; if unsuccessful, add the surface to the merged surface list
	
	m_merged_coll_data.num_surfaces = 0;
	
	// loop over the collision data surfaces
	for (int proposed_idx = m_coll_data.num_surfaces; proposed_idx--; )
	{
		Nx::S2DCollSurface proposed_surface = m_coll_data.p_surfaces[proposed_idx];
		
		if (p_surfaces_used[proposed_idx]) continue;
		
		// check to absorb unused surfaces into the proposed surface
		
		bool clean_pass;
		do // loop over the unused surfaces until we pass through them cleanly, finding no absorption opportunities
		{
			clean_pass = true;
			
			for (int check_idx = proposed_idx; check_idx--; )
			{
				Nx::S2DCollSurface& check_surface = m_coll_data.p_surfaces[check_idx];
				
				// check that this line is not already used
				if (p_surfaces_used[check_idx]) continue;
				
				// check that surfaces are parallel
				if (Mth::DotProduct(proposed_surface.normal, check_surface.normal) < 0.99f) continue;
				
				// check that collision lines are parallel
				if (Mth::Abs(Mth::DotProduct(p_line_directions[proposed_idx], p_line_directions[check_idx])) < 0.99f) continue;
				
				// check that collision lines connect
				if (very_close(proposed_surface.ends[0].point, check_surface.ends[0].point))
				{
	   				proposed_surface.ends[0].point = check_surface.ends[1].point;
	   				proposed_surface.ends[0].tangent_exists = check_surface.ends[1].tangent_exists;
	   				proposed_surface.ends[0].tangent = check_surface.ends[1].tangent;
					
				}
				else if (very_close(proposed_surface.ends[0].point, check_surface.ends[1].point))
				{
	   				proposed_surface.ends[0].point = check_surface.ends[0].point;
	   				proposed_surface.ends[0].tangent_exists = check_surface.ends[0].tangent_exists;
	   				proposed_surface.ends[0].tangent = check_surface.ends[0].tangent;
				}
				else if (very_close(proposed_surface.ends[1].point, check_surface.ends[0].point))
				{
					proposed_surface.ends[1].point = check_surface.ends[1].point;
	   				proposed_surface.ends[1].tangent_exists = check_surface.ends[1].tangent_exists;
	   				proposed_surface.ends[1].tangent = check_surface.ends[1].tangent;
				}
				else if (very_close(proposed_surface.ends[1].point, check_surface.ends[1].point))
				{
					proposed_surface.ends[1].point = check_surface.ends[0].point;
	   				proposed_surface.ends[1].tangent_exists = check_surface.ends[0].tangent_exists;
	   				proposed_surface.ends[1].tangent = check_surface.ends[0].tangent;
				}
				else
				{
					continue;
				}
				
				// we've successfully absorbed check_surface into proposed_surface
				p_surfaces_used[check_idx] = true;
				
				clean_pass = false;
			} // END loop over unvisited surfaces
		} while (clean_pass = false);
		
		// accept the poropsed surface into the reduced surface list
		
		m_merged_coll_data.p_surfaces[m_merged_coll_data.num_surfaces++] = proposed_surface;
	} // END loop over proposing surfaces
}

void CRectFeeler::DebugLines ( int r, int g, int b, int num_frames ) const
{
	uint32 color = MAKE_RGB(r, g, b);
	
	Gfx::AddDebugLine(m_corner, m_corner + m_first_edge, color, color, num_frames);
	Gfx::AddDebugLine(m_corner + m_first_edge, m_corner + m_first_edge + m_second_edge, color, color, num_frames);
	Gfx::AddDebugLine(m_corner + m_first_edge + m_second_edge, m_corner + m_second_edge, color, color, num_frames);
	Gfx::AddDebugLine(m_corner + m_second_edge, m_corner, color, color, num_frames);
	
	// if we've merged the surfaces
	if (m_merged_coll_data.num_surfaces > 0)
	{
		// draw the merged collision lines
		for (int n = m_merged_coll_data.num_surfaces; n--; )
		{
			color = MAKE_RGB(r += 201, g += 99, b += 31);
			Gfx::AddDebugLine(m_merged_coll_data.p_surfaces[n].ends[0].point, m_merged_coll_data.p_surfaces[n].ends[1].point, color, color, 1);
		}
	}
	else
	{
		// otherwise, draw the bare collision lines
		for (int n = m_coll_data.num_surfaces; n--; )
		{
			color = MAKE_RGB(r += 201, g += 99, b += 31);
			Gfx::AddDebugLine(m_coll_data.p_surfaces[n].ends[0].point, m_coll_data.p_surfaces[n].ends[1].point, color, color, 1);
		}
	}
}
	

