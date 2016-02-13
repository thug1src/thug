#ifndef __GS_H
#define __GS_H

#include "dma.h"

//---------------------------------------------------
//		R E G I S T E R   D E F I N I T I O N S
//---------------------------------------------------



// mask definitions for packing macros
#define M01	0x0000000000000001
#define M02	0x0000000000000003
#define M03	0x0000000000000007
#define M04	0x000000000000000F
#define M05	0x000000000000001F
#define M06	0x000000000000003F
#define M08	0x00000000000000FF
#define M09	0x00000000000001FF
#define M10	0x00000000000003FF
#define M11	0x00000000000007FF
#define M12	0x0000000000000FFF
#define M14	0x0000000000003FFF
#define M16	0x000000000000FFFF
#define M24	0x0000000000FFFFFF
#define M32	0x00000000FFFFFFFF
#define M64	0xFFFFFFFFFFFFFFFF


//----------------------------------------------------------------------------------------
//		G E N E R A L   P U R P O S E   R E G I S T E R   P A C K I N G   M A C R O S
//----------------------------------------------------------------------------------------

#define PackALPHA(A,B,C,D,FIX)				\
(											\
	( (uint64) (A)		& M02 )	<< 0	|	\
	( (uint64) (B)		& M02 )	<< 2	|	\
	( (uint64) (C)		& M02 )	<< 4	|	\
	( (uint64) (D)		& M02 )	<< 6	|	\
	( (uint64) (FIX)	& M08 )	<< 32		\
)

#define PackBITBLTBUF(SBP,SBW,SPSM,DBP,DBW,DPSM)\
(											\
	( (uint64) (SBP)	& M14 )	<< 0	|	\
	( (uint64) (SBW)	& M06 )	<< 16	|	\
	( (uint64) (SPSM)	& M06 )	<< 24	|	\
	( (uint64) (DBP)	& M14 )	<< 32	|	\
	( (uint64) (DBW)	& M06 )	<< 48	|	\
	( (uint64) (DPSM)	& M06 )	<< 56		\
)

#define PackCLAMP(WMS,WMT,MINU,MAXU,MINV,MAXV)\
(											\
	( (uint64) (WMS)	& M02 )	<< 0	|	\
	( (uint64) (WMT)	& M02 )	<< 2	|	\
	( (uint64) (MINU)	& M10 )	<< 4	|	\
	( (uint64) (MAXU)	& M10 )	<< 14	|	\
	( (uint64) (MINV)	& M10 )	<< 24	|	\
	( (uint64) (MAXV)	& M10 )	<< 34		\
)

#define PackCOLCLAMP(CLAMP)					\
(											\
	( (uint64) (CLAMP)	& M01 )	<< 0		\
)

#define PackDIMX(DM00,DM01,DM02,DM03,DM10,DM11,DM12,DM13,DM20,DM21,DM22,DM23,DM30,DM31,DM32,DM33)\
(											\
	( (uint64) (DM00)	& M03 )	<< 0	|	\
	( (uint64) (DM01)	& M03 )	<< 4	|	\
	( (uint64) (DM02)	& M03 )	<< 8	|	\
	( (uint64) (DM03)	& M03 )	<< 12	|	\
	( (uint64) (DM10)	& M03 )	<< 16	|	\
	( (uint64) (DM11)	& M03 )	<< 20	|	\
	( (uint64) (DM12)	& M03 )	<< 24	|	\
	( (uint64) (DM13)	& M03 )	<< 28	|	\
	( (uint64) (DM20)	& M03 )	<< 32	|	\
	( (uint64) (DM21)	& M03 )	<< 36	|	\
	( (uint64) (DM22)	& M03 )	<< 40	|	\
	( (uint64) (DM23)	& M03 )	<< 44	|	\
	( (uint64) (DM30)	& M03 )	<< 48	|	\
	( (uint64) (DM31)	& M03 )	<< 52	|	\
	( (uint64) (DM32)	& M03 )	<< 56	|	\
	( (uint64) (DM33)	& M03 )	<< 60		\
)

#define PackDTHE(DTHE)						\
(											\
	( (uint64) (DTHE)	& M01 )	<< 0		\
)

#define PackFBA(FBA)						\
(											\
	( (uint64) (FBA)	& M01 )	<< 0		\
)

#define PackFOG(F)							\
(											\
	( (uint64) (F)		& M08 )	<< 56		\
)

#define PackFOGCOL(FCR,FCG,FCB)				\
(											\
	( (uint64) (FCR)	& M08 )	<< 0	|	\
	( (uint64) (FCG)	& M08 )	<< 8	|	\
	( (uint64) (FCB)	& M08 )	<< 16		\
)

#define PackFRAME(FBP,FBW,PSM,FBMSK)		\
(											\
	( (uint64) (FBP)	& M09 )	<< 0	|	\
	( (uint64) (FBW)	& M06 )	<< 16	|	\
	( (uint64) (PSM)	& M06 )	<< 24	|	\
	( (uint64) (FBMSK)	& M32 )	<< 32		\
)

#define PackHWREG(DATA)						\
(											\
	( (uint64) (DATA)	& M64 )	<< 0		\
)

#define PackLABEL(ID,IDMSK)					\
(											\
	( (uint64) (ID)		& M32 )	<< 0	|	\
	( (uint64) (IDMSK)	& M32 )	<< 32		\
)

#define PackMIPTBP1(TBP1,TBW1,TBP2,TBW2,TBP3,TBW3)\
(											\
	( (uint64) (TBP1)	& M14 )	<< 0	|	\
	( (uint64) (TBW1)	& M06 )	<< 14	|	\
	( (uint64) (TBP2)	& M14 )	<< 20	|	\
	( (uint64) (TBW2)	& M06 )	<< 34	|	\
	( (uint64) (TBP3)	& M14 )	<< 40	|	\
	( (uint64) (TBW3)	& M06 )	<< 54		\
)

#define PackMIPTBP2(TBP4,TBW4,TBP5,TBW5,TBP6,TBW6)\
(											\
	( (uint64) (TBP4)	& M14 )	<< 0	|	\
	( (uint64) (TBW4)	& M06 )	<< 14	|	\
	( (uint64) (TBP5)	& M14 )	<< 20	|	\
	( (uint64) (TBW5)	& M06 )	<< 34	|	\
	( (uint64) (TBP6)	& M14 )	<< 40	|	\
	( (uint64) (TBW6)	& M06 )	<< 54		\
)

#define PackPABE(PABE)						\
(											\
	( (uint64) (PABE)	& M01 )	<< 0		\
)

#define PackPRIM(PRIM,IIP,TME,FGE,ABE,AA1,FST,CTXT,FIX)\
(											\
	( (uint64) (PRIM)	& M03 )	<< 0	|	\
	( (uint64) (IIP)	& M01 )	<< 3	|	\
	( (uint64) (TME)	& M01 )	<< 4	|	\
	( (uint64) (FGE)	& M01 )	<< 5	|	\
	( (uint64) (ABE)	& M01 )	<< 6	|	\
	( (uint64) (AA1)	& M01 )	<< 7	|	\
	( (uint64) (FST)	& M01 )	<< 8	|	\
	( (uint64) (CTXT)	& M01 )	<< 9	|	\
	( (uint64) (FIX)	& M01 )	<< 10		\
)

#define PackPRMODE(IIP,TME,FGE,ABE,AA1,FST,CTXT,FIX)\
(											\
	( (uint64) (IIP)	& M01 )	<< 3	|	\
	( (uint64) (TME)	& M01 )	<< 4	|	\
	( (uint64) (FGE)	& M01 )	<< 5	|	\
	( (uint64) (ABE)	& M01 )	<< 6	|	\
	( (uint64) (AA1)	& M01 )	<< 7	|	\
	( (uint64) (FST)	& M01 )	<< 8	|	\
	( (uint64) (CTXT)	& M01 )	<< 9	|	\
	( (uint64) (FIX)	& M01 )	<< 10		\
)

#define PackPRMODECONT(AC)					\
(											\
	( (uint64) (AC)		& M01 )	<< 0		\
)

#define PackRGBAQ(R,G,B,A,Q)				\
(											\
	( (uint64) (R)		& M08 )	<< 0	|	\
	( (uint64) (G)		& M08 )	<< 8	|	\
	( (uint64) (B)		& M08 )	<< 16	|	\
	( (uint64) (A)		& M08 )	<< 24	|	\
	( (uint64) (Q)		& M32 )	<< 32		\
)

#define PackSCANMSK(MSK)					\
(											\
	( (uint64) (MSK)	& M01 )	<< 0		\
)

#define PackSCISSOR(SCAX0,SCAX1,SCAY0,SCAY1)\
(											\
	( (uint64) (SCAX0)	& M11 )	<< 0	|	\
	( (uint64) (SCAX1)	& M11 )	<< 16	|	\
	( (uint64) (SCAY0)	& M11 )	<< 32	|	\
	( (uint64) (SCAY1)	& M11 )	<< 48		\
)

#define PackSIGNAL(ID,IDMSK)				\
(											\
	( (uint64) (ID)		& M32 )	<< 0	|	\
	( (uint64) (IDMSK)	& M32 )	<< 32		\
)

#define PackST(S,T)							\
(											\
	( (uint64) (S)		& M32 )	<< 0	|	\
	( (uint64) (T)		& M32 )	<< 32		\
)

#define PackTEST(ATE,ATST,AREF,AFAIL,DATE,DATM,ZTE,ZTST)\
(											\
	( (uint64) (ATE)	& M01 )	<< 0	|	\
	( (uint64) (ATST)	& M03 )	<< 1	|	\
	( (uint64) (AREF)	& M08 )	<< 4	|	\
	( (uint64) (AFAIL)	& M02 )	<< 12	|	\
	( (uint64) (DATE)	& M01 )	<< 14	|	\
	( (uint64) (DATM)	& M01 )	<< 15	|	\
	( (uint64) (ZTE)	& M01 )	<< 16	|	\
	( (uint64) (ZTST)	& M02 )	<< 17		\
)

#define PackTEX0(TBP0,TBW,PSM,TW,TH,TCC,TFX,CBP,CPSM,CSM,CSA,CLD)\
(											\
	( (uint64) (TBP0)	& M14 )	<< 0	|	\
	( (uint64) (TBW)	& M06 )	<< 14	|	\
	( (uint64) (PSM)	& M06 )	<< 20	|	\
	( (uint64) (TW)		& M04 )	<< 26	|	\
	( (uint64) (TH)		& M04 )	<< 30	|	\
	( (uint64) (TCC)	& M01 )	<< 34	|	\
	( (uint64) (TFX)	& M02 )	<< 35	|	\
	( (uint64) (CBP)	& M14 )	<< 37	|	\
	( (uint64) (CPSM)	& M04 )	<< 51	|	\
	( (uint64) (CSM)	& M01 )	<< 55	|	\
	( (uint64) (CSA)	& M05 )	<< 56	|	\
	( (uint64) (CLD)	& M03 )	<< 61		\
)

#define PackTEX1(LCM,MXL,MMAG,MMIN,MTBA,L,K)\
(											\
	( (uint64) (LCM)	& M01 )	<< 0	|	\
	( (uint64) (MXL)	& M03 )	<< 2	|	\
	( (uint64) (MMAG)	& M01 )	<< 5	|	\
	( (uint64) (MMIN)	& M03 )	<< 6	|	\
	( (uint64) (MTBA)	& M01 )	<< 9	|	\
	( (uint64) (L)		& M02 )	<< 19	|	\
	( (uint64) (K)		& M12 )	<< 32		\
)

#define PackTEX2(PSM,CBP,CPSM,CSM,CSA,CLD)	\
(											\
	( (uint64) (PSM)	& M06 )	<< 20	|	\
	( (uint64) (CBP)	& M14 )	<< 37	|	\
	( (uint64) (CPSM)	& M04 )	<< 51	|	\
	( (uint64) (CSM)	& M01 )	<< 55	|	\
	( (uint64) (CSA)	& M05 )	<< 56	|	\
	( (uint64) (CLD)	& M03 )	<< 61		\
)

#define PackTEXFLUSH(VAL)					\
(											\
	( (uint64) (VAL)	& M64 )	<< 0		\
)

#define PackTEXA(TA0,AEM,TA1)				\
(											\
	( (uint64) (TA0)	& M08 )	<< 0	|	\
	( (uint64) (AEM)	& M01 )	<< 15	|	\
	( (uint64) (TA1)	& M08 )	<< 32		\
)

#define PackTEXCLUT(CBW,COU,COV)			\
(											\
	( (uint64) (CBW)	& M06 )	<< 0	|	\
	( (uint64) (COU)	& M06 )	<< 6	|	\
	( (uint64) (COV)	& M10 )	<< 12		\
)

#define PackTRXDIR(XDIR)					\
(											\
	( (uint64) (XDIR)	& M02 )	<< 0		\
)

#define PackTRXPOS(SSAX,SSAY,DSAX,DSAY,DIR)	\
(											\
	( (uint64) (SSAX)	& M11 )	<< 0	|	\
	( (uint64) (SSAY)	& M11 )	<< 16	|	\
	( (uint64) (DSAX)	& M11 )	<< 32	|	\
	( (uint64) (DSAY)	& M11 )	<< 48	|	\
	( (uint64) (DIR)	& M02 )	<< 59		\
)

#define PackTRXREG(RRW,RRH)					\
(											\
	( (uint64) (RRW)	& M12 )	<< 0	|	\
	( (uint64) (RRH)	& M12 )	<< 32		\
)

#define PackUV(U,V)							\
(											\
	( (uint64) (U)		& M14 )	<< 0	|	\
	( (uint64) (V)		& M14 )	<< 16		\
)

#define PackXYOFFSET(OFX,OFY)				\
(											\
	( (uint64) (OFX)	& M16 )	<< 0	|	\
	( (uint64) (OFY)	& M16 )	<< 32		\
)

#define PackXYZ(X,Y,Z)						\
(											\
	( (uint64) (X)		& M16 )	<< 0	|	\
	( (uint64) (Y)		& M16 )	<< 16	|	\
	( (uint64) (Z)		& M32 )	<< 32		\
)

#define PackXYZF(X,Y,Z,F)					\
(											\
	( (uint64) (X)		& M16 )	<< 0	|	\
	( (uint64) (Y)		& M16 )	<< 16	|	\
	( (uint64) (Z)		& M24 )	<< 32	|	\
	( (uint64) (F)		& M08 )	<< 56		\
)

#define PackZBUF(ZBP,PSM,ZMSK)				\
(											\
	( (uint64) (ZBP)	& M09 )	<< 0	|	\
	( (uint64) (PSM)	& M04 )	<< 24	|	\
	( (uint64) (ZMSK)	& M01 )	<< 32		\
)


//----------------------------------------------------------------------------------------
//		P R I V I L E G E D   R E G I S T E R   P A C K I N G   M A C R O S
//----------------------------------------------------------------------------------------


#define PackBGCOLOR(R,G,B)					\
(											\
	( (uint64) (R)		& M08 )	<< 0	|	\
	( (uint64) (G)		& M08 )	<< 8	|	\
	( (uint64) (B)		& M08 )	<< 16		\
)

#define PackBUSDIR(DIR)						\
(											\
	( (uint64) (DIR)	& M01 )	<< 0		\
)

#define PackCSR(SIGNAL,FINISH,HSINT,VSINT,FLUSH,RESET,NFIELD,FIELD,FIFO,REV,ID)\
(											\
	( (uint64) (SIGNAL)	& M01 )	<< 0	|	\
	( (uint64) (FINISH)	& M01 )	<< 1	|	\
	( (uint64) (HSINT)	& M01 )	<< 2	|	\
	( (uint64) (VSINT)	& M01 )	<< 3	|	\
	( (uint64) (FLUSH)	& M01 )	<< 8	|	\
	( (uint64) (RESET)	& M01 )	<< 9	|	\
	( (uint64) (NFIELD)	& M01 )	<< 12	|	\
	( (uint64) (FIELD)	& M01 )	<< 13	|	\
	( (uint64) (FIFO)	& M02 )	<< 14	|	\
	( (uint64) (REV)	& M08 )	<< 16	|	\
	( (uint64) (ID)		& M08 )	<< 24		\
)

#define PackDISPFB(FBP,FBW,PSM,DBX,DBY)		\
(											\
	( (uint64) (FBP)	& M09 )	<< 0	|	\
	( (uint64) (FBW)	& M06 )	<< 9	|	\
	( (uint64) (PSM)	& M05 )	<< 15	|	\
	( (uint64) (DBX)	& M11 )	<< 32	|	\
	( (uint64) (DBY)	& M11 )	<< 43		\
)

#define PackDISPLAY(DX,DY,MAGH,MAGV,DW,DH)	\
(											\
	( (uint64) (DX)		& M12 )	<< 0	|	\
	( (uint64) (DY)		& M11 )	<< 12	|	\
	( (uint64) (MAGH)	& M04 )	<< 23	|	\
	( (uint64) (MAGV)	& M02 )	<< 27	|	\
	( (uint64) (DW)		& M12 )	<< 32	|	\
	( (uint64) (DH)		& M11 )	<< 44		\
)

#define PackEXTBUF(EXBP,EXBW,FBIN,WFFMD,EMODA,EMODC,WDX,WDY)\
(											\
	( (uint64) (EXBP)	& M14 )	<< 0	|	\
	( (uint64) (EXBW)	& M06 )	<< 14	|	\
	( (uint64) (FBIN)	& M02 )	<< 20	|	\
	( (uint64) (WFFMD)	& M01 )	<< 22	|	\
	( (uint64) (EMODA)	& M02 )	<< 23	|	\
	( (uint64) (EMODC)	& M02 )	<< 25	|	\
	( (uint64) (WDX)	& M11 )	<< 32	|	\
	( (uint64) (WDY)	& M11 )	<< 43		\
)

#define PackEXTDATA(SX,SY,SMPH,SMPV,WW,WH)	\
(											\
	( (uint64) (SX)		& M12 )	<< 0	|	\
	( (uint64) (SY)		& M11 )	<< 12	|	\
	( (uint64) (SMPH)	& M04 )	<< 23	|	\
	( (uint64) (SMPV)	& M02 )	<< 27	|	\
	( (uint64) (WW)		& M12 )	<< 32	|	\
	( (uint64) (WH)		& M11 )	<< 44		\
)

#define PackEXTWRITE(WRITE)					\
(											\
	( (uint64) (WRITE)	& M01 )	<< 0		\
)

#define PackIMR(SIGMSK,FINISHMSK,HSMSK,VSMSK)\
(											\
	( (uint64) (SIGMSK)	& M01 )	<< 8	|	\
	( (uint64) (FINISHMSK)& M01)<< 9	|	\
	( (uint64) (HSMSK)	& M01 )	<< 10	|	\
	( (uint64) (VSMSK)	& M01 )	<< 11	|	\
	( (uint64)            M04 ) << 12		\
)

#define PackPMODE(EN1,EN2,CRTMD,MMOD,AMOD,SLBG,ALP)\
(											\
	( (uint64) (EN1)	& M01 )	<< 0	|	\
	( (uint64) (EN2)	& M01 )	<< 1	|	\
	( (uint64) (CRTMOD)	& M03 )	<< 2	|	\
	( (uint64) (MMOD)	& M01 )	<< 5	|	\
	( (uint64) (AMOD)	& M01 )	<< 6	|	\
	( (uint64) (SLBG)	& M01 )	<< 7	|	\
	( (uint64) (ALP)	& M08 )	<< 8		\
)

#define PackSIGLBLID(SIGID,LBLID)			\
(											\
	( (uint64) (SIGID)	& M32 )	<< 0	|	\
	( (uint64) (LBLID)	& M32 )	<< 32		\
)

#define PackSMODE2(INT,FFMD,DPMS)			\
(											\
	( (uint64) (INT)	& M01 )	<< 0	|	\
	( (uint64) (FFMD)	& M01 )	<< 1	|	\
	( (uint64) (DPMS)	& M02 )	<< 2		\
)




//------------------------------------------------------
//		R E G I S T E R   F I E L D   V A L U E S
//------------------------------------------------------


// drawing primitives
#define	POINT		0
#define	LINE		1
#define	LINESTRIP	2
#define	TRIANGLE	3
#define	TRISTRIP	4
#define	TRIFAN		5
#define	SPRITE		6

// PRIM bit masks   (GS Manual p. 116)
#define	IIP			(1<<3)		// Gouraud Shading
#define	TME			(1<<4)		// Texture Mapping
#define	FGE			(1<<5)		// Fogging
#define	ABE			(1<<6)		// Alpha Blending
#define	AA1			(1<<7)		// Pass 1 Antialiasing
#define	FST			(1<<8)		// UV values used (UV Register)
#define	CTXT		(1<<9)		// Context 2 used
#define	FIX			(1<<10)		// Fixed Fragment Value Control

// pixel storage modes
#define PSMCT32		0x00
#define PSMCT24		0x01
#define PSMCT16		0x02
#define PSMCT16S	0x0A
#define PS_GPU24	0x12
#define PSMT8		0x13
#define PSMT4		0x14
#define PSMT8H		0x1B
#define PSMT4HL		0x24
#define PSMT4HH		0x2C
#define PSMZ32		0x30
#define PSMZ24		0x31
#define PSMZ16		0x32
#define PSMZ16S		0x3A

// depth test methods
#define ZNEVER		0
#define ZALWAYS		1
#define ZGEQUAL		2
#define ZGREATER	3

// alpha test methods
#define	ANEVER		0
#define	AALWAYS		1
#define ALESS		2
#define ALEQUAL		3
#define	AEQUAL		4
#define	AGEQUAL		5
#define	AGREATER	6
#define	ANOTEQUAL	7

// alpha fail processing
#define	KEEP		0
#define	FB_ONLY		1
#define	ZB_ONLY		2
#define	RGB_ONLY	3

// texture functions
#define MODULATE	0
#define DECAL		1
#define HIGHLIGHT	2
#define HIGHLIGHT2	3

// texture filters
#define NEAREST					0
#define LINEAR					1
#define NEAREST_MIPMAP_NEAREST	2
#define NEAREST_MIPMAP_LINEAR	3
#define LINEAR_MIPMAP_NEAREST	4
#define LINEAR_MIPMAP_LINEAR	5

// texture wrap modes
#define REPEAT			0
#define CLAMP			1
#define REGION_CLAMP	2
#define REGION_REPEAT	3

namespace NxPs2
{

class gs
{

public:

	enum eReg
	{
		PRIM		= 0x00,		// GS p. 116  Drawing Primitive Setting (AC == 1)
		RGBAQ		= 0x01,		// GS p. 119  Vertex Color Setting
		ST			= 0x02,		// GS p. 123  Specification of Vertex Texture Coordinates (sets S and T)  FST == 0
		UV			= 0x03,		// GS p. 135  Specification of Vertex Texture Coordinates (sets U and V)  FST == 1
		XYZF2		= 0x04,		// GS p. 139  Setting for Vertex Coordinate Values (Fog coefficient)
		XYZ2		= 0x05,		// GS p. 137  Setting for Vertex Coordinate Values
		TEX0_1		= 0x06,		// GS p. 125  Texture Information Setting (type of textures to be used)
		TEX0_2		= 0x07,		//            (mode, dimensions, blendfunc, CLUT)
		CLAMP_1		= 0x08,		// GS p. 102  Texture wrap mode
		CLAMP_2		= 0x09,
		FOG			= 0x0A,		// GS p. 108  Vertex Fog Value Setting (per vert)
		XYZF3		= 0x0C,		// GS p. 140  Setting for Vertex Coordinate Values (without Drawing Kick) (Fog coefficient)
		XYZ3		= 0x0D,		// GS p. 138  Setting for Vertex Coordinate Values (without Drawing Kick)
		A_D			= 0x0E,		// Undocumented???
		NOP			= 0x0F,
		TEX1_1		= 0x14,		// GS p. 127  Texture Information Setting (LOD info)
		TEX1_2		= 0x15,
		TEX2_1		= 0x16,		// GS p. 128  Texture Information Setting (Format/CLUT)
		TEX2_2		= 0x17,
		XYOFFSET_1	= 0x18,		// GS p. 136  Offset Value setting (prim to window coords)
		XYOFFSET_2	= 0x19,
		PRMODECONT	= 0x1A,		// GS p. 118  Specification of Primitive Attribute Setting Method
		PRMODE		= 0x1B,		// GS p. 117  Setting for Attributes of Drawing Primitives (AC == 0)
		TEXCLUT		= 0x1C,		// GS p. 130  CLUT Position Specification (CLUT offset in buffer)
		SCANMSK		= 0x22,		// GS p. 120  Raster Address Mask Setting
		MIPTBP1_1	= 0x34,		// GS p. 113  MIPMAP Information Setting (Level 1 to 3)
		MIPTBP1_2	= 0x35,
		MIPTBP2_1	= 0x36,		// GS p. 114  MIPMAP Information Setting (Level 4 to 6)
		MIPTBP2_2	= 0x37,
		TEXA		= 0x3B,		// GS p. 129  Texture Alpha Value Setting (sets alpha when alpha not 8-bit)
		FOGCOL		= 0x3D,		// GS p. 109  Distant Fog Color Setting
		TEXFLUSH	= 0x3F,		// GS p. 131  Texture Page Buffer Disabling (wait for drawing to complete)
		SCISSOR_1	= 0x40,		// GS p. 121  Setting for Scissoring Area
		SCISSOR_2	= 0x41,
		ALPHA_1		= 0x42,		// GS p. 100  Alpha Blending Setting (Context 1)
		ALPHA_2		= 0x43,
		DIMX		= 0x44,		// GS p. 104  Dither Matrix Setting
		DTHE		= 0x45,		// GS p. 105  Dither Control (0 off / 1 on)
		COLCLAMP	= 0x46,		// GS p. 103  Color Clamp Control  (0 mask / 1 clamp)
		TEST_1		= 0x47,		// GS p. 124  Pixel Test Control (Alpha test/Depth Test method and enable/disable, etc.)
		TEST_2		= 0x48,
		PABE		= 0x49,		// GS p. 115  Alpha Blending Control in Units of Pixels
		FBA_1		= 0x4A,		// GS p. 106  Alpha Correction Value  (Sets fixed alpha value)
		FBA_2		= 0x4B,
		FRAME_1		= 0x4C,		// GS p. 110  Frame Buffer Setting
		FRAME_2		= 0x4D,
		ZBUF_1		= 0x4E,		// GS p. 141  Z Buffer Setting
		ZBUF_2		= 0x4F,
		BITBLTBUF	= 0x50,		// GS p. 101  Setting for transmission between buffers
		TRXPOS		= 0x51,		// GS p. 133  Specification of Transmission Areas in Buffers (direction)
		TRXREG		= 0x52,		// GS p. 134  Specification of Transmission Areas in Buffers
		TRXDIR		= 0x53,		// GS p. 132  Activation of Transmission between Buffers
		HWREG		= 0x54,		// GS p. 111  Data Port for Transmission between Buffers
		SIGNAL		= 0x60,		// GS p. 122  SIGNAL Event Occurrence Request
		FINISH		= 0x61,		// GS p. 107  Finish Event Occurance Request
		LABEL		= 0x62,		// GS p. 112  LABEL Event Occurrence Request
		A_D_NOP		= 0x7F		// Undocumented???
	};

	static void BeginPrim(uint FLG, uint Prim, uint Pre);
	static void EndPrim(uint Eop);
	static void Reg1(eReg Reg, uint64 Data);
	static void Reg2(eReg Reg, uint64 Data);
	static uint32 *gs::Reg2_pData(eReg Reg, uint64 Data);
	static void ModifyTBP(eReg Reg, uint32 *p_reg_ptr, sint16 tbp1, sint16 tbp2 = -1, sint16 tbp3 = -1);

}; // class gs



// output 1 GS register via PATH1, V3_32 format
inline void gs::Reg1(eReg Reg, uint64 Data)
{
//	dma::Store64(Data);
//	dma::Store32((uint64)Reg);
	uint32 *p_loc = (uint32*)dma::pLoc;
	p_loc[0] = (uint32)Data;
	p_loc[1] = Data>>32;
	p_loc[2] = Reg;
	dma::pLoc = (uint8*) (p_loc + 3);

}


} // namespace NxPs2





#endif // __GS_H

