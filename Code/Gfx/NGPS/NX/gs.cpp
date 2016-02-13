#include <core/defines.h>
#include "vu1code.h"
#include "dma.h"
#include "vif.h"
#include "vu1.h"
#include "gif.h"
#include "gs.h"


namespace NxPs2
{


//--------------------------------------------------
//		G S   P R I M   C O N S T R U C T I O N
//--------------------------------------------------


// begin
void gs::BeginPrim(uint FLG, uint Prim, uint Pre)
{
	vif::STCYCL(1,1);
	vif::UNPACK(0, V4_32, 1, FLG, UNSIGNED, 0);
	gif::BeginTag1(gs::A_D, 1, PACKED, Prim, Pre, VU1_ADDR(GSPrim));
	vif::BeginUNPACK(0, V3_32, FLG, UNSIGNED, 1);
}


// end
void gs::EndPrim(uint Eop)
{
	vif::EndUNPACK();
	gif::EndTag1(Eop);
}




// output 1 GS register DIRECT via PATH2, V4_32 format
void gs::Reg2(eReg Reg, uint64 Data)
{
	dma::Store64(Data);
	dma::Store64((uint64)Reg);
}


// version returning address of data
uint32 *gs::Reg2_pData(eReg Reg, uint64 Data)
{
	uint32 *p_ret = (uint32 *) dma::pLoc;

	dma::Store64(Data);
	dma::Store64((uint64)Reg);

	return p_ret;
}


// Modify the TBPs of GS registers
void gs::ModifyTBP(eReg Reg, uint32 *p_reg_ptr, sint16 tbp1, sint16 tbp2, sint16 tbp3)
{
	switch (Reg)
	{
	case BITBLTBUF:		// only modifies DBP
		*(++p_reg_ptr) &= ~(M14);				// DBP
		*(  p_reg_ptr) |= (tbp1 & M14);
		break;

	case TEX0_1:
	case TEX0_2:
		*(  p_reg_ptr) &= ~(M14);				// TBP
		*(  p_reg_ptr) |= (tbp1 & M14);
		if (tbp2 >= 0)
		{
			*(++p_reg_ptr) &= ~(M14 << 5);		// CBP
			*(  p_reg_ptr) |= (tbp2 & M14) << 5;
		}
		break;

	case TEX2_1:
	case TEX2_2:
		*(++p_reg_ptr) &= ~(M14 << 5);			// CBP
		*(  p_reg_ptr) |= (tbp1 & M14) << 5;
		break;

	case MIPTBP1_1:
	case MIPTBP1_2:
	case MIPTBP2_1:
	case MIPTBP2_2:
	default:
		#ifdef __PLAT_NGPS__
		Dbg_MsgAssert(0, ("gs::ModifyTBP(): Can't modify register %x", Reg));
		#endif
		break;
	}
}

} // namespace NxPs2



