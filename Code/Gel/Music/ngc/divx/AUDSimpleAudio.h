/*!
 ******************************************************************************
 * \file AUDSimpleAudio.h
 *
 * \brief
 *		Header file for AUDSimpleAudio interface
 *
 * \date
 *		5/22/03
 *
 * \version
 *		1.0
 *
 * \author
 *		Thomas Engel/Achim Moller
 *
 ******************************************************************************
 */
#ifndef __AUDSIMPLE_AUDIO_H__
#define __AUDSIMPLE_AUDIO_H__

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

/******************************************************************************
 *  STRUCTS AND TYPES
 ******************************************************************************/

/******************************************************************************
 *  PROTOTYPES
 ******************************************************************************/

//! Initialize audio decoding / playback
extern BOOL AUDSimpleInitAudioDecoder(void);

//! Shutdown audio decoding / playback
extern void AUDSimpleExitAudioDecoder(void);

//! Decode audio data for one frame
extern BOOL AUDSimpleAudioDecode(const u8* bitstream, u32 bitstreamLen);

//! Change currently active audio channels
extern void AUDSimpleAudioChangePlayback(const u32 *playMaskArray,u32 numMasks);

//! Stop audio decode / output
extern void AUDSimpleAudioReset(void);

//! Return some information about running audio
extern void AUDSimpleAudioGetInfo(VidAUDH* audioHeader);

extern u32	AUDSimpleAudioGetFreeReadBufferSamples( void );
extern void	AUDSimpleAudioSetVolume( u16 vl, u16 vr );

extern void AUDSimplePause( bool pause );


#ifdef __cplusplus
}
#endif

#endif	// __AUDSIMPLE_PLAYER_H__


