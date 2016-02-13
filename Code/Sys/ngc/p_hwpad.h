/*---------------------------------------------------------------------------*
  Project:  Dolphin hw library
  File:     hwPad.h

  Copyright 1998-2001 Nintendo.  All rights reserved.

  These coded instructions, statements, and computer programs contain
  proprietary information of Nintendo of America Inc. and/or Nintendo
  Company Ltd., and are protected by Federal copyright law.  They may
  not be disclosed to third parties or copied or duplicated in any form,
  in whole or in part, without the prior written consent of Nintendo.

  $Log: /Dolphin/include/hw/hwPad.h $
    
    1     5/09/01 9:37p Hirose
    separated from hw.h
    
  $NoKeywords: $
 *---------------------------------------------------------------------------*/

#ifndef __hwPAD_H__
#define __hwPAD_H__


#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*
    hwPad.c
 *---------------------------------------------------------------------------*/
// used to detect which direction is the stick(s) pointing
#define hw_STICK_THRESHOLD      48

#define hw_STICK_UP             0x1000
#define hw_STICK_DOWN           0x2000
#define hw_STICK_LEFT           0x4000
#define hw_STICK_RIGHT          0x8000
#define hw_SUBSTICK_UP          0x0100
#define hw_SUBSTICK_DOWN        0x0200
#define hw_SUBSTICK_LEFT        0x0400
#define hw_SUBSTICK_RIGHT       0x0800

// extended pad status structure
typedef struct
{
    // contains PADStatus structure
    PADStatus   pst;

    // extended field
    u16         buttonDown;
    u16         buttonUp;
    u16         dirs;
    u16         dirsNew;
    u16         dirsReleased;
    s16         stickDeltaX;
    s16         stickDeltaY;
    s16         substickDeltaX;
    s16         substickDeltaY;
} hwPadStatus;

// the entity which keeps current pad status
extern hwPadStatus    hwPad[PAD_MAX_CONTROLLERS];
extern u32              hwNumValidPads;

// main function prototypes
extern void     hwPadInit( void );
extern void     hwPadRead( void );

// inline functions for getting each component
static inline u16 hwPadGetButton(u32 i)
    { return hwPad[i].pst.button; }

static inline u16 hwPadGetButtonUp(u32 i)
    { return hwPad[i].buttonUp; }

static inline u16 hwPadGetButtonDown(u32 i)
    { return hwPad[i].buttonDown; }

static inline u16 hwPadGetDirs(u32 i)
    { return hwPad[i].dirs; }

static inline u16 hwPadGetDirsNew(u32 i)
    { return hwPad[i].dirsNew; }

static inline u16 hwPadGetDirsReleased(u32 i)
    { return hwPad[i].dirsReleased; }

static inline s8  hwPadGetStickX(u32 i)
    { return hwPad[i].pst.stickX; }

static inline s8  hwPadGetStickY(u32 i)
    { return hwPad[i].pst.stickY; }

static inline s8  hwPadGetSubStickX(u32 i)
    { return hwPad[i].pst.substickX; }

static inline s8  hwPadGetSubStickY(u32 i)
    { return hwPad[i].pst.substickY; }

static inline u8  hwPadGetTriggerL(u32 i)
    { return hwPad[i].pst.triggerLeft; }

static inline u8  hwPadGetTriggerR(u32 i)
    { return hwPad[i].pst.triggerRight; }

static inline u8  hwPadGetAnalogA(u32 i)
    { return hwPad[i].pst.analogA; }

static inline u8  hwPadGetAnalogB(u32 i)
    { return hwPad[i].pst.analogB; }

static inline s8  hwPadGetErr(u32 i)
    { return hwPad[i].pst.err; }

/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif // __hwPAD_H__

/*===========================================================================*/
