#ifndef __VU_H
#define __VU_H

#include	<core/defines.h>
#include	<core/math.h>

// simplify VU1 call addresses
#ifdef __PLAT_NGPS__
#define VU1_ADDR(Label) ((uint)&(Label)/8)
#endif
#ifdef __PLAT_WN32__
#define VU1_ADDR(Label) (Label)
#endif

namespace NxPs2
{

class vu1
{

public:

	//-------------------------------------------
	//		S T A T I C   F U N C T I O N S
	//-------------------------------------------

	static void BeginPrim(uint FLG, uint Addr);
	static void EndPrim(uint Eop);
	static void BreakpointPrim(uint FLG, uint Eop);
	static void StoreVec(float v[4]);
	static void CopyQuads(uint32 *p0);
	static void CopyQuads(uint32 *p0,uint32 *p1);
	static void CopyQuads(uint32 *p0,uint32 *p1,uint32 *p2);
	static void CopyQuads(uint32 *p0,uint32 *p1,uint32 *p2, uint32 *p3);
	static void StoreVecAligned(float v[4]);
	static void CopyMat(float *p_mat);
	static void CopyMatAligned(float *p_mat);
	static void StoreMat(float m[4][4]);
	static void StoreMatAligned(float m[4][4]);

	//---------------------------------
	//		S T A T I C   D A T A
	//---------------------------------

	static uint Loc;
	static uint Buffer;
	static uint MaxBuffer;

}; // class vu1


#ifdef __PLAT_NGPS__

inline void asm_copy_vec(void* m0, void* m1)
{
	asm __volatile__("
	lq    $6,0x0(%1)
	sq    $6,0x0(%0)
	": : "r" (m0) , "r" (m1):"$6");
}

inline void vu1::StoreVecAligned(float v[4])
{
	asm_copy_vec(dma::pLoc,v);
	dma::pLoc += 16;
}

inline void vu1::StoreMatAligned(float m[4][4])
{
	*(Mth::Matrix*)dma::pLoc = *(Mth::Matrix*)(&m[0][0]);
	dma::pLoc += 64;
}

inline void vu1::CopyMatAligned(float *p_mat)
{
	*(Mth::Matrix*)dma::pLoc = *((Mth::Matrix*)(p_mat));
	dma::pLoc += 64;
}

inline void vu1::CopyMat(float *p_mat)
{
	uint32 *p = (uint32*)dma::pLoc;
	uint32 *p2 = (uint32*)p_mat;
	for (int i=0;i<16;i++)
		*p++ = *p2++;
	dma::pLoc = (uint8*)p;
}



inline void vu1::CopyQuads(uint32 *p0)
{
	uint32	* p = (uint32*)dma::pLoc;
	p[0] = p0[0];
	p[1] = p0[1];
	p[2] = p0[2];
	p[3] = p0[3];
	dma::pLoc = (uint8*)(p+4);
}

inline void vu1::CopyQuads(uint32 *p0, uint32 *p1)
{
	uint32	* p = (uint32*)dma::pLoc;
	p[0] = p0[0];
	p[1] = p0[1];
	p[2] = p0[2];
	p[3] = p0[3];
	p[4] = p1[0];
	p[5] = p1[1];
	p[6] = p1[2];
	p[7] = p1[3];
	dma::pLoc = (uint8*)(p+8);
}

inline void vu1::CopyQuads(uint32 *p0, uint32 *p1, uint32*p2)
{
	uint32	* p = (uint32*)dma::pLoc;
	p[0] = p0[0];
	p[1] = p0[1];
	p[2] = p0[2];
	p[3] = p0[3];
	p[4] = p1[0];
	p[5] = p1[1];
	p[6] = p1[2];
	p[7] = p1[3];
	p[8] = p2[0];
	p[9] = p2[1];
	p[10] = p2[2];
	p[11] = p2[3];
	dma::pLoc = (uint8*)(p+12);
}

inline void vu1::CopyQuads(uint32 *p0, uint32 *p1, uint32 *p2, uint32 *p3)
{
	uint32	* p = (uint32*)dma::pLoc;
	p[0] = p0[0];
	p[1] = p0[1];
	p[2] = p0[2];
	p[3] = p0[3];
	p[4] = p1[0];
	p[5] = p1[1];
	p[6] = p1[2];
	p[7] = p1[3];
	p[8] = p2[0];
	p[9] = p2[1];
	p[10] = p2[2];
	p[11] = p2[3];
	p[12] = p3[0];
	p[13] = p3[1];
	p[14] = p3[2];
	p[15] = p3[3];
	dma::pLoc = (uint8*)(p+16);
}

	
//	static void StoreQuads(uint32 *p0,uint32 *p1);
//	static void StoreQuads(uint32 *p0,uint32 *p1,uint32 *p2);
//	static void StoreQuads(uint32 *p0,uint32 *p1,uint32 *p2, uint32 *p0);



#endif



} // namespace NxPs2



#endif // __VU_H

