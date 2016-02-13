/********************************************************************************
 *																				*
 *	Module:																		*
 *				NsVector														*
 *	Description:																*
 *				Matrix functionality.											*
 *	Written by:																	*
 *				Paul Robinson													*
 *	Copyright:																	*
 *				2001 Neversoft Entertainment - All rights reserved.				*
 *																				*
 ********************************************************************************/

/********************************************************************************
 * Includes.																	*
 ********************************************************************************/
#include <math.h>
#include <string.h>
#include "p_matrix.h"

/********************************************************************************
 * Defines.																		*
 ********************************************************************************/

/********************************************************************************
 * Structures.																	*
 ********************************************************************************/

/********************************************************************************
 * Local variables.																*
 ********************************************************************************/

/********************************************************************************
 * Forward references.															*
 ********************************************************************************/

/********************************************************************************
 * Externs.																		*
 ********************************************************************************/

NsVector::NsVector()
{
	set( 0.0f, 0.0f, 0.0f );
}

NsVector::NsVector( float sx, float sy, float sz )
{
	set( sx, sy, sz );
}

void NsVector::add ( NsVector& a )
{
	VECAdd( (Vec*)this, (Vec*)&a, (Vec*)this );
}

void NsVector::add ( NsVector& a, NsVector& b )
{
	VECAdd( (Vec*)&a, (Vec*)&b, (Vec*)this );
}

void NsVector::cross ( NsVector& a )
{
	VECCrossProduct( (Vec*)this, (Vec*)&a, (Vec*)this );
}

void NsVector::cross ( NsVector& a, NsVector& b )
{
	VECCrossProduct( (Vec*)&a, (Vec*)&b, (Vec*)this );
}

float NsVector::length ( void )
{
	return VECMag( (Vec*)this );
}

float NsVector::distance ( NsVector& a )
{
	return VECDistance( (Vec*)this, (Vec*)&a );
}

float NsVector::dot ( NsVector& a )
{
	return VECDotProduct( (Vec*)this, (Vec*)&a );
}

void NsVector::normalize ( void )
{
	VECNormalize( (Vec*)this, (Vec*)this );
}

void NsVector::normalize ( NsVector& a )
{
	VECNormalize( (Vec*)&a, (Vec*)this );
}

void NsVector::reflect ( NsVector& normal )
{
	VECReflect( (Vec*)this, (Vec*)&normal, (Vec*)this );
}

void NsVector::reflect ( NsVector& a, NsVector& normal )
{
	VECReflect( (Vec*)&a, (Vec*)&normal, (Vec*)this );
}

void NsVector::scale ( float scale )
{
	VECScale( (Vec*)this, (Vec*)this, scale );
}

void NsVector::scale ( NsVector& a, float scale )
{
	VECScale( (Vec*)&a, (Vec*)this, scale );
}

void NsVector::sub ( NsVector& a )
{
	VECSubtract( (Vec*)this, (Vec*)&a, (Vec*)this );
}

void NsVector::sub ( NsVector& a, NsVector& b )
{
	VECSubtract( (Vec*)&a, (Vec*)&b, (Vec*)this );
}

void NsVector::lerp	( NsVector& a, NsVector& b, float t )
{
	NsVector res;
	res.sub( b, a );
	res.scale( t );
	res.add( a );
	set( res.x, res.y, res.z );
}
