/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Nx								 						**
**																			**
**	File name:		gel\collision\collide.cpp      							**
**																			**
**	Created by:		02/20/02	-	grj										**
**																			**
**	Description:															**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <gel/collision/collision.h>
#include <gel/collision/colltridata.h>
#include <gel/collision/movcollman.h>
#include <gel/collision/collcache.h>
#include <gel/collision/batchtricoll.h>
#include <engine/SuperSector.h>

#include <gel/object/compositeobject.h>

#include <gfx/nx.h>
#include <gfx/nxflags.h>	// for face flag stuff
#include <gfx/debuggfx.h>

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

#ifdef	__PLAT_NGPS__
#define USE_VU0_MICROMODE
#endif

#define PRINT_CACHE_HITS 0

/*****************************************************************************
**								Private Types								**
*****************************************************************************/


/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

Mth::Matrix		CCollStatic::sOrient;
Mth::Vector		CCollStatic::sWorldPos;

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

static bool s_check_for_far = false;			// if true then we return the farthest result

bool CCollObj::s_found_collision(const Mth::Line *p_is, CCollObj *p_coll_object, SCollSurface *p_collSurface,
								 float distance, CollData *p_collData)
{
	Dbg_Assert( p_collData );
	
	bool good = false;
	bool updated = false;

	if (s_check_for_far)
	{
		if( distance >= p_collData->dist )
		{
			good = true;
		}
	}
	else
	{
		//Dbg_Message("Checking new dist %f against old dist %f", distance, p_collData->dist);
		if( distance <= p_collData->dist )
		{
			good = true;
		}
	}
	CollData 	old;
	
	
	if (!good)
	{
		old = *p_collData;				 
	}
			 

	// Get the user plugin offset														
	int index = p_collSurface->index;		// the index of the surface in the object
	
	// get the face flags from within this
	// note that this uses the "index" field, which is
	// an index of a face within the sector
	uint32  flags = p_coll_object->GetFaceFlags(index);
	
	// Test flags against the ignore_1 (ignore if a bit is set in flags)
	// and against ignore_0 (ignore if the bit is clear in flags)
					
	//Dbg_Message("*Flags %x ignore_0 %x ignore_1 %x", flags, p_collData->ignore_0, p_collData->ignore_1);
	if (!(flags & p_collData->ignore_1) && !(~flags & p_collData->ignore_0))
	{
		// Setup the collData members		
		
		p_collData->coll_found = true;     		// found at least one (might be overrridden, but found at least one)
		p_collData->surface = *p_collSurface;	// Store the collision surface
		p_collData->dist = distance;			// set distance (will end up being the smallest dist)
		p_collData->flags = flags; 				// Store face flags
		p_collData->terrain = (ETerrainType) p_coll_object->GetFaceTerrainType(index);
		p_collData->p_coll_object = p_coll_object;
		p_collData->trigger = (flags& mFD_TRIGGER);	// set flag if it's a trigger

		p_collData->node_name =  p_coll_object->GetChecksum();
		// Determine the point of intersection on the triangle
		// which will be in the direction of the line's vector at "distance" length
		Mth::Vector coll_point, line_vec;
		line_vec = p_is->m_end - p_is->m_start;

		coll_point = line_vec.Scale(distance);
		coll_point = p_is->m_start + coll_point;
		coll_point[W] = 1.0f;					// Hack to force into homogenious space.  Even though the math is correct here,
												// many times the line data fed to it isn't.
		p_collData->surface.point = coll_point;	// After looking at their implementation, I don't
												// trust "point" so I recalculate it

		// check for additional per-face callback
		
		if (p_collData->callback)
		{
			// call the callback function, and pass it this col data 
			p_collData->callback(p_collData);
		}
	
		updated = true;
	}
		
	if (!good)
	{
		*p_collData = old;				 
	}

	return (good && updated);
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCollObj::sRayTriangleCollision(const Mth::Vector *rayStart, const Mth::Vector *rayDir,
									 Mth::Vector *v0, Mth::Vector *v1, Mth::Vector *v2, float *t)
{
#if 0
// #ifdef	__PLAT_NGPS__
// Turned off assem versions to test correction to false positive issue as discussed in C code below.  Dan - 1/21/3
#ifdef	USE_VU0_MICROMODE
	register bool result;

	asm __volatile__(
	"
	.set noreorder
	lqc2    vf06,%4				# v0
	lqc2    vf08,%6				# v2
	lqc2    vf07,%5				# v1
	lqc2    vf04,%2				# rayStart
	lqc2    vf05,%3				# rayDir

	vcallms	RayTriangleCollision	# call microsubroutine
	vnop							# interlocking instruction, waits for vu0 completion

	cfc2	%0,$vi02			# get boolean result from vu0

	beq		%0, $0, vu0RayTriDone	# skip copy of t if not needed
	nop

	qmfc2	$12, $vf17			# move t to $8
	sw		$12, %1				# and write out

vu0RayTriDone:

	.set reorder
	": "=&r" (result), "=m" (*t) : "m" (*rayStart), "m" (*rayDir), "m" (*v0), "m" (*v1) , "m" (*v2) : "$12" );

	return result;
#else
    //*t = 2.0f;
	bool result;

	asm __volatile__(
	"
	.set noreorder
	lqc2    vf06,0x0(%4)		# v0
	lqc2    vf08,0x0(%6)		# v2
	lqc2    vf07,0x0(%5)		# v1
	lqc2    vf04,0x0(%2)		# rayStart
	lqc2    vf05,0x0(%3)		# rayDir

	vsub.xyz vf10, vf08, vf06    # edge2 = v2 - v0
	vsub.xyz vf09, vf07, vf06    # edge1 = v1 - v0 
	vsub.xyz vf13, vf04, vf06    # tvec = rayStart - v0
   	li.s     $f4,0.00001		 # EPSILON
	vaddw.x  vf02, vf00, vf00w   # Load 1.0f to VF02x

	vopmula.xyz  ACC,vf05,vf10   # Cross product of ray normal and edge2 (pvec)
	vopmsub.xyz  vf12,vf10,vf05  # Second part of cross product
   	mfc1         $8,$f4
	li		     %0,0			 # store 0 for return value
	#li      $9,0x80	 		 # Set the mask X sign flag
	#li      $10,0x10			 # Set the mask W sign flag

	vmul.xyz  vf11,vf09,vf12     # det = edge1 * pvec [start]
	vmulax.x  ACC,vf09,vf12x
	vmul.xyz  vf15,vf13,vf12     # u = tvec * pvec [start]
	qmtc2	$8, $vf03
	vmadday.x ACC,vf02,vf11y
	vmaddz.x  vf11,vf02,vf11z    # det = edge1 * pvec [ready]
	vmulax.x  ACC,vf02,vf15x
	vmadday.x ACC,vf02,vf15y
	vmaddz.x  vf15,vf02,vf15z    # u = tvec * pvec [ready]
	vdiv 	  Q,vf00w,vf11x      # Q = 1.0f / det

	vopmula.xyz ACC,vf13,vf09    # qvec = crossProd(tvec, edge1);
	vopmsub.xyz vf14,vf09,vf13
	vsub.x      vf01,vf11,vf03   # If det < EPSILON
		vnop
		vnop
		vnop
		vnop
		vnop
	cfc2    $8,$vi17			 # transfer if det result (MAC register)
	vmulq.x vf15,vf15,Q          # u = (tvec * pvec) / det
	vnop
	vnop
	vnop
		vnop
		andi	$8, $8, 0x80		 # check result
	cfc2    $9,$vi17			 # transfer u result (MAC register)
	bne		$8, $0, vu0RayTriDone
	vaddx.w vf03, vf00, vf03x	 # put 1 + EPSILON in VF03w
	andi	$9, $9, 0x80		 # check result (u < 0) fail
	bne		$9, $0, vu0RayTriDone
	#vaddq.x vf20,vf00,Q          

    vsubw.x vf00,vf15,vf03w		 # if u > 1 + EPSILON
		vnop
		vnop
		vnop
		vnop
		vnop
	cfc2    $8,$vi17			 # transfer if u > 1 + EPSILON result (MAC register)
    vmul.xyz vf16,vf05,vf14      # v = rayDir * qvec [start] 
    vmulax.x ACC,vf05,vf14x
	andi	$8, $8, 0x80		 # check result (u > 1) fail
	beq		$8, $0, vu0RayTriDone
	vnop

    vmadday.x ACC,vf02,vf16
    vmaddz.x vf16,vf02,vf16z     # v = rayDir * qvec [ready]
	vnop
    vmul.xyz vf17,vf10,vf14      # t = edge2 * qvec [start]
    vmulax.x ACC,vf10,vf14x
    vmulq.x   vf16,vf16,Q        # v = (rayDir * qvec) / det;
	vnop
		vnop
		vnop
		vnop
		vnop
	cfc2    $8,$vi17			 # transfer if v < 0 result (MAC register)

	vmadday.x ACC,vf02,vf17
	vmaddz.x  vf17,vf02,vf17z    # t = edge2 * qvec [ready]
	vadd.x    vf01,vf15,vf16
	andi	  $8, $8, 0x80		 # check result (v < 0) fail
	bne		  $8, $0, vu0RayTriDone
    vmulq.x    vf17,vf17,Q       # t = (edge2 * qvec) / det

    vsubw.x vf31,vf01,vf03w      # if (u+v) > 1 + EPSILON [start]
	vnop
	vnop
	vnop
		vnop
		vnop
	cfc2    $8,$vi17			 # transfer if u+v > 1 + EPSILON result (MAC register)
    vsub.x vf00,vf17,vf00
	vnop
		vnop
		vnop
		vnop
	andi	$8, $8, 0x80		 # check result (u+v > 1 + EPSILON) fail
	cfc2    $9,$vi17			 # transfer if t < 0 result (MAC register)
	vsubx.w vf00,vf00,vf17       # if t > 1 [start]
	vnop
		vnop
		#vnop
		#vnop
		#vnop
	beq		$8, $0, vu0RayTriDone
	andi	$9, $9, 0x80		 # check result (t < 0) fail
	bne		$9, $0, vu0RayTriDone
	cfc2    $8,$vi17			 # transfer if t > 1 result (MAC register)
	andi	$8, $8, 0x10		 # check result (t > 1) fail
	bne		$8, $0, vu0RayTriDone
	qmfc2	$9, $vf17			 # move t to $9
	sw		$9, 0(%1)			 # and write out
	li		%0,1				 # store 1 for return value


vu0RayTriDone:
	.set reorder
	": "=r" (result), "+r" (t) : "r" (rayStart), "r" (rayDir), "r" (v0), "r" (v1) , "r" (v2) : "$8", "$9");
	//return *t <= 1.0f;
	return result;
#endif //	USE_VU0_MICROMODE
#else
    const float EPSILON = 0.00001f;

    /* find vectors for two edges sharing vert0 */
    Mth::Vector edge1 = *v1 - *v0;
    Mth::Vector edge2 = *v2 - *v0;
    
    /* begin calculating determinant - also used to calculate U parameter */
    Mth::Vector pvec = Mth::CrossProduct(*rayDir, edge2);

    /* if determinant is near zero, ray lies in plane of triangle */
    const float det = Mth::DotProduct(edge1, pvec);

#if 1 // Dan - 1/21/3
	// Moved division below several false returns and reduced mult count.  This probably removed the need for adding EPSILON to 1.0f for the u and v test,
	// but I don't know this for sure, so left it in.
	// Also, added EPSILON_2.  EPSILON is of appropriate scale for comparing floats of order one to zero, but is too small for comparing floats
	// on the order of the cube of a typical vertex edge.  In certain special cases the dot product in the calculation of det will require the
	// cancelation of very large floats, resulting in a zero which is larger than EPSILON.  This was causing false positives.
	// It is possible that this new larger EPSILON_2 will cause false negatives.  I suppose we shall see.
	// These changes HAVE NOT BEEN MADE to the assembler code versions.  They continue to suffer from the over-small EPSILON issue.

    const float EPSILON_2 = 0.03f;
    
	// We are back-facing, so also throw away negative
    if (det < EPSILON_2)
	{
        return false;
    }

	const float adjusted_det = (1.0f + EPSILON) * det;

    /* calculate distance from vert0 to ray origin */
    const Mth::Vector tvec = *rayStart - *v0;
    
    /* calculate U parameter and test bounds */
    float u = Mth::DotProduct(tvec, pvec);

    if (u < 0.0f || u > adjusted_det) {
        return false;
    }
    
    /* prepare to test V parameter */
    const Mth::Vector qvec = Mth::CrossProduct(tvec, edge1);
    
    /* calculate V parameter and test bounds */
    float v = Mth::DotProduct(*rayDir, qvec);

    if (v < 0.0f || u + v > adjusted_det) {
        return false;
    }

	const float inv_det = 1.0f / det;

    /* calculate t, ray intersects triangle */
    *t = Mth::DotProduct(edge2, qvec) * inv_det;

	// And update u & v, although we don't seem to use it now.
	//u *= inv_det;
	//v *= inv_det;

#else
	// We are back-facing, so also throw away negative
    if (det < EPSILON)
	{
        return false;
    }

	const float inv_det = 1.0f / det;

    /* calculate distance from vert0 to ray origin */
    const Mth::Vector tvec = *rayStart - *v0;
    
    /* calculate U parameter and test bounds */
    float u = Mth::DotProduct(tvec, pvec) * inv_det;

    if (u < 0.0f || u > (1.0f + EPSILON)) {
        return false;
    }
    
    /* prepare to test V parameter */
    const Mth::Vector qvec = Mth::CrossProduct(tvec, edge1);
    
    /* calculate V parameter and test bounds */
    float v = Mth::DotProduct(*rayDir, qvec) * inv_det;

    if (v < 0.0f || u + v > (1.0f + EPSILON)) {
        return false;
    }
    
    /* calculate t, ray intersects triangle */
    *t = Mth::DotProduct(edge2, qvec) * inv_det;

	// And update u & v, although we don't seem to use it now.
	//u *= inv_det;
	//v *= inv_det;
#endif

    return (*t <= 1.0f) && (*t >= 0.0f);
#endif

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCollObj::s2DLineLineCollision ( float p_start_X, float p_start_Y, float p_delta_X, float p_delta_Y, float q_start_X, float q_start_Y, float q_delta_X, float q_delta_Y, float *s )
{
	// determines collision between two lines (p and q) in 2D; reports the interval along p at which the collision occurs
	
	// Antonio's method; Real-Time Rendering, p 337
	
    const float EPSILON = 0.00000000001f;
	
	float a_X = q_delta_X;
	float a_Y = q_delta_Y;
	
	float b_X = p_delta_X;
	float b_Y = p_delta_Y;
	
	float c_X = p_start_X - q_start_X;
	float c_Y = p_start_Y - q_start_Y;
	
	float d = c_X * -a_Y + c_Y * a_X;
	
	float e = c_X * -b_Y + c_Y * b_X;
	
	float f = a_X * -b_Y + a_Y * b_X;
	
	if (f > 0.0f)
	{
		if (d < 0.0f || d > f) return false;
	}
	else
	{
		if (d > 0.0f || d < f) return false;
	}
	
	if (f > 0.0f)
	{
		if (e < 0.0f || e > f) return false;
	}
	else
	{
		if (e > 0.0f || e < f) return false;
	}
	
	if (Mth::Abs(f) < EPSILON)
	{
		*s = 0.0f;
		return true;
	}
	
	*s = d / f;
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CCollObj::sRectangleTriangleCollision(const Mth::Rectangle *rect, const Mth::Vector *v0, const Mth::Vector *v1, const Mth::Vector *v2, Mth::Vector p_collision_endpoints[2], ETriangleEdgeType p_collision_triangle_edges[2] )
{
	// ERIT method; Real-Time Rendering, p 317
	
    const float EPSILON = 0.00001f;
	
	// calculate the plane equation of the rectangle
	Mth::Vector rectangle_n = Mth::CrossProduct(rect->m_first_edge, rect->m_second_edge);
	float rectangle_d = -Mth::DotProduct(rectangle_n, rect->m_corner);
	
	// calculate the distance (times rectangle_n squared) of the triangle vertices from the rectangle's plane
	float d0 = Mth::DotProduct(rectangle_n, *v0) + rectangle_d; 
	float d1 = Mth::DotProduct(rectangle_n, *v1) + rectangle_d; 
	float d2 = Mth::DotProduct(rectangle_n, *v2) + rectangle_d;
	 
	// if all vertices are on the same side of the plane, we reject
	if (d0 > 0.0f && d1 > 0.0f && d2 > 0.0f) return false;
	if (d0 < 0.0f && d1 < 0.0f && d2 < 0.0f) return false;
	
	// snap verticies near the plane to the plane
	if (Mth::Abs(d0) < EPSILON)
	{
		d0 = 0.0f;
	}
	if (Mth::Abs(d1) < EPSILON)
	{
		d1 = 0.0f;
	}
	if (Mth::Abs(d2) < EPSILON)
	{
		d2 = 0.0f;
	}
	
	// find the two points (p and q) on the plane of the rectangle which are the endpoints of the triangle's interception with the rectangle's plane
	Mth::Vector p, q;
	ETriangleEdgeType p_edge, q_edge;
	if ((d0 <= 0.0f && d1 >= 0.0f) || (d0 >= 0.0f && d1 <= 0.0f))
	{
		p_edge = TRIANGLE_EDGE_V0_V1;
		if (Mth::Abs(d0 - d1) > EPSILON)
		{
			p = *v0 + (*v1 - *v0) * d0 / (d0 - d1);
		}
		else
		{
			p = *v0;
		}
		
		if ((d1 <= 0.0f && d2 >= 0.0f) || (d1 >= 0.0f && d2 <= 0.0f))
		{
			q_edge = TRIANGLE_EDGE_V1_V2;
			if (Mth::Abs(d1 - d2) > EPSILON)
			{
				q = *v1 + (*v2 - *v1) * d1 / (d1 - d2);
			}
			else
			{
				q = *v1;
			}
		}
		else
		{
			q_edge = TRIANGLE_EDGE_V2_V0;
			if (Mth::Abs(d2 - d0) > EPSILON)
			{
				q = *v2 + (*v0 - *v2) * d2 / (d2 - d0);
			}
			else
			{
				q = *v2;
			}
		}
	}
	else
	{
		p_edge = TRIANGLE_EDGE_V1_V2;
		if (Mth::Abs(d1 - d2) > EPSILON)
		{
			p = *v1 + (*v2 - *v1) * d1 / (d1 - d2);
		}
		else
		{
			p = *v1;
		}
		
		q_edge = TRIANGLE_EDGE_V2_V0;
		if (Mth::Abs(d0 - d2) > EPSILON)
		{
			q = *v0 + (*v2 - *v0) * d0 / (d0 - d2);
		}
		else
		{
			q = *v0;
		}
	}
	
	// project the rectangle and the intersection endpoints into the coordinate plane which maximizes the area of the rectangle's projection
	
	float r_corner_S, r_corner_T;
	float r_first_edge_S, r_first_edge_T;
	float r_second_edge_S, r_second_edge_T;
	float p_S, p_T;
	float q_S, q_T;
	
	if (Mth::Abs(rectangle_n[X]) < Mth::Abs(rectangle_n[Z]) && Mth::Abs(rectangle_n[Y]) < Mth::Abs(rectangle_n[Z]))
	{
		// project onto XY plane
		r_corner_S = rect->m_corner[X];
		r_corner_T = rect->m_corner[Y];
		r_first_edge_S = rect->m_first_edge[X];
		r_first_edge_T = rect->m_first_edge[Y];
		r_second_edge_S = rect->m_second_edge[X];
		r_second_edge_T = rect->m_second_edge[Y];
		p_S = p[X];
		p_T = p[Y];
		q_S = q[X];
		q_T = q[Y];
	}
	else if (Mth::Abs(rectangle_n[X]) < Mth::Abs(rectangle_n[Y]) && Mth::Abs(rectangle_n[Z]) < Mth::Abs(rectangle_n[Y]))
	{
		// project onto XZ plane
		r_corner_S = rect->m_corner[X];
		r_corner_T = rect->m_corner[Z];
		r_first_edge_S = rect->m_first_edge[X];
		r_first_edge_T = rect->m_first_edge[Z];
		r_second_edge_S = rect->m_second_edge[X];
		r_second_edge_T = rect->m_second_edge[Z];
		p_S = p[X];
		p_T = p[Z];
		q_S = q[X];
		q_T = q[Z];
	}
	else
	{
		// project onto YZ plane
		r_corner_S = rect->m_corner[Y];
		r_corner_T = rect->m_corner[Z];
		r_first_edge_S = rect->m_first_edge[Y];
		r_first_edge_T = rect->m_first_edge[Z];
		r_second_edge_S = rect->m_second_edge[Y];
		r_second_edge_T = rect->m_second_edge[Z];
		p_S = p[Y];
		p_T = p[Z];
		q_S = q[Y];
		q_T = q[Z];
	}
	
	// test p and q against the four edges of the rectangle; if both lie outside any edge, we reject
	
	// these variables define a rectangle's edge in 2D; as + bt + c = 0
	float a, b, c;
	
	float pd, qd;
	float inside_sign;
	
	// not really convinced that this bitmap method of faster 
	// char within = 0;
	// enum
	// {
		// P_TRIANGLE_EDGE_0_MASK = 1 << 0,
		// Q_TRIANGLE_EDGE_0_MASK = 1 << 1,
		// P_TRIANGLE_EDGE_1_MASK = 1 << 2,
		// Q_TRIANGLE_EDGE_1_MASK = 1 << 3,
		// P_TRIANGLE_EDGE_2_MASK = 1 << 4,
		// Q_TRIANGLE_EDGE_2_MASK = 1 << 5,
		// P_TRIANGLE_EDGE_3_MASK = 1 << 6,
		// Q_TRIANGLE_EDGE_3_MASK = 1 << 7,
		// ALL_P_MASKS = P_TRIANGLE_EDGE_0_MASK | P_TRIANGLE_EDGE_1_MASK | P_TRIANGLE_EDGE_2_MASK | P_TRIANGLE_EDGE_3_MASK,
		// ALL_Q_MASKS = Q_TRIANGLE_EDGE_0_MASK | Q_TRIANGLE_EDGE_1_MASK | Q_TRIANGLE_EDGE_2_MASK | Q_TRIANGLE_EDGE_3_MASK
	// };
	bool p_within[4] = { false, false, false, false };
	bool q_within[4] = { false, false, false, false };
	
	// edge 0: corner to corner + first_edge
	// second_edge defines inside direction
	a = r_first_edge_T;
	b = -r_first_edge_S;
	c = r_corner_T * r_first_edge_S - r_corner_S * r_first_edge_T;
	pd = a * p_S + b * p_T + c;
	qd = a * q_S + b * q_T + c;
	inside_sign = a * r_second_edge_S + b * r_second_edge_T;
	if (inside_sign > 0.0f)
	{
		// within = (P_TRIANGLE_EDGE_0_MASK * (pd >= 0.0f)) | (Q_TRIANGLE_EDGE_0_MASK * (qd >= 0.0f));
		p_within[0] = pd >= 0.0f;
		q_within[0] = qd >= 0.0f;
	}
	else
	{
		// within = (P_TRIANGLE_EDGE_0_MASK * (pd <= 0.0f)) | (Q_TRIANGLE_EDGE_0_MASK * (qd <= 0.0f));
		p_within[0] = pd <= 0.0f;
		q_within[0] = qd <= 0.0f;
	}
	// if (!(within & (P_TRIANGLE_EDGE_0_MASK | Q_TRIANGLE_EDGE_0_MASK))) return false;
	if (!p_within[0] && !q_within[0]) return false;
		
	// edge 1: corner + second_edge to corner + first_edge + second_edge
	// second_edge defines outside direction
	c = (r_corner_T + r_second_edge_T) * r_first_edge_S - (r_corner_S + r_second_edge_S) * r_first_edge_T;
	pd = a * p_S + b * p_T + c;
	qd = a * q_S + b * q_T + c;
	if (inside_sign < 0.0f)
	{
		// within |= (P_TRIANGLE_EDGE_1_MASK * (pd >= 0.0f)) | (Q_TRIANGLE_EDGE_1_MASK * (qd >= 0.0f));
		p_within[1] = pd >= 0.0f;
		q_within[1] = qd >= 0.0f;
	}
	else
	{
		// within |= (P_TRIANGLE_EDGE_1_MASK * (pd <= 0.0f)) | (Q_TRIANGLE_EDGE_1_MASK * (qd <= 0.0f));
		p_within[1] = pd <= 0.0f;
		q_within[1] = qd <= 0.0f;
	}
	// if (!(within & (P_TRIANGLE_EDGE_1_MASK | Q_TRIANGLE_EDGE_1_MASK))) return false;
	if (!p_within[1] && !q_within[1]) return false;
	
	// edge 2: corner to corner + second_edge
	// first_edge defines inside direction
	a = r_second_edge_T;
	b = -r_second_edge_S;
	c = r_corner_T * r_second_edge_S - r_corner_S * r_second_edge_T;
	pd = a * p_S + b * p_T + c;
	qd = a * q_S + b * q_T + c;
	inside_sign = a * r_first_edge_S + b * r_first_edge_T;
	if (inside_sign > 0.0f)
	{
		// within |= (P_TRIANGLE_EDGE_2_MASK * (pd >= 0.0f)) | (Q_TRIANGLE_EDGE_2_MASK * (qd >= 0.0f));
		p_within[2] = pd >= 0.0f;
		q_within[2] = qd >= 0.0f;
	}
	else
	{
		// within |= (P_TRIANGLE_EDGE_2_MASK * (pd <= 0.0f)) | (Q_TRIANGLE_EDGE_2_MASK * (qd <= 0.0f));
		p_within[2] = pd <= 0.0f;
		q_within[2] = qd <= 0.0f;
	}
	// if (!(within & (P_TRIANGLE_EDGE_2_MASK | Q_TRIANGLE_EDGE_2_MASK))) return false;
	if (!p_within[2] && !q_within[2]) return false;
	
	// edge 3: corner + first_edge to corner + first_edge + second_edge
	// first_edge defines outside direction
	c = (r_corner_T + r_first_edge_T) * r_second_edge_S - (r_corner_S + r_first_edge_S) * r_second_edge_T;
	pd = a * p_S + b * p_T + c;
	qd = a * q_S + b * q_T + c;
	if (inside_sign < 0.0f)
	{
		// within |= (P_TRIANGLE_EDGE_3_MASK * (pd >= 0.0f)) | (Q_TRIANGLE_EDGE_3_MASK * (qd >= 0.0f));
		p_within[3] = pd >= 0.0f;
		q_within[3] = qd >= 0.0f;
	}
	else
	{
		// within |= (P_TRIANGLE_EDGE_3_MASK * (pd <= 0.0f)) | (Q_TRIANGLE_EDGE_3_MASK * (qd <= 0.0f));
		p_within[3] = pd <= 0.0f;
		q_within[3] = qd <= 0.0f;
	}
	// if (!(within & (P_TRIANGLE_EDGE_3_MASK | Q_TRIANGLE_EDGE_3_MASK))) return false;
	if (!p_within[3] && !q_within[3]) return false;
	
	int next_collision_endpoint_idx;
	
	// if p or q is within all edges, it is the endpoint of the collision
	// if ((within & ALL_P_MASKS) == ALL_P_MASKS)
	if (p_within[0] && p_within[1] && p_within[2] && p_within[3])
	{
        p_collision_endpoints[0] = p;
		p_collision_triangle_edges[0] = p_edge;
		// if ((within & ALL_Q_MASKS) == ALL_Q_MASKS)
		if (q_within[0] && q_within[1] && q_within[2] && q_within[3])
		{
			p_collision_endpoints[1] = q;
			p_collision_triangle_edges[1] = q_edge;
			return true;
		}
		next_collision_endpoint_idx = 1;
	}
	// else if ((within & ALL_Q_MASKS) == ALL_Q_MASKS)
	else if (q_within[0] && q_within[1] && q_within[2] && q_within[3])
	{
		p_collision_endpoints[0] = q;
		p_collision_triangle_edges[0] = q_edge;
		next_collision_endpoint_idx = 1;
	}
	else
	{
		next_collision_endpoint_idx = 0;
	}
	
	// we've found any endpoints of the collision which are within the rectangle; further collision endpoints will lie on the edges of the rectangle;
	 
	// we check each edge of the rectangle for intersection with the triangle's plane-intersection line segment
	
	float s;
	
	// edge 0: corner to corner + first_edge
	if (s2DLineLineCollision(r_corner_S, r_corner_T, r_first_edge_S, r_first_edge_T, p_S, p_T, q_S - p_S, q_T - p_T, &s))
	{
		p_collision_endpoints[next_collision_endpoint_idx] = rect->m_first_edge;
		p_collision_endpoints[next_collision_endpoint_idx] *= s;
		p_collision_endpoints[next_collision_endpoint_idx] += rect->m_corner;
		p_collision_triangle_edges[next_collision_endpoint_idx] = NO_TRIANGLE_EDGE;
		if (next_collision_endpoint_idx == 1) return true;
		next_collision_endpoint_idx = 1;
	}
	
	// edge 1: corner + second_edge to corner + first_edge + second_edge
	if (s2DLineLineCollision(r_corner_S + r_second_edge_S, r_corner_T + r_second_edge_T, r_first_edge_S, r_first_edge_T, p_S, p_T, q_S - p_S, q_T - p_T, &s))
	{
		p_collision_endpoints[next_collision_endpoint_idx] = rect->m_first_edge;
		p_collision_endpoints[next_collision_endpoint_idx] *= s;
		p_collision_endpoints[next_collision_endpoint_idx] += rect->m_corner;
		p_collision_endpoints[next_collision_endpoint_idx] += rect->m_second_edge;
		p_collision_triangle_edges[next_collision_endpoint_idx] = NO_TRIANGLE_EDGE;
		if (next_collision_endpoint_idx == 1) return true;
		next_collision_endpoint_idx = 1;
	}
	
	// edge 2: corner to corner + second_edge
	if (s2DLineLineCollision(r_corner_S, r_corner_T, r_second_edge_S, r_second_edge_T, p_S, p_T, q_S - p_S, q_T - p_T, &s))
	{
		p_collision_endpoints[next_collision_endpoint_idx] = rect->m_second_edge;
		p_collision_endpoints[next_collision_endpoint_idx] *= s;
		p_collision_endpoints[next_collision_endpoint_idx] += rect->m_corner;
		p_collision_triangle_edges[next_collision_endpoint_idx] = NO_TRIANGLE_EDGE;
		if (next_collision_endpoint_idx == 1) return true;
		next_collision_endpoint_idx = 1;
	}
	
	// edge 3: corner + first_edge to corner + first_edge + second_edge
	if (s2DLineLineCollision(r_corner_S + r_first_edge_S, r_corner_T + r_first_edge_T, r_second_edge_S, r_second_edge_T, p_S, p_T, q_S - p_S, q_T - p_T, &s))
	{
		if (next_collision_endpoint_idx == 1)
		{
			p_collision_endpoints[next_collision_endpoint_idx] = rect->m_second_edge;
			p_collision_endpoints[next_collision_endpoint_idx] *= s;
			p_collision_endpoints[next_collision_endpoint_idx] += rect->m_corner;
			p_collision_endpoints[next_collision_endpoint_idx] += rect->m_first_edge;
			p_collision_triangle_edges[next_collision_endpoint_idx] = NO_TRIANGLE_EDGE;
			return true;
		}
	}
	
	// no collision could be found
	return false;
}

////////////////////////////////////////////////////////////////////
// CCollObj member functions
//

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollObj::CCollObj() : m_Flags(0), mp_coll_tri_data(NULL)
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollObj::~CCollObj()
{
	// this should only be done on clones
	if (mp_coll_tri_data && (m_Flags & mSD_CLONE))
	{
		delete mp_coll_tri_data;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void					CCollObj::SetChecksum(uint32 checksum)
{
	m_checksum = checksum;
	if (mp_coll_tri_data)
	{
		mp_coll_tri_data->SetChecksum(checksum);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CCollObj::SetMovingObject(Obj::CCompositeObject *p_movable_object)
{
	Dbg_MsgAssert(0, ("Can't call SetMovingObject() on a non-moving collision type"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CCompositeObject * CCollObj::GetMovingObject() const
{
	return NULL;
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

#define PRINT_RECT_COLLISION_DEBUG
const float CCollObj::sLINE_BOX_EXTENT = 0.5f;	// Extent of box around collision line

bool					CCollObj::sFindRectangleStaticCollision ( Mth::Rectangle& rect, S2DCollData* p_2D_coll_data, CCollCache *p_cache )
{
#if PRINT_CACHE_HITS
	static uint s_rect_cache_hits = 0, s_num_rect_collisions = 0;
#endif

	Dbg_Assert(p_2D_coll_data);
	
	Mth::CBBox rect_bbox(rect.m_corner);
	rect_bbox.AddPoint(rect.m_corner + rect.m_first_edge);
	rect_bbox.AddPoint(rect.m_corner + rect.m_second_edge);
	rect_bbox.AddPoint(rect.m_corner + rect.m_first_edge + rect.m_second_edge);
	
	bool use_cache = p_cache;
	if (use_cache)
	{
		use_cache = p_cache->Contains(rect_bbox);
#if PRINT_CACHE_HITS
		if (use_cache)
		{
			s_rect_cache_hits++;
		}
		if (++s_num_rect_collisions >= 500)
		{
			Dbg_Message("Static Rect Cache Hits %d/500", s_rect_cache_hits);
			s_rect_cache_hits = s_num_rect_collisions = 0;
		}
#endif
	}
	
	CCollStatic** p_coll_obj_list = NULL;
	if (!use_cache)
	{
		SSec::Manager* ss_man = Nx::CEngine::sGetNearestSuperSectorManager(rect.m_corner);
		if (!ss_man) return false;
		
		// get a list of sectors within the super sectors cut by the rectangle
		p_coll_obj_list = ss_man->GetIntersectingCollSectors(rect_bbox);
	}
	
	// bloat the rectangle bounding box for comparison to sector bounding boxes
	Mth::Vector extension(sLINE_BOX_EXTENT, sLINE_BOX_EXTENT, sLINE_BOX_EXTENT);
	rect_bbox.Set(rect_bbox.GetMin() - extension, rect_bbox.GetMax() + extension);
	
	p_2D_coll_data->num_surfaces = 0;
	
	if (use_cache)
	{
		int num_collisions = p_cache->GetNumStaticCollisions();
		const SCollCacheNode* p_coll_node = p_cache->GetStaticCollisionArray();
		
		// loop over sectors in the cache
		for (int i = num_collisions; i--; )
		{
			// check to see if the sector's bounding box intersects with the rectangle's bounding box
			if (rect_bbox.Intersect(*(p_coll_node->mp_bbox)))
			{
				// check the sector's trangle's for collision with the rectangle
				p_coll_node->mp_coll_obj->CollisionWithRectangle(rect, rect_bbox, p_2D_coll_data);
			}

			p_coll_node++;
		} // END loop over sectors in the cache
	}
	else
	{
		// loop over potentially collided with sectors
		CCollStatic* p_coll_obj;
		while ((p_coll_obj = *p_coll_obj_list))
		{
			Dbg_Assert(p_coll_obj->GetGeometry());
			
			// check to see if the sector's bounding box intersects with the rectangle's bounding box
			if (p_coll_obj->WithinBBox(rect_bbox))
			{
				// check the sector's trangle's for collision with the rectangle
				p_coll_obj->CollisionWithRectangle(rect, rect_bbox, p_2D_coll_data);
			}
			
			p_coll_obj_list++;
		} // END loop over potentially collided with sectors
	}
	
	return p_2D_coll_data->num_surfaces > 0;
}

bool GPrintCollisionData = false;

bool					CCollObj::sFindNearestStaticCollision( Mth::Line &is, CollData *p_data, void *p_callback, CCollCache *p_cache)
{
#if PRINT_CACHE_HITS
	static uint s_cache_hits = 0, s_num_collisions = 0;
#endif

    Dbg_Assert(p_data);
    Dbg_Assert(p_data->coll_found == false);

	p_data->callback = (void (*)( CollData*)) p_callback;

	// Make initial line bounding box
	Mth::CBBox line_bbox(is.m_start);
	line_bbox.AddPoint(is.m_end);

	if (GPrintCollisionData)
	{
		Dbg_Message("Inited line BBox (%f, %f, %f), (%f, %f, %f)",
					line_bbox.GetMin()[X], line_bbox.GetMin()[Y], line_bbox.GetMin()[Z],
					line_bbox.GetMax()[X], line_bbox.GetMax()[Y], line_bbox.GetMax()[Z]);
	}

	// Check to see if we can use the cache
	bool use_cache = p_cache != NULL;
	if (use_cache)
	{
		use_cache = p_cache->Contains(line_bbox);
		
		if (CCollCacheManager::sGetAssertOnCacheMiss())
		{
			Dbg_MsgAssert(use_cache, ("Collision cache miss"));
		}
#if PRINT_CACHE_HITS
		if (use_cache) s_cache_hits++;
		if (++s_num_collisions >= 5000)
		{
			Dbg_Message("Static Cache Hits %d/5000", s_cache_hits);
			s_cache_hits = s_num_collisions = 0;
		}
#endif
	}

	// initialize starting "max" distance to be max collision distance
	CCollStatic** p_coll_obj_list = NULL;
	if (!use_cache)
	{
		SSec::Manager *ss_man;

		ss_man = Nx::CEngine::sGetNearestSuperSectorManager(is);
		if (!ss_man)
			return NULL;

		p_coll_obj_list = ss_man->GetIntersectingCollSectors( is );
	}
	
	// For a line, the dist is returned as a fraction of the line length
	// so we set the dist field to just over 1.0, any found collision
	// will be less than this
	if (s_check_for_far)
	{
		p_data->dist = -0.01f; 		// but if checking in the other direction, we need to to reverse the check
	}
	else
	{
		p_data->dist = 1.01f;
	}

	// Bloat the collision line only for the purposes of deciding against which world 
	// sectors to test the actual collision line
	Mth::Vector extend;

	extend = line_bbox.GetMax();
	extend[X] += sLINE_BOX_EXTENT;
	extend[Y] += sLINE_BOX_EXTENT;
	extend[Z] += sLINE_BOX_EXTENT;
	line_bbox.AddPoint(extend);

	extend = line_bbox.GetMin();
	extend[X] -= sLINE_BOX_EXTENT;
	extend[Y] -= sLINE_BOX_EXTENT;
	extend[Z] -= sLINE_BOX_EXTENT;
	line_bbox.AddPoint(extend);

	// Calculate ray
	Mth::Vector line_dir(is.m_end - is.m_start);

	if (GPrintCollisionData)
	{
		Dbg_Message("Starting collision check with line BBox (%f, %f, %f), (%f, %f, %f)",
					line_bbox.GetMin()[X], line_bbox.GetMin()[Y], line_bbox.GetMin()[Z],
					line_bbox.GetMax()[X], line_bbox.GetMax()[Y], line_bbox.GetMax()[Z]);
		Dbg_Message("Line start (%f, %f, %f)", is.m_start[X], is.m_start[Y], is.m_start[Z]);
		Dbg_Message("Line end (%f, %f, %f)", is.m_end[X], is.m_end[Y], is.m_end[Z]);
	}

	bool new_coll_found = false;

	if (use_cache)
	{
		int num_collisions = p_cache->GetNumStaticCollisions();
		const SCollCacheNode *p_coll_node = p_cache->GetStaticCollisionArray();
		for (int i = 0; i < num_collisions; i++)
		{
			if(line_bbox.Intersect(*(p_coll_node->mp_bbox)))
			{
				/* It's a world sector, call the callback */
				if (p_coll_node->mp_coll_obj->CollisionWithLine( is, line_dir, p_data, &line_bbox ))
				{
					new_coll_found = true;
				}
			}
			p_coll_node++;
		}
	} else {
		CCollStatic* p_coll_obj;

	#ifdef BATCH_TRI_COLLISION
		// Init batch manager
		bool do_batch;
		do_batch = CBatchTriCollMan::sInit(p_data);
	#endif

		/* Start at the top */
		while((p_coll_obj = *p_coll_obj_list))
		{
			// TODO: Come up with a cleaner BBox check
			Dbg_Assert(p_coll_obj->GetGeometry());
			//if(line_bbox.Intersect(p_coll_obj->GetGeometry()->GetBBox()))
			if (p_coll_obj->WithinBBox(line_bbox))
			{
				// this line will display the bounding box of the potential collidable object
				// useful to make sure we are only colliding with the minimum necessary objects
				// p_coll_obj->mp_coll_tri_data->GetBBox().DebugRender(0xfffff,1);

				/* It's a world sector, call the callback */
				if (p_coll_obj->CollisionWithLine( is, line_dir, p_data, &line_bbox ))
				{
					new_coll_found = true;
				}
			}
			p_coll_obj_list++;
		}

	#ifdef BATCH_TRI_COLLISION
		// Wait for tri collision to finish first
		if (do_batch)
		{
			new_coll_found = CBatchTriCollMan::sWaitTriCollisions();
		}
		CBatchTriCollMan::sFinish();				// This should really be in the CFeeler code
	#endif
	}

    /* All done */
	if (p_data->coll_found)
	{
		if ((p_data->dist > 1.0f) || (p_data->dist < 0.0f))
		{
			Dbg_Message("Static Collision distance too big: %f", p_data->dist);
		}
	}

	// return true if we found something			
	return( new_coll_found );
}


bool					CCollObj::sFindFarStaticCollision( Mth::Line &is, CollData *p_data, void *p_callback, CCollCache *p_cache)
{
	s_check_for_far = true;
	bool result = sFindNearestStaticCollision(is,p_data,p_callback,p_cache);
	s_check_for_far = false;
	return result;		
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Note, p_data is assumed to be already initialized
// either it is set to default values, or we have
// come here after using it to check for static collision
// and the results of that are in there
// - see below, we don't set "dist" if we've previously found a collision
// (stops you snapping through walls onto cars)
				  
Obj::CCompositeObject *	CCollObj::sFindNearestMovableCollision( Mth::Line &is, CollData *p_data, void *p_callback, CCollCache *p_cache)
{
#if PRINT_CACHE_HITS
	static uint s_cache_hits = 0, s_num_collisions = 0;
#endif

    Dbg_Assert(p_data);

	Obj::CCompositeObject *p_collision_obj = NULL;

	p_data->callback = (void (*)( CollData*)) p_callback;

	
	// initialize starting "max" distance to be max collision distance
	// actually this	
	if (!p_data->coll_found)
	{
		// For a line, the dist is returned as a fraction of the line length
		// so we set the dist field to just over 1.0, any found collision
		// will be less than this
		if (s_check_for_far)
		{
			p_data->dist = -0.01f; 		// but if checking in the other direction, we need to to reverse the check
		}
		else
		{
			p_data->dist = 1.01f;
		}
	}

	// Make initial line bounding box
	Mth::CBBox line_bbox(is.m_start);
	line_bbox.AddPoint(is.m_end);

	// Check to see if we can use the cache
	bool use_cache = p_cache != NULL;
	if (use_cache)
	{
		use_cache = p_cache->Contains(line_bbox);
#if PRINT_CACHE_HITS
		if (use_cache) s_cache_hits++;
		if (++s_num_collisions >= 5000)
		{
			Dbg_Message("Movable Cache Hits %d/5000", s_cache_hits);
			s_cache_hits = s_num_collisions = 0;
		}
#endif
	}

	if (0 && GPrintCollisionData)
	{
		Dbg_Message("Inited line BBox (%f, %f, %f), (%f, %f, %f)",
					line_bbox.GetMin()[X], line_bbox.GetMin()[Y], line_bbox.GetMin()[Z],
					line_bbox.GetMax()[X], line_bbox.GetMax()[Y], line_bbox.GetMax()[Z]);
	}

	// Bloat the collision line only for the purposes of deciding against which world 
	// sectors to test the actual collision line
	Mth::Vector extend;

	extend = line_bbox.GetMax();
	extend[X] += sLINE_BOX_EXTENT;
	extend[Y] += sLINE_BOX_EXTENT;
	extend[Z] += sLINE_BOX_EXTENT;
	line_bbox.AddPoint(extend);

	extend = line_bbox.GetMin();
	extend[X] -= sLINE_BOX_EXTENT;
	extend[Y] -= sLINE_BOX_EXTENT;
	extend[Z] -= sLINE_BOX_EXTENT;
	line_bbox.AddPoint(extend);

	// Calculate ray
	Mth::Vector line_dir(is.m_end - is.m_start);

	if (0 && GPrintCollisionData)
	{
		Dbg_Message("Starting collision check with line BBox (%f, %f, %f), (%f, %f, %f)",
					line_bbox.GetMin()[X], line_bbox.GetMin()[Y], line_bbox.GetMin()[Z],
					line_bbox.GetMax()[X], line_bbox.GetMax()[Y], line_bbox.GetMax()[Z]);
		Dbg_Message("Line start (%f, %f, %f)", is.m_start[X], is.m_start[Y], is.m_start[Z]);
		Dbg_Message("Line end (%f, %f, %f)", is.m_end[X], is.m_end[Y], is.m_end[Z]);
	}

//	Dbg_Message("Size of movable list %d", Nx::CEngine::sGetMovableObjects().CountItems());
//	Dbg_Message("Size of movable collision list %d", CMovableCollMan::sGetCollisionList()->CountItems());
//	Dbg_Assert(0);

	if (use_cache)
	{
		int num_collisions = p_cache->GetNumMovableCollisions();
		const SCollCacheNode *p_coll_node = p_cache->GetMovableCollisionArray();
		for (int i = 0; i < num_collisions; i++)
		{
			if(p_coll_node->mp_bbox && line_bbox.Intersect(*(p_coll_node->mp_bbox)))
			{
				// Set the callback_object, so any callback will know
				// what object we were colliding with
				p_data->mp_callback_object = p_coll_node->mp_coll_obj->GetMovingObject();

				/* It's a world sector, call the callback */
				if (p_coll_node->mp_coll_obj->CollisionWithLine( is, line_dir, p_data, &line_bbox ))
				{
					p_collision_obj = p_coll_node->mp_coll_obj->GetMovingObject();
					Dbg_Assert(p_collision_obj);
				}

				// the callback object is only valid for the duration of a possible callback											 
				p_data->mp_callback_object = NULL;
			}
			p_coll_node++;
		}
	} else {
	#if 0 //def BATCH_TRI_COLLISION
		// Init batch manager
		bool do_batch;
		do_batch = CBatchTriCollMan::sInit(p_data);
	#endif


		Lst::Node< CCollObj > *p_movable_node = CMovableCollMan::sGetCollisionList()->GetNext();
		//CCollObj **p_movable_node = CMovableCollMan::sGetCollisionArray();

		while(p_movable_node)
		{
			CCollObj *p_coll_obj = p_movable_node->GetData();
			//CCollObj *p_coll_obj = *p_movable_node;

			if (p_coll_obj && !(p_coll_obj->m_Flags & (mSD_NON_COLLIDABLE | mSD_KILLED )))
			{
		
				/* It's a collision object, call the callback */
				//if (p_coll_obj->CollisionWithLine( is, line_dir, p_data , &line_bbox))
				if (p_coll_obj->WithinBBox(line_bbox))
				{
					// Set the callback_object, so any callback will know
					// what object we were colliding with
					p_data->mp_callback_object = p_coll_obj->GetMovingObject();
				
					if (p_coll_obj->CollisionWithLine( is, line_dir, p_data, &line_bbox ))
					{
						p_collision_obj = p_coll_obj->GetMovingObject();
						Dbg_Assert(p_collision_obj);
					}
					// the callback object is only valid for the duration of a possible callback											 
					p_data->mp_callback_object = NULL;
				}
			
			}

			p_movable_node = p_movable_node->GetNext();
			//p_movable_node++;
		}

	#if 0 //def BATCH_TRI_COLLISION
		// Wait for tri collision to finish first
		if (do_batch)
		{
			new_coll_found = CBatchTriCollMan::sWaitTriCollisions();
		}
		CBatchTriCollMan::sFinish();				// This should really be in the CFeeler code
	#endif
	}

    /* All done */
	if (p_collision_obj)
	{

		if ((p_data->dist > 1.0f) || (p_data->dist < 0.0f))
		{
			Dbg_Message("Movable Collision distance too big: %f", p_data->dist);
		}
	}

	return p_collision_obj;
}

Obj::CCompositeObject *	CCollObj::sFindFarMovableCollision( Mth::Line &is, CollData *p_data, void *p_callback, CCollCache *p_cache)
{
	s_check_for_far = true;
	Obj::CCompositeObject *result = sFindNearestMovableCollision(is,p_data,p_callback,p_cache);
	s_check_for_far = false;
	return result;		
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CollType	CCollObj::sConvertTypeChecksum(uint32 checksum)
{
	CollType c_type;

	switch (checksum)
	{
	case 0x6aadf154: // Geometry
		c_type = vCOLL_TYPE_GEOM;
		break;

	case 0xe460abde: // BoundingBox
	case 0x2ba71070: // Spherical
	case 0xdd42aa66: // Cylindrical
		c_type =  vCOLL_TYPE_BBOX;
		break;

	case 0x806fff30: // None
	default:
		c_type = vCOLL_TYPE_NONE;
		break;
	}

	return c_type;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollObj *	CCollObj::sCreateMovableCollision(CollType type, Nx::CCollObjTriData *p_coll_obj_data, int num_coll_objects, Obj::CCompositeObject *p_object)
{
	CCollObj *p_collision;

	switch (type)
	{
	case vCOLL_TYPE_BBOX:
		{
			CCollMovBBox *p_coll_bbox = new Nx::CCollMovBBox;

			// TODO: determine if we should make more than one collision object
			if (num_coll_objects >= 1) {
				Mth::CBBox bbox/*(Mth::Vector(0.0f, 0.0f, 0.0f))*/;	// Add origin for now

				for (int i = 0; i < num_coll_objects; i++)
				{
					bbox.AddPoint(p_coll_obj_data[i].GetBBox().GetMin());
					bbox.AddPoint(p_coll_obj_data[i].GetBBox().GetMax());
				}

				p_coll_bbox->SetBoundingBox(bbox);
			}

			p_collision = p_coll_bbox;
			break;
		}
	case vCOLL_TYPE_GEOM:
		if (num_coll_objects == 1)				// one node
		{
			p_collision = new CCollMovTri(p_coll_obj_data);
		} else if (num_coll_objects > 1) {		// multi nodes
			CCollMulti *p_coll_multi = new Nx::CCollMulti;

			for (int i = 0; i < num_coll_objects; i++)
			{
				CCollObj *p_coll_tri;

				p_coll_tri = new CCollMovTri(&(p_coll_obj_data[i]));
				p_coll_multi->AddCollision(p_coll_tri);
			}

			p_collision = p_coll_multi;
		} else {
			p_collision = NULL;
		}
		break;

	default:
		p_collision = NULL;						// default to no collision
		break;
	}

	// Add to movable collision manager
	if (p_collision)
	{
		p_collision->SetMovingObject(p_object);
		CMovableCollMan::sAddCollision(p_collision, p_object);
	}

	return p_collision;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollMulti::CCollMulti()
{
	mp_collision_list = new Lst::Head< CCollObj >;
	m_movement_changed = true;
	mp_movable_object = NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollMulti::~CCollMulti()
{
	if (mp_collision_list)
	{
		Lst::Node< CCollObj > *obj_node, *next;

		for(obj_node = mp_collision_list->GetNext(); obj_node; obj_node = next)
		{
			next = obj_node->GetNext();

			delete obj_node->GetData();
			delete obj_node;
		}

		delete mp_collision_list;
	}

	// Take out of caches
	CCollCacheManager::sDeleteCollision(this);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CCollMulti::AddCollision(CCollObj *p_collision)
{
	Lst::Node< CCollObj > *node = new Lst::Node< CCollObj > (p_collision);
	mp_collision_list->AddToTail(node);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CCollMulti::SetWorldPosition(const Mth::Vector & pos)
{
	Lst::Node< CCollObj > *obj_node;

	for(obj_node = mp_collision_list->GetNext(); obj_node; obj_node = obj_node->GetNext())
	{
		obj_node->GetData()->SetWorldPosition(pos);
	}

	m_movement_changed = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CCollMulti::SetOrientation(const Mth::Matrix & orient)
{
	Lst::Node< CCollObj > *obj_node;

	for(obj_node = mp_collision_list->GetNext(); obj_node; obj_node = obj_node->GetNext())
	{
		obj_node->GetData()->SetOrientation(orient);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CCollMulti::SetGeometry(CCollObjTriData *p_geom_data)
{
	Lst::Node< CCollObj > *obj_node;

	for(obj_node = mp_collision_list->GetNext(); obj_node; obj_node = obj_node->GetNext())
	{
		obj_node->GetData()->SetGeometry(p_geom_data);
	}

	m_movement_changed = true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CCollMulti::SetMovingObject(Obj::CCompositeObject *p_movable_object)
{
	mp_movable_object = p_movable_object;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CCompositeObject * CCollMulti::GetMovingObject() const
{
	return mp_movable_object;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollObj * 	CCollMulti::Clone(bool instance)
{
	Dbg_MsgAssert(!instance, ("CCollMulti::Clone() with instances not implemented yet"));
	Dbg_Assert(mp_coll_tri_data == NULL);

	CCollMulti *m_new_coll = new CCollMulti(*this);

	m_new_coll->m_Flags |=  mSD_CLONE;

	Lst::Node< CCollObj > *obj_node;

	// Clone List
	m_new_coll->mp_collision_list = new Lst::Head< CCollObj >;
	for(obj_node = mp_collision_list->GetNext(); obj_node; obj_node = obj_node->GetNext())
	{
		Lst::Node< CCollObj > *node = new Lst::Node< CCollObj > (obj_node->GetData()->Clone(instance));
		m_new_coll->mp_collision_list->AddToTail(node);
	}

	return m_new_coll;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CCollMulti::IsTriangleCollision() const
{
	Lst::Node< CCollObj > *obj_node;
	bool ret_val = true;

	for(obj_node = mp_collision_list->GetNext(); obj_node; obj_node = obj_node->GetNext())
	{
		ret_val = ret_val && obj_node->GetData()->IsTriangleCollision();
	}

	return ret_val;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CCollMulti::update_world_bbox()
{
	Lst::Node< CCollObj > *obj_node;

	m_world_bbox_valid = true;
	m_world_bbox.Reset();
	for(obj_node = mp_collision_list->GetNext(); obj_node; obj_node = obj_node->GetNext())
	{
		Mth::CBBox *obj_bbox;
		if ((obj_bbox = obj_node->GetData()->get_bbox()))
		{
			m_world_bbox.AddPoint(obj_bbox->GetMin());
			m_world_bbox.AddPoint(obj_bbox->GetMax());
		} else {
			m_world_bbox_valid = false;
			break;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::CBBox *CCollMulti::get_bbox()
{
	if (m_movement_changed)
	{
		update_world_bbox();
		m_movement_changed = false;
	}

	if (m_world_bbox_valid)
	{
		return &m_world_bbox;
	} else {
		return NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CCollMulti::WithinBBox(const Mth::CBBox & testBBox)
{
	if (m_movement_changed)
	{
		update_world_bbox();
		m_movement_changed = false;
	}

	return !m_world_bbox_valid || testBBox.Intersect(m_world_bbox);
	//return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CCollMulti::CollisionWithLine(const Mth::Line & testLine, const Mth::Vector & lineDir, CollData *p_data, Mth::CBBox * p_bbox)
{
	Lst::Node< CCollObj > *obj_node;
	bool ret_val = false;

	for(obj_node = mp_collision_list->GetNext(); obj_node; obj_node = obj_node->GetNext())
	{
		ret_val = obj_node->GetData()->CollisionWithLine(testLine, lineDir, p_data, p_bbox) || ret_val;
	}

	return ret_val;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool		CCollMulti::CollisionWithRectangle(const Mth::Rectangle& rect, const Mth::CBBox& rect_bbox, S2DCollData* p_data)
{
	Dbg_MsgAssert(false, ("CCollMulti::CollisionWithRecangle is not implemented"));
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CCollMulti::DebugRender(uint32 ignore_1, uint32 ignore_0)
{
	Lst::Node< CCollObj > *obj_node;

	for(obj_node = mp_collision_list->GetNext(); obj_node; obj_node = obj_node->GetNext())
	{
		obj_node->GetData()->DebugRender(ignore_1, ignore_0);
	}
}

// Don't call these!

const Mth::Vector &	CCollMulti::GetWorldPosition() const
{
	Dbg_MsgAssert(0, ("Don't call CCollMulti::GetWorldPosition()"));
	return mp_collision_list->GetNext()->GetData()->GetWorldPosition();
}

const Mth::Matrix &	CCollMulti::GetOrientation() const
{
	Dbg_MsgAssert(0, ("Don't call CCollMulti::GetOrientation()"));
	return mp_collision_list->GetNext()->GetData()->GetOrientation();
}

uint32 		CCollMulti::GetFaceFlags(int face_idx) const
{
	Dbg_MsgAssert(0, ("Don't call CCollMulti::GetFaceFlags()"));
	return 0;
}

uint16 		CCollMulti::GetFaceTerrainType(int face_idx) const
{
	Dbg_MsgAssert(0, ("Don't call CCollMulti::GetFaceTerrainType()"));
	return 0;
}

Mth::Vector	CCollMulti::GetVertexPos(int vert_idx) const
{
	Dbg_MsgAssert(0, ("Don't call CCollMulti::GetVertexPos()"));
	return Mth::Vector(0, 0, 0, 0);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollMovable::CCollMovable() : 
	m_world_pos(0, 0, 0, 1),
	m_orient(0, 0, 0),
	m_orient_transpose(0, 0, 0),
	mp_movable_object(NULL)
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollMovable::~CCollMovable()
{
	// Take out of caches
	CCollCacheManager::sDeleteMovableCollision(this);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector	CCollMovable::GetVertexPos(int vert_idx) const
{
	Dbg_Assert(mp_coll_tri_data);

	Mth::Vector pos(mp_coll_tri_data->GetRawVertexPos(vert_idx));

	pos[W] = 0.0f;				// start as vector
	pos.Rotate(m_orient);
	pos += m_world_pos;

	return pos;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CCollMovable::SetMovingObject(Obj::CCompositeObject *p_movable_object)
{
	mp_movable_object = p_movable_object;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Obj::CCompositeObject * CCollMovable::GetMovingObject() const
{
	return mp_movable_object;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CCollMovable::convert_line_to_local(const Mth::Line &world_line, Mth::Line &local_line)
{
	local_line = world_line;

	// First, translate back
	local_line -= m_world_pos;

	// And then do a reverse rotation
	local_line.m_start.Rotate(m_orient_transpose);
	local_line.m_end.Rotate(m_orient_transpose);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollStatic::CCollStatic() //: 
//	m_world_pos(0, 0, 0, 1)//,
//	m_orient(0, 0, 0)
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollStatic::~CCollStatic()
{
	// Take out of caches
	//CCollCacheManager::sDeleteStaticCollision(this);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Mth::Vector	CCollStatic::GetVertexPos(int vert_idx) const
{
	Dbg_Assert(mp_coll_tri_data);

	Mth::Vector pos(mp_coll_tri_data->GetRawVertexPos(vert_idx));

	pos[W] = 1.0f;				// make a point

	return pos;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollMovBBox::CCollMovBBox() :
	m_flags(0),
	m_terrain_type(0)
{
	// Sets a default bounding box, if nothing loaded
	//m_bbox.Set(Mth::Vector(-60, -20, -60, 1), Mth::Vector(60, 36, 60, 1));
	m_bbox.Set(Mth::Vector(-50, -20, -90, 1), Mth::Vector(30, 36, 90, 1));		// size of car
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollMovBBox::CCollMovBBox(CCollObjTriData *p_geom_data) :
	m_flags(0),
	m_terrain_type(0)
{
	SetGeometry(p_geom_data);
	Dbg_Assert(mp_coll_tri_data);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollMovBBox::~CCollMovBBox()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CCollMovBBox::SetGeometry(CCollObjTriData *p_geom_data)
{
	CCollObj::SetGeometry(p_geom_data);

	Dbg_Assert(mp_coll_tri_data);
	SetBoundingBox(mp_coll_tri_data->GetBBox());
}

// Temp functions
uint32 CCollMovBBox::GetFaceFlags(int face_idx) const
{
	return m_flags;
}

uint16 CCollMovBBox::GetFaceTerrainType(int face_idx) const
{
	return m_terrain_type;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollObj * 	CCollMovBBox::Clone(bool instance)
{
	Dbg_MsgAssert(!instance, ("CCollMovBBox::Clone() with instances not implemented yet"));

	CCollMovBBox *m_new_coll = new CCollMovBBox(*this);

	m_new_coll->m_Flags |=  mSD_CLONE;

	m_new_coll->mp_coll_tri_data = NULL;		// We don't need to clone the data since we have the bbox

	return m_new_coll;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CCollMovBBox::CollisionWithLine(const Mth::Line & testLine, const Mth::Vector & lineDir, CollData *p_data, Mth::CBBox *p_bbox)
{
	Dbg_Assert(p_data);

	// Find line in local space
	Mth::Line local_line;
	convert_line_to_local(testLine, local_line);


	if (0 && GPrintCollisionData)
	{
		Dbg_Message("Testing ray with start (%f, %f, %f)", testLine.m_start[X], testLine.m_start[Y], testLine.m_start[Z]);
		Dbg_Message("and end (%f, %f, %f)", testLine.m_end[X], testLine.m_end[Y], testLine.m_end[Z]);
		Dbg_Message("with the direction (%f, %f, %f)", lineDir[X], lineDir[Y], lineDir[Z]);
	}


	// Mick: temp optimization, generate a bounding box for the line
	// so it can be tested against the collision bounding box
	Mth::CBBox line_bbox(local_line.m_start);
	line_bbox.AddPoint(local_line.m_end);


	// Do collision
	if (m_bbox.Intersect(line_bbox))
	{
		Mth::Vector local_point, local_normal;

		if (m_bbox.LineIntersect(local_line, local_point, local_normal))
		{
			float distance = Mth::Vector(local_point - local_line.m_start).Length() / 			// gotta be a faster way
							 Mth::Vector(local_line.m_end - local_line.m_start).Length();

			SCollSurface collisionSurface;

			if (GPrintCollisionData)
			{
				Dbg_Message("\n*Start point (%f, %f, %f)", testLine.m_start[X], testLine.m_start[Y], testLine.m_start[Z]);
				Dbg_Message("*End point (%f, %f, %f)", testLine.m_end[X], testLine.m_end[Y], testLine.m_end[Z]);
				Dbg_Message("*Start local point (%f, %f, %f)", local_line.m_start[X], local_line.m_start[Y], local_line.m_start[Z]);
				Dbg_Message("*End local point (%f, %f, %f)", local_line.m_end[X], local_line.m_end[Y], local_line.m_end[Z]);
				Dbg_Message("*World pos (%f, %f, %f)", m_world_pos[X], m_world_pos[Y], m_world_pos[Z]);
				Dbg_Message("*Local collision pos (%f, %f, %f)", local_point[X], local_point[Y], local_point[Z]);
				Dbg_Message("*Local Normal (%f, %f, %f)", local_normal[X], local_normal[Y], local_normal[Z]);
				Dbg_Message("*Orientation [%f, %f, %f]", m_orient[X][X], m_orient[X][Y], m_orient[X][Z]);
				Dbg_Message("*            [%f, %f, %f]", m_orient[Y][X], m_orient[Y][Y], m_orient[Y][Z]);
				Dbg_Message("*            [%f, %f, %f]", m_orient[Z][X], m_orient[Z][Y], m_orient[Z][Z]);
			}

			// Convert to world coordinates
			collisionSurface.point = local_point.Rotate(m_orient);
			collisionSurface.point += m_world_pos;
			collisionSurface.normal = local_normal.Rotate(m_orient);

			if (GPrintCollisionData)
			{
				Dbg_Message("*Collision pos (%f, %f, %f)", collisionSurface.point[X], collisionSurface.point[Y], collisionSurface.point[Z]);
				Dbg_Message("*Normal (%f, %f, %f)", collisionSurface.normal[X], collisionSurface.normal[Y], collisionSurface.normal[Z]);
				Dbg_Message("*distance %f", distance);
			}

			return s_found_collision(&testLine, this, &collisionSurface, distance, p_data);
		}
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CCollMovBBox::CollisionWithRectangle(const Mth::Rectangle& rect, const Mth::CBBox& rect_bbox, S2DCollData* p_data)

{
	Dbg_MsgAssert(false, ("CCollMovBBox::CollisionWithRecangle is not implemented"));
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollMovBBox::DebugRender(uint32 ignore_1, uint32 ignore_0)
{
	// Debug box
	Mth::Matrix total_mat = m_orient;
	total_mat[W] = m_world_pos;
	m_bbox.DebugRender(total_mat, 0x80808080, 1);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollStaticTri::CCollStaticTri()
{
	// This instance can't be used until someone sets the geometry data
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollStaticTri::~CCollStaticTri()
{
	// Don't delete the Tri Data
}

// Temp functions
uint32 CCollStaticTri::GetFaceFlags(int face_idx) const
{
	Dbg_Assert(mp_coll_tri_data);
	return mp_coll_tri_data->GetFaceFlags(face_idx);
}

uint16 CCollStaticTri::GetFaceTerrainType(int face_idx) const
{
	Dbg_Assert(mp_coll_tri_data);
	return mp_coll_tri_data->GetFaceTerrainType(face_idx);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollStaticTri::SetWorldPosition(const Mth::Vector & pos)
{
	Dbg_Assert(mp_coll_tri_data);
	mp_coll_tri_data->Translate(pos - GetWorldPosition());

	CCollStatic::SetWorldPosition(pos);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void 	CCollStaticTri::SetOrientation(const Mth::Matrix & orient)
{
	CCollStatic::SetOrientation(orient);

	Dbg_MsgAssert(0, ("CCollStaticTri::SetOrientation() not implemented yet"));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollObj * 	CCollStaticTri::Clone(bool instance)
{
	Dbg_MsgAssert(!instance, ("CCollStaticTri::Clone() with instances not implemented yet"));

	// Clone only if there is geometry
	CCollObjTriData *p_new_tri_data = mp_coll_tri_data->Clone(instance, true);
	if (!p_new_tri_data)
	{
		return NULL;
	}

	CCollStaticTri *p_new_coll = new CCollStaticTri(*this);

	p_new_coll->m_Flags |=  mSD_CLONE;

	p_new_coll->mp_coll_tri_data = p_new_tri_data;

	return p_new_coll;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollStaticTri::RotateY(const Mth::Vector & world_origin, Mth::ERot90 rot_y)
{
	Dbg_Assert(mp_coll_tri_data);

	mp_coll_tri_data->RotateY(world_origin, rot_y);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollStaticTri::Scale(const Mth::Vector & world_origin, const Mth::Vector & scale)
{
	Dbg_Assert(mp_coll_tri_data);

	mp_coll_tri_data->Scale(world_origin, scale);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CCollStaticTri::WithinBBox(const Mth::CBBox & testBBox)
{
	Dbg_Assert(mp_coll_tri_data);
	return testBBox.Intersect(mp_coll_tri_data->GetBBox());
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CCollStaticTri::CollisionWithLine(const Mth::Line & testLine, const Mth::Vector & lineDir, CollData *p_data, Mth::CBBox *p_bbox)
{
	Dbg_Assert(p_data);
	Dbg_Assert(mp_coll_tri_data);

	bool best_collision = false;

	// Generate a bounding box for the line
	// so it can be tested against the BSP tree.	  
	// (Should we just pass along the one we had before, even though it won't work with movables?)
//	Mth::CBBox line_bbox(testLine.m_start);
//	line_bbox.AddPoint(testLine.m_end);

	uint num_faces;
	FaceIndex *p_face_indexes;
	p_face_indexes = mp_coll_tri_data->FindIntersectingFaces(*p_bbox, num_faces);
	Dbg_Assert(p_face_indexes);

#ifdef BATCH_TRI_COLLISION
	bool do_batch = !CBatchTriCollMan::sIsNested();
#endif

	for (uint fidx = 0; fidx < num_faces; fidx++, p_face_indexes++)
	{
#ifdef BATCH_TRI_COLLISION
		if (do_batch)
		{
			CBatchTriCollMan::sAddTriCollision(testLine, lineDir, this, *p_face_indexes);
			continue;
		}
#endif // BATCH_TRI_COLLISION

#if defined(FIXED_POINT_VERTICES) || defined(__PLAT_NGC__)
		Mth::Vector v0, v1, v2;
		if (mp_coll_tri_data->m_use_face_small)
		{
			CCollObjTriData::SFaceSmall *face = &(mp_coll_tri_data->mp_face_small[*p_face_indexes]);

			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[0], v0);
			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[1], v1);
			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[2], v2);
		} else {
			CCollObjTriData::SFace *face = &(mp_coll_tri_data->mp_faces[*p_face_indexes]);

			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[0], v0);
			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[1], v1);
			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[2], v2);
		}
#else
		Mth::Vector *v0, *v1, *v2;
		if (mp_coll_tri_data->m_use_face_small)
		{
			CCollObjTriData::SFaceSmall *face = &(mp_coll_tri_data->mp_face_small[*p_face_indexes]);

			v0 = &mp_coll_tri_data->mp_vert_pos[face->m_vertex_index[0]];
			v1 = &mp_coll_tri_data->mp_vert_pos[face->m_vertex_index[1]];
			v2 = &mp_coll_tri_data->mp_vert_pos[face->m_vertex_index[2]];
		} else {
			CCollObjTriData::SFace *face = &(mp_coll_tri_data->mp_faces[*p_face_indexes]);

			v0 = &mp_coll_tri_data->mp_vert_pos[face->m_vertex_index[0]];
			v1 = &mp_coll_tri_data->mp_vert_pos[face->m_vertex_index[1]];
			v2 = &mp_coll_tri_data->mp_vert_pos[face->m_vertex_index[2]];
		}
#endif // FIXED_POINT_VERTICES

#if 0
		uint32 coll_color = (uint32) MAKE_RGB(200, 200, 200);
		if (coll_color != (uint32) MAKE_RGB( 0, 200, 0 ))
		{
			Gfx::AddDebugLine(*v0,*v1,coll_color,1);
			Gfx::AddDebugLine(*v1,*v2,coll_color,1);
			Gfx::AddDebugLine(*v2,*v0,coll_color,1);
		}
#endif

		float distance;
#if defined(FIXED_POINT_VERTICES) || defined(__PLAT_NGC__)
		if (sRayTriangleCollision(&testLine.m_start, &lineDir, &v0, &v1, &v2, &distance))
#else
		if (sRayTriangleCollision(&testLine.m_start, &lineDir, v0, v1, v2, &distance))
#endif
		{
			SCollSurface collisionSurface;

			/* We've got one */
#if defined(FIXED_POINT_VERTICES) || defined(__PLAT_NGC__)
			collisionSurface.point = v0;
			collisionSurface.index = *p_face_indexes;

			// Find normal
			Mth::Vector vTmp1(v1 - v0);
			Mth::Vector vTmp2(v2 - v0);
#else
			collisionSurface.point = (*v0);
			collisionSurface.index = *p_face_indexes;

			// Find normal
			Mth::Vector vTmp1(*v1 - *v0);
			Mth::Vector vTmp2(*v2 - *v0);
#endif // FIXED_POINT_VERTICES
			collisionSurface.normal = Mth::CrossProduct(vTmp1, vTmp2);
			collisionSurface.normal.Normalize();

			if (s_found_collision(&testLine, this, &collisionSurface, distance, p_data))
			{
				best_collision = true;
			}
		}
	}

#ifdef BATCH_TRI_COLLISION
	if (do_batch)
	{
		CBatchTriCollMan::sStartNewTriCollisions();
	}
#endif // BATCH_TRI_COLLISION

	return best_collision;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CCollStaticTri::CollisionWithRectangle(const Mth::Rectangle& testRect, const Mth::CBBox& testRectBBox, S2DCollData *p_coll_data)
{
	Dbg_Assert(p_coll_data);
	
	uint num_faces;
	FaceIndex* p_face_indexes = mp_coll_tri_data->FindIntersectingFaces(testRectBBox, num_faces);
	Dbg_Assert(p_face_indexes);
	
	// loop over the relavent faces of the sector
	for (uint fidx = 0; fidx < num_faces; fidx++, p_face_indexes++)
	{
		uint32 face_flags = GetFaceFlags(fidx);
		if ((face_flags & p_coll_data->ignore_1) || (~face_flags & p_coll_data->ignore_0)) continue;

#if defined(FIXED_POINT_VERTICES) || defined(__PLAT_NGC__)
		Mth::Vector v0, v1, v2;
		if (mp_coll_tri_data->m_use_face_small)
		{
			CCollObjTriData::SFaceSmall *face = &(mp_coll_tri_data->mp_face_small[*p_face_indexes]);

			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[0], v0);
			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[1], v1);
			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[2], v2);
		} else {
			CCollObjTriData::SFace *face = &(mp_coll_tri_data->mp_faces[*p_face_indexes]);

			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[0], v0);
			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[1], v1);
			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[2], v2);
		}
#else
		Mth::Vector v0, v1, v2;
		if (mp_coll_tri_data->m_use_face_small)
		{
			CCollObjTriData::SFaceSmall *face = &(mp_coll_tri_data->mp_face_small[*p_face_indexes]);

			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[0], v0);
			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[1], v1);
			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[2], v2);
		} else {
			CCollObjTriData::SFace *face = &(mp_coll_tri_data->mp_faces[*p_face_indexes]);

			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[0], v0);
			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[1], v1);
			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[2], v2);
		}
		
#endif // FIXED_POINT_VERTICES

		Mth::Vector p_coll_point[2];
		ETriangleEdgeType p_triangle_edge[2];

		if (sRectangleTriangleCollision(&testRect, &v0, &v1, &v2, p_coll_point, p_triangle_edge))
		{
			if (p_coll_data->num_surfaces == MAX_NUM_2D_COLLISIONS_REPORTED)
			{
				Dbg_Message("Number of faces colliding with rectangle exceeds static maximum; dropping additional collisions");
				return true;
			}
			
			// add the face to the collided surface list
			S2DCollSurface& surface = p_coll_data->p_surfaces[p_coll_data->num_surfaces];
			surface.normal = Mth::CrossProduct(v1 - v0, v2 - v0).Normalize();

			// ignore triangles with zero area
			if (surface.normal.LengthSqr() <= 0.0f)	continue;
			
			surface.trigger = (face_flags & mFD_TRIGGER);
			surface.node_name = this->GetChecksum();
			
			surface.ends[0].point = p_coll_point[0];
			surface.ends[1].point = p_coll_point[1];

			for (int end_idx = 2; end_idx--; )
			{
				surface.ends[end_idx].tangent_exists = p_triangle_edge[end_idx] != NO_TRIANGLE_EDGE;
				switch (p_triangle_edge[end_idx])
				{
					case TRIANGLE_EDGE_V0_V1:
						surface.ends[end_idx].tangent = (v1 - v0).Normalize();
						break;
					case TRIANGLE_EDGE_V1_V2:
						surface.ends[end_idx].tangent = (v2 - v1).Normalize();
						break;
					case TRIANGLE_EDGE_V2_V0:
						surface.ends[end_idx].tangent = (v0 - v2).Normalize();
						break;
					case NO_TRIANGLE_EDGE:
						break;
				}
			}
			
			p_coll_data->num_surfaces++;
		}
	}
	
	return p_coll_data->num_surfaces > 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollStaticTri::DebugRender(uint32 ignore_1, uint32 ignore_0)
{
	Dbg_Assert(mp_coll_tri_data);
	mp_coll_tri_data->DebugRender(ignore_1, ignore_0);
}

void	CCollStaticTri::ProcessOcclusion()
{
	Dbg_Assert(mp_coll_tri_data);
	mp_coll_tri_data->ProcessOcclusion();
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollStaticTri::CheckForHoles()
{
	Dbg_Assert(mp_coll_tri_data);
#ifdef	__DEBUG_CODE__
	mp_coll_tri_data->CheckForHoles();
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollMovTri::CCollMovTri()
{
	// This instance can't be used until someone sets the geometry data
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollMovTri::CCollMovTri(CCollObjTriData *p_coll_data) :
	CCollMovable()
{
	SetGeometry(p_coll_data);
	Dbg_Assert(mp_coll_tri_data);

	//m_movement_changed = false;
	m_movement_changed = true;		// to update m_world_bbox
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollMovTri::~CCollMovTri()
{
	// Don't delete the Tri Data
}

// Temp functions
uint32 CCollMovTri::GetFaceFlags(int face_idx) const
{
	Dbg_Assert(mp_coll_tri_data);
	return mp_coll_tri_data->GetFaceFlags(face_idx);
}

uint16 CCollMovTri::GetFaceTerrainType(int face_idx) const
{
	Dbg_Assert(mp_coll_tri_data);
	return mp_coll_tri_data->GetFaceTerrainType(face_idx);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		CCollMovTri::SetGeometry(CCollObjTriData *p_geom_data)
{
	CCollObj::SetGeometry(p_geom_data);
	Dbg_Assert(mp_coll_tri_data);
	
	// Re-calculate radius
	float new_length;
	m_max_radius = 0.0f;
	for (int i = 0; i < mp_coll_tri_data->m_num_verts; i++)
	{
		new_length = mp_coll_tri_data->GetRawVertexPos(i).Length();
		if (new_length > m_max_radius)
		{
			m_max_radius = new_length;
		}
	}

	m_movement_changed = true;		// to update m_world_bbox
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CCollObj * 	CCollMovTri::Clone(bool instance)
{
	Dbg_MsgAssert(!instance, ("CCollMovTri::Clone() with instances not implemented yet"));

	CCollMovTri *m_new_coll = new CCollMovTri(*this);

	m_new_coll->m_Flags |=  mSD_CLONE;

	m_new_coll->mp_coll_tri_data = mp_coll_tri_data->Clone(instance);

	return m_new_coll;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CCollMovTri::WithinBBox(const Mth::CBBox & testBBox)
{
	Dbg_Assert(mp_coll_tri_data);

	if (m_movement_changed)
	{
		update_world_bbox();
		m_movement_changed = false;
	}

	return testBBox.Intersect(m_world_bbox);
	//return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CCollMovTri::CollisionWithLine(const Mth::Line & testLine, const Mth::Vector & lineDir, CollData *p_data, Mth::CBBox *p_bbox)
{
	Dbg_Assert(p_data);
	Dbg_Assert(mp_coll_tri_data);

	bool best_collision = false;

	// Find line in local space
	Mth::Line local_line;
	convert_line_to_local(testLine, local_line);
	Mth::Vector local_line_dir(local_line.m_end - local_line.m_start);

	// Generate a bounding box for the line
	// so it can be tested against the BSP tree
	Mth::CBBox line_bbox(local_line.m_start);
	line_bbox.AddPoint(local_line.m_end);

	uint num_faces;
	FaceIndex *p_face_indexes;
	p_face_indexes = mp_coll_tri_data->FindIntersectingFaces(line_bbox, num_faces);

	for (uint fidx = 0; fidx < num_faces; fidx++, p_face_indexes++)
	{
#if defined(FIXED_POINT_VERTICES) || defined(__PLAT_NGC__)
		Mth::Vector v0, v1, v2;
		if (mp_coll_tri_data->m_use_face_small)
		{
			CCollObjTriData::SFaceSmall *face = &(mp_coll_tri_data->mp_face_small[*p_face_indexes]);

			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[0], v0);
			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[1], v1);
			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[2], v2);
		} else {
			CCollObjTriData::SFace *face = &(mp_coll_tri_data->mp_faces[*p_face_indexes]);

			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[0], v0);
			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[1], v1);
			mp_coll_tri_data->GetRawVertexPos(face->m_vertex_index[2], v2);
		}
#else
		Mth::Vector *v0, *v1, *v2;
		if (mp_coll_tri_data->m_use_face_small)
		{
			CCollObjTriData::SFaceSmall *face = &(mp_coll_tri_data->mp_face_small[*p_face_indexes]);

			v0 = &mp_coll_tri_data->mp_vert_pos[face->m_vertex_index[0]];
			v1 = &mp_coll_tri_data->mp_vert_pos[face->m_vertex_index[1]];
			v2 = &mp_coll_tri_data->mp_vert_pos[face->m_vertex_index[2]];
		} else {
			CCollObjTriData::SFace *face = &(mp_coll_tri_data->mp_faces[*p_face_indexes]);

			v0 = &mp_coll_tri_data->mp_vert_pos[face->m_vertex_index[0]];
			v1 = &mp_coll_tri_data->mp_vert_pos[face->m_vertex_index[1]];
			v2 = &mp_coll_tri_data->mp_vert_pos[face->m_vertex_index[2]];
		}
#endif // FIXED_POINT_VERTICES

		float distance;
#if defined(FIXED_POINT_VERTICES) || defined(__PLAT_NGC__)
		if (sRayTriangleCollision(&local_line.m_start, &local_line_dir, &v0, &v1, &v2, &distance))
#else
		if (sRayTriangleCollision(&local_line.m_start, &local_line_dir, v0, v1, v2, &distance))
#endif
		{
			SCollSurface collisionSurface;

			/* We've got one */
#if defined(FIXED_POINT_VERTICES) || defined(__PLAT_NGC__)
			collisionSurface.point = v0;
			collisionSurface.index = *p_face_indexes;

			// Find normal
			Mth::Vector vTmp1(v1 - v0);
			Mth::Vector vTmp2(v2 - v0);
#else
			collisionSurface.point = (*v0);
			collisionSurface.index = *p_face_indexes;

			// Find normal
			Mth::Vector vTmp1(*v1 - *v0);
			Mth::Vector vTmp2(*v2 - *v0);
#endif // FIXED_POINT_VERTICES
			collisionSurface.normal = Mth::CrossProduct(vTmp1, vTmp2);
			collisionSurface.normal.Normalize();

			// Convert to world coordinates
			collisionSurface.point.Rotate(m_orient);
			collisionSurface.point += m_world_pos;
			collisionSurface.normal.Rotate(m_orient);

			if (s_found_collision(&testLine, this, &collisionSurface, distance, p_data))
			{
				best_collision = true;
			}
		}
	}

	return best_collision;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CCollMovTri::CollisionWithRectangle(const Mth::Rectangle& rect, const Mth::CBBox& rect_bbox, S2DCollData* p_data)
{
	Dbg_MsgAssert(false, ("CCollMovTri::CollisionWithRecangle is not implemented"));
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	CCollMovTri::DebugRender(uint32 ignore_1, uint32 ignore_0)
{
	// Debug box
	Mth::Matrix total_mat = m_orient;
	total_mat[W] = m_world_pos;
	mp_coll_tri_data->DebugRender(total_mat, ignore_1, ignore_0);
}

} // namespace Nx

