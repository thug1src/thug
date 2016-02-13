/*!
 ******************************************************************************
 * \file AUDSimplePlayer.h
 *
 * \brief
 *		Header file for AUDSimplePlayer interface
 *
 * \date
 *		5/21/03
 *
 * \version
 *		1.0
 *
 * \author
 *		Achim Moller
 *
 ******************************************************************************
 */
#ifndef __AUDSIMPLE_PLAYER_H__
#define __AUDSIMPLE_PLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 *  INCLUDES
 ******************************************************************************/
#include <dolphin/types.h>
#include <demo.h>
#include "vaud.h"

/******************************************************************************
 *  DEFINES
 ******************************************************************************/

//! Number of 'read-ahead' buffers
#define AUD_NUM_READ_BUFFERS	10
#define	ASYNC_OPEN_FAIL			666
#define	ASYNC_OPEN_SUCCESS		999

/******************************************************************************
 *  STRUCTS AND TYPES
 ******************************************************************************/

typedef struct {
	u8*				ptr;
	u32				size;
	u32				frameNumber;
	volatile BOOL 	valid;
} AudReadBuffer;

//! Required fields for simple demonstation player
typedef struct {

	VAUDAllocator 	cbAlloc;			// memory allocator
	VAUDDeallocator cbFree;				// memory deallocator

    DVDFileInfo		fileHandle;
	char			fileName[64];

	VidAUDH			audioInfo;			// common header and first 2 channels (or all channels if not ADPCM)
    u8*				audioHeaderChunk;
	
	BOOL            open;
	BOOL            error;
	BOOL			preFetchState;
	BOOL			loopMode;
	BOOL			asyncDvdRunning;
	u32				asyncOpenCallbackStatus;	// Value indicating status of callback position for async startup

	BOOL			decodeComplete;
	BOOL			playbackComplete;
	
	u32				nextFrameOffset;
	u32				nextFrameSize;
	u32				currentFrameCount;
	u32 			lastDecodedFrame;
	
	u32 			firstFrameOffset;
	u32				firstFrameSize;
	
	u32             readIndex;
	u32				decodeIndex;
	
	AudReadBuffer	readBuffer[AUD_NUM_READ_BUFFERS];
	u8*				readBufferBaseMem;
	
	VAUDDecoder		decoder;

} AudSimplePlayer;

/******************************************************************************
 *  PROTOTYPES
 ******************************************************************************/
extern void AUDSimpleInit(VAUDAllocator cbAlloc, VAUDDeallocator cbFree);

extern BOOL AUDSimpleAllocDVDBuffers(void);
extern void AUDSimpleFreeDVDBuffers(void);

extern BOOL AUDSimpleCreateDecoder(void);
extern void AUDSimpleDestroyDecoder(void);

extern BOOL AUDSimpleLoadStart(BOOL loop);
extern BOOL AUDSimpleLoadStop(void);

extern BOOL AUDSimpleOpen(char* fileName);
extern BOOL AUDSimpleClose(void);

extern BOOL AUDSimpleDecode(void);
extern u32  AUDSimpleGetAudioSampleRate(void);
extern u32  AUDSimpleGetFrameTime(void);

extern void AUDSimplePlayMaskChange(void);

extern BOOL AUDSimpleCheckDVDError(void);

#ifdef __cplusplus
}
#endif

#endif	// __AUDSIMPLE_PLAYER_H__


