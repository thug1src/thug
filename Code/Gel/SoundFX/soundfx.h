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
**	Module:			soundfx													**
**																			**
**	File name:		gel/soundfx/soundfx.h  									**
**																			**
**	Created: 		1/4/01	-	mjd											**
**																			**
*****************************************************************************/

#ifndef __GEL_SOUNDFX_H
#define __GEL_SOUNDFX_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/list.h>
#include <core/macros.h>
#include <core/math.h>
#include <sys/timer.h>

#ifdef __PLAT_NGPS__
#include <gel/soundfx/ngps/p_sfx.h>
#elif defined( __PLAT_XBOX__ )
#include <gel/soundfx/xbox/p_sfx.h>
#elif defined( __PLAT_NGC__ )
#include <gel/soundfx/ngc/p_sfx.h>
#endif

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

// change the following line to compile out soundfx from the game:
#ifdef __PLAT_XBOX__
#define NO_SOUND_PLEASE	0
#else
#define NO_SOUND_PLEASE	0
#endif

// if we need more, NO PROBLEMA!  Change this:
#if defined( __PLAT_NGPS__ )
#define WAVE_TABLE_MAX_ENTRIES			256	// non-permanent sounds
#define PERM_WAVE_TABLE_MAX_ENTRIES		256	// permanent sounds
#define MAX_POSITIONAL_SOUNDS			128
#elif defined( __PLAT_XBOX__ )
#define WAVE_TABLE_MAX_ENTRIES			256	// non-permanent sounds
#define PERM_WAVE_TABLE_MAX_ENTRIES		256	// permanent sounds
#define MAX_POSITIONAL_SOUNDS			128
#else
#define WAVE_TABLE_MAX_ENTRIES			256	// non-permanent sounds
#define PERM_WAVE_TABLE_MAX_ENTRIES		256	// permanent sounds
#define MAX_POSITIONAL_SOUNDS			128
#endif

#define DIST_FROM_DROPOFF_AT_WHICH_TO_START_SOUND	FEET_TO_INCHES( 20.0f )
#define DFDAWTSS									DIST_FROM_DROPOFF_AT_WHICH_TO_START_SOUND

// This buffer prevents rapid switching on/off of voices that are right on the border.
#define SOUND_DIST_BUFFER							FEET_TO_INCHES( 20.0f )
#define DIST_FROM_DROPOFF_AT_WHICH_TO_STOP_SOUND	( SOUND_DIST_BUFFER + DFDAWTSS )

#define DEFAULT_DROPOFF_DIST						FEET_TO_INCHES( 85.0f )

// When a voice is set to negative volume on some platforms, it puts the sound 'out of phase' to sound like it's behind you...
#if defined( __PLAT_NGPS__ ) || defined( __PLAT_XBOX__ )
#define PLATFORM_SUPPORTS_VOLUME_PHASING			1
#endif

#define MAX_VOL_ALLOWED								1000 // 1000 percent (10 times)

namespace Script
{
	class CStruct;
}

namespace Gfx
{
	class Camera;
}

namespace Obj
{
    class CSoundComponent;
    class CEmitterObject;
}

enum
{
	SFX_FLAG_LOAD_PERM						= ( 1 << 0 ),
//	SFX_FLAG_POSITIONAL_UPDATE				= ( 1 << 1 ),
	SFX_FLAG_POSITIONAL_UPDATE_WITH_DOPPLER = ( 1 << 2 ),
	SFX_FLAG_NO_REVERB						= ( 1 << 4 ),
};


// Function used to calculate volume dropoff
enum EDropoffFunc
{
	DROPOFF_FUNC_STANDARD,				// Combination of linear and exponential
	DROPOFF_FUNC_LINEAR,
	DROPOFF_FUNC_EXPONENTIAL,
	DROPOFF_FUNC_INV_EXPONENTIAL,
};


struct ObjectSoundInfo
{
	// Needed for long sounds played on a per-object basis and positionally updated:
	uint32	checksum;
	float	dropoffDist;
	float	currentPitch;
	float	currentVolume;
	float	origPitch;
	float	origVolume;
	float	targetPitch;
	float	targetVolume;
	float	deltaPitch;
	float	deltaVolume;
	EDropoffFunc dropoffFunction;

	// Used to check to see if the sound should be turned off or on, if too far/close to the camera:
	Tmr::Time timeForNextDistCheck;
	
	// Used by the script debugger code to fill in a structure
	// for transmitting to the monitor.exe utility running on the PC.
	void GetDebugInfo(Script::CStruct *p_info);
};


#ifdef __PLAT_WN32__
struct PlatformWaveInfo
{
	// Stub to compile under win32
};
#endif

namespace Sfx
{

struct WaveTableEntry
{
	uint32		checksum;	// name of the sound...
	uint32		flags;		// see SFX_FLAG_ 's in soundfx.h

	// All the shit designers want to be able to set in a script file:
	float		pitch;		// tweakable by designers in script file
	float		volume;		// tweakable by designers in script file
	float		dropoff;	// tweakable by designers in script file

	// Platform dependent info:
	PlatformWaveInfo platformWaveInfo;
};


struct VoiceInfo
{
	uint32				uniqueID;
	uint32				controlID;
	WaveTableEntry		*waveIndex;
	ObjectSoundInfo		info;
};


enum EVolumeType
{
	VOLUME_TYPE_BASIC_2_CHANNEL,		// Basic 2 channel left/right sound with no volume phasing.
	VOLUME_TYPE_2_CHANNEL_DOLBYII,		// As above, but with DolbyII volume phasing.
	VOLUME_TYPE_5_CHANNEL_DOLBY5_1		// Five discreet volumes, as per Dolby 5.1.
};



/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/

struct sVolume
{
	enum
	{
		MAX_CHANNELS = 5
	};
	float				m_channels[MAX_CHANNELS];	// Can store up to five discreet channel volumes.
	EVolumeType			m_volume_type;				// The type of the volume.

						sVolume( void );
						sVolume( EVolumeType type );
						sVolume( const sVolume& rhs );
						~sVolume( void );
	bool				operator== ( const sVolume& rhs );
	bool				operator!= ( const sVolume& rhs )			{ return !(*this == rhs); }
	bool				IsSilent( void );
	void				SetSilent( void );
	EVolumeType			GetVolumeType( void )						{ return m_volume_type; }
	float				GetChannelVolume( uint32 chan )				{ return m_channels[chan]; }
	void				SetChannelVolume( uint32 chan, float vol )	{ m_channels[chan] = vol; }
	void				PercentageAdjustment( float percentage );
	float				GetLoudestChannel( void );
};

struct PositionalSoundEntry;

struct SoundUpdateInfo
{
	// So each instance of Obj_PlaySound maintains the values sent in by the designer:
	float pitch; 
	float volume;
	float dropoffDist;
	EDropoffFunc dropoffFunction;
};


class CSfxManager : public Spt::Class
{
	DeclareSingletonClass( CSfxManager );

public:
					CSfxManager( void );
				    ~CSfxManager( void );

	int				MemAvailable( void );

	bool			LoadSound( const char *sfxName,  int flags = 0, float dropoff = 0.0f, float pitch = 100.0f, float volume = 100.0f );
	uint32			PlaySound( uint32 checksum, sVolume *p_vol, float pitch = 100.0f, uint32 controlID = 0, SoundUpdateInfo *pUpdateInfo = NULL, const char *pSoundName = NULL );
	uint32			PlaySoundWithPos( uint32 soundChecksum, SoundUpdateInfo *pUpdateInfo, Obj::CSoundComponent *pObj, bool noPosUpdate );
	ObjectSoundInfo	*GetObjectSoundProperties( Obj::CSoundComponent *pObj, uint32 checksum );

	void			CleanUp( void );
	void			StopAllSounds( void );
	bool			StopSound( uint32 uniqueID );
	bool			SetSoundParams( uint32 uniqueID, sVolume *p_vol, float pitch = 100.0f );	// For non-object sounds
	void			StopObjectSound( Obj::CSoundComponent *pObj, uint32 checksum );
	void			PauseSounds( void );
	void			SetReverb( float reverbLevel, int reverbMode = 0, bool instant = false );
	bool			SoundIsPlaying( uint32 uniqueID, int *pWhichVoice = NULL );
	int				GetNumSoundsPlaying();
	void			SetMainVolume( float volume );
	void			SetDefaultDropoffDist( float dist );
	void			SetVolumeFromPos( sVolume *p_vol, const Mth::Vector &soundSource, float dropoffDist,
									  EDropoffFunc dropoff_func = DROPOFF_FUNC_STANDARD,
									  Gfx::Camera* pCamera = NULL, const Mth::Vector *p_dropoff_pos = NULL );
	void			AdjustPitchForDoppler( float *pitch, const Mth::Vector &currentPos, const Mth::Vector &oldPos, float elapsedTime, Gfx::Camera* pCam = NULL );
	void			Get5ChannelMultipliers( const Mth::Vector &sound_source, float *p_multipliers );
	void			Get5ChannelMultipliers( float angle, float *p_multipliers );

	// A per-frame function. Does nothing on PS2, on Xbox checks for stop notification events.
	void			Update( void );
	void			UpdatePositionalSounds( void );
	
	// If, for some reason, the sound has stopped playing, this returns false, true otherwise.
	bool			UpdateLoopingSound( uint32 soundID, sVolume *p_vol, float pitch = 100.0f );

	float			GetMainVolume( void );
	float			GetDropoffDist( uint32 soundChecksum );

	// -1 on failure (sound not loaded)
	WaveTableEntry*	GetWaveTableIndex( uint32 checksum );

	void			ObjectBeingRemoved( Obj::CSoundComponent *pObj );
		
	void			SetDefaultVolumeType( EVolumeType type )	{ m_default_volume_type = type; }
	EVolumeType		GetDefaultVolumeType( void )				{ return m_default_volume_type; }

	bool			PositionalSoundSilenceMode();		// Returns true if in mode where environment sounds should be a 0 volume
	
private:

	EVolumeType		m_default_volume_type;

	bool			AdjustObjectSound( Obj::CSoundComponent *pObj, VoiceInfo *pVoiceInfo, Tmr::Time gameTime );
	void			TweakVolumeAndPitch( sVolume* p_vol, float *pitch, WaveTableEntry* waveTableIndex );
	void			RemovePositionalSoundFromList( PositionalSoundEntry *pEntry, bool stopIfPlaying );
	void			AddPositionalSoundToUpdateList( uint32 uniqueID, uint32 soundChecksum, Obj::CSoundComponent *pObj = NULL );

	// UniqueID functions
	bool			IDAvailable(uint32 id);
	int				GetVoiceFromID(uint32 id);
	uint32			GenerateUniqueID(uint32 id);
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

extern int				NumWavesInTable;
extern int				NumWavesInPermTable;
extern int				NumPositionalSounds;
extern WaveTableEntry	WaveTable[PERM_WAVE_TABLE_MAX_ENTRIES + WAVE_TABLE_MAX_ENTRIES];


/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/
bool			NoSoundPlease();
EDropoffFunc	GetDropoffFunctionFromChecksum(uint32 checksum);

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace Sfx

#endif	// __GEL_SOUNDFX_H


