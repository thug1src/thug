/*****************************************************************************
**																			**
**			              Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 2001 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GEL Library												**
**																			**
**	Module:			Sound effects (Sfx)	 									**
**																			**
**	File name:		ngc/p_sfx.cpp											**
**																			**
**	Created by:		08/21/01	-	dc										**
**																			**
**	Description:	NGC Sound effects										**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <core/macros.h>
#include <core/defines.h>
#include	<sys/file/filesys.h>
#include <sys/ngc/p_dma.h>
#include <gel/soundfx/ngc/p_sfx.h>
#include <gel/music/ngc/pcm/pcm.h>
#include <gel/music/ngc/p_music.h>
#include <gel/music/music.h>
#include <dolphin/mix.h>
#include <dolphin\dtk.h>
#include 	"gfx\ngc\nx\nx_init.h"


/*****************************************************************************
**							  DBG Information								**
*****************************************************************************/

namespace Sfx
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define	GAMECUBE_START_DSP_DATA_ADDR		( NxNgc::EngineGlobals.aram_dsp )
#define	GAMECUBE_END_DSP_DATA_ADDR			( NxNgc::EngineGlobals.aram_dsp + aram_dsp_size )

#define MAX_VOLUME						0x7FFF  // Max Gamecube volume.
#define MAX_CHANNEL_VOLUME				0x3FFF	// Half of the maximum, so sounds played at 100% will be half max volume.
#define MAX_REVERB_DEPTH				0x3FFF
#define MAX_PITCH						0xFFFFFL
#define UNALTERED_PITCH					0x10000L

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

struct sDSPADPCM
{
	// for WAV header generation during decode
	u32 num_samples;		// total number of RAW samples
	u32 num_adpcm_nibbles;	// number of ADPCM nibbles (including frame headers)
	u32 sample_rate;		// Sample rate, in Hz

	// DSP addressing and decode context
	u16 loop_flag; // 1=LOOPED, 0=NOT LOOPED
	u16 format; // Always 0x0000, for ADPCM
	u32 sa; // Start offset address for looped samples (zero for non-looped)
	u32 ea; // End offset address for looped samples
	u32 ca; // always zero
	u16 coef[16]; // decode coefficients (eight pairs of 16-bit words)

	// DSP decoder initial state
	u16 gain; // always zero for ADPCM
	u16 ps; // predictor/scale
	u16 yn1; // sample history
	u16 yn2; // sample history

	// DSP decoder loop context
	u16 lps; // predictor/scale for loop context
	u16 lyn1; // sample history (n-1) for loop context
	u16 lyn2; // sample history (n-2) for loop context
	u16 pad[10]; // reserved
};



struct VoiceEntry
{
	AXVPB*				p_axvpb;		// Pointer returned by AXAcquireVoice().
	struct sDSPADPCM*	p_dspadpcm;		// Pointer to sample header data in memory.
};



/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

// Because we don't use voices in the same way as on PS2, this array simulates
// the system. An entry with a NULL pointer means no sound is playing on that
// 'voice', a valid pointer indicates a sound is playing on that 'voice', and
// provides details of the playback.
static VoiceEntry		VoiceSimulator[NUM_VOICES];

// Global sfx multiplier, percentage.
static float			gSfxVolume		= 100.0f;

// allocated SPU RAM
int SpuRAMUsedTemp = 0;
int SpuRAMUsedPerm = 0;

// For maintaining pointers to the temporary sound headers, so they can be wiped
// when cleanup between levels is performed.
static void*			tempSoundHeaders[200];
static int				numTempSoundHeaders = 0;

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

bool					isStereo = true;

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void AXUserCBack( void )
{
	for( int i = 0; i < NUM_VOICES; ++i )
	{
		if( VoiceSimulator[i].p_axvpb != NULL )
		{
			if( VoiceSimulator[i].p_axvpb->pb.state == AX_PB_STATE_STOP )
			{
				// Playback on this voice has ended, so free up the voice.
				AXFreeVoice( VoiceSimulator[i].p_axvpb );
				VoiceSimulator[i].p_axvpb = NULL;
//				OSReport( "voice %d stopped %d\n", i, OSGetTick() );
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static void ax_voice_reacquisition_callback( void* p )
{
	OSReport( "Voice was reacquired\n" );

	for( int i = 0; i < NUM_VOICES; ++i )
	{
		if( VoiceSimulator[i].p_axvpb != NULL )
		{
			if( (void*)( VoiceSimulator[i].p_axvpb ) == p )
			{
				OSReport( "Voice was voice %d\n", i );
				VoiceSimulator[i].p_axvpb = NULL;
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static int getFreeVoice( void )
{
	for( int i = 0; i < NUM_VOICES; ++i )
	{
		if( VoiceSimulator[i].p_axvpb == NULL )
		{
			return i;
		}
	}
	return -1;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static bool loadGamecubeDSP( PlatformWaveInfo *pInfo, const char *filename, bool loadPerm, float *samplePitch )
{
	int vagAddr	= 0;			// VAG address in SPU2 RAM, returned to calling function
	bool rv = false;

	void* p_file = File::Open( filename, "rb" );
	if( p_file )
	{
		int		size		= File::GetFileSize( p_file );
		if ( size )
		{
			int		align_size	= ( size + 31 ) & ~31;
			int		data_size	= align_size - 96;
	
			// Grab enough memory to ensure a multiple of 32 bytes data length. To avoid fragmentation, we grab
			// memory for the header copy first.
			char*	p_header_copy	= (char*)Mem::Malloc( 96 );
//			File::Read( p_header_copy, 1, 96, p_file );
			Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
			char*	p_data			= (char*)Mem::Malloc( align_size );
			Mem::Manager::sHandle().BottomUpHeap()->PopAlign();
//	#ifdef	__NOPT_ASSERT__
			int		bytes_read		= File::Read( p_data, 1, size, p_file );
//	#else
//			File::Read( p_data, 1, size, p_file );
			DCFlushRange ( p_data, size );
//	#endif		//	__NOPT_ASSERT__
//			Dbg_Assert( bytes_read == size )
			if ( bytes_read != size )
			{
				File::Close( p_file );
				delete p_data;
				return false;
			}
//			file.close();
	
			// Copy the header to the reserved area.
			memcpy( p_header_copy, p_data, 96 );
	
			// Obtain information from the file header.
			sDSPADPCM*	p_header	= (sDSPADPCM*)p_header_copy;
	//		OSReport( "num samples: %d\n", p_header->num_samples );
	//		OSReport( "sample rate: %d\n", p_header->sample_rate );
	//		OSReport( "loop flag:	%d\n", p_header->loop_flag );
	
			if( loadPerm )
			{
				// Stack the permanent sounds up from the bottom.
				SpuRAMUsedPerm += data_size;
				if(( GAMECUBE_END_DSP_DATA_ADDR - SpuRAMUsedPerm ) & 0x0F )
				{
	#ifdef	__NOPT_ASSERT__
					int oldSpuRAM = SpuRAMUsedPerm;
	#endif		//	__NOPT_ASSERT__
					SpuRAMUsedPerm += (( END_WAVE_DATA_ADDR - SpuRAMUsedPerm ) & 0x0F );
					Dbg_MsgAssert( SpuRAMUsedPerm > oldSpuRAM, ( "Duh asshole." ));
				}
				vagAddr = GAMECUBE_END_DSP_DATA_ADDR - SpuRAMUsedPerm;
				Dbg_MsgAssert( !( vagAddr & 0x0F ), ( "Fire Matt please." ));
			}
			else
			{
				// If a temporary sound, add the details of the header to the cleanup array.
				tempSoundHeaders[numTempSoundHeaders++]	= p_header_copy;
	
				if(( SpuRAMUsedTemp + GAMECUBE_START_DSP_DATA_ADDR ) & 0x0F )
				{
					SpuRAMUsedTemp += 0x10 - ( ( SpuRAMUsedTemp + BASE_WAVE_DATA_ADDR ) & 0x0F );
				}
				vagAddr = GAMECUBE_START_DSP_DATA_ADDR + SpuRAMUsedTemp;
				SpuRAMUsedTemp += data_size;
			}
		
			// Start DMA transfer buffer to sound RAM...
			Dbg_Assert(((unsigned int)p_data & 31 ) == 0 );
			Dbg_Assert(((unsigned int)vagAddr & 31 ) == 0 );
	
			// Read data in chunks & DMA.
//			int toread = size - 96;
//			int offset = 0;
//			while ( toread )
//			{
//#define BUFFER_SIZE 16384
//				char readBuffer[BUFFER_SIZE];
//				if ( toread > BUFFER_SIZE )
//				{
//					// Read a full buffer.
//					File::Read( readBuffer, 1, BUFFER_SIZE, p_file );
//					toread -= BUFFER_SIZE;
//					offset += BUFFER_SIZE;
//					NsDMA::toARAM( vagAddr + offset, readBuffer, BUFFER_SIZE );
//				}
//				else
//				{
//					// Last chunk
//					File::Read( readBuffer, 1, toread, p_file );
//					NsDMA::toARAM( vagAddr + offset, readBuffer, ( toread + 31 ) & ~31 );
//					toread = 0;
//				}
//			}
//

			NsDMA::toARAM( vagAddr, p_data + 96, data_size );
	
			pInfo->headerAddress	= p_header_copy;
			pInfo->vagAddress		= vagAddr;
			pInfo->looping			= ( p_header->loop_flag > 0 );
	
			// Figure pitch adjustment coefficient.
			if( samplePitch )
			{
				*samplePitch = ( (float)p_header->sample_rate * 100.0f ) / 32000.0f;
			}
	
			// Release main RAM version of data.
			Mem::Free( p_data );
	
			rv = true;
		}
		File::Close( p_file );
	}
	else
	{
//		OSReport( 0, "************ failed to load sound: %s\n", filename );
//		while ( 1== 1 );
		//Dbg_MsgAssert( 0, ( "failed to load sound: %s\n", filename ));
	}
	return rv;
}



/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

void GetStreamBufferInfo( int* stream0_base, int* stream1_base, int* stream2_base,
						  int* stream0_size, int* stream1_size, int* stream2_size )
{
	*stream0_base	= NxNgc::EngineGlobals.aram_stream0;
	*stream0_size	= aram_stream0_size;
	*stream1_base	= NxNgc::EngineGlobals.aram_stream1;
	*stream1_size	= aram_stream1_size;
	*stream2_base	= NxNgc::EngineGlobals.aram_stream2;
	*stream2_size	= aram_stream2_size;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PerFrameUpdate( void )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void InitSoundFX( CSfxManager *p_manager )
{
	// Initialize the 'voice' array.
	for( int i = 0; i < NUM_VOICES; ++i )
	{
		VoiceSimulator[i].p_axvpb = NULL;
	}

	// See whether the unit is in mono or stereo mode.
	isStereo = !( OSGetSoundMode() == OS_SOUND_MODE_MONO );

	// Initialize Audio Interface API.
//	AIInit( NULL );

	// Initialize the AX library, and boot the DSP.
//	AXInit();

	// Set the callback responsible for freeing up voices.
	AXRegisterCallback( AXUserCBack );

	// Zero first 1024 bytes of ARAM - this is where one-shot sounds will end up when they finish.
#	if aram_zero_size != 1024
#	error aram zero buffer size has changed
#	endif
	
	char reset_buffer[1024];
	memset( reset_buffer, 0, 1024 );
	DCFlushRange( reset_buffer, 1024 );
	NsDMA::toARAM( NxNgc::EngineGlobals.aram_zero, reset_buffer, 1024 );

	CleanUpSoundFX();

	// Moved this from sioman.cpp since the pcm stuff for ngc needs the sfx stuff initialised first.
	Pcm::Init();
}




/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void ReInitSoundFX( void )
{
	// Initialize the 'voice' array.
	for( int i = 0; i < NUM_VOICES; ++i )
	{
		VoiceSimulator[i].p_axvpb = NULL;
	}

	// Flush pending ARQ transfers and wait for DMA to finish.
	ARQFlushQueue();
	while( ARGetDMAStatus() != 0 );

	AXQuit();
	AIReset();
	ARQReset();
	ARReset();

	// Initialize Audio Interface API.
	AIInit( NULL );

	// Initialize the AX library, and boot the DSP.
	AXInit();

	// Set the callback responsible for freeing up voices.
	AXRegisterCallback( AXUserCBack );

#ifndef DVDETH
	DTKInit();
	DTKSetRepeatMode( DTK_MODE_NOREPEAT );
#endif		// DVDETH
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool VoiceIsOn( int whichVoice )
{
	if( VoiceSimulator[whichVoice].p_axvpb != NULL )
	{
		// Check it hasn't already been stopped since then it will be dealt with by the 5ms callback.
//		if( VoiceSimulator[whichVoice].p_axvpb->pb.state != AX_PB_STATE_STOP )
		{
			return true;
		}
	}
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
//void SetVoiceParameters( int voiceNumber, float volL, float volR, float fPitch )
void	SetVoiceParameters( int whichVoice, sVolume *p_vol, float pitch )
{
	unsigned int rpitch = UNALTERED_PITCH;
	if( pitch != 0.0f )
	{
		rpitch = (unsigned int)PERCENT( UNALTERED_PITCH, pitch );
		if ( rpitch > MAX_PITCH )
			rpitch = MAX_PITCH;
		if ( rpitch <= 0 )
			rpitch = 1;
	}
	
	int old = OSDisableInterrupts();
	if( VoiceSimulator[whichVoice].p_axvpb )
	{
		AXPB* p_pb = &( VoiceSimulator[whichVoice].p_axvpb->pb );
//		Pcm::setDolby( p_pb, VoiceSimulator[voiceNumber].p_axvpb, volL, volR, pitch, true, gSfxVolume );
		//Pcm::setDolby( p_pb, VoiceSimulator[whichVoice].p_axvpb, p_vol->GetChannelVolume( 0 ), p_vol->GetChannelVolume( 1 ), rpitch, true, gSfxVolume );
		if ( p_vol )
		{
			Pcm::setDolby( p_pb, VoiceSimulator[whichVoice].p_axvpb, p_vol->GetChannelVolume( 0 ), p_vol->GetChannelVolume( 1 ), rpitch, true, gSfxVolume );
		}
		else
		{
			Pcm::setDolby( p_pb, VoiceSimulator[whichVoice].p_axvpb, 0.0f, 0.0f, rpitch, true, gSfxVolume );
		}
	}
	OSRestoreInterrupts( old );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool LoadSoundPlease( const char *sfxName, uint32 checksum, PlatformWaveInfo *pInfo, bool loadPerm )
{
	Dbg_Assert( pInfo );

	// Need to append file type. Copy string into a buffer.
	static char name_buffer[256] = "sounds\\dsp\\";
	const int SOUND_PREPEND_START_POS = 11;

	// Check there is room to add the extension.
	int length = strlen( sfxName );
	Dbg_Assert( strlen( sfxName ) <= ( 256 - ( SOUND_PREPEND_START_POS + 4 )));
	strcpy( name_buffer + SOUND_PREPEND_START_POS, sfxName );
	strcpy( name_buffer + SOUND_PREPEND_START_POS + length, ".dsp" );

//	OSReport( "Loading sound: %s\n", name_buffer );

	loadGamecubeDSP( pInfo, name_buffer, loadPerm, &pInfo->pitchAdjustmentForSamplingRate );


	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
//int PlaySoundPlease( PlatformWaveInfo *pInfo, float volL, float volR, float pitch )
int PlaySoundPlease( PlatformWaveInfo *pInfo, sVolume *p_vol, float pitch )
{
	//Dbg_Assert( pInfo->headerAddress != NULL );
	
//	Spt::SingletonPtr< Sfx::CSfxManager > sfx_manager;
//	if ( sfx_manager->GetMainVolume() < 1.0f ) return -1;

	int voice = getFreeVoice();
	if( voice == -1 )
	{
		// No voice in our array available.
		return -1;
	}

	// Attempt to grab a voice. Set the priority high if it is a looping sound.
	sDSPADPCM*	p_header	= (sDSPADPCM*)pInfo->headerAddress;
	// HACK!!!
	if( !p_header )
	{
		// No voice available.
		return -1;
	}
	AXVPB*		p_axvpb		= AXAcquireVoice( ( p_header->loop_flag ) ? AX_PRIORITY_NODROP : AX_PRIORITY_LOWEST, ax_voice_reacquisition_callback, 0 );
	if( p_axvpb == NULL )
	{
		// No voice available.
		return -1;
	}

	AXPBADDR		addr;
	AXPBSRC			src;
	AXPBADPCM		adpcm;
	AXPBADPCMLOOP	loop;        

	addr.loopFlag           = p_header->loop_flag ? AXPBADDR_LOOP_ON : AXPBADDR_LOOP_OFF;
	addr.format             = AX_PB_FORMAT_ADPCM;   

	// Convert from byte to nibble address, and split into low and high words.
	u32 start_address		= ( pInfo->vagAddress * 2 ) + 2;
	u32 end_address			= ( pInfo->vagAddress * 2 ) + ( p_header->num_adpcm_nibbles - 1 );

	addr.currentAddressHi   = (u16)( start_address >> 16 );
	addr.currentAddressLo   = (u16)( start_address & 0xFFFFU );
	addr.endAddressHi		= (u16)( end_address >> 16 );
	addr.endAddressLo		= (u16)( end_address & 0xFFFFU );

	if( p_header->loop_flag )
	{
		addr.loopAddressHi      = (u16)( start_address >> 16 );
		addr.loopAddressLo      = (u16)( start_address & 0xFFFFU );
	}
	else
	{
		u32 zero_buffer_address	= NxNgc::EngineGlobals.aram_zero * 2;
		addr.loopAddressHi      = (u16)( zero_buffer_address >> 16 );
		addr.loopAddressLo      = (u16)( zero_buffer_address & 0xFFFFU );
	}

	loop.loop_pred_scale    = p_header->lps;
	loop.loop_yn1           = p_header->lyn1;
	loop.loop_yn2           = p_header->lyn2;

//	uint32 srcBits			= (uint32)(0x00010000 * ((f32)p_header->sample_rate / AX_IN_SAMPLES_PER_SEC));
//	src.ratioHi				= (u16)(srcBits >> 16);
//	src.ratioLo				= (u16)(srcBits & 0xFFFF);
	src.ratioHi             = 1;
	src.ratioLo             = 0;
	src.currentAddressFrac  = 0;
	src.last_samples[0]     = 0;
	src.last_samples[1]     = 0;
	src.last_samples[2]     = 0;
	src.last_samples[3]     = 0;

	memcpy( &adpcm.a[0][0], p_header->coef, sizeof( u16 ) * 16 );
	adpcm.gain              = p_header->gain;
	adpcm.pred_scale        = p_header->ps;
	adpcm.yn1               = p_header->yn1;
	adpcm.yn2               = p_header->yn2;

	AXPBVE ve;
	ve.currentVolume		= 0x7FFF;					// This is max volume.
	ve.currentDelta			= 0x0000;

	AXSetVoiceType( p_axvpb, AX_PB_TYPE_NORMAL );
	
	AXSetVoiceAddr( p_axvpb, &addr );                   // input addressing
	AXSetVoiceAdpcm( p_axvpb, &adpcm );                 // ADPCM coefficients
	AXSetVoiceAdpcmLoop( p_axvpb, &loop );              // ADPCM loop context

	AXSetVoiceVe( p_axvpb, &ve );
	
	AXSetVoiceSrcType( p_axvpb, AX_SRC_TYPE_LINEAR );   // SRC type
	AXSetVoiceSrc( p_axvpb, &src );                     // initial SRC settings

	// Set the voice to run.
	AXSetVoiceState( p_axvpb, AX_PB_STATE_RUN );        
	
	// Fill in voice details.
	VoiceSimulator[voice].p_axvpb		=  p_axvpb;
	VoiceSimulator[voice].p_dspadpcm	=  p_header;
//	OSReport( "voice %d started %d\n", voice, OSGetTick() );

	// Adjust pitch to account for lower sampling rates:
	pitch = PERCENT( pitch, pInfo->pitchAdjustmentForSamplingRate );

	// This takes the place of AxSetVoiceMix().
	SetVoiceParameters( voice, p_vol, pitch );
	
	return voice;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void StopSoundPlease( int whichVoice )
{
	if( VoiceIsOn( whichVoice ))
	{
		AXSetVoiceState( VoiceSimulator[whichVoice].p_axvpb, AX_PB_STATE_STOP );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void StopAllSoundFX( void )
{
//	Pcm::StopMusic();
//	Pcm::StopStreams();

	for( int i = 0; i < NUM_VOICES; i++ )
	{
		StopSoundPlease( i );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SetVolumePlease( float volumeLevel )
{
	// Sets master volume level.
	gSfxVolume = volumeLevel;

//	Pcm::PCMAudio_SetStreamGlobalVolume( ( unsigned int ) volumeLevel );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PauseSoundsPlease( void )
{
//	sVolume silent_vol;
//	silent_vol.SetSilent();

	// Just turn the volume down on all playing voices.
	for( int i = 0; i < NUM_VOICES; i++ )
	{
		if( VoiceIsOn( i ))
		{
//			SetVoiceParameters( i, 0.0f, 0.0f );
//			SetVoiceParameters( i, &silent_vol );
			SetVoiceParameters( i, NULL );
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CleanUpSoundFX( void )
{
	StopAllSoundFX();
	SpuRAMUsedTemp = 0;
	SetReverbPlease( 0 );

	for( int i = 0; i < numTempSoundHeaders; ++i )
	{
		Dbg_Assert( tempSoundHeaders[i] );
		Mem::Free( tempSoundHeaders[i] );
		tempSoundHeaders[i] = NULL;
	}
	numTempSoundHeaders = 0;
}









// Macros and constants:

int ReverbModes[ ] =
{
	// put the default first in this list, since the default param to SetReverb() is 0
	0,
//	SD_REV_MODE_HALL,
//	SD_REV_MODE_ROOM,
//	SD_REV_MODE_STUDIO_A,
//	SD_REV_MODE_STUDIO_B,
//	SD_REV_MODE_STUDIO_C,
//	SD_REV_MODE_SPACE,
//	SD_REV_MODE_ECHO,
//	SD_REV_MODE_DELAY,
//	SD_REV_MODE_PIPE,
};

#define NUM_REVERB_MODES ( ( int )( sizeof( ReverbModes ) / sizeof( int ) ) )

#if NUM_STREAMS > 3
#error tell matt to handle more than 3 streams in sfx.cpp
#endif

#if NUM_STREAMS == 3
#define VOICES_MINUS_STREAMS ( 0xFFFFFF & ~( ( 1 << STREAM_VOICE( 0 ) | ( 1 << STREAM_VOICE( 1 ) ) | ( 1 << STREAM_VOICE( 2 ) ) ) )
#else
#define VOICES_MINUS_STREAMS ( 0xFFFFFF & ~( ( 1 << MUSIC_L_VOICE ) | ( 1 << MUSIC_R_VOICE ) ) )
#endif

// convert sample rate to pitch setting
#define FREQ2PITCH( x ) ( ( float )( ( ( x ) * 100 ) / 48000 ) )

#define DMA_CH						0

#if ( ( CORE_0_EFFECTS_END_ADDRESS - CORE_0_EFFECTS_START_ADDRESS ) < ( RAM_NEEDED_FOR_EFFECTS ) )
#error "not enough room for effects"
#endif

//int LoopingSounds[ NUM_CORES ];
// gotta keep track of voices used, to be able
// to tell whether a voice is free or not...
//int VoicesUsed[ NUM_CORES ];

// avoid checking voice status more than once per frame:
//uint64	gLastVoiceAvailableUpdate[ NUM_CORES ];
//int		gAvailableVoices[ NUM_CORES ];


#define BLOCKHEADER_LOOP_BIT	( 1 << 9 )

// Internal function prototypes:

static bool	loadGamecubeDSP( PlatformWaveInfo *pInfo, const char *filename, bool loadPerm, float *samplePitch = NULL );

int		PS2Sfx_PlaySound( PlatformWaveInfo *pInfo, int voiceNumber, float volL = 100.0f, float volR = 100.0f, float pitch = 100.0f );
void	PS2Sfx_StopSound( int voiceNumber );
int		PS2Sfx_GetVoiceStatus( int core );

#define ALLIGN_REQUIREMENT	64 // EE to IOP
#define SIZE_OF_LEADIN		16
#define USE_CLEVER_TRICK_FOR_CLOBBERED_DATA		1











// IOP_MIN_TRANSFER must be a power of 2!!!
#define IOP_MIN_TRANSFER	64	// iop will clobber memory if you're writing above a sound.  we'll restore the
								// potentially clobbered memory by keeping a buffer containing the beginning of
								// the previously loaded perm sound, and copying that to the end of our new
								// data...

#if USE_CLEVER_TRICK_FOR_CLOBBERED_DATA
#define		PAD_TO_PREVENT_CLOBBERING		0
#else
#define		PAD_TO_PREVENT_CLOBBERING		64
#endif

#define FULL_NAME_SIZE		256





int GetMemAvailable( void )
{
	return ( MAX_SPU_RAM_AVAILABLE - ( SpuRAMUsedTemp + SpuRAMUsedPerm ) );
}














//--------------------------------------------------------------------------------------------
// Function: void SetReverb(void)
// Description: Sets reverb level and type
// Parameters:  none
//
// Returns:
//-------------------------------------------------------------------------------------------
void SetReverbPlease( float reverbLevel, int reverbMode, bool instant = false )
{
//	int i;
//	sceSdEffectAttr r_attr;
//
//	Dbg_MsgAssert( reverbType >= 0,( "Bad reverb mode." ));
//	Dbg_MsgAssert( reverbType < NUM_REVERB_MODES,( "Bad reverb mode..." ));
//	
//	if ( !reverbLevel )
//	{
//		// --- set reverb attribute
//		r_attr.depth_L  = 0;
//		r_attr.depth_R  = 0;
//		r_attr.delay    = 0;
//		r_attr.feedback = 0;
//		r_attr.mode = SD_REV_MODE_OFF;
//#if REVERB_ONLY_ON_CORE_0
//		i = 0;  // just set reverb on core 0 (not the music core or any sfx on core 1)
//#else
//		for ( i = 0; i < 2; i++ )
//#endif
//		{
//			// just turning off reverb on all voices...
//			if(sceSdRemote( 1, rSdSetSwitch, i | SD_S_VMIXEL , 0 ) == -1 )
//				Dbg_Message("Error setting reverb4\n");
//			if(sceSdRemote( 1, rSdSetSwitch, i | SD_S_VMIXER , 0 ) == -1 )
//				Dbg_Message("Error setting reverb5\n");
//			if( sceSdRemote( 1, rSdSetEffectAttr, i, &r_attr ) == -1 )
//				Dbg_Message( "Error setting reverb\n" );
//		}
//	}
//	else
//	{
//		int reverbDepth = ( int )PERCENT( MAX_REVERB_DEPTH, reverbLevel );
//		
//		// --- set reverb attribute
//		r_attr.depth_L  = 0x3fff;
//		r_attr.depth_R  = 0x3fff;
//		r_attr.delay    = 30;
//		r_attr.feedback = 200;
//		r_attr.mode = ReverbModes[ reverbType ];
//#if REVERB_ONLY_ON_CORE_0
//		i = 0;
//#else		
//		for( i = 0; i < 2; i++ )
//#endif
//		{
//			if( sceSdRemote( 1, rSdSetEffectAttr, i, &r_attr ) == -1 )
//				Dbg_Message( "Error setting reverb\n" );
//	
//			if(sceSdRemote( 1, rSdSetCoreAttr, i | SD_C_EFFECT_ENABLE, 1 ) == -1 )
//				Dbg_Message("Error setting reverb\n");
//			if(sceSdRemote( 1, rSdSetParam, i | SD_P_EVOLL , reverbDepth ) == -1 )
//				Dbg_Message("Error setting reverb2\n");
//			if(sceSdRemote( 1, rSdSetParam, i | SD_P_EVOLR , reverbDepth ) == -1 )
//				Dbg_Message("Error setting reverb3\n");;
//			if(sceSdRemote( 1, rSdSetSwitch, i | SD_S_VMIXEL , VOICES_MINUS_STREAMS ) == -1 )
//				Dbg_Message("Error setting reverb4\n");
//			if(sceSdRemote( 1, rSdSetSwitch, i | SD_S_VMIXER , VOICES_MINUS_STREAMS ) == -1 )
//				Dbg_Message("Error setting reverb5\n");
//			
//		}
//	}
} // end of SetReverb( )


































} // namespace Sfx
