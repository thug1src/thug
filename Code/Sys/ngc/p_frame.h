#ifndef _FRAME_H_
#define _FRAME_H_

#include <dolphin.h>
#include "p_matrix.h"
#include "p_vector.h"

class NsFrame
{
	NsMatrix	m_model;
public:
				NsFrame			();

	void		setModelMatrix	( NsMatrix * m ) { m_model.copy( *m ); };
	NsMatrix  * getModelMatrix	( void ) { return &m_model; };

	void		translate		( NsVector * v, NsMatrix_Combine c ) { m_model.translate( v, c ); }
	void		rotate			( NsVector * axis, float angle, NsMatrix_Combine c ) { m_model.rotate( axis, angle, c ); }
	void		scale			( NsVector * v, NsMatrix_Combine c ) { m_model.scale( v, c ); }
	void		identity		( void ) { m_model.identity(); }
};

#endif		// _FRAME_H_
