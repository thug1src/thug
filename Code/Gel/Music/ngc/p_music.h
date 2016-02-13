// Audio streaming function prototypes:
// mjd jan 2001

#ifndef __P_MUSIC_H__
#define __P_MUSIC_H__

#include <dolphin.h>
#include <gel/music/music.h>
#include <dolphin/mix.h>

namespace Pcm
{

// allows one channel for music, another for audio:
enum{
	EXTRA_CHANNEL,
	MUSIC_CHANNEL,
};


// Onetime call once upon loading the game...
void PCMAudio_Init( void );

// Call every frame to make sure music and stream buffers are loaded and current status is checked each frame...
int PCMAudio_Update( void );

// Load a track and start it playing...
// You wanna loop a track?  Wait until this track is done, and PLAY IT AGAIN SAM!
bool PCMAudio_PlayMusicTrack( const char *filename, bool preload = false );
bool PCMAudio_PlayStream( uint32 checksum, int whichStream, float volumeL, float volumeR, float pitch, bool preload = false );

// keep song loaded, stop playing it (or continue playing paused song)
void PCMAudio_Pause( bool pause = true, int ch = MUSIC_CHANNEL );

void PCMAudio_PauseStream( bool pause, int whichStream );

void PCMAudio_StopMusic( bool waitPlease );
void PCMAudio_StopStream( int whichStream, bool waitPlease = true );
void PCMAudio_StopStreams( void );

// Preload streams. By preloading, you can guarantee they will start at a certain time.
bool PCMAudio_PreLoadStream( uint32 checksum, int whichStream );
bool PCMAudio_PreLoadStreamDone( int whichStream );
bool PCMAudio_StartPreLoadedStream( int whichStream, float volumeL, float volumeR, float pitch );

// Preload music streams. By preloading, you can guarantee they will start at a certain time.
bool PCMAudio_PreLoadMusicStream( uint32 checksum );
bool PCMAudio_PreLoadMusicStreamDone( void );
bool PCMAudio_StartPreLoadedMusicStream( void );

// set the music volume ( 0 to 100 )
int		PCMAudio_SetMusicVolume( float volume );
bool	PCMAudio_SetStreamVolume( float volumeL, float volumeR, int whichStream );
bool	PCMAudio_SetStreamPitch( float pitch, int whichStream );
//int PCMAudio_SetStreamVolumeAndPitch( float volumeL, float volumeR, float pitch, int whichStream );

void PCMAudio_SetStreamGlobalVolume( unsigned int volume );

enum{
	PCM_STATUS_FREE 			= ( 1 << 0 ),
	PCM_STATUS_PLAYING			= ( 1 << 1 ),
	PCM_STATUS_LOADING 			= ( 1 << 2 ),
};

// Return one of the PCM_STATUS values from above...
int PCMAudio_GetMusicStatus( );
int PCMAudio_GetStreamStatus( int whichStream );

// preload any CD location info (to avoid a hitch in framerate on PS2):
unsigned int PCMAudio_GetCDLocation( const char *pTrackName );

bool PCMAudio_TrackExists( const char *pTrackName, int ch );
bool PCMAudio_LoadMusicHeader( const char *nameOfFile );
bool PCMAudio_LoadStreamHeader( const char *nameOfFile );

uint32 PCMAudio_FindNameFromChecksum( uint32 checksum, int ch );

// borrow this memory for the movies and shit...
int PCMAudio_GetIopMemory( void );
		 
void setDolby( AXPB * p_pb, AXVPB * p_vpb, float volL, float volR, unsigned int pitch = 0, bool set_pitch = false, float volume_percent = 100.0f );

} // namespace PCM

#endif // __P_MUSIC_H__
