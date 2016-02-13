/*	Playstation 2 sound support functions.
	mjd	jan 10, 2001
*/
#ifndef __MODULES_P_SFX_H
#define __MODULES_P_SFX_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <gel/music/ngps/pcm/pcm.h>

namespace Sfx
{

struct sVolume;
class CSfxManager;

		  
#if NUM_STREAMS > 2
#define NUM_VOICES_PER_CORE		(24 - NUM_STREAMS)  // leave #stream voices per core for streaming audio off the CD and shit...
#else
#define NUM_VOICES_PER_CORE		22  // leave 2 voices per core for streaming audio off the CD and shit...
#endif

#define NUM_CORES				2
#define NUM_VOICES				( NUM_VOICES_PER_CORE * NUM_CORES )

// Non-reverb voices.  Make sure they are always the last voices on the last core, since we presently allow
// reverb sounds to use all voices.
#define NUM_NO_REVERB_VOICES		10
#define NUM_NO_REVERB_CORE			MUSIC_CORE
#define NUM_NO_REVERB_START_VOICE	( NUM_VOICES_PER_CORE - NUM_NO_REVERB_VOICES )

// platform specific info needed per wave (stored in the WaveTable):
struct PlatformWaveInfo{
	int		vagAddress; //PS2 waves played by setting a voice to the address of the wave on the sound chip...	
	bool	looping;
	float	pitchAdjustmentForSamplingRate; // figure out how much to alter the pitch due to lower sampling rates than 48000.
};

// on startup of the game only...
void PS2Sfx_InitCold( void );

void	InitSoundFX( CSfxManager *p_sfx_manager );
void	CleanUpSoundFX( void );
void	StopAllSoundFX( void );
bool	LoadSoundPlease( const char *sfxName, uint32 checksum, PlatformWaveInfo *pInfo, bool loadPerm = 0 );

int		GetMemAvailable( void );

// returns 0 - ( NUM_VOICES - 1 ), or -1 if no voices are free...
int		PlaySoundPlease( PlatformWaveInfo *pInfo, sVolume *p_vol, float pitch = 100.0f, bool no_reverb = false );

void	StopSoundPlease( int whichVoice );

// really just turning down the volume on all playing voices...
// Looping sounds will get the volume adjusted by the owner
// during the next logic loop.
// The non-looping sounds will play (with no volume) then stop.
// This is for when we go into the menus or whatnot...
void	PauseSoundsPlease( void );

// see reverb types in ReverbModes table (p_sfx.cpp), corresponding to reverb.q script file
void	SetReverbPlease( float reverbLevel = 0.0f, int reverbMode = 0, bool instant = false );

// volume from -100 to 100 ( negative if the sound is from behind us. ps2 alters the sound... )
void	SetVolumePlease( float volumeLevel );

// returns true if voice is being used currently.
bool	VoiceIsOn( int whichVoice );

// change parameters on an active voice:
// if pitch parameter is zero, pitch won't be changed (lowest pitch is 1)
void	SetVoiceParameters( int whichVoice, sVolume *p_vol, float pitch = 0.0f );

void	PerFrameUpdate( void );


///////////////////////////////////////////////////////////
// SPU Manager class
// All the SPU communication should go through here instead of sceSdRemote().
// It is asynchronous, so it doesn't stall the EE while waiting for the
// IOP (which can be milliseconds!)
class CSpuManager
{
public:
	static void				sInit();
	static void				sUpdateStatus();				// Called at the beginning of every frame to get the voice status

	static uint32			sGetVoiceStatus(int core);		// Gets the already requested status
	static void				sFlush();
	static volatile bool	sIsBusy();
	static void				sWaitForIO();

	static void				sClean();

#ifdef	__NOPT_ASSERT__
	static void				sPrintStatus();
#endif

	// Transfer functions
	static bool				sSendSdRemote(uint16 function, uint16 entry, uint32 value);
	static bool				sVoiceTrans(uint16 channel, uint16 mode, void *p_iop_addr, uint32 *p_spu_addr, uint32 size);

private:
	// Constants
	enum
	{
		MAX_QUEUED_COMMANDS	= 28,
	};

	static void				s_result_callback(void *p, void *q);	// SifCmd callback

	static bool				s_queue_new_command(uint16 function, uint16 entry, uint32 value, uint32 arg0 = 0, uint32 arg1 = 0, uint32 arg2 = 0);
	static uint32			s_get_switch_value(uint16 entry, uint32 value);
	static void				s_update_local_status(uint16 entry, uint32 value);

	static void				s_wait_for_voice_transfer();

	// Variables
	static volatile bool	s_sd_remote_busy;
	static volatile bool	s_sd_transferring_voice;
	static volatile uint32	s_sd_voice_status[NUM_CORES];
	static volatile uint32	s_sd_voice_status_update_frame[NUM_CORES];

	// Switch settings made in frame.  Because the SPU only reads registers every 1/48000 of a sec, we may have
	// to re-send a switch setting so we don't overwrite an old one.
	static uint32			s_spu_reg_kon[NUM_CORES];
	static uint32			s_spu_reg_koff[NUM_CORES];

	// Store multiple request packets, so we don't have to wait for the last DMA to finish before
	// starting a new one.
	static SSifCmdSoundRequestPacket	s_sd_remote_commands[MAX_QUEUED_COMMANDS] __attribute__ ((aligned(16)));
	static volatile int					s_sd_remote_command_free_index;
};


} // namespace Gel

#endif
