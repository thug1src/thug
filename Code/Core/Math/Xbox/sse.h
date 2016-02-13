//-----------------------------------------------------------------------------
// File: SSE.h
//
// Desc: P3 SSE conversions and estimates
//
// Hist: 1.7.03 - Created
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef P3_SSE
#define P3_SSE


//-----------------------------------------------------------------------------
// Name: Ftoi_ASM
// Desc: SSE float to int conversion.  Note that no control word needs to be
//       set to round down
//-----------------------------------------------------------------------------
__forceinline int Ftoi_ASM( const float f )
{
    __asm cvttss2si eax, f        // return int(f)
}


//-----------------------------------------------------------------------------
// Name: ReciprocalEstimate_ASM
// Desc: SSE reciprocal estimate, accurate to 12 significant bits of
//       the mantissa
//-----------------------------------------------------------------------------
__forceinline float ReciprocalEstimate_ASM( const float f )
{
    float rec;
    __asm rcpss xmm0, f             // xmm0 = rcpss(f)
    __asm movss rec , xmm0          // return xmm0
    return rec;
}


//-----------------------------------------------------------------------------
// Name: ReciprocalSqrtEstimate_ASM
// Desc: SSE reciprocal square root estimate, accurate to 12 significant
//       bits of the mantissa
//-----------------------------------------------------------------------------
__forceinline float ReciprocalSqrtEstimate_ASM( const float f )
{
    float recsqrt;
    __asm rsqrtss xmm0, f           // xmm0 = rsqrtss(f)
    __asm movss recsqrt, xmm0       // return xmm0
    return recsqrt;
}


//-----------------------------------------------------------------------------
// Name: SqrtEstimae_ASM
// Desc: SSE square root estimate, accurate to 12 significant bits of
//       the mantissa.  Note that a check for zero must be made since
//       sqrt(0) == 0 but 1/sqrt(0) = inf
//-----------------------------------------------------------------------------
__forceinline float SqrtEstimate_ASM( const float f )
{
    float recsqrt;
    __asm movss xmm0,f              // xmm0 = f
    __asm rsqrtss xmm1, xmm0        // xmm1 = rsqrtss(f)
    __asm mulss xmm1, xmm0          // xmm1 = rsqrtss(f) * f = sqrt(f)
    __asm xorps xmm2, xmm2          // xmm2 = 0
    __asm cmpneqss xmm2, xmm0       // xmm2 = (f != 0 ? 1s : 0s)
    __asm andps xmm1, xmm2          // xmm1 = xmm1 & xmm2 
    __asm movss recsqrt, xmm1       // return xmm1
    return recsqrt;
}


//-----------------------------------------------------------------------------
// Name: ReciprocalEstimateNR_ASM
// Desc: SSE Newton-Raphson reciprocal estimate, accurate to 23 significant
//       bits of the mantissa
//       One Newtown-Raphson Iteration:
//           f(i+1) = 2 * rcpss(f) - f * rcpss(f) * rcpss(f)
//-----------------------------------------------------------------------------
__forceinline float ReciprocalEstimateNR_ASM( const float f )
{
    float rec;
    __asm rcpss xmm0, f               // xmm0 = rcpss(f)
    __asm movss xmm1, f               // xmm1 = f
    __asm mulss xmm1, xmm0            // xmm1 = f * rcpss(f)
    __asm mulss xmm1, xmm0            // xmm2 = f * rcpss(f) * rcpss(f)
    __asm addss xmm0, xmm0            // xmm0 = 2 * rcpss(f)
    __asm subss xmm0, xmm1            // xmm0 = 2 * rcpss(f) 
                                      //        - f * rcpss(f) * rcpss(f)
    __asm movss rec, xmm0             // return xmm0
    return rec;
}

//-----------------------------------------------------------------------------
// Newton-Rapson square root iteration constants
//-----------------------------------------------------------------------------
const float g_SqrtNRConst[2] = {0.5f, 3.0f};


//-----------------------------------------------------------------------------
// Name: ReciprocalSqrtEstimateNR_ASM
// Desc: SSE Newton-Raphson reciprocal square root estimate, accurate to 23
//       significant bits of the mantissa
//       One Newtown-Raphson Iteration:
//          f(i+1) = 0.5 * rsqrtss(f) * (3.0 - (f * rsqrtss(f) * rsqrtss(f))
//       NOTE: rsqrtss(f) * rsqrtss(f) != rcpss(f) (presision is not maintained)
//-----------------------------------------------------------------------------
__forceinline float ReciprocalSqrtEstimateNR_ASM( const float f )
{
    float recsqrt;
    __asm rsqrtss xmm0, f             // xmm0 = rsqrtss(f)
    __asm movss xmm1, f               // xmm1 = f
    __asm mulss xmm1, xmm0            // xmm1 = f * rsqrtss(f)
    __asm movss xmm2, g_SqrtNRConst+4 // xmm2 = 3.0f
    __asm mulss xmm1, xmm0            // xmm1 = f * rsqrtss(f) * rsqrtss(f)
    __asm mulss xmm0, g_SqrtNRConst   // xmm0 = 0.5f * rsqrtss(f)
    __asm subss xmm2, xmm1            // xmm2 = 3.0f -
                                      //  f * rsqrtss(f) * rsqrtss(f)
    __asm mulss xmm0, xmm2            // xmm0 = 0.5 * rsqrtss(f)
                                      //  * (3.0 - (f * rsqrtss(f) * rsqrtss(f))
    __asm movss recsqrt, xmm0         // return xmm0
    return recsqrt;
}


//-----------------------------------------------------------------------------
// Name: SqrtEstimateNR_ASM
// Desc: SSE Newton-Raphson square root estimate, accurate to 23 significant
//       bits of the mantissa
//       NOTE: x/sqrt(x) = sqrt(x)
// One Newtown-Raphson Iteration (for 1/sqrt(x)) :
//      f(i+1) = 0.5 * rsqrtss(f) * (3.0 - (f * rsqrtss(f) * rsqrtss(f))
//      NOTE: rsqrtss(f) * rsqrtss(f) != rcpss(f) (presision is not maintained)
//-----------------------------------------------------------------------------
__forceinline float SqrtEstimateNR_ASM( const float f )
{
    float recsqrt;
    __asm rsqrtss xmm0, f             // xmm0 = rsqrtss(f)
    __asm movss xmm1, f               // xmm1 = f
    __asm mulss xmm1, xmm0            // xmm1 = f * rsqrtss(f)
    __asm movss xmm2, g_SqrtNRConst+4 // xmm2 = 3.0f
    __asm mulss xmm1, xmm0            // xmm1 = f * rsqrtss(f) * rsqrtss(f)
    __asm mulss xmm0, g_SqrtNRConst   // xmm0 = 0.5f * rsqrtss(f)
    __asm subss xmm2, xmm1            // xmm2 = 3.0f -
                                      //   f * rsqrtss(f) * rsqrtss(f)
    __asm mulss xmm0, xmm2            // xmm0 = 0.5 * rsqrtss(f)
                                      //  * (3.0 - (f * rsqrtss(f) * rsqrtss(f))
    __asm xorps xmm1, xmm1            // xmm1 = 0
    __asm mulss xmm0, f               // xmm0 = sqrt(f)
    __asm cmpneqss xmm1, f            // xmm1 = (f != 0 ? 1s : 0s)
    __asm andps xmm0, xmm1            // xmm0 = xmm1 & xmm2 
    __asm movss recsqrt, xmm0         // return xmm0
    return recsqrt;
}


#endif // P3_SSE
