// This code builds an IRX module that handles PCM streaming from
// the CD to SPU2 Sound Data Input area...

#include <kernel.h>
#include <sys/types.h>
#include <stdio.h>
#include "sif.h"
#include "sifcmd.h"
#include "sifrpc.h"
#include <libsd.h>
#include "bgm_i.h"

int gRpcArg[16];	//--- Receiving channel for RPC arguments transferred from EE


static void* bgmFunc(unsigned int fno, void *data, int size);

extern int  BgmInit(int ch, int useCD);
extern void BgmQuit(int channel, int status );
extern int  BgmOpen( int ch, char* filename );
extern void BgmClose(int ch, int status);
extern void BgmCloseNoWait(int ch, int status);
extern int  BgmPreLoad( int ch, int status );
extern void BgmStart(int ch, int status);
extern void BgmStop(int ch, int status);
extern void BgmSeek( int ch, unsigned int vol );
extern int BgmSetVolume( int ch, unsigned int vol );
extern int BgmRaw2Spu( int ch, int which, int mode );
extern int BgmSetVolumeDirect( int ch, unsigned int vol );
// fuck this you big assbreath.
//extern void BgmSetMasterVolume( int ch, unsigned int vol );
extern unsigned int BgmGetMode( int ch, int status );
//extern void BgmSetMode( int ch, u_int mode );
extern void BgmSdInit(int ch, int status );


/* ------------------------------------------------------------------------
   Main thread for the ezbgm module.
   After execution, initialize interrupt environment, register command, and
   then wait until there is a request from the EE.
   ------------------------------------------------------------------------ */
int sce_bgm_loop()
{
	sceSifQueueData qd;
	sceSifServeData sd;

	//-- Initialize interrupt environment in advance.

	CpuEnableIntr();
	EnableIntr( INUM_DMA_4 );
	EnableIntr( INUM_DMA_7 );
	
	//--- Register function that is called according to request


	sceSifInitRpc(0);

	sceSifSetRpcQueue( &qd, GetThreadId() );
	sceSifRegisterRpc( &sd, EZBGM_DEV, bgmFunc, (void*)gRpcArg, NULL, NULL, &qd );
	PRINTF(("goto bgm cmd loop\n"));

	//--- Command-wait loop

	sceSifRpcLoop(&qd);

	return 0;
}


/* ------------------------------------------------------------------------
   Function that is awakened by a request from the EE.
   The argument is stored in *data.  The leading four bytes are reserved and are not used.
   The return value of this function becomes the return value of the EE side's RPC.
   When the argument is a structure, it is sent to gRpcData and used.
   When a structure is returned to the EE, the value is sent to the address of the first argument (on the EE side).
   ------------------------------------------------------------------------ */
int ret = 0;

static void* bgmFunc(unsigned int command, void *data, int size)
{ 
	int ch;

//	asm volatile( "break 1");

//	PRINTF(( " bgmfunc %x, %x, %x, %x\n", *((int*)data + 0), 
//		*((int*)data + 1), *((int*)data + 2),*((int*)data + 3) ));

	ch = command&0x000F;

	switch( command&0xFFF0 )
	{
	case EzBGM_INIT:
		ret = BgmInit( ch, *((int*)data) );
		break;
	case EzBGM_QUIT:
		BgmQuit ( ch, *((int*)data) );
		break;
	case EzBGM_OPEN:
		ret = BgmOpen ( ch, (char*)((int*)data) );
		break;
	case EzBGM_CLOSE:
		BgmClose( ch, *((int*)data) );
		break;
	case EzBGM_CLOSE_NO_WAIT:
		BgmCloseNoWait( ch, *((int*)data) );
		break;
	case EzBGM_PRELOAD:
		ret = BgmPreLoad ( ch, *((int*)data) );
		break;
	case EzBGM_START:
		BgmStart( ch, *((int*)data) );
		break;
	case EzBGM_STOP:
		BgmStop( ch, *((int*)data) );
		break;
	case EzBGM_SEEK:
		BgmSeek( ch, (unsigned int)*((int*)data) );
		break;
	case EzBGM_SETVOL:
		ret = BgmSetVolume( ch, (unsigned int)*((int*)data) );
		break;
	case EzBGM_SETVOLDIRECT:
		ret = BgmSetVolumeDirect( ch, (unsigned int)*((int*)data) );
		break;
	case EzBGM_GETMODE:
		ret = BgmGetMode( ch, *((int*)data) );
		break;
	case EzBGM_SDINIT:
		BgmSdInit( ch, *((int*)data) );
		break;
	default:
		ERROR(("EzBGM driver error: unknown command %d \n", *((int*)data) ));
		break;
	}
//	PRINTF(( "return value = %x \n", ret )); 
	return (void*)(&ret);
}


/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */

