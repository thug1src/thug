///////////////////////////////////////////////////////////////////////////////////////
//
// p_AsyncFilesys.cpp		GRJ 11 Oct 2002
//
// Asynchronous file system
//
///////////////////////////////////////////////////////////////////////////////////////

#include <sys/config/config.h>

#include <sys/file/xbox/p_AsyncFilesys.h>

namespace File
{


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
static uint32 RoundToNearestSectorSize( uint32 size )
{
	// Round up to the nearest sector size of a DVD, which is 2048 bytes.
	return ( size + 0x7FF ) & ~0x7FF;
}

	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxAsyncFileHandle::CXboxAsyncFileHandle( void )
{
	m_num_open_requests		= 0;
	mp_non_aligned_buffer	= NULL;
	mp_temp_aligned_buffer	= NULL;
	mh_file					= INVALID_HANDLE_VALUE;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CXboxAsyncFileHandle::~CXboxAsyncFileHandle()
{
}
	

	
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxAsyncFileHandle::plat_open( const char *filename )
{
	m_position = 0;

	// The following code duplicates that in prefopen(). At some point the filenme modification code
	// should probably be abstracted out.

	// Used for prepending the root data directory on filesystem calls.
	char		nameConversionBuffer[256] = "d:\\data\\";
	int			index = 8;
	const char*	p_skip;

	if(( filename[0] == 'c' ) && ( filename[1] == ':' ))
	{
		// Filename is of the form c:\skate4\data\foo...
		p_skip = filename + 15;
	}
	else
	{
		// Filename is of the form foo...
		p_skip = filename;
	}

	while( nameConversionBuffer[index] = *p_skip )
	{
		// Switch forward slash directory separators to the supported backslash.
		if( nameConversionBuffer[index] == '/' )
		{
			nameConversionBuffer[index] = '\\';
		}
		++index;
		++p_skip;
	}
	nameConversionBuffer[index] = 0;

	// If this is a .tex file, switch to a .txx file.
	if((( nameConversionBuffer[index - 1] ) == 'x' ) &&
	   (( nameConversionBuffer[index - 2] ) == 'e' ) &&
	   (( nameConversionBuffer[index - 3] ) == 't' ))
	{
		nameConversionBuffer[index - 2] = 'x';
	}

	// If this is a .pre file, switch to a .prx file.
	if((( nameConversionBuffer[index - 1] ) == 'e' ) &&
	   (( nameConversionBuffer[index - 2] ) == 'r' ) &&
	   (( nameConversionBuffer[index - 3] ) == 'p' ))
	{
		nameConversionBuffer[index - 1] = 'x';
	}

	inc_busy_count();

	mh_file = CreateFile(	nameConversionBuffer,							// File name
							GENERIC_READ,									// Access mode
							FILE_SHARE_READ,								// Share mode
							NULL,											// SD
							OPEN_EXISTING,									// How to create
							FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,	// File attributes
							NULL );											// Handle to template file

	if( mh_file == INVALID_HANDLE_VALUE )
	{
        return false;
	}

	// Immediately obtain the file size.
	m_file_size = ::GetFileSize( mh_file, NULL );
	Dbg_Assert( m_file_size != -1 );

	// Round to nearest sector size, since this will be required for async reads anyway.
	m_file_size = RoundToNearestSectorSize( m_file_size );

	io_callback( m_current_function, (int)mh_file, m_file_size );

	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxAsyncFileHandle::io_callback( EAsyncFunctionType function, int result, uint32 data )
{
	// Do any special function processing
	switch (function)
	{
		case FUNC_OPEN:
		{
			if( result < 0 )
			{
//				Dbg_MsgAssert( 0, ( "Error: Async open returned %d", result ));
			}
			else
			{
				m_file_size = data;
			}
//			ShowAsyncInfo("io_callback: Open returned handle index %d with filesize %d\n", result, data);
			break;
		}

	case FUNC_SEEK:
		if (result < 0)
		{
			Dbg_MsgAssert(0, ("Error: Async seek returned %d", result));
		} else {
			m_position = result;
		}
		break;

	case FUNC_LOAD:
	case FUNC_READ:
		if (mp_temp_aligned_buffer && (result > 0))
		{
			memcpy(mp_non_aligned_buffer, mp_temp_aligned_buffer, result);
		}
		// don't break, continue on below

	case FUNC_WRITE:
		if (result < 0)
		{
			Dbg_MsgAssert(0, ("Error: Async IO returned %d", result));
		} else {
			m_position += result;
			if (mp_temp_aligned_buffer)
			{
				delete mp_temp_aligned_buffer;
				mp_non_aligned_buffer = NULL;
				mp_temp_aligned_buffer = NULL;
			}
		}
		break;

	case FUNC_IDLE:
		// Don't want m_busy to change or user-defined callback to be executed
		return;

	default:
		break;
	}

	// Check to make sure request buffer will be clear
	if (get_busy_count() > 0)
	{
		//Dbg_MsgAssert(m_num_open_requests == 1, ("Sill other open requests"));
	}

	// Now handle the above p-line stuff
	CAsyncFileHandle::io_callback(function, result, data);
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxAsyncFileHandle::add_request_id( int request_id, EAsyncFunctionType function )
{
	if (m_num_open_requests >= NUM_REQUESTS)
	{
		Dbg_MsgAssert(0, ("Too many open requests for file handle"));
		return false;
	}

	m_open_requests[m_num_open_requests].m_request_id = request_id;
	m_open_requests[m_num_open_requests++].m_function = function;

	//scePrintf("Adding request %d to entry #%d for %x with busy %d\n", request_id, (m_num_open_requests-1), this, get_busy_count());

	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
EAsyncFunctionType	CXboxAsyncFileHandle::get_request_function( int request_id ) const
{
	// Find request id in list
	for (int i = 0; i < m_num_open_requests; i++)
	{
		if (m_open_requests[i].m_request_id == request_id)
		{
			return m_open_requests[i].m_function;
		}
	}

	// Request not found
	Dbg_MsgAssert(0, ("Can't find request %d on %x", request_id, this));
	return FUNC_IDLE;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxAsyncFileHandle::clear_request_id( int request_id )
{
	if (m_num_open_requests <= 0)
	{
		Dbg_MsgAssert(0, ("Can't clear request: Open request list empty"));
		return false;
	}

	// Find request id in list
	for (int i = 0; i < m_num_open_requests; i++)
	{
		if (m_open_requests[i].m_request_id == request_id)
		{
			// Just move end of list to here
			m_open_requests[i] = m_open_requests[--m_num_open_requests];
			//scePrintf("Clearing request %d from entry #%d\n", request_id, i);
			return true;
		}
	}

	// Request not found
	Dbg_MsgAssert(0, ("Can't clear request: Not found"));
	return false;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxAsyncFileHandle::plat_init( void )
{
	m_num_open_requests		= 0;
	mp_non_aligned_buffer	= NULL;
	mp_temp_aligned_buffer	= NULL;

	// Set up the overlapped structure.
	m_overlapped.Offset		= 0;
	m_overlapped.OffsetHigh	= 0;
	m_overlapped.hEvent		= NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
volatile bool CXboxAsyncFileHandle::plat_is_done( void )
{
	bool is_done = false;

	if( m_last_result == ERROR_IO_PENDING )
	{
		is_done = HasOverlappedIoCompleted( &m_overlapped );
		if( is_done )
		{
			// I/O operation is complete, so GetOverlappedResult() should retuen true.
			uint32 bt;
			bool result	= GetOverlappedResult(	mh_file,		// handle to file, pipe, or device
												&m_overlapped,	// overlapped structure
												&bt,			// bytes transferred
												false );		// wait option

			Dbg_Assert( result );

			m_last_result = ERROR_SUCCESS;

			// Must call this when we are done.
			io_callback( m_current_function, bt, 0 );	
		}
	}
	return is_done;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
volatile bool CXboxAsyncFileHandle::plat_is_busy( void )
{
	return ( m_last_result == ERROR_IO_PENDING );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxAsyncFileHandle::plat_is_eof( void ) const
{
	return ( m_last_result == ERROR_HANDLE_EOF );
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxAsyncFileHandle::plat_set_priority( int priority )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxAsyncFileHandle::plat_set_stream( bool stream )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxAsyncFileHandle::plat_set_destination( EAsyncMemoryType destination )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxAsyncFileHandle::plat_set_buffer_size( size_t buffer_size )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CXboxAsyncFileHandle::plat_set_blocking( bool block )
{
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
size_t CXboxAsyncFileHandle::plat_load( void *p_buffer )
{
	// This function will just get the file size and do one big read.
	// Set up the overlapped structure.
	m_overlapped.Offset		= 0;
	m_overlapped.OffsetHigh	= 0;
	m_overlapped.hEvent		= NULL;

	// And do the read.
	return plat_read( p_buffer, 1, GetFileSize());
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
size_t CXboxAsyncFileHandle::plat_read( void *p_buffer, size_t size, size_t count )
{
	uint32 total_bytes = RoundToNearestSectorSize( size * count );

	inc_busy_count();

	bool result =  ReadFile(	mh_file,							// Handle to file
								p_buffer,							// Data buffer
								total_bytes,						// Number of bytes to read
								NULL,								// Number of bytes read
								&m_overlapped );					// Overlapped buffer

	// If there was a problem, or the async. operation's still pending... 
	if( !result ) 
	{ 
		// Deal with the error code. 
		m_last_result = GetLastError();
		switch( m_last_result ) 
		{ 
			case ERROR_HANDLE_EOF: 
			{ 
	            // We've reached the end of the file during the call to ReadFile.
				break;
			} 
 
			case ERROR_IO_PENDING: 
			{
				break;
			}

			default:
			{
				Dbg_Assert( 0 );
				break;
			}
		}
	}
	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
size_t CXboxAsyncFileHandle::plat_write( void *p_buffer, size_t size, size_t count )
{
	Dbg_Assert( 0 );
	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
char *CXboxAsyncFileHandle::plat_get_s( char *p_buffer, int maxlen )
{
	Dbg_Assert( 0 );
	return NULL;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
int CXboxAsyncFileHandle::plat_seek( long offset, int origin )
{
	Dbg_Assert( 0 );
	return 0;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CXboxAsyncFileHandle::plat_close( void )
{
	if( mh_file != INVALID_HANDLE_VALUE )
	{
		inc_busy_count();

		CloseHandle( mh_file );
		mh_file = INVALID_HANDLE_VALUE;

		io_callback( m_current_function, 0, 0 );
		return true;
	}
	return false;
}

	
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CAsyncFileLoader::s_plat_init( void )
{
	for( int i = 0; i < MAX_FILE_HANDLES; ++i )
	{
		if( s_file_handles[i] )
		{
			s_file_handles[i]->init();
		}
		else
		{
			s_file_handles[i] = new CXboxAsyncFileHandle;
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CAsyncFileLoader::s_plat_cleanup( void )
{
	for( int i = 0; i < MAX_FILE_HANDLES; ++i )
	{
		if( s_file_handles[i] )
		{
			delete s_file_handles[i];
			s_file_handles[i] = NULL;
		}
	}
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CAsyncFileLoader::s_plat_async_supported( void )
{
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
bool CAsyncFileLoader::s_plat_exist( const char *filename )
{
	HANDLE h_file = CreateFile( filename,							// File name
					0,												// Access mode
					FILE_SHARE_READ,								// Share mode
					NULL,											// SD
					OPEN_EXISTING,									// How to create
					FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,	// File attributes
					NULL );											// Handle to template file

	if( h_file == INVALID_HANDLE_VALUE )
	{
        return false;
	}

	CloseHandle( h_file );
	return true;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
CAsyncFileHandle *CAsyncFileLoader::s_plat_open( const char *filename, int priority )
{
	CXboxAsyncFileHandle *p_handle = new CXboxAsyncFileHandle();
	p_handle->plat_init();

	bool opened = p_handle->plat_open( filename );
	if( !opened )
	{
		delete p_handle;
		return NULL;
	}

	return p_handle;
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CAsyncFileLoader::s_plat_swap_callback_list( void )
{
	s_cur_callback_list_index	= s_cur_callback_list_index ^ 1;	// Swap indices...
	s_new_io_completion			= false;							// ...and clear flag.
}



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void CAsyncFileLoader::s_plat_update( void )
{
	for( int h = 0; h < MAX_FILE_HANDLES; ++h )
	{
		if( s_file_handles[h] )
		{
			if( s_file_handles[h]->IsBusy())
			{
				CXboxAsyncFileHandle*	p_xbox_handle = static_cast<CXboxAsyncFileHandle*>( s_file_handles[h] );
				p_xbox_handle->plat_is_done();
			}
		}
	}
}


}



