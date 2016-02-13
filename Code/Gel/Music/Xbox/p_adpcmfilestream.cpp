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
**	File name:		p_adpcmfilestream.cpp									**
**																			**
**	Created:		01/27/03	-	dc										**
**																			**
**	Description:	Xbox specific .pcm streaming code						**
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
#include "p_adpcmfilestream.h"

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

// Define the maximum amount of packets we will ever submit to the ADPCM renderer
#define ADPCMSTRM_PACKET_COUNT					8

// Define the ADPCM renderer packet size. See the comment block above for an explanation.
// This value is hard-coded assuming an ADPCM frame of 36 samples and 16 bit stereo (128 frames per packet)
#define ADPCMSTRM_16BIT_MONO_PACKET_BYTES		( 1 * 2 * 36 * 128 ) 
#define ADPCMSTRM_16BIT_STEREO_PACKET_BYTES		( 2 * 2 * 36 * 128 ) 

// Read size is 16k (most efficient size for DVD reads).
#define BYTES_PER_READ				16384

	
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

//-----------------------------------------------------------------------------
// Name: CADPCMFileStream()
// Desc: Object constructor.
//-----------------------------------------------------------------------------
CADPCMFileStream::CADPCMFileStream( bool use_3d )
{
    m_pSourceFilter		= NULL;
    m_pRenderFilter		= NULL;
    m_pvSourceBuffer	= NULL;
    m_pFileBuffer		= NULL;
	m_hFile				= INVALID_HANDLE_VALUE;
	m_hThread			= NULL;
	m_bUse3D			= use_3d;
	m_bOkayToPlay		= true;

    for( DWORD i = 0; i < ADPCMSTRM_PACKET_COUNT; i++ )
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
// Name: ~CADPCMFileStream()
// Desc: Object destructor.
//-----------------------------------------------------------------------------
CADPCMFileStream::~CADPCMFileStream()
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
void CADPCMFileStream::AsyncRead( void )
{
	Dbg_Assert( m_hFile != INVALID_HANDLE_VALUE );

    // If paused, do nothing.
	if( m_Paused )
	{
        return;
    }

    // See if the previous read is complete.
	DWORD	dwBytesTransferred;
	BOOL	bIsReadDone = GetOverlappedResult( m_hFile, m_pOverlapped, &dwBytesTransferred, FALSE );
	DWORD	dwLastError = GetLastError();

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

	++m_SuccessiveReads;

	if( dwBytesTransferred < BYTES_PER_READ )
	{ 
		// We've reached the end of the file during the call to ReadFile.

        // Close the file (if not using the global WAD file).
		if( !m_bUseWAD )
		{
			BOOL bSuccess = CloseHandle( m_hFile );
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
		BOOL bComplete	= ReadFile( m_hFile, (BYTE*)m_pFileBuffer + ( m_FileBytesRead % PCMAudio_GetFilestreamBufferSize()), BYTES_PER_READ, NULL, m_pOverlapped );
		dwLastError		= GetLastError();

		// Deal with hitting EOF (for files that are some exact multiple of BYTES_PER_READ bytes).
		if( bComplete || ( !bComplete && ( dwLastError == ERROR_HANDLE_EOF )))
		{
			// Close the file
			if( !m_bUseWAD )
			{
				BOOL bSuccess = CloseHandle( m_hFile );
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
HRESULT CADPCMFileStream::Initialize( HANDLE h_file, unsigned int offset, unsigned int length, void* fileBuffer )
{
    m_dwPercentCompleted = 0;
    
	// At this stage we don't want to create the decoder or the stream. We do want to just allocate the read
	// buffer, and start pulling in the data. Once sufficient data has been grabbed, we can analyze the header
	// and create the required objects for playback.
	m_hFile			= h_file;
	m_bUseWAD		= true;
	m_dwWADOffset	= offset;	
	m_dwWADLength	= length;
	m_pFileBuffer = fileBuffer;

	Dbg_Assert( DWORD( m_pFileBuffer ) % sizeof( DWORD ) == 0 );

	return PostInitialize();
}



//-----------------------------------------------------------------------------
// Name: PostInitialize()
// Desc: Initialisation stuff following the file creation
//-----------------------------------------------------------------------------
HRESULT CADPCMFileStream::PostInitialize( void )
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

	BOOL bComplete = ReadFile( m_hFile, m_pFileBuffer, BYTES_PER_READ, NULL, m_pOverlapped );
	if( !bComplete )
	{
		DWORD dwLastError = GetLastError();
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
// Name: InitializeFormatBlock()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CADPCMFileStream::InitializeFormatBlock( uint32 *p_header_data )
{
	// Ensure the format block is where we expect it to be.
	if( p_header_data[3] == 0x20746D66UL )
	{
		// Read format and number of channels.
		m_wfxExtendedSourceFormat.m_wfxSourceFormat.wFormatTag		= (uint16)( p_header_data[5] & 0xFFFFUL );
		m_wfxExtendedSourceFormat.m_wfxSourceFormat.nChannels		= (uint16)( p_header_data[5] >> 16 );

		// Make sure this is Xbox ADPCM.
		if( m_wfxExtendedSourceFormat.m_wfxSourceFormat.wFormatTag != WAVE_FORMAT_XBOX_ADPCM )
		{
			return E_FAIL;
		}

		// Read samples per second.
		m_wfxExtendedSourceFormat.m_wfxSourceFormat.nSamplesPerSec	= p_header_data[6];

		// Read average bytes per second.
		m_wfxExtendedSourceFormat.m_wfxSourceFormat.nAvgBytesPerSec	= p_header_data[7];

		// Read block alignment and bits per sample.
		m_wfxExtendedSourceFormat.m_wfxSourceFormat.nBlockAlign		= (uint16)( p_header_data[8] & 0xFFFFUL );
		m_wfxExtendedSourceFormat.m_wfxSourceFormat.wBitsPerSample	= (uint16)( p_header_data[8] >> 16 );

		// Extra information.
		m_wfxExtendedSourceFormat.m_wfxSourceFormat.cbSize			= (uint16)( p_header_data[9] & 0xFFFFUL );
		m_wfxExtendedSourceFormat.m_extendedInfo					= (uint16)( p_header_data[9] >> 16 );

		// We have now processed the first 48 bytes of data.
		m_FileBytesProcessed = 48;

		// Now that we know the format, we can set the packet size.
		if( m_wfxExtendedSourceFormat.m_wfxSourceFormat.nChannels == 1 )
			m_PacketBytes = ADPCMSTRM_16BIT_MONO_PACKET_BYTES;
		else
			m_PacketBytes = ADPCMSTRM_16BIT_STEREO_PACKET_BYTES;

		return S_OK;
	}
	return E_FAIL;
}



//-----------------------------------------------------------------------------
// Name: CreateSourceBuffer()
// Desc: 
//-----------------------------------------------------------------------------
bool CADPCMFileStream::CreateSourceBuffer( void )
{
	// Allocate data buffers. The source buffer holds the CPU decompressed packets ready to submit to the stream.
	// The size of the buffer will depend on the format of the stream - stereo requires double the packet size of mono.
	// Explicitly allocate from the bottom up heap.
	Mem::Manager::sHandle().PushContext( Mem::Manager::sHandle().BottomUpHeap());
	m_pvSourceBuffer = new BYTE[m_PacketBytes * ADPCMSTRM_PACKET_COUNT];
	Mem::Manager::sHandle().PopContext();

	return ( m_pvSourceBuffer != NULL );
}



//-----------------------------------------------------------------------------
// Name: PreLoadDone()
// Desc: 
//-----------------------------------------------------------------------------
bool CADPCMFileStream::PreLoadDone( void )
{
	if( m_DecoderCreation == 1 )
	{
		if( m_ReadComplete || ( m_FileBytesRead >= ( m_FileBytesProcessed + m_PacketBytes )))
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
HRESULT CADPCMFileStream::Process( void )
{
    HRESULT			hr;
	DSSTREAMDESC	dssd;
    DWORD			dwPacketIndex;
    
	// Do nothing if waiting to die.
	if( m_AwaitingDeletion )
	{
		return S_OK;
	}

	// Do we need to kick off another read of data?
	// Don't read anymore if we have read ahead to the point where we are within 32k of free read buffer space.
	if( !m_ReadComplete )
	{
		if( m_FileBytesRead <= ( m_FileBytesProcessed + ((int)PCMAudio_GetFilestreamBufferSize() - 32768 )))
		{
			AsyncRead();
		}
	}

	// Has the first block of raw data been read? If so we need to instantiate the playback objects and data buffers.
	if( m_FirstRead && ( m_FileBytesRead > 1024 ))
	{
		if( m_pSourceFilter == NULL )
		{
			// Create the thread which will create the in-memory decoder. 
			m_DecoderCreation = 0;

			// For WMA format, here is where we create the decoder. However, since there is no CPU-side decompression required
			// for ADPCM format, there is no requirement for a decoder.
			m_DecoderCreation	= 1;
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
			m_FirstRead = false;

			// Set up the format information.
			if( InitializeFormatBlock((uint32*)m_pFileBuffer ) == E_FAIL )
			{
				// Failed to decode format information, just mark for deletion.
				m_AwaitingDeletion = true;
				return S_OK;
			}

			// Create the render (DirectSoundStream) filter.
			DSMIXBINS			dsmixbins;
			DSMIXBINVOLUMEPAIR	dsmbvp[DSMIXBIN_ASSIGNMENT_MAX];
			ZeroMemory( &dssd, sizeof( dssd ));

			dssd.dwFlags					= 0;
			dssd.dwMaxAttachedPackets		= ADPCMSTRM_PACKET_COUNT;
			dssd.lpwfxFormat				= &m_wfxExtendedSourceFormat.m_wfxSourceFormat;
			dssd.lpMixBins					= &dsmixbins;

			if( m_bUse3D )
			{
				// This is only designed to be fed a mono signal.
				Dbg_Assert( m_wfxExtendedSourceFormat.m_wfxSourceFormat.nChannels == 1 );

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
				// This is currently disabled.
				if( false && ( XGetAudioFlags() & XC_AUDIO_FLAGS_ENABLE_AC3 ))
				{
					// This is only designed to be fed a stereo signal.
					Dbg_Assert( m_wfxExtendedSourceFormat.m_wfxSourceFormat.nChannels == 2 );

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
					Dbg_Assert( m_wfxExtendedSourceFormat.m_wfxSourceFormat.nChannels == 2 );

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
				m_pRenderFilter->Pause( 1 );
			}
		}
	}

	// We only want to do processing when there is sufficient data available.
	if( m_ReadComplete || ( m_FileBytesRead >= ( m_FileBytesProcessed + m_PacketBytes )))
	{
		if( !m_Completed && m_pRenderFilter )
		{
			// Find a free packet. If there's none free, we don't have anything to do.
			if( FindFreePacket( &dwPacketIndex ))
			{
				// Read from the source filter.
				if( m_bOkayToPlay )
				{
					hr = ProcessSource( dwPacketIndex );
					if( FAILED( hr ))
					{
						return hr;
					}

					if( !m_Paused )
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
	}
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FindFreePacket()
// Desc: Finds a render packet available for processing.
//-----------------------------------------------------------------------------
BOOL CADPCMFileStream::FindFreePacket( DWORD* pdwPacketIndex )
{
    for( DWORD dwPacketIndex = 0; dwPacketIndex < ADPCMSTRM_PACKET_COUNT; ++dwPacketIndex )
    {
        if( XMEDIAPACKET_STATUS_PENDING != m_adwPacketStatus[dwPacketIndex] )
        {
            if( pdwPacketIndex )
			{
                (*pdwPacketIndex) = dwPacketIndex;
			}
            return TRUE;
        }
    }
    return FALSE;
}




//-----------------------------------------------------------------------------
// Name: ProcessSource()
// Desc: Reads data from the source filter.
//-----------------------------------------------------------------------------
HRESULT CADPCMFileStream::ProcessSource( DWORD dwPacketIndex )
{
	if( m_pvSourceBuffer == NULL )
	{
		if( CreateSourceBuffer() == false )
		{
			// Failed to create the source buffer, mark for deletion.
			m_AwaitingDeletion = true;
			return E_FAIL;
		}
	}
    
	// We just want to copy a full packet's worth of data from the file buffer directly into the source buffer...
	uint8*	p_destination		= (BYTE*)m_pvSourceBuffer + ( dwPacketIndex * m_PacketBytes );

	// However we don't want to overrun the file buffer when copying.
	uint32	file_buffer_offset	= m_FileBytesProcessed % PCMAudio_GetFilestreamBufferSize();
	if(( PCMAudio_GetFilestreamBufferSize() - file_buffer_offset ) < (uint32)m_PacketBytes )
	{
		// Copying the data in one chunk will take us beyond the edge of the file buffer.
		// So we need to do the copy in two chunks.
		uint8*	p_source			= (BYTE*)m_pFileBuffer + file_buffer_offset;
		uint32	first_chunk_bytes	= PCMAudio_GetFilestreamBufferSize() - file_buffer_offset;
		CopyMemory( p_destination, p_source, first_chunk_bytes );

		// Wrap file buffer back round to start.
		p_source					= (BYTE*)m_pFileBuffer + 0;
		p_destination				= p_destination + first_chunk_bytes;
		CopyMemory( p_destination, p_source, m_PacketBytes - first_chunk_bytes );
	}
	else
	{
		// Copying the data in one chunk is fine.
		uint8* p_source = (BYTE*)m_pFileBuffer + file_buffer_offset;
		CopyMemory( p_destination, p_source, m_PacketBytes );
	}

	// Now these bytes have been processed.
	m_FileBytesProcessed += m_PacketBytes;

	// If we've caught up with the number of bytes read, it's because we've finished processing.
    if( m_FileBytesProcessed >= m_FileBytesRead )
	{
		// Set completion flag.
		m_Completed = true;

		// Zero remaining part of packet.
		uint32 bytes_to_zero = m_FileBytesProcessed - m_FileBytesRead;
		if( bytes_to_zero > 0 )
		{
			p_destination = (BYTE*)m_pvSourceBuffer + ( dwPacketIndex * m_PacketBytes ) + m_PacketBytes - bytes_to_zero;
			ZeroMemory( p_destination, bytes_to_zero );
		}
	}
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ProcessRenderer()
// Desc: Sends data to the renderer.
//-----------------------------------------------------------------------------
HRESULT CADPCMFileStream::ProcessRenderer( DWORD dwPacketIndex )
{
    XMEDIAPACKET xmp;
    HRESULT      hr;

	Dbg_Assert( m_pvSourceBuffer != NULL );

	// There's a full packet's worth of data ready for us to send to the renderer.  We want to track the status
	// of this packet since the render filter is asychronous and we need to know when the packet is completed.
    ZeroMemory( &xmp, sizeof( xmp ));
    xmp.pvBuffer  = (BYTE*)m_pvSourceBuffer + ( dwPacketIndex * m_PacketBytes );
    xmp.dwMaxSize = m_PacketBytes;
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
void CADPCMFileStream::Pause( DWORD dwPause )
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
void CADPCMFileStream::SetVolume( float volume )
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
void CADPCMFileStream::SetVolume( float volL, float volR )
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
void CADPCMFileStream::SetVolume( float v0, float v1, float v2, float v3, float v4 )
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
void CADPCMFileStream::SetDeferredVolume( void )
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
bool CADPCMFileStream::IsSafeToDelete( void )
{
	return true;
}




/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/


} // namespace PCM
