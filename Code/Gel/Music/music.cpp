/*  
	Music.cpp -- highest level
	p_music.cpp -- platform specific
	pcm*.c --  the code that runs on the IOP.
	
	This module should actually be renamed now, as it isn't just for
	music any more!

	This module should be called Streaming.cpp.
	
	This module handles streaming music, and soundfx.
	
	The stream data is read into IOP memory from the DVD, into a
	double buffer in IOP memory, about once every three seconds.
	
	The IOP chip has a thread that is running on a timer interrupt (see the pcm
	code, which compiles as an IOP module EzPcm.irx).  60 times per second,
	this thread updates small areas of memory on the SPU2 chip -- double
	buffers for each stream that need updating approximately every second
	and a half (or longer if the pitch has been turned down lower).

	These voices work just like the other voices for soundfx, except that they
	loop over a small area that is updated with the sound data streaming off
	the DVD.  You can change the pitch and volume of these voices on the fly
	just like soundFX, so streaming sounds can be adjusted positionally over time.

	Oh, yeah, just one more thing:  yo mamma, yo pappa, yo greasy bald gradma!
*/

#include <core/defines.h>
#include <core/macros.h>
#include <core/singleton.h>
#include <core/math.h>

#include <sys/timer.h>
#include <sys/config/config.h>

#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>

#include <gel/soundfx/soundfx.h>
#include <gel/music/music.h>
#include <gel/components/streamcomponent.h>

#include <sk/modules/skate/skate.h>
#include <sk/modules/skate/gamemode.h>

#include <gfx/debuggfx.h>

#ifdef __PLAT_NGPS__
#include <gel/music/ngps/p_music.h>
#include <gel/music/ngps/pcm/pcm.h>
#elif defined( __PLAT_XBOX__ )
#include <gel/music/xbox/p_music.h>
#include <gel/music/xbox/p_soundtrack.h>
#elif defined( __PLAT_NGC__ )
#include <gel/music/ngc/p_music.h>
#include <gel/music/ngc/pcm/pcm.h>
#endif
#include <sys/config/config.h>
#include <sys/replay/replay.h>
#include <sys/file/asyncfilesys.h>

#define TEST_FROM_CD 0
#define WAIT_AFTER_STOP_STREAM		0		// Set to 1 if we need to wait for stream to clear after stopping a stream

namespace Pcm
{

static bool music_hed_there = true;		// assume music and streams are there to start with, so we can load the hed
static bool streams_hed_there = true;	// if hed is not there, then these get set to false
										// which will disable all music and/or streams.

bool NoMusicPlease()
{
	#if NO_MUSIC_PLEASE
	return true;
	#else
	// Cannot have music if building for proview, because there is not enough IOP memory
	// (ProView uses up some IOP memory with it's monitor)
	if (Config::GetHardware()==Config::HARDWARE_PS2_PROVIEW)
	{
		return true;
	}
	
	return !music_hed_there;	
	#endif
}

bool StreamsDisabled()
{
	#if DISABLE_STREAMS
	return true;
	#else
	// Cannot have music if building for proview, because there is not enough IOP memory
	// (ProView uses up some IOP memory with it's monitor)
	if (Config::GetHardware()==Config::HARDWARE_PS2_PROVIEW)
	{
		return true;
	}
	
	return !streams_hed_there;	
	#endif
}

// prototypes...
bool TrackIsPlaying( int whichList, int whichTrack );

static int gLastStreamChannelPlayed = 0;

CurrentStreamInfo gCurrentStreamInfo[ NUM_STREAMS ];

TrackTitle PermTrackTitle[ MAX_NUM_TRACKS ];

// Static globals
static int          current_music_track = 0;
static TrackList	gTrackLists[ NUM_TRACKLISTS ];
static int			gCurrentTrackList = TRACKLIST_PERM;  // start out playing songs...
static char			gTrackName[ MAX_TRACKNAME_STRING_LENGTH ];
static float		gMusicVolume = DEFAULT_MUSIC_VOLUME;
static float		gMusicStreamVolume = DEFAULT_MUSIC_STREAM_VOLUME;
static bool 		gMusicInRandomMode = true;
static bool			gMusicLooping = false;
static int			sCounter = 1;
static bool			gPcmInitialized = false;
static int			gNumStreams = 0;
static Obj::CStreamComponent	*gpStreamingObj[ NUM_STREAMS ]; // should have as many of these as streams...

// Music stream data

static EMusicStreamType	gMusicStreamType = MUSIC_STREAM_TYPE_NONE;
#if WAIT_AFTER_STOP_STREAM
static uint32 			gMusicStreamChecksum;	// Checksum of music stream
static float 			gMusicStreamVolume;		// Volume of music stream
static bool 			gMusicStreamWaitingToStart;	// true if we are still waiting for the channel to become free
#endif // WAIT_AFTER_STOP_STREAM

// Limit is actually 500 ...
#define			MAX_USER_SONGS					600

#ifdef __PLAT_XBOX__
static bool		s_xbox_play_user_soundtracks	= false;
static int		s_xbox_user_soundtrack			= 0;
static uint32	s_xbox_user_soundtrack_song		= 0;
static bool		s_xbox_user_soundtrack_random	= false;
static uint32	s_xbox_random_index				= 0;

static int		sp_xbox_randomized_songs[MAX_USER_SONGS];
#endif

static bool     shuffle_random_songs=true;
static int      num_random_songs = 1;
static int      random_song_index = 0;
static int		sp_random_song_order[MAX_USER_SONGS];


// Stream ID functions
bool IDAvailable(uint32 id)
{
	for ( int i = 0; i < NUM_STREAMS; i++ )
	{
		if ( (gCurrentStreamInfo[ i ].uniqueID == id) && (PCMAudio_GetStreamStatus(gCurrentStreamInfo[ i ].voice) != PCM_STATUS_FREE))
		{
			return false;
		}
	}

	return true;
}

int GetChannelFromID(uint32 id, bool assert_if_duplicate = true)
{
	int i;

	for ( i = 0; i < NUM_STREAMS; i++ )
	{
		if ( (gCurrentStreamInfo[ i ].uniqueID == id) && (PCMAudio_GetStreamStatus(gCurrentStreamInfo[ i ].voice) != PCM_STATUS_FREE))
		{
			return i;
		}
	}

	for ( i = 0; i < NUM_STREAMS; i++ )
	{
		if ( (gCurrentStreamInfo[ i ].controlID == id) && (PCMAudio_GetStreamStatus(gCurrentStreamInfo[ i ].voice) != PCM_STATUS_FREE))
		{
			Dbg_MsgAssert(!assert_if_duplicate, ("Trying to control duplicate stream %s by checksum", Script::FindChecksumName(id)));
			return i;
		}
	}

	return -1;
}

uint32 GenerateUniqueID(uint32 id)
{
	// Keep incrementing ID until one works.
	while (!IDAvailable(id))
		id++;

	return id;
}

void StopMusic( void )
{
	if (NoMusicPlease()) return;
	
	// the counter has to reach SONG_UPDATE_INTERVAL before a new song is played...
	// so resetting the counter keeps a new song from playing right away.
	sCounter = 1;
	gMusicStreamType = MUSIC_STREAM_TYPE_NONE;		// In case we were in this mode
	PCMAudio_StopMusic( true );
}

// If a channel is specified, stop a specific stream (might be a streaming soundFX on an object that's getting
// destroyed, for example).
// If channel parameter is -1, stop all non-music streams!
void StopStreams( int channel )
{
	Replay::WriteStopStream(channel);
	if (StreamsDisabled()) return;
	
	Dbg_MsgAssert( ( channel >= -1 ) && ( channel < NUM_STREAMS ), ( "Invalid stream channel %d", channel ) );
	if ( channel == -1 )
	{
		PCMAudio_StopStreams( );
	}
	else
	{
		PCMAudio_StopStream( channel );
	}
}

// if uniqueID isn't specified, stop whatever stream is playing!
void StopStreamFromID( uint32 streamID )
{
	if (StreamsDisabled()) return;
	
	int channel = GetChannelFromID(streamID);

	if (channel >= 0)
	{
		PCMAudio_StopStream( channel, true );
		streamID = gCurrentStreamInfo[ channel ].uniqueID;		// change over to uniqueID
	}

	for ( int i = 0; i < NUM_STREAMS; i++ )
	{
		if ( gpStreamingObj[ i ] )
		{
			for ( int j = 0; j < MAX_STREAMS_PER_OBJECT; j++ )
			{
				if ( gpStreamingObj[ i ]->mStreamingID[ j ] == streamID )
				{
					gpStreamingObj[ i ]->mStreamingID[ j ] = 0;
					gpStreamingObj[ i ] = NULL;
					return;
				}
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool SetStreamVolumeFromID( uint32 streamID, Sfx::sVolume *p_volume )
{
	if (StreamsDisabled()) return true;
	
	int channel = GetChannelFromID(streamID);

	if (channel >= 0)
	{
#		ifdef __PLAT_XBOX__
		PCMAudio_SetStreamVolume( p_volume, channel );
#		else
		PCMAudio_SetStreamVolume( p_volume->GetChannelVolume( 0 ), p_volume->GetChannelVolume( 1 ), channel );
#		endif

		return true;
	}
	else
	{
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool SetStreamPitchFromID( uint32 streamID, float pitch )
{
	if (StreamsDisabled()) return true;
	
	int channel = GetChannelFromID(streamID);

	if (channel >= 0)
	{
		PCMAudio_SetStreamPitch( pitch, channel );

		return true;
	}
	else
	{
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
#ifdef __PLAT_XBOX__
static void sGenerateRandomSongOrder( void )
{
	int num_songs=Pcm::GetSoundtrackNumSongs( s_xbox_user_soundtrack );

	if( num_songs == 0 )
	{
		// Nothing to do.
		return;
	}
		
	// Limit to MAX_USER_SONGS to prevent any buffer overwriting.
	if (num_songs>MAX_USER_SONGS)
	{
		num_songs=MAX_USER_SONGS;
	}	

	// Remember the last song in the previous order, so we can make sure that
	// the first song in the new order is different from it.
	// Note: OK if the last song was just an uninitialised random value, won't cause any problems.
	int last_song=sp_xbox_randomized_songs[num_songs-1];
	
	// Initialise the order to be 0,1,2,3 ... etc
	for (int i=0; i<num_songs; ++i)
	{
		sp_xbox_randomized_songs[i]=i;
	}

	// Jumble it up.		
	for (int n=0; n<2000; ++n)
	{
		int a=Mth::Rnd(num_songs);
		int b=Mth::Rnd(num_songs);
		
		int temp=sp_xbox_randomized_songs[a];
		sp_xbox_randomized_songs[a]=sp_xbox_randomized_songs[b];
		sp_xbox_randomized_songs[b]=temp;
	}
	
	// If the first song in the new order equals the last song of the last order,
	// do one further swap to make sure it is different.
	if (sp_xbox_randomized_songs[0]==last_song)
	{
		// a is the first song.
		int a=0;
		
		// Choose b to be a random one of the other songs.
		int b;
		if (num_songs>1)
		{
			// Make b not able to equal 0, so that we don't swap the first with itself.
			b=1+Mth::Rnd(num_songs-1);
		}
		else
		{
			// Unless there is only one song, in which case just swap it with itself. No point but what the heck.
			b=0;
		}
		
		// Do the swap.
		int temp=sp_xbox_randomized_songs[a];
		sp_xbox_randomized_songs[a]=sp_xbox_randomized_songs[b];
		sp_xbox_randomized_songs[b]=temp;
	}
}
#endif // __PLAT_XBOX__



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void UseUserSoundtrack( int soundtrack )
{
#	ifdef __PLAT_XBOX__

	Dbg_MsgAssert(soundtrack>=0 && soundtrack<Pcm::GetNumSoundtracks(),("Bad soundtrack"));

	s_xbox_play_user_soundtracks=true;
	s_xbox_user_soundtrack=soundtrack;
	if (s_xbox_user_soundtrack_random)
	{
		sGenerateRandomSongOrder();
		s_xbox_random_index=0;
		s_xbox_user_soundtrack_song=sp_xbox_randomized_songs[0];
	}
	else
	{
		s_xbox_user_soundtrack_song=0;
	}	
#	endif // __PLAT_XBOX__
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void UseStandardSoundtrack( void )
{
#	ifdef __PLAT_XBOX__
	s_xbox_play_user_soundtracks = false;
#	endif
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool UsingUserSoundtrack( void )
{
#	ifdef __PLAT_XBOX__
	return s_xbox_play_user_soundtracks;
#	else
	return false;
#	endif
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SaveSoundtrackToStructure( Script::CStruct *pStuff)
{
#	ifdef __PLAT_XBOX__
	Dbg_MsgAssert( pStuff,("NULL pStuff"));
		
	if (s_xbox_play_user_soundtracks)
	{
		pStuff->AddComponent(Script::GenerateCRC("UserSoundtrackIndex"),ESYMBOLTYPE_INTEGER,s_xbox_user_soundtrack);

		char p_buf[100];
		const WCHAR* p_soundtrack_name_wide=Pcm::GetSoundtrackName(s_xbox_user_soundtrack);
		if (p_soundtrack_name_wide)
		{
			wsprintfA( p_buf, "%ls", p_soundtrack_name_wide);
		}	
		else
		{
			p_buf[0]=0;
		}	
				
		pStuff->AddComponent(Script::GenerateCRC("UserSoundtrackName"),ESYMBOLTYPE_STRING,p_buf);
	}	
#	endif // __PLAT_XBOX__
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void GetSoundtrackFromStructure( Script::CStruct *pStuff)
{
#	ifdef __PLAT_XBOX__
	Dbg_MsgAssert(pStuff,("NULL pStuff"));
	
	int user_soundtrack_index=0;
	if (pStuff->GetInteger("UserSoundtrackIndex",&user_soundtrack_index))
	{
		// A user soundtrack index is specified, but maybe the name does not match what is on this
		// machine, ie, they could have put the mem card into a different xbox ...
		
		// Get the name of this soundtrack on this machine
		char p_buf[100];
		const WCHAR* p_soundtrack_name_wide=Pcm::GetSoundtrackName(user_soundtrack_index);
		if (p_soundtrack_name_wide)
		{
			wsprintfA( p_buf, "%ls", p_soundtrack_name_wide);
		}	
		else
		{
			p_buf[0]=0;
		}	
	
		// Get the name as stored on the mem card
		const char *p_stored_name="";
		pStuff->GetText("UserSoundtrackName",&p_stored_name);
		
		if (strcmp(p_stored_name,p_buf)==0)
		{
			// They match! So use that soundtrack.
			UseUserSoundtrack(user_soundtrack_index);
			return;
		}	
	}	
	
	// Oh well, use the standard soundtrack instead.
	UseStandardSoundtrack();
#	endif	// __PLAT_XBOX__
}



bool _PlayMusicTrack( const char *filename, float volume )
{
	if (NoMusicPlease()) return false;
	
	Dbg_MsgAssert( gPcmInitialized,( "Calling playtrack %s, when PCM Audio not initialized.", filename ));
	Dbg_MsgAssert( strlen( filename ) < 200,( "Filename too long" ));

#	ifdef __PLAT_XBOX__
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	if( s_xbox_play_user_soundtracks && !skate_mod->GetGameMode()->IsFrontEnd())
	{
		if( Pcm::GetSoundtrackNumSongs( s_xbox_user_soundtrack ) == 0 )
		{
			return false;
		}
			
		if( !PCMAudio_PlaySoundtrackMusicTrack( s_xbox_user_soundtrack, s_xbox_user_soundtrack_song ))
		{
			return false;
		}
		else
		{
			if( s_xbox_user_soundtrack_random )
			{
				++s_xbox_random_index;
				if( s_xbox_random_index >= Pcm::GetSoundtrackNumSongs( s_xbox_user_soundtrack ))
				{
					sGenerateRandomSongOrder();
					s_xbox_random_index = 0;
				}	
				s_xbox_user_soundtrack_song=sp_xbox_randomized_songs[s_xbox_random_index];
			}	
			else
			{
				++s_xbox_user_soundtrack_song;
				if( s_xbox_user_soundtrack_song >= Pcm::GetSoundtrackNumSongs( s_xbox_user_soundtrack ))
				{
					s_xbox_user_soundtrack_song = 0;
				}
			}	
		}		
	}
	else
	{
		if( !PCMAudio_PlayMusicTrack( filename ))
		{
			return false;
		}
	}		
#	else
	if( !PCMAudio_PlayMusicTrack( filename ))
	{
		return false;
	}
#	endif // __PLAT_XBOX__

//	volume = volume * Config::GetMasterVolume()/100.0f;
//	if (volume <0.1f)
//	{
//		volume = 0.0f;
//	}
	PCMAudio_SetMusicVolume( volume );
	return ( true );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool _PlayMusicStream( uint32 checksum, float volume )
{
	if (NoMusicPlease()) return false;
	
	Dbg_MsgAssert( gPcmInitialized,( "Calling playtrack %s, when PCM Audio not initialized.", Script::FindChecksumName(checksum) ));

#ifdef __PLAT_NGPS__
	if( !PCMAudio_PlayMusicStream( checksum ))
	{
		Dbg_MsgAssert(0, ( "Failed playing track %s", Script::FindChecksumName(checksum) ) );
		Dbg_Message( "Failed playing track %s", Script::FindChecksumName(checksum) );
		return false;
	}
#else
	Dbg_MsgAssert(0, ("PCMAudio_PlayMusicStream() not implemented on this platform."));
#endif // __PLAT_NGPS__

	PCMAudio_SetMusicVolume( volume );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
// returns -1 on failure, otherwise the index into which voice played the sound.
int _PlayStream( uint32 checksum, Sfx::sVolume *p_volume, float pitch, int priority, uint32 controlID, bool preload_only )
{
	if (StreamsDisabled()) return -1;
	
#	ifdef __PLAT_XBOX__
	Dbg_MsgAssert( gPcmInitialized,( "Calling _PlayStream(), when PCM Audio not initialized." ));
#	endif // __PLAT_XBOX__

	// Initialize lowest to a valid entry
	int lowest_priority = gCurrentStreamInfo[ 0 ].priority;
	int lowest_priority_channel = 0;
	uint32 lowest_priority_start_frame = gCurrentStreamInfo[ 0 ].start_frame;

	bool success;

	for ( int i = 0; i < NUM_STREAMS; i++ )
	{
		if ( PCMAudio_GetStreamStatus( i ) == PCM_STATUS_FREE )
		{
			if (preload_only)
			{
				success = PCMAudio_PreLoadStream( checksum, i );
			}
			else
			{
//				success = PCMAudio_PlayStream( checksum, i, volumeL, volumeR, pitch );
#				ifdef __PLAT_XBOX__
				success = PCMAudio_PlayStream( checksum, i, p_volume, pitch );
#				else
				success = PCMAudio_PlayStream( checksum, i, p_volume->GetChannelVolume( 0 ), p_volume->GetChannelVolume( 1 ), pitch );
#				endif
			}

			if ( !success )
			{
				return ( -1 );
			}

	   		gCurrentStreamInfo[ i ].controlID = controlID;
	   		gCurrentStreamInfo[ i ].uniqueID = GenerateUniqueID(controlID);
	   		gCurrentStreamInfo[ i ].voice = i;
	   		gCurrentStreamInfo[ i ].priority = priority;
	   		gCurrentStreamInfo[ i ].start_frame = (uint32)Tmr::GetRenderFrame();
			gCurrentStreamInfo[ i ].p_frame_amp = CStreamFrameAmpManager::sGetFrameAmp(checksum);
			return ( i );
		}
		else if ( gCurrentStreamInfo[ i ].priority <= lowest_priority )
		{
			// If it is equal priority, we want to find the oldest one
			if ((gCurrentStreamInfo[ i ].priority == lowest_priority) &&
				(gCurrentStreamInfo[ i ].start_frame >= lowest_priority_start_frame))
			{
				continue;
			}

			lowest_priority = gCurrentStreamInfo[ i ].priority;
			lowest_priority_channel = i;
			lowest_priority_start_frame = gCurrentStreamInfo[ i ].start_frame;
		}
	}

	// Couldn't find a free one, so lets see if we can knock one off
	if (priority >= lowest_priority)
	{
		PCMAudio_StopStream( lowest_priority_channel );

	#if WAIT_AFTER_STOP_STREAM
		Dbg_MsgAssert(0, ("Priority streams not implemented on this system"));		// Not implemented
		return ( -1 );
	#else
		if (preload_only)
		{
			success = PCMAudio_PreLoadStream( checksum, lowest_priority_channel );
		}
		else
		{
//			success = PCMAudio_PlayStream( checksum, lowest_priority_channel, volumeL, volumeR, pitch );
#			ifdef __PLAT_XBOX__
			success = PCMAudio_PlayStream( checksum, lowest_priority_channel, p_volume, pitch );
#			else
			success = PCMAudio_PlayStream( checksum, lowest_priority_channel, p_volume->GetChannelVolume( 0 ), p_volume->GetChannelVolume( 1 ), pitch );
#			endif
		}

		if ( !success )
		{
			Dbg_MsgAssert(0, ("Higher priority stream could not start.  Make sure there was another error (like file not found)."));
			return ( -1 );
		}

		gCurrentStreamInfo[ lowest_priority_channel ].controlID = controlID;
		gCurrentStreamInfo[ lowest_priority_channel ].uniqueID = GenerateUniqueID(controlID);
		gCurrentStreamInfo[ lowest_priority_channel ].voice = lowest_priority_channel;
		gCurrentStreamInfo[ lowest_priority_channel ].priority = priority;
		gCurrentStreamInfo[ lowest_priority_channel ].start_frame = (uint32)Tmr::GetRenderFrame();
		gCurrentStreamInfo[ lowest_priority_channel ].p_frame_amp = CStreamFrameAmpManager::sGetFrameAmp(checksum);
	#endif // WAIT_AFTER_STOP_STREAM
		return ( lowest_priority_channel );
	}

	return ( -1 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool _PreLoadMusicStream( uint32 checksum )
{
	if (NoMusicPlease()) return false;

	Dbg_MsgAssert( gPcmInitialized,( "Calling _PreLoadMusicStream %s, when PCM Audio not initialized.", Script::FindChecksumName(checksum) ));

	return PCMAudio_PreLoadMusicStream( checksum );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool _StartPreLoadedMusicStream( float volume )
{
	if( !PCMAudio_StartPreLoadedMusicStream( ))
	{
		return false;
	}

	PCMAudio_SetMusicVolume( volume );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#define MIN_STREAM_VOL		2.0f



uint32 PlayStreamFromObject( Obj::CStreamComponent *pComponent, uint32 streamNameChecksum, float dropoff, float volume,
							 float pitch, int priority, int use_pos_info, EDropoffFunc dropoffFunc, uint32 controlID )
{
	Sfx::CSfxManager * sfx_manager = Sfx::CSfxManager::Instance();

	// Don't start a stream if it won't be heard
	if( sfx_manager->PositionalSoundSilenceMode() )
	{
		return 0;
	}

	Replay::WritePositionalStream(pComponent->GetObject()->GetID(),streamNameChecksum,dropoff,volume,pitch,priority,use_pos_info);
	if (StreamsDisabled()) return 0;
	
	Sfx::sVolume vol;
	vol.SetSilent();
	
	// Let the priority system figure this out
	//if (!StreamAvailable())
	//	return 0;
	
	// Set up volume according to position of object to camera.
	if ( use_pos_info != 0 )
	{
		Gfx::Camera *pCamera = Nx::CViewportManager::sGetClosestCamera( pComponent->GetObject()->m_pos );

		if (pCamera)
		{
			Mth::Vector dropoff_pos;
			Mth::Vector *p_dropoff_pos = NULL;
			if (pComponent->GetClosestDropoffPos(pCamera, dropoff_pos))
			{
				p_dropoff_pos = &dropoff_pos;
			}

			sfx_manager->SetVolumeFromPos( &vol, pComponent->GetClosestEmitterPos(pCamera), dropoff, dropoffFunc, pCamera, p_dropoff_pos );
		}
		else
		{
			vol.SetSilent();
		}

		// Seems strange to cancel out a stream if it starting low
		if( fabsf( vol.GetLoudestChannel()) < MIN_STREAM_VOL )
		{
			if( Script::GetInteger( 0xd7bb618d /* DebugSoundFx */, Script::NO_ASSERT ))
			{
				Dbg_Message("I wanted to cancel stream %s", Script::FindChecksumName(streamNameChecksum));
			}
//			return 0;
		}
	}
	else
	{
		vol.SetChannelVolume( 0, volume );
		vol.SetChannelVolume( 1, volume );
	}
	
	// K: The 'false' on the end means do not record the call in the replay, because this
	// call to PlayStreamFromObject has already been recorded.
	uint32 uniqueID = PlayStream( streamNameChecksum, &vol, pitch, priority, controlID, false );
	if ( uniqueID )
	{
		Dbg_MsgAssert( ( ( gLastStreamChannelPlayed >= 0 ) && ( gLastStreamChannelPlayed < MAX_STREAMS_PER_OBJECT ) ), ( "Tell Matt to fix object streams" ) );
		if ( gpStreamingObj[ gLastStreamChannelPlayed ] )
		{
			// the object might not have called StreamUpdate yet, so the slot might
			// still think it's in use... clear the slot if that's the case:
			gpStreamingObj[ gLastStreamChannelPlayed ]->mStreamingID[ gLastStreamChannelPlayed ] = 0;
			gpStreamingObj[ gLastStreamChannelPlayed ] = NULL;
		}
//		Dbg_MsgAssert( !gpStreamingObj[ gLastStreamChannelPlayed ], ( "Have Matt fix object streams... Kick him in the nutsack while you're at it." ) );
//		Dbg_MsgAssert( !pObject->mStreamingID[ gLastStreamChannelPlayed ], ( "Have Matt fix object streams... Hell, fire matt!!!" ) );
		gpStreamingObj[ gLastStreamChannelPlayed ] = pComponent;
		pComponent->mStreamingID[ gLastStreamChannelPlayed ] = uniqueID;
		// if volume modifier sent in, store that:
		if ( volume != 100.0f )
		{
			int i;
			for ( i = 0; i < NUM_STREAMS; i++ )
			{
				if ( gCurrentStreamInfo[ i ].uniqueID == uniqueID )
				{
					gCurrentStreamInfo[ i ].volume = PERCENT( gCurrentStreamInfo[ i ].volume, volume );
					gCurrentStreamInfo[ i ].dropoff = dropoff;
				}
			}
		}

		// store state of use_pos_info
		if ( !use_pos_info )
		{
			for ( int i = 0; i < NUM_STREAMS; i++ )
			{
				if ( gCurrentStreamInfo[i].uniqueID == uniqueID )
				{
					gCurrentStreamInfo[i].use_pos_info = false;
				}
			}
		}
		else
		{
			for ( int i = 0; i < NUM_STREAMS; i++ )
			{
				if ( gCurrentStreamInfo[i].uniqueID == uniqueID )
				{
					gCurrentStreamInfo[i].use_pos_info = true;
					gCurrentStreamInfo[i].dropoffFunction = dropoffFunc;
				}
			}
		}
	}
	return ( uniqueID );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint32 PlayStream( uint32 checksum, Sfx::sVolume *p_volume, float pitch, int priority, uint32 controlID, bool record_in_replay )
{
	if (record_in_replay)
	{
		Replay::WritePlayStream( checksum, p_volume, pitch, priority );
	}	
	if (StreamsDisabled()) return 0;
	
	// Just use checksum if no controlID is set
	if (controlID == 0)
	{
		controlID = checksum;
	}

	if (!IDAvailable(controlID))
	{
		Dbg_Message("Playing stream %s with same checksum as one already being played, won't be able to control directly.", Script::FindChecksumName(controlID));
	}
	
	int whichStream;

	// allow streams to play with default values, if not pre-loaded.
	whichStream = _PlayStream( checksum, p_volume, pitch, priority, controlID, false );
	if ( whichStream != -1 )
	{
		gCurrentStreamInfo[ whichStream ].dropoff = DEFAULT_STREAM_DROPOFF;
		gCurrentStreamInfo[ whichStream ].volume = 100.0f;
		gLastStreamChannelPlayed = whichStream;
		return gCurrentStreamInfo[ whichStream ].uniqueID;
	}

//	Dbg_MsgAssert( 0,( "Tried to play track %s not in header.", Script::FindChecksumName( checksum ) ));
	return ( 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 PreLoadStream( uint32 checksum, int priority )
{
	if (StreamsDisabled()) return 0;
	
	// Since this isn't hooked up to a script, we just supply the checksum
	uint32 controlID = checksum;

	int whichStream;

	// allow streams to play with default values, if not in list.
	Sfx::sVolume vol;
	vol.SetSilent();
	whichStream = _PlayStream( checksum, &vol, 0.0, priority, controlID, true );

	if ( whichStream != -1 )
	{
#ifdef __PLAT_NGPS__
		Dbg_Message("PreLoadStream for stream %s on channel %d", Script::FindChecksumName( checksum ), whichStream);
#endif 
		gCurrentStreamInfo[ whichStream ].dropoff = DEFAULT_STREAM_DROPOFF;
		gCurrentStreamInfo[ whichStream ].volume = 100.0f;
		return gCurrentStreamInfo[ whichStream ].uniqueID;
	}

//	Dbg_MsgAssert( 0,( "Tried to play track %s not in header.", Script::FindChecksumName( checksum ) ));
	return ( 0 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool PreLoadStreamDone( uint32 streamID )
{
	if (StreamsDisabled()) {
		return false;
	}

	int channel = GetChannelFromID(streamID);

	if ( channel >= 0 )
	{
#ifdef __PLAT_NGPS__
		if (PCMAudio_PreLoadStreamDone( channel ))
		{
			Dbg_Message("PreLoadStreamDone for stream %s on channel %d is true", Script::FindChecksumName( streamID ), channel );
		}
#endif 
		return PCMAudio_PreLoadStreamDone( channel );
	}

	// Couldn't find stream
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool StartPreLoadedStream( uint32 streamID, Sfx::sVolume *p_volume, float pitch )
{
	if (StreamsDisabled()) return false;
	
	int channel = GetChannelFromID(streamID);

	if ( channel >= 0 )
	{
#ifdef __PLAT_NGPS__
		Dbg_Message("StartPreLoadStream for stream %s on channel %d", Script::FindChecksumName( streamID ), channel);
#endif 

//		return PCMAudio_StartPreLoadedStream( channel, volumeL, volumeR, pitch );
#		ifdef __PLAT_XBOX__
		return PCMAudio_StartPreLoadedStream( channel, p_volume, pitch );
#		else
		return PCMAudio_StartPreLoadedStream( channel, p_volume->GetChannelVolume( 0 ), p_volume->GetChannelVolume( 1 ), pitch );
#		endif
	}

	// Couldn't find stream
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool PreLoadMusicStream( uint32 checksum )
{
	if (NoMusicPlease()) return false;

	Dbg_MsgAssert( gPcmInitialized,( "Calling Pcm::PlayMusicStream '%s', when PCM Audio not initialized.", Script::FindChecksumName(checksum) ));
	Dbg_MsgAssert( !gMusicStreamType,( "Calling Pcm::PlayMusicStream '%s', when one is already playing.", Script::FindChecksumName(checksum) ));

	// Stop any previous track
	PCMAudio_StopMusic(false);
	
	bool success = false;

#if WAIT_AFTER_STOP_STREAM
	gMusicStreamChecksum = checksum;
	gMusicStreamWaitingToStart = true;
#else
	// allow streams to play with default values, if not in list.
	success = _PreLoadMusicStream( checksum );
#endif // WAIT_AFTER_STOP_STREAM

	gMusicStreamType = MUSIC_STREAM_TYPE_PRELOAD;

	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool PreLoadMusicStreamDone( void )
{
	if (NoMusicPlease()) return false;
	Dbg_MsgAssert( gMusicStreamType == MUSIC_STREAM_TYPE_PRELOAD,( "Calling Pcm::PreLoadMusicStreamDone while in normal music mode." ));

	return PCMAudio_PreLoadMusicStreamDone( );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool StartPreLoadedMusicStream( float volume )
{
	if (NoMusicPlease()) return false;

	Dbg_MsgAssert( gMusicStreamType == MUSIC_STREAM_TYPE_PRELOAD,( "Calling Pcm::StartPreLoadedMusicStream while in normal music mode." ));

	if ( volume < 0.0f )
	{
		volume = gMusicStreamVolume;
	}
	
	return _StartPreLoadedMusicStream( volume );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool SetStreamVolume( int objStreamIndex )
{
	if ( StreamsDisabled() ) return false;
	
	Obj::CStreamComponent *pComp;
	pComp = gpStreamingObj[ objStreamIndex ];

	CurrentStreamInfo *pInfo = NULL;

	int i;
	for ( i = 0; i < NUM_STREAMS; i++ )
	{
		if ( gCurrentStreamInfo[ i ].uniqueID == pComp->mStreamingID[ objStreamIndex ] )
		{
			if ( PCMAudio_GetStreamStatus( gCurrentStreamInfo[ objStreamIndex ].voice ) == PCM_STATUS_FREE )
			{
				return ( false );
			}	
			pInfo = &gCurrentStreamInfo[ i ];

			// Update volume from dropoff/pan for the object responsible for streaming.
			Sfx::sVolume vol;

			Sfx::CSfxManager * sfx_manager = Sfx::CSfxManager::Instance();
			if ( pInfo->use_pos_info )
			{
				Gfx::Camera *pCamera = Nx::CViewportManager::sGetClosestCamera( pComp->GetObject()->m_pos );

				if (pCamera)
				{
					Mth::Vector dropoff_pos;
					Mth::Vector *p_dropoff_pos = NULL;
					if (pComp->GetClosestDropoffPos(pCamera, dropoff_pos))
					{
						p_dropoff_pos = &dropoff_pos;
					}

					sfx_manager->SetVolumeFromPos( &vol, pComp->GetClosestEmitterPos(pCamera), pInfo->dropoff, pInfo->dropoffFunction, pCamera, p_dropoff_pos );
				}
				else
				{
					vol.SetSilent();
				}
			}
			else
			{
				vol.SetChannelVolume( 0, pInfo->volume );
				vol.SetChannelVolume( 1, pInfo->volume );
			}

			if( pInfo->volume != 100.0f )
			{
				vol.PercentageAdjustment( pInfo->volume );
			}

#			ifdef __PLAT_XBOX__
			// We need the sign-correct version for Xbox to calculate proper 5.1 values.
			PCMAudio_SetStreamVolume( &vol, pInfo->voice );
#			else
			PCMAudio_SetStreamVolume( vol.GetChannelVolume( 0 ), vol.GetChannelVolume( 1 ), pInfo->voice );
#			endif
			return ( true );
		}
	}
	return ( false );
}

void Init( void )
{
	// Zero these out at startup.
	for( int s = 0; s < NUM_STREAMS; ++s )
	{
		gpStreamingObj[s] = NULL;
	}

	if (NoMusicPlease() && StreamsDisabled()) return;
	
	PCMAudio_Init( );
	gPcmInitialized = true;
	gMusicVolume = (float)Script::GetInteger( 0xabd4a575 ); // checksum 'MusicVolume'
	if ( !gMusicVolume )
	{
		gMusicVolume = DEFAULT_MUSIC_VOLUME;
	}
//	SetVolume( gMusicVolume );
	gMusicStreamVolume = (float)Script::GetInteger(CRCD(0x73f8e03b,"MusicStreamVolume"));
	if ( !gMusicStreamVolume )
	{
		gMusicStreamVolume = DEFAULT_MUSIC_STREAM_VOLUME;
	}

	int i;
	for ( i = 0; i < NUM_TRACKLISTS; i++ )
	{
		gTrackLists[ i ].numTracks = 0;
		gTrackLists[ i ].trackForbidden0 = 0;
        gTrackLists[ i ].trackForbidden1 = 0;
		gTrackLists[ i ].trackPlayed0 = 0;
        gTrackLists[ i ].trackPlayed1 = 0;
		// no tracks, so all tracks forbidden, right?
		gTrackLists[ i ].allTracksForbidden = true;
	}
	for ( i = 0; i < NUM_STREAMS; i++ )
	{
		gpStreamingObj[ i ] = NULL;
		gCurrentStreamInfo[ i ].controlID = 0;
		gCurrentStreamInfo[ i ].uniqueID = 0;
	}
} // end of Init( )


// Should be renamed to StopUsingCD( ) as if the cd is in use
// by streams, this function stops the streams and frees up the
// CD.
bool UsingCD( void )
{
	
	#if defined( __PLAT_NGC__ )
	if ( !( gPcmInitialized ) )
	{
		return ( false );
	}
	StopMusic( );
	StopStreams( );
	
	#else
	
	if (Config::CD() || TEST_FROM_CD)
	{
		if ( !( gPcmInitialized ) )
		{
	//		Dbg_Message( "uninitialized" );
			return ( false );
		}
		StopMusic( );
		StopStreams( );
	}	
	#endif
		
	return ( false );
} // end of UsingCD( )

// plays a music track.
void PlayTrack( const char *filename, bool loop )
{
	if (NoMusicPlease()) return;

	if (gMusicStreamType)
	{
		Dbg_MsgAssert(0, ("Trying to play a music track in stereo stream mode."));
		return;
	}
	
	Dbg_MsgAssert( gPcmInitialized,( "Calling Pcm::PlayTrack '%s', when PCM Audio not initialized.", filename ));
	Dbg_MsgAssert( strlen( filename ) < MAX_TRACKNAME_STRING_LENGTH,( "CD Audio Filename too long." ));
	strcpy( gTrackName, filename );
	
	int trackList = gCurrentTrackList;
	float volume;
	if ( trackList == TRACKLIST_PERM )
	{
		volume = gMusicVolume;
	}
	else
	{
		Sfx::CSfxManager * sfx_manager = Sfx::CSfxManager::Instance();
		volume = sfx_manager->GetMainVolume( );
	}
	
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	if ( skate_mod->GetGameMode( )->IsFrontEnd( ) )
	{
		trackList = TRACKLIST_LEVEL_SPECIFIC;
		volume = gMusicVolume;
	}

	SetLoopingMode(loop);
	
	if ( !_PlayMusicTrack( gTrackName, volume ) )
	{
		if (Config::CD() || TEST_FROM_CD)
		{
			Dbg_Message( "Failed playing track %s", filename );
		}	
		else
		{
			// see if this is a track in the current playlists...
			// if so, don't keep trying to play it...
			TrackList *pTrackList = &gTrackLists[ trackList ];
			int i;
			for ( i = 0; i < pTrackList->numTracks; i++ )
			{
				if ( Script::GenerateCRC( filename ) == Script::GenerateCRC( pTrackList->trackInfo[ i ].trackName ) )
				{
					SetTrackForbiddenStatus( i, true, trackList );
					return;
				}
			}
		}	
	}
}

// Plays a stereo stream through the music channels
void PlayMusicStream( uint32 checksum, float volume )
{
	if (NoMusicPlease()) return;

	Dbg_MsgAssert( gPcmInitialized,( "Calling Pcm::PlayMusicStream '%s', when PCM Audio not initialized.", Script::FindChecksumName(checksum) ));
	Dbg_MsgAssert( !gMusicStreamType,( "Calling Pcm::PlayMusicStream '%s', when one is already playing.", Script::FindChecksumName(checksum) ));

	// Stop any previous track
	PCMAudio_StopMusic(false);

	if ( volume < 0.0f )
	{
		volume = gMusicStreamVolume;
	}

#if WAIT_AFTER_STOP_STREAM
	gMusicStreamChecksum = checksum;
	gMusicStreamVolume = volume;
	gMusicStreamWaitingToStart = true;
#else
	_PlayMusicStream( checksum, volume );
#endif

	gMusicStreamType = MUSIC_STREAM_TYPE_NORMAL;
}

static bool sMusicIsPaused=false;
void PauseMusic( int pause )
{
	
	
	static bool wasPaused = false;

	if (NoMusicPlease()) return;
	
	Dbg_MsgAssert( gPcmInitialized,( "Calling Pcm::Pause when PCM Audio not initialized." ));
	switch ( pause )
	{
		case ( -1 ):
			// special case... unpause only if it was unpaused last time pause was called...
			if ( !wasPaused )
			{
				// recursive... unpause this bitch...
				PauseMusic( 0 );
				return;
			}
			break;
		case ( 1 ):
			wasPaused = sMusicIsPaused;
			sMusicIsPaused = true;
			PCMAudio_Pause( true, MUSIC_CHANNEL );
			break;
		case ( 0 ):
			sMusicIsPaused = false;
			PCMAudio_Pause( false, MUSIC_CHANNEL );
			break;
		default:
			Dbg_MsgAssert( 0, ( "Unknown pause state %d", pause ) );
			break;
	}
}

bool MusicIsPaused()
{
	return sMusicIsPaused;
}
	
void PauseStream( int pause )
{
	
	
	static bool paused = false;
	static bool wasPaused = false;

	if (StreamsDisabled()) return;
	
	Dbg_MsgAssert( gPcmInitialized,( "Calling Pcm::Pause when PCM Audio not initialized." ));
	switch ( pause )
	{
		case ( -1 ):
			// special case... unpause only if it was unpaused last time pause was called...
			if ( !wasPaused )
			{
				// recursive... unpause this bitch...
				PauseStream( 0 );
				return;
			}
			break;
		case ( 1 ):
			wasPaused = paused;
			paused = true;
			PCMAudio_Pause( true, EXTRA_CHANNEL );
			break;
		case ( 0 ):
			paused = false;
			PCMAudio_Pause( false, EXTRA_CHANNEL );
			break;
		default:
			Dbg_MsgAssert( 0, ( "Unknown pause state %d", pause ) );
			break;
	}
}

void SetVolume( float volume )
{
	
	if (NoMusicPlease()) return;
	
	Dbg_MsgAssert( gPcmInitialized,( "Calling Pcm::SetVolume, when PCM Audio not initialized." ));
	if ( ( volume > 100.0f ) || ( volume < 0.0f ) )
	{
		Dbg_MsgAssert( 0,( "Volume (%f) not between 0 and 100", volume ));
		return;
	}
	
	
	gMusicVolume = volume;
	if ( gPcmInitialized )
	{
		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		if ( ( gMusicStreamType == MUSIC_STREAM_TYPE_NONE) &&
			 ( ( gCurrentTrackList == TRACKLIST_PERM ) ||
			 ( skate_mod->GetGameMode( )->IsFrontEnd( ) ) ) )
		{
//			volume = volume * Config::GetMasterVolume()/100.0f;
//			if (volume <0.1f)
//			{
//				volume = 0.0f;
//			}
			PCMAudio_SetMusicVolume( volume );
		}
	}
}

float GetVolume( void )
{
	
	return ( gMusicVolume );
}

void SetMusicStreamVolume( float volume )
{
	if (NoMusicPlease()) return;
	
	Dbg_MsgAssert( gPcmInitialized,( "Calling Pcm::SetVolume, when PCM Audio not initialized." ));
	if ( ( volume > 100.0f ) || ( volume < 0.0f ) )
	{
		Dbg_MsgAssert( 0,( "Volume (%f) not between 0 and 100", volume ));
		return;
	}
	
	gMusicStreamVolume = volume;
	if ( gPcmInitialized )
	{
		if (gMusicStreamType == MUSIC_STREAM_TYPE_NORMAL)
		{
			PCMAudio_SetMusicVolume( volume );
		}
	}
}

float GetMusicStreamVolume( void )
{
	return gMusicStreamVolume;
}

void AddTrackToPlaylist( const char *trackName, int whichList, const char *trackTitle )
{
	
	if (NoMusicPlease()) return;
	

	Dbg_MsgAssert( trackName,( "What the fuck?" ));
	Dbg_MsgAssert( whichList < NUM_TRACKLISTS,( "Dumbass." ));
	TrackList *pTrackList = &gTrackLists[ whichList ];

	// Ken: This is a fix to TT6484. When an ambient track finished, it was choosing
	// a new random ambient track, rather than playing the same one again.
	// So to fix, just force numTracks to zero if adding an ambient track, so that there
	// can be only one. That way, it will be forced to play the same one again. Nya ha!
	if (whichList==TRACKLIST_LEVEL_SPECIFIC)
	{
		pTrackList->numTracks=0;
	}
	
	if ( pTrackList->numTracks >= MAX_NUM_TRACKS )
	{
		Dbg_MsgAssert( 0,( "Too many tracks in playlist (increase MAX_NUM_TRACKS or clear list)." ));
		return;
	}
	Dbg_MsgAssert( strlen( trackName ) < MAX_TRACKNAME_STRING_LENGTH,( "Track name too long." ));
	char *pTrackName = pTrackList->trackInfo[ pTrackList->numTracks ].trackName;
	strcpy( pTrackName, trackName );
	
	uint i;
	for ( i = 0; i < strlen( pTrackName ); i++ )
	{
		if ( ( pTrackName[ i ] >= 'a' ) && ( pTrackName[ i ] <= 'z' ) )
		{
			pTrackName[ i ] = pTrackName[ i ] - 'a' + 'A';
		}
	}
	// Music tracks should have a title (for the user to be able to turn songs on/off):
	if ( whichList == TRACKLIST_PERM )
	{
		char *pTrackTitle = PermTrackTitle[ pTrackList->numTracks ].trackTitle;
		if ( trackTitle )
		{
			Dbg_MsgAssert( strlen( trackTitle ) < TRACK_TITLE_MAX_SIZE, ( "Track title %s too long (max %d chars)", trackTitle, TRACK_TITLE_MAX_SIZE ) );
			strncpy( pTrackTitle, trackTitle, TRACK_TITLE_MAX_SIZE );
			pTrackTitle[ TRACK_TITLE_MAX_SIZE - 1 ] = '\0';
		}
		else
		{
	   		Dbg_MsgAssert( 0, ( "Track %s not given a title.", trackName ) );
			strcpy( pTrackTitle, "No title provided" );
		}
	}
	if ( !PCMAudio_TrackExists( pTrackName, MUSIC_CHANNEL ) )
	{
		Dbg_MsgAssert(!Config::CD(),("Could not find music track '%s'\n",pTrackName));
		Dbg_Message("Could not find music track '%s'",pTrackName);
		return;
	}	
	pTrackList->allTracksForbidden = false;
    pTrackList->numTracks++;
}

void ClearPlaylist( int whichList )
{
	
	if (NoMusicPlease()) return;
	
	Dbg_MsgAssert( whichList < NUM_TRACKLISTS,( "Dumbshit." ));
	TrackList *pTrackList = &gTrackLists[ whichList ];
	pTrackList->numTracks = 0;
	pTrackList->allTracksForbidden = true; // if there are no tracks, all are forbidden, no?
	pTrackList->trackForbidden0 = 0;
    pTrackList->trackForbidden1 = 0;
	pTrackList->trackPlayed0 = 0;
    pTrackList->trackPlayed1 = 0;
	
	gNumStreams = 0;
}

void SkipMusicTrack( void )
{
	
	if (NoMusicPlease()) return;
	
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	if ( skate_mod->GetGameMode( )->IsFrontEnd( ) )
	{
		return;
	}

	if ( gCurrentTrackList == TRACKLIST_PERM )
	{
		// will stop the current song, auto update will
		// cause music to continue with the next track...
		StopMusic( );
	}
}

void SetRandomMode( int randomModeOn )
{
	if (NoMusicPlease()) return;

	gMusicInRandomMode = randomModeOn;

#	ifdef __PLAT_XBOX__
	s_xbox_user_soundtrack_random = ( randomModeOn > 0 );
	if( randomModeOn )
	{
		// Generate a new random order now, since the current one may be invalid for this soundtrack.
		sGenerateRandomSongOrder();
		s_xbox_random_index = 0;
		s_xbox_user_soundtrack_song = sp_xbox_randomized_songs[0];
	}
#	endif // __PLAT_XBOX__
	
	Dbg_MsgAssert(gCurrentTrackList>=0 && gCurrentTrackList<NUM_TRACKLISTS,("Bad gCurrentTrackList"));
	TrackList *pTrackList = &gTrackLists[ gCurrentTrackList ];
	pTrackList->trackPlayed0=0;
    pTrackList->trackPlayed1=0;
}

int GetRandomMode( void )
{
	return ( gMusicInRandomMode );
}

void SetLoopingMode( bool loop )
{
	gMusicLooping = loop;
}

#if WAIT_AFTER_STOP_STREAM
static void _StartMusicStream()
{
	switch (gMusicStreamType)
	{
	case MUSIC_STREAM_TYPE_NORMAL:
		_PlayMusicStream( gMusicStreamChecksum, gMusicStreamVolume );
		break;

	case MUSIC_STREAM_TYPE_PRELOAD:
		_PreLoadMusicStream( gMusicStreamChecksum );
		break;

	case MUSIC_STREAM_TYPE_NONE:
		Dbg_MsgAssert(0, ("Bad start music stream state"));
		break;
	}

	// No longer waiting
	gMusicStreamWaitingToStart = false;
}
#endif // WAIT_AFTER_STOP_STREAM

static void sSetCorrectMusicVolume()
{
	#ifdef __PLAT_XBOX__
	if (s_xbox_play_user_soundtracks)
	{
		// This 'if' is part of the fix to TT6957, where user soundtracks were getting sfx vol
		// when music vol turned down to zero.
		PCMAudio_SetMusicVolume( Pcm::GetVolume() );
	}
	else if (gCurrentTrackList == TRACKLIST_LEVEL_SPECIFIC)
	{
		Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
		PCMAudio_SetMusicVolume( sfx_manager->GetMainVolume() );
	}
	#else
	// Ken: I added this as a fix to TT5588, 'ambient tracks don't play when music vol is set to zero'
	// This forces the ambient track to use the sfx volume rather than gMusicVolume.
	// Hopefully there is not too much of an overhead to calling PCMAudio_SetMusicVolume ...
	// This bit of code won't be executed every frame though, but every SONG_UPDATE_INTERVAL frames,
	// so I think it'll be OK ...
	if (gCurrentTrackList == TRACKLIST_LEVEL_SPECIFIC)
	{
		Sfx::CSfxManager * sfx_manager =  Sfx::CSfxManager::Instance();
		//printf("Playing ambient track: Forcing volume to %f ...\n",sfx_manager->GetMainVolume());
		PCMAudio_SetMusicVolume( sfx_manager->GetMainVolume() );
	}
	#endif
}

void RandomTrackUpdate( void )
{
	
	TrackList *pTrackList = &gTrackLists[ gCurrentTrackList ];
	
	if (NoMusicPlease()) return;
	
	
	if ( pTrackList->allTracksForbidden )
	{
//		Dbg_Message( "all tracks forbidden.. random mode" );
		return;
	}
	int pcmStatus = PCMAudio_GetMusicStatus( );
	if ( pcmStatus == PCM_STATUS_FREE )
	{
	#if WAIT_AFTER_STOP_STREAM
		// If we were waiting for StopMusic() to finish, we need to start the music stream
		if (gMusicStreamWaitingToStart)
		{
			_StartMusicStream();
			return;
		}
	#endif // WAIT_AFTER_STOP_STREAM

		gMusicStreamType = MUSIC_STREAM_TYPE_NONE;		// Just in case we were playing a music stream
		sSetCorrectMusicVolume();

		// Replay track if looping
		if (gMusicLooping)
		{
			PlayTrack( gTrackName, true );
			return;
		}
		
		if ( !pTrackList->numTracks )
		{
			Dbg_MsgAssert( 0,( "AllTracksForbidden flag should be set if no tracks exist." ));
			return;
		}
		
        if ( shuffle_random_songs )
        {
            // Remember the last song in the previous order, so we can make sure that
        	// the first song in the new order is different from it.
        	// Note: OK if the last song was just an uninitialised random value, won't cause any problems.
        	int last_song=sp_random_song_order[num_random_songs-1];
            
            // count active tracks
            num_random_songs=0;
            for (int i=0; i<pTrackList->numTracks; ++i)
            {
                if ( i < 64)
                {
                    if ( !( pTrackList->trackForbidden0 & ( ((uint64)1) << i ) ) )
                    {
                        // Intialize order
                        sp_random_song_order[num_random_songs]=i;
                        // add one
                        num_random_songs++;
                    }
                }
                else
                {
                    if ( !( pTrackList->trackForbidden1 & ( ((uint64)1) << (i-64) ) ) )
                    {
                        // Intialize order
                        sp_random_song_order[num_random_songs]=i;
                        // add one
                        num_random_songs++;
                    }
                }
            }

            // need to shuffle order
            printf("shuffling random song order num_songs=%i\n", num_random_songs );
            
            // Jumble it up.		
        	for (int n=0; n<2000; ++n)
        	{
        		int a=Mth::Rnd(num_random_songs);
        		int b=Mth::Rnd(num_random_songs);
        		
                if ( a != b )
                {
                    int temp=sp_random_song_order[a];
            		sp_random_song_order[a]=sp_random_song_order[b];
            		sp_random_song_order[b]=temp;
                }
        	}
        	
            // If the first song in the new order equals the last song of the last order,
        	// do one further swap to make sure it is different.
        	if (sp_random_song_order[0]==last_song)
        	{
        		// a is the first song.
        		int a=0;
        		
        		// Choose b to be a random one of the other songs.
        		int b;
        		if (num_random_songs>1)
        		{
        			// Make b not able to equal 0, so that we don't swap the first with itself.
        			b=1+Mth::Rnd(num_random_songs-1);
        		}
        		else
        		{
        			// Unless there is only one song, in which case just swap it with itself. No point but what the heck.
        			b=0;
        		}
        		
        		// Do the swap.
        		int temp=sp_random_song_order[a];
        		sp_random_song_order[a]=sp_random_song_order[b];
        		sp_random_song_order[b]=temp;
        	}
            
            // reset index
            random_song_index=0;
            shuffle_random_songs=false;
            pTrackList->trackPlayed0 = 0;
            pTrackList->trackPlayed1 = 0;
        }
        else
        {
            random_song_index++;
            if ( random_song_index >= (num_random_songs-1) )
            {
                shuffle_random_songs=true;
            }
        }
        
        for ( int i = 0; i < pTrackList->numTracks; i++ )
		{
			int t = ( sp_random_song_order[random_song_index] );
            //printf("index = %i song = %i\n", random_song_index, sp_random_song_order[random_song_index] );

            if ( ( !( pTrackList->trackForbidden0 & ( ((uint64)1) << t ) ) && ( t < 64 ) && 
                   !( pTrackList->trackPlayed0 & ( ((uint64)1) << t ) ) ) )
			{
				pTrackList->trackPlayed0 |= ( ((uint64)1) << t );
				PlayTrack( pTrackList->trackInfo[ t ].trackName );
                current_music_track=t;
				Dbg_Message( "Playing track %s %i %i", pTrackList->trackInfo[ t ].trackName, t, current_music_track );

                // update track text on screen
                Script::CStruct *pParams = new Script::CStruct;
                pParams->AddInteger(CRCD(0x8d02705d,"current_track"),current_music_track);
                Script::RunScript( "spawn_update_music_track_text", pParams );
                delete pParams;

				return;
			}

            if ( ( !( pTrackList->trackForbidden1 & ( ((uint64)1) << (t-64) ) ) && ( t >= 64 ) && 
                   !( pTrackList->trackPlayed1 & ( ((uint64)1) << (t-64) ) ) )
               )
			{
				pTrackList->trackPlayed1 |= ( ((uint64)1) << (t-64) );
				PlayTrack( pTrackList->trackInfo[ t ].trackName );
                current_music_track=t;
				Dbg_Message( "Playing track %s %i %i", pTrackList->trackInfo[ t ].trackName, t, current_music_track );

                // update track text on screen
                Script::CStruct *pParams = new Script::CStruct;
                pParams->AddInteger(CRCD(0x8d02705d,"current_track"),current_music_track);
                Script::RunScript( "spawn_update_music_track_text", pParams );
                delete pParams;

				return;
			}
		}
	}
}

void TrackUpdate( void )
{
	

	if (NoMusicPlease()) return;
	
	int trackList = gCurrentTrackList;
	
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	if ( skate_mod->GetGameMode( )->IsFrontEnd( ) )
	{
		trackList = TRACKLIST_LEVEL_SPECIFIC;
	}

	TrackList *pTrackList = &gTrackLists[ trackList ];
	
	if ( pTrackList->allTracksForbidden )
	{
//		Dbg_Message( "all tracks forbidden.. sequential mode" );
		return;
	}
	int pcmStatus = PCMAudio_GetMusicStatus( );
	if ( pcmStatus == PCM_STATUS_FREE )
	{
	#if WAIT_AFTER_STOP_STREAM
		// If we were waiting for StopMusic() to finish, we need to start the music stream
		if (gMusicStreamWaitingToStart)
		{
			_StartMusicStream();
			return;
		}
	#endif // WAIT_AFTER_STOP_STREAM

		gMusicStreamType = MUSIC_STREAM_TYPE_NONE;		// Just in case we were playing a music stream
		sSetCorrectMusicVolume();
		
		// Replay track if looping
		if (gMusicLooping)
		{
			PlayTrack( gTrackName, true );
			return;
		}

		// don't have a random function.  just play next track...
		int i;
		for ( i = 0; i < pTrackList->numTracks; i++ )
		{
            if ( ( !( pTrackList->trackForbidden0 & ( ((uint64)1) << i ) ) && ( i < 64 ) && 
                   !( pTrackList->trackPlayed0 & ( ((uint64)1) << i ) ) ) )
			{
				pTrackList->trackPlayed0 |= ( ((uint64)1) << i );
				PlayTrack( pTrackList->trackInfo[ i ].trackName );
                current_music_track=i;
				Dbg_Message( "Playing track %s %i %i", pTrackList->trackInfo[ i ].trackName, i, current_music_track );

                // update track text on screen
                Script::CStruct *pParams = new Script::CStruct;
                pParams->AddInteger(CRCD(0x8d02705d,"current_track"),current_music_track);
                Script::RunScript( "spawn_update_music_track_text", pParams );
                delete pParams;

				return;
			}

            if ( ( !( pTrackList->trackForbidden1 & ( ((uint64)1) << (i-64) ) ) && ( i >= 64 ) && 
                   !( pTrackList->trackPlayed1 & ( ((uint64)1) << (i-64) ) ) ) )
			{
				pTrackList->trackPlayed1 |= ( ((uint64)1) << (i-64) );
				PlayTrack( pTrackList->trackInfo[ i ].trackName );
                current_music_track=i;
				Dbg_Message( "Playing track %s %i %i", pTrackList->trackInfo[ i ].trackName, i, current_music_track );

                // update track text on screen
                Script::CStruct *pParams = new Script::CStruct;
                pParams->AddInteger(CRCD(0x8d02705d,"current_track"),current_music_track);
                Script::RunScript( "spawn_update_music_track_text", pParams );
                delete pParams;

				return;
			}
		}
		// all the tracks have been played... reset:
		pTrackList->trackPlayed0 = 0;
        pTrackList->trackPlayed1 = 0;
	}
}

//powers of 2 please...
#define SONG_UPDATE_INTERVAL					64
#define STREAM_POSITIONAL_UPDATE_INTERVAL		4

// Call this function every frame...
// keeps tracks playing during levels and shit like that...
void Update( void )
{
	
	//static int streamingObjToUpdate = 0;

	if (NoMusicPlease() && StreamsDisabled()) return;
	
	Dbg_MsgAssert( gPcmInitialized,( "Calling Pcm::Update when PCM Audio not initialized." ));

	int err_no = PCMAudio_Update( );
	if ( err_no != 0 )
	{
		if (err_no < 0)
		{
			StopStreams( );
			StopMusic( );
		} else {
			// Even though we are returning the stream number, lets just kill them all because we are at the end of the project
			StopStreams( );
		}
	}

	// Garrett: This should go away, since the IOP communication is async now.  But I left it in for
	// the music now since I don't know if there is a CPU hit there.
	sCounter++;

	//if ( !( sCounter & ( STREAM_POSITIONAL_UPDATE_INTERVAL - 1 ) ) )
	for (int streamingObjToUpdate = 0; streamingObjToUpdate < NUM_STREAMS; streamingObjToUpdate++)
	{
		if ( gpStreamingObj[ streamingObjToUpdate ] )
		{
			if ( !SetStreamVolume( streamingObjToUpdate ) )
			{
				gpStreamingObj[ streamingObjToUpdate ]->mStreamingID[ streamingObjToUpdate ] = 0;
				gpStreamingObj[ streamingObjToUpdate ] = NULL;
			}
		}
		//// update the other one next time...
		//streamingObjToUpdate++;
		//if ( streamingObjToUpdate >= NUM_STREAMS )
		//{
		//	streamingObjToUpdate = 0;
		//}
	}

	// Free any frame amp data that was used by a stream that already stopped
	for (int i = 0; i < NUM_STREAMS; i++)
	{
		if (gCurrentStreamInfo[i].p_frame_amp && (PCMAudio_GetStreamStatus(gCurrentStreamInfo[i].voice) == PCM_STATUS_FREE))
		{
			Dbg_Message("Clearing StreamFrameAmp %s", Script::FindChecksumName(gCurrentStreamInfo[i].controlID));
			CStreamFrameAmpManager::sFreeFrameAmp(gCurrentStreamInfo[i].p_frame_amp);
			gCurrentStreamInfo[i].p_frame_amp = NULL;
		}
	}
	
	if ( sCounter & ( SONG_UPDATE_INTERVAL - 1 ) )
		return;

	// if volume is turned all the way down, the current song will finish
	// playing and then we'll keep returning here until volume isn't zero,
	// as soon as it is turned up a new song will begin playing.
	if ( gMusicVolume == 0.0f && gCurrentTrackList != TRACKLIST_LEVEL_SPECIFIC) 
	{
		return;
	}

	#ifdef __PLAT_XBOX__
	// This is part of the fix to TT6957, 
	// where user soundtracks were getting sfx vol when music vol turned down to zero.
	// (The other part of the fix is a bit further down, search for TT6957)
	if ( gMusicVolume == 0.0f && s_xbox_play_user_soundtracks) 
	{
		return;
	}
	#endif
	
	if (sMusicIsPaused)
	{
		return;
	}


	// K: Commented this out since it does not appear to be needed, and was stopping random
	// track mode from working on XBox & Gamecube, I think because is_frontend is defined to
	// be 1 in mode_skateshop in gamemode.q. I commented this out instead of changing
	// is_frontend to 0 in case that broke something else.		
	
	// don't play music in the frontend: <- old comment
	/*
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	if ( skate_mod->GetGameMode( )->IsFrontEnd( ) )
	{
		TrackUpdate( );
		return;
	}
	*/

		
	// if song is finished, start a new one playing...
	if ( gMusicInRandomMode || ( gCurrentTrackList == TRACKLIST_LEVEL_SPECIFIC ) )
		RandomTrackUpdate( );
	else
		TrackUpdate( );
}

void GetPlaylist( uint64* flags1, uint64* flags2 )
{
    *flags1 = gTrackLists[ TRACKLIST_PERM ].trackForbidden0;
    *flags2 = gTrackLists[ TRACKLIST_PERM ].trackForbidden1;
}

void SetPlaylist( uint64 flags1, uint64 flags2 )
{
	gTrackLists[ TRACKLIST_PERM ].trackForbidden0 = flags1;
    gTrackLists[ TRACKLIST_PERM ].trackForbidden1 = flags2;
}

// Ambient volume is the same as SoundFX volume...
void SetAmbientVolume( float volPercent )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	if ( skate_mod->GetGameMode( )->IsFrontEnd( ) )
	{
		return;
	}
	if ( gCurrentTrackList == TRACKLIST_LEVEL_SPECIFIC )
	{
		PCMAudio_SetMusicVolume( volPercent );
	}
}

// set a particular song as 'forbidden' (don't play it, the player hates that song...)
void SetTrackForbiddenStatus( int trackNum, bool forbidden, int whichList )
{
    if (NoMusicPlease()) return;
	
    shuffle_random_songs=true;

	int i;
	Dbg_MsgAssert( whichList < NUM_TRACKLISTS,( "Dumbass." ));
	TrackList *pTrackList = &gTrackLists[ whichList ];
	Dbg_MsgAssert( trackNum < pTrackList->numTracks,( "Not that many tracks in list." ));
	if ( forbidden )
	{
		if ( trackNum < 64)
        {
            pTrackList->trackForbidden0 |= ( ((uint64)1) << trackNum );
        }
        else
        {
            pTrackList->trackForbidden1 |= ( ((uint64)1) << (trackNum-64) );
        }
        
        if ( TrackIsPlaying( whichList, trackNum ) )
		{
			StopMusic( );
		}
		for ( i = 0; i < pTrackList->numTracks; i++ )
		{
			if ( !( pTrackList->trackForbidden0 & ( ((uint64)1) << i ) ) || !( pTrackList->trackForbidden1 & ( ((uint64)1) << (i-64) ) ) )
			{
				pTrackList->allTracksForbidden = false;
				return;
			}
		}
		pTrackList->allTracksForbidden = true;
		return;
	}
	
    if ( trackNum < 64)
    {
        pTrackList->trackForbidden0 &= ~( ((uint64)1) << trackNum );
    }
    else
    {
        pTrackList->trackForbidden1 &= ~( ((uint64)1) << (trackNum-64) );
    }
	pTrackList->allTracksForbidden = false;
}

/*void CheckLockedTracks()
{
    if ( !Mdl::Skate::Instance()->GetCareer()->GetGlobalFlag(286) )
    {
        // if the kiss songs aren't unlocked make sure they stay forbidden
        SetTrackForbiddenStatus( 32, true, Pcm::TRACKLIST_PERM );
        SetTrackForbiddenStatus( 33, true, Pcm::TRACKLIST_PERM );
    }
}*/

bool TrackIsPlaying( int whichList, int whichTrack )
{
	
	if (NoMusicPlease()) return false;
	
	Dbg_MsgAssert( gPcmInitialized,( "Calling Pcm func when PCM Audio not initialized." ));
	int pcmStatus = PCMAudio_GetMusicStatus( );
	if ( pcmStatus == PCM_STATUS_FREE )
		return ( false );
	if ( whichTrack >= gTrackLists[ whichList ].numTracks )
		return ( false );
	// can't do a strcmp, because files in gTrackName have been converted to all uppercase...
	if ( Script::GenerateCRC( gTrackName ) == Script::GenerateCRC( gTrackLists[ whichList ].trackInfo[ whichTrack ].trackName ) )
		return ( true );
	return ( false );
}

int GetNumTracks( int whichList )
{
	

	if (NoMusicPlease()) return 0;
	

	Dbg_MsgAssert( whichList < NUM_TRACKLISTS,( "Dumbass." ));
	TrackList *pTrackList = &gTrackLists[ whichList ];
	return ( pTrackList->numTracks );
}

int GetTrackForbiddenStatus( int trackNum, int whichList )
{
	

	if (NoMusicPlease()) return 1;
	
	
	Dbg_MsgAssert( whichList < NUM_TRACKLISTS,( "Dumbass." ));
	TrackList *pTrackList = &gTrackLists[ whichList ];
	Dbg_MsgAssert( trackNum < pTrackList->numTracks,( "Requesting forbidden status on invalid track." ));
	
    if ( trackNum < 64 )
    {
        if (pTrackList->trackForbidden0 & ( ((uint64)1) << trackNum ))
    	{
    		return true;
    	}
    	else
    	{
    		return false;
    	}
    }
    else
    {
        if (pTrackList->trackForbidden1 & ( ((uint64)1) << (trackNum-64) ))
    	{
    		return true;
    	}
    	else
    	{
    		return false;
    	}
    }
}

const char *GetTrackName( int trackNum, int whichList )
{
	

	if (NoMusicPlease())
	{
		strcpy( gTrackLists[ 0 ].trackInfo[ 0 ].trackName, "Empty." );
		return ( gTrackLists[ 0 ].trackInfo[ 0 ].trackName );
	}
	
	//if ( whichList == TRACKLIST_PERM )
	//{
	//	Dbg_MsgAssert( trackNum < MAX_NUM_TRACKS, ( "Invalid track num specified." ) );
	//	return ( PermTrackTitle[ trackNum ].trackTitle );
	//}

	Dbg_MsgAssert( whichList < NUM_TRACKLISTS,( "Dumbass." ));
	TrackList *pTrackList = &gTrackLists[ whichList ];
	Dbg_MsgAssert( trackNum < pTrackList->numTracks,( "Requesting invalid track name." ));
	int i;
	char *pText = pTrackList->trackInfo[ trackNum ].trackName;
	char *pRet = pText;
	int len = strlen( pText );
	for ( i = 0; i < len; i++ )
	if ( ( pText[ i ] == '\\' ) || ( pText[ i ] == '/' ) )
		pRet = &pText[ i + 1 ];
	return ( pRet );
}

// Ambient (TRACKLIST_LEVEL_SPECIFIC) or music (TRACKLIST_PERM)
void SetActiveTrackList( int whichList )
{
	

	if (NoMusicPlease()) return;
	
	Dbg_MsgAssert( whichList < NUM_TRACKLISTS,( "Dumbass." ));
	if ( gCurrentTrackList != whichList )
	{
		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		if ( !skate_mod->GetGameMode( )->IsFrontEnd( ) )
		{
			StopMusic( );
		}
	}
	gCurrentTrackList = whichList;
}

// A user can choose to listen to music or ambience during gameplay.
int GetActiveTrackList( void )
{
	
	return ( gCurrentTrackList );
}

// A music header contains the names and sizes of all the music files available:
bool LoadMusicHeader( const char *nameOfFile )
{
	if (NoMusicPlease()) return false;
	
	music_hed_there =( PCMAudio_LoadMusicHeader( nameOfFile ) );
	
	return	music_hed_there; 
}

// Headers contain all available streams (probably will be one header per level).
bool LoadStreamHeader( const char *nameOfFile )
{
	if (StreamsDisabled()) return false;
	
	streams_hed_there = PCMAudio_LoadStreamHeader( nameOfFile );

	return streams_hed_there;
}

int GetNumStreamsAvailable( void )
{
	return ( NUM_STREAMS );
}

bool StreamAvailable( int whichStream )
{
	if (StreamsDisabled()) return false;
	
	if ( PCMAudio_GetStreamStatus( whichStream ) == PCM_STATUS_FREE )
	{
		return ( true );
	}
	return ( false );
}

bool StreamAvailable( void )
{

	if (StreamsDisabled()) return false;
	
	if (!(Config::CD() || TEST_FROM_CD))
	{
		int testStreams = 0;
		testStreams = Script::GetInt( 0x62df9442, false ); // checksum 'testStreamsFromHost'
		if ( !testStreams )
		{
			return ( true );
		}
	}

//	return ( PCMAudio_GetStreamStatus( 0 ) == PCM_STATUS_FREE );

	int i;
	for ( i = 0; i < NUM_STREAMS; i++ )
	{
		if ( PCMAudio_GetStreamStatus( i ) == PCM_STATUS_FREE )
		{
			return ( true );
		}
	}
	return ( false );
}

bool StreamExists( uint32 streamChecksum )
{
	// try to find the name in the header file using the checksum:
	uint32 streamName = PCMAudio_FindNameFromChecksum( streamChecksum, EXTRA_CHANNEL );
	return ( streamName != NULL );
}

bool StreamLoading( uint32 streamID )
{
	if (StreamsDisabled()) return false;
	
	if (!(Config::CD() || TEST_FROM_CD))
	{
		int testStreams = 0;
		testStreams = Script::GetInt( 0x62df9442, false ); // checksum 'testStreamsFromHost'
		if ( !testStreams )
		{
			return ( false );
		}
	}

	int channel = GetChannelFromID(streamID);

	if ( channel >= 0 )
	{
		return ( PCMAudio_GetStreamStatus( channel ) == PCM_STATUS_LOADING );
	}

	return ( false );
}

bool StreamPlaying( uint32 streamID )
{
	if (StreamsDisabled()) return false;
	
	int channel = GetChannelFromID(streamID, false);

	if ( channel >= 0 )
	{
		return true;	// Even if it is in loading state
	}

	return false;
}

int GetCurrentTrack()
{
    return ( current_music_track );
}

void SetCurrentTrack( int value )
{
    current_music_track=value;
}

///////////////////////////////////////////////////////////////////////////////////
// Pcm::CStreamFrameAmp

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CStreamFrameAmp::CStreamFrameAmp()
{
	Init();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CStreamFrameAmp::~CStreamFrameAmp()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CStreamFrameAmp::Init()
{
	m_checksum = vNOT_ALLOCATED;
	m_num_samples = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8				CStreamFrameAmp::GetSample(int frame) const
{
	Dbg_Assert(is_allocated());
	Dbg_Assert(frame < m_num_samples);

	return m_frame_amp_samples[frame];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const uint8 *		CStreamFrameAmp::GetSamples() const
{
	Dbg_Assert(is_allocated());

	return m_frame_amp_samples;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

const uint8 *		CStreamFrameAmp::GetSamples(int frame) const
{
	Dbg_Assert(is_allocated());
	Dbg_Assert(frame < m_num_samples);

	return &m_frame_amp_samples[frame];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int					CStreamFrameAmp::GetNumSamples() const
{
	return m_num_samples;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CStreamFrameAmp::load(uint32 stream_checksum)
{
	File::CAsyncFileHandle *p_handle;
	char filename[256];

	// Generate filename from checksum
	sprintf(filename,"fam\\%x.fam", stream_checksum);

	// Open with async blocking for now.  Look to make truly async later.
	p_handle = File::CAsyncFileLoader::sOpen(filename, true);
	if (p_handle)
	{
		m_num_samples = p_handle->GetFileSize();
		if (m_num_samples >= vMAX_SAMPLES)
		{
			Dbg_MsgAssert(0, (".fam file has too many samples: %d", m_num_samples));
			File::CAsyncFileLoader::sClose(p_handle);
			return false;
		}

		p_handle->Read(m_frame_amp_samples, 1, m_num_samples);
		File::CAsyncFileLoader::sClose(p_handle);

		m_checksum = stream_checksum;
		return true;
	}
	else
	{
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CStreamFrameAmp::free()
{
	if (is_allocated())
	{
		// Easiest way to clear, since there is nothing to de-allocate.
		Init();

		return true;
	}
	else
	{
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////////
// Pcm::CStreamFrameAmpManager

CStreamFrameAmp		CStreamFrameAmpManager::s_frame_amp_data[vMAX_BUFFERS];

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CStreamFrameAmp *	CStreamFrameAmpManager::sLoadFrameAmp(uint32 stream_checksum)
{
	for (int i = 0; i < vMAX_BUFFERS; i++)
	{
		if (!s_frame_amp_data[i].is_allocated())
		{
			if (s_frame_amp_data[i].load(stream_checksum))
			{
				return &s_frame_amp_data[i];
			}
			else
			{
				// If we got here, there weren't any free slots
				Dbg_MsgAssert(0, ("Couldn't load StreamFrameAmp %s", Script::FindChecksumName(stream_checksum)));
				return NULL;
			}
		}
	}

	// If we got here, there weren't any free slots
	Dbg_MsgAssert(0, ("Can't load StreamFrameAmp %s: no free slots", Script::FindChecksumName(stream_checksum)));
	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CStreamFrameAmp *	CStreamFrameAmpManager::sGetFrameAmp(uint32 stream_checksum)
{
	for (int i = 0; i < vMAX_BUFFERS; i++)
	{
		if (s_frame_amp_data[i].GetStreamNameCRC() == stream_checksum)
		{
			return &s_frame_amp_data[i];
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CStreamFrameAmpManager::sFreeFrameAmp(uint32 stream_checksum)
{
	for (int i = 0; i < vMAX_BUFFERS; i++)
	{
		if (s_frame_amp_data[i].GetStreamNameCRC() == stream_checksum)
		{
			return s_frame_amp_data[i].free();
		}
	}

	// If we got here, we didn't find it
	Dbg_MsgAssert(0, ("Can't find StreamFrameAmp %s data to free", Script::FindChecksumName(stream_checksum)));
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CStreamFrameAmpManager::sFreeFrameAmp(CStreamFrameAmp *p_fam_data)
{
	return p_fam_data->free();
}

}  // namespace Pcm
