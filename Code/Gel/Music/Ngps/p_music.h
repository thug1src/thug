// Audio streaming function prototypes:
// mjd jan 2001

#ifndef __P_MUSIC_H__
#define __P_MUSIC_H__

// Forward declarations
namespace File
{
	class CAsyncFileHandle;
	struct SHed;
	struct SHedFile;
}

namespace Pcm
{

// Stream types
enum EFileStreamType 
{
	FILE_STREAM_MUSIC = 0,
	FILE_STREAM_STREAM0,
	FILE_STREAM_STREAM1,
	FILE_STREAM_STREAM2,
	NUM_FILE_STREAMS
};

// The load state of a stream
enum ELoadState
{
	LOAD_STATE_IDLE = 0,
	LOAD_STATE_LOADING0,
	LOAD_STATE_LOADING1,
	LOAD_STATE_DONE,
};

// The play state of a stream
enum EPlayState
{
	PLAY_STATE_STOP = 0,
	PLAY_STATE_PRELOAD_EE,
	PLAY_STATE_PRELOAD_IOP,
	PLAY_STATE_START,
	PLAY_STATE_PLAYING,
	PLAY_STATE_PAUSED,
	PLAY_STATE_DONE,
};

// allows one channel for music, another for audio:
enum{
	EXTRA_CHANNEL,
	MUSIC_CHANNEL,
};


// Holds the info for each stream
class CFileStreamInfo
{
public:

	bool					IsMusicStream() const;
	int						GetAudioStreamIndex() const;		// returns a negative number if it isn't a audio stream

	bool					StartStream(File::SHedFile *pHedFile, bool play);
	bool					PlayStream();
	bool					StopStream();
	int						Update();

protected:
	void					start_audio();
	void					load_chunk(int buffer);

	// For preloading
	void					preload_spu_buffers();
	void					start_preloaded_audio();

	ELoadState				check_for_load_requests();
	bool					is_chunk_load_done(int & errno);	// errno will be non-zero if we were busy AND there was an error
	bool					file_finished();					// returns true and closes fine if we read the whole file

// This section should eventually become "protected"
public:
	EFileStreamType			m_streamType;			// Either music or audio stream
	ELoadState				m_loadState;
	EPlayState				m_playState;
	File::CAsyncFileHandle *mp_fileHandle;
	int						m_fileSize;
	int						m_offset;
	int						m_offsetInWad;
	int						m_pitch;
	uint32					m_volume;
};

// All the Wad information for music and audio
struct SWadInfo
{
	bool					LoadHeader(const char *nameOfFile, bool no_wad = false, bool assertOnError = false);

	char  					m_fileName[ 256 ];
	int						m_lsn;
	File::SHed *			mp_hed;
};



// Onetime call once upon loading the game...
void PCMAudio_Init( void );

// Combine the IOP sent status into gPcmStatus
void PCMAudio_UpdateStatus();

// Call every frame to make sure music and stream buffers are loaded and current status is checked each frame...
// Returns true if there is a CD error, false otherwise...
int PCMAudio_Update( void );

// Load a track and start it playing...
// You wanna loop a track?  Wait until this track is done, and PLAY IT AGAIN SAM!
bool PCMAudio_PlayMusicTrack( const char *filename );
bool PCMAudio_PlayMusicStream( uint32 checksum );
bool PCMAudio_PlayStream( uint32 checksum, int whichStream, float volumeL, float volumeR, float pitch );

// keep song loaded, stop playing it (or continue playing paused song)
void PCMAudio_Pause( bool pause = true, int ch = MUSIC_CHANNEL );

void PCMAudio_PauseStream( bool pause, int whichStream );

void PCMAudio_StopMusic( bool waitPlease );
void PCMAudio_StopStream( int whichStream, bool waitPlease = true );
void PCMAudio_StopStreams( void );

// Preload streams.  By preloading, you can guarantee they will start at a certain time.
bool PCMAudio_PreLoadStream( uint32 checksum, int whichStream );
bool PCMAudio_PreLoadStreamDone( int whichStream );
bool PCMAudio_StartPreLoadedStream( int whichStream, float volumeL, float volumeR, float pitch );

// Preload Music Streams.  By preloading, you can guarantee they will start at a certain time.
bool PCMAudio_PreLoadMusicStream( uint32 checksum );
bool PCMAudio_PreLoadMusicStreamDone( void );
bool PCMAudio_StartPreLoadedMusicStream( void );

// set the music volume ( 0 to 100 )
bool	PCMAudio_SetMusicVolume( float volume );
bool	PCMAudio_SetStreamVolume( float volumeL, float volumeR, int whichStream );
bool	PCMAudio_SetStreamPitch( float pitch, int whichStream );
//int PCMAudio_SetStreamVolumeAndPitch( float volumeL, float volumeR, float pitch, int whichStream );

void PCMAudio_SetStreamGlobalVolume( unsigned int volume );

enum{
	PCM_STATUS_FREE 			= ( 1 << 0 ),
	PCM_STATUS_PLAYING			= ( 1 << 1 ),
	PCM_STATUS_LOADING 			= ( 1 << 2 ),
	PCM_STATUS_END 				= ( 1 << 3 ),		// Done playing on IOP, but not completely cleaned up on the IOP
	PCM_STATUS_STOPPED 			= ( 1 << 4 ),
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
		 
//////////////////////////////////////////////////////////////////////////////
// Inlines
//

inline bool		CFileStreamInfo::IsMusicStream() const
{
	return m_streamType == FILE_STREAM_MUSIC;
}

inline int		CFileStreamInfo::GetAudioStreamIndex() const
{
	return (int) (m_streamType - FILE_STREAM_STREAM0);
}

} // namespace PCM

#endif // __P_MUSIC_H__
