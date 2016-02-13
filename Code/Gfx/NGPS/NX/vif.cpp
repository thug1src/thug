#include <core/defines.h>
#include <stdio.h>
#include "dma.h"
#include "vif.h"
#include "vu1.h"


namespace NxPs2
{


uint8 *vif::NextCode(uint8 *pVifcode)
{
	if ((pVifcode[3] & 0x60) != 0x60)
	{
		switch (pVifcode[3] & 0x7F)
		{
		case 0x00:	// NOP
		case 0x02:	// OFFSET
		case 0x03:	// BASE
		case 0x04:	// ITOP
		case 0x05:	// STMOD
		case 0x06:	// MSKPATH3
		case 0x07:	// MARK
		case 0x10:	// FLUSHE
		case 0x11:	// FLUSH
		case 0x13:	// FLUSHA
		case 0x14:	// MSCAL
		case 0x15:	// MSCALF
		case 0x17:	// MSCNT
			return pVifcode + 4;

		case 0x01:	// STCYCL
			vif::CL = pVifcode[0];
			vif::WL = pVifcode[1];
			return pVifcode + 4;

		case 0x20:	// STMASK
			return pVifcode + 8;

		case 0x30:	// STROW
		case 0x31:	// STCOL
			return pVifcode + 20;

		case 0x4A:	// MPG
			return pVifcode + (pVifcode[2] << 3) + 4;

		case 0x50:	// DIRECT
		case 0x51:	// DIRECTHL
			return pVifcode + (((uint16 *)pVifcode)[0] << 4) + 4;

		default:	// undefined vifcode
			return pVifcode;
		}
	}
	else			// UNPACK
	{
		uint vn = (pVifcode[3] >> 2) & 3;
		uint vl = pVifcode[3] & 3;
		uint num = pVifcode[2];
		uint dimension = vn+1;
		uint bitlength = 32>>vl;
		if (vif::WL <= vif::CL)
		{
			return pVifcode + 4 + (((bitlength * dimension * num + 31) >> 5) << 2);
		}
		else
		{
			uint rem = num % vif::WL;
			uint n = vif::CL * (num/vif::WL) + (rem > vif::CL ? vif::CL : rem);
			return pVifcode + 4 + (((bitlength * dimension * n + 31) >> 5) << 2);
		}
	}
}




//--------------------------------
//		S T A T I C   D A T A
//--------------------------------

uint  vif::UnpackSize;
uint  vif::CycleLength;
uint  vif::BitLength;
uint  vif::Dimension;
uint8 *vif::pVifCode;
uint  vif::WL;
uint  vif::CL;


} // namespace NxPs2



