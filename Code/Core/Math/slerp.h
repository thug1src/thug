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
**	File name:		core/math/slerp.h										**
**																			**
**	Created:		12/20/01	-	gj										**
**																			**
**	Description:	Slerp Interpolator Math Class							**
**																			**
*****************************************************************************/

#ifndef __CORE_MATH_SLERP_H
#define __CORE_MATH_SLERP_H

/*****************************************************************************
**								   Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <core/math/vector.h>
#include <core/math/matrix.h>
	
/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Mth
{

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

/****************************************************************************
 *																			
 * Class:			Slerp Interpolator
 *
 * Description:		Slerp Interpolator math class
 *
 ****************************************************************************/

class SlerpInterpolator
{
public:
				SlerpInterpolator();
				SlerpInterpolator( const Matrix* pStart, const Matrix* pEnd );

public:
	void		setMatrices( const Matrix* pStart, const Matrix* pEnd );	
	void		getMatrix( Matrix* result, float delta );
	
	void		invertDirection (   );
					  
	float		getRadians (   ) { return m_radians; }
	const Mth::Vector&	getAxis (   ) { return m_axis; }

protected:
	Matrix	    m_start;
	Matrix	    m_end;
	Vector	    m_axis;
	float		m_radians;
	bool		m_useLerp;  // do linear-interpolation, rather than slerping
};

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

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace Mth

#endif // __CORE_MATH_SLERP_H
