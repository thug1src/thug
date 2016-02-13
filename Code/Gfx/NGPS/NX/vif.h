#ifndef __VIF_H
#define __VIF_H

#include "dma.h"
#include "vu1.h"

namespace NxPs2
{

// VIF1_MODE settings
#define NORMAL_MODE		0
#define OFFSET_MODE		1
#define DIFFERENCE_MODE	2

// unpack formats
#define	S_32	0x0
#define	S_16	0x1
#define	S_8		0x2
#define	V2_32	0x4
#define	V2_16	0x5
#define	V2_8	0x6
#define	V3_32	0x8
#define	V3_16	0x9
#define	V3_8	0xA
#define	V4_32	0xC
#define	V4_16	0xD
#define	V4_8	0xE
#define	V4_5	0xF

// unpack FLG values
#define	ABS	0
#define REL 1

// unpack USN values
#define SIGNED		0
#define UNSIGNED	1



class vif
{

public:

	//------------------------------------------
	//		S T A T I C   F U N C T I O N S
	//------------------------------------------

	// vifcodes
	static void Code(uint8 CMD, uint8 NUM, uint16 IMMEDIATE);
	static void BASE(uint16 BASE);
	static void DIRECT(uint16 SIZE);
	static void DIRECTHL(uint16 SIZE);
	static void FLUSH(void);
	static void FLUSHA(void);
	static void FLUSHE(void);
	static void ITOP(uint16 ADDR);
	static void MARK(uint16 MARK);
	static void MPG(uint8 SIZE, uint16 LOADADDR);
	static void MSCAL(uint16 EXECADDR);
	static void MSCALF(uint16 EXECADDR);
	static void MSCNT(void);
	static void MSKPATH3(uint16 MASK);
	static void NOP(void);
	static void NOPi(void);
	static void OFFSET(uint16 OFFSET);
	static void STCOL(uint32 C0, uint32 C1, uint32 C2, uint32 C3);
	static void STCYCL(uint16 WL, uint16 CL);
	static void STMASK(uint32 MASK);
	static void STMOD(uint16 MODE);
	static void STROW(uint32 R0, uint32 R1, uint32 R2, uint32 R3);
	static void UNPACK(uint8 m, uint8 vnvl, uint8 SIZE, uint16 FLG, uint16 USN, uint16 ADDR);

	// vifcodes, begin-end style
	static void BeginDIRECT(void);
	static void EndDIRECT(void);
	static void BeginDIRECTHL(void);
	static void EndDIRECTHL(void);
	static void BeginMPG(uint16 LOADADDR);
	static void EndMPG(void);
	static void BeginUNPACK(uint8 m, uint8 vnvl, uint16 FLG, uint16 USN, uint16 ADDR);
	static void EndUNPACK(void);

	// storing data for various unpack formats
	static void StoreS_32(uint32 x);
	static void StoreS_16(uint16 x);
	static void StoreS_8(uint8 x);
	static void StoreV2_32(uint32 x, uint32 y);
	static void StoreV2_16(uint16 x, uint16 y);
	static void StoreV2_8(uint8 x, uint8 y);
	static void StoreV3_32(uint32 x, uint32 y, uint32 z);
	static void StoreV3_16(uint16 x, uint16 y, uint16 z);
	static void StoreV3_8(uint8 x, uint8 y, uint8 z);
	static void StoreV4_32F(float x, float y, float z, float w);
	static void StoreV4_32(sint32 x, sint32 y, sint32 z, sint32 w);
	static void StoreV4_16(sint16 x, sint16 y, sint16 z, sint16 w);
	static void StoreV4_8(sint8 x, sint8 y, sint8 z, sint8 w);
	static void StoreV4_5(uint8 R, uint8 G, uint8 B, uint8 A);

	// vifcode parsing
	static uint8 *vif::NextCode(uint8 *pVifcode);


	//--------------------------------
	//		S T A T I C   D A T A
	//--------------------------------

	static uint  UnpackSize;
	static uint  CycleLength;
	static uint  BitLength;
	static uint  Dimension;
	static uint8 *pVifCode;
	static uint  WL;
	static uint  CL;


}; // class vif



//--------------------------
//		V I F C O D E S
//--------------------------



// generic vifcode
//
//	 31   24 23   16 15            0
//	ÚÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
//	³  CMD  ³  NUM  ³   IMMEDIATE   ³
//	ÀÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ


inline void vif::Code(uint8 CMD, uint8 NUM, uint16 IMMEDIATE)
{
	dma::Store32((uint32)CMD<<24 | (uint32)NUM<<16 | (uint32)IMMEDIATE);
}



// Sets VIF1_BASE to lower 10 bits of BASE.

inline void vif::BASE(uint16 BASE)
{
	vif::Code(0x03, 0, BASE);
}



// Transfers SIZE 128-bit data via PATH2.

inline void vif::DIRECT(uint16 SIZE)
{
	vif::Code(0x50, 0, SIZE);
}



// Transfers SIZE 128-bit data via PATH2, stalling till PATH3 IMAGE mode transfer is complete.

inline void vif::DIRECTHL(uint16 SIZE)
{
	vif::Code(0x51, 0, SIZE);
}



// Waits for PATH1/PATH2 transfers to end, after VU1 microprogram has ended.

inline void vif::FLUSH(void)
{
	vif::Code(0x11, 0, 0);
}



// Waits till no PATH3 transfer request, after the end of VU1 microprogram and PATH1/PATH2 transfer.

inline void vif::FLUSHA(void)
{
	vif::Code(0x13, 0, 0);
}



// Waits for VU0/VU1 microprogram to end.

inline void vif::FLUSHE(void)
{
	vif::Code(0x10, 0, 0);
}



// Sets VIFn_ITOPS to lower to bits of ADDR.

inline void vif::ITOP(uint16 ADDR)
{
	vif::Code(0x04, 0, ADDR);
}



// Sets VIFn_MARK to MARK.

inline void vif::MARK(uint16 MARK)
{
	vif::Code(0x07, 0, MARK);
}



// Waits for end of microprogram and transfers SIZE 64-bit data to MicroMem address LOADADDR (low 11 bits).

inline void vif::MPG(uint8 SIZE, uint16 LOADADDR)
{
	vif::Code(0x4A, SIZE, LOADADDR);
}



// Waits for end of microprogram, and activates microprogram at address EXECADDR (low 11 bits).

inline void vif::MSCAL(uint16 EXECADDR)
{
	vif::Code(0x14, 0, EXECADDR);
	vu1::Buffer = vu1::Loc;
}



// Waits for end of bot microprogram and PATH1/PATH2 transfers,
// and activates microprogram at address EXECADDR (low 11 bits).

inline void vif::MSCALF(uint16 EXECADDR)
{
	vif::Code(0x15, 0, EXECADDR);
}



// Waits for end of microprogram, and activates microprogram at address held in PC.

inline void vif::MSCNT(void)
{
	vif::Code(0x17, 0, 0);
}



// MASK==1 => disables transfer via PATH3.

inline void vif::MSKPATH3(uint16 MASK)
{
	vif::Code(0x06, 0, MASK<<15);
}



// Does nothing.

inline void vif::NOP(void)
{
	vif::Code(0x00, 0, 0);
}



// Interrupt version.

inline void vif::NOPi(void)
{
	vif::Code(0x80, 0, 0);
}



// Sets VIF1_OFST to lower 10 bits of OFFSET.
// DBF flag of VIF1_STAT is cleared and VIF1_BASE is copied to VIF1_TOPS.

inline void vif::OFFSET(uint16 OFFSET)
{
	vif::Code(0x02, 0, OFFSET);
}



// Sets VIFn_C0-VIFn_C3 to C0-C3.

inline void vif::STCOL(uint32 C0, uint32 C1, uint32 C2, uint32 C3)
{
	vif::Code(0x31, 0, 0);
	dma::Store32(C0, C1, C2, C3);
}



// Sets VIFn_CYCLE.
// CL>=WL gives skipping write;
// CL<WL gives filling write.

inline void vif::STCYCL(uint16 WL, uint16 CL)
{
	vif::Code(0x01, 0, WL<<8|CL);
	CycleLength = WL>CL?WL:CL;
}



// Sets VIFn_MASK to MASK = (m15,...,m0) with m0 at the low end of the word.
//
//					 W	 Z	 Y	 X
//					---------------
// write cycle=1	m0	m1	m2	m3
// write cycle=2	m4	m5	m6	m7
// write cycle=3	m8	m9	m10	m11
// write cycle>=4	m15	m14	m13	m12
//
// m[n]=0 => decompressed data written as-is;
// m[n]=1 => row register written;
// m[n]=2 => column register written;
// m[n]=3 => write masked.

inline void vif::STMASK(uint32 MASK)
{
	vif::Code(0x20, 0, 0);
	dma::Store32(MASK);
}



// Sets VIFn_MODE to lower 2 bits of MODE.

inline void vif::STMOD(uint16 MODE)
{
	vif::Code(0x05, 0, MODE);
}



// Sets VIFn_R0-VIFn_R3 to R0-R3.

inline void vif::STROW(uint32 R0, uint32 R1, uint32 R2, uint32 R3)
{
	vif::Code(0x30, 0, 0);
	dma::Store32(R0,R1,R2,R3);
}



// Decompresses data to VUMem+ADDR.
// SIZE is the number of decompressed data;
// vnvl is the format of compressed data (S_8, V4_32, etc);
// destination in VUMem is ADDR (if FLG==0 on VU1) or VIF1_TOPS+ADDR (if FLG==1 on VU1);
// USN==0 => sign extension, USN==1 => padding with 0's;
// m==1 => masking write using VIFn_MASK.

inline void vif::UNPACK(uint8 m, uint8 vnvl, uint8 SIZE, uint16 FLG, uint16 USN, uint16 ADDR)
{
	vif::Code(0x60|m<<4|vnvl, SIZE, FLG<<15 | USN<<14 | ((vu1::Loc+ADDR)&0x03FF));
	UnpackSize = SIZE;
}





//--------------------------------------------------------------
//		V I F C O D E S   ( B E G I N - E N D   S T Y L E )
//--------------------------------------------------------------


// Auto-size version of DIRECT.
// Alignment necessary because data must align to 128-bit boundary.

inline void vif::BeginDIRECT(void)
{
	dma::Align(12,16);
	pVifCode = dma::pLoc;
	vif::Code(0x50, 0, 0);
}



// Patch up the IMMEDIATE field of a DIRECT generated using BeginDIRECT().
// Alignment necessary because vif will interpret till 16-byte boundary as data.

inline void vif::EndDIRECT(void)
{
	dma::Align(0,16);
	((uint16 *)pVifCode)[0] = (dma::pLoc - pVifCode - 4)/16;
}



// Auto-size version of DIRECTHL.
// Alignment necessary because data must align to 16-byte boundary.

inline void vif::BeginDIRECTHL(void)
{
	dma::Align(12,16);
	pVifCode = dma::pLoc;
	vif::Code(0x51, 0, 0);
}



// Patch up the IMMEDIATE field of a DIRECTHL generated using BeginDIRECTHL().
// Alignment necessary because vif will interpret till 16-byte boundary as data.

inline void vif::EndDIRECTHL(void)
{
	dma::Align(0,16);
	((uint16 *)pVifCode)[0] = (dma::pLoc - pVifCode - 4)/16;
}



// Auto-size version of MPG.
// Alignment necessary because data must align to 8-byte boundary.

inline void vif::BeginMPG(uint16 LOADADDR)
{
	dma::Align(4,8);
	pVifCode = dma::pLoc;
	vif::Code(0x4A, 0, LOADADDR);
}



// Patch up the NUM field of an MPG generated using BeginMPG().
// Alignment necessary because vif will interpret till 8-byte boundary as data.

inline void vif::EndMPG(void)
{
	dma::Align(0,8);
	pVifCode[2] = (dma::pLoc - pVifCode - 4)/8;
}



// Auto-size version of UNPACK.

inline void vif::BeginUNPACK(uint8 m, uint8 vnvl, uint16 FLG, uint16 USN, uint16 ADDR)
{
	pVifCode = dma::pLoc;
	BitLength = 32 >> (vnvl & 3);
	Dimension = (vnvl>>2 & 3) + 1;
	vif::Code(0x60 | m<<4 | vnvl, 0, FLG<<15 | USN<<14 | ((vu1::Loc+ADDR)&0x03FF) );
}



// Patch up the NUM field of an UNPACK generated using BeginUNPACK().
// Alignment is necessary because some data formats won't fill out to a 4-byte boundary.

inline void vif::EndUNPACK(void)
{
	UnpackSize = ((dma::pLoc - pVifCode - 4) << 3) / (Dimension * BitLength);
	dma::Align(0,4);
	if (UnpackSize==0)
		dma::pLoc -= 4;				// no data, rewind over vifcode
	else if (UnpackSize < 256)
		pVifCode[2] = UnpackSize;	// normal usage
	else if (UnpackSize == 256)
		pVifCode[2] = 0;				// 0 represents 256
	else
	{
		printf("unpack size greater than 256\n");
		#ifdef __PLAT_NGPS__
		exit(1);
		#endif
	}
}






//-----------------------------------------------------------------
//		S T O R I N G   C O M P R E S S E D   V I F   D A T A
//-----------------------------------------------------------------


// scalars

inline void vif::StoreS_32(uint32 x)
{
	*(uint32 *)dma::pLoc = x;
	dma::pLoc += 4;
}


inline void vif::StoreS_16(uint16 x)
{
	*(uint16 *)dma::pLoc = x;
	dma::pLoc += 2;
}


inline void vif::StoreS_8(uint8 x)
{
	*dma::pLoc++ = x;
}



// 2-vectors

inline void vif::StoreV2_32(uint32 x, uint32 y)
{
	((uint32 *)dma::pLoc)[0] = x;
	((uint32 *)dma::pLoc)[1] = y;
	dma::pLoc += 8;
}


inline void vif::StoreV2_16(uint16 x, uint16 y)
{
	((uint16 *)dma::pLoc)[0] = x;
	((uint16 *)dma::pLoc)[1] = y;
	dma::pLoc += 4;
}


inline void vif::StoreV2_8(uint8 x, uint8 y)
{
	((uint8 *)dma::pLoc)[0] = x;
	((uint8 *)dma::pLoc)[1] = y;
	dma::pLoc += 2;
}



// 3-vectors

inline void vif::StoreV3_32(uint32 x, uint32 y, uint32 z)
{
	((uint32 *)dma::pLoc)[0] = x;
	((uint32 *)dma::pLoc)[1] = y;
	((uint32 *)dma::pLoc)[2] = z;
	dma::pLoc += 12;
}


inline void vif::StoreV3_16(uint16 x, uint16 y, uint16 z)
{
	((uint16 *)dma::pLoc)[0] = x;
	((uint16 *)dma::pLoc)[1] = y;
	((uint16 *)dma::pLoc)[2] = z;
	dma::pLoc += 6;
}


inline void vif::StoreV3_8(uint8 x, uint8 y, uint8 z)
{
	((uint8 *)dma::pLoc)[0] = x;
	((uint8 *)dma::pLoc)[1] = y;
	((uint8 *)dma::pLoc)[2] = z;
	dma::pLoc[2] = z;
	dma::pLoc += 3;
}



// 4-vectors

inline void vif::StoreV4_32F(float x, float y, float z, float w)
{
	((float *)dma::pLoc)[0] = x;
	((float *)dma::pLoc)[1] = y;
	((float *)dma::pLoc)[2] = z;
	((float *)dma::pLoc)[3] = w;
	dma::pLoc += 16;
}


inline void vif::StoreV4_32(sint32 x, sint32 y, sint32 z, sint32 w)
{
	((sint32 *)dma::pLoc)[0] = x;
	((sint32 *)dma::pLoc)[1] = y;
	((sint32 *)dma::pLoc)[2] = z;
	((sint32 *)dma::pLoc)[3] = w;
	dma::pLoc += 16;
}


inline void vif::StoreV4_16(sint16 x, sint16 y, sint16 z, sint16 w)
{
	((sint16 *)dma::pLoc)[0] = x;
	((sint16 *)dma::pLoc)[1] = y;
	((sint16 *)dma::pLoc)[2] = z;
	((sint16 *)dma::pLoc)[3] = w;
	dma::pLoc += 8;
}


inline void vif::StoreV4_8(sint8 x, sint8 y, sint8 z, sint8 w)
{
	((sint8 *)dma::pLoc)[0] = x;
	((sint8 *)dma::pLoc)[1] = y;
	((sint8 *)dma::pLoc)[2] = z;
	((sint8 *)dma::pLoc)[3] = w;
	dma::pLoc += 4;
}


inline void vif::StoreV4_5(uint8 R, uint8 G, uint8 B, uint8 A)
{
	*(uint16 *)dma::pLoc = (A&0x80)<<8 | (B&0xF8)<<7 | (G&0xF8)<<2 | (R&0xF8)>>3;
	dma::pLoc += 2;
}




} // namespace NxPs2


#endif // __VIF_H

