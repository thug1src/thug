/*---------------------------------------------------------------------------*
  Project:  Dolphin hw Library
  File:     hwPad.c

  Copyright 1998-2001 Nintendo.  All rights reserved.

  These coded instructions, statements, and computer programs contain
  proprietary information of Nintendo of America Inc. and/or Nintendo
  Company Ltd., and are protected by Federal copyright law.  They may
  not be disclosed to third parties or copied or duplicated in any form,
  in whole or in part, without the prior written consent of Nintendo.

  $Log: /Dolphin/build/libraries/hw/src/hwPad.c $
    
    10    8/09/01 9:47 Shiki
    Fixed to clear pad delta status if PAD_ERR_TRANSFER is occurred.

    9     01/03/27 13:21 Shiki
    Detab.

    8     01/03/23 6:06p Yasu
    Change COMMENTed code to introduce pad-queue.

    7     01/03/22 21:44 Shiki
    Fixed hwPadRead() to conform to GameCube controller spec.
    Also refer to '/Dolphin/build/hws/padhw/src/cont.c'.

    6     2/14/01 1:48a Hirose
    Deleted first call check for hwPadInit( ).
    Now PadInit( ) can be called more than once.

    5     10/27/00 3:47p Hirose
    fixed build flags

    4     6/12/00 4:39p Hirose
    reconstructed structure and interface

    3     4/26/00 4:59p Carl
    CallBack -> Callback

    2     3/25/00 12:50a Hirose
    added some portion from cmn-pad.c
    added pad connection check

    1     3/23/00 1:21a Hirose
    Initial version

  $NoKeywords: $
 *---------------------------------------------------------------------------*/

#include <dolphin.h>
#include "p_hw.h"

/*---------------------------------------------------------------------------*
   Global Variables
 *---------------------------------------------------------------------------*/
hwPadStatus       hwPad[PAD_MAX_CONTROLLERS];
u32                 hwNumValidPads;

/*---------------------------------------------------------------------------*
   Local Variables
 *---------------------------------------------------------------------------*/
extern PADStatus padData[PAD_MAX_CONTROLLERS]; // internal use only

/*---------------------------------------------------------------------------*/
static u32 PadChanMask[PAD_MAX_CONTROLLERS] =
{
    PAD_CHAN0_BIT, PAD_CHAN1_BIT, PAD_CHAN2_BIT, PAD_CHAN3_BIT
};

/*---------------------------------------------------------------------------*
    Name:           hwPadCopy

    Description:    This function copies informations of PADStatus into
                    hwPadStatus structure. Also attaches some extra
                    informations such as down/up, stick direction.
                    This function is internal use only.

                    Keeps previous state if PAD_ERR_TRANSFER is returned.

    Arguments:      pad   : copy source. (PADStatus)
                    dmpad : copy destination. (hwPadStatus)

    Returns:        None
 *---------------------------------------------------------------------------*/
//static void hwPadCopy( PADStatus* pad, hwPadStatus* dmpad )
//{
//    u16  dirs;
//
//    if ( pad->err != PAD_ERR_TRANSFER )
//    {
//        // Detects which direction is the stick(s) pointing.
//        // This can be used when we want to use a stick as direction pad.
//        dirs = 0;
//        if ( pad->stickX    < - hw_STICK_THRESHOLD )
//            dirs |= hw_STICK_LEFT;
//        if ( pad->stickX    >   hw_STICK_THRESHOLD )
//            dirs |= hw_STICK_RIGHT;
//        if ( pad->stickY    < - hw_STICK_THRESHOLD )
//            dirs |= hw_STICK_DOWN;
//        if ( pad->stickY    >   hw_STICK_THRESHOLD )
//            dirs |= hw_STICK_UP;
//        if ( pad->substickX < - hw_STICK_THRESHOLD )
//            dirs |= hw_SUBSTICK_LEFT;
//        if ( pad->substickX >   hw_STICK_THRESHOLD )
//            dirs |= hw_SUBSTICK_RIGHT;
//        if ( pad->substickY < - hw_STICK_THRESHOLD )
//            dirs |= hw_SUBSTICK_DOWN;
//        if ( pad->substickY >   hw_STICK_THRESHOLD )
//            dirs |= hw_SUBSTICK_UP;
//
//        // Get the direction newly detected / released
//        dmpad->dirsNew      = PADButtonDown(dmpad->dirs, dirs);
//        dmpad->dirsReleased = PADButtonUp(dmpad->dirs, dirs);
//        dmpad->dirs         = dirs;
//
//        // Get DOWN/UP status of all buttons
//        dmpad->buttonDown = PADButtonDown(dmpad->pst.button, pad->button);
//        dmpad->buttonUp   = PADButtonUp(dmpad->pst.button, pad->button);
//
//        // Get delta of analogs
//        dmpad->stickDeltaX = (s16)(pad->stickX - dmpad->pst.stickX);
//        dmpad->stickDeltaY = (s16)(pad->stickY - dmpad->pst.stickY);
//        dmpad->substickDeltaX = (s16)(pad->substickX - dmpad->pst.substickX);
//        dmpad->substickDeltaY = (s16)(pad->substickY - dmpad->pst.substickY);
//
//        // Copy current status into DEMOPadStatus field
//        dmpad->pst = *pad;
//    }
//    else
//    {
//        // Get the direction newly detected / released
//        dmpad->dirsNew = dmpad->dirsReleased = 0;
//
//        // Get DOWN/UP status of all buttons
//        dmpad->buttonDown = dmpad->buttonUp = 0;
//
//
//
//
//        // Get delta of analogs
//        dmpad->stickDeltaX =    dmpad->stickDeltaY    = 0;
//        dmpad->substickDeltaX = dmpad->substickDeltaY = 0;
//    }
//}

/*---------------------------------------------------------------------------*
    Name:           hwPadRead

    Description:    Calls PADRead() and perform clamping. Get information
                    of button down/up and sets them into extended field.
                    This function also checks whether controllers are
                    actually connected.

    Arguments:      None

    Returns:        None
 *---------------------------------------------------------------------------*/
void hwPadRead( void )
{
    s32         i;
    u32         ResetReq = 0; // for error handling

    // Read current PAD status
    PADRead( padData );

    // Clamp analog inputs
    PADClamp( padData );

    hwNumValidPads = 0;
    for ( i = 0 ; i < PAD_MAX_CONTROLLERS ; i++ )
    {
        // Connection check
        if ( padData[i].err == PAD_ERR_NONE ||
			 padData[i].err == PAD_ERR_TRANSFER )
        {
            ++hwNumValidPads;
        }
        else if ( padData[i].err == PAD_ERR_NO_CONTROLLER )
        {
            ResetReq |= PadChanMask[i];
        }

//		hwPadCopy( &padData[i], &hwPad[i] );
    }

    // Try resetting pad channels which have been not valid
    if ( ResetReq )
    {
        // Don't care return status
        // If FALSE, then reset again in next hwPadRead.
        PADReset( ResetReq );
    }

    return;
}

/*---------------------------------------------------------------------------*
    Name:           hwPadInit

    Description:    Initialize PAD library and exported status

    Arguments:      None

    Returns:        None
 *---------------------------------------------------------------------------*/
void hwPadInit( void )
{
    s32         i;

	// Set pad analog mode.

	// Mode 3:
	// stickX, stickY				All 8 bits are valid. 
	// substickX, substickY			All 8 bits are valid. 
	// triggerLeft, triggerRight	All 8 bits are valid. 
	// analogA, analogB				All 8 bits are invalid and set to zero. 

	PADSetAnalogMode( PAD_MODE_3 );

    // Initialize pad interface
    PADInit();

    // Reset exported pad status
    for ( i = 0 ; i < PAD_MAX_CONTROLLERS ; i++ )
    {
        hwPad[i].pst.button = 0;
        hwPad[i].pst.stickX = 0;
        hwPad[i].pst.stickY = 0;
        hwPad[i].pst.substickX = 0;
        hwPad[i].pst.substickY = 0;
        hwPad[i].pst.triggerLeft = 0;
        hwPad[i].pst.triggerRight = 0;
        hwPad[i].pst.analogA = 0;
        hwPad[i].pst.analogB = 0;
        hwPad[i].pst.err = 0;
        hwPad[i].buttonDown = 0;
        hwPad[i].buttonUp = 0;
        hwPad[i].dirs = 0;
        hwPad[i].dirsNew = 0;
        hwPad[i].dirsReleased = 0;
        hwPad[i].stickDeltaX = 0;
        hwPad[i].stickDeltaY = 0;
        hwPad[i].substickDeltaX = 0;
        hwPad[i].substickDeltaY = 0;
    }

}

#if 0 // Currently this stuff is not used.
//============================================================================
//   PAD-QUEUE Functions: NOW WORKS
//
//   This set of functions helps the game engine with constant animation
//   rate.
//
//   [Sample Code]
//
//       BOOL  isRetraced;
//       while ( !gameDone )
//       {
//           do
//           {
//               isQueued = hwPadQueueRead( OS_MESSAGE_BLOCK );
//               Do_animation( );
//           }
//           while ( isQueued );
//
//           Do_rendering( );
//       }
//

//============================================================================
#define hw_PADQ_DEPTH    8

void hwPadQueueInit      ( void );
BOOL hwPadQueueRead      ( s32  );
void hwPadQueueFlush     ( void );

static  PADStatus       PadQueue[hw_PADQ_DEPTH][PAD_MAX_CONTROLLERS];
static  OSMessageQueue  PadValidMsgQ;
static  OSMessage       PadValidMsg[hw_PADQ_DEPTH];
static  OSMessageQueue  PadEmptyMsgQ;
static  OSMessage       PadEmptyMsg[hw_PADQ_DEPTH];

/*---------------------------------------------------------------------------*
    Name:           hwPadViCallback
    Description:    This function should be called once every frame.
    Arguments:      None
    Returns:        None
 *---------------------------------------------------------------------------*/
static void hwPadViCallback( u32 retraceCount )
{
#pragma  unused (retraceCount)

    PADStatus   *padContainer;
#ifdef  _DEBUG
    static BOOL caution = FALSE;
#endif

    // Get empty container.
    if ( OSReceiveMessage( &PadEmptyMsgQ,
                           (OSMessage *)&padContainer, OS_MESSAGE_NOBLOCK ) )
    {
        // Read the latest pad status into pad container
        PADRead( padContainer );

        // Send result as message
        if ( !OSSendMessage( &PadValidMsgQ,
                             (OSMessage)padContainer, OS_MESSAGE_NOBLOCK ) )
        {
            // The valid queue never be full.
            ASSERTMSG( 0, "Logical Error: Valid QUEUE is full." );
        }
#ifdef  _DEBUG
        caution = FALSE;
#endif
    }
    else
    {
#ifdef  _DEBUG
        ASSERTMSG( caution, "Pad Queue is full." );
        caution = TRUE;
#endif
    }

    return;
}

/*---------------------------------------------------------------------------*
    Name:           hwPadQueueRead
    Description:    Read gamepad state and set to hwPad[].
                    No need to care controller error.
                    (When error, PADState is set to zero in PADRead.)
    Arguments:      s32 flag: control block/noblock mode.
                       when flag == OS_MESSAGE_BLOCK,
                              wait next padinput if queue is empty.
                       when flag == OS_MESSAGE_NOBLOCK,
                              return immediately if queue is empty.

    Returns:        BOOL: FALSE if queue was empty.
                          TRUE  if get queued data.
 *---------------------------------------------------------------------------*/
BOOL hwPadQueueRead( s32 flag )
{
    PADStatus   *padContainer;
    BOOL        isQueued;
    u32         i;

    // Get pad data from valid data queue
    isQueued = OSReceiveMessage( &PadValidMsgQ,
                                 (OSMessage *)&padContainer, OS_MESSAGE_NOBLOCK );

    // If queue is empty, wait and sleep until coming pad input on v-retrace
    if ( !isQueued )
    {
        if ( flag == OS_MESSAGE_BLOCK )
        {
            OSReceiveMessage( &PadValidMsgQ,
                              (OSMessage *)&padContainer, OS_MESSAGE_BLOCK );
        }
        else
        {
            return FALSE;
        }
    }

    // Copy status to hwPad
    for ( i = 0 ; i < PAD_MAX_CONTROLLERS ; i ++ )
    {
        hwPadCopy( &padContainer[i], &hwPad[i] );
    }

    // Release pad container
    if ( !OSSendMessage( &PadEmptyMsgQ,
                         (OSMessage)padContainer, OS_MESSAGE_NOBLOCK ) )
    {
        // The valid queue never be full.
        ASSERTMSG( 0, "Logical Error: Empty QUEUE is full." );
    }

    return isQueued;
}

/*---------------------------------------------------------------------------*
    Name:           hwPadQueueFlush
    Description:    Flush Pad-Queue
    Arguments:      None
    Returns:        None
 *---------------------------------------------------------------------------*/
void hwPadQueueFlush( void )
{
    OSMessage   msg;

    while ( OSReceiveMessage( &PadValidMsgQ, &msg, OS_MESSAGE_NOBLOCK ) )
    {
        OSSendMessage( &PadEmptyMsgQ, msg, OS_MESSAGE_BLOCK );
    }

    return;
}

/*---------------------------------------------------------------------------*
    Name:           hwPadQueueInit
    Description:    Initialize Pad-Queue utility routines
    Arguments:      None
    Returns:        None
 *---------------------------------------------------------------------------*/
void hwPadQueueInit( void )
{
    u32         i;

    // Initialize basic pad function
    hwPadInit();

    OSInitMessageQueue( &PadValidMsgQ, &PadValidMsg[0], hw_PADQ_DEPTH );
    OSInitMessageQueue( &PadEmptyMsgQ, &PadEmptyMsg[0], hw_PADQ_DEPTH );

    // Entry pad container
    for ( i = 0; i < hw_PADQ_DEPTH; i ++ )
    {
        if ( !OSSendMessage( &PadEmptyMsgQ,
                             (OSMessage)&PadQueue[i][0], OS_MESSAGE_NOBLOCK ) )
        {
            ASSERTMSG( 0, "Logical Error: Send Message." );
        }
    }

    // Reset pad queue and initialize pad HW
    hwPadQueueFlush();

    // Register vi Callback function
    VISetPostRetraceCallback( hwPadViCallback );

    return;
}
/*---------------------------------------------------------------------------*/
#endif // 0


/*===========================================================================*/
