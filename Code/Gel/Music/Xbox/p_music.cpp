/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GFX (Graphics Library)									**
**																			**
**	Module:			Game Engine (GEL)	 									**
**																			**
**	File name:		p_music.cpp												**
**																			**
**	Created:		07/24/01	-	dc										**
**																			**
**	Description:	Xbox specific .wma streaming code						**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <xtl.h>
#include <core/macros.h>
#include <core/defines.h>
#include <core/math.h>
#include <core/crc.h>

#include <sys/config/config.h>
#include <gel/soundfx/soundfx.h>

#include "p_music.h"
#include "p_wmafilestream.h"
#include "p_adpcmfilestream.h"
#include "p_soundtrack.h"

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

namespace Pcm
{

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define STREAMS_ARE_PCM

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sFileStreamInfo
{
	enum FileStreamType
	{
		FILESTREAM_TYPE_WMA		= 0,
		FILESTREAM_TYPE_ADPCM	= 1
	};

	// These two could really be a union...
	CWMAFileStream*		p_wma_filestream;
	CADPCMFileStream*	p_adpcm_filestream;

	int					pitch;
	float				volume;
	bool				paused;

	void				CreateFileStream( FileStreamType type, bool use_3d = false );
	void				DestroyFileStream( void );
	
	HRESULT				Initialize( HANDLE h_file, void* fileBuffer );
    HRESULT				Initialize( HANDLE h_file, unsigned int offset, unsigned int length, void* fileBuffer );
	HRESULT				Process( void );
	void				Pause( uint32 pause );
	void				SetOkayToPlay( bool okay_to_play );
	bool				GetOkayToPlay( void );
	bool				HasFileStream( void )	{ return (( p_wma_filestream != NULL ) || ( p_adpcm_filestream != NULL )); }
	bool				IsCompleted( void );
	bool				IsAwaitingDeletion( void );
	void				SetAwaitingDeletion( bool is_awaiting );
	bool				IsSafeToDelete( void );
	void				Flush( void );
	bool				IsPreLoadDone( void );
	void				SetVolume( float v0 );
	void				SetVolume( float v0, float v1 );
	void				SetVolume( float v0, float v1, float v2, float v3, float v4 );
};



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sFileStreamInfo::CreateFileStream( FileStreamType type, bool use_3d )
{
	Dbg_Assert(( p_wma_filestream == NULL ) && ( p_adpcm_filestream == NULL ));
	if( type == FILESTREAM_TYPE_WMA )
	{
		p_wma_filestream = new CWMAFileStream( use_3d );
	}
	else if( type == FILESTREAM_TYPE_ADPCM )
	{
		p_adpcm_filestream = new CADPCMFileStream( use_3d );
	}
	else
	{
		Dbg_Assert( 0 );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sFileStreamInfo::DestroyFileStream( void )
{
	if( p_wma_filestream )
	{
		delete p_wma_filestream;
		p_wma_filestream = NULL;
	}

	if( p_adpcm_filestream )
	{
		delete p_adpcm_filestream;
		p_adpcm_filestream = NULL;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
HRESULT sFileStreamInfo::Initialize( HANDLE h_file, void* fileBuffer )
{
	if( p_wma_filestream )
	{
		return p_wma_filestream->Initialize( h_file, fileBuffer );
	}
	else if( p_adpcm_filestream )
	{
		Dbg_Assert( 0 );
	}
	return -1;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
HRESULT sFileStreamInfo::Initialize( HANDLE h_file, unsigned int offset, unsigned int length, void* fileBuffer )
{
	if( p_wma_filestream )
	{
		return p_wma_filestream->Initialize( h_file, offset, length, fileBuffer );
	}
	else if( p_adpcm_filestream )
	{
		return p_adpcm_filestream->Initialize( h_file, offset, length, fileBuffer );
	}
	return -1;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
HRESULT sFileStreamInfo::Process( void )
{
	if( p_wma_filestream )
	{
		return p_wma_filestream->Process();
	}
	else if( p_adpcm_filestream )
	{
		return p_adpcm_filestream->Process();
	}
	return -1;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sFileStreamInfo::Pause( uint32 pause )
{
	if( p_wma_filestream )
	{
		p_wma_filestream->Pause( pause );
	}
	else if( p_adpcm_filestream )
	{
		p_adpcm_filestream->Pause( pause );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sFileStreamInfo::SetOkayToPlay( bool okay_to_play )
{
	if( p_wma_filestream )
	{
		p_wma_filestream->m_bOkayToPlay = okay_to_play;
	}
	else if( p_adpcm_filestream )
	{
		p_adpcm_filestream->m_bOkayToPlay = okay_to_play;
	}
}

	
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool sFileStreamInfo::GetOkayToPlay( void )
{
	if( p_wma_filestream )
	{
		return p_wma_filestream->m_bOkayToPlay;
	}
	else if( p_adpcm_filestream )
	{
		return p_adpcm_filestream->m_bOkayToPlay;
	}
	return false;
}

	
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool sFileStreamInfo::IsCompleted( void )
{
	if( p_wma_filestream )
	{
		return p_wma_filestream->IsCompleted();
	}
	else if( p_adpcm_filestream )
	{
		return p_adpcm_filestream->IsCompleted();
	}
	return false;
}

	
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool sFileStreamInfo::IsAwaitingDeletion( void )
{
	if( p_wma_filestream )
	{
		return p_wma_filestream->m_AwaitingDeletion;
	}
	else if( p_adpcm_filestream )
	{
		return p_adpcm_filestream->m_AwaitingDeletion;
	}
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sFileStreamInfo::SetAwaitingDeletion( bool is_awaiting )
{
	if( p_wma_filestream )
	{
		p_wma_filestream->m_AwaitingDeletion = is_awaiting;
	}
	else if( p_adpcm_filestream )
	{
		p_adpcm_filestream->m_AwaitingDeletion = is_awaiting;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool sFileStreamInfo::IsSafeToDelete( void )
{
	if( p_wma_filestream )
	{
		return p_wma_filestream->IsSafeToDelete();
	}
	else if( p_adpcm_filestream )
	{
		return p_adpcm_filestream->IsSafeToDelete();
	}
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool sFileStreamInfo::IsPreLoadDone( void )
{
	if( p_wma_filestream )
	{
		return p_wma_filestream->PreLoadDone();
	}
	else if( p_adpcm_filestream )
	{
		return p_adpcm_filestream->PreLoadDone();
	}
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sFileStreamInfo::Flush( void )
{
	if( p_wma_filestream ) 
	{
		if( p_wma_filestream->m_pRenderFilter )
		{
			p_wma_filestream->m_pRenderFilter->FlushEx( 0, DSSTREAMFLUSHEX_ASYNC );
		}
	}
	else if( p_adpcm_filestream )
	{
		if( p_adpcm_filestream->m_pRenderFilter )
		{
			p_adpcm_filestream->m_pRenderFilter->FlushEx( 0, DSSTREAMFLUSHEX_ASYNC );
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sFileStreamInfo::SetVolume( float v0 )
{
	if( p_wma_filestream ) 
	{
		p_wma_filestream->SetVolume( v0 );
	}
	else if( p_adpcm_filestream )
	{
		p_adpcm_filestream->SetVolume( v0 );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sFileStreamInfo::SetVolume( float v0, float v1 )
{
	if( p_wma_filestream ) 
	{
		p_wma_filestream->SetVolume( v0, v1 );
	}
	else if( p_adpcm_filestream )
	{
		p_adpcm_filestream->SetVolume( v0, v1 );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void sFileStreamInfo::SetVolume( float v0, float v1, float v2, float v3, float v4 )
{
	if( p_wma_filestream ) 
	{
		p_wma_filestream->SetVolume( v0, v1, v2, v3, v4 );
	}
	else if( p_adpcm_filestream )
	{
		p_adpcm_filestream->SetVolume( v0, v1, v2, v3, v4 );
	}
}



/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

sFileStreamInfo	gMusicInfo;
sFileStreamInfo	gStreamInfo[NUM_STREAMS];

const uint32	FILESTREAM_BUFFER_SIZE	= 80 * 1024;

#pragma pack( 16 )
// Grab an 80k read buffer. Must be DWORD aligned.
// This is big enough for (9 * 8k packets) for WMA, where last packet mirrors the first packet,
// for cases when the decoder reads past the end of the ring buffer.
// Also big enough for ( 5 * 16k packets) for ADPCM, where no wraparound is required.
DWORD			gMusicFileBuffer[FILESTREAM_BUFFER_SIZE / 4];
DWORD			gStreamFileBuffer[NUM_STREAMS][FILESTREAM_BUFFER_SIZE / 4];
#pragma pack()

const int		NUM_OVERLAPPED	= 256;
OVERLAPPED		gOverlapped[NUM_OVERLAPPED];
int				gNextOverlap = 0;

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

HANDLE	ghWADFile				= INVALID_HANDLE_VALUE;
uint32	*pWADData				= NULL;
uint32	numWADFileEntries		= 0;

HANDLE	ghMusicWADFile			= INVALID_HANDLE_VALUE;
uint32	*pMusicWADData			= NULL;
uint32	numMusicWADFileEntries	= 0;



/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/


/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
OVERLAPPED *PCMAudio_GetNextOverlapped( void )
{
	OVERLAPPED *p_return = &( gOverlapped[gNextOverlap] );
	if( ++gNextOverlap >= NUM_OVERLAPPED )
	{
		gNextOverlap = 0;
	}

	ZeroMemory( p_return, sizeof( OVERLAPPED ));
	return p_return;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint32 PCMAudio_GetFilestreamBufferSize( void )
{
	return FILESTREAM_BUFFER_SIZE;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PCMAudio_Init( void )
{
	// Zero the music and filestream info arrays.
	ZeroMemory( &gMusicInfo, sizeof( sFileStreamInfo ));
	ZeroMemory( &gStreamInfo[0], sizeof( sFileStreamInfo ) * NUM_STREAMS );

	// Enumerate user soundtracks.
	GetNumSoundtracks();

	// Figure out the language, and open appropriate file.
	Config::ELanguage lang = Config::GetLanguage();

	// Assume English.
#	ifdef STREAMS_ARE_PCM
	ghWADFile		= CreateFile( "d:\\data\\streams\\pcm\\pcm.wad",
#	else
	ghWADFile		= CreateFile( "d:\\data\\streams\\wma\\wma.wad",
#	endif
								GENERIC_READ,
								FILE_SHARE_READ,								// Share mode.
								NULL,											// Ignored (security attributes).
								OPEN_EXISTING,									// File has to exist already.
								FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,	// Xbox has no asynchronous i/o buffering.
								NULL );											// Ignored (template file).
#	ifdef STREAMS_ARE_PCM
	ghMusicWADFile = CreateFile( "d:\\data\\streams\\pcm\\music_pcm.wad",
#	else
	ghMusicWADFile = CreateFile( "d:\\data\\streams\\wma\\music_wma.wad",
#	endif
								GENERIC_READ,
								FILE_SHARE_READ,								// Share mode.
								NULL,											// Ignored (security attributes).
								OPEN_EXISTING,									// File has to exist already.
								FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,	// Xbox has no asynchronous i/o buffering.
								NULL );											// Ignored (template file).

	// Now read in the data files used for indexing into the WAD files.
#	ifdef STREAMS_ARE_PCM
	HANDLE wad_data = CreateFile( "d:\\data\\streams\\pcm\\pcm.dat",
#	else
	HANDLE wad_data = CreateFile( "d:\\data\\streams\\wma\\wma.dat",
#	endif
									GENERIC_READ,
									FILE_SHARE_READ,								// Share mode.
									NULL,											// Ignored (security attributes).
									OPEN_EXISTING,									// File has to exist already.
									FILE_FLAG_SEQUENTIAL_SCAN,
									NULL );											// Ignored (template file).

	if( wad_data != INVALID_HANDLE_VALUE )
	{
		uint32 bytes_read;
		ReadFile( wad_data, &numWADFileEntries, sizeof( uint32 ), &bytes_read, NULL );
		pWADData = new uint32[numWADFileEntries * 3];
		ReadFile( wad_data, pWADData, sizeof( uint32 ) * numWADFileEntries * 3, &bytes_read, NULL );
		CloseHandle( wad_data );
	}
			
	// Sort the wad file entries into increasing checksum order, so that we can use a binary search algorithm to
	// find the checksum quickly.
	for( uint32 i = 0; i < numWADFileEntries; ++i )
	{
		for( uint32 j = i + 1; j < numWADFileEntries; ++j )
		{
			if( pWADData[i * 3] > pWADData[j * 3] )
			{
				uint32 temp[3];
				temp[0]				= pWADData[i * 3];
				temp[1]				= pWADData[i * 3 + 1];
				temp[2]				= pWADData[i * 3 + 2];
				pWADData[i * 3]		= pWADData[j * 3];
				pWADData[i * 3 + 1]	= pWADData[j * 3 + 1];
				pWADData[i * 3 + 2]	= pWADData[j * 3 + 2];
				pWADData[j * 3]		= temp[0];
				pWADData[j * 3 + 1]	= temp[1];
				pWADData[j * 3 + 2]	= temp[2];
			}
		}
	}
			
#	ifdef STREAMS_ARE_PCM
	wad_data = CreateFile( "d:\\data\\streams\\pcm\\music_pcm.dat",
#	else
	wad_data = CreateFile( "d:\\data\\streams\\wma\\music_wma.dat",
#	endif
							GENERIC_READ,
							FILE_SHARE_READ,								// Share mode.
							NULL,											// Ignored (security attributes).
							OPEN_EXISTING,									// File has to exist already.
							FILE_FLAG_SEQUENTIAL_SCAN,
							NULL );											// Ignored (template file).

	if( wad_data != INVALID_HANDLE_VALUE )
	{
		uint32 bytes_read;
		ReadFile( wad_data, &numMusicWADFileEntries, sizeof( uint32 ), &bytes_read, NULL );
		pMusicWADData = new uint32[numMusicWADFileEntries * 3];
		ReadFile( wad_data, pMusicWADData, sizeof( uint32 ) * numMusicWADFileEntries * 3, &bytes_read, NULL );
		CloseHandle( wad_data );
	}
}



/******************************************************************/
/*                                                                */
/* Call every frame to make sure music and stream buffers are	  */
/* loaded and current status is checked each frame...			  */
/*                                                                */
/******************************************************************/
int PCMAudio_Update( void )
{
	if( gMusicInfo.HasFileStream())
	{
		HRESULT hr = gMusicInfo.Process();

		if( gMusicInfo.IsCompleted())
		{
			gMusicInfo.DestroyFileStream();
		}
		else if( gMusicInfo.IsAwaitingDeletion() && gMusicInfo.IsSafeToDelete())
		{
			gMusicInfo.DestroyFileStream();
		}
	}

	for( int i = 0; i < NUM_STREAMS; ++i )
	{
		if( gStreamInfo[i].HasFileStream())
		{
			HRESULT hr = gStreamInfo[i].Process();

			if( gStreamInfo[i].IsCompleted())
			{
				gStreamInfo[i].DestroyFileStream();
			}
			else if( gStreamInfo[i].IsAwaitingDeletion() && gStreamInfo[i].IsSafeToDelete())
			{
				gStreamInfo[i].DestroyFileStream();
			}
		}
	}

	// A non-zero return value singnals an error condition.
	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PCMAudio_StopMusic( bool waitPlease )
{
	if( gMusicInfo.HasFileStream())
	{
		if( gMusicInfo.IsSafeToDelete())
		{	
			gMusicInfo.Flush();
			gMusicInfo.DestroyFileStream();
		}
		else
		{
			gMusicInfo.SetAwaitingDeletion( true );
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PCMAudio_StopStream( int whichStream, bool waitPlease )
{
	if( gStreamInfo[whichStream].HasFileStream())
	{
		if( gStreamInfo[whichStream].IsSafeToDelete())
		{
			gStreamInfo[whichStream].Flush();
			gStreamInfo[whichStream].DestroyFileStream();
		}
		else
		{
			gStreamInfo[whichStream].SetAwaitingDeletion( true );
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PCMAudio_StopStreams( void )
{
	for( int i = 0; i < NUM_STREAMS; ++i )
	{
		PCMAudio_StopStream( i, false );
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// This is temp code for the preload streams.  It just calls the normal one.

static uint32 sPreLoadChecksum[NUM_STREAMS];
static uint32 sPreLoadMusicChecksum;



/******************************************************************/
/*                                                                */
/* Get a stream loaded into a buffer, but don't play yet		  */
/*                                                                */
/******************************************************************/
bool PCMAudio_PreLoadStream( uint32 checksum, int whichStream )
{
	Dbg_Assert(( whichStream >= 0 ) && ( whichStream < NUM_STREAMS ));
	sPreLoadChecksum[whichStream] = checksum;

	// Start the track as normal...
	if( PCMAudio_PlayStream( checksum, whichStream, NULL, 0.0f, false ))
	{
		// ...but then flag it as not okay to play until we say so.
		if( gStreamInfo[whichStream].HasFileStream())
		{
			gStreamInfo[whichStream].SetOkayToPlay( false );
		}
        return true;
	}
	return false;
}



/******************************************************************/
/*                                                                */
/* Returns true if preload done. Assumes that caller is calling	  */
/* this on a preloaded, but not yet played, stream. The results	  */
/* are meaningless otherwise.									  */
/*                                                                */
/******************************************************************/
bool PCMAudio_PreLoadStreamDone( int whichStream )
{
	if( gStreamInfo[whichStream].HasFileStream())
	{
		return gStreamInfo[whichStream].IsPreLoadDone();
	}
	return true;
}



/******************************************************************/
/*                                                                */
/* Tells a preloaded stream to start playing.					  */
/* Must call PCMAudio_PreLoadStreamDone() first to guarantee that */
/* it starts immediately.										  */
/*                                                                */
/******************************************************************/
bool PCMAudio_StartPreLoadedStream( int whichStream, Sfx::sVolume *p_volume, float pitch )
{
	// Maybe we should check here to make sure the checksum of the music info filestream matches that
	// passed in when the music stream preload request came in.
	if( gStreamInfo[whichStream].HasFileStream())
	{
		gStreamInfo[whichStream].SetOkayToPlay( true );
		PCMAudio_SetStreamVolume( p_volume, whichStream );
		return true;
	}
	return false;	
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_PreLoadMusicStream( uint32 checksum )
{
	sPreLoadMusicChecksum = checksum;

	// Start the track as normal...
	if( PCMAudio_PlayMusicTrack( sPreLoadMusicChecksum ))
	{
		// ...but then flag it as not okay to play until we say so.
		if( gMusicInfo.HasFileStream())
		{
			gMusicInfo.SetOkayToPlay( false );
		}
        return true;
	}

	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_PreLoadMusicStreamDone( void )
{
	if( gMusicInfo.HasFileStream())
	{
		return gMusicInfo.IsPreLoadDone();
	}
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_StartPreLoadedMusicStream( void )
{
	// Maybe we should check here to make sure the checksum of the music info filestream matches that
	// passed in when the music stream preload request came in.
	if( gMusicInfo.HasFileStream())
	{
		// Call update immediately to start playback ASAP.
		gMusicInfo.SetOkayToPlay( true );
		PCMAudio_Update();

		return true;
	}
	return false;	
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int PCMAudio_GetMusicStatus( void )
{
	if( gMusicInfo.HasFileStream())
	{
		return PCM_STATUS_PLAYING;
	}
	else
	{
		return PCM_STATUS_FREE;
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int PCMAudio_GetStreamStatus( int whichStream )
{
	int start, end;

	// Negative one is used to signal 'any stream'.
	if( whichStream == -1 )
	{
		start	= 0;
		end		= NUM_STREAMS;
	}
	else
	{
		start	= whichStream;
		end		= start + 1;
	}

	for( int s = start; s < end; ++s )
	{
		if( !gStreamInfo[s].HasFileStream())
		{		
			return PCM_STATUS_FREE;
		}
	}

	return PCM_STATUS_PLAYING;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PCMAudio_Pause( bool pause, int ch )
{
	if( ch == MUSIC_CHANNEL )
	{
		gMusicInfo.paused = pause;
		if( gMusicInfo.HasFileStream())
		{
			if( pause )
			{
				gMusicInfo.Pause( 1 );
			}
			else
			{
				gMusicInfo.Pause( 0 );
			}
		}
	}
	else
	{
		for( int s = 0; s < NUM_STREAMS; ++s )
		{
			if( gStreamInfo[s].HasFileStream())
			{
				if( pause )
				{
					gStreamInfo[s].Pause( 1 );
					gStreamInfo[s].paused = true;
				}
				else
				{
					gStreamInfo[s].Pause( 0 );
					gStreamInfo[s].paused = false;
				}
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_TrackExists( const char *pTrackName, int ch )
{
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_LoadMusicHeader( const char *nameOfFile )
{
	// Legacy call left over from PS2 code.
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_LoadStreamHeader( const char *nameOfFile )
{
	// Legacy call left over from PS2 code.
	return true;
}



/******************************************************************/
/*                                                                */
/* Return (any) position if t in sorted x[0..n-1] or -1 if t is	  */
/* not present.                                                    */
/*                                                                */
/******************************************************************/
static int binarySearch( uint32 checksum )
{
	int l = 0;
	int u = numWADFileEntries - 1;
	while( l <= u )
	{
		int m = ( l + u ) / 2;
		if( pWADData[m * 3] < checksum )
			l = m + 1;
		else if ( pWADData[m * 3] == checksum )
			return m;
		else // x[m] > t
			u = m - 1;
	}
	return -1;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
uint32 PCMAudio_FindNameFromChecksum( uint32 checksum, int ch )
{
	if( ch != EXTRA_CHANNEL )
		return 0;

	int rv = binarySearch( checksum );
	
	return ( rv == -1 ) ? 0 : checksum;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_SetStreamVolume( Sfx::sVolume *p_volume, int whichStream )
{
	if( gStreamInfo[whichStream].HasFileStream())
	{
		// Adjust volumes for overall sound volume.
		Spt::SingletonPtr< Sfx::CSfxManager > sfx_manager;

		switch( p_volume->GetVolumeType())
		{
			case Sfx::VOLUME_TYPE_5_CHANNEL_DOLBY5_1:
			{
				float v0	= PERCENT( sfx_manager->GetMainVolume(), p_volume->GetChannelVolume( 0 ));
				float v1	= PERCENT( sfx_manager->GetMainVolume(), p_volume->GetChannelVolume( 1 ));
				float v2	= PERCENT( sfx_manager->GetMainVolume(), p_volume->GetChannelVolume( 2 ));
				float v3	= PERCENT( sfx_manager->GetMainVolume(), p_volume->GetChannelVolume( 3 ));
				float v4	= PERCENT( sfx_manager->GetMainVolume(), p_volume->GetChannelVolume( 4 ));

				gStreamInfo[whichStream].volume = ( v0 + v1 + v2 + v3 + v4 ) * ( 1.0f / 5.0f );
				gStreamInfo[whichStream].SetVolume( v0, v1, v2, v3, v4 );
				break;
			}
			case Sfx::VOLUME_TYPE_2_CHANNEL_DOLBYII:
			{
				float v0	= PERCENT( sfx_manager->GetMainVolume(), p_volume->GetChannelVolume( 0 ));
				float v1	= PERCENT( sfx_manager->GetMainVolume(), p_volume->GetChannelVolume( 1 ));

				gStreamInfo[whichStream].volume = v0;
				gStreamInfo[whichStream].SetVolume( v0, v1 );
				break;
			}
			case Sfx::VOLUME_TYPE_BASIC_2_CHANNEL:
			{
				float v0	= PERCENT( sfx_manager->GetMainVolume(), p_volume->GetChannelVolume( 0 ));

				gStreamInfo[whichStream].volume = v0;
				gStreamInfo[whichStream].SetVolume( v0 );
				break;
			}
			default:
			{
				Dbg_Assert( 0 );
				break;
			}
		}
	}
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int PCMAudio_SetMusicVolume( float volume )
{
	if( gMusicInfo.HasFileStream())
	{
		gMusicInfo.volume = volume;
		gMusicInfo.SetVolume( volume );
	}
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_SetStreamPitch( float fPitch, int whichStream )
{
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_PlayMusicTrack( uint32 checksum )
{
	// Find the entry in the offset array.
	bool found = false;
	
	unsigned int samplelength, sampleoffset;
	
	// English.
	for( unsigned int entry = 0; entry < numMusicWADFileEntries; ++entry )
	{
		if( pMusicWADData[entry * 3] == checksum )
		{
			sampleoffset	= pMusicWADData[entry * 3 + 1];
			samplelength	= pMusicWADData[entry * 3 + 2];
			found			= true;
			break;
		}
	}

	if( !found )
	{
		return false;
	}

	// Just a stream like everything else.
	if( PCMAudio_GetMusicStatus() != PCM_STATUS_FREE )
	{
		return false;
	}

	// Don't want to use 3d processing for the music track.
#	ifdef STREAMS_ARE_PCM
	gMusicInfo.CreateFileStream( sFileStreamInfo::FILESTREAM_TYPE_ADPCM, false );
#	else
	gMusicInfo.CreateFileStream( sFileStreamInfo::FILESTREAM_TYPE_WMA, false );
#	endif

	HRESULT hr = gMusicInfo.Initialize( ghMusicWADFile, sampleoffset, samplelength, gMusicFileBuffer );
	if( hr == S_OK )
	{
		// All started fine. Pause music if paused flag is set.
		if( gMusicInfo.paused )
		{
			PCMAudio_Pause( true, MUSIC_CHANNEL );
		}
		return true;
	}
	else
	{
		// Failed to initialize the stream.
		gMusicInfo.DestroyFileStream();
		Dbg_MsgAssert( 0, ( "Failed to initialize music stream: %x", checksum ));
	}
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_PlayMusicTrack( const char *filename )
{
	const char	*samplename = filename;
	char		*locate;
	
	// Search for the last directory seperator, and cut off all of the path prior to it.
	if( locate = strrchr( samplename, '\\' ))
	{
		samplename = locate + 1;
	}
	if( locate = strrchr( samplename, '/' ))
	{
		samplename = locate + 1;
	}

	// Now generate the checksum for this samplename.
	uint32 checksum = Crc::GenerateCRCFromString( samplename );

	bool rv = PCMAudio_PlayMusicTrack( checksum );
	if( rv == false )
	{
		Dbg_Message( "Failed to find stream: %s", filename );
	}
	return rv;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_PlaySoundtrackMusicTrack( int soundtrack, int track )
{
	// Just a stream like everything else.
	if( gMusicInfo.HasFileStream())
	{
		Dbg_MsgAssert( 0, ( "Playing new track without stopping the first track." ));
	}
	else
	{
		// Don't want to use 3d processing for the music track.
		gMusicInfo.CreateFileStream( sFileStreamInfo::FILESTREAM_TYPE_WMA, false );

		HANDLE h_song = GetSoundtrackWMAHandle( soundtrack, track );
		if( h_song == INVALID_HANDLE_VALUE )
		{
			return false;
		}

		HRESULT hr = gMusicInfo.Initialize( h_song, gMusicFileBuffer );
		if( hr == S_OK )
		{
			// All started fine. Pause music if paused flag is set.
			if( gMusicInfo.paused )
			{
				PCMAudio_Pause( true, MUSIC_CHANNEL );
			}
			return true;
		}
	}
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_StartStream( int whichStream )
{
	if( gStreamInfo[whichStream].HasFileStream())
	{
		if( gStreamInfo[whichStream].GetOkayToPlay() == false )
		{
			// Now okay to start playing.
			gStreamInfo[whichStream].SetOkayToPlay( true );
		}
		return true;
	}
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool PCMAudio_PlayStream( uint32 checksum, int whichStream, Sfx::sVolume *p_volume, float fPitch, bool preload )
{
	unsigned int samplelength, sampleoffset;

	// Perform a binary search on the checksum table to find the correct entry.	
	int entry = binarySearch( checksum );
	if( entry >= 0 )
	{
		sampleoffset	= pWADData[entry * 3 + 1];
		samplelength	= pWADData[entry * 3 + 2];
	}
	else
	{
		return false;
	}
	
	Dbg_Assert(( whichStream >= 0 ) && ( whichStream < NUM_STREAMS ));

	// Just a stream like everything else.
	if( PCMAudio_GetStreamStatus( whichStream ) != PCM_STATUS_FREE )
	{
		return false;
	}

#	ifdef STREAMS_ARE_PCM
	gStreamInfo[whichStream].CreateFileStream( sFileStreamInfo::FILESTREAM_TYPE_ADPCM, true );
#	else
	gStreamInfo[whichStream].CreateFileStream( sFileStreamInfo::FILESTREAM_TYPE_WMA, true );
#	endif

	// The preload parameter indicates that we want to get this stream in a position where it is ready to play
	// immediately at some later point in time. The m_bOkayToPlay member is used to signal this.
	if( preload )
	{
		// Not okay to start playing until told to do so.
		gStreamInfo[whichStream].SetOkayToPlay( false );
	}

	HRESULT hr = gStreamInfo[whichStream].Initialize( ghWADFile, sampleoffset, samplelength, &gStreamFileBuffer[whichStream][0] );

	if( hr == S_OK )
	{
		// All started fine.
		if( p_volume )
		{
			PCMAudio_SetStreamVolume( p_volume, whichStream );
		}
		return true;
	}
	else
	{
		// Failed to initialize the stream.
		gStreamInfo[whichStream].DestroyFileStream();
	}
	return false;
}


} // namespace PCM
