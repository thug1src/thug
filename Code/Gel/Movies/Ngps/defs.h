#ifndef _DEFS_H_
#define _DEFS_H_

#include <eeregs.h>
#include <eetypes.h>
#include <libmpeg.h>

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/macros.h>
#include <core/singleton.h>

#define STRFILE_NUM_STREAMING_SECTORS 80

#define UNCMASK 0x0fffffff
#define UNCBASE 0x20000000

#define MAX_WIDTH 720
#define MAX_HEIGHT 576

#define MOVIE_THREAD_PRIORITY 1
#define AUTODMA_CH 0

#define N_LDTAGS (MAX_WIDTH/16 * MAX_HEIGHT/16 * 6 + 10)
#define TS_NONE (-1)

#define bound(val, x) ((((val) + (x) - 1) / (x))*(x))
#define min(x, y) (((x) > (y))? (y): (x))
#define max(x, y) (((x) < (y))? (y): (x))

extern inline void *DmaAddr(void *val)
{
    return (void*)((u_int)val & UNCMASK);
}

extern inline void *UncAddr(void *val)
{
    return (void*)(((u_int)val & UNCMASK)|UNCBASE);
}
 
namespace Flx
{



void ErrMessage(char *message);
void switchThread();
void proceedAudio();

}



#endif // _DEFS_H_
