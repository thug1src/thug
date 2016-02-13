#ifndef _MATRIX_H_
#define _MATRIX_H_

#include <dolphin.h>
#include "p_vector.h"
//#include "p_quat.h"

typedef enum {
	NsMatrix_Combine_Replace,
	NsMatrix_Combine_Pre,
	NsMatrix_Combine_Post,

	NsMatrix_Combine_Max
} NsMatrix_Combine;

class NsMatrix
{
	Mtx			m_matrix;
public:
	friend class NsVector;
	friend class NsCamera;
	friend class NsMaterialMan;
	friend class NsClump;
	friend class NsDL;

	void		translate		( NsVector * v, NsMatrix_Combine c );
	void		rotate			( NsVector * axis, float angle, NsMatrix_Combine c );
	void		rotateX			( float degrees, NsMatrix_Combine c );
	void		rotateY			( float degrees, NsMatrix_Combine c );
	void		rotateZ			( float degrees, NsMatrix_Combine c );
	void		scale			( NsVector * v, NsMatrix_Combine c );
	void		invert			( void );
	void		invert			( NsMatrix& source );
	void		identity		( void );
	void		transform		( NsMatrix& a, NsMatrix_Combine c );
	void		cat				( NsMatrix& a, NsMatrix& b );

	void		copy			( NsMatrix& source );

	void		getRotation		( NsVector * axis, float * angle, NsVector * center );

	void		lookAt			( NsVector * pPos, NsVector * pUp, NsVector * pTarget );

//	void		fromQuat		( NsQuat * pQuat );

	NsVector  * getRight		( void ) { return (NsVector *)&m_matrix[0][0]; }
	NsVector  * getUp			( void ) { return (NsVector *)&m_matrix[1][0]; }
	NsVector  * getAt			( void ) { return (NsVector *)&m_matrix[2][0]; }

	float		getRightX		( void ) { return m_matrix[0][0]; }
	float		getRightY		( void ) { return m_matrix[0][1]; }
	float		getRightZ		( void ) { return m_matrix[0][2]; }

	float		getUpX			( void ) { return m_matrix[1][0]; }
	float		getUpY			( void ) { return m_matrix[1][1]; }
	float		getUpZ			( void ) { return m_matrix[1][2]; }

	float		getAtX			( void ) { return m_matrix[2][0]; }
	float		getAtY			( void ) { return m_matrix[2][1]; }
	float		getAtZ			( void ) { return m_matrix[2][2]; }

	float		getPosX			( void ) { return m_matrix[0][3]; }
	float		getPosY			( void ) { return m_matrix[1][3]; }
	float		getPosZ			( void ) { return m_matrix[2][3]; }

	void		setRight		( NsVector * p ) { m_matrix[0][0] = p->x; m_matrix[0][1] = p->y; m_matrix[0][2] = p->z; }
	void		setUp			( NsVector * p ) { m_matrix[1][0] = p->x; m_matrix[1][1] = p->y; m_matrix[1][2] = p->z; }
	void		setAt			( NsVector * p ) { m_matrix[2][0] = p->x; m_matrix[2][1] = p->y; m_matrix[2][2] = p->z; }
	void		setPos			( NsVector * p ) { m_matrix[0][3] = p->x; m_matrix[1][3] = p->y; m_matrix[2][3] = p->z; }

	void		setRight		( float x, float y, float z ) { m_matrix[0][0] = x; m_matrix[0][1] = y; m_matrix[0][2] = z; }
	void		setUp			( float x, float y, float z ) { m_matrix[1][0] = x; m_matrix[1][1] = y; m_matrix[1][2] = z; }
	void		setAt			( float x, float y, float z ) { m_matrix[2][0] = x; m_matrix[2][1] = y; m_matrix[2][2] = z; }
	void		setPos			( float x, float y, float z ) { m_matrix[0][3] = x; m_matrix[1][3] = y; m_matrix[2][3] = z; }

	void		setRightX		( float f ) { m_matrix[0][0] = f; }
	void		setUpX			( float f ) { m_matrix[1][0] = f; }
	void		setAtX			( float f ) { m_matrix[2][0] = f; }
	void		setPosX			( float f ) { m_matrix[0][3] = f; }

	void		setRightY		( float f ) { m_matrix[0][1] = f; }
	void		setUpY			( float f ) { m_matrix[1][1] = f; }
	void		setAtY			( float f ) { m_matrix[2][1] = f; }
	void		setPosY			( float f ) { m_matrix[1][3] = f; }

	void		setRightZ		( float f ) { m_matrix[0][2] = f; }
	void		setUpZ			( float f ) { m_matrix[1][2] = f; }
	void		setAtZ			( float f ) { m_matrix[2][2] = f; }
	void		setPosZ			( float f ) { m_matrix[2][3] = f; }

	void		multiply		( NsVector * pSourceBase, NsVector * pDestBase );
	void		multiply		( NsVector * pSourceBase, NsVector * pDestBase, int count );
};

#endif		// _MATRIX_H_
