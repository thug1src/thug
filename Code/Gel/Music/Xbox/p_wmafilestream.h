/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GEL (Game Engine Library)								**
**																			**
**	Module:			Game Engine												**
**																			**
**	File name:		p_wmafilesteam.h										**
**																			**
**	Created: 		01/27/2003	-	dc										**
**																			**
*****************************************************************************/

#ifndef __P_WMAFILESTREAM_H
#define __P_WMAFILESTREAM_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

// Define the maximum amount of packets we will ever submit to the renderer.
#define WMASTRM_PACKET_COUNT		8

namespace Pcm
{

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

/******************************************************************/
/*																  */
/* WMA file streaming object, designed for fully asynchronous	  */
/* streaming from disc.											  */
/* Note that WMA decompression does incur a CPU overhead.		  */
/*																  */
/******************************************************************/
class CWMAFileStream : public Spt::Class
{
	private:

	HANDLE				m_hFile;
	HANDLE				m_hThread;
	OVERLAPPED*			m_pOverlapped;							// OVERLAPPED structure for asynchronous file access.
	bool				m_FirstRead;							// Flag to indicate first request for more data to be streamed from disc.
	bool				m_ReadComplete;
	int					m_SuccessiveReads;						// Counts how many read operations performed in total.
	bool				m_bUseWAD;
	bool				m_bUse3D;								// Sets whether 2D or 3D mixbins are set up.
	uint32				m_dwWADOffset;	
	uint32				m_dwWADLength;

	public:

    IDirectSoundStream* m_pRenderFilter;                         // Render (DirectSoundStream) filter
    LPVOID              m_pvSourceBuffer;                        // Source filter data buffer
    LPVOID              m_pvRenderBuffer;                        // Render filter data buffer
    uint32				m_adwPacketStatus[WMASTRM_PACKET_COUNT]; // Packet status array
    uint32				m_dwStartingDataOffset;                  // Offset into wma file where data begins.
    uint32				m_dwPercentCompleted;					 // Percent completed
	bool				m_Paused;
	bool				m_Completed;							// For single-shot, indicates loading of final packet to renderer is completed
    uint32				m_LastPacket;							// Last packet array index
	bool				m_SetDeferredVolume;
	bool				m_SetDeferredVolumeLR;
	bool				m_SetDeferredVolume5Channel;
	bool				m_bOkayToPlay;							// Used when exact syncing is required - will load buffers but won't start decompression until this flag is true.
	float				m_DeferredVolume[5];
	uint32				m_Mixbins;								// Bitfield indicating which mixbins are active for this buffer.
	uint32				m_NumMixbins;

    // Packet processing
    bool				FindFreePacket( uint32* pdwPacketIndex );
	bool				FindRenderablePacket( uint32* pdwPacketIndex );
    HRESULT				ProcessSource( uint32 dwPacketIndex );
    HRESULT				ProcessRenderer( uint32 dwPacketIndex );

	WAVEFORMATEX		m_wfxSourceFormat;
    XFileMediaObject*   m_pSourceFilter;							// Source (wave file) filter
	LPVOID				m_pFileBuffer;								// Buffer for async read of raw data.
	int					m_FileBytesRead;							// Total number of raw bytes read from disc.
	int					m_FileBytesProcessed;						// Total number of raw bytes processed by XMO.
	int					m_DecoderCreation;
	bool				m_AwaitingDeletion;

    // Processing
    HRESULT				Process();

    // Initialization
    HRESULT				Initialize( HANDLE h_file, unsigned int offset, unsigned int length, void* fileBuffer );
    HRESULT				Initialize( HANDLE h_file, void* fileBuffer );
	HRESULT				PostInitialize( void );
	void				CreateSourceBuffer( void );
    
    // Play control
    void				Pause( uint32 dwPause );
	void				SetVolume( float volume );
	void				SetVolume( float volumeL, float volumeR );
	void				SetVolume( float v0, float v1, float v2, float v3, float v4 );
	void				SetDeferredVolume( void );

	// Query
	IDirectSoundStream*	GetSoundStream( void )					{ return m_pRenderFilter; }
	bool				IsCompleted( void )						{ return ( m_Completed && ( XMEDIAPACKET_STATUS_PENDING != m_adwPacketStatus[m_LastPacket] )); }
	bool				IsSafeToDelete( void );
	bool				PreLoadDone( void );

	// Asynchronous stuff.
	void				AsyncRead( void );


    CWMAFileStream( bool use_3d = false );
    ~CWMAFileStream();
};



/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/
	
	
/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/


/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Pcm

#endif	// __P_WMAFILESTREAM_H




