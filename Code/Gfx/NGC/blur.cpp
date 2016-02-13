/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GFX (Graphics Library)									**
**																			**
**	Module:			Graphics (GFX)		 									**
**																			**
**	File name:		blur.cpp												**
**																			**
**	Created:		06/21/00	-	mjb										**
**																			**
**	Description:	Motion-Blur effect										**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

//#include <eetypes.h>
//#include <eestruct.h>
//#include <eekernel.h>
//#include <eeregs.h>
//				 
//#include <core/defines.h>
//
///*****************************************************************************
//**								DBG Information								**
//*****************************************************************************/
//
//
///*****************************************************************************
//**								  Externals									**
//*****************************************************************************/
//
//extern int skyFrameBit;
//
//namespace Gfx
//{
//
///*****************************************************************************
//**								   Defines									**
//*****************************************************************************/
//
///*****************************************************************************
//**								Private Types								**
//*****************************************************************************/
//
//typedef struct
//{
//	sceGifTag       giftag;
//	u_int       	data_buf[0x80];
//} GsClearData __attribute__((aligned(16)));
//
///*****************************************************************************
//**								 Private Data								**
//*****************************************************************************/
//
///*****************************************************************************
//**								 Public Data								**
//*****************************************************************************/
//
///*****************************************************************************
//**							  Private Prototypes							**
//*****************************************************************************/
//
///*****************************************************************************
//**							   Private Functions							**
//*****************************************************************************/
//
//void copy_frame(void)
//{
//	int i,nloop;
//	GsClearData gcd;
//
//	nloop = 25;
//
//	SCE_GIF_CLEAR_TAG(&gcd.giftag);
//	gcd.giftag.NLOOP = nloop;
//	gcd.giftag.EOP = 1;
//	gcd.giftag.NREG = 1;
//	gcd.giftag.REGS0 = 0xe;	// A_D
//
//	i=0;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x0000001a; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000001; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x0000004c; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x000a0046; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000047; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000040; gcd.data_buf[i+1] = (u_int)0x00df0000; gcd.data_buf[i] = (u_int)0x027f0000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000042; gcd.data_buf[i+1] = (u_int)0x00000040; gcd.data_buf[i] = (u_int)0x00000064; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000008; gcd.data_buf[i+1] = (u_int)0x0000037c; gcd.data_buf[i] = (u_int)0x009fc00a; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000018; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000046; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000001; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000049; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000006; gcd.data_buf[i+1] = (u_int)0x00000006; gcd.data_buf[i] = (u_int)0xa8028000; i+=4;
//
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000014; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000060; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x0000003b; gcd.data_buf[i+1] = (u_int)0x00000080; gcd.data_buf[i] = (u_int)0x00000080; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000001; gcd.data_buf[i+1] = (u_int)0x3f800000; gcd.data_buf[i] = (u_int)0x80808080; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000000; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x0000015e; i+=4;
//
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000003; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000004; gcd.data_buf[i+1] = (u_int)0x00000001; gcd.data_buf[i] = (u_int)0x00100000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000003; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x0e002800; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000004; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x0e102800; i+=4;
//
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x0000003f; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000000; i+=4;
//
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000003; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00100000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000004; gcd.data_buf[i+1] = (u_int)0x00000001; gcd.data_buf[i] = (u_int)0x00100000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000003; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x0e102800; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000004; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x0e102800; i+=4;
//
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x0000003f; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000008; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000005; i+=4;
//
//	FlushCache(0);
//
//	*D2_QWC = gcd.giftag.NLOOP+1;
//
//	*D2_MADR = (u_int)&gcd.giftag & 0x0fffffff;
//	FlushCache(0);
//	*D2_CHCR = (1 << 8) | 1;
//
//	while( (*D2_CHCR & 0x0100) || (*GIF_STAT & 0x0c00) || ((*GS_CSR & 0x4000) == 0) );
//
//}
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//
//void copy_frame1(void)
//{
//	int i,nloop;
//	GsClearData gcd;
//
//	nloop = 25;
//
//	SCE_GIF_CLEAR_TAG(&gcd.giftag);
//	gcd.giftag.NLOOP = nloop;
//	gcd.giftag.EOP = 1;
//	gcd.giftag.NREG = 1;
//	gcd.giftag.REGS0 = 0xe;	// A_D
//
//	i=0;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x0000001a; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000001; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x0000004c; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x000a0000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000047; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000040; gcd.data_buf[i+1] = (u_int)0x00df0000; gcd.data_buf[i] = (u_int)0x027f0000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000042; gcd.data_buf[i+1] = (u_int)0x00000040; gcd.data_buf[i] = (u_int)0x00000064; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000008; gcd.data_buf[i+1] = (u_int)0x0000037c; gcd.data_buf[i] = (u_int)0x009fc00a; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000018; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00080008; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000046; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000001; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000049; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000006; gcd.data_buf[i+1] = (u_int)0x00000006; gcd.data_buf[i] = (u_int)0xa80288c0; i+=4;
//
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000014; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000060; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x0000003b; gcd.data_buf[i+1] = (u_int)0x00000080; gcd.data_buf[i] = (u_int)0x00000080; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000001; gcd.data_buf[i+1] = (u_int)0x3f800000; gcd.data_buf[i] = (u_int)0x80808080; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000000; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x0000015e; i+=4;
//
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000003; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000004; gcd.data_buf[i+1] = (u_int)0x00000001; gcd.data_buf[i] = (u_int)0x00000000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000003; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x0e002800; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000004; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x0e002800; i+=4;
//
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x0000003f; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000000; i+=4;
//
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000003; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00100000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000004; gcd.data_buf[i+1] = (u_int)0x00000001; gcd.data_buf[i] = (u_int)0x00000000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000003; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x0e102800; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000004; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x0e002800; i+=4;
//
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x0000003f; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000000; i+=4;
//	gcd.data_buf[i+3] = (u_int)0x0; gcd.data_buf[i+2] = (u_int)0x00000008; gcd.data_buf[i+1] = (u_int)0x00000000; gcd.data_buf[i] = (u_int)0x00000005; i+=4;
//
//	FlushCache(0);
//
//	*D2_QWC = gcd.giftag.NLOOP+1;
//
//	*D2_MADR = (u_int)&gcd.giftag & 0x0fffffff;
//	FlushCache(0);
//	*D2_CHCR = (1 << 8) | 1;
//
//	while( (*D2_CHCR & 0x0100) || (*GIF_STAT & 0x0c00) || ((*GS_CSR & 0x4000) == 0) );
//
//}
//
///*****************************************************************************
//**							   Public Functions								**
//*****************************************************************************/
//
//void	Blur( void )
//{
//	if (skyFrameBit & 0x1)
//	{
//		copy_frame1();
//	}
//	else
//	{
//		copy_frame();
//	}
//
//}
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//
//
//} // namespace Gfx
//
//