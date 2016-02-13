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
**	File name:		core/math/slerp.cpp										**
**																			**
**	Created:		12/20/01	-	gj										**
**																			**
**	Description:	Slerp Interpolator Math Class							**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/math.h>
#include <core/math/slerp.h>

namespace Mth
{
	
/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

const float USE_LERP_INSTEAD_DEGREES = 2.0f;
const float USE_LERP_INSTEAD_RADIANS = USE_LERP_INSTEAD_DEGREES * PI / 180.0f;

/*****************************************************************************
**								DBG Defines									**
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
**								Public Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

SlerpInterpolator::SlerpInterpolator()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

SlerpInterpolator::SlerpInterpolator( const Matrix * start, const Matrix * end )
{
	setMatrices( start, end );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void SlerpInterpolator::setMatrices( const Matrix * start, const Matrix * end )
{
	Matrix	inv;

    m_start = *start;
    m_end = *end;

	// Calculate the inverse transformation.
    inv = m_start;
    inv.Invert();
    inv = inv * m_end;
	
    // Get the axis and angle.
    inv.GetRotationAxisAndAngle( &m_axis, &m_radians );
    
#if 0    
    // Debugging stuff
    static int print_x_times = 50;

    if ( print_x_times > 0 )
    {
        print_x_times--;
        printf("Start mat\n");
        m_start.PrintContents();
        printf("Inv mat\n");
        inv.PrintContents();
        printf("End mat\n");
        m_end.PrintContents();
        printf("Start * Inv mat\n");
        Mth::Matrix siMat = m_start;
        siMat = siMat * inv;
        siMat.PrintContents();
        {
            printf("Rotated mat w=0\n");
            Mth::Matrix tempMatrix = m_start;
            m_axis[W] = 0.0f;
            tempMatrix.Rotate( m_axis, m_radians );
            tempMatrix.PrintContents();
        }
        {
            printf("Rotated mat w=1\n");
            Mth::Matrix tempMatrix2 = m_start;
            m_axis[W] = 1.0f;
            tempMatrix2.Rotate( m_axis, m_radians );
            tempMatrix2.PrintContents();
        }
        {
            printf("Rotated local mat w=0\n");
            Mth::Matrix tempMatrix = m_start;
            m_axis[W] = 0.0f;
            tempMatrix.RotateLocal( m_axis, m_radians );
            tempMatrix.PrintContents();
        }
        {
            printf("Rotated local mat w=1\n");
            Mth::Matrix tempMatrix2 = m_start;
            m_axis[W] = 1.0f;
            tempMatrix2.RotateLocal( m_axis, m_radians );
            tempMatrix2.PrintContents();
        }
        printf( "Slerp axis=(%f %f %f %f) angle=%f\n", m_axis[X], m_axis[Y], m_axis[Z], m_axis[W], m_radians );
    }
    else
    {
        Dbg_Assert( 0 );
    }
#endif    

	// If angle is too small, use lerp.
	m_useLerp = m_radians < USE_LERP_INSTEAD_RADIANS;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void SlerpInterpolator::getMatrix( Matrix * result, float delta )
{
    /* Cap and floor the delta */
    /* If we are at one end the solution is easy */
    if (delta <= 0.0f)
    {
        // delta = 0.0f;
        *result = m_start;
		return;
    }
    else if (delta >= 1.0f)
    {
        // delta = 1.0f;
        *result = m_end;
		return;
    }

#if 0
    // GJ:  always lerp, used while slerp was being debugged
//    m_useLerp = true;
#endif

    /* Do the lerp if we are, else... */
    if ( m_useLerp )
    {
        /* Get the lerp matrix */
		Matrix	lerp;
		Vector	lpos;
		Vector	spos;
		Vector	epos;
		Vector	rpos;

		lerp.Ident();
		
		spos = m_start[Mth::POS];
        epos = m_end[Mth::POS];

        lerp[Mth::RIGHT] = m_end[Mth::RIGHT] - m_start[Mth::RIGHT];
        lerp[Mth::UP] = m_end[Mth::UP] - m_start[Mth::UP];
        lerp[Mth::AT] = m_end[Mth::AT] - m_start[Mth::AT];
        lpos = epos - spos;

        /* Do lerp */
        lerp[Mth::RIGHT].Scale( delta );
        lerp[Mth::UP].Scale( delta );
        lerp[Mth::AT].Scale( delta );
        lpos.Scale( delta );        

        (*result)[Mth::RIGHT] = m_start[Mth::RIGHT] + lerp[Mth::RIGHT];
        (*result)[Mth::UP] = m_start[Mth::UP] + lerp[Mth::UP];
        (*result)[Mth::AT] = m_start[Mth::AT] + lerp[Mth::AT];        
        rpos = spos + lpos;        
        
		(*result)[Mth::RIGHT].Normalize();
		(*result)[Mth::UP].Normalize();
		(*result)[Mth::AT].Normalize();        
        
        (*result)[Mth::POS] = rpos;  
    }
    else
    {
		Vector	rpos;
		Vector	spos;
		Vector	epos;
        Vector  lpos;

		spos = m_start[Mth::POS];
        epos = m_end[Mth::POS];

        /* Remove the translation for now */
        *result = m_start;
        (*result)[Mth::POS] = Mth::Vector( 0.0f, 0.0f, 0.0f );

        /* Rotate the new matrix */
//      m_axis[W] = 0.0f;
        result->Rotate( m_axis, m_radians * delta );

        /* Do linear interpolation on position */
        lpos = epos - spos;
        lpos.Scale( delta );        
        rpos = spos + lpos;        

        (*result)[Mth::POS] = rpos;
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void SlerpInterpolator::invertDirection (   )
{
	m_axis = -m_axis;
	m_radians = (2.0f * Mth::PI) - m_radians;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Obj

