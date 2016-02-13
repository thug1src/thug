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
**	File name:		core/math/Matrix.h										**
**																			**
**	Created:		11/29/99	-	mjb										**
**																			**
**	Description:	4x4 Matrix Math Class									**
**																			**
*****************************************************************************/

#ifndef __CORE_MATH_MATRIX_H
#define __CORE_MATH_MATRIX_H

/*****************************************************************************
**								   Includes									**
*****************************************************************************/

	
/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Mth
{




enum
{
	RIGHT, UP, AT, POS
};

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

/****************************************************************************
 *																			
 * Class:			Matrix
 *
 * Description:		4x4 Matrix math class
 *
 ****************************************************************************/

class  Matrix
{
	

public:
					Matrix			( void );
					Matrix			( const Matrix& src );
					Matrix			( const Vector& axis, const float angle );
					Matrix			( int axis, const float angle );
					Matrix			( float p, float h, float r );	// Euler to Matrix
                    Matrix          ( const Mth::Quat& orientaion );
						
	Matrix& 		operator=		( const Matrix& src );
	Matrix&			operator+=		( const Matrix& n );
	Matrix&			operator*=		( const Matrix& n );
	Matrix&			operator*=		( const float f );
	bool			operator==		( const Matrix& n ) const;
	bool			operator!=		( const Matrix& n ) const;

	float			Determinant		( void ) const;
	bool			IsIdent			( void ) const;
	Matrix&			Ident			( void );					// set to 4x4 identity
	Matrix&			Identity		( void );					// same, but better name
	Matrix&			Zero			( void );					// set to all 0's
	Matrix&			Adjoint			( const Matrix& src );
	Matrix&			Invert			( const Matrix& src );		// this = Invert ( src )
	Matrix&			Invert			( void );			 	  	// this = Invert ( this )
	Matrix&			InvertUniform 	( void );
	Matrix&			Transpose		( const Matrix& src );
	Matrix&			Transpose		( void );

	Vector			Transform		( const Vector& src ) const;
	Vector			TransformAsPos	( const Vector& src ) const;
	Vector			Rotate			( const Vector& src ) const;

	Matrix&			OrthoNormalize	( const Matrix& src );		// this = OrthoNornalize ( src )
	Matrix&			OrthoNormalize	( void );					// this = OrthoNormalize ( this )

	void			RotateLocal		( const Vector& src );
	Matrix&			Translate		( const Vector& trans );
	Matrix&			TranslateLocal	( const Vector& trans );
	
	Matrix&			Rotate			( const Vector& axis, const float angle );
	Matrix&			RotateLocal		( const Vector& axis, const float angle );
	Matrix&			RotateLocal		( int axis, float angle );
	Matrix&			RotateX			( const float angle );
	Matrix&			RotateXLocal	( const float angle );
	Matrix&			RotateY			( const float angle );
	Matrix&			RotateYLocal	( const float angle );
	Matrix&			RotateZ			( const float angle );
	Matrix&			RotateZLocal	( const float angle );

	Matrix&			Scale			( const Vector& scale );
	Matrix&			ScaleLocal		( const Vector& scale );
	
	Vector&			GetRight		( void ) { return *(Vector*)(&row[RIGHT]); }
	Vector&			GetUp			( void ) { return *(Vector*)(&row[UP]); }
	Vector&			GetAt			( void ) { return *(Vector*)(&row[AT]); }
	Vector&			GetPos			( void ) { return *(Vector*)(&row[POS]); }
	void			SetPos			( const Vector& trans );


	Matrix&			SetColumn		( sint i, const Vector& v );
	Vector			GetColumn		( sint i ) const;
	const Vector&	operator[]		( sint i ) const;
	Vector&			operator[]		( sint i );

	Matrix&			OrthoNormalizeAbout(int r0);
	void			PrintContents() const;

	void			GetEulers( Vector& euler ) const;
	void			GetRotationAxisAndAngle( Vector* pAxis, float* pRadians );
	
	bool			PatchOrthogonality (   );
	
	Matrix&			SetFromAngles	( const Vector& angles );

private:

				enum
				{
					NUM_ELEMENTS = 4
				};

//	Vector		row[NUM_ELEMENTS];	   		// Old style, an array of vectors
	float		row[NUM_ELEMENTS][4];		// New style, an array of float, for constructor optimization

} nAlign(128);

/*****************************************************************************
**							 Private Inline Functions  						**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

#ifdef	__PLAT_NGPS__
void xsceVu0MulMatrix(Mth::Matrix* m0, Mth::Matrix* m1, const Mth::Matrix* m2);
#endif


/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/


/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

void		Swap ( Matrix& a, Matrix& b );
void		Slerp ( Matrix& result, const Matrix& s1, const Matrix& s2, float t );
Matrix		operator* ( const Matrix& m1, const Matrix& m2 );
ostream& 	operator<< ( ostream& os, const Matrix& m );

Matrix&		CreateRotateMatrix ( Matrix& mat, const Vector& axis, const float angle );
Matrix&		CreateRotateMatrix ( Matrix& mat, int axis, const float angle );
Matrix&		CreateRotateXMatrix ( Matrix& mat, const float angle );
Matrix&		CreateRotateYMatrix ( Matrix& mat, const float angle );
Matrix&		CreateRotateZMatrix ( Matrix& mat, const float angle );
Matrix&		CreateFromToMatrix( Matrix &mtx, Vector from, Vector to );

Matrix&		CreateMatrixLookAt( Matrix& mat, const Vector& pos, const Vector& lookat, const Vector& up );
Matrix&		CreateMatrixOrtho( Matrix& mat, float width, float height, float f_near, float f_far );


/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace Mth

#endif // __CORE_MATH_MATRIX_H

