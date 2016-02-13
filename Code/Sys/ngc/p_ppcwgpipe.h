/*---------------------------------------------------------------------------*
  Project: Write gather pipe definitions
  File:    PPCWGPipe.h

  Copyright 1998, 1999 Nintendo.  All rights reserved.

  These coded instructions, statements, and computer programs contain
  proprietary information of Nintendo of America Inc. and/or Nintendo
  Company Ltd., and are protected by Federal copyright law.  They may
  not be disclosed to third parties or copied or duplicated in any form,
  in whole or in part, without the prior written consent of Nintendo.

  $Log: /Dolphin/include/dolphin/base/PPCWGPipe.h $
    
    2     11/01/00 3:55p Shiki
    Removed #ifdef EPPC.

    1     7/14/99 6:59p Yoshya01
    Initial version.
  $NoKeywords: $
 *---------------------------------------------------------------------------*/
#ifndef __PPCWGPIPE_H__
#define __PPCWGPIPE_H__

#ifdef  __cplusplus
extern  "C" {
#endif

#include <dolphin/types.h>

/*---------------------------------------------------------------------------*
    PPC Write Gather Pipe

    Write Gather Pipe is defined as:
        PPCWGPipe wgpipe : <Write Gathered Address>;

    Then, used as:
        wgpipe.u8  = 0xff;
        wgpipe.s16 = -5;
        wgpipe.f32 = 0.10f;
 *---------------------------------------------------------------------------*/

#ifdef __SN__

typedef  s8 ts8;
typedef  s16 ts16;
typedef  s32 ts32;
typedef  s64 ts64;
typedef  u8 tu8;
typedef  u16 tu16;
typedef  u32 tu32;
typedef  u64 tu64;
typedef  f32 tf32;
typedef  f64 tf64;

typedef volatile union u_PPCWGPipe
{
    tu8  u8;
    tu16 u16;
    tu32 u32;
    tu64 u64;
    ts8  s8;
    ts16 s16;
    ts32 s32;
    ts64 s64;
    tf32 f32;
    tf64 f64;
} PPCWGPipe;

#else

typedef volatile union PPCWGPipe
{
    u8  u8;
    u16 u16;
    u32 u32;
    u64 u64;
    s8  s8;
    s16 s16;
    s32 s32;
    s64 s64;
    f32 f32;
    f64 f64;
} PPCWGPipe;

#endif


#ifdef  __cplusplus
}
#endif

#endif  //__PPCWGPIPE_H__
