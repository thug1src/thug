///********************************************************************************
// *																				*
// *	Module:																		*
// *				nsTriangle														*
// *	Description:																*
// *				Matrix functionality.											*
// *	Written by:																	*
// *				Paul Robinson													*
// *	Copyright:																	*
// *				2001 Neversoft Entertainment - All rights reserved.				*
// *																				*
// ********************************************************************************/
//
///********************************************************************************
// * Includes.																	*
// ********************************************************************************/
//#include <math.h>
//#include <string.h>
//#include "p_triangle.h"
//
///********************************************************************************
// * Defines.																		*
// ********************************************************************************/
//
///********************************************************************************
// * Structures.																	*
// ********************************************************************************/
//
///********************************************************************************
// * Local variables.																*
// ********************************************************************************/
//
///********************************************************************************
// * Forward references.															*
// ********************************************************************************/
//
///********************************************************************************
// * Externs.																		*
// ********************************************************************************/
//
//NsTriangle::NsTriangle()
//{
//	m_corner[0].set( 0.0f, 0.0f, 0.0f );
//	m_corner[1].set( 0.0f, 0.0f, 0.0f );
//	m_corner[2].set( 0.0f, 0.0f, 0.0f );
//}
//
//NsTriangle::NsTriangle( NsVector * pCorner0, NsVector * pCorner1, NsVector * pCorner2 )
//{
//	set( pCorner0, pCorner1, pCorner2 );
//}
//
//void NsTriangle::set( NsVector * pCorner0, NsVector * pCorner1, NsVector * pCorner2 )
//{
//	m_corner[0].copy( *pCorner0 );
//	m_corner[1].copy( *pCorner1 );
//	m_corner[2].copy( *pCorner2 );
//}
//
//NsTriangle::NsTriangle( float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2 )
//{
//	set( x0, y0, z0, x1, y1, z1, x2, y2, z2 );
//}
//
//void NsTriangle::set( float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2 )
//{
//	m_corner[0].set( x0, y0, z0 );
//	m_corner[1].set( x1, y1, z1 );
//	m_corner[2].set( x2, y2, z2 );
//}
//
//void NsTriangle::translate ( NsVector * pTrans )
//{
//	m_corner[0].add( *pTrans );
//	m_corner[1].add( *pTrans );
//	m_corner[2].add( *pTrans );
//}
//
//int NsLine::intersectTriangle( float * distance, NsTriangle * pTriangle )
//{
//#define INTERSECT_EPSILON (float)(1e-8)
//#define INTERSECT_EDGE (float)(1e-5)
//
//    NsVector	edge1, edge2, tVec, pVec, qVec;
//    float		det;
//	NsVector	linedelta;
//	bool		result;
//
//	// Generate line delta
//	linedelta.sub( end, start );
//
//    /* Find vectors for two edges sharing vert0 */
//	edge1.sub( *pTriangle->corner(1), *pTriangle->corner(0) );
//	edge2.sub( *pTriangle->corner(2), *pTriangle->corner(0) );
//
//    /* Begin calculating determinant
//     * - also used to calculate U parameter */
//	pVec.cross( linedelta, edge2 );
//
//    /* If determinant is
//     * + near zero, ray lies in plane of
//     *   triangle
//     * + negative, triangle is backfacing
//     */
//    det = edge1.dot( pVec );
//    result = (det > INTERSECT_EPSILON);
//
//    if ( result )
//    {
//        float  lo, hi, u;
//
//        /* Calculate bounds for parameters with tolerance */
//        lo = - det*INTERSECT_EDGE;
//        hi = det - lo;
//
//        /* Calculate U parameter and test bounds */
//		tVec.sub( start, *pTriangle->corner(0) );
//	    u = tVec.dot( pVec );
//        result =  (u >= lo && u <= hi);
//
//        if ( result )
//        {
//            float  v;
//
//            /* Calculate V parameter and test bounds */
//			qVec.cross( tVec, edge1 );
//		    v = qVec.dot( linedelta );
//            result = (v >= lo && (u + v) <= hi);
//
//            if ( result )
//            {
//                /* Calculate t,
//                 * and make sure intersection is in bounds of line */
//                *distance = qVec.dot( edge2 );
//
//                /* Within bounds of line? */
//                result = (*distance >= lo && *distance <= hi);
//
//                if ( result )
//                {
//                    *distance = *distance / det;
//                }
//            }
//        }
//    }
//	return result;
//}










