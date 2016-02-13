// Audio streaming function prototypes:
// mjd jan 2001

#ifndef __P_MUSIC_H__
#define __P_MUSIC_H__

// Needed so music.cpp will build. Grrr.
#define NUM_STREAMS 3

namespace Pcm
{

// allows one channel for music, another for audio:
enum{
	EXTRA_CHANNEL,
	MUSIC_CHANNEL,
};

// Onetime call once upon loading the game...
void PCMAudio_Init( void );

OVERLAPPED *PCMAudio_GetNextOverlapped( void );

// Call every frame to make sure music and stream buffers are loaded and current status is checked each frame...
int PCMAudio_Update( void );

// Load a track and start it playing...
// You wanna loop a track?  Wait until this track is done, and PLAY IT AGAIN SAM!
bool PCMAudio_PlayMusicTrack( const char *filename );
bool PCMAudio_PlayMusicTrack( uint32 checksum );
bool PCMAudio_PlaySoundtrackMusicTrack( int soundtrack, int track );
//bool PCMAudio_PlayStream( uint32 checksum, int whichStream, float volumeL, float volumeR, float pitch, bool preload = false );
bool PCMAudio_PlayStream( uint32 checksum, int whichStream, Sfx::sVolume *p_volume, float pitch, bool preload = false );
bool PCMAudio_StartStream( int whichStream );

// keep song loaded, stop playing it (or continue playing paused song)
void PCMAudio_Pause( bool pause = true, int ch = MUSIC_CHANNEL );

void PCMAudio_StopMusic( bool waitPlease );
void PCMAudio_StopStream( int whichStream, bool waitPlease = true );
void PCMAudio_StopStreams( void );

uint32	PCMAudio_GetFilestreamBufferSize( void );

// Preload streams. By preloading, you can guarantee they will start at a certain time.
bool PCMAudio_PreLoadStream( uint32 checksum, int whichStream );
bool PCMAudio_PreLoadStreamDone( int whichStream );
//bool PCMAudio_StartPreLoadedStream( int whichStream, float volumeL, float volumeR, float pitch );
bool PCMAudio_StartPreLoadedStream( int whichStream, Sfx::sVolume *p_volume, float pitch );

// Preload music streams. By preloading, you can guarantee they will start at a certain time.
bool PCMAudio_PreLoadMusicStream( uint32 checksum );
bool PCMAudio_PreLoadMusicStreamDone( void );
bool PCMAudio_StartPreLoadedMusicStream( void );

// set the music volume ( 0 to 100 )
int PCMAudio_SetMusicVolume( float volume );
bool	PCMAudio_SetStreamPitch( float pitch, int whichStream );
//int PCMAudio_SetStreamVolumeAndPitch( float volL, float volR, float pitch, int whichStream );

enum{
	PCM_STATUS_FREE 			= ( 1 << 0 ),
	PCM_STATUS_PLAYING			= ( 1 << 1 ),
	PCM_STATUS_LOADING 			= ( 1 << 2 ),
};

bool PCMAudio_SetStreamVolume( Sfx::sVolume *p_volume, int whichStream );

// Return one of the PCM_STATUS values from above...
// Return one of the PCM_STATUS values from above...
int PCMAudio_GetMusicStatus( );
int PCMAudio_GetStreamStatus( int whichStream );

bool PCMAudio_TrackExists( const char *pTrackName, int ch );
bool PCMAudio_LoadMusicHeader( const char *nameOfFile );
bool PCMAudio_LoadStreamHeader( const char *nameOfFile );

uint32 PCMAudio_FindNameFromChecksum( uint32 checksum, int ch );
	 
} // namespace PCM

#endif // __P_MUSIC_H__
