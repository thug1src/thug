/*****************************************************************************
**																			**
**			              Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GEL Library												**
**																			**
**	Module:			Sound effects (Sfx)	 									**
**																			**
**	File name:		xbox/p_sfx.cpp											**
**																			**
**	Created by:		01/10/01	-	dc										**
**																			**
**	Description:	XBox Sound effects										**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <xtl.h>
#include <dsound.h>
//#include <dsstdfx.h>

#include <core/macros.h>
#include <sys/file/filesys.h>
#include <gel/soundfx/soundfx.h>
#include <gel/soundfx/xbox/p_sfx.h>
#include <gel/soundfx/xbox/skate5fx.h>
#include <gel/music/music.h>

/*****************************************************************************
**							  DBG Information								**
*****************************************************************************/

int num_buffers = 0;

namespace Sfx
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define RIFFCHUNK_FLAGS_VALID   0x00000001

// FourCC definitions
const DWORD FOURCC_RIFF		= 'FFIR';
const DWORD FOURCC_WAVE		= 'EVAW';
const DWORD FOURCC_FORMAT	= ' tmf';
const DWORD FOURCC_DATA		= 'atad';
const DWORD FOURCC_SAMPLER	= 'lpms';

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

struct VoiceEntry
{
	IDirectSoundBuffer*	p_buffer;
	DWORD				default_frequency;
};

typedef struct
{
    WCHAR *         szName;
    DSI3DL2LISTENER ds3dl;
} I3DL2ENVIRONMENT;

/*****************************************************************************
 *
 * Name: RIFFHEADER
 * Desc: For parsing WAV files
 *
 ****************************************************************************/
struct RIFFHEADER
{
	FOURCC  fccChunkId;
	DWORD   dwDataSize;
};


struct SAMPLERCHUNK
{
	DWORD	manufacturer;
	DWORD	product;
	DWORD	sample_period;
	DWORD	MIDI_unity_note;
	DWORD	MIDI_pitch_fraction;
	DWORD	SMPTE_format;
	DWORD	SMPTE_offset;
	DWORD	num_sample_loops;
	DWORD	sampler_data;
};


/*****************************************************************************
 *
 * Name: class CRiffChunk
 * Desc: RIFF chunk utility class
 *
 ****************************************************************************/
class CRiffChunk
{
	FOURCC				m_fccChunkId;       // Chunk identifier
	const CRiffChunk*	m_pParentChunk;     // Parent chunk
	void*				m_hFile;
	DWORD				m_dwDataOffset;     // Chunk data offset
	DWORD				m_dwDataSize;       // Chunk data size
	DWORD				m_dwFlags;          // Chunk flags

	public:

    CRiffChunk();

    // Initialization.
    void	Initialize( FOURCC fccChunkId, const CRiffChunk* pParentChunk, void* hFile );
	bool	Open();
    bool    IsValid( void )		{ return !!( m_dwFlags & RIFFCHUNK_FLAGS_VALID ); }

    // Data
    bool	ReadData( LONG lOffset, VOID* pData, DWORD dwDataSize );

    // Chunk information
    FOURCC  GetChunkId()  { return m_fccChunkId; }
    DWORD   GetDataSize() { return m_dwDataSize; }
};




/*****************************************************************************
 *
 * Name: class CWaveFile
 * Desc: Wave file utility class
 *
 ****************************************************************************/
class CWaveFile
{
	void*		m_hFile;            // File handle
	CRiffChunk	m_RiffChunk;        // RIFF chunk
	CRiffChunk  m_FormatChunk;      // Format chunk
	CRiffChunk  m_DataChunk;        // Data chunk
	CRiffChunk  m_SamplerChunk;		// Sampler chunk, may or may not be present
    
	bool		m_ContainsSamplerChunk;

	public:

    CWaveFile( void );
    ~CWaveFile( void );

    // Initialization
    bool	Open( const char* strFileName );
    void    Close( void );

    // File format
    bool	GetFormat( WAVEFORMATEX* pwfxFormat, DWORD dwFormatSize );

    // File data
    bool	ReadSample( DWORD dwPosition, VOID* pBuffer, DWORD dwBufferSize, DWORD* pdwRead );

    // File properties
    void    GetDuration( DWORD* pdwDuration ) { *pdwDuration = m_DataChunk.GetDataSize(); }

	bool	ContainsLoop( void );
};





/*****************************************************************************
 *
 * Name: class CSound
 * Desc: Encapsulates functionality of a DirectSound buffer.
 *
 ****************************************************************************/
class CXBSound
{
	public:
	XBOXADPCMWAVEFORMAT		m_WaveFormat;		// This encapsulates WAVEFORMATEX.
    DSBUFFERDESC			m_dsbd;
    DWORD					m_dwBufferSize;
	void*					m_pRawData;
	bool					m_Loops;

    bool	Create( const CHAR* strFileName, DWORD dwFlags = 0L );
    bool	Create( const XBOXADPCMWAVEFORMAT* pwfxFormat, DWORD dwFlags, VOID* pBuffer, DWORD dwBytes );
    void    Destroy();

	DWORD	GetSampleRate( void ) const;

    CXBSound();
    ~CXBSound();
};



/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

// Because we don't use voices in the same way as on PS2, this array simulates
// the system. An entry with a NULL pointer means no sound is playing on that
// 'voice', a valid pointer indicates a sound is playing on that 'voice', and
// provides details of the buffer.
static VoiceEntry		VoiceSimulator[NUM_VOICES];
static IDirectSound8*	pDirectSound	= NULL;

static I3DL2ENVIRONMENT reverbEnvironments[] =
{
    { L"Default",           { DSI3DL2_ENVIRONMENT_PRESET_DEFAULT }        },
    { L"Generic",           { DSI3DL2_ENVIRONMENT_PRESET_GENERIC }        },
    { L"Padded Cell",       { DSI3DL2_ENVIRONMENT_PRESET_PADDEDCELL }     },
    { L"Room",              { DSI3DL2_ENVIRONMENT_PRESET_ROOM }           },
    { L"Bathroom",          { DSI3DL2_ENVIRONMENT_PRESET_BATHROOM }       },
    { L"Living Room",       { DSI3DL2_ENVIRONMENT_PRESET_LIVINGROOM }     },
    { L"Stone Room",        { DSI3DL2_ENVIRONMENT_PRESET_STONEROOM }      },
    { L"Auditorium",        { DSI3DL2_ENVIRONMENT_PRESET_AUDITORIUM }     },
    { L"Concert Hall",      { DSI3DL2_ENVIRONMENT_PRESET_CONCERTHALL }    },
    { L"Cave",              { DSI3DL2_ENVIRONMENT_PRESET_CAVE }           },
    { L"Arena",             { DSI3DL2_ENVIRONMENT_PRESET_ARENA }          },
    { L"Hangar",            { DSI3DL2_ENVIRONMENT_PRESET_HANGAR }         },
    { L"Carpeted Hallway",  { DSI3DL2_ENVIRONMENT_PRESET_CARPETEDHALLWAY }},
    { L"Hallway",           { DSI3DL2_ENVIRONMENT_PRESET_HALLWAY }        },
    { L"Stone Corridor",    { DSI3DL2_ENVIRONMENT_PRESET_STONECORRIDOR }  },
    { L"Alley",             { DSI3DL2_ENVIRONMENT_PRESET_ALLEY }          },
    { L"Forest",            { DSI3DL2_ENVIRONMENT_PRESET_FOREST }         },
    { L"City",              { DSI3DL2_ENVIRONMENT_PRESET_CITY }           },
    { L"Mountains",         { DSI3DL2_ENVIRONMENT_PRESET_MOUNTAINS }      },
    { L"Quarry",            { DSI3DL2_ENVIRONMENT_PRESET_QUARRY }         },
    { L"Plain",             { DSI3DL2_ENVIRONMENT_PRESET_PLAIN }          },
    { L"Parking Lot",       { DSI3DL2_ENVIRONMENT_PRESET_PARKINGLOT }     },
    { L"Sewer Pipe",        { DSI3DL2_ENVIRONMENT_PRESET_SEWERPIPE }      },
    { L"Underwater",        { DSI3DL2_ENVIRONMENT_PRESET_UNDERWATER }     },
};



/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

// Global sfx multiplier, percentage.
float					gSfxVolume		= 100.0f;

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/


//-----------------------------------------------------------------------------
// Name: CRiffChunk()
// Desc: Object constructor.
//-----------------------------------------------------------------------------
CRiffChunk::CRiffChunk()
{
    // Initialize defaults
    m_fccChunkId   = 0;
    m_pParentChunk = NULL;
    m_hFile        = NULL;
    m_dwDataOffset = 0;
    m_dwDataSize   = 0;
    m_dwFlags      = 0;
}




//-----------------------------------------------------------------------------
// Name: Initialize()
// Desc: Initializes the object
//-----------------------------------------------------------------------------
VOID CRiffChunk::Initialize( FOURCC fccChunkId, const CRiffChunk* pParentChunk, void* hFile )
{
    m_fccChunkId   = fccChunkId;
    m_pParentChunk = pParentChunk;
    m_hFile        = hFile;
}




//-----------------------------------------------------------------------------
// Name: Open()
// Desc: Opens an existing chunk.
//-----------------------------------------------------------------------------
bool CRiffChunk::Open()
{
    RIFFHEADER rhRiffHeader;
    LONG       lOffset = 0;

    // Seek to the first byte of the parent chunk's data section
    if( m_pParentChunk )
    {
        lOffset = m_pParentChunk->m_dwDataOffset;

        // Special case the RIFF chunk
        if( FOURCC_RIFF == m_pParentChunk->m_fccChunkId )
		{
            lOffset += sizeof(FOURCC);
		}
    }
    
    // Read each child chunk header until we find the one we're looking for
//	RwFileFunctions* fs = RwOsGetFileInterface();
    for( ;; )
    {
//		if( fs->rwfseek( m_hFile, lOffset, SEEK_SET ) != 0 )
		File::Seek( m_hFile, lOffset, SEEK_SET );
//		{
//			return false;
//		}

		DWORD dwRead;
//		dwRead = fs->rwfread( &rhRiffHeader, sizeof( rhRiffHeader ), 1, m_hFile );
		dwRead = File::Read( &rhRiffHeader, sizeof( rhRiffHeader ), 1, m_hFile );
		if( dwRead != sizeof( rhRiffHeader ))
		{
            return false;
		}

		// Check if we found the one we're looking for.
		if( m_fccChunkId == rhRiffHeader.fccChunkId )
        {
			// Save the chunk size and data offset.
            m_dwDataOffset = lOffset + sizeof( rhRiffHeader );
            m_dwDataSize   = rhRiffHeader.dwDataSize;

            // Success.
            m_dwFlags |= RIFFCHUNK_FLAGS_VALID;

            return true;
		}
		lOffset += sizeof( rhRiffHeader ) + rhRiffHeader.dwDataSize;
    }
	return false;
}



//-----------------------------------------------------------------------------
// Name: Read()
// Desc: Reads from the file
//-----------------------------------------------------------------------------
bool CRiffChunk::ReadData( LONG lOffset, VOID* pData, DWORD dwDataSize )
{
	// Seek to the offset
//	RwFileFunctions* fs = RwOsGetFileInterface();
//	if( fs->rwfseek( m_hFile, m_dwDataOffset+lOffset, SEEK_SET ) != 0 )
//	if( File::Seek( m_hFile, m_dwDataOffset+lOffset, SEEK_SET ) != 0 )
	File::Seek( m_hFile, m_dwDataOffset + lOffset, SEEK_SET );
//	{
//		return false;
//	}

    // Read from the file
	DWORD dwRead;
//	dwRead = fs->rwfread( pData, dwDataSize, 1, m_hFile );
	dwRead = File::Read( pData, dwDataSize, 1, m_hFile );
	if( dwRead != dwDataSize )
	{
		return false;
	}
	return true;
}




//-----------------------------------------------------------------------------
// Name: CWaveFile()
// Desc: Object constructor.
//-----------------------------------------------------------------------------
CWaveFile::CWaveFile()
{
    m_hFile = NULL;
}




//-----------------------------------------------------------------------------
// Name: ~CWaveFile()
// Desc: Object destructor.
//-----------------------------------------------------------------------------
CWaveFile::~CWaveFile()
{
	Close();
}




//-----------------------------------------------------------------------------
// Name: Open()
// Desc: Initializes the object.
//-----------------------------------------------------------------------------
bool CWaveFile::Open( const char* strFileName )
{
    // If we're already open, close.
    Close();
    
    // Open the file
	m_hFile = File::Open( strFileName, "rb" );

//	RwFileFunctions* fs = RwOsGetFileInterface();
//	m_hFile = fs->rwfopen( strFileName, "rb" );

	if( m_hFile == NULL )
	{
        return false;
	}

	// Initialize the chunk objects.
    m_RiffChunk.Initialize( FOURCC_RIFF, NULL, m_hFile );
    m_FormatChunk.Initialize( FOURCC_FORMAT, &m_RiffChunk, m_hFile );
    m_DataChunk.Initialize( FOURCC_DATA, &m_RiffChunk, m_hFile );
    m_SamplerChunk.Initialize( FOURCC_SAMPLER, &m_RiffChunk, m_hFile );

    bool hr = m_RiffChunk.Open();
    if( !hr )
	{
        return hr;
	}

    hr = m_FormatChunk.Open();
    if( !hr )
	{
        return hr;
	}

    hr = m_DataChunk.Open();
    if( !hr )
	{
        return hr;
	}

	// Not a problem if it doesn't, not every file will contain the sampler chunk.
    m_ContainsSamplerChunk	= m_SamplerChunk.Open();

    // Validate the file type.
    FOURCC fccType;
    hr = m_RiffChunk.ReadData( 0, &fccType, sizeof( fccType ));
    if( !hr )
	{
        return hr;
	}

    if( FOURCC_WAVE != fccType )
        return false;

    return true;
}




//-----------------------------------------------------------------------------
// Name: GetFormat()
// Desc: Gets the wave file format
//-----------------------------------------------------------------------------
bool CWaveFile::GetFormat( WAVEFORMATEX* pwfxFormat, DWORD dwFormatSize )
{
    DWORD dwValidSize = m_FormatChunk.GetDataSize();

    if( NULL == pwfxFormat || 0 == dwFormatSize )
        return false;

    // Read the format chunk into the buffer
    bool hr = m_FormatChunk.ReadData( 0, pwfxFormat, min( dwFormatSize, dwValidSize ));

	if( hr )
	{
	    // Zero out remaining bytes, in case enough bytes were not read
		if( dwFormatSize > dwValidSize )
		{
	        ZeroMemory( (BYTE*)pwfxFormat + dwValidSize, dwFormatSize - dwValidSize );
		}
	}

    return hr;
}




//-----------------------------------------------------------------------------
// Name: ReadSample()
// Desc: Reads data from the audio file.
//-----------------------------------------------------------------------------
bool CWaveFile::ReadSample( DWORD dwPosition, VOID* pBuffer, DWORD dwBufferSize, DWORD* pdwRead )
{                                   
    // Don't read past the end of the data chunk
    DWORD dwDuration;
    GetDuration( &dwDuration );

    if( dwPosition + dwBufferSize > dwDuration )
        dwBufferSize = dwDuration - dwPosition;

    bool hr = true;
    if( dwBufferSize )
        hr = m_DataChunk.ReadData( (LONG)dwPosition, pBuffer, dwBufferSize );

    if( pdwRead )
        *pdwRead = dwBufferSize;

    return hr;
}



//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
bool CWaveFile::ContainsLoop( void )
{
	// Figure out if it is a looping sound.
	if( m_ContainsSamplerChunk )
	{
		SAMPLERCHUNK sc;
		m_SamplerChunk.ReadData( 0, &sc, sizeof( sc ));
		if( sc.num_sample_loops > 0 )
		{
			return true;
		}		
	}
	return false;
}



//-----------------------------------------------------------------------------
// Name: Close()
// Desc: Closes the object
//-----------------------------------------------------------------------------
void CWaveFile::Close( void )
{
    if( m_hFile != NULL )
    {
//		RwFileFunctions* fs = RwOsGetFileInterface();
//		fs->rwfclose( m_hFile );
//		m_hFile = NULL;
		File::Close( m_hFile );
    }
}




//-----------------------------------------------------------------------------
// Name: CXBSound()
// Desc: 
//-----------------------------------------------------------------------------
CXBSound::CXBSound( void )
{
    m_dwBufferSize  = 0L;
	m_pRawData		= NULL;
}




//-----------------------------------------------------------------------------
// Name: ~CXBSound()
// Desc: 
//-----------------------------------------------------------------------------
CXBSound::~CXBSound( void )
{
    Destroy();
}




//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Creates the sound. Sound is buffered to memory allocated internally
//       by DirectSound.
//-----------------------------------------------------------------------------
bool CXBSound::Create( const CHAR* strFileName, DWORD dwFlags )
{
    bool	br;

    // Open the .wav file
    CWaveFile waveFile;
	br = waveFile.Open( strFileName );
    if( !br )
	{
        return false;
	}

    // Get the WAVEFORMAT structure for the .wav file
    br = waveFile.GetFormat( &m_WaveFormat.wfx, sizeof( WAVEFORMATEX ));
    if( !br )
	{
        return false;
	}

	// Required if the sound is Xbox ADPCM, will be ignored otherwise.
	m_WaveFormat.wSamplesPerBlock = 64;

    // Get the size of the .wav file
    waveFile.GetDuration( &m_dwBufferSize );

	// See if the sound is looping.
	m_Loops = waveFile.ContainsLoop();

    // Create the sound buffer
    br = Create( &m_WaveFormat, dwFlags, NULL, m_dwBufferSize );
    if( !br )
	{
        return false;
	}

    // Lock the buffer so it can be filled
    VOID* pLock1 = NULL;
    VOID* pLock2 = NULL;
    DWORD dwLockSize1 = 0L;
    DWORD dwLockSize2 = 0L;

	pLock1 = new char[m_dsbd.dwBufferBytes];

    // Read the wave file data into the buffer
    br = waveFile.ReadSample( 0L, pLock1, m_dsbd.dwBufferBytes, NULL );
    if( !br )
	{
        return false;
	}

	// Store pointer to raw sound data.
    m_pRawData = pLock1;

    return true;
}




//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Creates the sound and tells DirectSound where the sound data will be
//       stored. If pBuffer is NULL, DirectSound handles buffer creation.
//-----------------------------------------------------------------------------
bool CXBSound::Create( const XBOXADPCMWAVEFORMAT* pwfxFormat, DWORD dwFlags, VOID* pBuffer, DWORD dwBytes )
{
	// Setup the sound buffer description.
	ZeroMemory( &m_dsbd, sizeof(DSBUFFERDESC) );
	m_dsbd.dwSize      = sizeof(DSBUFFERDESC);
	m_dsbd.dwFlags     = dwFlags;

	if( pwfxFormat->wfx.wFormatTag == WAVE_FORMAT_XBOX_ADPCM )
	{
		m_dsbd.lpwfxFormat	= (LPWAVEFORMATEX)&m_WaveFormat;
	}
	else
	{
		m_dsbd.lpwfxFormat	= (LPWAVEFORMATEX)&pwfxFormat->wfx;
	}

	// If pBuffer is non-NULL, dwBufferBytes will be zero, which informs
	// DirectSoundCreateBuffer that we will presently be using SetBufferData().
	// Otherwise, we set dwBufferBytes to the size of the WAV data, potentially
	// including alignment bytes.
	if( pBuffer == NULL )
	{
		m_dsbd.dwBufferBytes = ( 0 == m_WaveFormat.wfx.nBlockAlign ) ? dwBytes : 
								 dwBytes - ( dwBytes % m_WaveFormat.wfx.nBlockAlign );
	}

	// If buffer specified, tell DirectSound to use it
	if( pBuffer != NULL )
	{
		// Store pointer to raw sound data.
		m_pRawData = pBuffer;
	}

    return true;
}




//-----------------------------------------------------------------------------
// Name: Destroy()
// Desc: Destroys the resources used by the sound
//-----------------------------------------------------------------------------
VOID CXBSound::Destroy()
{
	if( m_pRawData )
	{
		delete [] m_pRawData;
	}
}



//-----------------------------------------------------------------------------
// Name: GetSampleRate
// Desc: Provides the sample rate
//-----------------------------------------------------------------------------
DWORD CXBSound::GetSampleRate( void ) const
{
	return m_WaveFormat.wfx.nSamplesPerSec;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static int getFreeVoice( void )
{
	for( int i = 0; i < NUM_VOICES; ++i )
	{
		if( VoiceSimulator[i].p_buffer == NULL )
		{
			return i;
		}
	}
	return -1;
}



/*****************************************************************************
**								Public Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void InitSoundFX( CSfxManager *p_sfx_manager )
{
	HRESULT				hr;
    LPDSEFFECTIMAGEDESC	pDesc;
    DSEFFECTIMAGELOC	EffectLoc;

	Dbg_Assert( pDirectSound == NULL );

	// Set default volume type.
	if( p_sfx_manager )
	{
		p_sfx_manager->SetDefaultVolumeType( VOLUME_TYPE_5_CHANNEL_DOLBY5_1 );
	}

	// Initialize the 'voice' array.
	for( int i = 0; i < NUM_VOICES; ++i )
	{
		VoiceSimulator[i].p_buffer = NULL;
	}

	// Create the DirectSound interface.
	hr = DirectSoundCreate( NULL, &pDirectSound, NULL );

//	HANDLE hFile = CreateFile( "d:\\data\\sounds\\bin\\dsstdfx.bin", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
	HANDLE hFile = CreateFile( "d:\\data\\sounds\\bin\\skate5fx.bin", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
    if( hFile == INVALID_HANDLE_VALUE )
    {
		Dbg_Assert( 0 );
	}
	else
	{
		// Determine the size of the scratch image by seeking to the end of the file.
		int size = SetFilePointer( hFile, 0, NULL, FILE_END );
        SetFilePointer( hFile, 0, NULL, FILE_BEGIN );
    
		// Allocate memory to read the scratch image from disk
        BYTE* p_buffer = new BYTE[size];

        // Read the image in
        DWORD dwBytesRead;
        BOOL bResult = ReadFile( hFile, p_buffer, size, &dwBytesRead, 0 );
		if( !bResult )
        {
			Dbg_Assert( 0 );
        }
		else
	    {
	        // Call DSound API to download the image.
//			EffectLoc.dwI3DL2ReverbIndex	= I3DL2_CHAIN_I3DL2_REVERB;
//			EffectLoc.dwCrosstalkIndex		= I3DL2_CHAIN_XTALK;
			EffectLoc.dwI3DL2ReverbIndex	= GraphI3DL2_I3DL2Reverb;
			EffectLoc.dwCrosstalkIndex		= GraphXTalk_XTalk;
			hr = pDirectSound->DownloadEffectsImage( p_buffer, size, &EffectLoc, &pDesc );

			delete[] p_buffer;
        }
    }

	if( hFile != INVALID_HANDLE_VALUE ) 
	{
		CloseHandle( hFile );
	}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CleanUpSoundFX( void )
{
	// This just resets the SPU RAM pointer on the PS2. However, on Xbox it needs to explicitly
	// delete any sounds that were not marked as permanent at load time.
	StopAllSoundFX();
	SetReverbPlease( 0 );

	for( int i = 0; i < NumWavesInTable; ++i )
	{
		PlatformWaveInfo* p_info = &( WaveTable[PERM_WAVE_TABLE_MAX_ENTRIES + i].platformWaveInfo );
		Dbg_Assert( p_info->p_sound_data );
		delete p_info->p_sound_data;
		p_info->p_sound_data = NULL;
	}
	NumWavesInTable = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void StopAllSoundFX( void )
{
	Pcm::StopMusic();
	Pcm::StopStreams();

	for( int i = 0; i < NUM_VOICES; ++i )
	{
		StopSoundPlease( i );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int GetMemAvailable( void )
{
	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool LoadSoundPlease( const char *sfxName, uint32 checksum, PlatformWaveInfo *pInfo, bool loadPerm )
{
	Dbg_Assert( pInfo );

	// Need to append file type. Copy string into a buffer.
	static char name_buffer[256] = "sounds\\pcm\\";
	const int SOUND_PREPEND_START_POS = 11;

	// Check there is room to add the extension.
	int length = strlen( sfxName );
	Dbg_Assert( strlen( sfxName ) <= ( 256 - ( SOUND_PREPEND_START_POS + 4 )));
	strcpy( name_buffer + SOUND_PREPEND_START_POS, sfxName );
	strcpy( name_buffer + SOUND_PREPEND_START_POS + length, ".pcm" );

	CXBSound* p_sound = new CXBSound();

	// Pass the locdefer flag along to prevent this buffer taking up a voice because it will not be played directly.
	bool result = p_sound->Create( name_buffer, DSBCAPS_LOCDEFER );

	if( result )
	{
		pInfo->p_sound_data = p_sound;
		pInfo->looping		= p_sound->m_Loops;
		pInfo->permanent	= loadPerm;
	}
	else
	{
		OutputDebugString( name_buffer );
		OutputDebugString( "\n" );
		pInfo->p_sound_data = NULL;
		pInfo->looping		= false;
		pInfo->permanent	= false;
		delete p_sound;
	}
 	return result;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
//int	PlaySoundPlease( PlatformWaveInfo *pInfo, float volL, float volR, float pitch )
int	PlaySoundPlease( PlatformWaveInfo *pInfo, sVolume *p_vol, float pitch )
{
	Dbg_Assert( pInfo );

	if( pInfo->p_sound_data )
	{
		// Obtain a free voice.
		int voice = getFreeVoice();
		if( voice >= 0 )
		{
			// Create the dynamic buffer. Need to set buffer bytes to 0, since we will be setting our own data below.
			DWORD temp_flags	= pInfo->p_sound_data->m_dsbd.dwFlags;
			DWORD buffer_bytes	= pInfo->p_sound_data->m_dsbd.dwBufferBytes;

//			pInfo->p_sound_data->m_dsbd.dwFlags |= DSBCAPS_CTRLPOSITIONNOTIFY;			
			pInfo->p_sound_data->m_dsbd.dwBufferBytes	= 0;

			// Add in the MixBin setup data.			
			DSMIXBINS			dsmixbins;
			DSMIXBINVOLUMEPAIR	dsmbvp[7];
			pInfo->p_sound_data->m_dsbd.lpMixBins	= &dsmixbins;
			dsmixbins.dwMixBinCount					= 7;
			dsmixbins.lpMixBinVolumePairs			= dsmbvp;
			dsmbvp[0].dwMixBin						= DSMIXBIN_3D_FRONT_LEFT;
			dsmbvp[0].lVolume						= DSBVOLUME_EFFECTIVE_MIN;
			dsmbvp[1].dwMixBin						= DSMIXBIN_3D_FRONT_RIGHT;
			dsmbvp[1].lVolume						= DSBVOLUME_EFFECTIVE_MIN;
			dsmbvp[2].dwMixBin						= DSMIXBIN_FRONT_CENTER;
			dsmbvp[2].lVolume						= DSBVOLUME_EFFECTIVE_MIN;
			dsmbvp[3].dwMixBin						= DSMIXBIN_LOW_FREQUENCY;
			dsmbvp[3].lVolume						= DSBVOLUME_EFFECTIVE_MIN;
			dsmbvp[4].dwMixBin						= DSMIXBIN_3D_BACK_LEFT;
			dsmbvp[4].lVolume						= DSBVOLUME_EFFECTIVE_MIN;
			dsmbvp[5].dwMixBin						= DSMIXBIN_3D_BACK_RIGHT;
			dsmbvp[5].lVolume						= DSBVOLUME_EFFECTIVE_MIN;
			dsmbvp[6].dwMixBin						= DSMIXBIN_I3DL2;
			dsmbvp[6].lVolume						= DSBVOLUME_EFFECTIVE_MIN;

			HRESULT hr = DirectSoundCreateBuffer( &pInfo->p_sound_data->m_dsbd, &VoiceSimulator[voice].p_buffer );

			// Reset various buffer description values.
			pInfo->p_sound_data->m_dsbd.dwFlags			= temp_flags;
			pInfo->p_sound_data->m_dsbd.dwBufferBytes	= buffer_bytes;
			pInfo->p_sound_data->m_dsbd.lpMixBins		= NULL;

			++num_buffers;

			if( hr == DS_OK )
			{
				// Set the buffer to point to the raw sound data.
				hr = VoiceSimulator[voice].p_buffer->SetBufferData( pInfo->p_sound_data->m_pRawData, pInfo->p_sound_data->m_dsbd.dwBufferBytes );
				if( hr == DS_OK )
				{
					// Set the default sample rate.
					VoiceSimulator[voice].default_frequency = pInfo->p_sound_data->GetSampleRate();

					// Play the sound.
					hr = VoiceSimulator[voice].p_buffer->Play( 0, 0, ( pInfo->looping ) ? DSBPLAY_LOOPING : 0 );
					if( hr == DS_OK )
					{
//						SetVoiceParameters( voice, volL, volR, pitch );
						SetVoiceParameters( voice, p_vol, pitch );
						return voice;
					}
				}
			}
		}
	}

	return -1;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void StopSoundPlease( int voice )
{
	if( VoiceSimulator[voice].p_buffer )
	{
		HRESULT hr = VoiceSimulator[voice].p_buffer->Stop();
		Dbg_Assert( hr == DS_OK )
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SetVolumePlease( float volumeLevel )
{
	gSfxVolume = volumeLevel;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PauseSoundsPlease( void )
{
	sVolume vol;
	vol.SetSilent();
	
	// Just turn the volume down on all playing voices...
	for( int i = 0; i < NUM_VOICES; i++ )
	{
		if( VoiceIsOn( i ))
		{
//			SetVoiceParameters( i, 0.0f, 0.0f );
			SetVoiceParameters( i, &vol );
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void SetReverbPlease( float reverbLevel, int reverbMode, bool instant )
{
	if( reverbMode == 0 )
	{
		// Default to plain.
		reverbMode = 20;
	}
	reverbMode = 20;
	
	HRESULT hr = pDirectSound->SetI3DL2Listener( &reverbEnvironments[reverbMode].ds3dl, DS3D_IMMEDIATE );
	Dbg_Assert( hr == DS_OK );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool VoiceIsOn( int voice )
{
	return( VoiceSimulator[voice].p_buffer != NULL );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
//void SetVoiceParameters( int voice, float volL, float volR, float pitch )
void SetVoiceParameters( int voice, sVolume *p_vol, float pitch )
{
	if( VoiceSimulator[voice].p_buffer )
	{
		// This array will hold individual volumes for the five speakers.
		// In order, they are: front left, center, front right, rear right, rear left.
		float	volumes[5];
		int		i_volumes[5], max_i_volume;

		memset( volumes, 0, sizeof( float ) * 5 );
		
		if( p_vol->IsSilent())
		{
			// Pointless doing any more work.
		}
		else
		{
			switch( p_vol->GetVolumeType())
			{
				case VOLUME_TYPE_5_CHANNEL_DOLBY5_1:
				{
					// All the work has been done already - just copy volumes over.
					volumes[0] = p_vol->GetChannelVolume( 0 );
					volumes[1] = p_vol->GetChannelVolume( 4 );
					volumes[2] = p_vol->GetChannelVolume( 1 );
					volumes[3] = p_vol->GetChannelVolume( 2 );
					volumes[4] = p_vol->GetChannelVolume( 3 );
					break;
				}

				case VOLUME_TYPE_BASIC_2_CHANNEL:
				{
					// Simple two channel sound, so just setup as such.
					volumes[0] = p_vol->GetChannelVolume( 0 );
					volumes[1] = ( p_vol->GetChannelVolume( 0 ) + p_vol->GetChannelVolume( 1 )) * 0.5f;
					volumes[2] = p_vol->GetChannelVolume( 1 );
					volumes[3] = p_vol->GetChannelVolume( 0 ) * 0.5f;
					volumes[4] = p_vol->GetChannelVolume( 1 ) * 0.5f;
					break;
				}

				case VOLUME_TYPE_2_CHANNEL_DOLBYII:
				default:
				{
					// Wes should not get any of these.
					Dbg_Assert( 0 );

#					if 0
					// Get the length of the vector here which will be used to multiply out the normalised speaker volumes.
					Mth::Vector test( fabsf( p_vol->GetChannelVolume( 0 )), fabsf( p_vol->GetChannelVolume( 1 )), 0.0f, 0.0f );
					float amplitude = test.Length();
				
					// Look just at the normalized right component to figure the sound angle from Matt's calculations.
					test.Normalize();

					float angle;
					angle	= asinf( test[Y] );
					angle	= ( angle * 2.0f ) - ( Mth::PI * 0.5f );
					angle	= ( p_vol->GetChannelVolume( 0 ) < 0.0f ) ? ( Mth::PI - angle ) : angle;
				
				
					// Now figure volumes based on speaker coverage.
					angle	= Mth::RadToDeg( angle );

					// Left front channel.
					if(( angle >= 225.0f ) || ( angle <= 45.0f ))
					{
						// Because of the discontinuity, shift this angle round to the [0,180] range.
						float shift_angle = angle + 135;
						shift_angle = ( shift_angle >= 360.0f ) ? ( shift_angle - 360.0f ) : shift_angle;
						volumes[0]	= ( shift_angle / 180.0f ) * Mth::PI;
						volumes[0]	= sqrtf( sinf( volumes[0] ));
					}
				
					// Center channel.
					if(( angle >= -60.0f ) && ( angle <= 60.0f ))
					{
						// Scale this into [0,PI] space so we can get smooth fadeout.
						volumes[1]	= (( angle + 60.0f ) / 120.0f ) * Mth::PI;
						volumes[1]	= sqrtf( sinf( volumes[1] ));
					}
		
					// Right front channel.
					if(( angle >= -45.0f ) && ( angle <= 135.0f ))
					{
						// Scale this into [0,PI] space so we can get smooth fadeout.
						volumes[2]	= (( angle + 45.0f ) / 180.0f ) * Mth::PI;
						volumes[2]	= sqrtf( sinf( volumes[2] ));
					}

					// Right rear channel.
					if(( angle >= 45.0f ) && ( angle <= 225.0f ))
					{
						// Scale this into [0,PI] space so we can get smooth fadeout.
						volumes[3]	= (( angle - 45.0f ) / 180.0f ) * Mth::PI;
						volumes[3]	= sqrtf( sinf( volumes[3] ));
					}

					// Left rear channel.
					if(( angle >= 135.0f ) || ( angle <= -45.0f ))
					{
						// Because of the discontinuity, shift this angle round to the [0,180] range.
						float shift_angle = angle + 225;
						shift_angle = ( shift_angle >= 360.0f ) ? ( shift_angle - 360.0f ) : shift_angle;
						volumes[4]	= ( shift_angle / 180.0f ) * Mth::PI;
						volumes[4]	= sqrtf( sinf( volumes[4] ));
					}

					// Scale back up to original amplitude.
					for( int v = 0; v < 5; ++v )
					{
						volumes[v] *= amplitude;
					}
#					endif
					break;
				}
			}
		}

		// Now readjust the relative values...
		for( int v = 0; v < 5; ++v )
		{
			// Adjust for SFX volume level, and scale into limits.
			volumes[v] = fabsf( PERCENT( gSfxVolume, volumes[v] ));

				if( volumes[v] > 100.0f )
					volumes[v] = 100.0f;
		}
		
		// Now figure the attenuation of the sound. To convert to a decibel value, figure the ratio of requested
		// volume versus max volume, then calculate the log10 and multiply by (10 * 2). (The 2 is because sound
		// power varies as square of pressure, and squaring doubles the log value).
		max_i_volume = DSBVOLUME_EFFECTIVE_MIN;
		for( int v = 0; v < 5; ++v )
		{
			if( volumes[v] > 0.0f )
			{
				float attenuation	= 20.0f * log10f( volumes[v] * 0.01f );
				i_volumes[v]		= DSBVOLUME_MAX + (int)( attenuation * 100.0f );
				if( i_volumes[v] < DSBVOLUME_EFFECTIVE_MIN )
					i_volumes[v] = DSBVOLUME_EFFECTIVE_MIN;

				if( i_volumes[v] > max_i_volume )
					max_i_volume = i_volumes[v];
			}
			else
			{
				i_volumes[v] = DSBVOLUME_EFFECTIVE_MIN;
			}
		}
		
		// Set individual mixbins for panning.
		DSMIXBINS			dsmixbins;
		DSMIXBINVOLUMEPAIR	dsmbvp[DSMIXBIN_ASSIGNMENT_MAX];

		dsmixbins.dwMixBinCount			= 0;
		dsmixbins.lpMixBinVolumePairs	= dsmbvp;

		if( i_volumes[0] > DSBVOLUME_EFFECTIVE_MIN )
		{
            dsmbvp[dsmixbins.dwMixBinCount].dwMixBin	= DSMIXBIN_3D_FRONT_LEFT;
            dsmbvp[dsmixbins.dwMixBinCount].lVolume		= i_volumes[0];
            dsmixbins.dwMixBinCount++;
		}

		if( i_volumes[1] > DSBVOLUME_EFFECTIVE_MIN )
		{
            dsmbvp[dsmixbins.dwMixBinCount].dwMixBin	= DSMIXBIN_FRONT_CENTER;
            dsmbvp[dsmixbins.dwMixBinCount].lVolume		= i_volumes[1];
            dsmixbins.dwMixBinCount++;
		}

		if( i_volumes[2] > DSBVOLUME_EFFECTIVE_MIN )
		{
            dsmbvp[dsmixbins.dwMixBinCount].dwMixBin	= DSMIXBIN_3D_FRONT_RIGHT;
            dsmbvp[dsmixbins.dwMixBinCount].lVolume		= i_volumes[2];
            dsmixbins.dwMixBinCount++;
		}

		if( i_volumes[3] > DSBVOLUME_EFFECTIVE_MIN )
		{
            dsmbvp[dsmixbins.dwMixBinCount].dwMixBin	= DSMIXBIN_3D_BACK_RIGHT;
            dsmbvp[dsmixbins.dwMixBinCount].lVolume		= i_volumes[3];
            dsmixbins.dwMixBinCount++;
		}

		if( i_volumes[4] > DSBVOLUME_EFFECTIVE_MIN )
		{
            dsmbvp[dsmixbins.dwMixBinCount].dwMixBin	= DSMIXBIN_3D_BACK_LEFT;
            dsmbvp[dsmixbins.dwMixBinCount].lVolume		= i_volumes[4];
            dsmixbins.dwMixBinCount++;
		}

		if( dsmixbins.dwMixBinCount > 0 )
		{
            dsmbvp[dsmixbins.dwMixBinCount].dwMixBin	= DSMIXBIN_LOW_FREQUENCY;
            dsmbvp[dsmixbins.dwMixBinCount].lVolume		= max_i_volume;
            dsmixbins.dwMixBinCount++;

            dsmbvp[dsmixbins.dwMixBinCount].dwMixBin	= DSMIXBIN_I3DL2;
            dsmbvp[dsmixbins.dwMixBinCount].lVolume		= DSBVOLUME_MAX;
            dsmixbins.dwMixBinCount++;
		}

		// Set all speaker volumes.
		HRESULT hr = VoiceSimulator[voice].p_buffer->SetMixBinVolumes( &dsmixbins );

		// Set overall buffer volume.
		if( dsmixbins.dwMixBinCount > 0 )
		{
			hr = VoiceSimulator[voice].p_buffer->SetVolume( DSBVOLUME_MAX );
		}
		else
		{
			hr = VoiceSimulator[voice].p_buffer->SetVolume( DSBVOLUME_EFFECTIVE_MIN );

			// Pointless doing pitch calculation if volume is zero.
			return;
		}

		// Same kind of deal for pitch. Each consecutive doubling (or halving) of the pitch percentage
		// raises (or lowers) the pitch by 1 octave. So we need to figure x, where 2 ^ x = percentage / 100.
		// Can do this with logs...
		int ipitch;

		// Matt appears to be doing an extra adjustment to the pitch on the Ps2 to account for sample rates
		// not at 48,000Hz. Whilst this seems an unnecessary step, we need to do the same adjustment here.
		// Adjust pitch to account for lower sampling rates.
//		pitch = PERCENT( pitch, ((float)VoiceSimulator[voice].default_frequency / 48000.0f ) * 100.0f );
		
		if( pitch != 100.0f )
		{
			ipitch = (int)(( VoiceSimulator[voice].default_frequency * pitch ) * 0.01f );
			if( ipitch > DSBFREQUENCY_MAX )
			{
				ipitch = DSBFREQUENCY_MAX;
			}
			else if( ipitch < DSBFREQUENCY_MIN )
			{
				ipitch = DSBFREQUENCY_MIN;
			}
		}
		else
		{
			ipitch = VoiceSimulator[voice].default_frequency;
		}
		hr = VoiceSimulator[voice].p_buffer->SetFrequency( ipitch );
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void PerFrameUpdate( void )
{
	DirectSoundDoWork();

	int num_used_voices = 0;

	for( int i = 0; i < NUM_VOICES; ++i )
	{
		if( VoiceSimulator[i].p_buffer )
		{
			DWORD status;
			HRESULT hr = VoiceSimulator[i].p_buffer->GetStatus( &status );
			if(( hr == DS_OK ) && ( status == 0 ))
			{
				// This sound has stopped playing, so we can release the buffer.
				hr = VoiceSimulator[i].p_buffer->Release();

				--num_buffers;

				Dbg_Assert( hr == 0 );

				VoiceSimulator[i].p_buffer = NULL;
			}
			else
			{
				++num_used_voices;
			}
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Sfx
