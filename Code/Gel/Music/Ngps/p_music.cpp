// Streaming music off the cd...



#include <eetypes.h>
#include <eekernel.h>
#include <stdio.h>
#include <sifdev.h>
#include <libsdr.h>
#include <sdrcmd.h>
#include <sdmacro.h>
#include <string.h>
#include <sif.h>
#include <sifrpc.h>

#include <libcdvd.h>

#include <core/macros.h>
#include <core/defines.h>

#include <sys/config/config.h>
#include <sys/file/filesys.h>
#include <sys/file/ngps/p_AsyncFilesys.h>
#include <sys/file/ngps/hed.h>
#include <gel/music/ngps/pcm/pcm.h>

// temp for debugging ghost voice:
#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>

// temp for stream sim Rnd( ) call:
#include <core/math/math.h>

#include "p_music.h"

#define TEST_MOTHERFUCKING_CD 0			// Doubt this even works now
#define TEST_PLAY_TIMING	  0

namespace Pcm
{
	
#define MAX_VOL					( 0x3fff )
#define NORMAL_VOL				( 0x3fff >> 1 )

#define MUSIC_PRIORITY			( DEFAULT_ASYNC_PRIORITY - 20 )
#define STREAM_PRIORITY			( DEFAULT_ASYNC_PRIORITY - 10 )		// The lower the number, the higher the priority

int	gIopBuffer = 0;
int	gNonAllignedIopBuffer = 0;

int	gPcmStatus = 0;

// All the Wad file info for the music and audio streams
SWadInfo gWadInfo[ 2 ];

CFileStreamInfo gStreamList [ NUM_FILE_STREAMS ] = {
		{ FILE_STREAM_MUSIC, LOAD_STATE_DONE, PLAY_STATE_DONE, NULL, 0, 0, 0 },					// Music stream
		{ FILE_STREAM_STREAM0, LOAD_STATE_DONE, PLAY_STATE_DONE, NULL, 0, 0, 0 },				// Audio stream 0
		{ FILE_STREAM_STREAM1, LOAD_STATE_DONE, PLAY_STATE_DONE, NULL, 0, 0, 0 },   			// Audio stream 1
		{ FILE_STREAM_STREAM2, LOAD_STATE_DONE, PLAY_STATE_DONE, NULL, 0, 0, 0 }				// Audio stream 2
};

CFileStreamInfo * gpMusicInfo = &gStreamList[FILE_STREAM_MUSIC];
CFileStreamInfo * gpStreamInfo[ NUM_STREAMS ] = { &gStreamList[FILE_STREAM_STREAM0], &gStreamList[FILE_STREAM_STREAM1],
												  &gStreamList[FILE_STREAM_STREAM2] };

unsigned int gStreamVolume = 100;

// Communication to RPC -- ezpcm.irx:
// EzPcm is the function that the EE uses to call the IRX program EzPcm.irx.
// EzPcm.irx is a separate code module that runs on the IOP chip (the playstation one chip).
// The EzPcm module is build from the pcm* files in music\ngps\pcm.
// The files in the music\ngps\bgm folder were used to build a different irx that is
// no longer used for streaming music, so don't worry about the BGM files.

#define MAX_QUEUED_REQUESTS	(10)

// SifCmd packets
static SSifCmdStreamRequestPacket s_cmd_requests[MAX_QUEUED_REQUESTS] __attribute__ ((aligned(16)));
static SSifCmdStreamLoadStatusPacket s_cmd_load_status __attribute__ ((aligned(16)));

static uint32 s_cmd_request_dma_ids[MAX_QUEUED_REQUESTS] = { 0 };
static uint32 s_cmd_load_status_last_id = 0;
static int s_cmd_request_free_index = 0;

static int s_request_id = 0;
static volatile bool s_request_done;
static volatile int s_request_result;
static volatile int s_request_result_id;

// These are set by sifcmds sent directly from the IOP
volatile int sSentStatus = 0;
volatile bool sNewStatus = false;

// Timing test code
#if TEST_PLAY_TIMING
static volatile uint32 test_timing_start;
static volatile uint32 test_timing_end;
static 			int	   test_timing_request_id = -1;
#endif // TEST_PLAY_TIMING

void result_callback(void *p, void *q)
{
	SSifCmdStreamResultPacket *h = (SSifCmdStreamResultPacket *) p;

	s_request_result = h->m_return_value;
	s_request_result_id = h->m_header.opt;
	sSentStatus |= h->m_status_flags;
	sNewStatus = true;
	s_request_done = true;

	//scePrintf("*************** EzPcm() result ID %d result = %d\n", s_request_result_id, s_request_result);

#if TEST_PLAY_TIMING
	if (s_request_result_id == test_timing_request_id)
	{
		test_timing_end = Tmr::GetTimeInUSeconds();
		scePrintf("EzPcm() turnaround time Frame %d Time %d (%x - %x)\n", Tmr::GetVblanks(), (test_timing_end - test_timing_start), test_timing_end, test_timing_start);
	}
#endif // TEST_PLAY_TIMING

	ExitHandler();
}

void status_callback(void *p, void *q)
{
	SSifCmdStreamStatusPacket *h = (SSifCmdStreamStatusPacket *) p;

	sSentStatus |= h->m_status_flags;
	sNewStatus = true;

#if TEST_PLAY_TIMING
	if ((int) h->m_header.opt == test_timing_request_id)
	{
		test_timing_end = Tmr::GetTimeInUSeconds();
		scePrintf("EzPcm() final turnaround time Frame %d Time %d (%x - %x)\n", Tmr::GetVblanks(), (test_timing_end - test_timing_start), test_timing_end, test_timing_start);
	}
#endif // TEST_PLAY_TIMING

	ExitHandler();
}

bool get_status(int & status)
{
	bool new_stat;

	// Disable interrupts just in case we get a sifcmd callback
	DI();
	status = sSentStatus;
	new_stat = sNewStatus;
	sSentStatus = 0;
	sNewStatus = false;
	EI();

	return new_stat;
}

// prototype:
void EzPcm( int command, int data, int data2 = 0, int data3 = 0 );

// IOP
void EzPcm( int command, int data, int data2, int data3 )
{
	//scePrintf("Doing EzPcm request id %d with command %x\n", s_request_id, command);

	#if TEST_PLAY_TIMING
	if (command == EzADPCM_PLAYPRELOADEDMUSIC)
	{
		test_timing_start = Tmr::GetTimeInUSeconds();
		test_timing_request_id = s_request_id;
		Dbg_Message("Doing EzPcm request id %d with command %x", test_timing_request_id, command);
	}
	#endif // TEST_PLAY_TIMING

	int nextFreeIndex = (s_cmd_request_free_index + 1) % MAX_QUEUED_REQUESTS;
	SSifCmdStreamRequestPacket *p_packet = &s_cmd_requests[s_cmd_request_free_index];

	// Make sure this request packet is not still in the Sif DMA queue
	if (s_cmd_request_dma_ids[s_cmd_request_free_index])
	{
		while(sceSifDmaStat(s_cmd_request_dma_ids[s_cmd_request_free_index]) >= 0)
			;
	}

	p_packet->m_header.opt = s_request_id++;
	p_packet->m_request.m_command = command;
	p_packet->m_request.m_param[0] = data;
	p_packet->m_request.m_param[1] = data2;
	p_packet->m_request.m_param[2] = data3;

	s_request_done = false;		// Callback will change it to true
	s_cmd_request_dma_ids[s_cmd_request_free_index] = sceSifSendCmd(STREAM_REQUEST_COMMAND, p_packet, sizeof(*p_packet), 0, 0, 0);

	s_cmd_request_free_index = nextFreeIndex;

	//Dbg_MsgAssert(request_result_id == (request_id -1), ("Result id (%d) differs from request id (%d)", request_result_id, request_id - 1));
}

// Tell IOP that the buffer is done loading
void send_load_done(EFileStreamType streamType, int buffer_num, int bytes_loaded)
{
	Dbg_Assert((buffer_num >= 0) && (buffer_num <= 1));

	// Wait for last send to complete
	if (s_cmd_load_status_last_id)
	{
		while(sceSifDmaStat(s_cmd_load_status_last_id) >= 0)
			;
	}

	s_cmd_load_status.m_stream_num = streamType - 1;
	s_cmd_load_status.m_buffer_num = buffer_num;
	s_cmd_load_status.m_bytes_loaded = bytes_loaded;

	//FlushCache(0);

	s_cmd_load_status_last_id = sceSifSendCmd(STREAM_LOAD_STATUS_COMMAND, &s_cmd_load_status, sizeof(s_cmd_load_status), 0, 0, 0);
	//scePrintf("Audio stream %d buffer #%d finished loading\n", streamType, buffer_num);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#include <libcdvd.h>

void InitCDPlease( void )
{
	if (Config::CD())
	{
		// already done since we're building in CD mode...
		return;
	}
	
	#if TEST_MOTHERFUCKING_CD
	static bool initializedAndShit = false;
	if ( !initializedAndShit )
	{
		sceCdInit(SCECdINIT);
		// this may oughtta be changed to SCDCdDVD...
		// Actually, a smart thing to do would be to have this defined somewhere
		// and switched between SCECdCD or SCDCdDVD depending on the build?
		//sceCdMmode(SCECdCD);
		sceCdMmode(SCECdDVD);
		initializedAndShit = true;
	}
	#endif
}	

static bool CheckForCDErrors( void )
{
	int cdErr;
	cdErr = sceCdGetError( );
	switch ( cdErr )
	{
		case ( SCECdErNO ): // no error
			break;
		case ( SCECdErTRMOPN ):	// cover opened during playback
		case ( SCECdErREAD ):	// read error
			return ( true );
		default:
			printf( "cd err %d\n", cdErr );
			break;
	}
	return ( false );
}

///////////////////////////////////////////////////////////////////////////////////////
// CFileStreamInfo
//

// Preloads the SPU audio buffers (because this can take a few frames)
void			CFileStreamInfo::preload_spu_buffers()
{
	// Check if it is music or normal audio
	if (IsMusicStream())
	{
		// got initial music buffers loaded - start preloading SPU buffers
		EzPcm( EzADPCM_PRELOADMUSIC, m_fileSize / 2 ); // because there are two channels L and R
	}
	else
	{
		// got initial stream buffers loaded - start preloading SPU buffers
		EzPcm( EzADPCM_PRELOADSTREAM | GetAudioStreamIndex(), m_fileSize );
	}
}

// Starts playing a preloaded audio stream
void			CFileStreamInfo::start_preloaded_audio()
{
	gPcmStatus &= ~PCMSTATUS_LOAD_AUDIO( m_streamType );
	gPcmStatus |= PCMSTATUS_AUDIO_PLAYING( m_streamType );

	// Check if it is music or normal audio
	if (IsMusicStream())
	{
		// got initial music buffers loaded on the SPU - start it playing:
		EzPcm( EzADPCM_PLAYPRELOADEDMUSIC, 0 );
	}
	else
	{
		// got initial stream buffers loaded on the SPU - start it playing:
		EzPcm( EzADPCM_PLAYPRELOADEDSTREAM | GetAudioStreamIndex(), m_volume, m_pitch );
	}
}

// Starts playing the audio stream
void			CFileStreamInfo::start_audio()
{
	gPcmStatus &= ~PCMSTATUS_LOAD_AUDIO( m_streamType );
	gPcmStatus |= PCMSTATUS_AUDIO_PLAYING( m_streamType );

	// Check if it is music or normal audio
	if (IsMusicStream())
	{
		// got initial music buffers loaded - start it playing:
		EzPcm( EzADPCM_PLAYMUSIC, m_fileSize / 2 ); // because there are two channels L and R
	}
	else
	{
		// got initial stream buffers loaded - start it playing:
		EzPcm( EzADPCM_PLAYSTREAM | GetAudioStreamIndex(), m_fileSize, m_volume, m_pitch );
	}
}

// Starts the load of an audio stream chunk
void			CFileStreamInfo::load_chunk(int buffer)
{
	if ( !mp_fileHandle )
	{
		Dbg_MsgAssert( 0,( "s'it, yo... no file opened for stream index %d", GetAudioStreamIndex() ));
		return;
	}

	// Check if it is music or normal audio
	if (IsMusicStream())
	{
		void *p_iop_addr = (void *) (gIopBuffer + ( MUSIC_IOP_BUFFER_SIZE * buffer ));
		mp_fileHandle->Read( p_iop_addr, 1, MUSIC_IOP_BUFFER_SIZE );
		//scePrintf("Reading music buffer %d from disk to IOP buffer %x of size %d\n", buffer, p_iop_addr, MUSIC_IOP_BUFFER_SIZE);

		m_offset += MUSIC_IOP_BUFFER_SIZE;
	}
	else
	{
		int whichStream =  GetAudioStreamIndex();
		void *p_iop_addr = (void *) (gIopBuffer + STREAM_IOP_OFFSET( whichStream ) + ( STREAM_HALF_IOP_BUFFER_SIZE * buffer ));
		mp_fileHandle->Read( p_iop_addr, 1, STREAM_HALF_IOP_BUFFER_SIZE );

		m_offset += STREAM_HALF_IOP_BUFFER_SIZE;
	}
}

// Checks to see if the IOP module is requesting another stream chunk
ELoadState		CFileStreamInfo::check_for_load_requests()
{
	ELoadState state = LOAD_STATE_IDLE;

	if ( gPcmStatus & PCMSTATUS_NEED_AUDIO_BUFFER_0( m_streamType ) )
	{
		load_chunk( 0 );
		state = LOAD_STATE_LOADING0;
		gPcmStatus &= ~PCMSTATUS_NEED_AUDIO_BUFFER_0( m_streamType );
	}
	else if ( gPcmStatus & PCMSTATUS_NEED_AUDIO_BUFFER_1( m_streamType ) )
	{
		load_chunk( 1 );
		state = LOAD_STATE_LOADING1;
		gPcmStatus &= ~PCMSTATUS_NEED_AUDIO_BUFFER_1( m_streamType );
	}

	return state;
}

// Checks to see if loading of stream chunk is done
// errno will be non-zero if we were busy AND there was an error
bool			CFileStreamInfo::is_chunk_load_done(int & errno)
{
	Dbg_Assert(mp_fileHandle);

	// Clear error var
	errno = 0;

	// Check for delay errors
	// Garrett: We should only be concerned if we get a request for the buffer we are currently loading.  The
	// only problem to this is the fact that the IOP doesn't check to see if a buffer is loaded at all.  We
	// must add this so we can get rid of the conservative check of making sure we don't get ANY new load requests.
//	if ( gPcmStatus & ( PCMSTATUS_NEED_AUDIO_BUFFER_0( m_streamType ) | PCMSTATUS_NEED_AUDIO_BUFFER_1( m_streamType ) ) )
	int load_flag = (m_loadState == LOAD_STATE_LOADING0) ? PCMSTATUS_NEED_AUDIO_BUFFER_0( m_streamType ) : PCMSTATUS_NEED_AUDIO_BUFFER_1( m_streamType );
	if ( gPcmStatus & load_flag )
	{
		int cur_buffer_load = (int) (m_loadState - LOAD_STATE_LOADING0);
		Dbg_Message("Load State: %d", m_loadState);
		Dbg_MsgAssert( 0, ( "If repeatable, tell Garrett that audio #%d buffer #%d didn't load on time. Status: %x", m_streamType, cur_buffer_load, gPcmStatus ));
		printf( "If repeatable, tell Garrett audio #%d buffer #%d didn't load on time -- must increase IOP streaming buf size. Status %x", m_streamType, cur_buffer_load, gPcmStatus );
		errno = (IsMusicStream()) ? -1 : m_streamType;
	}

	if (!mp_fileHandle->IsBusy())
	{
		int cur_buffer_load = (int) (m_loadState - LOAD_STATE_LOADING0);

		// Tell IOP that we are done
		send_load_done(m_streamType, cur_buffer_load, IsMusicStream() ? MUSIC_IOP_BUFFER_SIZE : STREAM_HALF_IOP_BUFFER_SIZE);

		return true;
	}

	// Return that we aren't done
	return false;
}

// returns true and closes fine if we read the whole file
bool			CFileStreamInfo::file_finished()
{
	if ( m_offset < m_fileSize )
	{
		return false;
	} else {
		Dbg_Assert(mp_fileHandle);		// It shouldn't be closed yet

		File::CAsyncFileLoader::sClose( mp_fileHandle );
		mp_fileHandle = NULL;

		return true;
	}
}

// Starts an audio stream
bool			CFileStreamInfo::StartStream(File::SHedFile *pHedFile, bool play)
{
	// make sure CD isn't being used by another system:
	// Garrett: This should go away
	File::StopStreaming( );

	// Make sure these load flags aren't set
	if ( gPcmStatus & ( PCMSTATUS_NEED_AUDIO_BUFFER_0( m_streamType ) | PCMSTATUS_NEED_AUDIO_BUFFER_1( m_streamType ) ) )
	{
		Dbg_Message("Load State: %d", m_loadState);
		Dbg_MsgAssert( 0, ( "Start Stream on audio #%d still has load buffer flag. Status: %x", m_streamType, gPcmStatus ));
		printf( "Start Stream on audio #%d still has load buffer flag. Status %x", m_streamType, gPcmStatus );
	}

	// Find correct wad filename
	char *p_wadName;
	unsigned int wadLsn;
	int priority;

	if (IsMusicStream())
	{
		wadLsn = gWadInfo[MUSIC_CHANNEL].m_lsn;
		p_wadName = gWadInfo[MUSIC_CHANNEL].m_fileName;
		priority = MUSIC_PRIORITY;
	}
	else
	{
		wadLsn = gWadInfo[EXTRA_CHANNEL].m_lsn;
		p_wadName = gWadInfo[EXTRA_CHANNEL].m_fileName;
		priority = STREAM_PRIORITY;
	}

	// load Audio wad:
	if ( mp_fileHandle )
	{
		Dbg_Message( "What?  Audio file opened still: %d", mp_fileHandle );
		Dbg_MsgAssert( 0,( "What?  Stream file still (or already) opened." ));
		return false;
	}

	if (Config::CD() || TEST_MOTHERFUCKING_CD)
	{
		mp_fileHandle = File::CPs2AsyncFileLoader::sRawOpen(wadLsn, false, priority);
	}
	else
	{
		if (pHedFile->HasNoWad())
		{
			char local_file[256];

			strcpy(local_file, pHedFile->p_filename);
			if (IsMusicStream())
			{
				strcat(local_file, ".ivg");
			}
			else
			{
				strcat(local_file, ".vag");
			}
			mp_fileHandle = File::CAsyncFileLoader::sOpen(&local_file[1], false, priority);
		}
		else
		{
			mp_fileHandle = File::CAsyncFileLoader::sOpen(p_wadName, false, priority);
		}
	}

	if ( !mp_fileHandle )
	{
		Dbg_MsgAssert( 0,( "Play Audio: couldn't find audio wad file %s", p_wadName ));
		printf( "play audio failed to find audio wad file %s\n", p_wadName );
		return false;
	}

	m_offset = 0;
	mp_fileHandle->SetDestination(File::MEM_IOP);

	if (pHedFile->HasNoWad())
	{
		// .ivg files don't have the VAG header
		if (!IsMusicStream()) mp_fileHandle->Seek( SIZE_OF_VAGHEADER, SEEK_SET );
	}
	else
	{
		mp_fileHandle->Seek( pHedFile->Offset, SEEK_SET );
	}

	m_fileSize = pHedFile->GetFileSize();
	m_loadState = LOAD_STATE_LOADING0;
	m_playState = (play) ? PLAY_STATE_START : PLAY_STATE_PRELOAD_EE;

	// Start loading the data, but don't play it yet
	load_chunk(0);

	// Change the state
	gPcmStatus |= PCMSTATUS_LOAD_AUDIO(m_streamType);

	return true;
}

// Plays a preloaded audio stream
bool			CFileStreamInfo::PlayStream()
{
	switch (m_playState)
	{
	case PLAY_STATE_DONE:
	case PLAY_STATE_PLAYING:
		return false;

	case PLAY_STATE_PAUSED:
		// Not implemented yet
		Dbg_MsgAssert(0, ("Playing a paused stream not implemented yet"));
		return false;

	case PLAY_STATE_PRELOAD_EE:
		Dbg_MsgAssert(0, ("Starting a preloaded stream that isn't finished preloading"));
		return false;

	case PLAY_STATE_PRELOAD_IOP:
	case PLAY_STATE_STOP:
	case PLAY_STATE_START:
		switch (m_loadState)
		{
		case LOAD_STATE_DONE:
		case LOAD_STATE_IDLE:
		case LOAD_STATE_LOADING1:			// Since it may already be loading the next section
			m_playState = PLAY_STATE_PLAYING;

			//start_audio();
			start_preloaded_audio();

			break;

		case LOAD_STATE_LOADING0:
			m_playState = PLAY_STATE_START;
			Dbg_MsgAssert(0, ("Starting a preloaded stream that isn't finished preloading. Load state: %d", m_loadState));

			break;
		}

		break;
	}

	return true;
}

// Stops an audio stream
bool			CFileStreamInfo::StopStream()
{
	int status = IsMusicStream() ? PCMAudio_GetMusicStatus( ) : PCMAudio_GetStreamStatus( GetAudioStreamIndex() );

	// Check states
	if ( status == PCM_STATUS_PLAYING )
	{
		if (IsMusicStream())
		{
			EzPcm( EzADPCM_STOPMUSIC, 0 );
		}
		else
		{
			EzPcm( EzADPCM_STOPSTREAM, GetAudioStreamIndex() );
		}
	}

	// Close async file
	if ( mp_fileHandle )
	{
		m_loadState = LOAD_STATE_DONE;
		File::CAsyncFileLoader::sClose( mp_fileHandle );
		mp_fileHandle = NULL;
	}

	// Clear flags
	PCMAudio_UpdateStatus();
	m_playState = PLAY_STATE_DONE;
	gPcmStatus &= ~( PCMSTATUS_NEED_AUDIO_BUFFER_0( m_streamType ) |
					 PCMSTATUS_NEED_AUDIO_BUFFER_1( m_streamType ) |
					 PCMSTATUS_LOAD_AUDIO( m_streamType ) |
					 PCMSTATUS_AUDIO_PLAYING( m_streamType ) |
					 PCMSTATUS_AUDIO_READY(m_streamType) );

	return true;
}

// Update: executed on each stream every frame
// returns error number on error, 0 otherwise...
int				CFileStreamInfo::Update()
{
	int errno = 0;

	switch (m_loadState)
	{
	case LOAD_STATE_IDLE:
		// Make sure IOP didn't drop out (would probably be a state problem)
		//Dbg_MsgAssert(gPcmStatus & PCMSTATUS_AUDIO_PLAYING( m_streamType ), ("Generic stream %d not playing while in LOAD_STATE_IDLE", m_streamType));

		m_loadState = check_for_load_requests();

		break;

	case LOAD_STATE_LOADING0:
	case LOAD_STATE_LOADING1:
		// Make sure IOP didn't drop out (would probably be a state problem)
		//Dbg_MsgAssert(gPcmStatus & PCMSTATUS_AUDIO_PLAYING( m_streamType ), ("Generic stream %d not playing while in LOAD_STATE_LOADING", m_streamType));

		if (!is_chunk_load_done(errno))
		{
			if (errno != 0)
			{
				return errno;
			}

			break; // still busy loading
		}

		// Check if it needs starting
		if (m_playState == PLAY_STATE_START)
		{
			m_playState = PLAY_STATE_PLAYING;

			start_audio();
		}
		else if (m_playState == PLAY_STATE_PRELOAD_EE)
		{
			m_playState = PLAY_STATE_PRELOAD_IOP;

			preload_spu_buffers();
		}

		// Change state and possibly close file
		if (file_finished())
		{
			m_loadState = LOAD_STATE_DONE;
		} else {
			m_loadState = LOAD_STATE_IDLE;
		}

		break;

	case LOAD_STATE_DONE:
		// Just wait for the audio to finish playing
		if ( !(gPcmStatus & PCMSTATUS_AUDIO_PLAYING( m_streamType )) )
		{
			m_playState = PLAY_STATE_DONE;
			// Clear out any old sent load request flags
			gPcmStatus &= ~( PCMSTATUS_NEED_AUDIO_BUFFER_0( m_streamType ) | PCMSTATUS_NEED_AUDIO_BUFFER_1( m_streamType ) );
		}

		break;
	}

	return errno;
}

#define DEBUG_LOAD_FLAGS 0

#if DEBUG_LOAD_FLAGS
int old_load_state[NUM_FILE_STREAMS];
int old_play_state[NUM_FILE_STREAMS];
#endif

// Combine the IOP sent status into gPcmStatus
void PCMAudio_UpdateStatus()
{
	int new_status;

	if (get_status(new_status))
	{
		for ( int i = 0; i < NUM_FILE_STREAMS; i++ )
		{
			gPcmStatus &= ~( PCMSTATUS_AUDIO_PLAYING( i ) | PCMSTATUS_AUDIO_READY( i ) );
		}
		gPcmStatus |= new_status;
	}
}


// returns error number on error, 0 otherwise...
int PCMAudio_Update( void )
{
	int i;

#if DEBUG_LOAD_FLAGS
	for (int i = 0; i < NUM_FILE_STREAMS; i++)
	{
		old_load_state[i] = gStreamList[ i ].m_loadState;
		old_play_state[i] = gStreamList[ i ].m_playState;
	}
#endif

	// Clear the flags
	PCMAudio_UpdateStatus();

	static bool print_panic = true;		// Using this now until we can clear the PANIC flag on the IOP
	if ((gPcmStatus & PCMSTATUS_PANIC) && print_panic)
	{
		print_panic = false;
		scePrintf("ezpcm.irx module set PANIC flag: Look at its last error message.\n");
		//Dbg_MsgAssert(0, ("ezpcm.irx module set PANIC flag: Look at its last error message."));
	}

	// Make sure there aren't any CD errors
	if (0 && (Config::CD() || TEST_MOTHERFUCKING_CD) && CheckForCDErrors( ))
	{
		return ( -1 );
	}

	// Go through each stream and update
	for (i = 0; i < NUM_FILE_STREAMS; i++)
	{
		CFileStreamInfo *pInfo = &gStreamList[ i ];

		int ret_val = pInfo->Update();

		if (ret_val != 0)
		{
			return ret_val;
		}
	}

#if 0
		// If file loads go out of sync over a busy network, keep it from crashing:
		if ( gMusicInfo.mp_fileHandle && !( gPcmStatus & PCMSTATUS_MUSIC_PLAYING ) )
		{
			File::CAsyncFileLoader::sClose( gMusicInfo.mp_fileHandle );
			gMusicInfo.mp_fileHandle = NULL;
			Dbg_Message( "Symptom of a busy network: Closing music file." );
		}
		for ( i = 0; i < NUM_STREAMS; i++ )
		{
			if ( gStreamInfo[ i ].mp_fileHandle && !( gPcmStatus & (PCMSTATUS_STREAM_PLAYING( i ) | PCMSTATUS_LOAD_STREAM( i ))) )
			{
				File::CAsyncFileLoader::sClose( gStreamInfo[i].mp_fileHandle );
				gStreamInfo[ i ].mp_fileHandle = NULL;
				Dbg_Message( "Symptom of a busy network: Closing stream file." );
			}
		}
#endif

#if 0
	Dbg_MsgAssert(!(gPcmStatus & ( PCMSTATUS_NEED_MUSIC_BUFFER_0 | PCMSTATUS_NEED_MUSIC_BUFFER_1 |
						PCMSTATUS_NEED_STREAM0_BUFFER_0 | PCMSTATUS_NEED_STREAM0_BUFFER_1 |
						PCMSTATUS_NEED_STREAM1_BUFFER_0 | PCMSTATUS_NEED_STREAM1_BUFFER_1 |
						PCMSTATUS_NEED_STREAM2_BUFFER_0 | PCMSTATUS_NEED_STREAM2_BUFFER_1 )), ("Skipped a buffer load: status %x", gPcmStatus));
#else
	if (gPcmStatus & ( PCMSTATUS_NEED_MUSIC_BUFFER_0 | PCMSTATUS_NEED_MUSIC_BUFFER_1 |
						PCMSTATUS_NEED_STREAM0_BUFFER_0 | PCMSTATUS_NEED_STREAM0_BUFFER_1 |
						PCMSTATUS_NEED_STREAM1_BUFFER_0 | PCMSTATUS_NEED_STREAM1_BUFFER_1 |
						PCMSTATUS_NEED_STREAM2_BUFFER_0 | PCMSTATUS_NEED_STREAM2_BUFFER_1 ))
	{
		Dbg_Message("************Skipped a buffer load: status %x", gPcmStatus);
#if DEBUG_LOAD_FLAGS
		for (int i = 0; i < NUM_FILE_STREAMS; i++)
		{
			Dbg_Message("Audio Stream %d: load state = %d", i, gStreamList[ i ].m_loadState);
			Dbg_Message("Audio Stream %d: play state = %d", i, gStreamList[ i ].m_playState);
			Dbg_Message("Audio Stream %d: old load state = %d", i, old_load_state[i]);
			Dbg_Message("Audio Stream %d: old play state = %d", i, old_play_state[i]);
		}
#endif
	}
#endif

	return ( 0 );
}

/* ------------
   Initialize rpc  -- Just call once at the very beginning!
   ------------ */
void PCMAudio_Init( void )
{
	Dbg_MsgAssert(NUM_FILE_STREAMS == (NUM_STREAMS + 1),
				  ("Number of file slots allocated for audio streams (%d) different than number of IOP audio streams (%d)", NUM_FILE_STREAMS - 1, NUM_STREAMS));

	// Check to make sure stream array is filled
	for (int i = 0; i < NUM_STREAMS; i++)
	{
		Dbg_Assert(gpStreamInfo[i]);
	}

	DI();

	printf( "Setting up Sif Cmd with EzPCM Streaming IRX...\n" );
	// No longer need to call sceSifSetCmdBuffer() since we share it with async filesys
//	static sceSifCmdData cmdbuffer[NUM_COMMAND_HANDLERS];
//	sceSifSetCmdBuffer( &cmdbuffer[0], NUM_COMMAND_HANDLERS);

	sceSifAddCmdHandler(STREAM_RESULT_COMMAND, result_callback, NULL );
	sceSifAddCmdHandler(STREAM_STATUS_COMMAND, status_callback, NULL );

	EI();

	// if CD system hasn't been initialized,
	// but you want to test music from CD,
	// this will do it...
	InitCDPlease( );


	Dbg_MsgAssert( !gIopBuffer,( "What the fuck - buffer already exists?" ));

	FlushCache( 0 );
	
	PCMAudio_GetIopMemory( );
	
	EzPcm( EzADPCM_INIT, gIopBuffer );
	//EzPcm( EzADPCM_SETSTREAMGLOBVOL, gStreamVolume );
} // end of PCMAudio_Init( )

int PCMAudio_GetIopMemory( void )
{
	
	
	if ( !gNonAllignedIopBuffer )
	{
		// streaming buffers:
		gNonAllignedIopBuffer = ( int )sceSifAllocIopHeap( TOTAL_IOP_BUFFER_SIZE_NEEDED + ALLIGN_REQUIREMENT );
		gIopBuffer = gNonAllignedIopBuffer + ALLIGN_REQUIREMENT - ( gNonAllignedIopBuffer & ( ALLIGN_REQUIREMENT - 1 ) );
	
		if ( !gNonAllignedIopBuffer )
		{
			Dbg_MsgAssert( 0,( "Failed to allocate IOP memory - %d k\n", TOTAL_IOP_BUFFER_SIZE_NEEDED / 1024 ));
			return 0;
		}
	}
	return ( gIopBuffer );
}
void PCMAudio_SetStreamGlobalVolume( unsigned int volume )
{
	gStreamVolume = volume;
	//if ( gPcmStatus ) // make sure we've initialized PCM stuff:
	//{
	//	EzPcm( EzADPCM_SETSTREAMGLOBVOL, gStreamVolume );
	//}
}

bool PCMAudio_SetMusicVolume( float volume )
{
	int vol = ( int )PERCENT( volume, MAX_VOL );
	if ( vol > MAX_VOL )
		vol = MAX_VOL;

	EzPcm( EzADPCM_SETMUSICVOL, ( vol << 16 ) | vol );
	return true;
}// end of PCMAudio_SetMusicVolume( )


/*int PCMAudio_SetStreamVolumeAndPitch( float volumeL, float volumeR, float fPitch, int whichStream )
{
	Dbg_Message( "pitch %d", pitch );
   	return ( EzPcm( EzADPCM_SETSTREAMVOLANDPITCH | whichStream, ( ( volL << 16 ) | volR ), pitch ) );
}// end of PCMAudio_SetVolumeAndPitch( )
*/

// Should check before calling this to make sure the sound is playing and has the uniqueID of the sound you are
// trying to adjust:
bool PCMAudio_SetStreamVolume( float volumeL, float volumeR, int whichStream )
{
	int volL = ( int )PERCENT( volumeL, gStreamVolume );
	volL = ( int )PERCENT( volL, NORMAL_VOL );
	if ( volL > MAX_VOL )
		volL = MAX_VOL;
	else if ( volL < -MAX_VOL )
		volL = -MAX_VOL;
	int volR;
	if ( volumeR == volumeL )
	{
		volR = volL;
	}
	else
	{
		volR = ( int )PERCENT( volumeR, gStreamVolume );
		volR = ( int )PERCENT( volR, NORMAL_VOL );
		if ( volR > MAX_VOL )
			volR = MAX_VOL;
		else if ( volR < -MAX_VOL )
			volR = -MAX_VOL;
	}

	// for phasing effects (of sounds behind you), set the volume
	// to 0x7fff
	if ( volL < 0 )
	{
		volL = 0x7fff + volL;
	}
	if ( volR < 0 )		// Just in case we start phase shifting the right side instead
	{
		volR = 0x7fff + volR;
	}

	gpStreamInfo[ whichStream ]->m_volume = ( ( (volL & 0x7FFF) << 16 ) | (volR & 0x7FFF) );

	if (gpStreamInfo[ whichStream ]->m_playState == PLAY_STATE_PLAYING)
	{
		EzPcm( EzADPCM_SETSTREAMVOL | whichStream, gpStreamInfo[ whichStream ]->m_volume);
	}

	return true;
}// end of PCMAudio_SetVolume( )

// Currently, this only works when the stream isn't playing
bool PCMAudio_SetStreamPitch( float fPitch, int whichStream )
{
	if ( fPitch > 100.0f )
	{
		fPitch = 100.0f;
	}
	int pitch;
	pitch = ( int ) PERCENT( fPitch, ( float )DEFAULT_PITCH ); 
	if ( pitch == 0 )
	{
		Dbg_Message( "Trying to set stream to zero pitch." );
		return ( false );
	}

	if (gpStreamInfo[ whichStream ]->m_playState == PLAY_STATE_PLAYING)
	{
		Dbg_MsgAssert(0, ("Can't change pitch of stream %d while it is playing", whichStream));
		return false;
	}
	else
	{
		gpStreamInfo[ whichStream ]->m_pitch = pitch;
		return true;
	}
}

bool PCMAudio_PlayMusicTrack( const char *filename )
{
	if ( PCMAudio_GetMusicStatus( ) != PCM_STATUS_FREE )
	{
		Dbg_MsgAssert( 0 , ( "Playing new track without stopping the first track." ) );
		return ( false );
	}

	if (!(Config::CD() || TEST_MOTHERFUCKING_CD))
	{
		int testMusic = 0;
		testMusic = Script::GetInteger( 0x47ac7ba5 ); // checksum 'testMusicFromHost'
		if ( !testMusic )
		{
			return ( false );
		}
	}

	// make sure CD is not in use by another system:
	File::StopStreaming( );

	if ( !gWadInfo[MUSIC_CHANNEL].mp_hed )
	{
		Dbg_Message( "Music header not loaded, can't play track %s", filename );
		return ( false );
	}

	File::SHedFile *pHed = FindFileInHed( filename, gWadInfo[MUSIC_CHANNEL].mp_hed );
	if ( !pHed )
	{
		Dbg_Message( "Track %d not found in music header.", filename );
		return ( false );
	}

	// Start the actual stream
	return gpMusicInfo->StartStream(pHed, true);
} // end of PCMAudio_PlayMusicTrack( )


bool PCMAudio_PlayMusicStream( uint32 checksum )
{
	if ( PCMAudio_GetMusicStatus( ) != PCM_STATUS_FREE )
	{
		Dbg_MsgAssert( 0 , ( "Playing music stream without stopping the last music track." ) );
		return ( false );
	}

	if (!(Config::CD() || TEST_MOTHERFUCKING_CD))
	{
		int testMusic = 0;
		testMusic = Script::GetInteger( 0x47ac7ba5 ); // checksum 'testMusicFromHost'
		if ( !testMusic )
		{
			return ( false );
		}
	}

	// make sure CD is not in use by another system:
	File::StopStreaming( );

	if ( !gWadInfo[MUSIC_CHANNEL].mp_hed )
	{
		Dbg_Message( "Music header not loaded, can't play music stream %s", Script::FindChecksumName(checksum) );
		return ( false );
	}

	File::SHedFile *pHed = FindFileInHedUsingChecksum( checksum, gWadInfo[MUSIC_CHANNEL].mp_hed );
	if ( !pHed )
	{
		Dbg_Message( "Music Stream %s not found in music header.", Script::FindChecksumName(checksum) );
		return ( false );
	}

	// Start the actual stream
	return gpMusicInfo->StartStream(pHed, true);
}

bool PCMAudio_PlayStream( uint32 checksum, int whichStream, float volumeL, float volumeR, float fPitch )
{
	Dbg_MsgAssert( whichStream < NUM_STREAMS, ( "Stream is out of range" ) );

	if ( PCMAudio_GetStreamStatus( whichStream ) != PCM_STATUS_FREE )
	{
		Dbg_Message("Stream %d not free: status is %x", whichStream, PCMAudio_GetStreamStatus( whichStream ));
		return ( false );
	}

	if (!(Config::CD() || TEST_MOTHERFUCKING_CD))
	{
		int testStreams = 0;
		testStreams = Script::GetInteger( 0x62df9442 ); // checksum 'testStreamsFromHost'
		if ( !testStreams )
		{
			return ( false );
		}
	}	
	
	//Dbg_Message("Playing stream %x on %d", checksum, whichStream);

	// load from vag wad:
	if ( !gWadInfo[EXTRA_CHANNEL].mp_hed )
	{
		//Dbg_Message( "Stream header not loaded, can't play track %s", filename );
		return ( false );
	}
	File::SHedFile *pHed = FindFileInHedUsingChecksum( checksum, gWadInfo[EXTRA_CHANNEL].mp_hed );
	if ( !pHed )
	{
		Dbg_Message( "Stream %s not found in stream header.", Script::FindChecksumName(checksum) );
		return ( false );
	}

	// Set pitch and volume
	PCMAudio_SetStreamPitch(fPitch, whichStream);
	PCMAudio_SetStreamVolume(volumeL, volumeR, whichStream);

	// Start the actual stream
	return gpStreamInfo[ whichStream ]->StartStream(pHed, true);
} // end of PCMAudio_PlayStream( )

void PCMAudio_StopMusic( bool waitPlease )
{
	gpMusicInfo->StopStream();
}

void PCMAudio_StopStream( int whichStream, bool waitPlease )
{
	gpStreamInfo[ whichStream ]->StopStream();
}

void PCMAudio_StopStreams( void )
{
	int i;
	for ( i = 0; i < NUM_STREAMS; i++ )
	{
		PCMAudio_StopStream( i, true );
	}
}

// Get a stream loaded into a buffer, but don't play yet
bool PCMAudio_PreLoadStream( uint32 checksum, int whichStream )
{
	Dbg_MsgAssert( whichStream < NUM_STREAMS, ( "Stream is out of range" ) );

	if ( PCMAudio_GetStreamStatus( whichStream ) != PCM_STATUS_FREE )
	{
		return ( false );
	}

	if (!(Config::CD() || TEST_MOTHERFUCKING_CD))
	{
		int testStreams = 0;
		testStreams = Script::GetInteger( 0x62df9442 ); // checksum 'testStreamsFromHost'
		if ( !testStreams )
		{
			return ( false );
		}
	}

	// load from vag wad:
	if ( !gWadInfo[EXTRA_CHANNEL].mp_hed )
	{
		//Dbg_Message( "Stream header not loaded, can't play track %s", filename );
		return ( false );
	}
	File::SHedFile *pHed = FindFileInHedUsingChecksum( checksum, gWadInfo[EXTRA_CHANNEL].mp_hed );
	if ( !pHed )
	{
		//Dbg_Message( "Track %d not found in stream header.", filename );
		return ( false );
	}

	// Start the actual stream
	return gpStreamInfo[ whichStream ]->StartStream(pHed, false);
}

// Returns true if preload done.  Assumes that caller is calling this on a preloaded, but not yet played, stream.
// The results are meaningless otherwise.
bool PCMAudio_PreLoadStreamDone( int whichStream )
{
	Dbg_MsgAssert( whichStream < NUM_STREAMS, ( "Stream is out of range" ) );
	Dbg_MsgAssert( (gpStreamInfo[whichStream]->m_playState == PLAY_STATE_PRELOAD_EE) ||
				   (gpStreamInfo[whichStream]->m_playState == PLAY_STATE_PRELOAD_IOP), ( "PreLoadStreamDone(): This stream on channel %d is either playing or wasn't preloaded.  Load state %d.  Play State %d. IOP flags %x",
																						 whichStream, gpStreamInfo[whichStream]->m_loadState, gpStreamInfo[whichStream]->m_playState, gPcmStatus ) );

	//return gpStreamInfo[whichStream]->m_loadState != LOAD_STATE_LOADING0;
	if (gPcmStatus & PCMSTATUS_STREAM_READY(whichStream))
	{
		Dbg_Message("PCMAudio_PreLoadStreamDone true for stream channel %d.  Load state %d.  Play State %d. IOP flags %x", whichStream,
					gpStreamInfo[whichStream]->m_loadState, gpStreamInfo[whichStream]->m_playState, gPcmStatus);
	}
	return (gPcmStatus & PCMSTATUS_STREAM_READY(whichStream)) && (gpStreamInfo[whichStream]->m_playState == PLAY_STATE_PRELOAD_IOP);
}

// Tells a preloaded stream to start playing.  Must call PCMAudio_PreLoadStreamDone() first to guarantee that
// it starts immediately.
bool PCMAudio_StartPreLoadedStream( int whichStream, float volumeL, float volumeR, float pitch )
{
	Dbg_MsgAssert( whichStream < NUM_STREAMS, ( "Stream is out of range" ) );

	// Set pitch and volume
	PCMAudio_SetStreamPitch(pitch, whichStream);
	PCMAudio_SetStreamVolume(volumeL, volumeR, whichStream);

	// Start playing the actual stream
	return gpStreamInfo[ whichStream ]->PlayStream();
}

bool PCMAudio_PreLoadMusicStream( uint32 checksum )
{
	if ( PCMAudio_GetMusicStatus( ) != PCM_STATUS_FREE )
	{
		Dbg_MsgAssert( 0 , ( "Preloading music stream without stopping the last music track." ) );
		return ( false );
	}

	if (!(Config::CD() || TEST_MOTHERFUCKING_CD))
	{
		int testMusic = 0;
		testMusic = Script::GetInteger( 0x47ac7ba5 ); // checksum 'testMusicFromHost'
		if ( !testMusic )
		{
			return ( false );
		}
	}

	// load from vag wad:
	if ( !gWadInfo[MUSIC_CHANNEL].mp_hed )
	{
		//Dbg_Message( "Stream header not loaded, can't play track %s", filename );
		return ( false );
	}
	File::SHedFile *pHed = FindFileInHedUsingChecksum( checksum, gWadInfo[MUSIC_CHANNEL].mp_hed );
	if ( !pHed )
	{
		Dbg_Message( "Music Stream %s not found in music header.", Script::FindChecksumName(checksum) );
		return ( false );
	}

	// Start the actual stream
	return gpMusicInfo->StartStream(pHed, false);
}

bool PCMAudio_PreLoadMusicStreamDone( void )
{
	Dbg_MsgAssert( (gpMusicInfo->m_playState == PLAY_STATE_PRELOAD_EE) ||
				   (gpMusicInfo->m_playState == PLAY_STATE_PRELOAD_IOP), ( "PreLoadMusicStreamDone(): This stream is either playing or wasn't preloaded." ) );

	//return gpMusicInfo->m_loadState != LOAD_STATE_LOADING0;
	return (gPcmStatus & PCMSTATUS_MUSIC_READY) && (gpMusicInfo->m_playState == PLAY_STATE_PRELOAD_IOP);
}

bool PCMAudio_StartPreLoadedMusicStream( void )
{
	// Start playing the actual stream
	return gpMusicInfo->PlayStream();
}

int PCMAudio_GetMusicStatus( void )
{
	

	if ( gPcmStatus & PCMSTATUS_LOAD_MUSIC )
	{
		return ( PCM_STATUS_LOADING );
	}
	else if ( gPcmStatus & PCMSTATUS_MUSIC_PLAYING )
	{
		return ( PCM_STATUS_PLAYING );
	}
	else if ( gpMusicInfo->m_playState == PLAY_STATE_STOP)
	{
		return ( PCM_STATUS_STOPPED );
	}
	else if ( gpMusicInfo->m_playState == PLAY_STATE_PLAYING)		// EE thinks it is playing, but not IOP
	{
		return ( PCM_STATUS_END );
	}
	else
	{
		//Dbg_Message("Music status is free %x", gPcmStatus);
		return ( PCM_STATUS_FREE );
	}
	Dbg_MsgAssert( 0,( "Sell your stock in GCC." ));
	return ( 0 );
}

int PCMAudio_GetStreamStatus( int whichStream )
{
	
	if ( (whichStream < 0) || (whichStream >= NUM_STREAMS) )
	{
		Dbg_MsgAssert( 0, ( "Checking stream status on stream %d, past valid range ( 0 to %d ).", whichStream, NUM_STREAMS - 1 ) );
		return ( false );
	}
	if ( gPcmStatus & PCMSTATUS_LOAD_STREAM( whichStream ) )
	{
		return ( PCM_STATUS_LOADING );
	}
	else if ( gPcmStatus & PCMSTATUS_STREAM_PLAYING( whichStream ) )
	{
		return ( PCM_STATUS_PLAYING );
	}
	else if ( gpStreamInfo[whichStream]->m_playState == PLAY_STATE_STOP)
	{
		return ( PCM_STATUS_STOPPED );
	}
	else if ( gpStreamInfo[whichStream]->m_playState == PLAY_STATE_PLAYING)		// EE thinks it is playing, but not IOP
	{
		return ( PCM_STATUS_END );
	}
	else
	{
		return ( PCM_STATUS_FREE );
	}
	Dbg_MsgAssert( 0,( "Sell your stock in GCC." ));
	return ( 0 );
}

void PCMAudio_Pause( bool pause, int ch )
{
	if ( ch == MUSIC_CHANNEL )
	{
		EzPcm( EzADPCM_PAUSEMUSIC, pause );
	}
	else
	{
		EzPcm( EzADPCM_PAUSESTREAMS, pause );
	}
} // end of PCMAudio_Pause( )

void PCMAudio_PauseStream( bool pause, int whichStream )
{
	EzPcm( EzADPCM_PAUSESTREAM | whichStream, pause );
}

unsigned int GetCDLocation( const char *pWadName )
{
	

	InitCDPlease( );

	File::StopStreaming( );
	sceCdSync( 0 );

	char tempFilename[ 255 ];
	sprintf( tempFilename, "\\%s%s.WAD;1", Config::GetDirectory(), pWadName );
	int i;
	for ( i = strlen( tempFilename ) - 1; i > 0; i-- )
	{
		if ( tempFilename[ i ] >= 'a' && tempFilename[ i ] <= 'z' )
		{
			tempFilename[ i ] += 'A' - 'a';
		}
	}
	sceCdlFILE fileInfo;
	int retVal = 0;
	if ( !( retVal = sceCdSearchFile( &fileInfo, tempFilename ) ) )
	{
		printf( "Wad %s not found -- Err %d\n", tempFilename, retVal );
		return ( 0 );
	}
	return ( fileInfo.lsn );
}

bool PCMAudio_TrackExists( const char *pTrackName, int ch )
{
	if ( !gWadInfo[ch].mp_hed )
	{
		return ( false );
	}
	if ( !FindFileInHed( pTrackName, gWadInfo[ch].mp_hed ) )
	{
		Dbg_Message( "Audio file %s not found in header file.", pTrackName );
		return ( false );
	}
	return ( true );
	
}

bool PCMAudio_LoadMusicHeader( const char *nameOfFile )
{
	bool no_wad = false;

	if (!(Config::CD() || TEST_MOTHERFUCKING_CD))
	{
		no_wad = true;
	}	

	return gWadInfo[MUSIC_CHANNEL].LoadHeader(nameOfFile, no_wad);
}

bool PCMAudio_LoadStreamHeader( const char *nameOfFile )
{
	bool no_wad = false;

	if (!(Config::CD() || TEST_MOTHERFUCKING_CD))
	{
		int testStreams = 0;
		testStreams = Script::GetInteger( 0x62df9442 ); // checksum 'testStreamsFromHost'
		if ( !testStreams )
		{
			return ( false );
		}

		no_wad = true;
	}	
	
	return gWadInfo[EXTRA_CHANNEL].LoadHeader(nameOfFile, no_wad);
}

uint32 PCMAudio_FindNameFromChecksum( uint32 checksum, int ch )
{
	
	File::SHed *pHed;

	pHed = gWadInfo[ch].mp_hed;

	if ( !pHed )
	{
		return ( NULL );
	}

	File::SHedFile* pHedFile = File::FindFileInHedUsingChecksum( checksum, pHed );
	if( pHedFile )
	{
		return pHedFile->Checksum;
	}
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////
// SWadInfo
//
bool			SWadInfo::LoadHeader(const char *nameOfFile, bool no_wad, bool assertOnError)
{
	if ( mp_hed )
	{
		Mem::Free( mp_hed );
		mp_hed = NULL;
	}

	mp_hed = File::LoadHed( nameOfFile, TEST_MOTHERFUCKING_CD, no_wad );
	if ( !mp_hed )
	{
		Dbg_Message( "Couldn't find audio header %s", nameOfFile );
		Dbg_MsgAssert( !assertOnError, ( "Couldn't find audio header %s", nameOfFile ));
		return false;
	}

	if (Config::CD() || TEST_MOTHERFUCKING_CD)
	{
		Dbg_MsgAssert(!no_wad, ("Can't use a no-wad version of a hed file on the CD"));

		m_lsn = GetCDLocation( nameOfFile );
		if ( !m_lsn )
		{
			Mem::Free( mp_hed );
			mp_hed = NULL;
			Dbg_Message( "Couldn't find audio wad %s", nameOfFile );
			Dbg_MsgAssert( !assertOnError,( "Couldn't find audio wad %s", nameOfFile ));
			return false;
		}
	}
	else
	{
		 sprintf( m_fileName, "host:%s.WAD", nameOfFile );
	}	 

	return (bool) mp_hed;
}

} // namespace PCM
