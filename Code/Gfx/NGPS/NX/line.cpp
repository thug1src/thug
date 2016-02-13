#include <core/defines.h>
#include "line.h"
#include "render.h"
#include "dma.h"
#include "vif.h"
#include "vu1.h"
#include "gif.h"
#include "gs.h"
#include "sprite.h"
#include "switches.h"
#include "vu1code.h"


namespace NxPs2
{


void BeginLines2D(uint32 rgba)
{
	// dma tag
	dma::BeginTag(dma::cnt, 0);

	vu1::Buffer = vu1::Loc;

	// GS context
	gs::BeginPrim(ABS,0,0);
	gs::Reg1(gs::ALPHA_1, PackALPHA(0,1,0,1,0));
	gs::Reg1(gs::RGBAQ,   (uint64)rgba);
	gs::EndPrim(0);

	// unpack giftag
	vif::UNPACK(0, V4_32, 1, ABS, UNSIGNED, 0);
	gif::BeginTagImmediate(gs::XYZ2, 1, PACKED, LINE|ABE, 1, VU1_ADDR(GSPrim));

	// begin unpack for coordinates
	vif::BeginUNPACK(0, V4_32, ABS, SIGNED, 1);
}



void DrawLine2D(float x0, float y0, float z0, float x1, float y1, float z1)
{
	vif::StoreV4_32(XOFFSET+(sint32)((x0 * SDraw2D::GetScreenScaleX() * 16.0f)),
					YOFFSET+(sint32)((y0 * SDraw2D::GetScreenScaleY() * 16.0f)),0xFFFFFF,0);
	vif::StoreV4_32(XOFFSET+(sint32)((x1 * SDraw2D::GetScreenScaleX() * 16.0f)),
					YOFFSET+(sint32)((y1 * SDraw2D::GetScreenScaleY() * 16.0f)),0xFFFFFF,0);

	if (((dma::pLoc - gif::pTag)>>4) >= 250)
	{
		vif::EndUNPACK();
		gif::EndTagImmediate(1);
		vif::MSCAL(VU1_ADDR(Parser));
		vif::UNPACK(0, V4_32, 1, ABS, UNSIGNED, 0);
		gif::BeginTagImmediate(gs::XYZ2, 1, PACKED, LINE|ABE, 1, VU1_ADDR(GSPrim));
		vif::BeginUNPACK(0, V4_32, ABS, SIGNED, 1);
	}

}


void EndLines2D(void)
{
	vif::EndUNPACK();
	gif::EndTagImmediate(1);
	vif::MSCAL(VU1_ADDR(Parser));
	dma::EndTag();
}



void BeginLines3D(uint32 rgba)
{
	
	// dma tag  - this will encompass ALL the lines up to the next EndLines3D
	dma::BeginTag(dma::cnt, 0);

	vu1::Buffer = vu1::Loc;

	// VU context, uploading the view transform data
	vu1::BeginPrim(ABS, VU1_ADDR(L_VF10));	  	// Begin packed register upload to VF10 
	//vu1::StoreVec(*(Vec *)&render::ViewportScale);			// VF10
	vu1::StoreVec(*(Vec *)&render::InverseIntViewportScale);			// VF10

//	vu1::StoreVec(ViewportOffset);							// VF11

//	float z_push = 2.40741243e-34;							// was e-35
//	vif::StoreV4_32F(render::ViewportOffset[0],
//					 render::ViewportOffset[1],
//					 render::ViewportOffset[2]+z_push,
//					 render::ViewportOffset[3]);			// VF11
	
	Mth::Vector temp_inv_viewport_offset;
	temp_inv_viewport_offset[0] = -render::InverseIntViewportScale[0] * render::IntViewportOffset[0] * render::InverseIntViewportScale[3];
	temp_inv_viewport_offset[1] = -render::InverseIntViewportScale[1] * render::IntViewportOffset[1] * render::InverseIntViewportScale[3];
	temp_inv_viewport_offset[2] = -render::InverseIntViewportScale[2] * (render::IntViewportOffset[2]+1.0e-34f) * render::InverseIntViewportScale[3];
	temp_inv_viewport_offset[3] = 0.0f;
	vu1::StoreVec(*(Vec *)&temp_inv_viewport_offset);

	vu1::StoreMat(*(Mat *)&render::AdjustedWorldToIntViewport);	// VF12-15
	vu1::EndPrim(0);										// End upload

	// GS context, setting color
	gs::BeginPrim(ABS,0,0);
	gs::Reg1(gs::ALPHA_1, PackALPHA(0,1,0,1,0));
	gs::Reg1(gs::RGBAQ,   (uint64)rgba);
	gs::EndPrim(0);

	// all lines will be rendered with simple culling
	vif::ITOP(CULL);

	// unpack giftag
	vif::UNPACK(0, V4_32, 1, ABS, UNSIGNED, 0);
	gif::BeginTagImmediate(gs::XYZ2, 1, PACKED, LINE|ABE, 1, VU1_ADDR(Line));

	// offset mode
	vif::STMOD(1);

	// begin unpack for coordinates
	vif::BeginUNPACK(0, V4_32, ABS, SIGNED, 1);
}


void	ChangeLineColor(uint32 rgba)
{

// End the previous batch of lines
		EndLines3D();
		
// Like starting a new batch, but witout the VU context		
		dma::BeginTag(dma::cnt, 0);

		// GS context
		// Sets up the GS registers for color and alpha
		gs::BeginPrim(ABS,0,0);
		gs::Reg1(gs::ALPHA_1, PackALPHA(0,1,0,1,0));
		gs::Reg1(gs::RGBAQ,   (uint64)rgba);
		gs::EndPrim(0);
		
		vif::UNPACK(0, V4_32, 1, ABS, UNSIGNED, 0);
		gif::BeginTagImmediate(gs::XYZ2, 1, PACKED, LINE|ABE, 1, VU1_ADDR(Line));
		vif::STMOD(1);
		vif::BeginUNPACK(0, V4_32, ABS, SIGNED, 1);
	
}


void DrawLine3D(float x0, float y0, float z0, float x1, float y1, float z1)
{
	vif::StoreV4_32((sint32)(x0*SUB_INCH_PRECISION), (sint32)(y0*SUB_INCH_PRECISION), (sint32)(z0*SUB_INCH_PRECISION), 0);
	vif::StoreV4_32((sint32)(x1*SUB_INCH_PRECISION), (sint32)(y1*SUB_INCH_PRECISION), (sint32)(z1*SUB_INCH_PRECISION), 0);

	if (((dma::pLoc - gif::pTag)>>4) >= 250)
	{
		vif::EndUNPACK();
		gif::EndTagImmediate(1);
		vif::STMOD(0);
		vif::MSCAL(VU1_ADDR(Parser));
		vif::UNPACK(0, V4_32, 1, ABS, UNSIGNED, 0);
		gif::BeginTagImmediate(gs::XYZ2, 1, PACKED, LINE|ABE, 1, VU1_ADDR(Line));
		vif::STMOD(1);
		vif::BeginUNPACK(0, V4_32, ABS, SIGNED, 1);
	}

}


void EndLines3D(void)
{
	vif::EndUNPACK();

	if (vif::UnpackSize)
	{
		gif::EndTagImmediate(1);
		vif::STMOD(0);
		vif::MSCAL(VU1_ADDR(Parser));
	}
	else
		dma::pLoc -= 24;

	dma::EndTag();
}

} // namespace NxPs2

