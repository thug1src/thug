/*---------------------------------------------------------------------------*
  Project:  Dolphin hw library
  File:     hw.h

  Copyright 1998-2001 Nintendo.  All rights reserved.

  These coded instructions, statements, and computer programs contain
  proprietary information of Nintendo of America Inc. and/or Nintendo
  Company Ltd., and are protected by Federal copyright law.  They may
  not be disclosed to third parties or copied or duplicated in any form,
  in whole or in part, without the prior written consent of Nintendo.

  $Log: /Dolphin/include/hw.h $
    
    27    8/22/01 7:20p Carl
    BypassWorkaround is now GPHangWorkaround.
    Added DEMOSetGPHangMetric for hang diagnosis.
    
    26    5/09/01 10:10p Hirose
    moved hwPad/hwPuts/hwStat definitions into each individual header
    file.
    
    25    5/03/01 4:17p Tian
    Restored static inline changes
    
    24    01/04/25 14:28 Shiki
    Revised ROM font API interface. Added hwGetTextWidth().

    23    01/04/25 11:28 Shiki
    Added hwSetFontSize().

    22    01/04/19 13:37 Shiki
    Added ROM font functions.

    20    3/23/01 12:03p John
    Added stdio.h include to compensate for loss of geoPalette.h.

    19    3/22/01 2:38p John
    Removed geoPalette.h include (unnecessary).

    18    01/03/22 21:45 Shiki
    Removed hw_PAD_CHECK_INTERVAL.

    17    11/28/00 8:18p Hirose
    Enhancement of hwStat library
    Fixed hwSetTevOp definition for the emulator

    16    11/27/00 4:57p Carl
    Added hwSetTevColorIn and hwSetTevOp.

    15    10/26/00 10:31a Tian
    Added hwReInit and hwEnableBypassWorkaround.  Automatically repairs
    graphics pipe after a certain framecount based timeout.

    14    7/21/00 1:48p Carl
    Removed hwDoneRenderBottom.
    Added hwSwapBuffers.

    13    6/20/00 10:37a Alligator
    added texture bandwidth, texture miss rate calculations

    12    6/19/00 3:23p Alligator
    added fill rate virtual counter

    11    6/12/00 4:32p Hirose
    updated hwPad library structures

    10    6/12/00 1:46p Alligator
    updated hw statistics to support new api

    9     6/06/00 12:02p Alligator
    made changes to perf counter api

    8     6/05/00 1:53p Carl
    Added hwDoneRenderBottom

    7     5/18/00 2:56a Alligator
    added hwStats stuff

    6     5/02/00 3:28p Hirose
    added prototype of hwGetCurrentBuffer

    5     3/25/00 12:48a Hirose
    added hw_PAD_CHECK_INTERVAL macro

    4     3/23/00 1:20a Hirose
    added hwPad stuff

    3     1/19/00 3:43p Danm
    Added GXRenderModeObj *hwGetRenderModeObj(void) function.

    2     1/13/00 8:56p Danm
    Added GXRenderModeObj * parameter to hwInit()

    9     9/30/99 2:13p Ryan
    sweep to remove gxmodels libs

    8     9/28/99 6:56p Yasu
    Add defines of font type

    7     9/24/99 6:45p Yasu
    Change type of parameter of hwSetupScrnSpc().

    6     9/24/99 6:40p Yasu
    Change the number of parameter of hwSetupScrnSpc()

    5     9/24/99 6:35p Yasu
    Add hwSetupScrnSpc()

    4     9/23/99 5:35p Yasu
    Change function name cmMtxScreen to hwMtxScreen

    3     9/17/99 1:33p Ryan
    Added hwBeforeRender and hwDoneRender

    2     9/14/99 5:08p Yasu
    Add some functions which contained in hwPut.c

    1     7/23/99 2:36p Ryan

    1     7/20/99 6:42p Alligator
    new hw lib
  $NoKeywords: $
 *---------------------------------------------------------------------------*/

#ifndef __hw_H__
#define __hw_H__

/*---------------------------------------------------------------------------*/
#include <dolphin.h>
#include <charPipeline/texPalette.h>
#include <stdio.h>

#include <dolphin.h>
#include "p_hwpad.h"
//#include <hwPuts.h>
//#include <hwStats.h>
/*---------------------------------------------------------------------------*/


#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_FIFO_SIZE (1024 * 96)

/*---------------------------------------------------------------------------*
    hwInit.c
 *---------------------------------------------------------------------------*/
extern void             hwInit             ( GXRenderModeObj* mode );
extern void             hwBeforeRender     ( void );
extern void             hwDoneRender       ( bool clear = true );
extern int				hwLockBackBuffer   ( void ** buf_addr, unsigned int * width, unsigned int * height );
extern void				hwUnlockBackBuffer ( int updated );
extern void             hwSwapBuffers      ( void );
extern GXRenderModeObj* hwGetRenderModeObj ( void );
extern void*            hwGetCurrentBuffer ( void );

extern void             hwEnableGPHangWorkaround ( u32 timeoutFrames );
extern void             hwReInit           ( GXRenderModeObj *mode );
extern void             hwSetGPHangMetric  ( GXBool enable );
extern void             hwGXInit           ( void );


/*---------------------------------------------------------------------------*
    hw misc
 *---------------------------------------------------------------------------*/

// The hw versions of SetTevColorIn and SetTevOp are backwards compatible
// with the Rev A versions in that the texture swap mode will be set
// appropriately if one of TEXC/TEXRRR/TEXGGG/TEXBBB is selected.

#if ( GX_REV == 1 || defined(EMU) )
static inline void hwSetTevColorIn(GXTevStageID stage,
                              GXTevColorArg a, GXTevColorArg b,
                              GXTevColorArg c, GXTevColorArg d )
    { GXSetTevColorIn(stage, a, b, c, d); }

static inline void hwSetTevOp(GXTevStageID stage, GXTevMode mode)
    { GXSetTevOp(stage, mode); }
#else
void hwSetTevColorIn(GXTevStageID stage,
                       GXTevColorArg a, GXTevColorArg b,
                       GXTevColorArg c, GXTevColorArg d );

void hwSetTevOp(GXTevStageID stage, GXTevMode mode);
#endif

/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif // __hw_H__

/*===========================================================================*/
