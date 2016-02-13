/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:																**
**																			**
**	Module:																	**
**																			**
**	File name:		p_wmafilestream.cpp										**
**																			**
**	Created:		01/27/03	-	dc										**
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
#include <gel/soundfx/soundfx.h>

#include "p_music.h"
#include "p_wmafilestream.h"

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

// Define the source packet size:
// This value is hard-coded assuming a WMA file of stero, 16bit resolution.  If
// this value can by dynamically set based on the wma format, keeping in mind
// that wma needs enough buffer for a minimum of 2048 samples worth of PCM data
#define WMASTRM_SOURCE_PACKET_BYTES	( 2048 * 2 * 2 )



// This is a homegrown value which is used in conjunction with the existing
// DSound packet status values. This indicates a packet which has been written
// to, but which has not yet been submitted to the renderer. The low-order 24
// bits contain a timestamp. Packets should be submitted lowest-timestamp first.
#define XMEDIAPACKET_STATUS_AWAITING_RENDER		0x40000000UL

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

uint32 CALLBACK WMAXMediaObjectDataCallback( LPVOID pContext, uint32 offset, uint32 num_bytes, LPVOID *ppData )
{
	Dbg_Assert( pContext != NULL );
	Dbg_Assert( ppData != NULL );

	CWMAFileStream *p_this = (CWMAFileStream*)pContext;
	Dbg_Assert(( p_this->m_DecoderCreation == 0 ) || ( p_this->m_DecoderCreation == 1 ));

	*ppData = (BYTE*)( p_this->m_pFileBuffer ) + ( offset % ( 8 * 8192 ));

    // Update current progress.
    p_this->m_FileBytesProcessed	= offset;

    return num_bytes;
}



/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

//-----------------------------------------------------------------------------
// Name: CWMAFileStream()
// Desc: Object constructor.
//-----------------------------------------------------------------------------
CWMAFileStream::CWMAFileStream( bool use_3d )
{
    m_pSourceFilter		= NULL;
    m_pRenderFilter		= NULL;
    m_pvSourceBuffer	= NULL;
    m_pFileBuffer		= NULL;
	m_hFile				= INVALID_HANDLE_VALUE;
	m_hThread			= NULL;
	m_bUse3D			= use_3d;
	m_bOkayToPlay		= true;

    for( uint32 i = 0; i < WMASTRM_PACKET_COUNT; i++ )
	{
        m_adwPacketStatus[i] = XMEDIAPACKET_STATUS_SUCCESS;
	}

    m_dwStartingDataOffset	= 0;
	m_Completed				= false;
	m_Paused				= false;

	// Grab a pointer to the next overlapped structure.
	m_pOverlapped			= PCMAudio_GetNextOverlapped();
}



//-----------------------------------------------------------------------------
// Name: ~CWMAFileStream()
// Desc: Object destructor.
//-----------------------------------------------------------------------------
CWMAFileStream::~CWMAFileStream()
{
	if( m_hThread )
	{
		CloseHandle( m_hThread );
	}
	
	// If the file i/o is still active, we need to remove the event and close the file.
	if(( m_hFile != INVALID_HANDLE_VALUE ) && !m_bUseWAD )
	{
		CloseHandle( m_hFile );
	}

    if( m_pSourceFilter )
	{
		m_pSourceFilter->Release();
	}

    if( m_pRenderFilter )
	{
		m_pRenderFilter->Release();
	}

	if( m_pvSourceBuffer )
	{
		delete[] m_pvSourceBuffer;
	}
}



//-----------------------------------------------------------------------------
// Name: AsyncRead()
// Desc: Called while the async read is in progress. If the read is still
//       underway, simply returns. If the read has completed, sets up the
//       next read. If the file has been completely read, closes the file.
//-----------------------------------------------------------------------------
void CWMAFileStream::AsyncRead( void )
{
	Dbg_Assert( m_hFile != INVALID_HANDLE_VALUE );

    // If paused, do nothing.
	if( m_Paused )
	{
        return;
    }

    // See if the previous read is complete.
	uint32	dwBytesTransferred;
	bool	bIsReadDone = GetOverlappedResult( m_hFile, m_pOverlapped, &dwBytesTransferred, false );
	uint32	dwLastError = GetLastError();

    // If the read isn't complete, keep going.
	if( !bIsReadDone )
	{
		Dbg_Assert( dwLastError == ERROR_IO_INCOMPLETE );
		if( dwLastError != ERROR_IO_INCOMPLETE )
		{
			m_AwaitingDeletion = true;
		}
		return;
    }

    // If we get here, the read is complete.
    m_pOverlapped->Offset	+= dwBytesTransferred;
	m_FileBytesRead			+= dwBytesTransferred;

	// If we just read to the first block in the buffer, copy to the extra block at the end, to ensure jitter-free playback.
	if(( m_SuccessiveReads & 0x07 ) == 0 )
	{
		CopyMemory((BYTE*)m_pFileBuffer + ( 8 * 8192 ), (BYTE*)m_pFileBuffer, 8192 );
	}

	++m_SuccessiveReads;

#	define BYTES_PER_CALL	8192

	if( dwBytesTransferred < 8192 )
	{ 
		// We've reached the end of the file during the call to ReadFile.

        // Close the file (if not using the global WAD file).
		if( !m_bUseWAD )
		{
			bool bSuccess = CloseHandle( m_hFile );
	        Dbg_Assert( bSuccess );
		}

        m_hFile = INVALID_HANDLE_VALUE;

        // All done
		m_ReadComplete = true;
    }
    else
    {
		if( m_bUseWAD && ( m_FileBytesRead >= (int)m_dwWADLength ))
		{
			m_hFile = INVALID_HANDLE_VALUE;

			// All done
			m_ReadComplete = true;
			return;
		}
		
		// We still have more data to read. Start another asynchronous read from the file.
		bool bComplete	= ReadFile( m_hFile, (BYTE*)m_pFileBuffer + ( m_FileBytesRead % ( 8 * 8192 )), BYTES_PER_CALL, NULL, m_pOverlapped );
		dwLastError		= GetLastError();

		// Deal with hitting EOF (for files that are some exact multiple of 8192 bytes).
		if( bComplete || ( !bComplete && ( dwLastError == ERROR_HANDLE_EOF )))
		{
			// Close the file
			if( !m_bUseWAD )
			{
				bool bSuccess = CloseHandle( m_hFile );
				Dbg_Assert( bSuccess );
			}
			m_hFile = INVALID_HANDLE_VALUE;

			// All done
			m_ReadComplete = true;
		}
		else
		{
			Dbg_MsgAssert( bComplete || ( !bComplete && ( dwLastError == ERROR_IO_PENDING )), ( "ReadFile error: %x\n", dwLastError ));
			if( !bComplete && ( dwLastError != ERROR_IO_PENDING ))
			{
				// There was a problem, so shut this stream down.
				m_AwaitingDeletion = true;
			}
		}
    }
}



//-----------------------------------------------------------------------------
// Name: Initialize()
// Desc: Initializes the wave file streaming subsystem.
//-----------------------------------------------------------------------------
HRESULT CWMAFileStream::Initialize( HANDLE h_file, unsigned int offset, unsigned int length, void* fileBuffer )
{
    m_dwPercentCompleted = 0;
    
	// At this stage we don't want to create the decoder or the stream. We do want to just allocate the read
	// buffer, and start pulling in the data. Once sufficient data has been grabbed, we can analyze the header
	// and create the required objects for playback.
	m_hFile			= h_file;
	m_bUseWAD		= true;
	m_dwWADOffset	= offset;	
	m_dwWADLength	= length;
	
	// Grab a 72k (9 * 8k packets) read buffer. Must be uint32 aligned.
	// The last packet mirrors the first packet, for cases when the decoder reads past the end of the ring buffer.
	m_pFileBuffer = fileBuffer;
	Dbg_Assert( uint32( m_pFileBuffer ) % sizeof( uint32 ) == 0 );

	return PostInitialize();
}



//-----------------------------------------------------------------------------
// Name: Initialize()
// Desc: Initializes the wave file streaming subsystem.
//-----------------------------------------------------------------------------
HRESULT CWMAFileStream::Initialize( HANDLE h_song, void* fileBuffer )
{
    HRESULT			hr;

    m_dwPercentCompleted = 0;
    
	// At this stage we don't want to create the decoder or the stream. We do want to just allocate the read
	// buffer, and start pulling in the data. Once sufficient data has been grabbed, we can analyze the header
	// and create the required objects for playback.
	m_hFile = h_song;
	if( m_hFile == INVALID_HANDLE_VALUE )
	{
		hr = HRESULT_FROM_WIN32( GetLastError());
        return hr;
    }

	// Grab a 72k (9 * 8k packets) read buffer. Must be uint32 aligned.
	// The last packet mirrors the first packet, for cases when the decoder reads past the end of the ring buffer.
	m_pFileBuffer = fileBuffer;
	Dbg_Assert( uint32( m_pFileBuffer ) % sizeof( uint32 ) == 0 );

	return PostInitialize();
}



//-----------------------------------------------------------------------------
// Name: PostInitialize()
// Desc: Initialisation stuff following the file creation
//-----------------------------------------------------------------------------
HRESULT CWMAFileStream::PostInitialize( void )
{
	// Start the asynchronous read from the start of the file.
	m_FirstRead				= true;
	m_ReadComplete			= false;
	m_FileBytesRead			= 0;
	m_FileBytesProcessed	= 0;
	m_SuccessiveReads		= 0;

	if( m_bUseWAD )
	{
		m_pOverlapped->Offset	= m_dwWADOffset;
	}
	else
	{
		m_pOverlapped->Offset	= 0;
	}
	m_pOverlapped->OffsetHigh	= 0;

	bool bComplete = ReadFile( m_hFile, m_pFileBuffer, BYTES_PER_CALL, NULL, m_pOverlapped );
	if( !bComplete )
	{
		uint32 dwLastError = GetLastError();
		Dbg_Assert( dwLastError == ERROR_IO_PENDING );
		if( dwLastError != ERROR_IO_PENDING )
		{
			return HRESULT_FROM_WIN32( dwLastError );
		}
	}
	else
	{
		m_ReadComplete = true;
	}

	// That's it for now.
    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: CreateSourceBuffer()
// Desc: 
//-----------------------------------------------------------------------------
void CWMAFileStream::CreateSourceBuffer( void )
{
	// Allocate data buffers. The source buffer holds the CPU decompressed packets ready to submit to the stream.
	// Explicitly allocate from the bottom up heap.
	Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().BottomUpHeap());
	m_pvSourceBuffer = new BYTE[WMASTRM_SOURCE_PACKET_BYTES * WMASTRM_PACKET_COUNT];
	Mem::Manager::sHandle().PopContext();
	Dbg_Assert( m_pvSourceBuffer != NULL );
}



//-----------------------------------------------------------------------------
// Name: PreLoadDone()
// Desc: 
//-----------------------------------------------------------------------------
bool CWMAFileStream::PreLoadDone( void )
{
	if( m_DecoderCreation == 1 )
	{
		if( m_ReadComplete || ( m_FileBytesRead >= ( m_FileBytesProcessed + 8192 )))
		{
			return true;
		}
	}
	return false;
}



//-----------------------------------------------------------------------------
// Name: Process()
// Desc: Performs any work necessary to keep the stream playing.
//-----------------------------------------------------------------------------
HRESULT CWMAFileStream::Process( void )
{
    HRESULT			hr;
	DSSTREAMDESC	dssd;
    uint32			dwPacketIndex;
    
	// Do nothing if waiting to die.
	if( m_AwaitingDeletion )
	{
		return S_OK;
	}

	// Do we need to kick off another read of data? Don't read anymore if we have read ahead more than 32k.
	if( !m_ReadComplete )
	{
		if( m_FileBytesRead > ( m_FileBytesProcessed + 32768 ))
		{
//			OutputDebugString( "waiting...\n" );
		}
		else
		{
			AsyncRead();
//			OutputDebugString( "reading...\n" );
		}
	}
	else
	{
//		OutputDebugString( "complete...\n" );
	}

	// Has the first block of raw data been read? If so we need to instantiate the playback objects and data buffers.
	if( m_FirstRead && ( m_FileBytesRead > 1024 ))
	{
		if( m_pSourceFilter == NULL )
		{
			// Create the thread which will create the in-memory decoder. 
			m_DecoderCreation = 0;

			HRESULT hr = WmaCreateInMemoryDecoder(	WMAXMediaObjectDataCallback,			// Callback pointer.
													this,									// Callback context pointer.
													3,										// Yield rate during decoding.
													&m_wfxSourceFormat,						// Source format description.
													(LPXMEDIAOBJECT*)&m_pSourceFilter );	// Pointer to media object.
			if( FAILED( hr ))
			{
				// Signal a failed creation.
				m_DecoderCreation	= 2;
				m_pSourceFilter		= NULL;
			}
			else
			{
				m_DecoderCreation	= 1;
			}
			
			// That's all we can do until the decoder has been instantiated.
		}

		if( m_DecoderCreation == 0 )
		{
			// Still waiting to create decoder.
			return S_OK;
		}
		else if( m_DecoderCreation == 2 )
		{
			// Failed to create decoder, just mark for deletion.
			m_AwaitingDeletion = true;
			return S_OK;
		}
		else if( m_DecoderCreation == 1 )
		{
			// Managed to create decoder.
			Dbg_Assert( m_pSourceFilter != NULL );

			m_FirstRead = false;

			// Create the render (DirectSoundStream) filter.
			DSMIXBINS			dsmixbins;
			DSMIXBINVOLUMEPAIR	dsmbvp[7];
			ZeroMemory( &dssd, sizeof( dssd ));

			dssd.dwFlags					= 0;
			dssd.dwMaxAttachedPackets		= WMASTRM_PACKET_COUNT;
			dssd.lpwfxFormat				= &m_wfxSourceFormat;
			dssd.lpMixBins					= &dsmixbins;

			if( m_bUse3D )
			{
				// This is only designed to be fed a mono signal.
				Dbg_Assert( m_wfxSourceFormat.nChannels == 1 );

				dsmixbins.dwMixBinCount			= 6;
				dsmixbins.lpMixBinVolumePairs	= dsmbvp;
				dsmbvp[0].dwMixBin				= DSMIXBIN_3D_FRONT_LEFT;
				dsmbvp[0].lVolume				= DSBVOLUME_EFFECTIVE_MIN;
				dsmbvp[1].dwMixBin				= DSMIXBIN_3D_FRONT_RIGHT;
				dsmbvp[1].lVolume				= DSBVOLUME_EFFECTIVE_MIN;
				dsmbvp[2].dwMixBin				= DSMIXBIN_3D_BACK_LEFT;
				dsmbvp[2].lVolume				= DSBVOLUME_EFFECTIVE_MIN;
				dsmbvp[3].dwMixBin				= DSMIXBIN_3D_BACK_RIGHT;
				dsmbvp[3].lVolume				= DSBVOLUME_EFFECTIVE_MIN;
				dsmbvp[4].dwMixBin				= DSMIXBIN_FRONT_CENTER;
				dsmbvp[4].lVolume				= DSBVOLUME_EFFECTIVE_MIN;
				dsmbvp[5].dwMixBin				= DSMIXBIN_I3DL2;
				dsmbvp[5].lVolume				= DSBVOLUME_EFFECTIVE_MIN;

				m_Mixbins						= ( 1 << DSMIXBIN_3D_FRONT_LEFT ) |
												  ( 1 << DSMIXBIN_3D_FRONT_RIGHT ) |
												  ( 1 << DSMIXBIN_3D_BACK_LEFT ) |
												  ( 1 << DSMIXBIN_3D_BACK_RIGHT ) |
												  ( 1 << DSMIXBIN_FRONT_CENTER ) |
												  ( 1 << DSMIXBIN_I3DL2 );
				m_NumMixbins					= dsmixbins.dwMixBinCount;
			}
			else
			{
				// If we are playing a music track, and if proper 5.1 output is selected, we want to feed the music to the left and
				// right back speakers with a slight echo via mixbins 5 and 6.
				if( XGetAudioFlags() & XC_AUDIO_FLAGS_ENABLE_AC3 )
				{
					// This is only designed to be fed a stereo signal.
					Dbg_Assert( m_wfxSourceFormat.nChannels == 2 );

					dsmixbins.dwMixBinCount			= 4;
					dsmixbins.lpMixBinVolumePairs	= dsmbvp;
					dsmbvp[0].dwMixBin				= DSMIXBIN_FRONT_LEFT;
					dsmbvp[0].lVolume				= DSBVOLUME_MIN;
					dsmbvp[1].dwMixBin				= DSMIXBIN_FRONT_RIGHT;
					dsmbvp[1].lVolume				= DSBVOLUME_MIN;
					dsmbvp[2].dwMixBin				= DSMIXBIN_FXSEND_5;
					dsmbvp[2].lVolume				= DSBVOLUME_MIN;
					dsmbvp[3].dwMixBin				= DSMIXBIN_FXSEND_6;
					dsmbvp[3].lVolume				= DSBVOLUME_MIN;

					m_Mixbins						= ( 1 << DSMIXBIN_FRONT_LEFT ) |
													  ( 1 << DSMIXBIN_FRONT_RIGHT ) |
													  ( 1 << DSMIXBIN_FXSEND_5 ) |
													  ( 1 << DSMIXBIN_FXSEND_6 );
					m_NumMixbins					= dsmixbins.dwMixBinCount;
				}
				else
				{
					// This is only designed to be fed a stereo signal.
					Dbg_Assert( m_wfxSourceFormat.nChannels == 2 );

					dsmixbins.dwMixBinCount			= 4;
					dsmixbins.lpMixBinVolumePairs	= dsmbvp;
					dsmbvp[0].dwMixBin				= DSMIXBIN_FRONT_LEFT;
					dsmbvp[0].lVolume				= DSBVOLUME_MIN;
					dsmbvp[1].dwMixBin				= DSMIXBIN_FRONT_RIGHT;
					dsmbvp[1].lVolume				= DSBVOLUME_MIN;
					dsmbvp[2].dwMixBin				= DSMIXBIN_BACK_LEFT;
					dsmbvp[2].lVolume				= DSBVOLUME_MIN;
					dsmbvp[3].dwMixBin				= DSMIXBIN_BACK_RIGHT;
					dsmbvp[3].lVolume				= DSBVOLUME_MIN;

					m_Mixbins						= ( 1 << DSMIXBIN_FRONT_LEFT ) |
													  ( 1 << DSMIXBIN_FRONT_RIGHT ) |
													  ( 1 << DSMIXBIN_BACK_LEFT ) |
													  ( 1 << DSMIXBIN_BACK_RIGHT );
					m_NumMixbins					= dsmixbins.dwMixBinCount;
				}
			}

			hr = DirectSoundCreateStream( &dssd, &m_pRenderFilter );
			if( FAILED( hr ))
			{
				Dbg_Assert( 0 );
				m_pRenderFilter		= NULL;
				m_AwaitingDeletion	= true;
				return S_OK;
			}

			// Set deferred volume if present.
			SetDeferredVolume();

			// Handle deferred pause.
			if( m_Paused )
			{
				m_pRenderFilter->Pause( DSSTREAMPAUSE_PAUSE );
			}
		}
	}

	// We only want to do processing when there is sufficient data available.
	if( m_ReadComplete || ( m_FileBytesRead >= ( m_FileBytesProcessed + 8192 )))
	{
		if( !m_Completed && m_pSourceFilter && m_pRenderFilter )
		{
			// Find a free packet. If there's none free, we don't have anything to do.
			if( FindFreePacket( &dwPacketIndex ))
			{
				// Read from the source filter.
				hr = ProcessSource( dwPacketIndex );
				if( FAILED( hr ) )
				{
					return hr;
				}
			}

			// Don't want to submit if we're paused, or we're flagged not to play. (Flagging is used to allow
			// the stream to be spooled up ready to play instantly when triggered).
			if(( !m_Paused ) && m_bOkayToPlay )
			{
				if( FindRenderablePacket( &dwPacketIndex ))
				{
					// Send the data to the renderer
					hr = ProcessRenderer( dwPacketIndex );
					if( FAILED( hr ))
					{
						 return hr;
					}
				}
			}
		}
	}
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FindFreePacket()
// Desc: Finds a render packet available for processing.
//-----------------------------------------------------------------------------
bool CWMAFileStream::FindFreePacket( uint32* pdwPacketIndex )
{
    for( uint32 dwPacketIndex = 0; dwPacketIndex < WMASTRM_PACKET_COUNT; ++dwPacketIndex )
    {
		if(( m_adwPacketStatus[dwPacketIndex] != XMEDIAPACKET_STATUS_PENDING ) &&
		   (( m_adwPacketStatus[dwPacketIndex] & XMEDIAPACKET_STATUS_AWAITING_RENDER ) != XMEDIAPACKET_STATUS_AWAITING_RENDER ))
        {
            if( pdwPacketIndex )
			{
				*pdwPacketIndex = dwPacketIndex;

				// Mark this packet as awaiting rendering.
				uint32 timestamp = (uint32)Tmr::GetVblanks();
				m_adwPacketStatus[dwPacketIndex] = XMEDIAPACKET_STATUS_AWAITING_RENDER | ( timestamp & 0xFFFFFFUL );
			}
            return true;
        }
    }
    return false;
}



//-----------------------------------------------------------------------------
// Name: FindFreePacket()
// Desc: Finds a render packet available for rendering.
//-----------------------------------------------------------------------------
bool CWMAFileStream::FindRenderablePacket( uint32* pdwPacketIndex )
{
	// Found nothing yet.
	uint32 lowest_timestamp	= 0xFFFFFFUL;
	uint32 lowest_index		= WMASTRM_PACKET_COUNT;

	for( uint32 dwPacketIndex = 0; dwPacketIndex < WMASTRM_PACKET_COUNT; ++dwPacketIndex )
    {
		if(( m_adwPacketStatus[dwPacketIndex] & XMEDIAPACKET_STATUS_AWAITING_RENDER ) == XMEDIAPACKET_STATUS_AWAITING_RENDER )
        {
			uint32 timestamp = m_adwPacketStatus[dwPacketIndex] & 0xFFFFFFUL;
			if( timestamp < lowest_timestamp )
			{
				lowest_timestamp	= timestamp;
				lowest_index		= dwPacketIndex;
			}
		}
    }

	// If we found a packet, return its index.
	if( lowest_index < WMASTRM_PACKET_COUNT )
	{
		if( pdwPacketIndex )
		{
			*pdwPacketIndex = lowest_index;
		}
		return true;
	}

	// No packets awaiting rendering.
	return false;
}



//-----------------------------------------------------------------------------
// Name: ProcessSource()
// Desc: Reads data from the source filter.
//-----------------------------------------------------------------------------
HRESULT CWMAFileStream::ProcessSource( uint32 dwPacketIndex )
{
    uint32        dwTotalSourceUsed   = 0;
    uint32        dwSourceUsed;
    XMEDIAPACKET xmp;
    HRESULT      hr;
    
	if( m_pvSourceBuffer == NULL )
	{
		CreateSourceBuffer();
	}
    
	// We're going to read a full packet's worth of data into the source buffer.
    ZeroMemory( &xmp, sizeof( xmp ));
    xmp.pvBuffer         = (BYTE*)m_pvSourceBuffer + (dwPacketIndex * WMASTRM_SOURCE_PACKET_BYTES);
    xmp.dwMaxSize        = WMASTRM_SOURCE_PACKET_BYTES;
    xmp.pdwCompletedSize = &dwSourceUsed;

	// Read from the source.
	hr = m_pSourceFilter->Process( NULL, &xmp );
	if( FAILED( hr ))
	{
		return hr;
	}

	// Add the amount read to the total
	dwTotalSourceUsed += dwSourceUsed;

	// If we read less than the amount requested, it's because we hit the end of the file.
    if( dwSourceUsed < xmp.dwMaxSize )
	{
		// Set completion flag.
		m_Completed = true;

		// Zero remaining part of packet.
		xmp.pvBuffer  = (BYTE*)xmp.pvBuffer + dwSourceUsed;
		xmp.dwMaxSize = xmp.dwMaxSize - dwSourceUsed;
		ZeroMemory( xmp.pvBuffer, xmp.dwMaxSize );
	}

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ProcessRenderer()
// Desc: Sends data to the renderer.
//-----------------------------------------------------------------------------
HRESULT CWMAFileStream::ProcessRenderer( uint32 dwPacketIndex )
{
    XMEDIAPACKET xmp;
    HRESULT      hr;

	Dbg_Assert( m_pvSourceBuffer != NULL );

	// There's a full packet's worth of data ready for us to send to the renderer.  We want to track the status
	// of this packet since the render filter is asychronous and we need to know when the packet is completed.
    ZeroMemory( &xmp, sizeof( xmp ));
    xmp.pvBuffer  = (BYTE*)m_pvSourceBuffer + ( dwPacketIndex * WMASTRM_SOURCE_PACKET_BYTES );
    xmp.dwMaxSize = WMASTRM_SOURCE_PACKET_BYTES;
    xmp.pdwStatus = &m_adwPacketStatus[dwPacketIndex];

	if( m_Completed )
	{
		// Store index of last packet, since we will need to test the status of this for proper completion test.
		m_LastPacket = dwPacketIndex;
	}

    hr = m_pRenderFilter->Process( &xmp, NULL );

	if( m_Completed )
	{
		// Tell the renderer not to expect any more data.
		m_pRenderFilter->Discontinuity();
	}
	
	if( FAILED( hr ))
	{
		return hr;
	}
	
	return S_OK;
}



//-----------------------------------------------------------------------------
// Name: Pause
// Desc: Pauses and resumes stream playback
//-----------------------------------------------------------------------------
void CWMAFileStream::Pause( uint32 dwPause )
{
	m_Paused = ( dwPause > 0 );

	// Possible that the render filter hasn't been created yet.
	if( m_pRenderFilter )
	{
		m_pRenderFilter->Pause(( dwPause > 0 ) ? DSSTREAMPAUSE_PAUSE : DSSTREAMPAUSE_RESUME );
	}
}



//-----------------------------------------------------------------------------
// Name: SetVolume
// Desc: 
//-----------------------------------------------------------------------------
void CWMAFileStream::SetVolume( float volume )
{
	if( m_pRenderFilter )
	{
		int i_volume		= DSBVOLUME_EFFECTIVE_MIN;
		int i_volume_rear	= DSBVOLUME_EFFECTIVE_MIN;
		if( volume > 0.0f )
		{
			// Figure base volume.
			float attenuation	= 20.0f * log10f( volume * 0.01f );
			i_volume			= DSBVOLUME_MAX + (int)( attenuation * 100.0f );
			if( i_volume < DSBVOLUME_EFFECTIVE_MIN )
				i_volume = DSBVOLUME_EFFECTIVE_MIN;
			else if( i_volume > DSBVOLUME_MAX )
				i_volume = DSBVOLUME_MAX;

			// Also figure half volume, in case we are routing to the back speakers.
			attenuation			= 20.0f * log10f( volume * 0.5f * 0.01f );
			i_volume_rear		= DSBVOLUME_MAX + (int)( attenuation * 100.0f );
			if( i_volume_rear < DSBVOLUME_EFFECTIVE_MIN )
				i_volume_rear = DSBVOLUME_EFFECTIVE_MIN;
			else if( i_volume_rear > DSBVOLUME_MAX )
				i_volume_rear = DSBVOLUME_MAX;
		}
		
		// Set individual mixbins for panning.
		DSMIXBINS			dsmixbins;
		DSMIXBINVOLUMEPAIR	dsmbvp[DSMIXBIN_ASSIGNMENT_MAX];

		dsmixbins.dwMixBinCount			= 0;
		dsmixbins.lpMixBinVolumePairs	= dsmbvp;

		if( i_volume > DSBVOLUME_EFFECTIVE_MIN )
		{
			// Set the volume up depending on how the initial mixbins were set up.
			int mbbf = 0;
			for( uint32 mb = 0; mb < m_NumMixbins; ++mb )
			{
				while(( m_Mixbins & ( 1 << mbbf )) == 0 )
				{
					++mbbf;
					Dbg_Assert( mbbf < 32 );
				}
				dsmbvp[dsmixbins.dwMixBinCount].dwMixBin	= mbbf;

				// For rear speakers (or mixbins that route to the rear), use half volume.
				if(( mbbf == DSMIXBIN_FXSEND_5 ) || ( mbbf == DSMIXBIN_FXSEND_6 ) || ( mbbf == DSMIXBIN_BACK_LEFT ) || ( mbbf == DSMIXBIN_BACK_RIGHT ))
				{
					dsmbvp[dsmixbins.dwMixBinCount].lVolume		= i_volume_rear;
				}
				else
				{
					dsmbvp[dsmixbins.dwMixBinCount].lVolume		= i_volume;
				}
				++dsmixbins.dwMixBinCount;
				++mbbf;
			}
		}

		// Set all speaker volumes.
		m_pRenderFilter->SetMixBinVolumes( &dsmixbins );

		// Set overall buffer volume.
		if( dsmixbins.dwMixBinCount > 0 )
		{
			m_pRenderFilter->SetVolume( DSBVOLUME_MAX );
		}
		else
		{
			m_pRenderFilter->SetVolume( DSBVOLUME_MIN );
		}
	}
	else
	{
		m_SetDeferredVolume = true;
		m_DeferredVolume[0]	= volume;
	}
}



//-----------------------------------------------------------------------------
// Name: SetVolume
// Desc: 
//-----------------------------------------------------------------------------
void CWMAFileStream::SetVolume( float volL, float volR )
{
	if( m_pRenderFilter )
	{
		// This array will hold individual volumes for the five speakers.
		// In order, they are: front left, center, front right, rear right, rear left.
		float	volumes[5];
		int		i_volumes[5], max_i_volume;
		memset( volumes, 0, sizeof( float ) * 5 );
		
		if(( volL == 0.0f ) && ( volR == 0.0f ))
		{
			// Pointless doing any more work.
		}
		else
		{
			// Get the length of the vector here which will be used to multiply out the normalised speaker volumes.
			Mth::Vector test( fabsf( volL ), fabsf( volR ), 0.0f, 0.0f );
			float amplitude = test.Length();

			// Look just at the normalized right component to figure the sound angle from Matt's calculations.
			test.Normalize();

			float angle;
			angle	= asinf( test[Y] );
			angle	= ( angle * 2.0f ) - ( Mth::PI * 0.5f );
			angle	= ( volL < 0.0f ) ? ( Mth::PI - angle ) : angle;
		
			// Now figure volumes based on speaker coverage.
			angle	= Mth::RadToDeg( angle );
		
			Spt::SingletonPtr< Sfx::CSfxManager > sfx_manager;
			sfx_manager->Get5ChannelMultipliers( angle, &volumes[0] );

			// Now readjust the relative values...
			for( int v = 0; v < 5; ++v )
			{
				// Scale back up to original amplitude.
				volumes[v] *= amplitude;

				if( volumes[v] > 100.0f )
					volumes[v] = 100.0f;
			}
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
				else if( i_volumes[v] > DSBVOLUME_MAX )
					i_volumes[v] = DSBVOLUME_MAX;

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
            dsmbvp[dsmixbins.dwMixBinCount].dwMixBin	= DSMIXBIN_3D_FRONT_RIGHT;
            dsmbvp[dsmixbins.dwMixBinCount].lVolume		= i_volumes[1];
            dsmixbins.dwMixBinCount++;
		}
		if( i_volumes[2] > DSBVOLUME_EFFECTIVE_MIN )
		{
            dsmbvp[dsmixbins.dwMixBinCount].dwMixBin	= DSMIXBIN_3D_BACK_LEFT;
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
            dsmbvp[dsmixbins.dwMixBinCount].dwMixBin	= DSMIXBIN_FRONT_CENTER;
            dsmbvp[dsmixbins.dwMixBinCount].lVolume		= i_volumes[4];
            dsmixbins.dwMixBinCount++;
		}
		if( dsmixbins.dwMixBinCount > 0 )
		{
            dsmbvp[dsmixbins.dwMixBinCount].dwMixBin	= DSMIXBIN_I3DL2;
            dsmbvp[dsmixbins.dwMixBinCount].lVolume		= DSBVOLUME_MAX;
            dsmixbins.dwMixBinCount++;
		}

		// Set all speaker volumes.
		m_pRenderFilter->SetMixBinVolumes( &dsmixbins );

		// Set overall buffer volume.
		if( dsmixbins.dwMixBinCount > 0 )
		{
			m_pRenderFilter->SetVolume( DSBVOLUME_MAX );
		}
		else
		{
			m_pRenderFilter->SetVolume( DSBVOLUME_MIN );
		}
	}
	else
	{
		m_SetDeferredVolumeLR	= true;
		m_DeferredVolume[0]		= volL;
		m_DeferredVolume[1]		= volR;
	}
}



//-----------------------------------------------------------------------------
// Name: SetVolume
// Desc: 
//-----------------------------------------------------------------------------
void CWMAFileStream::SetVolume( float v0, float v1, float v2, float v3, float v4 )
{
	if( m_pRenderFilter )
	{
		float volumes[5];
		volumes[0] = ( v0 > 100.0f ) ? 100.0f : v0;
		volumes[1] = ( v1 > 100.0f ) ? 100.0f : v1;
		volumes[2] = ( v2 > 100.0f ) ? 100.0f : v2;
		volumes[3] = ( v3 > 100.0f ) ? 100.0f : v3;
		volumes[4] = ( v4 > 100.0f ) ? 100.0f : v4;

		int i_volumes[5], max_i_volume;

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
				else if( i_volumes[v] > DSBVOLUME_MAX )
					i_volumes[v] = DSBVOLUME_MAX;

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
            dsmbvp[dsmixbins.dwMixBinCount].dwMixBin	= DSMIXBIN_3D_FRONT_RIGHT;
            dsmbvp[dsmixbins.dwMixBinCount].lVolume		= i_volumes[1];
            dsmixbins.dwMixBinCount++;
		}
		if( i_volumes[2] > DSBVOLUME_EFFECTIVE_MIN )
		{
            dsmbvp[dsmixbins.dwMixBinCount].dwMixBin	= DSMIXBIN_3D_BACK_LEFT;
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
            dsmbvp[dsmixbins.dwMixBinCount].dwMixBin	= DSMIXBIN_FRONT_CENTER;
            dsmbvp[dsmixbins.dwMixBinCount].lVolume		= i_volumes[4];
            dsmixbins.dwMixBinCount++;
		}
		if( dsmixbins.dwMixBinCount > 0 )
		{
			dsmbvp[dsmixbins.dwMixBinCount].dwMixBin	= DSMIXBIN_I3DL2;
			dsmbvp[dsmixbins.dwMixBinCount].lVolume		= DSBVOLUME_MAX;
			dsmixbins.dwMixBinCount++;
		}

		// Set all speaker volumes.
		m_pRenderFilter->SetMixBinVolumes( &dsmixbins );

		// Set overall buffer volume.
		if( dsmixbins.dwMixBinCount > 0 )
		{
			m_pRenderFilter->SetVolume( DSBVOLUME_MAX );
		}
		else
		{
			m_pRenderFilter->SetVolume( DSBVOLUME_MIN );
		}
	}
	else
	{
		m_SetDeferredVolume5Channel	= true;
		m_DeferredVolume[0]			= v0;
		m_DeferredVolume[1]			= v1;
		m_DeferredVolume[2]			= v2;
		m_DeferredVolume[3]			= v3;
		m_DeferredVolume[4]			= v4;
	}
}



//-----------------------------------------------------------------------------
// Name: SetDeferredVolume
// Desc: 
//-----------------------------------------------------------------------------
void CWMAFileStream::SetDeferredVolume( void )
{
	if( m_SetDeferredVolume )
	{
		m_SetDeferredVolume = false;
		SetVolume( m_DeferredVolume[0] );
	}

	if( m_SetDeferredVolumeLR )
	{
		m_SetDeferredVolumeLR = false;
		SetVolume( m_DeferredVolume[0], m_DeferredVolume[1] );
	}

	if( m_SetDeferredVolume5Channel )
	{
		m_SetDeferredVolume5Channel = false;
		SetVolume( m_DeferredVolume[0], m_DeferredVolume[1], m_DeferredVolume[2], m_DeferredVolume[3], m_DeferredVolume[4] );
	}
}



//-----------------------------------------------------------------------------
// Name: IsSafeToDelete
// Desc: 
//-----------------------------------------------------------------------------
bool CWMAFileStream::IsSafeToDelete( void )
{
	return true;
}



//-----------------------------------------------------------------------------
// Name: WMAFileStreamThreadProc
//-----------------------------------------------------------------------------
uint32 WINAPI WMAFileStreamThreadProc( LPVOID lpParameter )
{
	CWMAFileStream*	p_filestream = (CWMAFileStream*)lpParameter;

	Dbg_Assert( p_filestream->m_DecoderCreation == 0 );

	// Create the memory-based XMO.
	HRESULT hr = WmaCreateInMemoryDecoder(	WMAXMediaObjectDataCallback,						// Callback pointer.
											p_filestream,										// Callback context pointer.
											3,													// Yield rate during decoding.
											&p_filestream->m_wfxSourceFormat,					// Source format description.
											(LPXMEDIAOBJECT*)&p_filestream->m_pSourceFilter );	// Pointer to media object.
	if( FAILED( hr ))
	{
		// Signal a failed creation.
		p_filestream->m_DecoderCreation	= 2;
		p_filestream->m_pSourceFilter	= NULL;
	}
	else
	{
		p_filestream->m_DecoderCreation	= 1;
	}

	// Terminate the thread.
	return 0;
}




} // namespace PCM
