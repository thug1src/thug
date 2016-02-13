

//--------------------------------------------------------------
//		V E C T O R   A N D   M A T R I X   R O U T I N E S
//--------------------------------------------------------------

					 
#include <core/math.h>
#include <core/defines.h>
#include "mikemath.h"

namespace NxPs2
{


// copy a vector
// vd = vs
void VecCopy(Vec vd, Vec vs)
{
	vd[0]=vs[0], vd[1]=vs[1], vd[2]=vs[2], vd[3]=vs[3];
}


// set a vector
// v = (x,y,z,w)
void VecSet(Vec v, float x, float y, float z, float w)
{
	v[0]=x, v[1]=y, v[2]=z, v[3]=w;
}


// add two vectors to make another
// vd = v0+v1
void VecSum(Vec vd, Vec v0, Vec v1)
{
	int i;
	for (i=0; i<4; i++)
		vd[i] = v0[i]+v1[i];
}


// add one vector onto another
// vd += vs
void VecAdd(Vec vd, Vec vs)
{
	int i;
	for (i=0; i<4; i++)
		vd[i] += vs[i];
}


// subtract two vectors to make another
// vd = v0-v1
void VecDifference(Vec vd, Vec v0, Vec v1)
{
	int i;
	for (i=0; i<4; i++)
		vd[i] = v0[i]-v1[i];
}


// subtract one vector from another
// vd -= vs
void VecSubtract(Vec vd, Vec vs)
{
	int i;
	for (i=0; i<4; i++)
		vd[i] -= vs[i];
}


// negate a vector
// v = -v
void VecNegate(Vec v)
{
	int i;
	for (i=0; i<4; i++)
		v[i] = -v[i];
}


// multiply a vector by a matrix
// v = v * M
void VecTransform(Vec v, Mat M)
{
	int i,j;
	float sum;
	Vec TempVec;
	for (i=0; i<4; i++)
	{
		sum = 0.0;
		for (j=0; j<4; j++)
			sum += v[j]*M[j][i];
		TempVec[i] = sum;
	}
	VecCopy(v,TempVec);
}


// product of a vector by a matrix
// vd = vs * M
void VecTransformed(Vec vd, Vec vs, Mat M)
{
	int i,j;
	float sum;
	for (i=0; i<4; i++)
	{
		sum = 0.0;
		for (j=0; j<4; j++)
			sum += vs[j]*M[j][i];
		vd[i] = sum;
	}
}


// rotate a vector through an angle about an axis
void VecRotate(Vec v, float Angle, char Axis)
{
	Mat M;
	MatRotation(M, Angle, Axis);
	VecTransform(v, M);
}


// normalise a vector
// v = v / |v|
void VecNormalise(Vec v)
{
	int i;
	float RecipLength = 1.0 / sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2] + v[3]*v[3]);
	for (i=0; i<4; i++)
		v[i] *= RecipLength;
}


// scale a vector
// v *= k
void VecScale(Vec v, float k)
{
	int i;
	for (i=0; i<4; i++)
		v[i] *= k;
}


// scaled vector
// vd = k*vs
void VecScaled(Vec vd, float k, Vec vs)
{
	int i;
	for (i=0; i<4; i++)
		vd[i] = k*vs[i];
}


// add scaled vector
// vd += k*vs
void VecAddScaled(Vec vd, float k, Vec vs)
{
	int i;
	for (i=0; i<4; i++)
		vd[i] += k*vs[i];
}


// weighted mean of 2 vectors
// v = (k0*v0 + k1*v1)/(k0+k1)
void VecWeightedMean2(Vec v, float k0, Vec v0, float k1, Vec v1)
{
	Vec Temp;
	VecScaled(v,k0,v0);
	VecScaled(Temp,k1,v1);
	VecAdd(v,Temp);
	VecScale(v,1.0f/(k0+k1));
}


// weighted mean of 3 vectors
// v = (k0*v0 + k1*v1 + k2*v2)/(k0+k1+k2)
void VecWeightedMean3(Vec v, float k0, Vec v0, float k1, Vec v1, float k2, Vec v2)
{
	Vec Temp;
	VecScaled(v,k0,v0);
	VecScaled(Temp,k1,v1);
	VecAdd(v,Temp);
	VecScaled(Temp,k2,v2);
	VecAdd(v,Temp);
	VecScale(v,1.0f/(k0+k1+k2));
}


// cross product
void CrossProduct(Vec vd, Vec v0, Vec v1)
{
	vd[0] = v0[1]*v1[2] - v0[2]*v1[1];
	vd[1] = v0[2]*v1[0] - v0[0]*v1[2];
	vd[2] = v0[0]*v1[1] - v0[1]*v1[0];
}


// dot product (3 components)
float DotProduct3(Vec v0, Vec v1)
{
	return v0[0]*v1[0] + v0[1]*v1[1] + v0[2]*v1[2];
}


// dot product (4 components)
float DotProduct4(Vec v0, Vec v1)
{
	return v0[0]*v1[0] + v0[1]*v1[1] + v0[2]*v1[2] + v0[3]*v1[3];
}


// length of a vector (3 components)
float VecLength3(Vec v)
{
	return sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
}


// copy a matrix
// Md = Ms
//void MatCopy(Mat Md, Mat Ms)
//{
//    int i,j;
//    for (i=0; i<4; i++)
//        for (j=0; j<4; j++)
//            Md[i][j] = Ms[i][j];
//	*(Mth::Matrix*)Md = *(Mth::Matrix*)Ms;
//}


// set a matrix
// M = ((m00,m01,m02,m03), (m10,m11,m12,m13), (m20,m21,m22,m23), (m30,m31,m32,m33))
void MatSet(Mat M, float m00, float m01, float m02, float m03,
				   float m10, float m11, float m12, float m13,
				   float m20, float m21, float m22, float m23,
				   float m30, float m31, float m32, float m33)
{
	M[0][0]=m00, M[0][1]=m01, M[0][2]=m02, M[0][3]=m03;
	M[1][0]=m10, M[1][1]=m11, M[1][2]=m12, M[1][3]=m13;
	M[2][0]=m20, M[2][1]=m21, M[2][2]=m22, M[2][3]=m23;
	M[3][0]=m30, M[3][1]=m31, M[3][2]=m32, M[3][3]=m33;
}




// zero a matrix
// set M to 4x4 zero matrix
void MatZero(Mat M)
{
    int i,j;
    for (i=0; i<4; i++)
        for (j=0; j<4; j++)
            M[i][j] = 0.0;
}


// set identity matrix
// set M to 4x4 identity
void MatIdentity(Mat M)
{
    int i,j;
    for (i=0; i<4; i++)
        for (j=0; j<4; j++)
            M[i][j] = (i==j) ? 1.0 : 0.0;
}


// transpose a matrix
// M = MT
void MatTranspose(Mat M)
{
	float temp;
	int i,j;
	for (i=0; i<4; i++)
		for (j=i+1; j<4; j++)
			temp=M[i][j], M[i][j]=M[j][i], M[j][i]=temp;
}


// transpose a matrix
// Md = MsT
void MatTransposed(Mat Md, Mat Ms)
{
	int i,j;
	for (i=0; i<4; i++)
		for (j=0; j<4; j++)
			Md[i][j]=Ms[j][i];
}


// make a diagonal matrix from the elements of v
void MatDiagonal(Mat M, Vec v)
{
	int i;
	MatZero(M);
	for (i=0; i<4; i++)
		M[i][i] = v[i];
}


// scale all elements of a matrix
void MatScale(Mat M, float k)
{
	int i,j;
	for (i=0; i<4; i++)
		for (j=0; j<4; j++)
			M[i][j] *= k;
}


// set M to matrix for rotation through Angle (radians) about Axis ('x','y' or 'z')
void MatRotation(Mat M, float Angle, char Axis)
{
    float s,c;
    int i0=(int)((Axis+1-'x'))%3, i1=(int)((Axis+2-'x'))%3;
    s=sinf(Angle), c=cosf(Angle);
    MatIdentity(M);
    M[i0][i0] = M[i1][i1] = c;
    M[i1][i0] = -(M[i0][i1] = s);
}


// set M to matrix for translation by v
void MatTranslation(Mat M, Vec v)
{
    MatIdentity(M);
    VecCopy(M[3],v);
}


// multiply two matrices to make another
// set AB = A * B
void MatProduct(Mat AB, Mat A, Mat B)
{
	
	#if 1	
	sceVu0MulMatrix((sceVu0FVECTOR*)AB,(sceVu0FVECTOR*)B,(sceVu0FVECTOR*)A);   
	return;
	#else 
	
	float sum;
    int i,j,k;
    for (i=0; i<4; i++)
        for (j=0; j<4; j++)
        {
            sum=0.0;
            for (k=0; k<4; k++)
                sum += A[i][k]*B[k][j];
            AB[i][j] = sum;
        }
	#endif		
}


// premulitplication of one matrix by another
// A = B * A
void MatPremultiply(Mat A, Mat B)
{
    Mat TempMat;
    MatProduct(TempMat,B,A);
    MatCopy(A,TempMat);
}


// postmultiplication of one matrix by another
// A = A * B
void MatPostmultiply(Mat A, Mat B)
{
    Mat TempMat;
    MatProduct(TempMat,A,B);
    MatCopy(A,TempMat);
}


// invert a 4x4 matrix
// Md = inv(Ms)
// Note: this is not a general matrix invert: it only works for matrices which incorporate rotation and translation.
void MatInverse(Mat Md, Mat Ms)
{
	int i,j;
	Mat InvT, InvR;
	// set up negative of original translation
	MatIdentity(InvT);
	InvT[3][0] = -Ms[3][0];
	InvT[3][1] = -Ms[3][1];
	InvT[3][2] = -Ms[3][2];
	// set up transpose of original rotation
	MatIdentity(InvR);
	for (i=0; i<3; i++)
		for (j=0; j<3; j++)
			InvR[i][j] = Ms[j][i];
	// take product
	MatProduct(Md,InvT,InvR);
}


// build a matrix from 3 Euler angles
// M = Rz(Angles[2]) * Rx(Angles[0]) * Ry(Angles[1])
void MatEuler(Mat M, Vec Angles)
{
	Mat Temp;
	MatRotation(M, Angles[2], 'z');
	MatRotation(Temp, Angles[0], 'x');
	MatPostmultiply(M, Temp);
	MatRotation(Temp, Angles[1], 'y');
	MatPostmultiply(M, Temp);
}

} // namespace NxPs2

