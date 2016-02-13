#include <core/defines.h>
#include <stdio.h>
#include "mikemath.h"
#include "dma.h"
#include "vif.h"
#include "vu1.h"
#include "gif.h"
#include "gs.h"
#include "vu1code.h"

namespace NxPs2
{


//---------------------------------------------------
//		V U   P R I M   C O N S T R U C T I O N
//---------------------------------------------------


// begin

#if 0
// original version
void vu1::BeginPrim(uint FLG, uint Addr)
{
	vif::STCYCL(1,1);
	vif::BeginUNPACK(0, V4_32, FLG, SIGNED, 0);
	gif::BeginTag1(gs::NOP, 1, PACKED, 0, 0, Addr);
}
#else
// a quicker version
void vu1::BeginPrim(uint FLG, uint Addr)
{
	register uint32 *p_dest = (uint32 *)dma::pLoc;
	*p_dest++ = 0x01000101;
	vif::pVifCode = (uint8 *)p_dest;
	*p_dest++ = 0x6C000000 | FLG<<15 | vu1::Loc&0x03FF;
	gif::pTag = (uint8 *)p_dest;
	*p_dest++ = 0x34000000;
	*p_dest++ = 0x10000000 | Addr;
	*p_dest   = 0x0000000F;
	dma::pLoc = (uint8 *)(p_dest+2);
}
#endif

// end

#if 0
// original version
void vu1::EndPrim(uint Eop)
{
	vif::EndUNPACK();
	vif::UnpackSize--;			// a fudge to ignore the giftag which was inside the unpack
	gif::EndTag1(Eop);
}
#else
// a quicker version
void vu1::EndPrim(uint Eop)
{
	register uint unpack_size = (dma::pLoc - vif::pVifCode - 4) >> 4;
	Dbg_MsgAssert(unpack_size>0, ("Error: unpack size 0 not supported for vu prims\n"));
	Dbg_MsgAssert(unpack_size<=256, ("Error: unpack size greater than 256\n"));
	vif::pVifCode[2] = unpack_size;
	((uint32 *)gif::pTag)[0] |= Eop<<15 | (unpack_size-1);
	((uint32 *)gif::pTag)[3] = unpack_size - 1;
	vu1::Loc += unpack_size;
}
#endif


// output vector

void vu1::StoreVec(Vec v)
{
	vif::StoreV4_32F(v[0], v[1], v[2], v[3]);
}


// output matrix

void vu1::StoreMat(Mat m)
{
	#if 1
	// we basically need to copy 16 32 bit words
	register uint32 * source = (uint32*)&m[0][0];
	register uint32 * dest = (uint32*)dma::pLoc;
	// 16 copies
	*dest++ = *source++;
	*dest++ = *source++;
	*dest++ = *source++;
	*dest++ = *source++;
	*dest++ = *source++;
	*dest++ = *source++;
	*dest++ = *source++;
	*dest++ = *source++;
	*dest++ = *source++;
	*dest++ = *source++;
	*dest++ = *source++;
	*dest++ = *source++;
	*dest++ = *source++;
	*dest++ = *source++;
	*dest++ = *source++;
	*dest++ = *source++;
	
	dma::pLoc = (uint8*) dest;
	
	#else
	vif::StoreV4_32F(m[0][0], m[0][1], m[0][2], m[0][3]);
	vif::StoreV4_32F(m[1][0], m[1][1], m[1][2], m[1][3]);
	vif::StoreV4_32F(m[2][0], m[2][1], m[2][2], m[2][3]);
	vif::StoreV4_32F(m[3][0], m[3][1], m[3][2], m[3][3]);
	#endif
}


// routine has a breakpoint for debugging

void vu1::BreakpointPrim(uint FLG, uint Eop)
{
	vif::UNPACK(0, V4_32, 1, FLG, SIGNED, 0);
	gif::Tag1(0, 0, 0, 0, 0, Eop, 0, VU1_ADDR(Breakpoint));
	Loc += 1;
}


//---------------------------------
//		S T A T I C   D A T A
//---------------------------------

uint vu1::Loc;
uint vu1::Buffer;
uint vu1::MaxBuffer;


} // namespace NxPs2

