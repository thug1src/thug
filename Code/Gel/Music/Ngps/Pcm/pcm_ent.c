/* Vag streaming into SPU2 -- Converted from Sony samples -- matt may 2001 */

#include <kernel.h>
#include <sys/types.h>
#include <stdio.h>
#include <sif.h>
#include <sifcmd.h>
#include <sifrpc.h>
#include "pcm.h"
#include "pcmiop.h"

ModuleInfo Module = {"pcm_driver", 0x0102};

// in command.c
extern int sce_adpcm_loop (void);
extern int sce_sound_loop (void);

int start ( void )
{
    struct ThreadParam param;
    int th;

    if (! sceSifCheckInit ())
	sceSifInit ();
    sceSifInitRpc (0);

    printf ("PCM driver version 666\n");

    param.attr         = TH_C;
    param.entry        = sce_adpcm_loop;
    param.initPriority = BASE_priority - 2;
    param.stackSize    = 0x800;
    param.option       = 0;
    th = CreateThread (&param);
    if (th > 0) {
		StartThread (th, 0);
    } else {
		return 1;
    }

    param.attr         = TH_C;
    param.entry        = sce_sound_loop;
    param.initPriority = BASE_priority - 2;
    param.stackSize    = 0x800;
    param.option       = 0;
    th = CreateThread (&param);
    if (th > 0) {
		StartThread (th, 0);
        printf (" Exit PCM loader thread \n");
		return 0;
	} else {
		return 1;
	}
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */
