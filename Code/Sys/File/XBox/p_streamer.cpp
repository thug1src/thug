#include <core/defines.h>
#include <core/crc.h>
#include "p_streamer.h"


//-----------------------------------------------------------------------------
// DVD/HD sector sizes (these will never change)
//-----------------------------------------------------------------------------
#define HD_SECTOR_SIZE	( 512 )
#define DVD_SECTOR_SIZE	( 2 * 1024 )

const char *p_time_string = __TIME__;
const char *p_date_string = __DATE__;

//-----------------------------------------------------------------------------
// Name: CheckDMAAlignment
// Desc: checks the alignment of file offset, read/write size, and 
//       target/source memory for DMA IO
//-----------------------------------------------------------------------------
#ifdef _DEBUG
VOID Check_DMA_Alignment( HANDLE hFile, VOID* pBuffer, DWORD dwOffset, DWORD dwNumBytesIO )
{
    // assert proper target memory alignment
    Dbg_Assert( pBuffer );
    Dbg_Assert( (DWORD(pBuffer) & 0x00000003) == 0 );

     // figure out whether this file is on the DVD or hard disk
    BY_HANDLE_FILE_INFORMATION Info;
    GetFileInformationByHandle( hFile, &Info );
    DWORD dwDVDSerialNumber;
    GetVolumeInformation( "D:\\", NULL, 0, &dwDVDSerialNumber, NULL, NULL, NULL, 0 );

    // assert proper file offset and read/write size
    if( Info.dwVolumeSerialNumber == dwDVDSerialNumber )
    {
        Dbg_Assert( dwOffset % DVD_SECTOR_SIZE == 0 );         
        Dbg_Assert( dwNumBytesIO % DVD_SECTOR_SIZE == 0); 
    }
    else
    {
        Dbg_Assert( dwOffset % HD_SECTOR_SIZE == 0 );       
        Dbg_Assert( dwNumBytesIO % HD_SECTOR_SIZE == 0 ); 
    }
}
#endif


//-----------------------------------------------------------------------------
// Name: CLevelLoader()
// Desc: Constructor
//-----------------------------------------------------------------------------
CLevelLoader::CLevelLoader()
:   m_pSysMemData( NULL ),
    m_dwSysMemSize( 0 ),
    m_pLevels( NULL ),
    m_dwNumLevels( 0 ),
    m_pCurrentLevel( NULL ),
    m_IOState( Idle ),
    m_pSysMemBuffer( NULL ),
    m_pFileSig( NULL ),
    m_hSignature( INVALID_HANDLE_VALUE )
{
    // create file sig buffer
    Dbg_Assert( m_pFileSig == NULL );
    Dbg_Assert( XCALCSIG_SIGNATURE_SIZE + sizeof(SIG_MAGIC) <= HD_SECTOR_SIZE );
	m_pFileSig = new BYTE[HD_SECTOR_SIZE];
}



//-----------------------------------------------------------------------------
// Name: ~CLevelLoader()
// Desc: Destructor
//-----------------------------------------------------------------------------
CLevelLoader::~CLevelLoader()
{
    // Ensure we are not being deleted in the middle of an IO op.
	Dbg_Assert( IsIdle());

    // These should be already be cleaned up.
    Dbg_Assert( m_hSignature == INVALID_HANDLE_VALUE );
    Dbg_Assert( m_pSysMemBuffer == NULL );
    Dbg_Assert( m_pFileSig == NULL );

    // Close all handles.
    for( UINT i = 0; i < m_dwNumLevels; i++ )
    {
        CloseHandle( m_pLevels[i].hDVDFile );
        CloseHandle( m_pLevels[i].hHDFile );
        CloseHandle( m_pLevels[i].hSigFile );
    }

	// We don't own this memory, so don't delete it.
    m_pSysMemData = NULL;

	// Free the level state array.
	delete [] m_pLevels;
	m_pLevels = NULL;
}



//-----------------------------------------------------------------------------
// Name: Initialize
// Desc: Initialize loader
//-----------------------------------------------------------------------------
HRESULT CLevelLoader::Initialize( SLevelDesc* pDescs, DWORD dwNumLevels, BYTE* pSysMemData, DWORD dwSysMemSize, BOOL *p_signal )
{
	// Make sure relevant data directories are on the HD utility section.
	CreateDirectory( "Z:\\data", NULL );
	CreateDirectory( "Z:\\data\\pre", NULL );
	CreateDirectory( "Z:\\data\\music", NULL );
	CreateDirectory( "Z:\\data\\music\\wma", NULL );

	// Copy signal.
	m_pOkayToUseUtilityDrive = p_signal;
	
	// Initialize data.
	Dbg_Assert( pSysMemData );
    m_dwSysMemSize	= dwSysMemSize;
    m_pSysMemData	= pSysMemData;

	// Create level states.
	m_dwNumLevels = dwNumLevels;
	m_pLevels = new SLevelState[m_dwNumLevels];
    
	// Init level states.
    for( UINT i = 0; i < m_dwNumLevels; i++ )
    {
        // Copy level desc.
        m_pLevels[i].Desc = pDescs[i];

		// Clear file handle.
        m_pLevels[i].hDVDFile			= INVALID_HANDLE_VALUE;
        m_pLevels[i].hHDFile			= INVALID_HANDLE_VALUE;
        m_pLevels[i].hSigFile			= INVALID_HANDLE_VALUE;
        m_pLevels[i].bWasPreCached		= FALSE;
        m_pLevels[i].bWasCacheCorrupted	= FALSE;
        m_pLevels[i].dwDVDFileSize		= 0;
    }

	return RefreshLevelStates();
}



//-----------------------------------------------------------------------------
// Name: RefreshLevelStates
// Desc: Refreshes the level loader's level states
//-----------------------------------------------------------------------------
HRESULT CLevelLoader::RefreshLevelStates()
{
    Dbg_Assert( IsIdle());

    // Init states.
    for( UINT i = 0; i < m_dwNumLevels; i++ )
    {   
        m_pLevels[i].bIsPreCached = FALSE;
        m_pLevels[i].bIsCacheCorrupted = FALSE;

        m_pLevels[i].bIsOpen = FALSE;
        m_pLevels[i].dwSysMemSize = 0;
        m_pLevels[i].dwVidMemSize = 0;

        // Close any opened handles.
        CloseHandle( m_pLevels[i].hDVDFile );
        CloseHandle( m_pLevels[i].hSigFile );
        CloseHandle( m_pLevels[i].hHDFile );

        m_pLevels[i].hDVDFile = INVALID_HANDLE_VALUE;
        m_pLevels[i].hHDFile = INVALID_HANDLE_VALUE;
        m_pLevels[i].hSigFile = INVALID_HANDLE_VALUE;

    }
    return S_OK;
}



/******************************************************************/
/* Name: OpenLevel												  */
/* Desc: opens a level											  */
/******************************************************************/
HRESULT CLevelLoader::OpenLevel( SLevelState* pLevel, DWORD dwFlags )
{
	// If the level has never been opened before, open in.
    if( !pLevel->bIsOpen )
    {
        // Check that the level file handles are not open.
        Dbg_Assert( pLevel->hDVDFile == INVALID_HANDLE_VALUE );
        Dbg_Assert( pLevel->hHDFile == INVALID_HANDLE_VALUE );
        Dbg_Assert( pLevel->hSigFile == INVALID_HANDLE_VALUE );

        char szBuffer[MAX_PATH];
        
		// DVD File.
        sprintf( szBuffer, "D:\\data\\%s", pLevel->Desc.szName );

        // Open DVD file for reading.
		pLevel->hDVDFile = CreateFile( szBuffer, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, dwFlags, NULL );

		if( pLevel->hDVDFile == INVALID_HANDLE_VALUE )
			return EndOpenLevel( pLevel, BadOpen );

		// Get size of file on DVD. NOTE: This size is used to help detect corrupt cached levels. If a cached level
		// is not the same size as its DVD counterpart, the cached level is considered corrupt.
        pLevel->dwDVDFileSize = ::GetFileSize( pLevel->hDVDFile, NULL );

		// NOTE: Both files stored on the utility drive (signature file and level) are pre-sized. This prevents the files from
		// becoming fragmented in the FATX.  Also, Overlapped reads/writes to the hard disk are synchronous if the file system
		// has to hit the FATX to determine local->physical cluster mapping.

        // SIG File.
        sprintf( szBuffer, "Z:\\data\\%s.sig", pLevel->Desc.szName );

		// See if we have a cached sig.
        pLevel->bIsPreCached = ( GetFileAttributes( szBuffer ) != DWORD( -1 ));
                
		// Open sig file for reading and writing.
        pLevel->hSigFile = CreateFile( szBuffer,
                                       GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ |
                                       FILE_SHARE_WRITE |
                                       FILE_SHARE_DELETE,
                                       NULL,
                                       OPEN_ALWAYS,
                                       dwFlags,
                                       NULL );
        if( pLevel->hSigFile == INVALID_HANDLE_VALUE )
            return EndOpenLevel( pLevel, BadOpen );
        
		// Cache is corrupted if sig file is not the right size. NOTE: The sig file is HD_SECTOR_SIZE in size. The actual
		// signature calculated by XCalculateSignature* is much smaller that HD_SECTOR_SIZE, but we must write at least
		// HD_SECTOR_SIZE to use DMA on the hard disk.
		pLevel->bIsCacheCorrupted = pLevel->bIsPreCached && ::GetFileSize( pLevel->hSigFile, NULL ) != HD_SECTOR_SIZE;
        
		// If the sig file is corrupted or not saved, resize it.
        if( !pLevel->bIsPreCached || pLevel->bIsCacheCorrupted )
        {
			// Set file size for faster write.
            SetFilePointer( pLevel->hSigFile, HD_SECTOR_SIZE, NULL, FILE_BEGIN );
			SetEndOfFile( pLevel->hSigFile );
            
			// Clear Sig magic number.
            BYTE apyBuffer[HD_SECTOR_SIZE];
            *(DWORD*)(apyBuffer) = ~SIG_MAGIC;
            SetCurrentFile( pLevel->hSigFile );
            if( FAILED( DoIO( Write, apyBuffer, HD_SECTOR_SIZE )))
                return EndOpenLevel( pLevel, BadSigMagicWrite );

			// Wait for IO completion for sig magic writes.
            while( HasIOCompleted() != S_OK );
        }

        
        // CACHED file.
        sprintf( szBuffer, "Z:\\data\\%s", pLevel->Desc.szName );

		// See if we have a cached file.
        pLevel->bIsPreCached = pLevel->bIsPreCached && ( GetFileAttributes( szBuffer ) != DWORD( -1 )); 
        
        // Open cached file for reading and writing.
        pLevel->hHDFile = CreateFile( szBuffer,
                                      GENERIC_WRITE | GENERIC_READ, 
                                      FILE_SHARE_READ |
                                      FILE_SHARE_WRITE |
                                      FILE_SHARE_DELETE,
                                      NULL,
                                      OPEN_ALWAYS,
                                      dwFlags,
                                      NULL );

        if( pLevel->hHDFile == INVALID_HANDLE_VALUE )
            return EndOpenLevel( pLevel, BadOpen );
        
		// Cache is corrupted if cache file is not the right size
		pLevel->bIsCacheCorrupted = pLevel->bIsPreCached && (( ::GetFileSize( pLevel->hHDFile, NULL ) != pLevel->dwDVDFileSize ) || pLevel->bIsCacheCorrupted );

        // If the cache file is corrupted or not saved, resize it.
        if( !pLevel->bIsPreCached || pLevel->bIsCacheCorrupted )
        {
            // Set file size for faster write.
			DWORD rv = SetFilePointer( pLevel->hHDFile, pLevel->dwDVDFileSize, NULL, FILE_BEGIN );
			if( rv == INVALID_SET_FILE_POINTER )
			{
				DWORD last_error = GetLastError();
				printf( "Last error: %x\n", last_error );
			}
			SetEndOfFile( pLevel->hHDFile );
        }
		return EndOpenLevel( pLevel, FilesOpened );
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: AsyncStreamLevel
// Desc: Loads all the resources from the given XPR asynchronously using DMA
//-----------------------------------------------------------------------------
VOID CLevelLoader::AsyncStreamLevel( DWORD dwLevel )
{
    Dbg_Assert( IsIdle() );
    Dbg_Assert( dwLevel < m_dwNumLevels );

	// We are no longer idle.
	m_IOState = Begin;

    // set current level
	m_pCurrentLevel		= &m_pLevels[dwLevel];
    m_dwCurrentLevel	= dwLevel;

	// Start timer.
	m_dStartTime = Tmr::GetTime();
}


//-----------------------------------------------------------------------------
// Name: StreamCurrentBundle()
// Desc: updates the streaming state, returning S_OK when finished
//-----------------------------------------------------------------------------
HRESULT CLevelLoader::StreamCurrentLevel()
{
    HRESULT hr;
  
    switch(m_IOState)
    {
        case Begin:
        {
            // reset IO
            ResetStreaming();

            // if we are reading from the cache, get the signature
            if( IsCurrentCacheGood())
            {
                SetCurrentFile( m_pCurrentLevel->hSigFile );
                if( FAILED( hr = DoIO( Read, m_pFileSig, HD_SECTOR_SIZE )))
                    return EndStreamLevel( BadRead );
            }
            m_IOState = LoadSig;
        }

        case LoadSig:
        {
            // wait for previous IO to complete
            if( FAILED( hr = HasIOCompleted()))
                return EndStreamLevel( hr == E_PENDING ? Pending : BadRead );
            
            // Reading cached file, so a signature exists
            if( IsCurrentCacheGood())
            {
				// Look for sig magic number.
				DWORD dwSig;
				dwSig = *(DWORD*)( m_pFileSig );
				if( dwSig != SIG_MAGIC )
                {
                    if( dwSig == ~SIG_MAGIC )
                        return EndStreamLevel( NoSigMagic );
                    else
                        return EndStreamLevel( BadSig );
                }
				
				// Check date and time.            
				dwSig = *(((DWORD*)m_pFileSig ) + 1 );
				if( dwSig != Crc::GenerateCRCFromString( p_date_string ))
                {
					return EndStreamLevel( BadSig );
                }
				dwSig = *(((DWORD*)m_pFileSig ) + 2 );
				if( dwSig != Crc::GenerateCRCFromString( p_time_string ))
                {
					return EndStreamLevel( BadSig );
                }
			}
            else
			{
                // Set sig magic number so it is written out.
                *(((DWORD*)m_pFileSig )	+ 0 )	= SIG_MAGIC;
                *(((DWORD*)m_pFileSig ) + 1 )	= Crc::GenerateCRCFromString( p_date_string );
                *(((DWORD*)m_pFileSig ) + 2 )	= Crc::GenerateCRCFromString( p_time_string );
			}

            m_IOState = LoadSysMem;
        }

        case LoadSysMem:
        {
            // Wait for previous IO to complete.
            if( FAILED( hr = HasIOCompleted()))
                return EndStreamLevel( hr == E_PENDING ? Pending : BadRead );
        
            // Read initial system memory buffer.
            if( IsCurrentCacheGood())
			{
				// At this point we have decided the sig file is fine, which indicates that the cached file is fine also.
				// As such, no further work required for this file.
//				SetCurrentFile( m_pCurrentLevel->hHDFile );
				m_pCurrentLevel->bIsPreCached = TRUE;
				m_pCurrentLevel->bIsCacheCorrupted = FALSE;
				return EndStreamLevel( Finished );
			}
            else
			{
				// Set file pointer to start of HD file.
                SetCurrentFile( m_pCurrentLevel->hHDFile );
				
				// Set file pointer to start of DVD file.
                SetCurrentFile( m_pCurrentLevel->hDVDFile );
			}

			// At this point we want to loop through reading the buffer and writing it until teh file is fully written.
			DWORD	total_bytes_read		= 0;
			DWORD	total_bytes_written		= 0;
			int		bytes_remaining			= m_pCurrentLevel->dwDVDFileSize;
			for( ;; )
			{
				// Set the DVD file, but don't reset the file pointer.
                SetCurrentFile( m_pCurrentLevel->hDVDFile, FALSE );

//				DWORD bytes_to_transfer = ( bytes_remaining >= m_dwSysMemSize ) ? m_dwSysMemSize : bytes_remaining;
				DWORD bytes_to_transfer = ( bytes_remaining >= (int)m_dwSysMemSize ) ? m_dwSysMemSize : m_dwSysMemSize;
				
				if( FAILED( DoIO( Read, m_pSysMemData, bytes_to_transfer )))
	                return EndStreamLevel( BadRead );

				total_bytes_read += bytes_to_transfer;

				// Set the HD file, but don't reset the file pointer.
				SetCurrentFile( m_pCurrentLevel->hHDFile, FALSE );

				if( FAILED( DoIO( Write, m_pSysMemData, bytes_to_transfer )))
	                return EndStreamLevel( BadWrite );
				
				total_bytes_written += bytes_to_transfer;
				bytes_remaining -= bytes_to_transfer;
				
				if( bytes_remaining <= 0 )
				{
					DWORD rv = SetFilePointer( m_pCurrentLevel->hHDFile, m_pCurrentLevel->dwDVDFileSize, NULL, FILE_BEGIN );
					if( rv == INVALID_SET_FILE_POINTER )
					{
						DWORD last_error = GetLastError();
						printf( "Last error: %x\n", last_error );
					}
					
					SetEndOfFile( m_pCurrentLevel->hHDFile );
					
					m_IOState = WriteSig;
					break;
				}
			}
        }

        case WriteSig:
        {
            // Wait for previous IO to complete.
            if( FAILED( hr = HasIOCompleted() ) )
                return EndStreamLevel( hr == E_PENDING ? Pending : BadRead );
    
			// We are now loaded.
			m_dLoadTime = Tmr::GetTime() - m_dStartTime;

            // We write the sig file last. If the box is turned off during a cache operation, the sig file will be missing.
            // If the box is turned off during the write of the sig file, it won't have a header or will not match the level
            // data. In either case, the level will be re-cached.
            SetCurrentFile( m_pCurrentLevel->hSigFile );
            if( FAILED( hr = DoIO( Write, m_pFileSig, HD_SECTOR_SIZE) ) )
                return EndStreamLevel( BadWrite );

            m_IOState = End;
        }

        case End:
        {
            // Wait for previous IO to complete.
            if( FAILED( hr = HasIOCompleted() ) )
                return EndStreamLevel( hr == E_PENDING ? Pending : BadRead );
    
			// Record cache time.
			m_dCacheTime = Tmr::GetTime() - m_dStartTime;

			// We are now cached.
            m_pCurrentLevel->bIsPreCached		= TRUE;
            m_pCurrentLevel->bIsCacheCorrupted	= FALSE;

            return EndStreamLevel( Finished );
        }
    }

    // should never reach here
    Dbg_Assert( FALSE );
    return E_FAIL;
}



//-----------------------------------------------------------------------------
// Name: EndOpenLevel()
// Desc: called when IO has completed or there is an error
//-----------------------------------------------------------------------------
HRESULT CLevelLoader::EndOpenLevel( SLevelState* pLevel, FileOpenStatus Status)
{
    Dbg_Assert(pLevel);

    if(Status == BadOpen || Status == BadSigMagicWrite)
    {
        // close all file handles
        CloseHandle( pLevel->hDVDFile );
        CloseHandle( pLevel->hHDFile );
        CloseHandle( pLevel->hSigFile );
        pLevel->hDVDFile = INVALID_HANDLE_VALUE;
        pLevel->hHDFile = INVALID_HANDLE_VALUE;
        pLevel->hSigFile = INVALID_HANDLE_VALUE;

        return E_FAIL;
    }

    // level is now opened
    pLevel->bIsOpen = TRUE;

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: EndStreamLevel()
// Desc: called when IO has completed or there is an error
//-----------------------------------------------------------------------------
HRESULT CLevelLoader::EndStreamLevel( FileIOStatus Status )
{
    // if IO is not pending, we are finished
    if(Status != Pending)
    {
        // close the signature if needed
        if( m_hSignature != INVALID_HANDLE_VALUE )
        {
            XCalculateSignatureEnd( m_hSignature, NULL );
            m_hSignature = INVALID_HANDLE_VALUE;
        }
    }

    // determine new state
    switch(Status)
    {
        case BadRead:
        case BadHeader:
        case BadSig:
        case SigMismatch:
            m_pCurrentLevel->bIsCacheCorrupted = TRUE;
            m_IOState = Begin; // retry streaming the level
            return E_FAIL;
        case NoSigMagic:
            m_pCurrentLevel->bIsPreCached = FALSE;
            m_IOState = Begin; // retry streaming the level
            return E_FAIL;
        case BadWrite:  // if we have a bad write, finish anyways since the cache will be corrupt next time around
            m_pCurrentLevel->bIsCacheCorrupted = TRUE;
        case Finished:
            m_IOState = Idle;  // done with IO, so we are now idle

			// close all file handles
			CloseHandle( m_pCurrentLevel->hDVDFile );
			CloseHandle( m_pCurrentLevel->hHDFile );
			CloseHandle( m_pCurrentLevel->hSigFile );
			m_pCurrentLevel->hDVDFile = INVALID_HANDLE_VALUE;
			m_pCurrentLevel->hHDFile = INVALID_HANDLE_VALUE;
			m_pCurrentLevel->hSigFile = INVALID_HANDLE_VALUE;
			
			return S_OK;
        case Pending:
            return E_PENDING;
    }

    // should never reach here
    Dbg_Assert( FALSE );
    return E_FAIL;
}


//-----------------------------------------------------------------------------
// Name: CRestIO()
// Desc: Records last state for current level and get ready for IO
//-----------------------------------------------------------------------------
VOID CLevelLoader::ResetStreaming()
{
    // record last state
    m_pCurrentLevel->bWasPreCached = m_pCurrentLevel->bIsPreCached;
    m_pCurrentLevel->bWasCacheCorrupted = m_pCurrentLevel->bIsCacheCorrupted;

    // begin signature
    Dbg_Assert( m_hSignature == INVALID_HANDLE_VALUE );
    m_hSignature = XCalculateSignatureBegin( XCALCSIG_FLAG_NON_ROAMABLE );
    Dbg_Assert( m_hSignature != INVALID_HANDLE_VALUE );

    // create file sig buffer
//    Dbg_Assert( m_pFileSig == NULL );
//    Dbg_Assert( XCALCSIG_SIGNATURE_SIZE + sizeof(SIG_MAGIC) <= HD_SECTOR_SIZE );
//    m_pFileSig = new BYTE[HD_SECTOR_SIZE];
}







//-----------------------------------------------------------------------------
// Name: IOProc
// Desc: Thread proc for IO
//-----------------------------------------------------------------------------
DWORD WINAPI IOProc( LPVOID lpParameter )
{
    CThreadedLevelLoader* pLoader = (CThreadedLevelLoader*)lpParameter;
    Dbg_Assert( pLoader );
   
	// Wait for the IO event to be signaled.
	WaitForSingleObject( pLoader->m_hEvent, INFINITE );
	
	for( ;; )
    {
        
        // NOTE: In the threaded version of the loader, we are able to open
        //       files and set file sizes mid-game without noticing a severe
        //       frame "glitch."  If you use overlapped IO, you must remember
        //       that OpenFile, SetEndOfFile, etc. are blocking

        // open the current level
        // If we can't open files, we are in trouble, so in the release build, we just keep trying
//        while( FAILED( pLoader->OpenLevel( pLoader->m_pCurrentLevel, FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN )))
        while( FAILED( pLoader->OpenLevel( pLoader->m_pCurrentLevel, FILE_FLAG_SEQUENTIAL_SCAN )))
        {
            // break in the debug build to find out what the problem is
            Dbg_Assert( FALSE );
        }
    
        // Keep trying to load and cache the level until we are successful.
        while( FAILED( pLoader->StreamCurrentLevel()));

		// Level loaded and cached.
		++pLoader->m_dwCurrentLevel;
		if( pLoader->m_dwCurrentLevel < pLoader->m_dwNumLevels )
		{
			pLoader->m_pCurrentLevel = &pLoader->m_pLevels[pLoader->m_dwCurrentLevel];
			pLoader->m_IOState = CThreadedLevelLoader::Begin;
		}
		else
		{
			break;
		}
	
	}

	// If the loaded has closed the thread we are finished. Signal the boolean value passed in.
	*( pLoader->m_pOkayToUseUtilityDrive ) = true;

	ExitThread( 0 );
}



//-----------------------------------------------------------------------------
// Name: CThreadedLevelLoader()
// Desc: Constructor
//-----------------------------------------------------------------------------
CThreadedLevelLoader::CThreadedLevelLoader() : m_bKillThread( FALSE )
{
    // Create event that is used to block the thread when it is not being used for IO.
    m_hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    // Create IO thread.
    m_hThread =  CreateThread( NULL, 0, IOProc, this, 0, NULL);     

	SetCurrentFile( INVALID_HANDLE_VALUE );
}



//-----------------------------------------------------------------------------
// Name: ~CThreadedLevelLoader()
// Desc: Destructor
//-----------------------------------------------------------------------------
CThreadedLevelLoader::~CThreadedLevelLoader()
{
    Dbg_Assert( IsIdle() );

    // Kill the thread.

    // signal thread 
    m_bKillThread = TRUE;
    SetEvent( m_hEvent );
    
    // wait for thread to exit
    DWORD dwExitStatus;
    do
    {
        SwitchToThread();  // try to get the thread to execute
        GetExitCodeThread( m_hThread, &dwExitStatus );
    }
    while( dwExitStatus == STILL_ACTIVE );

    // close handle to thread and event
    CloseHandle( m_hThread );
    CloseHandle( m_hEvent );
}



//-----------------------------------------------------------------------------
// Name: DoIO
// Desc: wrapper for ::ReadFile and ::WriteFile that checks parameters
//-----------------------------------------------------------------------------
HRESULT CThreadedLevelLoader::DoIO( IOType Type, VOID* pBuffer, DWORD dwNumBytes )
{
    // Check DMA alignment
#	ifdef _DEBUG
    Check_DMA_Alignment( m_hFile, pBuffer, SetFilePointer( m_hFile, 0, NULL, FILE_CURRENT ), dwNumBytes );
#	endif

    // Do IO
    DWORD dwNumBytesIO;
    BOOL bSuccess;
    if(Type == Read)
    {
        bSuccess = ::ReadFile( m_hFile, pBuffer, dwNumBytes, &dwNumBytesIO, NULL );
    }
    else
        bSuccess = ::WriteFile( m_hFile, pBuffer, dwNumBytes, &dwNumBytesIO, NULL );

	// If IO is successful and we have the number of expected bytes, return success.
    // NOTE: If the file size is X*SECTORSIZE + Y where Y is less than SECTORSIZE, we still request SECTORSIZE bytes when reading Y
    // in order to get DMA. The file system will return success, and the number of bytes read/written is Y.
    if( !bSuccess || ( dwNumBytesIO != dwNumBytes && SetFilePointer( m_hFile, 0, NULL, FILE_CURRENT ) != GetFileSize( m_hFile, NULL )))
//	if( !bSuccess )
        return E_FAIL;
        
    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: SetCurrentFile
// Desc: Sets the current file to be used during IO
//-----------------------------------------------------------------------------
VOID CThreadedLevelLoader::SetCurrentFile( HANDLE hFile, BOOL to_start )
{
	if(( hFile != INVALID_HANDLE_VALUE ) && to_start )
	{
		// Reset file pointer to beginning.
		SetFilePointer( hFile, 0, NULL, FILE_BEGIN );
	}
    m_hFile = hFile;
}



//-----------------------------------------------------------------------------
// Name: AsyncStreamLevel
// Desc: Begins streaming of the level.  returns immediately
//-----------------------------------------------------------------------------
VOID CThreadedLevelLoader::AsyncStreamLevel( DWORD dwLevel )
{
    CLevelLoader::AsyncStreamLevel( dwLevel );

	// Signal thread to start IO.
	SetEvent( m_hEvent );
}
