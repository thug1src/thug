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
**	Module:			Music (Pcm)			 									**
**																			**
**	File name:		ngc/p_music.cpp											**
**																			**
**	Created by:		08/23/01	-	dc										**
**																			**
**	Description:	NGC audio streaming										**
**																			**
*****************************************************************************/

#ifndef DVDETH
#define USE_VORBIS
#endif		// DVDETH

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <dolphin.h>
#include <core/macros.h>
#include <core/defines.h>
#include <sys/file/filesys.h>
#include <sys/file/ngc/hed.h>
#include <gel/music/ngc/pcm/pcm.h>
#include <sys\timer.h>

#ifdef USE_VORBIS
#include <gel/music/ngc/divx/AUDSimpleAudio.h>
#include <gel/music/ngc/divx/AUDSimplePlayer.h>
#endif

#include <gel/soundfx/soundfx.h>
#include <gel/scripting/symboltype.h> 
//#include <dolphin\dtk.h>
#include "p_music.h"
#include <sys/ngc/p_dvd.h>
#include "gfx\ngc\nx\nx_init.h"
#include <charpipeline/GQRSetup.h>

/*****************************************************************************
**							  DBG Information								**
*****************************************************************************/

#ifdef USE_VORBIS
// Player instance
extern AudSimplePlayer audio_player;
#endif

//bool g_using_dtk = false;
//extern bool g_in_cutscene;

// Info about last 8 reads.
static DVDFileInfo* last_fileInfo[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static void * last_addr[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }; 
static s32 last_length[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 last_offset[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

// Callback will happen 8 frames later.
DVDCallback last_callback[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };  
s32 last_callback_length[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; 
DVDFileInfo* last_callback_fileInfo[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
int last_callback_counter[8] = { -1, -1, -1, -1, -1, -1, -1, -1 }; 

namespace Pcm
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define MAX_DTK_VOL				0xFF				// Max volume for DTK hardware streamed audio tracks.
#define MAX_VOL					0x7FFF
#define NORMAL_VOL				( MAX_VOL >> 1 )
#define UNALTERED_PITCH			0x10000L

#define	QUIT_DMA_INACTIVE			0
#define	QUIT_DMA_PENDING			1
#define	QUIT_DMA_COMPLETE			2

#define STREAM_BUFFER_HEADER_SIZE	96				// Size of the header for each stream
#define STREAM_BUFFER_SIZE			16384

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

struct FileStreamInfo
{
	int				fileSize;
	int				offset;
	int				hedOffset;
	int				offsetInARAM;
	float			pitch;
	float			pitchAdjustmentForSamplingRate;
	float			volumeL;
	float			volumeR;
	bool			paused;
	bool			hasStartedPlaying;
	unsigned int	currentFileInfo;
	DVDFileInfo		fileInfo[8];
	AXVPB*			p_voice;
	uint32			nextWrite;		// 0 for first half of ARAM buffer, 1 for second half of ARAM buffer.
	bool			has_paused;
	bool			preloadActive;
};

// This is also defined in p_sfx.cpp - should probably share a common definition.
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



/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

//#ifndef USE_VORBIS
//#ifndef DVDETH
//static DTKTrack	s_dtk_track;
//#endif		// DVDETH
//#endif		// USE_VORBIS

FileStreamInfo		gStreamInfo[NUM_STREAMS];
int					gPcmStatus					= 0;
unsigned int		gStreamVolume				= 100;
File::SHed*			gpStreamHed					= NULL;
int					stream_base[3];
int					stream_size[3];
static char			stream_0_mem[STREAM_BUFFER_SIZE + STREAM_BUFFER_HEADER_SIZE] __attribute__((aligned( 32 )));	// Must be 32 for DVD.
static char			stream_1_mem[STREAM_BUFFER_SIZE + STREAM_BUFFER_HEADER_SIZE] __attribute__((aligned( 32 )));	// Must be 32 for DVD.
static char			stream_2_mem[STREAM_BUFFER_SIZE + STREAM_BUFFER_HEADER_SIZE] __attribute__((aligned( 32 )));	// Must be 32 for DVD.
static char*		stream_mem_buffer[3]		= { stream_0_mem, stream_1_mem, stream_2_mem };
static ARQRequest	stream_arq_request[NUM_STREAMS];
bool				musicPaused					= false;
float				musicVolume					= 0.0f;
bool				musicPreloadActive			= false;
bool				streamsPaused				= false;

static uint32		sPreLoadChecksum[NUM_STREAMS];
static uint32		sPreLoadMusicChecksum;
static char			sPreLoadMusicFilename[256];

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

BOOL _DVDReadAsync( DVDFileInfo* fileInfo, void* addr, s32 length, s32 offset, DVDCallback callback )
{
	// See if this is a repeat read.
	bool repeat_read = false;
	for ( int lp = 0; lp < 8; lp++ )
	{
		if ( ( fileInfo == last_fileInfo[lp] ) &&
			 ( addr == last_addr[lp] ) &&
			 ( length == last_length[lp] ) &&
			 ( offset == last_offset[lp] ) )
		{
			repeat_read = true;
			break;
		}
	}

	if ( repeat_read )
	{
		// Find an empty slot for this callback.
		int cb = -1;
		for ( int lp = 0; lp < 8; lp++ )
		{
			if ( last_callback_counter[lp] == -1 )
			{
				cb = lp;
				break;
			}
		}

		if ( cb == -1 )
		{
#			ifdef __NOPT_FINAL__
			OSReport( "Warning: Overflowed DVD callback buffer\n" );
#			else
			Dbg_MsgAssert( 0, ( "Overflowed DVD callback buffer" ));
#			endif
			// Issue read normally in this case.
			DVDReadAsync( fileInfo, addr, length, offset, callback ); 
		}
		last_callback[cb] = callback;
		last_callback_length[cb] = length;
		last_callback_fileInfo[cb] = fileInfo;
		last_callback_counter[cb] = 8;
		
		return TRUE;
	}
	else
	{
		// Shunt last reads down & fill in new one.
		for ( int lp = 7; lp > 0; lp-- )
		{
			last_fileInfo[lp] = last_fileInfo[lp-1];
			last_addr[lp] = last_addr[lp-1];
			last_length[lp] = last_length[lp-1];
			last_offset[lp] = last_offset[lp-1];
		}
		last_fileInfo[0] = fileInfo;
		last_addr[0] = addr;
		last_length[0] = length;
		last_offset[0] = offset;

		return DVDReadAsync( fileInfo, addr, length, offset, callback );
	}
}

void setDolby( AXPB * p_pb, AXVPB * p_vpb, float volL, float volR, unsigned int pitch, bool set_pitch, float volume_percent )
{
	float	volumes[4];
	int		i_volumes[4];	//, max_i_volume;
	volumes[0] = 0.0f;
	volumes[1] = 0.0f;
	volumes[2] = 0.0f;
	volumes[3] = 0.0f;
	
	if( Sfx::isStereo )
	{
		if(( volL == 0.0f ) && ( volR == 0.0f ))
		{
			// Dave note - in theory setting the mixerCtrl to zero should cause the sound not to play.
			// In practice however it seems to lead to small audio 'pops'.
//			p_pb->mixerCtrl = 0;
//			p_vpb->sync |= AX_SYNC_USER_MIXCTRL;
			
			// Pointless doing any more work.
			p_pb->mix.vL	= 0;
			p_pb->mix.vR	= 0;
			p_pb->mixerCtrl = AX_PB_MIXCTRL_L | AX_PB_MIXCTRL_R;
			p_vpb->sync	   |= AX_SYNC_USER_MIX | AX_SYNC_USER_MIXCTRL;
		}
		else
		{
			// Get the length of the vector here which will be used to multiply out the normalised speaker volumes.
			Mth::Vector test( fabsf( volL ), fabsf( volR ), 0.0f, 0.0f );
			float amplitude = test.Length();

			// Look just at the normalized right component to figure the sound angle from Matt's calculations.
			test.Normalize();

			float angle;
			angle	= asinf( test[Y] );
			angle	= ( angle * 2.0f ) - ( Mth::PI * 0.5f );
			angle	= ( volL < 0.0f ) ? ( Mth::PI - angle ) : angle;

			// Now figure volumes based on speaker coverage.
			angle	= Mth::RadToDeg( angle );

			// Left front channel.
			if(( angle >= 225.0f ) || ( angle <= 45.0f ))
			{
				// Because of the discontinuity, shift this angle round to the [0,180] range.
				float shift_angle = angle + 135;
				shift_angle = ( shift_angle >= 360.0f ) ? ( shift_angle - 360.0f ) : shift_angle;
				volumes[0]	= ( shift_angle / 180.0f ) * Mth::PI;
				volumes[0]	= sqrtf( sinf( volumes[0] ));
			}

//			// Center channel.
//			if(( angle >= -60.0f ) && ( angle <= 60.0f ))
//			{
//				// Scale this into [0,PI] space so we can get smooth fadeout.
//				volumes[1]	= (( angle + 60.0f ) / 120.0f ) * Mth::PI;
//				volumes[1]	= sqrtf( sinf( volumes[1] ));
//			}

			// Right front channel.
			if(( angle >= -45.0f ) && ( angle <= 135.0f ))
			{
				// Scale this into [0,PI] space so we can get smooth fadeout.
				volumes[1]	= (( angle + 45.0f ) / 180.0f ) * Mth::PI;
				volumes[1]	= sqrtf( sinf( volumes[1] ));
			}

			// Right rear channel.
			if(( angle >= 45.0f ) && ( angle <= 225.0f ))
			{
				// Scale this into [0,PI] space so we can get smooth fadeout.
				volumes[2]	= (( angle - 45.0f ) / 180.0f ) * Mth::PI;
				volumes[2]	= sqrtf( sinf( volumes[2] ));
			}

			// Left rear channel.
			if(( angle >= 135.0f ) || ( angle <= -45.0f ))
			{
				// Because of the discontinuity, shift this angle round to the [0,180] range.
				float shift_angle = angle + 225;
				shift_angle = ( shift_angle >= 360.0f ) ? ( shift_angle - 360.0f ) : shift_angle;
				volumes[3]	= ( shift_angle / 180.0f ) * Mth::PI;
				volumes[3]	= sqrtf( sinf( volumes[3] ));
			}

			// Now readjust the relative values...
			for( int v = 0; v < 4; ++v )
			{
				// Scale back up to original amplitude.
				volumes[v] *= amplitude;

				// Then adjust for SFX volume level, and scale into limits.
//				volumes[v] = PERCENT( volume_percent, volumes[v] );
//				volumes[v] = fabsf( PERCENT( sfx_manager->GetMainVolume(), volumes[v] ));

				if( volumes[v] > 100.0f )
					volumes[v] = 100.0f;

			}

			// Now figure the attenuation of the sound. To convert to a decibel value, figure the ratio of requested
			// volume versus max volume, then calculate the log10 and multiply by (10 * 2). (The 2 is because sound
			// power varies as square of pressure, and squaring doubles the log value).
//#define DSBVOLUME_EFFECTIVE_MIN -6400
//#define DSBVOLUME_MAX 0
//			max_i_volume = DSBVOLUME_EFFECTIVE_MIN;
			for( int v = 0; v < 4; ++v )
			{
//				if( volumes[v] > 0.0f )
//				{
//					float attenuation	= 20.0f * log10f( volumes[v] * 0.01f );
//					i_volumes[v]		= DSBVOLUME_MAX + (int)( attenuation * 100.0f );
//					if( i_volumes[v] < DSBVOLUME_EFFECTIVE_MIN )
//						i_volumes[v] = DSBVOLUME_EFFECTIVE_MIN;
//
//					if( i_volumes[v] > max_i_volume )
//						max_i_volume = i_volumes[v];
//				}
//				else
//				{
//					i_volumes[v] = DSBVOLUME_EFFECTIVE_MIN;
//				}
				// GameCube:: Change from -6400 -> 0 to 0 -> 0x1fff.
//				i_volumes[v] = ( ( i_volumes[v] + -DSBVOLUME_EFFECTIVE_MIN ) * 0x1fff ) / -DSBVOLUME_EFFECTIVE_MIN;
//				i_volumes[v] = (int)PERCENT( volume_percent, (float)i_volumes[v] );
				i_volumes[v] = (int)(( volumes[v] * (float)0x3fff ) / 100.0f);
				i_volumes[v] = (int)PERCENT( volume_percent, (float)i_volumes[v] );
			}

			// Set individual mixbins for panning. Clampo very low volumes to zero, and for zero volumes, early out
			// without setting the Dolby stuff up.
			if(( i_volumes[0] <= 32 ) && ( i_volumes[1] <= 32 ) && ( i_volumes[2] <= 32 ) && ( i_volumes[3] <= 32 ))
			{
				// Pointless doing any more work.
				p_pb->mix.vL	= 0;
				p_pb->mix.vR	= 0;
				p_pb->mixerCtrl = AX_PB_MIXCTRL_L | AX_PB_MIXCTRL_R;
				p_vpb->sync	   |= AX_SYNC_USER_MIX | AX_SYNC_USER_MIXCTRL;
			}
			else
			{
				p_pb->mixerCtrl = AX_PB_MIXCTRL_B_DPL2;
			
				if( i_volumes[0] > 0 )
				{
					p_pb->mix.vL = i_volumes[0];
					p_pb->mixerCtrl |= AX_PB_MIXCTRL_L;
				}
	
				if( i_volumes[1] > 0 )
				{
					p_pb->mix.vR = i_volumes[1];
					p_pb->mixerCtrl |= AX_PB_MIXCTRL_R;
				}
	
				if( i_volumes[2] > 0 )
				{
					p_pb->mix.vAuxBL = i_volumes[2];
					p_pb->mixerCtrl |= AX_PB_MIXCTRL_B_L;
				}
	
				if( i_volumes[3] > 0 )
				{
					p_pb->mix.vAuxBR = i_volumes[3];
					p_pb->mixerCtrl |= AX_PB_MIXCTRL_B_R;
				}
				p_vpb->sync |= AX_SYNC_USER_MIX | AX_SYNC_USER_MIXCTRL;
			}
		}
	}
	else
	{
		// Adjust for SFX volume level.
		volL = PERCENT( volume_percent, volL );
		volR = PERCENT( volume_percent, volR );

		// Adjust for channel volume.
		int lVol = (int)PERCENT( 0x3fff, volL );	
		int rVol = (int)PERCENT( 0x3fff, volR );

		// Clamp to maximum allowed volume.
		if ( lVol > 0x7fff )
			lVol = 0x7fff;
		else if ( lVol < 0 )
			lVol = 0;

		if ( rVol > 0x7fff )
			rVol = 0x7fff;
		else if ( rVol < 0 )
			rVol = 0;

		p_pb->mix.vL	= ( lVol + rVol ) / 2;
		p_pb->mix.vR	= ( lVol + rVol ) / 2;
		p_pb->mixerCtrl = AX_PB_MIXCTRL_L | AX_PB_MIXCTRL_R;
		p_vpb->sync	   |= AX_SYNC_USER_MIX | AX_SYNC_USER_MIXCTRL;
	}
	
	if ( set_pitch )
	{
		p_pb->src.ratioHi		= ( pitch >> 16 );
		p_pb->src.ratioLo		= ( pitch & 0xFFFF );
		p_vpb->sync			   |= AX_SYNC_USER_SRCRATIO;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static void ax_voice_reacquisition_callback( void* p )
{
	// Should never happen for stream voices.
	Dbg_Assert( 0 );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
////#ifndef USE_VORBIS
//#ifndef DVDETH
//static void s_dtk_callback( u32 event_mask )
//{
//	if( event_mask & DTK_EVENT_TRACK_ENDED )
//	{
//		// Stop the music track.
//		DTKSetState( DTK_STATE_STOP );
//		gPcmStatus &= ~PCMSTATUS_MUSIC_PLAYING;
//	}
//
//	if( event_mask & DTK_EVENT_PLAYBACK_STARTED )
//	{
//		if( musicPaused )
//		{
//			// Pause the music track.
//			DTKSetState( DTK_STATE_PAUSE );
//		}
//	}
//}
//#endif		// DVDETH
////#endif		// USE_VORBIS



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static void s_start_voice_callback( u32 pointerToARQRequest )
{
	// Figure which stream this is for.
	int stream = -1;
	
	for( int s = 0; s < NUM_STREAMS; ++s )
	{
		if( pointerToARQRequest == (u32)( &stream_arq_request[s] ))
		{
			stream = s;
			break;
		}
	}
	
	if( stream == -1 )
	{
		// Something has gone very wrong.
		Dbg_MsgAssert( 0, ( "Unsynced dvd callback" ));
		return;
	}

	// Deal with the voice already having been deleted.
	if( gStreamInfo[stream].p_voice == NULL )
	{
		return;
	}

	// Set the voice to run.
	gStreamInfo[stream].hasStartedPlaying	= true;
	gStreamInfo[stream].paused				= streamsPaused;
	if( !streamsPaused && !gStreamInfo[stream].preloadActive )
	{
		if( gStreamInfo[stream].p_voice == NULL )
		{
			Dbg_Assert( 0 );
			return;			
		}
		AXSetVoiceState( gStreamInfo[stream].p_voice, AX_PB_STATE_RUN );
	}

	// Adjust pitch to account for lower sampling rates:
	float			pitch	= PERCENT( gStreamInfo[stream].pitch, gStreamInfo[stream].pitchAdjustmentForSamplingRate );
	unsigned int	u_pitch	= (unsigned int)PERCENT( UNALTERED_PITCH, pitch );

	int old = OSDisableInterrupts();
	AXPB*	p_pb						= &( gStreamInfo[stream].p_voice->pb );
	Spt::SingletonPtr< Sfx::CSfxManager > sfx_manager;
	setDolby( p_pb, gStreamInfo[stream].p_voice, gStreamInfo[stream].volumeL, gStreamInfo[stream].volumeR, u_pitch, true, sfx_manager->GetMainVolume() );
	OSRestoreInterrupts( old );

	gPcmStatus |= PCMSTATUS_STREAM_PLAYING( stream );
	gPcmStatus &= ~PCMSTATUS_LOAD_STREAM( stream );
}

						

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static void s_dvd_callback( s32 result, DVDFileInfo* fileInfo )
{
	// Figure which stream this is for.
	int stream	= -1;

	for( unsigned int i = 0; i < 8; ++i )
	{
		for( uint32 s = 0; s < NUM_STREAMS; ++s )
		{
			if( fileInfo == &( gStreamInfo[s].fileInfo[i] ))
			{
				if( i == gStreamInfo[s].currentFileInfo )
				{
					stream = s;
					break;
				}
			}
		}
		if( stream >= 0 )
			break;
	}

	if( stream == -1 )
	{
		// Possibly the result of a read in which we are no longer interested.
		return;
	}

	// Handle read errors by just stopping the stream.
	if(( result < 0 ) || gStreamInfo[stream].has_paused )
	{
		int enabled = OSDisableInterrupts();
		if( gStreamInfo[stream].p_voice )
		{
			// This should be the case.
			AXFreeVoice( gStreamInfo[stream].p_voice );
		}
		gStreamInfo[stream].p_voice			= NULL;
		gStreamInfo[stream].preloadActive	= false;
		gStreamInfo[stream].currentFileInfo	= ( gStreamInfo[stream].currentFileInfo + 1 ) & 0x07;
		OSRestoreInterrupts( enabled );

		gPcmStatus &= ~PCMSTATUS_LOAD_STREAM( stream );
		gPcmStatus &= ~PCMSTATUS_STREAM_PLAYING( stream );
		return;
	}

	// Check if has already been stopped.
	AXVPB* p_axvpb = gStreamInfo[stream].p_voice;
	if( p_axvpb == NULL )
	{
		return;
	}

	// If this is the first chunk read for this voice, need to grab the voice and look at the header information.
	if( gStreamInfo[stream].offset == 0 )
	{
		AXPBADDR        addr;
		AXPBADPCM       adpcm;
		AXPBVE			ve;
		
		int enabled = OSDisableInterrupts();
		// Grab pointer to header (first 96 byes of data).
		sDSPADPCM*	p_header		= (sDSPADPCM*)( stream_mem_buffer[stream] );

		addr.format					= AX_PB_FORMAT_ADPCM;   

		// Depending on the number of bytes in the sound, we may not need to loop.
		// Also depending on how many times we will be uploading to ARAM, we may want to start the stream
		// halfway through the ARAM buffer, in order to ensure that we always write the last block to the upper half of ARAM
		// (in order that we can safely set the end position).
		int num_adpcm_bytes			= p_header->num_adpcm_nibbles / 2;

		// Convert from byte to nibble address, and split into low and high words.
		u32 start_address			= ( stream_base[stream] * 2 ) + 2;
		addr.currentAddressHi		= (u16)( start_address >> 16 );
		addr.currentAddressLo		= (u16)( start_address & 0xFFFFU );

		if( num_adpcm_bytes <= 32768 )
		{
			// Stream fits in to ARAM buffer completely. No need to loop.
			u32 end_address					= ( stream_base[stream] * 2 ) + p_header->num_adpcm_nibbles - 1;
		
			addr.endAddressHi				= (u16)( end_address >> 16 );
			addr.endAddressLo				= (u16)( end_address & 0xFFFFU );
		
			u32 zero_buffer_address			= NxNgc::EngineGlobals.aram_zero * 2;
			addr.loopAddressHi				= (u16)( zero_buffer_address >> 16 );
			addr.loopAddressLo				= (u16)( zero_buffer_address & 0xFFFFU );
		
			addr.loopFlag					= AXPBADDR_LOOP_OFF;
		
			gStreamInfo[stream].nextWrite	= 1;
//			OSReport( "Stream: %d is simple <= 32768 bytes\n", stream );
		}
		else
		{
			// Stream does not fit in to ARAM buffer completely. Needs to loop.
			uint32 num_uploads				= num_adpcm_bytes / STREAM_BUFFER_SIZE;
			if(( num_adpcm_bytes % STREAM_BUFFER_SIZE ) > 0 )
				++num_uploads;
			Dbg_Assert( num_uploads > 2 );

			addr.loopAddressHi		= addr.currentAddressHi;
			addr.loopAddressLo		= addr.currentAddressLo;
			
			if( num_uploads & 0x01 )
			{
				// This is the tricky situation, where we start halfway through the ARAM buffer.
				gStreamInfo[stream].offsetInARAM   += STREAM_BUFFER_SIZE;
				start_address					   += ( STREAM_BUFFER_SIZE * 2 );
				addr.currentAddressHi				= (u16)( start_address >> 16 );
				addr.currentAddressLo				= (u16)( start_address & 0xFFFFU );
				gStreamInfo[stream].nextWrite		= 0;
//				OSReport( "Stream: %d is complex tricky %d bytes\n", stream, num_adpcm_bytes );
			
			}
			else
			{
				// This is the easy situation, where we start at the start of the ARAM buffer.
				gStreamInfo[stream].offsetInARAM	= 0;
				gStreamInfo[stream].nextWrite		= 1;
//				OSReport( "Stream: %d is complex non-tricky %d bytes\n", stream, num_adpcm_bytes );
			}
			
			u32 end_address			= ( stream_base[stream] * 2 ) + ( 32768 * 2 ) - 1;
			addr.endAddressHi		= (u16)( end_address >> 16 );
			addr.endAddressLo		= (u16)( end_address & 0xFFFFU );
		
			addr.loopFlag			= AXPBADDR_LOOP_ON;
		}

		memcpy( &adpcm.a[0][0], p_header->coef, sizeof( u16 ) * 16 );
		adpcm.gain					= p_header->gain;
		adpcm.pred_scale			= p_header->ps;
		adpcm.yn1					= p_header->yn1;
		adpcm.yn2					= p_header->yn2;

		ve.currentVolume			= 0x7FFF;
		ve.currentDelta				= 0;

		// Set voice parameters.
		AXSetVoiceType( p_axvpb, AX_PB_TYPE_NORMAL );		// No loop context.
		AXSetVoiceSrcType( p_axvpb, AX_PB_SRCSEL_LINEAR );	// Use linear interpolation.
		AXSetVoiceVe( p_axvpb, &ve );						// Set overall volume.
		AXSetVoiceAddr( p_axvpb, &addr );					// Input addressing.
		AXSetVoiceAdpcm( p_axvpb, &adpcm );					// ADPCM coefficients.

//		p_axvpb->sync |= AX_SYNC_USER_ALLPARAMS;
//		AXSetVoiceSrcRatio( p_axvpb, (48000.0f / ((float)AX_IN_SAMPLES_PER_SEC)) );

		// If this chunk is less than buffersize, zero out the excess bytes to prevent sound pop.
		if( result < ( STREAM_BUFFER_SIZE + STREAM_BUFFER_HEADER_SIZE ))
		{
			memset( stream_mem_buffer[stream] + result, 0, ( STREAM_BUFFER_SIZE + STREAM_BUFFER_HEADER_SIZE ) - result );
			DCFlushRange( stream_mem_buffer[stream] + result, ( STREAM_BUFFER_SIZE + STREAM_BUFFER_HEADER_SIZE ) - result );
		}
		OSRestoreInterrupts( enabled );

		// DMA this chunk up to ARAM, then use the ARQ callback to start the voice.
		ARQPostRequest(	&stream_arq_request[stream],							// Address to user-allocated storage for an ARQTask data structure.
						0,														// User-enumerated identifier for the owner of the ARAM transaction.
						ARQ_TYPE_MRAM_TO_ARAM,									// Direction.
						ARQ_PRIORITY_HIGH,										// Priority.
						(u32)( stream_mem_buffer[stream] + 96 ),				// Source address.
						stream_base[stream] + gStreamInfo[stream].offsetInARAM,	// Destination address.
						STREAM_BUFFER_SIZE,										// Transfer length (bytes). Must be multiple of 4.
						&s_start_voice_callback );								// DMA complete callback function.

		gStreamInfo[stream].pitchAdjustmentForSamplingRate	= ((float)p_header->sample_rate * 100.0f ) / 32000.0f;
		
		// Increment the ARAM offset and wrap when appropriate.
		gStreamInfo[stream].offsetInARAM += STREAM_BUFFER_SIZE;
		if( gStreamInfo[stream].offsetInARAM >= ( STREAM_BUFFER_SIZE * 2 ))
		{
			gStreamInfo[stream].offsetInARAM -= ( STREAM_BUFFER_SIZE * 2 );
		}
		
		// Increment the file pointer (remember we read the extra buffer bytes for the first read).
		gStreamInfo[stream].offset += STREAM_BUFFER_SIZE + STREAM_BUFFER_HEADER_SIZE;
	}
	else
	{
		// If this chunk is less than buffersize, zero out the excess bytes to prevent sound pop.
		if( result < STREAM_BUFFER_SIZE )
		{
			memset( stream_mem_buffer[stream] + result, 0, STREAM_BUFFER_SIZE - result );
			DCFlushRange( stream_mem_buffer[stream] + result, STREAM_BUFFER_SIZE - result );
		}

		// We want to DMA this chunk up to ARAM, but only if safe to do so, i.e. only when the playback position is in
		// the opposite half of the ARAM buffer.
		int		current_aram_pos	= ((uint32)( p_axvpb->pb.addr.currentAddressHi ) << 16 ) | (uint32)( p_axvpb->pb.addr.currentAddressLo );
		bool	safe_to_dma			= false;

		// Convert from current ARAM position (in nibbles) to current offset within buffer (in bytes).
		current_aram_pos			= ( current_aram_pos - ( stream_base[stream] * 2 )) / 2;
		
		if( current_aram_pos < STREAM_BUFFER_SIZE )
		{
			// Safe to write to the second half of the ARAM buffer, if that is what needs updating.
			if( gStreamInfo[stream].nextWrite == 1 )
			{
				safe_to_dma = true;
//				OSReport( "Stream: %d ARAM: %d writing to second half\n", stream, current_aram_pos );
			}
		}
		else
		{
			// Safe to write to the first half of the ARAM buffer, if that is what needs updating.
			if( gStreamInfo[stream].nextWrite == 0 )
			{
				safe_to_dma = true;
//				OSReport( "Stream: %d ARAM: %d writing to first half\n", stream, current_aram_pos );
			}
		}
		
		if( safe_to_dma )
		{
			// DMA this chunk up to ARAM.
			ARQPostRequest(	&stream_arq_request[stream],							// Address to user-allocated storage for an ARQTask data structure.
							0,														// User-enumerated identifier for the owner of the ARAM transaction.
							ARQ_TYPE_MRAM_TO_ARAM,									// Direction.
							ARQ_PRIORITY_HIGH,										// Priority.
							(u32)( stream_mem_buffer[stream] ),						// Source address.
							stream_base[stream] + gStreamInfo[stream].offsetInARAM,	// Destination address.
							STREAM_BUFFER_SIZE,										// Transfer length (bytes). Must be multiple of 4.
							NULL );													// DMA complete callback function.

			// Increment the ARAM offset and wrap when appropriate.
			gStreamInfo[stream].offsetInARAM += STREAM_BUFFER_SIZE;
			if( gStreamInfo[stream].offsetInARAM >= ( STREAM_BUFFER_SIZE * 2 ))
			{
				gStreamInfo[stream].offsetInARAM -= ( STREAM_BUFFER_SIZE * 2 );
			}
	
			// We have now processed another chunk.
			gStreamInfo[stream].nextWrite = ( gStreamInfo[stream].nextWrite == 0 ) ? 1 : 0;
		
			// Increment the file pointer.
			gStreamInfo[stream].offset += STREAM_BUFFER_SIZE;
		}
		else
		{
//			OSReport( "Stream: %d ARAM: %d discarding file buffer\n", stream, current_aram_pos );
		}
	}

	// If there is more data, kick off another read. NOTE this could potentially write over what we are about to DMA to ARAM.
	int total_bytes_after_next_read	= gStreamInfo[stream].offset + STREAM_BUFFER_SIZE;
	int bytes_to_read;
	if( total_bytes_after_next_read <= gStreamInfo[stream].fileSize )
	{
		bytes_to_read = STREAM_BUFFER_SIZE;
	}
	else
	{
		bytes_to_read = STREAM_BUFFER_SIZE - ( total_bytes_after_next_read - gStreamInfo[stream].fileSize );

		// Reduce bytes_to_read down to the next lowest multiple of 32.
		bytes_to_read &= ~31;
	}

	if( bytes_to_read > 0 )
	{
		_DVDReadAsync(	&( gStreamInfo[stream].fileInfo[gStreamInfo[stream].currentFileInfo] ),
						stream_mem_buffer[stream],												// Base pointer of memory to read data into.
						bytes_to_read,															// Bytes to read.
						gStreamInfo[stream].hedOffset + gStreamInfo[stream].offset,				// Offset in file.
						s_dvd_callback );														// Read complete callback.
	}
	else
	{
		// Here we may need to set the stop point for the voice, and switch from looping to non-looping.
		if( p_axvpb->pb.addr.loopFlag == AXPBADDR_LOOP_ON )
		{
//			OSReport( "Stream: %d switching to loop off\n", stream );
			p_axvpb->pb.addr.loopFlag		= AXPBADDR_LOOP_OFF;
			u32 zero_buffer_address			= NxNgc::EngineGlobals.aram_zero * 2;
			p_axvpb->pb.addr.loopAddressHi	= (u16)( zero_buffer_address >> 16 );
			p_axvpb->pb.addr.loopAddressLo	= (u16)( zero_buffer_address & 0xFFFFU );
			p_axvpb->sync |= ( AX_SYNC_USER_LOOP | AX_SYNC_USER_LOOPADDR );
		}
	}
}



/*****************************************************************************
**							    Public Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PCMAudio_Init( void )
{
////#	ifdef USE_VORBIS
////#	else
//#	ifndef DVDETH
//	DTKInit();
//	DTKSetVolume( 64, 64 );		// Defaults to half volume.
//	DTKSetRepeatMode( DTK_MODE_NOREPEAT );
//#	endif		// DVDETH
////#	endif		// VORBIS

	memset( gStreamInfo, 0, sizeof( FileStreamInfo ) * NUM_STREAMS );

	// Just zero out the audio_player structure.
	memset( &audio_player, 0, sizeof( audio_player ));
	
	Sfx::GetStreamBufferInfo( &stream_base[0], &stream_base[1], &stream_base[2],
							  &stream_size[0], &stream_size[1], &stream_size[2] );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int PCMAudio_SetMusicVolume( float volume )
{
	// Save this value off so it can be used again for async startup.
	musicVolume = volume;
	
	// Normalize down to match PS2.
	volume *= 0.5f;

//#	ifdef USE_VORBIS
//	if ( !g_using_dtk )
	{
#		ifdef USE_VORBIS
		if( PCMAudio_GetMusicStatus() == PCM_STATUS_PLAYING )
		{
			uint32 vol = (uint32)PERCENT( volume, 0x7FFF );
			AUDSimpleAudioSetVolume( vol, vol );
		}
#		endif	// VORBIS
	}
//	else
//	{
//		int vol = (int)PERCENT( volume, MAX_DTK_VOL );
//		if( vol > MAX_DTK_VOL )
//		{
//			vol = MAX_DTK_VOL;
//		}
//	#	ifndef DVDETH
//		DTKSetVolume( vol, vol );
//	#	endif	// DVDETH
//	}
////#	else	
////#	endif	// VORBIS

	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_LoadMusicHeader( const char *nameOfFile )
{
	// For now, just fix this as true.
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_TrackExists( const char *pTrackName, int ch )
{
	if( ch == MUSIC_CHANNEL )
	{
		// For now, assume all music tracks exist.
		return true;

/*
		if( !gpMusicHed )
		{
			return false;
		}

		if( !FindFileInHed( pTrackName, gpMusicHed ))
		{
			Dbg_Message( "Track %s not found in header file.", pTrackName );
			return false;
		}

		return true;
*/
	}

	if( !gpStreamHed )
	{
		return false;
	}
	if( !FindFileInHed( pTrackName, gpStreamHed ))
	{
		return false;
	}
	return true;
}













#ifdef USE_VORBIS
/*!
 ******************************************************************************
 * \brief
 *		Memory allocation callback.
 *
 *		The player calls this function for all its memory needs. In this example
 *		an OSAlloc() is all we need to do.
 *
 * \note
 *		The returned memory address MUST be aligned on a 32 byte boundary.
 *		Otherwise, the player will fail!
 *
 * \param size
 *		Number of bytes to allocate
 *
 * \return
 *		Ptr to allocated memory (aligned to 32 byte boundary)
 *	
 ******************************************************************************
 */

static void* myAlloc(u32 size)
{
//	if ( g_in_cutscene )
//	{
//		int mem_available;
//
//		Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().FrontEndHeap() );
//		mem_available = Mem::Manager::sHandle().Available();
//		if ( (int)size < ( mem_available - ( 30 * 1024 ) ) )
//		{
//		}
//		else
//		{
//			Mem::Manager::sHandle().PopContext();
//			Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ThemeHeap() );
//			mem_available = Mem::Manager::sHandle().Available();
//			if ( (int)size < ( mem_available - ( 5 * 1024 ) ) )
//			{
//			}
//			else
//			{
//				Mem::Manager::sHandle().PopContext();
//				Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().ScriptHeap() );
//				mem_available = Mem::Manager::sHandle().Available();
//				if ( (int)size < ( mem_available - ( 15 * 1024 ) ) )
//				{
//				}
//				else
//				{
//					Mem::Manager::sHandle().PopContext();
//					Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().CutsceneTopDownHeap());	
//				}
//			}
//		}
//	}
//	else
//	{

//	Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().TopDownHeap());
//	Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().BottomUpHeap());
	Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().AudioHeap());

//	}
	Mem::Manager::sHandle().BottomUpHeap()->PushAlign( 32 );
	void * p = new u8[size]; 
	Mem::Manager::sHandle().BottomUpHeap()->PopAlign();
	Mem::Manager::sHandle().PopContext();

	return p;
}

/*!
 ******************************************************************************
 * \brief
 *		Memory free callback.
 *
 * \param ptr
 *		Memory address to free
 *	
 ******************************************************************************
 */
static void myFree(void* ptr)
{
	ASSERT(ptr);	// free on address 0 is only valid in C++ delete
	delete [] ptr;
}





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PCMAudio_DecodeLoop( void )
{
	// Deal with the asynchronous opening of music tracks.
	if( audio_player.asyncOpenCallbackStatus > 0 )
	{
		gPcmStatus |= PCMSTATUS_MUSIC_PLAYING;
	
		if( audio_player.asyncOpenCallbackStatus == ASYNC_OPEN_SUCCESS )
		{
			// Reset the callback status.
			audio_player.asyncOpenCallbackStatus = 0;
			
			// At this point we can complete the initialization process.
			if( AUDSimpleAllocDVDBuffers() == FALSE )
				OSHalt("*** AUDSimpleAllocDVDBuffers failed!\n");
			if( AUDSimpleCreateDecoder() == FALSE )
				OSHalt("*** AUDSimpleCreateDecoder failed!\n");
			if( AUDSimpleInitAudioDecoder() == FALSE )
				OSHalt("*** AUDSimpleInitAudioDecoder failed!\n");
			
			// Reset the music volume, now that we have voices allocated.
			PCMAudio_SetMusicVolume( musicVolume );
			
			// Preload all DVD buffers and set 'loop' mode to false.
			AUDSimpleLoadStart( FALSE );
		}
		return;
	}
	
	// Decode one frame.
	if( audio_player.decoder && !musicPaused )
	{
		gPcmStatus |= PCMSTATUS_MUSIC_PLAYING;
		
		if(	audio_player.decodeComplete )
		{
			if(	audio_player.playbackComplete )
			{
//				OSReport( "Ending song\n" );
			
				// Come to the end of the music data. Free our resources.
				AUDSimpleLoadStop();
				AUDSimpleClose();
				AUDSimpleFreeDVDBuffers();
				
				AUDSimpleDestroyDecoder();
				AUDSimpleExitAudioDecoder();
			
				gPcmStatus &= ~PCMSTATUS_MUSIC_PLAYING;
			}
		}
		else if( audio_player.preFetchState && !musicPreloadActive )
		{
			// See how many free samples in the read buffer (the big buffer into which the bitstream
			// is decoded). Overflowing this buffer causes DivX to behave very badly.
			u32	free_samples = AUDSimpleAudioGetFreeReadBufferSamples();
//			OSReport( "Free samples is: %d\n", free_samples );

			if( free_samples > 20000 )
			{
				// 20000 samples was derived empirically.
				if( AUDSimpleDecode() == FALSE )
				{
//					OSReport( "Decode complete\n" );
					audio_player.decodeComplete = true;
				}
			}
			else if( audio_player.playbackComplete )
			{
				// Seems like if the CPU is starved, the playbackComplete flag can be set despite the 
				// decodeComplete flag not being set. Try to cater for this situation here.
				if( AUDSimpleDecode() == FALSE )
				{
//					OSReport( "Decode complete\n" );
					audio_player.decodeComplete = true;
				}
			}
		}
	}

	// The decoder changes these, so reset them, otherwise animations/particles are messed up.
    GQRSetup6( GQR_SCALE_64,		// Pos
               GQR_TYPE_S16,
			   GQR_SCALE_64,
               GQR_TYPE_S16 );
    GQRSetup7( 14,		// Normal
               GQR_TYPE_S16,
               14,
               GQR_TYPE_S16 );
//	if( AUDSimpleCheckDVDError() == FALSE )
//	{
//		OSHalt( "*** DVD error occured. A true DVD error handler is not part of this demo.\n" );
//	}


}
#endif







/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_PlayMusicTrack( const char *filename, bool preload )
{
	// Set the preload flag.
	musicPreloadActive = preload;
	
	if ( NxNgc::EngineGlobals.disableReset ) return false;

	if( PCMAudio_GetMusicStatus() != PCM_STATUS_FREE )
	{
		Dbg_MsgAssert( 0, ( "Playing new track without stopping the first track." ));
		return false;
	}

	File::StopStreaming();

//#	ifdef USE_VORBIS
#	ifdef USE_VORBIS
	{
		// Fix up the filename, which comes in the form music\vag\songs\<foo>, and add the .ogg extension.
		static char	name_buffer[256] = "music/ogg/";
		strcpy( &name_buffer[10], &filename[10] );
		strcpy( name_buffer + strlen( name_buffer ), ".ogg" );
		char* p = name_buffer;
		while( *p != '\0' )
		{
			if ( *p == '\\' ) *p = '/';
			++p;
		}

		AUDSimpleInit( myAlloc, myFree );
		
		// This will open the file asynchronously, so nothing after this point except wait.
		if( AUDSimpleOpen( name_buffer ) == FALSE )
		{
			// File not found.
			return false;
		}

		audio_player.decodeComplete		= false;
		audio_player.playbackComplete	= false;
		
//		if( AUDSimpleAllocDVDBuffers() == FALSE )
//			OSHalt("*** AUDSimpleAllocDVDBuffers failed!\n");
//		if( AUDSimpleCreateDecoder() == FALSE )
//			OSHalt("*** AUDSimpleCreateDecoder failed!\n");
//		if( AUDSimpleInitAudioDecoder() == FALSE )
//			OSHalt("*** AUDSimpleInitAudioDecoder failed!\n");

		// Preload all DVD buffers and set 'loop' mode
//		AUDSimpleLoadStart( FALSE );
		
		return true;
	}
#	endif		// USE_VORBIS
////#	else
//
//	// Allow turning off of music from script global.
////	int testMusic = Script::GetInt( 0x47ac7ba5, false ); // checksum 'testMusicFromHost'
////	if( !testMusic )
////	{
////		return false;
////	}
//
//	{
//		// Fix up the filename, and add the .dtk extension.
//		static char	name_buffer[256] = "music/dtk/";
//		strcpy( &name_buffer[10], &filename[10] );
//		strcpy( name_buffer + strlen( name_buffer ), ".dtk" );
//		char* p = name_buffer;
//		while( *p != '\0' )
//		{
//			if ( *p == '\\' ) *p = '/';
//			++p;
//		}
//
//	#	ifndef DVDETH
//		u32 dtk_result = DTKQueueTrack( name_buffer, &s_dtk_track, 0xff, s_dtk_callback );
//	//	Dbg_MsgAssert( dtk_result == DTK_QUEUE_SUCCESS, ( "Failed to play: %s\n", name_buffer ));
//
//		if( dtk_result == DTK_QUEUE_SUCCESS )
//		{
//			if(	musicPaused )
//			{
//				// The callback will handle pausing the track.
//				DTKSetState( DTK_STATE_RUN );
//	//			DTKSetState( DTK_STATE_PAUSE );
//			}
//			else
//			{
//				DTKSetState( DTK_STATE_RUN );
//			}
//
//			gPcmStatus |= PCMSTATUS_MUSIC_PLAYING;
//			return true;
//		}
//	#	endif		// DVDETH
//		// If we get here then the track did not get queued.
//	}
//	return false;
////#	endif // USE_VORBIS
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PCMAudio_StopMusic( bool waitPlease )
{
//	int status = PCMAudio_GetMusicStatus();

//	if ( g_using_dtk )
//	{
//		if( status == PCM_STATUS_LOADING )
//		{
//			Dbg_MsgAssert( 0, ( "What, how can we be loading?" ));
//		}
//		else if( status == PCM_STATUS_PLAYING )
//		{
//			// Stop the track playing.
//#			ifndef DVDETH
//			DTKSetState( DTK_STATE_STOP );
//#			endif		// DVDETH
//		}
//		gPcmStatus &= ~PCMSTATUS_MUSIC_PLAYING;
//	}
//	else
	{
#		ifdef USE_VORBIS
		if( audio_player.asyncOpenCallbackStatus > 0 )
		{
			// Player is in async startup state, hasn't allocated buffers yet.
			audio_player.error = TRUE;
		}
		else if( audio_player.decoder )
		{
			if( !audio_player.preFetchState )
			{
				// Still seeding the DVD buffers, so dangerous to stop the music immediately.
				s32 tick = OSGetTick();
				while( audio_player.preFetchState == false )
				{
					s32 diff = OSDiffTick( OSGetTick(), tick );
					
					// Wait no more than quarter of a second.
					if( diff > (s32)( OS_TIMER_CLOCK / 4 ))
					{
						break;
					}
				}
			}
			
			AUDSimpleAudioReset();
			AUDSimpleLoadStop();
			AUDSimpleClose();
	
			AUDSimpleFreeDVDBuffers();
			AUDSimpleDestroyDecoder();
			AUDSimpleExitAudioDecoder();
		}
#	endif
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int PCMAudio_GetMusicStatus( void )
{
//	if ( g_using_dtk )
//	{
//		if( gPcmStatus & PCMSTATUS_LOAD_MUSIC )
//		{
//			return PCM_STATUS_LOADING;
//		}
//		else if( gPcmStatus & PCMSTATUS_MUSIC_PLAYING )
//		{
//			return PCM_STATUS_PLAYING;
//		}
//		else
//		{
//			// Sanity check.
//	#		ifndef DVDETH
//			u32 state = DTKGetState();
//			if( state == DTK_STATE_BUSY )
//			{
//				return PCM_STATUS_PLAYING;
//			}
//	#		endif		// DVDETH
//			return PCM_STATUS_FREE;
//		}
//		return 0;
//	}
//	else
	{
#		ifdef USE_VORBIS
		if( audio_player.asyncOpenCallbackStatus > 0 )
		{
			return PCM_STATUS_LOADING;
		}
		if( audio_player.decoder )
		{
			return PCM_STATUS_PLAYING;
		}
#		endif // USE_VORBIS
		return PCM_STATUS_FREE;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PCMAudio_Pause( bool pause, int ch )
{
	if( ch == MUSIC_CHANNEL )
	{
		musicPaused = pause;

//		if ( g_using_dtk )
//		{
//#			ifndef DVDETH
//			u32 state = DTKGetState();
//			if( pause && ( state == DTK_STATE_RUN ))
//			{
//				DTKSetState( DTK_STATE_PAUSE );
//			}
//			else if( !pause && ( state == DTK_STATE_PAUSE ))
//			{
//				DTKSetState( DTK_STATE_RUN );
//			}
//#			endif	// DVDETH
//		}
//		else
		{
#			ifdef USE_VORBIS
			if( audio_player.decoder )
			{
				AUDSimplePause( pause );
			}

//			PCMAudio_StopMusic( true );
#			endif // USE_VORBIS
		}
//#		ifdef USE_VORBIS
//#		else
//#		endif	// USE_VORBIS
	}
	else
	{
		streamsPaused = pause;

		for( int i = 0; i < NUM_STREAMS; ++i )
		{
			if( gStreamInfo[i].p_voice )
			{
				if( pause && !gStreamInfo[i].paused )
				{
					gStreamInfo[i].paused = true;
					AXSetVoiceState( gStreamInfo[i].p_voice, AX_PB_STATE_STOP );
				}
				else if( !pause && gStreamInfo[i].paused )
				{
					gStreamInfo[i].paused = false;
					AXSetVoiceState( gStreamInfo[i].p_voice, AX_PB_STATE_RUN );
				}
			}
		}
		if ( pause )
		{
			for( int s = 0; s < NUM_STREAMS; ++s )
			{
				gStreamInfo[s].has_paused = true;
				if( gStreamInfo[s].p_voice )
					setDolby( &gStreamInfo[s].p_voice->pb, gStreamInfo[s].p_voice, 0, 0 );
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/* Returns true if there is a pcm error.                          */
/*                                                                */
/******************************************************************/
//static uint64 last_vb = 0;
int PCMAudio_Update( void )
{
//	uint64 vb = Tmr::GetVblanks();
//	if ( last_vb == vb ) return true;
//	last_vb = vb;

	gPcmStatus &= ~PCMSTATUS_MUSIC_PLAYING;

//	if ( g_using_dtk )
//	{
//#		ifndef DVDETH
//		u32 state = DTKGetState();
//	
//		if( state != DTK_STATE_STOP )
//		{
//			gPcmStatus |= PCMSTATUS_MUSIC_PLAYING;
//		}
//#		endif	// DVDETH
//	}
//	else
	{
#		ifdef USE_VORBIS
		PCMAudio_DecodeLoop();
#		endif		// USE_VORBIS
	}
#	ifdef USE_VORBIS
#	else
#	endif	// USE_VORBIS

	// Check the stream voices to see if they are finished playing.
	bool check = false;
	for( int i = 0; i < NUM_STREAMS; ++i )
	{
		if( gStreamInfo[i].p_voice )
		{
			if(( gStreamInfo[i].p_voice->pb.state == AX_PB_STATE_STOP ) && ( gStreamInfo[i].hasStartedPlaying ) && !gStreamInfo[i].paused && !gStreamInfo[i].preloadActive )
			{
				// Playback on this voice has ended, so free up the voice.
				int enabled = OSDisableInterrupts();
				AXFreeVoice( gStreamInfo[i].p_voice );
				gStreamInfo[i].p_voice = NULL;
				gStreamInfo[i].currentFileInfo = ( gStreamInfo[i].currentFileInfo + 1 ) & 0x07;
				OSRestoreInterrupts( enabled );

				gPcmStatus &= ~PCMSTATUS_LOAD_STREAM( i );
				gPcmStatus &= ~PCMSTATUS_STREAM_PLAYING( i );
//				OSReport( "freed voice for stream: %d\n", i );
			}
			else
			{
				// Check to see whether the sound has played beyond the amount of data already dma'ed.
				int offset = gStreamInfo[i].p_voice->pb.addr.currentAddressLo + ((int)gStreamInfo[i].p_voice->pb.addr.currentAddressHi * 65536 );

				// Convert offset from nibble to byte address, and make relative to stream base.
				offset = ( offset / 2 ) - stream_base[i];
//				OSReport( "stream %d offset is at: %d\n", i, offset );

				if( offset > gStreamInfo[i].offsetInARAM )
				{
//					OSReport( "play point beyond byuffer...\n" );
				}

				// Flag this stream as playing.
				gPcmStatus |= PCMSTATUS_STREAM_PLAYING( i );
				check = true;
			}
		}
	}
	// Deal with disk errors.
	if ( gPcmStatus & PCMSTATUS_MUSIC_PLAYING ) check = true;
	if ( check ) DVDCheckAsync();
	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint32 PCMAudio_FindNameFromChecksum( uint32 checksum, int ch )
{
	File::SHed*	pHed;

	if( ch == EXTRA_CHANNEL )
	{
		pHed = gpStreamHed;
	}
	else
	{
		// Don't use the music hed file.
		return NULL;
	}

	if( !pHed )
	{
		return NULL;
	}

	File::SHedFile* pHedFile = File::FindFileInHedUsingChecksum( checksum, pHed );
	if( pHedFile )
	{
		return pHedFile->Checksum;
	}
	return NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_PlayStream( uint32 checksum, int whichStream, float volumeL, float volumeR, float fPitch, bool preload )
{
//#ifdef 
	
	if( PCMAudio_GetStreamStatus( whichStream ) != PCM_STATUS_FREE )
	{
		return false;
	}

//	int testStreams = Script::GetInt( 0x62df9442, false ); // checksum 'testStreamsFromHost'
//	if( !testStreams )
//	{
//		return false;
//	}

	// Incoming filenames are either of the form '\skateshp\menu', or '\streams\la\la_argue51' etc.
	// Strip the preceeding '\streams' if present.
//	if( strnicmp( filename, "\\streams", 8 ) == 0 )
//	{
//		filename += 8;
//	}
//
//	static char name_buffer[128] = "streams/dsp";
//	const int MUSIC_PREPEND_START_POS = 11;
//
//	// Need to append file type. Copy string into a buffer. Check there is room to add the extension.
//	int length = strlen( filename );
//	Dbg_Assert( length <= ( 128 - ( MUSIC_PREPEND_START_POS + 4 )));
//	strcpy( name_buffer + MUSIC_PREPEND_START_POS, filename );
//	strcpy( name_buffer + MUSIC_PREPEND_START_POS + length, ".dsp" );
//
//	// Fix directory separators.
//	char* p = name_buffer;
//	while( *p != '\0' )
//	{
//		if ( *p == '\\' ) *p = '/';
//		++p;
//	}

	File::SHedFile *pHed = FindFileInHedUsingChecksum( checksum, gpStreamHed );

#ifdef DVDETH
	BOOL result = false;
#else
	BOOL result = DVDFastOpen( DVDConvertPathToEntrynum( "streams/streamsngc.wad" ), &( gStreamInfo[whichStream].fileInfo[gStreamInfo[whichStream].currentFileInfo] ) );
#endif
//	DVDSeek( &( gStreamInfo[whichStream].fileInfo[gStreamInfo[whichStream].currentFileInfo] ), pHed->Offset );
	if( result )
	{
		if ( pHed && pHed->FileSize )
		{
			// Allocate a voice for use, and store it for later.
			AXVPB*		p_axvpb							= AXAcquireVoice( AX_PRIORITY_NODROP, ax_voice_reacquisition_callback, 0 );
			Dbg_Assert( p_axvpb );
			gStreamInfo[whichStream].p_voice			= p_axvpb;
			gStreamInfo[whichStream].paused				= false;
			gStreamInfo[whichStream].hasStartedPlaying	= false;
	
			// Flag that we are loading the stream up.
			gPcmStatus |= PCMSTATUS_LOAD_STREAM( whichStream );
	
			// Read in the first chunk.
			gStreamInfo[whichStream].offset				= 0;
			gStreamInfo[whichStream].offsetInARAM		= 0;
			gStreamInfo[whichStream].nextWrite			= 0;
	
			gStreamInfo[whichStream].hedOffset			= pHed->Offset;
			
			// Round filesize up to 32 byte alignment.
			gStreamInfo[whichStream].fileSize			= pHed->FileSize;
			gStreamInfo[whichStream].pitch				= fPitch;
			gStreamInfo[whichStream].volumeL			= volumeL;
			gStreamInfo[whichStream].volumeR			= volumeR;
//			OSReport( "%s %d\n", name_buffer, gStreamInfo[whichStream].fileSize );
	
			// Shhhh.....
			setDolby( &p_axvpb->pb, p_axvpb, 0, 0 );

			// Set the preload flag.
			gStreamInfo[whichStream].preloadActive = preload;
			
			// The first chunk we read is STREAM_BUFFER_HEADER_SIZE bigger than normal, so that even with the header in
			// the first read, we can still DMA the full amount up to ARAM.
			int bytes_to_read = ( pHed->FileSize >= ( STREAM_BUFFER_SIZE + STREAM_BUFFER_HEADER_SIZE )) ? ( STREAM_BUFFER_SIZE + STREAM_BUFFER_HEADER_SIZE ) : ( pHed->FileSize & ~31 );
			gStreamInfo[whichStream].has_paused = false;
//			OSReport( "Starting stream %x on channel %d\n", checksum, whichStream );
			result = DVDReadAsync(	&( gStreamInfo[whichStream].fileInfo[gStreamInfo[whichStream].currentFileInfo] ),
									stream_mem_buffer[whichStream],		// Base pointer of memory to read data into.
									bytes_to_read,						// Bytes to read.
									pHed->Offset,						// Offset in file.
									s_dvd_callback );					// Read complete callback.
	
			// Nothing more required for now.
			return true;
		}
		else
		{
			DVDClose( &( gStreamInfo[whichStream].fileInfo[gStreamInfo[whichStream].currentFileInfo] ));
		}
	}
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PCMAudio_SetStreamGlobalVolume( unsigned int volume )
{
	gStreamVolume = volume;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_SetStreamVolume( float volumeL, float volumeR, int whichStream )
{
	if( gStreamInfo[whichStream].p_voice )
	{
		int old = OSDisableInterrupts();

		AXPB*	p_pb	= &( gStreamInfo[whichStream].p_voice->pb );

		Spt::SingletonPtr< Sfx::CSfxManager > sfx_manager;
		volumeL = PERCENT( sfx_manager->GetMainVolume() / 2.0f, volumeL );
		volumeR = PERCENT( sfx_manager->GetMainVolume() / 2.0f, volumeR );
		setDolby( p_pb, gStreamInfo[whichStream].p_voice, volumeL, volumeR );

		OSRestoreInterrupts( old );

//		// Adjust stream volume based on global sound effect volume.
//		Spt::SingletonPtr< Sfx::CSfxManager > sfx_manager;
//		p_pb->mix.vL						= (unsigned int)(( volL * sfx_manager->GetMainVolume()) * 0.01f );
//		p_pb->mix.vR						= (unsigned int)(( volR * sfx_manager->GetMainVolume()) * 0.01f );
//
//		if( !Sfx::isStereo )
//		{
//			unsigned int mix	= ( p_pb->mix.vL + p_pb->mix.vR ) / 2;
//			p_pb->mix.vL		= mix;
//			p_pb->mix.vR		= mix;
//		}
//
//		gStreamInfo[whichStream].p_voice->sync  |= ( AX_SYNC_USER_MIX );
	}

	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_SetStreamPitch( float fPitch, int whichStream )
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PCMAudio_StopStream( int whichStream, bool waitPlease )
{
	// Clear the preload flag to be safe.
	gStreamInfo[whichStream].preloadActive	= false;
	sPreLoadChecksum[whichStream]			= 0;
	
	if( gStreamInfo[whichStream].p_voice )
	{
		int enabled = OSDisableInterrupts();
		AXFreeVoice( gStreamInfo[whichStream].p_voice );
		gStreamInfo[whichStream].p_voice			= NULL;
		gStreamInfo[whichStream].currentFileInfo	= ( gStreamInfo[whichStream].currentFileInfo + 1 ) & 0x07;
		OSRestoreInterrupts( enabled );

		gPcmStatus &= ~PCMSTATUS_LOAD_STREAM( whichStream );
		gPcmStatus &= ~PCMSTATUS_STREAM_PLAYING( whichStream );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PCMAudio_StopStreams( void )
{
	for( int i = 0; i < NUM_STREAMS; i++ )
	{
		PCMAudio_StopStream( i, true );
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/******************************************************************/
/*                                                                */
/* Get a stream loaded into a buffer, but don't play yet.		  */
/*                                                                */
/******************************************************************/
bool PCMAudio_PreLoadStream( uint32 checksum, int whichStream )
{
	bool rv = PCMAudio_PlayStream( checksum, whichStream, 0.0f, 0.0f, 100.0f, true );
	if( rv )
	{
		sPreLoadChecksum[whichStream] = checksum;
	}
	return rv;
}



/******************************************************************/
/*                                                                */
/* Returns true if preload done. Assumes that caller is calling	  */
/* this on a preloaded, but not yet played, stream. The results	  */
/* are meaningless otherwise.									  */
/*                                                                */
/******************************************************************/
bool PCMAudio_PreLoadStreamDone( int whichStream )
{
	if( gStreamInfo[whichStream].preloadActive )
	{
		// The hasStartedPlaying member will be set to true in the start voice callback, which is when
		// we are ready to allow playback.
		if( gStreamInfo[whichStream].hasStartedPlaying )
		{
			return true;
		}
	}
	return false;
}



/******************************************************************/
/*                                                                */
/* Tells a preloaded stream to start playing. Must call			  */
/* PCMAudio_PreLoadStreamDone() first to guarantee that it starts */
/* immediately.													  */
/*                                                                */
/******************************************************************/
bool PCMAudio_StartPreLoadedStream( int whichStream, float volumeL, float volumeR, float pitch )
{
	Dbg_Assert( gStreamInfo[whichStream].offset > 0 );
	
	sPreLoadChecksum[whichStream] = 0;
	
	if( gStreamInfo[whichStream].p_voice )
	{
		gStreamInfo[whichStream].preloadActive = false;
		
		// If we are not paused, set voice to RUN.
		if( !streamsPaused )
		{
			AXSetVoiceState( gStreamInfo[whichStream].p_voice, AX_PB_STATE_RUN );
		}
		
		// Set the correct volume.
		PCMAudio_SetStreamVolume( volumeL, volumeR, whichStream );
		PCMAudio_SetStreamPitch( pitch, whichStream );
		return true;
	}
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_PreLoadMusicStream( uint32 checksum )
{
	sPreLoadMusicChecksum = checksum;

	static char* p_filename[] =
	{
		"AU/AU_01V_Female",
		"AU/AU_01V_Male",
		"AU/AU_02_Female",
		"AU/AU_02_Male",
		"Final/Final_Female",
		"Final/Final_Male",
		"FL/FL_01_Female",
		"FL/FL_01_Male",
		"FL/FL_02_Female",
		"FL/FL_02_Male",
		"FL/FL_03_Female",
		"FL/FL_03_Male",
		"FL/FL_04_Female",
		"FL/FL_04_Male",
		"HI/HI_01_Female",
		"HI/HI_01_Male",
		"HI/HI_02V_Female",
		"HI/HI_02V_Male",
		"HI/HI_03_Female",
		"HI/HI_03_Male",
		"HI/HI_04_Female",
		"HI/HI_04_Male",
		"HI/HI_05_Female",
		"HI/HI_05_Male",
		"Intro/Intro_01_Female",
		"Intro/Intro_01_Male",
		"Intro/Intro_02_Female",
		"Intro/Intro_02_Male",
		"NJ/NJ_01V_Female",
		"NJ/NJ_01V_Male",
		"NJ/NJ_02A_Female",
		"NJ/NJ_02A_Male",
		"NJ/NJ_02B_Female",
		"NJ/NJ_02B_Male",
		"NJ/NJ_02_Female",
		"NJ/NJ_02_Male",
		"NJ/NJ_03_Female",
		"NJ/NJ_03_Male",
		"NJ/NJ_04_Female",
		"NJ/NJ_04_Male",
		"NJ/NJ_05B_Female",
		"NJ/NJ_05B_Male",
		"NJ/NJ_05_Female",
		"NJ/NJ_05_Male",
		"NJ/NJ_06_Female",
		"NJ/NJ_06_Male",
		"NJ/NJ_07_Female",
		"NJ/NJ_07_Male",
		"NJ/NJ_08_Female",
		"NJ/NJ_08_Male",
		"NJ/NJ_09_ALT_Female",
		"NJ/NJ_09_ALT_Male",
		"NJ/NJ_09_Female",
		"NJ/NJ_09_Male",
		"NJ/NJ_10_Female",
		"NJ/NJ_10_Male",
		"NJ/NJ_Pool_Female",
		"NJ/NJ_Pool_Male",
		"NY/NY_01V_Female",
		"NY/NY_01V_Male",
		"NY/NY_02_Female",
		"NY/NY_02_Male",
		"NY/NY_03_Female",
		"NY/NY_03_Male",
		"RU/RU_01V_Female",
		"RU/RU_01V_Male",
		"RU/RU_02_Female",
		"RU/RU_02_Male",
		"RU/RU_03B_Female",
		"RU/RU_03B_Male",
		"RU/RU_03_Female",
		"RU/RU_03_Male",
		"SD/SD_01_Female",
		"SD/SD_01_Male",
		"SD/SD_02_Female",
		"SD/SD_02_Male",
		"SD/SD_03_Female",
		"SD/SD_03_Male",
		"SJ/SJ_01A_Female",
		"SJ/SJ_01A_Male",
		"SJ/SJ_01B_Female",
		"SJ/SJ_01B_Male",
		"SJ/SJ_02_Female",
		"SJ/SJ_02_Male",
		"VC/VC_01V_Female",
		"VC/VC_01V_Male",
		"VC/VC_02_Female",
		"VC/VC_02_Male"
	};

	// Clear the preload flag.
	musicPreloadActive = false;
	
	for ( uint lp = 0; lp < ( sizeof( p_filename ) / sizeof( char* )); lp++ )
	{
		char *p_name = strchr( p_filename[lp], '/' );
		if( p_name )
		{
			uint32 name_checksum = Crc::GenerateCRCFromString( p_name + 1 );
			if( checksum == name_checksum )
			{
				// Got a match - hook up filename here....
				sprintf( sPreLoadMusicFilename, "music/vag/cutscenes/%s", p_filename[lp] );
				return PCMAudio_PlayMusicTrack( sPreLoadMusicFilename, true );
			}
		}
	}
	sPreLoadMusicFilename[0] = 0;
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_PreLoadMusicStreamDone( void )
{
	if( musicPreloadActive )
	{
		if( audio_player.preFetchState )
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
bool PCMAudio_StartPreLoadedMusicStream( void )
{
	// Maybe we should check here to make sure the checksum of the music info filestream matches that
	// passed in when the music stream preload request came in.
	if( sPreLoadMusicFilename[0] > 0 )
	{
		// Turn off the preload flag, which will allow decoding to start.
		musicPreloadActive = false;

		// Call the update loop immediately.
		PCMAudio_DecodeLoop();
		return true;
	}
	return false;	
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int PCMAudio_GetStreamStatus( int whichStream )
{
	if( whichStream >= NUM_STREAMS )
	{
		Dbg_MsgAssert( 0, ( "Checking stream status on stream %d, past valid range ( 0 to %d ).", whichStream, NUM_STREAMS - 1 ) );
		return false;
	}

	if( sPreLoadChecksum[whichStream] > 0 )
	{
		return PCM_STATUS_LOADING;
	}

	if( gPcmStatus & PCMSTATUS_LOAD_STREAM( whichStream ))
	{
		return PCM_STATUS_LOADING;
	}
	else if( gPcmStatus & PCMSTATUS_STREAM_PLAYING( whichStream ))
	{
		return PCM_STATUS_PLAYING;
	}
	else
	{
		return PCM_STATUS_FREE;
	}

	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PCMAudio_PauseStream( bool pause, int whichStream )
{
	// Not sure how to do this yet...
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_LoadStreamHeader( const char *nameOfFile )
{
	if( gpStreamHed )
	{
		Mem::Free( gpStreamHed );
		gpStreamHed = NULL;
	}

	gpStreamHed = File::LoadHed( nameOfFile );

	if( !gpStreamHed )
	{
		Dbg_Message( "Couldn't find stream header %s\n", nameOfFile );
		return false;
	}
	
	return true;
}







} // namespace Pcm
