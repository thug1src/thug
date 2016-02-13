/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GEL (Game Engine Library)								**
**																			**
**	Module:			Sound effects  (Sfx)									**
**																			**
**	File name:		gel/													**
**																			**
**	Created: 		01/10/2001	-	dc										**
**																			**
*****************************************************************************/

#ifndef __MODULES_P_SFX_H
#define __MODULES_P_SFX_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
 
/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Sfx
{

#define NUM_VOICES_PER_CORE				24
#define NUM_CORES						2
#define NUM_VOICES						( NUM_VOICES_PER_CORE * NUM_CORES )

#define WAVE_TABLE_MAX_ENTRIES			256	// non-permanent sounds
#define PERM_WAVE_TABLE_MAX_ENTRIES		256	// permanent sounds
#define MAX_POSITIONAL_SOUNDS			128

// (From Xbox docs) Even though DirectSound allows you to set the minimum volume to -100 dB,
// the XBox audio hardware clamps to silence at -64 dB. For efficiency, your game should avoid
// making volume calls and calculations where the volume is below -64 dB, particularly when
// keeping 3D sounds on that are intended to sound distant or far away.
#define DSBVOLUME_EFFECTIVE_MIN	-6400


/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class CXBSound;
class CSfxManager;

// Platform specific info needed per wave (stored in the WaveTable).
struct PlatformWaveInfo
{
	CXBSound*	p_sound_data;
	bool		looping;		// this flag is checked in soundfx.cpp.  might oughtta be added to WaveTableEntry struct...
	bool		permanent;
};

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

extern float	gSfxVolume;
struct sVolume;

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

void	InitSoundFX( CSfxManager *p_sfx_manager );
void	CleanUpSoundFX( void );
void	StopAllSoundFX( void );
bool	LoadSoundPlease( const char *sfxName, uint32 checksum, PlatformWaveInfo *pInfo, bool loadPerm = 0 );

// returns 0 - ( NUM_VOICES - 1 ), or -1 if no voices are free...
//int		PlaySoundPlease( PlatformWaveInfo *pInfo, float volL = 100.0f, float volR = 100.0f, float pitch = 100.0f );
int		PlaySoundPlease( PlatformWaveInfo *pInfo, sVolume *p_vol, float pitch = 100.0f );

void	StopSoundPlease( int whichVoice );

int		GetMemAvailable( void );

// really just turning down the volume on all playing voices...
// Looping sounds will get the volume adjusted by the owner
// during the next logic loop.
// The non-looping sounds will play (with no volume) then stop.
// This is for when we go into the menus or whatnot...
void	PauseSoundsPlease( void );

// see reverb types above...
void	SetReverbPlease( float reverbLevel = 0.0f, int reverbMode = 0, bool instant = false );

// volume from -100 to 100 ( negative if the sound is from behind us. ps2 alters the sound... )
void	SetVolumePlease( float volumeLevel );

// returns true if voice is being used currently.
bool	VoiceIsOn( int whichVoice );

// change parameters on an active voice:
// if pitch parameter is zero, pitch won't be changed (lowest pitch is 1)
void	SetVoiceParameters( int whichVoice, sVolume *p_vol, float pitch = 0.0 );

void	PerFrameUpdate( void );


/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Sfx

#endif	// __MODULES_P_SFX_H
