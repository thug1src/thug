#include <kernel.h>
#include <sys/types.h>
#include <stdio.h>
#include <sif.h>
#include <sifcmd.h>
#include <sifrpc.h>
#include "bgm_i.h"

ModuleInfo Module = {"pcmaudio_driver", 0x0102 };

extern int sce_bgm_loop();
extern volatile int gStThid;

int create_th();

int start ()
{
	struct ThreadParam param;
	int th;

	CpuEnableIntr();

	if( ! sceSifCheckInit() )
		sceSifInit();
	sceSifInitRpc(0);

	printf("Entering PCM Audio Driver\n");

	param.attr         = TH_C;
	param.entry        = sce_bgm_loop;
	param.initPriority = BASE_priority-2;
	param.stackSize    = 0x800;
	param.option       = 0;
	th = CreateThread(&param);
	if (th > 0) {
		StartThread(th,0);
		printf("Exit PCM Audio Driver\n");
		return 0;
	}else{
		return 1;
	}
}


/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */

