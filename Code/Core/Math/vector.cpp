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
**	Module:			Vector (MTH)	 										**
**																			**
**	File name:		vector.cpp												**
**																			**
**	Created by:		11/24/99	-	Mick									**
**																			**
**	Description:	Math Library code										**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/


#include <core/math.h>
#include <core/defines.h>
//#include <core/math.h>
#include <core/math/math.h>
#include <core/math/vector.h>
#include <core/math/matrix.h>
#include <core/macros.h>

#include <core/math/quat.h>
#include <core/math/vector.inl>


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

Vector&			Vector::RotateY90( ERot90 angle )
{	
	float temp;

	switch (angle)
	{
	case ROT_0:
		break;

	case ROT_90:
		temp = col[X];
		col[X] = col[Z];
		col[Z] = -temp;
		break;

	case ROT_180:
		col[X] = -col[X];
		col[Z] = -col[Z];
		break;

	case ROT_270:
		temp = col[X];
		col[X] = -col[Z];
		col[Z] = temp;
		break;

	default:
		Dbg_MsgAssert(0, ("Vector::RotateY90() out of range: %d", angle));
		break;
	}

	return *this;
}

/*****************************************************************************
// Project the vector onto the plane define by a normal
*****************************************************************************/
								
Vector&			Vector::ProjectToPlane(const Vector& normal)
{

	
	float perp_component = Mth::DotProduct(*this,normal);	
	
	*this -= normal * perp_component;	
	
	return *this;	

}

Vector&			Vector::RotateToPlane(const Vector& normal)
{
	// get the length of the vector
	float length = Length();
	
	// Project the vector onto the plane
	ProjectToPlane(normal);
	
	// now we've projected it ontot he plane
	// we need to handle the case where it dissapears into a point
	// which would indicate that we were perpendicular to the plane
	// so need to get an arbitary vector in the plane	
	float  projected_length = Length();
	if (projected_length == 0.0f) 		// is this a valid comparision?
	{
		// Rotate vector through -90 degrees about Y then +90 about X
		col[X] = -normal[Z];
		col[Y] =  normal[X];
		col[Z] = -normal[Y];	
	}
	
	// get unit vector in this direction
	Normalize();	
	
	// multiply by original speed to rotate velocity onto plane
	*this *= length;						 

	col[W] = 0.0f; 			// clean up W, otherwise multilications will accumelate over time...
	
	return *this;	
}	
	
Vector&			Vector::RotateToNormal(const Vector& normal)
{

	*this  = normal*Length();

	return *this;
}

Vector&			Vector::ProjectToNormal(const Vector& normal)
{

	float dot = Mth::DotProduct(*this,normal); 							   
	*this  = normal * dot;

	return *this;
}

/* Finds the largest contributor to the length of the vector...
   If you pass in whichAxis, it will fill it in with which axis
   is the max...
*/
float	Vector::GetMaxAxis( int *whichAxis )
{
	int which_axis;
	if ( !whichAxis )
	{
		whichAxis = &which_axis;
	}
	
	*whichAxis = X;
	
	if ( Abs( this->col[ Y ] ) > Abs( this->col[ X ] ) )
	{
		if ( Abs( this->col[ Z ] ) > Abs( this->col[ Y ] ) )
			*whichAxis = Z;
		else
			*whichAxis = Y;
	}
	else if ( Abs( this->col[ Z ] ) > Abs( this->col[ X ] ) )
	{
		*whichAxis = Z;
	}
	return ( this->col[ *whichAxis ] );
}

void Vector::DegreesToRadians( void )
{
	this->col[ X ] = DEGREES_TO_RADIANS( this->col[ X ] );
	this->col[ Y ] = DEGREES_TO_RADIANS( this->col[ Y ] );
	this->col[ Z ] = DEGREES_TO_RADIANS( this->col[ Z ] );
}

void Vector::RadiansToDegrees( void )
{
	this->col[ X ] = RADIANS_TO_DEGREES( this->col[ X ] );
	this->col[ Y ] = RADIANS_TO_DEGREES( this->col[ Y ] );
	this->col[ Z ] = RADIANS_TO_DEGREES( this->col[ Z ] );
}

void Vector::FeetToInches( void )
{
	this->col[ X ] = FEET_TO_INCHES( this->col[ X ] );
	this->col[ Y ] = FEET_TO_INCHES( this->col[ Y ] );
	this->col[ Z ] = FEET_TO_INCHES( this->col[ Z ] );
}

// convert to 3d studio max coords ( Z+ up, Y- forward, X to the right... )
void Vector::ConvertToMaxCoords( void )
{
	this->col[ X ] = -this->col[ X ];
	float temp = this->col[ Y ];
	this->col[ Y ] = -this->col[ Z ];
	this->col[ Z ] = temp;
}

// convert to 3d studio max coords ( Z+ up, Y- forward, X to the right... )
void Vector::ConvertFromMaxCoords( void )
{
	this->col[ X ] = -this->col[ X ];
	float temp = this->col[ Y ];
	this->col[ Y ] = this->col[ Z ];
	this->col[ Z ] = -temp;
}

float GetAngle( const Matrix &currentOrientation, const Vector &desiredHeading, int headingAxis, int rotAxis )
{
	

	Dbg_MsgAssert( headingAxis == X || headingAxis == Y || headingAxis == Z,( "Improper heading axis." ));
	Dbg_MsgAssert( rotAxis == X || rotAxis == Y || rotAxis == Z,( "Improper rot axis." ));
	Dbg_MsgAssert( rotAxis != headingAxis,( "Can't use heading axis as rot axis." ));

	Mth::Vector tempHeading = desiredHeading;
	tempHeading.ProjectToPlane( currentOrientation[ rotAxis ] );
	if ( !tempHeading.Length( ) )
	{
		// If our rot axis is along Y, for example, the
		// target heading is right above us... Can't pick
		// a rotation!
		return ( 0.0f );
	}
	tempHeading.Normalize( );

	// while we have these two vectors, find the angle between them...
	float angCos = DotProduct( currentOrientation[ headingAxis ], tempHeading );
	
	// Mick:  contrain Dot product to range -1.0f to +1.0f, since FP innacuracies might
	// make it go outside this range
	// (previously this was done only on NGC.  Not sure why it has started failing now
	// but it seems logical that it would fail occasionally, so this check is necessary) 	
	float	ang = angCos;
	if ( ang > 1.0f ) ang = 1.0f;
	if ( ang < -1.0f ) ang = -1.0f;
	float retVal = -acosf( ang );

	Mth::Vector temp = Mth::CrossProduct( currentOrientation[ headingAxis ], tempHeading );

	// check to see if the cross product is in the same quatrant as our rot axis...
	// if so, gots to rotate the other way and shit like that...
	int whichAxis;
	float tempMax = temp.GetMaxAxis( &whichAxis );
	if ( ( tempMax > 0 ) == ( currentOrientation[ rotAxis ][ whichAxis ] > 0 ) )
	{
		return ( -retVal );
	}
	return ( retVal );
}

float GetAngle(Vector &v1, Vector &v2)
{
	Vector a=v1;
	Vector b=v2;
	a.Normalize();
	b.Normalize();
	float dot = Mth::DotProduct(a,b);	
	if ( dot > 1.0f ) dot = 1.0f;
	if ( dot < -1.0f ) dot = -1.0f;
	return Mth::RadToDeg(acosf(dot));
}

// Returns true if the angle between a and b is greater than the passed Angle.
bool AngleBetweenGreaterThan(const Vector& a, const Vector& b, float Angle)
{
	float dot=Mth::DotProduct(a,b);
	float len=a.Length();
	if (len<0.5f)
	{
		return false;
	}	
	dot/=len;
	
	len=b.Length();
	if (len<0.5f)
	{
		return false;
	}	
	dot/=len;
	
	float TestAngleCosine=cosf(Mth::DegToRad(Angle));
	
	if (dot>=0.0f)
	{
		return dot<TestAngleCosine;
	}
	else
	{
		if (TestAngleCosine>=0.0f)
		{
			return true;
		}
		else
		{
			return dot<TestAngleCosine;
		}
	}		
}

// Mick:
// Given two vectors, v1 and v2 and an Axis vector
// return the amount that v1 would have to rotate about the Axis
// in order to be pointing in the same direction as v2
// when viewed along the Axis
//
// Typically, "Axis" will be the Y axis of the object (Like the skater)
// and v1 will be the Z direction
// which we want to rotate toward v2 (about Y)
// 
// retunr value will be in the range -PI .. PI   (-180 to 180 degrees)
float GetAngleAbout(Vector &v1, Vector &v2, Vector &Axis)
{
	Axis.Normalize();
	v1.ProjectToPlane(Axis);
	v2.ProjectToPlane(Axis);
	v1.Normalize();
	v2.Normalize();
	Vector v3 = Mth::CrossProduct(Axis,v1);
	float v1_dot = Mth::DotProduct(v1,v2);
	float v3_dot = Mth::DotProduct(v3,v2);

	float angle = acosf(Mth::Clamp(v1_dot, -1.0f, 1.0f));

	if (v3_dot < 0.0f)
	{
		angle = -angle;
	}
	return angle;
}

#ifdef		DEBUG_OVERFLOW_VECTORS

const float&		Vector::operator[] ( sint i ) const
{
//	Dbg_MsgAssert (Abs(col[i]) < 1000000000.0f  || col[i]==1.0e+30f,("vector component (%f) over a billion!!!",col[i]));
	Dbg_MsgAssert (!isnanf(col[i]),("vector component [%d] (%f) not a number!!!!",i,col[i]));
//	Dbg_MsgAssert (!isinff(col[i]),("vector component [%d] (%f) infinite!!!!",i,col[i]));
	return col[i];
}
#endif

#ifdef		DEBUG_UNINIT_VECTORS

const float&		Vector::operator[] ( sint i ) const
{
// This assertion will catch any of the predefined NaN values that we initilize the vectors and matrix to
// when DEBUG_UNINIT_VECTORS is defined.
	Dbg_MsgAssert ( (*(uint32*)(&col[i]) & 0xffffff00) != 0x7F800000,("uninitialized vector[%d] = 0x%x",i,(*(uint32*)(&col[i]))));
	return col[i];
}


// for debugging purposes, this function is not inline when we want the 
// uninitialized vector debugging
// as it makes it easier to track down wehre the problem is
Vector&		Vector::operator=	( const Vector& v )
{	

	col[X] = v[X];
	col[Y] = v[Y];
	col[Z] = v[Z];
	col[W] = v[W];
	return *this;
}


#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Vector::PrintContents() const
{
//	printf( "-----------------\n" );
	printf( "[%f %f %f %f]\n", col[X], col[Y], col[Z], col[W] );
//	printf( "-----------------\n" );
}

}

