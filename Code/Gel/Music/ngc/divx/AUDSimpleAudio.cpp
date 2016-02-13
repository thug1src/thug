#ifndef DVDETH
/*!
 ******************************************************************************
 * \file AUDSimpleAudio.cpp
 *
 * \brief
 *		This file provides the required player audio functions.
 *
 * \note
 *      This is a demonstration source only!
 *
 * \date
 *		08/19/02
 *		04/28/03 - updated to AX audio playback
 *
 * \version
 *		1.0
 *
 * \author
 *		Thomas Engel
 *
 ******************************************************************************
 */

//
// Note about sample frequencies on Nintendo GameCube:
//
// GameCube's AI knows two native sample frequencies. Generally these are
// referenced as 48KHz and 32KHz. The DSP (no matter if driven by MusyX or AX)
// is using 32KHz.
//
// The frequencies that are in fact used are not exactly these frequencies, though!
//
// While this is not important when handling sound effects or even streamed audio,
// it is very important when interlaeving audio and video data since buffer under-runs
// might be triggered if no care is taken.
//
// The actual AI output frequencies are 48043Hz and 32028.66Hz.
//
// Streamed audio data interleaved with video should match these frequencies as
// closely as possible.
//

/******************************************************************************
 *  INCLUDES
 ******************************************************************************/
#include <dolphin.h>
#include <string.h>

#include "AUDSimplePlayer.h"
#include "AUDSimpleAudio.h"
#include "gel/soundfx/ngc/p_sfx.h"
#include "gfx/ngc/nx/nx_init.h"

/******************************************************************************
 *  DEFINES
 ******************************************************************************/

// Number of audio frames that should be able to be stored prio to being routed into the AX buffer
#define	AUD_AUDIO_READAHEADFRAMES	0.5f

#define	AUD_AUDIO_AIBUFFERSAMPLES	(2*256)		// 10.6ms of 48KHz data per AI buffer (32 byte multiple), that'll be about 15.9ms at 32Khz
#define	AUD_AUDIO_NUMAIBUFFERS		2			// Number of AI playback buffers (this has an impact on audio latency. 2 is the minimum needed)

#define	AX_ARAM_BUFFER_SIZE			(AUD_AUDIO_AIBUFFERSAMPLES * AUD_AUDIO_NUMAIBUFFERS)
//#define	AX_ARAM_LEFT_CHANNEL		0x200000	// @ 4MB (16-Bit addressing for DSP!)
//#define	AX_ARAM_RIGHT_CHANNEL		(AX_ARAM_LEFT_CHANNEL + AX_ARAM_BUFFER_SIZE)
#define	AX_ARAM_LEFT_CHANNEL		( NxNgc::EngineGlobals.aram_music >> 1 )
#define	AX_ARAM_RIGHT_CHANNEL		(AX_ARAM_LEFT_CHANNEL + AX_ARAM_BUFFER_SIZE)

#define	AUD_NUM_ARQ_REQUESTS		16

/******************************************************************************
 *  LOCAL VARIABLES & LOCAL EXTERNAL REFERENCES
 ******************************************************************************/

extern AudSimplePlayer	audio_player;								// Player instance

static void				*audioReadBuffer;							// Buffer to store audio data received from the data stream
static u32				audioReadBufferNumSamples;					// Size of the above buffer in samples
static u32				audioReadBufferWritePos;					// Write position in the above buffer in samples
static u32				audioReadBufferReadPos;						// Read position in the above buffer in samples
static u8				audioReadBufferNumChannels;					// Number of channels stored in the read buffer

static void				*audioPlayBuffer[AUD_AUDIO_NUMAIBUFFERS];	// AI playback buffers
static u8				audioPlayBufferWriteIndex;					// Index to next AI buffer to be written to from the read buffer
static u32				audioPlayBufferFrq;							// Playback frequency of AI in Hz
static volatile BOOL	audioPlayBufferEnabled;						// TRUE if playback is enabled. AI will operate at all times, but zeros will be filled in instead of real data if this is FALSE.

static const u32		*audioPlayMaskArray;						// Pointer to an array of channel play masks (each set bit signals an active channel)
static u32				audioNumPlayMasks;							// Number of play masks specified (0 indicates all channels should be active)
static u32				audioNumActiveVoices;						// Number of active voices

static AXVPB			*axVoice[2];								// AX voice structures
static ARQRequest		arqRequest[2][AUD_NUM_ARQ_REQUESTS];		// Enough ARQ request structures for worst case scenario
static u32				axLastAddr;									// Last known address DSP read from for 1st voice
static u32				axPlayedSamples;							// Number of samples played on first voice since last update
static u32				axPlayedSamplesTotal;						// Number of samples played on first voice since playback began

typedef enum VID_AXPHASE {
		AX_PHASE_STARTUP = 0,
		AX_PHASE_START,
		AX_PHASE_PLAY
		} VID_AXPHASE;

static VID_AXPHASE		axPhase;


/******************************************************************************
 *  LOCAL PROTOTYPES
 ******************************************************************************/

static void AXCallback(void);

/*!
 ******************************************************************************
 * \brief
 *		Initialize audio decoder
 *
 *		This function allocates all neccessary memory for the audio processing
 *		and sets the audio decoder into an idle state, waiting for first data.
 *		A file must be opened with the VIDSimplePlayer before calling this
 *		function.
 *
 * \return
 *		FALSE if any problem was detected
 *
 ******************************************************************************
 */
BOOL AUDSimpleInitAudioDecoder(void)
	{
	u32	 		i, ratio;
	BOOL 		old;
	AXPBMIX		axMix[2];
	AXPBVE 		axVE;
	AXPBSRC		axSRC;
	AXPBADDR 	axAddr;
	AXPBADPCM 	axADPCM;

	// Calculate buffer size to allocate proper memry to keep a bit of "extra" audio data around...
	audioReadBufferNumSamples = (u32)((f32)AUD_AUDIO_READAHEADFRAMES * audio_player.audioInfo.vaud.frq);
	audioReadBufferNumChannels = audio_player.audioInfo.vaud.numChannels <= 2 ? audio_player.audioInfo.vaud.numChannels : 2;

	// Allocate read buffer
	audioReadBuffer = audio_player.cbAlloc(audioReadBufferNumSamples * sizeof(s16) * audio_player.audioInfo.vaud.numChannels);
	if (audioReadBuffer == NULL)
		return FALSE;					// error
	
	// Reset ring buffer
	audioReadBufferReadPos = audioReadBufferWritePos = 0;

	// What frquency is best?
	audioPlayBufferFrq = audio_player.audioInfo.vaud.frq;
	
	// Allocate AI playback buffer
	audioPlayBuffer[0] = audio_player.cbAlloc(2 * sizeof(s16) * AUD_AUDIO_AIBUFFERSAMPLES * AUD_AUDIO_NUMAIBUFFERS);
	if (audioPlayBuffer[0] == NULL)
		return FALSE;					// error
	
	for(i=1; i<AUD_AUDIO_NUMAIBUFFERS; i++)
		audioPlayBuffer[i] = (void *)((u32)audioPlayBuffer[i - 1] + (2 * sizeof(s16) * AUD_AUDIO_AIBUFFERSAMPLES));
	
	// Reset buffer index
	audioPlayBufferWriteIndex = 0;
	
	// We disable AI output for now (logically)
	audioPlayBufferEnabled = FALSE;
	
	// We assume to playback all we get by default
	audioPlayMaskArray = NULL;
	audioNumPlayMasks = 0;
	audioNumActiveVoices = 2;
	
	// Clear out AI buffers to avoid any noise what so ever
	memset(audioPlayBuffer[0],0,2 * sizeof(s16) * AUD_AUDIO_AIBUFFERSAMPLES * AUD_AUDIO_NUMAIBUFFERS);
	DCFlushRange(audioPlayBuffer[0],2 * sizeof(s16) * AUD_AUDIO_AIBUFFERSAMPLES * AUD_AUDIO_NUMAIBUFFERS);
	
	// Init GCN audio system
	old = OSDisableInterrupts();

	axVoice[0] = AXAcquireVoice(AX_PRIORITY_NODROP,NULL,0);
	ASSERT(axVoice[0] != NULL);
	axVoice[1] = AXAcquireVoice(AX_PRIORITY_NODROP,NULL,0);
	ASSERT(axVoice[1] != NULL);

	memset(&axMix[0],0,sizeof(axMix[0]));
	axMix[0].vL = 0x7FFF;
	memset(&axMix[1],0,sizeof(axMix[1]));
	axMix[1].vR = 0x7FFF;
	
	AXSetVoiceMix(axVoice[0],&axMix[0]);
	AXSetVoiceMix(axVoice[1],&axMix[1]);
	
	axVE.currentDelta = 0;
	axVE.currentVolume = 0x7FFF;
	AXSetVoiceVe(axVoice[0],&axVE);
	AXSetVoiceVe(axVoice[1],&axVE);
	
	memset(&axSRC,0,sizeof(AXPBSRC));
	
	ratio = (u32)(65536.0f * (f32)audioPlayBufferFrq / (f32)AX_IN_SAMPLES_PER_SEC);
	axSRC.ratioHi = (u16)(ratio >> 16);
	axSRC.ratioLo = (u16)ratio;
	
	AXSetVoiceSrcType(axVoice[0],AX_SRC_TYPE_4TAP_16K);
	AXSetVoiceSrc(axVoice[0],&axSRC);
	AXSetVoiceSrcType(axVoice[1],AX_SRC_TYPE_4TAP_16K);
	AXSetVoiceSrc(axVoice[1],&axSRC);
	
	*(u32 *)&axAddr.currentAddressHi = AX_ARAM_LEFT_CHANNEL;
	*(u32 *)&axAddr.loopAddressHi = AX_ARAM_LEFT_CHANNEL;
	*(u32 *)&axAddr.endAddressHi = AX_ARAM_LEFT_CHANNEL + AX_ARAM_BUFFER_SIZE - 1;
	axAddr.format = AX_PB_FORMAT_PCM16;
	axAddr.loopFlag = AXPBADDR_LOOP_ON;
	AXSetVoiceAddr(axVoice[0],&axAddr);
	
	*(u32 *)&axAddr.currentAddressHi = AX_ARAM_RIGHT_CHANNEL;
	*(u32 *)&axAddr.loopAddressHi = AX_ARAM_RIGHT_CHANNEL;
	*(u32 *)&axAddr.endAddressHi = AX_ARAM_RIGHT_CHANNEL + AX_ARAM_BUFFER_SIZE - 1;
	AXSetVoiceAddr(axVoice[1],&axAddr);

	memset(&axADPCM,0,sizeof(axADPCM));
	axADPCM.gain = 0x0800;
	
	AXSetVoiceAdpcm(axVoice[0],&axADPCM);
	AXSetVoiceAdpcm(axVoice[1],&axADPCM);
	
	AXSetVoiceType(axVoice[0],AX_PB_TYPE_STREAM);
	AXSetVoiceType(axVoice[1],AX_PB_TYPE_STREAM);
	
	AXRegisterCallback( AXCallback );
	
	axLastAddr				= AX_ARAM_LEFT_CHANNEL;
	axPlayedSamples			= AUD_AUDIO_AIBUFFERSAMPLES * AUD_AUDIO_NUMAIBUFFERS;
	axPlayedSamplesTotal	= 0;
	axPhase					= AX_PHASE_STARTUP;
	
	// All is setup for the voices. We'll start them inside the AX callback as soon as we got data in the ARAM buffers
	OSRestoreInterrupts(old);

	return TRUE;
    }

/*!
 ******************************************************************************
 * \brief
 *		Shutdown audio decoder and free resources
 *
 ******************************************************************************
 */
void AUDSimpleExitAudioDecoder(void)
	{
	// Yes. Unregister callback & stop AI DMA
	BOOL old = OSDisableInterrupts();
	
	// Register our default AX callback.
	AXRegisterCallback( Sfx::AXUserCBack );
	
	AXSetVoiceState(axVoice[0],AX_PB_STATE_STOP);
	AXSetVoiceState(axVoice[1],AX_PB_STATE_STOP);
	AXFreeVoice(axVoice[0]);
	AXFreeVoice(axVoice[1]);
	
	axVoice[0] = axVoice[1] = NULL;
	
	OSRestoreInterrupts(old);

	// Free allocated resources...
	audio_player.cbFree(audioPlayBuffer[0]);
	audio_player.cbFree(audioReadBuffer);
	}

/*!
 ******************************************************************************
 * \brief
 *		Stop audio playback without shutting down AI etc.
 *
 ******************************************************************************
 */
void AUDSimpleAudioReset(void)
	{
	BOOL	old;

	old = OSDisableInterrupts();
	
	audioReadBufferWritePos = 0;
	audioReadBufferReadPos = 0;
	
	audioPlayBufferEnabled = FALSE;
		
	OSRestoreInterrupts(old);
	}

/*!
 ******************************************************************************
 * \brief
 *		Return some information about current audio stream.
 *
 ******************************************************************************
 */
void AUDSimpleAudioGetInfo(VidAUDH* audioHeader)
{
	ASSERT(audioHeader);
	memcpy(audioHeader, &audio_player.audioInfo, sizeof(*audioHeader));
}



void AUDSimpleAudioSetVolume( u16 vl, u16 vr )
{
	AXPBVE axVE;
	
	axVE.currentDelta	= 0;
	axVE.currentVolume	= vl;
	AXSetVoiceVe( axVoice[0], &axVE );
	
	axVE.currentVolume	= vr;
	AXSetVoiceVe( axVoice[1], &axVE );
}



/*!
 ******************************************************************************
 * \brief
 *
 ******************************************************************************
 */
static void writeChannelData(s16* dest, u32 channels, const s16** samples, u32 sampleOffset, u32 sampleNum)
	{
    u32 j;
	if(channels == 1)
		{
		const s16* in = samples[0] + sampleOffset;
		for(j = 0; j < sampleNum; j++)
			*dest++ = in[j];
		}
	else
		{
		const s16* inL;
		const s16* inR;

		ASSERT(channels == 2);
        inL = samples[0] + sampleOffset;
		inR = samples[1] + sampleOffset;
        for(j = 0; j < sampleNum; j++)
			{
			*dest++ = *inL++;
			*dest++ = *inR++;
			}
		}
	}


u32 AUDSimpleAudioGetFreeReadBufferSamples( void )
{
	u32 freeSamples;
	
	if( audioReadBufferWritePos >= audioReadBufferReadPos )
	{
		freeSamples = audioReadBufferNumSamples - (audioReadBufferWritePos - audioReadBufferReadPos);
	}
	else
	{
		freeSamples = audioReadBufferReadPos - audioReadBufferWritePos;
	}
	return freeSamples;
}





/*!
 ******************************************************************************
 * \brief
 *		Main audio data decode function.
 *
 *		This function will be used as a callback from the VIDAudioDecode
 *		function or is called directely in case of PCM or ADPCM.
 *
 * \param numChannels
 *		Number of channels present in the sample array
 *
 * \param samples
 *		Array of s16 pointers to the sample data
 *
 * \param sampleNum
 *		Number of samples in the array. All arrays have the same amount
 *		of sample data
 *
 * \param userData
 *		Some user data
 *
 * \return
 *		FALSE if data could not be interpreted properly
 *
 ******************************************************************************
 */
static BOOL audioDecode(u32 numChannels, const s16 **samples, u32 sampleNum, void* _UNUSED(userData))
	{
	u32	 freeSamples;
	u32	 sampleSize;
	u32	 len1;
	BOOL old;
	
	// we can only play mono or stereo!
	ASSERT(numChannels <= 2);

	// Disable IRQs. We must make sure we don't get interrupted by the AI callback.
	old = OSDisableInterrupts();

	// Did the video decoder just jump back to the beginning of the stream?
	if (audio_player.readBuffer[audio_player.decodeIndex].frameNumber < audio_player.lastDecodedFrame)
		{
		// Yes! We have to reset the internal read buffer and disable any audio output
		// until we got new video data to display...
		//
		// Note: we have to reset our buffers because the stream contains more audio data than
		// neccessary to cover a single video frame within the first few frames to accumulate
		// some safety buffer. If the stream would just be allowed to loop we would get an
		// overflow (unless we used up the extra buffer due to read / decode delays) after a few
		// loops...
		//
		AUDSimpleAudioReset();
		}

	// Calculate the read buffer's sample size
	sampleSize = sizeof(s16) * audioReadBufferNumChannels;

	// How many samples could we put into the buffer?
	if (audioReadBufferWritePos >= audioReadBufferReadPos)
	{
		freeSamples = audioReadBufferNumSamples - (audioReadBufferWritePos - audioReadBufferReadPos);

		if( freeSamples < sampleNum )
		{
			OSRestoreInterrupts(old);
			#ifndef FINAL
			OSReport("*** audioDecode: overflow case 1\n");
			#endif
			return FALSE;				// overflow!
		}

		// We might have a two buffer update to do. Check for it...
		if ((len1 = (audioReadBufferNumSamples - audioReadBufferWritePos)) >= sampleNum)
		{
			// No. We got ourselfs a nice, simple single buffer update.
			writeChannelData((s16 *)((u32)audioReadBuffer + audioReadBufferWritePos * sampleSize),numChannels,samples,0,sampleNum);
		}
		else
		{
			// Dual buffer case
			writeChannelData((s16 *)((u32)audioReadBuffer + audioReadBufferWritePos * sampleSize),numChannels,samples,0,len1);
            writeChannelData((s16 *)audioReadBuffer,numChannels,samples,len1,sampleNum-len1);
		}
	}
	else
	{
		freeSamples = audioReadBufferReadPos - audioReadBufferWritePos;

		if (freeSamples < sampleNum)
		{
			OSRestoreInterrupts(old);
			#ifndef FINAL
			OSReport("*** audioDecode: overflow case 2\n");
			#endif
			return FALSE;				// overflow!
		}

		// We're save to assume to have a single buffer update in any case...
		writeChannelData((s16 *)((u32)audioReadBuffer + audioReadBufferWritePos * sampleSize),numChannels,samples,0,sampleNum);
	}

	// Advance write position...
	audioReadBufferWritePos += sampleNum;

	if (audioReadBufferWritePos >= audioReadBufferNumSamples)
		audioReadBufferWritePos -= audioReadBufferNumSamples;

	// We're done with all critical stuff. IRQs may be enabled again...
	OSRestoreInterrupts(old);

	return TRUE;
	}


/*!
 ******************************************************************************
 * \brief
 *		Receive data from bitstream and direct to required encoding facility
 *
 *		This function will receive the AUDD chunck from the VIDSimplePlayer's
 *		decode function.
 *
 * \param bitstream
 *		Pointer to the data of a AUDD chunk
 *
 * \param bitstreamLen
 *		Length of the data in the AUDD chunk pointed to by above pointer
 *
 * \return
 *		FALSE if data could not be interpreted properly
 *
 ******************************************************************************
 */
BOOL AUDSimpleAudioDecode(const u8* bitstream, u32 bitstreamLen)
	{

	// select first two channels by default
	u32 channelSelectMask = audioPlayMaskArray ? audioPlayMaskArray[0] : 0x3;		
	
	u32 headerSize = ((VidAUDDVAUD*)bitstream)->size;
	if(!VAUDDecode(audio_player.decoder, bitstream+headerSize, bitstreamLen-headerSize, channelSelectMask, audioDecode, NULL))
		return FALSE;
	
	audioPlayBufferEnabled = TRUE;
	
	return TRUE;
	}

/*!
 ******************************************************************************
 * \brief
 *		Change the active audio channels
 *
 ******************************************************************************
 */
				
void AUDSimpleAudioChangePlayback(const u32 *playMaskArray,u32 numMasks)
	{
	u32		i,b;
	BOOL	old = OSDisableInterrupts();
	
	audioPlayMaskArray = playMaskArray;
	audioNumPlayMasks = numMasks;
	
	// Any playback mask specified?
	if (audioNumPlayMasks != 0)
		{
		// Yes. Count the active voices...
		audioNumActiveVoices = 0;
		
		for(i=0; i<numMasks; i++)
			{
			for(b=0; b<32; b++)
				{
				if (audioPlayMaskArray[i] & (1<<b))
					audioNumActiveVoices++;
				}
			}
		
        // So, did we have too much?
		ASSERT(audioNumActiveVoices <= 2);
		}
	else
		audioNumActiveVoices = audioReadBufferNumChannels;		// Make all active...
	
	OSRestoreInterrupts(old);
	}

void AUDSimplePause( bool pause )
{
	AXSetVoiceState(axVoice[0],pause ? AX_PB_STATE_STOP : AX_PB_STATE_RUN);
	AXSetVoiceState(axVoice[1],pause ? AX_PB_STATE_STOP : AX_PB_STATE_RUN);
}
	

/*!
 ******************************************************************************
 * \brief
 *		Copy PCM16 data from the read buffer into the audio buffer
 *
 *		This function serves as an internal service function to make sure
 *		mono and stereo audio data gets properly converted into audio buffer
 *		format.
 *
 * \param smpOffset
 *		Offset in samples into the current write buffer
 *
 * \param numSamples
 *		Number of samples to be updated / copied
 *
 ******************************************************************************
 */
static void audioCopy(u32 smpOffset,u32 numSamples)
	{
	s16		*destAddrL,*destAddrR,*srcAddr,s;
	u32		j;
	
	// Target address in audio buffer
	destAddrL = (s16 *)((u32)audioPlayBuffer[audioPlayBufferWriteIndex] + smpOffset * sizeof(s16));
	destAddrR = (s16 *)((u32)audioPlayBuffer[audioPlayBufferWriteIndex] + smpOffset * sizeof(s16) + (AUD_AUDIO_AIBUFFERSAMPLES * sizeof(s16)));

	// Single or dual channel setup?
    if (audioReadBufferNumChannels == 2)
		{
		// Stereo! Get source address of data...
		srcAddr = (s16 *)((u32)audioReadBuffer + audioReadBufferReadPos * 2 * sizeof(s16));
		
		// Copy samples into AI buffer and swap channels
		for(j=0; j<numSamples; j++)
			{
			*(destAddrL++) = *(srcAddr++);
			*(destAddrR++) = *(srcAddr++);
			}
		}
	else
		{
		// Mono case!
		
		// Make sure it's truely mono!
		ASSERT(audioReadBufferNumChannels == 1);
		
		// Get source address...
		srcAddr = (s16 *)((u32)audioReadBuffer + audioReadBufferReadPos * sizeof(s16));
		
		// Copy samples into AI buffer (AI is always stereo, so we have to dublicate data)
		for(j=0; j<numSamples; j++)
			{
			s = (*srcAddr++);
			*(destAddrL++) = s;
			*(destAddrR++) = s;
			}
		}
	
	// Advance read position as needed...
	audioReadBufferReadPos += numSamples;
	if (audioReadBufferReadPos >= audioReadBufferNumSamples)
		audioReadBufferReadPos -= audioReadBufferNumSamples;
	}

/*!
 ******************************************************************************
 * \brief
 *		AX callback
 *
 ******************************************************************************
 */
static void AXCallback(void)
{
	// First thing to do here is call the regular soundfx callback.
	Sfx::AXUserCBack();

	u32		availSamples,availFrames,numUpdate,i,n;
	u32		audioPlayBufferNeededAudioFrames;
	u32		currentAddr;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	
	if (axPhase == AX_PHASE_START)
	{
		AXSetVoiceState(axVoice[0],AX_PB_STATE_RUN);
		AXSetVoiceState(axVoice[1],AX_PB_STATE_RUN);
		axPhase = AX_PHASE_PLAY;
	}
		
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	
	currentAddr = *(u32 *)&axVoice[0]->pb.addr.currentAddressHi;

	if( currentAddr >= axLastAddr )
	{
		axPlayedSamples			+= currentAddr - axLastAddr;
		axPlayedSamplesTotal	+= currentAddr - axLastAddr;
	}
	else
	{
		axPlayedSamples			+= (( AX_ARAM_LEFT_CHANNEL + AX_ARAM_BUFFER_SIZE ) - axLastAddr ) + ( currentAddr - AX_ARAM_LEFT_CHANNEL );
		axPlayedSamplesTotal	+= (( AX_ARAM_LEFT_CHANNEL + AX_ARAM_BUFFER_SIZE ) - axLastAddr ) + ( currentAddr - AX_ARAM_LEFT_CHANNEL );
	}
	
	axLastAddr = currentAddr;

	// If we have played the required number of samples, stop the voice.
	if( axPlayedSamplesTotal >= audio_player.audioInfo.vaudex.totalSampleCount )
	{
		AXSetVoiceState( axVoice[0], AX_PB_STATE_STOP );
		AXSetVoiceState( axVoice[1], AX_PB_STATE_STOP );
		audio_player.playbackComplete = true;
		return;
	}
		
	if( axPlayedSamples >= AUD_AUDIO_AIBUFFERSAMPLES )
	{
		audioPlayBufferNeededAudioFrames = axPlayedSamples / AUD_AUDIO_AIBUFFERSAMPLES;

		// Make sure that we never get an underrun we don't notice...
		if( !( audioPlayBufferNeededAudioFrames <= AUD_AUDIO_NUMAIBUFFERS ))
		{
//			OSReport( "AX audio buffer underrun!\n" );

			// Disable playback.
			audioPlayBufferEnabled = false;
		}

		// Is actual audio playback enabled?
		if( audioPlayBufferEnabled )
		{
			// How many samples could we get from the read buffer?
			if( audioReadBufferWritePos >= audioReadBufferReadPos )
				availSamples = audioReadBufferWritePos - audioReadBufferReadPos;
			else
				availSamples = audioReadBufferNumSamples - ( audioReadBufferReadPos - audioReadBufferWritePos );
			
			// That's how many audio frames?
			availFrames = availSamples / AUD_AUDIO_AIBUFFERSAMPLES;

			//OSReport("AX: %d %d (%d)\n",availSamples,audioPlayBufferNeededAudioFrames * VID_AUDIO_AIBUFFERSAMPLES,axPlayedSamples);
	
			// So, how many can we update?
			numUpdate = ( availFrames > audioPlayBufferNeededAudioFrames ) ? audioPlayBufferNeededAudioFrames : availFrames;
	
			// If anything... go do it!
			if( numUpdate != 0 )
			{
				axPlayedSamples -= numUpdate * AUD_AUDIO_AIBUFFERSAMPLES;
				
				// Perform updates on each AI buffer in need of data...
				for( i = 0; i < numUpdate; i++ )
				{
					u32 leftSource, rightSource, leftTarget, rightTarget;

					// Can we copy everything from a single source or does the data wrap around?
					if(( n = audioReadBufferNumSamples - audioReadBufferReadPos) < AUD_AUDIO_AIBUFFERSAMPLES )
					{
						// It wraps...
						audioCopy( 0, n );
						audioCopy( n, AUD_AUDIO_AIBUFFERSAMPLES - n );
					}
					else
					{
						// We got one continous source buffer
						audioCopy( 0, AUD_AUDIO_AIBUFFERSAMPLES );
					}
						
					// Make sure the data ends up in real physical memory
					DCFlushRange( audioPlayBuffer[audioPlayBufferWriteIndex], AUD_AUDIO_AIBUFFERSAMPLES * sizeof( s16 ) * 2 );
					
					leftSource	= (u32)audioPlayBuffer[audioPlayBufferWriteIndex];
					rightSource	= (u32)audioPlayBuffer[audioPlayBufferWriteIndex] + ( AUD_AUDIO_AIBUFFERSAMPLES * sizeof( s16 ));
					leftTarget	= 2 * ( AX_ARAM_LEFT_CHANNEL + audioPlayBufferWriteIndex * AUD_AUDIO_AIBUFFERSAMPLES );
					rightTarget	= 2 * ( AX_ARAM_RIGHT_CHANNEL + audioPlayBufferWriteIndex * AUD_AUDIO_AIBUFFERSAMPLES );
					
					// Make sure we get this into ARAM ASAP...
					ARQPostRequest( &arqRequest[0][i%AUD_NUM_ARQ_REQUESTS], 0, ARQ_TYPE_MRAM_TO_ARAM,ARQ_PRIORITY_HIGH, leftSource, leftTarget, AUD_AUDIO_AIBUFFERSAMPLES * sizeof( s16 ), NULL );
					ARQPostRequest( &arqRequest[1][i%AUD_NUM_ARQ_REQUESTS], 1, ARQ_TYPE_MRAM_TO_ARAM,ARQ_PRIORITY_HIGH, rightSource, rightTarget, AUD_AUDIO_AIBUFFERSAMPLES * sizeof( s16 ), NULL );
					
					// Advance write index...
					audioPlayBufferWriteIndex = (u8)(( audioPlayBufferWriteIndex + 1 ) % AUD_AUDIO_NUMAIBUFFERS );
				}
				
				if( axPhase == AX_PHASE_STARTUP )
				{
					axPhase = AX_PHASE_START;
				}
			}
		}
		else
		{
			// Update buffer(s) with silence...
			axPlayedSamples -= audioPlayBufferNeededAudioFrames * AUD_AUDIO_AIBUFFERSAMPLES;
			
			for( i = 0; i < audioPlayBufferNeededAudioFrames; i++ )
			{
				u32 leftSource, rightSource, leftTarget, rightTarget;

				memset(audioPlayBuffer[audioPlayBufferWriteIndex],0,2 * sizeof(s16) * AUD_AUDIO_AIBUFFERSAMPLES);
				DCFlushRange(audioPlayBuffer[audioPlayBufferWriteIndex],2 * sizeof(s16) * AUD_AUDIO_AIBUFFERSAMPLES);
	
				leftSource = (u32)audioPlayBuffer[audioPlayBufferWriteIndex];
				rightSource = (u32)audioPlayBuffer[audioPlayBufferWriteIndex] + (AUD_AUDIO_AIBUFFERSAMPLES * sizeof(s16)) / 2;
				leftTarget = 2 * (AX_ARAM_LEFT_CHANNEL + audioPlayBufferWriteIndex * AUD_AUDIO_AIBUFFERSAMPLES);
				rightTarget = 2 * (AX_ARAM_RIGHT_CHANNEL + audioPlayBufferWriteIndex * AUD_AUDIO_AIBUFFERSAMPLES);
				
				// Make sure we get this into ARAM ASAP...
				ARQPostRequest(&arqRequest[0][i%AUD_NUM_ARQ_REQUESTS],0,ARQ_TYPE_MRAM_TO_ARAM,ARQ_PRIORITY_HIGH,leftSource,leftTarget,AUD_AUDIO_AIBUFFERSAMPLES * sizeof(s16),NULL);
				ARQPostRequest(&arqRequest[1][i%AUD_NUM_ARQ_REQUESTS],1,ARQ_TYPE_MRAM_TO_ARAM,ARQ_PRIORITY_HIGH,rightSource,rightTarget,AUD_AUDIO_AIBUFFERSAMPLES * sizeof(s16),NULL);
				
				audioPlayBufferWriteIndex = (u8)((audioPlayBufferWriteIndex + 1) % AUD_AUDIO_NUMAIBUFFERS);
			}
			
			if (axPhase == AX_PHASE_STARTUP)
				axPhase = AX_PHASE_START;
		}
	}
}

#endif		// DVDETH

