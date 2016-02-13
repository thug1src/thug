/*****************************************************************************
**																			**
**			              Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Core Library											**
**																			**
**	Module:			Geometry (MTH)	 										**
**																			**
**	File name:		Geometry.cpp											**
**																			**
**	Created by:		11/14/00	-	Mick									**
**																			**
**	Description:	Math 									**
// This module handles the representation and manipulation of line segements
// and planes
// 
// a line segment (Mth::Line) is defined by two points, m_start and m_end
//
// a plane is defined by a point and a normal

**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/math.h>
#include <core/math/geometry.h>

#include <gfx/debuggfx.h>

/*****************************************************************************
**							  DBG Information								**
*****************************************************************************/

namespace Mth
{



/*****************************************************************************
**							  DBG Information								**
*****************************************************************************/

//

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
**							   Private Functions							**
*****************************************************************************/


/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

Line::Line()
{
}

Line::Line ( const Vector &start, const Vector &end )			
{
		m_start = start;
		m_end = end;
}

Plane::Plane()
{
} 
 
Plane::Plane (const Vector &point, const Vector &normal)
{
		m_point = point;
		m_normal = normal;
}		

Rectangle::Rectangle()
{
}

Rectangle::Rectangle ( const Vector& corner, const Vector& first_edge, const Vector& second_edge )
{
	m_corner = corner;
	m_first_edge = first_edge;
	m_second_edge = second_edge;
}

/*
   Calculate the line segment PaPb that is the shortest route between
   two lines P1P2 and P3P4. Calculate also the values of mua and mub where
      Pa = P1 + mua (P2 - P1)
      Pb = P3 + mub (P4 - P3)
   Return FALSE if no solution exists.
   original algorithm from  http://www.swin.edu.au/astronomy/pbourke/geometry/lineline3d/
   note that I (Mick) modified it to clamp the points to within the line segments
   if you remove the "Clamp" calls below, then the lines will be assumed to be
   of infinite length 
*/

#define EPS  0.00001f

bool LineLineIntersect( Line & l1, Line & l2, Vector *pa, Vector *pb, float *mua, float *mub, bool clamp )
{

	Vector &p1 = l1.m_start;
	Vector &p2 = l1.m_end;
	Vector &p3 = l2.m_start;
	Vector &p4 = l2.m_end;
   
	Vector p13,p43,p21;
	  
	float d1343,d4321,d1321,d4343,d2121;
	float numer,denom;
	
	p13 = p1 - p3;
	p43 = p4 - p3;
	
	if (Abs(p43[X])  < EPS && Abs(p43[Y])  < EPS && Abs(p43[Z])  < EPS)
	  return(false);
	
	p21 = p2 - p1;  
		  
	if (Abs(p21[X])  < EPS && Abs(p21[Y])  < EPS && Abs(p21[Z])  < EPS)
	  return(false);

	d1343 = DotProduct(p13,p43);
	d4321 = DotProduct(p43,p21);
	d1321 = DotProduct(p13,p21);
	d4343 = DotProduct(p43,p43);
	d2121 = DotProduct(p21,p21);
	
	denom = d2121 * d4343 - d4321 * d4321;
	if (Abs(denom) < EPS)
	  return(false);
	numer = d1343 * d4321 - d1321 * d4343;
	
	*mua = numer / denom;
	if( clamp )
	{
		*mua = Clamp(*mua,0.0f,1.0f);
	}
	*mub = (d1343 + d4321 * (*mua)) / d4343;

	if( clamp )
	{
		*mub = Clamp(*mub,0.0f,1.0f);
	}
	
	*pa = p1 + (*mua * p21);     
	*pb = p3 + (*mub * p43);     
	
	return(true);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CBBox::CBBox()
{
	Reset();
}

CBBox::CBBox(const Vector &point)   
{
	Set(point, point);
}

CBBox::CBBox(const Vector &min, const Vector &max)			
{
	Set(min, max);
}

void CBBox::AddPoint(const Vector &point)   
{
	// Adjust min/max points
	if (point[X] < m_min[X])
		m_min[X] = point[X];
	if (point[Y] < m_min[Y])
		m_min[Y] = point[Y];
	if (point[Z] < m_min[Z])
		m_min[Z] = point[Z];

	if (point[X] > m_max[X])
		m_max[X] = point[X];
	if (point[Y] > m_max[Y])
		m_max[Y] = point[Y];
	if (point[Z] > m_max[Z])
		m_max[Z] = point[Z];
}

// Returns number of axes within and flags for which ones
int CBBox::WithinAxes(const Vector &point, uint32 &axis_flags) const
{
	int number_of_axes = 0;
	axis_flags = 0;

	if ((point[X] >= m_min[X]) && (point[X] <= m_max[X]))
	{
		number_of_axes++;
		axis_flags |= 1 << X;
	}

	if ((point[Y] >= m_min[Y]) && (point[Y] <= m_max[Y]))
	{
		number_of_axes++;
		axis_flags |= 1 << Y;
	}

	if ((point[Z] >= m_min[Z]) && (point[Z] <= m_max[Z]))
	{
		number_of_axes++;
		axis_flags |= 1 << Z;
	}

	return number_of_axes;
}

void CBBox::DebugRender(uint32 rgba, int frames) const
{
	// Min YZ square
	Gfx::AddDebugLine(Mth::Vector(m_min[X],m_min[Y],m_min[Z]),Mth::Vector(m_min[X],m_min[Y],m_max[Z]),rgba,rgba,frames);
	Gfx::AddDebugLine(Mth::Vector(m_min[X],m_min[Y],m_max[Z]),Mth::Vector(m_min[X],m_max[Y],m_max[Z]),rgba,rgba,frames);
	Gfx::AddDebugLine(Mth::Vector(m_min[X],m_max[Y],m_max[Z]),Mth::Vector(m_min[X],m_max[Y],m_min[Z]),rgba,rgba,frames);
	Gfx::AddDebugLine(Mth::Vector(m_min[X],m_max[Y],m_min[Z]),Mth::Vector(m_min[X],m_min[Y],m_min[Z]),rgba,rgba,frames);

	// Max YZ square
	Gfx::AddDebugLine(Mth::Vector(m_max[X],m_min[Y],m_min[Z]),Mth::Vector(m_max[X],m_min[Y],m_max[Z]),rgba,rgba,frames);
	Gfx::AddDebugLine(Mth::Vector(m_max[X],m_min[Y],m_max[Z]),Mth::Vector(m_max[X],m_max[Y],m_max[Z]),rgba,rgba,frames);
	Gfx::AddDebugLine(Mth::Vector(m_max[X],m_max[Y],m_max[Z]),Mth::Vector(m_max[X],m_max[Y],m_min[Z]),rgba,rgba,frames);
	Gfx::AddDebugLine(Mth::Vector(m_max[X],m_max[Y],m_min[Z]),Mth::Vector(m_max[X],m_min[Y],m_min[Z]),rgba,rgba,frames);

	// lines joining corners
	Gfx::AddDebugLine(Mth::Vector(m_min[X],m_min[Y],m_min[Z]),Mth::Vector(m_max[X],m_min[Y],m_min[Z]),rgba,rgba,frames);
	Gfx::AddDebugLine(Mth::Vector(m_min[X],m_min[Y],m_max[Z]),Mth::Vector(m_max[X],m_min[Y],m_max[Z]),rgba,rgba,frames);
	Gfx::AddDebugLine(Mth::Vector(m_min[X],m_max[Y],m_max[Z]),Mth::Vector(m_max[X],m_max[Y],m_max[Z]),rgba,rgba,frames);
	Gfx::AddDebugLine(Mth::Vector(m_min[X],m_max[Y],m_min[Z]),Mth::Vector(m_max[X],m_max[Y],m_min[Z]),rgba,rgba,frames);
}

void CBBox::DebugRender(const Mth::Matrix & transform, uint32 rgba, int frames) const
{
	uint32 rgba_min = 0xFF000080;
	uint32 rgba_max = 0x0000FF80;

	Mth::Vector min_yz_1(m_min[X], m_min[Y], m_min[Z]);
	Mth::Vector min_yz_2(m_min[X], m_min[Y], m_max[Z]);
	Mth::Vector min_yz_3(m_min[X], m_max[Y], m_max[Z]);
	Mth::Vector min_yz_4(m_min[X], m_max[Y], m_min[Z]);

	Mth::Vector max_yz_1(m_max[X], m_min[Y], m_min[Z]);
	Mth::Vector max_yz_2(m_max[X], m_min[Y], m_max[Z]);
	Mth::Vector max_yz_3(m_max[X], m_max[Y], m_max[Z]);
	Mth::Vector max_yz_4(m_max[X], m_max[Y], m_min[Z]);

	// Transform
	min_yz_1 = transform.Transform(min_yz_1);
	min_yz_2 = transform.Transform(min_yz_2);
	min_yz_3 = transform.Transform(min_yz_3);
	min_yz_4 = transform.Transform(min_yz_4);

	max_yz_1 = transform.Transform(max_yz_1);
	max_yz_2 = transform.Transform(max_yz_2);
	max_yz_3 = transform.Transform(max_yz_3);
	max_yz_4 = transform.Transform(max_yz_4);

	// Min YZ square
	Gfx::AddDebugLine(min_yz_1, min_yz_2,rgba_min,rgba_min,frames);
	Gfx::AddDebugLine(min_yz_2, min_yz_3,rgba_min,rgba_min,frames);
	Gfx::AddDebugLine(min_yz_3, min_yz_4,rgba_min,rgba_min,frames);
	Gfx::AddDebugLine(min_yz_4, min_yz_1,rgba_min,rgba_min,frames);

	// Max YZ square
	Gfx::AddDebugLine(max_yz_1, max_yz_2,rgba_max,rgba_max,frames);
	Gfx::AddDebugLine(max_yz_2, max_yz_3,rgba_max,rgba_max,frames);
	Gfx::AddDebugLine(max_yz_3, max_yz_4,rgba_max,rgba_max,frames);
	Gfx::AddDebugLine(max_yz_4, max_yz_1,rgba_max,rgba_max,frames);

	// lines joining corners
	Gfx::AddDebugLine(min_yz_1, max_yz_1,rgba,rgba,frames);
	Gfx::AddDebugLine(min_yz_2, max_yz_2,rgba,rgba,frames);
	Gfx::AddDebugLine(min_yz_3, max_yz_3,rgba,rgba,frames);
	Gfx::AddDebugLine(min_yz_4, max_yz_4,rgba,rgba,frames);
}

bool CBBox::LineIntersect( const Mth::Line &line, Mth::Vector &point, Mth::Vector &normal ) const
{
	bool start_point_within = Within(line.m_start);
	bool end_point_within = Within(line.m_end);

	// Doesn't intersect side if both points within
	if (start_point_within && end_point_within)
	{
		return false;
	}

	// Trivial rejection.
	if ((line.m_start[Y] > m_max[Y]) && (line.m_end[Y] > m_max[Y])) return false;
	if ((line.m_start[Y] < m_min[Y]) && (line.m_end[Y] < m_min[Y])) return false;
	if ((line.m_start[X] > m_max[X]) && (line.m_end[X] > m_max[X])) return false;
	if ((line.m_start[X] < m_min[X]) && (line.m_end[X] < m_min[X])) return false;
	if ((line.m_start[Z] > m_max[Z]) && (line.m_end[Z] > m_max[Z])) return false;
	if ((line.m_start[Z] < m_min[Z]) && (line.m_end[Z] < m_min[Z])) return false;

	float dx = line.m_end[X] - line.m_start[X];
	float dy = line.m_end[Y] - line.m_start[Y];
	float dz = line.m_end[Z] - line.m_start[Z];
	
	// avoid divide by zeros.
	if ( !dx )
	{
		dx = ( 0.000001f );
	}
	if ( !dy )
	{
		dy = ( 0.000001f );
	}
	if ( !dz )
	{
		dz = ( 0.000001f );
	}

	//
	// Garrett: I back-face cull 3 of the sides to reduce the calculations and keep
	// the collisions down to one side.

	// Check the max-x face.
	if (line.m_start[X] > m_max[X] && line.m_end[X] < m_max[X] && (dx < 0.0f))
	{
		// It crosses the plane of the face, so calculate the y & z coords
		// of the intersection and see if they are in the face,
		float d=m_max[X] - line.m_start[X];
		float y=d*dy/dx + line.m_start[Y];
		float z=d*dz/dx + line.m_start[Z];
		if (y < m_max[Y] && y > m_min[Y] && z < m_max[Z] && z > m_min[Z])
		{
			// It does collide!
			point = Mth::Vector(m_max[X], y, z, 1.0f);
			normal = Mth::Vector(1.0f, 0.0f, 0.0f, 0.0f);
			return true;
		}
	}

	// Check the min-x face.
	if (line.m_start[X] < m_min[X] && line.m_end[X] > m_min[X] && (dx > 0.0f))
	{
		// It crosses the plane of the face, so calculate the y & z coords
		// of the intersection and see if they are in the face,
		float d=m_min[X] - line.m_start[X];
		float y=d*dy/dx + line.m_start[Y];
		float z=d*dz/dx + line.m_start[Z];
		if (y < m_max[Y] && y > m_min[Y] && z < m_max[Z] && z > m_min[Z])
		{
			// It does collide!
			point = Mth::Vector(m_min[X], y, z, 1.0f);
			normal = Mth::Vector(-1.0f, 0.0f, 0.0f, 0.0f);
			return true;
		}
	}

	// Check the max-y face.
	if (line.m_start[Y] > m_max[Y] && line.m_end[Y] < m_max[Y] && (dy < 0.0f))
	{
		// It crosses the plane of the face, so calculate the x & z coords
		// of the intersection and see if they are in the face,
		float d=m_max[Y] - line.m_start[Y];
		float x=d*dx/dy + line.m_start[X];
		float z=d*dz/dy + line.m_start[Z];
		if (x < m_max[X] && x > m_min[X] && z < m_max[Z] && z > m_min[Z])
		{
			// It does collide!
			point = Mth::Vector(x, m_max[Y], z, 1.0f);
			normal = Mth::Vector(0.0f, 1.0f, 0.0f, 0.0f);
			return true;
		}
	}

	// Check the min-y face.
	if (line.m_start[Y] < m_min[Y] && line.m_end[Y] > m_min[Y] && (dy > 0.0f))
	{
		// It crosses the plane of the face, so calculate the x & z coords
		// of the intersection and see if they are in the face,
		float d=m_min[Y] - line.m_start[Y];
		float x=d*dx/dy + line.m_start[X];
		float z=d*dz/dy + line.m_start[Z];
		if (x < m_max[X] && x > m_min[X] && z < m_max[Z] && z > m_min[Z])
		{
			// It does collide!
			point = Mth::Vector(x, m_min[Y], z, 1.0f);
			normal = Mth::Vector(0.0f, -1.0f, 0.0f, 0.0f);
			return true;
		}
	}

	// Check the max-z face.
	if (line.m_start[Z] > m_max[Z] && line.m_end[Z] < m_max[Z] && (dz < 0.0f))
	{
		// It crosses the plane of the face, so calculate the x & y coords
		// of the intersection and see if they are in the face,
		float d=m_max[Z] - line.m_start[Z];
		float x=d*dx/dz + line.m_start[X];
		float y=d*dy/dz + line.m_start[Y];
		if (x < m_max[X] && x > m_min[X] && y < m_max[Y] && y > m_min[Y])
		{
			// It does collide!
			point = Mth::Vector(x, y, m_max[Z], 1.0f);
			normal = Mth::Vector(0.0f, 0.0f, 1.0f, 0.0f);
			return true;
		}
	}


	// Check the min-z face.
	if (line.m_start[Z] < m_min[Z] && line.m_end[Z] > m_min[Z] && (dz > 0.0f))
	{
		// It crosses the plane of the face, so calculate the x & y coords
		// of the intersection and see if they are in the face,
		float d=m_min[Z] - line.m_start[Z];
		float x=d*dx/dz + line.m_start[X];
		float y=d*dy/dz + line.m_start[Y];
		if (x < m_max[X] && x > m_min[X] && y < m_max[Y] && y > m_min[Y])
		{
			// It does collide!
			point = Mth::Vector(x, y, m_min[Z], 1.0f);
			normal = Mth::Vector(0.0f, 0.0f, -1.0f, 0.0f);
			return true;
		}
	}

	return false;
}

void CBBox::GetClosestIntersectPoint(const Mth::Vector &pos, Mth::Vector &point) const
{
	uint32 axes_within_flags;
	int axes_within = WithinAxes(pos, axes_within_flags);

	if (axes_within == 3)
	{
		float closest_dist = -1.0f;
		int closest_axis = -1;
		float closest_axis_coord = 0.0f;		// Value of the minimum axis coordinate

		// Find the closest rectangle and move the point to that rectangle to find the intersection
		// point.
		for (int axis = X; axis <= Z; axis++)
		{
			float min_to_pos_dist = pos[axis] - m_min[axis];
			float max_to_pos_dist = m_max[axis] - pos[axis];

			Dbg_Assert(min_to_pos_dist >= 0.0f);
			Dbg_Assert(max_to_pos_dist >= 0.0f);

			float min_dist;
			float min_coord;

			// Figure out the closest distance
			if (min_to_pos_dist < max_to_pos_dist)
			{
				min_dist = min_to_pos_dist;
				min_coord = m_min[axis];
			}
			else
			{
				min_dist = max_to_pos_dist;
				min_coord = m_max[axis];
			}
			
			// See if this is the first axis or if this one is closer
			if ((axis == X) || (min_dist < closest_dist))
			{
				closest_axis = axis;
				closest_dist = min_dist;
				closest_axis_coord = min_coord;
			}
		}

		Dbg_Assert((closest_axis >= X) && (closest_axis <= Z));
		Dbg_Assert(closest_dist >= 0.0f);

		// Calc the intersection point
		point = pos;				// Start with position
		point[closest_axis] = closest_axis_coord;

#if 0
		static int print_now;
		if ((print_now++ % 60) == 0)
		{
			Dbg_Message("Closest axis %d, (min - max) pos (%f - %f) %f, closest dist %f, closest coord %f", closest_axis,
						m_min[closest_axis], m_max[closest_axis], pos[closest_axis], closest_dist, closest_axis_coord);
		}
#endif
	}
	else
	{
		point[W] = 1.0f;			// Homogeneous

		// Go through each axis.  If the pos coordinate is between the min and max, use that
		// coordinate.  Otherwise, use the min or max coordinate that is closest.
		for (int axis = X; axis <= Z; axis++)
		{
			if (axes_within_flags & (1 << axis))
			{
				// Within, use pos coord
				point[axis] = pos[axis];
			}
			else
			{
				// Figure out which end we need
				if (pos[axis] <= m_min[axis])
				{
					point[axis] = m_min[axis];
				}
				else
				{
					point[axis] = m_max[axis];
				}
			}
		}
	}
}


}



