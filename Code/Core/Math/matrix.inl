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
**	Module:			Math (Mth)			   									**
**																			**
**	File name:		core/math/Matrix.inl									**
**																			**
**	Created:		11/23/99	-	mjb										**
**																			**
**	Description:	4x4 Matrix Math Class									**
**																			**
*****************************************************************************/

#ifndef __CORE_MATH_MATRIX_INL
#define __CORE_MATH_MATRIX_INL

namespace Mth
{
						  


/*****************************************************************************
**							 Private Inline Functions  						**
*****************************************************************************/


/*****************************************************************************
**							Public Inline Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix&		Matrix::Ident ( void )
{
	row[X][X] = 1.0f;
	row[X][Y] = 0.0f;
	row[X][Z] = 0.0f;
	row[X][W] = 0.0f;

	row[Y][X] = 0.0f;
	row[Y][Y] = 1.0f;
	row[Y][Z] = 0.0f;
	row[Y][W] = 0.0f;

	row[Z][X] = 0.0f;
	row[Z][Y] = 0.0f;
	row[Z][Z] = 1.0f;
	row[Z][W] = 0.0f;

	row[W][X] = 0.0f;
	row[W][Y] = 0.0f;
	row[W][Z] = 0.0f;
	row[W][W] = 1.0f;

	return *this;
}


inline Matrix::Matrix( void )
{

#ifdef		DEBUG_UNINIT_VECTORS
// For debugging uninitialized vectors, we set them to a magic value
// and then the vector code should assert when they are accessed
// via the [] operator, which casts a row to a vector	
	*((uint32*)(&row[X][X])) = 0x7F800010;				// Positive NANS (Not A Number - Signaling)	
	*((uint32*)(&row[X][Y])) = 0x7F800011;				// Positive NANS (Not A Number - Signaling)	
	*((uint32*)(&row[X][Z])) = 0x7F800012;				// Positive NANS (Not A Number - Signaling)	
	*((uint32*)(&row[X][W])) = 0x7F800013;				// Positive NANS (Not A Number - Signaling)	
	*((uint32*)(&row[Y][X])) = 0x7F800014;				// Positive NANS (Not A Number - Signaling)	
	*((uint32*)(&row[Y][Y])) = 0x7F800015;				// Positive NANS (Not A Number - Signaling)	
	*((uint32*)(&row[Y][Z])) = 0x7F800016;				// Positive NANS (Not A Number - Signaling)	
	*((uint32*)(&row[Y][W])) = 0x7F800017;				// Positive NANS (Not A Number - Signaling)	
	*((uint32*)(&row[Z][X])) = 0x7F800018;				// Positive NANS (Not A Number - Signaling)	
	*((uint32*)(&row[Z][Y])) = 0x7F800019;				// Positive NANS (Not A Number - Signaling)	
	*((uint32*)(&row[Z][Z])) = 0x7F80001a;				// Positive NANS (Not A Number - Signaling)	
	*((uint32*)(&row[Z][W])) = 0x7F80001b;				// Positive NANS (Not A Number - Signaling)	
	*((uint32*)(&row[W][X])) = 0x7F80001c;				// Positive NANS (Not A Number - Signaling)	
	*((uint32*)(&row[W][Y])) = 0x7F80001d;				// Positive NANS (Not A Number - Signaling)	
	*((uint32*)(&row[W][Z])) = 0x7F80001e;				// Positive NANS (Not A Number - Signaling)	
	*((uint32*)(&row[W][W])) = 0x7F80001f;				// Positive NANS (Not A Number - Signaling)	
#else
#ifdef	__INITIALIZE_VECTORS__
	Ident();	

#endif
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix::Matrix( const Vector& axis, const float angle )
{
	

	CreateRotateMatrix( *this, axis, angle );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix::Matrix( int axis, const float angle )
{
	

	CreateRotateMatrix( *this, axis, angle );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix::Matrix( const Mth::Quat &orientation )
{
    orientation.GetMatrix(*this);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const Vector&	Matrix::operator[] ( sint i ) const
{ 
//	Dbg_MsgAssert (( i >= X && i < NUM_ELEMENTS ),( "index out of range" ));
	return *(Vector*)(row[i]);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Vector&			Matrix::operator[] ( sint i )
{ 
//	Dbg_MsgAssert (( i >= X && i < NUM_ELEMENTS ),( "index out of range" ));
	return *(Mth::Vector*)(&row[i]);
}


/*
#ifdef	__USE_VU0__
inline void asm_copy(void* m0, void* m1)
{
	asm __volatile__("
	lq    $6,0x0(%1)
	lq    $7,0x10(%1)
	lq    $8,0x20(%1)
	lq    $9,0x30(%1)
	sq    $6,0x0(%0)
	sq    $7,0x10(%0)
	sq    $8,0x20(%0)
	sq    $9,0x30(%0)
	": : "r" (m0) , "r" (m1):"$6","$7","$8","$9");
}
#endif
*/


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
						
inline Matrix& 		Matrix::operator= ( const Matrix& src )
{
#ifdef	__USE_VU0__
// Mick:  This actually is faster than the asm_copy method as
// the compiler can optimize it better	
	*((uint128 *)&row[0])=*((uint128 *)&src[0]);
	*((uint128 *)&row[1])=*((uint128 *)&src[1]);
	*((uint128 *)&row[2])=*((uint128 *)&src[2]);
	*((uint128 *)&row[3])=*((uint128 *)&src[3]);
	// asm_copy(this,(void*)&src[0]);
#else
	*(Vector*)row[X]	= *(Vector*)src.row[X];
	*(Vector*)row[Y]	= *(Vector*)src.row[Y];
	*(Vector*)row[Z] 	= *(Vector*)src.row[Z]; 
	*(Vector*)row[W]	= *(Vector*)src.row[W];
#endif
	return *this;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix::Matrix( const Matrix& src )
{
	*this = src;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void	Swap ( Matrix& a, Matrix& b )
{
	Matrix t = a;
	a = b;
	b = t;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix&		Matrix::operator*= ( const Matrix& n )
{	
#	ifdef __PLAT_XBOX__
	D3DXMatrixMultiply((D3DXMATRIX*)this, (D3DXMATRIX*)this, (D3DXMATRIX*)&n );
#	else
#ifdef	__USE_VU0__
	sceVu0MulMatrix((sceVu0FVECTOR*)this,(sceVu0FVECTOR*)&n,(sceVu0FVECTOR*)this);	
#	else

	Vector	vec;

	vec[X] = row[X][X] * n[X][X] + row[X][Y] * n[Y][X] + row[X][Z] * n[Z][X] + row[X][W] * n[W][X];
	vec[Y] = row[X][X] * n[X][Y] + row[X][Y] * n[Y][Y] + row[X][Z] * n[Z][Y] + row[X][W] * n[W][Y];
	vec[Z] = row[X][X] * n[X][Z] + row[X][Y] * n[Y][Z] + row[X][Z] * n[Z][Z] + row[X][W] * n[W][Z];
	vec[W] = row[X][X] * n[X][W] + row[X][Y] * n[Y][W] + row[X][Z] * n[Z][W] + row[X][W] * n[W][W];
	*(Vector*)row[X] = vec;

	vec[X] = row[Y][X] * n[X][X] + row[Y][Y] * n[Y][X] + row[Y][Z] * n[Z][X] + row[Y][W] * n[W][X];
	vec[Y] = row[Y][X] * n[X][Y] + row[Y][Y] * n[Y][Y] + row[Y][Z] * n[Z][Y] + row[Y][W] * n[W][Y];
	vec[Z] = row[Y][X] * n[X][Z] + row[Y][Y] * n[Y][Z] + row[Y][Z] * n[Z][Z] + row[Y][W] * n[W][Z];
	vec[W] = row[Y][X] * n[X][W] + row[Y][Y] * n[Y][W] + row[Y][Z] * n[Z][W] + row[Y][W] * n[W][W];
	*(Vector*)row[Y] = vec;

	vec[X] = row[Z][X] * n[X][X] + row[Z][Y] * n[Y][X] + row[Z][Z] * n[Z][X] + row[Z][W] * n[W][X];
	vec[Y] = row[Z][X] * n[X][Y] + row[Z][Y] * n[Y][Y] + row[Z][Z] * n[Z][Y] + row[Z][W] * n[W][Y];
	vec[Z] = row[Z][X] * n[X][Z] + row[Z][Y] * n[Y][Z] + row[Z][Z] * n[Z][Z] + row[Z][W] * n[W][Z];
	vec[W] = row[Z][X] * n[X][W] + row[Z][Y] * n[Y][W] + row[Z][Z] * n[Z][W] + row[Z][W] * n[W][W];
	*(Vector*)row[Z] = vec;

	vec[X] = row[W][X] * n[X][X] + row[W][Y] * n[Y][X] + row[W][Z] * n[Z][X] + row[W][W] * n[W][X];
	vec[Y] = row[W][X] * n[X][Y] + row[W][Y] * n[Y][Y] + row[W][Z] * n[Z][Y] + row[W][W] * n[W][Y];
	vec[Z] = row[W][X] * n[X][Z] + row[W][Y] * n[Y][Z] + row[W][Z] * n[Z][Z] + row[W][W] * n[W][Z];
	vec[W] = row[W][X] * n[X][W] + row[W][Y] * n[Y][W] + row[W][Z] * n[Z][W] + row[W][W] * n[W][W];
	*(Vector*)row[W] = vec;
#	endif
#	endif

	return *this;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix	operator* ( const Matrix& m1, const Matrix& m2 )
{
	
	
	Matrix	result = m1;
	result *= m2;

	return result;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix& 		Matrix::operator*= ( const float s )
{	
	
	
	if ( s != 1.0f )
	{
		*(Vector*)(row[X]) *= s;
		*(Vector*)(row[Y]) *= s;
		*(Vector*)(row[Z]) *= s;
		*(Vector*)(row[W]) *= s;
	}

	return *this;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix& 		Matrix::operator+= ( const Matrix& n )
{	
	row[X][X] += n[X][X];
	row[X][Y] += n[X][Y];
	row[X][Z] += n[X][Z];
	row[X][W] += n[X][W];

	row[Y][X] += n[Y][X];
	row[Y][Y] += n[Y][Y];
	row[Y][Z] += n[Y][Z];
	row[Y][W] += n[Y][W];

	row[Z][X] += n[Z][X];
	row[Z][Y] += n[Z][Y];
	row[Z][Z] += n[Z][Z];
	row[Z][W] += n[Z][W];

	row[W][X] += n[W][X];
	row[W][Y] += n[W][Y];
	row[W][Z] += n[W][Z];
	row[W][W] += n[W][W];

	return *this;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool				Matrix::operator== ( const Matrix& n ) const
{
	

	for ( sint i = 0; i < NUM_ELEMENTS; i++ )
	{
		for ( sint j = 0; j < NUM_ELEMENTS; j++ )
		{
			if ( row[i][j] != n[i][j] )
			{
				return false;
			}
		}
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool				Matrix::operator!= ( const Matrix& n ) const
{
	

	return !( *this == n );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float			Matrix::Determinant ( void ) const
{
	

    return row[X][X] * Determinant3 ( row[Y][Y], row[Z][Y], row[W][Y], row[Y][Z], row[Z][Z], row[W][Z], row[Y][W], row[Z][W], row[W][W] )
         - row[X][Y] * Determinant3 ( row[Y][X], row[Z][X], row[W][X], row[Y][Z], row[Z][Z], row[W][Z], row[Y][W], row[Z][W], row[W][W] )
         + row[X][Z] * Determinant3 ( row[Y][X], row[Z][X], row[W][X], row[Y][Y], row[Z][Y], row[W][Y], row[Y][W], row[Z][W], row[W][W] )
         - row[X][W] * Determinant3 ( row[Y][X], row[Z][X], row[W][X], row[Y][Y], row[Z][Y], row[W][Y], row[Y][Z], row[Z][Z], row[W][Z] );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool				Matrix::IsIdent	( void ) const
{
	

	return ( row[X][X] == 1.0f && row[X][Y] == 0.0f && row[X][Z] == 0.0f && row[X][W] == 0.0f &&
		     row[Y][X] == 0.0f && row[Y][Y] == 1.0f && row[Y][Z] == 0.0f && row[Y][W] == 0.0f &&
		     row[Z][X] == 0.0f && row[Z][Y] == 0.0f && row[Z][Z] == 1.0f && row[Z][W] == 0.0f &&
		     row[W][X] == 0.0f && row[W][Y] == 0.0f && row[W][Z] == 0.0f && row[W][W] == 1.0f );
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix&		Matrix::Identity ( void )
{
	row[X][X] = 1.0f;
	row[X][Y] = 0.0f;
	row[X][Z] = 0.0f;
	row[X][W] = 0.0f;

	row[Y][X] = 0.0f;
	row[Y][Y] = 1.0f;
	row[Y][Z] = 0.0f;
	row[Y][W] = 0.0f;

	row[Z][X] = 0.0f;
	row[Z][Y] = 0.0f;
	row[Z][Z] = 1.0f;
	row[Z][W] = 0.0f;

	row[W][X] = 0.0f;
	row[W][Y] = 0.0f;
	row[W][Z] = 0.0f;
	row[W][W] = 1.0f;

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix&		Matrix::Zero ( void )
{
	row[X][X] = 0.0f;
	row[X][Y] = 0.0f;
	row[X][Z] = 0.0f;
	row[X][W] = 0.0f;

	row[Y][X] = 0.0f;
	row[Y][Y] = 0.0f;
	row[Y][Z] = 0.0f;
	row[Y][W] = 0.0f;

	row[Z][X] = 0.0f;
	row[Z][Y] = 0.0f;
	row[Z][Z] = 0.0f;
	row[Z][W] = 0.0f;

	row[W][X] = 0.0f;
	row[W][Y] = 0.0f;
	row[W][Z] = 0.0f;
	row[W][W] = 0.0f;

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix&		Matrix::Adjoint ( const Matrix& src )
{
	

    row[X][X]  =  Determinant3 ( src[Y][Y], src[Z][Y], src[W][Y], src[Y][Z], src[Z][Z], src[W][Z], src[Y][W], src[Z][W], src[W][W] );
    row[Y][X]  = -Determinant3 ( src[Y][X], src[Z][X], src[W][X], src[Y][Z], src[Z][Z], src[W][Z], src[Y][W], src[Z][W], src[W][W] );
    row[Z][X]  =  Determinant3 ( src[Y][X], src[Z][X], src[W][X], src[Y][Y], src[Z][Y], src[W][Y], src[Y][W], src[Z][W], src[W][W] );
    row[W][X]  = -Determinant3 ( src[Y][X], src[Z][X], src[W][X], src[Y][Y], src[Z][Y], src[W][Y], src[Y][Z], src[Z][Z], src[W][Z] );
     
    row[X][Y]  = -Determinant3 ( src[X][Y], src[Z][Y], src[W][Y], src[X][Z], src[Z][Z], src[W][Z], src[X][W], src[Z][W], src[W][W] );
    row[Y][Y]  =  Determinant3 ( src[X][X], src[Z][X], src[W][X], src[X][Z], src[Z][Z], src[W][Z], src[X][W], src[Z][W], src[W][W] );
    row[Z][Y]  = -Determinant3 ( src[X][X], src[Z][X], src[W][X], src[X][Y], src[Z][Y], src[W][Y], src[X][W], src[Z][W], src[W][W] );
    row[W][Y]  =  Determinant3 ( src[X][X], src[Z][X], src[W][X], src[X][Y], src[Z][Y], src[W][Y], src[X][Z], src[Z][Z], src[W][Z] );
        
    row[X][Z]  =  Determinant3 ( src[X][Y], src[Y][Y], src[W][Y], src[X][Z], src[Y][Z], src[W][Z], src[X][W], src[Y][W], src[W][W] );
    row[Y][Z]  = -Determinant3 ( src[X][X], src[Y][X], src[W][X], src[X][Z], src[Y][Z], src[W][Z], src[X][W], src[Y][W], src[W][W] );
    row[Z][Z]  =  Determinant3 ( src[X][X], src[Y][X], src[W][X], src[X][Y], src[Y][Y], src[W][Y], src[X][W], src[Y][W], src[W][W] );
    row[W][Z]  = -Determinant3 ( src[X][X], src[Y][X], src[W][X], src[X][Y], src[Y][Y], src[W][Y], src[X][Z], src[Y][Z], src[W][Z] );
        
    row[X][W]  = -Determinant3 ( src[X][Y], src[Y][Y], src[Z][Y], src[X][Z], src[Y][Z], src[Z][Z], src[X][W], src[Y][W], src[Z][W] );
    row[Y][W]  =  Determinant3 ( src[X][X], src[Y][X], src[Z][X], src[X][Z], src[Y][Z], src[Z][Z], src[X][W], src[Y][W], src[Z][W] );
    row[Z][W]  = -Determinant3 ( src[X][X], src[Y][X], src[Z][X], src[X][Y], src[Y][Y], src[Z][Y], src[X][W], src[Y][W], src[Z][W] );
    row[W][W]  =  Determinant3 ( src[X][X], src[Y][X], src[Z][X], src[X][Y], src[Y][Y], src[Z][Y], src[X][Z], src[Y][Z], src[Z][Z] );

	return *this;

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix&		Matrix::Invert ( const Matrix& src )
{
	

	Adjoint (src);

    float det = src.Determinant();

	if ( det != 1.0f )
	{
		*this *= ( 1.0f / det );
	}

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix&		Matrix::Invert ( void )
{
	
	
	Matrix	d;

	d.Invert( *this );

	*this = d;
	
	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#ifdef	__USE_VU0__
// K: Copied from sceVu0InversMatrix, just tweaked to take one param & be inline.
// This is about 18% faster than in C. (1000000 calls take .121 s instead of .148)
inline void Vu0Invert(sceVu0FMATRIX m0)
{
	asm __volatile__("
	lq $8,0x0000(%0)
	lq $9,0x0010(%0)
	lq $10,0x0020(%0)
	lqc2 vf4,0x0030(%0)

	vmove.xyzw vf5,vf4
	vsub.xyz vf4,vf4,vf4		#vf4.xyz=0;
	vmove.xyzw vf9,vf4
	qmfc2    $11,vf4

	#“]’u
	pextlw     $12,$9,$8
	pextuw     $13,$9,$8
	pextlw     $14,$11,$10
	pextuw     $15,$11,$10
	pcpyld     $8,$14,$12
	pcpyud     $9,$12,$14
	pcpyld     $10,$15,$13

	qmtc2    $8,vf6
	qmtc2    $9,vf7
	qmtc2    $10,vf8

	#“àÏ
	vmulax.xyz	ACC,   vf6,vf5
	vmadday.xyz	ACC,   vf7,vf5
	vmaddz.xyz	vf4,vf8,vf5
	vsub.xyz	vf4,vf9,vf4

	sq $8,0x0000(%0)
	sq $9,0x0010(%0)
	sq $10,0x0020(%0)
	sqc2 vf4,0x0030(%0)
	": : "r" (m0) :"$8","$9","$10","$11","$12","$13","$14","$15");
}
#endif

inline Matrix&		Matrix::InvertUniform ()
{
	
	
	// Only works for orthonormal Matrix
			
	#if 0	// (Mike) this version is buggy!
	
	Transpose ( src );
	
	row[POS][X] = -DotProduct ( src[POS], src[RIGHT] );
	row[POS][Y] = -DotProduct ( src[POS], src[UP] );
	row[POS][Z] = -DotProduct ( src[POS], src[AT] );
	
	return *this;

	#else	// (Mike) try this one instead...
	
	// (Dan) actually, don't use the vu0; it returns before the correct values are safely stored in memory; if you access the results too
	// quickly, you get the wrong answers; this is because the compiler may reorder instructions
	#if 0
	// (Ken) or try this one, uses vu0 
	Vu0Invert((sceVu0FMATRIX)this);
	return *this;
	#else
	
	// need to copy the old row,
	// or else it will change on us as we're doing
	// our dot products...
	
	Mth::Vector oldPos = *(Vector*)row[POS];

	row[POS][X] = -DotProduct ( oldPos, *(Vector*)row[RIGHT] );
	row[POS][Y] = -DotProduct ( oldPos, *(Vector*)row[UP] );
	row[POS][Z] = -DotProduct ( oldPos, *(Vector*)row[AT] );

	Swap ( row[X][Y], row[Y][X] );
	Swap ( row[X][Z], row[Z][X] );
	Swap ( row[Y][Z], row[Z][Y] );

	return *this;
	
	#endif
	
	#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline Matrix&			Matrix::Transpose ( void )
{
	

	Swap ( row[X][Y], row[Y][X] );
	Swap ( row[X][Z], row[Z][X] );
	Swap ( row[X][W], row[W][X] );
	Swap ( row[Y][Z], row[Z][Y] );
	Swap ( row[Y][W], row[W][Y] );
	Swap ( row[Z][W], row[W][Z] );

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline Matrix&			Matrix::Transpose ( const Matrix& src )
{
	

	if ( &src == this )
	{
		return Transpose();
	}

	row[X][X] = src[X][X];
	row[X][Y] = src[Y][X];
	row[X][Z] = src[Z][X];
	row[X][W] = src[W][X];
	row[Y][X] = src[X][Y];
	row[Y][Y] = src[Y][Y];
	row[Y][Z] = src[Z][Y];
	row[Y][W] = src[W][Y];
	row[Z][X] = src[X][Z];
	row[Z][Y] = src[Y][Z];
	row[Z][Z] = src[Z][Z];
	row[Z][W] = src[W][Z];
	row[W][X] = src[X][W];
	row[W][Y] = src[Y][W];
	row[W][Z] = src[Z][W];
	row[W][W] = src[W][W];

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline Vector			Matrix::Transform ( const Vector& src ) const
{
	
		
	Vector	v;

	#ifdef	__USE_VU0__
	sceVu0ApplyMatrix((float*)&v,(sceVu0FVECTOR*)this,(float*)&src);
	#else

	v[X] = src[X] * row[RIGHT][X] + src[Y] * row[UP][X] + src[Z] * row[AT][X] + src[W] * row[POS][X];
	v[Y] = src[X] * row[RIGHT][Y] + src[Y] * row[UP][Y] + src[Z] * row[AT][Y] + src[W] * row[POS][Y];
	v[Z] = src[X] * row[RIGHT][Z] + src[Y] * row[UP][Z] + src[Z] * row[AT][Z] + src[W] * row[POS][Z];
	v[W] = src[X] * row[RIGHT][W] + src[Y] * row[UP][W] + src[Z] * row[AT][W] + src[W] * row[POS][W];

	#endif
	
	return v;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline Vector			Matrix::TransformAsPos ( const Vector& src ) const
{
	
	Vector	v;

	v[X] = src[X] * row[RIGHT][X] + src[Y] * row[UP][X] + src[Z] * row[AT][X] + row[POS][X];
	v[Y] = src[X] * row[RIGHT][Y] + src[Y] * row[UP][Y] + src[Z] * row[AT][Y] + row[POS][Y];
	v[Z] = src[X] * row[RIGHT][Z] + src[Y] * row[UP][Z] + src[Z] * row[AT][Z] + row[POS][Z];
	v[W] = src[X] * row[RIGHT][W] + src[Y] * row[UP][W] + src[Z] * row[AT][W] + row[POS][W];

	return v;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline Vector			Matrix::Rotate ( const Vector& src ) const
{
	
		
	Vector	v;

	v[X] = src[X] * row[RIGHT][X] + src[Y] * row[UP][X] + src[Z] * row[AT][X];
	v[Y] = src[X] * row[RIGHT][Y] + src[Y] * row[UP][Y] + src[Z] * row[AT][Y];
	v[Z] = src[X] * row[RIGHT][Z] + src[Y] * row[UP][Z] + src[Z] * row[AT][Z];
	v[W] = src[W];
	
	return v;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline Matrix&			Matrix::Translate( const Vector& trans )
{
	

	*(Vector*)(row[POS]) += trans;

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Ken: Added cos I needed it.
inline void	Matrix::SetPos( const Vector& trans )
{
	*(Vector*)(row[POS]) = trans;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline Matrix&			Matrix::TranslateLocal( const Vector& trans )
{
	

	Vector&	vec = *(Vector*)(row[POS]);

	vec[X] +=  (( trans[X] * row[RIGHT][X] ) +
				( trans[Y] * row[UP][X] ) +
				( trans[Z] * row[AT][X] ));

	vec[Y] +=  (( trans[X] * row[RIGHT][Y] ) +
				( trans[Y] * row[UP][Y] ) +
				( trans[Z] * row[AT][Y] ));

	vec[Z] +=  (( trans[X] * row[RIGHT][Z] ) +
				( trans[Y] * row[UP][Z] ) +
				( trans[Z] * row[AT][Z] ));

	return *this;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix&			Matrix::Rotate( const Vector& axis, const float angle )
{
	

	Matrix	rot_mat( axis, angle );

	*this *= rot_mat;

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix&			Matrix::RotateLocal( const Vector& axis, const float angle )
{
	

	Matrix	rot_mat( axis, angle );

	*this = rot_mat * *this;

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix&			Matrix::RotateLocal( int axis, const float angle )
{
	

	Matrix	rot_mat( axis, angle );

	*this = rot_mat * *this;

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix&		Matrix::RotateX( const float angle )
{
	
	Matrix rotX;

	*this *= CreateRotateXMatrix( rotX, angle );
	
	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix&		Matrix::RotateXLocal( const float angle )
{
	
	Matrix rotX;

	*this = CreateRotateXMatrix( rotX, angle ) * *this;

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix&		Matrix::RotateY( const float angle )
{
	
	Matrix rotY;

	*this *= CreateRotateYMatrix( rotY, angle );

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix&		Matrix::RotateYLocal( const float angle )
{
	
	#ifdef	__USE_VU0__
	
	// Use the end of SCache for a quick and easy speed increase.
	// (Mick, changed to use the end, so start can be used for permanent stuff)
	Matrix *p_rot_y=(Matrix*)(0x70004000-4*4*4);

	*this = CreateRotateYMatrix( *p_rot_y, angle ) * *this;
	
	#else
	
	Matrix rotY;
	*this = CreateRotateYMatrix( rotY, angle ) * *this;
	
	#endif

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix&		Matrix::RotateZ( const float angle )
{
	

	Matrix rotZ;

	*this *= CreateRotateZMatrix( rotZ, angle );

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix&		Matrix::RotateZLocal( const float angle )
{
	

	Matrix rotZ;

	*this = CreateRotateZMatrix( rotZ, angle ) * *this;

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline Matrix& Matrix::Scale( const Vector& scale )
{
	
	
	*(Vector*)(row[RIGHT]) *= scale;
	*(Vector*)(row[UP]) *= scale;
	*(Vector*)(row[AT]) *= scale;
	*(Vector*)(row[POS]) *= scale;

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline Matrix& Matrix::ScaleLocal( const Vector& scale )
{
	

	*(Vector*)(row[RIGHT]) *= scale[X];
	*(Vector*)(row[UP]) *= scale[Y];
	*(Vector*)(row[AT]) *= scale[Z];

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
inline Matrix&			Matrix::SetColumn ( sint i,  const Vector& v )
{
	
	
//	Dbg_MsgAssert (( i >= X && i < NUM_ELEMENTS ),( "index out of range" ));
	
	row[X][i] = v[X]; 
	row[Y][i] = v[Y]; 
	row[Z][i] = v[Z]; 
	row[W][i] = v[W];
	
	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Vector			Matrix::GetColumn ( sint i ) const	
{
	
	
//	Dbg_MsgAssert (( i >= X && i < NUM_ELEMENTS ),( "index out of range" ));
	
	return Vector ( row[X][i], row[Y][i], row[Z][i], row[W][i] );
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline ostream& operator<<	( ostream& os, const Matrix& m )
{
	return os << "{ " << m[X] << ",\n " << m[Y] << ",\n " << m[Z] << ",\n " << m[W] << " }";
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Matrix&			Matrix::SetFromAngles ( const Vector& angles )
{
	Identity();
	if (angles[X] != 0.0f || angles[Y] != 0.0f || angles[Z] != 0.0f)
	{
		RotateX(angles[X]);
		RotateY(angles[Y]);
		RotateZ(angles[Z]);
	}
	return *this;
}

} // namespace Mth

#endif // __CORE_MATH_MATRIX_INL

