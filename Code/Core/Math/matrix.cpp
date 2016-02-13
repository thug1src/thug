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
**	Module:			Math (MTH)	 											**
**																			**
**	File name:		matrix.cpp												**
**																			**
**	Created by:		10/03/2000	-	mjb										**
**																			**
**	Description:	Matrix Math Library code								**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/math.h>

/*****************************************************************************
**							  DBG Information								**
*****************************************************************************/

namespace Mth
{

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

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Matrix::Matrix( float p, float h, float r)
{
	
	// Algorithm from http://www.flipcode.com/documents/matrfaq.html#Q37
	float A,B,C,D,E,F,AD,BD;

	A = cosf( p );
    B = sinf( p );
    C = cosf( h );
    D = sinf( h );
    E = cosf( r );
    F = sinf( r );

    AD =   A * D;
    BD =   B * D;

    row[0][0] =   C * E;
    row[0][1] =  -C * F;
    row[0][2] =  -D;
    row[1][0] = -BD * E + A * F;
    row[1][1] =  BD * F + A * E;
    row[1][2] =  -B * C;
    row[2][0] =  AD * E + B * F;
    row[2][1] = -AD * F + B * E;
    row[2][2] =   A * C;

    row[0][3] = row[1][3] = row[2][3] = row[3][0] = row[3][1] = row[3][2] = 0.0f;
    row[3][3] = 1.0f;
}

#ifdef	__PLAT_NGPS__
void xsceVu0MulMatrix(Mth::Matrix* m0, Mth::Matrix* m1, const Mth::Matrix* m2)
{

	asm __volatile__("
	lqc2    vf4,0x0(%2)
	lqc2    vf5,0x10(%2)
	lqc2    vf6,0x20(%2)
	lqc2    vf7,0x30(%2)
	li    $7,4
_loopMulMatrix:
	lqc2    vf8,0x0(%1)
	vmulax.xyzw	ACC,   vf4,vf8
	vmadday.xyzw	ACC,   vf5,vf8
	vmaddaz.xyzw	ACC,   vf6,vf8
	vmaddw.xyzw	vf9,vf7,vf8
	sqc2    vf9,0x0(%0)
	addi    $7,-1
	addi    %1,0x10
	addi    %0,0x10
	bne    $0,$7,_loopMulMatrix
	": : "r" (m0), "r" (m2), "r" (m1) : "$7");
}
#endif


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// Ken: Faster version of atan2f, using the same algorithm used by VU1
// (Coeefficients got from the VU manual, from documentation of EATANxy function, p130
// SPEEDOPT: This could probably be made even faster using a less accurate approx with few coeffs.
float katan(float y, float x)
{
	if (fabs(y)>fabs(x))
	{
		// The approximation only works for y<=x, bummer!
		return atan2f(y,x);
	}
	register bool x_negative=false;
	register bool y_negative=false;
	if (x<0.0f)
	{
		x=-x;
		x_negative=true;
	}	
	if (y<=-0.0f)
	{
		y=-y;
		y_negative=true;
	}	

	// Use the non-ratio version, because the ratio version goes 
	// all innaccurate for some values ... don't know why ...
	y=y/x;
	
	register float t1=0.999999344348907f;
	register float t2=-0.333298563957214f;
	register float t3=0.199465364217758f;
	register float t4=-0.139085337519646f;
	register float t5=0.096420042216778f;
	register float t6=-0.055909886956215f;
	register float t7=0.021861229091883f;
	register float t8=-0.004054057877511f;
	
	register float t=(y-1.0f)/(y+1.0f);
	register float tt=t*t;
	register float s=t8*t;
	s=s*tt+t7*t;
	s=s*tt+t6*t;
	s=s*tt+t5*t;
	s=s*tt+t4*t;
	s=s*tt+t3*t;
	s=s*tt+t2*t;
	s=s*tt+t1*t;
	if (x_negative)
	{
		if (y_negative)
		{
			return s+0.785398185253143f-3.141592653589793f;
		}
		else
		{
			return 3.141592653589793f-0.785398185253143f-s;
		}
	}
	else
	{
		if (y_negative)
		{
			return -0.785398185253143f-s;
		}
		else
		{
			return s+0.785398185253143f;
		}
	}
}

void Matrix::GetEulers( Vector& euler ) const
{
	

	// Algorithm from http://www.flipcode.com/documents/matrfaq.html#Q37
	float C, D;
	float tr_x, tr_y;

#ifdef __PLAT_NGC__
	float	ang = row[0][2];
	if ( ang > 1.0f ) ang = 1.0f;
	if ( ang < -1.0f ) ang = -1.0f;
	euler[Y] = D = -asinf( ang );        /* Calculate Y-axis angle */
#else
	float	ang = row[0][2];
	if ( ang > 1.0f ) ang = 1.0f;
	if ( ang < -1.0f ) ang = -1.0f;
	euler[Y] = D = -asinf( ang );        /* Calculate Y-axis angle */
//	euler[Y] = D = -asinf( row[0][2]);        /* Calculate Y-axis angle */
#endif		// __PLAT_NGC__
    C =  cosf( euler[Y] );
    	
	if( fabsf( C ) > 0.005f )             /* Gimball lock? */
	{
		tr_x      =  row[2][2] / C;           /* No, so get X-axis angle */
		tr_y      = -row[1][2] / C;
	
		euler[X]  = katan( tr_y, tr_x );
	
		tr_x      =  row[0][0] / C;            /* Get Z-axis angle */
		tr_y      = -row[0][1] / C;
	
		euler[Z]  = katan( tr_y, tr_x );
	}
	else                                 /* Gimball lock has occurred */
	{
		euler[X]  = 0;                      /* Set X-axis angle to zero */
	
		tr_x      = row[1][1];                 /* And calculate Z-axis angle */
		tr_y      = row[1][0];
	
		euler[Z]  = katan( tr_y, tr_x );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Matrix::PrintContents() const
{
	printf( "-----------------\n" );
	printf( "%f %f %f %f\n", row[RIGHT][X], row[RIGHT][Y], row[RIGHT][Z], row[RIGHT][W] );
	printf( "%f %f %f %f\n", row[UP][X], row[UP][Y], row[UP][Z], row[UP][W] );
	printf( "%f %f %f %f\n", row[AT][X], row[AT][Y], row[AT][Z], row[AT][W] );
	printf( "%f %f %f %f\n", row[POS][X], row[POS][Y], row[POS][Z], row[POS][W] );
	printf( "-----------------\n" );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*
// Original source of the optimized fromToRoation function

#include <math.h>

#define EPSILON 0.000001

#define CROSS(dest, v1, v2){                 \
          dest[0] = v1[1] * v2[2] - v1[2] * v2[1]; \
          dest[1] = v1[2] * v2[0] - v1[0] * v2[2]; \
          dest[2] = v1[0] * v2[1] - v1[1] * v2[0];}

#define DOT(v1, v2) (v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2])

#define SUB(dest, v1, v2){       \
          dest[0] = v1[0] - v2[0]; \
          dest[1] = v1[1] - v2[1]; \
          dest[2] = v1[2] - v2[2];}

//
// * A function for creating a rotation matrix that rotates a vector called
// * "from" into another vector called "to".
// * Input : from[3], to[3] which both must be *normalized* non-zero vectors
// * Output: mtx[3][3] -- a 3x3 matrix in colum-major form
// * Authors: Tomas Möller, John Hughes 1999
// 
void fromToRotation(float from[3], float to[3], float mtx[3][3])
{
  float v[3];
  float e, h, f;

  CROSS(v, from, to);
  e = DOT(from, to);
  f = (e < 0)? -e:e;
  if (f > 1.0 - EPSILON)     // "from" and "to"-vector almost parallel 
  {
    float u[3], v[3]; // temporary storage vectors 
    float x[3];       // vector most nearly orthogonal to "from" 
    float c1, c2, c3; // coefficients for later use 
    int i, j;

    x[0] = (from[0] > 0.0)? from[0] : -from[0];
    x[1] = (from[1] > 0.0)? from[1] : -from[1];
    x[2] = (from[2] > 0.0)? from[2] : -from[2];

    if (x[0] < x[1]) 
    {
      if (x[0] < x[2]) 
      {
        x[0] = 1.0; x[1] = x[2] = 0.0;
      }
      else 
      {
        x[2] = 1.0; x[0] = x[1] = 0.0;
      }
    }
    else 
    {
      if (x[1] < x[2]) 
      {
        x[1] = 1.0; x[0] = x[2] = 0.0;
      }
      else 
      {
        x[2] = 1.0; x[0] = x[1] = 0.0;
      }
    }

    u[0] = x[0] - from[0]; u[1] = x[1] - from[1]; u[2] = x[2] - from[2];
    v[0] = x[0] - to[0];   v[1] = x[1] - to[1];   v[2] = x[2] - to[2];

    c1 = 2.0 / DOT(u, u);
    c2 = 2.0 / DOT(v, v);
    c3 = c1 * c2  * DOT(u, v);
    
    for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) {
        mtx[i][j] =  - c1 * u[i] * u[j]
                     - c2 * v[i] * v[j]
                     + c3 * v[i] * u[j];
      }
      mtx[i][i] += 1.0;
    }
  }
  else  // the most common case, unless "from"="to", or "from"=-"to" 
  {
#if 0
    // unoptimized version - a good compiler will optimize this. 
    h = (1.0 - e)/DOT(v, v);
    mtx[0][0] = e + h * v[0] * v[0];  
    mtx[0][1] = h * v[0] * v[1] - v[2]; 
    mtx[0][2] = h * v[0] * v[2] + v[1];
    
    mtx[1][0] = h * v[0] * v[1] + v[2]; 
    mtx[1][1] = e + h * v[1] * v[1];    
    mtx[1][2] = h * v[1] * v[2] - v[0];

    mtx[2][0] = h * v[0] * v[2] - v[1]; 
    mtx[2][1] = h * v[1] * v[2] + v[0]; 
    mtx[2][2] = e + h * v[2] * v[2];
#else
    // ...otherwise use this hand optimized version (9 mults less) 
    float hvx, hvz, hvxy, hvxz, hvyz;
    h = (1.0 - e)/DOT(v, v);
    hvx = h * v[0];
    hvz = h * v[2];
    hvxy = hvx * v[1];
    hvxz = hvx * v[2];
    hvyz = hvz * v[1];
    mtx[0][0] = e + hvx * v[0]; 
    mtx[0][1] = hvxy - v[2];     
    mtx[0][2] = hvxz + v[1];

    mtx[1][0] = hvxy + v[2];  
    mtx[1][1] = e + h * v[1] * v[1]; 
    mtx[1][2] = hvyz - v[0];

    mtx[2][0] = hvxz - v[1];  
    mtx[2][1] = hvyz + v[0];     
    mtx[2][2] = e + hvz * v[2];
#endif
  }
}
*/


#define FROMTO_EPSILON 0.000001f


////////////////////////////////////////////////////////////////////////////////
//Matrix&		 CreateFromToMatrix( Matrix &mtx, Vector from, Vector to )
//
// * A function for creating a rotation matrix that rotates a vector called
// * "from" into another vector called "to".
// * Input : from[3], to[3] which both must be *normalized* non-zero vectors
// * Output: mtx[3][3] -- a 3x3 matrix in colum-major form
// * Authors: Tomas Möller, John Hughes 1999
// 
//
// Micknote .. on testing this, it seems like it produces the inverse of what we want
// possibly just need to swap the from/to vectors, but for now I'm just inverting it
// at the end of the function, before it returns the matrix
//
// Sample usage:
//			Mth::Matrix  rotate;
//			Mth::CreateFromToMatrix(rotate,m_matrix[Y],m_normal);
//			m_matrix *= rotate;
//
// will rotate m_matrix, so the Y component is coincident with m_normal
// (this is used to align the skater to a surface, given the surface normal)
//

Matrix&		 CreateFromToMatrix( Matrix &mtx, Vector from, Vector to )
{
  Vector cross;
  float e, h, f;
  
  
  mtx.Ident();			 			// clean up W rows and cols
  

  cross = CrossProduct(from,to);		//  CROSS(v, from, to);
  e = DotProduct(from,to);			//  e = DOT(from, to);
  f = (e < 0.0f)? -e:e;
  if (f > 1.0f - FROMTO_EPSILON)     		// "from" and "to"-vector almost parallel 
  {
    Vector u, v; // temporary storage vectors 
    Vector x;       // vector most nearly orthogonal to "from" 
    float c1, c2, c3; // coefficients for later use 
    int i, j;

    x[0] = (from[0] >= 0.0f)? from[0] : -from[0];
    x[1] = (from[1] >= 0.0f)? from[1] : -from[1];
    x[2] = (from[2] >= 0.0f)? from[2] : -from[2];

    if (x[0] < x[1]) 
    {
      if (x[0] < x[2]) 
      {
        x[0] = 1.0f; x[1] = x[2] = 0.0f;
      }
      else 
      {
        x[2] = 1.0f; x[0] = x[1] = 0.0f;
      }
    }
    else 
    {
      if (x[1] < x[2]) 
      {
        x[1] = 1.0f; x[0] = x[2] = 0.0f;
      }
      else 
      {
        x[2] = 1.0f; x[0] = x[1] = 0.0f;
      }
    }

    u[0] = x[0] - from[0]; u[1] = x[1] - from[1]; u[2] = x[2] - from[2];
    v[0] = x[0] - to[0];   v[1] = x[1] - to[1];   v[2] = x[2] - to[2];

    c1 = 2.0f / DotProduct(u, u);
    c2 = 2.0f / DotProduct(v, v);
    c3 = c1 * c2  * DotProduct(u, v);
    
    for (i = 0; i < 3; i++) 
	{
      for (j = 0; j < 3; j++)
	  {
        mtx[i][j] =  - c1 * u[i] * u[j]
                     - c2 * v[i] * v[j]
                     + c3 * v[i] * u[j];
      }
      mtx[i][i] += 1.0f;
    }
  }
  else  // the most common case, unless "from"="to", or "from"=-"to" 
  {
#if 0
    // unoptimized version - a good compiler will optimize this. 
    h = (1.0f - e)/DOT(v, v);
    mtx[0][0] = e + h * v[0] * v[0];  
    mtx[0][1] = h * v[0] * v[1] - v[2]; 
    mtx[0][2] = h * v[0] * v[2] + v[1];
    
    mtx[1][0] = h * v[0] * v[1] + v[2]; 
    mtx[1][1] = e + h * v[1] * v[1];    
    mtx[1][2] = h * v[1] * v[2] - v[0];

    mtx[2][0] = h * v[0] * v[2] - v[1]; 
    mtx[2][1] = h * v[1] * v[2] + v[0]; 
    mtx[2][2] = e + h * v[2] * v[2];
#else
    // ...otherwise use this hand optimized version (9 mults less) 
    float hvx, hvz, hvxy, hvxz, hvyz;
    h = (1.0f - e)/DotProduct(cross, cross);
    hvx = h * cross[0];
    hvz = h * cross[2];
    hvxy = hvx * cross[1];
    hvxz = hvx * cross[2];
    hvyz = hvz * cross[1];
    mtx[0][0] = e + hvx * cross[0]; 
    mtx[0][1] = hvxy - cross[2];     
    mtx[0][2] = hvxz + cross[1];

    mtx[1][0] = hvxy + cross[2];  
    mtx[1][1] = e + h * cross[1] * cross[1]; 
    mtx[1][2] = hvyz - cross[0];

    mtx[2][0] = hvxz - cross[1];  
    mtx[2][1] = hvyz + cross[0];     
    mtx[2][2] = e + hvz * cross[2];
#endif
  }
  
  
  	mtx.Invert();	 		// Micknote: not sure why we need to invert it, but let it be for now
	
	 
	return mtx;
}
			
			
Matrix&		CreateRotateMatrix ( Matrix& mat, const Vector& axis, const float angle )
{
	

    Vector	unitAxis = axis;
	unitAxis.Normalize();
	
	float oneMinusCosine = 1.0f - cosf( angle );
	
	Vector	leading;

	leading[X] = 1.0f - ( unitAxis[X] * unitAxis[X] );
	leading[Y] = 1.0f - ( unitAxis[Y] * unitAxis[Y] );
	leading[Z] = 1.0f - ( unitAxis[Z] * unitAxis[Z] );
	leading *= oneMinusCosine; 
	
	Vector	crossed;

	crossed[X] = ( unitAxis[Y] * unitAxis[Z] );
	crossed[Y] = ( unitAxis[Z] * unitAxis[X] );
	crossed[Z] = ( unitAxis[X] * unitAxis[Y] );
	crossed *= oneMinusCosine;
	
	unitAxis *= sinf( angle );
		
	mat[RIGHT][X] = 1.0f - leading[X];
	mat[RIGHT][Y] = crossed[Z] + unitAxis[Z];
	mat[RIGHT][Z] = crossed[Y] - unitAxis[Y];
	mat[RIGHT][W] = 0.0f;
	
	mat[UP][X] = crossed[Z] - unitAxis[Z];
	mat[UP][Y] = 1.0f - leading[Y];
	mat[UP][Z] = crossed[X] + unitAxis[X];
	mat[UP][W] = 0.0f;
	
	mat[AT][X] = crossed[Y] + unitAxis[Y];
	mat[AT][Y] = crossed[X] - unitAxis[X];
	mat[AT][Z] = 1.0f - leading[Z];
	mat[AT][W] = 0.0f;
	
	mat[POS][X] = 0.0f;
	mat[POS][Y] = 0.0f;
	mat[POS][Z] = 0.0f;
	mat[POS][W] = 1.0f;

	return mat;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
Matrix& CreateRotateXMatrix( Matrix& rotX, float angle )
{
	

	float	s = sinf( angle );
	float	c = cosf( angle );

	rotX[RIGHT][X] = 1.0f;
	rotX[RIGHT][Y] = 0.0f;
	rotX[RIGHT][Z] = 0.0f;
	rotX[RIGHT][W] = 0.0f;
	
	rotX[UP][X] = 0.0f;
	rotX[UP][Y] = c;
	rotX[UP][Z] = s;
	rotX[UP][W] = 0.0f;
	
	rotX[AT][X] = 0.0f;
	rotX[AT][Y] = -s;
	rotX[AT][Z] = c;
	rotX[AT][W] = 0.0f;
	
	rotX[POS][X] = 0.0f;
	rotX[POS][Y] = 0.0f;
	rotX[POS][Z] = 0.0f;
	rotX[POS][W] = 1.0f;

	return rotX;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
Matrix& CreateRotateYMatrix( Matrix& rotY, float angle )
{
	

	float	s = sinf( angle );
	float	c = cosf( angle );

	rotY[RIGHT][X] = c;
	rotY[RIGHT][Y] = 0.0f;
	rotY[RIGHT][Z] = -s;
	rotY[RIGHT][W] = 0.0f;
	
	rotY[UP][X] = 0.0f;
	rotY[UP][Y] = 1.0f;
	rotY[UP][Z] = 0.0f;
	rotY[UP][W] = 0.0f;
	
	rotY[AT][X] = s;
	rotY[AT][Y] = 0.0f;
	rotY[AT][Z] = c;
	rotY[AT][W] = 0.0f;
	
	rotY[POS][X] = 0.0f;
	rotY[POS][Y] = 0.0f;
	rotY[POS][Z] = 0.0f;
	rotY[POS][W] = 1.0f;

	return rotY;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
Matrix& CreateRotateZMatrix( Matrix& rotZ, float angle )
{
	

	float	s = sinf( angle );
	float	c = cosf( angle );

	rotZ[RIGHT][X] = c;
	rotZ[RIGHT][Y] = s;
	rotZ[RIGHT][Z] = 0.0f;
	rotZ[RIGHT][W] = 0.0f;
	
	rotZ[UP][X] = -s;
	rotZ[UP][Y] = c;
	rotZ[UP][Z] = 0;
	rotZ[UP][W] = 0.0f;
	
	rotZ[AT][X] = 0.0f;
	rotZ[AT][Y] = 0.0f;
	rotZ[AT][Z] = 1.0f;
	rotZ[AT][W] = 0.0f;
	
	rotZ[POS][X] = 0.0f;
	rotZ[POS][Y] = 0.0f;
	rotZ[POS][Z] = 0.0f;
	rotZ[POS][W] = 1.0f;

	return rotZ;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Matrix& CreateRotateMatrix( Matrix &rot, int axis, const float angle )
{
	int		a = (axis + 1) % (Z + 1);
	int		b = (axis + 2) % (Z + 1);

	float	s = sinf( angle );
	float	c = cosf( angle );
	
	rot.Ident();
	
	rot[a][a] = c;
	rot[a][b] = s;
	rot[b][a] = -s;
	rot[b][b] = c;
	
	return rot;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
Matrix& CreateMatrixLookAt( Matrix& mat, const Vector& pos, const Vector& lookat, const Vector& up )
{
	// Create Z axis
	Mth::Vector z_axis(lookat - pos);
	z_axis.Normalize();
	//mat[Z] = z_axis;

	// Create X from Z and up (up assumed to be normalized)
	Mth::Vector x_axis(Mth::CrossProduct(z_axis, up));
	//mat[X] = x_axis;

	// Create Y from Z and X
	Mth::Vector y_axis(Mth::CrossProduct(x_axis, z_axis));
	//mat[Y] = y_axis;

	// Orientation needs transposing (since we want the inverse orientation)
	//mat.Transpose();
	mat[X][X] = x_axis[X];
	mat[Y][X] = x_axis[Y];
	mat[Z][X] = x_axis[Z];

	mat[X][Y] = y_axis[X];
	mat[Y][Y] = y_axis[Y];
	mat[Z][Y] = y_axis[Z];

	mat[X][Z] = z_axis[X];
	mat[Y][Z] = z_axis[Y];
	mat[Z][Z] = z_axis[Z];

	// These may not be zero, but should be
	mat[X][W] = 0.0f;
	mat[Y][W] = 0.0f;
	mat[Z][W] = 0.0f;

	// Create inverse translation
	mat[POS][X] = -Mth::DotProduct(x_axis, pos);
	mat[POS][Y] = -Mth::DotProduct(y_axis, pos);
	mat[POS][Z] = -Mth::DotProduct(z_axis, pos);
	mat[POS][W] = 1.0f;

	//Dbg_Message("LookAt matrix:");
	//Dbg_Message("[ %f, %f, %f, %f ]", mat[X][X], mat[X][Y], mat[X][Z], mat[X][W]);
	//Dbg_Message("[ %f, %f, %f, %f ]", mat[Y][X], mat[Y][Y], mat[Y][Z], mat[Y][W]);
	//Dbg_Message("[ %f, %f, %f, %f ]", mat[Z][X], mat[Z][Y], mat[Z][Z], mat[Z][W]);
	//Dbg_Message("[ %f, %f, %f, %f ]", mat[W][X], mat[W][Y], mat[W][Z], mat[W][W]);

	return mat;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
Matrix& CreateMatrixOrtho( Matrix& mat, float width, float height, float f_near, float f_far )
{
	mat[X] = Mth::Vector(2.0f / width, 0.0f, 0.0f, 0.0f);
	mat[Y] = Mth::Vector(0.0f, 2.0f / height, 0.0f, 0.0f);
	mat[Z] = Mth::Vector(0.0f, 0.0f, 2.0f / (f_far - f_near), 0.0f);
	mat[W] = Mth::Vector(0.0f, 0.0f, -(f_far + f_near) / (f_far - f_near), 1.0f);

	//Dbg_Message("Orthographic matrix:");
	//Dbg_Message("[ %f, %f, %f, %f ]", mat[X][X], mat[X][Y], mat[X][Z], mat[X][W]);
	//Dbg_Message("[ %f, %f, %f, %f ]", mat[Y][X], mat[Y][Y], mat[Y][Z], mat[Y][W]);
	//Dbg_Message("[ %f, %f, %f, %f ]", mat[Z][X], mat[Z][Y], mat[Z][Z], mat[Z][W]);
	//Dbg_Message("[ %f, %f, %f, %f ]", mat[W][X], mat[W][Y], mat[W][Z], mat[W][W]);

	return mat;
}
		
// Orthonormalize a matrix keeping one row r0 unchanged 
// (Mick accepts responsibility for this).					   
Matrix&	Matrix::OrthoNormalizeAbout(int r0)
{
	int r1, r2;
	r1 = r0+1;
	if (r1 == 3)
	{ 
		r1 = 0;
	}
	r2 = r1+1;
	if (r2 == 3)
	{
		r2 = 0;
	}
	// Now regarding Rows r0,r1,r2
	// r0 = r1 x r2	   (implied)
	// r1 = r2 x r0	   (calculate this)
	// r2 = r0 x r1	   (and this)
	//
	// We need to recalculate rows r1 and r2 using the above cross produces
	// however if r0 is close to r2, then the calculation of r1 will be off
	// so it's better to calulate r2 and then r1 
	// the first pair to do will be whichever has the smaller dot product

	if (Abs(DotProduct(*(Vector*)(row[r2]),*(Vector*)(row[r0]))) < Abs(DotProduct(*(Vector*)(row[r0]),*(Vector*)(row[r1]))))
	{		
		*(Vector*)(row[r1]) = Mth::CrossProduct(*(Vector*)(row[r2]),*(Vector*)(row[r0]));
		(*(Vector*)(row[r1])).Normalize();
		*(Vector*)(row[r2]) = Mth::CrossProduct(*(Vector*)(row[r0]),*(Vector*)(row[r1]));
		(*(Vector*)(row[r2])).Normalize();
	}
	else
	{		
		*(Vector*)(row[r2]) = Mth::CrossProduct(*(Vector*)(row[r0]),*(Vector*)(row[r1]));
		(*(Vector*)(row[r2])).Normalize();
		*(Vector*)(row[r1]) = Mth::CrossProduct(*(Vector*)(row[r2]),*(Vector*)(row[r0]));
		(*(Vector*)(row[r1])).Normalize();
	}

	return *this;
	
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Matrix::GetRotationAxisAndAngle( Vector* pAxis, float* pRadians )
{
	// Given a transform matrix (end = start * xform)
	// this will return its rotation axis and angle

	Dbg_Assert( pAxis );
	Dbg_Assert( pRadians );

    float		nTwoSinTheta, nTwoCosTheta;
    Mth::Vector	vTwoSinThetaAxis;

    nTwoCosTheta = row[Mth::RIGHT][X] + row[Mth::UP][Y] + row[Mth::AT][Z] - 1.0f;

	vTwoSinThetaAxis[X] = row[Mth::UP][Z] - row[Mth::AT][Y];
    vTwoSinThetaAxis[Y] = row[Mth::AT][X] - row[Mth::RIGHT][Z];
    vTwoSinThetaAxis[Z] = row[Mth::RIGHT][Y] - row[Mth::UP][X];
    vTwoSinThetaAxis[W] = 1.0f;
  
	// Gary:  There used to be a magic patch added by Dave
	// (basically negating the axis) which made it work with
	// the RW-based Slerp object.  This doesn't seem to be
	// necessary any more, but I'm going to leave it
	// in the code just in case our Slerp stops working...
//  vTwoSinThetaAxis[X] = row[Mth::AT][Y] - row[Mth::UP][Z];
//  vTwoSinThetaAxis[Y] = row[Mth::RIGHT][Z] - row[Mth::AT][X];
//  vTwoSinThetaAxis[Z] = row[Mth::UP][X] - row[Mth::RIGHT][Y];
//  vTwoSinThetaAxis[W] = 1.0f;
	
	nTwoSinTheta = vTwoSinThetaAxis.Length();
    
    if (nTwoSinTheta > 0.0f)
    {
        float  recipLength = (1.0f / (nTwoSinTheta));

        *pAxis = vTwoSinThetaAxis;
        pAxis->Scale( recipLength );
    }
    else
    {
        pAxis->Set( 0.0f, 0.0f, 0.0f );
    }
    
	(*pRadians) = (float)atan2(nTwoSinTheta, nTwoCosTheta);
    if ((nTwoSinTheta <= 0.01f) && (nTwoCosTheta <= 0.0f))
    {
        /*
         * sin theta is 0; cos theta is -1; theta is 180 degrees
         * vTwoSinThetaAxis was degenerate
         * axis will have to be found another way.
         */

		//Vector	vTwoSinThetaAxis;

		/*
		 * Matrix is:
		 * [ [ 2 a_x^2 - 1,  2 a_x a_y,   2 a_x a_z,  0 ]
		 *   [  2 a_x a_y,  2 a_y^2 - 1,  2 a_y a_z,  0 ]
		 *   [  2 a_x a_z,   2 a_y a_z,  2 a_z^2 - 1, 0 ]
		 *   [      0,           0,           0,      1 ] ]
		 * Build axis scaled by 4 * component of maximum absolute value
		 */
	    if (row[Mth::RIGHT][X] > row[Mth::UP][Y])
	    {
	        if (row[Mth::RIGHT][X] > row[Mth::AT][Z])
	        {
	            vTwoSinThetaAxis[X] = 1.0f + row[Mth::RIGHT][X];
	            vTwoSinThetaAxis[X] = vTwoSinThetaAxis[X] + vTwoSinThetaAxis[X];
	            vTwoSinThetaAxis[Y] = row[Mth::RIGHT][Y] + row[Mth::UP][X];
	            vTwoSinThetaAxis[Z] = row[Mth::RIGHT][Z] + row[Mth::AT][X];
	        }
	        else
	        {
	            vTwoSinThetaAxis[Z] = 1.0f + row[Mth::AT][Z];
	            vTwoSinThetaAxis[Z] = vTwoSinThetaAxis[Z] + vTwoSinThetaAxis[Z];
	            vTwoSinThetaAxis[X] = row[Mth::AT][X] + row[Mth::RIGHT][Z];
	            vTwoSinThetaAxis[Y] = row[Mth::AT][Y] + row[Mth::UP][Z];
	        }
	    }
	    else
	    {
	        if (row[Mth::UP][Y] > row[Mth::AT][Z])
	        {
	            vTwoSinThetaAxis[Y] = 1.0f + row[Mth::UP][Y];
	            vTwoSinThetaAxis[Y] = vTwoSinThetaAxis[Y] + vTwoSinThetaAxis[Y];
	            vTwoSinThetaAxis[Z] = row[Mth::UP][Z] + row[Mth::AT][Y];
	            vTwoSinThetaAxis[X] = row[Mth::UP][X] + row[Mth::RIGHT][Y];
	        }
	        else
	        {
	            vTwoSinThetaAxis[Z] = 1.0f + row[Mth::AT][Z];
	            vTwoSinThetaAxis[Z] = vTwoSinThetaAxis[Z] + vTwoSinThetaAxis[Z];
	            vTwoSinThetaAxis[X] = row[Mth::AT][X] + row[Mth::RIGHT][Z];
	            vTwoSinThetaAxis[Y] = row[Mth::AT][Y] + row[Mth::UP][Z];
	        }
	    }

		/*
		 * and normalize the axis
		 */

        *pAxis = vTwoSinThetaAxis;
        pAxis->Normalize();
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Matrix::RotateLocal( const Vector &rot )
{
	if ( rot[ X ] )
	{
		RotateXLocal( rot[ X ] );
	}
	if ( rot[ Y ] )
	{
		RotateYLocal( rot[ Y ] );
	}
	if ( rot[ Z ] )
	{
		RotateZLocal( rot[ Z ] );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool Matrix::PatchOrthogonality (   )
{
	// Given a matrix m which is assumed to be orthonormal, check to see if it is, and if not then fix it up
	// returns true if any patching was done
	
	float lx,ly,lz;
	lx = (*(Vector*)(row[X])).Length();
	ly = (*(Vector*)(row[Y])).Length();
	lz = (*(Vector*)(row[Z])).Length();

	const float	near1 = 0.99;
	
	if (lx < near1)
	{
		if (ly < near1)
		{
			if (lz < near1)
			{
				// everything collapsing to a point!!!
				// just reset to the identity matrix
				Ident();
			}
			else
			{
			   // x and y have collapsed, but Z is okay 
				Ident();
			}
		}
		else
		{
			if (lz < near1)
			{
				// x and z have collapsed, y is okay (most likely situation)			
				Ident();
			}
			else
			{
			   // just x has collapsed, y and z are okay
				*(Vector*)(row[X]) = Mth::CrossProduct(*(Vector*)(row[Y]),*(Vector*)(row[Z]));
				(*(Vector*)(row[X])).Normalize();
			}
		}	
	}
	else
	{
		if (ly < near1)
		{
			if (lz < near1)
			{
				// y and z collapsed, x is okay
				Ident();
			}
			else
			{
			   // y has collapsed, x and Z are okay 
				*(Vector*)(row[Y]) = Mth::CrossProduct(*(Vector*)(row[Z]),*(Vector*)(row[X]));	
				(*(Vector*)(row[Y])).Normalize();
			}
		}
		else
		{
			if (lz < near1)
			{
				// just z has collapsed, x and y is okay
				*(Vector*)(row[Z]) = Mth::CrossProduct(*(Vector*)(row[X]),*(Vector*)(row[Y]));
				(*(Vector*)(row[Z])).Normalize();
			}
			else
			{
			   // nothing has collapsed
			   return false;
			}
		}	
	}
											  
	return true;											  
}

} // namespace Mth


