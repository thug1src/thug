//#include <math.h>
//#include "p_quat.h"
//
//NsQuat::NsQuat()
//{
//	m_quat.x = 0;
//	m_quat.y = 0;
//	m_quat.z = 0;
//	m_quat.w = 0;
//}
//
////***************************************************************************
////* Slerp 2 quaternions 0.0 = all of quat0, 1.0 = all of quat1.
////***************************************************************************
//
//#define _EPSILON          ((float)(0.001))
//#define _TOL_COS_ZERO     (((float)1) - _EPSILON)
//
//#define RwV3dDotProduct(a, b)                              \
//    ((((( (((a).m_quat.x) * ((b).m_quat.x))) +                              \
//        ( (((a).m_quat.y) * ((b).m_quat.y))))) +                            \
//        ( (((a).m_quat.z) * ((b).m_quat.z)))))
//
//void NsQuat::slerp ( NsQuat& pQ0, NsQuat& pQ1, float time )
//{
//	// Compute dot product (equal to cosine of the angle between quaternions).
//	float              fCosTheta = (RwV3dDotProduct(pQ0, pQ1) + pQ0.m_quat.w * pQ1.m_quat.w);
//	float              fAlphaQ = (float)time;
//    float              fBeta;
//    unsigned int       bObtuseTheta;
//    unsigned int       bNearlyZeroTheta;
//
//	// Check angle to see if quaternions are in opposite hemispheres.
//    bObtuseTheta = (fCosTheta < ((float)0.0f ));
//
//    if (bObtuseTheta)
//    {
//        // If so, flip one of the quaterions.
//		fCosTheta = ( fCosTheta < ((float) - 1.0f )) ? ((float)1.0f ) : -fCosTheta;
//
//		pQ1.m_quat.x	= -pQ1.m_quat.x;
//		pQ1.m_quat.y	= -pQ1.m_quat.y;
//		pQ1.m_quat.z	= -pQ1.m_quat.z;
//		pQ1.m_quat.w	= -pQ1.m_quat.w;
//    }
//    else
//    {
//		fCosTheta = ( fCosTheta > ((float) 1.0f )) ? ((float)1.0f ) : fCosTheta;
//    }
//
//	// Set factors to do linear interpolation, as a special case where the
//	// quaternions are close together.
//	fBeta = ((float)1.0f ) - fAlphaQ;
//
//	// If the quaternions aren't close, proceed with spherical interpolation.
//    bNearlyZeroTheta = ( fCosTheta >= _TOL_COS_ZERO );
//
//    if ( !bNearlyZeroTheta )
//    {
//        const float fTheta			= acosf( fCosTheta );
//		const float fCosecTheta	= 1.0f / sinf( fTheta );
//        fBeta						= (float)( sinf( fTheta * fBeta ) * fCosecTheta );
//        fAlphaQ						= (float)( sinf( fTheta * fAlphaQ ) * fCosecTheta );
//    }
//
//	// Do the interpolation.
//	m_quat.x	= fBeta * pQ0.m_quat.x + fAlphaQ * pQ1.m_quat.x;
//	m_quat.y	= fBeta * pQ0.m_quat.y + fAlphaQ * pQ1.m_quat.y;
//	m_quat.z	= fBeta * pQ0.m_quat.z + fAlphaQ * pQ1.m_quat.z;
//	m_quat.w	= fBeta * pQ0.m_quat.w + fAlphaQ * pQ1.m_quat.w;
//}





















