//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       immediate.cpp
//* OWNER:          Garrett Jost
//* CREATION DATE:  7/19/2002
//****************************************************************************

#include <core/defines.h>
#include "gfx/ngps/nx/render.h"
#include "gfx/ngps/nx/dma.h"
#include "gfx/ngps/nx/vif.h"
#include "gfx/ngps/nx/vu1.h"
#include "gfx/ngps/nx/gif.h"
#include "gfx/ngps/nx/gs.h"

#include "gfx/ngps/nx/line.h"
#include "gfx/ngps/nx/vu1code.h"
#include "gfx/ngps/nx/mesh.h"
#include "gfx/ngps/nx/sprite.h"
#include "gfx/ngps/nx/switches.h"

#include "gfx/ngps/nx/immediate.h"

namespace NxPs2
{

uint64 CImmediateMode::sGetTextureBlend( uint32 blend_checksum, int fix )
{
	uint64 rv = PackALPHA(0,0,0,0,0);
	switch ( blend_checksum )
	{
		case 0x54628ed7:		// Blend
			rv = PackALPHA(0,1,0,1,0);
			break;
		case 0x02e58c18:		// Add
			rv = PackALPHA(0,2,0,1,0);
			break;
		case 0xa7fd7d23:		// Sub
		case 0xdea7e576:		// Subtract
			rv = PackALPHA(2,0,0,1,0);
			break;
		case 0x40f44b8a:		// Modulate
			rv = PackALPHA(1,2,0,2,0);
			break;
		case 0x68e77f40:		// Brighten
			rv = PackALPHA(1,2,0,1,0);
			break;
		case 0x18b98905:		// FixBlend
			rv = PackALPHA(0,1,2,1,fix);
			break;
		case 0xa86285a1:		// FixAdd
			rv = PackALPHA(0,2,2,1,fix);
			break;
		case 0x0d7a749a:		// FixSub
		case 0x0eea99ff:		// FixSubtract
			rv = PackALPHA(2,0,2,1,fix);
			break;
		case 0x90b93703:		// FixModulate
			rv = PackALPHA(1,2,2,2,fix);
			break;
		case 0xb8aa03c9:		// FixBrighten
			rv = PackALPHA(1,2,2,1,fix);
			break;
		case 0x515e298e:		// Diffuse
		case 0x806fff30:		// None
			rv = PackALPHA(0,0,0,0,0);
			break;
		default:
			Dbg_MsgAssert(0,("Illegal blend mode specified. Please use (fix)blend/add/sub/modulate/brighten or diffuse/none."));
			break;
	}
	return rv;
}

void CImmediateMode::sViewportInit()
{
	// start a cnt tag
	dma::BeginTag(dma::cnt, 0);
	vif::STROW((int)render::RowRegI[0], (int)render::RowRegI[1], (int)render::RowRegI[2], (int)render::RowRegI[3]);

	// VU context, uploading the view transform data
	vu1::BeginPrim(ABS, VU1_ADDR(L_VF09));	  				// Begin packed register upload to VF10 
	vu1::StoreVec(*(Vec *)&render::AltFrustum);				// VF09
	vu1::StoreVec(*(Vec *)&render::InverseViewportScale);	// VF10
	vu1::StoreVec(*(Vec *)&render::InverseViewportOffset);	// VF11
//	vu1::StoreMat(*(Mat *)&render::AdjustedWorldToFrustum);	// VF12-15
	vu1::StoreMat(*(Mat *)&render::AdjustedWorldToViewport);// VF12-15
	vu1::EndPrim(0);										// End upload

	// GS context
	gs::BeginPrim(ABS,0,0);
	gs::Reg1(gs::CLAMP_1,		PackCLAMP(CLAMP,CLAMP,0,0,0,0));		// may want to make REPEAT an option
	gs::Reg1(gs::FRAME_1,		PackFRAME(FRAME_START,HRES/64,PSMCT32,0xFF000000));
	gs::Reg1(gs::SCISSOR_1,		render::reg_SCISSOR);
	gs::Reg1(gs::XYOFFSET_1,	render::reg_XYOFFSET);
	gs::Reg1(gs::TEST_1, 		PackTEST(0,0,0,0,0,0,1,ZGEQUAL));
	gs::Reg1(gs::COLCLAMP,		PackCOLCLAMP(1));
	gs::Reg1(gs::ZBUF_1,		PackZBUF(ZBUFFER_START,PSMZ24,1));
	gs::EndPrim(0);

	// end the dma tag
	dma::EndTag();
}

void CImmediateMode::sTextureGroupInit(uint unpackFLG)
{
	vif::STROW((int)render::RowRegI[0], (int)render::RowRegI[1], (int)render::RowRegI[2], (int)render::RowRegI[3]);

	// VU context, uploading the view transform data
	vu1::BeginPrim(unpackFLG, VU1_ADDR(L_VF10));	  				// Begin packed register upload to VF10 
	vu1::StoreVec(*(Vec *)&render::InverseViewportScale);	// VF10
	vu1::StoreVec(*(Vec *)&render::InverseViewportOffset);	// VF11
//	vu1::StoreMat(*(Mat *)&render::AdjustedWorldToFrustum);	// VF12-15
	vu1::StoreMat(*(Mat *)&render::AdjustedWorldToViewport);// VF12-15
	vu1::EndPrim(0);										// End upload
}

void CImmediateMode::sSetZPush(float zpush)
{
	Mth::Matrix localToViewport(render::AdjustedWorldToViewport);
	localToViewport[0][2] += localToViewport[0][3] * zpush * 64.0f;
	localToViewport[1][2] += localToViewport[1][3] * zpush * 64.0f;
	localToViewport[2][2] += localToViewport[2][3] * zpush * 64.0f;
	localToViewport[3][2] += localToViewport[3][3] * zpush * 64.0f;

	// VU context, uploading the view transform data
	vu1::BeginPrim(ABS, VU1_ADDR(L_VF12));	  				// Begin packed register upload to VF12 
	vu1::StoreMat(*(Mat *)&localToViewport);				// VF12-15
	vu1::EndPrim(0);										// End upload
}

void CImmediateMode::sClearZPush()
{
	// VU context, uploading the view transform data
	vu1::BeginPrim(ABS, VU1_ADDR(L_VF12));	  				// Begin packed register upload to VF12 
	vu1::StoreMat(*(Mat *)&render::AdjustedWorldToViewport);// VF12-15
	vu1::EndPrim(0);										// End upload
}

void CImmediateMode::sStartPolyDraw( SSingleTexture * p_engine_texture, uint64 blend, uint unpackFLG, bool clip )
{
	// GS context
	gs::BeginPrim(unpackFLG,0,0);
	gs::Reg1(gs::ALPHA_1,		blend);

	if ( p_engine_texture )
	{
		gs::Reg1(gs::TEX0_1,	p_engine_texture->m_RegTEX0);
		gs::Reg1(gs::TEX1_1,	p_engine_texture->m_RegTEX1);
	}
	gs::EndPrim(0);

	// Set render flags.
	vif::ITOP(clip ? CLIP : CULL);
}

//
// Starts a poly draw with pre-packed texture registers (probably stolen from some other dma list)
//
void CImmediateMode::sStartPolyDraw( uint32 * p_packed_texture_regs, int num_texture_regs, uint unpackFLG, bool clip )
{
	// GS context
	if (p_packed_texture_regs)
	{
		gs::BeginPrim(unpackFLG,0,0);
		for (int i = 0; i < num_texture_regs; i++)
		{
			NxPs2::dma::Store32(*(p_packed_texture_regs++));
			NxPs2::dma::Store32(*(p_packed_texture_regs++));
			NxPs2::dma::Store32(*(p_packed_texture_regs++));
		}
		gs::EndPrim(0);
	}

	// Set render flags.
	vif::ITOP(clip ? CLIP : CULL);
}

void CImmediateMode::sDrawQuadTexture( SSingleTexture * p_engine_texture, const Mth::Vector& vert0, const Mth::Vector& vert1,
									   const Mth::Vector& vert2, const Mth::Vector& vert3,
									   uint32 col0, uint32 col1, uint32 col2, uint32 col3 )
{
	// begin a batch of vertices
	// Note: UV must always be 1st, Pos must always be last.
	if ( p_engine_texture )
	{
		//BeginModelPrim(gs::XYZ2<<8|gs::RGBAQ<<4|gs::UV, 3, TRISTRIP|IIP|ABE|TME|FST, 1, VU1_ADDR(Proj));
		vif::STCYCL(1,3);
		vif::UNPACK(0,V4_32,1,ABS,UNSIGNED,0);
		gif::BeginTag1(gs::XYZF2<<8|gs::RGBAQ<<4|gs::UV, 3, PACKED, TRISTRIP|IIP|ABE|TME|FST, 1, VU1_ADDR(Proj));
	}
	else
	{
		//BeginModelPrim(gs::XYZ2<<4|gs::RGBAQ, 2, TRISTRIP|IIP|ABE, 1, VU1_ADDR(Proj));
		vif::STCYCL(1,2);
		vif::UNPACK(0,V4_32,1,ABS,UNSIGNED,0);
		gif::BeginTag1(gs::XYZF2<<4|gs::RGBAQ, 2, PACKED, TRISTRIP|IIP|ABE, 1, VU1_ADDR(Proj));
	}

	int loc = 1;

	if ( p_engine_texture )
	{
		int u = p_engine_texture->GetWidth() << 4;
		int v = p_engine_texture->GetHeight() << 4;

		vif::BeginUNPACK(0, V2_16, ABS, UNSIGNED, loc);
		vif::StoreV2_16(0,0);
		vif::StoreV2_16(u,0);
		vif::StoreV2_16(0,v);
		vif::StoreV2_16(u,v);
		vif::EndUNPACK();
		loc++;
	}

	vif::BeginUNPACK(0, V4_8, ABS, UNSIGNED, loc);
	vif::StoreS_32(col0);
	vif::StoreS_32(col1);
	vif::StoreS_32(col3);
	vif::StoreS_32(col2);
	vif::EndUNPACK();
	loc++;

	vif::STMOD(OFFSET_MODE);
	vif::BeginUNPACK(0, V4_32, ABS, SIGNED, loc);
	vif::StoreV4_32((sint32)(vert0[X]*SUB_INCH_PRECISION), (sint32)(vert0[Y]*SUB_INCH_PRECISION), (sint32)(vert0[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert1[X]*SUB_INCH_PRECISION), (sint32)(vert1[Y]*SUB_INCH_PRECISION), (sint32)(vert1[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert3[X]*SUB_INCH_PRECISION), (sint32)(vert3[Y]*SUB_INCH_PRECISION), (sint32)(vert3[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert2[X]*SUB_INCH_PRECISION), (sint32)(vert2[Y]*SUB_INCH_PRECISION), (sint32)(vert2[Z]*SUB_INCH_PRECISION), 0);
	vif::EndUNPACK();
	loc++;
	vif::STMOD(NORMAL_MODE);

	// finish batch of vertices
	//EndModelMultiPrim();
	gif::EndTag1(1);
	vif::MSCAL(VU1_ADDR(Parser));
}

void CImmediateMode::sDrawTri( const Mth::Vector& vert0, const Mth::Vector& vert1, const Mth::Vector& vert2,
							   uint32 col0, uint32 col1, uint32 col2, uint unpackFLG )
{
	// begin a batch of vertices
	// Note: ST must always be 1st, Pos must always be last.
	//BeginModelPrim(gs::XYZ2<<4|gs::RGBAQ, 2, TRISTRIP|IIP|ABE, 1, VU1_ADDR(Proj));
	vif::STCYCL(1,2);
	vif::UNPACK(0,V4_32,1,unpackFLG,UNSIGNED,0);
	gif::BeginTag1(gs::XYZF2<<4|gs::RGBAQ, 2, PACKED, TRISTRIP|IIP|ABE, 1, VU1_ADDR(Proj));

	int loc = 1;

	vif::BeginUNPACK(0, V4_8, unpackFLG, UNSIGNED, loc);
	vif::StoreS_32(col0);
	vif::StoreS_32(col1);
	vif::StoreS_32(col2);
	vif::EndUNPACK();
	loc++;

	vif::STMOD(OFFSET_MODE);
	vif::BeginUNPACK(0, V4_32, unpackFLG, SIGNED, loc);
	vif::StoreV4_32((sint32)(vert0[X]*SUB_INCH_PRECISION), (sint32)(vert0[Y]*SUB_INCH_PRECISION), (sint32)(vert0[Z]*SUB_INCH_PRECISION), 0xC000);
	vif::StoreV4_32((sint32)(vert1[X]*SUB_INCH_PRECISION), (sint32)(vert1[Y]*SUB_INCH_PRECISION), (sint32)(vert1[Z]*SUB_INCH_PRECISION), 0xC000);
	vif::StoreV4_32((sint32)(vert2[X]*SUB_INCH_PRECISION), (sint32)(vert2[Y]*SUB_INCH_PRECISION), (sint32)(vert2[Z]*SUB_INCH_PRECISION), 0);
	vif::EndUNPACK();
	vif::STMOD(NORMAL_MODE);
	loc++;

	// finish batch of vertices
	//EndModelMultiPrim();
	//EndModelPrim(1);
	gif::EndTag1(1);
	vif::MSCAL(VU1_ADDR(Parser));
}

void CImmediateMode::sDrawTriUV( const Mth::Vector& vert0, const Mth::Vector& vert1, const Mth::Vector& vert2,
								 float u0, float v0, float u1, float v1, float u2, float v2,
								 uint32 col0, uint32 col1, uint32 col2, uint unpackFLG )
{
	// dummy gs context
	gs::BeginPrim(unpackFLG,0,0);
	gs::Reg1(gs::A_D_NOP, 0);
	gs::EndPrim(0);

	// gif tag
	vif::UNPACK(0,V4_32,1,unpackFLG,UNSIGNED,0);
	gif::BeginTag1(gs::XYZF2<<8|gs::RGBAQ<<4|gs::ST, 3, PACKED, TRISTRIP|IIP|ABE|TME, 1, VU1_ADDR(PTex) | (0x6 << 11) /* turns off UV correction */);

	// each vertex has 3 elements
	vif::STCYCL(1,3);

	// ST
	vif::UNPACK(0, V2_32, 3, unpackFLG, UNSIGNED, 1);
	vif::StoreV2_32((sint32) (u0 * 4096.0f), (sint32) (v0 * 4096.0f));
	vif::StoreV2_32((sint32) (u1 * 4096.0f), (sint32) (v1 * 4096.0f));
	vif::StoreV2_32((sint32) (u2 * 4096.0f), (sint32) (v2 * 4096.0f));

	// RGBA
	vif::UNPACK(0, V4_8, 3, unpackFLG, UNSIGNED, 2);
	vif::StoreS_32(col0);
	vif::StoreS_32(col1);
	vif::StoreS_32(col2);

	// XYZ
	vif::STMOD(OFFSET_MODE);
	vif::UNPACK(0, V4_32, 3, unpackFLG, SIGNED, 3);
	vif::StoreV4_32((sint32)(vert0[X]*SUB_INCH_PRECISION), (sint32)(vert0[Y]*SUB_INCH_PRECISION), (sint32)(vert0[Z]*SUB_INCH_PRECISION), 0xC000);
	vif::StoreV4_32((sint32)(vert1[X]*SUB_INCH_PRECISION), (sint32)(vert1[Y]*SUB_INCH_PRECISION), (sint32)(vert1[Z]*SUB_INCH_PRECISION), 0xC000);
	vif::StoreV4_32((sint32)(vert2[X]*SUB_INCH_PRECISION), (sint32)(vert2[Y]*SUB_INCH_PRECISION), (sint32)(vert2[Z]*SUB_INCH_PRECISION), 0);
	vif::STMOD(NORMAL_MODE);

	// finish batch of vertices
	gif::EndTag1(1);
	vif::MSCAL(VU1_ADDR(Parser));
}

void CImmediateMode::sDraw5QuadTexture( SSingleTexture * p_engine_texture, const Mth::Vector& vert0, const Mth::Vector& vert1,
									    const Mth::Vector& vert2, const Mth::Vector& vert3, const Mth::Vector& vert4,
									    uint32 col0, uint32 col1 )
{
	// begin a batch of vertices
	// Note: UV must always be 1st, Pos must always be last.
	if ( p_engine_texture )
	{
		//BeginModelPrim(gs::XYZ2<<8|gs::RGBAQ<<4|gs::UV, 3, TRISTRIP|IIP|ABE|TME|FST, 1, VU1_ADDR(Proj));
		vif::STCYCL(1,3);
		vif::UNPACK(0,V4_32,1,ABS,UNSIGNED,0);
		gif::BeginTag1(gs::XYZF2<<8|gs::RGBAQ<<4|gs::ST, 3, PACKED, TRISTRIP|IIP|ABE|TME|FST, 1, VU1_ADDR(Proj));
	}
	else
	{
		//BeginModelPrim(gs::XYZ2<<4|gs::RGBAQ, 2, TRIFAN|IIP|ABE, 1, VU1_ADDR(Proj));
		vif::STCYCL(1,2);
		vif::UNPACK(0,V4_32,1,ABS,UNSIGNED,0);
		gif::BeginTag1(gs::XYZF2<<4|gs::RGBAQ, 2, PACKED, TRIFAN|IIP|ABE, 1, VU1_ADDR(Proj));
	}

	int loc = 1;

	if ( p_engine_texture )
	{
		int u = p_engine_texture->GetWidth() << 4;
		int v = p_engine_texture->GetHeight() << 4;

		vif::BeginUNPACK(0, V2_16, ABS, UNSIGNED, loc);
		vif::StoreV2_16(u>>1,v>>1);
		vif::StoreV2_16(0,0);
		vif::StoreV2_16(u,0);
		vif::StoreV2_16(0,v);
		vif::StoreV2_16(u,v);
		vif::StoreV2_16(0,0);
		vif::EndUNPACK();
		loc++;
	}

	vif::BeginUNPACK(0, V4_8, ABS, UNSIGNED, loc);
	vif::StoreS_32(col0);
	vif::StoreS_32(col1);
	vif::StoreS_32(col1);
	vif::StoreS_32(col1);
	vif::StoreS_32(col1);
	vif::StoreS_32(col1);
	vif::EndUNPACK();
	loc++;

	vif::STMOD(OFFSET_MODE);
	vif::BeginUNPACK(0, V4_32, ABS, SIGNED, loc);
	vif::StoreV4_32((sint32)(vert0[X]*SUB_INCH_PRECISION), (sint32)(vert0[Y]*SUB_INCH_PRECISION), (sint32)(vert0[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert1[X]*SUB_INCH_PRECISION), (sint32)(vert1[Y]*SUB_INCH_PRECISION), (sint32)(vert1[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert2[X]*SUB_INCH_PRECISION), (sint32)(vert2[Y]*SUB_INCH_PRECISION), (sint32)(vert2[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert3[X]*SUB_INCH_PRECISION), (sint32)(vert3[Y]*SUB_INCH_PRECISION), (sint32)(vert3[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert4[X]*SUB_INCH_PRECISION), (sint32)(vert4[Y]*SUB_INCH_PRECISION), (sint32)(vert4[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert1[X]*SUB_INCH_PRECISION), (sint32)(vert1[Y]*SUB_INCH_PRECISION), (sint32)(vert1[Z]*SUB_INCH_PRECISION), 0);
	vif::EndUNPACK();
	loc++;
	vif::STMOD(NORMAL_MODE);

	// finish batch of vertices
	//EndModelMultiPrim();
	gif::EndTag1(1);
	vif::MSCAL(VU1_ADDR(Parser));
}

void CImmediateMode::sDrawGlowSegment( const Mth::Vector& vert0, const Mth::Vector& vert1,
									   const Mth::Vector& vert2, const Mth::Vector& vert3, const Mth::Vector& vert4,
									   uint32 col0, uint32 col1, uint32 col2 )
{
	// begin a batch of vertices
	// Note: UV must always be 1st, Pos must always be last.
	//BeginModelPrim(gs::XYZ2<<4|gs::RGBAQ, 2, TRISTRIP|IIP|ABE, 1, VU1_ADDR(Proj));
	vif::STCYCL(1,2);
	vif::UNPACK(0,V4_32,1,ABS,UNSIGNED,0);
	gif::BeginTag1(gs::XYZF2<<4|gs::RGBAQ, 2, PACKED, TRISTRIP|IIP|ABE, 1, VU1_ADDR(Proj));

	int loc = 1;

	vif::BeginUNPACK(0, V4_8, ABS, UNSIGNED, loc);
	vif::StoreS_32(col0);
	vif::StoreS_32(col1);
	vif::StoreS_32(col1);
	vif::StoreS_32(col2);
	vif::StoreS_32(col2);
	vif::EndUNPACK();
	loc++;

	vif::STMOD(OFFSET_MODE);
	vif::BeginUNPACK(0, V4_32, ABS, SIGNED, loc);
	vif::StoreV4_32((sint32)(vert0[X]*SUB_INCH_PRECISION), (sint32)(vert0[Y]*SUB_INCH_PRECISION), (sint32)(vert0[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert1[X]*SUB_INCH_PRECISION), (sint32)(vert1[Y]*SUB_INCH_PRECISION), (sint32)(vert1[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert2[X]*SUB_INCH_PRECISION), (sint32)(vert2[Y]*SUB_INCH_PRECISION), (sint32)(vert2[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert3[X]*SUB_INCH_PRECISION), (sint32)(vert3[Y]*SUB_INCH_PRECISION), (sint32)(vert3[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert4[X]*SUB_INCH_PRECISION), (sint32)(vert4[Y]*SUB_INCH_PRECISION), (sint32)(vert4[Z]*SUB_INCH_PRECISION), 0);
	vif::EndUNPACK();
	loc++;
	vif::STMOD(NORMAL_MODE);

	// finish batch of vertices
	//EndModelMultiPrim();
	gif::EndTag1(1);
	vif::MSCAL(VU1_ADDR(Parser));
}

void CImmediateMode::sDrawStarSegment( const Mth::Vector& vert0, const Mth::Vector& vert1,
									   const Mth::Vector& vert2, const Mth::Vector& vert3,
									   uint32 col0, uint32 col1, uint32 col2 )
{
	// begin a batch of vertices
	// Note: UV must always be 1st, Pos must always be last.
	//BeginModelPrim(gs::XYZ2<<4|gs::RGBAQ, 2, TRISTRIP|IIP|ABE, 1, VU1_ADDR(Proj));
	vif::STCYCL(1,2);
	vif::UNPACK(0,V4_32,1,ABS,UNSIGNED,0);
	gif::BeginTag1(gs::XYZF2<<4|gs::RGBAQ, 2, PACKED, TRISTRIP|IIP|ABE, 1, VU1_ADDR(Proj));

	int loc = 1;

	vif::BeginUNPACK(0, V4_8, ABS, UNSIGNED, loc);
	vif::StoreS_32(col0);
	vif::StoreS_32(col1);
	vif::StoreS_32(col1);
	vif::StoreS_32(col2);
	vif::EndUNPACK();
	loc++;

	vif::STMOD(OFFSET_MODE);
	vif::BeginUNPACK(0, V4_32, ABS, SIGNED, loc);
	vif::StoreV4_32((sint32)(vert0[X]*SUB_INCH_PRECISION), (sint32)(vert0[Y]*SUB_INCH_PRECISION), (sint32)(vert0[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert1[X]*SUB_INCH_PRECISION), (sint32)(vert1[Y]*SUB_INCH_PRECISION), (sint32)(vert1[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert2[X]*SUB_INCH_PRECISION), (sint32)(vert2[Y]*SUB_INCH_PRECISION), (sint32)(vert2[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert3[X]*SUB_INCH_PRECISION), (sint32)(vert3[Y]*SUB_INCH_PRECISION), (sint32)(vert3[Z]*SUB_INCH_PRECISION), 0);
	vif::EndUNPACK();
	loc++;
	vif::STMOD(NORMAL_MODE);

	// finish batch of vertices
	//EndModelMultiPrim();
	gif::EndTag1(1);
	vif::MSCAL(VU1_ADDR(Parser));
}

void CImmediateMode::sDrawSmoothStarSegment( const Mth::Vector& vert0, const Mth::Vector& vert1,
											 const Mth::Vector& vert2, const Mth::Vector& vert3, const Mth::Vector& vert4,
											 uint32 col0, uint32 col1, uint32 col2 )
{
	// begin a batch of vertices
	// Note: UV must always be 1st, Pos must always be last.
	//BeginModelPrim(gs::XYZ2<<4|gs::RGBAQ, 2, TRIFAN|IIP|ABE, 1, VU1_ADDR(Proj));
	vif::STCYCL(1,2);
	vif::UNPACK(0,V4_32,1,ABS,UNSIGNED,0);
	gif::BeginTag1(gs::XYZF2<<4|gs::RGBAQ, 2, PACKED, TRIFAN|IIP|ABE, 1, VU1_ADDR(Proj));

	int loc = 1;

	vif::BeginUNPACK(0, V4_8, ABS, UNSIGNED, loc);
	vif::StoreS_32(col1);
	vif::StoreS_32(col0);
	vif::StoreS_32(col2);
	vif::StoreS_32(col2);
	vif::StoreS_32(col2);
	vif::StoreS_32(col0);
	vif::EndUNPACK();
	loc++;

	vif::STMOD(OFFSET_MODE);
	vif::BeginUNPACK(0, V4_32, ABS, SIGNED, loc);
	vif::StoreV4_32((sint32)(vert0[X]*SUB_INCH_PRECISION), (sint32)(vert0[Y]*SUB_INCH_PRECISION), (sint32)(vert0[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert1[X]*SUB_INCH_PRECISION), (sint32)(vert1[Y]*SUB_INCH_PRECISION), (sint32)(vert1[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert2[X]*SUB_INCH_PRECISION), (sint32)(vert2[Y]*SUB_INCH_PRECISION), (sint32)(vert2[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert3[X]*SUB_INCH_PRECISION), (sint32)(vert3[Y]*SUB_INCH_PRECISION), (sint32)(vert3[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert4[X]*SUB_INCH_PRECISION), (sint32)(vert4[Y]*SUB_INCH_PRECISION), (sint32)(vert4[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert1[X]*SUB_INCH_PRECISION), (sint32)(vert1[Y]*SUB_INCH_PRECISION), (sint32)(vert1[Z]*SUB_INCH_PRECISION), 0);
	vif::EndUNPACK();
	loc++;
	vif::STMOD(NORMAL_MODE);

	// finish batch of vertices
	//EndModelMultiPrim();
	gif::EndTag1(1);
	vif::MSCAL(VU1_ADDR(Parser));
}

void CImmediateMode::sDrawLine( const Mth::Vector& vert0, const Mth::Vector& vert1, uint32 col0, uint32 col1 )
{
	// begin a batch of vertices
	// Note: UV must always be 1st, Pos must always be last.
	//BeginModelPrim(gs::XYZ2<<4|gs::RGBAQ, 2, LINE|IIP|ABE, 1, VU1_ADDR(Proj));
	vif::STCYCL(1,2);
	vif::UNPACK(0,V4_32,1,ABS,UNSIGNED,0);
	gif::BeginTag1(gs::XYZ2<<4|gs::RGBAQ, 2, PACKED, LINE|IIP|ABE, 1, VU1_ADDR(Line));

	int loc = 1;

	vif::BeginUNPACK(0, V4_8, ABS, UNSIGNED, loc);
	vif::StoreS_32(col0);
	vif::StoreS_32(col1);
	vif::EndUNPACK();
	loc++;

	vif::STMOD(OFFSET_MODE);
	vif::BeginUNPACK(1, V4_32, ABS, SIGNED, loc);
	vif::StoreV4_32((sint32)(vert0[X]*SUB_INCH_PRECISION), (sint32)(vert0[Y]*SUB_INCH_PRECISION), (sint32)(vert0[Z]*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(vert1[X]*SUB_INCH_PRECISION), (sint32)(vert1[Y]*SUB_INCH_PRECISION), (sint32)(vert1[Z]*SUB_INCH_PRECISION), 0);
	vif::EndUNPACK();
	loc++;
	vif::STMOD(NORMAL_MODE);

	// finish batch of vertices
	//EndModelMultiPrim();
	gif::EndTag1(1);
	vif::MSCAL(VU1_ADDR(Parser));
}

}



