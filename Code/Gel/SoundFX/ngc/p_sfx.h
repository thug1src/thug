/*	Playstation 2 sound support functions.
	mjd	jan 10, 2001
*/
#ifndef __MODULES_P_SFX_H
#define __MODULES_P_SFX_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

// Need to include this to get NUM_STREAMS.
#include <gel/music/ngc/pcm/pcm.h>

namespace Sfx
{
		  
#define NUM_MUSIC_VOICES		2
#define NUM_VOICES				( 64 - NUM_STREAMS - NUM_MUSIC_VOICES ) // leave some voices per core for streaming audio off the CD and shit...

struct sVolume;

// platform specific info needed per wave (stored in the WaveTable):
struct PlatformWaveInfo
{
	int		vagAddress;		// PS2 waves played by setting a voice to the address of the wave on the sound chip...	
	void*	headerAddress;	// Main memory location of header information.
	bool	looping;
	float	pitchAdjustmentForSamplingRate; // figure out how much to alter the pitch due to lower sampling rates than 48000.
};

void	GetStreamBufferInfo( int* stream0_base, int* stream1_base, int* stream2_base, int* stream0_size, int* stream1_size, int* stream2_size );
class	CSfxManager;

// on startup of the game only...
void PS2Sfx_InitCold( void );

void	AXUserCBack( void );


void	InitSoundFX( CSfxManager *p_sfx_manager );
void	ReInitSoundFX( void );
void	CleanUpSoundFX( void );
void	StopAllSoundFX( void );
bool	LoadSoundPlease( const char *sfxName, uint32 checksum, PlatformWaveInfo *pInfo, bool loadPerm = 0 );

int		GetMemAvailable( void );

// returns 0 - ( NUM_VOICES - 1 ), or -1 if no voices are free...
//int		PlaySoundPlease( PlatformWaveInfo *pInfo, float volL = 100.0f, float volR = 100.0f, float pitch = 100.0f );
int		PlaySoundPlease( PlatformWaveInfo *pInfo, sVolume *p_vol, float pitch = 100.0f );

void	StopSoundPlease( int whichVoice );

extern bool	isStereo;

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
//void	SetVoiceParameters( int whichVoice, float volL, float volR, float pitch = 0.0f );
void	SetVoiceParameters( int whichVoice, sVolume *p_vol, float pitch = 0.0f );

void	PerFrameUpdate( void );


} // namespace Gel

#endif
