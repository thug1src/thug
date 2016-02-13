/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Core Library											**
**																			**
**	Module:			Math (MTH)			   									**
**																			**
**	File name:		core/math/Vector.h										**
**																			**
**	Created:		11/29/99	-	mjb										**
**																			**
**	Description:	Vector Math Class										**
**																			**
*****************************************************************************/

#ifndef __CORE_MATH_VECTOR_H
#define __CORE_MATH_VECTOR_H

// (Mick) If this is defined, then vectors will be initialized to (0,0,0,1), and
// matrices to:
// (1,0,0,0)
// (0,1,0,0)
// (0,0,1,0)
// (0,0,0,1)
// This is sligtly differnet to before, where mats were intialized to all (0,0,0,1)
// since they were defined as an Array of vecotors
// This makes the code run about 3% slower (on PS2), which is why we don't use it
//#define		__INITIALIZE_VECTORS__

// you can catch code that used to rely on this (and other bugs)
// by uncommenting the following line
// this will initilize the vectors to garbage values
// and check all accesses of vectors to see if these values
// have been left before using....
// (this requires a full recompile, and makes the code run really slow)
//#define		DEBUG_UNINIT_VECTORS


// we also might want to catch situations where a component of a vector is very large
// which could indicate some potential overflow, especially if the W components is 
// accumalating errors, getting unseemly large, then getting incorporated
// into an equation that involves X,Y,Z
// (see Quat::Rotate for an example)
//#define		DEBUG_OVERFLOW_VECTORS

/*****************************************************************************
**								   Includes									**
*****************************************************************************/

#include <core/math/rot90.h>

	
/*****************************************************************************
**								   Defines									**
*****************************************************************************/

enum
{
	X, Y, Z, W
};

namespace Mth
{




/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class Quat;		// forward refs
class Matrix;

/****************************************************************************
 *																			
 * Class:			Vector
 *
 * Description:		Vector math class
 *
 ****************************************************************************/

class  Vector
{
	

public:
			// Made special enum so we can have a constructor that does not initialize
			enum ENoInitType
			{
				NO_INIT
			};

			Vector ( );
			Vector ( ENoInitType ); 			/// A constructor that doesn't init the members.
			Vector ( float cx, float cy, float cz );
			Vector ( float cx, float cy, float cz, float cw );
//			Vector ( float cx = 0.0f, float cy = 0.0f, float cz = 0.0f, float cw = 1.0f );
			Vector ( const Vector& v );
	

			Vector& 		operator=	( const Vector& v );
			bool			operator==  ( const Vector& v ) const;
			bool			operator!=  ( const Vector& v ) const;
        	Vector&			operator+=	( const Vector& v );
        	Vector&			operator-=	( const Vector& v );
        	Vector&			operator*=	( const float s );
			Vector&			operator/=	( float s );
			Vector&			operator*=	( const Vector& v );
			Vector&			operator/=	( const Vector& v );
			Vector&			operator*=	( const Matrix& mat );
	const	float&			operator[]	( sint i ) const;
			float&			operator[]	( sint i );
	
			Vector&			Set 		( float cx = 0.0f, float cy = 0.0f, float cz = 0.0f, float cw = 1.0f );
			float			Length		( void ) const;
			float			LengthSqr	( void ) const;
        	Vector&			Negate		( void );						// this = -this
			Vector&			Negate 		( const Vector& src ); 			// this = -src
        	Vector&			Normalize	( );							// faster version normlaizes to 1
        	Vector&			Normalize	( float len);

			Vector&			ShearX		( float shy, float shz );
			Vector&			ShearY		( float shx, float shz );
			Vector&			ShearZ		( float shx, float shy );

			Vector&			RotateX		( float s, float c );
			Vector&			RotateY		( float s, float c );
			Vector&			RotateZ		( float s, float c );

			Vector&			RotateX		( float angle );
			Vector&			RotateY		( float angle );
			Vector&			RotateZ		( float angle );
			
			Vector&			Rotate		( const Quat& quat );
			Vector&			Rotate		( const Matrix& matrix );
			Vector&			Rotate		( const Vector& axis, const float angle );
			
			// 90 degree increment Rotation (uses no multiplication)
			Vector&			RotateY90	( ERot90 angle );

			Vector&			Scale		( float scale );
			Vector&			Scale		( const Vector& scale );

			Vector&			RotateToPlane(const Vector& normal);
			Vector&			RotateToNormal(const Vector& normal);
			Vector&			ProjectToPlane(const Vector& normal);
			Vector&			ProjectToNormal(const Vector& normal);

			Vector&			ZeroIfShorterThan	( float length );				 
													 
			Vector&			ClampMin	( float min );
			Vector&			ClampMax	( float max );
			Vector&			Clamp		( float min = 0.0f, float max = 1.0f );
			Vector&			ClampMin	( Vector& min );
			Vector&			ClampMax	( Vector& max );
			Vector&			Clamp		( Vector& min, Vector& max );

			float			GetX		( void ) const;
			float			GetY		( void ) const;
			float			GetZ		( void ) const;
			float 			GetW		( void ) const;
			float			GetMaxAxis	( int *whichAxis = NULL );

			void			DegreesToRadians( void );
			void			RadiansToDegrees( void );
			void			FeetToInches( void );
			// convert to 3d studio max coords ( Z+ up, Y- forward, X to the right... )
			void			ConvertToMaxCoords( void );
			void			ConvertFromMaxCoords( void );
			
			// debug info
			void			PrintContents() const;
			
private:

			enum
			{
				NUM_ELEMENTS = 4
			};
		
			float				col[NUM_ELEMENTS];
} nAlign(128);

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

Vector		operator+ ( const Vector& v1, const Vector& v2 );
Vector		operator- ( const Vector& v1, const Vector& v2 );
Vector		operator* ( const Vector& v, const float s );
Vector		operator* ( const float s, const Vector& v );
Vector		operator* ( const Vector& v, const Matrix& m );
Vector		operator/ ( const Vector& v, const float s );
Vector		operator- ( const Vector& v );
float		DotProduct ( const Vector& v1, const Vector& v2 );
Vector		CrossProduct ( const Vector& v1, const Vector& v2 );
Vector		Clamp ( const Vector& v, float min = 0.0f, float max = 1.0f );
Vector		Clamp ( const Vector& v, Vector& min, Vector& max );
float		Distance ( const Vector& v1, const Vector& v2 );
float		DistanceSqr ( const Vector& v1, const Vector& v2 );
float  		GetAngle	( const Matrix &currentOrientation, const Vector &desiredHeading, int headingAxis = Z, int rotAxis = Y );
float		GetAngle(Vector &v1, Vector &v2);
bool 		AngleBetweenGreaterThan(const Vector& a, const Vector& b, float Angle);
float 		GetAngleAbout(Vector &v1, Vector &v2, Vector &Axis);
bool		Equal ( const Vector& v1, const Vector& v2, const float tol = 0.0001f );
Vector		Lerp ( const Vector& v1, const Vector& v2, const float val );
Vector		Min ( const Vector& v1, const Vector& v2 );
Vector		Max ( const Vector& v1, const Vector& v2 );
Vector		Min ( const Vector& v, float c );
Vector		Max ( const Vector& v, float c );
void		Swap ( Vector& a, Vector& b );
ostream& 	operator<< ( ostream& os, const Vector& v );

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace Mth

#endif // __CORE_MATH_VECTOR_H

