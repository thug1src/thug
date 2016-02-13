///********************************************************************************
// *																				*
// *	Module:																		*
// *				NsSlerp															*
// *	Description:																*
// *				Slerps matrices.												*
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
//#include "p_slerp.h"
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
///********************************************************************************
// *																				*
// *	Method:																		*
// *				NsSlerp															*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Declares a scene object.										*
// *																				*
// ********************************************************************************/
//
//NsSlerp::NsSlerp()
//{
//}
//
//NsSlerp::NsSlerp( NsMatrix * start, NsMatrix * end )
//{
//	setMatrices( start, end );
//}
//
//void NsSlerp::setMatrices( NsMatrix * start, NsMatrix * end )
//{
//	NsMatrix	inv;
//	NsVector	center;
//
//	m_start.copy( *start );
//	m_end.copy( *end );
//
//	// Calculate the inverse transformation.
//	inv.invert( *start );
//	inv.transform( *end, NsMatrix_Combine_Post );
//
//	// Get the axis and angle.
//	inv.getRotation( &m_axis, &m_angle, &center );
//
//	// If angle is too small, use lerp.
//	m_lerp = ( m_angle < 2.0f );
//}
//	
//void NsSlerp::getMatrix( NsMatrix * result, float delta )
//{
//    /* Cap and floor the delta */
//    /* If we are at one end the solution is easy */
//    if (delta <= 0.0f)
//    {
//        delta = 0.0f;
//		result->copy( m_start );
//    }
//    else if (delta >= 1.0f)
//    {
//        delta = 1.0f;
//		result->copy( m_end );
//    }
//
//    /* Do the lerp if we are, else... */
//    if ( m_lerp )
//    {
//        /* Get the lerp matrix */
//		NsMatrix	lerp;
//		NsVector	lpos;
//		NsVector	spos;
//		NsVector	epos;
//		NsVector	rpos;
//
//		lerp.identity();
//		spos.set( m_start.getPosX(), m_start.getPosY(), m_start.getPosZ() );
//		epos.set( m_end.getPosX(), m_end.getPosY(), m_end.getPosZ() );
//
//		lerp.getRight()->sub( *m_end.getRight(), *m_start.getRight() );
//		lerp.getUp()->sub( *m_end.getUp(), *m_start.getUp() );
//		lerp.getAt()->sub( *m_end.getAt(), *m_start.getAt() );
//		lpos.sub( epos, spos );
//
//        /* Do lerp */
//		lerp.getRight()->scale( delta );
//		lerp.getUp()->scale( delta );
//		lerp.getAt()->scale( delta );
//		lpos.scale( delta );
//
//		result->getRight()->add( *m_start.getRight(), *lerp.getRight() );
//		result->getUp()->add( *m_start.getUp(), *lerp.getUp() );
//		result->getAt()->add( *m_start.getAt(), *lerp.getAt() );
//		rpos.add( spos, lpos );
//
//		result->getRight()->normalize();
//		result->getUp()->normalize();
//		result->getAt()->normalize();
//
//		result->setPosX( rpos.x );
//		result->setPosY( rpos.y );
//		result->setPosZ( rpos.z );
//    }
//    else
//    {
//		NsVector	rpos;
//		NsVector	spos;
//		NsVector	epos;
//
//		spos.set( m_start.getPosX(), m_start.getPosY(), m_start.getPosZ() );
//		epos.set( m_end.getPosX(), m_end.getPosY(), m_end.getPosZ() );
//
//        /* Remove the translation for now */
//		result->copy( m_start );
//		result->setPos( 0.0f, 0.0f, 0.0f );
//
//        /* Rotate the new matrix */
//		result->rotate( &m_axis, m_angle * delta, NsMatrix_Combine_Post );
//
//        /* Do linear interpolation on position */
//		rpos.set( result->getPosX(), result->getPosY(), result->getPosZ() );
//		rpos.sub( epos, spos );
//		rpos.scale( delta );
//		rpos.sub( spos );
//		result->setPos( &rpos );
//    }
////	result->copy( m_end );
//}
