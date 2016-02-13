/*****************************************************************************
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
**	File name:		core/math/Quat.h										**
**																			**
**	Created:		11/23/99	-	mjb										**
**																			**
**	Description:	Quaternion Math Class									**
**																			**
*****************************************************************************/

#ifndef __CORE_MATH_QUAT_H
#define __CORE_MATH_QUAT_H

/*****************************************************************************
**								   Includes									**
*****************************************************************************/

#include <core/math/vector.h>	

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Mth
{
 


/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class Matrix;	// forward declaration

/****************************************************************************
 *																			
 * Class:			Quat
 *
 * Description:		Quaternion math class
 *
 ****************************************************************************/

class  Quat  //: public Spt::Class
{
	

public:

	friend	bool	quat_equal ( const Quat& q1, const Quat& q2, const float tol );

					Quat ( float cx = 0.0f, float cy = 0.0f, float cz = 0.0f, float cw = 1.0f ); 

					Quat ( const Vector& axis, float angle );
					Quat ( const Matrix& mat );

			Quat&	Invert ( void );
			Quat&	Invert ( const Quat& src );
			float	Modulus	( void );
			float	ModulusSqr ( void ) const;
			Quat&	Normalize	( float len = 1.0f );
			Vector	Rotate	( const Vector& vec ) const;
			
			void	SetScalar ( const float w );
			void	SetVector ( const float x, const float y, const float z );
			void	SetVector ( const Vector& v );
	const	float&	GetScalar ( void ) const;
	const	Vector&	GetVector ( void ) const { return quat;	}
		
			void	GetMatrix ( Matrix& mat ) const;

			Quat&	operator= ( const Quat& q );
			Quat&	operator+= ( const Quat& q );
			Quat&	operator-= ( const Quat& q );
			Quat&	operator*= ( const Quat& q );
			Quat&	operator*= ( float s );
			Quat&	operator/= ( float s );
	const	float&	operator[] ( sint i ) const;
			float&	operator[] ( sint i );

			
private:

			Vector	quat; 		// X,Y,Z : Imaginary (Vector) component
								// W	 : Real (Scalar) component
};


/*****************************************************************************
**							 Private Inline Functions  						**
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

Quat		operator+	( const Quat& q1, const Quat& q2 );
Quat		operator-	( const Quat& q1, const Quat& q2 );
Quat		operator* ( const Quat& q1, const Quat& q2 );
Quat		operator* ( const Quat& q, const float s );
Quat		operator* ( const float s, const Quat& q );	  
Quat		operator- ( const Quat& q ); // negate all elements -> equilavent rotation
float		DotProduct ( const Quat& v1, const Quat& v2 );
Quat		Slerp ( const Quat& q1, const Quat& q2, const float t );
Quat		FastSlerp( Quat& qIn1, Quat& qIn2, const float t );
bool		Equal ( const Quat& q1, const Quat& q2, const float tol = 0.0001f );
ostream& 	operator<< ( ostream& os, const Quat& v );
Quat		EulerToQuat( const Vector& v );

// converts a quat+vector xform into a matrix xform
void QuatVecToMatrix( Mth::Quat* pQ, Mth::Vector* pT, Mth::Matrix* pMatrix );
void SCacheQuatVecToMatrix( Mth::Quat* pQ, Mth::Vector* pT, Mth::Matrix* pMatrix );

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace Mth

#endif // __CORE_MATH_QUAT_H


