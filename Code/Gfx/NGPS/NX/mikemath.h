#ifndef __MIKEMATH_H
#define __MIKEMATH_H

#include <core/defines.h>
#include <core/math.h>

namespace NxPs2
{

// typedefs
typedef float Vec[4] nAlign(128);		// Structurally equivalent to Mth::Vector
typedef float Mat[4][4] nAlign(128);	// Structurally equivalent to Mth::Matrix

// function prototypes
void VecCopy(Vec vd, Vec vs);
void VecSet(Vec v, float x, float y, float z, float w);
void VecSum(Vec vd, Vec v0, Vec v1);
void VecAdd(Vec vd, Vec vs);
void VecDifference(Vec vd, Vec v0, Vec v1);
void VecSubtract(Vec vd, Vec vs);
void VecNegate(Vec v);
void VecTransform(Vec v, Mat M);
void VecTransformed(Vec vd, Vec vs, Mat M);
void VecRotate(Vec v, float Angle, char Axis);
void VecNormalise(Vec v);
void VecScale(Vec v, float k);
void VecScaled(Vec vd, float k, Vec vs);
void VecAddScaled(Vec vd, float k, Vec vs);
void VecWeightedMean2(Vec v, float k0, Vec v0, float k1, Vec v1);
void VecWeightedMean3(Vec v, float k0, Vec v0, float k1, Vec v1, float k2, Vec v2);
void CrossProduct(Vec vd, Vec v0, Vec v1);
float DotProduct3(Vec v0, Vec v1);
float DotProduct4(Vec v0, Vec v1);
float VecLength3(Vec v);
void MatCopy(Mat Md, Mat Ms);
void MatSet(Mat M, float m00, float m01, float m02, float m03,
				   float m10, float m11, float m12, float m13,
				   float m20, float m21, float m22, float m23,
				   float m30, float m31, float m32, float m33);
void MatZero(Mat M);
void MatIdentity(Mat M);
void MatTranspose(Mat M);
void MatTransposed(Mat Md, Mat Ms);
void MatDiagonal(Mat M, Vec v);
void MatScale(Mat M, float k);
void MatRotation(Mat M, float Angle, char Axis);
void MatTranslation(Mat M, Vec v);
void MatProduct(Mat AB, Mat A, Mat B);
void MatPremultiply(Mat A, Mat B);
void MatPostmultiply(Mat A, Mat B);
void MatInverse(Mat Md, Mat Ms);
void MatEuler(Mat M, Vec Angles);


inline void MatCopy(Mat Md, Mat Ms)
{
	*(Mth::Matrix*)Md = *(Mth::Matrix*)Ms;
}


} // namespace NxPs2
					  
					  
#endif // __MIKEMATH_H
