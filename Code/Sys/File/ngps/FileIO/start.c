/* FileIO -- Converted from Sony samples -- Garrett Oct 2002 */

#include <kernel.h>
#include <sys/types.h>
#include <stdio.h>
#include <sif.h>
#include <sifrpc.h>
#include <sifcmd.h>
#include "fileio.h"
#include "fileio_iop.h"

ModuleInfo Module = {"fileio_driver", 0x0112};

// in command.c
extern int fileio_command_loop (void);

int start()
{
    struct ThreadParam param;
    int th;

    if (!sceSifCheckInit())
	{
		sceSifInit();
	}
    sceSifInitRpc(0);

    printf("FileIO driver version 0.1\n");

    param.attr         = TH_C;
    param.entry        = fileio_command_loop;
    param.initPriority = BASE_priority - 2;
    param.stackSize    = 0x800;
    param.option       = 0;
    th = CreateThread(&param);
    if (th > 0) {
		StartThread(th, 0);
    } else {
		return 1;
    }

	return 0;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
