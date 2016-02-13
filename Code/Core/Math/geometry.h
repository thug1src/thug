/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Core Library											**
**																			**
**	Module:			Math (MTH)			   									**
**																			**
**	File name:		core/math/geometry.h										**
**																			**
**	Created:		11/29/99	-	mjb										**
**																			**
**	Description:	Vector Math Class										**
**																			**
*****************************************************************************/

#ifndef __CORE_MATH_GEOMETRY_H
#define __CORE_MATH_GEOMETRY_H

/*****************************************************************************
**								   Includes									**
*****************************************************************************/

#include <core/math.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/


namespace Mth
{




/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/


/****************************************************************************
 *																			
 * Class:			Vector
 *
 * Description:		Vector math class
 *
 ****************************************************************************/

class Plane;

class  Line
{
	
	
	friend class Plane;

public:

			Line();
			Line ( const Vector &start, const Vector &end );			
//        	Line&			operator+=	( const Vector& v );			

			inline void			MirrorAboutStart();
			inline void			FlipDirection();

			bool			operator==  ( const Line& l ) const;
			bool			operator!=  ( const Line& l ) const;
        	Line&			operator+=	( const Vector& v );
        	Line&			operator-=	( const Vector& v );

			float			Length() const {return (m_end-m_start).Length();}			
			
			Vector m_start, m_end;
private:
};


class  Plane
{
	
	
	friend class Line;

public:
		Plane();
		Plane (const Vector &point, const Vector &normal);
		Vector	m_point, m_normal;
private: 
};

class	Rectangle
{
public:
		Rectangle (   );
		Rectangle ( const Vector& corner, const Vector& first_edge, const Vector& second_edge );
		Vector m_corner, m_first_edge, m_second_edge;
private:
};


/****************************************************************************
 *																			
 * Class:			CBBox
 *
 * Description:		Axis-aligned Bounding Box
 *
 ****************************************************************************/

class CBBox
{
public:
	// Constructors
	CBBox();
	CBBox(const Vector &point);
	CBBox(const Vector &min, const Vector &max);

	//
	inline void Set(const Vector &min, const Vector &max);
	inline void Reset();

    //
    inline const Vector &GetMin() const;
    inline const Vector &GetMax() const;
    inline void SetMin(const Vector &min);
    inline void SetMax(const Vector &max);

	//
	void AddPoint(const Vector &point);
	inline bool Intersect(const CBBox &bbox) const;
	inline bool CouldIntersect(const Vector &v0, const Vector &v1, const Vector &v2) const;	// Intersect w/ triangle
	inline bool Within(const Vector &point) const;
	inline bool Within(const CBBox &bbox) const;
	int WithinAxes(const Vector &point, uint32 &axis_flags) const;		// Returns number of axes within and flags for which ones

	// Checks to see if the line intersects any of the sides
	bool LineIntersect( const Mth::Line &line, Mth::Vector &point, Mth::Vector &normal ) const;

	// Finds the closest point on the bounding box to a position
	void GetClosestIntersectPoint(const Mth::Vector &pos, Mth::Vector &point) const;

	void DebugRender(uint32 rgba, int frames = 0) const;
	void DebugRender(const Mth::Matrix & transform, uint32 rgba, int frames = 0) const;

private:
	//
	Vector m_min, m_max;
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

bool LineLineIntersect( Line & l1, Line & l2, Vector *pa, Vector *pb, float *mua, float *mub, bool clamp = true );


/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool					Line::operator== ( const Line& l ) const
{	
	

	return  ( m_start == l.m_start) && ( m_end == l.m_end );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool					Line::operator!= ( const Line& l ) const
{	
	

	return !( *this == l );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Line&		Line::operator+= ( const Vector& v )
{
	

	m_start += v;
	m_end += v;		
	
	return *this;
}		

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Line&		Line::operator-= ( const Vector& v )
{
	

	m_start -= v;
	m_end -= v;	
	
	return *this;
}



// A->B becomes B<-A, where A is unchanged
inline void  Line::MirrorAboutStart()
{
	m_end = m_start - 2.0f * ( m_end - m_start );	
}

// A->B becores B->A  (A and B swap positions)
inline void Line::FlipDirection()
{
	Mth::Vector temp = m_end;
	m_end = m_start;
	m_start = temp;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void CBBox::Reset()   
{
//	m_min = Vector( (float)HUGE_VAL,  (float)HUGE_VAL,  (float)HUGE_VAL);
//	m_max = Vector((float)-HUGE_VAL, (float)-HUGE_VAL, (float)-HUGE_VAL);
	m_min = Vector( 10000000.0f,  10000000.0f,  10000000.0f);
	m_max = Vector(-10000000.0f, -10000000.0f, -10000000.0f);
}

inline void CBBox::Set(const Vector &min, const Vector &max)			
{
	m_min = min;
	m_max = max;
}

inline const Vector &CBBox::GetMin() const
{ 
	return m_min;
}

inline const Vector &CBBox::GetMax() const
{ 
	return m_max;
}

inline void CBBox::SetMin(const Vector &min)
{
	m_min = min;
}

inline void CBBox::SetMax(const Vector &max)
{
	m_max = max;
}

// for intersection test, we do X and Z first, 
// as we war aer more likely to be intersecting in the Y
// as everyhting is about the same height
// so this eliminates thigns quicker
inline bool CBBox::Intersect(const CBBox &bbox) const
{
	if ((m_min[X] > bbox.m_max[X]) || (bbox.m_min[X] > m_max[X]) ||
		(m_min[Z] > bbox.m_max[Z]) || (bbox.m_min[Z] > m_max[Z]) ||
		(m_min[Y] > bbox.m_max[Y]) || (bbox.m_min[Y] > m_max[Y]))
	{
		return false;
	}

	return true;
}

inline bool CBBox::Within(const Vector &point) const
{
	return ((point[X] >= m_min[X]) && (point[Y] >= m_min[Y]) && (point[Z] >= m_min[Z]) &&
			(point[X] <= m_max[X]) && (point[Y] <= m_max[Y]) && (point[Z] <= m_max[Z]));
}

inline bool CBBox::Within(const CBBox &bbox) const
{
	if ((bbox.m_min[X] >= m_min[X]) && (bbox.m_max[X] <= m_max[X]) &&
		(bbox.m_min[Y] >= m_min[Y]) && (bbox.m_max[Y] <= m_max[Y]) &&
		(bbox.m_min[Z] >= m_min[Z]) && (bbox.m_max[Z] <= m_max[Z]))
	{
		return true;
	}

	return false;
}

// Not So Quick & Dirty Intersection w/ Triangle
inline bool CBBox::CouldIntersect(const Vector &v0, const Vector &v1, const Vector &v2) const
{
	if ( ( v0[X] > m_max[X] ) &&
		 ( v1[X] > m_max[X] ) &&
		 ( v2[X] > m_max[X] ) ) return false;
	if ( ( v0[X] < m_min[X] ) &&
		 ( v1[X] < m_min[X] ) &&
		 ( v2[X] < m_min[X] ) ) return false;
	if ( ( v0[Y] > m_max[Y] ) &&
		 ( v1[Y] > m_max[Y] ) &&
		 ( v2[Y] > m_max[Y] ) ) return false;
	if ( ( v0[Y] < m_min[Y] ) &&
		 ( v1[Y] < m_min[Y] ) &&
		 ( v2[Y] < m_min[Y] ) ) return false;
	if ( ( v0[Z] > m_max[Z] ) &&
		 ( v1[Z] > m_max[Z] ) &&
		 ( v2[Z] > m_max[Z] ) ) return false;
	if ( ( v0[Z] < m_min[Z] ) &&
		 ( v1[Z] < m_min[Z] ) &&
		 ( v2[Z] < m_min[Z] ) ) return false;

	return true;

}

} // namespace Mth

#endif // __CORE_MATH_GEOMETRY_H

