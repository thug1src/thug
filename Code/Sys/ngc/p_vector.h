#ifndef _VECTOR_H_
#define _VECTOR_H_

#include <dolphin.h>

class NsVector
{
public:
	float	x;
	float	y;
	float	z;

			NsVector	();
			NsVector	( float sx, float sy, float sz );

	void	add			( NsVector& a );
	void	add			( NsVector& a, NsVector& b );
	void	cross		( NsVector& a );
	void	cross		( NsVector& a, NsVector& b );
	float	length		( void );
	float	distance	( NsVector& a );
	float	dot			( NsVector& a );
	void	normalize	( void );
	void	normalize	( NsVector& a );
	void	reflect		( NsVector& normal );
	void	reflect		( NsVector& a, NsVector& normal );
	void	scale		( float scale );
	void	scale		( NsVector& a, float scale );
	void	sub			( NsVector& a );
	void	sub			( NsVector& a, NsVector& b );
	void	lerp		( NsVector& a, NsVector& b, float t );

	void	set			( float sx, float sy, float sz ) { x = sx; y = sy; z = sz; }

	void	copy		( NsVector& pSource ) { x = pSource.x; y = pSource.y; z = pSource.z; }
};

#endif		// _VECTOR_H_
