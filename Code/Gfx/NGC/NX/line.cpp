#include "line.h"
#include "render.h"


namespace NxPs2
{


void BeginLines2D(uint32 rgba)
{
//	// dma tag
//	dma::BeginTag(dma::cnt, 0);
//
//	vu1::Buffer = vu1::Loc;
//
//	// GS context
//	gs::BeginPrim(ABS,0,0);
//	gs::Reg1(gs::ALPHA_1, PackALPHA(0,1,0,1,0));
//	gs::Reg1(gs::RGBAQ,   (uint64)rgba);
//	gs::EndPrim(0);
//
//	// unpack giftag
//	vif::UNPACK(0, V4_32, 1, ABS, UNSIGNED, 0);
//	gif::BeginTagImmediate(gs::XYZ2, 1, PACKED, LINE|ABE, 1, VU1_ADDR(GSPrim));
//
//	// begin unpack for coordinates
//	vif::BeginUNPACK(0, V4_32, ABS, SIGNED, 1);
}



void DrawLine2D(float x0, float y0, float z0, float x1, float y1, float z1)
{
//	vif::StoreV4_32(0x6C00+(sint32)(x0*16),0x7200+(sint32)(y0*16),0xFFFFFF,0);
//	vif::StoreV4_32(0x6C00+(sint32)(x1*16),0x7200+(sint32)(y1*16),0xFFFFFF,0);
//
//	if (((dma::pLoc - gif::pTag)>>4) >= 250)
//	{
//		vif::EndUNPACK();
//		gif::EndTagImmediate(1,0);
//		vif::MSCAL(VU1_ADDR(Parser));
//		vif::UNPACK(0, V4_32, 1, ABS, UNSIGNED, 0);
//		gif::BeginTagImmediate(gs::XYZ2, 1, PACKED, LINE|ABE, 1, VU1_ADDR(GSPrim));
//		vif::BeginUNPACK(0, V4_32, ABS, SIGNED, 1);
//	}
//
}


void EndLines2D(void)
{
//	vif::EndUNPACK();
//	gif::EndTagImmediate(1,0);
//	vif::MSCAL(VU1_ADDR(Parser));
//	dma::EndTag();
}



void BeginLines3D(uint32 rgba)
{
//	// dma tag
//	dma::BeginTag(dma::cnt, 0);
//
//	vu1::Buffer = vu1::Loc;
//
//	// VU context
//	vu1::BeginPrim(ABS, VU1_ADDR(L_VF10));
//	vu1::StoreVec(ViewportScale);		// VF10
//	vu1::StoreVec(ViewportOffset);		// VF11
//	vu1::StoreMat(WorldToFrustum);		// VF12-15
//	vu1::EndPrim(0);
//
//	// GS context
//	gs::BeginPrim(ABS,0,0);
//	gs::Reg1(gs::ALPHA_1, PackALPHA(0,1,0,1,0));
//	gs::Reg1(gs::RGBAQ,   (uint64)rgba);
//	gs::EndPrim(0);
//
//	// all lines will be rendered with simple culling
//	vif::ITOP(CULL);
//
//	// unpack giftag
//	vif::UNPACK(0, V4_32, 1, ABS, UNSIGNED, 0);
//	gif::BeginTagImmediate(gs::XYZ2, 1, PACKED, LINE|ABE, 1, VU1_ADDR(ProjLine));
//
//	// offset mode
//	vif::STMOD(1);
//
//	// begin unpack for coordinates
//	vif::BeginUNPACK(0, V4_32, ABS, SIGNED, 1);
}


void DrawLine3D(float x0, float y0, float z0, float x1, float y1, float z1)
{
//	vif::StoreV4_32((sint32)(x0*256.0f), (sint32)(y0*256.0f), (sint32)(z0*256.0f), 0);
//	vif::StoreV4_32((sint32)(x1*256.0f), (sint32)(y1*256.0f), (sint32)(z1*256.0f), 0);
//
//	if (((dma::pLoc - gif::pTag)>>4) >= 250)
//	{
//		vif::EndUNPACK();
//		gif::EndTagImmediate(1,1);
//		vif::STMOD(0);
//		vif::MSCAL(VU1_ADDR(Parser));
//		vif::UNPACK(0, V4_32, 1, ABS, UNSIGNED, 0);
//		gif::BeginTagImmediate(gs::XYZ2, 1, PACKED, LINE|ABE, 1, VU1_ADDR(ProjLine));
//		vif::STMOD(1);
//		vif::BeginUNPACK(0, V4_32, ABS, SIGNED, 1);
//	}
//
}


void EndLines3D(void)
{
//	vif::EndUNPACK();
//
//	if (vif::UnpackSize)
//	{
//		gif::EndTagImmediate(1,1);
//		vif::STMOD(0);
//		vif::MSCAL(VU1_ADDR(Parser));
//	}
//	else
//		dma::pLoc -= 24;
//
//	dma::EndTag();
}

} // namespace NxPs2

