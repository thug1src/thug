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
**	File name:		core/math/Quat.inl										**
**																			**
**	Created:		11/23/99	-	mjb										**
**																			**
**	Description:	Quaternion Math Class									**
**																			**
*****************************************************************************/

#ifndef __CORE_MATH_QUAT_INL
#define __CORE_MATH_QUAT_INL

namespace Mth
{
							  

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const float&		Quat::operator[] ( sint i ) const
{
	
	
	return quat[i];
}


/*****************************************************************************
**							 Private Inline Functions  						**
*****************************************************************************/

inline	bool		quat_equal( const Quat& q1, const Quat& q2, const float tol )
{	
	
	
	return ( Equal( q1.quat, q2.quat, tol ));
}

/*****************************************************************************
**							Public Inline Functions							**
*****************************************************************************/

inline Quat::Quat( float cx, float cy, float cz, float cw )
: quat( cx, cy, cz, cw )
{
	

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Quat::Quat( const Vector& axis, float angle )
{
	

	float	mod = axis.Length();
	float	ang = angle / 2.0f;

	mod  = ( mod > 0.0f ) ? ( 1.0f / mod ) : 0.0f;
	mod *= sinf( ang );
	
	SetVector( mod * axis[X], mod * axis[Y], mod * axis[Z] );
	SetScalar( cosf ( ang ));
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float&			Quat::operator[] ( sint i )
{
	
	
	return quat[i];
}

					
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
				
inline Quat::Quat( const Matrix& m )
{
	
	
	float t = m[X][X] + m[Y][Y] + m[Z][Z];	
	float trace;

	if ( t > 0.0f )																						
	{
		trace = sqrtf( t + 1.0f );
		
		SetScalar( trace * 0.5f );
		trace = 0.5f / trace;
		
		SetVector(( m[Z][Y] - m[Y][Z] ) * trace,
				  ( m[X][Z] - m[Z][X] ) * trace,
				  ( m[Y][X] - m[X][Y] ) * trace );
	}
	else
	{
		// find greatest element in Matrix diagonal
		sint i = X;
		if ( m[Y][Y] > m[X][X] ) i = Y;
		if ( m[Z][Z] > m[i][i] ) i = Z;
		
		sint j = ( i + 1 ) % W;
		sint k = ( j + 1 ) % W;

		trace = sqrtf(( m[i][i] - (m[j][j] + m[k][k] )) + 1.0f );
		
		quat[i]	= ( trace * 0.5f );
		trace	= 0.5f / trace;
		quat[j]	= ( m[j][i] + m[i][j] ) * trace;
		quat[k]	= ( m[k][i] + m[i][k] ) * trace;		
		quat[W]	= ( m[k][j] - m[j][k] ) * trace;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Quat&	Quat::Invert( void ) 							// this = Invert ( this )
{
	

	quat.Negate();

	return	*this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Quat&	Quat::Invert( const Quat& src ) 			// this = Invert ( src )
{
	

	quat.Negate( src.quat );
	quat[W] = src.quat[W];

	return	*this;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float		Quat::Modulus( void )
{
	
	
	return sqrtf( ModulusSqr () );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float		Quat::ModulusSqr ( void ) const
{
	
	
	return (( quat[X] * quat[X] ) +
			( quat[Y] * quat[Y] ) +
			( quat[Z] * quat[Z] ) +
			( quat[W] * quat[W] ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	Quat&	Quat::Normalize( float len )
{
	
	
	float	mod = Modulus();
		
	if ( mod > 0.0f ) 
	{
		mod = len / mod;
		
		quat *= mod;
	}
	
	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	Vector	Quat::Rotate( const Vector& vec ) const
{
	

	Quat	inv;
//	Quat 	pt( vec[X], vec[Y], vec[Z], vec[W] );
	Quat 	pt( vec[X], vec[Y], vec[Z], 1.0f );		// Mick: Setting W to sensible value, otherwise can cause overflow
	Quat 	res = *this;
	
	inv.Invert( *this );	
	res *= pt;
	res *= inv;

	return Vector( res[X], res[Y], res[Z], res[W] );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
			
inline void		Quat::SetScalar( const float w )
{
	
	
	quat[W] = w;
};
			
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void		Quat::SetVector( const float x, const float y, const float z )
{
	
	
	quat[X] = x;
	quat[Y] = y;
	quat[Z] = z;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void		Quat::SetVector( const Vector& v )
{
	
	
	quat[X] = v[X];
	quat[Y] = v[Y];
	quat[Z] = v[Z];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Quat& Quat::operator= ( const Quat& q )
{	
	
	
	quat = q.quat;

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Quat& Quat::operator+= ( const Quat& q )
{	
	
	
	quat += q.quat;

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Quat& Quat::operator-= ( const Quat& q )
{	
	
	
	quat -= q.quat;

	return *this;
}


inline const	float&	Quat::GetScalar ( void ) const
{
	return quat[W];
} 

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Quat& Quat::operator*= ( const Quat& q )
{	
	
	
	float s1 = GetScalar();
	float s2 = q.GetScalar();

	Vector v1 = GetVector();
	Vector v2 = q.GetVector();

	SetVector (( v2 * s1 ) + ( v1 * s2 ) + CrossProduct ( v1, v2 ));
	SetScalar (( s1 * s2 ) - DotProduct ( v1, v2 ));

	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	Quat&	Quat::operator*= ( float s )
{
	
	
	quat *= s;
	
	return	*this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	Quat&	Quat::operator/= ( float s )
{
	
	
	quat /= s;
	
	return	*this;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void		Quat::GetMatrix ( Matrix& mat ) const
{
	

	float 	xs, ys, zs,
			wx,	wy,	wz,
			xx,	xy,	xz,
			yy,	yz,	zz, ss;


#if 0 
	// our original version.  Seems essentially broken, as LengthSqr ignores W				  
	ss = 2.0f / quat.LengthSqr();
#else							  
	// version suggested by Andre at Left Field
	// uses proper Modulus, and clamps 2.0/0.0f to zero
	ss = ModulusSqr();
	ss = ( ss>0.0f ? 2.0f/ss : 0.0f);
#endif

	xs = quat[X] * ss;	
	ys = quat[Y] * ss;	
	zs = quat[Z] * ss;

	wx = quat[W] * xs;
	wy = quat[W] * ys;	
	wz = quat[W] * zs;

	xx = quat[X] * xs;	
	xy = quat[X] * ys;	
	xz = quat[X] * zs;

	yy = quat[Y] * ys;	
	yz = quat[Y] * zs;	

	zz = quat[Z] * zs;

	mat[X][X] = 1.0f - (yy + zz);
	mat[Y][X] = xy + wz;
	mat[Z][X] = xz - wy;

	mat[X][Y] = xy - wz;
	mat[Y][Y] = 1.0f - (xx + zz);
	mat[Z][Y] = yz + wx;

	mat[X][Z] = xz + wy;
	mat[Y][Z] = yz - wx;
	mat[Z][Z] = 1.0f - (xx + yy );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Quat	operator+	( const Quat& q1, const Quat& q2 )
{
	
		
	Quat	sum = q1;

	sum += q2;

	return sum;
}    

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Quat	operator-	( const Quat& q1, const Quat& q2 )
{
	
		
	Quat	diff = q1;

	diff -= q2;

	return diff;
}    

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Quat	operator*	( const Quat& q1, const Quat& q2 )
{
	
		
	Quat	prod = q1;

	prod *=	q2;

	return prod;
}    

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Quat	operator*	( const Quat& q, const float s )
{
	Quat	prod = q;

	prod *=	s;

	return prod;
} 


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Quat	operator*	( const float s, const Quat& q )			
{
	return q * s; 
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	Quat operator-	( const Quat& q )
{
	
	
	return Quat ( -q[X], -q[Y], -q[Z], -q[W] );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	float	DotProduct ( const Quat& v1, const Quat& v2 ) 
{
	
	
	return ( v1[X] * v2[X] ) + ( v1[Y] * v2[Y] ) + ( v1[Z] * v2[Z] ) + ( v1[W] * v2[W] );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Quat Slerp ( const Quat& q1, const Quat& q3, float t )
{
	float	sclp, sclq;
		
	// GJ:  Need to possibly negate the second quaternion,
	// to avoid spinning in the wrong direction.
	Quat q2 = q3;
	if ( DotProduct( q1, q3 ) < 0.0f )
	{
		q2 = -q2;
	}

	float	coso = q1.GetScalar() * q2.GetScalar() + 
				   DotProduct ( q1.GetVector(), q2.GetVector());

	Quat	q;

	if (( 1.0f + coso + 1.0f ) > EPSILON )
	{ 
		if (( 1.0f - coso ) > EPSILON )   	// slerp
		{  
			float 	omega = acosf ( coso );
			float	sino  = sinf ( omega );

			sclp  = sinf (( 1.0f - t ) * omega ) / sino;
			sclq  = sinf ( t * omega ) / sino;
		}
		else   									// lerp ( angle tends to 0 )
		{  
			sclp = 1.0f - t;
			sclq = t;
		}

		q = ( sclp * q1 ) + ( sclq * q2 );
	}
	else 										// angle tends to 2*PI
	{ 
		q.SetVector ( -q1[Y], q1[X], -q1[W] );
		q.SetScalar ( q1[Z] );

		sclp = sinf (( 1.0f - t ) * PI / 2.0f );
		sclq = sinf ( t * PI / 2.0f );

		q.SetVector (( sclp * q1.GetVector()) + ( sclq * q.GetVector()));
	}

	return q;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	bool		Equal ( const Quat& q1, const Quat& q2, const float tol )
{	
	
	
	if (( quat_equal (  q1, q2, tol )) ||
		( quat_equal ( -q1, q2, tol )))
	{
		return true;
	}
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline ostream& operator<< ( ostream& os, const Quat& q ) 
{
	return os << "(( " << q[X] << ", " << q[Y] << ", " << q[Z] << ") , " << q[W] << " )";
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void QuatVecToMatrix( Mth::Quat* pQ, Mth::Vector* pT, Mth::Matrix* pMatrix )
{
	Dbg_Assert( pQ );
   	Dbg_Assert( pT );
	Dbg_Assert( pMatrix );

	pQ->Invert();

	Mth::Vector		square;
	Mth::Vector		cross;
	Mth::Vector		wimag;

	square[X] = (*pQ)[X] * (*pQ)[X];
	square[Y] = (*pQ)[Y] * (*pQ)[Y];
	square[Z] = (*pQ)[Z] * (*pQ)[Z];

	cross[X] = (*pQ)[Y] * (*pQ)[Z];
	cross[Y] = (*pQ)[Z] * (*pQ)[X];
	cross[Z] = (*pQ)[X] * (*pQ)[Y];

	wimag[X] = (*pQ)[W] * (*pQ)[X];
	wimag[Y] = (*pQ)[W] * (*pQ)[Y];
	wimag[Z] = (*pQ)[W] * (*pQ)[Z];

	(*pMatrix)[Mth::RIGHT][X] = 1 - 2 * (square[Y] + square[Z]);
	(*pMatrix)[Mth::RIGHT][Y] = 2 * (cross[Z] + wimag[Z]);
	(*pMatrix)[Mth::RIGHT][Z] = 2 * (cross[Y] - wimag[Y]);
	(*pMatrix)[Mth::RIGHT][W] = 0.0f;

	(*pMatrix)[Mth::UP][X] = 2 * (cross[Z] - wimag[Z]);
	(*pMatrix)[Mth::UP][Y] = 1 - 2 * (square[X] + square[Z]);
	(*pMatrix)[Mth::UP][Z] = 2 * (cross[X] + wimag[X]);
	(*pMatrix)[Mth::UP][W] = 0.0f;

	(*pMatrix)[Mth::AT][X] = 2 * (cross[Y] + wimag[Y]);
	(*pMatrix)[Mth::AT][Y] = 2 * (cross[X] - wimag[X]);
	(*pMatrix)[Mth::AT][Z] = 1 - 2 * (square[X] + square[Y]);
	(*pMatrix)[Mth::AT][W] = 0.0f;

	(*pMatrix)[Mth::POS] = *pT;
	(*pMatrix)[Mth::POS][W] = 1.0f;
}

inline void SCacheQuatVecToMatrix( Mth::Quat* pQ, Mth::Vector* pT, Mth::Matrix* pMatrix )
{
	Dbg_Assert( pQ );
   	Dbg_Assert( pT );
	Dbg_Assert( pMatrix );

	pQ->Invert();

	Mth::Vector	*p_square = ((Mth::Vector*)0x7000000)+0;
	Mth::Vector	*p_cross  = ((Mth::Vector*)0x7000000)+1;
	Mth::Vector	*p_wimag  = ((Mth::Vector*)0x7000000)+2;

	(*p_square)[X] = (*pQ)[X] * (*pQ)[X];
	(*p_square)[Y] = (*pQ)[Y] * (*pQ)[Y];
	(*p_square)[Z] = (*pQ)[Z] * (*pQ)[Z];

	(*p_cross)[X] = (*pQ)[Y] * (*pQ)[Z];
	(*p_cross)[Y] = (*pQ)[Z] * (*pQ)[X];
	(*p_cross)[Z] = (*pQ)[X] * (*pQ)[Y];

	(*p_wimag)[X] = (*pQ)[W] * (*pQ)[X];
	(*p_wimag)[Y] = (*pQ)[W] * (*pQ)[Y];
	(*p_wimag)[Z] = (*pQ)[W] * (*pQ)[Z];

	(*pMatrix)[Mth::RIGHT][X] = 1 - 2 * ((*p_square)[Y] + (*p_square)[Z]);
	(*pMatrix)[Mth::RIGHT][Y] = 2 * ((*p_cross)[Z] + (*p_wimag)[Z]);
	(*pMatrix)[Mth::RIGHT][Z] = 2 * ((*p_cross)[Y] - (*p_wimag)[Y]);
	(*pMatrix)[Mth::RIGHT][W] = 0.0f;

	(*pMatrix)[Mth::UP][X] = 2 * ((*p_cross)[Z] - (*p_wimag)[Z]);
	(*pMatrix)[Mth::UP][Y] = 1 - 2 * ((*p_square)[X] + (*p_square)[Z]);
	(*pMatrix)[Mth::UP][Z] = 2 * ((*p_cross)[X] + (*p_wimag)[X]);
	(*pMatrix)[Mth::UP][W] = 0.0f;

	(*pMatrix)[Mth::AT][X] = 2 * ((*p_cross)[Y] + (*p_wimag)[Y]);
	(*pMatrix)[Mth::AT][Y] = 2 * ((*p_cross)[X] - (*p_wimag)[X]);
	(*pMatrix)[Mth::AT][Z] = 1 - 2 * ((*p_square)[X] + (*p_square)[Y]);
	(*pMatrix)[Mth::AT][W] = 0.0f;

	(*pMatrix)[Mth::POS] = *pT;
	(*pMatrix)[Mth::POS][W] = 1.0f;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void	Slerp ( Matrix& result, const Matrix& s1, const Matrix& s2, float t )
{
	Slerp ( Quat(s1), Quat(s2), t).GetMatrix ( result );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline float counter_warp(float t, float cos_alpha) 
{
    const float ATTENUATION = 0.82279687f;
    const float WORST_CASE_SLOPE = 0.58549219f;

    float factor = 1.0f - ATTENUATION * cos_alpha;
    factor *= factor;
    float k = WORST_CASE_SLOPE * factor;

    return t*(k*t*(2.0f*t - 3.0f) + 1.0f + k);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Mth::Quat FastSlerp( Mth::Quat& qIn1, Mth::Quat& qIn2, const float t )
{
	if ( Mth::DotProduct( qIn1, qIn2 ) < 0.0f )
	{
		qIn2 = -qIn2;
	}

#	if 0
	float cos_a = v1[X]*v2[X] + v1[Y]*v2[Y] + v1[Z]*v2[Z] + v1[W]*v2[W];
	if (t <= 0.5f)
	{
		t = counter_warp(t, cos_a);
	} 
	else
	{
		t = 1.0f - counter_warp(1.0f - t, cos_a);
	}
#	endif	

	float	xxx_x	= qIn1[X] + ( qIn2[X] - qIn1[X] ) * t;
	float	xxx_y	= qIn1[Y] + ( qIn2[Y] - qIn1[Y] ) * t;
	float	xxx_z	= qIn1[Z] + ( qIn2[Z] - qIn1[Z] ) * t;
	float	xxx_w	= qIn1[W] + ( qIn2[W] - qIn1[W] ) * t;

	float	len		= 1.0f / sqrtf( xxx_x * xxx_x + xxx_y * xxx_y + xxx_z * xxx_z + xxx_w * xxx_w );

	qIn2.SetVector( xxx_x * len, xxx_y * len, xxx_z * len );
	qIn2.SetScalar( xxx_w * len );

	return qIn2;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Mth::Quat EulerToQuat( const Mth::Vector& euler )
{
	// This code is a modified version of Left Field's
	// MakeQuatFromEuler() function.

	Mth::Quat q;

	float roll = euler[X];
	float pitch = euler[Y];
	float yaw = euler[Z];

	float cyaw, cpitch, croll, syaw, spitch, sroll;
	float cyawcpitch, syawspitch, cyawspitch, syawcpitch;

	cyaw = cosf(0.5f * yaw);
	cpitch = cosf(0.5f * pitch);
	croll = cosf(0.5f * roll);

	syaw = sinf(0.5f * yaw);
	spitch = sinf(0.5f * pitch);
	sroll = sinf(0.5f * roll);

	cyawcpitch = cyaw * cpitch;
	syawspitch = syaw * spitch;
	cyawspitch = cyaw * spitch;
	syawcpitch = syaw * cpitch;

	q[W] = cyawcpitch * croll + syawspitch * sroll;
	q[X] = cyawcpitch * sroll - syawspitch * croll;
	q[Y] = cyawspitch * croll + syawcpitch * sroll;
	q[Z] = syawcpitch * croll - cyawspitch * sroll;

	return q;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mth

#endif // __CORE_MATH_QUAT_INL

