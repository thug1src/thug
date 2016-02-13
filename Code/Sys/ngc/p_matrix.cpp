/********************************************************************************
 *																				*
 *	Module:																		*
 *				NsMatrix														*
 *	Description:																*
 *				Matrix functionality.											*
 *	Written by:																	*
 *				Paul Robinson													*
 *	Copyright:																	*
 *				2001 Neversoft Entertainment - All rights reserved.				*
 *																				*
 ********************************************************************************/

/********************************************************************************
 * Includes.																	*
 ********************************************************************************/
#include <math.h>
#include <string.h>
#include "p_frame.h"
#include "p_matrix.h"
#include <core/defines.h>
#include <core/math/math.h>

/********************************************************************************
 * Defines.																		*
 ********************************************************************************/

/********************************************************************************
 * Structures.																	*
 ********************************************************************************/

/********************************************************************************
 * Local variables.																*
 ********************************************************************************/

/********************************************************************************
 * Forward references.															*
 ********************************************************************************/

/********************************************************************************
 * Externs.																		*
 ********************************************************************************/

void NsMatrix::translate ( NsVector * v, NsMatrix_Combine c )
{
	Mtx temp;
    switch (c) {
        case NsMatrix_Combine_Replace:
			MTXTrans( m_matrix, v->x, v->y, v->z );
			break;
        case NsMatrix_Combine_Pre:
			MTXTrans( temp, v->x, v->y, v->z );
			MTXConcat( m_matrix, temp, m_matrix );
			break;
        case NsMatrix_Combine_Post:
			MTXTrans( temp, v->x, v->y, v->z );
			MTXConcat( temp, m_matrix, m_matrix );
			break;
        default:
			break;
    }
}

void NsMatrix::rotate ( NsVector * axis, float angle, NsMatrix_Combine c )
{
	Mtx temp;

    switch (c) {
        case NsMatrix_Combine_Replace:
			MTXRotAxisDeg( m_matrix, (Vec*)axis, angle );
			break;
        case NsMatrix_Combine_Pre:
			MTXRotAxisDeg( temp, (Vec*)axis, angle );
			MTXConcat( m_matrix, temp, m_matrix );
			break;
        case NsMatrix_Combine_Post:
			MTXRotAxisDeg( temp, (Vec*)axis, angle );
			MTXConcat( temp, m_matrix, m_matrix );
			break;
        default:
			break;
    }
}

void NsMatrix::rotateX( float degrees, NsMatrix_Combine c )
{
	Mtx temp;

    switch (c) {
        case NsMatrix_Combine_Replace:
		    MTXRotDeg(m_matrix, 'x', degrees );
			break;
        case NsMatrix_Combine_Pre:
		    MTXRotDeg(temp, 'x', degrees );
			MTXConcat( m_matrix, temp, m_matrix );
			break;
        case NsMatrix_Combine_Post:
		    MTXRotDeg(temp, 'x', degrees );
			MTXConcat( temp, m_matrix, m_matrix );
			break;
        default:
			break;
    }
}

void NsMatrix::rotateY( float degrees, NsMatrix_Combine c )
{
	Mtx temp;

    switch (c) {
        case NsMatrix_Combine_Replace:
		    MTXRotDeg(m_matrix, 'y', degrees );
			break;
        case NsMatrix_Combine_Pre:
		    MTXRotDeg(temp, 'y', degrees );
			MTXConcat( m_matrix, temp, m_matrix );
			break;
        case NsMatrix_Combine_Post:
		    MTXRotDeg(temp, 'y', degrees );
			MTXConcat( temp, m_matrix, m_matrix );
			break;
        default:
			break;
    }
}

void NsMatrix::rotateZ( float degrees, NsMatrix_Combine c )
{
	Mtx temp;

    switch (c) {
        case NsMatrix_Combine_Replace:
		    MTXRotDeg(m_matrix, 'z', degrees );
			break;
        case NsMatrix_Combine_Pre:
		    MTXRotDeg(temp, 'z', degrees );
			MTXConcat( m_matrix, temp, m_matrix );
			break;
        case NsMatrix_Combine_Post:
		    MTXRotDeg(temp, 'z', degrees );
			MTXConcat( temp, m_matrix, m_matrix );
			break;
        default:
			break;
    }
}

void NsMatrix::scale ( NsVector * v, NsMatrix_Combine c )
{
	Mtx temp;
    switch (c) {
        case NsMatrix_Combine_Replace:
			MTXScale( m_matrix, v->x, v->y, v->z );
			break;
        case NsMatrix_Combine_Pre:
			MTXScale( temp, v->x, v->y, v->z );
			MTXConcat( m_matrix, temp, m_matrix );
			break;
        case NsMatrix_Combine_Post:
			MTXScale( temp, v->x, v->y, v->z );
			MTXConcat( temp, m_matrix, m_matrix );
			break;
        default:
			break;
    }
}

void NsMatrix::identity ( void )
{
	MTXIdentity( m_matrix );
}

void NsMatrix::getRotation( NsVector * axis, float * angle, NsVector * center )
{
    float		nTwoSinTheta, nTwoCosTheta;
    NsVector	vTwoSinThetaAxis;
    float		nRadians;
//    Mtx		mISubMat;
//    Mtx		mISubMatInverse;


    nTwoCosTheta = getRight()->x + getUp()->y + getAt()->z - 1.0f;

	//	Dave 08/07/01 - Unsure as to exactly why the negation of the axis fixes the slerping problem
	// (getRotation() is called from NsSlerp::SetMatrices(), but it does. Investigate further at some point...
//	vTwoSinThetaAxis.x = getUp()->z - getAt()->y;
//	vTwoSinThetaAxis.y = getAt()->x - getRight()->z;
//	vTwoSinThetaAxis.z = getRight()->y - getUp()->x;
    vTwoSinThetaAxis.x = getAt()->y - getUp()->z;
    vTwoSinThetaAxis.y = getRight()->z - getAt()->x;
    vTwoSinThetaAxis.z = getUp()->x - getRight()->y;

	nTwoSinTheta = vTwoSinThetaAxis.length();
    if (nTwoSinTheta > 0.0f)
    {
        float  recipLength = (1.0f / (nTwoSinTheta));

		axis->scale( vTwoSinThetaAxis, recipLength );
    }
    else
    {
        axis->set( 0.0f, 0.0f, 0.0f );
    }
    nRadians = atan2(nTwoSinTheta, nTwoCosTheta);
    (*angle) = nRadians * (((float) 180) / ((float) Mth::PI));
    if ((nTwoSinTheta <= 0.01f) && (nTwoCosTheta <= 0.0f))
    {
        /*
         * sin theta is 0; cos theta is -1; theta is 180 degrees
         * vTwoSinThetaAxis was degenerate
         * axis will have to be found another way.
         */

		NsVector	vTwoSinThetaAxis;

		/*
		 * Matrix is:
		 * [ [ 2 a_x^2 - 1,  2 a_x a_y,   2 a_x a_z,  0 ]
		 *   [  2 a_x a_y,  2 a_y^2 - 1,  2 a_y a_z,  0 ]
		 *   [  2 a_x a_z,   2 a_y a_z,  2 a_z^2 - 1, 0 ]
		 *   [      0,           0,           0,      1 ] ]
		 * Build axis scaled by 4 * component of maximum absolute value
		 */
	    if (getRight()->x > getUp()->y)
	    {
	        if (getRight()->x > getAt()->z)
	        {
	            vTwoSinThetaAxis.x = 1.0f + getRight()->x;
	            vTwoSinThetaAxis.x = vTwoSinThetaAxis.x + vTwoSinThetaAxis.x;
	            vTwoSinThetaAxis.y = getRight()->y + getUp()->x;
	            vTwoSinThetaAxis.z = getRight()->z + getAt()->x;
	        }
	        else
	        {
	            vTwoSinThetaAxis.z = 1.0f + getAt()->z;
	            vTwoSinThetaAxis.z = vTwoSinThetaAxis.z + vTwoSinThetaAxis.z;
	            vTwoSinThetaAxis.x = getAt()->x + getRight()->z;
	            vTwoSinThetaAxis.y = getAt()->y + getUp()->z;
	        }
	    }
	    else
	    {
	        if (getUp()->y > getAt()->z)
	        {
	            vTwoSinThetaAxis.y = 1.0f + getUp()->y;
	            vTwoSinThetaAxis.y = vTwoSinThetaAxis.y + vTwoSinThetaAxis.y;
	            vTwoSinThetaAxis.z = getUp()->z + getAt()->y;
	            vTwoSinThetaAxis.x = getUp()->x + getRight()->y;
	        }
	        else
	        {
	            vTwoSinThetaAxis.z = 1.0f + getAt()->z;
	            vTwoSinThetaAxis.z = (vTwoSinThetaAxis.z + vTwoSinThetaAxis.z);
	            vTwoSinThetaAxis.x = getAt()->x + getRight()->z;
	            vTwoSinThetaAxis.y = getAt()->y + getUp()->z;
	        }
	    }

		/*
		 * and normalize the axis
		 */

		axis->normalize( vTwoSinThetaAxis );
    }

//    /*
//     * Find center of line of rotation:
//     * [ v - c ] * R + c   = v * R + s
//     * -> v * R + c - c *  R = v * R + s
//     * -> c * [ I - R ]      = s
//     * -> c                  = s * [ I - R ] ^ -1
//     */
//
//    MtxSetIdentity(&mISubMat);
//    /*
//     * Find [ I - R ] ^ -1
//     */
//    VecSubMacro(&mISubMat.right, &mISubMat.right, &matrix->right);
//    VecSubMacro(&mISubMat.up, &mISubMat.up, &matrix->up);
//    VecSubMacro(&mISubMat.at, &mISubMat.at, &matrix->at);
//    MtxSetFlags(&mISubMat, GENERIC_FLAGS(&mISubMatInverse));
//    MtxSetFlags(&mISubMatInverse, GENERIC_FLAGS(&mISubMatInverse));
//    /* MatrixInvertOrthoNormalized(&mISubMatInverse, &mISubMat); */
//    MatrixInvertGeneric(&mISubMatInverse, &mISubMat);
//    /*
//     * Find s * [ I - R ] ^ -1
//     */
//    *center = mISubMatInverse.pos;
//    VecIncrementScaledMacro(center, &mISubMatInverse.right, m[0][3]);
//    VecIncrementScaledMacro(center, &mISubMatInverse.up, m[1][3]);
//    VecIncrementScaledMacro(center, &mISubMatInverse.at, m[2][3]);
//
//    RWRETURN(matrix);
}

void NsMatrix::transform ( NsMatrix& a, NsMatrix_Combine c )
{
    switch (c) {
        case NsMatrix_Combine_Replace:
			copy( a );
			break;
        case NsMatrix_Combine_Pre:
			MTXConcat( m_matrix, a.m_matrix, m_matrix );
			break;
        case NsMatrix_Combine_Post:
			MTXConcat( a.m_matrix, m_matrix, m_matrix );
			break;
        default:
			break;
    }
}

void NsMatrix::cat ( NsMatrix& a, NsMatrix& b )
{
	MTXConcat( a.m_matrix, b.m_matrix, m_matrix );
}

void NsMatrix::invert( void )
{
	MTXInverse( m_matrix, m_matrix );
}

void NsMatrix::invert( NsMatrix& source )
{
	MTXInverse( source.m_matrix, m_matrix );
}

void NsMatrix::copy( NsMatrix& source )
{
	memcpy( this, &source, sizeof( NsMatrix ) );
}

void NsMatrix::lookAt ( NsVector * pPos, NsVector * pUp, NsVector * pTarget )
{
	MTXLookAt( m_matrix, (Vec*)pPos, (Vec*)pUp, (Vec*)pTarget );
}

//void NsMatrix::fromQuat ( NsQuat * pQuat )
//{
//	MTXQuat ( m_matrix, &pQuat->m_quat );
//}

void NsMatrix::multiply( NsVector * pSourceBase, NsVector * pDestBase )
{
	MTXMultVec( m_matrix, (Vec*)pSourceBase, (Vec*)pDestBase );
}

void NsMatrix::multiply( NsVector * pSourceBase, NsVector * pDestBase, int count )
{
	MTXMultVecArray( m_matrix, (Vec*)pSourceBase, (Vec*)pDestBase, count );
}
