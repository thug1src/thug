/*	Stream audio off the disc...
	Play music tracks or what not.
	
	md Jan 2001
*/

#ifndef __MUSIC_H__
#define __MUSIC_H__

#ifndef __GEL_OBJECT_H
#include <gel/object.h>
#endif

#include <gel/soundfx/soundfx.h>

// change the following line to compile out music or streams:
#define NO_MUSIC_PLEASE	0
#define DISABLE_STREAMS	0

namespace Obj
{
	class CStreamComponent;
}
			
namespace Pcm
{

// Forward declarations
class CStreamFrameAmp;
class CStreamFrameAmpManager;


#define NUM_CHANNELS				2

#define DEFAULT_MUSIC_VOLUME		50.0f
#define DEFAULT_MUSIC_STREAM_VOLUME	100.0f

#define MAX_NUM_TRACKS				80
#define MAX_TRACKNAME_STRING_LENGTH	40
#define TRACK_TITLE_MAX_SIZE		100

#define DEFAULT_STREAM_DROPOFF		DEFAULT_DROPOFF_DIST

#define STREAM_DEFAULT_PRIORITY		(50)		// Stream priority that is used when not set

enum EMusicStreamType
{
	MUSIC_STREAM_TYPE_NONE = 0,
	MUSIC_STREAM_TYPE_NORMAL,
	MUSIC_STREAM_TYPE_PRELOAD,
};

// Structures
struct CurrentStreamInfo
{
	uint32				controlID;		// ID used from script to control stream (default value is checksum of stream)
	uint32				uniqueID;		// ID used within code (same as controlID unless this stream is playing on more than 1 channel)
	float				volume;
	float				dropoff;
	int					voice;
	int					priority;
	uint32				start_frame;
	bool				use_pos_info;
	EDropoffFunc		dropoffFunction;
	CStreamFrameAmp *	p_frame_amp;
};

struct TrackInfo
{
	char trackName[ MAX_TRACKNAME_STRING_LENGTH ];
};

struct TrackTitle
{
	char trackTitle[ TRACK_TITLE_MAX_SIZE ];
};

struct TrackList
{
	// on PS2 version, the sceCdlFILE info is put into the end of this string:
	TrackInfo 		trackInfo[ MAX_NUM_TRACKS ];
	int				numTracks;
	// the following are bitflags:
	uint64			trackForbidden0;
    uint64			trackForbidden1;
	uint64			trackPlayed0;
    uint64			trackPlayed1;
	bool			allTracksForbidden;
};


void	Init( void );

bool NoMusicPlease();
bool StreamsDisabled();

int GetNumStreamsAvailable( void );
int GetCurrentTrack( void );
void SetCurrentTrack( int value );

// Call update every frame...
// this function checks once every second to see if the previous
// song finished playing... if so it starts a new song.
// Also handles Stop/Play commands.
void	Update( void );

void	PlayTrack( const char *filename, bool loop = false );
void	PlayMusicStream( uint32 checksum, float volume = -1 );		// Plays a stereo stream through the music channels

// These five only for XBox.
void UseUserSoundtrack( int soundtrack );
void UseStandardSoundtrack();
bool UsingUserSoundtrack();
void SaveSoundtrackToStructure( Script::CStruct *pStuff );
void GetSoundtrackFromStructure( Script::CStruct *pStuff );

uint32	PlayStream( uint32 checksum, Sfx::sVolume *p_volume, float pitch = 100, int priority = STREAM_DEFAULT_PRIORITY,
					uint32 controlID = 0, bool record_in_replay=true );
uint32	PlayStreamFromObject( Obj::CStreamComponent *pObject, uint32 streamNameChecksum, float dropoff = 0, float volume = 0,
							  float pitch = 100, int priority = STREAM_DEFAULT_PRIORITY, int use_pos_info = 0,
							  EDropoffFunc dropoffFunc = DROPOFF_FUNC_STANDARD, uint32 controlID = 0 );

// Control functions
bool	SetStreamVolumeFromID( uint32 streamID, Sfx::sVolume *p_volume );		// For non-object streams
bool	SetStreamPitchFromID( uint32 streamID, float pitch );					// For non-object streams
void	StopStreamFromID( uint32 streamID );
void	StopStreams( int channel = -1 );
bool	StreamAvailable( void );
bool	StreamAvailable( int whichStream );

bool	StreamExists( uint32 streamChecksum );

bool	StreamLoading( uint32 streamID );
bool	StreamPlaying( uint32 streamID );

// Preload streams.  By preloading, you can guarantee they will start at a certain time.
uint32	PreLoadStream( uint32 checksum, int priority = STREAM_DEFAULT_PRIORITY );
bool	PreLoadStreamDone( uint32 streamID );
bool	StartPreLoadedStream( uint32 streamID, Sfx::sVolume* p_volume, float pitch = 100 );

// Preload Music streams.  By preloading, you can guarantee they will start at a certain time.
bool	PreLoadMusicStream( uint32 checksum );
bool	PreLoadMusicStreamDone( void );
bool	StartPreLoadedMusicStream( float volume = -1 );

/*	Arg:	-1 to unpause only if it was unpaused last time pause was called
			1 to pause
			0 to unpause
*/
void	PauseMusic( int pause );
bool	MusicIsPaused();
void	PauseStream( int pause );

void	StopMusic( void );

// ambient tracks use the soundfx volume instead of music volume:
void	SetAmbientVolume( float volPercent );

void	SetVolume( float volume );
float	GetVolume( void );
void	SetMusicStreamVolume( float volume );
float	GetMusicStreamVolume( void );
void	GetPlaylist( uint64* flags1, uint64* flags2 );
void	SetPlaylist( uint64 flags1, uint64 flags2 );

enum{
		TRACKLIST_PERM,
		TRACKLIST_LEVEL_SPECIFIC,
		NUM_TRACKLISTS,
};

// Music track management:
void	AddTrackToPlaylist( const char *trackName, int whichList, const char *trackTitle = NULL );
void	ClearPlaylist( int whichList );

const char *GetTrackName( int trackNum, int whichList );
int		GetNumTracks( int whichList );

bool UsingCD( void );

// set a particular song as 'forbidden' (don't play it, the player hates that song...)
void	SetTrackForbiddenStatus( int trackNum, bool forbidden, int whichList );
//void	CheckLockedTracks( );
int		GetTrackForbiddenStatus( int trackNum, int whichList );
void	SkipMusicTrack( void );
void	SetRandomMode( int randomModeOn );
int		GetRandomMode( void );
void	SetLoopingMode( bool loop );

// Perm list? Or ambient list?
void	SetActiveTrackList( int whichList );
int		GetActiveTrackList( void );

bool LoadMusicHeader( const char *nameOfFile );
bool LoadStreamHeader( const char *nameOfFile );

///////////////////////////////////////////////////////////////////////////////////
// Pcm::CStreamFrameAmp

class CStreamFrameAmp
{
public:
	// Constants
	enum
	{
		vNOT_ALLOCATED			= 0,				// 0 indicates not allocated
		vMAX_SAMPLES			= 4096,
	};

								CStreamFrameAmp();
								~CStreamFrameAmp();

	void						Init();
	uint32						GetStreamNameCRC() const;

	uint8						GetSample(int frame) const;
	const uint8 *  				GetSamples() const;
	const uint8 *  				GetSamples(int frame) const;
	int							GetNumSamples() const;

protected:

	bool						load(uint32 stream_checksum);
	bool						free();
	bool						is_allocated() const;

	uint32						m_checksum;
	int							m_num_samples;
	uint8						m_frame_amp_samples[vMAX_SAMPLES] nAlign(128);	// Aligned data for the loading code

	// Friends
	friend CStreamFrameAmpManager;
};

///////////////////////////////////////////////////////////////////////////////////
// Pcm::CStreamFrameAmpManager
class CStreamFrameAmpManager
{
public:
	// Constants
	enum
	{
		vMAX_BUFFERS			= 1,
	};

	static CStreamFrameAmp *	sLoadFrameAmp(uint32 stream_checksum);
	static CStreamFrameAmp *	sGetFrameAmp(uint32 stream_checksum);
	static bool					sFreeFrameAmp(uint32 stream_checksum);
	static bool					sFreeFrameAmp(CStreamFrameAmp *p_fam_data);

protected:

	static CStreamFrameAmp		s_frame_amp_data[vMAX_BUFFERS];
};

///////////////////////////////////////////////////////////////////////////////////

inline uint32			CStreamFrameAmp::GetStreamNameCRC() const
{
	return m_checksum;
}

inline bool				CStreamFrameAmp::is_allocated() const
{
	return m_checksum != vNOT_ALLOCATED;
}


}  // namespace Pcm

#endif // __MUSIC_H__

